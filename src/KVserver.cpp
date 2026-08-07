#include "KVserver.hpp"

using Clock =
    std::chrono::steady_clock;
    
 std::uint64_t to_ns(Clock::duration duration) noexcept
{
    const auto value =
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(duration).count();

    return static_cast<std::uint64_t>(value);
}

void KVserver::run()
{
    LOG_INFO(
        "[SERVER] event loop started"
    );

    constexpr int MAX_EVENTS = 64;
    constexpr int EPOLL_TIMEOUT_MS = 1000;

    std::array<epoll_event, MAX_EVENTS> ready_events{};

    while (true) {
        const int ready_count = ::epoll_wait(
            epoll_fd_,
            ready_events.data(),
            static_cast<int>(ready_events.size()),
            EPOLL_TIMEOUT_MS
        );

        if (ready_count == -1) {
            if (errno == EINTR) {
                continue;
            }

            throw std::runtime_error(
                std::string("epoll_wait failed: ") +
                std::strerror(errno)
            );
        }

        closeTimedOutConnections();

        for (int i = 0; i < ready_count; ++i) {
            const int fd =
                ready_events[i].data.fd;

            const std::uint32_t events =
                ready_events[i].events;

            const bool is_listener =
                listen_fds_.find(fd) !=
                listen_fds_.end();

            if (is_listener) {
                handleAccept(fd);
                continue;
            }

            handleClientEvent(fd, events);
        }
    }
}

 void KVserver::handleAccept( int eventfd ){

    while(true){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        
        int clientsock= accept(eventfd,(struct sockaddr*)&client, &len);//建立一个clientsock的代理，认为这个代理在读事件与写事件中都会起到作用
        
        if(clientsock <0){
            if(errno==EAGAIN || errno ==EWOULDBLOCK){
                break;
            }else{
                perror("accept");
                break;
            }
        }
        setnonblocking(clientsock); //无论是读写都是非阻塞

        conns_.emplace(clientsock, clientsock);
        
        epoll_event cev;
        cev.data.fd=clientsock;
        cev.events=EPOLLIN|EPOLLET;//似乎只接受读事件
        epoll_ctl(epoll_fd_,EPOLL_CTL_ADD,clientsock,&cev);
        LOG_INFO("[ACCEPT] fd=%d", clientsock);
    }

    }

void KVserver::handleRead( int eventfd){
        auto it = conns_.find(eventfd);
    if (it == conns_.end()) {
        return;
    }

    ConnState& conn = it->second;
    char buf[4096];

    // -------------------------
    // read 阶段
    // -------------------------
    const auto read_begin = Clock::now();

    while (true) {
        const ssize_t n =
            recv(eventfd, buf, sizeof(buf), 0);

            /*LOG_DEBUG(
            "[READ] fd=%d bytes=%ld",
                eventfd,n);*/

        if (n > 0) {
            conn.inbuf.append(
                buf,
                static_cast<std::size_t>(n)
            );

            conn.last_active = time(nullptr);
            continue;
        }

        if (n == 0) {
            close_conn(
                eventfd,
                "peer closed"
            );
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK) {
            break;
        }

        close_conn(
            eventfd,
            "read error"
        );
        return;
    }

    stats_.read_time_ns +=
        to_ns(Clock::now() - read_begin);

    ++stats_.read_event_count;

    // -------------------------
    // 处理完整命令
    // -------------------------
    while (true) {
        auto line = conn.pop_command_line();

        if (!line) {
            // 这是正常情况，不是错误，不要打印
            break;
        }

        //测量epoll耗时
        if (!stats_.benchmark_started) {
            stats_.benchmark_started = true;
        }


        // parse
        const auto parse_begin = Clock::now();

        Command cmd = parser_.parse(*line);
        
        /*LOG_DEBUG(
            "[PARSE] fd=%d input=%s",
            eventfd,
            line.c_str()
        );*/

        stats_.parse_time_ns +=
            to_ns(Clock::now() - parse_begin);
        ++stats_.parse_count;

        // AOF 不要算进 parse
        if (cmd.type == CommandType::SET ||
            cmd.type == CommandType::DEL) {
            aoflog_.append(*line);
        }

        // execute
        const auto execute_begin = Clock::now();
        std::string response =
            executor_.execute(cmd);

        stats_.execute_time_ns +=
            to_ns(Clock::now() - execute_begin);

        conn.outbuf += response;
        ++stats_.execute_count;

    }

    // -------------------------
    // epoll_ctl 阶段
    // -------------------------
    if (!conn.outbuf.empty()) {
        const auto write_begin = Clock::now();
        WriteResult result = conn.write_response();

        stats_.write_time_ns +=
            to_ns(Clock::now() - write_begin);

        ++stats_.write_attempt_count;

        if (result == WRITE_AGAIN) {
            // socket 发送缓冲区暂时满了，
            // 这时才让 epoll 等待 EPOLLOUT。
            const auto ctl_begin = Clock::now();

            enable_write_event(eventfd);

            stats_.epoll_ctl_time_ns +=
                to_ns(Clock::now() - ctl_begin);

            ++stats_.epoll_ctl_count;
        }
        else if (result == WRITE_ERROR) {
            close_conn(
                eventfd,
                "write error"
            );

            return;
        }
        else if (result == WRITE_DONE) {
            // 响应已经在 handle_read 中全部发送完。
            // 不需要启用 EPOLLOUT。
        }
    }
}

    

void KVserver::handleWrite(int eventfd){
     auto it = conns_.find(eventfd);

    if (it == conns_.end()) {
        return;
    }

    ConnState& conn = it->second;

    const auto write_begin = Clock::now();

    WriteResult result =
        conn.write_response();

    stats_.write_time_ns +=
        to_ns(Clock::now() - write_begin);

    ++stats_.write_event_count;

    if (result == WRITE_AGAIN) {
        // 如果本来已经监听 EPOLLOUT，
        // 这里不一定需要再次 epoll_ctl。
        return;
    }

    if (result == WRITE_ERROR) {
        close_conn(
            eventfd,
            "write error"
        );
        return;
    }

    if (result == WRITE_DONE) {
        const auto ctl_begin = Clock::now();

        disable_write_event(eventfd);

        stats_.epoll_ctl_time_ns +=
            to_ns(Clock::now() - ctl_begin);

        ++stats_.epoll_ctl_count;
    }

    }

void KVserver::handleClientEvent(
    int fd,
    std::uint32_t events
)
{
    if (events & (EPOLLERR | EPOLLHUP)) {
        close_conn(fd, "epoll error or hangup");
        return;
    }

#ifdef EPOLLRDHUP
    if (events & EPOLLRDHUP) {
        close_conn(fd, "peer half-closed");
        return;
    }
#endif

    if (events & EPOLLIN) {
        handleRead(fd);

        // handleRead 可能已经关闭并删除了连接。
        if (conns_.find(fd) == conns_.end()) {
            return;
        }
    }

    if (events & EPOLLOUT) {
        handleWrite(fd);
    }
}

void KVserver::closeTimedOutConnections()
{
    constexpr std::time_t CONNECTION_TIMEOUT_SECONDS = 18;

    const std::time_t now = std::time(nullptr);

    std::vector<int> timeout_fds;
    timeout_fds.reserve(conns_.size());

    for (const auto& [fd, conn] : conns_) {
        if (now - conn.last_active >
            CONNECTION_TIMEOUT_SECONDS) {
            timeout_fds.push_back(fd);
        }
    }

    for (int fd : timeout_fds) {
        close_conn(fd, "out of time");
    }
}


bool KVserver::addListener(std::uint16_t port){
{
  
    const int listen_fd =
        initserver(static_cast<int>(port));

          LOG_INFO(
        "[LISTEN] fd=%d port=%d",
        listen_fd,
        port
    );

    if (listen_fd < 0) {
        return false;
    }

    if (setnonblocking(listen_fd) == -1) {
        ::close(listen_fd);
        return false;
    }

    epoll_event event{};
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLET;

    if (::epoll_ctl(
            epoll_fd_,
            EPOLL_CTL_ADD,
            listen_fd,
            &event
        ) == -1) {
        ::close(listen_fd);
        return false;
    }

    listen_fds_.insert(listen_fd);
    return true;
}
}

bool KVserver::recoverFromAof(){
     
    return aoflog_.replay(
        store_,
        parser_,
        executor_
    );

}


void KVserver::close_conn(
    int fd,
    const char* reason)
{
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    conns_.erase(fd);

    LOG_INFO("[CLOSE] fd=%d reason=%s", fd, reason);

}

void KVserver::enable_write_event(int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);

}

void KVserver::disable_write_event(int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);

    LOG_DEBUG("[DISABLE_WRITE] fd=%d", fd);
}

int KVserver::setnonblocking(int fd) // 估计读事件，写事件都要非阻塞
{
  int flags;
  // 获取fd的状态。
  if ((flags=fcntl(fd,F_GETFL,0))==-1)
    flags = 0;
  return fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

int KVserver::initserver(int port) // 规定socket协议，建立sock,使用更好用的servaddr的填充基本信息，比如协议族，传输协议，端口
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket() failed"); return -1; }
 
    int opt = 1; 
    unsigned int len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
 
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
 
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    { perror("bind() failed"); close(sock); return -1; }
 
    if (listen(sock, 5) != 0 )
    { perror("listen() failed"); close(sock); return -1; } //实现绑定端口并且监听
 
    return sock;
}