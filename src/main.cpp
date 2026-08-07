
#include "KVserver.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr
            << "usage: ./kvserver <port> [port ...]\n";

        return 1;
    }

    try {
        KVserver server("append.aof");

        for (int i = 1; i < argc; ++i) {
            const int port = std::stoi(argv[i]);

            if (port < 1 || port > 65535) {
                std::cerr
                    << "invalid port: "
                    << argv[i]
                    << '\n';

                return 1;
            }

            if (!server.addListener(
                    static_cast<std::uint16_t>(port)
                )) {
                std::cerr
                    << "failed to listen on port "
                    << port
                    << '\n';

                return 1;
            }
        }

        if (!server.recoverFromAof()) {
            std::cerr << "AOF replay failed\n";
            return 1;
        }

        server.run();
    }
    catch (const std::exception& error) {
        std::cerr
            << "fatal: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}

//该版本为ET加非阻塞
//int initserver(int port);

/*void disable_write_event(int eventfd, int epollfd);
 
void enable_write_event(int eventfd, int epollfd);
 
void handle_accept(  // 建立连接并且实现监听
    int listensock,    //服务器用来执行监听功能的句柄
    int epollfd,        //监听listensock的epoll句柄
    std::unordered_map<int, ConnState>& conns // listensock的状态
);

void handle_read(
    int eventfd,//在服务器端用来执行读事件的句柄
    int epollfd,//用来监听的读事件句柄的，不知道在ET情况下epoll会如何处理，估计运行良好的前提是每次读事件都读到没有字节
    std::unordered_map<int, ConnState>& conns, // 读事件对应句柄的状态
    Parser& parser,
    Executor& executor,
     AOFLog& aoflog,
      PerfStats& stats
);

void handle_write(
    int eventfd,//在服务器端用来执行写事件的句柄
    int epollfd,//不知道如何使用epoll来处理
    std::unordered_map<int, ConnState>& conns,//写事件对应句柄的状态
     PerfStats& stats
);

int setnonblocking(int fd) // 估计读事件，写事件都要非阻塞
{
  int flags;
  // 获取fd的状态。
  if ((flags=fcntl(fd,F_GETFL,0))==-1)
    flags = 0;
  return fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

void close_conn(
    int fd,
    int epollfd,             
    std::unordered_map<int, ConnState>& conns ,
     const char* reason
);*/

/*int initserver(int port) // 规定socket协议，建立sock,使用更好用的servaddr的填充基本信息，比如协议族，传输协议，端口
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

/*
void handle_accept( int listensock,int epollfd, std::unordered_map<int, ConnState>& conns){
    while(true){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        
        int clientsock= accept(listensock,(struct sockaddr*)&client, &len);//建立一个clientsock的代理，认为这个代理在读事件与写事件中都会起到作用
        
        if(clientsock <0){
            if(errno==EAGAIN || errno ==EWOULDBLOCK){
                break;
            }else{
                perror("accept");
                break;
            }
        }
        setnonblocking(clientsock); //无论是读写都是非阻塞

        conns.emplace(clientsock, clientsock);
        
        epoll_event cev;
        cev.data.fd=clientsock;
        cev.events=EPOLLIN|EPOLLET;//似乎只接受读事件
        epoll_ctl(epollfd,EPOLL_CTL_ADD,clientsock,&cev);
        
        LOG_INFO("[ACCEPT] fd=%d", clientsock);
    }
}

void handle_read(
    int eventfd,
    int epollfd,
    std::unordered_map<int, ConnState>& conns,
    Parser& parser,
    Executor& executor,
    AOFLog& aoflog,
    PerfStats& stats
)
{
    auto it = conns.find(eventfd);
    if (it == conns.end()) {
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
                epollfd,
                conns,
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
            epollfd,
            conns,
            "read error"
        );
        return;
    }

    stats.read_time_ns +=
        to_ns(Clock::now() - read_begin);

    ++stats.read_event_count;

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
        if (!stats.benchmark_started) {
            stats.benchmark_started = true;
        }

        // parse
        const auto parse_begin = Clock::now();

        Command cmd = parser.parse(*line);

        stats.parse_time_ns +=
            to_ns(Clock::now() - parse_begin);
        ++stats.parse_count;

        // AOF 不要算进 parse
        if (cmd.type == CommandType::SET ||
            cmd.type == CommandType::DEL) {
            aoflog.append(*line);
        }

        // execute
        const auto execute_begin = Clock::now();
        std::string response =
            executor.execute(cmd);

        stats.execute_time_ns +=
            to_ns(Clock::now() - execute_begin);

        conn.outbuf += response;
        ++stats.execute_count;
    }

    // -------------------------
    // epoll_ctl 阶段
    // -------------------------
    if (!conn.outbuf.empty()) {
        const auto write_begin = Clock::now();
        WriteResult result = conn.write_response();

        stats.write_time_ns +=
            to_ns(Clock::now() - write_begin);

        ++stats.write_attempt_count;

        if (result == WRITE_AGAIN) {
            // socket 发送缓冲区暂时满了，
            // 这时才让 epoll 等待 EPOLLOUT。
            const auto ctl_begin = Clock::now();

            enable_write_event(eventfd, epollfd);

            stats.epoll_ctl_time_ns +=
                to_ns(Clock::now() - ctl_begin);

            ++stats.epoll_ctl_count;
        }
        else if (result == WRITE_ERROR) {
            close_conn(
                eventfd,
                epollfd,
                conns,
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

void handle_write(
    int eventfd,
    int epollfd,
    std::unordered_map<int, ConnState>& conns,
    PerfStats& stats
)
{
    auto it = conns.find(eventfd);

    if (it == conns.end()) {
        return;
    }

    ConnState& conn = it->second;

    const auto write_begin = Clock::now();

    WriteResult result =
        conn.write_response();

    stats.write_time_ns +=
        to_ns(Clock::now() - write_begin);

    ++stats.write_event_count;

    if (result == WRITE_AGAIN) {
        // 如果本来已经监听 EPOLLOUT，
        // 这里不一定需要再次 epoll_ctl。
        return;
    }

    if (result == WRITE_ERROR) {
        close_conn(
            eventfd,
            epollfd,
            conns,
            "write error"
        );
        return;
    }

    if (result == WRITE_DONE) {
        const auto ctl_begin = Clock::now();

        disable_write_event(
            eventfd,
            epollfd
        );

        stats.epoll_ctl_time_ns +=
            to_ns(Clock::now() - ctl_begin);

        ++stats.epoll_ctl_count;
    }
}


void close_conn(
    int fd,
    int epollfd,
    std::unordered_map<int, ConnState>& conns,
    const char* reason)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    conns.erase(fd);

    LOG_INFO("[CLOSE] fd=%d reason=%s", fd, reason);

}

void enable_write_event(int fd, int epollfd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);

}

void disable_write_event(int fd, int epollfd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);

    LOG_DEBUG("[DISABLE_WRITE] fd=%d", fd);
}

*/




