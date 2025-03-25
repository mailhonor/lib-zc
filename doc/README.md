
# 简介
LIB-ZC 是一个Linux平台通用 C/C++ 扩展库


## 模块列表 (C++ 版本)

* [通用配置](./config_cpp.md), [通用命令行参数](./main_argument_cpp.md), [通用日志](./log_cpp.md)
* [BASE64/QUOTED-PRINTABLE/HEX/NCR](./encode_cpp.md), [URL](./url_cpp.md)
* [字符集转码, 字符集探测](./charset_cpp.md)
* [常见的IO函数, 超时IO函数](./io_cpp.md), [TCP SOCKET](./tcp_socket_cpp.md), [DNS/IP地址/MAC地址](./dns_cpp.md)
* [字符串函数](./string_cpp.md), [时间函数](./time_cpp.md)
* [操作系统函数](./os_cpp.md), [文件操作函数](./file_cpp.md)
* [OPENSSL函数, 线程安全,异步SSL,支持SNI](./openssl_cpp.md)
* [IO流(STREAM),支持SSL](./stream_cpp.md)
* [master/server服务管理框架, server包括异步IO模式和协程模式](./master_cpp.md)
* [异步IO开发框架, 包括 TRIGER, 异步IO, 缓存读写, 定时器, 支持SSL](./aio_cpp.md)
* [协程开发框架, 支持文件类(read, lseek, link等)操作协程化,支持锁和条件](./coroutine.md)
* [IO管道(代理)库, 协程风格和异步IO风格](./iopipe_cpp.md)
* [MIME 邮件解析库](./mime_cpp.md), [TNEF 邮件解析库](./tnef_cpp.md), [BASE64/QUOTED-PRINTABLE](./encode_cpp.md)
* [REDIS 客户端(支持集群)](./redis_client_cpp.md)
*  [MEMCACHE 客户端](./memcache_client_cpp.md)
* [httpd服务器库](./httpd_cpp.md), [URL编解码](./url.md)
* [IMAP 客户端](../blob/master/include/zcc/zcc_imap.h)
* [POP 客户端](../blob/master/include/zcc/zcc_pop.h)
* [JSON 解析库](./json_cpp.md)
* [SQLITE3封装](../blob/master/include/zcc/zcc_sqlite3.h)
* [一个全新的CONST DB库](./cdb_cpp.md)
* [多关键字搜索](./msearch_cpp.md)
* SQLITE3 代理服务器/客户端
* 类 REDIS 服务器端

## 模块列表 (C 版本)

* [通用配置](./config.md), [通用命令行参数](./main_argument.md), [通用日志](./log.md)
* [链表/容器](./list.md), [链表/数据结构](./link.md), [链表/宏](./macro_link.md), [RBTREE/数据结构](./rbtree.md), [词典](./dict.md), [MAP](./map.md), [VECTOR](./vector.md), [ARGV](./argv.md), [不定长字符串](./buf.md)
* [BASE64/QUOTED-PRINTABLE/HEX/NCR](./encode.md), [URL](./url.md)
* [字符集转码, 字符集探测](./charset.md)
* [常见的IO函数](./io.md), [超时IO函数](./timed_io.md), [TCP SOCKET](./tcp_socket.md), [DNS/IP地址/MAC地址](./dns.md)
* [字符串函数](./string.md), [时间函数](./time.md), [文件操作函数](./file.md)
* [OPENSSL函数, 线程安全,异步SSL,支持SNI](./openssl.md)
* [IO流(STREAM),支持SSL](./stream.md)
* [master/server服务管理框架, server包括异步IO模式和协程模式](./master.md)
* [异步IO开发框架, 包括 TRIGER, 异步IO, 缓存读写, 定时器, 支持SSL](./aio.md)
* [协程开发框架, 支持文件类(read, lseek, link等)操作协程化,支持锁和条件](./coroutine.md)
* [IO管道(代理)库, 协程风格和异步IO风格](./iopipe.md)
* [JSON 解析库](./json.md), [NCR 解析](./encode.md)
* [MIME 邮件解析库](./mime.md), [TNEF 邮件解析库](./tnef.md), [BASE64/QUOTED-PRINTABLE](./encode.md)
* [REDIS 客户端(支持集群)](./redis_client.md), [MEMCACHE 客户端](./memcache_client.md)
* [httpd服务器库](./httpd.md), [httpd服务开发通用模版](../blob/master/sample/http/general_coroutine_server_httpd.c), [URL编解码](./url.md)
* [一个全新的CONST DB库](./cdb.md)
* [多关键字搜索](./msearch.md)
* SQLITE3 代理服务器/客户端
* 类 REDIS 服务器端
* [一种简单的序列号](./license.md)
