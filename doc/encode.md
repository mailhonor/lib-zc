# 编解码/base64/quoted-printable/hex/url/ncr

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了常见的编解码函数

## BASE64

```
/* 编码 */
/* 输入 src, 长度 src_size, 输出到 str */
/* 如果 mime_flag == 1, 则每输出76个字节则追加一组 "\r\n" */
void zbase64_encode(const void *src, int src_size, zbuf_t *str, int mime_flag);

/* 解码 */
/* 输入 src, 长度 src_size, 输出到 str */
/* 如果 delead_size != 0, 则成功解码的长度保存在 *dealed_size */
void zbase64_decode(const void *src, int src_size, zbuf_t *str, int *dealed_size);

/*  估计解码后需要的空间大概长度,并返回 */
int zbase64_decode_get_valid_len(const void *src, int src_size);

/* 估计编码后需要空间的最小长度, 并返回 */
int zbase64_encode_get_min_len(int in_len, int mime_flag);
```

## quoted-printable

2045 和 2047 有什么区别呢? 细节就不说了, 在应用上举个例子: 邮件解析.
- 正文如果是 quoted-printable 编码, 则 使用 2045
- 邮件头(主题, 收件人,...)如果是quoted-printable编码, 则使用 2047

```
/* 解码  rfc 2045 */
void zqp_decode_2045(const void *src, int src_size, zbuf_t *str);

/* 解码  rfc 2047 */
void zqp_decode_2047(const void *src, int src_size, zbuf_t *str);

/* 估计解码后需要空间的最小长度,并返回 */
int zqp_decode_get_valid_len(const void *src, int src_size);

```

## 16进制编码

```
/* 解码 */
/* 输入 src, 长度 src_size, 输出到 str */
void zhex_decode(const void *src, int src_size, zbuf_t *dest);

/* 编码 */
/* 输入 src, 长度 src_size, 输出到 str */
void zhex_encode(const void *src, int src_size, zbuf_t *dest);
```

## URL字段

```
/* 解码 */
/* 输入 src, 长度 src_size, 输出到 str */
void zurl_hex_decode(const void *src, int src_size, zbuf_t *str);

/* 编码 */
/* 输入 src, 长度 src_size, 输出到 str */
/* 如果 strict_flag == 1, 则采用严格编码,  (不明白的话, 就取值 为 1) */
/* 如果 strict_flag == 0, 有些字符不编码,如 8BIT字节 */ 
void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag);
```

## NCR
```
/* 返回写入 wchar 的长度 */
int zncr_decode(int ins, char *wchar);
```

## URL协议解析

```
/* 数据结构 */
struct zurl_t {
    char *scheme;
    char *destination;
    char *host;
    char *path;
    char *query;
    char *fragment;
    int port;
};

/* 解析URL字符串, 并返回 */
zurl_t *zurl_parse(const char *url_string);
void zurl_free(zurl_t *url);

/* debug输出 */
void zurl_debug_show(zurl_t *url);

/* 解析URL的query部分, 保存在query_vars, 并返回; 如果 query_vars==0, 则新建 zdict_t */
zdict_t *zurl_query_parse(const char *query_string,  zdict_t *query_vars);

/* 从字典query_vars, 组成query字符串并保存在query_result */
/* strict_flag: 是否严格编码 */
char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_result, zbool_t strict);
```

举例

已知 字符串

```
const char *url_string= "https://www.baidu.com/s?wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1";
```

执行

```
zurl_t *url = zurl_parse(url_string);
```

得到

```
url->scheme = https
url->host = www.baidu.com
url->port = -1
url->destination = www.baidu.com
url->path = s
url->query = wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1
url->fragment = mark
```

上面所谓的"query" 就是下面的字符串:

```
const char * query_string = "wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1";
```

解开上面的字符键

```
zdict_t *dict = zurl_query_parse(query_string,  0);
```

则, dict的内容就是

```
rsv_spt = 1
rsv_sug = 1
rsv_sug2 = 0
wd = Linux
```

反过来, 对dict 执行, query编码

```
zbuf_t *qurery_result = zbuf_create(0);
char *r = zurl_query_build(dict, query_result,  1);
```

得到

```
"rsv_spt=1&rsv_sug=1&rsv_sug2=2&wd=Linux"
```
