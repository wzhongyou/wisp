#include "wisp/server.h"

#include "wisp/parser.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace wisp {
namespace {

constexpr int kBacklog = 128;
constexpr std::size_t kReadBufferSize = 4096;

void set_reuse_addr(int fd) {
    int enabled = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) < 0) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed: " + std::string(std::strerror(errno)));
    }
}

void send_all(int fd, const std::string& response) {
    std::size_t sent = 0;
    while (sent < response.size()) {
        const auto n = ::send(fd, response.data() + sent, response.size() - sent, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (n == 0) {
            return;
        }
        sent += static_cast<std::size_t>(n);
    }
}

Connection create_listen_socket(std::uint16_t port) {
    Connection listen_socket(::socket(AF_INET, SOCK_STREAM, 0));
    if (!listen_socket.valid()) {
        throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));
    }

    set_reuse_addr(listen_socket.fd());

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(listen_socket.fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }

    if (::listen(listen_socket.fd(), kBacklog) < 0) {
        throw std::runtime_error("listen() failed: " + std::string(std::strerror(errno)));
    }

    return listen_socket;
}

}  // namespace

Server::Server(std::uint16_t port) : port_(port) {}

void Server::run() {
    ::signal(SIGPIPE, SIG_IGN);

    auto listen_socket = create_listen_socket(port_);

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        const int client_fd = ::accept(
            listen_socket.fd(), reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("accept() failed: " + std::string(std::strerror(errno)));
        }

        std::thread([this, connection = Connection(client_fd)]() mutable {
            handle_client(std::move(connection));
        }).detach();
    }
}

void Server::handle_client(Connection connection) {
    std::string pending;
    char buffer[kReadBufferSize];

    while (true) {
        const auto n = ::recv(connection.fd(), buffer, sizeof(buffer), 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (n == 0) {
            return;
        }

        pending.append(buffer, static_cast<std::size_t>(n));

        std::size_t newline_pos = std::string::npos;
        while ((newline_pos = pending.find('\n')) != std::string::npos) {
            const auto line = pending.substr(0, newline_pos + 1);
            pending.erase(0, newline_pos + 1);

            const auto parse_result = parse_command_line(line);
            if (!parse_result.command.has_value()) {
                send_all(connection.fd(), "ERR " + parse_result.error + "\r\n");
                continue;
            }

            send_all(connection.fd(), execute(*parse_result.command));
        }
    }
}

std::string Server::execute(const Command& command) {
    switch (command.type) {
        case CommandType::Ping:
            return "PONG\r\n";
        case CommandType::Set:
            store_.set(command.args[0], command.args[1]);
            return "OK\r\n";
        case CommandType::Get: {
            const auto value = store_.get(command.args[0]);
            if (!value.has_value()) {
                return "(nil)\r\n";
            }
            return *value + "\r\n";
        }
        case CommandType::Del:
            return std::to_string(store_.del(command.args[0])) + "\r\n";
        case CommandType::Exists:
            return std::string(store_.exists(command.args[0]) ? "1" : "0") + "\r\n";
    }

    return "ERR unsupported command\r\n";
}

}  // namespace wisp
