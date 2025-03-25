
[C++版本](./encode_cpp.md)

## 编解码: base64/quoted-printable/hex/ncr, [LIB-ZC](./README.md)

命名空间: zcc

## BASE64

```c++
void base64_encode(const void *src, int src_size, std::string &str, bool mime_flag = false);
inline void base64_encode(const std::string &src, std::string &str, bool mime_flag = false);
std::string base64_encode(const void *src, int src_size, bool mime_flag = false);
inline std::string base64_encode(const std::string &src, bool mime_flag = false);
//
void base64_decode(const void *src, int src_size, std::string &str);
inline void base64_decode(const std::string &src, std::string &str);
std::string base64_decode(const void *src, int src_size);
inline std::string base64_decode(const std::string &src)
int base64_decode_get_valid_len(const void *src, int src_size);
int base64_encode_get_min_len(int in_len, bool mime_flag = false);
```

## QP

```c++
void qp_encode_2045(const void *src, int src_size, std::string &result, bool mime_flag = false);
inline void qp_encode_2045(const std::string &src, std::string &result, bool mime_flag = false);
std::string qp_encode_2045(const void *src, int src_size, bool mime_flag = false);
inline std::string qp_encode_2045(const std::string &src, bool mime_flag = false);
void qp_decode_2045(const void *src, int src_size, std::string &str);
inline void qp_decode_2045(const std::string &src, std::string &str);
std::string qp_decode_2045(const void *src, int src_size);
inline std::string qp_decode_2045(const std::string &src);
//
void qp_encode_2047(const void *src, int src_size, std::string &result);
inline void qp_encode_2047(const std::string &src, std::string &result);
std::string qp_encode_2047(const void *src, int src_size);
inline std::string qp_encode_2047(const std::string &src);
void qp_decode_2047(const void *src, int src_size, std::string &str);
inline void qp_decode_2047(const std::string &src, std::string &str);
std::string qp_decode_2047(const void *src, int src_size);
inline std::string qp_decode_2047(const std::string &src);
int qp_decode_get_valid_len(const void *src, int src_size);
```

## hex

```c++
// hex
void hex_encode(const void *src, int src_size, std::string &str);
inline void hex_encode(const std::string &src, std::string &str);
std::string hex_encode(const void *src, int src_size);
inline std::string hex_encode(const std::string &src);
void hex_decode(const void *src, int src_size, std::string &str);
inline void hex_decode(const std::string &src, std::string &str);
std::string hex_decode(const void *src, int src_size);
inline std::string hex_decode(const std::string &src);
```

## xml

```c++
void xml_unescape_string(const char *data, int len, std::string &content);
inline void xml_unescape_string(std::string &content, const char *data, int len);
std::string xml_unescape_string(const char *data, int len);
```

## ncr
```c++
int ncr_decode(int ins, char *wchar);
```

## 例子

* ../blob/master/cpp_sample/encode/

