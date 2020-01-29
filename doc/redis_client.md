## redis 客户端

支持单点和集群模式. 源码见 src/redis/

---

```
/* 创建连接器 */
/* destinations: 见 zconnect */
/* password: 密码, 空: 没有密码 */
/* cmd_timeout: 连接超时,单位秒 */
/* auto_reconnect: 是否自动重连 */
zredis_client_t *zredis_client_connect(const char *destination, const char *password, int cmd_timeout, zbool_t auto_reconnect);

/* 创建连接器, 同上. 连接集群 */
zredis_client_t *zredis_client_connect_cluster(const char *destination, const char *password, int cmd_timeout, zbool_t auto_reconnect);

/* 设置命令超时时间, 单位秒 */
void zredis_client_set_cmd_timeout(zredis_client_t *rc, int cmd_timeout);

/* 设置是否自动重连 */
void zredis_client_set_auto_reconnect(zredis_client_t *rc, zbool_t auto_reconnect);

/* 获取错误信息 */
const char * zredis_client_get_error_msg(zredis_client_t *rc);

/* 释放 */
void zredis_client_free(zredis_client_t *rc);

/* redis命令返回结果可以抽象为json, 绝大部分可以简化为4类:
 * 1: 成功/失败, 2: 整数, 3: 字符串, 4:字符换vecgtor */

/* 下面的 (redis_fmt, ...) 介绍:
 * 's':  char *; 'S': zuf_t *; d: int; l: long int; f: double;
 * 'L': zlist_t * <zbuf_t *>
 * 'V': zvector_t * <zbuf_t *>
 * 'A': zargv_t *
 * 'P': char **, 0结尾
 * 例子1: 检查key是否存在:
 *      zredis_client_get_success(rc, "ss", "EXISTS", "somekey");
 * 例子2: 将 key 所储存的值加上增量 increment:
 *      zredis_client_get_long(rc, &number_ret, "ssl", "INRBY", "somekey", some_longint); 或
 *      zredis_client_get_long(rc, &number_ret, "sss", "INRBY", "somekey", "some_longint_string"); 或
 * */

/* 返回 -1: 错; 0: 失败/不存在/逻辑错误/...; 1: 成功/存在/逻辑正确/... */
int zredis_client_get_success(zredis_client_t *rc, const char *redis_fmt, ...);

/* 返回: 如上; 一些命令, 适合得到一个整数结果并赋值给 *number_ret, 如 klen/incrby/ttl 等 */
int zredis_client_get_long(zredis_client_t *rc, long *number_ret, const char *redis_fmt, ...);

/* 返回: 如上; 一些命令, 适合得到一个字符串结果赋值给string_ret, 如 GET/HGET/ */
int zredis_client_get_string(zredis_client_t *rc, zbuf_t *string_ret, const char *redis_fmt, ...);

/* 返回: 如上; 一些命令, 适合得到一串字符串结果并赋值给string_ret, 如 MGET/HMGET/ */
int zredis_client_get_vector(zredis_client_t *rc, zvector_t *vector_ret, const char *redis_fmt, ...);

/* 返回: 如上; 所有命令都可以用 zredis_client_get_json */
int zredis_client_get_json(zredis_client_t *rc, zjson_t *json_ret, const char *redis_fmt, ...);

/* 返回: 如上; 多类结论 */
int zredis_client_vget(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, const char *redis_fmt, va_list ap);

/* 返回: 如上; 命令选择 SCAN/HSCAN/SSCAN/ZSCAN/... */
/* vector_ret: 保存当前结果; *cursor_ret: 保存cursor */
int zredis_client_scan(zredis_client_t *rc,zvector_t *vector_ret,long *cursor_ret,const char *redis_fmt, ...);

/* 返回: 如上; 命令info, 对返回的字符换结果分析成词典, 保存在info */
int zredis_client_get_info_dict(zredis_client_t *rc, zdict_t *info);

/* 返回: 如上; 订阅命令, 不必输入 "SUBSCRIBE" */
int zredis_client_subscribe(zredis_client_t *rc, const char *redis_fmt, ...);

/* 返回: 如上; 模式订阅命令, 不必输入 "PSUBSCRIBE" */
int zredis_client_psubscribe(zredis_client_t *rc, const char *redis_fmt, ...);

/* 返回: 如上; 获取消息, 结果保存在 vector_ret; 如果没有消息则阻塞 */
int zredis_client_fetch_channel_message(zredis_client_t *rc, zvector_t *vector_ret);


/* redis puny server ######################################### */
/* 模拟标准redis服务, 部分支持 键/字符串/哈希表 */
extern void (*zredis_puny_server_before_service)(void);
extern void (*zredis_puny_server_before_reload)(void);
extern void (*zredis_puny_server_before_exit)(void);
extern void (*zredis_puny_server_service_register) (const char *service, int fd, int fd_type);
int zredis_puny_server_main(int argc, char **argv);
void zredis_puny_server_exec_cmd(zvector_t *cmd);

```

---

### 例子
见源码 sample/redis
