/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-01-22
 * ================================
 */

#include "zc.h"
#include <errno.h>

/* {{{ structure */
typedef struct main_db_t main_db_t;
typedef struct main_node_t main_node_t;
typedef struct hash_node_t hash_node_t;
typedef struct set_node_t set_node_t;
enum node_type_t {
    node_type_init = 0,
    node_type_string,
    node_type_integer,
    node_type_hash,
    node_type_set,
};
typedef enum  node_type_t node_type_t;

struct main_db_t {
    zrbtree_t key_tree;
    zrbtree_t timeout_tree;
    int count;
};

struct main_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    union {
        char *ptr;
        char str[sizeof(char *)];
        long num;
        zrbtree_t *hash_tree;
        zrbtree_t *set_tree;
    } val;
    unsigned int val_len;
    zrbtree_node_t rbkey;
    long timeout;
    zrbtree_node_t rbtimeout;
};

static int main_node_cmp_key(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    main_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = ZCONTAINER_OF(n1, main_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = ZCONTAINER_OF(n2, main_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

static int main_node_cmp_timeout(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    main_node_t *t1, *t2;
    long r;
    t1 = ZCONTAINER_OF(n1, main_node_t, rbkey);
    t2 = ZCONTAINER_OF(n2, main_node_t, rbkey);
    r = t1->timeout - t2->timeout;
    if (r==0) {
        r = (int)(n1-n2);
    }
    return r;
}

struct hash_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    union {
        char *ptr;
        char str[sizeof(char *)];
        long num;
    } val;
    unsigned int val_len;
    zrbtree_node_t rbkey;
};

static int hash_node_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    hash_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = ZCONTAINER_OF(n1, hash_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = ZCONTAINER_OF(n2, hash_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

struct set_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    zrbtree_node_t rbkey;
};

static int set_node_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    set_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = ZCONTAINER_OF(n1, set_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = ZCONTAINER_OF(n2, set_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

typedef struct connection_context_t connection_context_t;
struct connection_context_t {
    zstream_t *fp;
    main_db_t *current_db;
    char quit;
};
typedef void (*do_cmd_fn_t)(connection_context_t *context, zvector_t *cmd_vector);

/* }}} */

/* {{{ main node */
main_db_t default_db;

static void main_node_set_key(main_node_t *node, zbuf_t *key)
{
    char *ptr = zbuf_data(key);
    int klen = zbuf_len(key);
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            node->key.str[i] = ptr[i];
        }
    } else {
        node->key.ptr = (char *)zmemdup(ptr, klen);
    }
    node->key_len = klen;
}

static void main_node_get_key(main_node_t *node, zbuf_t *key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        zbuf_memcat(key, node->key.str, node->key_len);
    } else {
        zbuf_memcat(key, node->key.ptr, node->key_len);
    }
}

static void main_node_set_value(main_node_t *node, zbuf_t *val)
{
    char *ptr = zbuf_data(val);
    int vlen = zbuf_len(val);
    if (vlen <= (int)sizeof(char *)) {
        for (int i=0; i<vlen; i++) {
            node->val.str[i] = ptr[i];
        }
    } else {
        node->val.ptr = (char *)zmemdup(ptr, vlen);
    }
    node->val_len = vlen;
    node->type = node_type_string;
}

static void main_node_clear_value(main_node_t *node)
{
    if (node->type == node_type_init) {
    } else if (node->type == node_type_string){
        if (node->val_len > sizeof(char *)) {
            zfree(node->val.ptr);
        }
    } else if (node->type == node_type_integer){
    } else if (node->type == node_type_hash){
        zrbtree_t *tree = node->val.hash_tree;
        while (tree && zrbtree_have_data(tree)) {
            zrbtree_node_t *n = zrbtree_first(tree);
            hash_node_t *hn = ZCONTAINER_OF(n, hash_node_t, rbkey);
            zrbtree_detach(tree, n);
            if (hn->key_len > (int)sizeof(char *)) {
                zfree(hn->key.ptr);
            }
            if (hn->type == node_type_init) {
            } else if (hn->type == node_type_string) {
                if (hn->val_len > (int)sizeof(char *)) {
                    zfree(hn->val.ptr);
                }
            } else if (hn->type == node_type_integer) {
            }
            zfree(hn);
        }
        zfree(tree);
    }
    node->type = node_type_init;
    node->val_len = 0;
    node->val.ptr = 0;
}

static void main_node_get_value(main_node_t *node, zbuf_t *val)
{
    if (node->type == node_type_string) {
        int vlen = node->val_len;
        if (vlen <= (int)sizeof(char *)) {
            zbuf_memcat(val, node->val.str, vlen);
        } else {
            zbuf_memcat(val, node->val.ptr, vlen);
        }
    } else if (node->type == node_type_integer) {
        char buf[128];
        zsprintf(buf, "%ld", node->val.num);
        zbuf_strcat(val, buf);
    }
}

static main_node_t *main_node_create(main_db_t *db, zbuf_t *key)
{
    main_node_t *node = (main_node_t *)zcalloc(1, sizeof(main_node_t));
    node->type = node_type_init;
    main_node_set_key(node, key);
    node->val_len = 0;
    zrbtree_attach(&(db->key_tree), &(node->rbkey));
    db->count ++;
    return node;
}

static main_node_t *main_node_find(main_db_t *db, zbuf_t *k)
{
    char *ptr = zbuf_data(k);
    int klen = zbuf_len(k);

    main_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    zrbtree_node_t *rnp = zrbtree_find(&(db->key_tree), &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return ZCONTAINER_OF(rnp, main_node_t, rbkey);
}

static void main_node_free(main_db_t *db, main_node_t *node)
{
    zrbtree_detach(&(db->key_tree), &(node->rbkey));
    db->count --;
    if (node->key_len > (int)sizeof(char *)) {
        zfree(node->key.ptr);
    }
    main_node_clear_value(node);
    zfree(node);
}

#define RETURN_WRONG_VALUE_TYPE() \
    zstream_write(context->fp, "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n", 68); return;

#define RETURN_WRONG_NUMBER_ARGUMENTS(c) \
    zstream_write(context->fp, "-ERROR wrong number of arguments for '", 36); \
    zstream_write(context->fp, c, sizeof(c)-1); \
    zstream_write(context->fp, "' command\r\n", 11); return;

#define RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE() \
    zstream_write(context->fp, "-ERROR value is not an integer or out of range\r\n", 46); return;

static zbool_t get_check_long_integer(const char *str, long *r)
{
    long l = atoll(str);
    char buf[128];
    zsprintf(buf, "%ld", l);
    if (r) {
        *r = l;
    }
    if (!strcmp(str, buf)) {
        return 1;
    }
    return 0;
}

static void do_cmd_unkonwn(connection_context_t *context, zvector_t *cmd_vector)
{
    zstream_puts(context->fp, "-ERROR unknown command '");
    zstream_puts(context->fp, zstr_tolower(zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[0]))));
    zstream_puts(context->fp, "\r\n");
}

static void do_cmd_flushdb(connection_context_t *context, zvector_t *cmd_vector)
{
    while(1) {
        zrbtree_node_t *rbn = zrbtree_first(&(context->current_db->key_tree));
        if (!rbn) {
            break;
        }
        main_node_t *node = ZCONTAINER_OF(rbn, main_node_t, rbkey);
        if (node->timeout != -1) {
            zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
        }
        main_node_free(context->current_db, node);
    }
    zstream_write(context->fp, "+OK\r\n", 5);
}

static void do_cmd_flushall(connection_context_t *context, zvector_t *cmd_vector)
{
    do_cmd_flushdb(context, cmd_vector);
}

static void do_cmd_quit(connection_context_t *context, zvector_t *cmd_vector)
{
    context->quit = 1;
    zstream_puts(context->fp, "+OK\r\n");
}

static void do_cmd_dbsize(connection_context_t *context, zvector_t *cmd_vector)
{
    zstream_printf_1024(context->fp, ":%d\r\n", context->current_db->count);
}

static void do_cmd_ping(connection_context_t *context, zvector_t *cmd_vector)
{
    zstream_write(context->fp, "+PONG\r\n", 7);
}

static void do_cmd_del(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) < 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("del");
    }
    int first = 1;
    int rnum = 0;
    ZVECTOR_WALK_BEGIN(cmd_vector, zbuf_t *, bf) {
        if (first) {
            first = 0;
            continue;
        }
        main_node_t *node = main_node_find(context->current_db, bf);
        if (!node) {
            continue;
        }
        rnum++;
        if (node->timeout != -1) {
            zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
        }
        main_node_free(context->current_db, node);
    } ZVECTOR_WALK_END;
    zstream_printf_1024(context->fp, ":%d\r\n", rnum);
}

static void do_cmd_exists(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("exists");
    }
    zstream_printf_1024(context->fp, ":%d\r\n", main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]))?1:0);
}

static void do_cmd_expire(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("expire");
    }
    int rnum = 0;
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    long expire;
    if (!get_check_long_integer(zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[2])), &expire)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        rnum = 0;
    } else {
        rnum = 1;
        if (expire < 1) {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
            }
            main_node_free(context->current_db, node);
        } else {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = ztimeout_set_millisecond(expire * 1000);
                zrbtree_attach(&(context->current_db->timeout_tree), &(node->rbtimeout));
            }
        }
    }
    zstream_printf_1024(context->fp, ":%d\r\n", rnum);
}

static void do_cmd_pexpire(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("pexpire");
    }
    int rnum = 0;
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    long expire;
    if (!get_check_long_integer(zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[2])), &expire)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        rnum = 0;
    } else {
        rnum = 1;
        if (expire < 1) {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
            }
            main_node_free(context->current_db, node);
        } else {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = ztimeout_set_millisecond(expire);
                zrbtree_attach(&(context->current_db->timeout_tree), &(node->rbtimeout));
            }
        }
    }
    zstream_printf_1024(context->fp, ":%d\r\n", rnum);
}

static void do_cmd_keys(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("keys");
    }
    /* 返回 全部 */
    int rnum = 0;
    zbuf_t *result = zbuf_create(1024);
    for (zrbtree_node_t *rbn = zrbtree_first(&(context->current_db->key_tree)); rbn; rbn = zrbtree_next(rbn)) {
        main_node_t * node = ZCONTAINER_OF(rbn, main_node_t, rbkey);
        zbuf_printf_1024(result, "$%d\r\n", node->key_len);
        main_node_get_key(node, result);
        zbuf_puts(result, "\r\n");
        rnum ++;
    }
    zstream_printf_1024(context->fp, "*%d\r\n", rnum);
    if (zbuf_len(result)) {
        zstream_write(context->fp, zbuf_data(result), zbuf_len(result));
    }
    zbuf_free(result);
}

static void do_cmd_ttl(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("ttl");
    }
    int r = 0;
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        r = -2;
    } else {
        if (node->timeout == -1) {
            r = -1;
        }
        r = ztimeout_left_millisecond(node->timeout)/1000;
        if (r < 1) {
            r = 1;
        }
    }
    zstream_printf_1024(context->fp, ":%d\r\n", r);
}

static void do_cmd_pttl(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("pttl");
    }
    long r = 0;
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        r = -2;
    } else {
        if (node->timeout == -1) {
            r = -1;
        }
        r = ztimeout_left_millisecond(node->timeout);
        if (r < 1) {
            r = 1;
        }
    }
    zstream_printf_1024(context->fp, ":%ld\r\n", r);
}

static void do_cmd_persist(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("persist");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if ((!node) || (node->timeout==-1)) {
        zstream_write(context->fp, ":0\r\n", 4);
    } else {
        zstream_write(context->fp, ":1\r\n", 4);
        zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
        node->timeout = -1;
    }
}

static void do_cmd_rename(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("rename");
    }
    if ((zbuf_t *)(zvector_data(cmd_vector)[1]) == (zbuf_t *)(zvector_data(cmd_vector)[2])) {
        zstream_puts(context->fp, "-ERROR source and destination objects are the same\r\n");
        return;
    }
    main_node_t *fn = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!fn) {
        zstream_puts(context->fp, "-ERROR no such key\r\n");
        return;
    }

    main_node_t *tn = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (tn) {
        if (tn->timeout != -1) {
            zrbtree_detach(&(context->current_db->timeout_tree), &(tn->rbtimeout));
        }
        main_node_free(context->current_db, tn);
    }

    zrbtree_detach(&(context->current_db->key_tree), &(fn->rbkey));
    if (fn->key_len > (int)sizeof(char *)) {
        zfree(fn->key.ptr);
    }
    main_node_set_key(fn, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    zrbtree_attach(&(context->current_db->key_tree), &(fn->rbkey));
    zstream_puts(context->fp, "+OK\r\n");
}

static void do_cmd_renamenx(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("rename");
    }
    if ((zbuf_t *)(zvector_data(cmd_vector)[1]) == (zbuf_t *)(zvector_data(cmd_vector)[2])) {
        zstream_puts(context->fp, "-ERROR source and destination objects are the same\r\n");
        return;
    }
    main_node_t *fn = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!fn) {
        zstream_puts(context->fp, "-ERROR no such key\r\n");
        return;
    }

    main_node_t *tn = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (tn) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }

    zrbtree_detach(&(context->current_db->key_tree), &(fn->rbkey));
    if (fn->key_len > (int)sizeof(char *)) {
        zfree(fn->key.ptr);
    }
    main_node_set_key(fn, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    zrbtree_attach(&(context->current_db->key_tree), &(fn->rbkey));
    zstream_write(context->fp, ":1\r\n", 4);
}
static void do_cmd_type(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("type");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    const char *r = "none";
    if (!node) {
        r = "none";
    } else if (node->type == node_type_string) {
        r = "string";
    } else if (node->type == node_type_integer) {
        r = "string";
    } else if (node->type == node_type_hash) {
        r = "hash";
    } else if (node->type == node_type_set) {
        r = "set";
    }
    zstream_printf_1024(context->fp, "+%s\r\n", r);
}

static void do_cmd_integer_count(connection_context_t *context, zbuf_t *key, int op, const char *incr)
{
    main_node_t *node = main_node_find(context->current_db, key);
    long num;

    if (!get_check_long_integer(incr, &num)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        node = main_node_create(context->current_db, key);
        node->type = node_type_integer;
        node->val.num = 0;
    } else {
        if (node->type ==node_type_integer) {
        } else if (node->type == node_type_string) {
            zbuf_t *k = zbuf_create(-1);
            long num;
            main_node_get_value(node, k);
            if (!get_check_long_integer(zbuf_data(k), &num)) {
                zbuf_free(k);
                RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
            }
            zbuf_free(k);
            node->type = node_type_integer;
            node->val.num = num;
        } else {
            RETURN_WRONG_VALUE_TYPE();
        }
    }
    if (op == '+') {
        node->val.num += atoll(incr);
    } else {
        node->val.num -= atoll(incr);
    }

    zstream_printf_1024(context->fp, ":%ld\r\n", node->val.num);
}

static void do_cmd_decr(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("decr");
    }
    return do_cmd_integer_count(context, (zbuf_t *)(zvector_data(cmd_vector)[1]), '-', "1");
}

static void do_cmd_decrby(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("decrby");
    }
    return do_cmd_integer_count(context, (zbuf_t *)(zvector_data(cmd_vector)[1]), '-', zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[2])));
}

static void do_cmd_incr(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("incr");
    }
    return do_cmd_integer_count(context, (zbuf_t *)(zvector_data(cmd_vector)[1]), '+', "1");
}

static void do_cmd_incrby(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("incrby");
    }
    return do_cmd_integer_count(context, (zbuf_t *)(zvector_data(cmd_vector)[1]), '+', zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[2])));
}

static void do_cmd_get(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("get");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (node == 0) {
        zstream_puts(context->fp, "$-1\r\n");
    } else if (node->type == node_type_integer || node->type == node_type_string) {
        zbuf_t *tmp = zbuf_create(-1);
        main_node_get_value(node, tmp);
        zstream_printf_1024(context->fp, "$%d\r\n", zbuf_len(tmp));
        zstream_write(context->fp, zbuf_data(tmp), zbuf_len(tmp));
        zstream_write(context->fp, "\r\n", 2);
        zbuf_free(tmp);
    } else {
        RETURN_WRONG_VALUE_TYPE();
    }
}
static void do_cmd_set(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if (pcount < 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("set");
    }
    int idx = 3;
    int have_ex = 0, have_px = 0, have_nx = 0, have_xx = 0;
    long epx = 1, epx_tmp;
    int syntax_error = 0, number_ok = 1;
    while (pcount > idx) {
        char *epnx = zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[idx]));
        if ((zbuf_len((zbuf_t *)(zvector_data(cmd_vector)[idx])) != 2) || (ztoupper(epnx[1]) != 'X')) {
            syntax_error = 1;
            break;
        }

        char ch1 = ztoupper(epnx[0]);
        if ((ch1 == 'E') || (ch1 == 'P')) {
            if (idx+1 == pcount) {
                syntax_error = 1;
                break;
            }
            number_ok=get_check_long_integer(zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[idx+1])), &epx_tmp);
            if (!number_ok) {
                break;
            }
            if (ch1 == 'E') {
                have_ex = 1;
                epx = epx_tmp * 1000;
            } else {
                have_px = 1;
                epx = epx_tmp;
            }
            idx += 2;
        } else if (ch1 == 'N'){
            have_nx = 1;
            idx += 1;
        } else if (ch1 == 'X'){
            have_xx = 1;
            idx += 1;
        } else {
            syntax_error = 1;
            break;
        }
    }
    if (have_ex && have_px) {
        syntax_error = 1;
    }
    if (syntax_error) {
        RETURN_WRONG_NUMBER_ARGUMENTS("set");
    }
    if (!number_ok) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }

    int gogogo = 0;
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (have_xx) {
        if (node) {
            gogogo = 1;
        }
    } else if (have_nx) {
        if (!node) {
            gogogo = 1;
        }
    } else {
        gogogo = 1;
    }

    if (gogogo) {
        if (node) {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = -1;
            }
            main_node_clear_value(node);
        } else {
            node = main_node_create(context->current_db, ((zbuf_t *)(zvector_data(cmd_vector)[1])));
        }
    } else if (!gogogo) {
        zstream_puts(context->fp, "$-1\r\n");
        return;
    }
    if (have_ex || have_px) {
        node->timeout = ztimeout_set_millisecond(epx);
        zrbtree_attach(&(context->current_db->timeout_tree), &(node->rbtimeout));
    }
    main_node_set_value(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    zstream_puts(context->fp, "+OK\r\n");
    return;
}

static void do_cmd_getset(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("getset");
    }
    int have_old = 0;
    zbuf_t *old_val = zbuf_create(-1);
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (node) {
        if ((node->type != node_type_integer) && (node->type != node_type_string)) {
            RETURN_WRONG_VALUE_TYPE();
        }
        if (node->timeout != -1) {
            zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
            node->timeout = -1;
        }
        main_node_get_value(node, old_val);
        main_node_clear_value(node);
        have_old = 1;
    } else {
        node = main_node_create(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    main_node_set_value(node, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (have_old) {
        zstream_printf_1024(context->fp, "$%d\r\n", zbuf_len(old_val));
        zstream_write(context->fp, zbuf_data(old_val), zbuf_len(old_val));
        zstream_puts(context->fp, "\r\n");
    } else {
        zstream_puts(context->fp, "$-1\r\n");
    }
    zbuf_free(old_val);
}

static void do_cmd_mget(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) < 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("mget");
    }
    zvector_t *result_list = zvector_create(-1);
    int first = 1;
    int count = 0;
    ZVECTOR_WALK_BEGIN(cmd_vector, zbuf_t *, key) {
        if (first) {
            first = 0;
            continue;
        }
        count ++;
        main_node_t *node = main_node_find(context->current_db, key);
        if (!node) {
            zvector_push(result_list, 0);
        } else if ((node->type == node_type_integer) || (node->type == node_type_string)) {
            zbuf_t *s = zbuf_create(-1);
            main_node_get_value(node, s);
            zvector_push(result_list, s);
        } else {
            zvector_push(result_list, 0);
        }
    } ZVECTOR_WALK_END;
    zstream_printf_1024(context->fp, "*%d\r\n", count);
    while(zvector_len(result_list)) {
        zbuf_t *s = 0;
        zvector_shift(result_list, (void **)&s);
        if (s) {
            zstream_printf_1024(context->fp, "$%d\r\n", zbuf_len(s));
            zstream_write(context->fp, zbuf_data(s), zbuf_len(s));
            zstream_write(context->fp, "\r\n", 2);
            zbuf_free(s);
        } else {
            zstream_puts(context->fp, "$-1\r\n");
        }
    }
    zvector_free(result_list);
}

static void do_cmd_mset(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3) || (pcount%2 != 1)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("mset");
    }

    for (int i = 1; i < zvector_len(cmd_vector); i+=2) {
        zbuf_t *key = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        zbuf_t *val = (zbuf_t *)(zvector_data(cmd_vector)[i+1]);
        main_node_t *node = main_node_find(context->current_db, key);
        if (node) {
            if (node->timeout != -1) {
                zrbtree_detach(&(context->current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = -1;
            }
            main_node_clear_value(node);
        } else {
            node = main_node_create(context->current_db, key);
        }
        main_node_set_value(node, val);
    }
    zstream_puts(context->fp, "+OK\r\n");
}

static void do_cmd_msetnx(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3) || (pcount%2 != 1)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("msetnx");
    }
    for (int i = 1; i < zvector_len(cmd_vector); i++) {
        zbuf_t *key = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        main_node_t *node = main_node_find(context->current_db, key);
        if (node) {
            zstream_puts(context->fp, ":0\r\n");
            return;
        }
    }
    for (int i = 1; i < zvector_len(cmd_vector); i+=2) {
        zbuf_t *key = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        zbuf_t *val = (zbuf_t *)(zvector_data(cmd_vector)[i+1]);
        main_node_t *node = main_node_create(context->current_db, key);
        main_node_set_value(node, val);
    }
    zstream_puts(context->fp, ":1\r\n");
}

/* }}} */

/* {{{ hash node */
static hash_node_t *hash_node_create(main_node_t *node, zbuf_t *key)
{
    hash_node_t *hnode = (hash_node_t *)zcalloc(1, sizeof(hash_node_t));
    hnode->type = node_type_init;
    char *ptr = zbuf_data(key);
    int klen = zbuf_len(key);
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            hnode->key.str[i] = ptr[i];
        }
    } else {
        hnode->key.ptr = (char *)zmemdup(ptr, klen);
    }
    hnode->key_len = klen;
    zrbtree_attach(node->val.hash_tree, &(hnode->rbkey));
    node->val_len++;
    return hnode;
}

static hash_node_t *hash_node_find(main_node_t *node, zbuf_t *k)
{
    char *ptr = zbuf_data(k);
    int klen = zbuf_len(k);

    hash_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    zrbtree_node_t *rnp = zrbtree_find(node->val.hash_tree, &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return ZCONTAINER_OF(rnp, hash_node_t, rbkey);
}

static void hash_node_set_value(hash_node_t *node, zbuf_t *val)
{
    char *ptr = zbuf_data(val);
    int vlen = zbuf_len(val);
    if (vlen <= (int)sizeof(char *)) {
        for (int i=0; i<vlen; i++) {
            node->val.str[i] = ptr[i];
        }
    } else {
        node->val.ptr = (char *)zmemdup(ptr, vlen);
    }
    node->val_len = vlen;
    node->type = node_type_string;
}

static void hash_node_get_key(hash_node_t *node, zbuf_t *key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        zbuf_memcat(key, node->key.str, node->key_len);
    } else {
        zbuf_memcat(key, node->key.ptr, node->key_len);
    }
}

static void hash_node_get_value(hash_node_t *node, zbuf_t *val)
{
    if (node->type == node_type_string) {
        int vlen = node->val_len;
        if (vlen <= (int)sizeof(char *)) {
            zbuf_memcat(val, node->val.str, vlen);
        } else {
            zbuf_memcat(val, node->val.ptr, vlen);
        }
    } else if (node->type == node_type_integer) {
        char buf[128];
        zsprintf(buf, "%ld", node->val.num);
        zbuf_strcat(val, buf);
    }
}

static void hash_node_clear_value(hash_node_t *node)
{
    if (node->type == node_type_init) {
    } else if (node->type == node_type_string){
        if (node->val_len > sizeof(char *)) {
            zfree(node->val.ptr);
        }
    } else if (node->type == node_type_integer){
    }
    node->type = node_type_init;
    node->val_len = -1;
    node->val.ptr = 0;
}
static void hash_node_free(main_node_t *node, hash_node_t *hnode)
{
    node->val_len--;
    zrbtree_detach(node->val.hash_tree, &(hnode->rbkey));
    if (hnode->key_len > (int)sizeof(char *)) {
        zfree(hnode->key.ptr);
    }
    hash_node_clear_value(hnode);
    zfree(hnode);
}


static main_node_t * main_node_create_with_hash_val(connection_context_t *context, zbuf_t *key)
{
    main_node_t * node = main_node_create(context->current_db, key);
    node->type = node_type_hash;
    node->val_len = 0;
    node->val.hash_tree = (zrbtree_t *)zcalloc(1, sizeof(zrbtree_t));
    zrbtree_init(node->val.hash_tree, hash_node_cmp);
    return node;
}

static void do_cmd_hdel(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hdel");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int dcount = 0;
    for (int i = 2; i < zvector_len(cmd_vector); i++) {
        zbuf_t *bf = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        hash_node_t *hnode = hash_node_find(node, bf);
        if (!hnode) {
            continue;
        }
        dcount++;
        hash_node_free(node, hnode);
    }
    if ( node->val_len == 0) {
        main_node_free(context->current_db, node);
    }
    zstream_printf_1024(context->fp, ":%d\r\n", dcount);
}

static void do_cmd_hexists(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hexists");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    if (hash_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]))) {
        zstream_write(context->fp, ":1\r\n", 4);
    } else {
        zstream_write(context->fp, ":0\r\n", 4);
    }
}

static void do_cmd_hlen(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hlen");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    zstream_printf_1024(context->fp, ":%d\r\n", node->val_len);
}

static void do_cmd_hget(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hget");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, "$-1\r\n", 5);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (hnode) {
        zbuf_t *tmpv = zbuf_create(-1);
        hash_node_get_value(hnode, tmpv);
        zstream_printf_1024(context->fp, "$%d\r\n", zbuf_len(tmpv));
        zstream_write(context->fp, zbuf_data(tmpv), zbuf_len(tmpv));
        zstream_write(context->fp, "\r\n", 2);
        zbuf_free(tmpv);
    } else {
        zstream_write(context->fp, "$-1\r\n", 5);
    }
}

static void do_cmd_hset(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hset");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        node = main_node_create_with_hash_val(context, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (!hnode) {
        hnode = hash_node_create(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
        zstream_write(context->fp, ":0\r\n", 4);
    } else {
        hash_node_clear_value(hnode);
        zstream_write(context->fp, ":1\r\n", 4);
    }
    hash_node_set_value(hnode, (zbuf_t *)(zvector_data(cmd_vector)[3]));
}

static void do_cmd_hsetnx(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hsetnx");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        node = main_node_create_with_hash_val(context, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (!hnode) {
        hnode = hash_node_create(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
        hash_node_set_value(hnode, (zbuf_t *)(zvector_data(cmd_vector)[3]));
        zstream_write(context->fp, ":1\r\n", 4);
    } else {
        zstream_write(context->fp, ":0\r\n", 4);
    }
}

static void do_cmd_hincrby(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hincrby");
    }
    long num;
    if (!get_check_long_integer(zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[3])), &num)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        node = main_node_create_with_hash_val(context, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (!hnode) {
        hnode = hash_node_create(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
        hnode->type = node_type_integer;
        hnode->val.num = 0;
    } else {
        zbuf_t *tmpv = zbuf_create(-1);
        hash_node_get_value(hnode, tmpv);
        long num2;
        if (!get_check_long_integer(zbuf_data(tmpv), &num2)) {
            zbuf_free(tmpv);
            zstream_write(context->fp, "-ERROR hash value is not an integerr\r\n", 36);
            return;
        }
        zbuf_free(tmpv);
        hash_node_clear_value(hnode);
        hnode->val.num = num2;
        hnode->type = node_type_integer;
    }
    hnode->val.num += num;
    zstream_printf_1024(context->fp, ":%ld\r\n", hnode->val.num);
}

static void do_cmd_hkeys(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hkeys");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, "*0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    zbuf_t *result = zbuf_create(-1);
    for (zrbtree_node_t *rbn = zrbtree_first(node->val.hash_tree); rbn; rbn = zrbtree_next(rbn)) {
        hash_node_t * hnode = ZCONTAINER_OF(rbn, hash_node_t, rbkey);
        zbuf_printf_1024(result, "$%d\r\n", hnode->key_len);
        hash_node_get_key(hnode, result);
        zbuf_strcat(result, "\r\n");
        rnum ++;
    }
    zstream_printf_1024(context->fp, "*%d\r\n", rnum);
    if (zbuf_len(result)) {
        zstream_write(context->fp, zbuf_data(result), zbuf_len(result));
    }
    zbuf_free(result);
}

static void do_cmd_hmget(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) < 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hmget");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (node && (node->type != node_type_hash)) {
        RETURN_WRONG_VALUE_TYPE();
    }

    zbuf_t *tmpval = zbuf_create(-1);
    zbuf_t *result = zbuf_create(-1);
    zbuf_printf_1024(result, "*%d\r\n", zvector_len(cmd_vector) - 2);
    for (int i=2; i < zvector_len(cmd_vector); i++) {
        zbuf_t *bf = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        if (!node) {
            zbuf_memcat(result, "$-1\r\n", 5);
        } else {
            hash_node_t *hnode = hash_node_find(node, bf);
            if (hnode) {
                zbuf_reset(tmpval);
                hash_node_get_value(hnode, tmpval);
                zbuf_printf_1024(result, "$%d\r\n", zbuf_len(tmpval));
                zbuf_append(result, tmpval);
                zbuf_puts(result, "\r\n");
            } else {
                zbuf_memcat(result, "$-1\r\n", 5);
            }
        }
    }
    zstream_append(context->fp, result);
    zbuf_free(tmpval);
    zbuf_free(result);
}

static void do_cmd_hmset(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 4) || (pcount%2 != 0)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hmset");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        node = main_node_create_with_hash_val(context, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    for (int i=2; i< zvector_len(cmd_vector); i+=2) {
        zbuf_t *key = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        zbuf_t *val = (zbuf_t *)(zvector_data(cmd_vector)[i+1]);
        hash_node_t *hnode = hash_node_find(node, key);
        if (hnode) {
            hash_node_clear_value(hnode);
        } else {
            hnode = hash_node_create(node, key);
        }
        hash_node_set_value(hnode, val);
    }
    zstream_puts(context->fp, "+OK\r\n");
}

static void do_cmd_hgetall(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hgetall");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, "*0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    zbuf_t *result = zbuf_create(-1);
    zbuf_t *tmpv = zbuf_create(-1);
    for (zrbtree_node_t *rbn = zrbtree_first(node->val.hash_tree); rbn; rbn = zrbtree_next(rbn)) {
        hash_node_t * hnode = ZCONTAINER_OF(rbn, hash_node_t, rbkey);
        zbuf_printf_1024(result, "$%d\r\n", hnode->key_len);
        hash_node_get_key(hnode, result);
        zbuf_memcat(result, "\r\n", 2);
        zbuf_reset(tmpv);
        hash_node_get_value(hnode, tmpv);
        zbuf_printf_1024(result, "$%d\r\n", zbuf_len(tmpv));
        zbuf_append(result, tmpv);
        zbuf_memcat(result, "\r\n", 2);
        rnum +=2;
    }
    zstream_printf_1024(context->fp, "*%d\r\n", rnum);
    if (zbuf_len(result)) {
        zstream_append(context->fp, result);
    }
    zbuf_free(tmpv);
    zbuf_free(result);
}

/* }}} */

/* {{{ set node */
static set_node_t *set_node_create(main_node_t *node, zbuf_t *key)
{
    set_node_t *hnode = (set_node_t *)zcalloc(1, sizeof(set_node_t));
    hnode->type = node_type_init;
    char *ptr = zbuf_data(key);
    int klen = zbuf_len(key);
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            hnode->key.str[i] = ptr[i];
        }
    } else {
        hnode->key.ptr = (char *)zmemdup(ptr, klen);
    }
    hnode->key_len = klen;
    zrbtree_attach(node->val.set_tree, &(hnode->rbkey));
    node->val_len++;
    return hnode;
}

static set_node_t *set_node_find(main_node_t *node, zbuf_t *k)
{
    char *ptr = zbuf_data(k);
    int klen = zbuf_len(k);

    set_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    zrbtree_node_t *rnp = zrbtree_find(node->val.set_tree, &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return ZCONTAINER_OF(rnp, set_node_t, rbkey);
}

static void set_node_get_key(set_node_t *node, zbuf_t *key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        zbuf_memcat(key, node->key.str, node->key_len);
    } else {
        zbuf_memcat(key, node->key.ptr, node->key_len);
    }
}

static void set_node_free(main_node_t *node, set_node_t *hnode)
{
    node->val_len--;
    zrbtree_detach(node->val.set_tree, &(hnode->rbkey));
    if (hnode->key_len > (int)sizeof(char *)) {
        zfree(hnode->key.ptr);
    }
    zfree(hnode);
}


static main_node_t * main_node_create_with_set_val(connection_context_t *context, zbuf_t *key)
{
    main_node_t * node = main_node_create(context->current_db, key);
    node->type = node_type_set;
    node->val_len = 0;
    node->val.set_tree = (zrbtree_t *)zcalloc(1, sizeof(zrbtree_t));
    zrbtree_init(node->val.set_tree, set_node_cmp);
    return node;
}

static void do_cmd_sadd(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("sadd");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        node = main_node_create_with_set_val(context, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int add_count = 0;
    for (int i = 2; i < zvector_len(cmd_vector); i++) {
        zbuf_t *key = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        set_node_t *hnode = set_node_find(node, key);
        if (!hnode) {
            set_node_create(node, key);
            add_count++;
        }
    }
    zstream_printf_1024(context->fp, ":%d\r\n", add_count);
}

static void do_cmd_scard(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 2)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("scard");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_printf_1024(context->fp, ":0\r\n");
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    zstream_printf_1024(context->fp, ":%d\r\n", node->val_len);
}

static void do_cmd_sismember(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("sismember");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    set_node_t *hnode = set_node_find(node, (zbuf_t *)(zvector_data(cmd_vector)[2]));
    if (!hnode) {
        zstream_write(context->fp, ":0\r\n", 4);
    } else {
        zstream_write(context->fp, ":1\r\n", 4);
    }
}

static void do_cmd_smembers(connection_context_t *context, zvector_t *cmd_vector)
{
    if (zvector_len(cmd_vector) != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("smembers");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, "*0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    zbuf_t *result = zbuf_create(-1);
    for (zrbtree_node_t *rbn = zrbtree_first(node->val.set_tree); rbn; rbn = zrbtree_next(rbn)) {
        set_node_t * hnode = ZCONTAINER_OF(rbn, set_node_t, rbkey);
        zbuf_printf_1024(result, "$%d\r\n", hnode->key_len);
        set_node_get_key(hnode, result);
        zbuf_memcat(result, "\r\n", 2);
        rnum +=1;
    }
    zstream_printf_1024(context->fp, "*%d\r\n", rnum);
    if (zbuf_len(result)) {
        zstream_append(context->fp, result);
    }
}

static void do_cmd_srem(connection_context_t *context, zvector_t *cmd_vector)
{
    int pcount = (int)zvector_len(cmd_vector);
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("srem");
    }
    main_node_t *node = main_node_find(context->current_db, (zbuf_t *)(zvector_data(cmd_vector)[1]));
    if (!node) {
        zstream_write(context->fp, ":0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int dcount = 0;
    for (int i=2; i<zvector_len(cmd_vector); i++) {
        zbuf_t *bf = (zbuf_t *)(zvector_data(cmd_vector)[i]);
        set_node_t *hnode = set_node_find(node, bf);
        if (!hnode) {
            continue;
        }
        dcount++;
        set_node_free(node, hnode);
    }
    if ( node->val_len == 0) {
        main_node_free(context->current_db, node);
    }
    zstream_printf_1024(context->fp, ":%d\r\n", dcount);
}
/* }}} */

/* {{{ cmd tree */
static zmap_t *redis_cmd_tree = 0;
static void redis_cmd_tree_init()
{
    redis_cmd_tree = zmap_create();

    zmap_update(redis_cmd_tree, "FLUSHDB", do_cmd_flushdb, 0);
    zmap_update(redis_cmd_tree, "FLUSHALL", do_cmd_flushall, 0);
    zmap_update(redis_cmd_tree, "QUIT", do_cmd_quit, 0);
    zmap_update(redis_cmd_tree, "DBSIZE", do_cmd_dbsize, 0);
    zmap_update(redis_cmd_tree, "PING", do_cmd_ping, 0);
    zmap_update(redis_cmd_tree, "DEL", do_cmd_del, 0);
    zmap_update(redis_cmd_tree, "EXISTS", do_cmd_exists, 0);
    zmap_update(redis_cmd_tree, "EXPIRE", do_cmd_expire, 0);
    zmap_update(redis_cmd_tree, "PEXPIRE", do_cmd_pexpire, 0);
    zmap_update(redis_cmd_tree, "KEYS", do_cmd_keys, 0);
    zmap_update(redis_cmd_tree, "TTL", do_cmd_ttl, 0);
    zmap_update(redis_cmd_tree, "PTTL", do_cmd_pttl, 0);
    zmap_update(redis_cmd_tree, "PERSIST", do_cmd_persist, 0);
    zmap_update(redis_cmd_tree, "RENAME", do_cmd_rename, 0);
    zmap_update(redis_cmd_tree, "RENAMENX", do_cmd_renamenx, 0);
    zmap_update(redis_cmd_tree, "TYPE", do_cmd_type, 0);
    zmap_update(redis_cmd_tree, "DECR", do_cmd_decr, 0);
    zmap_update(redis_cmd_tree, "DECRBY", do_cmd_decrby, 0);
    zmap_update(redis_cmd_tree, "INCR", do_cmd_incr, 0);
    zmap_update(redis_cmd_tree, "INCRBY", do_cmd_incrby, 0);
    zmap_update(redis_cmd_tree, "GET", do_cmd_get, 0);
    zmap_update(redis_cmd_tree, "SET", do_cmd_set, 0);
    zmap_update(redis_cmd_tree, "GETSET", do_cmd_getset, 0);
    zmap_update(redis_cmd_tree, "MGET", do_cmd_mget, 0);
    zmap_update(redis_cmd_tree, "MSET", do_cmd_mset, 0);
    zmap_update(redis_cmd_tree, "MSETNX", do_cmd_msetnx, 0);
    zmap_update(redis_cmd_tree, "HDEL", do_cmd_hdel, 0);
    zmap_update(redis_cmd_tree, "HEXISTS", do_cmd_hexists, 0);
    zmap_update(redis_cmd_tree, "HLEN", do_cmd_hlen, 0);
    zmap_update(redis_cmd_tree, "HGET", do_cmd_hget, 0);
    zmap_update(redis_cmd_tree, "HSET", do_cmd_hset, 0);
    zmap_update(redis_cmd_tree, "HSETNX", do_cmd_hsetnx, 0);
    zmap_update(redis_cmd_tree, "HINCRBY", do_cmd_hincrby, 0);
    zmap_update(redis_cmd_tree, "HKEYS", do_cmd_hkeys, 0);
    zmap_update(redis_cmd_tree, "HMGET", do_cmd_hmget, 0);
    zmap_update(redis_cmd_tree, "HMSET", do_cmd_hmset, 0);
    zmap_update(redis_cmd_tree, "HGETALL", do_cmd_hgetall, 0);
    zmap_update(redis_cmd_tree, "SADD", do_cmd_sadd, 0);
    zmap_update(redis_cmd_tree, "SCARD", do_cmd_scard, 0);
    zmap_update(redis_cmd_tree, "SMEMBERS", do_cmd_smembers, 0);
    zmap_update(redis_cmd_tree, "SISMEMBER", do_cmd_sismember, 0);
    zmap_update(redis_cmd_tree, "SREM", do_cmd_srem, 0);
}
/* }}} */

/* {{{ server */
static void parse_query_logic_line(zvector_t *cmd_vector, zbuf_t *strbuf)
{
    zbuf_trim_right_rn(strbuf);
    char *ps = zbuf_data(strbuf);
    int size = zbuf_len(strbuf);
    ps[size] = 0;
    zstrtok_t sk;
    zstrtok_init(&sk, ps);
    while(zstrtok(&sk, " \t")) {
        zbuf_t *tmp = zbuf_create(sk.len);
        zbuf_memcpy(tmp, sk.str, sk.len);
        zvector_push(cmd_vector, tmp);
    }
}

static int get_query_data(zvector_t *cmd_vector, zbuf_t *strbuf, connection_context_t *context)
{
    zstream_t *fp = context->fp;
    char *ps = zbuf_data(strbuf);
    int count = atoi(ps+1);
    if (count < 1) {
        return -2;
    }
    for (int i = 0; i < count; i++) {
        zbuf_reset(strbuf);
        zstream_gets(fp, strbuf, 1024 * 1024);
        if (zstream_is_exception(fp)) {
            return -2;
        }
        ps = zbuf_data(strbuf);
        if (*ps != '$') {
            return -2;
        }
        int slen = atoi(ps + 1);
        if (slen < 0 || slen > 1024 * 1024) {
            return -2;
        }
        zbuf_reset(strbuf);
        if (slen > 0) {
            zstream_readn(fp, strbuf, slen);
        }
        zstream_readn(fp, 0, 2);
        if (zstream_is_exception(fp)) {
            return -2;
        }
        zbuf_t *bf = zbuf_create(zbuf_len(strbuf));
        zbuf_append(bf, strbuf);
        zvector_push(cmd_vector, bf);
    }
    return 0;
}

static int do_one_query(connection_context_t *context, zvector_t *cmd_vector, zbuf_t *strbuf)
{
    zstream_t *fp = context->fp;
    zbuf_vector_reset(cmd_vector);
    zbuf_reset(strbuf);
    while(zstream_timed_read_wait(fp, 10) == 0);
    /* do not care about too much long line */
    zstream_gets(fp, strbuf, 1024 * 1024);
    if (zstream_is_exception(fp)) {
        return -2;
    }
    if (zbuf_data(strbuf)[0] != '*') {
        parse_query_logic_line(cmd_vector, strbuf);
    } else if (get_query_data(cmd_vector, strbuf, context) < 0) {
        return -2;
    }
    if (zvector_len(cmd_vector) == 0) {
        zstream_write(fp, "\r\n", 2);
    } else {
        do_cmd_fn_t cmdfn = 0;
        char *cmd_name = zbuf_data((zbuf_t *)(zvector_data(cmd_vector)[0]));
        zstr_toupper(cmd_name);
        if (!zmap_find(redis_cmd_tree, cmd_name, (void **)&cmdfn)) {
            cmdfn = do_cmd_unkonwn;
        }
        cmdfn(context, cmd_vector);
    }
    zstream_flush(fp);
    if (zstream_is_exception(fp)) {
        return -2;
    }
    return 1;
}

static void *do_job(void *arg)
{
    ztype_convert_t ct;
    ct.VOID_PTR = arg;
    connection_context_t context;
    memset(&context, 0, sizeof(connection_context_t));
    context.fp = zstream_open_fd(ct.fdinfo.fd);
    context.current_db = &default_db;
    zvector_t *cmd_vector = zvector_create(-1);
    zbuf_t *strbuf = zbuf_create(-1);
    while (1) {
        if ((do_one_query(&context, cmd_vector, strbuf) < 0) || context.quit) {
            break;
        }
    }
    zbuf_vector_free(cmd_vector);
    zbuf_free(strbuf);
    zstream_close(context.fp, 1);
    return arg;
}

static void *do_accept(void *arg)
{
    ztype_convert_t ct;
    ct.VOID_PTR = arg;
    int sock = ct.fdinfo.fd;
    int sock_type = ct.fdinfo.fd_type;
    while(zvar_sigint_flag == 0) {
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            zfatal("redis_puny_server, socket fd error");
        }
        ct.fdinfo.fd = fd;
        zcoroutine_go(do_job, ct.VOID_PTR, 0);
    }
    return arg;
}

static void ___service_register(const char *service_name, int fd, int fd_type)
{
    if (zredis_puny_server_service_register && (zredis_puny_server_service_register(service_name, fd, fd_type) == 1)) {
        return;
    }

    ztype_convert_t ct;
    ct.fdinfo.fd = fd;
    ct.fdinfo.fd_type = fd_type;
    zcoroutine_go(do_accept, ct.VOID_PTR, 16);
}

static void *do_timeout(void *arg)
{
    while(1) {
        zcoroutine_sleep(1);
        long nowtime = ztimeout_set_millisecond(0);
        while(1) {
            zrbtree_node_t *rbn = zrbtree_first(&(default_db.timeout_tree));
            if (!rbn) {
                break;
            }
            main_node_t *node = ZCONTAINER_OF(rbn, main_node_t, rbtimeout);
            if (node->timeout <= nowtime) {
                zrbtree_detach(&(default_db.timeout_tree), &(node->rbtimeout));
                main_node_free(&default_db, node);
            } else {
                break;
            }
        }
    }
    return arg;
}

void (*zredis_puny_server_before_service)() = 0;
void (*zredis_puny_server_before_softstop)() = 0;
zbool_t (*zredis_puny_server_service_register)(const char *service, int fd, int fd_type) = 0;

static void ___before_service_prepare_load(char *fn)
{
    if (zempty(fn)) {
        return;
    }
    connection_context_t ctx;
    memset(&ctx, 0, sizeof (connection_context_t));
    ctx.current_db = &default_db;
    ctx.fp = zstream_open_file("/dev/null", "w");
    zstream_t *fp = zstream_open_file(fn, "r");
    if (!fp) {
        zfatal("can not open %s", fn);
    }
    zbuf_t *linebuf = zbuf_create(4096);
    zvector_t *cmds = zvector_create(-1);
    zjson_t *jss = zjson_create();
    while(zstream_gets(fp, linebuf, 1024 * 1024) > 0) {
        zbuf_trim_right_rn(linebuf);
        if (zbuf_len(linebuf) < 1) {
            continue;
        }
        zbuf_vector_reset(cmds);
        zjson_reset(jss);
        zjson_unserialize(jss, zbuf_data(linebuf), zbuf_len(linebuf));
        if (!zjson_is_array(jss)) {
            continue;
        }
        const zvector_t *jvec = zjson_get_array_value(jss);
        for (int i=0;i < zvector_len(jvec); i++) {
            zjson_t *js = (zjson_t *)(zvector_data(jvec)[i]);
            if (!zjson_is_string(js)) {
                zfatal("all element must be string: %s", zbuf_data(linebuf));
            }
            zbuf_t *bf, *bf2 = *zjson_get_string_value(js);
            bf = zbuf_create(zbuf_len(bf2));
            zbuf_append(bf, bf2);
            zvector_push(cmds, bf);
        }
        if (zvector_len(cmds) == 0) {
            continue;
        }
        char *cmd_name = zbuf_data((zbuf_t *)(zvector_data(cmds)[0]));
        zstr_toupper(cmd_name);
        do_cmd_fn_t cmdfn = 0;
        if (!zmap_find(redis_cmd_tree, cmd_name, (void **)&cmdfn)) {
            zfatal("unknown cmd: %s", cmd_name);
        }
        cmdfn(&ctx, cmds);
    }
    zstream_close(ctx.fp, 1);
    zbuf_free(linebuf);
    zbuf_vector_free(cmds);
    zjson_free(jss);
    zstream_close(fp, 1);
}

static void ___before_softstop()
{
    if (zredis_puny_server_before_softstop) {
        zredis_puny_server_before_softstop();
    } else {
        zaio_server_stop_notify(0);
    }
}

static void ___before_service()
{
    zrbtree_init(&(default_db.key_tree), main_node_cmp_key);
    zrbtree_init(&(default_db.timeout_tree), main_node_cmp_timeout);
    redis_cmd_tree_init();

    char *attr;
    attr = zconfig_get_str(zvar_default_config, "redis-server-prepare-cmd", "");
    if (!zempty(attr)) {
        system(attr);
    }

    attr = zconfig_get_str(zvar_default_config, "redis-server-prepare-load", "");
    ___before_service_prepare_load(attr);

    if (zredis_puny_server_before_service) {
        zredis_puny_server_before_service();
    }

    zcoroutine_go(do_timeout, 0, 0);
}

int zredis_puny_server_main(int argc, char **argv)
{
    zcoroutine_server_service_register = ___service_register;
    zcoroutine_server_before_service = ___before_service;
    zcoroutine_server_before_softstop = ___before_softstop;
    int ret = zcoroutine_server_main(argc, argv);
    zmap_free(redis_cmd_tree);
    return ret;
}

void zredis_puny_server_exec_cmd(zvector_t *cmds)
{
    if (zvector_len(cmds) == 0) {
        return;
    }
    char *cmd_name = zbuf_data((zbuf_t *)(zvector_data(cmds)[0]));
    zstr_toupper(cmd_name);
    do_cmd_fn_t cmdfn;
    if (!zmap_find(redis_cmd_tree, cmd_name, (void **)&cmdfn)){
        return;
    }
    connection_context_t ctx;
    memset(&ctx, 0, sizeof(connection_context_t));
    ctx.current_db = &default_db;
    cmdfn(&ctx, cmds);
}

/* }}} */

/* Local variables:
* End:
* vim600: fdm=marker
*/
