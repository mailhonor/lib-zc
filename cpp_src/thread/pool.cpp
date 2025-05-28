/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-12-02
 * ================================
 */

#include "zcc/zcc_thread.h"
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <chrono>

zcc_namespace_begin;

#define engine_handler_dosth(a) \
    {                           \
        auto &f = a;            \
        if (f)                  \
        {                       \
            f();                \
        }                       \
    }

struct thread_pool_node
{
    thread_pool_engine *engine{nullptr};
    bool stop_flag{false};
};

struct thread_pool_engine
{
    thread_pool *pool{nullptr};
    std::function<void()> thread_init_handler{nullptr};
    std::function<void()> thread_fini_handler{nullptr};
    std::function<void()> thread_loop_handler{nullptr};
    std::mutex mutex;
    std::condition_variable cond;
    std::list<std::function<void()>> task_queue;
    std::map<std::tuple<int64_t, int64_t>, std::function<void()>> timer_map;
    std::list<thread_pool_node *> pool_node_list;
    int64_t timer_plus{0};
    bool soft_stop_flag{false};
    bool debug_flag{false};
};

static thread_local thread_pool_node *_current_node = nullptr;

static void _pool_worker_run(thread_pool_engine *engine);

static void _pool_start_one_thread(thread_pool_engine *engine)
{
    std::thread t(std::bind(_pool_worker_run, engine));
    t.detach();
}

thread_pool *thread_pool::get_current_thread_pool()
{
    return _current_node ? _current_node->engine->pool : nullptr;
}

void thread_pool::stop_current_thread()
{
    if (!_current_node)
    {
        return;
    }
    _current_node->stop_flag = true;
}

thread_pool::thread_pool()
{
    engine_ = new thread_pool_engine();
    engine_->pool = this;
}

thread_pool::~thread_pool()
{
    wait_all_stopped(1);
    if (var_memleak_check_enable)
    {
        while (!engine_->timer_map.empty())
        {
            auto it = engine_->timer_map.begin();
            engine_->timer_map.erase(it);
        }
    }
    delete engine_;
}

void thread_pool::set_debug(bool tf)
{
    engine_->debug_flag = tf;
}

int thread_pool::get_thread_count()
{
    return engine_->pool_node_list.size();
}

int thread_pool::get_task_queue_length()
{
    int r = 0;
    lock();
    r = (int)(engine_->task_queue.size());
    unlock();
    return r;
}

void thread_pool::set_thread_init_handler(std::function<void()> fn)
{
    engine_->thread_init_handler = fn;
}

void thread_pool::set_thread_fini_handler(std::function<void()> fn)
{
    engine_->thread_fini_handler = fn;
}

void thread_pool::set_thread_loop_handler(std::function<void()> fn)
{
    engine_->thread_loop_handler = fn;
}

void thread_pool::lock()
{
    engine_->mutex.lock();
}

void thread_pool::unlock()
{
    engine_->mutex.unlock();
}

void thread_pool::softstop()
{
    engine_->soft_stop_flag = 1;
    engine_->cond.notify_all();
}

void thread_pool::wait_all_stopped(int max_second)
{
    softstop();

    if (max_second > 1024 * 1024)
    {
        max_second = 1024 * 1024;
    }
    max_second *= 10;

    for (int i = 0; i < max_second; i++)
    {
        if (engine_->pool_node_list.size() < 1)
        {
            break;
        }
        sleep_millisecond(100);
    }
}

void thread_pool::create_thread(int count)
{
    for (int i = 0; i < count; i++)
    {
        _pool_start_one_thread(engine_);
    }
}

static std::function<void()> _pool_worker_run_get(thread_pool_node *ptn)
{
    thread_pool_engine *engine = ptn->engine;
    thread_pool *pool = engine->pool;
    int left_timeout_flag = 0;
    int need_create = 0;
    std::function<void()> dosth = nullptr;

    std::unique_lock<std::mutex> locker(engine->mutex, std::defer_lock);
    locker.lock();
    while ((!var_sigint_flag) && (!(engine->soft_stop_flag)))
    {
        int64_t stamp = millisecond();
        int64_t wait = 0;
        if (!engine->timer_map.empty())
        {
            auto it = engine->timer_map.begin();
            if (std::get<0>(it->first) < stamp)
            {
                dosth = it->second;
                engine->timer_map.erase(it);
                break;
            }
            wait = std::get<0>(it->first) - stamp;
        }
        if (!engine->task_queue.empty())
        {
            dosth = engine->task_queue.front();
            engine->task_queue.pop_front();
            break;
        }
        else
        {
            wait = 1000;
        }
        if (wait < 0)
        {
            wait = 0;
        }
        wait++;
        engine->cond.wait_for(locker, std::chrono::milliseconds{wait});
    }
    locker.unlock();
    return dosth;
}

static thread_pool_node *_pool_worker_run_init(thread_pool_engine *engine)
{
    thread_pool_node *ptn = new thread_pool_node();
    _current_node = ptn;
    ptn->engine = engine;
    thread_pool *pool = engine->pool;

    pool->lock();
    engine->pool_node_list.push_back(ptn);
    pool->unlock();

    engine_handler_dosth(engine->thread_init_handler);
    return ptn;
}

static void _pool_worker_run_fini(thread_pool_node *ptn)
{
    auto engine = ptn->engine;
    thread_pool *pool = engine->pool;
    if (engine->thread_fini_handler)
    {
        engine->thread_fini_handler();
    }
    engine_handler_dosth(engine->thread_fini_handler);
    engine->mutex.lock();
    for (auto it = engine->pool_node_list.begin(); it != engine->pool_node_list.end(); it++)
    {
        if (*it == ptn)
        {
            engine->pool_node_list.erase(it);
            break;
        }
    }
    engine->mutex.unlock();
    _current_node = nullptr;
    delete ptn;
}

static void _pool_worker_run(thread_pool_engine *engine)
{
    thread_pool_node *ptn = _pool_worker_run_init(engine);
    while ((!var_sigint_flag) && (!(engine->soft_stop_flag)) && (!(ptn->stop_flag)))
    {
        if (engine->thread_loop_handler)
        {
            engine->thread_loop_handler();
        }
        std::function<void()> dosth = _pool_worker_run_get(ptn);
        if (!dosth)
        {
            continue;
        }
        dosth();
    }
    _pool_worker_run_fini(ptn);
}

void thread_pool::enter_task(std::function<void()> fn)
{
    if (!fn)
    {
        return;
    }
    if ((engine_->soft_stop_flag))
    {
        fn();
        return;
    }

    lock();
    engine_->task_queue.push_back(fn);
    unlock();
    engine_->cond.notify_one();
}

void thread_pool::enter_timer_millisecond(std::function<void()> fn, int64_t timeout_millisecond)
{
    if (!fn)
    {
        return;
    }
    if (engine_->soft_stop_flag)
    {
        return;
    }

    int64_t until = millisecond() + timeout_millisecond;
    lock();
    engine_->timer_plus++;
    engine_->timer_map[std::make_tuple(until, engine_->timer_plus)] = fn;
    unlock();
}

zcc_namespace_end;
