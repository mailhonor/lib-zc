/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2012-09-24
 * ================================
 */
#ifndef ___ZINCLUDE_STDLIB_
#define ___ZINCLUDE_STDLIB_
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ################################################################## */
typedef struct zring_t zring_t;
typedef struct zlink_node_t zlink_node_t;
typedef struct zlink_t zlink_t;
typedef struct zhtable_node_t zhtable_node_t;
typedef struct zhtable_t zhtable_t;
typedef struct zrbtree_node_t zrbtree_node_t;
typedef struct zrbtree_t zrbtree_t;
typedef struct zmpool_t zmpool_t;
typedef struct zmcot_t zmcot_t;
typedef struct zbuf_t zbuf_t;
typedef struct zsdata_t zsdata_t;
typedef struct zsdlist_t zsdlist_t;
typedef struct zargv_t zargv_t;
typedef struct zarray_t zarray_t;
typedef struct zchain_t zchain_t;
typedef struct zchain_node_t zchain_node_t;
typedef struct zdict_t zdict_t;
typedef struct zidict_node_t zidict_node_t;
typedef struct zidict_t zidict_t;
typedef struct zdict_node_t zdict_node_t;
typedef struct zgrid_t zgrid_t;
typedef struct zgrid_node_t zgrid_node_t;
typedef struct zigrid_t zigrid_t;
typedef struct zigrid_node_t zigrid_node_t;
#define zconfig_t zdict_t
typedef struct zkvlist_t zkvlist_t;
typedef struct zmap_t zmap_t;
typedef struct zmap_node_t zmap_node_t;
typedef struct zaddr_t zaddr_t;
typedef struct zsslctx_t zsslctx_t;
typedef struct zssl_t zssl_t;
typedef struct zstream_t zstream_t;
typedef struct zalarm_t zalarm_t;
typedef struct zev_t zev_t;
typedef struct zaio_t zaio_t;
typedef struct zevtimer_t zevtimer_t;
typedef struct zevbase_t zevbase_t;
typedef struct ziopipe_base_t ziopipe_base_t;
typedef struct zmmap_reader zmmap_reader;
typedef struct zmd5_t zmd5_t;
typedef struct zsha1_t zsha1_t;
typedef struct zcharset_iconv_t zcharset_iconv_t;
typedef struct zmail_parser_t zmail_parser_t;
typedef struct zmail_mime_t zmail_mime_t;
typedef struct zmail_addr_t zmail_addr_t;
typedef struct zmail_header_line_t zmail_header_line_t;
typedef struct zmail_references_t zmail_references_t;
typedef struct ztnef_parser_t ztnef_parser_t;
typedef struct ztnef_mime_t ztnef_mime_t;

/* ################################################################## */
#define ZFREE(a)                     (zfree(a),a=0)
#define ZEMPTY(str)                  (!(str)||!(*(str)))
#define ZCHAR_PTR_TO_INT(ptr)        ((int) (long) (ptr))
#define ZINT_TO_CHAR_PTR(val)        ((char *) (long) (val))
#define ZVOID_PTR_TO_INT(ptr)        ((int) (long) (ptr))
#define ZINT_TO_VOID_PTR(val)        ((void *) (long) (val))
#define ZCONTAINER_OF(ptr,app_type,member)          \
    ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))
#define ZERRORMSG_LEN                1024

#define ZSTR_N_CASE_EQ(a, b, n)       ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strncasecmp(a,b,n)))
#define ZSTR_CASE_EQ(a, b)            ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strcasecmp(a,b)))
#define ZSTR_N_EQ(a, b, n)            ((a[0] == b[0]) && (!strncmp(a,b,n)))
#define ZSTR_EQ(a, b)                 ((a[0] == b[0]) && (!strcmp(a,b)))

#define ZTYPE_BUF       0x1
#define ZTYPE_ZBUF      0x2
#define ZTYPE_FILE      0x3

#define zpthread_lock(l)        {if(pthread_mutex_lock(l)){zfatal("mutex:%m");}}
#define zpthread_unlock(l)      {if(pthread_mutex_unlock(l)){zfatal("mutex:%m");}}

/* ################################################################## */
extern char *zvar_progname;

/* ################################################################## */
/* 常用函数类型 */
typedef void (*zvoid_t) (void);
typedef void (*zfunc_v_v_t) (void);
typedef void (*zfunc_vp_v_t) (void *);
typedef void *(*zfunc_vp_vp_t) (void *);

/* ################################################################## */
/* LOG, 通用 */
extern int zvar_fatal_catch;
extern int zvar_log_level;
typedef int (*zlog_output_t) (int level, char *fmt, ...);
typedef int (*zlog_voutput_t) (int level, char *fmt, va_list ap);
extern zlog_output_t zlog_output;
extern zlog_output_t zlog_fatal_output;
extern zlog_voutput_t zlog_voutput;
zlog_voutput_t zlog_set_voutput(zlog_voutput_t voutput_fn);
int zlog_set_level(int level);
int zlog_set_level_from_console(int level);
int zlog_parse_level(char *levstr);

#define ZLOG_FATAL       1
#define ZLOG_ERROR       2
#define ZLOG_WARNING     3
#define ZLOG_NOTICE      4
#define ZLOG_INFO        5
#define ZLOG_DEBUG       6
#define ZLOG_VERBOSE     7

#define zfatal(fmt, args...) {zlog_fatal_output(ZLOG_FATAL, fmt, ##args);}
#define zerror(fmt, args...) {if(ZLOG_ERROR<=zvar_log_level){zlog_output(ZLOG_ERROR, fmt, ##args);}}
#define zwarning(fmt, args...) {if(ZLOG_WARNING<=zvar_log_level){zlog_output(ZLOG_WARNING, fmt, ##args);}}
#define znotice(fmt, args...) {if(ZLOG_NOTICE<=zvar_log_level){zlog_output(ZLOG_NOTICE, fmt, ##args);}}
#define zinfo(fmt, args...) {if(ZLOG_INFO<=zvar_log_level){zlog_output(ZLOG_INFO, fmt, ##args);}}
#define zdebug(fmt, args...) {if(ZLOG_DEBUG<=zvar_log_level){zlog_output(ZLOG_DEBUG, fmt, ##args);}}
#define zverbose(fmt, args...) {if(ZLOG_VERBOSE<=zvar_log_level){zlog_output(ZLOG_DEBUG, fmt, ##args);}}

/* ################################################################## */
/* malloc */
#define zmalloc         zmalloc_20160308
#define zcalloc         zcalloc_20160308
#define zrealloc        zrealloc_20160308
#define zfree           zfree_20160308
#define zstrdup         zstrdup_20160308
#define zstrndup        zstrndup_20160308
#define zmemdup         zmemdup_20160308
void *zmalloc(int len);
void *zcalloc(int nmemb, int size);
void *zrealloc(void *ptr, int len);
void zfree(void *ptr);
char *zstrdup(char *ptr);
char *zstrndup(char *ptr, int n);
char *zmemdup(void *ptr, int n);

/* ################################################################## */
/* time */
long ztimeout_set(long timeout);
long ztimeout_left(long timeout);
void zmsleep(long mdelay);
void zsleep(long delay);

/* ################################################################## */
/* STRING */
/* strcase */
extern const unsigned char zchar_lowercase_list[];
extern const unsigned char zchar_uppercase_list[];
#define ZCHAR_TOLOWER(c)    ((int)zchar_lowercase_list[(unsigned char )(c)])
#define ZCHAR_TOUPPER(c)    ((int)zchar_uppercase_list[(unsigned char )(c)])
#define zchar_tolower       ZCHAR_TOLOWER
#define zchar_toupper       ZCHAR_TOUPPER
char *ztolower(char *str);
char *ztoupper(char *str);

/* trim */
char *ztrim_left(char *str);
char *ztrim_right(char *str);
char *ztrim(char *str);

/* strtok */
typedef struct {
    char *sstr;
    char *str;
    int len;
} zstrtok_t;
void zstrtok_init(zstrtok_t * k, char *sstr);
zstrtok_t *zstrtok(zstrtok_t * k, char *delim);

/* convert to unit */
int zstr_to_bool(char *s, int def);
long zstr_to_second(char *s);
long zstr_to_size(char *s);

/* sprintf */
int zsnprintf(char *str, int size, char *format, ...);
int zvsnprintf(char *str, int size, char *format, va_list ap);

/* */
char *zmemstr(void *s, char *needle, int len);
char *zmemcasestr(void *s, char *needle, int len);

/* strncpy, strncat */
char *zstrncpy(char *dest, char *src, int len);
char *zstrncat(char *dest, char *src, int len);

/* ################################################################## */
/* SIZEDATA */
struct zsdata_t {
    int size;
    char *data;
};
int zsdata_parse(zsdata_t * sd, void *data, int size);
void zsdata_escape(zsdata_t * sd, zbuf_t * bf);

/* ################################################################## */
/* BUF */
struct zbuf_t {
    char *data;
    int len:31;
    int size:31;
    unsigned int stack_mode;
};
#define ZBUF_DATA(b)    ((b)->data)
#define ZBUF_LEN(b)     ((b)->len)
#define ZBUF_PUT(b, c)  \
    (((b)->len<(b)->size)?((int)((b)->data[(b)->len++]=(c))):(((b)->stack_mode?0:zbuf_put_do((b), (c)))))
#define ZBUF_RESET(b)    ((b)->len=0)
#define ZBUF_TERMINATE(b)    ((b)->data[(b)->len]=0)
#define ZBUF_TRUNCATE(b, n)    ((((b)->len>n)&&(n>0))?((b)->len=n):0)

zbuf_t *zbuf_create(int size);
void zbuf_free(zbuf_t * bf);
#define zbuf_data(b)    ((b)->data)
#define zbuf_len(b)     ((b)->len)
int zbuf_need_space(zbuf_t * bf, int need);
int zbuf_put_do(zbuf_t * bf, int ch);
static inline int zbuf_put(zbuf_t * bf, int ch)
{
    if (bf->len == bf->size)
        zbuf_need_space(bf, 1);
    return ZBUF_PUT(bf, ch);
}

#define zbuf_putch       zbuf_put
#define zbuf_putchar     zbuf_put
void zbuf_reset(zbuf_t * bf);
void zbuf_terminate(zbuf_t * bf);
void zbuf_truncate(zbuf_t * bf, int new_len);
int zbuf_strncpy(zbuf_t * bf, char *src, int len);
int zbuf_strcpy(zbuf_t * bf, char *src);
int zbuf_strncat(zbuf_t * bf, char *src, int len);
int zbuf_strcat(zbuf_t * bf, char *src);
int zbuf_memcpy(zbuf_t * bf, void *src, int len);
int zbuf_memcat(zbuf_t * bf, void *src, int len);
int zbuf_add_int(zbuf_t * bf, int num);
int zbuf_simple_sprintf(zbuf_t * bf, char *format, ...);
int zbuf_vprintf(zbuf_t * bf, char *format, va_list ap);
int zbuf_sprintf(zbuf_t * bf, char *format, ...);
int zbuf_vsprintf(zbuf_t * bf, char *format, va_list ap);

void zbuf_sizedata_escape(zbuf_t * bf, void *data, int len);
void zbuf_sizedata_escape_int(zbuf_t * bf, int i);
void zbuf_sizedata_escape_long(zbuf_t * bf, long i);
void zbuf_sizedata_escape_dict(zbuf_t * bf, zdict_t * wd);
void zbuf_sizedata_escape_pp(zbuf_t * bf, char **pp, int len);

/* STACK_BUF */
#define ZSTACK_BUF(name, _size)    \
    zbuf_t name ## _ZSTACT_BUF_ , *name; \
    name = &name ## _ZSTACT_BUF_; \
    char name ## _databuf_STACK [_size+1]; \
    name->size = _size; name->len = 0; \
    name->data = name ## _databuf_STACK; \
    name->stack_mode = 1;

#define ZSTACK_BUF_FROM(name, _data, _size)    \
    zbuf_t name ## _ZSTACT_BUF_, *name; \
    name = &name ## _ZSTACT_BUF_; \
    name->size = _size; name->len = 0; \
    name->data = (char *)(_data);

/* ################################################################## */
/* ZRING */
#define ZRING_WALK_BEGIN(head, var_your_node)         {\
    for (var_your_node = (head)->next; var_your_node != (head); var_your_node = var_your_node->next){
#define ZRING_WALK_END                }}
struct zring_t {
    zring_t *prev;
    zring_t *next;
};
void zring_init(zring_t * ring);
void zring_append(zring_t * ring, zring_t * entry);
void zring_prepend(zring_t * ring, zring_t * entry);
void zring_detach(zring_t * entry);

/* ################################################################## */
/* MLINK*/
#define ZMLINK_INIT(p)    (p=0)
#define ZMLINK_APPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZMLINK_PREPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node, _before_1106 = before;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else if(_before_1106==0){_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    else if(_before_1106==_head_1106){_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    else {_node_1106->prev=_before_1106->prev; _node_1106->next=_before_1106; _before_1106->prev->next=_node_1106; _before_1106->prev=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZMLINK_DETACH(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_node_1106->prev){ _node_1106->prev->next=_node_1106->next; }else{ _head_1106=_node_1106->next; }\
    if(_node_1106->next){ _node_1106->next->prev=_node_1106->prev; }else{ _tail_1106=_node_1106->prev; }\
    head = _head_1106; tail = _tail_1106; \
}

/* ################################################################## */
/* LINK */
struct zlink_t {
    zlink_node_t *head;
    zlink_node_t *tail;
};
struct zlink_node_t {
    zlink_node_t *prev;
    zlink_node_t *next;
};
void zlink_init(zlink_t * link);
zlink_node_t *zlink_attach_before(zlink_t * link, zlink_node_t * node, zlink_node_t * before);
zlink_node_t *zlink_detach(zlink_t * link, zlink_node_t * node);
zlink_node_t *zlink_push(zlink_t * link, zlink_node_t * node);
zlink_node_t *zlink_unshift(zlink_t * link, zlink_node_t * node);
zlink_node_t *zlink_pop(zlink_t * link);
zlink_node_t *zlink_shift(zlink_t * link);
void zlink_fini(zlink_t * link, void (*fini_fn) (zlink_node_t *));
void zlink_walk(zlink_t * link, void (*walk_fn) (zlink_node_t *));
#define ZLINK_HEAD(link)     ((link)->head)
#define ZLINK_TAIL(link)     ((link)->tail)
#define ZLINK_PREV(node)     ((node)->prev)
#define ZLINK_NEXT(node)     ((node)->next)
#define zlink_head(link)     ((link)->head)
#define zlink_tail(link)     ((link)->tail)
#define zlink_prev(node)     ((node)->prev)
#define zlink_next(node)     ((node)->next)

/* ################################################################## */
/* ARGV */
struct zargv_t {
    int size;
    int argc;
    char **argv;
    zmpool_t *mpool;
};
#define ZARGC(ar)               ((ar)->argc)
#define ZARGV(ar)               ((ar)->argv)
#define ZARGV_LEN(ar)           ((ar)->argc)
#define ZARGV_ARGC(ar)          ((ar)->argc)
#define ZARGV_ARGV(ar)          ((ar)->argv)
#define ZARGV_DATA(ar)          ((ar)->argv)
#define ZARGV_REST(ar)          (zargv_truncate((ar), 0))
#define ZARGV_WALK_BEGIN(ar, var_your_chp)   {\
    int  zargv_tmpvar_i; zargv_t *___ar_tmp_ptr = ar;\
        for(zargv_tmpvar_i=0;zargv_tmpvar_i<(___ar_tmp_ptr)->argc;zargv_tmpvar_i++){\
            var_your_chp = (___ar_tmp_ptr)->argv[zargv_tmpvar_i];
#define ZARGV_WALK_END                       }}

#define zargv_len(ar)           ((ar)->argc)
#define zargv_argc(ar)          ((ar)->argc)
#define zargv_argv(ar)          ((ar)->argv)
#define zargv_data(ar)          ((ar)->argv)
zargv_t *zargv_create_mpool(int size, zmpool_t * mpool);
zargv_t *zargv_create(int size);
void zargv_free(zargv_t * argvp);
void zargv_add(zargv_t * argvp, char *ns);
void zargv_addn(zargv_t * argvp, char *ns, int nlen);
void zargv_truncate(zargv_t * argvp, int len);
void zargv_rest(zargv_t * argvp);
zargv_t *zargv_split_append(zargv_t * argvp, char *string, char *delim);
void zargv_show(zargv_t * argvp);

/* ################################################################## */
/* ARRAY */
struct zarray_t {
    int len;
    int size;
    char **data;
};
#define ZARRAY_LEN(arr)            ((arr)->len)
#define ZARRAY_DATA(arr)        ((arr)->data)
#define zarray_len(arr)            ((arr)->len)
#define zarray_data(arr)        ((arr)->data)
zarray_t *zarray_create(int size);
void zarray_free(zarray_t * arr, void (*free_fn) (void *, void *), void *ctx);
void zarray_truncate(zarray_t * arr, int len, void (*free_fn) (void *, void *), void *ctx);
void zarray_add(zarray_t * arr, void *ns);
#define ZARRAY_WALK_BEGIN(arr, var_your_chp)    {\
    int  zargv_tmpvar_i;\
    for(zargv_tmpvar_i=0;zargv_tmpvar_i<(arr)->len;zargv_tmpvar_i++){\
        var_your_chp = (typeof(var_your_chp))((arr)->data[zargv_tmpvar_i]);
#define ZARRAY_WALK_END                }}

/* ################################################################## */
/* HTABLE */
typedef int (*zhtable_cmp_t) (zhtable_node_t * node1, zhtable_node_t * node2);
typedef int (*zhtable_hash_t) (zhtable_node_t * node, int size);
struct zhtable_node_t {
    zhtable_node_t *next;
    zhtable_node_t *prev;
};
struct zhtable_t {
    int size;
    int len;
    zhtable_node_t **data;
    zhtable_cmp_t cmp_fn;
    zhtable_hash_t hash_fn;
};
void zhtable_init(zhtable_t * table, int size, zhtable_cmp_t cmp_fn, zhtable_hash_t hash_fn);
zhtable_node_t *zhtable_attach(zhtable_t * table, zhtable_node_t * node);
zhtable_node_t *zhtable_lookup(zhtable_t * table, zhtable_node_t * vnode);
zhtable_node_t *zhtable_remove(zhtable_t * table, zhtable_node_t * vnode);
zhtable_node_t *zhtable_detach(zhtable_t * table, zhtable_node_t * node);
void zhtable_fini(zhtable_t * table, void (*fini_fn) (zhtable_node_t *, void *), void *ctx);
void zhtable_walk(zhtable_t * table, void (*walk_fn) (zhtable_node_t *, void *), void *ctx);
zhtable_node_t **zhtable_list(zhtable_t * table, zhtable_node_t ** list);
#define ZHTABLE_LEN(table)    ((table)->len)
#define ZHTABLE_SIZE(table)    ((table)->size)
#define ZHTABLE_DATA(table)    ((table)->data)
#define ZHTABLE_WALK_BEGIN(table, var_your_node) {                            \
    unsigned var_your_node ## __LINE__ ## i = (table)->size;                    \
    zhtable_node_t **var_your_node ## __LINE__ ## h = (table)->data;                    \
    zhtable_node_t *var_your_node ## __LINE__ ## next;                        \
    while (var_your_node ## __LINE__ ## i --){                             \
        for (var_your_node= *var_your_node ## __LINE__ ## h++;                    \
                var_your_node;var_your_node = var_your_node ## __LINE__ ## next){    \
            var_your_node ## __LINE__ ## next = var_your_node->next;
#define ZHTABLE_WALK_END                                        }}}
#define ZHTABLE_BUF_HASH(result, pre_hash, key, cond)    \
{                            \
    unsigned long g;                \
    result = pre_hash;                \
    while (cond) {                    \
        result = (result << 4U) + *key++;    \
        if ((g = (result & 0xf0000000)) != 0) { \
            result ^= (g >> 24U);        \
            result ^= g;            \
        }                    \
    }                        \
}

/* ################################################################## */
/* RBTREE */
typedef int (*zrbtree_cmp_t) (zrbtree_node_t * node1, zrbtree_node_t * node2);
struct zrbtree_t {
    zrbtree_node_t *zrbtree_node;
    zrbtree_cmp_t cmp_fn;
};
struct zrbtree_node_t {
    unsigned long __zrbtree_parent_color;
    zrbtree_node_t *zrbtree_right;
    zrbtree_node_t *zrbtree_left;
/* The alignment might seem pointless, but allegedly CRIS needs it */
} __attribute__ ((aligned(sizeof(long))));

#define zrbtree_have_data(tree)     ((tree)->zrbtree_node?1:0)
void zrbtree_init(zrbtree_t * tree, zrbtree_cmp_t cmp_fn);
void zrbtree_insert_color(zrbtree_t *, zrbtree_node_t *);
void zrbtree_erase(zrbtree_t * tree, zrbtree_node_t * node);
void zrbtree_replace_node(zrbtree_t * tree, zrbtree_node_t * victim, zrbtree_node_t * _new);
zrbtree_node_t *zrbtree_prev(zrbtree_node_t * tree);
zrbtree_node_t *zrbtree_next(zrbtree_node_t * tree);
zrbtree_node_t *zrbtree_first(zrbtree_t * node);
zrbtree_node_t *zrbtree_last(zrbtree_t * node);
zrbtree_node_t *zrbtree_near_prev(zrbtree_t * tree, zrbtree_node_t * vnode);
zrbtree_node_t *zrbtree_near_next(zrbtree_t * tree, zrbtree_node_t * vnode);
static inline zrbtree_node_t *zrbtree_parent(zrbtree_node_t * node)
{
    return ((zrbtree_node_t *) ((node)->__zrbtree_parent_color & ~3));
}

zrbtree_node_t *zrbtree_attach(zrbtree_t * tree, zrbtree_node_t * node);
zrbtree_node_t *zrbtree_lookup(zrbtree_t * tree, zrbtree_node_t * vnode);
static inline zrbtree_node_t *zrbtree_detach(zrbtree_t * tree, zrbtree_node_t * node)
{
    zrbtree_erase(tree, node);
    return node;
}

void zrbtree_walk(zrbtree_t * tree, void (*walk_fn) (zrbtree_node_t *, void *), void *ctx);
#define zrbtree_fini     zrbtree_walk
void zrbtree_walk_forward(zrbtree_t * tree, void (*walk_fn) (zrbtree_node_t *, void *), void *ctx);
void zrbtree_walk_back(zrbtree_t * tree, void (*walk_fn) (zrbtree_node_t *, void *), void *ctx);

static inline void zrbtree_link_node(zrbtree_node_t * node, zrbtree_node_t * parent, zrbtree_node_t ** zrbtree_link)
{
    node->__zrbtree_parent_color = (unsigned long)parent;
    node->zrbtree_left = node->zrbtree_right = 0;

    *zrbtree_link = node;
}

#define ZRBTREE_INIT(tree, _cmp_fn)     ((tree)->zrbtree_node=0, (tree)->cmp_fn = _cmp_fn)
#define ZRBTREE_PARENT(node)    ((zrbtree_node_t *)((node)->__zrbtree_parent_color & ~3))
#define ZRBTREE_ATTACH_PART1(root, node, cmp_node) {                            \
    zrbtree_node_t ** ___Z_new_pp = &((root)->zrbtree_node), * ___Z_parent = 0;            \
    while (*___Z_new_pp) {                                        \
        ___Z_parent = *___Z_new_pp;                                \
        cmp_node = *___Z_new_pp;                                \
        {
#define ZRBTREE_ATTACH_PART2(root, node, cmp_result, return_node)                     \
        }                                            \
        return_node = 0;                                    \
        if (cmp_result < 0) {                                    \
            ___Z_new_pp = &((*___Z_new_pp)->zrbtree_left);                    \
        } else if (cmp_result > 0) {                                \
            ___Z_new_pp = &((*___Z_new_pp)->zrbtree_right);                    \
        } else {                                        \
            return_node = *___Z_new_pp;                            \
            break;                                        \
        }                                            \
    }                                                \
    if(!return_node){                                        \
        zrbtree_link_node(node, ___Z_parent, ___Z_new_pp);                    \
        zrbtree_insert_color(root, node);                            \
        return_node = node;                                    \
    }                                                \
}

#define ZRBTREE_LOOKUP_PART1(root, cmp_node) {                                \
    zrbtree_node_t * ___Z_node_tmp = (root)->zrbtree_node;                        \
    while (___Z_node_tmp) {                                        \
        cmp_node = ___Z_node_tmp;                                \
        {
#define ZRBTREE_LOOKUP_PART2(root, cmp_result, return_node)                        \
        }                                            \
        return_node = 0;                                    \
        if (cmp_result < 0) {                                    \
            ___Z_node_tmp = ___Z_node_tmp->zrbtree_left;                    \
        } else if (cmp_result > 0) {                                \
            ___Z_node_tmp = ___Z_node_tmp->zrbtree_right;                    \
        } else {                                        \
            return_node = ___Z_node_tmp;                            \
            break;                                        \
        }                                            \
    }                                                \
}

#define ZRBTREE_DETACH(root, node) {                                    \
    zrbtree_erase(root, node);                                    \
}

#define ZRBTREE_WALK_BEGIN(root, var_your_node) {                            \
    struct { zrbtree_node_t *node; unsigned char lrs; } ___Z_list[64];                \
    zrbtree_node_t *___Z_node = (root)->zrbtree_node;                            \
    int ___Z_idx = 0, ___Z_lrs;                                    \
    ___Z_list[0].node = ___Z_node;                                    \
    ___Z_list[0].lrs = 0;                                        \
    while (1) {                                            \
        var_your_node = ___Z_node = ___Z_list[___Z_idx].node;                    \
        ___Z_lrs = ___Z_list[___Z_idx].lrs;                            \
        if (!___Z_node || ___Z_lrs == 2) {                            \
            if (___Z_node) {
#define ZRBTREE_WALK_END                                        \
            }                                        \
            ___Z_idx--;                                    \
            if (___Z_idx == -1){                                \
                break;                                    \
                   }                                        \
            ___Z_list[___Z_idx].lrs++;                            \
            continue;                                    \
        }                                            \
        ___Z_idx++;                                        \
        ___Z_list[___Z_idx].lrs = 0;                                \
        ___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->zrbtree_left : ___Z_node->zrbtree_right);\
    }                                                \
}

#define ZRBTREE_WALK_FORWARD_BEGIN(root, var_your_node)     {                    \
    for (var_your_node = zrbtree_first(root); var_your_node; var_your_node = zrbtree_next(var_your_node)) {
#define ZRBTREE_WALK_FORWARD_END                }}

#define ZRBTREE_WALK_BACK_BEGIN(root, var_your_node)     {                    \
    for (var_your_node = zrbtree_last(root); var_your_node; var_your_node = zrbtree_prev(var_your_node)) {
#define ZRBTREE_WALK_BACK_END                }}

/* ################################################################## */
/* dichotomy */
void *zdichotomy_search(void *list, long element_size, int element_count, int (*cmp_fn) (void *, void *), void *key);

/* ################################################################## */
/* CHAIN */
struct zchain_t {
    zchain_node_t *head;
    zchain_node_t *tail;
    int len;
};
struct zchain_node_t {
    zchain_node_t *prev;
    zchain_node_t *next;
    void *value;
};
#define ZCHAIN_NEXT(n)   ((n)->next)
#define ZCHAIN_PREV(n)   ((n)->prev)
#define ZCHAIN_HEAD(c)   ((c)->head)
#define ZCHAIN_TAIL(c)   ((c)->tail)
#define ZCHAIN_LEN(c)    ((c)->len)
#define zchain_next(n)   ((n)->next)
#define zchain_prev(n)   ((n)->prev)
#define zchain_head(c)   ((c)->head)
#define zchain_tail(c)   ((c)->tail)
#define zchain_len(c)    ((c)->len)
zchain_t *zchain_create(void);
void zchain_free(zchain_t * chain, void (*free_fn) (void *, void *), void *ctx);
void zchain_free_STR(zchain_t * chain);
void zchain_walk(zchain_t * chain, void (*walk_fn) (zchain_node_t *, void *ctx), void *ctx);
int zchain_attach_before(zchain_t * chain, zchain_node_t * n, zchain_node_t * before);
int zchain_detach(zchain_t * chain, zchain_node_t * n);
zchain_node_t *zchain_add_before(zchain_t * chain, void *value, zchain_node_t * before);
zchain_node_t *zchain_delete(zchain_t * chain, zchain_node_t * n, char **value);
#define zchain_push(chain, value)           zchain_add_before(chain, value, 0)
#define zchain_unshift(chain, value)        zchain_add_before(chain, value, (chain)->head)
#define zchain_pop(chain, value)            zchain_delete(chain, (chain)->tail, value)
#define zchain_shift(chain, value)          zchain_delete(chain, (chain)->head, value)

#define ZCHAIN_WALK_BEGIN(chain, var_your_node)  {\
    var_your_node=(chain)->head;\
    for(;var_your_node;var_your_node=var_your_node->next){
#define ZCHAIN_WALK_END                          }}

/* ################################################################## */
/* DICT */
struct zdict_t {
    int len;
    zrbtree_t rbtree;
    zmpool_t *mpool;
};
struct zdict_node_t {
    char *key;
    char *value;
    zrbtree_node_t rbnode;
};
zdict_t *zdict_create(void);
zdict_t *zdict_create_mpool(zmpool_t * mpool);
zdict_node_t *zdict_add(zdict_t * dict, char *key, char *value);
zdict_node_t *zdict_lookup(zdict_t * dict, char *key, char **value);
zdict_node_t *zdict_lookup_near_prev(zdict_t * dict, char *key, char **value);
zdict_node_t *zdict_lookup_near_next(zdict_t * dict, char *key, char **value);
void zdict_delete_node(zdict_t * mp, zdict_node_t * n);
void zdict_delete(zdict_t * dict, char *key);
zdict_node_t *zdict_first(zdict_t * dict);
zdict_node_t *zdict_last(zdict_t * dict);
zdict_node_t *zdict_prev(zdict_node_t * node);
zdict_node_t *zdict_next(zdict_node_t * node);
void zdict_walk(zdict_t * dict, void (*walk_fn) (zdict_node_t *, void *), void *ctx);
void zdict_free(zdict_t * dict);
int zdict_keys(zdict_t * dict, char **key_list, int size);
#define zdict_len(dict)              ((dict)->len)
#define zdict_key(n)               ((n)->key)
#define zdict_value(n)             ((n)->value)
#define zdict_update_value(n, v)   ((n)->value = zstrdup((char *)(v)))
#define ZDICT_WALK_BEGIN(dict, n) { \
    for(n = zdict_first(dict); n; n = zdict_next(n)) {
#define ZDICT_WALK_END    }}

/* ################################################################## */
/* IDICT */
struct zidict_t {
    int len;
    zrbtree_t rbtree;
    zmpool_t *mpool;
};
struct zidict_node_t {
    long key;
    void *value;
    zrbtree_node_t rbnode;
};
zidict_t *zidict_create(void);
zidict_t *zidict_create_mpool(zmpool_t * mpool);
zidict_node_t *zidict_add(zidict_t * dict, long key, char *value);
zidict_node_t *zidict_lookup(zidict_t * dict, long key, char **value);
zidict_node_t *zidict_lookup_near_prev(zidict_t * dict, long key, char **value);
zidict_node_t *zidict_lookup_near_next(zidict_t * dict, long key, char **value);
void zidict_delete_node(zidict_t * mp, zidict_node_t * n);
void zidict_delete(zidict_t * dict, long key);
zidict_node_t *zidict_first(zidict_t * dict);
zidict_node_t *zidict_last(zidict_t * dict);
zidict_node_t *zidict_prev(zidict_node_t * node);
zidict_node_t *zidict_next(zidict_node_t * node);
void zidict_walk(zidict_t * dict, void (*walk_fn) (zidict_node_t *, void *), void *ctx);
void zidict_free(zidict_t * dict);
int zidict_keys(zidict_t * dict, long *key_list, int size);
#define zidict_len(dict)            ((dict)->len)
#define zidict_key(n)               ((n)->key)
#define zidict_value(n)             ((n)->value)
#define zidict_update_value(n, v)   ((n)->value = zstrdup((char *)(v)))
#define ZIDICT_WALK_BEGIN(idict, n) { \
    for(n = zidict_first(idict); n; n = zidict_next(n)) {
#define ZIDICT_WALK_END    }}

/* ################################################################## */
/* GRID */
struct zgrid_t {
    int len;
    zrbtree_t rbtree;
    zmpool_t *mpool;
};
struct zgrid_node_t {
    char *key;
    void *value;
    zrbtree_node_t rbnode;
};
zgrid_t *zgrid_create(void);
zgrid_t *zgrid_create_mpool(zmpool_t * mpool);
zgrid_node_t *zgrid_add(zgrid_t * mp, char *key, void *value, char **old_value);
zgrid_node_t *zgrid_lookup(zgrid_t * mp, char *key, char **value);
zgrid_node_t *zgrid_lookup_near_prev(zgrid_t * mp, char *key, char **value);
zgrid_node_t *zgrid_lookup_near_next(zgrid_t * mp, char *key, char **value);
void zgrid_delete_node(zgrid_t * mp, zgrid_node_t * n);
void zgrid_delete(zgrid_t * mp, char *key, char **old_value);
zgrid_node_t *zgrid_first(zgrid_t * mp);
zgrid_node_t *zgrid_last(zgrid_t * mp);
zgrid_node_t *zgrid_prev(zgrid_node_t * node);
zgrid_node_t *zgrid_next(zgrid_node_t * node);
void zgrid_walk(zgrid_t * mp, void (*walk_fn) (zgrid_node_t *, void *), void *ctx);
void zgrid_free(zgrid_t * mp, void (*free_fn) (zgrid_node_t *, void *), void *ctx);
int zgrid_keys(zgrid_t * mp, char **key_list, int size);
#define zgrid_len(mp)              ((mp)->len)
#define zgrid_key(n)               ((n)->key)
#define zgrid_value(n)             ((char *)((n)->value))
#define zgrid_set_value(n, v)      ((n)->value = (char *)(v))
#define ZGRID_WALK_BEGIN(grid, n) { \
    for(n = zgrid_first(grid); n; n = zgrid_next(n)) {
#define ZGRID_WALK_END    }}

/* ################################################################## */
/* igrid */
struct zigrid_t {
    int len;
    zrbtree_t rbtree;
    zmpool_t *mpool;
};
struct zigrid_node_t {
    long key;
    void *value;
    zrbtree_node_t rbnode;
};
zigrid_t *zigrid_create(void);
zigrid_t *zigrid_create_mpool(zmpool_t * mpool);
zigrid_node_t *zigrid_add(zigrid_t * mp, long key, void *value, char **old_value);
zigrid_node_t *zigrid_lookup(zigrid_t * mp, long key, char **value);
zigrid_node_t *zigrid_lookup_near_prev(zigrid_t * mp, long key, char **value);
zigrid_node_t *zigrid_lookup_near_next(zigrid_t * mp, long key, char **value);
void zigrid_delete_node(zigrid_t * mp, zigrid_node_t * n);
void zigrid_delete(zigrid_t * mp, long key, char **old_value);
zigrid_node_t *zigrid_first(zigrid_t * mp);
zigrid_node_t *zigrid_last(zigrid_t * mp);
zigrid_node_t *zigrid_prev(zigrid_node_t * node);
zigrid_node_t *zigrid_next(zigrid_node_t * node);
void zigrid_walk(zigrid_t * mp, void (*walk_fn) (zigrid_node_t *, void *), void *ctx);
void zigrid_free(zigrid_t * mp, void (*free_fn) (zigrid_node_t *, void *), void *ctx);
int zigrid_keys(zigrid_t * mp, long *key_list, int size);
#define zigrid_len(mp)              ((mp)->len)
#define zigrid_key(n)               ((n)->key)
#define zigrid_value(n)             ((char *)((n)->value))
#define zigrid_set_value(n, v)      ((n)->value = (char *)(v))
#define ZIGRID_WALK_BEGIN(igrid, n) { \
    for(n = zigrid_first(igrid); n; n = zigrid_next(n)) {
#define ZIGRID_WALK_END    }}

/* ################################################################## */
/* CONFIG */
extern zconfig_t *zvar_config;
void zvar_config_init(void);
#define zconfig_create  zdict_create
#define zconfig_free    zdict_free
#define zconfig_add     zdict_add
#define zconfig_update  zdict_add
#define zconfig_delete  zdict_delete
void zconfig_show(zconfig_t * cf);
#define ZCONFIG_WALK_BEGIN(cf, key, value)  {\
    zdict_node_t * ___node_dict_config_1219; \
    ZDICT_WALK_BEGIN(cf, ___node_dict_config_1219) {\
        key = zdict_key(___node_dict_config_1219); \
        value = (char *)zdict_value(___node_dict_config_1219);
#define ZCONFIG_WALK_END   } ZDICT_WALK_END; }

/* config load */
int zconfig_load(zconfig_t * cf, char *filename);

/* config value */
typedef struct {
    char *name;
    char *defval;
    char **target;
} zconfig_str_table_t;
typedef struct {
    char *name;
    int defval;
    int *target;
    int min;
    int max;
} zconfig_int_table_t;
typedef struct {
    char *name;
    long defval;
    long *target;
    long min;
    long max;
} zconfig_long_table_t;
#define zconfig_bool_table_t zconfig_int_table_t
#define zconfig_second_table_t zconfig_long_table_t
#define zconfig_size_table_t zconfig_long_table_t
char *zconfig_get_str(zconfig_t * cf, char *name, char *def);
int zconfig_get_bool(zconfig_t * cf, char *name, int def);
int zconfig_get_int(zconfig_t * cf, char *name, int def, int min, int max);
long zconfig_get_long(zconfig_t * cf, char *name, long def, long min, long max);
long zconfig_get_second(zconfig_t * cf, char *name, long def, long min, long max);
long zconfig_get_size(zconfig_t * cf, char *name, long def, long min, long max);
void zconfig_get_str_table(zconfig_t * cf, zconfig_str_table_t * table);
void zconfig_get_int_table(zconfig_t * cf, zconfig_int_table_t * table);
void zconfig_get_long_table(zconfig_t * cf, zconfig_long_table_t * table);
void zconfig_get_bool_table(zconfig_t * cf, zconfig_bool_table_t * table);
void zconfig_get_second_table(zconfig_t * cf, zconfig_second_table_t * table);
void zconfig_get_size_table(zconfig_t * cf, zconfig_size_table_t * table);

/* ################################################################## */
/* kvlist */
zkvlist_t *zkvlist_create(char *path);
void zkvlist_free(zkvlist_t * kv);
int zkvlist_begin(zkvlist_t * kv);
int zkvlist_end(zkvlist_t * kv);
int zkvlist_changed(zkvlist_t * kv);
int zkvlist_add(zkvlist_t * kv, char *key, char *value);
int zkvlist_delete(zkvlist_t * kv, char *key);
int zkvlist_lookup(zkvlist_t * kv, char *key, char **value);
int zkvlist_load(zkvlist_t * kv);
zdict_t *zkvlist_get_dict(zkvlist_t * kv);

/* ################################################################## */

struct zmap_node_t {
	int (*query) (zmap_node_t * node, char *query, zbuf_t *result, int timeout);
	int (*close) (zmap_node_t * node);
    char *title;
    int used;
};

struct zmap_t {
   zmap_node_t **node_list;
   int node_len;
};

extern int zvar_map_pthread_mode;
extern zgrid_t *zvar_map_node_list;
int zmap_main(int argc, char **argv);
zmap_t *zmap_create(char *map_string, int flags_unused);
int zmap_close(zmap_t * zm);
int zmap_query(zmap_t * zm, char *query, zbuf_t *result, int timeout);
#define zmap_get_error(zm)      ((zm)->error_info)
int zmap_read_line(zstream_t *fp, char *buf, int len, int *reach_end);
int zmap_title_split(char *opstr, char **list);

/* ################################################################## */
/* IO or FD's stat */
#define ZEV_NONE              0x00
#define ZEV_READ              0x01
#define ZEV_WRITE             0x02
#define ZEV_RDWR              0x03
#define ZEV_PERSIST           0x04
/* exception */
#define ZEV_RDHUP             0x10
#define ZEV_HUP               0x20
#define ZEV_ERROR             0x40
#define ZEV_ERRORS            0x70
#define ZEV_TIMEOUT           0x80
#define ZEV_EXCEPTION         0xF0

/* io */
int zrwable(int fd);
int zreadable(int fd);
int zwriteable(int fd);
int zread_wait(int fd, int timeout);
int ztimed_read(int fd, void *buf, int len, int timeout);
int ztimed_strict_read(int fd, void *buf, int len, int timeout);
int zwrite_wait(int fd, int timeout);
int ztimed_write(int fd, void *buf, int len, int timeout);
int ztimed_strict_write(int fd, void *buf, int len, int timeout);
int zflock(int fd, int flags);
int znonblocking(int fd, int on);
#define zblocking(fd, on)   znonblocking(fd, 1-on)
int zclose_on_exec(int fd, int on);
int zpeek(int fd);

/* ################################################################## */
/* simple dns utils */
struct zaddr_t {
    char addr[16];
};
int zgetlocaladdr(zaddr_t * addr_list, int max_count);
int zgetaddr(char *host, zaddr_t * addr_list, int max_count);
int zgetpeer(int sockfd, int *host, int *port);

/* ################################################################## */
/* socket */
int zunix_connect(char *addr, int timeout);
int zinet_connect(char *dip, int port, int timeout);
int zhost_connect(char *host, int port, int timeout);
int zunix_listen(char *addr, int backlog);
int zinet_listen(char *sip, int port, int backlog);
int zfifo_listen(char *path);
int zunix_accept(int fd);
int zinet_accept(int fd);
int zconnect(char *netpath, int timeout);
int zlisten(char *netpath, int backlog);

/* ################################################################## */
/* parameter deal */
typedef int (*zparameter_fn_t) (int, char **);
typedef struct {
    char *name;
    zparameter_fn_t func;
} zparameter_pair_t;
int zparameter_run(int argc, char **argv, zparameter_fn_t param_fn);
int zparameter_run_dict(int argc, char **argv, zdict_t * param_fn_dict);
int zparameter_run_list(int argc, char **argv, zparameter_pair_t * param_fn_list);
int zparameter_run_test(int argc, char **argv);

/* ################################################################## */
struct zsslctx_t {
    void *ssl_ctx;              /* SSL_CTX* */
    int server_or_client;
};
struct zssl_t {
    void *ssl;                  /* SSL* */
    int fd;
    int server_or_client;
};
int zssl_INIT(int unused_flags);
zsslctx_t *zsslctx_server_create(int unused_flags);
zsslctx_t *zsslctx_client_create(int unused_flags);
int zsslctx_set_cert(zsslctx_t * ssl_ctx, char *cert_file, char *key_file);
void zsslctx_free(zsslctx_t * ctx);
void zssl_get_error(unsigned long *errcode, char *buf, int buf_len);
zssl_t *zssl_create(zsslctx_t * ctx, int fd);
void zssl_free(zssl_t * ssl);
void *zssl_detach_ssl(zssl_t * zssl);
int zssl_connect(zssl_t * ssl, int timeout);
int zssl_accept(zssl_t * ssl, int timeout);
int zssl_shutdown(zssl_t * ssl, int timeout);
int zssl_read(zssl_t * ssl, void *buf, int len, int timeout);
int zssl_write(zssl_t * ssl, void *buf, int len, int timeout);
void *___zopenssl_create(zsslctx_t * ctx, int fd);
#define zopenssl_create(ctx, fd)   ((SSL *)___zopenssl_create(ctx, fd))
void zopenssl_free(void *ssl);  /* SSL * */

/* ################################################################## */
/* file vstream */
#define ZFILE_RBUF_SIZE           4096
#define ZFILE_WBUF_SIZE           4096

typedef int (*zstream_read_t) (zstream_t *, void *, int, int);
typedef int (*zstream_write_t) (zstream_t *, void *, int, int);
struct zstream_t {
    int read_buf_p1;
    int read_buf_p2;
    int write_buf_len;
    unsigned int error:1;
    unsigned int eof:1;
    char read_buf[ZFILE_WBUF_SIZE + 1];
    char write_buf[ZFILE_WBUF_SIZE + 1];
    zstream_read_t read_fn;
    zstream_write_t write_fn;
    void *io_ctx;
    long timeout;
};
#define ZFGETCHAR(fp)        (((fp)->read_buf_p1<(fp)->read_buf_p2)\
    ?((int)((fp)->read_buf[(fp)->read_buf_p1++])):(zfgetchar(fp)))
#define ZFGET                ZFGETCHAR
#define ZFGETC               ZFGETCHAR

#define ZFFLUSH(fp)          (((fp)->write_buf_len>0)?(zfflush(fp)):(0))
#define ZFPUTCHAR(fp, ch)    (((fp)->write_buf_len<ZFILE_WBUF_SIZE)\
        ?((fp)->write_buf[(fp)->write_buf_len++]=(ch)):(zfputchar(fp, ch)))
#define ZFPUT                ZFPUTCHAR
#define ZFPUTC               ZFPUTCHAR

#define ZFERROR(fp)          ((fp)->error)
#define ZFEOF(fp)            ((fp)->eof)
#define ZFEXCEPTION(fp)      ((fp)->eof || (fp)->error)
#define ZFCTX(fp)            ((fp)->io_ctx)
#define zferror(fp)          ((fp)->error)
#define zfeof(fp)            ((fp)->eof)
#define zfexception(fp)      ((fp)->eof || (fp)->error)
#define zfctx(fp)            ((fp)->io_ctx)
#define zfileno(fp)          (ZVOID_PTR_TO_INT((fp)->ctx))

zstream_t *zstream_create(int unused);
void *zstream_free(zstream_t * fp);
void zfset_ioctx(zstream_t * fp, void *io_ctx, zstream_read_t read_fn, zstream_write_t write_fn);
void zfset_timeout(zstream_t * fp, int timeout);

zstream_t *zfopen_FD(int fd);
int zfclose_FD(zstream_t * fp);
#define zfdopen              zfopen_FD
#define zfdclose             zfclose_FD

zstream_t *zfopen_SSL(zssl_t * ssl);
zssl_t *zfclose_SSL(zstream_t * fp);

int zfgetchar(zstream_t * fp);
#define zfget                zfgetchar
#define zfgetc               zfgetchar
int zfread(zstream_t * fp, void *buf, int len);
int zfread_n(zstream_t * fp, void *buf, int len);
int zfread_delimiter(zstream_t * fp, void *buf, int len, char delimiter);
#define zfread_line(fp, buf, len)   zfread_delimiter(fp, buf, len, '\n')
int zfgets_n(zstream_t * fp, zbuf_t * bf, int len);
int zfgets_delimiter(zstream_t * fp, zbuf_t * bf, char delimiter);
#define zfgets(fp, bf)              zfgets_delimiter(fp, bf, '\n')

int zfflush(zstream_t * fp);
void zfputchar(zstream_t * fp, int ch);
#define zfput                zfputchar
#define zfputc               zfputchar
int zfwrite_n(zstream_t * fp, void *buf, int len);
int zfputs(zstream_t * fp, char *s);
int zfprintf(zstream_t * fp, char *format, ...);

/* ################################################################## */
/* alarm */
typedef int (*zalarm_cb_t) (zalarm_t *);
struct zalarm_t {
    unsigned int in_time:1;
    unsigned int enable_time:1;
    unsigned int auto_release:1;
    long timeout;
    zalarm_cb_t callback;
    void *context;
    zrbtree_node_t rbnode_time;
};
void zalarm_use_lock(void);
void zalarm_set_sig(int sig);
void zalarm_env_init(void);
void zalarm_env_fini(void);
void zalarm_init(zalarm_t * alarm);
void zalarm_fini(zalarm_t * alarm);
zalarm_t *zalarm_create(void);
void zalarm_free(zalarm_t * alarm);
void zalarm_set(zalarm_t * alarm, zalarm_cb_t callback, long timeout);
void zalarm_stop(zalarm_t * alarm);
void zalarm_continue(zalarm_t * alarm);
void zalarm(zalarm_cb_t callback, void *context, long timeout);
#define zalarm_set_context(alarm,  ctx)     ((alarm)->context = ctx)
#define zalarm_get_context(alarm)   ((alarm)->context)

/* ################################################################## */
/* MPOOL */
struct zmpool_t {
    void *worker;
    void *(*malloc) (zmpool_t *, int);
    void *(*realloc) (zmpool_t *, void *, int);
    void (*free) (zmpool_t *, void *);
    void (*reset) (zmpool_t *);
    void (*free_pool) (zmpool_t *);
};

zmpool_t *zmpool_create_default_pool(int *register_list);
zmpool_t *zmpool_create_spring_pool(int element_size);
zmpool_t *zmpool_create_grow_pool(void);
void zmpool_free_pool(zmpool_t * mp);

void *zmpool_malloc(zmpool_t * mp, int len);
void *zmpool_calloc(zmpool_t * mp, int nmemb, int size);
void *zmpool_realloc(zmpool_t * mp, void *ptr, int len);
void *zmpool_strdup(zmpool_t * mp, char *ptr);
void *zmpool_strndup(zmpool_t * mp, char *ptr, int n);
void *zmpool_memdup(zmpool_t * mp, void *ptr, int n);
void zmpool_reset(zmpool_t * mp);
void zmpool_free(zmpool_t * mp, void *ptr);

/* ################################################################## */
/* MCOT */
struct zmcot_t {
    zmpool_t *mpool;
    int element_size;
};
zmcot_t *zmcot_create(int element_size);
void zmcot_free(zmcot_t * cot);
void *zmcot_alloc_one(zmcot_t * cot);
void zmcot_free_one(zmcot_t * cot, void *ptr);


/* ################################################################## */
/* EVENT AIO */
#define ZEV_TYPE_EVENT         0x1
#define ZEV_TYPE_AIO           0x2

typedef int (*zev_cb_t) (zev_t *);
struct zev_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char events;
    unsigned char recv_events;
    int fd;
    zev_cb_t callback;
    void *context;
    zevbase_t *evbase;
};
#define zev_get_base(ev)                 ((ev)->evbase)
#define zev_get_fd(ev)                   ((ev)->fd)
#define zev_get_events(ev)               ((ev)->recv_events)
#define zev_set_context(ev, ctx)         ((ev)->context=(ctx))
#define zev_get_context(ev)              ((ev)->context)
#define zev_get_callback(ev)             ((ev)->callback)
zev_t *zev_create(void);
void zev_free(zev_t * ev);
void zev_init(zev_t * ev, zevbase_t * eb, int fd);
void zev_fini(zev_t * tg);
int zev_set(zev_t * ev, int events, zev_cb_t callback);
#define zev_read(ev, callback)           (zev_set(ev, ZEV_READ, callback))
#define zev_write(ev, callback)          (zev_set(ev, ZEV_WRITE, callback))
#define zev_rwable(ev, callback)         (zev_set(ev, ZEV_READ|ZEV_WRITE, callback)
int zev_unset(zev_t * ev);

/* AIO */
typedef struct zaio_rwbuf_t zaio_rwbuf_t;
typedef struct zaio_rwbuf_list_t zaio_rwbuf_list_t;
#define ZAIO_RWBUF_SIZE            10240
struct zaio_rwbuf_t {
    zaio_rwbuf_t *next;
    unsigned int p1:16;
    unsigned int p2:16;
    char data[ZAIO_RWBUF_SIZE];
};
struct zaio_rwbuf_list_t {
    zaio_rwbuf_t *head;
    zaio_rwbuf_t *tail;
    int len;
};
typedef int (*zaio_cb_t) (zaio_t *);
typedef struct zaio_ssl_t zaio_ssl_t;
struct zaio_ssl_t {
    unsigned char server_or_client:1;
    unsigned char session_init:1;
    unsigned char read_want_read:1;
    unsigned char read_want_write:1;
    unsigned char write_want_read:1;
    unsigned char write_want_write:1;
    unsigned char error:1;
    void *ssl;                  /* SSL* */
};
struct zaio_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char in_time:1;
    unsigned char enable_time:1;
    unsigned char want_read:1;
    unsigned char events;
    unsigned char recv_events;
    unsigned char rw_type;
    char delimiter;
    int fd;
    int read_magic_len;
    int ret;
    zaio_cb_t callback;
    void *context;
    zaio_rwbuf_list_t read_cache;
    zaio_rwbuf_list_t write_cache;
    long timeout;
    zrbtree_node_t rbnode_time;
    zevbase_t *evbase;
    zaio_ssl_t *ssl;
    zaio_t *queue_prev;
    zaio_t *queue_next;
};

#define zaio_get_base(aio)          ((aio)->evbase)
#define zaio_get_fd(aio)            ((aio)->fd)
#define zaio_get_ret(aio)           ((aio)->ret)
#define zaio_get_callback(aio)      ((aio)->callback)
#define zaio_set_context(aio, ctx)         ((aio)->context=(ctx))
#define zaio_get_context(aio)              ((aio)->context)
zaio_t *zaio_create(void);
void zaio_free(zaio_t * aio);
void zaio_init(zaio_t * aio, zevbase_t * eb, int fd);
void zaio_fini(zaio_t * aio);
void zaio_set_local_mode(zaio_t * aio);
int zaio_fetch_rbuf(zaio_t * aio, char *buf, int len);
int zaio_attach(zaio_t * aio, zaio_cb_t callback);
int zaio_read(zaio_t * aio, int max_len, zaio_cb_t callback, int timeout);
int zaio_read_n(zaio_t * aio, int strict_len, zaio_cb_t callback, int timeout);
int zaio_read_delimiter(zaio_t * aio, char delimiter, int max_len, zaio_cb_t callback, int timeout);
#define zaio_read_line(a,c,d,e) zaio_read_delimiter(a,'\n',c,d,e)
int zaio_printf(zaio_t * aio, char *fmt, ...);
int zaio_puts(zaio_t * aio, char *s);
int zaio_write_cache_append(zaio_t * aio, void *buf, int len);
int zaio_write_cache_flush(zaio_t * aio, zaio_cb_t callback, int timeout);
int zaio_write_cache_get_len(zaio_t * aio);
int zaio_sleep(zaio_t * aio, zaio_cb_t callback, int timeout);

int zaio_ssl_init(zaio_t * aio, zsslctx_t * ctx, zaio_cb_t callback, int timeout);
void zaio_ssl_fini(zaio_t * aio);
int zaio_ssl_attach(zaio_t * aio, zaio_ssl_t * zssl);
zaio_ssl_t *zaio_ssl_detach(zaio_t * aio);
void *___zaio_ssl_detach_ssl(zaio_ssl_t * assl);    /* return SSL* */
#define zaio_ssl_detach_ssl(assl)   ((SSL *)(___zaio_ssl_detach_ssl(assl)))

/* TIMER */
typedef int (*zevtimer_cb_t) (zevtimer_t *);
struct zevtimer_t {
    long timeout;
    zevtimer_cb_t callback;
    void *context;
    zrbtree_node_t rbnode_time;
    unsigned char in_time:1;
    zevbase_t *evbase;
};

#define zevtimer_get_base(timer)          ((timer)->evbase)
#define zevtimer_set_context(timer, ctx)    ((timer)->context=(ctx))
#define zevtimer_get_context(timer)         ((timer)->context)
zevtimer_t *zevtimer_create(void);
void zevtimer_free(zevtimer_t * timer);
void zevtimer_init(zevtimer_t * timer, zevbase_t * eb);
void zevtimer_fini(zevtimer_t * timer);
int zevtimer_start(zevtimer_t * timer, zevtimer_cb_t callback, int timeout);
int zevtimer_stop(zevtimer_t * timer);

/* BASE */
extern zevbase_t *zvar_evbase;
typedef int (*zevbase_cb_t) (zevbase_t *);
typedef int (*zevbase_loop_t) (zevbase_t *);

struct zevbase_t {
    int epoll_fd;
    struct epoll_event *epoll_event_list;
    void *locker;
    zrbtree_t general_timer_tree;
    zrbtree_t aio_timer_tree;
    zev_t eventfd_event;
    void *context;
    zevbase_loop_t loop_fn;
    zmcot_t *aio_rwbuf_mpool;

    zaio_t *queue_head;
    zaio_t *queue_tail;

    zaio_t *extern_queue_head;
    zaio_t *extern_queue_tail;
};
void zvar_evbase_init(void);
int zevbase_notify(zevbase_t * eb);
void zevbase_single_mode(void);
zevbase_t *zevbase_create(void);
void zevbase_free(zevbase_t * eb);
int zevbase_dispatch(zevbase_t * eb, long delay);
#define zevbase_set_context(eb, ctx)        ((eb)->context=(ctx))
#define zevbase_get_context(eb)             ((eb)->context)

/* ################################################################## */
/* iopipe */
typedef void (*ziopipe_after_close_fn_t) (void *);
ziopipe_base_t *ziopipe_base_create(void);
void ziopipe_base_free(ziopipe_base_t * iopb);
void ziopipe_base_set_break(ziopipe_base_t * iopb);
int ziopipe_base_run(ziopipe_base_t * iopb);
/* void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL * client_ssl, int server_fd, SSL * server_ssl); */
void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, void *client_ssl, int server_fd, void *server_ssl, ziopipe_after_close_fn_t after_close, void *context);

/* ################################################################## */
/* server master */
#define ZMASTER_LISTEN_INET            'i'
#define ZMASTER_LISTEN_UNIX            'u'
#define ZMASTER_LISTEN_FIFO            'f'
#define ZMASTER_SERVER_STATUS_FD    3
#define ZMASTER_MASTER_STATUS_FD    4
#define ZMASTER_LISTEN_FD            5

typedef int (*zmaster_load_config_fn_t) (zarray_t * config_list);
extern zmaster_load_config_fn_t zmaster_load_config_fn;
extern int zvar_master_child_exception_check;
void zmaster_load_config_default(zarray_t * config_list);
int zmaster_main(int argc, char **argv);

typedef void (*zmaster_server_service_t) (int fd);
typedef void (*zmaster_server_cb_t) (void);
extern int zvar_master_server_listen_fd;
extern int zvar_master_server_listen_type;
extern zmaster_server_service_t zmaster_server_service;
extern zmaster_server_cb_t zmaster_server_before_service;
extern zmaster_server_cb_t zmaster_server_reload;
extern zmaster_server_cb_t zmaster_server_loop;
extern zmaster_server_cb_t zmaster_server_before_exit;
int zmaster_server_main(int argc, char **argv);
void zmaster_server_stop_notify(void);
void zmaster_server_disconnect(int fd);

/* ################################################################## */
int zncr_decode(int ins, char *wchar);
/* base64 */
int zbase64_encode_to_df(void *src, int src_size, void *filter, int filter_type, int mime_flag);
#define zbase64_encode zbase64_encode_to_df
#define zbase64_encode_to_zbuf(src, src_size, filter, mime_flag) \
    zbase64_encode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZBUF, mime_flag)
#define zbase64_encode_to_file(src, src_size, filter, mime_flag) \
    zbase64_encode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_FILE, mime_flag)
#define zbase64_encode_to_zstream(src, src_size, filter, mime_flag) \
    zbase64_encode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZSTREAM, mime_flag)

int zbase64_decode_to_df(void *src, int src_size, void *filter, int filter_type);
#define zbase64_decode zbase64_decode_to_df
#define zbase64_decode_to_zbuf(src, src_size, filter) \
    zbase64_decode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZBUF)
#define zbase64_decode_to_file(src, src_size, filter) \
    zbase64_decode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_FILE)
#define zbase64_decode_to_zstream(src, src_size, filter) \
    zbase64_decode_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZSTREAM)

int zbase64_decode_validate(void *src, int src_size, int *valid_len);
int zbase64_encode_get_min_len(int in_len, int mime_flag);

/* quoted_printable */
int zquoted_printable_decode_2045_to_df(void *src, int src_size, void *filter, int filter_type);
int zquoted_printable_decode_2047_to_df(void *src, int src_size, void *filter, int filter_type);
int zquoted_printable_decode_validate(void *src, int src_size, int *valid_len);
#define zqp_decode_validate      zquoted_printable_decode_validate
#define zqp_decode_2045 zquoted_printable_decode_2045_to_df
#define zqp_decode_2045_to_df zquoted_printable_decode_2045_to_df
#define zqp_decode_2045_to_zbuf(src, src_size, filter) \
    zquoted_printable_decode_2045_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZBUF)
#define zqp_decode_2045_to_file(src, src_size, filter) \
    zquoted_printable_decode_2045_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_FILE)
#define zqp_decode_2045_to_zstream(src, src_size, filter) \
    zquoted_printable_decode_2045_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZSTREAM)

#define zqp_decode_2047 zquoted_printable_decode_2047_to_df
#define zqp_decode_2047_to_df zquoted_printable_decode_2047_to_df
#define zqp_decode_2047_to_zbuf(src, src_size, filter) \
    zquoted_printable_decode_2047_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZBUF)
#define zqp_decode_2047_to_file(src, src_size, filter) \
    zquoted_printable_decode_2047_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_FILE)
#define zqp_decode_2047_to_zstream(src, src_size, filter) \
    zquoted_printable_decode_2047_to_df(src, src_size, filter, ZDATA_FILTER_TYPE_ZSTREAM)

/* hex */
extern char zhex_to_dec_list[256];
int zhex_encode(void *src, int src_size, void *dest);
int zhex_decode(void *src, int src_size, void *dest);

/* crc */
unsigned int zcrc32(void *data, int size, unsigned int init_value);
unsigned long zcrc64(void *data, int size, unsigned long init_value);

/* md5 */
struct zmd5_t {
    uint_fast32_t lo, hi;
    uint_fast32_t a, b, c, d;
    unsigned char buffer[64];
    uint_fast32_t block[16];
};
void zmd5_init(zmd5_t * ctx);
void zmd5_update(zmd5_t * ctx, void *data, int size);
void zmd5_final(zmd5_t * ctx, void *result);    //result'size >= 16
char *zmd5(void *data, int size, void *result);

/* sha1 */
struct zsha1_t {
    union {
        uint8_t b8[20];
        uint32_t b32[5];
    } h;
    union {
        uint8_t b8[8];
        uint64_t b64[1];
    } c;
    union {
        uint8_t b8[64];
        uint32_t b32[16];
    } m;
    uint8_t count;
};
void zsha1_init(zsha1_t * ctx);
void zsha1_update(zsha1_t * ctx, void *input, int len);
void zsha1_result(zsha1_t * ctx, void *result);
char *zsha1(void *data, int size, void *result);

/* ################################################################## */
/* file */
int zfile_get_size(char *filename);
int zfile_put_contents(char *filename, void *data, int len);
int zfile_get_contents(char *filename, void *data, int len);
int zfile_get_contents_to_zbuf(char *filename, zbuf_t * bf);

/* mmap reader */
struct zmmap_reader {
    int fd;
    int len;
    char *data;
};
int zmmap_reader_init(zmmap_reader * reader, char *filename);
int zmmap_reader_fini(zmmap_reader * reader);

/* ################################################################## */
/* general data stream macro */
#define ZDATA_FILTER_TYPE_ZBUF          -2
#define ZDATA_FILTER_TYPE_FILE          -3
#define ZDATA_FILTER_TYPE_ZSTREAM       -4

#define ZDATA_FILTER_BUF(filter, type) \
    char filter ## ___FILTER_CACHE_BUF___1223[4100]; \
    int filter ## ___FILTER_CACHE_LEN___1223 = 0;\
    int filter ## ___FILTER_CACHE_TYPE___1223 = type;

#define ZDATA_FILTER_PUTC(filter, ch) \
{ \
    if (filter ## ___FILTER_CACHE_LEN___1223 > 4096) {\
        zdata_filter_write(filter, filter ##___FILTER_CACHE_TYPE___1223, \
            filter ## ___FILTER_CACHE_BUF___1223, filter ## ___FILTER_CACHE_LEN___1223); \
        filter ## ___FILTER_CACHE_LEN___1223 = 0; \
    } \
    filter ## ___FILTER_CACHE_BUF___1223[filter ## ___FILTER_CACHE_LEN___1223++] = (ch); \
}

#define ZDATA_FILTER_FLUSH(filter) { \
    if(filter ## ___FILTER_CACHE_LEN___1223 > 0) { \
        zdata_filter_write(filter, filter ##___FILTER_CACHE_TYPE___1223, \
            filter ## ___FILTER_CACHE_BUF___1223, filter ## ___FILTER_CACHE_LEN___1223); \
        filter ## ___FILTER_CACHE_LEN___1223 = 0; \
    }\
}

void zdata_filter_write(void *filter, int type, void *data, int len);

/* ################################################################## */
/* CHARSET */
#define ZCHARSET_ICONV_ERROR_OPEN       (-2016)
struct zcharset_iconv_t {
    char *to_charset;
    char *from_charset;
    char *in_str;
    int in_len;
    int in_converted_len;
    void *filter;
    int filter_type;
    int omit_invalid_bytes;
    int omit_invalid_bytes_count;
    iconv_t ic;
    char *to_charset_regular;
    char *from_charset_regular;
    int charset_regular;
    char *default_charset;
    char *out_str_runing;
    int out_len_runing;
};

#define ZICONV_CREATE(ic)   \
    zcharset_iconv_t ic ## ___ZICONV_BUF___1224___; \
    zcharset_iconv_t *ic = &ic ## ___ZICONV_BUF___1224___; \
    zcharset_iconv_init(ic);

#define ZICONV_FREE(ic) zcharset_iconv_fini(ic)

void zcharset_iconv_init(zcharset_iconv_t * ic);
void zcharset_iconv_fini(zcharset_iconv_t * ic);
char *zcharset_correct_charset(char *charset, char *default_charset);
int zcharset_detect(char *data, int len, char *charset_ret, char **charset_list);
int zcharset_iconv(zcharset_iconv_t * ic);

extern char *zvar_charset_chinese[];
extern char *zvar_charset_japanese[];
extern char *zvar_charset_korean[];
extern char *zvar_charset_cjk[];
#define zcharset_detect_chinese(d, l, ret) zcharset_detect(d, l, ret, zvar_charset_chinese)
#define zcharset_detect_japanese(d, l, ret) zcharset_detect(d, l, ret, zvar_charset_japanese)
#define zcharset_detect_korean(d, l, ret) zcharset_detect(d, l, ret, zvar_charset_korean)
#define zcharset_detect_cjk(d, l, ret) zcharset_detect(d, l, ret, zvar_charset_cjk)

/* ################################################################## */
/* MAIL PARSER */

#define ZMAIL_HEADER_LINE_MAX_LENGTH            10240

#define ZMAIL_PARSER_MIME_TYPE_MULTIPART        1
#define ZMAIL_PARSER_MIME_TYPE_ATTACHMENT       2
#define ZMAIL_PARSER_MIME_TYPE_PLAIN            3
#define ZMAIL_PARSER_MIME_TYPE_HTML             4

struct zmail_references_t {
    char *message_id;
    zmail_references_t *next;
};

struct zmail_addr_t {
    char *name;
    char *name_rd;
    char *mail;
    char *adl;
    zmail_addr_t *next;
};

struct zmail_header_line_t {
    char *name;
    char *line;
    int line_len;
    zmail_header_line_t *next;
};

struct zmail_mime_t {
    char *type;
    char *encoding;
    char *charset;
    char *disposition;
    char *name;
    char *name_rd;
    char *filename;
    char *filename_star;
    char *filename_rd;
    char *content_id;
    char *boundary;
    short int boundary_len;
    /* mime proto, for imapd */
    char *section;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    short int mime_type;
    short int is_tnef;

    /* mime original header-logic-line */
    zmail_header_line_t *header_head;
    zmail_header_line_t *header_tail;

    /* relationship */
    zmail_mime_t *next;
    zmail_mime_t *child;
    zmail_mime_t *parent;
    zmail_mime_t *all_next;

    /* user's data that be maintained self */
    void *user_context;
};

struct zmail_parser_t {
    int header_len;
    char *subject;
    char *subject_rd;
    char *date;
    long date_unix;
    zmail_addr_t *from;
    zmail_addr_t *sender;
    zmail_addr_t *reply_to;
    zmail_addr_t *to;
    zmail_addr_t *cc;
    zmail_addr_t *bcc;
    zmail_addr_t *receipt;
    char *in_reply_to;
    char *message_id;
    zmail_references_t *references;

    /* mime-tree */
    zmail_mime_t *mime;

    /* text(plain,html) type mime-list except for attachment */
    zmail_mime_t **text_mime_list;
    short int text_mime_count;

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    zmail_mime_t **view_mime_list;
    short int view_mime_count;

    /* attachment(and background-image) type mime-list */
    zmail_mime_t **attachment_mime_list;
    short int attachment_mime_count;

    char default_src_charset[32];
    char default_dest_charset[32];
    short int mime_max_depth;

    /* other */
    zmpool_t *mpool;
    char *mail_data;
    char *mail_pos;
    int mail_size;
};

extern int zmail_parser_only_test_parse;

zmail_parser_t *zmail_parser_create(char *mail_data, int mail_data_len);
void zmail_parser_free(zmail_parser_t * parser);
zmail_parser_t *zmail_parser_create_mpool(zmpool_t * imp, char *mail_data, int mail_data_len);
int zmail_parser_set_default_charset(zmail_parser_t * paser, char *default_src_charset, char *default_dest_charset);
int zmail_parser_set_mime_max_depth(zmail_parser_t * parser, int length);
int zmail_parser_run(zmail_parser_t * paser);
void zmail_parser_show(zmail_parser_t * parser);
void zmail_parser_show_json(zmail_parser_t * parser, zbuf_t * result);
int zmail_parser_iconv(zmail_parser_t * parser, char *from_charset, char *in, int in_len, void *filter, int filter_type);
int zmail_parser_decode_mime_body_to_df(zmail_parser_t * parser, zmail_mime_t * mime, void *filter, int filter_type);
#define zmail_parser_decode_mime_body zmail_parser_decode_mime_body_to_df
#define zmail_parser_decode_mime_body_to_zbuf(parser, mime, filter) \
    zmail_parser_decode_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_ZBUF)
#define zmail_parser_decode_mime_body_to_file(parser, mime, filter) \
    zmail_parser_decode_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_FILE)
#define zmail_parser_decode_mime_body_to_zstream(parser, mime, filter) \
    zmail_parser_decode_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_ZSTREAM)

int zmail_parser_decode_text_mime_body_to_df(zmail_parser_t * parser, zmail_mime_t * mime, void *filter, int filter_type);
#define zmail_parser_decode_text_mime_body zmail_parser_decode_text_mime_body_to_df
#define zmail_parser_decode_text_mime_body_to_zbuf(parser, mime, filter) \
    zmail_parser_decode_text_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_ZBUF)
#define zmail_parser_decode_text_mime_body_to_file(parser, mime, filter) \
    zmail_parser_decode_text_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_FILE)
#define zmail_parser_decode_text_mime_body_to_zstream(parser, mime, filter) \
    zmail_parser_decode_text_mime_body_to_df(parser, mime, filter, ZDATA_FILTER_TYPE_ZSTREAM)

int zmail_parser_decode_tnef(zmail_parser_t * parser, char *tnef_data, int tnef_len, zmail_mime_t ** mime_tree);

#define ZMAIL_PARSER_MIME_WALK_BEGIN(mime_top, var_your_node) {\
    zmail_mime_t *___Next_0611; \
    for (var_your_node = (mime_top); var_your_node; var_your_node = ___Next_0611) { \
        ___Next_0611 = var_your_node->all_next;  {
#define ZMAIL_PARSER_MIME_WALK_END                }}}

/* for dev */
int zmail_parser_mimetrim_dup(zmail_parser_t * parser, char *in_src, int in_len, char *out);
int zmail_parser_save_header(zmail_parser_t * parser, zmail_mime_t * cmime, char *line, int len);

int zmail_parser_2231_decode_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src);
int zmail_parser_addr_decode(zmail_parser_t * parser, char *str, int len, zmail_addr_t ** maddr);
int zmail_parser_decode_mime(zmail_parser_t * parser, zmail_mime_t * pmime, zmail_mime_t * cmime, char *buf);
int zmail_parser_get_body_line(zmail_parser_t * parser, char **ptr);
int zmail_parser_get_header_line(zmail_parser_t * parser, char **ptr);
int zmail_parser_header_value_decode_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src);
int zmail_parser_header_value_decode(zmail_parser_t * parser, char *in_src, int in_len, char *out);
int zmail_parser_header_value_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src);
int zmail_parser_header_value_trim(zmail_parser_t * parser, char *line, int len, char **ptr);
int zmail_parser_header_parse_param(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len, char **attr);
int zmail_parser_header_signle_token_decode_dup(zmail_parser_t * parser, char *line, int len, char **out_src);
int zmail_parser_mail_header_decode_by_line(zmail_parser_t * parser, char *line, int len);
int zmail_parser_references_decode(zmail_parser_t * parser, char *refs, zmail_references_t ** list);
long zmail_parser_header_date_decode(zmail_parser_t * parser, char *str);
void zmail_parser_addr_free(zmail_parser_t * parser, zmail_addr_t * ma);
void zmail_parser_free_mime(zmail_parser_t * parser, zmail_mime_t * mime);
void zmail_parser_format_mime(zmail_parser_t * parser, zmail_mime_t * mime);
int zmail_parser_mime_identify_type(zmail_parser_t * parser);

/* for imap */
int zmail_parser_mime_section(zmail_parser_t * parser);

/* ################################################################## */
/* MAIL TNEF PARSER */
struct ztnef_mime_t {
    char *type;
    char *filename;
    char *filename_rd;
    char *content_id;
    int body_offset;
    int body_len;

    short int mime_type;

    /* relationship */
    ztnef_mime_t *all_last;
    ztnef_mime_t *all_next;

    /* user's data that be maintained self */
    void *user_context;
};

struct ztnef_parser_t {
    ztnef_mime_t **attachment_mime_list;
    short int attachment_mime_count;

    char default_src_charset[32];
    char default_dest_charset[32];
    /* */
    zmpool_t *mpool;
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
};

ztnef_parser_t *ztnef_parser_create(char *mail_data, int mail_data_len);
void ztnef_parser_free(ztnef_parser_t * parser);
ztnef_parser_t *ztnef_parser_create_mpool(zmpool_t * imp, char *tnef_data, int tnef_data_len);
int ztnef_parser_run(ztnef_parser_t * parser);
int ztnef_parser_set_default_charset(ztnef_parser_t * parser, char *default_src_charset, char *default_dest_charset);
int ztnef_parser_get_mime_body(ztnef_parser_t * parser, ztnef_mime_t * mime, char **out_ptr);

/* ################################################################## */
/* util */
int zquery_line(char *connection, char *query, char *result, int timeout);

int zlicense_mac_check(char *salt, char *license);
void zlicense_mac_build(char *salt, char *_mac, char *rbuf);

#ifdef LIBZC_MALLOC_NAMESAPCE
#undef zmalloc
#undef zcalloc
#undef zrealloc
#undef zfree
#undef zstrdup
#undef zstrndup
#undef zmemdup
#endif

#endif /* ___ZINCLUDE_STDLIB_ */
