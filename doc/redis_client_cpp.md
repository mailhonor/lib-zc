
## REDIS 客户端, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了 redis 客户端,
支持单点(standalone)和集群(cluster)

必须了解 redis 的命令和协议才能顺利使用本客户端, 请参阅 http://doc.redisfans.com/

[C版本](./redis_client.md)

## redis 客户端规律

redis命令返回结果可以抽象为json, 绝大部分可以简化为4类:
* 成功/失败
* 整数
* 字符串
* 字符串 list

作者也写过几个版本的客户端, 都不满意, 封装的东西很难满足使用者包括本人的需求. 哪怕仅仅是 GET 协议也是如此.

最后采用如下类似通用封装:

``` c++
// zcc::redis_client rc;
std::string sval;
rc.exec_command(sval, {"GET", "abc"});
```

## 返回值
* redis_client 类里面所有的操作类方法,其返回值规范
* -2: 网络错误/系统错误/协议错误
* -1: 逻辑错误
*  0: 失败/不存在/逻辑错误/等
* >0: 成功/存在/逻辑正确/等

## zcc::redis_clinet

方法很有先, 直接见 include

* [goto](../blob/master/include/zcc/zcc_redis.h)


## 简单的用法

```c++
zcc::redis_client rc;
rc.set_password("123456");
if (rc.connect("127.0.0.1:6379") < 1)
{
    if (c_ret == 0)
    {
        std::printf("auth failed\n");
    }
    else if (c_ret == -1)
    {
        std::printf("unknown error\n");
    }
    else
    {
        std::printf("open error\n");
    }
    zcc::exit(1);
}
std::string sval;
int64_t lval;
zcc::json jval;
rc.exec_command({"SET", "", "ssssss"});
rc.exec_command({"GET", "abc"});
rc.exec_command(sval, {"GET", "abc"});
rc.exec_command(sval, {"HGET", "xxx.com_u", "ac.tai"});

rc.exec_command({"STRLEN", "abc"});
rc.exec_command({"STRLEN", "abc"});

rc.exec_command(lval, {"mget", "abc", "fasdfdsaf"});

rc.exec_command(jval, {"MGET", "abc", "sss"});
rc.exec_command(jval, {"SCAN", "0"});
rc.exec_command(jval, {"EVAL", "return {1,2,{3,'Hello World!'}}", "0"});
rc.exec_command(jval, {"fffSCAN", "0"});
```

## 例子

* [goto](../blob/master/cpp_sample/redis/)

