/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-01-16
 * ================================
 */

#include "zc.h"

namespace zcc
{

class aio_context {
public:
    zinline aio_context() { }
    zinline ~aio_context() { }
    std::function<void()> callback_;
    aio_base *aiobase_;
};

class aio_base_public: public aio_base {
public:
    zinline aio_base_public() {}
    zinline ~aio_base_public() {}
    zinline zaio_base_t *get_aio_base() { return engine_; }
};

} /* namespace zcc */

#ifndef ZCC_AIO_NEED_PUBLIC
namespace zcc
{

aio_base *var_default_aio_base = 0;

static void _aio_callback_do(zaio_t *engine)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine);
    ac->callback_();
}

aio::aio()
{
    engine_ = 0;
    rebind_fd(-1, 0);
}

aio::aio(int fd, aio_base *aiobase)
{
    engine_ = 0;
    rebind_fd(fd, aiobase);
}

aio::~aio()
{
    close(true);
}

void aio::close(bool close_fd_and_release_ssl)
{
    if (!engine_) {
        return;
    }
    aio_context *ac = (aio_context *)zaio_get_context(engine_);
    if (ac) {
        delete ac;
    }
    zaio_free(engine_, close_fd_and_release_ssl?1:0);
    engine_ = 0;
}

void aio::rebind_fd(int fd, aio_base *aiobase)
{
    close(true);
    aio_context *ac = new aio_context();
    ac->aiobase_ = (aiobase?aiobase:var_default_aio_base);
    engine_ = zaio_create_by_fd(fd, ((aio_base_public *)(ac->aiobase_))->get_aio_base());
    zaio_set_context(engine_, ac);
}

void aio::rebind_aio_base(aio_base *aiobase)
{
    zaio_rebind_aio_base(engine_, ((aio_base_public *)(aiobase))->get_aio_base());
}

void aio::readable(std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_readable(engine_, _aio_callback_do);
}

void aio::writeable(std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_writeable(engine_, _aio_callback_do);
}

void aio::read(int max_len, std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_read(engine_, max_len, _aio_callback_do);
}

void aio::readn(int strict_len, std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_readn(engine_, strict_len, _aio_callback_do);
}

void aio::read_delimiter(int delimiter, int max_len, std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_read_delimiter(engine_, delimiter, max_len, _aio_callback_do);
}

void aio::cache_flush(std::function<void()> callback)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_cache_flush(engine_, _aio_callback_do);
}

void aio::sleep(std::function<void()> callback, int timeout)
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    ac->callback_ = callback;
    zaio_sleep(engine_, _aio_callback_do, timeout);
}

aio_base *aio::get_aio_base()
{
    aio_context *ac =(aio_context *)zaio_get_context(engine_);
    return ac->aiobase_;
}

void aio::get_read_cache(std::string &bf, int strict_len)
{
    char buf[1024 + 1];
    while (strict_len > 0) {
        int len = (strict_len > 1024?1024:strict_len);
        zaio_get_read_cache_to_buf(engine_, buf, len);
        bf.append(buf, len);
        strict_len -= len;
    }
}

void aio::get_write_cache(std::string &bf, int strict_len)
{
    char buf[1024 + 1];
    while (strict_len > 0) {
        int len = (strict_len > 1024?1024:strict_len);
        zaio_get_write_cache_to_buf(engine_, buf, len);
        bf.append(buf, len);
        strict_len -= len;
    }
}

void aio::cache_printf_1024(const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, fmt);
    len = zvsnprintf(buf, 1024, fmt, ap);
    len = ((len<1024)?len:(1024-1));
    va_end(ap);
    cache_write(buf, len);
}

/* aio_base */
extern "C"
{
void zaio_base_set_context_for_child(zaio_base_t *eb, const void *ctx);
void *zaio_base_get_context_for_child(zaio_base_t *eb);
}

class aio_base_context {
public:
    aio_base_context();
    ~aio_base_context();
    aio_base *current_pthread_aio_base_;
    std::function<void()> loop_fn_;
    bool need_release_base_;
};

aio_base_context::aio_base_context()
{
    current_pthread_aio_base_ = 0;
    need_release_base_ = false;
}

aio_base_context::~aio_base_context()
{
}

static void _aio_base_loop_do(zaio_base_t *engine)
{
    aio_base_context *ac =(aio_base_context *)zaio_base_get_context_for_child(engine);
    ac->loop_fn_();
}

aio_base::aio_base()
{
    engine_ = zaio_base_create();
    aio_base_context *ac = new aio_base_context();
    zaio_base_set_context_for_child(engine_, ac);
    ac->need_release_base_ = true;
}

aio_base::aio_base(zaio_base_t *ab)
{
    engine_ = ab;
    aio_base_context *ac = new aio_base_context();
    zaio_base_set_context_for_child(engine_, ac);
    ac->need_release_base_ = false;
}

aio_base::~aio_base()
{
    do {
        if (!engine_) {
            break;
        }
        bool need_release_base = false;
        aio_base_context *ac = (aio_base_context *)zaio_base_get_context_for_child(engine_);
        if (ac) {
            if (ac->need_release_base_) {
                need_release_base = true;
            }
            delete ac;
        }
        zaio_base_set_context_for_child(engine_, 0);
        if (need_release_base) {
            zaio_base_free(engine_);
        }
    } while (0);
}

void aio_base::set_loop_fn(std::function<void()> callback)
{
    aio_base_context *ac =(aio_base_context *)zaio_base_get_context_for_child(engine_);
    ac->loop_fn_ = callback;
    zaio_base_set_loop_fn(engine_, _aio_base_loop_do);
    if (callback == 0) {
        zaio_base_set_loop_fn(engine_, 0);
    }
}

void aio_base::run()
{
    aio_base_context *ac =(aio_base_context *)zaio_base_get_context_for_child(engine_);
    ac->current_pthread_aio_base_ = this;
    zaio_base_run(engine_);
}

void aio_base::stop_notify()
{
    zaio_base_stop_notify(engine_);
}

void aio_base::touch()
{
    zaio_base_touch(engine_);
}

static void _aio_base_timer_do(aio *a, std::function<void()> f)
{
    f();
    delete a;
}

void aio_base::timer(std::function<void()> callback, int timeout)
{
    aio *a = new aio(-1, this);
    a->sleep(std::bind(_aio_base_timer_do, a, callback), timeout);
}

aio_base *aio_base_get_current_pthread_aio_base()
{
    zaio_base_t *ab = zaio_base_get_current_pthread_aio_base();
    if (!ab) {
        return 0;
    }
    aio_base_context *ac =(aio_base_context *)zaio_base_get_context_for_child(ab);
    if (!ac) {
        return 0;
    }
    return ac->current_pthread_aio_base_;
}

} /* namespace zcc */
#endif /* ZCC_AIO_NEED_PUBLIC */

