/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-18
 * ================================
 */

#pragma GCC diagnostic ignored "-Wunused-function"

#define zpthread_lock(l)    if(l){if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}
#define zpthread_unlock(l)  if(l){if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}

#include "zc.h"
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

zbool_t zvar_openssl_debug = 0;

/* {{{ pthread safe */
static pthread_mutex_t *var_pthread_safe_lock_vec = 0;
static void pthread_safe_locking_fn(int mode, int n, const char *file, int line)
{
  if(mode & CRYPTO_LOCK) {
    zpthread_lock(var_pthread_safe_lock_vec + n);
  } else {
    zpthread_unlock(var_pthread_safe_lock_vec + n);
  }
}

static unsigned long pthread_safe_id_fn(void)
{
  return ((unsigned long)pthread_self());
}

struct CRYPTO_dynlock_value { 
    pthread_mutex_t mutex; 
};

static struct CRYPTO_dynlock_value *pthread_safe_dynlock_create_fn(const char *file, int line)
{
    struct CRYPTO_dynlock_value *v = (struct CRYPTO_dynlock_value *)zmalloc(sizeof(struct CRYPTO_dynlock_value));
    pthread_mutex_init(&(v->mutex), NULL);
    return v;
}

static void pthread_safe_dynlock_lock_fn(int mode, struct CRYPTO_dynlock_value *value, const char *file, int line)
{
    if (mode &CRYPTO_LOCK) {
        pthread_mutex_lock(&(value->mutex));
    } else {
        pthread_mutex_unlock(&value->mutex);
    }
}

static void pthread_safe_dynlock_destroy_fn(struct CRYPTO_dynlock_value *value, const char *file, int line)
{
    pthread_mutex_destroy(&(value->mutex));
    zfree(value);
}

static void pthread_safe_setup(void)
{
    var_pthread_safe_lock_vec = (pthread_mutex_t *)zmalloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    for(int i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_init(var_pthread_safe_lock_vec + i, 0);
    }
    CRYPTO_set_id_callback(pthread_safe_id_fn);
    CRYPTO_set_locking_callback(pthread_safe_locking_fn);

    CRYPTO_set_dynlock_create_callback(pthread_safe_dynlock_create_fn);
    CRYPTO_set_dynlock_lock_callback(pthread_safe_dynlock_lock_fn);
    CRYPTO_set_dynlock_destroy_callback(pthread_safe_dynlock_destroy_fn);
}

static void pthread_safe_cleanup(void)
{
    if (!var_pthread_safe_lock_vec) {
        return;
    }

    CRYPTO_set_id_callback(0);
    CRYPTO_set_locking_callback(0);
    CRYPTO_set_dynlock_create_callback(0);
    CRYPTO_set_dynlock_lock_callback(0);
    CRYPTO_set_dynlock_destroy_callback(0);

    for(int i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_destroy(var_pthread_safe_lock_vec + i);
    }
    zfree(var_pthread_safe_lock_vec);
    var_pthread_safe_lock_vec = 0;
}
/* }}} */

static int ___openssl_init = 0;
void zopenssl_init(void)
{
    if (___openssl_init) {
        return;
    }
    ___openssl_init = 1;
    SSL_library_init();
    pthread_safe_setup();
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
}

void zopenssl_fini(void)
{
    if (___openssl_init == 0) {
        return;
    }
#if 1
    pthread_safe_cleanup();
    ___openssl_init = 0;
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
    pthread_safe_cleanup();
    ___openssl_init = 0;
#endif
}

void zopenssl_phtread_fini(void)
{
    if (___openssl_init == 0) {
        return;
    }
#if 0
    /* Deprecated */
    ERR_remove_state(0);
#endif
}

static SSL_CTX *zopenssl_SSL_CTX_create_by_method(const SSL_METHOD *method, int server_or_client)
{
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        zfatal("SSL_CTX_new");
    }
    SSL_CTX_set_options(ctx, SSL_OP_ALL);
    SSL_CTX_set_options(ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);

    SSL_CTX_clear_options(ctx, SSL_OP_NO_SSLv2);
    SSL_CTX_clear_options(ctx, SSL_OP_NO_SSLv3);
    SSL_CTX_clear_options(ctx, SSL_OP_NO_TLSv1);
    SSL_CTX_clear_options(ctx, SSL_OP_NO_TLSv1_1);
    SSL_CTX_clear_options(ctx, SSL_OP_NO_TLSv1_2);

    SSL_CTX_set_read_ahead(ctx, 1);

#if OPENSSL_VERSION_NUMBER > 0x10100080L
    SSL_CTX_set_security_level(ctx, 1);
#endif
    SSL_CTX_set_cipher_list(ctx, "HIGH:SSLv3:SSLv2:SSLv1:TLSv1.3:TLSv1.2:TLSv1.1:TLSv1.0:!aNULL:!eNULL");
    
    return ctx;
}

SSL_CTX *zopenssl_SSL_CTX_create_server(void)
{
     return zopenssl_SSL_CTX_create_by_method(SSLv23_server_method(), 1);
}

SSL_CTX *zopenssl_SSL_CTX_create_client(void)
{
     return zopenssl_SSL_CTX_create_by_method(SSLv23_client_method(), 0);
}

int zopenssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file)
{
    ERR_clear_error();
    if ((!cert_file) || (SSL_CTX_use_certificate_chain_file(ctx, cert_file) <= 0)) {
        return (-1);
    }
    if ((!key_file) || (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0)) {
        return (-1);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        return (-1);
    }

    return 1;
}

void zopenssl_SSL_CTX_free(SSL_CTX * ctx)
{
    SSL_CTX_free(ctx);
}

void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len)
{
    unsigned long ec;
    ec = ERR_get_error();
    if (ecode) {
        *ecode = ec;
    }

    if (buf) {
        ERR_error_string_n(ec, buf, buf_len);
    }
}

SSL *zopenssl_SSL_create(SSL_CTX * ctx, int fd)
{
    SSL *ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    return ssl;
}

void zopenssl_SSL_free(SSL * ssl)
{
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
}

int zopenssl_SSL_get_fd(SSL *ssl)
{
    return SSL_get_fd(ssl);
}

#define ___Z_SSL_TIMED_DO(excute_sentence)  \
    if (timeout < 0) { \
        timeout = zvar_max_timeout; \
    } \
    if (timeout == 0) { \
        return excute_sentence; \
    } \
    int _fd = SSL_get_fd(ssl); \
    int ret, err; \
    long cirtical_time, left_time; \
    cirtical_time = zmillisecond()+(timeout)*1000; \
    for (;;) { \
        ret = excute_sentence; \
        if (ret > 0) { break; } \
        err = SSL_get_error(ssl, ret); \
        if (err == SSL_ERROR_WANT_WRITE) { \
            if ((left_time = (cirtical_time-zmillisecond()+1)) < 1) { ret = -1; break; } \
            if (ztimed_write_wait_millisecond(_fd, left_time) < 1) { ret=-1; break; }\
        } else if (err == SSL_ERROR_WANT_READ) { \
            if ((left_time = (cirtical_time-zmillisecond()+1)) < 1) { ret = -1; break;} \
            if (ztimed_read_wait_millisecond(_fd, left_time) < 1) { ret = -1; break; } \
        } else { \
            if (zvar_openssl_debug) { zinfo("openssl: found error ret=%d, status=%d", ret, err); } \
            { ret = -1; break; } \
        } \
    } 

int zopenssl_timed_connect(SSL * ssl, int timeout)
{
    ___Z_SSL_TIMED_DO(SSL_connect(ssl));
    return (ret==1?1:-1);
}

int zopenssl_timed_accept(SSL * ssl, int timeout)
{
    ___Z_SSL_TIMED_DO(SSL_accept(ssl));
    return (ret==1?1:-1);
}

int zopenssl_timed_shutdown(SSL * ssl, int timeout)
{
    ___Z_SSL_TIMED_DO(SSL_shutdown(ssl));
    return (ret==1?1:-1);
}

int zopenssl_timed_read(SSL * ssl, void *buf, int len, int timeout)
{
    ___Z_SSL_TIMED_DO(SSL_read(ssl, buf, len));
    return ret;
}

int zopenssl_timed_write(SSL * ssl, const void *buf, int len, int timeout)
{
    ___Z_SSL_TIMED_DO(SSL_write(ssl, buf, len));
    return ret;
}

/* Local variables:
* End:
* vim600: fdm=marker
*/
