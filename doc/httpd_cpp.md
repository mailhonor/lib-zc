
[C版本](./httpd.md)

## httpd 服务器, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 httpd服务器, STRUCT 类型是 **zhttpd_t**

zcc::httpd 作为 [LIB-ZC](./README.md) 的一个模块, 和专门的 httpd 库不同:

* 只负责自己该做的事: 根据 zcc::stream 创建 zcc::httpd, http 协议处理完毕后, 释放 zcc::httpd
* 端口的打开, SSL的创建, 进程还是线程还是协程模式, 服务的管理, 配置, 等等由其他模块负责
* 这个库适合开发简单的 http 业务
* 可以和第三方库无缝结合, 如其他的线程库, socket库, SSL 库等等

## 最简单的用法

```c++
static void do_httpd_serve_once(zcc::httpd &httpd)
{
    if (!httpd.request_read_all())
    {
        return;
    }
    // 获取请求的 path
    const std::string &path = httpd.request_get_url().path_
    // 根据 path
    if (1) {
        httpd.response_200("123456");
    }
    if (0) {
        httpd.response_404();
    }
    if (0) {
        httpd.response_304();
    }
    if (0) {
        // httpd.response_header....
        // httpd.response_write....
    }
}

// 假设已经拿到 fp
void do_httpd_serve(zcc::stream &fp)
{
    zcc::httpd *httpd = new zcc::httpd(fp);
    httpd->set_enable_form_data();
    while (1)
    {
        do_httpd_serve_once(*httpd);
        if (!httpd->maybe_continue())
        {
            break;
        }
    }
    // 自动 delete fp
    delete httpd;
}
```

## cookie 操作

```c++
namespace zcc{
// 解析cookie
dict http_cookie_parse(const char *raw_cookie);
inline dict http_cookie_parse(const std::string &raw_cookie);

// 生成cookie
std::string http_cookie_build_item(const char *name, const char *value = nullptr, int64_t expires = 0, const char *path = nullptr, const char *domain = nullptr, bool secure = false, bool httponly = false);
inline std::string http_cookie_build_item(const std::string &name, const std::string &value, int64_t expires, std::string &path, std::string &domain, bool secure, bool httponly);
}
```


## 例子

* [goto](../cpp_sample/http/)

