
## 几个时间函数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了几个时间函数

[C++版本](./time_cpp.md)

## 时间, 毫秒

### #define zvar_max_timeout_millisecond (3600L * 24 * 365 * 10 * 1000)

* 宏, 最大毫秒

### long zmillisecond(void);

* 返回毫秒精度的时间 

### void zsleep_millisecond(int delay);

* 睡眠 delay 毫秒
* 忽略 EINTR

### long ztimeout_set_millisecond(long timeout);

* 设置超时时间, 返回 (zmillisecond(void) + timeout)

### long ztimeout_left_millisecond(long stamp);

* 距离超时时间还差多少毫秒? 返回 (stamp - zmillisecond())


### #define zvar_max_timeout (3600 * 24 * 365 * 10)

* 宏, 最大秒

## 时间, 秒

### long zsecond(void);

* 返回当前秒精度的时间
* 就是 time(0)

### void zsleep(int delay);

* 睡眠 delay 秒; 忽略 EINTR

### long ztimeout_set(int timeout);

* 设置超时时间, 返回 (zsecond() + timeout)

### int ztimeout_left(long stamp);

* 距离超时时间还差多少秒? 返回 (stamp - zsecond())

## RFC 1123(http 头用的时间格式)

### #define zvar_rfc1123_date_string_size 32

* 宏

### char *zbuild_rfc1123_date_string(long t, char *buf);

* 根据 t(unix秒) 生成 rfc1123 格式的时间字符串, 存储在 buf, 并返回
* buf 的长度不小于 zvar_rfc1123_date_string_size
* 不在 buf 后面追加 '\n'

## 邮件头 Date

### #define zvar_rfc822_date_string_size 38

* 宏

### char *zbuild_rfc822_date_string(long t, char *buf);

* 根据 t(unix秒) 生成rfc822/rfc2822 (格式的时间字符串, 存储在 buf, 并返回
* buf 的长度不小于 zvar_rfc822_date_string_size
* 不在 buf 后面追加 '\n'


### long zmime_header_line_decode_date(const char *str);

* 解析邮件头 Date 字段, 返回 unix 秒

