/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_THREAD__
#define ZCC_LIB_INCLUDE_THREAD__

#include "./zcc_stdlib.h"
#include <functional>
#include <thread>

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

struct thread_pool_engine;

class thread_pool
{
public:
    static thread_pool *get_current_thread_pool();
    static void stop_current_thread();

public:
    thread_pool();
    ~thread_pool();
    //
    void create_thread(int count = 1);
    inline void create_one_thread()
    {
        create_thread(1);
    }
    void enter_task(std::function<void()> fn);
    void enter_timer_millisecond(std::function<void()> fn, int64_t timeout_millisecond);
    inline void enter_timer(std::function<void()> fn, int timeout)
    {
        enter_timer_millisecond(fn, timeout * 1000);
    }
    void softstop();
    void wait_all_stopped(int max_second);
    //
    void set_debug(bool tf = true);
    int get_thread_count();
    int get_task_queue_length();
    void set_thread_init_handler(std::function<void()> fn);
    void set_thread_fini_handler(std::function<void()> fn);
    void set_thread_loop_handler(std::function<void()> fn);
    void lock();
    void unlock();

protected:
    thread_pool_engine *engine_{nullptr};
};

#ifndef __APPLE__
void set_thread_name(std::thread &t, const char *name);
inline void set_thread_name(std::thread &t, const std::string &name)
{
    set_thread_name(t, name.c_str());
}
#endif // __APPLE__
void set_thread_name(const char *name);
inline void set_thread_name(const std::string &name)
{
    set_thread_name(name.c_str());
}
std::string get_thread_name(std::thread &t);
std::string get_thread_name();

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_THREAD__
