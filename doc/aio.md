# 异步io开发框架

[LIB-ZC](https://gitee.com/linuxmail/lib-zc)是一个C扩展库. 支持异步IO开发模型

基于epoll, 支持 IO事件, IO读写(支持SSL), 定时器

## 框架特色

- 读一行, 执行回调函数
- 读指定长度后, 执行回调函数
- 写指定长度后, 执行回调函数
- SSL连接后后, 执行回调函数
- 触发可读后, 执行回调函数
- 触发可写后, 执行回调函数
- 超时后, 执行回调函数
- 跨线程操作

## 创建运行环境

首先说明, 可以创建多个运行环境. 每个运行环境独占用一个线程.

第一步  创建运行环境 

```
zaio_base_t * aiobase = zaio_base_create();
```

第二步 开启具体业务 

支持 IO事件, IO读写(支持SSL), 定时器

举例 监听端口fd是不是可读(比如有connect请求).

先根据fd创建一个io事件 ev

```
zaio_t *ev = zaio_create(fd, aiobase);
```

在可读的时候执行函数 before_accept

```
/* 下面这个函数立即返回, 等到fd可读则执行函数 before_accept */
zaio_readable(ev, before_accept); 
```

第三步 事件循环 

*等到 fd可读则执行函数 before_accept*

```
/* loop_fn: 每次事件循环执行函数 */
void zaio_base_run(zaio_base_t *eb, void (*loop_fn)());
zaio_base_run(aiobase, 0);
```

第四步  释放运行环境

```
zaio_base_free(aiobase);
```

## IO事件

例子, 创建一个回显服务器, 监听8899端口.

参考 "创建运行环境"部分, 

```
/* zlisten是listen函数的封装.*/
/* 监听0:8899, backlog是5 */
int fd = zlisten("0:8899", 0, 5);
znonblocking(fd, 1);
zaio_t *ev = zaio_create(fd, aiobase);

/* 可读后执行 before_accept */
zaio_readable(ev, before_accept); 
```

before_accept的实现

```
static void before_accept(zaio_t * ev) 
{
    int sock = zaio_get_fd(ev);
    
    /* zinet_accept 是accept的封装, 忽略 EINTR */
    /* 注意, 这个fd是阻塞的 */
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }
    /* 再创建一个IO事件 */ 
    zaio_t *nev = zaio_create(fd, aiobase);
    
    /* 如果可读则回调执行 do_echo */
    zaio_readable(nev, do_echo);
}
```

do_echo的实现

```
static void do_echo(zaio_t *ev) 
{
    int fd= zaio_get_fd(ev);
    /* 现在得到fd了, 注意: 这fd可能可读, 也可能有异常了 */
    /* 可以读一行数据, 然后写回去了 */
    /* 根据fd是不是阻塞,读写方式不太一样, 我们这处理的是阻塞的fd */
    /* 题外话, 因为fclose会关闭fd, 所以通过dup(fd)生成了新的文件描述符 */

    FILE *fp = fdopen(dup(fd), "a+");
    char buf[1024000+1];
    fgets(buf, 1024000, fp);
    fputs(buf, fp);
    fclose(fp);

    if (1) {
        /* 继续等可读 */
        zaio_readable(ev, do_echo);
    } 

    if (0) {
        /* 或者监听可写 */
        zaio_write_able(ev, do_write); */
    }
    if (0) {
        /* 或者暂停, 则禁用ev */
        zaio_disable(ev);
    }
    if (0) {
        /* 或者, 终止 */
        zaio_free(ev, 1);
    }
    if (0) {
        /* 或者, 其他业务 */
        /* 不关闭fd, 只释放ev本身资源, 应用 fd 继续其他业务 */
        zaio_free(ev, 0);
    }
}
```

## 异步IO

例子, 创建一个回显服务器, 监听8899端口.

参考 "创建运行环境", 和 "IO事件" 部分, 

before_accept的实现2

```
static void before_accept(zaio_t * ev) 
{
    int sock = zaio_get_fd(ev);
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }   

    /* 设置 fd 为 非阻塞 模式 */
    znonblocking(fd, 1); 

    /* 创建 异步io */
    zaio_t aio = zaio_create(fd, aiobase);

    /* 读一行, 最多读取10240个字节 后, 执行回调函数after_read */
    zaio_gets(aio, 10240,  after_read);
}
```

after_read的实现

```
static void after_read(zaio_t * aio)
{
    ZSTACK_BUF(buf, 10240);
    /* zaio_get_result(aio) 获取上次命令执行的结果 */
    /* 上次命令是读, 则返回的是已读的长度 */
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, 释放aio, 并close(aio的fd) */
        zaio_free(aio, 1);
        return;
    }   

    /* 为了方便演示, 这里假设, 一行的长度不超过10240 */
    /* 把上次读取的行数据保存到buf, 除了下面函数, 还请参考 zaio_fetch_rbuf */
    zaio_get_read_cache(aio, buf, ret);
    
    /* 写字符串到缓存 */
    zaio_cache_puts(aio, "your input:   ");
    /* 写固定长度数据到缓存*/
    zaio_cache_write(aio, buf, ret);
    /* 刷缓存的数据, 写完后执行 after_write */
    zaio_cache_flush(aio, after_write); 
}
```

after_write的实现

```
static void after_write(zaio_t * aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, 释放aio, 并close(aio的fd) */
        zaio_free(aio, 1);
        return;
   }   
    /* 这个时候, 回显已经完成, 继续读客户端数据 */
    zaio_gets(aio, 10240, after_read);
}
```

##  异步IO,SSL

接上节, 假设8899是ssl端口.

before_accept的实现3

```
static void before_accept(zaio_t * ev) 
{
    int sock = zaio_get_fd(ev);
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    /* 设置 fd 为 非阻塞 模式 */
    znonblocking(fd, 1); 
    /* 创建 异步io */
    zaio_t aio = zaio_create(fd, aiobase);
    
    /* 完成SSL accept 后, 执行after_ssl_accept */
    zaio_tls_accept(aio, sslctx,  after_ssl_accept);

    /* 上一行代码中的 sslctx 是 openssl的 SSL_CTX * */
    /* 可以通过其他方法创建 */
    /* 也可以应用LIB-ZC封装的方法, zopenssl_SSL_CTX_create_server  */ 
}
```

after_ssl_accept 的实现

```
static void after_ssl_accept(zaio_t * aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        /* 错, (SSL accept 失败), 释放aio, 并close(aio的fd) */
        zaio_free(aio, 1);
        return;
    }
    /* 这个时候, SSL accept握手成功, 此后, 逻辑上, 代码上和上节相同 */
    zaio_gets(aio, 10240, after_read);
}
```

## 定时器

例子, 创建三个定时器

参考 "创建运行环境"部分,

```
zaio_t *tm;

/* 第一个timer, 每一秒,输出一条日志 */
tm = zaio_create(-1, aiobase);
zaio_sleep(tm,  foo1, 1);

/* 第二个timer,  3秒后输出一条日志 */
tm = zaio_create(-1, aiobase);
zaio_sleep(tm,  foo2, 3);

/* 第三个timer, 10秒后进程退出 */
zaio_sleep(zaio_create(-1, aiobase), foo3, 10);

```

第一个timer, 每一秒,输出一条日志 

```
static void foo1(zaio_t *zt)
{
    /* 此时, 定时器的注册函数已经注销 */
	  printf(stderr, "%s\n", "log1....................");
	  zaio_sleep(zt, foo1, 1);
}
```

第二个timer,  3秒后输出一条日志

```
static void foo2(zaio_t *zt)
{
	printf(stderr, "%s\n", "log2....................");
	zaio_free(zt);
}
```

第三个timer, 10秒后进程退出

```
static void foo3(zaio_t *zt)
{
	printf(stderr, "%s\n", "log3....................");
	zaio_free(zt);
	exit(1);
}
```

## 跨线程操作

已知一个

```
zaio_t * aio;
```

*每次(注意:是每次)*执行aio的一个回调函数结束后, 希望在其他线程操作aio. 则前提是必须先执行

```
zaio_disable(aio);
```

之后就可以可以在其他线程操作了
