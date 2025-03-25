
[C版本](./charset.md)

## 字符集转码, 字符集探测, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 对字符集转码做了封装, 支持字符集探测

## 字符集

```c++
namespace zcc {
namespace charset {
// 变量
const char *chinese[] = {"UTF-8", "GB18030", "BIG5", "UTF-7", 0};
const char *japanese[] = {"UTF-8", "EUC-JP", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0};
const char *korean[] = {"UTF-8", "EUC-KR", "UTF-7", 0};
const char *cjk[] = {"WINDOWS-1252", "UTF-8", "GB18030", "BIG5", "EUC-JP", "SHIFT-JIS", "ISO-2022-JP", "EUC-KR", "UTF-7", 0};
// 使用 icu
void use_uconv();
}
}
```

## 字符集转码

```c++
namespace zcc {
namespace charset {
// 转码
inline std::string convert(const char *from_charset, const char *data, int size, const char *to_charset, int *invalid_bytes = 0);
std::string iconv_convert(const char *from_charset, const char *src, int src_len, const char *to_charset, int *invalid_bytes = 0);
std::string uconv_convert(const char *from_charset, const char *src, int src_len, const char *to_charset, int *invalid_bytes = 0);
// 转码为 UTF-8
std::string convert_to_utf8(const char *from_charset, const char *data, int size);
inline std::string convert_to_utf8(const char *data, int size);
inline std::string convert_to_utf8(const char *from_charset, const std::string &data);
inline std::string convert_to_utf8(const std::string &from_charset, const std::string &data);
inline std::string convert_to_utf8(const std::string &data);
}
}
```

### 例子: 字符集转码

类似 Linux 的程序 iconv

* [goto](../cpp_sample/charset/iconv.cpp)


## 字符集探测

```c++
namespace zcc {
namespace charset {
std::string detect(detect_data *dd, const char **charset_list, const char *data, int size);
std::string detect_cjk(const char *data, int size);
inline std::string detect_cjk(const std::string &data);
}
}
```

### 字符集探测问答

**问**: 下面字符串是什么字符集

```
const char *s="\xb9\xe3\xb6\xab\xca\xa1";

```

**答**: 至少有 **4** 种字符集编码是合法的:

```
SHIFT-JIS     ｹ羝ｫﾊ｡
EUC-JP        鴻叫福
BIG5          嫘陲吽
GB18030       广东省
```

从经验看应该是 GB18030

通过本文提供的函数可以做到正确识别

### 例子: 字符集探测

* [goto](../cpp_sample/charset/detact.cpp)

## 其他工具

```c++
namespace zcc {
namespace charset {
// 不规范字符集修正, 返回字符集大写
// 如: "KS_C_5601-1989" => "EUC-KR"
const char *correct_name(const char *charset);
// utf8 截短
int utf8_tail_complete(const char *ps, int length);
char *utf8_tail_complete_and_terminate(char *ps, int length);
std::string &utf8_tail_complete_and_terminate(std::string &s);
// 从前往后取前宽度为need_width的字符. 一个英文字符算 1, 一个中文字符算 2
// 去除连续的空格, 去除不可显示字符, 去除换行符
std::string utf8_get_simple_digest(const char *s, int len, int need_width);
inline std::string utf8_get_simple_digest(const std::string &s, int need_width);
// UTF-8 一个 utf8 字符的字节数
static inline int utf8_len(const unsigned char *buf);
static inline int utf8_len(int ch);
}
}
```