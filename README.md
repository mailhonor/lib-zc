
# 简介

LIB-ZC 是一个Linux平台通用C/C++扩展库.

兼容 Windows/Macos, 支持 GCC/CLANG/MINGW/MSVC, 支持 X64/AMD64/ARM64


## 模块列表

* 通用配置, 通用命令行参数, 通用日志
* BASE64/QUOTED-PRINTABLE/HEX/NCR, URL
* 字符集转码, 字符集探测
* 常见的IO函数, 超时IO函数, TCP SOCKET, DNS/IP地址/MAC地址
* 字符串函数, 时间函数
* 操作系统函数, 文件操作函数
* OPENSSL函数, 线程安全,异步SSL,支持SNI
* IO流(STREAM),支持SSL
* master/server服务管理框架, server包括异步IO模式和协程模式
* 异步IO开发框架, 包括 TRIGER, 异步IO, 缓存读写, 定时器, 支持SSL
* 协程开发框架, 支持文件类(read, lseek, link等)操作协程化,支持锁和条件
* IO管道(代理)库, 协程风格和异步IO风格
* MIME 邮件解析库, TNEF 邮件解析库, BASE64/QUOTED-PRINTABLE
* REDIS 客户端(支持集群)
* MEMCACHE 客户端
* httpd服务器库, URL编解码
* IMAP 客户端
* POP 客户端
* JSON 解析库
* SQLITE3封装
* 一个全新的CONST DB库
* 多关键字搜索
* SQLITE3 代理服务器/客户端
* 类 REDIS 服务器端

## 连接或监听地址
* "domain_socket_somepath"
* "somedomain:port"

## 时间/超时
* 如无特别说明, 所有的时间单位都是秒
* 如果是毫秒, 函数名或形参会明确提示
* 实参取值-1则表示无限长

## 返回值
* 和(网络)io相关的函数, 如果返回值类型是int, 如果返回 < 0, 表示出错

## 编译

cmake 环境, 推荐 vscode 开发和编译

得到: libzc.a(基础库) 和 libzc_coroutine.a(协程库)

// Linux平台 make 亦可

## 帮助文档

在 doc/ 目录下

## 使用
源码 cpp_sample/, sample/ 下有大量例子,可供参考

libzc.a 在源码目录下, include 文件 在 include/zcc/*.h

```shell
$ cat a.cpp 
#include "zcc/zcc_stdlib.h"
int main(int argc, char **argv)
{
  /* foo(); */
  return 0;
}
$ gcc a.cpp ./libzc.a
```
