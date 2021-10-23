/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-01-02
 * ================================
 */

#pragma once

#ifndef ___ZC_LIB_INCLUDE_COROUTINE___
#define ___ZC_LIB_INCLUDE_COROUTINE___

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>

#pragma pack(push, 4)

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct zcoroutine_base_t zcoroutine_base_t;
typedef struct zcoroutine_t zcoroutine_t;
typedef struct zcoroutine_mutex_t zcoroutine_mutex_t;
typedef struct zcoroutine_cond_t zcoroutine_cond_t;

typedef int zbool_t;

/* 协程框架, 本协程不得跨线程操作 */

extern int zvar_coroutine_max_fd; /* 10240 */

/* 在线程内初始化协程基础环境 */
zcoroutine_base_t *zcoroutine_base_init();

/* 获取当前协程环境 */
zcoroutine_base_t *zcoroutine_base_get_current();

/* 设置当前协程环境每次epoll循环需要执行的函数 */
void zcoroutine_base_set_loop_fn(void (*loop_fn)(zcoroutine_base_t *cb));

/* 在线程内运行当前协程框架 */
void zcoroutine_base_run();

/* 通知协程环境(cobs==0,表示当前)退出, 既 zcoroutine_base_run() 返回 */
void zcoroutine_base_stop_notify(zcoroutine_base_t *cobs);

/* 回收协程框架资源 */
void zcoroutine_base_fini();

/* 进入协程, 然后, 执行 start_job, 参数为ctx */
/* stack_size: 协程栈大小(单位K), 建议不小于16 */
void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);
/* 同 zcoroutine_go, 指定协程环境 */
void zcoroutine_advanced_go(zcoroutine_base_t *cobs, void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);

/* 返回当前协程 */
zcoroutine_t * zcoroutine_self();

/* 放弃当前协程使用权 */
void zcoroutine_yield();

/* 主动结束协程 */
void zcoroutine_exit();

/* 返回当前协程ID */
long zcoroutine_getid();

/* 睡眠 */
/* 不支持 usleep nanosleep */
/* unsigned int sleep(unsigned int seconds); */
void zcoroutine_sleep_millisecond(int milliseconds);

/* 获取当前协程上下文 */
void *zcoroutine_get_context();

/* 设置当前协程上下文 */
void zcoroutine_set_context(const void *ctx);

/* 主动设置fd支持协程切换 */
void zcoroutine_enable_fd(int fd);

/* 主动设置fd不支持协程切换 */
void zcoroutine_disable_fd(int fd);

/* 创建协程锁 */
zcoroutine_mutex_t * zcoroutine_mutex_create();
void zcoroutine_mutex_free(zcoroutine_mutex_t *);

/* 锁 */
void zcoroutine_mutex_lock(zcoroutine_mutex_t *);

/* 解锁 */
void zcoroutine_mutex_unlock(zcoroutine_mutex_t *);

/* 创建条件 */
zcoroutine_cond_t * zcoroutine_cond_create();
void zcoroutine_cond_free(zcoroutine_cond_t *);

/* 条件等待, 参考 pthread_cond_wait */
void zcoroutine_cond_wait(zcoroutine_cond_t *, zcoroutine_mutex_t *);

/* 条件信号, 参考 pthread_cond_signal */
void zcoroutine_cond_signal(zcoroutine_cond_t *);

/* 条件广播, 参考 pthread_cond_broadcast */
void zcoroutine_cond_broadcast(zcoroutine_cond_t *);

/* 禁用 UDP 协程切换 */
extern zbool_t zvar_coroutine_disable_udp/* = 0 */;

/* 禁用 53 端口的 UDP 协程切换 */
extern zbool_t zvar_coroutine_disable_udp_53/* = 0 */;

/* 启用limit个线程池, 用于文件io,和 block_do */
extern int zvar_coroutine_block_pthread_count_limit/* = 0 */;

/* 如果
      zvar_coroutine_block_pthread_count_limit > 0 且
      zvar_coroutine_fileio_use_block_pthread == 1
   则 文件io在线程池执行, 否则在当前线程执行 */
extern zbool_t zvar_coroutine_fileio_use_block_pthread/* = 0 */;

/* 如果 zvar_coroutine_block_pthread_count_limit > 0
   则 block_func(ctx) 在线程池执行, 否则在当前程直接执行 */
void *zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx);

/* zcoroutine_block_XXX 基于 zcoroutine_block_do 机制 */
int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset);
int zcoroutine_block_write(int fd, const void *data, int len);
long zcoroutine_block_lseek(int fd, long offset, int whence);
int zcoroutine_block_open(const char *pathname, int flags, mode_t mode);
int zcoroutine_block_close(int fd);
int zcoroutine_block_rename(const char *oldpath, const char *newpath);
int zcoroutine_block_unlink(const char *pathname);

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

#endif /*___ZC_LIB_INCLUDE_COROUTINE___ */
