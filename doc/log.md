# 日志

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

LIB-ZC内部使用的通用日志机制, 推荐使用

请注意: 本日志系统没有日志级别的概念

## 日志输出函数

**一般日志输出**

```
/*  source_fn: 程序文件名 */
/*  line_number: 行数 */
/*  (const char *fmt, ...)  和 printf(fmt, ...) 一样 */
void zlog_info(const char *source_fn, size_t line_number, const char *fmt, ...); 
```

**日志输出后,进程退出**

```
void zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...); 
```

可以这么理解 zlog_fatal

```
void zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...); 
{
	zlog_info(souce_fn, line_number, fmt, ...);
	_exit(1);
}
```

## 日志输出宏

**实际经常使用的宏**

```
#define zfatal(fmt, args...) { zlog_fatal(__FILE__, __LINE__, fmt, ##args); }
#define zinfo(fmt, args...) { zlog_info(__FILE__, __LINE__, fmt, ##args); }
```

**DEBUG输出**

LIB-ZC内部没有使用, 留给开发人员

```
#define zdebug(fmt,args...) { if(zvar_log_debug_enable){zinfo(fmt, ##args);} }
```

## 全局变量

**int zvar_log_fatal_catch = 0;**

如果 zvar_log_fatal_catch == 1, 则当执行 zfatal的时候, 进程产生 段错误

**int zvar_log_debug_enable = 0;**

如果 zvar_log_debug_enable == 1, 则 zdebug生效(相当于 zinfo)

## 自定义日志输出后端函数

```
extern void (*zlog_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap);
```

例如

```
static void zvprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap) 
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, " [%s:%zu]\n", source_fn, line_number);
}
zlog_vprintf= zvprintf_default;
```

##  用syslog做后端

```
/* identity/facility  参考 syslog  */
void zlog_use_syslog(const char *identity, int facility);

/* 转换字符串facility, 如: "LOG_MAIL" => LOG_MAIL */
int zlog_get_facility_from_str(const char *facility);
```

PS: 当然, 用户可以自己实现

##  用 LIB-ZC内嵌的masterlog

PS:看不明白就跳过

master/server 框架下

```
/*  identity: 程序名 */
/*  dest: master-server提供的服务地址, domain_socket, udp协议 */
void zlog_use_masterlog(const char *identity, const char *dest);
```

## 例子:常用

```
zinfo("start");
zinfo("progname: %s",  "do_something");
if(need_exit) {
	zfatal("FATAL: can not do something!");
}
zdebug("哦了");
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
