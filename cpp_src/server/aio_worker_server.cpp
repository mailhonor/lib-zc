/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-20
 * ================================
 */

#ifdef __linux__

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zcc/zcc_server.h"
#include "zcc/zcc_errno.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/prctl.h>
#include <unistd.h>

zcc_namespace_begin;

static aio_worker_server *aio_worker_server_instance = nullptr;

static bool flag_run = false;
static bool flag_softstop = false;
static bool flag_stop = false;
static bool master_mode = false;
static bool flag_detach_from_master = false;
static pid_t parent_pid = 0;
static aio *ev_status = 0;
std::vector<aio *> event_ios;
static bool flag_server_status_fd_closed = false;

static void enter_stop()
{
    flag_stop = 1;
}

static void detach_from_master_check()
{
}

static void on_master_softstop()
{
    aio *ev = ev_status;
    flag_softstop = 1;

    if (ev_status)
    {
        delete ev_status;
    }
    ev_status = 0;

    if (flag_detach_from_master)
    {
        return;
    }

    aio_worker_server_instance->before_softstop();

    int exit_after = var_main_config.get_int("server-stop-on-softstop-after", 0);
    if (exit_after > 0)
    {
        var_main_aio_base->enter_timer(enter_stop, exit_after);
    }
    else
    {
        flag_stop = 1;
    }
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
        close_on_exec(fd);
        aio_worker_server_instance->service_register(service, fd, fd_type);
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
        nonblocking(fd, false);
        close_on_exec(fd);
        aio_worker_server_instance->service_register(service_name, fd, fdtype);
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

static void _aio_main_loop()
{
    if (flag_stop)
    {
        var_main_aio_base->stop_notify();
        return;
    }

    static int loop_times = 0;
    if (loop_times++ > 10)
    {
        if (getppid() != parent_pid)
        {
            var_main_aio_base->stop_notify();
        }
        loop_times = 0;
    }

    if (master_mode)
    {
        static int detach_dealed = 0;
        if (flag_detach_from_master && (!detach_dealed))
        {
            detach_dealed = 1;
            if (!flag_server_status_fd_closed)
            {
                flag_server_status_fd_closed = 1;
                ::close(worker_server_status_fd);
            }
            int exit_after = var_main_config.get_int("server-stop-on-softstop-after", 0);
            if (exit_after < 1)
            {
                exit_after = 3600;
            }
            var_main_aio_base->enter_timer(enter_stop, exit_after);
            for (auto it = event_ios.begin(); it != event_ios.end(); it++)
            {
                (*it)->disable();
            }
        }
    }
}

static void _server_init(aio_worker_server *aws, int argc, char **argv)
{
    if (flag_run)
    {
        zcc_fatal("aio worker main: only once");
    }
    flag_run = 1;
    aio_worker_server_instance = aws;

    main_argument::run(argc, argv);
    deal_argument();

    signal_ignore(SIGPIPE);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    parent_pid = getppid();
    if (parent_pid == 1)
    {
        exit(1);
    }

    var_main_aio_base = new aio_base();
    var_main_aio_base->set_loop_fn(_aio_main_loop);

    const char *attr;
    attr = var_main_config.get_cstring("server-config-path", "");
    if (!empty(attr))
    {
        config cf = master_server::load_global_config_from_dir(attr);
        cf.load_another(var_main_config);
        var_main_config.load_another(cf);
    }

    logger::use_syslog_by_config(zcc::var_main_config.get_cstring("server-syslog"));

    if (master_mode)
    {
        close_on_exec(master_server_status_fd);
        nonblocking(master_server_status_fd);
        ev_status = new aio(master_server_status_fd);
        ev_status->readable(on_master_softstop);
    }

    aio_worker_server_instance->before_service();

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
        var_main_aio_base->enter_timer(enter_stop, exit_after);
    }
}

static void _server_fini()
{
    if (!var_memleak_check_enable)
    {
        return;
    }
    if (ev_status)
    {
        ev_status->close(false);
        ev_status = 0;
    }
    /* The master would receive the signal of closing zvar_aio_server_status_fd */
    if (master_mode)
    {
        if (!flag_server_status_fd_closed)
        {
            flag_server_status_fd_closed = 1;
            ::close(worker_server_status_fd);
        }
    }

    for (auto it = event_ios.begin(); it != event_ios.end(); it++)
    {
        (*it)->disable();
        delete *it;
    }

    delete var_main_aio_base;

    sleep(1);
}

aio_worker_server *aio_worker_server::get_instance()
{
    return aio_worker_server_instance;
}

static void ___simple_server_accept(aio *ev, int type, std::function<void(int)> callback)
{
    int fd = -1, listen_fd = ev->get_fd();

    if (flag_softstop || flag_stop)
    {
        return;
    }

    if (type == var_tcp_listen_type_inet)
    {
        fd = inet_accept(listen_fd);
    }
    else if (type == var_tcp_listen_type_inet)
    {
        fd = unix_accept(listen_fd);
    }
    else if (type == var_tcp_listen_type_fifo)
    {
        fd = listen_fd;
    }
    if (fd < 0)
    {
        int ec = errno;
        if (ec == EINTR)
        {
            return;
        }
        if (ec == EAGAIN)
        {
            return;
        }
        zcc_fatal("inet_server_accept: %m");
        return;
    }
    close_on_exec(fd);
    callback(fd);
}

void aio_worker_server::simple_service_register(aio_base *aiobase, int fd, int fd_type, std::function<void(int)> callback)
{
    close_on_exec(fd);
    nonblocking(fd);
    aio *ev = new aio(fd, aiobase);
    ev->readable(std::bind(___simple_server_accept, ev, fd_type, callback));
    event_ios.push_back(ev);
}

void aio_worker_server::simple_service_register(aio_base *aiobase, int fd, int fd_type, void (*callback)(int))
{
    simple_service_register(aiobase, fd, fd_type, std::bind(callback, std::placeholders::_1));
}

aio_worker_server::aio_worker_server()
{
}

aio_worker_server::~aio_worker_server()
{
}

void aio_worker_server::before_service()
{
}

void aio_worker_server::before_softstop()
{
}

void aio_worker_server::stop_notify(int stop_after_second)
{
    if (stop_after_second < 1)
    {
        flag_stop = 1;
        return;
    }
    var_main_aio_base->enter_timer(enter_stop, stop_after_second);
}

void aio_worker_server::detach_from_master()
{
    flag_detach_from_master = 1;
}

void aio_worker_server::main_run(int argc, char **argv)
{
    _server_init(this, argc, argv);
    var_main_aio_base->run();
    _server_fini();
}

bool is_worker_server_mode()
{
    if (zcc::main_argument::var_argc < 1)
    {
        return false;
    }
    if ((!strcmp(zcc::main_argument::var_argv[0], "MASTER")) || (!strcmp(zcc::main_argument::var_argv[0], "alone")))
    {
        return true;
    }
    const char *attr = var_main_config.get_cstring("server-service", "");
    if (!empty(attr))
    {
        return true;
    }
    return false;
}

zcc_namespace_end;

#endif // __linux__
