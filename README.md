<A name="readme_md" id="readme_md"></A>

# 简介
LIB-ZC 是一个Linux平台通用C扩展库

开源, 仓库地址 https://gitee.com/linuxmail/lib-zc

- [通用配置](./doc/config.md), [通用命令行参数](./doc/main_argument.md), [通用日志](./doc/log.md)
- 封装了常见的数据结构和算法<BR/>
  [链表/容器](./doc/list.md), [链表/数据结构](./doc/link.md), [链表/宏](./doc/macro_link.md)<BR/>
  [RBTREE/数据结构](./doc/rbtree.md), [RBTREE/宏](./doc/macro_rbtree.md)<BR/>
  [词典](./doc/dict.md), [MAP](./doc/map.md)<BR/>
  [VECTOR](./doc/vector.md), [ARGV](./doc/argv.md), [不定长字符串](./doc/buf.md)
- 封装了常用编解码 [base64/quoted-printable/hex/url/ncr](./doc/encode.md)
- 封装了常见的[io函数](./doc/io.md), [超时io函数](./doc/timed_io.md), [tcp socket](./doc/tcp_socket.md), [dns函数](./doc/dns.md)
- 封装了常见的[字符串函数](./doc/string.md), [时间函数](./doc/time.md), [文件操作函数](./doc/file.md)
- 封装了[openssl函数](./doc/openssl.md), 线程安全, 异步SSL, 支持 SNI
- 封装了[io流](./doc/stream.md), 支持ssl
- master/server服务管理框架, server包括异步io模式和协程模式
- [异步io开发框架](./doc/aio.md). 包括 triger, 异步io, 缓存读写, 定时器, 支持ssl
- [协程开发框架](./doc/coroutine.md), 支持文件类(read, lseek, link等)操作协程化,支持锁和条件
- [io管道(代理)库](./doc/iopipe.md), 协程风格和异步IO风格
- [json 解析库](./doc/json.md)
- mime 邮件解析库, [tnef邮件解析库](./doc/tnef.md)
- redis客户端(支持集群), memcache客户端
- 类redis服务器端
- sqlite3 代理服务器/客户端
- http服务器库
- 一个全新的const db库

## 背景
在Linux平台, 仅基于GNU C标准库, 开发一个全新的完整的邮件系统, 包括
* 高并发smtp/imap4/pop3服务器
* 高并发http服务器(webmail, 管理)
* 反垃圾邮件网关, 基于贝叶斯的垃圾邮件识别系统
* 邮件解析, json, redis
* 服务进程管理器
* 希望全部程序静态编译
* 等等

为此开发了 LIB-ZC 作为基础库

## 命名约定
* 函数, 结构体, 变量, 宏等 以字母 **z** 或 **Z** 开始
* 结构体以 **\_t** 结尾

## 连接或监听地址
* "local:domain_socket_somepath", 等价于, "domain_socket_somepath"
* "fifo:somepath"
* "domain_socket_somepath"
* "somedomain:port"

## 时间/超时
* 如无特别说明, 所有的时间单位都是秒
* 如果是毫秒, 函数名或形参会明确提示
* 实参取值-1则表示无限长

## 返回值
* 和(网络)io相关的函数, 如果返回值类型是int, 如果返回 < 0, 表示出错

## 编译
make 即可

得到: libzc.a(基础库) 和 libzc_coroutine.a(协程库)

## 使用
源码 sample/下有大量例子(make sample),可供参考

zc.h 和 libzc.a 在源码目录下
```
$ cat a.c 
#include "zc.h"
int main(int argc, char **argv)
{
  /* foo(); */
  return 0;
}
$ gcc a.c ./libzc.a
```
