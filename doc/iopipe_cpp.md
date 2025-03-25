
[C版本](./iopipe.md)

## IO管道, IO映射, IO端口转发, [LIB-ZC](./README.md)

[LIB-ZC](./README.md)
支持 IO管道, IO映射, IO端口转发, 支持 OPENSSL

## 基于异步IO

```c++
namespace zcc {
void aio_iopipe_enter(aio *client, aio *server, aio_base *aiobase, std::function<void()> after_close)
}
```


### 基本流程

1. 准备一个"异步IO环境" (zcc::aio_base *)aiobase
2. 和第1个端口建立了连接(或OPENSSL连接), 得到 aio1
3. 和第2个端口建立了连接(或OPENSSL连接), 得到 aio2
4. 执行函数 aio_iopipe_enter(aio1, aio2, aiobase, [](){});<BR />aiobase 可以在其他线程

则在异步IO环境aiobase, 管道建立成功

## 案例讲解: IMAP 服务器反向代理

### 需求描述

已知: 内网有多台 IMAP 服务器, 每台服务器服务不同的帐号; 网关对外提供 IMAP 服务

问题: 在网关根据不同的帐号反向代理到不同的内网服务器, 怎么办 ?

重点提示: IMAP 协议极其复杂, 代理服务器没必要实现全部 IMAP 协议解析

提示: nginx 可以实现此功能

本文只介绍 [LIB-ZC](./README.md) 提供的 IO管道的方法

### 实现: (异步IO环境)

1. 初始化 [异步IO环境](./aio_cpp.md), 得到 (zcc::aio_base *)aiobase
2. 监听 143 端口, 993(SSL) 端口; 客户端成功连接后, 得到 (zcc::aio *)client_aio
3. 处理客户端基本 IMAP 协议(欢迎语, capability, starttls 等)
4. 处理客户端命令 **login**(login 命令里有帐号信息), 根据帐号得到相应的后端服务器地址
5. 连接后端服务器 143/993 端口, 成功后, 得到 (zcc::aio *)server_aio
6. 处理服务端基本 IMAP协议(欢迎语, 等)
7. 处理自定义指令; 如传递客户端的ip/port等信息
8. 执行下面函数建立管道 <BR />

```c++
zcc::aio_iopipe_enter(client_aio, server_aio, aiobase, after_close)
```

从现在开始, 程序不再干预客户端和后端服务器之间的数据传输

等连接终止后, 执行函数

```c++
after_close();
```

## 例子

* [goto](../blob/master/cpp_sample/event/iopipe.cpp)

