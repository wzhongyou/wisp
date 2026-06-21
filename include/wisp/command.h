#pragma once

#include <string>
#include <vector>

namespace wisp {

enum class CommandType {
    Ping,
    Set,
    Get,
    Del,
    Exists,
};

struct Command {
    CommandType type;
    std::vector<std::string> args;
};

}  // namespace wisp
