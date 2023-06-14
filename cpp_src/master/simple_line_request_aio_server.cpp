/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-10-15
 * ================================
 */

#ifdef __linux__

#include "zc.h"

namespace zcc
{

int simple_line_request_aio_server::var_max_use = 0;
int simple_line_request_aio_server::var_concurrency = 0;
int simple_line_request_aio_server::var_request_timeout = 0;
int simple_line_request_aio_server::var_line_max_len = 10240;
int simple_line_request_aio_server::var_extra_data_max_len = 0;

static simple_line_request_aio_server *_slras = 0;
static int _simple_line_request_use_count = 0;
static int _softstop_flag = 0;
static zpthread_pool_t *_worker_pool = 0;
static aio *_worker_monitor_aio = 0;

static void _do_request_once(void *ctx);

simple_line_request_aio_server::simple_line_request_aio_server()
{
    _slras = this;
}

simple_line_request_aio_server::~simple_line_request_aio_server()
{
}

void simple_line_request_aio_server::every_request()
{
}

static void _simple_line_request_release(simple_line_request *a)
{
    if (a) {
        delete a;
    }
}

static int _do_line_request_parse_object(simple_line_request *a, const std::string &req_line)
{
    json *tmpjs = 0;

    if (!a->json_.unserialize(req_line)) {
        return 0;
    }

    if (!(tmpjs = a->json_.object_get("cmd"))) {
        return 0;
    }
    a->cmd_name_ = &(tmpjs->get_string_value());

    if ((tmpjs = a->json_.object_get("length"))) {
        a->extra_data_len_ = (int)tmpjs->get_long_value();
        if (a->extra_data_len_ < 0) {
            return 0;
        }
    }

    return 1;
}

static int _do_line_request_parse_argv(simple_line_request *a, const std::string &req_line)
{
    int ok = 0;
    int i;
    zargv_t *cmdv = 0;
    json *args_js = 0;
    
    cmdv = zargv_create(6);
    if (strchr(req_line.c_str(), '|')) {
        zargv_split_append(cmdv, req_line.c_str(), "|");
    } else {
        zargv_split_append(cmdv, req_line.c_str(), " ");
    }
    if (zargv_len(cmdv) < 1) {
        goto over;
    }
    a->cmd_name_ = &(a->json_.object_update("cmd", zargv_argv(cmdv)[0], true)->get_string_value());
    args_js = a->json_.object_update("args", new json(), true);
    args_js->array_size();

    if (zargv_data(cmdv)[zargv_len(cmdv) - 1][0] == '{') {
        a->extra_data_len_ = atoi((zargv_data(cmdv)[zargv_len(cmdv) - 1]) + 1);
        if (a->extra_data_len_ < 0) {
            goto over;
        }
        zargv_truncate(cmdv, zargv_len(cmdv) - 1);
        a->json_.object_update("length", (long long)a->extra_data_len_);
    }
    /* FIXME limit */

    i = 0;
    ZARGV_WALK_BEGIN(cmdv, v) {
        if (i) {
            args_js->array_push(v);
        }
        i ++;
    } ZARGV_WALK_END;

    ok = 1;

over:
    zargv_free(cmdv);
    return ok;
}

static void _do_line_request_with_length_data(simple_line_request *a)
{
    if (a->get_result() < a->extra_data_len_ + 2) {
        _simple_line_request_release(a);
        return;
    }
    if (a->extra_data_ptr_) {
        zfree(a->extra_data_ptr_);
    }
    a->extra_data_ptr_ = (char *)zmalloc(a->extra_data_len_ + 2 + 1);
    a->get_read_cache((char *)a->extra_data_ptr_, a->extra_data_len_ + 2);
    ((char *)(a->extra_data_ptr_))[a->extra_data_len_] = 0;
    a->disable();
    zpthread_pool_job(_worker_pool, _do_request_once, a);
}

static void _do_request_once(void *ctx)
{
    simple_line_request *a = (simple_line_request *)ctx;
    std::string &cmd_name = *(a->cmd_name_);
    if ((cmd_name == "exit") || (cmd_name == "quit")) {
        a->cache_puts("OK BYE\n");
        a->cache_flush_and_stop();
        return;
    }
    
    _slras->handler(a);

    _simple_line_request_use_count ++;
    if ((simple_line_request_aio_server::var_max_use > 0) && (_simple_line_request_use_count >= simple_line_request_aio_server::var_max_use)) {
        _softstop_flag = 1;
        zinfo("simple_line_request_aio_server usage reaches the upper limit(%d>=%d)", _simple_line_request_use_count, simple_line_request_aio_server::var_max_use);
        _slras->detach_from_master();
    }

    _slras->every_request();
}

static void _do_request(simple_line_request *a)
{
    int ret = -1;
    int ok = 0;
    std::string req_line;

    a->clear();
    ret = a->get_result();
    if (ret < 1) {
        goto line_over;
    }
    a->get_read_cache(req_line, ret);
    trim_right(req_line, "\r\n");
    if (req_line[0] == '{') {
        ok = _do_line_request_parse_object(a, req_line);
    } else {
        ok = _do_line_request_parse_argv(a, req_line);
    }
    if (!ok) {
        goto line_over;
    }
    ok = 1;
line_over:
    if (!ok) {
        _simple_line_request_release(a);
        return;
    }

    if (a->extra_data_len_ > -1) {
        a->readn(a->extra_data_len_ + 2, std::bind(_do_line_request_with_length_data, a)); 
    } else {
        a->disable();
        zpthread_pool_job(_worker_pool, _do_request_once, a);
    }
}

static void _do_after_flush_and_request(simple_line_request *a)
{
    if (a->get_result() < 1) {
        _simple_line_request_release(a);
        return;
    }
    if (_softstop_flag) {
        _simple_line_request_release(a);
        return;
    }
    a->gets(simple_line_request_aio_server::var_line_max_len, std::bind(_do_request, a));
}

static void _do_after_flush_and_stop(simple_line_request *a)
{
    _simple_line_request_release(a);
}

static void _do_service(int fd)
{
    znonblocking(fd, 1);
    simple_line_request *a = new simple_line_request();
    a->rebind_fd(fd);
    a->set_read_wait_timeout(10);
    a->set_write_wait_timeout(10);
    _do_after_flush_and_request(a);
}

void simple_line_request_aio_server::service_register(const char *service, int fd, int fd_type)
{
    aio_server::general_service_register(fd, fd_type, _do_service);
}

static void _worker_monitor()
{
    if (!_worker_monitor_aio) {
        return;
    }
    if (_softstop_flag || zvar_sigint_flag) {
        delete _worker_monitor_aio;
        _worker_monitor_aio = 0;
        return;
    }
    long max = zpthread_pool_get_max_running_millisecond(_worker_pool);
    if (max > (1000L*simple_line_request_aio_server::var_request_timeout)) {
        zinfo("the running time of a instance reached the upper limit (%ld>=%d)", max/1000, simple_line_request_aio_server::var_request_timeout);
        _slras->detach_from_master();
        _softstop_flag = 1;
        delete _worker_monitor_aio;
        _worker_monitor_aio = 0;
        return;
    }
    _worker_monitor_aio->sleep(_worker_monitor, 1);
}

void simple_line_request_aio_server::before_service(void)
{
    aio_server::before_service();

    simple_line_request_aio_server::var_max_use  = zconfig_get_int(zvar_default_config, "server-service-max-use", simple_line_request_aio_server::var_max_use);
    simple_line_request_aio_server::var_request_timeout = (int)zconfig_get_second(zvar_default_config, "server-request-max-run-time",  simple_line_request_aio_server::var_request_timeout);
    simple_line_request_aio_server::var_concurrency  = zconfig_get_int(zvar_default_config, "server-service-concurrency_limit", simple_line_request_aio_server::var_concurrency);

    if (simple_line_request_aio_server::var_concurrency > 0) {
        _worker_pool = zpthread_pool_create();
        zpthread_pool_set_min_max_count(_worker_pool, simple_line_request_aio_server::var_concurrency, simple_line_request_aio_server::var_concurrency);
        zpthread_pool_start(_worker_pool);
        if (simple_line_request_aio_server::var_request_timeout > 0) {
            _worker_monitor_aio = new aio(-1);
            _worker_monitor_aio->sleep(_worker_monitor, 1);
        }
    }
}

void simple_line_request_aio_server::before_softstop(void)
{
    _softstop_flag = 1;
    aio_server::before_softstop();
}

static void _fini_all()
{
    if (!zvar_memleak_check) {
        return;
    }
    if (_worker_pool) {
        zpthread_pool_softstop(_worker_pool);
        zpthread_pool_wait_all_stopped(_worker_pool, 10);
        zpthread_pool_free(_worker_pool);
    }
}

int simple_line_request_aio_server::run(int argc, char **argv)
{
    int ret = aio_server::run(argc, argv);
    _fini_all();
    return ret;
}

simple_line_request::simple_line_request()
{
    cmd_name_ = 0;
    extra_data_len_ = -1;
    extra_data_ptr_ = 0;
}

simple_line_request::~simple_line_request()
{
    if (extra_data_ptr_) {
        zfree(extra_data_ptr_);
    }
}

void simple_line_request::clear()
{
    cmd_name_ = 0;
    if (extra_data_ptr_) {
        zfree(extra_data_ptr_);
    }
    extra_data_len_ = -1;
    extra_data_ptr_ = 0;
    json_.reset();
}

void simple_line_request::cache_flush_and_request()
{
    clear();
    cache_flush(std::bind(_do_after_flush_and_request, this));
}

void simple_line_request::cache_flush_and_stop()
{
    clear();
    cache_flush(std::bind(_do_after_flush_and_stop, this));
}

void simple_line_request::stop()
{
    clear();
    _simple_line_request_release(this);
}

void simple_line_request::request()
{
    clear();
    _do_after_flush_and_request(this);
}

void simple_line_request_aio_server::enter_job(void (*fn)(void *ctx), void *ctx)
{
    if (!fn) {
        return;
    }
    if (_worker_pool) {
        zpthread_pool_job(_worker_pool, fn, ctx);
    } else {
        fn(ctx);
    }
}

void simple_line_request_aio_server::enter_timer(void (*fn)(void *ctx), void *ctx, int timeout)
{
    if (!fn) {
        return;
    }
    if (_worker_pool) {
        zpthread_pool_timer(_worker_pool, fn, ctx, timeout);
    } else {
        zaio_base_timer(zvar_default_aio_base, fn, ctx, timeout);
    }
}

} /* namespace zcc */

#endif // __linux__

