#include "wisp/server.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

constexpr std::uint16_t kDefaultPort = 6379;

std::uint16_t parse_port(const char* value) {
    std::size_t consumed = 0;
    const auto port = std::stoul(value, &consumed, 10);
    if (consumed != std::string(value).size() || port == 0 ||
        port > std::numeric_limits<std::uint16_t>::max()) {
        throw std::invalid_argument("invalid port");
    }
    return static_cast<std::uint16_t>(port);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [port]\n";
        return 1;
    }

    try {
        const auto port = argc == 2 ? parse_port(argv[1]) : kDefaultPort;
        std::cout << "wisp server listening on 0.0.0.0:" << port << std::endl;
        wisp::Server server(port);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "wisp server error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
