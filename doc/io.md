<A name="readme_md" id="readme_md"></A>

## 常见的 IO 函数, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了常见的 IO 函数

## 状态

### int zrwable(int fd);

* 返回 0: 不可读写
* 返回 1: 可读写或 socket 异常 

### int zreadable(int fd);

* 返回 0: 不可读
* 返回 1: 可读或 socket 异常 

### int zwriteable(int fd);

* 返回 0: 不可写
* 返回 1: 可写或 socket 异常 

### int znonblocking(int fd, int flag);

* 如果 flag==1, 设置非阻塞
* 如果 flag==0, 设置阻塞
* 返回 0: 现在是阻塞
* 返回 1: 现在是非阻塞


### int zclose_on_exec(int fd, int flag);

* 如果 flag ==1, 设置 close_on_exec
* 如果 flag == 0, 取消设置 close_on_exec
* 返回 0: 未设置 close_on_exec
* 返回 1: 已设置 close_on_exec

### int zget_readable_count(int fd);

* 可读字节个数

## 进程间传递文件描述符

### int zsend_fd(int fd, int sendfd);

* 传递, 把 sendfd 通过 fd 传出
* 返回值 &gt;= -1: 成功

### int zrecv_fd(int fd);

* 接收, 通过(读取)fd, 获得要传入的文件描述符并返回
* 返回值 &gt;= -1 成功

## 忽略信号 EINTR

下面函数忽略 EINTR信号, 其他方面和对应的系统函数一致

```
int zopen(const char *pathname, int flags, mode_t mode);
ssize_t zread(int fd, void *buf, size_t count);
ssize_t zwrite(int fd, const void *buf, size_t count);
int zclose(int fd);
int zrename(const char *oldpath, const char *newpath);
int zunlink(const char *pathname);
```

