#pragma once

#include <string>
#include <cstddef>
#include <optional>
#include <cstddef>
#include "state_types.hpp"
#include "perfstats.hpp"

// 如果 ReadState / ReadResult / WriteResult 定义在 http.hpp 里，就 include 它
class ConnState {
    /*public:

    void read();

    WriteResult write();

    bool timeout();

    explicit ConnState(int socket)
        : fd(socket) {}

    std::optional<std::string> pop_command_line();

    WriteResult write_response();

    void reset_state();


private:

    int fd;

    Buffer input;

    Buffer output;

    time_t last_active=time(nullptr);*/

public:
    int fd;
    std::string inbuf;
    std::string outbuf;
    size_t write_offset = 0;
    time_t last_active = time(nullptr);

    explicit ConnState(int socket)
        : fd(socket) {}

    std::optional<std::string> pop_command_line();

    WriteResult write_response();

    void reset_state();
};
