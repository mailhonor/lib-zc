
[C++版本](./encode_cpp.md)

## 编解码: base64/quoted-printable/hex/ncr, [LIB-ZC](./README.md)

## BASE64

### void zbase64_encode(const void *src, int src_size, zbuf_t *dest, int mime_flag);

* 编码
* 输入 src, 长度 src_size, 输出追加到 dest
* 如果 mime_flag == 1, 则每输出76个字节会追加一组 "\r\n"

### void zbase64_decode(const void *src, int src_size, zbuf_t *dest);

* 解码
* 输入 src, 长度 src_size, 输出追加到 dest

### int zbase64_decode_get_valid_len(const void *src, int src_size);

* 估计解码后需要的空间长度, 并返回

### int zbase64_encode_get_min_len(int in_len, int mime_flag);

* 估计编码后需要的空间长度, 并返回

## quoted-printable

本节中 2045 和 2047 有什么区别呢? 细节就不说了; 在应用上举个邮件解析的例子:

* 正文如果是 quoted-printable 编码, 则使用 2045
* 邮件头(主题, 收件人,...)如果是 quoted-printable 编码, 则使用 2047

### void zqp_encode_2045(const void *src, int src_size, zbuf_t *dest, int mime_flag);

* 编码
* 输入 src, 长度 src_size, 输出追加到 dest
* 如果 mime_flag == 1, 则每输出76个字节会追加一组 "\r\n"

### void zqp_encode_2047(const void *src, int src_size, zbuf_t *dest);

* 编码
* 输入 src, 长度 src_size, 输出追加到 dest

### void zqp_decode_2045(const void *src, int src_size, zbuf_t *dest);

* 解码, rfc 2045
* 输入 src, 长度 src_size, 输出追加到 dest

### void zqp_decode_2047(const void *src, int src_size, zbuf_t *dest);

* 解码, rfc 2047
* 输入 src, 长度 src_size, 输出追加到 dest

### int zqp_decode_get_valid_len(const void *src, int src_size);

* 估计解码后需要的空间长度, 并返回

## 16进制

### void zhex_decode(const void *src, int src_size, zbuf_t *dest);

* 解码
* 输入 src, 长度 src_size, 输出追加到 dest

### void zhex_encode(const void *src, int src_size, zbuf_t *dest);

* 编码
* 输入 src, 长度 src_size, 输出追加到 dest


## NCR

### int zncr_decode(int ins, char *wchar);

* 解开 ins, 写入到wchar
* 返回写入的长度

## 例子

* [goto](../blob/master/sample/encode/base64.c)
* [goto](../blob/master/sample/encode/quoted_printable.c)

