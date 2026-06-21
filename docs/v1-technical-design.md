# wisp V1 技术设计

## 1. 背景与目标

wisp V1 是一个轻量级内存键值存储服务器。V1 的目标是用尽量小的复杂度跑通完整主链路：

```text
客户端连接 -> 发送命令 -> 服务端解析命令 -> 访问内存存储 -> 返回结果
```

V1 同时建立基础工程骨架，包括 CMake 构建、单元测试、CI 和基本使用文档。

## 2. V1 范围

### 2.1 功能范围

- TCP 服务端监听指定端口。
- 每个客户端连接使用一个独立线程处理。
- 支持简化文本协议：命令以空格分隔，并以 `\r\n` 结束。
- 支持 5 个命令：
  - `PING`
  - `SET key value`
  - `GET key`
  - `DEL key`
  - `EXISTS key`
- 使用单一内存键值存储：`std::unordered_map<std::string, std::string>`。
- 使用互斥锁保证多连接并发访问时的数据一致性。
- 使用 GoogleTest 覆盖存储层和协议解析逻辑。
- 使用 GitHub Actions 执行构建和测试。

### 2.2 非目标

V1 不实现以下能力：

- Redis RESP 协议兼容。
- `epoll` / Reactor / 异步事件循环。
- 持久化、AOF、快照。
- 多值类型，例如 List、Hash、Set。
- Key 过期机制。
- 发布订阅。
- LRU / LFU 淘汰策略。
- 认证、权限、TLS。

## 3. 协议设计

### 3.1 请求格式

每个请求是一行文本：

```text
COMMAND [arg ...]\r\n
```

解析规则：

- 命令名和参数使用空格分隔。
- 行尾支持 `\r\n`，也兼容 `\n`。
- V1 中 value 不支持包含空格。
- 空命令、未知命令、参数数量错误都会返回错误响应。

### 3.2 命令与响应

| 请求 | 说明 | 响应 |
| --- | --- | --- |
| `PING` | 健康检查 | `PONG\r\n` |
| `SET key value` | 写入字符串值 | `OK\r\n` |
| `GET key` | 读取字符串值 | 命中：`<value>\r\n`；未命中：`(nil)\r\n` |
| `DEL key` | 删除 key | 删除成功：`1\r\n`；key 不存在：`0\r\n` |
| `EXISTS key` | 判断 key 是否存在 | 存在：`1\r\n`；不存在：`0\r\n` |

错误响应：

```text
ERR <message>\r\n
```

## 4. 架构设计

```mermaid
flowchart LR
    Client[Client] --> Server[TCP Server]
    Server --> Thread[Connection Thread]
    Thread --> Parser[Command Parser]
    Parser --> Dispatcher[Command Dispatcher]
    Dispatcher --> Store[Mutex-protected Store]
    Store --> Map[unordered_map<string,string>]
```

### 4.1 模块划分

| 模块 | 责任 |
| --- | --- |
| `Connection` | RAII 封装 socket fd，负责连接生命周期管理 |
| `Server` | 创建监听 socket，accept 客户端连接，为每个连接创建处理线程 |
| `Parser` | 将一行文本协议解析为结构化命令 |
| `Command` | 表达命令类型和参数 |
| `Store` | 封装线程安全的内存键值存储 |
| `main` | 解析启动参数并启动服务 |

## 5. 目录结构

```text
wisp/
├── CMakeLists.txt
├── README.md
├── docs/
│   └── v1-technical-design.md
├── include/
│   └── wisp/
│       ├── command.h
│       ├── connection.h
│       ├── parser.h
│       ├── server.h
│       └── store.h
├── src/
│   ├── connection.cpp
│   ├── main.cpp
│   ├── parser.cpp
│   ├── server.cpp
│   └── store.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── parser_test.cpp
│   └── store_test.cpp
└── .github/
    └── workflows/
        └── ci.yml
```

## 6. 核心接口设计

### 6.1 Store

```cpp
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
```

设计约束：

- `get` 未命中返回 `std::nullopt`。
- 不用裸指针表达查询结果。
- 不用异常表达正常的 key 未命中。
- 不在持有存储锁时执行 socket I/O。

### 6.2 Command 与 Parser

```cpp
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

struct ParseResult {
    std::optional<Command> command;
    std::string error;
};

ParseResult parse_command_line(std::string_view line);

}  // namespace wisp
```

设计约束：

- 解析阶段可以使用 `std::string_view` 切分输入。
- `Command` 中保存自有的 `std::string`，避免引用临时 buffer。
- Parser 不依赖 socket，也不依赖 Store，便于单元测试。

### 6.3 Connection

```cpp
namespace wisp {

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    int fd() const;
    int release();

private:
    int fd_;
};

}  // namespace wisp
```

设计约束：

- `Connection` 独占 socket fd。
- 析构时自动关闭有效 fd。
- 支持移动，不支持拷贝。

### 6.4 Server

```cpp
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
```

设计约束：

- `run` 创建监听 socket，并进入 accept 循环。
- 每个客户端连接由一个 detached `std::thread` 处理。
- 单个客户端连接可以连续发送多条命令。
- 连接处理逻辑维护读缓冲，支持 TCP 拆包和一次读取多条命令。

## 7. 并发模型

V1 使用 thread-per-connection：

```text
accept client fd -> 创建 Connection -> std::thread(handle_client).detach()
```

优点：

- 实现简单。
- 每个连接的控制流清晰。
- 容易验证主链路正确性。

限制：

- 大量连接时线程数量会线性增长。
- 不适合作为高并发最终形态。

V1 接受这些限制，后续版本可以再演进到线程池或事件循环模型。

## 8. 构建与测试设计

### 8.1 CMake target

| Target | 类型 | 说明 |
| --- | --- | --- |
| `wisp_core` | library | Store、Parser、Connection、Server |
| `wisp_server` | executable | 服务端入口 |
| `wisp_tests` | executable | 单元测试 |

### 8.2 单元测试

Store 测试覆盖：

- 写入后读取。
- 读取不存在 key 返回空 optional。
- 覆盖写入。
- 删除存在 key 返回 `1`。
- 删除不存在 key 返回 `0`。
- 判断 key 是否存在。

Parser 测试覆盖：

- 正确解析 `PING` / `SET` / `GET` / `DEL` / `EXISTS`。
- 正确处理 `\r\n` 和 `\n`。
- 拒绝空命令。
- 拒绝未知命令。
- 拒绝参数数量错误。

### 8.3 CI

GitHub Actions 执行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## 9. 验收方式

验收目标是确认 V1 主链路已经跑通：服务端能启动，客户端能连接，客户端能连续发送命令，服务端能返回符合协议的结果。

### 9.1 本地构建与测试

在项目根目录执行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

验收标准：

- `cmake` 配置成功。
- `cmake --build` 编译成功，生成 `build/wisp_server`。
- `ctest` 全部通过。

### 9.2 启动服务端

默认端口为 `6379`。启动服务端：

```sh
./build/wisp_server
```

也可以显式指定端口：

```sh
./build/wisp_server 6379
```

验收标准：

- 服务端进程保持运行，不立即退出。
- 服务端监听 `127.0.0.1:6379` 可被本机客户端连接。
- 如果端口被占用，服务端应输出错误并退出。

### 9.3 连接客户端

可以使用 `telnet` 连接：

```sh
telnet 127.0.0.1 6379
```

也可以使用 `nc` 连接：

```sh
nc 127.0.0.1 6379
```

连接成功后，直接输入命令并回车。每条命令是一行文本，格式为：

```text
COMMAND [arg ...]
```

服务端内部按 `\r\n` / `\n` 作为命令结束符处理。

### 9.4 基础命令验收

以下操作在同一个客户端连接中依次执行。

#### 9.4.1 PING

输入：

```text
PING
```

期望输出：

```text
PONG
```

验收点：服务端能处理最简单的请求-响应。

#### 9.4.2 SET + GET 命中

输入：

```text
SET foo bar
```

期望输出：

```text
OK
```

继续输入：

```text
GET foo
```

期望输出：

```text
bar
```

验收点：`SET` 能写入字符串值，`GET` 能读取刚写入的值。

#### 9.4.3 GET 未命中

输入：

```text
GET missing
```

期望输出：

```text
(nil)
```

验收点：不存在的 key 返回 `(nil)`，不是空字符串，也不是错误。

#### 9.4.4 EXISTS

输入：

```text
EXISTS foo
```

期望输出：

```text
1
```

输入：

```text
EXISTS missing
```

期望输出：

```text
0
```

验收点：存在的 key 返回 `1`，不存在的 key 返回 `0`。

#### 9.4.5 DEL + 删除后 GET

输入：

```text
DEL foo
```

期望输出：

```text
1
```

继续输入：

```text
GET foo
```

期望输出：

```text
(nil)
```

再次输入：

```text
DEL foo
```

期望输出：

```text
0
```

验收点：

- 删除已存在 key 返回 `1`。
- 删除后再次读取返回 `(nil)`。
- 删除不存在 key 返回 `0`。

### 9.5 一次性脚本验收

可以用 `printf + nc` 一次性发送多条命令：

```sh
printf "PING\r\nSET foo bar\r\nGET foo\r\nEXISTS foo\r\nDEL foo\r\nGET foo\r\nDEL foo\r\n" | nc 127.0.0.1 6379
```

期望输出：

```text
PONG
OK
bar
1
1
(nil)
0
```

验收点：服务端能在同一个 TCP 连接中连续处理多条命令。

### 9.6 多客户端验收

打开两个客户端连接。

客户端 A：

```text
SET shared value
```

期望输出：

```text
OK
```

客户端 B：

```text
GET shared
```

期望输出：

```text
value
```

客户端 B：

```text
DEL shared
```

期望输出：

```text
1
```

客户端 A：

```text
EXISTS shared
```

期望输出：

```text
0
```

验收点：

- 多个客户端共享同一份内存存储。
- `SET` / `GET` / `DEL` / `EXISTS` 在多连接场景下结果一致。
- 服务端不会因为多个客户端同时连接而崩溃或返回串线数据。

### 9.7 错误输入验收

输入未知命令：

```text
UNKNOWN
```

期望输出格式：

```text
ERR <message>
```

输入参数数量错误的命令：

```text
SET only_key
```

期望输出格式：

```text
ERR <message>
```

验收点：错误输入不会导致服务端崩溃，服务端返回 `ERR` 响应并继续处理后续命令。
