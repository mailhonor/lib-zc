
## IO 流(stream), [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持IO 流(stream), 其 STRUCT 类型是 **zstream_t**

[C++版本](./stream_cpp.md)

## 数据结构

```
#define zvar_stream_rbuf_size           4096
#define zvar_stream_wbuf_size           4096

struct zstream_t {
    int read_wait_timeout;
    int write_wait_timeout;
    void *ioctx;
    zstream_engine_t *engine;
    short int read_buf_p1;
    short int read_buf_p2;
    short int write_buf_len;
    unsigned short int error:1;
    unsigned short int eof:1;
    unsigned char read_buf[zvar_stream_rbuf_size];
    unsigned char write_buf[zvar_stream_wbuf_size];
};
```

## 函数: 基本操作

### zstream_t *zstream_open_fd(int fd);

* 创建
* 根据文件描述符 fd 创建, fd 一般是 socket

### zstream_t *zstream_open_ssl(SSL *ssl);

* 创建
* 根据 ssl 创建

### zstream_t *zstream_open_file(const char *pathname, const char *mode);

* 创建
* 打开本地文件
* 返回 0: 失败
* mode: "r", "r+", "w", "w+", "a", "a+"

_除非了解使用本方法的优点, 否则不推荐使用, 建议使用标准 C 库的 FILE_

### zstream_t *zstream_open_destination(const char *destination, int timeout);

* 创建
* 打开地址 destination
* 返回 0: 失败
* timeout: 超时
* destination: 目标地址, 参考 [zconnect](./tcp_socket.md)

### int zstream_close(zstream_t *fp, zbool_t release);

* 如果 release == 0: 则仅仅释放 stream;
* 如果 release == 1: 则释放 stream, 同时关闭 fd 或 释放 ssl


## 函数: 读

### int ZSTREAM_GETC(zstream_t *fp);

* 宏, 返回读取的下一个字符
* 返回 -1: 错

### int zstream_getc(zstream_t *fp);

* inline, 返回读取的下一个字符
* 返回 -1: 错

### int zstream_read(zstream_t *fp, zbuf_t *bf, int max_len);

* 最多读 max_len 个字节到(覆盖) bf
* 返回 -1: 错
* 返回 0: 不可读
* 返回 &gt;0: 读取字节数

### int zstream_readn(zstream_t *fp, zbuf_t *bf, int strict_len);

* 严格读取strict_len个字符
* 其他同上

### int zstream_gets_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len);

* 读取最多 max_len 个字符, 读取到 delimiter
* 其他同上

### int zstream_gets(zstream_t *fp, zbuf_t *bf, int max_len);

* 分隔符为换行符 '\n'
* 其他同上

### int zstream_timed_read_wait(zstream_t *fp, int read_wait_timeout);

* 超时等待可读
* 返回 -1: 出错
* 返回 0: 不可读
* 返回 1: 可读 或 异常

### void zstream_set_read_wait_timeout(zstream_t *fp, int read_wait_timeout);

* 设置可读超时

## 函数: 写

### int ZSTREAM_PUTC(zstream_t *fp, int ch);

* 宏, 写字节 ch
* 返回 ch
* 返回 -1: 出错

### int zstream_putc(zstream_t *fp, int c);

* inline, 写一个字节 ch
* 返回 ch
* 返回 -1: 出错

### int zstream_write(zstream_t *fp, const void *buf, int len);

* 写长度为 len 的 buf 到 fp
* 返回 -1:失败
* 返回 &gt;=0: 成功写入字节数

### int zstream_puts(zstream_t *fp, const char *line);

* 写一行 line 到 fp
* 返回 -1: 失败
* &gt;=0: 成功写入字节数

### int zstream_append(zstream_t *fp, zbuf_t *bf);

* 写 bf 到 fp
* 返回 -1:失败
* 返回 &gt;=0: 成功写入字节数

### int zstream_flush(zstream_t *fp);

* 刷新写缓存

### int zstream_timed_write_wait(zstream_t *fp, int write_wait_timeout);

* 超时等待可写
* 返回 -1: 出错
* 返回 0: 不可写
* 返回1: 可写 或 异常

### int zstream_printf_1024(zstream_t *fp, const char *format, ...);

* 格式化写
* zstream_printf_1024 逻辑上是这个意思:

```
char buf[1024+1];
sprintf(buf, format, ...);
zstream_puts(fp, buf);
```

## 函数: SSL

### int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx);

* ssl 握手连接 tls_connect
* 返回 -1: 错
* 返回 1: 成功

### int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx);

* ssl 握手接受 tls_accept
* 返回 -1: 错
* 返回 1: 成功

### SSL *zstream_get_ssl(zstream_t *fp);

* 返回 SSL 指针
* 如果不是 SSL 连接则返回 0

## 函数: 属性

### int zstream_get_fd(zstream_t *fp);

* 返回 fd;
* 如果是 ssl 连接则返回对应的 fd

### zbool_t zstream_is_error(zstream_t *fp);

* 是否出错

### zbool_t zstream_is_eof(zstream_t *fp);

* 是否读到结尾

### zbool_t zstream_is_exception(zstream_t *fp);

* 是否异常(错或读到结尾)

### int zstream_get_read_cache_len(zstream_t *fp);

* 返回读缓存的长度 

### void zstream_set_write_wait_timeout(zstream_t *fp, int write_wait_timeout);

* 设置可写超时

## 例子: 打开 fd

* 已知 fd(一般是 socket)

```
void foo(int fd)
{
    zstream_t *fp =  zstream_open_fd(fd);
    bar();
    zstream_close(fp, 1); /* 或 zstream_close(fp, 0); close(fd); */
}
```

## 例子: 打开已经存在 SSL

```
void foo(SSL *ssl)
{
    zstream_t *fp =  zstream_open_ssl(ssl);
    bar();
    zstream_close(fp, 1);
}
```

## 例子: 打开 destination

```
void foo(const char *destination, int timeout)
{
    zstream_t *fp = zstream_open_destination(destination, timeout);
    if (fp) {
        bar();
        zstream_close(fp, 1);
    }
}
foo("127.0.0.1:25;/opt/smtpd_socket;somehost.com:25", 10);

```

## 例子: SSL accept

```
void foo(zstream_t *fp, SSL_CTX *ssl_ctx)
{
    bar1();
    if (zstream_tls_connect(fp,  ssl_ctx) < 0) {
        /* 出错了*/
    }
    bar2();
    zstream_close(fp, 1);
}
```

## 例子: 连接 smtp 服务器

[goto](../blob/master/sample/stream/io.c)

这个例子,连接smtp服务器, 做几个简单的smtp协议, 逻辑流程如下

根据参数的不同组合(见程序):

* 直接连接ssl端口(465), 执行 smtp 协议
* 连接非ssl端口(25), 执行 smtp 协议
* 连接非ssl端口(25), 执行 smtp 协议, 执行 STARTTLS (既开始 SSL 连接), 执行 smtp 协议

