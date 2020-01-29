# 协程框架

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库, 内嵌协程框架.

本协程框架支持文件io, 锁, 条件等, 支持静态编译.

代码实现参考了libco, libgo, nodejs等. 

如果没使用LIB-ZC, 且仅仅简单使用协程, 则不推荐使用本框架.

在下列情况,推荐使用本协程框架:

> 已经使用LIB-ZC

> 需要文件io支持协程. (文件读写, chmod,mkdir,readdir, flock  等)

> 有慢操作, 需要非协程线程执行

例子参考 https://gitee.com/linuxmail/lib-zc/tree/master/sample/coroutine

## 特别提示

关于dns解析

如果GLIBC的resolv库支持res_ninit, 则不要使用dns解析,直接用ip

## 运行环境

支持多个协程运行环境, 每个运行环境占用一个线程.

协程全部函数**线程不安全**.

在某线程内, 运行协程环境, 步骤如下

第一步, 初始化协程环境

```
zcoroutine_base_init();
```

第二步, 至少执行一个 zcoroutine_go, 如

```
/* foo是协程回调函数, 第二个参数是foo的参数, 第三个参数是协程栈空大小(0默认) */
zcoroutine_go(foo, 0, 0);
```

第三步,  运行环境

```
/* loop_fn: 每次事件循环时执行行数 */
void zcoroutine_base_run(void (*loop_fn)());
zcoroutine_base_run(0);
```

第四步,释放环境

```
zcoroutine_base_fini();
```

## 核心函数 zcoroutine_go;

函数原型

```
void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_size);
```

此函数执行后立即返回. 

协程框架启动后, 执行

```
start_job(ctx);
```

## 睡眠

见 "运行环境" 章节

例子, 执行协程 test_sleep1 和 test_sleep2

```
zcoroutine_go(test_sleep1, 0, 0); 
zcoroutine_go(test_sleep2, 0, 0); 
```

test_sleep1, test_sleep2 的实现

```
static void *test_sleep1(void *context)
{
    while(1){
        zcoroutine_sleep_millisecond(1000);
        printf("sleep coroutine_msleep, 1 * 1000(ms)\n");
    }   
    return context;
}

static void *test_sleep2(void *context)
{
    while(1){
        /* sleep 是系统函数, 已经重载了, 支持协程 */
        sleep(10);
        printf("sleep system sleep, 10 * 1000(ms)\n");
    }   
    return context;
}
```

##  回显服务器

见 "运行环境" 章节

例子: 回显服务器, 监听在8899端口, 执行

```
zcoroutine_go(do_listen_8899, 0, 0);
```

函数do_listen_8899

```
void *do_listen_8899(void *context)
{   
    int sock_type;
    int sock = zlisten(server_address, &sock_type, 5, 0);
    while(1) {
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            if (errno == EAGAIN) {
                continue;
            } 
            printf("accept error(%m)\n");
            exit(1);
        }
        printf("accept ok: %d\n", fd);
        void *arg = (void *)((long)fd);
        /* 进入新的协程 */
        zcoroutine_go(echo_service, arg, 0);
    }
    return 0;
}
```

echo_service


```
static void *echo_service(void *context)
{
    int ret;
    int fd = (int)(long)context;
    znonblocking(fd, 1); 
    /* zstream_t 是 流封装 */
    zstream_t *fp = zstream_open_fd(fd);
    zbuf_t *bf = zbuf_create(0);
    while(1) {
        zbuf_reset(bf);
        ret = zstream_gets(fp, bf, 1024);
        if (ret < 0) {
            printf("socket error\n");
            break;
        }   
        if (ret == 0) {
            printf("socket closed\n");
            break;
        }   
        zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
        zstream_flush(fp);
    }   
    zstream_close(fp, 1); 
    zbuf_free(bf);
    return context;
    /* 至此, 此协程资源释放 */
}
```

## 文件io

文件io支持协程的前提条件是

在 zcoroutine_base_init()前执行

```
/* 支持文件io协程化 */
zvar_coroutine_fileio_use_block_pthread = 1;

/* 指定文件io线程池线程个数 > 0 */
zvar_coroutine_block_pthread_count_limit = 3;
```

支持协程的文件io类的系统调用包括

open, openat, close, read, readv, write, writev, lseek, 

fdatasync, fsync,

rename, 	truncate, ftruncate,

rmdir, mkdir, ge'tents

stat, fstat, link, symlink, readlink, unlink

chmod, fchmod,chown,fchown,lchown

## 慢操作

```
zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx);
```

在 zvar_coroutine_block_pthread_count_limit > 0的情况下

会在线程池执行 block_func(ctx) ; 

否则在当前上下文执行

## 锁

创建锁

```
zcoroutine_mutex_t * zcoroutine_mutex_create();
```

锁lock

```
 void zcoroutine_mutex_lock(zcoroutine_mutex_t *m);
```

释放锁

```
void zcoroutine_mutex_unlock(zcoroutine_mutex_t *m)
```

删除锁

```
void zcoroutine_mutex_free(zcoroutine_mutex_t *m);
```

## 条件

创建条件

```
zcoroutine_cond_t *zcoroutine_cond_create();
```

条件等待, 参考 pthread_cond_wait

```
 void zcoroutine_cond_wait(zcoroutine_cond_t *, zcoroutine_mutex_t *);
```

条件信号, 参考 pthread_cond_signal

```
void zcoroutine_cond_signal(zcoroutine_cond_t *);
```

条件广播, 参考 pthread_cond_broadcast

```
void zcoroutine_cond_broadcast(zcoroutine_cond_t *)
```

释放

```
void zcoroutine_cond_free(zcoroutine_cond_t *);
```
