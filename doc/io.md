# 常见io函数封装

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了常见的io函数

PS: LIB-ZC, 所有和io相关的函数, 如果返回值是 int, 则返回值 < 0 表示出错

## 函数

### 是否可读写

```
/* -1: 出错  0: 不可读写, 1: 可读写或socket异常  */
int zrwable(int fd); 
int zreadable(int fd); 
int zwriteable(int fd); 
```

### 设置(非)阻塞

```
/* no==1, 设置阻塞; no==0,  设置非阻塞 */
/* 返回 -1: 出错, 0: 现在是阻塞, 1: 现在是非阻塞 */
int znonblocking(int fd, int no);
```

### 设置 close_on_exec

```
/* on ==1, 设置;  on == 0, 取消设置 */
/* 返回 -1: 出错, 0: 没设置, 1: 已经设置 */
int zclose_on_exec(int fd, int on);
```

### 获取可读字节个数

```
/* 返回 -1: 出错, >=0: 可读字节数 */
int zget_readable_count(int fd);
```

### 进程间传输fd

```
/* 进程间传递fd, 把sendfd通过fd传出*/
/* 返回 -1: 错, >-1: 成功 */
int zsend_fd(int fd, int sendfd);

/* 进程间接受fd, 通过(读取)fd, 获得要传入的文件描述符并返回 */
/* 返回 -1: 错, >-1: 接受到的fd */
int zrecv_fd(int fd);
```

### 忽略信号EINTR的函数封装

```
/* 和对应的系统函数一致 */
int zopen(const char *pathname, int flags, mode_t mode);
ssize_t zread(int fd, void *buf, size_t count);
ssize_t zwrite(int fd, const void *buf, size_t count);
int zclose(int fd); 
int zflock(int fd, int operation);
int zfunlock(int fd); 
int zrename(const char *oldpath, const char *newpath);
int zunlink(const char *pathname);
```
