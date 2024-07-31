/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "./redis.h"

zcc_namespace_begin;

static const short int slot_count = 16384;

// A: 应该在 standalone 模式的服务器执行, 报错
// B: 在集群模式下几乎不可能实现, 报错
// a-z: 表示计算slot的键所在位置
static const std::map<std::string, char> cmd_attrs{
    {"APPEND", 'b'},
    {"AUTH", 'A'},
    {"BGREWRITEAOF", 'A'},
    {"BGSAVE", 'A'},
    {"BITCOUNT", 'b'},
    {"BITOP", 'c'},
    {"BLPOP", 'b'},
    {"BRPOP", 'b'},
    {"BRPOPLPUSH", 'b'},
    {"CLIENT", 'A'},
    {"CONFIG", 'A'},
    {"DBSIZE", 'A'},
    {"DEBUG", 'A'},
    {"DECR", 'b'},
    {"DECRBY", 'b'},
    {"DEL", 'b'},
    {"DISCARD", 'B'},
    {"DUMP", 'b'},
    {"ECHO", 'A'},
    {"EVAL", 'A'},
    {"EVALSHA", 'A'},
    {"EXEC", 'A'},
    {"EXISTS", 'b'},
    {"EXPIREAT", 'b'},
    {"EXPIRE", 'b'},
    {"FLUSHALL", 'A'},
    {"FLUSHDB", 'A'},
    {"GET", 'b'},
    {"GETBIT", 'b'},
    {"GETRANGE", 'b'},
    {"GETSET", 'b'},
    {"HDEL", 'b'},
    {"HEXISTS", 'b'},
    {"HGETALL", 'b'},
    {"HGET", 'b'},
    {"HINCRBY", 'b'},
    {"HINCRBYFLOAT", 'b'},
    {"HKEYS", 'b'},
    {"HLEN", 'b'},
    {"HMGET", 'b'},
    {"HMSET", 'b'},
    {"HSCAN", 'b'},
    {"HSET", 'b'},
    {"HSETNX", 'b'},
    {"HVALS", 'b'},
    {"INCR", 'b'},
    {"INCRBY", 'b'},
    {"INCRBYFLOAT", 'b'},
    {"INFO", 'A'},
    {"KEYS", 'A'},
    {"LASTSAVE", 'A'},
    {"LINDEX", 'b'},
    {"LINSERT", 'b'},
    {"LLEN", 'b'},
    {"LPOP", 'b'},
    {"LPUSH", 'b'},
    {"LPUSHX", 'b'},
    {"LRANGE", 'b'},
    {"LREM", 'b'},
    {"LSET", 'b'},
    {"LTRIM", 'b'},
    {"MGET", 'b'},
    {"MIGRATE", 'd'},
    {"MONITOR", 'b'},
    {"MOVE", 'b'},
    {"MSET", 'b'},
    {"MSETNX", 'b'},
    {"MULTI", 'A'},
    {"OBJECT", 'A'},
    {"PERSIST", 'b'},
    {"PEXPIREAT", 'b'},
    {"PEXPIRE", 'b'},
    {"PING", 'A'},
    {"PSETEX", 'b'},
    {"PSUBSCRIBE", 'A'},
    {"PSYNC", 'A'},
    {"PTTL", 'b'},
    {"PUBLISH", 'A'},
    {"PUBSUB", 'A'},
    {"PUNSUBSCRIBE", 'A'},
    {"QUIT", 'A'},
    {"RANDOMKEY", 'A'},
    {"RENAME", 'b'},
    {"RENAMENX", 'b'},
    {"RESTORE", 'b'},
    {"RPOP", 'b'},
    {"RPOPLPUSH", 'b'},
    {"RPUSH", 'b'},
    {"RPUSHX", 'b'},
    {"SADD", 'b'},
    {"SAVE", 'A'},
    {"SCAN", 'A'},
    {"SCARD", 'b'},
    {"SCRIPT", 'A'},
    {"SDIFF", 'b'},
    {"SDIFFSTORE", 'b'},
    {"SELECT", 'B'},
    {"SET", 'b'},
    {"SETBIT", 'b'},
    {"SETEX", 'b'},
    {"SETNX", 'b'},
    {"SETRANGE", 'b'},
    {"SHUTDOWN", 'A'},
    {"SINTER", 'b'},
    {"SINTERSTORE", 'b'},
    {"SISMEMBER", 'b'},
    {"SLAVEOF", 'A'},
    {"SLOWLOG", 'A'},
    {"SMEMBERS", 'b'},
    {"SMOVE", 'b'},
    {"SORT", 'b'},
    {"SPOP", 'b'},
    {"SRANDMEMBER", 'b'},
    {"SREM", 'b'},
    {"SSCAN", 'b'},
    {"STRLEN", 'b'},
    {"SUBSCRIBE", 'A'},
    {"SUNION", 'b'},
    {"SUNIONSTORE", 'b'},
    {"SYNC", 'A'},
    {"TIME", 'A'},
    {"TTL", 'b'},
    {"TYPE", 'b'},
    {"UNSUBSCRIBE", 'A'},
    {"UNWATCH", 'B'},
    {"WATCH", 'b'},
    {"ZADD", 'b'},
    {"ZCARD", 'b'},
    {"ZCOUNT", 'b'},
    {"ZINCRBY", 'b'},
    {"ZINTERSTORE", 'b'},
    {"ZRANGE", 'b'},
    {"ZRANGEBYSCORE", 'b'},
    {"ZRANK", 'b'},
    {"ZREM", 'b'},
    {"ZREMRANGEBYRANK", 'b'},
    {"ZREMRANGEBYSCORE", 'b'},
    {"ZREVRANGE", 'b'},
    {"ZREVRANGEBYSCORE", 'b'},
    {"ZREVRANK", 'b'},
    {"ZSCAN", 'b'},
    {"ZSCORE", 'b'},
    {"ZUNIONSTORE", 'b'},
};

struct connection_info_node
{
    std::string destination;
    iostream fp;
    int used{0};
    short int idx{-1};
    bool connected_{false};
};

class redis_client_cluster_engine : public redis_client_basic_engine
{
public:
    redis_client_cluster_engine();
    ~redis_client_cluster_engine();
    void close();
    int open();
    int query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens);
    int query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens, connection_info_node &node);
    void set_timeout(int wait_timeout);
    int timed_read_wait(int wait_timeout);
    int timed_write_wait(int wait_timeout);
    void set_auto_reconnect(bool tf = true);
    int connect_node(connection_info_node &node);
    int find_node_by_query_tokens(const std::list<std::string> &query_tokens, connection_info_node *&node);
    connection_info_node *find_node_random();
    int deal_moved(connection_info_node *&node);
    void clear_connections();
    int do_auth(connection_info_node &node);

public:
    std::map<std::string, connection_info_node *> connection_info_map_;
    std::vector<connection_info_node *> connection_info_vector_;
    short int slot_vector_[slot_count];
    int node_idx_plus_{0};
};

redis_client_cluster_engine::redis_client_cluster_engine()
{
    for (int i = 0; i < slot_count; i++)
    {
        slot_vector_[i] = -1;
    }
}

redis_client_cluster_engine::~redis_client_cluster_engine()
{
    close();
}

connection_info_node *redis_client_cluster_engine::find_node_random()
{
    connection_info_node *node = nullptr;
    node_idx_plus_++;
    for (size_t i = 0; i < connection_info_vector_.size(); i++)
    {
        int idx = (i + node_idx_plus_) % connection_info_vector_.size();
        node = connection_info_vector_[idx];
        if (node)
        {
            break;
        }
    }
    if (!node)
    {
        if (open() > 0)
        {
            node = connection_info_vector_[0];
        }
    }
    return node;
}

int redis_client_cluster_engine::find_node_by_query_tokens(const std::list<std::string> &query_tokens, connection_info_node *&node)
{
    node = nullptr;
    int count = query_tokens.size();
    if (count < 1)
    {
        return redis_fatal;
    }
    std::string key = query_tokens.front();
    toupper(key);
    auto it = cmd_attrs.find(key);
    if (it == cmd_attrs.end())
    {
        if (!(node = find_node_random()))
        {
            return redis_fatal;
        }
        return redis_ok;
    }
    int ch = it->second;
    if (ch == 'A')
    {
        return redis_error;
    }
    if (ch == 'B')
    {
        return redis_error;
    }
    int offset = ch - 'a';
    auto it2 = query_tokens.begin();
    for (int i = 0; i < offset; i++)
    {
        it2++;
    }
    if (it2 == query_tokens.end())
    {
        return redis_error;
    }

    int slot_id = crc16(it2->c_str(), it2->size(), 0) % slot_count;
    int idx = slot_vector_[slot_id];
    if (idx == -1)
    {
        return redis_error;
    }
    node = connection_info_vector_[idx];
    if (!node)
    {
        node = find_node_random();
    }
    if (!node)
    {
        return redis_fatal;
    }
    return redis_ok;
}

void redis_client_cluster_engine::clear_connections()
{
    for (auto it = connection_info_vector_.begin(); it != connection_info_vector_.end(); it++)
    {
        auto info = *it;
        if ((!info) || (info->used))
        {
            continue;
        }
        connection_info_map_.erase(info->destination);
        delete info;
        *it = nullptr;
    }
}

int redis_client_cluster_engine::deal_moved(connection_info_node *&node)
{
    const char *ps = info_msg_.c_str();
    int slot_id = -1;
    std::string dest;
    ps += 7;
    const char *p = std::strchr(ps, ' ');
    if (!p)
    {
        return redis_fatal;
    }
    slot_id = std::atoi(ps);
    if ((slot_id < 0) || (slot_id >= slot_count))
    {
        return redis_fatal;
    }
    ps = p + 1;
    dest = ps;
    trim_line_end_rn(dest);

    node = nullptr;
    auto it = connection_info_map_.find(dest);
    if (it == connection_info_map_.end())
    {
        node = new connection_info_node();
        node->used = 0;
        node->destination = dest;
        connection_info_map_[node->destination] = node;
        bool have_space = false;
        for (auto it = connection_info_vector_.begin(); it != connection_info_vector_.end(); it++)
        {
            node->idx++;
            if (*it == nullptr)
            {
                have_space = true;
                *it = node;
                break;
            }
        }
        if (!have_space)
        {
            node->idx = connection_info_vector_.size();
            connection_info_vector_.push_back(node);
        }
    }
    else
    {
        node = it->second;
    }

    bool need_clear = false;
    if (slot_vector_[slot_id] > -1)
    {
        if (connection_info_vector_[slot_vector_[slot_id]])
        {
            connection_info_vector_[slot_vector_[slot_id]]->used--;
            connection_info_vector_[slot_vector_[slot_id]] = nullptr;
            need_clear = true;
        }
    }
    node->used++;
    slot_vector_[slot_id] = node->idx;
    if (need_clear)
    {
        clear_connections();
    }
    return redis_ok;
}

int redis_client_cluster_engine::query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens)
{
    connection_info_node *node;
    int ret;
    ret = find_node_by_query_tokens(query_tokens, node);
    if (ret < 1)
    {
        info_msg_ = "protocol error or not supported in cluster mode";
        return redis_error;
    }
    for (int move_try_times = 0; move_try_times < 5; move_try_times++)
    {
        ret = query_protocol(number_ret, string_ret, list_ret, json_ret, query_tokens, *node);
        if (ret > -1)
        {
            return ret;
        }
        if (ret == redis_fatal)
        {
            continue;
        }

        const char *ps = info_msg_.c_str();
        if (ps[0] != '-')
        {
            return redis_fatal;
        }
        ps++;
        if ((*ps == 'C') && (!strncmp(ps, "CLUSTERDOWN ", 12)))
        {
            return redis_error;
        }
        if ((*ps == 'A') && (!strncmp(ps, "ASK ", 4)))
        {
            info_msg_ = "system";
            return redis_error;
        }

        if ((*ps == 'M') && (!std::strncmp(ps, "MOVED ", 6)))
        {
            node = nullptr;
            if (deal_moved(node) < 1)
            {
                return redis_fatal;
            }
            continue;
        }
        return redis_error;
    }
    return redis_error;
}

int redis_client_cluster_engine::query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens, connection_info_node &node)
{
    int ret = redis_fatal;
    int times = 1;
    if (auto_reconnect_)
    {
        times = 3;
    }
    for (int i = 0; i < times; i++)
    {
        if (!node.connected_)
        {
            if (connect_node(node) < redis_ok)
            {
                continue;
            }
        }
        ret = query_protocol_by_stream(number_ret, string_ret, list_ret, json_ret, query_tokens, node.fp);
        if (ret == redis_fatal)
        {
            node.fp.close();
            node.connected_ = false;
            continue;
        }
        break;
    }
    return ret;
}

int redis_client_cluster_engine::do_auth(connection_info_node &node)
{
    if (password_.empty())
    {
        return 1;
    }
    int ret = query_protocol(0, 0, 0, 0, {"auth", password_}, node);
    if (ret == 0)
    {
        return 0;
    }
    if (ret < 0)
    {
        if (!std::strncmp(info_msg_.c_str(), "-err ", 5))
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    return 1;
}

int redis_client_cluster_engine::connect_node(connection_info_node &node)
{
    if (node.connected_)
    {
        return redis_ok;
    }
    if (!node.fp.connect(node.destination))
    {
        return redis_fatal;
    }
    int ret = do_auth(node);
    if (ret < 1)
    {
        node.fp.close();
        return ret;
    }
    node.connected_ = true;
    return 1;
}

int redis_client_cluster_engine::open()
{
    close();
    connection_info_node *cnode = new connection_info_node();
    int host, port;
    if (!cnode->fp.connect(destination_))
    {
        delete cnode;
        return redis_fatal;
    }
    if (get_peername(cnode->fp.get_socket(), &host, &port) < 1)
    {
        cnode->destination = "default";
    }
    else
    {
        cnode->destination.append(get_ipstring(host)).append(":").append(std::to_string(port));
    }
    int ret = do_auth(*cnode);
    if (ret < 1)
    {
        delete cnode;
        return ret;
    }
    cnode->connected_ = true;
    cnode->used = 1;
    connection_info_map_[cnode->destination] = cnode;
    cnode->idx = 0;
    connection_info_vector_.push_back(cnode);

    return redis_ok;
}

void redis_client_cluster_engine::close()
{
    for (auto it = connection_info_map_.begin(); it != connection_info_map_.end(); it++)
    {
        delete it->second;
    }
    connection_info_map_.clear();
    connection_info_vector_.clear();
    for (int i = 0; i < slot_count; i++)
    {
        slot_vector_[i] = -1;
    }
}

void redis_client_cluster_engine::set_timeout(int wait_timeout)
{
    wait_timeout_ = wait_timeout;
    for (auto it = connection_info_map_.begin(); it != connection_info_map_.end(); it++)
    {
        it->second->fp.set_timeout(wait_timeout);
    }
}

int redis_client_cluster_engine::timed_read_wait(int wait_timeout)
{
    return 1;
}

int redis_client_cluster_engine::timed_write_wait(int wait_timeout)
{
    return 1;
}

void redis_client_cluster_engine::set_auto_reconnect(bool tf)
{
    auto_reconnect_ = tf;
}

redis_client_basic_engine *redis_client_cluster_engine_create()
{
    redis_client_cluster_engine *engine = new redis_client_cluster_engine();
    return engine;
}

zcc_namespace_end;
