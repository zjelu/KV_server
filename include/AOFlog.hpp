#pragma once
#include "kvstore.hpp"
#include "AOFLog.hpp"
#include "parser.hpp"
#include "executor.hpp"
class AOFLog {
public:
    explicit AOFLog(const std::string& path);
    
    ~AOFLog();
    
    bool append(const std::string& record);

    bool replay(KVStore& store,Parser& parser,Executor& executor);

    bool is_open() const;

private:
    int fd_;
};