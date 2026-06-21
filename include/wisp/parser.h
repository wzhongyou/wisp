#pragma once

#include "wisp/command.h"

#include <optional>
#include <string>
#include <string_view>

namespace wisp {

struct ParseResult {
    std::optional<Command> command;
    std::string error;
};

ParseResult parse_command_line(std::string_view line);

}  // namespace wisp
