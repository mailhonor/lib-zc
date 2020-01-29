## memcache

memcache客户端. 源码见 src/memcache/

---

```
/* 创建连接器; destination: 见 zconnect; cmd_timeout: 连接超时,单位秒; auto_reconnect: 是否自动重连 */
zmemcache_client_t *zmemcache_client_connect(const char *destination, int cmd_timeout, zbool_t auto_reconnect);

/* 设置命令超时时间, 单位秒 */
void zmemcache_client_set_cmd_timeout(zmemcache_client_t *mc, int timeout);

/* 设置是否自动重连 */
void zmemcache_client_set_auto_reconnect(zmemcache_client_t *mc, zbool_t auto_reconnect);

/* 断开连接 */
void zmemcache_client_disconnect(zmemcache_client_t *mc);

/* GET命令, 返回 -1: 错, 0: 不存在, 1: 存在 */
/* key: 键; *flag: 返回的标记; value: 返回的值, zbuf_reset(value) */
int zmemcache_client_get(zmemcache_client_t *mc, const char *key, int *flag, zbuf_t *value);

/* ADD/SET/REPLACE/APPEND/PREPEND命令, 返回 -1:错; 0:存储失败; 1:存储成功 */
/* key: 键; flag: 标记; data/len: 值/长度; */
int zmemcache_client_add(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_set(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_replace(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_append(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_prepend(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 

/* INCR命令, 返回 -1: 错; >= 0: incr的结果 */
long zmemcache_client_incr(zmemcache_client_t *mc, const char *key, unsigned long n);

/* DECR命令, 返回 -1: 错; >= 0: decr的结果 */
long zmemcache_client_decr(zmemcache_client_t *mc, const char *key, unsigned long n);

/* DEL命令, 返回 -1: 错; 0: 不存在; 1: 删除成功 */
int zmemcache_client_del(zmemcache_client_t *mc, const char *key);


/* FLUASH_ALL命令, 返回 -1:错; 0: 未知; 1: 成功 */
int zmemcache_client_flush_all(zmemcache_client_t *mc, long after_second);

/* VERSION命令, -1: 错; 1:成功 */
int zmemcache_client_version(zmemcache_client_t *mc, zbuf_t *version);
```

---

### 例子
见源码 sample/memcache/
