# 常见带超时io函数封装

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了常见的带超时io函数

PS: 除非函数名或其他特别标注, 所有timeout单位都是秒, -1表示无限长

### 带超时等待可读写

```
/* <0: 出错  0: 不可读写, 1: 可读写或socket异常 */
int ztimed_read_write_wait(int fd, int read_write_wait_timeout, int *readable, int *writeable);
int ztimed_read_write_wait_millisecond(int fd, long read_write_wait_timeout, int *readable, int *writeable);

```

### 带超时等待可读

```
/* <0: 出错  0: 不可读, 1: 可读或socket异常 */
int ztimed_read_wait_millisecond(int fd, long read_wait_timeout);
int ztimed_read_wait(int fd, int read_wait_timeout);

```

### 带超时等待可写

```
/* <-: 出错  0: 不可写, 1: 可写或socket异常 */
int ztimed_write_wait_millisecond(int fd, long write_wait_timeout);
int ztimed_write_wait(int fd, int write_wait_timeout);

```

### 带超时读

```
/* < 0: 出错, >0: 正常, 0: 不可读 */
int ztimed_read(int fd, void *buf, int size, int read_wait_timeout);

```

### 带超时写

```
/* < 0: 出错, >0: 正常, 0: 不可写 */
int ztimed_write(int fd, const void *buf, int size, int write_wait_timeout);
```
