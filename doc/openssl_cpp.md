
## OEPNSSL初始化/线程安全/SSL上下文/超时读写/SNI, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了openssl重要操作

[C版本](./openssl.md)

## 特别说明

* [LIB-ZC](./README.md) 并没有对openssl的数据结构(SSL_CTX/SSL)做任何封装
* 尽管[LIB-ZC](./README.md) 内部很多模块使用openssl, 使用者也可以自己生成这些 SSL_CTX/SSL, 而不必依赖本文提供的方法

## 初始化 openssl 环境

执行本初始化后, openssl 就可以多线程操作了

```c++
zcc::openssl::env_init();
// zcc::openssl::env_fini();
```

## SSL_CTX

```c++
// 客户端
SSL_CTX *zcc::openssl::SSL_CTX_create_client(void);
// 服务器
SSL_CTX *zcc::openssl::SSL_CTX_create_server(const char *cert_file, const char *key_file);
```

## 微不足道的封装

```c++
namespace zcc {
namespace openssl {
void SSL_CTX_free(SSL_CTX *ctx);
void get_error(unsigned long *ecode, char *buf, int buf_len);
SSL *SSL_create(SSL_CTX *ctx, int fd);
void SSL_free(SSL *ssl);
int SSL_get_fd(SSL *ssl);
}
}
```

## 带超时的connect/accept/shutdown/read/write

```c++
namespace zcc {
namespace openssl {
int timed_connect(SSL *ssl, int wait_timeout);
int timed_accept(SSL *ssl, int wait_timeout);
int timed_shutdown(SSL *ssl, int wait_timeout);
int timed_read(SSL *ssl, void *buf, int len, int wait_timeout);
int timed_write(SSL *ssl, const void *buf, int len, int wait_timeout);
}
}
```

## 支持 SNI

所谓 **SNI**, 既 TLS extension, Server Name Indication

### SNI 简单流程

第一, 创建一个服务端 (SSL_CTX *)ctx

第二, 执行一次:

```c++

zcc::openssl::SSL_CTX_set_sni_handler(ctx, _change_ssl_ctx_by_servername);

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

* [zcc::iostream](./stream_cpp.md) 是流实现, 支持 OPENSSL
* [zcc::aio](./aio_cpp.md) 是异步 IO 框架, 支持 OPENSSL

