#pragma once
#include "Connstate.hpp"
#include "parser.hpp"
#include "executor.hpp"
#include "AOFLog.hpp"
#include "log.hpp"
#include "perfstats.hpp"
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>
#include <chrono>

#include <unordered_map>
#include <string>
#include <iostream>


class KVserver
{
public:
    explicit KVserver(std::string aof_path):
     epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      store_{},
      executor_(store_),
      aoflog_(std::move(aof_path))
{
     LOG_INFO(
        "[SERVER] created epoll_fd=%d",
        epoll_fd_
    );
    
    if (epoll_fd_ == -1) {
        throw std::runtime_error(
            std::string("epoll_create1 failed: ") +
            ::strerror(errno)
        );
    }
}

    ~KVserver()
    {
        for(auto& [fd, conn] : conns_)
        {
            ::close(fd);
        }

        for(int fd : listen_fds_)
        {
            ::close(fd);
        }

        if(epoll_fd_ != -1)
        {
            ::close(epoll_fd_);
        }
    }

    KVserver(const KVserver&) = delete;
    KVserver& operator=(const KVserver&) = delete;

    bool addListener(std::uint16_t port);
    bool recoverFromAof();

    void run();

private:
    void handleAccept(int eventfd);
    void handleRead(int eventfd);
    void handleWrite(int eventfd);
    void handleClientEvent(
        int client_fd,
        std::uint32_t events
    );


    void closeTimedOutConnections();

    void disable_write_event(int eventfd);
 
    void enable_write_event(int eventfd);

    int setnonblocking(int fd); // 估计读事件，写事件都要非阻塞

    void close_conn(
    int fd,
     const char* reason
);

int initserver(int port);
    

private:
    int epoll_fd_ = -1;

    std::unordered_set<int> listen_fds_;
    std::unordered_map<int, ConnState> conns_;

    Parser parser_;

    // store_ 必须声明在 executor_ 前面
    KVStore store_;
    Executor executor_;

    AOFLog aoflog_;
    PerfStats stats_;
};