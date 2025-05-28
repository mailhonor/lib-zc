/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_SERVER___
#define ZCC_LIB_INCLUDE_SERVER___

#include <functional>
#include "./zcc_stdlib.h"
#include "./zcc_aio.h"
#include "./zcc_json.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

static const int master_server_status_fd = 3;
static const int worker_server_status_fd = 4;
static const int worker_server_listen_fd = 5;

class master_server
{
public:
    static master_server *get_instance();
    static aio_base *get_aio_base();
    static std::list<config> load_server_configs_from_dir(const char *dirname);
    static config load_global_config_from_dir(const char *dirname);

public:
    master_server();
    virtual ~master_server();
    void main_run(int argc, char **argv);
    virtual std::list<config> load_server_configs();
    virtual void before_service();

protected:
    void *unused_;
};

class aio_worker_server
{
public:
    static aio_worker_server *get_instance();
    static void simple_service_register(aio_base *aiobase, int fd, int fd_type, std::function<void(int)> callback);
    static void simple_service_register(aio_base *aiobase, int fd, int fd_type, void (*callback)(int));
    inline static void simple_service_register(int fd, int fd_type, void (*callback)(int))
    {
        simple_service_register(var_main_aio_base, fd, fd_type, callback);
    }

public:
    aio_worker_server();
    virtual ~aio_worker_server();
    virtual void service_register(const char *service, int fd, int fd_type) = 0;
    virtual void before_service();
    virtual void before_softstop();
    virtual void stop_notify(int stop_after_second = 0);
    virtual void detach_from_master();
    virtual void main_run(int argc, char **argv);

public:
    void *unused_;
};

class simple_line_aio_worker_server : public aio_worker_server
{
public:
    class simple_line_aio : public aio
    {
    public:
        simple_line_aio();
        virtual ~simple_line_aio();
        void clear();
        void cache_flush_and_request();
        void cache_flush_and_stop();
        void request();
        void stop();
        std::string cmd_name_;
        json json_;
        int extra_data_len_{-1};
        void *extra_data_ptr_{nullptr};
    };

public:
    static int var_read_wait_timeout;
    static int var_concurrency;
    static int var_line_max_len;
    static int var_extra_data_max_len;

public:
    simple_line_aio_worker_server();
    virtual ~simple_line_aio_worker_server();
    virtual void service_register(const char *service, int fd, int fd_type);
    virtual void handler(simple_line_aio &slr) = 0;
    virtual void main_run(int argc, char **argv);
    virtual void before_service();
    virtual void before_softstop();
    virtual void enter_task(std::function<void()> task);
    virtual void enter_timer(std::function<void()> task, int timeout);
};

class coroutine_worker_server
{
public:
    static coroutine_worker_server *get_instance();

public:
    coroutine_worker_server();
    virtual ~coroutine_worker_server();
    virtual void service_register(const char *service, int fd, int fd_type) = 0;
    virtual void before_service();
    virtual void before_softstop();
    virtual void stop_notify(int stop_after_second = 0);
    virtual void detach_from_master();
    virtual void main_run(int argc, char **argv);

public:
    void *unused_;
};

bool is_worker_server_mode();

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_SERVER___
