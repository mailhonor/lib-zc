#ifdef __linux__
/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-10-15
 * ================================
 */

#include "zcc/zcc_server.h"
#include "zcc/zcc_thread.h"
#include <functional>

zcc_namespace_begin;

int simple_line_aio_worker_server::var_read_wait_timeout = -1;
int simple_line_aio_worker_server::var_concurrency = 0;
int simple_line_aio_worker_server::var_line_max_len = 10240;
int simple_line_aio_worker_server::var_extra_data_max_len = 0;

static simple_line_aio_worker_server *_slras = 0;
static int _softstop_flag = 0;
static thread_pool *_worker_pool = 0;
static aio *_worker_monitor_aio = 0;

static void _do_request_once(simple_line_aio_worker_server::simple_line_aio *a);

simple_line_aio_worker_server::simple_line_aio_worker_server()
{
    _slras = this;
}

simple_line_aio_worker_server::~simple_line_aio_worker_server()
{
}

static void _simple_line_request_release(simple_line_aio_worker_server::simple_line_aio *a)
{
    if (a)
    {
        delete a;
    }
}

static int _do_line_request_parse_object(simple_line_aio_worker_server::simple_line_aio *a, const std::string &req_line)
{
    json *tmpjs = 0;

    if (!a->json_.unserialize(req_line))
    {
        return 0;
    }

    if (!(tmpjs = a->json_.object_get("cmd")))
    {
        return 0;
    }
    a->cmd_name_ = tmpjs->get_string_value();

    if ((tmpjs = a->json_.object_get("length")))
    {
        a->extra_data_len_ = (int)tmpjs->get_long_value();
        if (a->extra_data_len_ < 0)
        {
            return 0;
        }
    }

    return 1;
}

static int _do_line_request_parse_argv(simple_line_aio_worker_server::simple_line_aio *a, const std::string &req_line)
{
    std::vector<std::string> cmdv;

    if (std::strchr(req_line.c_str(), '|'))
    {
        cmdv = split(req_line, '|');
    }
    else
    {
        cmdv = split(req_line, ' ');
    }
    if (cmdv.size() < 1)
    {
        return 0;
    }
    a->cmd_name_ = a->json_.object_update("cmd", cmdv[0], true)->get_string_value();
    json *args_js = a->json_.object_update("args", new json(json_type_array), true);

    if ((!cmdv.back().empty()) && (cmdv.back()[0] == '{'))
    {
        a->extra_data_len_ = std::atoi(cmdv.back().c_str() + 1);
        if (a->extra_data_len_ < 0)
        {
            return 0;
        }
        cmdv.pop_back();
        a->json_.object_update("length", (int64_t)(a->extra_data_len_));
    }
    /* FIXME limit */

    for (auto it = cmdv.begin(); it != cmdv.end(); it++)
    {
        if (it != cmdv.begin())
        {
            args_js->array_push(*it);
        }
    }

    return 1;
}

static void _do_line_request_with_length_data(simple_line_aio_worker_server::simple_line_aio *a)
{
    if (a->get_result() < a->extra_data_len_ + 2)
    {
        _simple_line_request_release(a);
        return;
    }
    if (a->extra_data_ptr_)
    {
        free(a->extra_data_ptr_);
    }
    a->extra_data_ptr_ = (char *)malloc(a->extra_data_len_ + 2 + 1);
    a->get_read_cache((char *)a->extra_data_ptr_, a->extra_data_len_ + 2);
    ((char *)(a->extra_data_ptr_))[a->extra_data_len_] = 0;
    if (_worker_pool)
    {
        a->disable();
        _worker_pool->enter_task(std::bind(_do_request_once, a));
    }
    else
    {
        _do_request_once(a);
    }
}

static void _do_request_once(simple_line_aio_worker_server::simple_line_aio *a)
{
    std::string &cmd_name = a->cmd_name_;
    if ((cmd_name == "exit") || (cmd_name == "quit"))
    {
        a->cache_puts("OK BYE\n");
        a->cache_flush_and_stop();
        return;
    }

    _slras->handler(*a);
}

static void _do_request(simple_line_aio_worker_server::simple_line_aio *a)
{
    int ret = -1;
    int ok = 0;
    std::string req_line;

    a->clear();
    ret = a->get_result();
    if (ret < 1)
    {
        goto line_over;
    }
    a->get_read_cache(req_line, ret);
    trim_right(req_line, "\r\n");
    if (req_line[0] == '{')
    {
        ok = _do_line_request_parse_object(a, req_line);
    }
    else
    {
        ok = _do_line_request_parse_argv(a, req_line);
    }
    if (!ok)
    {
        goto line_over;
    }
    ok = 1;
line_over:
    if (!ok)
    {
        _simple_line_request_release(a);
        return;
    }

    if (a->extra_data_len_ > -1)
    {
        a->readn(a->extra_data_len_ + 2, std::bind(_do_line_request_with_length_data, a));
    }
    else
    {
        a->disable();
        if (_worker_pool)
        {
            a->disable();
            _worker_pool->enter_task(std::bind(_do_request_once, a));
        }
        else
        {
            _do_request_once(a);
        }
    }
}

static void _do_after_flush_and_request(simple_line_aio_worker_server::simple_line_aio *a)
{
    if (a->get_result() < 1)
    {
        _simple_line_request_release(a);
        return;
    }
    if (_softstop_flag)
    {
        _simple_line_request_release(a);
        return;
    }
    a->gets(simple_line_aio_worker_server::var_line_max_len, std::bind(_do_request, a));
}

static void _do_after_flush_and_stop(simple_line_aio_worker_server::simple_line_aio *a)
{
    _simple_line_request_release(a);
}

static void _do_service(int fd)
{
    nonblocking(fd);
    simple_line_aio_worker_server::simple_line_aio *a = new simple_line_aio_worker_server::simple_line_aio();
    a->rebind_fd(fd);
    a->set_timeout(simple_line_aio_worker_server::var_read_wait_timeout);
    _do_after_flush_and_request(a);
}

void simple_line_aio_worker_server::service_register(const char *service, int fd, int fd_type)
{
    aio_worker_server::simple_service_register(nullptr, fd, fd_type, _do_service);
}

void simple_line_aio_worker_server::before_service()
{
    aio_worker_server::before_service();

    simple_line_aio_worker_server::var_concurrency = var_main_config.get_int("server-service-concurrency", simple_line_aio_worker_server::var_concurrency);

    if (simple_line_aio_worker_server::var_concurrency > 0)
    {
        _worker_pool = new thread_pool();
        _worker_pool->create_thread(simple_line_aio_worker_server::var_concurrency);
    }
}

void simple_line_aio_worker_server::before_softstop()
{
    _softstop_flag = 1;
    aio_worker_server::before_softstop();
}

static void _fini_all()
{
    if (!var_memleak_check_enable)
    {
        return;
    }
    if (_worker_pool)
    {
        _worker_pool->softstop();
        _worker_pool->wait_all_stopped(10);
        delete _worker_pool;
        _worker_pool = nullptr;
    }
}

void simple_line_aio_worker_server::main_run(int argc, char **argv)
{
    aio_worker_server::main_run(argc, argv);
    _fini_all();
}

simple_line_aio_worker_server::simple_line_aio::simple_line_aio()
{
    extra_data_len_ = -1;
    extra_data_ptr_ = 0;
}

simple_line_aio_worker_server::simple_line_aio::~simple_line_aio()
{
    if (extra_data_ptr_)
    {
        free(extra_data_ptr_);
    }
}

void simple_line_aio_worker_server::simple_line_aio::clear()
{
    cmd_name_.clear();
    if (extra_data_ptr_)
    {
        free(extra_data_ptr_);
    }
    extra_data_len_ = -1;
    extra_data_ptr_ = nullptr;
    json_.reset();
}

void simple_line_aio_worker_server::enter_task(std::function<void()> task)
{
    if (!task)
    {
        return;
    }
    if (_worker_pool)
    {
        _worker_pool->enter_task(task);
    }
    else
    {
        task();
    }
}

void simple_line_aio_worker_server::enter_timer(std::function<void()> task, int timeout)
{
    if (!task)
    {
        return;
    }
    if (_worker_pool)
    {
        _worker_pool->enter_timer(task, timeout);
    }
    else
    {
        var_main_aio_base->enter_timer(task, timeout);
    }
}

void simple_line_aio_worker_server::simple_line_aio::cache_flush_and_request()
{
    clear();
    cache_flush(std::bind(_do_after_flush_and_request, this));
}

void simple_line_aio_worker_server::simple_line_aio::cache_flush_and_stop()
{
    clear();
    cache_flush(std::bind(_do_after_flush_and_stop, this));
}

void simple_line_aio_worker_server::simple_line_aio::stop()
{
    clear();
    _simple_line_request_release(this);
}

void simple_line_aio_worker_server::simple_line_aio::request()
{
    clear();
    _do_after_flush_and_request(this);
}

zcc_namespace_end;

#endif // __linux__
