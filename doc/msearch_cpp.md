
[C版本](./msearch.md)

## 多关键字匹配, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 内嵌多关键字搜索模块

文件格式和实现, 参考:  ../blob/master/cpp_src/search/msearch.cpp

本实现, 使用了 mmap 机制, 多进程共享数据

## 用途举例

* 十万百万量级的关键字匹配
* 大量 URL 匹配

## 类

../blob/master/include/zcc/zcc_search.h

```c++
// 生成
zcc::msearch_builder;
// 搜索
zcc::msearch_reader;
// 遍历
zcc::msearch_walker;
```

## 生成

```c++
zcc::msearch_builder builder;
// builder.add_token("abc", 3);
builder.add_token("12345", 5);
builder.add_token_from_file("/etc/postfix/main.cf");
builder.add_token("abc", 3);
// builder.add_token("12345", 5);
builder.add_over();
zcc::file_put_contents("./a.cdb", builder.get_compiled_data(), builder.get_compiled_size();
```

## 搜索

搜索器, 线程安全

```c++
zcc::msearch_reader reader;
if (zcc::msearch_reader::is_my_file("a.cdb"))
{
    // 从文件加载, 也可以从数据Buffer加载
    if (reader.load_from_file("./a.cdb") < 1)
    {
        zcc_error_and_exit("can not open a.cdb");
    }
}

// 测试数据
std::string con = zcc::file_get_contents_sample("some.txt");
int offset;
const char *result;
int len = reader.match(con.c_str(), con.size(), &result, &offset);
if (len < 1)
{
    zcc_info("NOT FOUND");
}
else
{
    std::string s(result, offset);
    zcc_info("FOUND: %s", s.c_str());
}
```

## 遍历

```c++
    zcc::msearch_reader reader;
    if (reader.load_from_file("./a.cdb") < 1)
    {
        zcc_error_and_exit("can not open a.cdb");
    }
    zcc::msearch_walker walker(reader);
    const char *token;
    int tlen;
    while (walker.walk(&token, &tlen) > 0)
    {
        std::string s(token, tlen);
        zcc_info("key: %s", s.c_str());
    }
```

## 例子

* ../blob/master/cpp_sample/search/

