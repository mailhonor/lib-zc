<A name="readme_md" id="readme_md"></A>

## 异步IO开发框架, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 异步 IO 开发框架, 基于 epoll 实现, 支持 IO 事件, IO 读写(支持SSL), 定时器

## 框架特色

* 读一行**后**, 执行回调函数
* 读指定长度**后**, 执行回调函数
* 写指定长度**后**, 执行回调函数
* SSL 握手**后**, 执行回调函数
* 触发可读**后**, 执行回调函数
* 触发可写**后**, 执行回调函数
* 超时**后**, 执行回调函数
* 跨线程操作

## 框架一般模型

首先说明, 可以创建多个运行环境, 每个运行环境独占用一个线程

### 第一步  创建运行环境

```
zaio_base_t * aiobase = zaio_base_create();
```

### 第二步 具体业务

举例: 监听端口 fd 是不是可读(比如有connect请求)

先根据 fd 创建一个 IO事件 ev

```
zaio_t *ev = zaio_create(fd, aiobase);
```

再设置: 可读的时候执行执行函数 before_accept

```
void before_accept(zaio_t * ev);
zaio_readable(ev, before_accept); 
```
zaio_readable 立即返回

* 注册一个可读事件
* 在 fd 可读的时候, 框架执行 before_accept

### 第三步 事件循环

```
zaio_base_run(aiobase, 0);
```
zaio_base_run 阻塞运行

### 第四步 释放运行环境

```
zaio_base_free(aiobase);
```

## 异步 IO事件

### 例子: 创建一个回显服务器, 监听 0:8899 端口

```
 /* 监听 0:8899 */
int fd = zlisten("0:8899", 0, 5);

/* 设置: fd 非阻塞 */
znonblocking(fd, 1);

/* 创建: IO事件 */
zaio_t *ev = zaio_create(fd, aiobase);

/* 设置: 监听可读 */
zaio_readable(ev, before_accept); 
```

### before_accept 的实现

```
static void before_accept(zaio_t * ev) 
{
    /* 获取 ev 绑定的 fd
    int sock = zaio_get_fd(ev);
    
    /* 注意: 这个 sock 是非阻塞的 */
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }
    /* 上面的 fd 是阻塞的 */
    /* 再创建一个IO事件 */ 
    zaio_t *nev = zaio_create(fd, aiobase);
    
    /* 设置: 如果 fd 可读则回调执行 do_echo */
    zaio_readable(nev, do_echo);
}
```

### do_echo 的实现

```
static void do_echo(zaio_t *ev) 
{
    int fd= zaio_get_fd(ev);
    /* 注意: 此 fd 可能可读, 也可能有异常了 */

    /* 题外话: 因为 fclose 会关闭 fd, 所以通过 dup(fd) 生成了新的文件描述符 */
    FILE *fp = fdopen(dup(fd), "a+");
    char buf[1024000+1];
    /* 阻塞读一行 */
    fgets(buf, 1024000, fp);
    /* 阻塞写回去 */
    fputs(buf, fp);
    fclose(fp);

    /* 继续, 有好几种选择 */
    if (1) {
        /* 可读时, 执行 do_echo */
        zaio_readable(ev, do_echo);
        return;
    } 

    if (0) {
        /* 或 可写时, 执行 do_write
        zaio_write_able(ev, do_write); */
        return;
    }
    if (0) {
        /* 或者暂停, 则禁用 ev */
        zaio_disable(ev);
        return;
    }
    if (0) {
        /* 或者, 终止 */
        zaio_free(ev, 1);
        return;
    }
    if (0) {
        /* 或者, 其他业务 */
        /* 不关闭 fd, 只释放 ev 本身资源, 其他业务继续操作 fd */
        zaio_free(ev, 0);
        return;
    }
}
```

## 异步 IO读写

### 创建一个回显服务器, 监听8899端口

参考 "异步 IO事件"

### before_accept的实现2

```
static void before_accept(zaio_t * ev) 
{
    int sock = zaio_get_fd(ev);
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }   

    /* 设置: fd 为 非阻塞 模式 */
    znonblocking(fd, 1); 

    /* 创建: 异步io */
    zaio_t aio = zaio_create(fd, aiobase);

    /* 为了方便演示, 这里假设, 一行的长度不超过10240 */
    /* 设置: 读一行, 最多读取10240个字节后, 执行回调函数 after_read */
    zaio_gets(aio, 10240,  after_read);
}
```

### after_read 的实现

```
static void after_read(zaio_t * aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, 释放 aio, 并 close(aio的fd) */
        zaio_free(aio, 1);
        return;
    }   

    zbuf_t *buf = zbuf_create(10240);

    /* 把上次读取的行数据保存到 buf, 除了下面函数, 还请参考 zaio_fetch_rbuf */
    zaio_get_read_cache(aio, buf, ret);
    
    /* 写字符串到缓存 */
    zaio_cache_puts(aio, "your input:   ");

    /* 写固定长度数据到缓存*/
    zaio_cache_write(aio, buf, ret);

    /* 刷缓存的数据, 写完后执行 after_write */
    zaio_cache_flush(aio, after_write); 

    zbuf_free(buf);
}
```

### after_write 的实现

```
static void after_write(zaio_t * aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, 释放 aio, 并 close(aio的fd) */
        zaio_free(aio, 1);
        return;
   }   
    /* 这个时候, 回显已经完成, 继续读客户端数据 */
    zaio_gets(aio, 10240, after_read);
}
```

## 异步 IO读写, SSL

接上节, 假设 0:8899 是 ssl 端口

### before_accept 的实现3

```
static void before_accept(zaio_t * ev) 
{
    int sock = zaio_get_fd(ev);
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    /* 设置: fd 为 非阻塞 模式 */
    znonblocking(fd, 1); 

    /* 创建: 异步io */
    zaio_t aio = zaio_create(fd, aiobase);
    
    /* 设置: 完成 SSL accept 后, 执行 after_ssl_accept */
    zaio_tls_accept(aio, sslctx,  after_ssl_accept);

    /* 上一行代码中的 sslctx 是 openssl的 SSL_CTX * */
}
```

### after_ssl_accept 的实现

```
static void after_ssl_accept(zaio_t * aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, (SSL accept 失败), 释放 aio, 并 close(aio的fd) */
        zaio_free(aio, 1);
        return;
    }
    /* 这个时候, SSL 握手成功; 此后, 逻辑上, 代码和上节相同 */
    zaio_gets(aio, 10240, after_read);
}
```

## 定时器

参考 "框架一般模型" 部分

```
zaio_t *tm;
tm = zaio_create(-1, aiobase);
/* 1 秒后执行函数 foo1 */
zaio_sleep(tm, foo1, 1);

/* 10 秒后执行函数 foo2 */
zaio_sleep(zaio_create(-1, aiobase), foo2, 10);
```


foo1 的实现
```
static void foo1(zaio_t *zt)
{
    /* 此时, 定时器的注册函数已经注销 */
    printf(stderr, "%s\n", "log1....................");
    /* 再次注册, 1 秒后执行函数 foo1 */
    zaio_sleep(zt, foo1, 1);
}
```

foo2 的实现
```
static void foo2(zaio_t *zt)
{
    printf(stderr, "%s\n", "log2....................");
    /* 执行一次就释放这个 timer */
    zaio_free(zt);
}
```

## 跨线程操作

如果希望在其他线程操作 aio:

前提是: 每次(_注意:是每次_)执行aio(_zaio_t *_)的一个回调函数后, 执行:

```
zaio_disable(aio);
```

之后就可以可以在其他线程操作了

## 函数: 基本操作

### zaio_t *zaio_create(int fd, zaio_base_t *aiobase);<BR />zaio_t *zaio_create_by_fd(int fd, zaio_base_t *aiobase);

* 基于 fd 创建异步io

### zaio_t *zaio_create_by_ssl(SSL *ssl, zaio_base_t *aiobase);

* 基于 ssl 创建异步io

### void zaio_free(zaio_t *aio, int close_fd_and_release_ssl);

* 释放
* close_fd_and_release_ssl == 1: 同时关闭 fd 或 释放 ssl

### void zaio_rebind_aio_base(zaio_t *aio, zaio_base_t *aiobase);

* 重新绑定 zaio_base_t

### int zaio_get_result(zaio_t *aio);

* 返回 -2: 超时(且没有任何数据)
* 返回  &lt;0: 错
* 返回 &gt;0: 成功, 或可读的字节数

### int zaio_get_fd(zaio_t *aio);

* fd 

### SSL *zaio_get_ssl(zaio_t *aio);

* SSL *

### void zaio_set_read_wait_timeout(zaio_t *aio, int read_wait_timeout);

* 设置可读超时

### void zaio_set_write_wait_timeout(zaio_t *aio, int write_wait_timeout);

* 设置可写超时

### void zaio_set_context(zaio_t *aio, const void *ctx);<BR />void *zaio_get_context(zaio_t *aio);

* 设置/获取上下文 

### zaio_base_t *zaio_get_aio_base(zaio_t *aio);

* 获取 zaio_base_t

## 函数: 停止

### void zaio_disable(zaio_t *aio);

* 停止 aio
* 只能在所属 aio_base 运行的线程执行

## 函数: SSL

### void zaio_tls_connect(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio));

* 发起 tls 连接
* 成功/失败/超时后回调执行 callback

### void zaio_tls_accept(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio));

* 发起 tls 接受
* 成功/失败/超时后回调执行 callback

## 函数: 读

### int zaio_get_read_cache_size(zaio_t *aio);

* 返回读缓存数据长度

### void zaio_get_read_cache(zaio_t *aio, zbuf_t *bf, int strict_len);

* 从读缓存中获取数据

### void zaio_readable(zaio_t *aio, void (*callback)(zaio_t *aio));

* 可读(或出错)后回调执行函数 callback

### void zaio_read(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio));

* 请求读, 最多读取 max_len 个字节
* 成功/失败/超时后回调执行 callback

### void zaio_readn(zaio_t *aio, int strict_len, void (*callback)(zaio_t *aio));

* 请求读, 严格读取 strict_len 个字节
* 成功/失败/超时后回调执行 callback

### void zaio_read_delimiter(zaio_t *aio, int delimiter, int max_len, void (*callback)(zaio_t *aio));

* 请求读, 读到 delimiter 为止, 最多读取 max_len 个字节
* 成功/失败/超时后回调执行 callback 

### void zaio_gets(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio))

* inline, 读行, 其他同 zaio_read_delimiter

### void zaio_get_cint(zaio_t *aio, void (*callback)(zaio_t *aio));

* 请求读, 读取[压缩的 int](./cint_and_data.md)
* 成功/失败/超时后回调执行 callback 

### void zaio_get_cint_and_data(zaio_t *aio, void (*callback)(zaio_t *aio));

* 请求读, 读取[压缩的 int](./cint_and_data.md)所指的数据
* 成功/失败/超时后回调执行 callback 

## 函数: 写

### int zaio_get_write_cache_size(zaio_t *aio);

* 返回已经写的缓存数据长度

### void zaio_get_write_cache(zaio_t *aio, zbuf_t *bf, int strict_len);

* 从写缓存中获取数据

### void zaio_writeable(zaio_t *aio, void (*callback)(zaio_t *aio));

* 可写(或出错)后回调执行函数 callback

### void zaio_cache_printf_1024(zaio_t *aio, const char *fmt, ...);

* 向缓存格式化写数据, (fmt, ...) 不能超过1024个字节

### void zaio_cache_puts(zaio_t *aio, const char *s);

* 向缓存写字符串 s

### void zaio_cache_write(zaio_t *aio, const void *buf, int len);

* 向缓存写数据 

### void zaio_cache_write_cint(zaio_t *aio, int len);

* 向缓存写[压缩的 int](./cint_and_data.md)

### void zaio_cache_write_cint_and_data(zaio_t *aio, const void *data, int len);

* 向缓存写[压缩的 int](./cint_and_data.md)和数据

### void zaio_cache_write_direct(zaio_t *aio, const void *buf, int len);

* 向缓存写数据
* 特别注意: 不复制 buf, 只是把 buf 连接到写缓存队列, 使用者需要保证 buf 在上下文的有效性

### void zaio_cache_flush(zaio_t *aio, void (*callback)(zaio_t *aio));

* 请求写, 成功/失败/超时后回调执行 callback

## 函数: sleep

### void zaio_sleep(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout);

* 请求 sleep
* timeout 秒后回调执行callback

## 函数: 环境

### extern zaio_base_t *zvar_default_aio_base;

* 默认 zaio_base_t, master/server 模式下的 zaio_server_main 使用

### zaio_base_t *zaio_base_create();

* 创建 zaio_base_t

### void zaio_base_free(zaio_base_t *eb);

* 释放

### zaio_base_t *zaio_base_get_current_pthread_aio_base();

* 获取当前线程运行的 zaio_base_t

### void zaio_base_run(zaio_base_t *eb, void (*loop_fn)());

* 运行 zaio_base_t
* loop_fn: 每次事件循环执行 loop_fn

### void zaio_base_stop_notify(zaio_base_t *eb);

* 通知 zaio_base_t 停止, 既导致 zaio_base_run 返回 

### void zaio_base_touch(zaio_base_t *eb);

* 通知 zaio_base_t, epoll_wait 立即返回

### void zaio_base_set_context(zaio_base_t *eb, const void *ctx);<BR />void *zaio_base_get_context(zaio_base_t *eb);

* 设置/获取上下文 

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/event/

