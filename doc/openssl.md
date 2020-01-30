# OEPNSSL初始化/线程安全/SSL上下文/超时读写/SNI

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了openssl常见操作

## 特别说明

LIB-ZC中有一些功能模块, 使用了openssl,包括 stream, aio等

- 这些模块只使用了openssl中的 **SSL_CTX** / **SSL**
- 也就是说,LIB-ZC并没有对SSL_CTX/SSL做封装使用
- 使用者可以自己生成 这些SSL_CTX/SSL, 而不必依赖本文提供的方法

## 函数

### 初始化openssl环境

```
/* 初始化环境 */
void zopenssl_init(void);\
/* 释放环境 */
void zopenssl_fini(void);
```

执行 **zopenssl_init()** 初始化, 同时支持了**openssl线程安全**

### 创建客户端 SSL_CTX

```
SSL_CTX *zopenssl_SSL_CTX_create_client(void);
```

### 创建服务端 SSL_CTX

```
/* cert_file: 证书文件, key_file: 私钥文件 */
SSL_CTX *zopenssl_SSL_CTX_create_server(const char *cert_file, const char *key_file);
```

### 微不足道的封装

```
/* 释放 SSL_CTX */
void zopenssl_SSL_CTX_free(SSL_CTX *ctx);

/* 获取错误, *ecode: 错误码, buf: 错误信息, buf_len: 错误信息buf长度 */
void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len);

/* 创建 SSL */
SSL *zopenssl_SSL_create(SSL_CTX *ctx, int fd); 
void zopenssl_SSL_free(SSL *ssl);

/* 获取 fd */
int zopenssl_SSL_get_fd(SSL *ssl);
```

### 带超时的connect/accept/shutdown/read/write
 
- 下面都是带超时的函数
- timeout单位是秒
- 返回 <0 表示失败(或超时)
- 支持超时(timeout)的前提是: SSL的fd是非阻塞的

```
/* 返回 1: 成功 */
int zopenssl_timed_connect(SSL *ssl, int timeout);

/* 返回 1: 成功 */
int zopenssl_timed_accept(SSL *ssl, int timeout);

/* 返回 1: 成功 */
int zopenssl_timed_shutdown(SSL *ssl, int timeout);

/* 返回值: 请参考 SSL_read */
int zopenssl_timed_read(SSL *ssl, void *buf, int len, int timeout);

/* 返回值: 请参考 SSL_write
int zopenssl_timed_write(SSL *ssl, const void *buf, int len, int timeout);

```

### 支持 SNI

所谓 **SNI**, 既 TLS extension, Server Name Indication

```
/* get_ssl_ctx_by_server_name 为回调函数, 其参数为servername, 根据servername 返回合适的 SSL_CTX */
void zopenssl_SSL_CTX_support_sni(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername));
```

## 应用

zstream_t 是一个流, 提供了对SSL的封装

zaio_t 是异步io框架, 提供了对异步SSL的封装
