#pragma once

namespace wisp {

class Connection {
public:
    explicit Connection(int fd = -1);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    int fd() const;
    int release();
    bool valid() const;

private:
    void close_if_valid();

    int fd_;
};

}  // namespace wisp
