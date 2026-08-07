#pragma once

#include "state_types.hpp"
#include "perfstats.hpp"
#include <string>
#include <unordered_map>

class Parser{
    
    public:
    Command parse( std::string& line);
};

