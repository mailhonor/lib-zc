<A name="readme_md" id="readme_md"></A>

[C版本](./io.md)

## 常见的 IO 函数, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了常见的 IO 函数

命名空间 zcc

```c++
// 是否可读/可写
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable);
int timed_read_write_wait(int fd, int read_wait_timeout, int *readable, int *writeable);
// 带超时, 是否可读
int timed_read_wait_millisecond(int fd, int wait_timeout);
int timed_read_wait(int fd, int wait_timeout);
// 超时读
int timed_read(int fd, void *buf, int size, int wait_timeout);
// 带超时, 是否可写
int timed_write_wait_millisecond(int fd, int wait_timeout);
int timed_write_wait(int fd, int wait_timeout);
// 超时写
int timed_write(int fd, const void *buf, int size, int wait_timeout);
// 是否可读写
int rwable(int fd);
// 是否可读
int readable(int fd);
// 是否可写
int writeable(int fd);
// 设置fd阻塞
int nonblocking(int fd, bool tf = true);
#ifdef _WIN64
int close(HANDLE fd);
#else  // _WIN64
int close(int fd);
#endif // _WIN64
// close_on_exec
int close_on_exec(int fd, bool tf = true);
// 获取真实可读字节数
int get_readable_count(int fd);
#ifdef __linux__
// 跨父子进程接收 fd
int recv_fd(int fd);
// 跨父子进程发送 fd
int send_fd(int fd, int sendfd);
#endif // __linux__
```
