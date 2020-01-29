# 几个时间函数

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了几个时间函数

## 函数

### 毫秒

```
#define zvar_max_timeout_millisecond (3600L * 24 * 365 * 10 * 1000)

/* 返回当前毫秒精度的时间 */
long zmillisecond(void);

/* 睡眠delay毫秒 */
void zsleep_millisecond(int delay);

/* 设置超时时间, 返回 zmillisecond(void) + timeout */
long ztimeout_set_millisecond(long timeout);

/* 距离超时时间还差多少毫秒? 返回 stamp - zmillisecond(void) */
long ztimeout_left_millisecond(long stamp);
```

### 秒

```
#define zvar_max_timeout (3600 * 24 * 365 * 10)

/* 返回当前秒精度的时间 */
long zsecond(void);

/* 睡眠delay秒 */
void zsleep(int delay);

/* 设置超时时间, 返回 zsecond(void) + timeout */
long ztimeout_set(int timeout);

/* 距离超时时间还差多少秒? 返回 stamp - zsecond(void) */
int ztimeout_left(long stamp);
```

### RFC1122(http 头用的时间格式) */

```
#define zvar_rfc1123_date_string_size 32

/* 根据t(unix秒)生成rfc1123格式的时间字符串,存储在buf, 并返回, */
/* buf的长度不小于 zvar_rfc1123_date_string_size */
char *zbuild_rfc1123_date_string(long t, char *buf);
```

### 邮件头 Date

```
#define zvar_rfc822_date_string_size 38
/* 根据t(unix秒)生成rfc822(l格式的时间字符,存储在buf并 返回 */
/* fuf的长度不小于 zvar_rfc822_date_string_size */
char *zbuild_rfc822_date_string(long t, char *buf);


/* 解析邮件头Date字段, 返回 unix秒 */
long zmime_header_line_decode_date(const char *str);

```
