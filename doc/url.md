
## 编解码: url, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 URL 解析, 其 STRUCT 类型是 **zurl_t**

[C++版本](./url_cpp.md)

## 数据结构

```
struct zurl_t {
    char *scheme;
    char *destination;
    char *host;
    char *path;
    char *query;
    char *fragment;
    int port;
};
```

## 函数: URL

### zurl_t *zurl_parse(const char *url_string);

* 解析URL字符串, 并返回

### void zurl_free(zurl_t *url);

* 释放

### void zurl_debug_show(zurl_t *url);

* 打印 zurl_t, 调试用

## 函数: query

### zdict_t *zurl_query_parse(const char *query_string,  zdict_t *query_vars);

* 解析 URL 的 query 部分, 保存在 query_vars, 并返回
* 如果 query_vars==0, 则新建一个 zdict_t

### char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_result, zbool_t strict);

* 从字典 query_vars, 生成 query 字符串并保存(追加)到 query_result
* 如果 strict_flag==1, 则严格编码

## 函数: 字段

### void zurl_hex_decode(const void *src, int src_size, zbuf_t *dest);

* 字段解码
* 输入 src, 长度 src_size
* 输出追加到 dest

### void zurl_hex_encode(const void *src, int src_size, zbuf_t *dest, int strict_flag);

* 字段编码
* 输入 src, 长度 src_size
* 输出追加到 dest
* 如果 strict_flag == 1, 则采用严格编码, (不清楚需求的话, 就取值 为 1)
* 如果 strict_flag == 0, 有些字符不编码, 如 8BIT字节 

## 详细讲解

### 解析 URL

对下面的字符串 url_string, 执行 zurl_parse (解析)

```
const char *url_string= "https://www.baidu.com/s?wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1";
zurl_t *url = zurl_parse(url_string);
```

则, 得到:

```
url->scheme = https
url->host = www.baidu.com
url->port = -1
url->destination = www.baidu.com
url->path = s
url->query = wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1
url->fragment = mark
```

### 解析 query 

对上面的字符串 url-&gt;query, 执行 zurl_query_parse (解析)

```
/* url->query = wd=Linux&rsv_spt=1&rsv_sug2=0&rsv_sug=1 */
zdict_t *dict = zurl_query_parse(url->query,  0);
```

则, 得到 dict:

```
rsv_spt = 1
rsv_sug = 1
rsv_sug2 = 0
wd = Linux
```

### 编码 query 

反过来, 对上面的 dict 执行 zurl_query_build (编码)

```
zbuf_t *qurery_result = zbuf_create(0);
char *r = zurl_query_build(dict, query_result,  1);
```

则, 得到 r:

```
rsv_spt=1&rsv_sug=1&rsv_sug2=2&wd=Linux
```

## 例子
* ../blob/master/sample/http/url.c

