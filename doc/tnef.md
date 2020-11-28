<A name="readme_md" id="readme_md"></A>

## winmail.dat(tnef,Transport Neutral Encapsulation Format)文件解析, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持解析 winmail.dat(tnef,Transport Neutral Encapsulation Format)

电子邮件中有一种附件

* 名为 winmail.dat
* Content-Type 为 application/ms-tnef
* 数据格式为tnef, Transport Neutral Encapsulation Format

_PS: 在Linux系统, 可以通过命令 **tnef** 解开 winmail.dat_


下面介绍通过 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的 API 解析 winmail.dat, 并介绍这些 API

## 第一步, 创建解析器

### ztnef_t *ztnef_create_parser_from_pathname(const char *pathname, const char *default_charset);

* 从邮件文件创建解析器
* pathname: 是文件路径名
* default_charset: 默认的字符集, 一般取值为空 ""

### ztnef_t * ztnef_create_parser_from_data(const char *tnef_data, int tnef_data_len, const char *default_charset);

* 从数据创建解析器
* tnef_data 是数据
* tnef_len 是长度

## 第二步, 获取节点(mime)

### const zvector_t *vec = ztnef_get_all_mimes(parser);

* 全部节点

### const zvector_t *vec = ztnef_get_text_mimes(parser);

* 文本类型的节点(应该当作正文显示的节点)

### const zvector_t *vec = ztnef_get_attachment_mimes(parser);

* 附件类型的节点

## 第三步, 获取节点属性

* 下列函数不返回 NULL

### const char *ztnef_mime_get_type(ztnef_mime_t *mime);

* 节点的 Content-Type

### const char *ztnef_mime_get_filename(ztnef_mime_t *mime);

* 如果节点类型是附件, 返回节点的 filename, 否则返回 ""

### const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime);

* 如果节点类型是附件, 返回返回节点的 utf-8 字符集的 filename, 否则返回 ""

### const char *ztnef_mime_get_content_id(ztnef_mime_t *mime);

* 如果节点类型是附件, 返回返回节点的 Content-ID, 否则返回 ""

### const char *ztnef_mime_get_body_data(ztnef_mime_t *mime);

* 节点的数据

### int ztnef_mime_get_body_len(ztnef_mime_t *mime);

* 节点的数据的长度

### const char *ztnef_mime_get_charset(ztnef_mime_t *mime);

* 如果节点是 TEXT 类型, 则返回节点的字符集 

### void ztnef_debug_show(ztnef_t *parser);

* 打印调试信息, 调试用

## 第四步,释放解析器

### void ztnef_free(ztnef_t *parser);

* 释放解析器

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/mime/tnef_parser.c

