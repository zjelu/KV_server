#include "KVStore.hpp"

bool KVStore::erase_if_expired(const std::string& key)
{
    auto it = data_.find(key);

    if (it == data_.end()) {
        return false;
    }

    const auto& expire_at = it->second.expire_at;

    if (!expire_at.has_value()) {
        return false;
    }

    if (std::chrono::steady_clock::now() >= *expire_at) {
        data_.erase(it);
        return true;
    }

    return false;
}


void KVStore::set(
    const std::string& key,
    const std::string& value)
{
    data_[key] = Entry{
        value,
        std::nullopt
    };
}


std::optional<std::string>
KVStore::get(const std::string& key)
{
    erase_if_expired(key);

    auto it = data_.find(key);

    if (it == data_.end()) {
        return std::nullopt;
    }

    return it->second.value;
}


 Status KVStore::del(const std::string& key)
{
    erase_if_expired(key);

    const std::size_t erased = data_.erase(key);

    if (erased == 0) {
        return Status::NOT_FOUND;
    }

    return Status::OK;
}
 
bool KVStore::exists(const std::string& key)
{
    erase_if_expired(key);

    return data_.find(key) != data_.end();
}

void KVStore::erase_expired_entries()
{
    const auto now = std::chrono::steady_clock::now();

    for (auto it = data_.begin(); it != data_.end();) {
        const auto& expire_at = it->second.expire_at;

        if (expire_at.has_value() && now >= *expire_at) {
            it = data_.erase(it);
        } else {
            ++it;
        }
    }
}

 Status KVStore::expire(
    const std::string& key,
    std::chrono::seconds ttl)
{
    erase_if_expired(key);

    auto it = data_.find(key);

    if (it == data_.end()) {
        return Status::NOT_FOUND;
    }

    if (ttl.count() <= 0) {
        data_.erase(it);
        return Status::OK;
    }

    it->second.expire_at =
        std::chrono::steady_clock::now() + ttl;

    return Status::OK;
}

std::size_t KVStore::size() 
{
    erase_expired_entries();
    return data_.size();
}