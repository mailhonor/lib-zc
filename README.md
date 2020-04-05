<A name="readme_md" id="readme_md"></A>

# 简介
LIB-ZC 是一个Linux平台通用C扩展库

开源, 仓库地址 https://gitee.com/linuxmail/lib-zc

帮助文档 [http://linuxmail.cn/lib-zc/](http://linuxmail.cn/lib-zc/)

## 模块列表

* [通用配置](http://linuxmail.cn/lib-zc/config.html), [通用命令行参数](http://linuxmail.cn/lib-zc/main_argument.html), [通用日志](http://linuxmail.cn/lib-zc/log.html)
* [链表/容器](http://linuxmail.cn/lib-zc/list.html), [链表/数据结构](http://linuxmail.cn/lib-zc/link.html), [链表/宏](http://linuxmail.cn/lib-zc/macro_link.html), [RBTREE/数据结构](http://linuxmail.cn/lib-zc/rbtree.html), [RBTREE/宏](http://linuxmail.cn/lib-zc/macro_rbtree.html), [词典](http://linuxmail.cn/lib-zc/dict.html), [MAP](http://linuxmail.cn/lib-zc/map.html), [VECTOR](http://linuxmail.cn/lib-zc/vector.html), [ARGV](http://linuxmail.cn/lib-zc/argv.html), [不定长字符串](http://linuxmail.cn/lib-zc/buf.html)
* [BASE64/QUOTED-PRINTABLE/HEX/NCR](http://linuxmail.cn/lib-zc/encode.html), [URL](http://linuxmail.cn/lib-zc/url.html)
* [字符集转码, 字符集探测](http://linuxmail.cn/lib-zc/charset.html)
* [常见的IO函数](http://linuxmail.cn/lib-zc/io.html), [超时IO函数](http://linuxmail.cn/lib-zc/timed_io.html), [TCP SOCKET](http://linuxmail.cn/lib-zc/tcp_socket.html), [DNS/IP地址/MAC地址](http://linuxmail.cn/lib-zc/dns.html)
* [字符串函数](http://linuxmail.cn/lib-zc/string.html), [时间函数](http://linuxmail.cn/lib-zc/time.html), [文件操作函数](http://linuxmail.cn/lib-zc/file.html)
* [OPENSSL函数, 线程安全,异步SSL,支持SNI](http://linuxmail.cn/lib-zc/openssl.html)
* [IO流(STREAM),支持SSL](http://linuxmail.cn/lib-zc/stream.html)
* [master/server服务管理框架, server包括异步IO模式和协程模式](http://linuxmail.cn/lib-zc/master.html)
* [异步IO开发框架, 包括 TRIGER, 异步IO, 缓存读写, 定时器, 支持SSL](http://linuxmail.cn/lib-zc/aio.html)
* [协程开发框架, 支持文件类(read, lseek, link等)操作协程化,支持锁和条件](http://linuxmail.cn/lib-zc/coroutine.html)
* [IO管道(代理)库, 协程风格和异步IO风格](http://linuxmail.cn/lib-zc/iopipe.html)
* [JSON 解析库](http://linuxmail.cn/lib-zc/json.html), [NCR 解析](http://linuxmail.cn/lib-zc/encode.html)
* [MIME 邮件解析库](http://linuxmail.cn/lib-zc/mime.html), [TNEF 邮件解析库](http://linuxmail.cn/lib-zc/tnef.html), [BASE64/QUOTED-PRINTABLE](http://linuxmail.cn/lib-zc/encode.html)
* [REDIS 客户端(支持集群)](http://linuxmail.cn/lib-zc/redis_client.html), [MEMCACHE 客户端](http://linuxmail.cn/lib-zc/memcache_client.html)
* [httpd服务器库](http://linuxmail.cn/lib-zc/httpd.html), [httpd服务开发通用模版](https://gitee.com/linuxmail/lib-zc/blob/master/sample/http/general_coroutine_server_httpd.c), [URL编解码](http://linuxmail.cn/lib-zc/url.html)
* [一个全新的CONST DB库](http://linuxmail.cn/lib-zc/cdb.html)
* SQLITE3 代理服务器/客户端
* 类 REDIS 服务器端
* [一种简单的序列号](http://linuxmail.cn/lib-zc/license.html)

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
