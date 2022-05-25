/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-01-16
 * ================================
 */

#define ZCC_AIO_NEED_PUBLIC
#include "./aio.cpp"

namespace zcc
{

static void _aio_callback_do(zaio_t *engine)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine);
    ac->callback_();
}

ssl_aio::ssl_aio(SSL *ssl, aio_base *aiobase)
{
    engine_ = 0;
    rebind_SLL(ssl, aiobase);
}

void ssl_aio::rebind_SLL(SSL *ssl, aio_base *aiobase)
{
    close(true);
    aio_context *ac = new aio_context();
    ac->aiobase_ = (aiobase?aiobase:var_default_aio_base);
    engine_ = zaio_create_by_ssl(ssl, ((aio_base_public *)(ac->aiobase_))->get_aio_base());
    zaio_set_context(engine_, ac);
}

void ssl_aio::tls_connect(SSL_CTX *ctx, std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_tls_connect(engine_, ctx, _aio_callback_do);
}

void ssl_aio::tls_accept(SSL_CTX * ctx, std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_tls_accept(engine_, ctx, _aio_callback_do);
}

SSL *ssl_aio::get_ssl()
{
    return zaio_get_ssl(engine_);
}

} /* namespace zcc */

