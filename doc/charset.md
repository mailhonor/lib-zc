<A name="readme_md" id="readme_md"></A>

## 字符集转码, 字符集探测, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 对字符集转码做了封装, 支持字符集探测

## 字符集

### extern const char *zvar_charset_chinese[];

* = { "UTF-8", "GB18030", "BIG5", "UTF-7", 0 } 

### extern const char *zvar_charset_japanese[];

* = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0 }

### extern const char *zvar_charset_korean[];

* = { "UTF-8", "KS_C_5601", "KS_C_5861", "UTF-7", 0 }

### extern const char *zvar_charset_cjk[];

* = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", "UTF-7", 0 }

### char *zcharset_correct_charset(const char *charset);

* 修正字符集名称, 如 "GBK' =&gt; "GB18030", "KS_C_5861" =&gt; "EUC-KR"

## 字符集转码


### int zcharset_iconv(const char *from_charset, const char *src, int src_len, <BR />&nbsp;&nbsp;const char *to_charset, zbuf_t *result, int *src_converted_len, <BR />&nbsp;&nbsp;int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);

* 字符集转码, 有点复杂, 建议再次封装
* 返回目标字符串的长度
* 返回 -1: 错
* from_charset: 源字符集
* src, src_len: 源字符串和长度
* to_charset: 目标字符集
* result: 目标字符串(zbuf_t, 首先 zbuf_reset(result))
* *src_converted_len: 成功转码的字节数
* omit_invalid_bytes_limit: 设置可忽略的错误字节个数(&lt;0 表示无限大)
* *omit_invalid_bytes_count: 实际忽略字节数

### 简单用法

```
zbuf_t *result = zbuf_create(-1);
zcharset_iconv("GB18030", "abcdef", 6, "UTF-8" , result, 0, 0, 0);
```

## 例子: 字符集转码

类似 Linux 的程序 iconv

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/charset/iconv.c


## 字符集探测

### char *zcharset_detect(const char **charset_list, const char *data, int size, char *charset_result);

* 探测字符串 data 是什么字符集, 结果存储(覆盖)到 charset_result, 并返回 zbuf_t *
* charset_list 是字符集列表, NULL 结尾

### char *zcharset_detect_cjk(const char *data, int size, zbuf_t *charset_result);

* 如上, 探测范围 "中日韩"

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

## 例子: 字符集探测

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/charset/detact.c

