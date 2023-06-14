<A name="readme_md" id="readme_md"></A>

## 日志输出, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持通用日志机制

_请注意: 本日志系统没有日志级别的概念_

## 日志输出函数

### void zlog_info(const char *source_fn, size_t line_number, const char *fmt, ...);

* 一般日志输出
* source_fn: 程序文件名 
* line_number: 所在行
* (const char *fmt, ...) 和 printf(fmt, ...) 一样 

### void zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...);

* 日志输出(如zlog_info)后,进程退出
* 可以这么理解(逻辑上) zlog_fatal

```
void zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...)
{
	zlog_info(souce_fn, line_number, fmt, ...);
	_exit(1);
}
```

## 日志输出宏

实际上, 代码里经常使用的是宏, 而不是函数

### # define zinfo(fmt, args...)

```
/* 一般日志输出 */
# define zinfo(fmt, args...) { zlog_info(__FILE__, __LINE__, fmt, ##args); }

/* 例子: */
zinfo("connection from %s:%d", ip, port);
zinof("somebody logout");
```

### #define zfatal(fmt, args...)

```
/* 日志输出(同zlog_info)后,进程退出 */
#define zfatal(fmt, args...) { zlog_fatal(__FILE__, __LINE__, fmt, ##args); }

/* 例子: */
zfatal("too much error, refuse");
zfatal("parameters error");
```

### #define zdebug(fmt,args...)

```
/* DEBUG输出 */
### #define zdebug(fmt,args...) { if(zvar_log_debug_enable){zinfo(fmt, ##args);} }

/* LIB-ZC 内部没有使用, 留给开发人员 */
```


## 全局变量

### int zvar_log_fatal_catch = 0;

* 如果 zvar_log_fatal_catch == 1, 则当执行 zfatal的时候, 进程产生 段错误

### int zvar_log_debug_enable = 0;

* 如果 zvar_log_debug_enable == 1, 则 zdebug生效(相当于 zinfo)

## 日志输出后端函数(引擎)

### extern void (*zlog_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap);

* 例如

```
static void zvprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap) 
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, " [%s:%zu]\n", source_fn, line_number);
}
zlog_vprintf= zvprintf_default;
```

### 用 syslog 做后端输出

* 用户可以自己实现(见本文的例子)
* 也可以用本系统封装好的函数

```
/* identity/facility  参考 syslog  */
void zlog_use_syslog(const char *identity, int facility);

/* 转换字符串facility, 如: "LOG_MAIL" => LOG_MAIL */
int zlog_get_facility_from_str(const char *facility);
```

### 用 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)内嵌的 masterlog

_PS:看不明白就跳过本节_

master/server 框架下

```
/* identity: 程序名 */
/* dest: master-server提供的服务地址, domain_socket, udp协议 */
void zlog_use_masterlog(const char *identity, const char *dest);
```

## 例子: 常用

```
zinfo("start");
zinfo("progname: %s",  "do_something");
zdebug("哦了");
zfatal("can not do something!"); /* then exist */
```

## 例子: 自定义后端函数

```
static void zvprintf_syslog(const char *source_fn, size_t line_number, const char *fmt, va_list ap) 
{
    char buf[10240 + 10];
    vsnprintf(buf, 10240, fmt, ap);
    syslog(LOG_INFO, "%s [%s:%ld]", buf, source_fn, (long)line_number);
}
zlog_vprintf = zvprintf_syslog;
openlog(identity, LOG_NDELAY | LOG_PID, facility);
```

