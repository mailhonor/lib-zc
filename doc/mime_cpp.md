<A name="readme_md" id="readme_md"></A>

[C版本](./mime.md)

## 邮件解析, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持解析 RFC822 格式邮件

本文所有函数返回的指针: 除非特别说明, 则此指针不可能为 NULL

## 类列表

```c++
// 邮件解析
zcc::mail_parser;
// mime 节点
zcc::mail_parser::mime_node;
// 邮件地址
zcc::mail_parser::mail_address;
// 邮件头行节后的节点
zcc::mail_parser::header_line_node;
```

## 解析一封信件

```c++
zcc::mail_parser *parser;
parser = zcc::mail_parser::create_from_file(eml_fn);
// parser = mail_parser *mail_parser::create_from_data(const char *mail_data, int64_t mail_data_len, const char *default_charset);
```

## 获取主题等, 获取附件信息等

* https://gitee.com/linuxmail/lib-zc/blob/master/include/zcc/zcc_mime.h

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/cpp_sample/mime/


## TNEF

* 参考 [TNEF](./tnef_cpp.md)
