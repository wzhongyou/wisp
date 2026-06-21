#include "wisp/store.h"

namespace wisp {

void Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[key] = value;
}

std::optional<std::string> Store::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::size_t Store::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.erase(key);
}

bool Store::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

}  // namespace wisp
