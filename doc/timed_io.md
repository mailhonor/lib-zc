<A name="readme_md" id="readme_md"></A>

## 带超时IO函数封装, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了常见的带超时IO函数

## 超时等待可读写

### int ztimed_read_write_wait(int fd, int read_write_wait_timeout, int *readable, int *writeable);<BR />int ztimed_read_write_wait_millisecond(int fd, long read_write_wait_timeout, int *readable, int *writeable);

* 返回 0: 不可读写
* 返回 1: 可读写或 socket 异常
* 如果可读: *readable 赋值为 1
* 如果可写: *writeable 赋值为 1

### int ztimed_read_wait_millisecond(int fd, long read_wait_timeout);<BR />int ztimed_read_wait(int fd, int read_wait_timeout);

* 返回 0: 不可读
* 返回 1: 可读或 socket 异常

### int ztimed_write_wait_millisecond(int fd, long write_wait_timeout);<BR />int ztimed_write_wait(int fd, int write_wait_timeout);

* 返回 0: 不可写
* 返回 1: 可写或 socket 异常

## 带超时读写

### int ztimed_read(int fd, void *buf, int size, int read_wait_timeout);

* 返回 -1: 错误或超时
* 返回 0: socket关闭;
* 返回 &gt;0: 读取字节数

### int ztimed_write(int fd, const void *buf, int size, int write_wait_timeout);

* 返回 -1: 错误或超时
* 返回 &gt;0: 成功写的字节数

