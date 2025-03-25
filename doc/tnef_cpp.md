
## winmail.dat(tnef,Transport Neutral Encapsulation Format)文件解析, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持解析 winmail.dat(tnef,Transport Neutral Encapsulation Format)

[C版本](./tnef.md)

电子邮件中有一种附件

* 名为 winmail.dat
* Content-Type 为 application/ms-tnef
* 数据格式为tnef, Transport Neutral Encapsulation Format

_PS: 在Linux系统, 可以通过命令 **tnef** 解开 winmail.dat_

下面介绍通过 [LIB-ZC](./README.md) 的 API 解析 winmail.dat, 并介绍这些 API

## 类见:

[goto](../include/zcc/zcc_mime.h)


## 解析

```c++
void foo()
{
    zcc::tnef_parser *parser = zcc::tnef_parser::create_from_file("./1.tnef", 0);
    if (parser == 0)
    {
        std::printf("ERROR open %s\n", eml_fn);
        zcc::exit(1);
    }
    parser->debug_show();
}
```

## 例子

* [goto](../cpp_sample/mime/)

