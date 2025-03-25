
[C版本](./master.md)

## MASTER/SERVER 服务进程管理框架, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 MASTER/SERVER 服务进程管理框架

此框架下:

* 有两类程序, 管理器程序(master)和服务程序(server)
* master 和 server 是父子进程关系
* SERVER 包括异步 IO 模式和协程模式

命名空间 zcc

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

最简单的调用方法:

```c++
zcc::master_server ms;
ms.main_run(argc, argv);
```

### 参数 -C config_dir

* zcc::master_server 加载每一个配置文件 config_dir/*.cf 为一个服务

### 参数 -pid-file pid_file

* pid_file: 文件, 存储当前 master 进程的进程id
* master 进程会 flock 锁定文件 pid_file

### 参数 --try-lock

* master 进程尝试锁定文件 pid_file, 并立即返回
* 返回 0: 锁定成功
* 返回 1: 锁定失败

### 启动/停止/重启

* master 进程的启动/停止/重启, 请参考脚本, 这个脚本稍微修改即可通用
* [goto](../cpp_sample/server/general_master.sh)

### server 服务配置

根据参数 **-C config_dir**, master 程序会遍历(无序) config_dir/*.cf, 把每个配置文件(如:config/dir/imapd.cf)作为一个服务启动

例如 

```shell
[xxx@zytest]$ cat some_config_dir/imapd.cf

# 服务程序
server-command = bin/imapd

# 启动的端口和服务名字(自定义)
# imapd 和 imapd-ssl 是服务名字
# 0:143, 127.0.0.1:1143, 0;993, 0:12143 是监听的网络端口, ./var/socket/imapd 是 domain socket 端口
server-service = imapd://0:143, imapd-ssl://0:993, imapd://127.0.0.1:1143,  imapd://./var/socket/imapd, 0:12143

# 启动几个进程
server-proc-count = 1

# 下面是其他配置, server 会读取
abc = xxx
host = linuxmail.cn
```

### zcc::master_server

```c++
class master_server
{
public:
    // 获取实例
    static master_server *get_instance();
    static aio_base *get_aio_base();
    // 手动获取全部配置
    static std::list<config> load_server_configs_from_dir(const char *dirname);
    static config load_global_config_from_dir(const char *dirname);

public:
    master_server();
    ~master_server();
    void main_run(int argc, char **argv);
    // 不依赖参数 -C config_dir, 自己维护具体服务的配置
    virtual std::list<config> load_server_configs();
    virtual void before_service();

protected:
    void *unused_;
};
```

## server 服务进程: 异步 IO 模式(aio)

此服务模式运行在异步IO环境, 默认已经建好环境 zcc::var_main_aio_base

### 基本模型
```c++
class my_aio;

static void after_write(zcc::aio *aio)
{
    // ...
}

static void simple_service(int fd, std::string service_name)
{
    zcc::nonblocking(fd);
    my_aio *aio = new my_aio(fd);
    aio->service_name_ = service_name;
    aio->cache_printf_1024("echo server(%s): %zd\n", service_name.c_str(), zcc::second());
    aio->cache_flush(std::bind(after_write, aio));
}

class my_aio_worker_server : public zcc::aio_worker_server
{
public:
    void service_register(const char *service, int fd, int fd_type) {
        std::string service_name = service;
        // service_name 就是上面的 imapd, imapd-ssl
        // fd 是 从 master 继承的监听端口, 应该通过 accept 处理
        // fd_type 可选 var_tcp_listen_type_inet, var_tcp_listen_type_unix
        // 下面这个函数是一个通用的, 简单的处理机制
        zcc::aio_worker_server::simple_service_register(zcc::var_main_aio_base, fd, fd_type, std::bind(simple_service, std::placeholders::_1, service_name));
    }
    // service 服务前,执行的方法
    void before_service(void) {}
    // 进程退出前执行的方法
    void before_softstop(void) {}
    // 通知master本进程 stop_after_second 秒后退出 
    void stop_notify(int stop_after_second = 0) {}
    // 主动从master管理中剥离, master 不在管理此进程
    void detach_from_master();
    void main_run(int argc, char **argv);

};

class my_aio : public zcc::aio
{
public:
    inline my_aio(int fd) : aio(fd, zcc::var_main_aio_base) {}
    std::string service_name_;
};

int main(int argc, char **argv)
{
    my_aio_worker_server ms;
    ms.main_run(argc, argv);
    return 0;
}
```

### 

## server 服务进程: 协程模式(coroutine) 

此服务模式运行在协程环境

### 基本模型

```c++
static void *do_accept(void *arg)
{
    int listen_fd = (int)(int64_t)arg;
    while (1)
    {
        // accept
        // ...
    }
}

class my_server : public zcc::coroutine_worker_server
{
public:
    void service_register(const char *service_name, int fd, int fd_type)
    {
        zcoroutine_go(do_accept, (void *)(int64_t)(fd), -1);
    }
    // 同 aio 模式
    // void before_service();
    // void before_softstop();
    // void stop_notify(int stop_after_second = 0);
    // void detach_from_master();
};

int main(int argc, char **argv)
{
    ms.main_run(argc, argv);
    return 0;
}

## 配置

见上文的 some_config_dir/imapd.cf

程序启动后, 会(无序)加载 some_config_dir/*.gcf, 然后再加载 some_config_dir/imapd.cf


```
## 例子

* [goto](../cpp_sample/server/)

## 命令行(调试)启动 server 

[通用参数](./main_argument_cpp.md)配置风格

```shell
./some_server alone \
    [ -server-service imapd://0:143,0:1143,imapd-ssl://0:993 ] \
    [ -server-config-path ./etc/service/ ] \ # 加载全局配置 ./etc/service/*.gcf
    [ -config ./etc/service/imapd.cf ] \
    [ ... ]
```

