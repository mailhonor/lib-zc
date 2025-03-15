<A name="readme_md" id="readme_md"></A>

[C++版本](./master_cpp.md)

## MASTER/SERVER 服务进程管理框架, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持 MASTER/SERVER 服务进程管理框架

此框架下:

* 有两类程序, 管理器程序(master)和服务程序(server)
* master 和 server 是父子进程关系
* SERVER 包括异步 IO 模式和协程模式

## 简要说明

举个简单的例子, 一个邮件系统有 3 个服务: smtpd, imapd, pop3d :

* smtpd 服务器, 25端口, 465端口(ssl), 启动 10 个进程
* imapd 服务器, 143端口, 993端口(ssl), 启动 6 个进程
* pop3d 服务器, 110端口, 启动 1 个进程

master/server 行为:

* 立即启动 10 个 smtpd, 6 个 imapd, 1 个pop3d
* 当发现 某个 server 进程退出后, 后立即再启动一个
* 监听 25, 143等端口但不对外服务
* server 子进程继承父进程打开端口的文件描述符

## master 管理器

### int zmaster_server_main(int argc, char **argv);

* master程序主函数
* 最简单的调用方法:

```
int main(int argc, char **argv)
{
    return zmaster_server_main(argc, argv);
}
```

zmaster_server_main 函数根据命令参数启用相应的功能

### extern void (*zmaster_server_before_service)() = 0;

* zmaster_server_main 执行具体业务前执行的函数

### extern void (*zmaster_server_load_config)(zvector_t *cfs) = 0;

* zmaster_server_main (初始)重新加载各 server 配置的函数
* zvector_t *cfs, 是 zconfig_t *的vector, 一个zconfig_t 对应一个服务<BR>
_zmaster_server_main 会根据参数 "**-C config_dir**" 读取 server 配置文件_

### void zmaster_server_load_config_from_dirname(const char *config_dir_pathname, zvector_t *cfs);

* 加载一个目录下所有 server 的配置

### 参数 -C config_dir

* zmaster_server_main 加载每一个配置文件 config_dir/*.cf 为一个服务

### 参数 -pid-file pid_file

* pid_file: 文件, 存储当前 master 进程的进程id
* master 进程会 flock 锁定文件 pid_file

### 参数 --try-lock

* master 进程尝试锁定文件 pid_file, 并立即返回
* 返回 0: 锁定成功
* 返回 1: 锁定失败

### 启动/停止/重启

* master 进程的启动/停止/重启, 请参考脚本, 这个脚本稍微修改即可通用
* https://gitee.com/linuxmail/lib-zc/blob/master/sample/master/general_master.sh

## server 服务配置

根据参数 **-C config_dir**, master 程序会遍历(无序) config_dir/*.cf, 把每个配置文件(如:config/dir/imapd.cf)作为一个服务启动

例如 

```
[xxx@zytest]$ cat some_config_dir/imapd.cf

# 服务程序
server-command = bin/imapd

# 启动的端口和服务名字(自定义)
# imapd 和 imapd-ssl 是服务名字
# 0:143, 127.0.0.1:1143, 0;993, 0:12143 是监听的网络端口, ./var/socket/imapd 是 domain socket 端口
server-service = imapd://0:143, imapd-ssl://0:993, imapd://127.0.0.1:1143,  imapd://./var/socket/imapd, 0:12143

# 启动几个进程
server-proc-count = 1

# server模式下, 通用的日志配置, 可选
# syslog,mail : 使用syslog, facility 是 mail, 级别固定是 INFO
# masterlog,./var/socket/log : 内置的日志服务, 参考
#     https://gitee.com/linuxmail/lib-zc/blob/master/sample/master/master.sh
server-log = syslog,mail

# 下面是其他配置, server 会读取
abc = xxx
host = linuxmail.cn
```

## server 服务进程: 异步 IO 模式(aio)

此服务模式运行在异步IO环境, 默认已经建好环境 zvar_default_aio_base

### 基本模型
```
static void do_something(int fd)
{
    /* fd 是 accept 后得到的文件描述符, 使用者自己处理, 最后记得要 close */
    foo();
    close(fd);
}

static void _service_register(const char *service_name, int fd, int fd_type)
{
    /* service_name 就是上面的 imapd, imapd-ssl */
    /* fd 是 从 master 继承的监听端口, 应该通过 accept 处理 */
    /* fd_type 可选 zvar_tcp_listen_type_inet, zvar_tcp_listen_type_unix */
    
    /* 下面这个函数是一个通用的, 简单的处理机制 */
    zaio_server_general_aio_register(zvar_default_aio_base, fd, fd_type, do_something);
}

int main(int argc, char **argv)
{

    zaio_server_service_register = _service_register;
    return zaio_server_main(argc, argv);
}
```

### int zaio_server_main(int argc, char **argv)

* 主要函数, argc 和 argv 就是 main 函数的参数, 也就是程序的参数
* 程序会根据 some_config_dir/imapd.cf 的配置启动, (一般情况下)不可以手动启动

### extern void (*zaio_server_service_register) (const char *service, int fd, int fd_type);

* 注册服务
* service 是服务名
* fd继承自master
* fd_type: inet/unix/fifo

### extern void (*zaio_server_before_service)(void) = 0;
* 进入主服务前执行的函数
* 此时 zvar_default_aio_base 已经准备好

### extern void (*zaio_server_before_softstop)(void) = 0;

* 接到 master 重启信号后执行的函数
* 首先, 假设配置项 server-stop-on-softstop-after 的值为 M(秒)
* 如果此函数为 0, zaio_server_main(epoll循环后) M 秒后返回(M 可以是 0)
* 如果此函数不为 0 , zaio_server_main M 秒后返回, 如果 M < 1, 则 M 取 3600

### void zaio_server_stop_notify(int stop_after_second);

* 手动通知主程序 stop_after_second 秒后循环停止, 然后 zaio_server_main 返回

### void zaio_server_detach_from_master();

* 主动和 master 管理器分离
* master 程序会以为此进程已经终止, 会启动一个新的进程
* master 发起重启信号时, 不会再次通知此进程
* 3600 秒后 zaio_server_main 返回

### zaio_t *zaio_server_general_aio_register(zaio_base_t *eb, int fd, int fd_type, void (*callback) (int));

* 通用的服务注册函数
* 逻辑上这么理解:

```
fd2 = accept(fd);
callback(fd2);
```

### 配置

见上文的 some_config_dir/imapd.cf

程序启动后, 会(无序)加载 some_config_dir/*.gcf, 然后再加载 some_config_dir/imapd.cf

## server 服务进程: 协程模式(coroutine) 

此服务模式运行在协程环境

### 基本模型

```
static void *do_accept(void *ctx)
{
    int sock = (int)(long)ctx;
    while (1) {
        int fd = accpet(fd);
        foo();
        close(fd);
    }
    return 0;
}

static void _service_register(const char *service_name, int fd, int fd_type)
{
    /* service_name 就是上面的 imapd, imapd-ssl */
    /* fd 是 从 master 继承的监听端口, 应该通过 accept 处理 */
    /* fd_type 可选 zvar_tcp_listen_type_inet, zvar_tcp_listen_type_unix */

    /* 大部分情况下, 这个 fd 需要 accpet, 一般通过启动新的协程来处理 */
    zcoroutine_go(do_accept, (void *)(long)fd, 0);
}

int main(int argc, char **argv)
{
    zcoroutine_server_service_register = _service_register;
    return zcoroutine_server_main(argc, argv);
}
```

### int zcoroutine_server_main(int argc, char **argv);

* 主函数

### extern void (*zcoroutine_server_service_register) (const char *service, int fd, int fd_type);

* 注册服务
* service 是服务名
* fd继承自 master
* fd_type: inet/unix/fifo

### extern void (*zcoroutine_server_before_service) (void) = 0;

* 进入主服务前执行的函数
* 此时协程环境已经准备好

### extern void (*zcoroutine_server_before_softstop) (void);

* 接到 master 重启信号后执行的函数
* 首先, 假设配置项 server-stop-on-softstop-after 的值为 M(秒)
* 如果此函数为 0, zcoroutine_server_main(epoll循环后) M 秒后返回(M 可以是 0)
* 如果此函数不为 0 , zcoroutine_server_main M 秒后返回, 如果 M < 1, 则 M 取 3600

### void zcoroutine_server_stop_notify(int stop_after_second);

* 手动通知主程序 stop_after_second 秒后循环停止, 然后 zcoroutine_server_main 返回

### void zcoroutine_server_detach_from_master();

* 主动和 master 管理器分离
* master 程序会以为此进程已经终止, 会启动一个新的进程
* master 发起重启信号时, 不会再次通知此进程
* 3600 秒后 zcoroutine_server_main 返回

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/master/

## 命令行(调试)启动 server 

[通用参数](./main_argument.md)配置风格

```
./some_server alone \
    [ -server-service imapd://0:143,0:1143,imapd-ssl://0:993 ] \
    [ -server-config-path ./etc/service/ ] \ # 加载全局配置 ./etc/service/*.gcf
    [ -config ./etc/service/imapd.cf ] \
    [ ... ]
```

