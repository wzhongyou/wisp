# wisp

wisp 是一个用 C++17 实现的轻量级内存键值存储服务器。

当前版本提供一个简单的 TCP 服务端，客户端可以通过文本命令读写内存中的字符串 key/value 数据。

## Quick start

构建项目：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

启动服务端：

```sh
./build/wisp_server 6379
```

连接服务端：

```sh
nc 127.0.0.1 6379
```

输入命令：

```text
PING
SET foo bar
GET foo
EXISTS foo
DEL foo
GET foo
```

期望输出：

```text
PONG
OK
bar
1
1
(nil)
```

## Usage

### 启动服务端

不指定端口时，服务端默认监听 `6379`：

```sh
./build/wisp_server
```

也可以显式指定端口：

```sh
./build/wisp_server 6380
```

### 连接服务端

可以使用 `nc`：

```sh
nc 127.0.0.1 6379
```

也可以使用 `telnet`：

```sh
telnet 127.0.0.1 6379
```

连接建立后，每行输入一条命令并回车。

## Commands

每条命令是一行文本，命令名和参数使用空格分隔：

```text
COMMAND [arg ...]
```

| 命令 | 说明 | 返回 |
| --- | --- | --- |
| `PING` | 检查服务是否可用 | `PONG` |
| `SET key value` | 写入字符串值 | `OK` |
| `GET key` | 读取字符串值 | 命中返回 value，未命中返回 `(nil)` |
| `DEL key` | 删除 key | 删除成功返回 `1`，key 不存在返回 `0` |
| `EXISTS key` | 判断 key 是否存在 | 存在返回 `1`，不存在返回 `0` |

错误命令返回：

```text
ERR <message>
```

## Examples

一次性发送多条命令：

```sh
printf "PING\r\nSET foo bar\r\nGET foo\r\nEXISTS foo\r\nDEL foo\r\nGET foo\r\nDEL foo\r\n" | nc 127.0.0.1 6379
```

输出：

```text
PONG
OK
bar
1
1
(nil)
0
```

多个客户端可以连接同一个服务端，并共享同一份内存数据。例如，客户端 A 写入：

```text
SET shared value
```

客户端 B 可以读取：

```text
GET shared
```

返回：

```text
value
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## License

MIT
