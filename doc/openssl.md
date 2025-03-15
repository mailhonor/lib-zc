<A name="readme_md" id="readme_md"></A>

## OEPNSSL初始化/线程安全/SSL上下文/超时读写/SNI, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了openssl重要操作

[C++版本](./openssl_cpp.md)

## 特别说明

* [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 并没有对openssl的数据结构(SSL_CTX/SSL)做任何封装
* 尽管[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 内部很多模块使用openssl, 使用者也可以自己生成这些 SSL_CTX/SSL, 而不必依赖本文提供的方法

## 初始化 openssl 环境

### void zopenssl_init(void);

* 初始化环境
* 此时启用了openssl 线程安全
* 一般在进程开始的时候执行

### void zopenssl_fini(void);

* 释放环境
* 一般在进程结束前执行(当然, 进程都结束了, 就没必要执行了)

## SSL_CTX

### SSL_CTX *zopenssl_SSL_CTX_create_client(void);

* 创建客户端 SSL_CTX

### SSL_CTX *zopenssl_SSL_CTX_create_server(const char *cert_file, const char *key_file);

* 创建服务端 SSL_CTX
* cert_file: 证书文件
* key_file: 私钥文件
* 返回 0: 失败


## 微不足道的封装

本节的函数封装仅仅是为了方便

### void zopenssl_SSL_CTX_free(SSL_CTX *ctx);

* 释放 SSL_CTX

### void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len);

* 获取错误
* *ecode: 存储错误码
* buf: 错误信息
* buf_len: 错误信息buf长度

### SSL *zopenssl_SSL_create(SSL_CTX *ctx, int fd);

* 创建 SSL 

### void zopenssl_SSL_free(SSL *ssl);

* 释放 SSL 

### int zopenssl_SSL_get_fd(SSL *ssl);

* 获取 fd 

## 带超时的connect/accept/shutdown/read/write
 
支持超时(timeout)的前提是: SSL 的 fd 是非阻塞的

### int zopenssl_timed_connect(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

* 带超时的 SSL_connect
* 返回 1: 成功
* read_wait_timeout: 读超时
* write_wait_timeout: 写超时

### int zopenssl_timed_accept(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

* 带超时的 SSL_accept
* 返回 1: 成功
* read_wait_timeout: 读超时
* write_wait_timeout: 写超时

### int zopenssl_timed_shutdown(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

* 带超时的 SSL_shutdown
* 返回 1: 成功
* read_wait_timeout: 读超时
* write_wait_timeout: 写超时

### int zopenssl_timed_read(SSL *ssl, void *buf, int len, int read_wait_timeout, int write_wait_timeout);

* 带超时的 SSL_read
* 返回 &gt;0: 读成功字节数, 成功
* read_wait_timeout: 读超时
* write_wait_timeout: 写超时

### int zopenssl_timed_write(SSL *ssl, const void *buf, int len, int read_wait_timeout, int write_wait_timeout);

* 带超时的 SSL_write
* 返回 &gt;0: 写成功字节数, 成功
* read_wait_timeout: 读超时
* write_wait_timeout: 写超时


## 支持 SNI

所谓 **SNI**, 既 TLS extension, Server Name Indication

### void zopenssl_SSL_CTX_support_sni(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername));
* 设置支持 SNI 的函数
* get_ssl_ctx_by_server_name 为回调函数, 其参数为servername<BR />根据 servername 返回合适的 SSL_CTX

PS: 所谓 servername:

```
const char *servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
```

### SNI 简单流程

第一, 创建一个服务端 (SSL_CTX *)ctx

第二, 执行一次:

```
SSL_CTX_set_tlsext_servername_callback(ctx,  _change_ssl_ctx_by_servername);

/* _change_ssl_ctx_by_servername 是自定义函数 */
SSL_CTX *_change_ssl_ctx_by_servername(const char *servername)
{
    if (!strcmp(servername, "linuxmmail.cn")) {
        return ssl_ctx_linuxmail_cn;
    }
    if (!strcmp(servername, "www2.linuxmmail.cn")) {
        return ssl_ctx_linuxmail_cn2;
    }
    return 0;
}
```

第三, SSL握手前

通过执行 _change_ssl_ctx_by_servername(servername) 得到和 servername 匹配的 SSL_CTX *, 并替换

### SNI, 原理

* https://gitee.com/linuxmail/codes/d8ymz6xn1b2o5ts9gjqaf50

## 内部应用

* [zstream_t](./stream.md) 是流实现, 支持 OPENSSL
* [zaio_t](./aio.md) 是异步 IO 框架, 支持 OPENSSL

