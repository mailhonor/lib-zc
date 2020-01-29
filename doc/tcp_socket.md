# TCP socket 封装

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了tcp socket常见函数

## 宏定义

```
/* inet, tcp */
#define zvar_tcp_listen_type_inet  'i'
/* domain socket */
#define zvar_tcp_listen_type_unix  'u'
/* fifo */
#define zvar_tcp_listen_type_fifo  'f'
```

## 函数

*下面函数*

- 返回 <0 都表示失败.
- 超时单位都是 秒, -1 表示无限长
- 全部函数 忽略信号 EINTR
- 超时返回 -1 

### ACCEPT

```
/* domain socket ,  忽略 EINTR */
/* 返回 accept 后的 文件描述符, 且设置 SO_KEEPALIVE */
int zunix_accept(int fd);

/* tcp socket ,  忽略 EINTR */
/* 返回 accept 后的 文件描述符, 且设置 SO_KEEPALIVE */
int zinet_accept(int fd);
```

### listen

```
/* 监听地址 addr, 类似函数 listen */
int zunix_listen(char *addr, int backlog);

/* 监听host为sip, 端口为port的地址 */
int zinet_listen(const char *sip, int port, int backlog);

/* nextpath 形如 192.168.6.5:25, 或 路径(domain socket, 例如 /home/xxx/a/b.socket) */
int zlisten(const char *netpath, int *type, int backlog);

/* 监听 fifo */
int zfifo_listen(const char *path);
```

### connect

```
int zunix_connect(const char *addr, int timeout);

int zinet_connect(const char *dip, int port, int timeout);

int zhost_connect(const char *host, int port,, int timeout);

/* nextpath 形如 192.168.6.5:25, 或 路径(domain socket, 例如 /home/xxx/a/b.socket) */
int zconnect(const char *netpath int timeout);
```

## 带超时连接的原理

第一步: 设置 socket_fd 非阻塞

第二步:  执行 connnect

- 返回0,  成功(一般是本机)
- 返回-1, 如果errno == EINPROGRESS, 则成功, 否则失败

第三步:  等待socket读写状态

- 可写(当连接成功后，socket\_fd 就会处于可写状态，此时表示连接成功)
- 可读可写: 1) 连接成功, 且对方发送了数据. 2)连接失败<BR>检测方法: 再次执行connect，然后查看error是否等于EISCONN(表示已经连接到该套接字)
- 错误

