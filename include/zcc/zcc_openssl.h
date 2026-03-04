/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_OPENSSL___
#define ZCC_LIB_INCLUDE_OPENSSL___

extern "C"
{
#ifndef HEADER_OPENSSL_TYPES_H
    typedef struct ssl_st SSL;
    typedef struct ssl_ctx_st SSL_CTX;
#endif // HEADER_OPENSSL_TYPES_H
}

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;
zcc_general_namespace_begin(openssl);

ZCC_LIB_API extern bool var_debug_mode;
ZCC_LIB_API extern bool var_disable_server_tls1_0;
ZCC_LIB_API extern bool var_disable_client_tls1_0;
// openssl 环境初始化, 执行完后, 可多线程操作openssl
ZCC_LIB_API void env_init(void);
ZCC_LIB_API void env_fini(void);

// 创建服务端 SSL_CTX
ZCC_LIB_API SSL_CTX *SSL_CTX_create_client(void);
// 创建客户端 SSL_CTX
ZCC_LIB_API SSL_CTX *SSL_CTX_create_server(const char *cert_file, const char *key_file);

// sni 处理
ZCC_LIB_API void SSL_CTX_set_sni_handler(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername));

// 微不足道的封装
ZCC_LIB_API void SSL_CTX_free(SSL_CTX *ctx);
ZCC_LIB_API void get_error(unsigned long *ecode, char *buf, int buf_len);
ZCC_LIB_API SSL *SSL_create(SSL_CTX *ctx, int fd);
ZCC_LIB_API void SSL_free(SSL *ssl);
ZCC_LIB_API int SSL_get_fd(SSL *ssl);

// 超时操作
ZCC_LIB_API int timed_connect(SSL *ssl, int wait_timeout);
ZCC_LIB_API int timed_accept(SSL *ssl, int wait_timeout);
ZCC_LIB_API int timed_shutdown(SSL *ssl, int wait_timeout);
ZCC_LIB_API int timed_read(SSL *ssl, void *buf, int len, int wait_timeout);
ZCC_LIB_API int timed_write(SSL *ssl, const void *buf, int len, int wait_timeout);

zcc_general_namespace_end(openssl);
zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_OPENSSL___
