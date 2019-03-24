/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-18
 * ================================
 */

#pragma pack(push, 4)
#pragma once

#ifndef ___ZC_INCLUDE_STDLIB_
#define ___ZC_INCLUDE_STDLIB_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

/* ################################################################## */
typedef int zbool_t;
typedef union ztype_convert_t ztype_convert_t;
typedef struct zsize_data_t zsize_data_t;
typedef struct zstrtok_t zstrtok_t;
typedef struct zbuf_t zbuf_t;
typedef struct zlink_node_t zlink_node_t;
typedef struct zlink_t zlink_t;
typedef struct zvector_t zvector_t;
typedef struct zrbtree_node_t zrbtree_node_t;
typedef struct zrbtree_t zrbtree_t;
typedef struct zmpool_t zmpool_t;
#define zmpiece_t zmpool_t
typedef struct zsdata_t zsdata_t;
typedef struct zsdlist_t zsdlist_t;
typedef struct zargv_t zargv_t;
typedef struct zlist_t zlist_t;
typedef struct zlist_node_t zlist_node_t;
typedef struct zdict_node_t zdict_node_t;
typedef struct zdict_t zdict_t;
typedef struct zidict_node_t zidict_node_t;
typedef struct zidict_t zidict_t;
#define zconfig_t zdict_t
typedef struct zaddr_t zaddr_t;
typedef struct zstream_t zstream_t;
typedef struct zalarm_t zalarm_t;
typedef struct zev_t zev_t;
typedef struct zaio_t zaio_t;
typedef struct zevtimer_t zevtimer_t;
typedef struct zevbase_t zevbase_t;
typedef struct ziopipe_base_t ziopipe_base_t;
typedef struct zmmap_reader_t zmmap_reader_t;
typedef struct zmd5_t zmd5_t;
typedef struct zsha1_t zsha1_t;
typedef struct zfinder_t zfinder_t;
typedef struct zmail_t zmail_t;
typedef struct zmime_t zmime_t;
typedef struct zmime_address_t zmime_address_t;
typedef struct zmime_header_line_element_t zmime_header_line_element_t;
typedef struct ztnef_t ztnef_t;
typedef struct ztnef_mime_t ztnef_mime_t;

extern char zblank_buffer[];
/* ################################################################## */
#define ZFREE(a)                     (zfree(a),a=0)
static inline int zempty(const void *ptr) { return ((!ptr)||(!(*(const char *)(ptr)))); }
#define ZEMPTY(str)                  (!(str)||!(*((const char *)str)))
#define ZCONTAINER_OF(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))
#define ZCONTAINER_OF2(ptr,app_type,offset) ((app_type *) (((char *) (ptr)) - offset))

union ztype_convert_t {
    const void *ptr_const_void;
    const char *ptr_const_char;
    void * ptr_void;
    char * ptr_char;
    long i_long;
    int i_int;
};
#define ZCHAR_PTR_TO_INT(_ptr, _int)    {ztype_convert_t _ct;_ct.ptr_char=(_ptr);_int=_ct.i_int;}
#define ZINT_TO_CHAR_PTR(_int, _ptr)    {ztype_convert_t _ct;_ct.i_int=(_int);_ptr=_ct.ptr_char;}

#define ZSTR_N_CASE_EQ(a, b, n)       ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strncasecmp(a,b,n)))
#define ZSTR_CASE_EQ(a, b)            ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strcasecmp(a,b)))
#define ZSTR_N_EQ(a, b, n)            ((a[0] == b[0]) && (!strncmp(a,b,n)))
#define ZSTR_EQ(a, b)                 ((a[0] == b[0]) && (!strcmp(a,b)))

static inline void zint_to_buf4(int a, unsigned char *buf)
{ buf[0] = (a >> 24) & 0X0F; buf[1] = (a >> 16) & 0X0F; buf[2] = (a >> 8) & 0X0F; buf[3] = (a) & 0XFF; }

static inline int zbuf4_to_int(unsigned char *buf)
{ return (buf[0] << 24)|(buf[1] << 16)||(buf[2] << 24) | buf[3]; }

#define zpthread_lock(l)    if(l){if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}
#define zpthread_unlock(l)  if(l){if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}

#define Z_DF_ZBUF        (-1)
#define Z_DF_ADD_CHAR(type, obj, len, ch) { \
    if((int)type>0) {if(len<type){(obj)[len]=(ch);}} \
    else if((int)type==Z_DF_ZBUF) {ZBUF_PUT((zbuf_t *)(obj), (ch));} \
    len++; \
}

/* ################################################################## */
extern char *zvar_progname;

/* ################################################################## */
/* LOG, 通用 */
typedef void (*zlog_voutput_t) (const char *fmt, va_list ap);

extern zlog_voutput_t zlog_voutput;
extern int zvar_fatal_catch;

void zinfo(const char *fmt, ...);
void zfatal(const char *fmt, ...);

/* ################################################################## */
/* malloc */
#define zmalloc         zmalloc_20160308
#define zcalloc         zcalloc_20160308
#define zrealloc        zrealloc_20160308
#define zfree           zfree_20160308
#define zstrdup         zstrdup_20160308
#define zstrndup        zstrndup_20160308
#define zmemdup         zmemdup_20160308
#define zmemdupnull     zmemdupnull_20160308
void *zmalloc(int len);
void *zcalloc(int nmemb, int size);
void *zrealloc(const void *ptr, int len);
void zfree(const void *ptr);
char *zstrdup(const char *ptr);
char *zstrndup(const char *ptr, int n);
char *zmemdup(const void *ptr, int n);
char *zmemdupnull(const void *ptr, int n);

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
static inline int zchar_tolower(int c) { return ZCHAR_TOLOWER(c); }
static inline int zchar_toupper(int c) { return ZCHAR_TOUPPER(c); }
char *ztolower(char *str);
char *ztoupper(char *str);

/* trim */
char *ztrim_left(char *str);
char *ztrim_right(char *str);
char *ztrim(char *str);
/* skip */
char *zskip_left(const char *str, const char *ignores);
char *zskip_right(const char *str, int size, const char *ignores);
int zskip(const char *line, int len, const char *ignores_left, const char *ignores_right, char **start);
char *zfind_delim(const char *str, const char *delims);

/* strtok */
struct zstrtok_t {
    char *sstr;
    char *str;
    int len;
};
void zstrtok_init(zstrtok_t * k, const char *sstr);
zstrtok_t *zstrtok(zstrtok_t * k, const char *delim);

/* convert to unit */
int zstr_to_bool(const char *s, int def);
long zstr_to_second(const char *s);
long zstr_to_size(const char *s);

/* */
char *zmemstr(const void *s, const char *needle, int len);
char *zmemcasestr(const void *s, const char *needle, int len);

/* strncpy, strncat */
char *zstrncpy(char *dest, const char *src, int len);
char *zstrncat(char *dest, const char *src, int len);

/* strncpy, strncat */
static inline int zvsnprintf(char *str, size_t size, const char *fmt, va_list ap) {
    int ret=vsnprintf((str),(size),(fmt),(ap));
    return ((ret<(int)size)?ret:(size-1));
}

/* ################################################################## */
/* data_size */
/* ################################################################## */
/* BUF */
struct zbuf_t {
    char *data;
    int len:31;
    unsigned int wrap_mode:1;
    int size:31;
    unsigned int static_mode:1;
};
#define ZBUF_DATA(b)    ((b)->data)
#define ZBUF_LEN(b)     ((b)->len)
#define ZBUF_PUT(b, c)  \
    (((b)->len<(b)->size)?((int)((b)->data[(b)->len++]=(c))):(((b)->static_mode?0:zbuf_put_do((b), (c)))))
#define ZBUF_RESET(b)    ((b)->len=0, (b)->data[0]=0)
#define ZBUF_TERMINATE(b)    ((b)->data[(b)->len]=0)
#define ZBUF_TRUNCATE(b, n)  (((((b)->len>n)&&(n>0))?((b)->len=n):0),(b)->data[(b)->len]=0)
#define ZBUF_LEFT(b)    ((b)->size - (b)->len)
#define ZBUF_SET_LEN(b, nl)  ((b)->len = (nl))

zbuf_t *zbuf_create(int size);
void zbuf_free(zbuf_t * bf);
int zbuf_need_space(zbuf_t * bf, int need);
int zbuf_put_do(zbuf_t * bf, int ch);
static inline int zbuf_put(zbuf_t * bf, int ch) { return ZBUF_PUT(bf, ch); }
static inline void zbuf_reset(zbuf_t * bf) { ZBUF_RESET(bf); }
static inline void zbuf_terminate(zbuf_t * bf) { ZBUF_TERMINATE(bf); }
static inline void zbuf_truncate(zbuf_t * bf, int new_len) { ZBUF_TRUNCATE(bf, new_len); }

int zbuf_strncpy(zbuf_t * bf, const char *src, int len);
int zbuf_strcpy(zbuf_t * bf, const char *src);
int zbuf_strncat(zbuf_t * bf, const char *src, int len);
int zbuf_strcat(zbuf_t * bf, const char *src);
int zbuf_memcpy(zbuf_t * bf, const void *src, int len);
int zbuf_memcat(zbuf_t * bf, const void *src, int len);
int zbuf_printf_1024(zbuf_t * bf, const char *format, ...);

/* STACK_BUF */
#define ZSTACK_BUF(name, _size)    \
    zbuf_t name ## _ZSTACT_BUF_ , *name; \
    name = &name ## _ZSTACT_BUF_; \
    char name ## _databuf_STACK [_size+1]; \
    name->size = _size; name->len = 0; \
    name->data = name ## _databuf_STACK; \
    name->static_mode = 1;

#define ZSTACK_BUF_FROM(name, _data, _size)    \
    zbuf_t name ## _ZSTACT_BUF_, *name; \
    name = &name ## _ZSTACT_BUF_; \
    name->size = _size; name->len = 0; \
    name->data = (char *)(_data); \
    name->static_mode = 1;

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
#define ZLINK_HEAD(link)     ((link)->head)
#define ZLINK_TAIL(link)     ((link)->tail)
#define ZLINK_PREV(node)     ((node)->prev)
#define ZLINK_NEXT(node)     ((node)->next)


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

zargv_t *zargv_create_mpool(zmpool_t * mpool, int size);
zargv_t *zargv_create(int size);
void zargv_free(zargv_t * argvp);
void zargv_add(zargv_t * argvp, const char *ns);
void zargv_addn(zargv_t * argvp, const char *ns, int nlen);
void zargv_truncate(zargv_t * argvp, int len);
void zargv_rest(zargv_t * argvp);
zargv_t *zargv_split_append(zargv_t * argvp, const char *string, const char *delim);
void zargv_show(zargv_t * argvp);

/* ################################################################## */
/* VECTOR */
struct zvector_t {
    char **data;
    int len;
    int size;
    zmpool_t *mpool;
};
#define ZVECTOR_LEN(arr)         ((arr)->len)
#define ZVECTOR_DATA(arr)        ((arr)->data)
zvector_t *zvector_create_MPOOL(zmpool_t *mpool, int size);
void zvector_free(zvector_t * arr);
zvector_t *zvector_create(int size);
void zvector_free(zvector_t * arr);
void zvector_add(zvector_t * arr, void *ns);
void zvector_reset(zvector_t * arr);
void zvector_truncate(zvector_t * arr, size_t new_len);
#define ZVECTOR_WALK_BEGIN(arr, var_your_chp)    {\
    int  zargv_tmpvar_i;\
    for(zargv_tmpvar_i=0;zargv_tmpvar_i<(arr)->len;zargv_tmpvar_i++){\
        var_your_chp = (typeof(var_your_chp))((arr)->data[zargv_tmpvar_i]);
#define ZVECTOR_WALK_END                }}

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

#define ZRBTREE_HAVE_DATA(tree)     ((tree)->zrbtree_node?1:0)
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
zrbtree_node_t *zrbtree_find(zrbtree_t * tree, zrbtree_node_t * vnode);
static inline zrbtree_node_t *zrbtree_detach(zrbtree_t * tree, zrbtree_node_t * node)
{
    zrbtree_erase(tree, node);
    return node;
}

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
/* MPOOL */
typedef struct zmpool_api_t zmpool_api_t;
struct zmpool_api_t {
    void *(*malloc) (zmpool_t *, int);
    void *(*realloc) (zmpool_t *, const void *, int);
    void (*free) (zmpool_t *, const void *);
    void (*reset) (zmpool_t *);
    void (*free_pool) (zmpool_t *);
};
extern zmpool_api_t zmpool_api_vector[];
struct zmpool_t {
    unsigned char api_id;
};

zmpool_t *zmpool_create_common_pool(int *register_size_list);
zmpool_t *zmpool_create_greedy_pool(void);
void zmpool_free_pool(zmpool_t * mp);
static inline void *zmpool_malloc(zmpool_t * mp, int len)
{
    if (!mp) { return zmalloc(len); }
    return zmpool_api_vector[mp->api_id].malloc(mp, len);
}
static inline void *zmpool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    if (!mp) { return zrealloc(ptr, len); }
    return zmpool_api_vector[mp->api_id].realloc(mp, ptr, len);
}
static inline void *zmpool_calloc(zmpool_t * mp, int nmemb, int size)
{
    void *r;
    if (!mp) { return zcalloc(nmemb, size); }
    r = zmpool_malloc(mp, nmemb * size);
    memset(r, 0, nmemb * size);
    return r;
}
static inline void zmpool_free(zmpool_t * mp, const void *ptr)
{
    if (!mp) { return zfree(ptr); }
    return zmpool_api_vector[mp->api_id].free(mp, ptr);
}
void *zmpool_strdup(zmpool_t * mp, const char *ptr);
void *zmpool_strndup(zmpool_t * mp, const char *ptr, int n);
void *zmpool_memdup(zmpool_t * mp, const void *ptr, int n);
void *zmpool_memdupnull(zmpool_t * mp, const void *ptr, int n);
void zmpool_reset(zmpool_t * mp);

/* ################################################################## */
/* MPIECE */
static inline zmpiece_t *zmpiece_create(int element_size)
{
    int e[2] = {element_size, 0};
    return  zmpool_create_common_pool(e);
}
#define zmpiece_free(piece)           zmpool_free_pool(piece)
#define zmpiece_alloc_one(piece)      zmpool_malloc(piece, 1)
#define zmpiece_free_one(piece, ptr)  zmpool_free(piece, ptr)

/* ################################################################## */
/* LIST */
struct zlist_t {
    zlist_node_t *head;
    zlist_node_t *tail;
    int len;
};
struct zlist_node_t {
    zlist_node_t *prev;
    zlist_node_t *next;
    char *value;
};
#define ZLIST_NEXT(n)   ((n)->next)
#define ZLIST_PREV(n)   ((n)->prev)
#define ZLIST_HEAD(c)   ((c)->head)
#define ZLIST_TAIL(c)   ((c)->tail)
#define ZLIST_LEN(c)    ((c)->len)
zlist_t *zlist_create(void);
void zlist_free(zlist_t * list);
void zlist_attach_before(zlist_t * list, zlist_node_t * n, zlist_node_t * before);
void zlist_detach(zlist_t * list, zlist_node_t * n);
zlist_node_t *zlist_add_before(zlist_t * list, const void *value, zlist_node_t * before);
zbool_t zlist_delete(zlist_t * list, zlist_node_t * n, char **value);
static inline zlist_node_t *zlist_push(zlist_t *l,const void *v){return zlist_add_before(l,v,0);}
static inline zlist_node_t *zlist_unshift(zlist_t *l,const void *v){return zlist_add_before(l,v,l->head);}
static inline zbool_t zlist_pop(zlist_t *l,char **v){return zlist_delete(l,l->tail,v);}
static inline zbool_t zlist_shift(zlist_t *l,char **v){return zlist_delete(l,l->head,v);}

#define ZLIST_WALK_BEGIN(list, var_your_node)  {\
    var_your_node=(list)->head;\
    for(;var_your_node;var_your_node=var_your_node->next){
#define ZLIST_WALK_END                          }}

/* ################################################################## */
/* DICT */
struct zdict_t {
    int len:31;
    unsigned int is_STR;
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
zdict_t *zdict_create_STR(void);
zdict_t *zdict_create_mpool_STR(zmpool_t * mpool);
zdict_node_t *zdict_update(zdict_t * dict, const char *key, const void *value, char **old_value);
zdict_node_t *zdict_update_STR(zdict_t * dict, const char *key, const char *value);
zdict_node_t *zdict_find(zdict_t * dict, const char *key, char **value);
zdict_node_t *zdict_find_near_prev(zdict_t * dict, const char *key, char **value);
zdict_node_t *zdict_find_near_next(zdict_t * dict, const char *key, char **value);
void zdict_erase_node(zdict_t * mp, zdict_node_t * n);
void zdict_erase(zdict_t * dict, const char *key, char **old_value);
void zdict_erase_STR(zdict_t * dict, const char *key);
zdict_node_t *zdict_first(zdict_t * dict);
zdict_node_t *zdict_last(zdict_t * dict);
zdict_node_t *zdict_prev(zdict_node_t * node);
zdict_node_t *zdict_next(zdict_node_t * node);
void zdict_free(zdict_t * dict);
int zdict_keys(zdict_t * dict, char **key_list, int size);
void zdict_show(zdict_t * dict);
#define ZDICT_LEN(dict)            ((dict)->len)
#define ZDICT_KEY(n)               ((n)->key)
#define ZDICT_VALUE(n)             ((n)->value)
#define ZDICT_WALK_BEGIN(dict, n)  { for(n = zdict_first(dict); n; n = zdict_next(n)) {
#define ZDICT_WALK_END    }}

/* ################################################################## */
/* IDICT */
struct zidict_t {
    int len:31;
    unsigned int is_STR:1;
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
zidict_t *zidict_create_STR(void);
zidict_t *zidict_create_mpool_STR(zmpool_t * mpool);
zidict_node_t *zidict_update(zidict_t * dict, long key, const void *value, char **old_value);
zidict_node_t *zidict_update_STR(zidict_t * dict, long key, const char *value);
zidict_node_t *zidict_find(zidict_t * dict, long key, char **value);
zidict_node_t *zidict_find_near_prev(zidict_t * dict, long key, char **value);
zidict_node_t *zidict_find_near_next(zidict_t * dict, long key, char **value);
void zidict_erase_node(zidict_t * mp, zidict_node_t * n);
void zidict_erase(zidict_t * dict, long key, char **old_value);
void zidict_erase_STR(zidict_t * dict, long key);
zidict_node_t *zidict_first(zidict_t * dict);
zidict_node_t *zidict_last(zidict_t * dict);
zidict_node_t *zidict_prev(zidict_node_t * node);
zidict_node_t *zidict_next(zidict_node_t * node);
void zidict_free(zidict_t * dict);
int zidict_keys(zidict_t * dict, long *key_list, int size);
#define ZIDICT_LEN(dict)            ((dict)->len)
#define ZIDICT_KEY(n)               ((n)->key)
#define ZIDICT_VALUE(n)             ((n)->value)
#define ZIDICT_WALK_BEGIN(idict, n) { \
    for(n = zidict_first(idict); n; n = zidict_next(n)) {
#define ZIDICT_WALK_END    }}


/* CONFIG ########################################################## */
extern zconfig_t *zvar_default_config;
void zdefault_config_init(void);
void zdefault_config_fini(void);
#define zconfig_create  zdict_create_STR
#define zconfig_free    zdict_free
#define zconfig_update  zdict_update_STR
#define zconfig_erase   zdict_erase
#define zconfig_show    zdict_show

/* config load */
int zconfig_load(zconfig_t * cf, const char *filename);

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
char *zconfig_get_str(zconfig_t * cf, const char *name, const char *def);
int zconfig_get_bool(zconfig_t * cf, const char *name, int def);
int zconfig_get_int(zconfig_t * cf, const char *name, int def, int min, int max);
long zconfig_get_long(zconfig_t * cf, const char *name, long def, long min, long max);
long zconfig_get_second(zconfig_t * cf, const char *name, long def, long min, long max);
long zconfig_get_size(zconfig_t * cf, const char *name, long def, long min, long max);
void zconfig_get_str_table(zconfig_t * cf, zconfig_str_table_t * table);
void zconfig_get_int_table(zconfig_t * cf, zconfig_int_table_t * table);
void zconfig_get_long_table(zconfig_t * cf, zconfig_long_table_t * table);
void zconfig_get_bool_table(zconfig_t * cf, zconfig_bool_table_t * table);
void zconfig_get_second_table(zconfig_t * cf, zconfig_second_table_t * table);
void zconfig_get_size_table(zconfig_t * cf, zconfig_size_table_t * table);

#define ZCONFIG_WALK_BEGIN(cf, key, value) { zdict_node_t *___nd; \
    for (___nd = zdict_first(cf);___nd;___nd=zdict_next(___nd)) { \
        key = ZDICT_KEY(___nd); value = ZDICT_VALUE(___nd); {
#define ZCONFIG_WALK_END }}}

/* size _data ######################################################## */
struct zsize_data_t {
    int size;
    char *data;
};
int zsize_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len);
int zsize_data_unescape_all(const void *src_data, int src_size, zsize_data_t *vec, int vec_size);
void zsize_data_escape(zbuf_t * zb, const void *data, int len);
void zsize_data_escape_int(zbuf_t * zb, int i);
void zsize_data_escape_long(zbuf_t * zb, long i);
void zsize_data_escape_dict(zbuf_t * zb, zdict_t * zd);
void zsize_data_escape_pp(zbuf_t * zb, const char **pp, int size);
int zsize_data_put_size(int size, char *buf);
int zsize_data_get_size_from_zstream(zstream_t *fp);

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
int zread_wait(int fd, long timeout);
int ztimed_read(int fd, void *buf, int len, long timeout);
int ztimed_strict_read(int fd, void *buf, int len, long timeout);
int zwrite_wait(int fd, long timeout);
int ztimed_write(int fd, const void *buf, int len, long timeout);
int ztimed_strict_write(int fd, const void *buf, int len, long timeout);
int zflock(int fd, int flags);
int zshared_flock(int fd);
int zexclusive_flock(int fd);
int zremove_flock(int fd);
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
int zgetaddr(const char *host, zaddr_t * addr_list, int max_count);
int zgetpeer(int sockfd, int *host, int *port);

/* ################################################################## */
/* socket */
int zunix_connect(const char *addr, long timeout);
int zinet_connect(const char *dip, int port, long timeout);
int zhost_connect(const char *host, int port, long timeout);
int zinet_listen(const char *sip, int port, int backlog);
int zfifo_listen(const char *path);
int zunix_accept(int fd);
int zinet_accept(int fd);
int zconnect(const char *netpath, long timeout);
int zlisten(const char *netpath, int backlog, int *type);

/* ################################################################## */
/* parameter */
void zusage(void);
int zparameter_run(int argc, char **argv);
void zparameter_run_2();
#define ZPARAMETER_BEGIN() { zdefault_config_init(); \
    int opti, ___optret_123, optval_count; char *optname, *optval; \
    for (opti = 1; opti < argc;) { optname = argv[opti]; optval = 0; optval_count = 0;\
        ___optret_123 = zparameter_run(argc-opti, argv+opti); \
        if (___optret_123 > 0) { opti += ___optret_123; continue; } \
        if (___optret_123 < 0) { ___usage(argv[opti]); exit(1); } \
        if (opti+1 < argc) { optval = argv[opti+1];} \
        for(optval_count = 0; opti + 1 + optval_count < argc; optval_count++) { \
            if(argv[opti + 1+ optval_count][0] == '-') break; \
        } (void)optname; (void)optval; (void)opti; (void)optval_count; {
#define ZPARAMETER_END      } ___usage(argv[opti]); exit(1);} zparameter_run_2(); }

/* ################################################################## */
void zopenssl_init(void);
SSL_CTX *zopenssl_create_SSL_CTX_server(void);
SSL_CTX *zopenssl_create_SSL_CTX_client(void);
int zopenssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file);
void zopenssl_SSL_CTX_free(SSL_CTX * ctx);
void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len);
SSL *zopenssl_create_SSL(SSL_CTX * ctx, int fd);
void zopenssl_SSL_free(SSL * ssl);
int zopenssl_SSL_get_fd(SSL *ssl);
int zopenssl_connect(SSL * ssl, long timeout);
int zopenssl_accept(SSL * ssl, long timeout);
int zopenssl_shutdown(SSL * ssl, long timeout);
int zopenssl_read(SSL * ssl, void *buf, int len, long timeout);
int zopenssl_write(SSL * ssl, const void *buf, int len, long timeout);

/* ################################################################## */
/* file vstream */
#define ZSTREAM_RBUF_SIZE           4096
#define ZSTREAM_WBUF_SIZE           4096

typedef int (*zstream_read_t) (zstream_t *, void *, int, long);
typedef int (*zstream_write_t) (zstream_t *, const void *, int, long);
struct zstream_t {
    int read_buf_p1:16;
    int read_buf_p2:16;
    int write_buf_len:16;
    unsigned int error:1;
    unsigned int eof:1;
    char read_buf[ZSTREAM_RBUF_SIZE + 1];
    char write_buf[ZSTREAM_WBUF_SIZE + 1];
    zstream_read_t read_fn;
    zstream_write_t write_fn;
    void *io_ctx;
    long timeout;
};
#define ZSTREAM_GETCHAR(fp)        (((fp)->read_buf_p1<(fp)->read_buf_p2)\
    ?((int)((fp)->read_buf[(fp)->read_buf_p1++])):(zstream_getchar(fp)))
#define ZSTREAM_GET                ZSTREAM_GETCHAR
#define ZSTREAM_GETC               ZSTREAM_GETCHAR

#define ZSTREAM_FLUSH(fp)          (((fp)->write_buf_len>0)?(zstream_flush(fp)):(0))
#define ZSTREAM_PUTCHAR(fp, ch)    {if((fp)->write_buf_len<ZSTREAM_WBUF_SIZE) \
    { (fp)->write_buf[(fp)->write_buf_len++]=(ch); } else {  zstream_putchar(fp, ch); } \
}
#define ZSTREAM_PUT                ZSTREAM_PUTCHAR
#define ZSTREAM_PUTC               ZSTREAM_PUTCHAR

#define ZSTREAM_ERROR(fp)          ((fp)->error)
#define ZSTREAM_EOF(fp)            ((fp)->eof)
#define ZSTREAM_EXCEPTION(fp)      ((fp)->eof || (fp)->error)
#define ZSTREAM_CTX(fp)            ((fp)->io_ctx)
static inline zbool_t zstream_is_error(zstream_t *fp) { return fp->error;}
static inline zbool_t zstream_is_eof(zstream_t *fp) { return fp->eof;}
static inline zbool_t zstream_is_exception(zstream_t *fp) { return (fp->eof)||(fp)->error;}
static inline void * zstream_get_ctx(zstream_t *fp) { return fp->io_ctx; }
static inline int zstream_get_FD(zstream_t *fp) { ztype_convert_t tc;tc.ptr_const_void=fp->io_ctx;return tc.i_int;}

zstream_t *zstream_create(void);
void *zstream_free(zstream_t * fp);
void zstream_set_ioctx(zstream_t * fp, const void *io_ctx, zstream_read_t read_fn, zstream_write_t write_fn);
void zstream_set_timeout(zstream_t * fp, long timeout);

zstream_t *zstream_open_FD(int fd);
int zstream_close_FD(zstream_t * fp);
#define zstream_dopen              zstream_open_FD
#define zstream_dclose             zstream_close_FD

zstream_t *zstream_open_SSL(SSL * ssl);
SSL *zstream_close_SSL(zstream_t * fp);

int zstream_getchar(zstream_t * fp);
#define zstream_get                zstream_getchar
#define zstream_getc               zstream_getchar
int zstream_read(zstream_t * fp, void *buf, int len);
int zstream_read_n(zstream_t * fp, void *buf, int len);
int zstream_read_delimiter(zstream_t * fp, void *buf, int len, int delimiter);
static inline int zstream_read_line(zstream_t * fp, void *buf, int len)
{ return zstream_read_delimiter(fp, buf, len, '\n'); }
#define ZSTREAM_READ_LINE(fp, buf, len)   zstream_read_delimiter(fp, buf, len, '\n')
int zstream_gets_n(zstream_t * fp, zbuf_t * bf, int len);
int zstream_gets_delimiter(zstream_t * fp, zbuf_t * bf, int delimiter);
static inline int zstream_gets_line(zstream_t * fp, zbuf_t * bf) {return zstream_gets_delimiter(fp, bf, '\n'); }
#define ZSTREAM_GETS(fp, bf)              zstream_gets_delimiter(fp, bf, '\n')

int zstream_flush(zstream_t * fp);
void zstream_putchar(zstream_t * fp, int ch);
#define zstream_put                zstream_putchar
#define zstream_putc               zstream_putchar
int zstream_write_n(zstream_t * fp, const void *buf, int len);
int zstream_puts(zstream_t * fp, const char *s);
int zstream_printf_1024(zstream_t * fp, const char *format, ...);

/* ################################################################## */
/* alarm */
typedef void (*zalarm_cb_t) (zalarm_t *);
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
static inline void zalarm_set_context(zalarm_t *a, const void *ctx) {a->context = (void *)(ctx);}
static inline void *zalarm_get_context(zalarm_t *a) {return a->context;}

/* ################################################################## */
/* EVENT AIO */
#define ZEV_TYPE_EVENT         0x1
#define ZEV_TYPE_AIO           0x2

typedef void (*zev_cb_t) (zev_t *);
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
#define zev_set_context(ev, ctx)         ((ev)->context=(void *)(ctx))
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
#define zev_exception(ev)                ((ev)->recv_events & ZEV_EXCEPTION)
int zev_unset(zev_t * ev);

/* AIO */
typedef struct zaio_rwbuf_t zaio_rwbuf_t;
typedef struct zaio_rwbuf_list_t zaio_rwbuf_list_t;
#define ZAIO_RWBUF_SIZE            4096
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
typedef void (*zaio_cb_t) (zaio_t *);
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
    unsigned char ssl_server_or_client:1;
    unsigned char ssl_session_init:1;
    unsigned char ssl_read_want_read:1;
    unsigned char ssl_read_want_write:1;
    unsigned char ssl_write_want_read:1;
    unsigned char ssl_write_want_write:1;
    unsigned char ssl_error:1;
    SSL *ssl;                  /* SSL* */
    zaio_t *queue_prev;
    zaio_t *queue_next;
    zevbase_t *evbase;
};

#define zaio_get_base(aio)          ((aio)->evbase)
#define zaio_get_fd(aio)            ((aio)->fd)
#define zaio_get_ret(aio)           ((aio)->ret)
#define zaio_get_callback(aio)      ((aio)->callback)
#define zaio_set_context(aio, ctx)  ((aio)->context=(ctx))
#define zaio_get_context(aio)       ((aio)->context)
zaio_t *zaio_create(void);
void zaio_free(zaio_t * aio);
void zaio_init(zaio_t * aio, zevbase_t * eb, int fd);
void zaio_fini(zaio_t * aio);
void zaio_set_local_mode(zaio_t * aio);
int zaio_fetch_rbuf(zaio_t * aio, char *buf, int len);
void zaio_attach(zaio_t * aio, zaio_cb_t callback);
void zaio_read(zaio_t * aio, int max_len, zaio_cb_t callback, long timeout);
void zaio_read_n(zaio_t * aio, int strict_len, zaio_cb_t callback, long timeout);
void zaio_read_delimiter(zaio_t * aio, int delimiter, int max_len, zaio_cb_t callback, long timeout);
#define zaio_read_line(a,c,d,e) zaio_read_delimiter(a,'\n',c,d,e)
void zaio_read_size_data(zaio_t * aio, zaio_cb_t callback, long timeout);
void zaio_write_cache_printf_1024(zaio_t * aio, const char *fmt, ...);
void zaio_puts(zaio_t * aio, const char *s);
void zaio_write_cache_append(zaio_t * aio, const void *buf, int len);
void zaio_write_cache_size_data(zaio_t * aio, const void *buf, int len);
void zaio_write_cache_flush(zaio_t * aio, zaio_cb_t callback, long timeout);
int zaio_write_cache_get_len(zaio_t * aio);
void zaio_sleep(zaio_t * aio, zaio_cb_t callback, long timeout);
void zaio_ssl_init(zaio_t * aio, SSL_CTX * ctx, int server_or_client, zaio_cb_t callback, long timeout);
void zaio_ssl_init_server(zaio_t * aio, SSL_CTX * ctx, zaio_cb_t callback, long timeout);
void zaio_ssl_init_client(zaio_t * aio, SSL_CTX * ctx, zaio_cb_t callback, long timeout);
void zaio_ssl_fini(zaio_t * aio);
void zaio_attach_SSL(zaio_t * aio, SSL * ssl);
SSL *zaio_detach_SSL(zaio_t * aio);

/* TIMER */
typedef void (*zevtimer_cb_t) (zevtimer_t *);
struct zevtimer_t {
    long timeout;
    zevtimer_cb_t callback;
    void *context;
    zrbtree_node_t rbnode_time;
    unsigned char in_time:1;
    zevbase_t *evbase;
};

#define zevtimer_get_base(timer)            ((timer)->evbase)
#define zevtimer_set_context(timer, ctx)    ((timer)->context=(void *)(ctx))
#define zevtimer_get_context(timer)         ((timer)->context)
zevtimer_t *zevtimer_create(void);
void zevtimer_free(zevtimer_t * timer);
void zevtimer_init(zevtimer_t * timer, zevbase_t * eb);
void zevtimer_fini(zevtimer_t * timer);
void zevtimer_start(zevtimer_t * timer, zevtimer_cb_t callback, long timeout);
void zevtimer_stop(zevtimer_t * timer);

/* BASE */
typedef int (*zevbase_cb_t) (zevbase_t *);
typedef int (*zevbase_loop_t) (zevbase_t *);

struct zevbase_t {
    int epoll_fd;
    struct epoll_event *epoll_event_list;
    zrbtree_t general_timer_tree;
    zrbtree_t aio_timer_tree;
    zev_t eventfd_event;
    void *context;
    zevbase_loop_t loop_fn;
    zmpiece_t *aio_rwbuf_mpool;

    zaio_t *queue_head;
    zaio_t *queue_tail;

    zaio_t *extern_queue_head;
    zaio_t *extern_queue_tail;

    void *locker_context;
    void (*lock) (zevbase_t *);
    void (*unlock) (zevbase_t *);
    void (*locker_fini) (zevbase_t *);
};
int zevbase_notify(zevbase_t * eb);
int zevbase_use_pthread(zevbase_t * eb);
zevbase_t *zevbase_create(void);
void zevbase_free(zevbase_t * eb);
int zevbase_dispatch(zevbase_t * eb, long delay);
#define zevbase_set_context(eb, ctx)        ((eb)->context=(void *)(ctx))
#define zevbase_get_context(eb)             ((eb)->context)

/* ################################################################## */
/* iopipe */
typedef void (*ziopipe_after_close_fn_t) (void *);
ziopipe_base_t *ziopipe_base_create(void);
void ziopipe_base_free(ziopipe_base_t * iopb);
void ziopipe_base_notify_stop(ziopipe_base_t * iopb);
int ziopipe_base_run(ziopipe_base_t * iopb);
void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, ziopipe_after_close_fn_t after_close, const void *context);

/* ################################################################## */
int zncr_decode(int ins, char *wchar);
/* base64 */
int zbase64_encode(const void *src, int src_size, char *dest, int dest_size, int mime_flag);
static inline int zbase64_encode_zbuf(const void *src, int src_size, zbuf_t *dest, int mime_flag)
{ return zbase64_encode(src, src_size, (char *)dest, Z_DF_ZBUF, mime_flag); }
int zbase64_decode(const void *src, int src_size, char *dest, int dest_size);
static inline int zbase64_decode_zbuf(const void *src, int src_size, zbuf_t *dest)
{ return zbase64_decode(src, src_size, (char *)dest, Z_DF_ZBUF); }
int zbase64_decode_get_valid_len(const void *src, int src_size);
int zbase64_encode_get_min_len(int in_len, int mime_flag);
/* quoted_printable */
int zqp_decode_2045(const void *src, int src_size, char *dest, int dest_size);
static inline int zqp_decode_2045_zbuf(const void *src, int src_size, zbuf_t *dest)
{ return zqp_decode_2045(src, src_size, (char *)dest, Z_DF_ZBUF); }
int zqp_decode_2047(const void *src, int src_size, char *dest, int dest_size);
static inline int zqp_decode_2047_zbuf(const void *src, int src_size, zbuf_t *dest)
{ return zqp_decode_2047(src, src_size, (char *)dest, Z_DF_ZBUF); }
int zqp_decode_get_valid_len(const void *src, int src_size);
/* hex */
extern char zhex_to_dec_list[256];
int zhex_encode(const void *src, int src_size, char *dest, int dest_size);
static inline int zhex_encode_zbuf(const void *src, int src_size, zbuf_t *dest)
{ return zhex_encode(src, src_size, (char *)dest, Z_DF_ZBUF); }
int zhex_decode(const void *src, int src_size, char *dest, int dest_size);
static inline int zhex_decode_zbuf(const void *src, int src_size, zbuf_t *dest)
{ return zhex_decode(src, src_size, (char *)dest, Z_DF_ZBUF); }
/* crc */
unsigned int zcrc32(const void *data, int size, unsigned int init_value);
unsigned long zcrc64(const void *data, int size, unsigned long init_value);

/* md5 */
struct zmd5_t {
    uint_fast32_t lo, hi;
    uint_fast32_t a, b, c, d;
    unsigned char buffer[64];
    uint_fast32_t block[16];
};
void zmd5_init(zmd5_t * ctx);
void zmd5_update(zmd5_t * ctx, const void *data, int size);
void zmd5_final(zmd5_t * ctx, void *result);    //result'size >= 16
char *zmd5(const void *data, int size, void *result);

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
void zsha1_update(zsha1_t * ctx, const void *input, int len);
void zsha1_result(zsha1_t * ctx, void *result);
char *zsha1(const void *data, int size, void *result);

/* ################################################################## */
/* SYSTEM */
zbool_t chroot_user(const char *root_dir, const char *user_name);

/* not thread-safe */
int zatexit(void (*function)(void));

/* ################################################################## */
/* file */
int zfile_get_size(const char *filename);
int zfile_put_contents(const char *filename, const void *data, int len);
int zfile_get_contents(const char *filename, zbuf_t * dest);
int zfile_get_contents_mmap(const char *filename, zbuf_t * bf);
void zfile_get_contents_sample(const char *filename, zbuf_t * dest);

/* mmap reader */
struct zmmap_reader_t {
    int fd;
    int len;
    char *data;
};
int zmmap_reader_init(zmmap_reader_t * reader, const char *filename);
int zmmap_reader_fini(zmmap_reader_t * reader);

/* ################################################################## */
/* server master */
#define ZMASTER_SERVER_LISTEN_INET     'i'
#define ZMASTER_SERVER_LISTEN_UNIX     'u'
#define ZMASTER_SERVER_LISTEN_FIFO     'f'
#define ZMASTER_SERVER_STATUS_FD       3
#define ZMASTER_MASTER_STATUS_FD       4
#define ZMASTER_LISTEN_FD              5
extern zevbase_t *zvar_master_evbase;
int zmaster_main(int argc, char **argv);
void (*zmaster_load_server_config_fn)(zvector_t *cfs);
void zmaster_load_server_config_by_dir(const char *config_path, zvector_t * cfs);

extern void (*zmaster_server_simple_service) (int fd);
extern void (*zmaster_server_service_register) (const char *service, int fd, int fd_type);
extern void (*zmaster_server_before_service) (void);
extern void (*zmaster_server_event_loop) (void );
extern void (*zmaster_server_before_reload) (void);
extern void (*zmaster_server_before_exit) (void);

extern zevbase_t *zvar_master_server_evbase;
extern int zvar_master_server_stopping;
extern int zvar_master_server_reloading;

int zmaster_server_main(int argc, char **argv);
void zmaster_server_stop_notify(void);
zev_t *zmaster_server_general_aio_register(zevbase_t *eb, int fd, int fd_type, void (*callback) (int));

/* ################################################################## */
/* FINDER */
struct zfinder_t {
    char *title;
    char *uri;
    char *prefix;
    char *suffix;
    zdict_t *parameters;
    void *db;
    int (*get) (zfinder_t *finder, const char *query, zbuf_t * result, long timeout);
    int (*close) (zfinder_t *finder);
};
zfinder_t *zfinder_create(const char *title);
int zfinder_get(zfinder_t *finder, const char *key, zbuf_t * result, int timeout);
void zfinder_close(zfinder_t *finder);
int zfinder_get_once(const char *title, const char *key, zbuf_t * result, int timeout);
int zfinder_main(int argc, char **argv);

/* ################################################################## */
/* CHARSET */
extern int zvar_charset_debug;
char *zcharset_correct_charset(const char *charset);
int zcharset_iconv(
        const char *from_charset, const char *src, int src_len
        , const char *to_charset,  char *dest, int dest_len
        , int *src_converted_len
        , int omit_invalid_bytes_limit, int *omit_invalid_bytes_count
        );
static inline int zcharset_iconv_zbuf(
        const char *from_charset, const char *src, int src_len
        , const char *to_charset, zbuf_t *dest
        , int *src_converted_len
        , int omit_invalid_bytes_limit, int *omit_invalid_bytes_count
        ) {
    return zcharset_iconv(from_charset, src, src_len, to_charset, (char *)dest, Z_DF_ZBUF
            , src_converted_len, omit_invalid_bytes_limit, omit_invalid_bytes_count);
}


extern const char *zvar_charset_chinese[];
extern const char *zvar_charset_japanese[];
extern const char *zvar_charset_korean[];
extern const char *zvar_charset_cjk[];
zbool_t zcharset_detect(const char *data, size_t len, char *charset_ret, const char **charset_list);
static inline zbool_t zcharset_detect_cjk(const char *data, int len, char *charset_ret)
{
    return zcharset_detect(data, len, charset_ret, zvar_charset_cjk);
}

#ifdef LIBZC_USE_LIBICONV
typeof(iconv) libiconv;
typeof(iconv_open) libiconv_open;
typeof(iconv_close) libiconv_close;

iconv_t iconv_open(const char *tocode, const char *fromcode)
{
    return libiconv_open(tocode, fromcode);
}

size_t iconv(iconv_t cd, char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft)
{
    return libiconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

int iconv_close(iconv_t cd)
{
    return libiconv_close(cd);
}
#endif


/* MAIL PARSER ######################################################### */
#define ZMAIL_HEADER_LINE_MAX_LENGTH        102400
#define ZMAIL_HEADER_LINE_MAX_ELEMENT       10240

/* iconv */
ssize_t zmime_iconv(const char *src_charset, const char *data, size_t size, zbuf_t *dest);

/* do not use the funcs forever */
char *zmail_get(zmail_t *parser, int module);

/* header line element */
struct zmime_header_line_element_t {
    char charset[32];
    char *data;
    size_t size;
    unsigned char encode;
};
size_t zmime_header_line_element_split(const char *in_src, size_t in_len
        , zmime_header_line_element_t * mt_list, size_t mt_max_count);
/* */
void zmime_header_get_first_token(const char *line, size_t len, char **val, int *vlen);

/* header line unescape */
size_t zmime_header_line_unescape_advanced(const char *data, size_t size, char *dest);
size_t zmime_header_line_unescape(const char *data, size_t size, char *dest, ssize_t dest_size);

/* header line param */
void zmime_header_param_decode(const char *data, size_t len, zbuf_t *val, zargv_t *params);

/* header line charset convert */
void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, zbuf_t *dest);
void zmime_header_line_2231_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, int with_charset, zbuf_t *dest);

/* address */
struct zmime_address_t {
    char *name;
    char *name_utf8;
    char *address;
};
const zvector_t *zmime_address_vector_decode_MPOOL(zmpool_t *mpool, const char *in_src, size_t in_len);
const zvector_t *zmime_address_vector_decode_utf8_MPOOL(zmpool_t *mpool, const char *src_charset_def, const char *in_src, size_t in_len);
void zmime_address_vector_free(zvector_t *address_vec);
const zvector_t *zmime_address_vector_decode(const char *in_src, size_t in_len);
const zvector_t *zmime_address_vector_decode_utf8(const char *src_charset_def, const char *in_src, size_t in_len);
const zmime_address_t *zmime_address_decode_MPOOL(zmpool_t *mpool, const char *in_src, size_t in_len);
const zmime_address_t *zmime_address_decode_utf8_MPOOL(zmpool_t *mpool, const char *src_charset_def, const char *in_src, size_t in_len);
const zmime_address_t *zmime_address_decode_utf8(const char *src_charset_def, const char *in_src, size_t in_len);
void zmime_address_free_MPOOL(zmpool_t *mpool, zmime_address_t *addr);
void zmime_address_free(zmime_address_t *addr);

/* date */
long zmime_header_date_decode(const char *str);

/* mail parser */
struct zmime_t {
    char *type;
    char *encoding;
    char *charset;
    char *disposition;
    char *name;
    char *name_utf8;
    char *filename;
    char *filename2231;
    unsigned char filename2231_with_charset:1;
    char *filename_utf8;
    char *content_id;
    char *boundary;
    /* mime proto, for imapd */
    char *imap_section;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    short int mime_type;
    short int is_tnef;

    /* mime original header-logic-line */
    zvector_t *header_lines;

    /* relationship */
    zmime_t *next;
    zmime_t *child;
    zmime_t *parent;
    /* */
    zmail_t * parser;
};

char *zmime_get(zmime_t *mime, int module);
static inline const char *zmime_type(zmime_t *m){ return m->type; }
static inline const char *zmime_encoding(zmime_t *m){ return(m->encoding?m->encoding:zmime_get(m, 1)); }
static inline const char *zmime_charset(zmime_t *m){ return m->charset; }
static inline const char *zmime_disposition(zmime_t *m){ return(m->disposition?m->disposition:zmime_get(m, 2)); }
static inline const char *zmime_name(zmime_t *m){ return m->name; }
static inline const char *zmime_name_utf8(zmime_t *m){ return(m->name_utf8?m->name_utf8:zmime_get(m, 3)); }
static inline const char *zmime_filename(zmime_t *m){ return(m->filename?m->filename:zmime_get(m, 4)); }
static inline const char *zmime_filename2231(zmime_t *m, int *with_charset)
{if(!m->filename2231)zmime_get(m,5); *with_charset=m->filename2231_with_charset; return m->filename2231; } 
static inline const char *zmime_filename_utf8(zmime_t *m){return(m->filename_utf8?m->filename_utf8:zmime_get(m,6));}
static inline const char *zmime_content_id(zmime_t *m){return(m->content_id?m->content_id:zmime_get(m,7));} 
static inline const char *zmime_boundary(zmime_t *m){ return m->boundary; }
static inline const char *zmime_imap_section(zmime_t *m)
{if(!m->imap_section)zmail_get(m->parser, 61); return m->imap_section; }
static inline size_t zmime_header_offset(zmime_t *m){ return m->header_offset; }
static inline size_t zmime_header_size(zmime_t *m){ return m->header_len; }
static inline size_t zmime_body_offset(zmime_t *m){ return m->body_offset; }
static inline size_t zmime_body_size(zmime_t *m){ return m->body_len; }
static inline int zmime_is_tnef(zmime_t *m){ return m->is_tnef; }
static inline zmime_t * zmime_next(zmime_t *m){ return m->next; }
static inline zmime_t * zmime_child(zmime_t *m){ return m->child; }
static inline zmime_t * zmime_parent(zmime_t *m){ return m->parent; }
static inline zvector_t *zmime_all_header_lines(zmime_t *m){ return m->header_lines; }
/* sn == 0: first, sn == -1: last */
const zsize_data_t *zmime_header_line(zmime_t *m, const char *header_name, int sn);
void zmime_header_lines(zmime_t *m, const char *header_name, zvector_t *vec);
ssize_t zmime_decoded_content(zmime_t *m, zbuf_t *dest);
ssize_t zmime_decoded_content_utf8(zmime_t *m, zbuf_t *dest);

struct zmail_t {
    int header_len;
    char *subject;
    char *subject_utf8;
    char *date;
    long date_unix;
    short int from_flag:2;
    short int sender_flag:2;
    short int reply_to_flag:2;
    short int to_flag:2;
    short int cc_flag:2;
    short int bcc_flag:2;
    short int receipt_flag:2;
    short int references_flag:2;
    short int classify_flag:2;
    short int section_flag:2;
    zmime_address_t *from;
    zmime_address_t *sender;
    zmime_address_t *reply_to;
    zvector_t *to;
    zvector_t *cc;
    zvector_t *bcc;
    zmime_address_t *receipt;
    char *in_reply_to;
    char *message_id;
    zvector_t *references;

    /* mime-tree */
    zmime_t *top_mime;

    /* all-mime-vector */
    zvector_t *all_mimes;

    /* text(plain,html) type mime-list except for attachment */
    zvector_t *text_mimes;

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    zvector_t *show_mimes;

    /* attachment(and background-image) type mime-list */
    zvector_t *attachment_mimes;

    /* option */
    short int mime_max_depth;
    char src_charset_def[32];

    /* other */
    zmpool_t *mpool;
    char *mail_data;
    char *mail_pos;
    int mail_size;
    /* tmp or cache */
    zvector_t *tmp_header_lines;
};
/* mail parser main funcs */
zmail_t *zmail_parser_create_MPOOL(zmpool_t * imp, const char *mail_data, size_t mail_data_len);
zmail_t *zmail_parser_create(const char *mail_data, int mail_data_len);
void zmail_parser_option_mime_max_depth(zmail_t * parser, int length);
void zmail_parser_option_src_charset_def(zmail_t * parser, const char *src_charset_def);
void zmail_parser_run(zmail_t * parser);
void zmail_parser_free(zmail_t * parser);
static inline const char *zmail_parser_data(zmail_t * parser) { return parser->mail_data; }
static inline size_t zmail_parser_size(zmail_t * parser) { return parser->mail_size; }
void zmail_parser_show(zmail_t * parser);
void zmail_parser_show_json(zmail_t * parser, zbuf_t * result);

static inline size_t zmail_header_size(zmail_t *mp) { return mp->top_mime->header_len; }
static inline size_t zmail_body_offset(zmail_t *mp) { return mp->top_mime->body_offset; }
static inline size_t zmail_body_size(zmail_t *mp) { return mp->top_mime->body_len; }
static inline const char *zmail_message_id(zmail_t *mp){return mp->message_id?mp->message_id:zmail_get(mp,13);}
static inline const char *zmail_subject(zmail_t *mp) { return mp->subject?mp->subject:zmail_get(mp, 1); }
static inline const char *zmail_subject_utf8(zmail_t *mp){return mp->subject_utf8?mp->subject_utf8:zmail_get(mp,2);}
static inline const char *zmail_date(zmail_t *mp){return mp->date?mp->date:zmail_get(mp,3);}
static inline long zmail_date_unix(zmail_t *mp){ if(!mp->date_unix)zmail_get(mp,4); return mp->date_unix; }
static inline const zmime_address_t *zmail_from(zmail_t *mp){ if(!mp->from_flag)zmail_get(mp,5); return mp->from; }
static inline const zmime_address_t *zmail_from_utf8(zmail_t *mp)
{ if((mp->from_flag!=2))zmail_get(mp,105); return mp->from; }
static inline const zmime_address_t *zmail_sender(zmail_t *mp)
{ if(!mp->sender_flag)zmail_get(mp,6); return mp->sender; }
static inline const zmime_address_t *zmail_reply_to(zmail_t *mp)
{if(!mp->reply_to_flag)zmail_get(mp,7); return mp->reply_to; }
static inline const zmime_address_t *zmail_receipt(zmail_t *mp)
{ if(!mp->receipt_flag)zmail_get(mp,11);return mp->receipt; }
static inline const char *zmail_in_reply_to(zmail_t *mp){return mp->in_reply_to?mp->in_reply_to:zmail_get(mp,12);}
static inline const zvector_t *zmail_to(zmail_t *mp){ if(!mp->to_flag)zmail_get(mp,8); return mp->to; }
static inline const zvector_t *zmail_to_utf8(zmail_t *mp){if(mp->to_flag!=2)zmail_get(mp,108); return mp->to; }
static inline const zvector_t *zmail_cc(zmail_t *mp){ if(!mp->cc_flag)zmail_get(mp,9); return mp->cc; }
static inline const zvector_t *zmail_cc_utf8(zmail_t *mp){if(mp->cc_flag!=2)zmail_get(mp,109); return mp->cc; }
static inline const zvector_t *zmail_bcc(zmail_t *mp){ if(!mp->bcc_flag)zmail_get(mp,10); return mp->bcc; }
static inline const zvector_t *zmail_bcc_utf8(zmail_t *mp){if(mp->bcc_flag!=2)zmail_get(mp,110); return mp->bcc; }
static inline const zvector_t *zmail_references(zmail_t *mp)
{ if(!mp->references_flag)zmail_get(mp,14); return mp->references; }
static inline const zmime_t *zmail_top_mime(zmail_t *mp) { return mp->top_mime; }
static inline const zvector_t *zmail_all_mimes(zmail_t *mp) { return mp->all_mimes; }
static inline const zvector_t *zmail_text_mimes(zmail_t *mp)
{ if (!mp->classify_flag) zmail_get(mp, 62); return mp->text_mimes; }
static inline const zvector_t *zmail_show_mimes(zmail_t *mp)
{ if (!mp->classify_flag) zmail_get(mp, 62); return mp->show_mimes; }
static inline const zvector_t *zmail_attachment_mimes(zmail_t *mp)
{ if (!mp->classify_flag) zmail_get(mp, 62); return mp->attachment_mimes; }
static inline const zvector_t *zmail_all_header_lines(zmail_t *mp){return mp->top_mime->header_lines;}
static inline const zsize_data_t *zmail_header_line(zmail_t *mp, const char *header_name, int sn)
{ return zmime_header_line(mp->top_mime, header_name, sn); }
static inline void zmail_header_lines(zmail_t *mp, const char *header_name, zvector_t *vec)
{ return zmime_header_lines(mp->top_mime, header_name, vec); }

/* ################################################################## */
/* MAIL TNEF PARSER */
struct ztnef_mime_t {
    char *type;
    char *filename;
    char *filename_utf8;
    char *content_id;
    int body_offset;
    int body_len;

    /* relationship */
    ztnef_mime_t *all_last;
    ztnef_mime_t *all_next;

    /* */
    short int mime_type;
    ztnef_t *parser;
};

void ztnef_mime_get_filename_utf8(ztnef_mime_t *m);
static inline const char * ztnef_mime_type(ztnef_mime_t *m) { return m->type; }
static inline const char * ztnef_mime_filename(ztnef_mime_t *m) { return m->filename; }
static inline const char * ztnef_mime_filename_utf8(ztnef_mime_t *m)
{ if (!m->filename_utf8) ztnef_mime_get_filename_utf8(m); return m->filename_utf8; }
static inline const char * ztnef_mime_content_id(ztnef_mime_t *m) { return m->content_id; }
static inline size_t ztnef_mime_body_offset(ztnef_mime_t *m) { return m->body_offset; }
static inline size_t ztnef_mime_body_size(ztnef_mime_t *m) { return m->body_len; }

struct ztnef_t {
    zvector_t *all_mimes;
    char src_charset_def[32];
    /* */
    zmpool_t *mpool;
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
};
static inline const char *ztnef_parser_data(ztnef_t * parser) { return parser->tnef_data;}
static inline size_t ztnef_parser_size(ztnef_t * parser) { return parser->tnef_size;}
ztnef_t *ztnef_parser_create_MPOOL(zmpool_t * imp, char *tnef_data, int tnef_data_len);
ztnef_t *ztnef_parser_create(char *mail_data, int mail_data_len);
void ztnef_parser_option_src_charset_def(ztnef_t * parser, const char *src_charset_def);
void ztnef_parser_run(ztnef_t * parser);
void ztnef_parser_free(ztnef_t * parser);
static inline const zvector_t * ztnef_all_mimes(ztnef_t *parser) { return parser->all_mimes; }

/* ################################################################## */
/* util */
int zquery_line(const char *connection, const char *query, char *result, long timeout);
zbool_t zlicense_mac_check(const char *salt, const char *license);
void zlicense_mac_build(const char *salt, const char *_mac, char *rbuf);

#ifdef LIBZC_MALLOC_NAMESAPCE
#undef zmalloc
#undef zcalloc
#undef zrealloc
#undef zfree
#undef zstrdup
#undef zstrndup
#undef zmemdup
#undef zmemdupnull
#endif

#endif /* ___ZC_INCLUDE_STDLIB_ */
