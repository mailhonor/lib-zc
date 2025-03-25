
## 编解码: url, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 URL 解析, 其 STRUCT 类型是 **zurl_t**

[C版本](./url.md)

## 解析 url

```c++
void parse_url(const std::string &url_string)
{
    zcc::http_url url(url_string);
    url.debug_show();
}
```

## 生成 url
```c++
std::string build_url()
{
    zcc::http_url url;
    url.query_ = "";
    return url.build_url();
}
```

## 例子
* [goto](../blob/master/cpp_sample/http/)
