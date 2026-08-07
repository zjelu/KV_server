#pragma once
#include <string>
#include "state_types.hpp"
#include "KVStore.hpp"
#include "perfstats.hpp"


class Executor {
public:
     Executor(KVStore& store);

    std::string execute(const Command& cmd);

private:
    KVStore& store_;
};