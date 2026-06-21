#include "wisp/parser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace wisp {
namespace {

std::string to_upper(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

std::vector<std::string_view> split_by_space(std::string_view line) {
    std::vector<std::string_view> parts;
    std::size_t pos = 0;

    while (pos < line.size()) {
        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }

        const auto start = pos;
        while (pos < line.size() && line[pos] != ' ') {
            ++pos;
        }

        if (start < pos) {
            parts.push_back(line.substr(start, pos - start));
        }
    }

    return parts;
}

std::string expected_arity_error(std::string_view command, std::size_t expected, std::size_t actual) {
    return std::string(command) + " expects " + std::to_string(expected) + " argument(s), got " +
           std::to_string(actual);
}

ParseResult make_command(CommandType type, std::vector<std::string_view> args) {
    Command command{type, {}};
    command.args.reserve(args.size());
    for (const auto arg : args) {
        command.args.emplace_back(arg);
    }
    return ParseResult{std::move(command), {}};
}

}  // namespace

ParseResult parse_command_line(std::string_view line) {
    if (!line.empty() && line.back() == '\n') {
        line.remove_suffix(1);
    }
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }

    const auto parts = split_by_space(line);
    if (parts.empty()) {
        return ParseResult{std::nullopt, "empty command"};
    }

    const auto command_name = to_upper(parts.front());
    std::vector<std::string_view> args(parts.begin() + 1, parts.end());

    if (command_name == "PING") {
        if (!args.empty()) {
            return ParseResult{std::nullopt, expected_arity_error(command_name, 0, args.size())};
        }
        return make_command(CommandType::Ping, args);
    }

    if (command_name == "SET") {
        if (args.size() != 2) {
            return ParseResult{std::nullopt, expected_arity_error(command_name, 2, args.size())};
        }
        return make_command(CommandType::Set, args);
    }

    if (command_name == "GET") {
        if (args.size() != 1) {
            return ParseResult{std::nullopt, expected_arity_error(command_name, 1, args.size())};
        }
        return make_command(CommandType::Get, args);
    }

    if (command_name == "DEL") {
        if (args.size() != 1) {
            return ParseResult{std::nullopt, expected_arity_error(command_name, 1, args.size())};
        }
        return make_command(CommandType::Del, args);
    }

    if (command_name == "EXISTS") {
        if (args.size() != 1) {
            return ParseResult{std::nullopt, expected_arity_error(command_name, 1, args.size())};
        }
        return make_command(CommandType::Exists, args);
    }

    return ParseResult{std::nullopt, "unknown command: " + command_name};
}

}  // namespace wisp
