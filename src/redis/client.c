/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zc.h"

#define zredis_client_error_msg_size 64
struct zredis_client_t {
    char *password;
    char *destination;
    int cmd_timeout;
    unsigned int cluster_mode:1;
    unsigned int auto_reconnect:1;
    char error_msg[zredis_client_error_msg_size + 1];
};

/* {{{ engine ############################################################# */
static zbuf_t *___zbuf_create_from_str(const char *s, int len)
{
    if (len < 0) {
        if (!s) {
            len = 0;
        } else {
            len = strlen(s);
        }
    }
    zbuf_t *bf = zbuf_create(len);
    if (len) {
        zbuf_memcpy(bf, s, len);
    }
    return bf;
}

static int ___query_by_io_vector(zbuf_t *rstr, char *error_msg, int lnum, zvector_t *lval, zstream_t *fp)
{
    char *rp, firstch;
    int rlen, i, tmp, num2;
    for (i = 0; i < lnum ;i++) {
        zbuf_reset(rstr);
        if (zstream_gets(fp, rstr, 1024 * 1024) < 3) {
            strcpy(error_msg, "the length of the response < 3");
            return -2;
        }
        rp = zbuf_data(rstr);
        rlen = zbuf_len(rstr) - 2;
        rp[rlen] = 0;
        firstch = rp[0];
        if (firstch  == '*') {
            tmp = atoi(rp + 1);
            if (tmp < 1) {
                if (lval) {
                    zvector_push(lval, ___zbuf_create_from_str(zblank_buffer, 0));
                }
            } else {
                lnum += tmp;
            }
        } else if (firstch == ':') {
            if (rlen  < 1) {
                if (lval) {
                    zvector_push(lval, ___zbuf_create_from_str(zblank_buffer, 0));
                }
            } else {
                if (lval) {
                    zvector_push(lval, ___zbuf_create_from_str(rp+1, rlen-1));
                }
            }
        } else if (firstch == '+') {
        } else if (firstch == '$') {
            num2 = atoi(rp+1);
            zbuf_reset(rstr);
            if (num2 >0) {
                zstream_readn(fp, rstr, num2);
            }
            if (num2 > -1) {
                zstream_readn(fp, 0, 2);
            }
            if (zstream_is_exception(fp)) {
                strcpy(error_msg, "read/write");
                return -2;
            }
            if (lval) {
                if (num2 > -1) {
                    zvector_push(lval, ___zbuf_create_from_str(zbuf_data(rstr), zbuf_len(rstr)));
                } else {
                    zvector_push(lval, 0);
                }
            }
        } else {
            strcpy(error_msg, "the initial of the response shold be $");
            return -2;
        }
    }
    return 1;
}

static int ___query_by_io_vector_json(zbuf_t *rstr, char *error_msg, int lnum, zjson_t *jval, zstream_t *fp)
{
    long idx, num, tmp;
    int ret, rlen, num2;
    char *rp, firstch;
    zjson_t *jn, *jn_tmp;
    zvector_t *stack_vec = zvector_create(9); /* <int num, int idx, zjson_t *> */
    
    tmp = lnum;
    zvector_push(stack_vec, (void *)tmp);
    zvector_push(stack_vec, (void *)-1);
    zvector_push(stack_vec, jval);

    zjson_get_array_value(jval);

    while(zvector_len(stack_vec)) {
        zvector_pop(stack_vec, (void **)&jn);
        zvector_pop(stack_vec, (void **)&idx);
        idx = idx+1;
        zvector_pop(stack_vec, (void **)&num);
        for (; idx < num ;idx++) {
            zbuf_reset(rstr);
            if (zstream_gets(fp, rstr, 1024 * 1024) < 3) {
                strcpy(error_msg, "the length of the response < 3");
                ret = -2;
                goto over;
            }
            rp = zbuf_data(rstr);
            rlen = zbuf_len(rstr) - 2;
            rp[rlen] = 0;
            firstch = rp[0];
            if (firstch == '*') {
                tmp = atoi(rp + 1);
                if (tmp < 1) {
                    zjson_array_push(jn, zjson_create_string("", 0));
                } else {
                    zvector_push(stack_vec, (void *)num);
                    zvector_push(stack_vec, (void *)idx);
                    zvector_push(stack_vec, jn);

                    zvector_push(stack_vec, (void *)tmp);
                    zvector_push(stack_vec, (void *)-1);
                    zvector_push(stack_vec, zjson_array_push(jn, zjson_create()));
                }
                break;
            }
            if (firstch == ':') {
                zjson_array_push(jn, zjson_create_long((rlen < 1)?-1:atol(rp+1)));
                continue;
            }
            if (firstch == '+') {
                continue;
            }
            if (firstch == '$') {
                num2 = atoi(rp+1);
                jn_tmp = zjson_array_push(jn, zjson_create());
                if (num2 < 0){
                } else if (num2 < 1){
                    zjson_get_string_value(jn_tmp);
                } else {
                    zstream_readn(fp, (*zjson_get_string_value(jn_tmp)), num2);
                }
                if (num2 > -1) {
                    zstream_readn(fp, 0, 2);
                }
                if (zstream_is_exception(fp)) {
                    strcpy(error_msg, "read/write");
                    ret = -2;
                    goto over;
                }
                continue;
            }
            strcpy(error_msg, "the initial of the response shold be $");
            ret = -2;
            goto over;
        }
    }
    ret = 1;
over:
    zvector_free(stack_vec);
    return ret;
}

static int ___query_by_io_string(zbuf_t *rstr, char *error_msg, int length, zbuf_t *sval, zstream_t *fp)
{
    if (length < 0) {
        return 0;
    }
    if (length >0){
        zstream_readn(fp, sval, length);
    }
    zstream_readn(fp, 0, 2);
    if (zstream_is_exception(fp)) {
        strcpy(error_msg, "read/write");
        return -2;
    }
    return 1;
}

static int ___query_by_io_string_json(zbuf_t *rstr, char *error_msg, int length, zjson_t *jval, zstream_t *fp)
{
    if (length < 0) {
        return 0;
    }
    if (length >0){
        zstream_readn(fp, *zjson_get_string_value(jval), length);
    }
    zstream_readn(fp, 0, 2);
    if (zstream_is_exception(fp)) {
        strcpy(error_msg, "read/write");
        return -2;
    }
    return 1;
}

static int ___query_by_io_prepare_and_write(zbuf_t *rstr, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, zvector_t *query_vec, long cmd_timeout, char *error_msg, zstream_t *fp)
{
    int ret;
    error_msg[0] = 0;
    if (string_ret) {
        zbuf_reset(string_ret);
    }
    if (vector_ret) {
        zbuf_vector_reset(vector_ret);
    }
    if (json_ret) {
        zjson_reset(json_ret);
    }

    if (zvector_len(query_vec) == 0) {
        if ((ret = zstream_timed_read_wait(fp, cmd_timeout)) < 0) {
            strcpy(error_msg, "read timeout");
            return -2;
        } else if (ret == 0 ) {
            strcpy(error_msg, "no available readable data");
            return 0;
        }
        zstream_set_timeout(fp, cmd_timeout);
        if (zstream_gets(fp, rstr, 1024*1024) < 3) {
            strcpy(error_msg, "data too short");
            return -2;
        }
        return 1;
    }

    if (zstream_timed_write_wait(fp, cmd_timeout) < 1) {
        strcpy(error_msg, "write timeout");
        return -2;
    }
    zstream_set_timeout(fp, cmd_timeout);
    zstream_printf_1024(fp, "*%d\r\n", zvector_len(query_vec));
    ZVECTOR_WALK_BEGIN(query_vec, zbuf_t *, bf) {
        zstream_printf_1024(fp, "$%d\r\n", zbuf_len(bf));
        zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
        zstream_write(fp, "\r\n", 2);

    } ZVECTOR_WALK_END;
    zstream_flush(fp);

    if (number_ret == (long *)-1) {
        return 3;
    }

    zstream_set_timeout(fp, cmd_timeout);
    if (zstream_gets(fp, rstr, 1024*1024) < 3) {
        strcpy(error_msg, "data too short");
        return -2;
    }
    return 1;
}

static int ___query_by_io(long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, zvector_t *query_vec, long cmd_timeout, char *error_msg, zstream_t *fp)
{
    char *rp;
    int rlen, num, firstch, ret;
    long lret;
    zbuf_t *rstr = zbuf_create(-1);

    if ((ret = ___query_by_io_prepare_and_write(rstr, number_ret, string_ret, vector_ret, json_ret, query_vec,  cmd_timeout,error_msg, fp)) < 1) {
        goto over;
    }
    if (ret == 3) {
        ret = 1;
        goto over;
    }
    rp = zbuf_data(rstr);
    rlen = zbuf_len(rstr) - 2;
    rp[rlen] = 0;
    firstch = rp[0];
    if (firstch == '-') {
        strncpy(error_msg, rp, rlen>zredis_client_error_msg_size?zredis_client_error_msg_size:rlen);
        if (json_ret) {
            (*zjson_get_bool_value(json_ret)) = 1;
        }
        ret = -1;
        goto over;
    }
    if (firstch == '+') {
        ret = 1;
        if (json_ret) {
            (*zjson_get_bool_value(json_ret)) = 1;
        }
        goto over;
    }
    if (firstch == '*') {
        num = atoi(rp + 1);
        if (json_ret) {
            ret = ___query_by_io_vector_json(rstr, error_msg, num, json_ret, fp);
        } else {
            ret = ___query_by_io_vector(rstr, error_msg, num, vector_ret, fp);
        }
        goto over;
    }
    if (firstch == ':') {
        lret = atol(rp + 1);
        if (json_ret) {
            (*zjson_get_long_value(json_ret)) = lret;
        }
        if (number_ret) {
            *number_ret = lret;
        }
        ret = 1;
        goto over;
    }
    if (firstch == '$') {
        num = atoi(rp+1);
        if (json_ret) {
            ret =  ___query_by_io_string_json(rstr, error_msg, num, json_ret, fp);
        } else { 
            ret =  ___query_by_io_string(rstr, error_msg, num, string_ret, fp);
        }
        goto over;
    }
    strcpy(error_msg, "read/write, or unknown protocol");
over:
    zbuf_free(rstr);
    return ret;
}

#pragma pack(push,1)
typedef struct {
    unsigned char count;
    const char *vector;
    const char *info;
} redis_cmd_info_t;
#pragma pack(pop)

static redis_cmd_info_t redis_cmd_info_vector[] = 
{
    {
        4,
        "DEL" "GET" "SET" "TTL",
        "bbbb"
    },
    {
        35,
        "AUTH" "DECR" "DUMP" "ECHO" "EVAL"
            "EXEC" "HDEL" "HGET" "HLEN" "HSET"
            "INCR" "INFO" "KEYS" "LLEN" "LPOP"
            "LREM" "LSET" "MGET" "MOVE" "MSET"
            "PING" "PTTL" "QUIT" "RPOP" "SADD"
            "SAVE" "SCAN" "SORT" "SPOP" "SREM"
            "SYNC" "TIME" "TYPE" "ZADD" "ZREM",
        "AbbAA" "Abbbb" "bAAbb" "bbbbb" "AbAbb" "AAbbb" "AAbbb"
    },
    {
        24,
         "BITOP" "BLPOP" "BRPOP" "DEBUG" "HKEYS"
            "HMGET" "HMSET" "HSCAN" "HVALS" "LPUSH"
            "LTRIM" "MULTI" "PSYNC" "RPUSH" "SCARD"
            "SDIFF" "SETEX" "SETNX" "SMOVE" "SSCAN"
            "WATCH" "ZCARD" "ZRANK" "ZSCAN",
        "cbbAb" "bbbbb" "bAAbb" "bbbbb" "bbbb"
    },
    {
        31,
        "APPEND" "BGSAVE" "CLIENT" "CONFIG" "DBSIZE"
            "DECRBY" "EXISTS" "EXPIRE" "GETBIT" "GETSET"
            "HSETNX" "INCRBY" "LINDEX" "LPUSHX" "LRANGE"
            "MSETNX" "OBJECT" "PSETEX" "PUBSUB" "RENAME"
            "RPUSHX" "SCRIPT" "SELECT" "SETBIT" "SINTER"
            "STRLEN" "SUNION" "ZCOUNT" "ZRANGE" "ZSCORE"
            "DISCARD",

        "bAAAA" "bbbbb"  "bbbbb" "bAbAb" "bABbb" "bbbbb" "B"
    },
    {
        16,
        "EVALSHA" "FLUSHDB" "HEXISTS" "HGETALL" "HINCRBY"
            "LINSERT" "MIGRATE" "MONITOR" "PERSIST" "PEXPIRE"
            "PUBLISH" "RESTORE" "SLAVEOF" "SLOWLOG" "UNWATCH"
            "ZINCRBY",
        "AAbbb" "bdbbb" "AbAAB" "b"
    },
    {
        10,
        "BITCOUNT" "EXPIREAT" "FLUSHALL" "GETRANGE" "LASTSAVE"
            "RENAMENX" "SETRANGE" "SHUTDOWN" "SMEMBERS" "ZREVRANK",
        "bbAbA" "bbAbb"
    },
    {
        6,
        "PEXPIREAT" "RANDOMKEY" "RPOPLPUSH" "SISMEMBER" "SUBSCRIBE"
            "ZREVRANGE",
        "bAbbA" "b"
    },
    {
        3,
        "BRPOPLPUSH" "PSUBSCRIBE" "SDIFFSTORE",
        "bAb"
    },
    {
        7,
        "INCRBYFLOAT" "SINTERSTORE" "SRANDMEMBER" "SUNIONSTORE" "UNSUBSCRIBE"
            "ZINTERSTORE" "ZUNIONSTORE",
        "bbbbA" "bb"
    },
    {
        3,
        "BGREWRITEAOF" "HINCRBYFLOAT" "PUNSUBSCRIBE",
        "AbA"
    },
    {
        1,
        "ZRANGEBYSCORE",
        "b"
    },
    {
        0,
        0,
        0
    },
    {
        1,
        "ZREMRANGEBYRANK",
        "b"
    },
    {
        1,
        "ZREMRANGEBYSCORE" "ZREVRANGEBYSCORE",
        "bb"
    }
};

static int ___query_get_hash_key_idx(const char *_cmd, int clen)
{
    if (clen < 0) {
        if (_cmd == 0) {
            return -1;
        }
        clen = strlen(_cmd);
    }
    char cmd[20];
    if ((clen > 16) || (clen <3)) {
        return -1;
    }
    memcpy(cmd, _cmd, clen);
    cmd[clen] = 0;
    zstr_toupper(cmd);
    char *cmd_p = cmd;
    redis_cmd_info_t *info = redis_cmd_info_vector + (clen - 3);
    if (info->count == 0) {
        return -1;
    }
    int cmd_count = info->count;
    const char *cmd_vector = info->vector;
    int left = 0, right = cmd_count -1, middle;
    int found = 0;
    while (left <= right) {
        middle = (left + right)/2;
        int rcmp = strncmp(cmd_p, cmd_vector + clen * middle, clen);
        if (rcmp < 0) {
            right = middle - 1;
        } else  if (rcmp > 0) {
            left = middle + 1;
        } else {
            found = 1;
            break;
        }
    }
    if (found) {
        char r =info->info[middle];
        if (r =='A') {
            return -2;
        }
        if (r == 'B') {
            return -3;
        }
        return (r - 'a');
    }
    return -1;
}

/* }}} */

/* {{{ standalone ######################################################### */
typedef struct zredis_client_standalone_t zredis_client_standalone_t;
struct zredis_client_standalone_t {
    zredis_client_t rc;
    int fd;
    zstream_t *fp;
};

zredis_client_t *zredis_client_connect(const char *destination, const char *password, int cmd_timeout, zbool_t auto_reconnect)
{
    int fd = zconnect(destination, 1, cmd_timeout);
    if (fd < 0) {
        return 0;
    }
    zredis_client_t *rc = (zredis_client_t *)zcalloc(1, sizeof(zredis_client_standalone_t));
    zredis_client_standalone_t *sc = (zredis_client_standalone_t *)rc;
    if (!zempty(password)) {
        rc->password = zstrdup(password);
    }
    rc->destination = zstrdup(destination);
    rc->cmd_timeout = cmd_timeout;
    rc->cluster_mode = 0;
    rc->auto_reconnect = auto_reconnect;
    sc->fd = fd;
    return rc;
}

static int zredis_client_standalone_vget_inner(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, zvector_t *query_vec)
{
    zredis_client_standalone_t *sc = (zredis_client_standalone_t *)rc;
    if (sc->fd < 0) {
        if (rc->auto_reconnect) {
            sc->fd = zconnect(rc->destination, 1, rc->cmd_timeout);
            snprintf(rc->error_msg, zredis_client_error_msg_size, "connect %s", rc->destination);
        }
    }
    if (sc->fd < 0) {
        return -1;
    }
    if (!(sc->fp)) {
        sc->fp = zstream_open_fd(sc->fd);
    }
    int ret = ___query_by_io(number_ret, string_ret, vector_ret, json_ret, query_vec, rc->cmd_timeout, rc->error_msg, sc->fp);
    if ((zstream_get_read_cache_len(sc->fp) == 0) || (ret == -2)) {
        zstream_close(sc->fp, 0);
        sc->fp = 0;
    }
    if (ret == -2) {
        zclose(sc->fd);
        sc->fd = -1;
        ret = -1;
    }
    return ret;
}

static void zredis_client_free_standalone_inner(zredis_client_t *rc)
{
    zredis_client_standalone_t *sc = (zredis_client_standalone_t *)rc;
    if (sc->fd > -1) {
        zclose(sc->fd);
        sc->fd = -1;
    }
    zfree(rc->password);
    zfree(rc->destination);
    zfree(rc);
}
/* }}} */

/* {{{ cluster ############################################################ */
typedef struct _cluster_connection_t _cluster_connection_t;
struct _cluster_connection_t {
    int ip;
    int port;
    int fd;
};

typedef struct _cluster_slotrange_t _cluster_slotrange_t;
struct _cluster_slotrange_t {
    short int start;
    short int end;
    short int connection_offset;
};

typedef struct zredis_client_cluster_t zredis_client_cluster_t;
struct zredis_client_cluster_t {
    zredis_client_t rc;
    _cluster_connection_t *connections;
    short int connection_size;
    short int connection_used;
    _cluster_slotrange_t *slotranges;
    short int slotrange_used;
    short int slotrange_size;
    zstream_t *fp;
};

zredis_client_t *zredis_client_connect_cluster(const char *destination, const char *password, int cmd_timeout, zbool_t auto_reconnect)
{
    int fd = zconnect(destination, 1, cmd_timeout);
    if (fd < 0) {
        return 0;
    }
    int zget_peername(int sockfd, int *host, int *port);
    zredis_client_t *rc = (zredis_client_t *)zcalloc(1, sizeof(zredis_client_cluster_t));
    zredis_client_cluster_t *cc = (zredis_client_cluster_t *)rc;
    if (!zempty(password)) {
        rc->password = zstrdup(password);
    }
    rc->destination = zstrdup(destination);
    rc->cmd_timeout = cmd_timeout;
    rc->cluster_mode = 1;
    rc->auto_reconnect = auto_reconnect;

    cc->connection_size = 16;
    cc->connection_used = 1;
    cc->connections = (_cluster_connection_t *)zmalloc(sizeof(_cluster_connection_t) * cc->connection_size);
    cc->connections->fd = fd;
    zget_peername(fd, &(cc->connections->ip), &(cc->connections->port));

    cc->slotrange_size = 16;
    cc->slotrange_used = 1;
    cc->slotranges = (_cluster_slotrange_t *)zmalloc(sizeof(_cluster_slotrange_t) * cc->slotrange_size);
    cc->slotranges->start = 0;
    cc->slotranges->end = 16383;
    cc->slotranges->connection_offset = 0;

    return rc;
}

static int _cluster_vget_inner_get_slotrange(zredis_client_cluster_t *cc, short int slot_id)
{
    for (int i=0;i<cc->connection_used;i++) {
        _cluster_slotrange_t *slots = cc->slotranges + i;
        if (slot_id <= slots->end) {
            return i;
        }
    }
    return 0;
}

static int _cluster_get_slod_id(zredis_client_t *rc, zvector_t *query_vec)
{
    zbuf_t *bf;
    int hash_key_idx = -1;
    short int slot_id = 0;
    if (zvector_len(query_vec)) {
        bf = (zbuf_t *)(zvector_data(query_vec)[0]);
        hash_key_idx = ___query_get_hash_key_idx(zbuf_data(bf), zbuf_len(bf));
        if (hash_key_idx >= zvector_len(query_vec)) {
            strcpy(rc->error_msg, "-ERR wrong number of arguments");
            return -1;
        }
        if (hash_key_idx > -1) {
            bf = (zbuf_t *)(zvector_data(query_vec)[hash_key_idx]);
            slot_id =  zcrc16(zbuf_data(bf), zbuf_len(bf), 0)%16384;
        }
    }
    return slot_id;
}

static _cluster_connection_t *_cluster_vget_inner_prepare(zredis_client_t *rc, zvector_t *query_vec, short int slot_id)
{
    zredis_client_cluster_t *cc = (zredis_client_cluster_t *)rc;
    char ip[18];
    int slotrange_idx = 0;
    _cluster_slotrange_t *slotrange;
    _cluster_connection_t *connection;
    slotrange_idx = _cluster_vget_inner_get_slotrange(cc, slot_id);
    slotrange = cc->slotranges + slotrange_idx;
    connection = cc->connections+slotrange->connection_offset;
    if (connection->fd < 0) {
        if ((connection->fd == -2) && (rc->auto_reconnect)) {
            connection->fd = zinet_connect(zget_ipstring(connection->ip, ip), connection->port, 1, rc->cmd_timeout);
            if (connection->fd < 0) {
                snprintf(rc->error_msg, zredis_client_error_msg_size, "connect %s:%d", zget_ipstring(connection->ip, ip), connection->port);
            }
        }
        if (connection->fd < 0) {
            return 0;
        }
    }
    if ((cc->fp) && (zstream_get_fd(cc->fp) != connection->fd)) {
        strcpy(rc->error_msg, "system");
        return 0;
    }
    if (!(cc->fp)) {
        cc->fp = zstream_open_fd(connection->fd);
    }
    return connection;
}

static int _cluster_after_query(zredis_client_t *rc, short int slot_id)
{
    zredis_client_cluster_t *cc = (zredis_client_cluster_t *)rc;
    int slotrange_idx = 0, connection_idx;
    _cluster_slotrange_t *slotrange, *next_slotrange;
    _cluster_connection_t *connection, *next_connection;
    char ip[18];
    char *p, *ps = rc->error_msg;

    if (ps[0] != '-') {
        return -1;
    }
    ps++;
    if ((*ps == 'M') &&(!strncmp(ps, "MOVED ", 6))) {
        ps += 6;
        p = strchr(ps, ' ');
        if (!p) {
            return -1;
        }
        *p = 0;
        if (slot_id != atoi(ps)) {
            return -1;
        }
        *p = ' ';
        ps = p + 1;
        p = strchr(ps, ':');
        if (!p) {
            return -1;
        }
        if ((p-ps < 7) || (p-ps > 16)) {
            return -1;
        }
        memcpy(ip, ps, p-ps);
        ip[p-ps] = 0;
        int ipint = zget_ipint(ip);
        int port = atoi(p+1);

        slotrange_idx = _cluster_vget_inner_get_slotrange(cc, slot_id);
        slotrange = cc->slotranges + slotrange_idx;
        connection = cc->connections+slotrange->connection_offset;
        if ((connection->ip==ipint) && (connection->port==port)) {
            return -1;
        }
        if (slotrange_idx + 1 < cc->slotrange_used) {
            next_slotrange = slotrange+1;
            next_connection = cc->connections + next_slotrange->connection_offset;
            if ((next_connection->ip == ipint) && (next_connection->port == port)) {
                next_slotrange->start = slot_id;
                slotrange->end = slot_id - 1;
                return 0;
            }
        }
        for (connection_idx = 0; connection_idx < cc->connection_used; connection_idx++) {
            next_connection = cc->connections + connection_idx;
            if ((next_connection->ip == ipint) && (next_connection->port == port)) {
                break;
            }
        }
        if (connection_idx == cc->connection_used) {
            if (cc->connection_used == cc->connection_size) {
                cc->connection_size *= 2;
                cc->connections = (_cluster_connection_t *)malloc(sizeof(_cluster_connection_t)*(cc->connection_size));
            }
            cc->connection_used++;
            next_connection = cc->connections + connection_idx;
            next_connection->ip = ipint;
            next_connection->port = port;
            next_connection->fd = -2;
        }
        if (cc->slotrange_used == cc->slotrange_size) {
            if (cc->slotrange_used == cc->slotrange_size) {
                cc->slotrange_size *= 2;
                cc->slotranges = (_cluster_slotrange_t *)malloc(sizeof(_cluster_slotrange_t)*(cc->slotrange_size));
            }
        }
        if (cc->slotrange_used - (slotrange_idx+1) > 0) {
            memmove(cc->slotranges + slotrange_idx, cc->slotranges + slotrange_idx + 1, sizeof(_cluster_slotrange_t) *(cc->slotrange_used-slotrange_idx-1));
        }
        next_slotrange = cc->slotranges + slotrange_idx+1;
        next_slotrange->connection_offset = connection_idx;
        next_slotrange->end = slotrange->end;
        next_slotrange->start = slot_id;
        slotrange->end = slot_id-1;
        cc->slotrange_used++;
        return 0;
    } else if ((*ps == 'C') &&(!strncmp(ps, "CLUSTERDOWN ", 12))) {
        strcpy(rc->error_msg, "-CLUSTERDOWN");
        return -1;
    } else if ((*ps == 'A') &&(!strncmp(ps, "ASK ", 4))) {
        strcpy(rc->error_msg, "system");
        return -1;
    }
    return -1;
}

static int zredis_client_cluster_vget_inner(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, zvector_t *query_vec)
{
    zredis_client_cluster_t *cc = (zredis_client_cluster_t *)rc;
    short int slot_id = _cluster_get_slod_id(rc, query_vec);
    int ret;

    for (int loop = 0; loop < 5; loop++) {
        _cluster_connection_t *connection = _cluster_vget_inner_prepare(rc, query_vec, slot_id);
        if (connection == 0) {
            return -1;
        }
        ret = ___query_by_io(number_ret, string_ret, vector_ret, json_ret, query_vec, rc->cmd_timeout, rc->error_msg, cc->fp);
        if ((zstream_get_read_cache_len(cc->fp) == 0) || (ret == -2)) {
            zstream_close(cc->fp, 0);
            cc->fp = 0;
        }
        if (ret == -2) {
            zclose(connection->fd);
            connection->fd = -1;
            return -1;
        }
        if (ret > -1) {
            return ret;
        }
        ret = _cluster_after_query(rc, slot_id);
        if (ret == -1) {
            return -1;
        }
    }
    strcpy(rc->error_msg, "too many MOVED");
    return -1;
}

static void zredis_client_free_cluster_inner(zredis_client_t *rc)
{
    zredis_client_cluster_t *cc = (zredis_client_cluster_t *)rc;
    zfree(rc->password);
    zfree(rc->destination);

    if (cc->fp) {
        zstream_close(cc->fp, 0);
    }
    for (short int i=0;i<cc->connection_used;i++) {
        if (cc->connections[i].fd > -1) {
            zclose(cc->connections[i].fd);
        }
    }
    zfree(cc->connections);
    zfree(cc->slotranges);

    zfree(rc);
}

/* }}} */

/* {{{ client ############################################################# */
void zredis_client_free(zredis_client_t *rc)
{
    if (!rc) {
        return;
    }
    if (rc->cluster_mode) {
        zredis_client_free_cluster_inner(rc);
    } else {
        zredis_client_free_standalone_inner(rc);
    }
}

void zredis_client_set_cmd_timeout(zredis_client_t *rc, int cmd_timeout)
{
    rc->cmd_timeout = cmd_timeout;
}

void zredis_client_set_auto_reconnect(zredis_client_t *rc, zbool_t auto_reconnect)
{
    rc->auto_reconnect = auto_reconnect;
}

const char * zredis_client_get_error_msg(zredis_client_t *rc)
{
    return rc->error_msg;
}

static void ___build_query_vec_ptr(zvector_t *query_vec, const char *p, int len)
{
    if (len < 0 ){
        if (!p) {
            p = zblank_buffer;
            len = 0;
        } else {
            len = strlen(p);
        }
    }
    zbuf_t * bf = zbuf_create(len);
    zbuf_memcpy(bf, p, len);
    zvector_push(query_vec, bf);
}

static void ___build_query_vec_buf(zvector_t *query_vec, zbuf_t *_bf)
{
    zbuf_t * bf = zbuf_create(zbuf_len(_bf));
    zbuf_memcpy(bf, zbuf_data(_bf), zbuf_len(_bf));
    zvector_push(query_vec, bf);
}

static void ___build_query_vec(zvector_t *query_vec, const char *redis_fmt, va_list ap)
{
    if (redis_fmt == 0) {
        return;
    }
    const char *fmt = redis_fmt;
    int ch, i;
    char nbuf[256];
    zlist_t *L;
    zlist_node_t *LN;
    zvector_t *V;
    zargv_t *A;
    char **P, *p;
    (void)p;

    while((ch = *fmt++)) {
        switch(ch) {
        case 's':
            ___build_query_vec_ptr(query_vec, va_arg(ap, char *), -1);
            break;
        case 'S':
            ___build_query_vec_buf(query_vec, va_arg(ap, zbuf_t *));
            break;
        case 'd':
            sprintf(nbuf, "%d", va_arg(ap, int));
            ___build_query_vec_ptr(query_vec, nbuf, -1);
            break;
        case 'l':
            sprintf(nbuf, "%ld", va_arg(ap, long));
            ___build_query_vec_ptr(query_vec, nbuf, -1);
            break;
        case 'f':
            sprintf(nbuf, "%lf", va_arg(ap, double));
            ___build_query_vec_ptr(query_vec, nbuf, -1);
            break;
        case 'L':
            L = va_arg(ap, zlist_t *);
            if (L) {
                for (LN=zlist_head(L); LN; LN=zlist_node_next(LN)) {
                    ___build_query_vec_buf(query_vec, (zbuf_t *)zlist_node_value(LN));
                }
            }
            break;
        case 'V':
            V = va_arg(ap, zvector_t *);
            if (V) {
                for (i=0;i<zvector_len(V);i++) {
                    ___build_query_vec_buf(query_vec, (zbuf_t *)zvector_data(V)[i]);
                }
            }
            break;
        case 'A':
            A = va_arg(ap, zargv_t *);
            if (A) {
                for (i=0;i<zargv_len(A);i++) {
                    ___build_query_vec_ptr(query_vec, zargv_data(A)[i], -1);
                }
            }
            break;
        case 'P':
            P = va_arg(ap, char **);
            if (P) {
                for (;*P;P++) {
                    ___build_query_vec_ptr(query_vec, *P, -1);
                }
            }
            break;
        }
    }
}

int zredis_client_vget(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, const char *redis_fmt, va_list ap)
{
    if (!rc) {
        return -1;
    } 
    zvector_t *query_vec = zvector_create(16);
    ___build_query_vec(query_vec, redis_fmt, ap);
    int ret;
    if (rc->cluster_mode) {
        ret = zredis_client_cluster_vget_inner(rc, number_ret, string_ret, vector_ret, json_ret, query_vec);
    } else {
        ret = zredis_client_standalone_vget_inner(rc, number_ret, string_ret, vector_ret, json_ret, query_vec);
    }
    zbuf_vector_free(query_vec);
    return ret;
}

#define zredis_client_get_macro(n, s, v, j) \
    if (!rc) { return -1; } \
    int ret;\
    va_list ap; \
    va_start(ap, redis_fmt); \
    ret = zredis_client_vget(rc, n, s, v, j, redis_fmt, ap); \
    va_end(ap); \
    return ret;

int zredis_client_get_success(zredis_client_t *rc, const char *redis_fmt, ...)
{
    zredis_client_get_macro(0, 0, 0, 0);
}

int zredis_client_get_long(zredis_client_t *rc, long *number_ret, const char *redis_fmt, ...)
{
    zredis_client_get_macro(number_ret, 0, 0, 0);
}

int zredis_client_get_string(zredis_client_t *rc, zbuf_t *string_ret, const char *redis_fmt, ...)
{
    zredis_client_get_macro(0, string_ret, 0, 0);
}

int zredis_client_get_vector(zredis_client_t *rc, zvector_t *vector_ret, const char *redis_fmt, ...)
{
    zredis_client_get_macro(0, 0, vector_ret, 0);
}

int zredis_client_get_json(zredis_client_t *rc, zjson_t *json_ret, const char *redis_fmt, ...)
{
    zredis_client_get_macro(0, 0, 0, json_ret);
}
#undef zredis_client_get_macro

int zredis_client_subscribe(zredis_client_t *rc, const char *redis_fmt, ...)
{
    if (!rc) {
        return -1;
    }
    int ret;
    zvector_t *query_vec = zvector_create(-1);
    ___build_query_vec_ptr(query_vec, "SUBSCRIBE", 9);

    va_list ap;
    va_start(ap, redis_fmt);
    ___build_query_vec(query_vec, redis_fmt, ap);
    va_end(ap);

    if (rc->cluster_mode) {
        ret = zredis_client_cluster_vget_inner(rc, (long *)-1, 0, 0, 0, query_vec);
    } else {
        ret = zredis_client_standalone_vget_inner(rc, (long *)-1, 0, 0, 0, query_vec);
    }
    zbuf_vector_free(query_vec);
    return ret;
}

int zredis_client_psubscribe(zredis_client_t *rc, const char *redis_fmt, ...)
{
    if (!rc) {
        return -1;
    }
    int ret;
    zvector_t *query_vec = zvector_create(-1);
    ___build_query_vec_ptr(query_vec, "PSUBSCRIBE", 10);

    va_list ap;
    va_start(ap, redis_fmt);
    ___build_query_vec(query_vec, redis_fmt, ap);
    va_end(ap);

    if (rc->cluster_mode) {
        ret = zredis_client_cluster_vget_inner(rc, (long *)-1, 0, 0, 0, query_vec);
    } else {
        ret = zredis_client_standalone_vget_inner(rc, (long *)-1, 0, 0, 0, query_vec);
    }
    zbuf_vector_free(query_vec);
    return ret;
}

int zredis_client_fetch_channel_message(zredis_client_t *rc, zvector_t *vector_ret)
{
    return zredis_client_get_vector(rc, vector_ret, 0);
}

int zredis_client_scan(zredis_client_t *rc, zvector_t *vector_ret, long *cursor_ret, const char *redis_fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, redis_fmt);
    ret = zredis_client_vget(rc, 0, 0, vector_ret, 0, redis_fmt, ap);
    va_end(ap);
    if (!vector_ret) {
        return ret;
    }
    if (ret > 0) {
        if (!zvector_len(vector_ret)) {
            return -1;
        }
        zbuf_t *bf;
        zvector_shift(vector_ret, (void **)&bf);
        if (cursor_ret) {
            *cursor_ret = atol(zbuf_data(bf));
        }
    }
    return ret;
}

int zredis_client_get_info_dict(zredis_client_t *rc, zdict_t *info)
{
    zdict_reset(info);
    zbuf_t *bf = zbuf_create(1024);
    int ret =  zredis_client_get_string(rc, bf, "s", "INFO");
    if (ret < 1) {
        zbuf_free(bf);
        return ret;
    }
    char *ps, *p = zbuf_data(bf);
    char *tmpn, *tmpv;
    while(*p) {
        if ((*p == '#') || (*p == ' ') || (*p == '\r')) {
            p++;
            for (;*p;p++) {
                if (*p == '\n') {
                    break;
                }
            }
            if (*p == '\n') {
                p++;
            }
            continue;
        }
        ps = p;
        for (;*p;p++) {
            if (*p == ':') {
                break;
            }
        }
        if (*p == 0) {
            break;
        }
        tmpn = 0;
        if (p > ps) {
            tmpn = ps;
            tmpn[p-ps] = 0;
        }
        p++;
        ps = p;

        for (;*p;p++) {
            if (*p == '\n') {
                break;
            }
        }
        if (*p == 0) {
            break;
        }
        tmpv = zblank_buffer;
        if (p - ps > 1) {
            tmpv = ps;
            tmpv[p-ps-1] = 0;
        }
        if (!zempty(tmpn)) {
            zdict_update_string(info, tmpn, tmpv, -1);
        }
        p++;
    }
    zbuf_free(bf);
    return ret;
}
/* }}} */

/*
 * vim600: fdm=marker
 */
