#pragma once
#include<string>
#include<unordered_map>
#include <optional>
#include <chrono>
#include "state_types.hpp"
#include "perfstats.hpp"


struct Entry {
    std::string value;

    std::optional<std::chrono::steady_clock::time_point>
        expire_at;
};

class KVStore{
public:
 void set(const std::string&key,
          const std::string &value);

    std::optional<std::string> get(const std::string&key);

    Status del(const std::string& key);

    bool exists(const std::string& key);
    
    size_t size() ;

    Status expire(
        const std::string& key,
        std::chrono::seconds ttl
    );

    std::optional<std::chrono::seconds> ttl(
        const std::string& key
    );

private:
    bool erase_if_expired(const std::string& key);
    void erase_expired_entries();
std::unordered_map<std::string, Entry> data_;
};