<A name="readme_md" id="readme_md"></A>

## 协程框架, LIB-ZO, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 是 Linux 平台通用 C 扩展库, 内嵌 LIB-ZO

LIB-ZO 是协程开发框架的单独封装, 可独立使用, 代码仓库 https://gitee.com/linuxmail/lib-zo

LIB-ZO 协程库参考了 libco, libgo, nodejs 的实现

## 数据结构

```
/* 隐藏细节, 不必深究 */
struct zcoroutine_base_t {};    /* 运行环境 */
struct zcoroutine_t {};         /* 协程 */
struct zcoroutine_mutex_t {};   /* 锁 */
struct zcoroutine_cond_t {};    /* 条件 */
```

## 特性

LIB-ZO 协程库, 支持:

* 协程条件, 协程锁
* 慢操作协程化
* IO类协程化(如mkdir, getdents等)
* 睡眠(sleep)
* 可以禁用 UDP 协程切换
* DNS 协议
* 静态编译

### 睡眠

* 支持函数sleep
* 不支持 usleep 和 nanosleep
* 毫秒睡眠请用 void zcoroutine_sleep_millisecond(int milliseconds)

### 文件 IO

可开启支持文件IO在其他线程池工作, 这些IO函数包括:

* open, openat, create, close, read, readv, write, writev, lseek, truncate, ftruncate,
* stat, fstat, lstat, link, linkat, symlink, symlinkat, readlink, readlinkat, unlink, unlinkat, rename, renameat
* chmod, fchmod, chown, fchown, lchown, utime, utimes, fdatasync, fsync,
* rmdir, mkdir, mkdirat, getdents

### 慢(阻塞式)操作

* 慢操作可以在其他线程池工作

### DNS 协议

* 大部分 glibc 版本不支持 DNS 解析, 至少 glibc 版本 2.12 没问题
* 如果有问题, 建议:
    * 如果需要查询常用域名的 IP 地址, 可以写到 hosts 文件
    * 可以考虑 "慢操作协程化"
    * 可以禁用 53 端口(既 DNS)的 UDP 协程切换

### 可以禁用 UDP 协程切换

* 可以禁用 UDP 协程切换
* 可以禁用 53 端口(既 DNS)的 UDP 协程切换

### 协程环境

* 支持多个协程运行环境, 每个运行环境独占一个线程
* 协程全部函数 **线程不安全**

## 模型

在线程内, 运行协程环境, 步骤如下

### 第一步, 初始化协程环境

```
zcoroutine_base_init();
```

### 第二步, 至少执行一个 zcoroutine_go

```
/* foo是协程回调函数, 第二个参数是foo的参数, 第三个参数是协程栈空大小(0默认), 单位(K) */
zcoroutine_go(foo, 0, 64);
```

### 第三步, 启动环境

```
zcoroutine_base_run(0);
```

### 第四步,释放环境
```
zcoroutine_base_fini();
```


## 函数: 环境

### zcoroutine_base_t *zcoroutine_base_init();

* 在线程内初始化协程基础环境 

### zcoroutine_base_t *zcoroutine_base_get_current();

* 获取当前线程的协程环境

### void zcoroutine_base_run(void (*loop_fn)());

* 在线程内运行当前协程环境
* 每次事件循环会执行 loop_fn()

### void zcoroutine_base_stop_notify(zcoroutine_base_t *cobs);

* 通知协程环境 cobs 退出, 既 zcoroutine_base_run() 返回
* cobs==0: 表示当前线程的协程环境

### void zcoroutine_base_fini();

* 释放当前线程的协程环境

### void zcoroutine_enable_fd(int fd);

* 主动设置 fd 支持协程切换

### void zcoroutine_disable_fd(int fd);

* 主动设置 fd 不支持协程切换

### extern zbool_t zvar_coroutine_disable_udp

* 禁用 UDP 协程切换

### extern zbool_t zvar_coroutine_disable_udp_53

* 禁用 53 端口的 UDP 协程切换

## 函数: 协程

### void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);

* 创建一个协程, 立即返回
* 此协程激活后首先执行 start_job(ctx)
* stack_size: 协程栈大小(单位K), 建议不小于16 

### void zcoroutine_advanced_go(zcoroutine_base_t *cobs, void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);

* 在指定的协程环境下创建协程
* 其他同 zcoroutine_go

### zcoroutine_t * zcoroutine_self();

* 当前协程

### void zcoroutine_yield();

* 临时放弃当前协程使用权, 让出 CPU

### void zcoroutine_exit();

* 主动结束协程

### long zcoroutine_getid();

* 当前协程ID

### void *zcoroutine_get_context();

* 获取当前协程上下文

### void zcoroutine_set_context(const void *ctx);

* 设置当前协程上下文

## 函数: 睡眠

支持函数 sleep, 不支持 usleep 和 nanosleep

### #define zcoroutine_sleep zsleep

* 睡眠

### #define zcoroutine_sleep_millisecond zsleep_millisecond

* 睡眠, 毫秒

## 函数: 锁

### zcoroutine_mutex_t * zcoroutine_mutex_create();

* 创建协程(独占)锁

### void zcoroutine_mutex_free(zcoroutine_mutex_t *);

* 释放

### void zcoroutine_mutex_lock(zcoroutine_mutex_t *);

* 锁

### void zcoroutine_mutex_unlock(zcoroutine_mutex_t *);

* 解锁


## 函数: 条件

### zcoroutine_cond_t * zcoroutine_cond_create();

* 创建条件

### void zcoroutine_cond_free(zcoroutine_cond_t *);

* 释放

### void zcoroutine_cond_wait(zcoroutine_cond_t *, zcoroutine_mutex_t *);

* 条件等待, 参考 pthread_cond_wait

### void zcoroutine_cond_signal(zcoroutine_cond_t *);

* 条件信号, 参考 pthread_cond_signal

### void zcoroutine_cond_broadcast(zcoroutine_cond_t *);

* 条件广播, 参考 pthread_cond_broadcast

## 函数: IO协程化

```
/* 慢操作线程池的线程个数, 需要在协程环境运行前赋值 */
extern int zvar_coroutine_block_pthread_count_limit;

/* 是否开启文件 IO 协程化, 需要在协程环境运行前赋值 */
extern zbool_t zvar_coroutine_fileio_use_block_pthread;
```

**如果**

```
zvar_coroutine_block_pthread_count_limit > 0
```
**并且**

```
zvar_coroutine_fileio_use_block_pthread == 1
```

**则**

* 文件 IO 在线程池执行

**否则**

* 在当前线程执行(会阻塞其他协程)

## 函数: 慢操作

### void *zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx);

* 如果 (zvar_coroutine_block_pthread_count_limit &gt; 0)
* 则 block_func(ctx) 在线程池执行
* 否则在当前线程直接执行(会阻塞其他协程)

### int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset);<BR />int zcoroutine_block_write(int fd, const void *data, int len);<BR />long zcoroutine_block_lseek(int fd, long offset, int whence);<BR />int zcoroutine_block_open(const char *pathname, int flags, mode_t mode);<BR />int zcoroutine_block_close(int fd);<BR />int zcoroutine_block_rename(const char *oldpath, const char *newpath);<BR />int zcoroutine_block_unlink(const char *pathname);

* zcoroutine_block_XXX 基于 zcoroutine_block_do 机制, 封装最常见的 IO 操作
* 此时不必激活 (zvar_coroutine_fileio_use_block_pthread == 1)
* 如果激活 zvar_coroutine_fileio_use_block_pthread == 1), 会导致本框架支持的所有的文件IO都在线程池执行

## 例子

* https://gitee.com/linuxmail/lib-zo/
* https://gitee.com/linuxmail/lib-zo/blob/master/

