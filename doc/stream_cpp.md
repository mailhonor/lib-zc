
## IO 流(stream), [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持IO 流(stream), 其 STRUCT 类型是 **zstream_t**

[C版本](./stream.md)

## 用法

* [goto](../blob/master/include/zcc/zcc_stream.h)

## 简单用法

```c++
#include "zcc/zcc_stream.h"

static void () {
    zcc::iostream fp;
    fp.set_timeout(timeout);
    if (!fp.connect("127.0.0.1:25")) {
        // ...
    }
    std::printf("connected\n");
    fp.gets(tmpline, 10240);
    std::printf("S: %s", tmpline.c_str());
    
    //
    fp.write("HELO XXX\r\n");
    // fp.flush();

    fp.gets(tmpline, 10240);
    // ....
    
    // tls
    // fp.tls_connect(ssl_ctx);
}
```

## 例子: 连接 smtp 服务器

* [goto](../blob/master/cpp_sample/stream/)
