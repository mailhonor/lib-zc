## stream流(支持ssl)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

zstream_t 是流封装, 支持openssl

## 数据结构

```
#define zvar_stream_rbuf_size           4096
#define zvar_stream_wbuf_size           4096

struct zstream_t {
    short int read_buf_p1;
    short int read_buf_p2;
    short int write_buf_len;
    unsigned short int error:1;
    unsigned short int eof:1;
    unsigned short int ssl_mode:1;
    unsigned short int file_mode:1;
    unsigned char read_buf[zvar_stream_rbuf_size];
    unsigned char write_buf[zvar_stream_wbuf_size];
    long cutoff_time;
    union { int fd; SSL *ssl; } ioctx;
};
```

## 函数

### 创建stream

```
/* 基于文件描述符fd创建stream, fd一般是socket */
zstream_t *zstream_open_fd(int fd);

/* 基于ssl创建stream */
zstream_t *zstream_open_ssl(SSL *ssl);

/* 打开本地文件, mode: "r", "r+", "w", "w+", "a", "a+" */
/* 不推荐使用, 建议使用 FILE */
zstream_t *zstream_open_file(const char *pathname, const char *mode);

/* 打开地址destination, timeout:是超时, 单位秒. destination: 见 zconnect */
zstream_t *zstream_open_destination(const char *destination, int timeout);
```

### 关闭stream

```
/* 关闭stream */
/* close_fd_and_release_ssl: 是否同时关闭相关fd,SSL */
int zstream_close(zstream_t *fp, zbool_t release);
```

### 读

```
/* 宏, 返回读取的下一个字符,  返回 -1:错 */
#define ZSTREAM_GETC(fp)    ... 

/* 超时等待可读, 单位秒, timeout<0: 表示无限长 */
/*返回 -1: 出错  0: 不可读, 1: 可读或socket异常 */
int zstream_timed_read_wait(zstream_t *fp, int timeout);

/* 读取并返回一个字符,  返回 < 0: 错误 */
/* 不应该使用这个函数 */
int zstream_getc_do(zstream_t *fp);

/* 读取并返回一个字符,返回 < 0: 错误 */
zinline int zstream_getc(zstream_t *fp);

/* 读 max_len个字节到bf, 返回 -1: 错, 0: 不可读,  >0: 读取字节数 */
int zstream_read(zstream_t *fp, zbuf_t *bf, int max_len);

/* 严格读取strict_len个字符, 返回值 同上 */
int zstream_readn(zstream_t *fp, zbuf_t *bf, int strict_len);

`/* 读取最多max_len个字符到bf, 读取到delimiter为止, 返回值同上 */
int zstream_gets_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len);

/* 读一行 */
int zstream_gets(zstream_t *fp, zbuf_t *bf, int max_len);
```

### 写

```
/* flush, 返回-1:错 */
int zstream_flush(zstream_t *fp);

/* 宏, 写一个字符ch到fp, 返回 -1: 错 */
#define ZSTREAM_PUTC(fp, ch) ...

/* 超时等待可写, 单位秒 */
/* 返回 <0: 出错  0: 不可写, 1: 可写或socket异常 */
int zstream_timed_write_wait(zstream_t *fp, int timeout);
  
/* 写一个字节ch, 返回-1:错, 返回ch:成功 */
/* 不推荐使用zstream_putc_do, 而是使用 zstream_putc */
int zstream_putc_do(zstream_t *fp, int ch);
                                      
/* 写一个字节ch,  返回-1:错, 返回ch:成功 */
inline int zstream_putc(zstream_t *fp, int c) ;

/* 写长度为len的buf到fp, 返回-1:失败, 其他:成功写入字节数 */
int zstream_write(zstream_t *fp, const void *buf, int len);

/* 写一行s 到fp, 返回-1:失败, 其他:成功写入字节数 */
int zstream_puts(zstream_t *fp, const char *s);

#define zstream_puts_const(fp, s) ...
/* 如 */ zstream_puts_const(fp. "const char start");

/* 写bf到fp, 返回-1:失败, 其他:成功写入字节数 */
int zstream_append(zstream_t *fp, zbuf_t *bf);

/* zstream_printf_1024 逻辑上是这个意思: */
/* char buf[1024+1];  sprintf(buf, format, ...); zstream_puts(fp, buf);  */
int zstream_printf_1024(zstream_t *fp, const char *format, ...);
```

### SSL

```
/* 发起openssl 连接tls_connect,  返回-1:错, */
int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx);

/* 发起openssl接受 tls_accept,  返回-1:错 */
int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx);

/* 返回ssl */
SSL *zstream_get_ssl(zstream_t *fp);
```

### 属性

```
/* 返回 fd */
/* 如果是openssl连接则返回对应的fd */
int zstream_get_fd(zstream_t *fp);

/* 是否出错 */
zbool_t zstream_is_error(zstream_t *fp) ;

/* 是否读到结尾 */
zbool_t zstream_is_eof(zstream_t *fp);

/* 是否异常(错或读到结尾) */
zbool_t zstream_is_exception(zstream_t *fp) ;

/* 可读缓存的长度 */
int zstream_get_read_cache_len(zstream_t *fp);

/* 设置超时, 单位秒(小于0则无限长) */
void zstream_set_timeout(zstream_t *fp, int timeout);
```

## 例子: 连接fd

```
/* 这个 fd 一般是 socket, 如果需要带超时处理流, fd应该是非阻塞的 */
zstream_t *fp =  zstream_open_fd(fd);
foo();
zstream_close(fp, 1);  /* 或  */
/* zstream_close(fp, 0); close(fd); */
```

## 例子: 直接使用已经存在SSL

```
/* 如果需要带超时处理流, 下面SSL(ssl_ptr)对应的fd应该是非阻塞的 */
zstream_t *fp =  zstream_open_ssl(ssl_ptr);
foo();
zstream_close(fp, 1);  /* 或 */
/*  zstream_close(fp, 0); 此时, 关闭了fp, 但SSL(ssl_ptr)留给了使用者 
    int fd=SSL_get_fd(ssl_ptr);
    SSL_shutdown(ssl_ptr);
    SSL_free(ssl_ptr);
    close(fd); */
```

## 例子: 连接destination

```
zstream_t *fp = zstream_open_destination("1.1.1.1:25", 10);
/* zstream_t *fp = zstream_open_destination("/home/some_domain_socket...", 10); */
foo();
zstream_close(fp, 1);
```

## 例子:发起SSL连接(接受)

```
/* zstream *fp; */
/* SSL_CTX *ssl_ctx */
foo1();
if (zstream_tls_connect(fp,  ssl_ctx) < 0) {
	/* 出错了*/
}
foo2();
zstream_close(fp, 1);
```

## 例子: 见仓库

https://gitee.com/linuxmail/lib-zc/blob/master/sample/stream/io.c

这个例子,连接smtp服务器, 做几个简单的smtp协议, 逻辑流程如下

根据参数的不同组合(见程序):

- 直接连接ssl端口(465), 执行smtp协议
- 连接非ssl端口(25), 执行smtp协议
- 连接非ssl端口(25), 执行smtp协议, 执行STARTTLS(既开始SSL连接), 执行smtp协议

