## 邮件解析
mime/tnef格式邮件解析. 源码见 src/mime/

---

```
typedef struct zmail_t zmail_t;
typedef struct zmime_t zmime_t;

extern const char *zvar_mime_type_application_cotet_stream /* = "application/octet-stream" */;

/* 返回mime类型, 如 txt => text/plain */
const char *zget_mime_type_from_suffix(const char *suffix, const char *def);
const char *zget_mime_type_from_pathname(const char *pathname, const char *def);


/* 假设邮件头逻辑行最长长度不超过 zvar_mime_header_line_max_length */
#define zvar_mime_header_line_max_length   1024000

/* 邮件用, 字符集转码 */
/* 目标字符集为 UTF-8 */
/* from_charset: 原字符集, 为空则探测字符集, 探测失败则取值 GB18030 */
void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result);

/* 解码原始邮件头行 */
void zmime_raw_header_line_unescape(const char *in_line, int in_len, zbuf_t *result);

/* 获取邮件头行的第一个单词, trim两侧的 " \t<?\"'" */
void zmime_header_line_get_first_token(const char *in_line, int in_len, zbuf_t *result);

/* 邮件头行单词节点 */
typedef struct zmime_header_line_element_t zmime_header_line_element_t;
struct zmime_header_line_element_t {
    char *charset; /* 可能为空 */
    char *data;
    int dlen;
    char encode_type; /* 'B':base64, 'Q':qp, 0:unknown */
};

/* 分解邮件头行 */
const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len);
void zmime_header_line_element_vector_free(const zvector_t *element_vector);

/* 对邮件头行做分解并转码, 目标字符集UTF-8, ... 参考 zmime_iconv */
void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result);

/* 对符合RFC2232的邮件头行做分解并转码, 如上 */
void zmime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, int with_charset_flag);

/* 对邮件头行做处理, value 得到 第一个单词, params存储key<=>value */
void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *value, zdict_t *params);

/* 解码 Date 字段 */
long zmime_header_line_decode_date(const char *str);

/* 邮件地址 */
typedef struct zmime_address_t zmime_address_t;
struct zmime_address_t {
    char *name; /* 原始名称 */
    char *address; /* email 地址 */
    char *name_utf8; /* 原始名称转码为UTF-8 字符集 */
};

/* 分解邮件头行为多个邮件地址并返回, 这个时候不处理 name_utf8 */
zvector_t *zmime_header_line_get_address_vector(const char *in_str, int in_len);

/* 分解邮件头行为多个邮件地址并返回, 这个时候不处理 name_utf8 */
zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len);
void zmime_header_line_address_vector_free(zvector_t *address_vector);

/* 邮件解析mime节点函数列表, 全部只读 */

/* type */
const char *zmime_get_type(zmime_t *mime);

/* 编码 */
const char *zmime_get_encoding(zmime_t *mime);

/* 字符街 */
const char *zmime_get_charset(zmime_t *mime);

/* disposition */
const char *zmime_get_disposition(zmime_t *mime);

/* 可读名字, 把mime的name或filename转码为UTF-8 */
const char *zmime_get_show_name(zmime_t *mime);

/* name */
const char *zmime_get_name(zmime_t *mime);

/* name转UTF-8 */
const char *zmime_get_name_utf8(zmime_t *mime);

/* filename */
const char *zmime_get_filename(zmime_t *mime);

/* RFC2231类型的filename;  *with_charset_flag是否带字符集 */
const char *zmime_get_filename2231(zmime_t *mime, zbool_t *with_charset_flag);

/* filename 转 UTF-8 */
const char *zmime_get_filename_utf8(zmime_t *mime);

/* CONTENT-ID */
const char *zmime_get_content_id(zmime_t *mime);

/* boundary */
const char *zmime_get_boundary(zmime_t *mime);

/* imap协议代码, 形如 1.2 或 2.1.6 */
const char *zmime_get_imap_section(zmime_t *mime);

/* mime头部 */
const char *zmime_get_header_data(zmime_t *mime);

/* mime邮件体 */
const char *zmime_get_body_data(zmime_t *mime);

/* mime头部相对于邮件起始偏移 */
int zmime_get_header_offset(zmime_t *mime);

/* mime头部长度 */
int zmime_get_header_len(zmime_t *mime);

/* mime邮件体相对于邮件起始偏移 */
int zmime_get_body_offset(zmime_t *mime);

/* mime 邮件体长度 */
int zmime_get_body_len(zmime_t *mime);

/* 下一个节点 */
zmime_t *zmime_next(zmime_t *mime);

/* 第一个子节点 */
zmime_t *zmime_child(zmime_t *mime);

/* 父节点 */
zmime_t *zmime_parent(zmime_t *mime);

/* 获取mime头 */
const zvector_t *zmime_get_raw_header_line_vector(zmime_t *mime); /* zsize_data_t* */

/* 获取第sn个名称为header_name的原始逻辑行, 返回 -1: 不存在 */
/* sn -1: 倒数第一个, 0:第一个, 1: 第二个, ... 下同 */
int zmime_get_raw_header_line(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

/* 获取第sn个名称为header_name的行的值, 返回 -1: 不存在 */
int zmime_get_header_line_value(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

/* 获取解码(base64, qp)后的mime邮件体 */
void zmime_get_decoded_content(zmime_t *mime, zbuf_t *result);

/* 获取解码(base64, qp)后并转UTF-8的mime邮件体 */
void zmime_get_decoded_content_utf8(zmime_t *mime, zbuf_t *result);

/* 是不是 tnef(application/ms-tnef) 类型的附件 */
zbool_t zmime_is_tnef(zmime_t *mime);

/* 下面是邮件解析函数 */

/* 创建邮件解析器, default_charset: 默认字符集 */
zmail_t *zmail_create_parser_from_data(const char *mail_data, int mail_data_len, const char *default_charset);
zmail_t *zmail_create_parser_from_pathname(const char *pathname, const char *default_charset);
void zmail_free(zmail_t *parser);

/* debug输出解析结果 */

void zmail_debug_show(zmail_t *parser);

/* 邮件源码data */
const char *zmail_get_data(zmail_t *parser);

/* 邮件源码长度 */
int zmail_get_len(zmail_t *parser);

/* 邮件头data */
const char *zmail_get_header_data(zmail_t *parser);

/* 邮件头偏移 */
int zmail_get_header_offset(zmail_t *parser);

/* 邮件头长度 */
int zmail_get_header_len(zmail_t *parser);

/*  邮件体 */
const char *zmail_get_body_data(zmail_t *parser);

/* 邮件体偏移 */
int zmail_get_body_offset(zmail_t *parser);

/* 邮件体偏移 */
int zmail_get_body_len(zmail_t *parser);

/* Message-ID */
const char *zmail_get_message_id(zmail_t *parser);

/* Subject */
const char *zmail_get_subject(zmail_t *parser);

/* Subject 转 UTF-8 */
const char *zmail_get_subject_utf8(zmail_t *parser);

/* 日期 */
const char *zmail_get_date(zmail_t *parser);

/* 日期, unix时间 */
long zmail_get_date_unix(zmail_t *parser);

/* In-Reply-To */
const char *zmail_get_in_reply_to(zmail_t *parser);

/* From */
const zmime_address_t *zmail_get_from(zmail_t *parser);

/* From并且处理name_utf8 */
const zmime_address_t *zmail_get_from_utf8(zmail_t *parser);

/* Sender */
const zmime_address_t *zmail_get_sender(zmail_t *parser);

/* Reply-To */
const zmime_address_t *zmail_get_reply_to(zmail_t *parser);

/* Disposition-Notification-To */
const zmime_address_t *zmail_get_receipt(zmail_t *parser);

/* To */
const zvector_t *zmail_get_to(zmail_t *parser); /* zmime_address_t* */

/* To并且处理name_utf8 */
const zvector_t *zmail_get_to_utf8(zmail_t *parser);
const zvector_t *zmail_get_cc(zmail_t *parser);
const zvector_t *zmail_get_cc_utf8(zmail_t *parser);
const zvector_t *zmail_get_bcc(zmail_t *parser);
const zvector_t *zmail_get_bcc_utf8(zmail_t *parser);

/* References */
const zargv_t *zmail_get_references(zmail_t *parser);

/* 顶层mime */
const zmime_t *zmail_get_top_mime(zmail_t *parser);

/* 全部mime */
const zvector_t *zmail_get_all_mimes(zmail_t *parser); /* zmime_t* */

/* 全部text/html,text/plain类型的mime */
const zvector_t *zmail_get_text_mimes(zmail_t *parser);

/* 应该在客户端显示的mime, alternative情况首选html */
const zvector_t *zmail_get_show_mimes(zmail_t *parser);

/* 所有附件类型的mime, 包括内嵌图片 */
const zvector_t *zmail_get_attachment_mimes(zmail_t *parser);

/* 获取所有邮件头 */
const zvector_t *zmail_get_raw_header_line_vector(zmail_t *parser); /* zsize_data_t* */


/* 获取第sn个名称为header_name的原始逻辑行, 返回 -1: 不存在 */
/* sn -1: 倒数第一个, 0:第一个, 1: 第二个, ... 下同 */
int zmail_get_raw_header_line(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

/* 获取第sn个名称为header_name的行的值, 返回 -1: 不存在 */
int zmail_get_header_line_value(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

```

---


```

/* 解析 tnef(application/ms-tnef) */
typedef struct ztnef_t ztnef_t;
typedef struct ztnef_mime_t ztnef_mime_t;

/* type */
const char *ztnef_mime_get_type(ztnef_mime_t *mime);

/* filename_utf8 */
const char *ztnef_mime_get_show_name(ztnef_mime_t *mime);

/* filename */
const char *ztnef_mime_get_filename(ztnef_mime_t *mime);

/* filename_utf8 */
const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime);

/* Content-ID */
const char *ztnef_mime_get_content_id(ztnef_mime_t *mime);

/* mime_body 便宜 */
int ztnef_mime_get_body_offset(ztnef_mime_t *mime);

/* mime_body 长度 */
int ztnef_mime_get_body_len(ztnef_mime_t *mime);

/* 如果是text类型的mime, 则返回其文本字符集 */
const char *ztnef_mime_get_charset(ztnef_mime_t *mime);


/* 创建tnef解析器 */
ztnef_t * ztnef_create_parser_from_data(const char *tnef_data, int tnef_data_len, const char *default_charset);
ztnef_t *ztnef_create_parser_from_pathname(const char *pathname, const char *default_charset);
void ztnef_free(ztnef_t *parser);

/* 原始数据 */
const char *ztnef_get_data(ztnef_t *parser);

/* 原始数据长度 */
int ztnef_get_len(ztnef_t *parser);

/* 全部 mime */
const zvector_t *ztnef_get_all_mimes(ztnef_t *parser);

/* 全部附件 mime */
const zvector_t *ztnef_get_attachment_mimes(ztnef_t *parser);

/* 全部文本(应该作为正文显示) mime, 包括 plain, html, 和 rtf */
const zvector_t *ztnef_get_text_mimes(ztnef_t *parser);


/* debug输出 */
void ztnef_debug_show(ztnef_t *parser);

#endif /*___ZC_LIB_INCLUDE___ */
```

### 例子
见源码 sample/mime/
