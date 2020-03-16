<A name="readme_md" id="readme_md"></A>

# 简介
LIB-ZC 是一个Linux平台通用C扩展库

开源, 仓库地址 https://gitee.com/linuxmail/lib-zc

帮助文档 [http://linuxmail.cn/lib-zc/](http://linuxmail.cn/lib-zc/)

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
