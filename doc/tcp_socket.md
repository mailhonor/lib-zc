
## TCP SOCKET 封装, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了常见的 TCP SOCKET 函数

[C++版本](./tcp_socket_cpp.md)

本文全部函数忽略信号 EINTR

## 宏常量

### #define zvar_tcp_listen_type_inet  'i'

* 网络 socket

### #define zvar_tcp_listen_type_unix  'u'
* domain socket

### #define zvar_tcp_listen_type_fifo  'f'

* 命名管道 fifo

## ACCEPT

### int zunix_accept(int fd);

* 参考 accept
* 返回 accept 后的文件描述符, 且设置 SO_KEEPALIVE 
* domain socket 类型

### int zinet_accept(int fd);

* 参考 accept
* 返回 accept 后的文件描述符, 且设置 SO_KEEPALIVE 
* inet 类型

## LISTEN

### int zunix_listen(char *addr, int backlog);

* 参考 listen
* 监听地址 domain socket 地址 addr

### int zinet_listen(const char *sip, int port, int backlog);

* 参考 listen
* 监听 sip:port

### int zlisten(const char *netpath, int *type, int backlog);

* 参考 listen
* 监听 netpath
* netpath 形如: 192.168.1.1:25 或 0:25 或 /homepath/some/abc

### int zfifo_listen(const char *path);

* 监听命名管道 

## CONNECT

* 下面函数 参考 connect

### int zunix_connect(const char *addr, int timeout);

* connect addr

### int zinet_connect(const char *dip, int port, int timeout);

* connect dip:port

### int zhost_connect(const char *host, int port,, int timeout);

* connect host:port

### int zconnect(const char *netpath int timeout);

* connecet netpath
* 举例 netpath:
    * 192.168.1.1:25
    * /somepath/someppp/123_domain_socket_path
    * 0:25;127.0.0.1:46;./somepath/123;/home/xxx/111;0:8899

## socket属性

### zbool_t zget_peername(int sockfd, int *host, int *port);

* 获取 sockfd 另一端的 ip 和 端口信息

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

