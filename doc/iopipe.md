# IO映射 / IO端口转发

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

支持端口转发(socket管道), 支持openssl

## 应用场景

举个例子: IMAP服务器代理

**服务器**

- 前端: 1台服务器, 对外, 开放imap服务器端口 143, 993(ssl)
- 后端: 2台服务器, 内部, imapd服务, 开放 143端口, 

**两个帐号**

- abc1@xxx.com: 这个帐号的数据在 第1台后端服务器
- def2@xxx.com: 这个帐号的数据在 第2台后端服务器

**目标**

在前端实现一个imap代理服务器,做到

- 客户端能通过前端服务器在第1台后端访问帐号 abc1@xxx.com
- 客户端能通过前端服务器在第2台后端访问帐号 def2@xxx.com

重点提示: imap协议相当复杂, 代理服务器没必要实现全部imap协议解析

**方法**

可以考虑使用LIB-ZC提供的端口转发函数解决此类问题

## 方法1(协程环境)

```
void zcoroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, void (*after_close)(void *ctx), void *ctx);
```

## 例子(需求见:应用场景)

在协程环境, 开发一个imap转发服务器

**第一步:初始化协程环境**

LIB-ZC的协程环境使用见:

https://blog.csdn.net/eli960/article/details/93623763

https://gitee.com/linuxmail/lib-zc/blob/master/doc/coroutine.md

**第二步:监听143, 993(SSL)端口**

接受客户端成功连接后, 得到client_fd或 client_ssl

**第三步: 基本imap协议**

处理和客户端基本的协议请求, imap命令包括:

*welcome banner, capability, starttls, **login*** 等

**第四步: 处理命令login**

认证成功后, 根据用户名获得后端服务器地址

**第五步: 连接后端服务器143端口**

成功后, 得到server_fd.并处理如下协议

- 接受banner
- 自定义指令(如: 传递客户端ip/port给服务器)
- 可以不包括命令login

**第六步: 两个连接(和客户端的连接,和服务器的连接)做一个管道**

调用函数

```
zcoroutine_go_iopipe(client_fd, client_ssl, server_fd,  0, after_close, ctx);
```

**结尾**

从现在开始, 程序不在干预客户端和后端服务器之间的数据传输

等连接终止后, 执行函数

```
after_close(ctx);
```

## 方法2(异步IO环境)

```
void zaio_iopipe_enter(zaio_t *client, zaio_t *server, zaio_base_t *aiobase, void (*after_close)(void *ctx), void *ctx);
```

## 例子(需求见:应用场景)

在异步环境(aio)环境, 开发一个imap转发服务器

**第一步:初始化aio环境**

得到 zaio\_base\_t \*aiobase

LIB-ZC的异步IO环境使用见:

https://gitee.com/linuxmail/lib-zc/blob/master/doc/aio.md

https://blog.csdn.net/eli960/article/details/93343842

**第二步:监听143, 993(SSL)端口**

接受客户端成功连接后, 创建并得到 zaio\_t \*client_aio;

**第三步: 基本imap协议**

处理和客户端基本的协议请求, imap命令包括:

*welcome banner, capability, starttls, **login*** 等

**第四步: 处理命令login**

认证成功后, 根据用户名获得后端服务器地址

**第五步: 连接后端服务器143端口**

成功后, 创建 zaio\_t \*server_aio, 并处理如下协议

- 接受banner
- 自定义指令(如: 传递客户端ip/port给服务器)
- 可以不包括命令login

**第六步: 两个连接(和客户端的连接,和服务器的连接)做一个管道**

调用函数

```
void zaio_iopipe_enter(client_aio, server_aio, aiobase, after_close, ctx);
```

**结尾**

从现在开始, 程序不在干预客户端和后端服务器之间的数据传输

等连接终止后, 执行函数

```
after_close(ctx);
```
