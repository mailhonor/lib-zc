<A name="readme_md" id="readme_md"></A>

## httpd 服务器, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持 httpd服务器, STRUCT 类型是 **zhttpd_t**

zhttpd_t 作为 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的一个模块, 和专门的 httpd 库不同:

* 只负责自己该做的事: 根据一个 fd(文件描述符) 或 SSL创建 zhttpd_t, http 协议处理完毕后, 释放 zhttpd_t
* 端口的打开, SSL的创建, 进程还是线程还是协程模式, 服务的管理, 配置, 等等由其他模块负责
* 这个库适合开发简单的 http 业务
* 可以和第三方库无缝结合, 如其他的线程库, socket库, SSL 库等等

建议开发者浏览 **httpd 服务开发通用模版**:

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/http/general_coroutine_server_httpd.c

## 数据结构

```
struct zhttpd_uploaded_file_t { /* 上传的文件; 隐藏细节, 不必深究 };
struct zhttpd_t { /* httpd 服务; 隐藏细节, 不必深究 */};
/* 源码 https://gitee.com/linuxmail/lib-zc/blob/master/src/http/httpd.h */
```

## 最简单的用法

```
/* fd 一般是 accpet 得到的文件描述符; */
void foo(int fd)
{
    zhttpd_t *hd = zhttpd_open_fd(fd);
    zhttpd_run(hd);
    zhttpd_close(hd, 1);
}
```

## 函数: 基本操作

### zhttpd_t *zhttpd_open_fd(int fd);

* 通过文件描述符 fd 创建 zhttpd_t
* fd 一般是 socket; fd 的状态由开发者决定, 比如是否阻塞等

### zhttpd_t *zhttpd_open_ssl(SSL *ssl);

* 通过 (SSL *)ssl 创建 zhttpd_t
* ssl 的属性由开发者决定, 比如如何支持 [SNI](./openssl.md) 等

### void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl);

* 释放 zhttpd_t
* 如果 close_fd_and_release_ssl == 0, 则 忽略 fd/ssl, 否则 关闭fd/释放ssl及关闭其fd

### void zhttpd_run(zhttpd_t *httpd);

* 处理 http 协议

## 函数: 属性和控制

### void zhttpd_set_context(zhttpd_t *httpd, const void *context);<BR />void *zhttpd_get_context(zhttpd_t *httpd);

* 设置/获取上下文

### void zhttpd_set_stop(zhttpd_t *httpd);

* 停止 httpd, zhttpd_run 返回

### void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout);

* 设置 keep-alive 时间, 既 http 协议第一行可读等待时间

### void zhttpd_set_read_wait_timeout(zhttpd_t *httpd, int read_wait_timeout);

* 设置等待可读超时

### void zhttpd_set_write_wait_timeout(zhttpd_t *httpd, int write_wait_timeout);

* 设置等待可写超时

### void zhttpd_set_max_length_for_post(zhttpd_t *httpd, int max_length);

* 设置 post 方法下, 数据最大长度

### void zhttpd_set_tmp_path_for_post(zhttpd_t *httpd, const char *tmp_path);

* 设置 post 方法下, 上传文件的临时目录, 默认 /tmp/

### void zhttpd_enable_form_data(zhttpd_t *httpd);

* 设置支持 **multipart/form-data** 协议
* 默认不支持

### extern zbool_t zvar_httpd_no_cache = 0;

* 如果 zvar_httpd_no_cache == 1, 则强制(此库提供的功能)不缓存

## 函数: 设置业务函数

### void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

* 设置函数 handler 处理 GET/POST 协议
* handler 默认值: zhttpd_response_404

### void zhttpd_set_HEAD_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_OPTIONS_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_DELETE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_TRACE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_PATCH_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

* 设置函数 handler 处理 HEAD/OPTIONS/DELETE/TRACE/PATH 协议
* handler 默认值: zhttpd_response_501

## 函数: 获取/处理请求数据

### const char *zhttpd_request_get_method(zhttpd_t *httpd);

* 方法: GET/POST/...

### const char *zhttpd_request_get_host(zhttpd_t *httpd);

* 主机名

### const char *zhttpd_request_get_path(zhttpd_t *httpd);

* url 路径

### const char *zhttpd_request_get_uri(zhttpd_t *httpd);

* URI

### const char *zhttpd_request_get_version(zhttpd_t *httpd);

* http 版本(字符串)

### int zhttpd_request_get_version_code(zhttpd_t *httpd);

* http 版本(数字)
* 返回 0: "1.0"
* 返回 1: "1.1"

### long zhttpd_request_get_content_length(zhttpd_t *httpd);

* http 头 Content-Lengtgh 字段

### zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd);

* 客户端是否支持 gzip

### zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd);

* 客户端是否支持 deflate

### const zdict_t *zhttpd_request_get_headers(zhttpd_t *httpd);

* 全部 http 头; key 全部小写, 如 Content-Length =&gt; content-length

### const zdict_t *zhttpd_request_get_query_vars(zhttpd_t *httpd);

* URI 全部参数

### const zdict_t *zhttpd_request_get_post_vars(zhttpd_t *httpd);

* Content-Type 是 application/x-www-form-urlencoded 或 multipart/form-data 的情况下, 解析的参数(不包括文件)

### const zdict_t *zhttpd_request_get_cookies(zhttpd_t *httpd);

* 全部(解析后的) cookie
* 原始 cookie 可以通过 zdict_get_str(zhttpd_request_get_headers(httpd), "cookie", 0) 获取

### const zvector_t *zhttpd_request_get_uploaded_files(zhttpd_t *httpd);

* 获取全部上传文件的向量
* zvector_t 的成员类型为 zhttpd_uploaded_file_t *, 下文详述

## 函数: 快速输出完整 http 协议

### void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size);

* 输出 http 码为 200 的 数据
* 这个数据是正文, 不包括 协议头;
* Content-Type 为 text/html

### void zhttpd_response_304(zhttpd_t *httpd, const char *etag);

* 输出 http 码为 304 的协议头
* etag 既 http 头 Etag 的值
* Etag 或 If-None-Match 等知识请另找参考文档

### void zhttpd_response_404(zhttpd_t *httpd);<BR />void zhttpd_response_500(zhttpd_t *httpd);<BR />void zhttpd_response_501(zhttpd_t *httpd);

* 输出 http 码为 404/500/501 的 协议

### void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size));<BR />void zhttpd_set_304_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *etag));<BR />void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));<BR />void zhttpd_set_501_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

* 设置 zhttpd_response_200/304/404/500/501 的处理函数
* 一般情况默认就可以

## 函数: 输出(http 头)

### void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *version, const char *status);

* 输出 http banner
* version: "http/1.0" 或 "http/1.1", 0
* status: 如 "200 XXX"

### void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value);

* 输出 http 头
* 如 zhttpd_response_header(httpd, "Content-type", "text/plain");

### void zhttpd_response_header_date(zhttpd_t *httpd, const char *name, long value);

* 输出 http 头 Date
* value 为 unix 时间

### void zhttpd_response_header_content_type(zhttpd_t *httpd, const char *value, const char *charset);

* 输出 http 头 Content-Type
* value: content-type, 如 text/html;
* charset: 是字符集, 为空则取 UTF-8

### void zhttpd_response_header_content_length(zhttpd_t *httpd, long length);

* 输出 http 头 Content-Length
* length: 长度, 既 http 正文的长度;

### void zhttpd_response_header_set_cookie(zhttpd_t *httpd, const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly);

* 设置 cookie
* name: 名称
* value: 值
* expires: 过期(unix)时间
* path: 路径
* domain: 域名

### void zhttpd_response_header_unset_cookie(zhttpd_t *httpd, const char *name);

* 删除 cookie
* name: 需要删除的 cookie 的名称

### void zhttpd_response_header_over(zhttpd_t *httpd);

* http 头输出完毕

## 函数: 输出(通用)

### void zhttpd_response_write(zhttpd_t *httpd, const void *data, int len);

* 输出数据长度为 len 的 data

### void zhttpd_response_puts(zhttpd_t *httpd, const char *data);

* 输出行

### void zhttpd_response_append(zhttpd_t *httpd, const zbuf_t *bf);

* 输出 bf

### void zhttpd_response_printf_1024(zhttpd_t *httpd, const char *format, ...);

* 格式化输出, 长度不能超过 1024
* 风格参考 snprintf(somebuf, 1024, format, ....)

### void zhttpd_response_flush(zhttpd_t *httpd);

* 刷新输出数据

### zstream_t *zhttpd_get_stream(zhttpd_t *httpd);

* 获取 httpd 的 zstream_t

## 函数: 处理上传的文件

### const zvector_t *zhttpd_request_get_uploaded_files(zhttpd_t *httpd);

* 获取全部上传文件的向量
* zvector_t 的成员类型为 zhttpd_uploaded_file_t *

### const char *zhttpd_uploaded_file_get_pathname(zhttpd_uploaded_file_t *fo);

* 上传文件的文件名(filename), 一般指(原始的)文件名

### const char *zhttpd_uploaded_file_get_name(zhttpd_uploaded_file_t *fo);

* 上传文件名称(name), 一般指网页里 form 表单里文件控件的 name 
* 特别提醒注意, 通过 form 表单上传的文件可能出现相同的 name

### int zhttpd_uploaded_file_get_size(zhttpd_uploaded_file_t *fo);

* 上传文件大小
* 返回-1: 错

### int zhttpd_uploaded_file_save_to(zhttpd_uploaded_file_t *fo, const char *pathname);

* 保存上传文件到指定文件路径

### int zhttpd_uploaded_file_get_data(zhttpd_uploaded_file_t *fo, zbuf_t *data);

* 获取上传文件数据

## 函数: 快速输出一个文件

### void zhttpd_response_file(zhttpd_t *httpd, const char *pathname, const char *content_type, int max_age);

* 输出一个文件
* pathname: 文件路径
* content_type: Content-Type, 如果为空则根据 pathname 的后缀判定
* max_age: 过期
    * -1: 10天
    * 0: no-cache
    * &gt;0: 生存期(秒)
    * &lt;-1: 忽略

### void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *gzip_pathname, const char *content_type, int max_age);

* 如上
* 输出一个文件
* Content-Encoding 设置为 gzip

### void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *pathname, const char *gzip_pathname, const char *content_type, int max_age);

* 如上
* 如果 gzip_pathname 存在, 功能如 zhttpd_response_file_with_gzip, 否则功能如 zhttpd_response_file

## 函数: 日志

### extern const char *(*zhttpd_get_prefix_log_msg)(zhttpd_t *httpd);

* 内部使用的日志输出函数变量
* 如果使用者想修改 httpd 库内部的日志输出, 则需要重新赋值此函数

### #define zhttpd_show_log(httpd, fmt, args...) { zinfo("%s "fmt, zhttpd_get_prefix_log_msg(httpd), ##args) }

* 宏, 内部实际使用的日志输出


## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/http/

