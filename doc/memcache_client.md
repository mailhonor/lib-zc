
[C++版本](./memcache_client_cpp.md)

## MEMCACHE 客户端, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装 memcache 客户端

## 函数: 基本操作

### zmemcache_client_t *zmemcache_client_connect(const char *destination, int connect_timeout);

* 创建连接器
* destination: 目的地址
* password: 密码, 空(没有密码)
* connect_timeout: 连接超时

### void zmemcache_client_disconnect(zmemcache_client_t *mc);

* 关闭连接, 并释放

### void zmemcache_client_set_connect_timeout(zmemcache_client_t *mc, int connect_timeout);

* 设置 connect 超时时间
* 重连的时候有效

### void zmemcache_client_set_read_wait_timeout(zmemcache_client_t *mc, int read_wait_timeout);

* 设置可读超时时间

### void zmemcache_client_set_write_wait_timeout(zmemcache_client_t *mc, int write_wait_timeout);

* 设置可写超时时间

### void zmemcache_client_set_auto_reconnect(zmemcache_client_t *mc, zbool_t auto_reconnect);

* 设置是否自动重连

## 函数: 命令

### int zmemcache_client_version(zmemcache_client_t *mc, zbuf_t *version);

* VERSION命令
* 输出存储在 version
* 返回 -1: 错
* 返回 1: 成功

### int zmemcache_client_get(zmemcache_client_t *mc, const char *key, int *flag, zbuf_t *value);

* GET命令
* key: 键
* flag: 标记
* data/len: 值/长度;
* 输出存储到 *value, 标记存储到 *flag
* 返回 -1: 错
* 返回 0: 不存在
* 返回 1: 存在 


### int zmemcache_client_add(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len);<BR />int zmemcache_client_set(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len);<BR />int zmemcache_client_replace(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len);<BR />int zmemcache_client_append(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len);<BR />int zmemcache_client_prepend(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 

* ADD/SET/REPLACE/APPEND/PREPEND命令
* 键为 key
* 返回 -1: 错
* 返回 0: 失败
* 返回 1: 成功

### long zmemcache_client_incr(zmemcache_client_t *mc, const char *key, unsigned long n);

* INCR 命令
* 键为 key
* 返回 -1: 错
* 返回 &gt;= 0: 命令的输出结果

### long zmemcache_client_decr(zmemcache_client_t *mc, const char *key, unsigned long n);

* DECR 命令
* 键为 key
* 返回 -1: 错
* 返回 &gt;= 0: 命令的输出结果

### int zmemcache_client_del(zmemcache_client_t *mc, const char *key);

* DEL命令
* 键为 key
* 返回 -1: 错
* 返回 0: 不存在
* 返回 1: 删除成功

### int zmemcache_client_flush_all(zmemcache_client_t *mc, long after_second);

* FLUASH_ALL命令
* 返回 -1:错
* 返回 0: 未知
* 返回 1: 成功

## 例子

* [goto](../sample/memcache/client.c)

