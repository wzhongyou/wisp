#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace wisp {

class Store {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    std::size_t del(const std::string& key);
    bool exists(const std::string& key) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

}  // namespace wisp
