<A name="readme_md" id="readme_md"></A>

# 简介
LIB-ZC 是一个Linux平台通用C扩展库

开源, 仓库地址 https://gitee.com/linuxmail/lib-zc

## 模块列表

* [通用配置](https://gitee.com/linuxmail/lib-zc/blob/master/doc/config.md), [通用命令行参数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/main_argument.md), [通用日志](https://gitee.com/linuxmail/lib-zc/blob/master/doc/log.md)
* [链表/容器](https://gitee.com/linuxmail/lib-zc/blob/master/doc/list.md), [链表/数据结构](https://gitee.com/linuxmail/lib-zc/blob/master/doc/link.md), [链表/宏](https://gitee.com/linuxmail/lib-zc/blob/master/doc/macro_link.md), [RBTREE/数据结构](https://gitee.com/linuxmail/lib-zc/blob/master/doc/rbtree.md), [词典](https://gitee.com/linuxmail/lib-zc/blob/master/doc/dict.md), [MAP](https://gitee.com/linuxmail/lib-zc/blob/master/doc/map.md), [VECTOR](https://gitee.com/linuxmail/lib-zc/blob/master/doc/vector.md), [ARGV](https://gitee.com/linuxmail/lib-zc/blob/master/doc/argv.md), [不定长字符串](https://gitee.com/linuxmail/lib-zc/blob/master/doc/buf.md)
* [BASE64/QUOTED-PRINTABLE/HEX/NCR](https://gitee.com/linuxmail/lib-zc/blob/master/doc/encode.md), [URL](https://gitee.com/linuxmail/lib-zc/blob/master/doc/url.md)
* [字符集转码, 字符集探测](https://gitee.com/linuxmail/lib-zc/blob/master/doc/charset.md)
* [常见的IO函数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/io.md), [超时IO函数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/timed_io.md), [TCP SOCKET](https://gitee.com/linuxmail/lib-zc/blob/master/doc/tcp_socket.md), [DNS/IP地址/MAC地址](https://gitee.com/linuxmail/lib-zc/blob/master/doc/dns.md)
* [字符串函数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/string.md), [时间函数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/time.md), [文件操作函数](https://gitee.com/linuxmail/lib-zc/blob/master/doc/file.md)
* [OPENSSL函数, 线程安全,异步SSL,支持SNI](https://gitee.com/linuxmail/lib-zc/blob/master/doc/openssl.md)
* [IO流(STREAM),支持SSL](https://gitee.com/linuxmail/lib-zc/blob/master/doc/stream.md)
* [master/server服务管理框架, server包括异步IO模式和协程模式](https://gitee.com/linuxmail/lib-zc/blob/master/doc/master.md)
* [异步IO开发框架, 包括 TRIGER, 异步IO, 缓存读写, 定时器, 支持SSL](https://gitee.com/linuxmail/lib-zc/blob/master/doc/aio.md)
* [协程开发框架, 支持文件类(read, lseek, link等)操作协程化,支持锁和条件](https://gitee.com/linuxmail/lib-zc/blob/master/doc/coroutine.md)
* [IO管道(代理)库, 协程风格和异步IO风格](https://gitee.com/linuxmail/lib-zc/blob/master/doc/iopipe.md)
* [JSON 解析库](https://gitee.com/linuxmail/lib-zc/blob/master/doc/json.md), [NCR 解析](https://gitee.com/linuxmail/lib-zc/blob/master/doc/encode.md)
* [MIME 邮件解析库](https://gitee.com/linuxmail/lib-zc/blob/master/doc/mime.md), [TNEF 邮件解析库](https://gitee.com/linuxmail/lib-zc/blob/master/doc/tnef.md), [BASE64/QUOTED-PRINTABLE](https://gitee.com/linuxmail/lib-zc/blob/master/doc/encode.md)
* [REDIS 客户端(支持集群)](https://gitee.com/linuxmail/lib-zc/blob/master/doc/redis_client.md), [MEMCACHE 客户端](https://gitee.com/linuxmail/lib-zc/blob/master/doc/memcache_client.md)
* [httpd服务器库](https://gitee.com/linuxmail/lib-zc/blob/master/doc/httpd.md), [httpd服务开发通用模版](https://gitee.com/linuxmail/lib-zc/blob/master/sample/http/general_coroutine_server_httpd.c), [URL编解码](https://gitee.com/linuxmail/lib-zc/blob/master/doc/url.md)
* [一个全新的CONST DB库](https://gitee.com/linuxmail/lib-zc/blob/master/doc/cdb.md)
* [多关键字搜索](https://gitee.com/linuxmail/lib-zc/blob/master/doc/msearch.md)
* SQLITE3 代理服务器/客户端
* 类 REDIS 服务器端
* [一种简单的序列号](https://gitee.com/linuxmail/lib-zc/blob/master/doc/license.md)

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
