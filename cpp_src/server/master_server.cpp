/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-10
 * ================================
 */

#ifdef __linux__

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zcc/zcc_server.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define mydebug(fmt, args...)      \
    {                              \
        if (_log_debug_enable)     \
        {                          \
            zcc_info(fmt, ##args); \
        }                          \
    }

zcc_namespace_begin;

static master_server *master_server_instance = nullptr;

static bool _log_debug_enable = false;

static int master_reload_signal = SIGHUP;

static void set_signal_handler();

/* MASTER ######################################################### */
struct server_info_t;
struct listen_pair_t;
struct child_status_t;
struct server_info_t
{
    std::string config_pn;
    std::string config_realpn;
    std::string cmd;
    std::string master_chroot;
    std::string master_chdir;
    int proc_limit{0};
    int proc_count{0};
    int64_t stamp_next_start{0};
    std::map<int, listen_pair_t *> listens;
    std::vector<std::string> args;
};

struct listen_pair_t
{
    std::string service_name;
    std::string uri;
    int used{0};
    int fd{-1};
    int iuf{0};
    server_info_t *server{nullptr};
};

struct child_status_t
{
    server_info_t *server{nullptr};
    aio *status_ev{nullptr};
    int fd{-1};
    int pid{-1};
    int64_t stamp{-1};
    std::vector<std::string> uri_argv;
};

static std::string config_path;
static std::string config_realpath;
static int flag_reload = 0;
static int flag_stop = 0;
static int master_status_fd[2];
static std::map<std::string, listen_pair_t *> listen_pair_map;
static std::map<int, child_status_t *> child_status_map;
static std::vector<server_info_t *> server_info_vec;

static void start_one_child(server_info_t *server);

/* ########################################################### */
static server_info_t *server_info_create()
{
    server_info_t *s = new server_info_t();
    return s;
}

static void server_info_free(server_info_t *s)
{
    delete s;
}

static void server_info_start_all(server_info_t *s)
{
    int left = s->proc_limit - s->proc_count;
    long time_permit = millisecond() - s->stamp_next_start;
    mydebug("master: start_all server:%s, left:%d, permit: %d", s->cmd, left, (time_permit > 0 ? 1 : 0));

    if ((left < 1) || (time_permit < 1))
    {
        return;
    }

    for (int i = 0; i < left; i++)
    {
        start_one_child(s);
    }
}

/* ########################################################### */
static listen_pair_t *listen_pair_create()
{
    listen_pair_t *lp = new listen_pair_t();
    return lp;
}

static void listen_pair_free(listen_pair_t *lp)
{
    if (lp->fd != -1)
    {
        close_socket(lp->fd);
    }
    delete lp;
}

static void listen_pair_set_unused(listen_pair_t *lp)
{
    lp->used = 0;
    lp->server = 0;
}

/* ########################################################### */
child_status_t *child_status_create()
{
    child_status_t *c = new child_status_t();
    return c;
}

static void child_status_free(child_status_t *c)
{
    if (c->status_ev)
    {
        delete c->status_ev;
        c->status_ev = 0;
    }
    delete c;
}

/* ########################################################### */
static void on_child_strike(aio *a, child_status_t *cs)
{
    server_info_t *server = cs->server;
    if (server)
    {
        mydebug("master: on_child_strike, cmd:%s", server->cmd);
        if (millisecond() - cs->stamp < 100)
        {
            server->stamp_next_start = millisecond() + 100;
        }
        else if (millisecond() - cs->stamp > 1000)
        {
            server->stamp_next_start = 0;
        }
        server->proc_count--;
    }
    auto it = child_status_map.find(cs->fd);
    if (it != child_status_map.end())
    {
        child_status_map.erase(it);
    }
    child_status_free(cs);
}

static std::string _get_relative_path(const std::string &path, std::string &chroot_path)
{
    if ((chroot_path.size() > path.size()) || (path.compare(0, chroot_path.size(), chroot_path)))
    {
        zcc_fatal("path(%s) not in chroot_path(%s)", path, chroot_path);
    }
    return path.substr(chroot_path.size());
}

static void start_one_child(server_info_t *server)
{
    mydebug("master: start_one_child, cmd:%s", server->cmd);

    if (server->proc_count >= server->proc_limit)
    {
        return;
    }

    int status_fd[2];
    if (pipe(status_fd) == -1)
    {
        zcc_fatal("master: pipe (%m)");
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        /* error */
        ::close(status_fd[0]);
        ::close(status_fd[1]);
        sleep_millisecond(100);
        return;
    }
    else if (pid)
    {
        /* parent */
        server->proc_count++;
        ::close(status_fd[0]);
        child_status_t *cs = child_status_create();
        cs->server = server;
        cs->fd = status_fd[1];
        cs->pid = pid;
        cs->status_ev = new aio(status_fd[1], var_main_aio_base);
        cs->status_ev->readable(std::bind(on_child_strike, cs->status_ev, cs));
        for (auto it = server->listens.begin(); it != server->listens.end(); it++)
        {
            auto lp = it->second;
            cs->uri_argv.push_back(lp->uri);
        }

        child_status_map[cs->fd] = cs;
        cs->stamp = millisecond();
        return;
    }
    else
    {
        /* child */
        std::vector<std::string> exec_argv;

        ::close(status_fd[1]);
        dup2(status_fd[0], worker_server_status_fd);
        ::close(status_fd[0]);

        ::close(master_status_fd[0]);
        dup2(master_status_fd[1], master_server_status_fd);
        ::close(master_status_fd[1]);

        auto cmdname_pos = server->cmd.rfind('/');
        if (cmdname_pos != std::string::npos)
        {
            exec_argv.push_back(server->cmd.substr(cmdname_pos + 1));
        }
        else
        {
            exec_argv.push_back(server->cmd);
        }
        exec_argv.push_back("MASTER");

        if (!config_path.empty())
        {
            if (server->master_chroot.empty())
            {
                exec_argv.push_back("-server-config-path");
                exec_argv.push_back(config_path);

                if (!server->config_pn.empty())
                {
                    exec_argv.push_back("-config");
                    exec_argv.push_back(server->config_pn);
                }
            }
            else
            {
                exec_argv.push_back("-server-config-path");
                exec_argv.push_back(_get_relative_path(config_realpath, server->master_chroot));

                if (!server->config_pn.empty())
                {
                    exec_argv.push_back("-config");
                    exec_argv.push_back(_get_relative_path(server->config_realpn, server->master_chroot));
                }
            }
        }

        int fdnext = worker_server_listen_fd;
        std::string service_fork;
        for (auto it = server->listens.begin(); it != server->listens.end(); it++)
        {
            auto &lp2 = it->second;
            dup2(lp2->fd, fdnext);
            ::close(lp2->fd);
            sprintf_1024(service_fork, "%s:%c%d,", lp2->service_name.c_str(), lp2->iuf, fdnext);
            fdnext++;
        }
        if (!service_fork.empty())
        {
            service_fork.pop_back();
            exec_argv.push_back("-server-service");
            exec_argv.push_back(service_fork);
        }

        for (auto it = server->args.begin(); it != server->args.end(); it++)
        {
            exec_argv.push_back(*it);
        }

        if (!server->master_chroot.empty())
        {
            if (::chroot(server->master_chroot.c_str()))
            {
                zcc_fatal("chroot (%s) : %m", server->master_chroot);
            }
        }
        if (!server->master_chdir.empty())
        {
            if (::chdir(server->master_chdir.c_str()) == -1)
            {
                zcc_fatal("chdir (%s) : %m", server->master_chdir);
            }
        }

        const char **args = (const char **)calloc(exec_argv.size() + 1, sizeof(char *));
        for (int i = 0; i < (int)exec_argv.size(); i++)
        {
            args[i] = exec_argv[i].c_str();
        }
        ::execvp(server->cmd.c_str(), (char *const *)args);
        zcc_fatal("master: start child(%s) error: %m", server->cmd.c_str());
    }
}

static void remove_old_child()
{
    for (auto it = child_status_map.begin(); it != child_status_map.end(); it++)
    {
        int pid = it->first;
        auto cs = it->second;
        int have = 0;
        for (auto it2 = cs->uri_argv.begin(); it2 != cs->uri_argv.end(); it2++)
        {
            auto &uri = *it2;
            if (have == 0)
            {
                if (listen_pair_map.find(uri) != listen_pair_map.end())
                {
                    have = 1;
                }
            }
        }
        if (have == 0)
        {
            mydebug("kill child");
            kill(cs->pid, SIGTERM);
        }
        mydebug("remove_old_child");
        cs->server = 0;
    }
}

static void remove_server()
{
    for (auto it = server_info_vec.begin(); it != server_info_vec.end(); it++)
    {
        server_info_free(*it);
    }
    server_info_vec.clear();
}

static void set_listen_unused()
{
    for (auto it = listen_pair_map.begin(); it != listen_pair_map.end(); it++)
    {
        listen_pair_set_unused(it->second);
    }
}

static void prepare_server_by_config(config &cf)
{
    const char *cmd, *listen, *pn;
    server_info_t *server;
    cmd = cf.get_cstring("server-command");
    listen = cf.get_cstring("server-service");
    if (empty(cmd) || empty(listen))
    {
        return;
    }
    pn = cf.get_cstring("___Z_20181216_fn");
    server = server_info_create();
    server_info_vec.push_back(server);
    server->config_pn = pn;
    server->cmd = cmd;
    server->config_realpn = realpath(pn);
    server->master_chroot = realpath(cf.get_cstring("master-chroot", ""));
    server->master_chdir = cf.get_cstring("master-chdir", "");
    server->proc_limit = cf.get_int("server-proc-count", 1);
    server->proc_count = 0;
    if (empty(pn))
    {
        for (auto it = cf.begin(); it != cf.end(); it++)
        {
            const std::string &k = it->first;
            if (k == "server-proc-count")
            {
                continue;
            }
            if (k == "server-command")
            {
                continue;
            }
            if (k == "server-service")
            {
                continue;
            }
            if (k == "master-chroot")
            {
                continue;
            }
            std::string kk = "-";
            kk.append(k);
            server->args.push_back(kk);
            server->args.push_back(it->second);
        }
    }

    /* listens */
    std::vector<std::string> splitor = split(listen, ";, \t");
    for (auto it = splitor.begin(); it != splitor.end(); it++)
    {
        const std::string &uri_str = *it;
        char lbuf[1024];
        if (uri_str.size() > 1000)
        {
            zcc_fatal("master: service url too long, %s", uri_str);
        }
        std::strcpy(lbuf, uri_str.c_str());
        char *service = lbuf;
        char *uri = std::strstr(service, "://");
        if (!uri)
        {
            uri = service;
            service = var_blank_buffer;
        }
        else
        {
            *uri = 0;
            uri += 3;
        }

        listen_pair_t *lp = nullptr;
        auto lpit = listen_pair_map.find(uri);
        if (lpit == listen_pair_map.end())
        {
            lp = listen_pair_create();
            listen_pair_map[uri] = lp;
            lp->uri = uri;
            lp->fd = netpath_listen(uri, 5, (int *)&(lp->iuf));
            if (lp->fd < 0)
            {
                zcc_fatal("master: open %s error", uri);
            }
            nonblocking(lp->fd);
            close_on_exec(lp->fd);
        }
        else
        {
            lp = lpit->second;
        }
        if (lp->used)
        {
            zcc_fatal("master: open %s twice", uri);
        }
        lp->used = 1;
        lp->service_name = service;
        server->listens[lp->fd] = lp;
    }
}

static void reload_config()
{
    std::list<config> cfs = master_server_instance->load_server_configs();

    for (auto it = cfs.begin(); it != cfs.end(); it++)
    {
        prepare_server_by_config(*it);
    }
}

static void release_unused_listen()
{
    std::vector<std::string> delete_list;
    for (auto it = listen_pair_map.begin(); it != listen_pair_map.end(); it++)
    {
        auto &lp = it->second;
        if (lp->used)
        {
            continue;
        }
        delete_list.push_back(it->first);
        listen_pair_free(lp);
    }

    for (auto it = delete_list.begin(); it != delete_list.end(); it++)
    {
        listen_pair_map.erase(*it);
    }
}

static int64_t ___next_start_all_child_stamp = 0;
static void start_all_child()
{
    if (millisecond() < ___next_start_all_child_stamp)
    {
        return;
    }
    for (auto it = server_info_vec.begin(); it != server_info_vec.end(); it++)
    {
        server_info_start_all(*it);
    }
    ___next_start_all_child_stamp = millisecond() + 100;
}

static void reload_server()
{
    /* MASTER STATUS */
    ::close(master_status_fd[0]);
    ::close(master_status_fd[1]);
    if (::pipe(master_status_fd) == -1)
    {
        zcc_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0]);

    remove_server();
    set_listen_unused();
    reload_config();
    release_unused_listen();
    remove_old_child();
    start_all_child();
}

static bool master_lock_pfile(const char *lock_file)
{
    int lock_fd;
    char pid_buf[64];

    lock_fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (lock_fd < 0)
    {
        zcc_error("master: open %s(%m)", lock_file);
        return false;
    }
    close_on_exec(lock_fd);

    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0)
    {
        zcc_error("master: flock %s(%m)", lock_file);
        ::close(lock_fd);
        return false;
    }
    else
    {
        std::sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10)
        {
            zcc_error("master: open %s(%m)", lock_file);
            ::close(lock_fd);
            return false;
        }
        return true;
    }

    return false;
}

static void sighup_handler(int sig)
{
    flag_reload = sig;
}

static void sigterm_handler(int sig)
{
    flag_stop = sig;
}

static void set_signal_handler()
{
    signal_ignore(SIGPIPE);
    signal_ignore(SIGCHLD);

    signal(SIGTERM, sigterm_handler);

    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(master_reload_signal, &sig, (struct sigaction *)0) < 0)
    {
        zcc_fatal("%s: sigaction(%d) : %m", __FUNCTION__, master_reload_signal);
    }
}

static void _main_loop()
{
    if (flag_stop)
    {
        var_main_aio_base->stop_notify();
        return;
    }
    if (flag_reload)
    {
        flag_reload = 0;
        reload_server();
    }
    start_all_child();
}

static bool ___init_flag = false;
static void init_all(int argc, char **argv)
{
    const char *lock_file = 0;
    bool try_lock = false;

    if (___init_flag)
    {
        zcc_fatal("master: master::run only be excuted once");
    }
    ___init_flag = true;

    main_argument::run(argc, argv);
    int64_t sl = var_main_config.get_second("sleep", 0);
    if (sl > 0)
    {
        sleep_millisecond(sl);
        exit(0);
    }

    _log_debug_enable = var_main_config.get_bool("DEBUG");
    try_lock = var_main_config.get_bool("try-lock");
    lock_file = var_main_config.get_cstring("pid-file");
    config_path = var_main_config.get_string("C");
    config_realpath = realpath(config_path.c_str());
    if (!config_path.empty())
    {
        if (config_path.back() != '/')
        {
            config_path.push_back('/');
        }
    }

    master_server_instance->before_service();

    if (!empty(lock_file))
    {
        if (try_lock)
        {
            if (master_lock_pfile(lock_file))
            {
                exit(0);
            }
            else
            {
                exit(1);
            }
        }
        if (!master_lock_pfile(lock_file))
        {
            exit(1);
        }
    }

    var_main_aio_base = new aio_base();
    var_main_aio_base->set_loop_fn(_main_loop);

    set_signal_handler();

    if (::pipe(master_status_fd) == -1)
    {
        zcc_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0]);

    logger::use_syslog_by_config(zcc::var_main_config.get_cstring("server-syslog"));
}

static void fini_all()
{
    remove_server();
    set_listen_unused();
    release_unused_listen();
    remove_old_child();

    var_main_aio_base->stop_notify();
}

master_server::master_server()
{
}

master_server::~master_server()
{
}

void master_server::main_run(int argc, char **argv)
{
    master_server_instance = this;
    init_all(argc, argv);
    reload_server();
    flag_reload = 0;
    var_main_aio_base->run();
    fini_all();
}

std::list<config> master_server::load_server_configs()
{
    std::list<config> cfs;
    if (config_path.empty())
    {
        return cfs;
    }
    cfs = load_server_configs_from_dir(config_path.c_str());
    return cfs;
}

void master_server::before_service()
{
}

master_server *master_server::get_instance()
{
    return master_server_instance;
}

aio_base *master_server::get_aio_base()
{
    return var_main_aio_base;
}

std::list<config> master_server::load_server_configs_from_dir(const char *dirname)
{
    std::list<config> cfs;
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;

    dir = opendir(dirname);
    if (!dir)
    {
        zcc_fatal("master: open %s/(%m)", dirname);
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list))
    {
        fn = ent.d_name;
        if (fn[0] == '.')
        {
            continue;
        }
        p = std::strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "cf")))
        {
            continue;
        }

        std::string pn = config_path;
        pn.append(fn);
        config cf;
        if (cf.load_from_file(pn) < 1)
        {
            zcc_fatal("master: open %s", pn);
        }
        cf["___Z_20181216_fn"] = pn;
        cfs.push_back(cf);
    }
    closedir(dir);
    return cfs;
}

config master_server::load_global_config_from_dir(const char *dirname)
{
    config cf;
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];

    dir = opendir(dirname);
    if (!dir)
    {
        return cf;
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list))
    {
        fn = ent.d_name;
        if (fn[0] == '.')
        {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "gcf")))
        {
            continue;
        }
        std::string pn = config_path;
        pn.append(fn);
        if (cf.load_from_file(pn) < 1)
        {
            zcc_fatal("master: open %s", pn);
        }
    }
    closedir(dir);
    return cf;
}
zcc_namespace_end;

#endif // __linux__
