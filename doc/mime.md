
[C++版本](./mime_cpp.md)

## 邮件解析, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持解析 RFC822 格式邮件

本文所有函数返回的指针: 除非特别说明, 则此指针不可能为 NULL

本文所有函数返回的数据: 只读

## 数据结构

```
/* 邮件数据结构; 隐藏细节, 不必深究 */
struct zmail_t {
};

/* 邮件节点数据结构; 隐藏细节, 不必深究 */
struct zmime_t {
};

/* 邮件头行单词节点 */
struct zmime_header_line_element_t {
    char *charset;       /* 可能为空, 小写 */
    char *data;          /* const char * */
    int dlen;
    char encode_type;    /* 'B':base64, 'Q':qp, 0:unknown */
};

/* 邮件地址 */
struct zmime_address_t {
    char *name;          /* 原始名称 */
    char *address;       /* email 地址, 小写 */
    char *name_utf8;     /* 原始名称解码并转为 UTF-8 字符集 */
};
```

## 函数: 基本操作

### zmail_t *zmail_create_parser_from_pathname(const char *pathname, const char *default_charset);

* 从邮件文件创建解析器
* 返回 NULL: 文件不可读
* pathname: 是文件路径名
* default_charset: 默认的字符集, 一般取值为空 ""

### zmail_t *zmail_create_parser_from_data(const char *mail_data, int mail_data_len, const char *default_charset);

* 从数据创建解析器
* mail_data 是数据
* mail_data_len 是长度

### void zmail_free(zmail_t *parser);

* 释放解析器

### void zmail_debug_show(zmail_t *parser);

* 打印解析结果, 调试用

## 函数: 邮件头属性

### const char *zmail_get_data(zmail_t *parser);

* 邮件源码

### int zmail_get_len(zmail_t *parser);

* 邮件源码长度

### const char *zmail_get_header_data(zmail_t *parser);

* 邮件头

### int zmail_get_header_offset(zmail_t *parser);

* 邮件头偏移(相对于信头, 下同)

### int zmail_get_header_len(zmail_t *parser);

* 邮件头长度

### const char *zmail_get_body_data(zmail_t *parser);

* 邮件体

### int zmail_get_body_offset(zmail_t *parser);

* 邮件体偏移

### int zmail_get_body_len(zmail_t *parser);

* 邮件体长度

### const char *zmail_get_message_id(zmail_t *parser);

* Message-ID 头

### const char *zmail_get_subject(zmail_t *parser);

* Subject 头
* 形如:  =?GB2312?B?z8LO58zhx7C3xbzZ?=

### const char *zmail_get_subject_utf8(zmail_t *parser);

* 解码且字符集转为 UTF-8 的 Subject
* 如: "=?GB2312?B?z8LO58zhx7C3xbzZ?=" =&gt; "下午提前放假"

### const char *zmail_get_date(zmail_t *parser);

* Date 头

### long zmail_get_date_unix(zmail_t *parser);

* Date 头转为 unix 时间戳

### const char *zmail_get_in_reply_to(zmail_t *parser);

* In-Reply-To 头

### const zmime_address_t *zmail_get_from(zmail_t *parser);

* From
* 解析为 zmime_address_t

### const zmime_address_t *zmail_get_from_utf8(zmail_t *parser);

* From
* 解析为 zmime_address_t
* 并且处理 name_utf8

### const zmime_address_t *zmail_get_sender(zmail_t *parser);

* Sender
* 解析为 zmime_address_t

### const zmime_address_t *zmail_get_reply_to(zmail_t *parser);

* Reply-To
* 解析为 zmime_address_t

### const zmime_address_t *zmail_get_receipt(zmail_t *parser);

* Disposition-Notification-To
* 解析为 zmime_address_t

### const zvector_t *zmail_get_to(zmail_t *parser);<BR />const zvector_t *zmail_get_cc(zmail_t *parser);<BR />const zvector_t *zmail_get_bcc(zmail_t *parser);

* To/Cc/Bcc
* 解析为 zvector_t, 其成员为 (zmime_address_t *)

### const zvector_t *zmail_get_to_utf8(zmail_t *parser);<BR />const zvector_t *zmail_get_cc_utf8(zmail_t *parser);<BR />const zvector_t *zmail_get_bcc_utf8(zmail_t *parser);

* To/Cc/Bcc
* 解析为 zvector_t, 其成员为 zmime_address_t
* 并处理 name_utf8

### const zargv_t *zmail_get_references(zmail_t *parser);

* References
* 解析为 zargv_t

### const zmime_t *zmail_get_top_mime(zmail_t *parser);

* 顶层 mime 节点

### const zvector_t *zmail_get_all_mimes(zmail_t *parser); /* zmime_t* */

* 全部 mime 节点

### const zvector_t *zmail_get_text_mimes(zmail_t *parser);

* 全部 text/html 和 text/plain 类型的 mime节点

### const zvector_t *zmail_get_show_mimes(zmail_t *parser);

* 应该在客户端显示的 mime 节点
* alternative 情况首选 text/html

### const zvector_t *zmail_get_attachment_mimes(zmail_t *parser);

* 所有附件类型的 mime 节点
* 包括内嵌图片

### const zvector_t *zmail_get_raw_header_line_vector(zmail_t *parser); /* zsize_data_t* */

* 全部(原始)邮件头

### int zmail_get_raw_header_line(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

* 获取第 sn 个名称为 header_name 的原始逻辑行
* 返回 -1: 不存在
* sn = -1: 倒数第 1 个
* sn = 0: 第 1 个
* sn = 1: 第 2 个, ... , 下同

### int zmail_get_header_line_value(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

* 获取第 sn 个名称为 header_name 的行的值


## 函数: mime 节点属性

### const char *zmime_get_type(zmime_t *mime);

* Content-Type, 小写

### const char *zmime_get_encoding(zmime_t *mime);

* 编码, 小写

### const char *zmime_get_charset(zmime_t *mime);

* 字符集, 小写

### const char *zmime_get_disposition(zmime_t *mime);

* disposition, 小写

### const char *zmime_get_show_name(zmime_t *mime);

* 可读名字
* 把 mime 的 name 或 filename 解码并转为 UTF-8 

### const char *zmime_get_name(zmime_t *mime);

* name
* 原始数据

### const char *zmime_get_name_utf8(zmime_t *mime);

* name
* 解码并转为 UTF-8

### const char *zmime_get_filename(zmime_t *mime);

* filename
* 原始数据

### const char *zmime_get_filename2231(zmime_t *mime, zbool_t *with_charset_flag);

* 返回原始的 RFC2231 类型的 filename
* (*with_charset_flag) 表示是否带字符集

### const char *zmime_get_filename_utf8(zmime_t *mime);

* filename 或 filename2231
* 解码并转为 UTF-8

### const char *zmime_get_content_id(zmime_t *mime);

* Content-Id

### const char *zmime_get_boundary(zmime_t *mime);

* boundary

### const char *zmime_get_imap_section(zmime_t *mime);

* 按照 imap 协议, 返回本节点的 编号
* 形如: 1.2 或 2.1.6

### const char *zmime_get_header_data(zmime_t *mime);

* mime 头部

### const char *zmime_get_body_data(zmime_t *mime);

* mime 邮件体

### int zmime_get_header_offset(zmime_t *mime);

* mime 头部相对于邮件起始偏移

### int zmime_get_header_len(zmime_t *mime);

* mime 头部长度

### int zmime_get_body_offset(zmime_t *mime);

* mime 邮件体相对于邮件起始偏移

### int zmime_get_body_len(zmime_t *mime);

* mime 体长度 

### zmime_t *zmime_next(zmime_t *mime);

* 下一个节点

### zmime_t *zmime_child(zmime_t *mime);

* 第一个子节点

### zmime_t *zmime_parent(zmime_t *mime);

* 父节点

### const zvector_t *zmime_get_raw_header_line_vector(zmime_t *mime); /* zsize_data_t* */

* 全部(原始) mime 头

### int zmime_get_raw_header_line(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

* 获取第 sn 个名称为 header_name 的原始逻辑行

### int zmime_get_header_line_value(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

*  获取第 sn 个名称为 header_name 的行的值
* 返回 -1: 不存在

### void zmime_get_decoded_content(zmime_t *mime, zbuf_t *result);

* 获取解码(base64, qp)后的 mime 邮件体

### void zmime_get_decoded_content_utf8(zmime_t *mime, zbuf_t *result);

* 获取解码(base64, qp)后并转 UTF-8 的 mime 邮件体

### zbool_t zmime_is_tnef(zmime_t *mime);

* 是不是 tnef(application/ms-tnef) 类型的附件

## 邮件解析工具

### void zmime_raw_header_line_unescape(const char *in_line, int in_len, zbuf_t *result);

解码原始邮件头行, 例子:

已知: 原始逻辑头

```
X-Received: from xxx (xxx [192.168.1.1])
 by linuxmail.cn
```

解析后, 得到

```
X-Received: from xxx (xxx [192.168.1.1]) by linuxmail.cn
```

### void zmime_header_line_get_first_token(const char *in_line, int in_len, zbuf_t *result);

* 获取邮件头行的第一个单词, trim 两侧的 **\t &lt;&gt;"'?**

### const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len);

* 分解邮件头行

### void zmime_header_line_element_vector_free(const zvector_t *element_vector);

* 释放上面的 (zvector_t *)element_vector

### void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result);

* 邮件头行做分解并转码, 目标字符集UTF-8
* src_charset_def: 默认字符集

### void zmime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, int with_charset_flag);

* 对符合 RFC2231 的邮件头行做分解并转码, 如上

### void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *value, zdict_t *params);

* 对邮件头行做处理
* 第一个单词存储(覆盖)到 value
* key=value型参数存储到 params

### long zmime_header_line_decode_date(const char *str);

* 解码 Date 字段

### zvector_t *zmime_header_line_get_address_vector(const char *src_charset_def, const char *in_str, int in_len);

* 分解邮件头行为多个邮件地址并返回
* 这个时候不处理 name_utf8

### zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len);

* 分解邮件头行为多个邮件地址并返回
* 此时做 UTF-8 转码

### void zmime_header_line_address_vector_free(zvector_t *address_vector);

* 释放上面的 (zvector_t *)address_vector

## BENCHMARK

* 简单的结论: 每秒解析大小为 **G** 级别的邮件
* 程序 mime_benchmark 见例子

```
[xxx@zytest mime]$ ./mime_benchmark ./1.eml -loop 10000 --onlymime
eml     : ./61cb81dee3633aa7c8d1d937918a07c3.eml
size    : 334244(bytes)
loop    : 10000
total   : 3,342,440,000(bytes)
elapse  : 0.975(second)
%second : 3,428,143,589(bytes)
```

```
[xxx@zytest mime]$ ./mime_benchmark ./2.eml -loop 10000 --onlymime
eml     : ./08822e157fbd0e30dded7548f4ea5f3c.eml
size    : 172513(bytes)
loop    : 10000
total   : 1,725,130,000(bytes)
elapse  : 0.764(second)
%second : 2,258,023,560(bytes)
```

## TNEF

* 参考 [TNEF](./tnef.md)

## 例子

* [goto](../sample/mime/)

