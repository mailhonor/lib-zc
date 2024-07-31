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

#ifndef HEADER_OPENSSL_TYPES_H
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
#endif // HEADER_OPENSSL_TYPES_H

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;
zcc_general_namespace_begin(openssl);

extern bool var_debug_mode;
extern bool var_disable_server_tls1_0;
extern bool var_disable_client_tls1_0;
void env_init(void);
void env_fini(void);
SSL_CTX *SSL_CTX_create_client(void);
SSL_CTX *SSL_CTX_create_server(const char *cert_file, const char *key_file);
void SSL_CTX_set_sni_handler(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername));
void SSL_CTX_free(SSL_CTX *ctx);
void get_error(unsigned long *ecode, char *buf, int buf_len);
SSL *SSL_create(SSL_CTX *ctx, int fd);
void SSL_free(SSL *ssl);
int SSL_get_fd(SSL *ssl);
int timed_connect(SSL *ssl, int wait_timeout);
int timed_accept(SSL *ssl, int wait_timeout);
int timed_shutdown(SSL *ssl, int wait_timeout);
int timed_read(SSL *ssl, void *buf, int len, int wait_timeout);
int timed_write(SSL *ssl, const void *buf, int len, int wait_timeout);

zcc_general_namespace_end(openssl);
zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_OPENSSL___
