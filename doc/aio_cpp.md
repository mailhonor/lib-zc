<A name="readme_md" id="readme_md"></A>

## 异步IO开发框架, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[C版本](./aio.md)

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

```c++
zcc::aio_base * aiobase = new zcc::aio_base()
```

### 第二步 具体业务

举例: 监听端口 fd 是不是可读(比如有connect请求)

先根据 fd 创建一个 IO事件 ev

```c++
zcc::aio *ev = new zcc::aio(fd, aiobase);
```

再设置: 可读的时候执行执行函数 brefore_accept

```c++
void zcc::aio::readable(std::function<void()> callback);
ev->readable(std::bind(before_accept, ev)); 
```
方法 ev->readable 会立即返回

### 第三步 事件循环

```c++
aiobase->run();
```

aiobase->run() 阻塞运行, 其内部是epoll循环

### 第四步 释放运行环境

```c++
delete aiobase;
```

## 异步 IO事件

### 例子: 创建一个回显服务器, 监听 0:8899 端口

```c++
// 监听 0:8899
int fd = zcc::inet_listen("0:8899", 0, 5);

// 设置: fd 非阻塞
zcc::nonblocking(fd);

// 创建: IO事件
zcc::aio *ev = new zcc::aio(fd, aiobase);

// 设置: 监听可读
nev->readable(std::bind(before_accept, ev));
```

### before_accepet 的实现

```c++
static void before_accept(zcc::aio *ev)
{
    // 获取 ev 绑定的文件描述符, 注意: 这个 sock 是非阻塞的
    int sock = ev->get_fd()

    // 下面的 fd 是阻塞的 
    int fd = zcc::inet_accept(sock);
    if (fd < 0) {
        return;
    }

    // 再创建一个IO事件
    zcc::aio *nev = new zcc::aio(fd, aiobase);
    
    // 设置: 如果 fd 可读则回调执行 do_echo
    nev->readable(std::bind(do_echo, nev));
}
```

### do_echo 的实现

```c++
static void do_echo(zcc::aio *ev) 
{
    // 注意: 此 fd 可能可读, 也可能有异常了
    int fd= ev->get_fd();

    // 题外话: 因为 fclose 会关闭 fd, 所以通过 dup(fd) 生成了新的文件描述符
    FILE *fp = fdopen(dup(fd), "a+");
    char buf[1024000+1];
    // 阻塞读一行
    fgets(buf, 1024000, fp);
    // 阻塞写回去
    fputs(buf, fp);
    fclose(fp);

    // 继续, 有好几种选择
    if (1) {
        // 可读时, 执行 do_echo
        ev->readable(std::bind(do_echo, ev));
        return;
    } 

    if (0) {
        // 或 可写时, 执行 do_write
        ev->readable(std::bind(do_write, ev));
        return;
    }
    if (0) {
        // 或者暂停, 则禁用 ev
        ev->disable();
        return;
    }
    if (0) {
        // 或者, 终止
        delete ev;
        return;
    }
    if (0) {
        // 或者, 其他业务 
        // 不关闭 fd, 只释放 ev 本身资源, 其他业务继续操作此 fd
        ev->close(false);
        delete ev;
        return;
    }
}
```

## 异步 IO读写

### 创建一个回显服务器, 监听8899端口

参考 "异步 IO事件"

### before_accept的实现2

```c++
static void before_accept(zcc::aio *ev)
{
    int sock = ev->get_fd();
    int fd = acc::inet_accept(sock);
    if (fd < 0) {
        return;
    }   

    // 设置: fd 为 非阻塞 模式
    zcc::nonblocking(fd);

    // 创建: 异步io
    zcc::aio aio = new zcc::aio(fd, aiobase);

    // 为了方便演示, 这里假设, 一行的长度不超过10240
    // 设置: 读一行, 最多读取10240个字节后, 执行回调函数 after_read
    aio->gets(10240, std::bind(after_read, aio));
}
```

### after_read 的实现

```c++
static void after_read(zcc::aio *aio)
{
    int ret = aio->get_result();
    if (ret < 1) {
        // 错, 释放 aio, 并 close(aio的fd)
        delete aio;
        return;
    }   

    std::string buf;

    // 把上次读取的行数据保存到 buf
    aio->get_read_cache(buf, ret);
    
    // 写字符串到缓存
    aio->cache_write("your input:   ");

    // 写固定长度数据到缓存
    aio->cache_write(buf);

    // 刷写缓存的数据, 写完后(成功或失败)执行 after_write
    aio->cache_flush(std::bind(after_write, aio)); 
}
```

### after_write 的实现

```c++
static void after_write(zcc::aio *aio)
{
    int ret = aio->get_result();
    if (ret < 1) {
        // 错, 释放 aio, 同时自动 close(aio的fd)
        delete aio;
        return;
    }   
    // 这个时候, 回显已经完成, 继续读取客户端发起的行请求
    aio->gets(10240, std::bind(after_read, aio));
}
```

## 异步 IO 读写, SSL

接上节, 假设 0:8899 是 ssl 端口

### before_accept 的实现3

```c++
static void before_accept(zcc::aio *aio)
{
    int sock = ev->get_fd();
    int fd = zcc::inte_accept(sock);
    if (fd < 0) {
        return;
    }

    // 设置: fd 为 非阻塞 模式
    zcc::nonblocking(fd); 

    /* 创建: 异步io */
    zcc::aio aio = new zcc::aio(fd, aiobase);
    
    // 设置: 完成 SSL accept 后, 执行 after_ssl_accept
    // sslctx 是 openssl的 SSL_CTX *
    aio->tls_accept(sslctx,  std::bind(after_ssl_accept, aio));
}
```

### after_ssl_accept 的实现

```c++
static void after_ssl_accept(zcc::aio *aio)
{
    int ret = aio->get_result();
    if (ret < 1) {
        // 错, (SSL accept 失败), 释放 aio, 并 close(aio的fd) 
        delete aio;
        return;
    }
    // 这个时候, SSL 握手成功;
    // 此后, 逻辑上, 代码和上节相同 
    aio->gets(10240, std::bind(after_read, aio));
}
```

## 定时器

参考 "框架一般模型" 部分

// 方法1

```c++
//
zcc::aio_timer *tm = new zcc::aio_timer(aiobase);
// 1 秒后执行函数 foo1
tm->after(std::bind(foo1, tm), 1);

// foo1 的实现
static void foo1(zcc::aio_timer *timer)
{
    // 此时, 定时器的注册函数已经注销 
    fprintf(stderr, "%s\n", "log1....................");
    // 再次注册, 1 秒后执行函数 foo1
    tm->after(std::bind(foo1, timer), 1);
    // 如果不在需要此 timer,则:
    // delete tm
}
```
方法2

```c++
// 10 秒后执行函数 foo2 
aiobase->enter_timer(foo2, 10);

// foo2 的实现
static void foo2()
{
    fprintf(stderr, "%s\n", "log2....................");
}
```

## 跨线程操作

如果希望在其他线程操作 aio:

前提是: 每次(_注意:是每次_)执行aio(_zcc::aio *_)的一个回调函数后, 执行:

```c++
aio->disable();
```

之后就可以在其他线程操作了

## 基础类

### 类, 事件 zcc::aio

```c++
class aio
{
    friend aio_friend;

public:
    aio();
    aio(int fd, aio_base *aiobase = nullptr);
    aio(SSL *ssl, aio_base *aiobase = nullptr);
    virtual ~aio();
    // 重新绑定 fd
    void rebind_fd(int fd, aio_base *aiobase = nullptr);
    // 重新绑定 SSL
    void rebind_SLL(SSL *ssl, aio_base *aiobase = nullptr);
    // 关闭aio,  close_fd_and_release_ssl==true 会同时释放fd/SSL资源
    void close(bool close_fd_and_release_ssl = true);
    // 重新绑定 aio_base
    void rebind_aio_base(aio_base *aiobase);
    // 设置可读写超时
    void set_timeout(int wait_timeout);
    // 停止 aio, 只能在所属 aio_base 运行的线程执行
    void disable();
    // 获取结果, -2:超时(且没有任何数据), <0: 错, >0: 写成功,或可读的字节数
    int get_result();
    // 获取 fd
    int get_fd();
    // 获取可读的缓存数据的长度
    int get_read_cache_size();
    // 从可读缓冲区严格读取长度为strict_len的数据
    void get_read_cache(char *buf, int strict_len);
    void get_read_cache(std::string &bf, int strict_len);
    // 获取写缓存数据的长度
    int get_write_cache_size();
    void get_write_cache(char *buf, int strict_len);
    void get_write_cache(std::string &bf, int strict_len);
    // 如果可读(或出错)则回调执行函数 callback
    void readable(std::function<void()> callback);
    // 如果可写(或出错)则回调执行函数 callback
    void writeable(std::function<void()> callback);
    // 请求读, 最多读取max_len个字节, 成功/失败/超时后回调执行callback
    void read(int max_len, std::function<void()> callback);
    // 请求读, 严格读取strict_len个字节, 成功/失败/超时后回调执行callback
    void readn(int strict_len, std::function<void()> callback);
    /* */
    // 请求读, 读到delimiter为止, 最多读取max_len个字节, 成功/失败/超时后回调执行callback
    void read_delimiter(int delimiter, int max_len, std::function<void()> callback);
    // 如上, 读行
    inline void gets(int max_len, std::function<void()> callback) { read_delimiter('\n', max_len, callback); }
    // compact data
    void get_cint(std::function<void()> callback);
    void get_cint_and_data(std::function<void()> callback);
    // 向缓存写数据
    int cache_write(const void *buf, int len = -1);
    inline int cache_write(const std::string &bf) { return cache_write(bf.c_str(), bf.size()); }
    inline int cache_append(const std::string &bf) { return cache_write(bf); }
    inline int cache_append(const char *s) { return cache_write(s); }
    inline int cache_puts(const char *s) { return cache_write(s); }
    // compact data
    void cache_write_cint(int len);
    void cache_write_cint_and_data(const void *data, int len);
#ifdef _WIN64
    void cache_printf_1024(const char *fmt, ...);
#else  // _WIN64
    void __attribute__((format(gnu_printf, 2, 3))) cache_printf_1024(const char *fmt, ...);
#endif // _WIN64
    /* */
    // 向缓存写数据, 不复制buf, 直接挂在buf到写链条上, 在flush完毕前要保证此buf可用
    void cache_write_direct(const void *buf, int len);
    // 请求写, 成功/失败/超时后回调执行callback
    void cache_flush(std::function<void()> callback);
    /* */
    // 请求sleep, sleep秒后回调执行callback
    void sleep(std::function<void()> callback, int timeout);
    // 获取 aio_base
    aio_base *get_aio_base();
    // 获取 SSL 句柄
    SSL *get_ssl();
    // 发起tls连接, 成功/失败/超时后回调执行callback
    void tls_connect(SSL_CTX *ctx, std::function<void()> callback);
    // 发起tls接受, 成功/失败/超时后回调执行callback
    void tls_accept(SSL_CTX *ctx, std::function<void()> callback);

protected:
    aio_engine *engine_{nullptr};
};
```

### 类,定时器 zcc::aio_timer

```c++
// 定时器
class aio_timer : public aio
{
public:
    aio_timer(aio_base *aiobase = nullptr);
    virtual ~aio_timer();
    void rebind_aio_base(aio_base *aiobase = nullptr);
    void after(std::function<void()> callback, int timeout);
};

### 类,环境 zcc::aio_base
```c++
class aio_base
{
    friend aio_friend;

public:
    static aio_base *get_current_thread_aio_base();

public:
    aio_base();
    ~aio_base();
    // 设置 aio_base 每次epoll循环需要执行的函数
    void set_loop_fn(std::function<void()> callback);
    // 运行 aio_base
    void run();
    // 通知 aio_base 停止, 既 zaio_base_run 返回
    void stop_notify();
    // 通知 aio_base, 手动打断 epoll_wait
    void touch();
    // sleep秒后回调执行callback, 只执行一次 callback(ctx)
    void enter_timer(std::function<void()> callback, int timeout);

protected:
    aio_base_engine *engine_;
};
```

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/cpp_sample/event/

