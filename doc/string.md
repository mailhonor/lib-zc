<A name="readme_md" id="readme_md"></A>

## 常用字符串函数, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)


[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了常用字符串函数

## 字符操作,类别判定

都是宏, 实验性质

<TABLE class="tbview" width="100%">
<TR><TD>ztolower(int ch)</TD><TD>字符转小写</TD></TR>
<TR><TD>ztoupper(int ch)</TD><TD>字符转大写</TD></TR>
<TR><TD>zisalnum(int ch)</TD><TD>字符是否是字母或数字</TD></TR>
<TR><TD>zisalpha(int ch)</TD><TD>字符是否是字母</TD></TR>
<TR><TD>zislower(int ch)</TD><TD>字符是否是小写</TD></TR>
<TR><TD>zisupper(int ch)</TD><TD>字符是否是大写</TD></TR>
<TR><TD>zisdigit(int ch)</TD><TD>字符是否是数字</TD></TR>
<TR><TD>zisxdigit(int ch)</TD><TD>字符是否是 16 进制数字 既: 0-9a-hA-H</TD></TR>
<TR><TD>zhexval(int ch)</TD><TD>16 进制字符转 10 进制, 其他字符返回 -1</TD></TR>
<TR><TD>zistrim(int ch)</TD><TD>字符是否是可以 trim 掉的</TD></TR>
</TABLE>

## 字符串函数

### char *zstr_toupper(char *str);<BR />char *zstr_tolower(char *str);

* 字符串大写(小写)转换

### char *ztrim_left(const char *str);<BR />char *ztrim_right(char *str);<BR />char *ztrim(char *str);

* trim
* 举例:

```
char str[] = "\r \t\n \f ABC \r\n"

ztrim_left(str)  返回 "ABC \r\n"
ztrim_right(str) 返回 "\r \t\n \f ABC"
ztrim(str)       返回 "ABC"
```

### char *zskip_left(const char *str, const char *ignores);<BR />int zskip_right(const char *str, int len, const char *ignores);<BR />int zskip(const char *str, int len, const char *ignores_left, const char *ignores_right, char **start);

* skip
* 举例:

```
const char str[] = "abcdefg123rst"; /* 长度: 13 */

zskip_left(str, "abd")                返回 str+2
zskip_left(str, "bdcaeg")             返回 str+5
zskip_right(str, -1, "ts2fd")         返回 11
zskip_right(str, -1, "r3as")          返回 13
zskip(str, -1, "abd', "ts2fd", &ptr)  返回 9; ptr = str+2
```

## 单位转换函数

### int zstr_to_bool(const char *s, int def);

* 转bool值
* 详细:
    * s 是 "0", "n", "N", "no", "NO", "false", "FALSE" 返回 0
    * s 是 "1", "y", "Y", "yes", "YES", "true", "TRUE" 返回 1
    * 否则 返回 def 

### long zstr_to_second(const char *s, long def);</h3>

* 转秒值(时间)
* 支持 h/H(小时), m/M(分), s/S(秒), d/D(天), w/W(周)
* 如:
    * "1026S" =&gt; 1026
    * "8h"=&gt; (8*3600)
    * "3W" =&gt; (3*24*3600)
    * "" =&gt; def

### long zstr_to_size(const char *s, long def);

* 转size值
* 支持 g/G(G), m/M(兆), k/K(千), b/B(字节)
* 如:
    * "9M" =&gt; (9*1024*1024)
    * "100k" =&gt; (100*1024)
    * "" =&gt; def

## strtok

zstrtok_t 功能类似 strtok, 不过 zstrtok_t 不复制内存

### 数据结构

```
struct zstrtok_t {
    char *sstr;
    char *str;
    int len;
};
```

### void zstrtok_init(zstrtok_t *k, const char *sstr)

* 初始化
* sstr 是需要操作的字符串

### zstrtok_t *zstrtok(zstrtok_t *k, const char *delim)

* delim 是分隔符(和strtok类似);
* 返回 0: 表示没找到
* 在找到的情况下:
    * k->str 是地址
    * k->len 是长度

### 例子

```
const char *ps = " abc \tdef\tlinux windows unix\tos "
zstrtok_t sk;
zstrtok_init(&sk, ps);
while(zstrtok(&sk, " \t")) {
    /* 得到长度为 sk.len 的(char *)指针 sk.str, */
    fwrite(sk.str, 1, sk.len, stdout);
    printf("\n");
}
```

