/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-18
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_openssl.h"
#include <thread>
#include <mutex>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>

#ifndef CRYPTO_num_locks
extern "C"
{
    struct CRYPTO_dynlock_value
    {
        std::mutex mutex;
    };
}
#endif // CRYPTO_num_locks

zcc_namespace_begin;
zcc_general_namespace_begin(openssl);

bool var_debug_mode = false;
bool var_disable_server_tls1_0 = 0;
bool var_disable_client_tls1_0 = 0;

#ifdef CRYPTO_num_locks
#define thread_safe_setup()
#define thread_safe_cleanup()
#else  // CRYPTO_num_locks
static std::mutex **thread_safe_lock_vec = 0;
static void thread_safe_locking_fn(int mode, int n, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        thread_safe_lock_vec[n]->lock();
    }
    else
    {
        thread_safe_lock_vec[n]->unlock();
    }
}

static unsigned long thread_safe_id_fn(void)
{
    return get_thread_id();
}

static struct CRYPTO_dynlock_value *thread_safe_dynlock_create_fn(const char *file, int line)
{
    struct CRYPTO_dynlock_value *v = new CRYPTO_dynlock_value();
    return v;
}

static void thread_safe_dynlock_lock_fn(int mode, struct CRYPTO_dynlock_value *value, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        value->mutex.lock();
    }
    else
    {
        value->mutex.unlock();
    }
}

static void thread_safe_dynlock_destroy_fn(struct CRYPTO_dynlock_value *value, const char *file, int line)
{
    if (value)
    {
        delete value;
    }
}

static void thread_safe_setup(void)
{
    thread_safe_lock_vec = new std::mutex *[CRYPTO_num_locks()];
    for (int i = 0; i < CRYPTO_num_locks(); i++)
    {
        thread_safe_lock_vec[i] = new std::mutex();
    }
    CRYPTO_set_id_callback(thread_safe_id_fn);
    CRYPTO_set_locking_callback(thread_safe_locking_fn);

    CRYPTO_set_dynlock_create_callback(thread_safe_dynlock_create_fn);
    CRYPTO_set_dynlock_lock_callback(thread_safe_dynlock_lock_fn);
    CRYPTO_set_dynlock_destroy_callback(thread_safe_dynlock_destroy_fn);
}

static void thread_safe_cleanup(void)
{
    if (!thread_safe_lock_vec)
    {
        return;
    }

    CRYPTO_set_id_callback(0);
    CRYPTO_set_locking_callback(0);
    CRYPTO_set_dynlock_create_callback(0);
    CRYPTO_set_dynlock_lock_callback(0);
    CRYPTO_set_dynlock_destroy_callback(0);

    for (int i = 0; i < CRYPTO_num_locks(); i++)
    {
        delete thread_safe_lock_vec[i];
    }
    delete thread_safe_lock_vec;
    thread_safe_lock_vec = 0;
}
#endif // CRYPTO_num_locks

// 应该在程序启动时初始化, 应该在主进程执行
static bool ___openssl_init = false;
void env_init(void)
{
    if (___openssl_init)
    {
        return;
    }
    ___openssl_init = true;
#if (OPENSSL_VERSION_NUMBER >= 0x10100003L)
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL);
    thread_safe_setup();
    SSL_library_init();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
#else
    OPENSSL_config(NULL);
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif
}

void env_fini(void)
{
    if (!___openssl_init)
    {
        return;
    }
#if 1
    ___openssl_init = false;
    thread_safe_cleanup();
    return;
#else
#if 0
    CONF_modules_unload(1)
    ENGINE_cleanup();
#endif
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
#if 0
    /* Deprecated */
    ERR_remove_state(0);
#endif
    sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
    thread_safe_cleanup();
    ___openssl_init = 0;
#endif
}

SSL_CTX *SSL_CTX_create_client(void)
{
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    const SSL_METHOD *method = TLS_client_method();
#else
    const SSL_METHOD *method = SSLv23_client_method();
#endif
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        zcc_error("SSL_CTX_new");
        return nullptr;
    }

    SSL_CTX_set_options(ctx, SSL_OP_ALL);

#ifdef SSL_CTX_set_min_proto_version
    if (var_disable_client_tls1_0)
    {
        SSL_CTX_set_min_proto_version(ctx, TLS1_1_VERSION);
    }
    else
    {
        SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION);
    }
    SSL_CTX_set_max_proto_version(ctx, TLS_MAX_VERSION);
#endif

#if OPENSSL_VERSION_NUMBER > 0x10100003L
    SSL_CTX_set_security_level(ctx, 1);
#endif

    return ctx;
}

SSL_CTX *SSL_CTX_create_server(const char *cert_file, const char *key_file)
{
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    const SSL_METHOD *method = TLS_server_method();
#else
    const SSL_METHOD *method = SSLv23_server_method();
#endif
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        zcc_error("SSL_CTX_new");
        return nullptr;
    }
    SSL_CTX_set_options(ctx, SSL_OP_ALL);

#ifdef SSL_CTX_set_min_proto_version
    if (var_disable_server_tls1_0)
    {
        SSL_CTX_set_min_proto_version(ctx, TLS1_1_VERSION);
    }
    else
    {
        SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION);
    }
    SSL_CTX_set_max_proto_version(ctx, TLS_MAX_VERSION);
#endif

    ERR_clear_error();
    if ((!cert_file) || (!key_file))
    {
        zcc_error("cert_file or key_file is null");
        goto err;
    }

    if (SSL_CTX_load_verify_locations(ctx, cert_file, NULL) != 1)
    {
        zcc_error("SSL_CTX_load_verify_locations");
        goto err;
    }

    if ((SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1) && (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) != 1))
    {
        zcc_error("SSL_CTX_use_certificate_chain_file AND SSL_CTX_use_certificate_file");
        goto err;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1)
    {
        zcc_error("SSL_CTX_use_PrivateKey_file");
        goto err;
    }

    if (SSL_CTX_check_private_key(ctx) != 1)
    {
        zcc_error("SSL_CTX_check_private_key");
        goto err;
    }

    return ctx;

err:
    ::SSL_CTX_free(ctx);
    return 0;
}

typedef SSL_CTX *(*get_ssl_ctx_by_server_name_fn_t)(const char *servername);

static int _change_ssl_ctx_by_servername(SSL *ssl, int *ad, void *arg)
{
    const char *servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (empty(servername))
    {
        return SSL_TLSEXT_ERR_NOACK;
    }

    get_ssl_ctx_by_server_name_fn_t get_ssl_ctx_by_server_name = (get_ssl_ctx_by_server_name_fn_t)arg;
    if (!get_ssl_ctx_by_server_name)
    {
        return SSL_TLSEXT_ERR_NOACK;
    }

    SSL_CTX *my_ctx = SSL_get_SSL_CTX(ssl);
    if (!my_ctx)
    {
        return SSL_TLSEXT_ERR_NOACK;
    }

    SSL_CTX *another_ctx = get_ssl_ctx_by_server_name(servername);
    if (!another_ctx)
    {
        return SSL_TLSEXT_ERR_NOACK;
    }

    if (another_ctx != my_ctx)
    {
        SSL_set_SSL_CTX(ssl, another_ctx);
    }

    return SSL_TLSEXT_ERR_OK;
}

void SSL_CTX_set_sni_handler(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername))
{
    do
    {
        if (SSL_CTX_set_tlsext_servername_callback(ctx, _change_ssl_ctx_by_servername) == 0)
        {
            zcc_warning("SSL_CTX_set_tlsext_servername_callback failure");
            break;
        }
        if (SSL_CTX_set_tlsext_servername_arg(ctx, (void *)get_ssl_ctx_by_server_name) == 0)
        {
            zcc_warning("SSL_CTX_set_tlsext_servername_arg failure");
            break;
        }
    } while (0);
}

void SSL_CTX_free(SSL_CTX *ctx)
{
    ::SSL_CTX_free(ctx);
}

void get_error(unsigned long *ecode, char *buf, int buf_len)
{
    unsigned long ec;
    ec = ERR_get_error();
    if (ecode)
    {
        *ecode = ec;
    }

    if (buf)
    {
        ERR_error_string_n(ec, buf, buf_len);
    }
}

SSL *SSL_create(SSL_CTX *ctx, int fd)
{
    SSL *ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    return ssl;
}

void SSL_free(SSL *ssl)
{
    if (ssl)
    {
        SSL_shutdown(ssl);
        ::SSL_free(ssl);
    }
}

int SSL_get_fd(SSL *ssl)
{
    return ::SSL_get_fd(ssl);
}

#define ___Z_SSL_TIMED_DO(excute_sentence, need_check_read_close)                            \
    if (wait_timeout == 0)                                                                   \
    {                                                                                        \
        return excute_sentence;                                                              \
    }                                                                                        \
    int _fd = SSL_get_fd(ssl);                                                               \
    int ret, err;                                                                            \
    for (;;)                                                                                 \
    {                                                                                        \
        ret = excute_sentence;                                                               \
        if (ret > 0)                                                                         \
        {                                                                                    \
            break;                                                                           \
        }                                                                                    \
        err = SSL_get_error(ssl, ret);                                                       \
        if (err == SSL_ERROR_WANT_WRITE)                                                     \
        {                                                                                    \
            if (timed_write_wait(_fd, wait_timeout) == 0)                                    \
            {                                                                                \
                ret = -1;                                                                    \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
        else if (err == SSL_ERROR_WANT_READ)                                                 \
        {                                                                                    \
            if (timed_read_wait(_fd, wait_timeout) == 0)                                     \
            {                                                                                \
                ret = -1;                                                                    \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            /* FIXME 这里有问题 */                                                           \
            if (need_check_read_close && (ret == 0) /* && (err == SSL_ERROR_ZERO_RETURN) */) \
            {                                                                                \
                {                                                                            \
                    ret = 0;                                                                 \
                    break;                                                                   \
                }                                                                            \
            }                                                                                \
            if (var_debug_mode)                                                              \
            {                                                                                \
                zcc_info("openssl: found error ret=%d, status=%d", ret, err);                \
            }                                                                                \
            {                                                                                \
                ret = -1;                                                                    \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
    }

int timed_connect(SSL *ssl, int wait_timeout)
{
    ___Z_SSL_TIMED_DO(::SSL_connect(ssl), 0);
    return (ret == 1 ? 1 : -1);
}

int timed_accept(SSL *ssl, int wait_timeout)
{
    ___Z_SSL_TIMED_DO(::SSL_accept(ssl), 0);
    return (ret == 1 ? 1 : -1);
}

int timed_shutdown(SSL *ssl, int wait_timeout)
{
    ___Z_SSL_TIMED_DO(::SSL_shutdown(ssl), 0);
    return (ret == 1 ? 1 : -1);
}

int timed_read(SSL *ssl, void *buf, int len, int wait_timeout)
{
    ___Z_SSL_TIMED_DO(::SSL_read(ssl, buf, len), 1);
    return ret;
}

int timed_write(SSL *ssl, const void *buf, int len, int wait_timeout)
{
    ___Z_SSL_TIMED_DO(::SSL_write(ssl, buf, len), 0);
    return ret;
}

zcc_general_namespace_end(openssl);
zcc_namespace_end;
