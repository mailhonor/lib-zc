<A name="readme_md" id="readme_md"></A>

## REDIS 客户端, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了 redis 客户端,
支持单点(standalone)和集群(cluster)

必须了解 redis 的命令和协议才能顺利使用本客户端, 请参阅 http://doc.redisfans.com/

## 函数: 基本操作

### zredis_client_t *zredis_client_connect(const char *destination, const char *password, int connect_timeout);

* 创建连接器
* destination: 目的地址
* password: 密码, 空(没有密码)
* connect_timeout: 连接超时

### zredis_client_t *zredis_client_connect_cluster(const char *destination, const char *password, int cmd_timeout);

* 创建连接
* 参数同上

### void zredis_client_disconnect(zredis_client_t *rc);

* 关闭连接, 并释放

### void zredis_client_set_read_wait_timeout(zredis_client_t *rc, int read_wait_timeout);

* 设置可读超时

### void zredis_client_set_write_wait_timeout(zredis_client_t *rc, int write_wait_timeout);

* 设置可写超时

### void zredis_client_set_auto_reconnect(zredis_client_t *rc, zbool_t auto_reconnect);

* 设置是否自动重连

### const char *zredis_client_get_error_msg(zredis_client_t *rc);

* 获取错误信息

## 函数: 通用redis命令

redis 命令返回结果可以抽象为json

redis 命令返回结果大部分情况可以简化为 4 类: 成功/失败, 整数, 字符串, 字符串向量

对下面的函数做一个统一说明:

* 执行一个 redis 命令, 所谓 "redis 命令" 如何执行稍后再谈
* 返回 -1: 错
* 返回 0: 失败/不存在/逻辑错误/等
* 返回 &gt;0: 成功/存在/逻辑正确/等

### int zredis_client_get_json(zredis_client_t *rc, zjson_t *json_ret, const char *redis_fmt, ...);

* 通用
* 得到的结果存储到 json_ret

### int zredis_client_get_success(zredis_client_t *rc, const char *redis_fmt, ...);

* 结果体现在返回值上
* 适合 EXISTS, DEL, HEXISTS, SISMEMBER 等

### int zredis_client_get_long(zredis_client_t *rc, long *number_ret, const char *redis_fmt, ...);

* 得到一个整数结果, 并赋值给 *number_ret
* 适合 INCR, DBSIZE, HLEN 等

### int zredis_client_get_string(zredis_client_t *rc, zbuf_t *string_ret, const char *redis_fmt, ...);

* 得到一个字符串结果, 并赋值(覆盖)给 string_ret
* 适合 GET, LPOP, 等

### int zredis_client_get_vector(zredis_client_t *rc, zvector_t *vector_ret, const char *redis_fmt, ...);

* 得到一字符串向量结果, 并赋值(追加)给 vector_ret;
* 适合 HGETALL, MGET, HMGET 等

### int zredis_client_vget(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret,<BR />&nbsp;&nbsp;zvector_t *vector_ret, zjson_t *json_ret, const char *redis_fmt, va_list ap);

* 得到结果, 尽量写入 *number_ret, string_ret, vector_ret, json_ret

## redis_fmt

 介绍所谓 "redis 命令", 所有的 redis 命令都是通过上面的(redis_fmt, ...)拼接得到的

### redis_fmt 和 (redis_fmt, ...)

<table class="tbview" width="100%">
<tr><td class="label">类型</td><td>解释</td></tr>
<tr><td>s</td><td>char *</td></tr>
<tr><td>S</td><td>zbuf_t *</td></tr>
<tr><td>d</td><td>int</td></tr>
<tr><td>l</td><td>long</td></tr>
<tr><td>f</td><td>doubel</td></tr>
<tr><td>L</td><td>zlist_t *; &lt;zbuf_t *&gt;</td></tr>
<tr><td>V</td><td>zvector_t *; &lt;zbuf_t *&gt;</td></tr>
<tr><td>A</td><td>zargv_t *</td></tr>
<tr><td>P</td><td>char **; NULL结尾</td></tr>
</table>

### 命令例子

```
zredis_client_get_success(rc, "ss", "EXISTS", "somekey");
```

```
zbuf_t *number_ret = zbuf_create(-1);
zredis_client_get_long(rc, &number_ret, "ssl", "INRBY", "somekey", 123);
zredis_client_get_long(rc, &number_ret, "sss", "INRBY", "somekey", "123");
```

```
zredis_client_get_vector(rc, vector_ret, "sss", "MGET", "abc1", "def2");

zbuf_t *bf = zbuf_create(-1); zbuf_strcpy(bf, "def2);
zredis_client_get_vector(rc, vector_ret, "ssS", "MGET", "abc1", bf);

zargv_t *argv = zargv_create(-1); zargv_add(argv, "abc1"); zargv_add(argv, "def2");
zredis_client_get_vector(rc, vector_ret, "sA", argv);

const char *pp[4]; pp[0]="MGET"; pp[1]="abc1"; pp[2]="def2"; pp[3]=0;
zredis_client_get_vector(rc, vector_ret, "P", pp);
```

## 函数: SCAN相关

SCAN/HSCAN/SSCAN/ZSCAN 这些 redis 命令当然也可以通过上节所提函数使用

但不自然, 所以单独封一个函数使用

### int zredis_client_scan(zredis_client_t *rc, zvector_t *vector_ret, long *cursor_ret, const char *redis_fmt, ...);

* vector_ret: 保存当前结果
* *cursor_ret: 保存 cursor
* 使用的时候 *cursor_ret 设置为初始值(如 0)

## 函数: 命令 INFO

redis 命令 info 返回的是一堆字符串, 下面提供函数, 同时解析 info 的返回为一个词典

### int zredis_client_get_info_dict(zredis_client_t *rc, zdict_t *info);

* redis 命令 INFO
* 对返回的字符换结果分析成词典, 保存在info

## 函数: 订阅

### int zredis_client_subscribe(zredis_client_t *rc, const char *redis_fmt, ...);

* 订阅频道
* 不能输入命令字段 "SUBSCRIBE"

### int zredis_client_psubscribe(zredis_client_t *rc, const char *redis_fmt, ...);

* 模式订阅频道
* 不能输入命令字段 "PSUBSCRIBE"

### int zredis_client_fetch_channel_message(zredis_client_t *rc, zvector_t *vector_ret);

* 获取消息
* 结果保存在 vector_ret
* 有消息则返回
* 没有消息则阻塞 

### 简单用法

```
if (zredis_client_subscribe(rc, "ss", "chan1", "chan2")< 1) {
    printf("ERR network or protocal; can not subscribe\n"); exit(1);
}

zvector_t *msg_vec = zvector_create(-1);
for (int i = 0; i < 100; i++) {
    zbuf_vector_reset(msg_vec);
    int ret = zredis_client_fetch_channel_message(rc, msg_vec);
    if (ret < 0) {
        printf("ERR network or protocal\n");
        break;
    } else if (ret == 0) {
        printf("no message\n");
    } else {
        ZVECTOR_WALK_BEGIN(msg_vec, zbuf_t *, bf) {
            printf("%s\n", zbuf_data(bf));
        } ZVECTOR_WALK_END;
        printf("\n");
    }
}
```

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/redis/client.c
* https://gitee.com/linuxmail/lib-zc/blob/master/sample/redis/subscribe.c

