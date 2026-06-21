#pragma once

#include "wisp/command.h"
#include "wisp/connection.h"
#include "wisp/store.h"

#include <cstdint>
#include <string>

namespace wisp {

class Server {
public:
    explicit Server(std::uint16_t port);
    void run();

private:
    void handle_client(Connection connection);
    std::string execute(const Command& command);

    std::uint16_t port_;
    Store store_;
};

}  // namespace wisp
