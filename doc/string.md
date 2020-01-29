# 字符串常用函数封装

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了一些常用的字符/字符串处理函数

## 字符函数(宏)

| 宏 | 含义  |
|:--|:--|
| ztolower( c ) | 字符转小写 |
| ztoupper( c ) | 字符转大写 |
| zisalnum( c ) | 字符是否是字母或数字 |
| zisalpha( c ) | 字符是否是字母 |
| zislower( c ) | 字符是否是小写 |
| zisupper( c ) | 字符是否是大写 |
| zisdigit( c ) | 字符是否是数字 |
| zisxdigit( c ) | 字符是否是 16进制数字 既: 0-9a-hA-H
| zhexval( c ) | 16进制字符转10进制, 其他字符返回 -1 |
| zistrim( c ) | 字符是否是可以trim掉的 |

## 字符串函数

**字符串大写(小写)转换**

```
char *zstr_toupper(char *str);
char *zstr_tolower(char *str);
```

**trim函数**

```
char *ztrim_left(const char *str);
char *ztrim_right(char *str);
char *ztrim(char *str);
```

例子 char str[] = "\r \t\n \f ABC \r\n"

```
ztrim_left(str) => "ABC \r\n"
ztrim_right(str) => ""\r \t\n \f ABC"
ztrim(str) => "ABC"
```

**skip函数**

```
char *zskip_left(const char *str, const char *ignores);
int zskip_right(const char *str, int len, const char *ignores);
int zskip(const char *str, int len, const char *ignores_left, const char *ignores_right, char **start);
```

例子 char str[] = "\r \t\n \f ABC \r\n"

```
zskip_left(str, "\r\n \t\f") => "ABC \r\n"
zskip_left(str, "\r\n  \f") => "\t\n \f ABC \r\n"
zskip_right(str, -1, "\r") = >  strlen("\r \t\n \f ABC \r\n")
zskip_right(str, -1, "\rC\n ") = >  strlen("\r \t\n \f AB")
zskip(str, -1, "\t\r', "\r\n ", &ptr) => 4; ptr = " ABC"   
```

**单位转换函数**

**转bool值**

```
/* s 是 "0", "n", "N", "no", "NO", "false", "FALSE" 返回 0 */
/* s 是 "1", "y", "Y", "yes", "YES", "true", "TRUE" 返回 1 */
/* 否则 返回 def */
int zstr_to_bool(const char *s, int def);
```

转秒值(时间)

```
/* 转换字符串为秒, 支持 h(小时), m(分), s(秒), d(天), w(周) */
/* 如 "1026S" = > 1026, "8h" => 8 * 3600, "" = > def  */
long zstr_to_second(const char *s, long def);
```

转size值

```
/* 转换字符串为大小, 支持 g(G), m(兆), k(千), b */
/* 如 "9M" = > 9 * 1024 * 1024  */
long zstr_to_size(const char *s, long def);
```

例子

```
zstr_to_bool("Y",  0)   => 1
zstr_to_bool("Y",  1)   => 1
zstr_to_bool("No",  0)   => 0
zstr_to_bool("",  0)   => 0
zstr_to_bool("",  1)   => 1

zstr_to_second("100m",  1231) => 100*60
zstr_to_second("",  1231) => 1231

```
