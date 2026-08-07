#pragma once
#include<unordered_map>
#include<string>

enum ReadResult {
    READ_AGAIN,       // 暂时没数据了，回 epoll_wait
    READ_CLOSED,      // 对方关闭
    READ_ERROR,       // 真错误
    READ_CONTINUE,    // 读完一个阶段，可以继续处理
    READ_MSG_DONE     // 一条完整消息读完
};

enum ReadState{
    READING_HEADERS,
    READING_BODY,
    REQUEST_READY,
    WRITING,
    CLOSED
};

enum WriteResult {
    WRITE_AGAIN,
    WRITE_ERROR,
    WRITE_DONE
};


enum class CommandType {
    PING,
    SET,
    GET,
    DEL,
    EXISTS,
    SIZE,
    EXPIRE,
    INVALID
};

struct Command {
    CommandType type{CommandType::INVALID};
    std::string key;
    std::string value;
    long long seconds{0};
};

struct Response {
    int status_code = 200;
   
    std::string to_string() const;
};

enum class Status {
    OK,
    NOT_FOUND,
    INVALID_ARGUMENT,
    UNKNOWN_COMMAND,
    INTERNAL_ERROR
};