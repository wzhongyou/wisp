#include "wisp/connection.h"

#include <unistd.h>

#include <utility>

namespace wisp {

Connection::Connection(int fd) : fd_(fd) {}

Connection::~Connection() {
    close_if_valid();
}

Connection::Connection(Connection&& other) noexcept : fd_(other.release()) {}

Connection& Connection::operator=(Connection&& other) noexcept {
    if (this != &other) {
        close_if_valid();
        fd_ = other.release();
    }
    return *this;
}

int Connection::fd() const {
    return fd_;
}

int Connection::release() {
    const int fd = fd_;
    fd_ = -1;
    return fd;
}

bool Connection::valid() const {
    return fd_ >= 0;
}

void Connection::close_if_valid() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

}  // namespace wisp
