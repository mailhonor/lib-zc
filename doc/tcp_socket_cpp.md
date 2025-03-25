
## TCP SOCKET 封装, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了常见的 TCP SOCKET 函数

[C版本](./tcp_socket.md)

本文全部函数忽略信号 EINTR

## 常量

```c++
static const char var_tcp_listen_type_inet = 'i';
static const char var_tcp_listen_type_unix = 'u';
static const char var_tcp_listen_type_fifo = 'f';
```

## netpath

举例 netpath:

* 192.168.1.1:25
* /somepath/someppp/123_domain_socket_path
* 0:25;127.0.0.1:46;./somepath/123;/home/xxx/111;0:8899

## 函数

```c++
// window平台需要执行这个函数初始化环境
int WSAStartup();

// 关闭 fd, 兼容 windows closesocket
int close_socket(int fd);

//
int unix_accept(int fd);
int inet_accept(int fd);
int socket_accept(int fd, int type);

//
int unix_listen(char *addr, int backlog = 128);
int inet_listen(const char *sip, int port, int backlog = 128);
int fifo_listen(const char *path);
int netpath_listen(const char *netpath, int backlog = 128, int *type = nullptr);

//
int unix_connect(const char *addr, int timeout);
int inet_connect(const char *dip, int port, int timeout);
int host_connect(const char *host, int port, int timeout);
int netpath_connect(const char *netpath, int timeout);

// 获取对端的 地址 和 端口
int get_peername(int sockfd, int *host, int *port);
```

## 带超时连接(connect)的实现过程

```
假设 socket_fd 为connect用的 socket

设置 socket_fd 非阻塞, 继续

执行 connnect
    返回 -1 且 errno != EINPROGRESS: 失败, 流程终止
    否则: 继续

超时等待 socket_fd 读写状态
    可读不可写: 失败, 流程终止
    可写不可读: 成功, 流程终止
    可读又可写: 再次执行 connect:
        返回 -1 且 errno==EISCONN: 成功, 流程终止
        否则: 失败, 流程终止
```

