/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-20
 * ================================
 */

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zc_coroutine.h"
#include "zcc/zcc_server.h"
#include "zcc/zcc_errno.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/prctl.h>
#include <unistd.h>

zcc_namespace_begin;

static coroutine_worker_server *coroutine_worker_server_instance = nullptr;

static zcoroutine_base_t *current_cobs = 0;
static bool flag_run = false;
static bool flag_stop = false;
static bool master_mode = false;
static bool flag_detach_from_master = false;
static pid_t parent_pid = 0;
static std::vector<int> alone_listen_fd_vector;

static void *do_stop_after(void *arg)
{
    sleep(ZCC_PTR_TO_NUMBER(arg));
    flag_stop = 1;
    return arg;
}

static void *monitor_reload_signal(void *arg)
{
    while (timed_read_wait(master_server_status_fd, 10) == 0)
    {
    }

    if (flag_detach_from_master)
    {
        return 0;
    }

    int need_stop_coroutine = 0;
    int exit_after = var_main_config.get_second("server-stop-on-softstop-after", 0);
    if (exit_after > 0)
    {
        coroutine_worker_server_instance->stop_notify(exit_after);
    }
    else
    {
        flag_stop = 1;
    }
    return 0;
}

static void *detach_monitor(void *arg)
{
    while (1)
    {
        sleep(10);
        if (flag_detach_from_master)
        {
            close_socket(worker_server_status_fd);
            break;
        }
    }
    int exit_after = var_main_config.get_second("server-stop-on-softstop-after", 0);
    if (exit_after < 1)
    {
        exit_after = 3600;
    }
    sleep(exit_after);
    flag_stop = 1;
    return 0;
}

static void *ppid_check(void *arg)
{
    while (1)
    {
        sleep(1);
        if (getppid() != parent_pid)
        {
            break;
        }
    }
    flag_stop = 1;
    return 0;
}

static void alone_register()
{
    const char *alone_url = var_main_config.get_cstring("server-service", "");
    if (empty(alone_url))
    {
        std::fprintf(stderr, "FATAL USAGE: %s [ ... ] -server-service 0:8899 [ ... ]\n", zcc::progname);
        zcc_fatal("USAGE: %s [ ... ] -server-service 0:8899 [ ... ]", zcc::progname);
    }
    char service_buf[1024];
    char *service, *url, *p;
    std::vector<std::string> splitor = split(alone_url, " \t,;\r\n");
    for (auto it = splitor.begin(); it != splitor.end(); it++)
    {
        const std::string &token = *it;
        if (token.size() > 1000)
        {
            zcc_fatal("alone_register: url too long");
        }
        std::strcpy(service_buf, token.c_str());
        p = std::strstr(service_buf, "://");
        if (p)
        {
            *p = 0;
            service = service_buf;
            url = p + 3;
        }
        else
        {
            service = var_blank_buffer;
            url = service_buf;
        }

        int fd_type;
        int fd = netpath_listen(url, 128, &fd_type);
        if (fd < 0)
        {
            zcc_fatal("alone_register: open %s(%m)", alone_url);
        }
        if (var_memleak_check_enable)
        {
            alone_listen_fd_vector.push_back(fd);
        }
        close_on_exec(fd);
        coroutine_worker_server_instance->service_register(service, fd, fd_type);
    }
}

static void master_register()
{
    const char *master_url = var_main_config.get_cstring("server-service", "");
    std::vector<std::string> service_argv = split(master_url, ",");
    for (auto it = service_argv.begin(); it != service_argv.end(); it++)
    {
        const std::string &token = *it;
        const char *service_name, *typefd;
        std::vector<std::string> stfd = split(token, ":");
        if (stfd.size() == 1)
        {
            service_name = var_blank_buffer;
            typefd = stfd[0].c_str();
        }
        else
        {
            service_name = stfd[0].c_str();
            typefd = stfd[1].c_str();
        }
        int fdtype = typefd[0];
        switch (fdtype)
        {
        case var_tcp_listen_type_inet:
        case var_tcp_listen_type_unix:
        case var_tcp_listen_type_fifo:
            break;
        default:
            zcc_fatal("master_aio_server: unknown service type %c", fdtype);
            break;
        }
        int fd = std::atoi(typefd + 1);
        if (fd < worker_server_listen_fd)
        {
            zcc_fatal("master_aio_server: fd(%s) is invalid", typefd + 1);
        }
        zcoroutine_enable_fd(fd);
        nonblocking(fd, false);
        close_on_exec(fd);
        coroutine_worker_server_instance->service_register(service_name, fd, fdtype);
    }
}

static void deal_argument()
{
    const char *s = "";
    if (!main_argument::var_parameters.empty())
    {
        s = main_argument::var_parameters[0];
    }
    if (empty(s))
    {
        s = "alone";
    }
    if (!strcmp(s, "MASTER"))
    {
        master_mode = 1;
    }
    else if (!strcmp(s, "alone"))
    {
        master_mode = 0;
    }
    else
    {
        std::fprintf(stderr, "FATAL USAGE: %s [ ... ] -server-service 0:8899 [ ... ]\n", zcc::progname);
        zcc_fatal("USAGE: %s [ ... ] -server-service 0:8899 [ ... ]", zcc::progname);
    }
}

static void _loop_fn(zcoroutine_base_t *cb)
{
    if (flag_stop)
    {
        zcoroutine_base_stop_notify(current_cobs);
    }
}

static void _server_init(coroutine_worker_server *cws, int argc, char **argv)
{
    if (flag_run)
    {
        zcc_fatal("zaio_server_main: only once");
    }
    flag_run = 1;

    coroutine_worker_server_instance = cws;

    zcc::main_argument::run(argc, argv);
    deal_argument();

    signal_ignore(SIGPIPE);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    parent_pid = getppid();
    if (parent_pid == 1)
    {
        exit(1);
    }

    const char *attr = var_main_config.get_cstring("server-config-path", "");
    if (!empty(attr))
    {
        config cf = master_server::load_global_config_from_dir(attr);
        cf.load_another(var_main_config);
        var_main_config.load_another(cf);
    }

    logger::use_syslog_by_config(zcc::var_main_config.get_cstring("server-syslog"));

    zcoroutine_base_init();
    zcoroutine_base_set_loop_fn(_loop_fn);
    current_cobs = zcoroutine_base_get_current();

    if (master_mode)
    {
        close_on_exec(master_server_status_fd);
        nonblocking(master_server_status_fd);
        zcoroutine_enable_fd(master_server_status_fd);
        zcoroutine_go(monitor_reload_signal, 0, 4);
        zcoroutine_go(detach_monitor, 0, 4);
        zcoroutine_go(ppid_check, 0, 4);
    }

    coroutine_worker_server_instance->before_service();

    if (master_mode == 0)
    {
        alone_register();
    }
    else
    {
        master_register();
    }

    int64_t exit_after = var_main_config.get_second("exit-after", 0);
    if (exit_after > 0)
    {
        alarm(0);
        zcoroutine_go(do_stop_after, ZCC_NUMBER_TO_PTR(exit_after), 4);
    }
}

static void _alone_listen_fd_clear()
{
    for (auto it = alone_listen_fd_vector.begin(); it != alone_listen_fd_vector.end(); it++)
    {
        close_socket(*it);
    }
    alone_listen_fd_vector.clear();
}

coroutine_worker_server::coroutine_worker_server()
{
}

coroutine_worker_server::~coroutine_worker_server()
{
}

void coroutine_worker_server::before_service()
{
}

void coroutine_worker_server::before_softstop()
{
}

void coroutine_worker_server::stop_notify(int stop_after_second)
{
    if (stop_after_second < 1)
    {
        flag_stop = 1;
        return;
    }
    zcoroutine_go(do_stop_after, ZCC_NUMBER_TO_PTR(stop_after_second), 4);
}

void coroutine_worker_server::detach_from_master()
{
    flag_detach_from_master = 1;
}

void coroutine_worker_server::main_run(int argc, char **argv)
{

    _server_init(this, argc, argv);
    zcoroutine_base_run();
    _alone_listen_fd_clear();
    zcoroutine_base_fini();
    if (var_memleak_check_enable)
    {
        sleep(1);
    }
}

zcc_namespace_end;

#endif // __linux__
