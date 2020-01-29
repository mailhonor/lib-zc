# winmail.dat(tnef,Transport Neutral Encapsulation Format)文件解析

电子邮件中有一种附件

* 名为 winmail.dat,
* Content-Type 为 application/ms-tnef
* 数据格式为tnef, Transport Neutral Encapsulation Format

在Linux系统, 可以通过命令 **tnef** 解开 winmail.dat

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) ,是一个C扩展库, 支持解析winmail.dat

下面介绍应用LIB-ZC的API解析winmail.dat

## 解析 winmail.dat

**第一步, 创建解析器**

```
/* 从文件创建解析器, 可能返回 NULL */
ztnef_t *parser = ztnef_create_parser_from_pathname(eml_fn, "");
```

或

```
/* 从数据创建解析器*/
ztnef_t *parser = ztnef_create_parser_from_data(tnef_data, tnef_len,  "");
```

**第二步, 获取节点(mime)**

```
/* 获取全部节点 */
const zvector_t *vec = ztnef_get_all_mimes(parser);

/* 获取文本节点(应该当作正文显示的节点) */
const zvector_t *vec = ztnef_get_text_mimes(parser);

/* 获取附件节点 */
const zvector_t *vec = ztnef_get_attachment_mimes(parser);
```

遍历 vec

```
/* zvector_t 是LIB-ZC中vector的实现 */
/* 遍历 */
ZVECTOR_WALK_BEGIN(vec, ztnef_mime_t *, m) {
	foo(m);
} ZVECTOR_WALK_END;

```

**第三步,获取节点属性**

```
/* 下列函数不返回 NULL */

/* 获取节点 Content-Type */
const char *ztnef_mime_get_type(ztnef_mime_t *mime);

/* 如果节点是附件, 返回 filename*/
const char *ztnef_mime_get_filename(ztnef_mime_t *mime);

/* 如果节点是附件, 返回 utf-8字符集的 filename */
const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime);

/* 如果节点是附件, 返回 Content-ID */
const char *ztnef_mime_get_content_id(ztnef_mime_t *mime);

/* 返回节点的数据 */
const char *ztnef_mime_get_body_data(ztnef_mime_t *mime);

/* 返回节点的数据的长度 */
int ztnef_mime_get_body_len(ztnef_mime_t *mime);

/* 如果节点是TEXT类型, 则返回其文本字符集 */
const char *ztnef_mime_get_charset(ztnef_mime_t *mime);
```

**第四步,输出调试信息**

```
ztnef_debug_show(parser);
```

**第五步,释放解析器**

```
ztnef_free(parser);
```

## 例子

https://gitee.com/linuxmail/lib-zc/blob/master/sample/mime/tnef_parser.c

## 源码

https://gitee.com/linuxmail/lib-zc/blob/master/src/mime/tnef.c
