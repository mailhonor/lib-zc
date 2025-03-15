<A name="readme_md" id="readme_md"></A>

[C++版本](./iopipe_cpp.md)

## IO管道, IO映射, IO端口转发, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)
支持 IO管道, IO映射, IO端口转发, 支持 OPENSSL

两种方法: 基于基于 [协程](./coroutine.md) 和 [异步IO](./aio.md)

## 基于协程环境

### void zcoroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, zcoroutine_base_t *cobs, void (*after_close)(void *ctx), void *ctx);

* 在协程环境下, 此函数在两组 socket(fd1,ssl1 和 fd2,ssl2) 之间建立管道, 做数据转发 
* 数据转发终止后, 执行函数 after_close(ctx), 并自动释放 fd1/ssl1, fd2/ssl2

### 基本流程

1. 准备一个协程环境 (zcoroutine_base_t *)cobs
2. 和第1个端口建立了连接, 得到 (int)fd1; 如果是 SSL, 则建立 OEPNSSL 通道, 得到 (SSL *)ssl1
3. 和第2个端口建立了连接, 得到 (int)fd2; 如果是 SSL, 则建立 OEPNSSL 通道, 得到 (SSL *)ssl2
4. 执行函数 zcoroutine_go_iopipe(fd1, ssl1, fd2, ssl2, cobs, 0, 0);<BR />cobs==0: 表示当前线程的协程环境

则在当协程环境 cobs, 管道建立成功


## 基于异步IO

### void zaio_iopipe_enter(zaio_t *aio1, zaio_t *aio2, zaio_base_t *aiobase, void (*after_close)(void *ctx), void *ctx)

* 在异步IO环境下, 此函数在两个"异步IO"(client和server) 之间建立管道, 做数据转发
* 数据转发终止后, 执行函数 after_close(ctx), 并自动释放 aio1, aio2 和底层资源(fd, ssl)

### 基本流程

1. 准备一个"异步IO环境" (zaio_base_t *)aiobase
2. 和第1个端口建立了连接(或OPENSSL连接), 得到 aio1
3. 和第2个端口建立了连接(或OPENSSL连接), 得到 aio2
4. 执行函数 zaio_iopipe_enter(aio1, aio2, aiobase, 0, 0);<BR />aiobase 可以在其他线程

则在异步IO环境aiobase, 管道建立成功

## 案例讲解: IMAP 服务器反向代理

### 需求描述

已知: 内网有多台 IMAP 服务器, 每台服务器服务不同的帐号; 网关对外提供 IMAP 服务

问题: 在网关根据不同的帐号反向代理到不同的内网服务器, 怎么办 ?

重点提示: IMAP 协议极其复杂, 代理服务器没必要实现全部 IMAP 协议解析

提示: nginx 可以实现此功能

本文只介绍 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 提供的 IO管道的方法

### 实现1: (协程环境)

1. 初始化 [协程环境](./coroutine.md), 得到 (zcoroutine_base_t *)cobs
2. 监听 143 端口, 993(SSL) 端口; 客户端成功连接后, 得到 (int)client_fd 或 (SSL *)client_ssl
3. 处理客户端基本 IMAP 协议(欢迎语, capability, starttls 等)
4. 处理客户端命令 **login**(login 命令里有帐号信息), 根据帐号得到相应的后端服务器地址
5. 连接后端服务器 143/993 端口, 成功后, 得到 (int)server_fd 或 (SSL *)server_ssl
6. 处理服务端基本 IMAP协议(欢迎语, 等)
7. 处理自定义指令; 如传递客户端的ip/port等信息
8. 执行下面函数建立管道<BR />
    ```zcoroutine_go_iopipe(client_fd, client_ssl, server_fd,  server_ssl, cobs, after_close, ctx);```

从现在开始, 程序不再干预客户端和后端服务器之间的数据传输

等连接终止后, 执行函数

```
after_close(ctx);
```

### 实现2: (异步IO环境)

1. 初始化 [异步IO环境](./aio.md), 得到 (zaio_base_t *)aiobase
2. 监听 143 端口, 993(SSL) 端口; 客户端成功连接后, 得到 (zaio_t *)client_aio
3. 处理客户端基本 IMAP 协议(欢迎语, capability, starttls 等)
4. 处理客户端命令 **login**(login 命令里有帐号信息), 根据帐号得到相应的后端服务器地址
5. 连接后端服务器 143/993 端口, 成功后, 得到 (zaio_t *)server_aio
6. 处理服务端基本 IMAP协议(欢迎语, 等)
7. 处理自定义指令; 如传递客户端的ip/port等信息
8. 执行下面函数建立管道 <BR />
    ```void zaio_iopipe_enter(client_aio, server_aio, aiobase, after_close, ctx);```

从现在开始, 程序不再干预客户端和后端服务器之间的数据传输

等连接终止后, 执行函数

```
after_close(ctx);
```

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/event/iopipe.c
* https://gitee.com/linuxmail/lib-zc/blob/master/sample/coroutine/iopipe.c

