/* 
 *================================
 *eli960@qq.com
 *http://www.mailhonor.com/
 *2017-02-18
 *================================
 */

#pragma once

#ifndef ___ZC_LIB_INCLUDE___
#define ___ZC_LIB_INCLUDE___
#pragma pack(push, 4)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

#ifdef  __cplusplus
extern "C" {
#endif

/* ################################################################## */
typedef int zbool_t;
typedef union ztype_convert_t ztype_convert_t;
typedef struct zsize_data_t zsize_data_t;
typedef struct zbuf_t zbuf_t;
typedef struct zstrtok_t zstrtok_t;
typedef struct zargv_t zargv_t;
typedef struct zlink_node_t zlink_node_t;
typedef struct zlink_t zlink_t;
typedef struct zvector_t zvector_t;
typedef struct zlist_t zlist_t;
typedef struct zlist_node_t zlist_node_t;
typedef struct zrbtree_node_t zrbtree_node_t;
typedef struct zrbtree_t zrbtree_t;
typedef struct zdict_node_t zdict_node_t;
typedef struct zdict_t zdict_t;
typedef struct zmap_node_t zmap_node_t;
typedef struct zmap_t zmap_t;
typedef struct zmpool_t zmpool_t;
typedef struct zstream_t zstream_t;
#define zconfig_t zdict_t
typedef struct zmmap_reader_t zmmap_reader_t;
typedef struct zeio_t zeio_t;
typedef struct zaio_t zaio_t;
typedef struct zetimer_t zetimer_t;
typedef struct zevent_base_t zevent_base_t;
typedef struct ziopipe_base_t ziopipe_base_t;
typedef struct zcoroutine_t zcoroutine_t;
typedef struct zcoroutine_mutex_t zcoroutine_mutex_t;
typedef struct zcoroutine_cond_t zcoroutine_cond_t;
typedef struct zmail_t zmail_t;
typedef struct zmime_t zmime_t;
typedef struct ztnef_t ztnef_t;
typedef struct ztnef_mime_t ztnef_mime_t;
typedef struct zjson_t zjson_t;
typedef struct zmemcache_client_t zmemcache_client_t;
typedef struct zredis_client_t zredis_client_t;
typedef struct zurl_t zurl_t;
typedef struct zhttpd_upload_file_t zhttpd_upload_file_t;
typedef struct zhttpd_t zhttpd_t;
typedef struct zsqlite3_proxy_client_t zsqlite3_proxy_client_t;

#define zinline inline __attribute__((always_inline))

zinline int zempty(const void *ptr) { return ((!ptr)||(!(*(const char *)(ptr)))); }
#define ZEMPTY(str)                  (!(str)||!(*((const char *)str)))
#define ZCONTAINER_OF(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))
#define ZCONTAINER_OF2(ptr,app_type,offset) ((app_type *) (((char *) (ptr)) - offset))
#define ZCONVERT_CHAR_PTR(const_void_ptr)   (char *)(void*)(const_void_ptr)

union ztype_convert_t {
    const void *ptr_const_void;
    const char *ptr_const_char;
    void *ptr_void;
    char *ptr_char;
    long i_long;
    int i_int;
};

#define ZCHAR_PTR_TO_INT(_ptr, _int)    {ztype_convert_t _ct;_ct.ptr_char=(_ptr);_int=_ct.i_int;}
#define ZINT_TO_CHAR_PTR(_int, _ptr)    {ztype_convert_t _ct;_ct.i_int=(_int);_ptr=_ct.ptr_char;}

#define ZSTR_N_CASE_EQ(a, b, n)       ((ztoupper(a[0]) == ztoupper(b[0])) && (!strncasecmp(a,b,n)))
#define ZSTR_CASE_EQ(a, b)            ((ztoupper(a[0]) == ztoupper(b[0])) && (!strcasecmp(a,b)))
#define ZSTR_N_EQ(a, b, n)            ((a[0] == b[0]) && (!strncmp(a,b,n)))
#define ZSTR_EQ(a, b)                 ((a[0] == b[0]) && (!strcmp(a,b)))

/* log ############################################################ */
extern int zvar_log_fatal_catch;  /*= 0; */
extern int zvar_log_debug_enable; /*= 0; */
extern void (*zlog_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap);
void __attribute__((format(printf,3,4))) zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...);
void __attribute__((format(printf,3,4))) zlog_info(const char *source_fn, size_t line_number, const char *fmt, ...);

#define zfatal(fmt, args...) { zlog_fatal(__FILE__, __LINE__, fmt, ##args); }
#define zinfo(fmt, args...) { zlog_info(__FILE__, __LINE__, fmt, ##args); }
#define zdebug(fmt,args...) { if(zvar_log_debug_enable){zinfo(fmt, ##args);} }

void zlog_use_syslog(const char *identity, int facility);
int zlog_get_facility_from_str(const char *facility);

void zlog_use_masterlog(const char *identity, const char *dest);

/* malloc ########################################################### */
extern char *zblank_buffer;
#define ZFREE(a)                     (zfree(a),a=0)
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

/* buf ############################################################## */
struct zbuf_t {
    char *data;
    int len:31;
    unsigned int static_mode:1;
    int size:31;
    unsigned int unused_flag1:1;
};
#define zbuf_data(b)            ((b)->data)
#define zbuf_len(b)             ((b)->len)
#define ZBUF_PUT(b, c)  \
    (((b)->len<(b)->size)?((int)(((unsigned char *)((b)->data))[(b)->len++]=(int)(c))):(((b)->static_mode?0:zbuf_put_do((b), (c)))))

zbuf_t *zbuf_create(int size);
void zbuf_free(zbuf_t *bf);
void zbuf_init(zbuf_t *bf, int size);
void zbuf_fini(zbuf_t *bf);
int zbuf_need_space(zbuf_t *bf, int need);
int zbuf_put_do(zbuf_t *bf, int ch);
zinline int zbuf_put(zbuf_t *bf, int ch) { int ret = ZBUF_PUT(bf, ch); bf->data[bf->len] = 0; return ret; }
zinline void zbuf_reset(zbuf_t *bf) { bf->len=0, bf->data[0]=0; }
zinline void zbuf_terminate(zbuf_t *bf) { bf->data[bf->len]=0; }
zinline void zbuf_truncate(zbuf_t *bf, int new_len) {
    if ((bf->len>new_len) && (new_len>1)) { bf->len=new_len; bf->data[bf->len] = 0; }
}
int zbuf_strncpy(zbuf_t *bf, const char *src, int len);
int zbuf_strcpy(zbuf_t *bf, const char *src);
int zbuf_strncat(zbuf_t *bf, const char *src, int len);
int zbuf_strcat(zbuf_t *bf, const char *src);
#define zbuf_puts zbuf_strcat
int zbuf_memcpy(zbuf_t *bf, const void *src, int len);
int zbuf_memcat(zbuf_t *bf, const void *src, int len);
zinline int zbuf_append(zbuf_t *bf, zbuf_t *bf2) { return zbuf_memcat(bf, zbuf_data(bf2), zbuf_len(bf2)); }
int zbuf_printf_1024(zbuf_t *bf, const char *format, ...);
int zbuf_trim_right_rn(zbuf_t *bf);

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

/* size_data ####################################################### */
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

/* char string ###################################################### */
extern const unsigned char zchar_lowercase_list[];
extern const unsigned char zchar_uppercase_list[];
#define ztolower(c)    ((int)zchar_lowercase_list[(unsigned char )(c)])
#define ztoupper(c)    ((int)zchar_uppercase_list[(unsigned char )(c)])
char *zstr_tolower(char *str);
char *zstr_toupper(char *str);

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
void zstrtok_init(zstrtok_t *k, const char *sstr);
zstrtok_t *zstrtok(zstrtok_t *k, const char *delim);

/* convert to unit */
int zstr_to_bool(const char *s, int def);
long zstr_to_long(const char *s, long def);
long zstr_to_second(const char *s, long def);
long zstr_to_size(const char *s, long def);

/* find */
char *zmemstr(const void *s, const char *needle, size_t len);
char *zmemcasestr(const void *s, const char *needle, size_t len);

/* argv ############################################################# */
struct zargv_t {
    char **argv;
    int argc:31;
    int unused:1;
    int size:31;
    int mpool_used:1;
};
#define zargv_len(ar)           ((ar)->argc)
#define zargv_argc(ar)          ((ar)->argc)
#define zargv_argv(ar)          ((ar)->argv)
#define zargv_data(ar)          ((ar)->argv)
#define zargv_reset(ar)          (zargv_truncate((ar), 0))
#define ZARGV_WALK_BEGIN(ar, var_your_chp)   {\
    int  zargv_tmpvar_i; const zargv_t *___ar_tmp_ptr = ar; char *var_your_chp; \
        for(zargv_tmpvar_i=0;zargv_tmpvar_i<(___ar_tmp_ptr)->argc;zargv_tmpvar_i++){ \
            var_your_chp = (___ar_tmp_ptr)->argv[zargv_tmpvar_i];
#define ZARGV_WALK_END                       }}

zargv_t *zargv_create(int size);
void zargv_init(zargv_t *argvp, int size);
void zargv_fini(zargv_t *argvp);
void zargv_free(zargv_t *argvp);
void zargv_add(zargv_t *argvp, const char *ns);
void zargv_addn(zargv_t *argvp, const char *ns, int nlen);
void zargv_truncate(zargv_t *argvp, int len);
void zargv_rest(zargv_t *argvp);
zargv_t *zargv_split_append(zargv_t *argvp, const char *string, const char *delim);
void zargv_debug_show(zargv_t *argvp);

/* mlink ############################################################ */
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

/* link ############################################################# */
struct zlink_t {
    zlink_node_t *head;
    zlink_node_t *tail;
};
struct zlink_node_t {
    zlink_node_t *prev;
    zlink_node_t *next;
};
void zlink_init(zlink_t *link);
zlink_node_t *zlink_attach_before(zlink_t *link, zlink_node_t *node, zlink_node_t *before);
zlink_node_t *zlink_detach(zlink_t *link, zlink_node_t *node);
zlink_node_t *zlink_push(zlink_t *link, zlink_node_t *node);
zlink_node_t *zlink_unshift(zlink_t *link, zlink_node_t *node);
zlink_node_t *zlink_pop(zlink_t *link);
#define zlink_head(link)          ((link)->head)
#define zlink_tail(link)          ((link)->tail)
#define zlink_node_prev(node)     ((node)->prev)
#define zlink_node_next(node)     ((node)->next)

/* vector ########################################################### */
struct zvector_t {
    char **data;
    int len;
    int size;
    int offset:31;
    unsigned int mpool_used:1;
};
#define zvector_data(v)        ((v)->data)
#define zvector_len(v)         ((v)->len)
zvector_t *zvector_create(int size);
void zvector_free(zvector_t *v);
void zvector_init(zvector_t *v, int size);
void zvector_fini(zvector_t *v);
#define zvector_add zvector_push
void zvector_push(zvector_t *v, const void *val);
void zvector_unshift(zvector_t *v, const void *val);
zbool_t zvector_pop(zvector_t *v, void **val);
zbool_t zvector_shift(zvector_t *v, void **val);
void zvector_insert(zvector_t *v, int idx, void *val);
zbool_t zvector_delete(zvector_t *v, int idx, void **val);
void zvector_reset(zvector_t *v);
void zvector_truncate(zvector_t *v, int new_len);
#define ZVECTOR_WALK_BEGIN(arr, you_chp_type, var_your_chp)    {\
    int  zvector_tmpvar_i; you_chp_type var_your_chp;\
    for(zvector_tmpvar_i=0;zvector_tmpvar_i<(arr)->len;zvector_tmpvar_i++){\
        var_your_chp = (you_chp_type)((arr)->data[zvector_tmpvar_i]);
#define ZVECTOR_WALK_END                }}

void zbuf_vector_reset(zvector_t *v);
void zbuf_vector_free(zvector_t *v);

/* list ############################################################# */
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
#define zlist_head(c)   ((c)->head)
#define zlist_tail(c)   ((c)->tail)
#define zlist_len(c)    ((c)->len)
#define zlist_node_next(n)   ((n)->next)
#define zlist_node_prev(n)   ((n)->prev)
#define zlist_node_value(n)  ((n)->value)
zlist_t *zlist_create(void);
void zlist_free(zlist_t *list);
void zlist_init(zlist_t *list);
void zlist_fini(zlist_t *list);
void zlist_reset(zlist_t *list);
void zlist_attach_before(zlist_t *list, zlist_node_t *n, zlist_node_t *before);
void zlist_detach(zlist_t *list, zlist_node_t *n);
zlist_node_t *zlist_add_before(zlist_t *list, const void *value, zlist_node_t *before);
int zlist_delete(zlist_t *list, zlist_node_t *n, void **value);
zinline zlist_node_t *zlist_push(zlist_t *l,const void *v){return zlist_add_before(l,v,0);}
zinline zlist_node_t *zlist_unshift(zlist_t *l,const void *v){return zlist_add_before(l,v,l->head);}
zinline int zlist_pop(zlist_t *l, void **v){return zlist_delete(l,l->tail,v);}
zinline int zlist_shift(zlist_t *l, void **v){return zlist_delete(l,l->head,v);}

#define ZLIST_WALK_BEGIN(list, var_your_type, var_your_ptr)  { \
    zlist_node_t *list_current_node=(list)->head; var_your_type var_your_ptr; \
    for(;list_current_node;list_current_node=list_current_node->next){ \
        var_your_ptr = (var_your_type)(void *)(list_current_node->value);
#define ZLIST_WALK_END                          }}

#define ZLIST_NODE_WALK_BEGIN(list, var_your_node)  { \
    zlist_node_t *var_your_node=(list)->head;\
    for(;var_your_node;var_your_node=var_your_node->next){
#define ZLIST_NODE_WALK_END                          }}
/* rbtree ########################################################### */
typedef int (*zrbtree_cmp_t) (zrbtree_node_t *node1, zrbtree_node_t *node2);
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
zinline int zrbtree_have_data(zrbtree_t *tree) { return ZRBTREE_HAVE_DATA(tree); }
void zrbtree_init(zrbtree_t *tree, zrbtree_cmp_t cmp_fn);
void zrbtree_insert_color(zrbtree_t *, zrbtree_node_t *);
void zrbtree_erase(zrbtree_t *tree, zrbtree_node_t *node);
void zrbtree_replace_node(zrbtree_t *tree, zrbtree_node_t *victim, zrbtree_node_t *_new);
zrbtree_node_t *zrbtree_prev(const zrbtree_node_t *tree);
zrbtree_node_t *zrbtree_next(const zrbtree_node_t *tree);
zrbtree_node_t *zrbtree_first(const zrbtree_t *node);
zrbtree_node_t *zrbtree_last(const zrbtree_t *node);
zrbtree_node_t *zrbtree_near_prev(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_near_next(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_parent(const zrbtree_node_t *node);
zrbtree_node_t *zrbtree_attach(zrbtree_t *tree, zrbtree_node_t *node);
zrbtree_node_t *zrbtree_find(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_detach(zrbtree_t *tree, zrbtree_node_t *node);
void zrbtree_link_node(zrbtree_node_t *node, zrbtree_node_t *parent, zrbtree_node_t **zrbtree_link);

#define ZRBTREE_INIT(tree, _cmp_fn)     ((tree)->zrbtree_node=0, (tree)->cmp_fn = _cmp_fn)
#define ZRBTREE_PARENT(node)    ((zrbtree_node_t *)((node)->__zrbtree_parent_color & ~3))
#define ZRBTREE_ATTACH_PART1(root, node, cmp_node) {                            \
    zrbtree_node_t **___Z_new_pp = &((root)->zrbtree_node), *___Z_parent = 0;            \
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
    zrbtree_node_t *___Z_node_tmp = (root)->zrbtree_node;                        \
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
        zrbtree_node_t *var_your_node = ___Z_node = ___Z_list[___Z_idx].node;                    \
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
    zrbtree_node_t *var_your_node; \
    for (var_your_node = zrbtree_first(root); var_your_node; var_your_node = zrbtree_next(var_your_node)) {
#define ZRBTREE_WALK_FORWARD_END                }}

#define ZRBTREE_WALK_BACK_BEGIN(root, var_your_node)     {                    \
    zrbtree_node_t *var_your_node; \
    for (var_your_node = zrbtree_last(root); var_your_node; var_your_node = zrbtree_prev(var_your_node)) {
#define ZRBTREE_WALK_BACK_END                }}

/* dict ############################################################# */
struct zdict_t {
    zrbtree_t rbtree;
    int len;
};
struct zdict_node_t {
    zrbtree_node_t rbnode;
    zbuf_t value;
    char *key;
};
zdict_t *zdict_create(void);
void zdict_free(zdict_t *dict);
void zdict_reset(zdict_t *dict);
zdict_node_t *zdict_update(zdict_t *dict, const char *key, const zbuf_t *value);
zdict_node_t *zdict_update_string(zdict_t *dict, const char *key, const char *value, int len);
zdict_node_t *zdict_find(const zdict_t *dict, const char *key, zbuf_t **value);
zdict_node_t *zdict_find_near_prev(const zdict_t *dict, const char *key, zbuf_t **value);
zdict_node_t *zdict_find_near_next(const zdict_t *dict, const char *key, zbuf_t **value);
void zdict_delete_node(zdict_t *dict, zdict_node_t *n);
zdict_node_t *zdict_first(const zdict_t *dict);
zdict_node_t *zdict_last(const zdict_t *dict);
zdict_node_t *zdict_prev(const zdict_node_t *node);
zdict_node_t *zdict_next(const zdict_node_t *node);
char *zdict_get_str(const zdict_t *dict, const char *name, const char *def);
int zdict_get_bool(const zdict_t *dict, const char *name, int def);
int zdict_get_int(const zdict_t *dict, const char *name, int def, int min, int max);
long zdict_get_long(const zdict_t *dict, const char *name, long def, long min, long max);
long zdict_get_second(const zdict_t *dict, const char *name, long def, long min, long max);
long zdict_get_size(const zdict_t *dict, const char *name, long def, long min, long max);
void zdict_debug_show(const zdict_t *dict);
zinline int zdict_len(const zdict_t *dict) { return dict->len; }
zinline char *zdict_key(const zdict_node_t *node) { return node->key; }
zinline zbuf_t *zdict_value(const zdict_node_t *node) { return &(((zdict_node_t*)node)->value); }
#define zdict_len(dict)            ((dict)->len)
#define zdict_node_key(n)               ((n)->key)
#define zdict_node_value(n)             (&((n)->value))
#define ZDICT_WALK_BEGIN(dict, var_your_key, var_your_value)  { \
    zdict_node_t *var_your_node; char *var_your_key; zbuf_t *var_your_value; \
    for(var_your_node = zdict_first(dict); var_your_node; var_your_node = zdict_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=&(var_your_node->value); {
#define ZDICT_WALK_END    }}}

#define ZDICT_NODE_WALK_BEGIN(dict, var_your_node)  { \
    zdict_node_t *var_your_node; \
    for(var_your_node = zdict_first(dict); var_your_node; var_your_node = zdict_next(var_your_node)) {
#define ZDICT_NODE_WALK_END    }}

/* map ############################################################## */
struct zmap_t {
    zrbtree_t rbtree;
    int len;
};
struct zmap_node_t {
    char *key;
    void *value;
    zrbtree_node_t rbnode;
};
zmap_t *zmap_create(void);
zmap_node_t *zmap_update(zmap_t *map, const char *key, const void *value, void **old_value);
zmap_node_t *zmap_find(const zmap_t *map, const char *key, void **value);
zmap_node_t *zmap_find_near_prev(const zmap_t *map, const char *key, void **value);
zmap_node_t *zmap_find_near_next(const zmap_t *map, const char *key, void **value);
zbool_t zmap_delete(zmap_t * map, const char *key, void **old_value);
void zmap_delete_node(zmap_t *map, zmap_node_t *n, void **old_value);
void zmap_node_update(zmap_node_t *n, const void *value, void **old_value);
zmap_node_t *zmap_first(const zmap_t *map);
zmap_node_t *zmap_last(const zmap_t *map);
zmap_node_t *zmap_prev(const zmap_node_t *node);
zmap_node_t *zmap_next(const zmap_node_t *node);
void zmap_free(zmap_t *map);
zinline int zmap_len(const zmap_t *map) { return map->len; }
zinline char *zmap_key(const zmap_node_t *node) { return node->key; }
zinline void *zmap_value(const zmap_node_t *node) { return node->value; }
void zmap_reset(zmap_t *map);
#define zmap_len(map)            ((map)->len)
#define zmap_node_key(n)              ((n)->key)
#define zmap_node_value(n)            ((n)->value)
#define ZMAP_NODE_WALK_BEGIN(map, var_your_node)  { \
    zmap_node_t *var_your_node; \
    for(var_your_node = zmap_first(map); var_your_node; var_your_node = zmap_next(var_your_node)) {
#define ZMAP_NODE_WALK_END    }}

#define ZMAP_WALK_BEGIN(map, var_your_key, var_your_value_type, var_your_value)  { \
    zmap_node_t *var_your_node; char *var_your_key; var_your_value_type var_your_value; \
    (void)var_your_key; (void)var_your_value; \
    for(var_your_node = zmap_first(map); var_your_node; var_your_node = zmap_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=(var_your_value_type)(void *)var_your_node->value; {
#define ZMAP_WALK_END    }}}

/* mpool ############################################################# */
typedef struct zmpool_method_t zmpool_method_t;
struct zmpool_method_t {
    void *(*malloc) (zmpool_t *, int);
    void *(*calloc) (zmpool_t *, int, int);
    void *(*realloc) (zmpool_t *, const void *, int);
    void (*free) (zmpool_t *, const void *);
    void (*reset) (zmpool_t *);
    void (*free_pool) (zmpool_t *);
};
struct zmpool_t {
    zmpool_method_t *method;
};
extern zmpool_t *zvar_system_mpool;
zmpool_t *zmpool_create_common_pool(int *register_size_list);
zmpool_t *zmpool_create_greedy_pool(int single_buf_size, int once_malloc_max_size);
void zmpool_free_pool(zmpool_t * mp);
zinline void *zmpool_malloc(zmpool_t * mp, int len) { return mp->method->malloc(mp, len); }
zinline void *zmpool_calloc(zmpool_t * mp, int nmemb, int size)
{
    return mp->method->calloc(mp, nmemb, size);
}
zinline void *zmpool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    return mp->method->realloc(mp, ptr, len);
}
zinline void zmpool_free(zmpool_t * mp, const void *ptr) { 
    if(ptr&&(ptr!=(const void *)zblank_buffer)) { mp->method->free(mp, ptr);}
}
void *zmpool_strdup(zmpool_t * mp, const char *ptr);
void *zmpool_strndup(zmpool_t * mp, const char *ptr, int n);
void *zmpool_memdup(zmpool_t * mp, const void *ptr, int n);
void *zmpool_memdupnull(zmpool_t * mp, const void *ptr, int n);
void zmpool_reset(zmpool_t * mp);

/* encode/decode ################################################### */
int zbase64_encode(const void *src, int src_size, zbuf_t *str, int mime_flag);
int zbase64_decode(const void *src, int src_size, zbuf_t *str, int *dealed_size);
int zbase64_decode_get_valid_len(const void *src, int src_size);
int zbase64_encode_get_min_len(int in_len, int mime_flag);

int zqp_decode_2045(const void *src, int src_size, zbuf_t *str);
int zqp_decode_2047(const void *src, int src_size, zbuf_t *str);
int zqp_decode_get_valid_len(const void *src, int src_size);

extern char zhex_to_dec_table[];
int zhex_encode(const void *src, int src_size, zbuf_t *dest);
int zhex_decode(const void *src, int src_size, zbuf_t *dest);
int zurl_hex_decode(const void *src, int src_size, zbuf_t *str);
void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag);

int zncr_decode(int ins, char *wchar);

/* crc ############################################################# */
unsigned short int zcrc16(const void *data, int size, unsigned short int init_value);
unsigned int zcrc32(const void *data, int size, unsigned int init_value);
unsigned long zcrc64(const void *data, int size, unsigned long init_value);

/* config ########################################################## */
extern zconfig_t *zvar_default_config;
zconfig_t *zdefault_config_init(void);
void zdefault_config_fini(void);
#define zconfig_create  zdict_create
#define zconfig_free    zdict_free
#define zconfig_update  zdict_update
#define zconfig_update_string  zdict_update_string
#define zconfig_delete   zdict_delete
#define zconfig_debug_show    zdict_debug_show

/* config load */
int zconfig_load_from_filename(zconfig_t *cf, const char *filename);
void zconfig_load_annother(zconfig_t *cf, zconfig_t *another);

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
#define zconfig_bool_table_t    zconfig_int_table_t
#define zconfig_second_table_t  zconfig_long_table_t
#define zconfig_size_table_t    zconfig_long_table_t
#define zconfig_get_str         zdict_get_str
#define zconfig_get_bool        zdict_get_bool
#define zconfig_get_int         zdict_get_int
#define zconfig_get_long        zdict_get_long
#define zconfig_get_second      zdict_get_second
#define zconfig_get_size        zdict_get_size
void zconfig_get_str_table(zconfig_t *cf, zconfig_str_table_t *table);
void zconfig_get_int_table(zconfig_t *cf, zconfig_int_table_t *table);
void zconfig_get_long_table(zconfig_t *cf, zconfig_long_table_t *table);
void zconfig_get_bool_table(zconfig_t *cf, zconfig_bool_table_t *table);
void zconfig_get_second_table(zconfig_t *cf, zconfig_second_table_t *table);
void zconfig_get_size_table(zconfig_t *cf, zconfig_size_table_t *table);
#define ZCONFIG_WALK_BEGIN(cf, key, value) { \
    zdict_node_t *___nd; char *key; zbuf_t *value; \
    for (___nd = zdict_first(cf);___nd;___nd=zdict_next(___nd)) { \
        key = zdict_node_key(___nd); value = zdict_node_value(___nd); {
#define ZCONFIG_WALK_END }}}

/* io ############################################################# */
/* return , -1: error, 0: not, 1: yes */
int zrwable(int fd);
int zreadable(int fd);
int zwriteable(int fd);
int znonblocking(int fd, int no);
int zclose_on_exec(int fd, int on);
int zget_readable_count(int fd);

int zclose(int fd);
int zflock(int fd, int operation);
int zflock_share(int fd);
int zflock_exclusive(int fd);
int zfunlock(int fd);
int zrename(const char *oldpath, const char *newpath);
int zunlink(const char *pathname);

/* timed_io ######################################################## */
/* -1: error, 0: not, 1: yes */
int ztimed_read_write_wait(int fd, int timeout, int *readable, int *writeable);
int ztimed_read_write_wait_millisecond(int fd, long timeout, int *readable, int *writeable);
/* -1: error, 0: not, 1: yes */
int ztimed_read_wait_millisecond(int fd, long timeout);
int ztimed_read_wait(int fd, int timeout);
/* < 0: error, >=0: bytes of read */
int ztimed_read(int fd, void *buf, int size, int timeout);
/* strictly read n bytes */
int ztimed_readn(int fd, void *buf, int size, int timeout);
/* -1: error, 0: not, 1: yes */
int ztimed_write_wait_millisecond(int fd, long timeout);
int ztimed_write_wait(int fd, int timeout);
/* strictly write n btyes */
int ztimed_write(int fd, const void *buf, int size, int timeout);

/* tcp socket ##################################################### */
#define zvar_tcp_listen_type_inet  'i'
#define zvar_tcp_listen_type_unix  'u'
#define zvar_tcp_listen_type_fifo  'f'
int zunix_accept(int fd);
int zinet_accept(int fd);
int zaccept(int sock, int type);
int zunix_listen(char *addr, int backlog, int nonblock_flag);
int zinet_listen(const char *sip, int port, int backlog, int nonblock_flag);
int zlisten(const char *netpath, int *type, int backlog, int nonblock_flag);
int zfifo_listen(const char *path);
int zunix_connect(const char *addr, int nonblock_flag, int timeout);
int zinet_connect(const char *dip, int port, int nonblock_flag, int timeout);
int zhost_connect(const char *host, int port, int nonblock_flag, int timeout);
int zconnect(const char *netpath, int nonblock_flag, int timeout);

/* openssl ########################################################## */
extern int zvar_openssl_debug;
void zopenssl_init(void);
void zopenssl_fini(void);
void zopenssl_phtread_fini(void);
SSL_CTX *zopenssl_SSL_CTX_create_server(void);
SSL_CTX *zopenssl_SSL_CTX_create_client(void);
int zopenssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file);
void zopenssl_SSL_CTX_free(SSL_CTX *ctx);
void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len);
SSL *zopenssl_SSL_create(SSL_CTX *ctx, int fd);
void zopenssl_SSL_free(SSL *ssl);
int zopenssl_SSL_get_fd(SSL *ssl);
int zopenssl_timed_connect(SSL *ssl, int timeout);
int zopenssl_timed_accept(SSL *ssl, int timeout);
int zopenssl_timed_shutdown(SSL *ssl, int timeout);
int zopenssl_timed_read(SSL *ssl, void *buf, int len, int timeout);
int zopenssl_timed_write(SSL *ssl, const void *buf, int len, int timeout);

/* stream ########################################################### */
#define zvar_stream_rbuf_size           4096
#define zvar_stream_wbuf_size           4096

struct zstream_t {
    short int read_buf_p1;
    short int read_buf_p2;
    short int write_buf_len;
    unsigned short int error:1;
    unsigned short int eof:1;
    unsigned short int ssl_mode:1;
    unsigned short int file_mode:1;
    unsigned char read_buf[zvar_stream_rbuf_size];
    unsigned char write_buf[zvar_stream_wbuf_size];
    long cutoff_time;
    union { int fd; SSL *ssl; } ioctx;
};

#define ZSTREAM_GETC(fp)            (((fp)->read_buf_p1<(fp)->read_buf_p2)?((int)((fp)->read_buf[(fp)->read_buf_p1++])):(zstream_getc_do(fp)))
#define ZSTREAM_PUTC(fp, ch)        (((fp)->write_buf_len<zvar_stream_wbuf_size)?((fp)->write_buf[(fp)->write_buf_len++]=(int)(ch),(int)(ch)):(zstream_putc_do(fp, ch)))

#define zstream_is_error(fp)        ((fp)->error)
#define zstream_is_eof(fp)          ((fp)->eof)
#define zstream_is_exception(fp)    ((fp)->eof||(fp)->error)
#define zstream_get_read_cache_len(fp) ((fp)->read_buf_p2-(fp)->read_buf_p1)

zstream_t *zstream_open_fd(int fd);
zstream_t *zstream_open_ssl(SSL *ssl);
zstream_t *zstream_open_file(const char *pathname, const char *mode);
zstream_t *zstream_open_destination(const char *destination, int timeout);
int zstream_close(zstream_t *fp, zbool_t close_fd_and_release_ssl);

int zstream_get_fd(zstream_t *fp);
SSL *zstream_get_ssl(zstream_t *fp);
int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx);
int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx);
void zstream_set_timeout(zstream_t *fp, int timeout);
int zstream_timed_read_wait(zstream_t *fp, int timeout);
int zstream_timed_write_wait(zstream_t *fp, int timeout);

int zstream_getc_do(zstream_t *fp);
zinline int zstream_getc(zstream_t *fp) { return ZSTREAM_GETC(fp); }
void zstream_ungetc(zstream_t *fp);
int zstream_read(zstream_t *fp, zbuf_t *bf, int max_len);
int zstream_readn(zstream_t *fp, zbuf_t *bf, int strict_len);
int zstream_gets_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len);
zinline int zstream_gets(zstream_t *fp, zbuf_t *bf, int max_len)
{
    return zstream_gets_delimiter(fp, bf, '\n', max_len);
}
int zstream_size_data_get_size(zstream_t *fp);

int zstream_putc_do(zstream_t *fp, int ch);
zinline int zstream_putc(zstream_t *fp, int c) { return ZSTREAM_PUTC(fp, c); }
int zstream_write(zstream_t *fp, const void *buf, int len);
int zstream_puts(zstream_t *fp, const char *s);
#define zstream_puts_const(fp, s) zstream_write(fp, s, sizeof(s)-1)
zinline int zstream_append(zstream_t *fp, zbuf_t *bf) {
    return zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
}
int zstream_printf_1024(zstream_t *fp, const char *format, ...);
int zstream_write_size_data_size(zstream_t *fp, int len);
int zstream_write_size_data(zstream_t *fp, const void *buf, int len);
int zstream_write_size_data_int(zstream_t *fp, int i);
int zstream_write_size_data_long(zstream_t *fp, long i);
int zstream_write_size_data_dict(zstream_t *fp, zdict_t * zd);
int zstream_write_size_data_pp(zstream_t *fp, const char **pp, int size);
int zstream_flush(zstream_t *fp);

/* time ############################################################# */
#define zvar_max_timeout_millisecond (3600L * 24 * 365 * 10 * 1000)
long zmillisecond(void);
void zsleep_millisecond(int delay);
long ztimeout_set_millisecond(long timeout);
long ztimeout_left_millisecond(long stamp);

#define zvar_max_timeout (3600 * 24 * 365 * 10)
long zsecond(void);
void zsleep(int delay);
long ztimeout_set(int timeout);
int ztimeout_left(long stamp);

/* date ############################################################# */
#define zvar_rfc1123_date_string_size 32
char *zbuild_rfc1123_date_string(long t, char *buf);

/* dns ############################################################## */
int zget_localaddr(zargv_t *addrs);
int zget_hostaddr(const char *host, zargv_t *addrs);
int zget_peername(int sockfd, int *host, int *port);
char *zget_ipstring(int ip, char *ipstr);
int zget_ipint(const char *ipstr);
int zget_network(int ip, int masklen);
int zget_netmask(int masklen);
int zget_broadcast(int ip, int masklen);
int zget_ip_min(int ip, int masklen);
int zget_ip_max(int ip, int masklen);
int zip_is_intranet(int ip);
int zip_is_intranet2(const char *ip);

/* mime type ######################################################## */
extern const char *zvar_mime_type_application_cotec_stream;
const char *zget_mime_type_from_suffix(const char *suffix, const char *def);
const char *zget_mime_type_from_filename(const char *filename, const char *def);

/* unique id ######################################################## */
#define zvar_unique_id_size 22
char *zbuild_unique_id(char *buf);
long zget_time_from_unique_id(char *buf);

/* system ########################################################### */
int zchroot_user(const char *root_dir, const char *user_name);
/* file */
int zfile_get_size(const char *filename);
int zfile_put_contents(const char *filename, const void *data, int len);
int zfile_get_contents(const char *filename, zbuf_t *result);
int zfile_get_contents_sample(const char *filename, zbuf_t *result);
int zstdin_get_contents(zbuf_t *bf);
/* mmap reader */
struct zmmap_reader_t {
    int fd;
    int len;
    char *data;
};
int zmmap_reader_init(zmmap_reader_t *reader, const char *filename);
int zmmap_reader_fini(zmmap_reader_t *reader);

/* mac */
int zget_mac_address(zargv_t *mac_list);

/* main main_parameter ################################################## */
extern char *zvar_progname;
extern int zvar_proc_stop;
extern int zvar_test_mode;
extern int zvar_max_fd;

extern char **zvar_main_parameter_argv;
extern int zvar_main_parameter_argc;
void zmain_parameter_run(int argc, char **argv);
/* not thread-safe */
void zinner_atexit(void (*function)(void));

/* license ############################################################## */
int zlicense_mac_check(const char *salt, const char *license);
void zlicense_mac_build(const char *salt, const char *_mac, zbuf_t *result);
int zlicense_mac_check_from_config_filename(const char *salt, const char *config_file, const char *key);

/* event ############################################################### */
/* event io based on zevent_base_t */
zeio_t *zeio_create(int fd, zevent_base_t *evbase);
void zeio_free(zeio_t *eio, int close_fd);
void zeio_set_local(zeio_t *eio);
int zeio_get_result(zeio_t *eio);
int zeio_get_fd(zeio_t *eio);
void zeio_enable_read(zeio_t *eio, void (*callback)(zeio_t *eio));
void zeio_enable_write(zeio_t *eio, void (*callback)(zeio_t *eio));
void zeio_disable(zeio_t *eio);
void zeio_set_context(zeio_t *eio, const void *ctx);
void *zeio_get_context(zeio_t *eio);
zevent_base_t *zeio_get_event_base(zeio_t *eio);

/* async io based on zevent_base_t */
zaio_t *zaio_create(int fd, zevent_base_t *evbase);
void zaio_free(zaio_t *aio, int close_fd_and_release_ssl);
void zaio_set_local(zaio_t *aio);
int zaio_get_result(zaio_t *aio);
int zaio_get_fd(zaio_t *aio);
SSL *zaio_get_ssl(zaio_t *aio);
void zaio_tls_connect(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio), int timeout);
void zaio_tls_accept(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio), int timeout);
void zaio_fetch_rbuf(zaio_t *aio, zbuf_t *bf, int strict_len);
void zaio_fetch_rbuf_data(zaio_t *aio, void *data, int strict_len);
void zaio_read(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio), int timeout);
void zaio_readn(zaio_t *aio, int strict_len, void (*callback)(zaio_t *aio), int timeout);
void zaio_read_size_data(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout);
void zaio_gets_delimiter(zaio_t *aio, int delimiter, int max_len, void (*callback)(zaio_t *aio), int timeout);
void zaio_gets(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio), int timeout);
void zaio_cache_printf_1024(zaio_t *aio, const char *fmt, ...);
void zaio_cache_puts(zaio_t *aio, const char *s);
void zaio_cache_write(zaio_t *aio, const void *buf, int len);
void zaio_cache_write_size_data(zaio_t *aio, const void *buf, int len);
void zaio_cache_write_direct(zaio_t *aio, const void *buf, int len);
void zaio_cache_flush(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout);
int zaio_get_cache_size(zaio_t *aio);
void zaio_sleep(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout);
void zaio_set_context(zaio_t *aio, const void *ctx);
void *zaio_get_context(zaio_t *aio);
zevent_base_t *zaio_get_event_base(zaio_t *aio);

void zaio_list_append(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio);
void zaio_list_detach(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio);

/* event timer based on zevent_base_t */
zetimer_t *zetimer_create(zevent_base_t *evbase);
void zetimer_free(zetimer_t *et);
void zetimer_start(zetimer_t *et, void (*callback)(zetimer_t *et), int timeout);
void zetimer_stop(zetimer_t *et);
void zetimer_set_local(zetimer_t *et);
void zetimer_set_context(zetimer_t *et, const void *ctx);
void *zetimer_get_context(zetimer_t *et);
zevent_base_t *zetimer_get_event_base(zetimer_t *et);

/* event base */
extern zevent_base_t *zvar_default_event_base;
zevent_base_t *zevent_base_create();
void zevent_base_free(zevent_base_t *eb);
zbool_t zevent_base_dispatch(zevent_base_t *eb);
void zevent_base_notify(zevent_base_t *eb);
void zevent_base_set_local(zevent_base_t *eb);
void zevent_base_set_context(zevent_base_t *eb, const void *ctx);
void *zevent_base_get_context(zevent_base_t *eb);

/* iopipe ########################################################### */
typedef void (*ziopipe_after_close_fn_t) (void *);
ziopipe_base_t *ziopipe_base_create();
void ziopipe_base_free(ziopipe_base_t *iopb);
int ziopipe_base_get_count(ziopipe_base_t *iopb);
void ziopipe_base_stop_notify(ziopipe_base_t *iopb);
void ziopipe_base_after_peer_closed_timeout(ziopipe_base_t *iopb, int timeout);
void ziopipe_base_run(ziopipe_base_t *iopb);
void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, ziopipe_after_close_fn_t after_close, const void *context);

/* coroutine ########################################################## */
void zcoroutine_base_init();
void zcoroutine_base_run();
void zcoroutine_base_stop_notify();
void zcoroutine_base_fini();

void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_size);

zcoroutine_t * zcoroutine_self();
void zcoroutine_yield();
void zcoroutine_exit();
#define zcoroutine_sleep zsleep
#define zcoroutine_sleep_millisecond zsleep_millisecond
void *zcoroutine_get_context();
void zcoroutine_set_context(const void *ctx);
void zcoroutine_enable_fd(int fd);
void zcoroutine_disable_fd(int fd);

zcoroutine_mutex_t * zcoroutine_mutex_create();
void zcoroutine_mutex_free(zcoroutine_mutex_t *);
void zcoroutine_mutex_lock(zcoroutine_mutex_t *);
void zcoroutine_mutex_unlock(zcoroutine_mutex_t *);

zcoroutine_cond_t * zcoroutine_cond_create();
void zcoroutine_cond_free(zcoroutine_cond_t *);
void zcoroutine_cond_wait(zcoroutine_cond_t *, zcoroutine_mutex_t *);
void zcoroutine_cond_signal(zcoroutine_cond_t *);
void zcoroutine_cond_broadcast(zcoroutine_cond_t *);

/* 启用limit个线程池, 用于文件io,和 block_do */
void zcoroutine_set_block_pthread_limit(size_t limit);
void *coroutine_block_do(void *(*block_func)(void *ctx), void *ctx);
/* 文件io是否用线程池模式, 前提是 zcoroutine_set_block_pthread_limit(limit>0) */
void zcoroutine_set_fileio_use_block_pthread(int flag);

void zcoroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, void (*after_close)(void *ctx), void *ctx);

/* master ############################################################# */
/* master_server */
extern int zvar_master_server_log_debug_enable;
extern int zvar_master_server_reload_signal; /* SIGHUP */

extern void (*zmaster_server_load_config)(zvector_t *cfs);
extern void (*zmaster_server_before_service)();

void zmaster_server_load_config_from_dirname(const char *config_dir_pathname, zvector_t *cfs);
void zmaster_server_main(int argc, char **argv);

/* master_event_server */
extern void (*zevent_server_service_register) (const char *service, int fd, int fd_type);
extern void (*zevent_server_before_service) (void);
extern void (*zevent_server_before_reload) (void);
extern void (*zevent_server_before_exit) (void);

void zevent_server_stop_notify(void);
zeio_t *zevent_server_general_aio_register(zevent_base_t *eb, int fd, int fd_type, void (*callback) (int));
int zevent_server_main(int argc, char **argv);

/* master_coroutine_server */
extern void (*zcoroutine_server_service_register) (const char *service, int fd, int fd_type);
extern void (*zcoroutine_server_before_service) (void);
extern void (*zcoroutine_server_before_reload) (void);
extern void (*zcoroutine_server_before_exit) (void);

void zcoroutine_server_stop_notify(void);
int zcoroutine_server_main(int argc, char **argv);

/* charset ############################################################ */
#define zvar_charset_name_max_size          32
extern int zvar_charset_debug;
extern const char *zvar_charset_chinese[];
extern const char *zvar_charset_japanese[];
extern const char *zvar_charset_korean[];
extern const char *zvar_charset_cjk[];
char *zcharset_correct_charset(const char *charset);
char *zcharset_detect(const char **charset_list, const char *data, int size, zbuf_t *charset_result);
char *zcharset_detect_cjk(const char *data, int size, zbuf_t *charset_result);

int zcharset_iconv(const char *from_charset, const char *src, int src_len,
        const char *to_charset, zbuf_t *result, int *src_converted_len,
        int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);

/* mime utils ######################################################## */
#define zvar_mime_header_line_max_length   1024000

/* iconv */
void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result);

/* general header line utils */
void zmime_raw_header_line_unescape(const char *in_line, int in_len, zbuf_t *result);
void zmime_header_line_get_first_token(const char *in_line, int in_len, zbuf_t *result);

typedef struct zmime_header_line_element_t zmime_header_line_element_t;
struct zmime_header_line_element_t {
    char *charset;
    char *data;
    int dlen;
    char encode_type; /* 'B':base64, 'Q':qp, 0:unknown */
};
const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len);
void zmime_header_line_element_vector_free(const zvector_t *element_vector);

void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result);
void zmime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, int with_charset_flag);

void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *value, zdict_t *params);
long zmime_header_line_decode_date(const char *str);

/* address */
typedef struct zmime_address_t zmime_address_t;
struct zmime_address_t {
    char *name;
    char *address;
    char *name_utf8;
};
zvector_t *zmime_header_line_get_address_vector(const char *in_str, int in_len);
zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len);
void zmime_header_line_address_vector_free(zvector_t *address_vector);

/* mail parser */
const char *zmime_get_type(zmime_t *mime);
const char *zmime_get_encoding(zmime_t *mime);
const char *zmime_get_charset(zmime_t *mime);
const char *zmime_get_disposition(zmime_t *mime);
const char *zmime_get_show_name(zmime_t *mime);
const char *zmime_get_name(zmime_t *mime);
const char *zmime_get_name_utf8(zmime_t *mime);
const char *zmime_get_filename(zmime_t *mime);
const char *zmime_get_filename2231(zmime_t *mime, zbool_t *with_charset_flag);
const char *zmime_get_filename_utf8(zmime_t *mime);
const char *zmime_get_content_id(zmime_t *mime);
const char *zmime_get_boundary(zmime_t *mime);
const char *zmime_get_imap_section(zmime_t *mime);
const char *zmime_get_header_data(zmime_t *mime);
const char *zmime_get_body_data(zmime_t *mime);
int zmime_get_header_offset(zmime_t *mime);
int zmime_get_header_len(zmime_t *mime);
int zmime_get_body_offset(zmime_t *mime);
int zmime_get_body_len(zmime_t *mime);
zmime_t *zmime_next(zmime_t *mime);
zmime_t *zmime_child(zmime_t *mime);
zmime_t *zmime_parent(zmime_t *mime);
const zvector_t *zmime_get_raw_header_line_vector(zmime_t *mime); /* zsize_data_t* */
int zmime_get_raw_header_line(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);
int zmime_get_header_line_value(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);
void zmime_get_decoded_content(zmime_t *mime, zbuf_t *result);
void zmime_get_decoded_content_utf8(zmime_t *mime, zbuf_t *result);
zbool_t zmime_is_tnef(zmime_t *mime);

zmail_t *zmail_create_parser_from_data(const char *mail_data, int mail_data_len, const char *default_charset);
zmail_t *zmail_create_parser_from_filename(const char *filename, const char *default_charset);
void zmail_free(zmail_t *parser);
void zmail_debug_show(zmail_t *parser);
const char *zmail_get_data(zmail_t *parser);
int zmail_get_len(zmail_t *parser);
const char *zmail_get_header_data(zmail_t *parser);
int zmail_get_header_offset(zmail_t *parser);
int zmail_get_header_len(zmail_t *parser);
const char *zmail_get_body_data(zmail_t *parser);
int zmail_get_body_offset(zmail_t *parser);
int zmail_get_body_len(zmail_t *parser);
const char *zmail_get_message_id(zmail_t *parser);
const char *zmail_get_subject(zmail_t *parser);
const char *zmail_get_subject_utf8(zmail_t *parser);
const char *zmail_get_date(zmail_t *parser);
const char *zmail_get_in_reply_to(zmail_t *parser);
long zmail_get_date_unix(zmail_t *parser);
const zmime_address_t *zmail_get_from(zmail_t *parser);
const zmime_address_t *zmail_get_from_utf8(zmail_t *parser);
const zmime_address_t *zmail_get_sender(zmail_t *parser);
const zmime_address_t *zmail_get_reply_to(zmail_t *parser);
const zmime_address_t *zmail_get_receipt(zmail_t *parser);
const zvector_t *zmail_get_to(zmail_t *parser); /* zmime_address_t* */
const zvector_t *zmail_get_to_utf8(zmail_t *parser);
const zvector_t *zmail_get_cc(zmail_t *parser);
const zvector_t *zmail_get_cc_utf8(zmail_t *parser);
const zvector_t *zmail_get_bcc(zmail_t *parser);
const zvector_t *zmail_get_bcc_utf8(zmail_t *parser);
const zargv_t *zmail_get_references(zmail_t *parser);
const zmime_t *zmail_get_top_mime(zmail_t *parser);
const zvector_t *zmail_get_all_mimes(zmail_t *parser); /* zmime_t* */
const zvector_t *zmail_get_text_mimes(zmail_t *parser);
const zvector_t *zmail_get_show_mimes(zmail_t *parser);
const zvector_t *zmail_get_attachment_mimes(zmail_t *parser);
const zvector_t *zmail_get_raw_header_line_vector(zmail_t *parser); /* zsize_data_t* */
zbool_t zmail_get_raw_header_line(zmail_t *parser, const char *header_name, zbuf_t *result, int sn); /* 0:first, 1:second, -1:last */
zbool_t zmail_get_header_line_value(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

/* tnef */
const char *ztnef_mime_get_type(ztnef_mime_t *mime);
const char *ztnef_mime_get_show_name(ztnef_mime_t *mime);
const char *ztnef_mime_get_filename(ztnef_mime_t *mime);
const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime);
const char *ztnef_mime_get_content_id(ztnef_mime_t *mime);
int ztnef_mime_get_body_offset(ztnef_mime_t *mime);
int ztnef_mime_get_body_len(ztnef_mime_t *mime);

ztnef_t *ztnef_create_parser();
void ztnef_set_default_charset(ztnef_t *parser, const char *charset);
void ztnef_parse_from_data(ztnef_t *parser, const char *tnef_data, int tnef_data_len);
zbool_t ztnef_parse_from_filename(ztnef_t *parser, const char *filename);
void ztnef_free(ztnef_t *parser);
const char *ztnef_get_data(ztnef_t *parser);
int ztnef_get_len(ztnef_t *parser);
const zvector_t *ztnef_get_all_mimes(ztnef_t *parser);
void ztnef_debug_show(ztnef_t *parser);

/* zjson_t ############################################################### */

#define zvar_json_type_null        0
#define zvar_json_type_bool        1
#define zvar_json_type_string      2
#define zvar_json_type_long        3
#define zvar_json_type_double      4
#define zvar_json_type_object      5
#define zvar_json_type_array       6
#define zvar_json_type_unknown     7

#pragma pack(push, 1)
struct zjson_t {
    union {
        zbool_t b;
        long l;
        double d;
        zbuf_t *s;
        zvector_t *v; /* <zjson_t *> */
        zmap_t *m; /* <char *, zjson_t *> */
    } val;
    zjson_t *parent;
    unsigned char type;
};
#pragma pack(pop)

zjson_t *zjson_create();
#define zjson_create_null zjson_create
zjson_t *zjson_create_bool(zbool_t b);
zjson_t *zjson_create_long(long l);
zjson_t *zjson_create_double(double d);
zjson_t *zjson_create_string(const void *s, int len);
void zjson_free(zjson_t *j);
void zjson_reset(zjson_t *j);
/* */
zbool_t zjson_load_from_filename(zjson_t *j, const char *filename);
zbool_t zjson_unserialize(zjson_t *j, const char *s, int len);
void zjson_serialize(zjson_t *j, zbuf_t *result, int strict);
/* */
zinline int zjson_get_type(zjson_t *j) { return j->type; }
zinline zbool_t zjson_is_null(zjson_t *j)   { return j->type==zvar_json_type_null; }
zinline zbool_t zjson_is_bool(zjson_t *j)   { return j->type==zvar_json_type_bool; }
zinline zbool_t zjson_is_long(zjson_t *j)   { return j->type==zvar_json_type_long; }
zinline zbool_t zjson_is_double(zjson_t *j) { return j->type==zvar_json_type_double; }
zinline zbool_t zjson_is_string(zjson_t *j) { return j->type==zvar_json_type_string; }
zinline zbool_t zjson_is_object(zjson_t *j) { return j->type==zvar_json_type_object; }
zinline zbool_t zjson_is_array(zjson_t *j)  { return j->type==zvar_json_type_array; }
/* get value */
zbool_t *zjson_get_bool_value(zjson_t *j);
long *zjson_get_long_value(zjson_t *j);
double *zjson_get_double_value(zjson_t *j);
zbuf_t **zjson_get_string_value(zjson_t *j);
const zvector_t *zjson_get_array_value(zjson_t *j); /* <zjson_t *> */
const zmap_t *zjson_get_object_value(zjson_t *j); /* <char *, zjson_t *> */
zjson_t *zjson_array_get(zjson_t *j, int idx);
zjson_t *zjson_object_get(zjson_t *j, const char *key);
int zjson_array_get_len(zjson_t *j);
int zjson_object_get_len(zjson_t *j);

/* zjson_array_push 在数组后追加element(json). 返回element */
zjson_t *zjson_array_push(zjson_t *j, zjson_t *element);
#define zjson_array_add zjson_array_push

/* zjson_array_pop, 存在则返回真,否则假.
 * element不为0,则pop出来的json赋值给*element, 否则销毁 */
zbool_t zjson_array_pop(zjson_t *j, zjson_t **element);
zjson_t *zjson_array_unshift(zjson_t *j, zjson_t *element);
zbool_t zjson_array_shift(zjson_t *j, zjson_t **element);

/* 已知 json = [1, {}, "ss" "aaa"]
  1, zjson_array_update 给键idx设置成员element. 返回element
 * 2, 如果键idx不存在, 则直接赋值
 *    2.1, 例子: zjson_array_update(json, 6, element, 0)
 *         结果: [1, {}, "ss", "aaa", null, null, 6]
 * 3, 如果键idx存在
 *    3.1, 把旧值赋值给 *old_element, 如果old_element为0,则销毁.
 *         再做element的赋值
 *    3.2, 例子: zjson_array_update_element(json, 2, element, &old_element)
 *         结果: [1, {}, element, "aaa"], 且 *old_element 为 "ss"
 *    3.3, 例子: zjson_array_update_element(json, 2, element, 0);
 *         结果: [1, {}, element, "aaa"], 且 销毁 "ss" */
zjson_t *zjson_array_update(zjson_t *j, int idx, zjson_t *element, zjson_t **old_element);
zjson_t *zjson_array_insert(zjson_t *j, int idx, zjson_t *element);
void zjson_array_delete(zjson_t *j, int idx, zjson_t **old_element);

/* zjson_object_add_element 如上 */
zjson_t *zjson_object_update(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element);
#define zjson_object_add zjson_object_update
void zjson_object_delete(zjson_t *j, const char *key, zjson_t **old_element);

/* zjson_get_element_by_path 得到路径path对应的zjson_t值, 并返回
 * 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
 * zjson_get_element_by_path(json, "group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
zjson_t *zjson_get_element_by_path(zjson_t *j, const char *path);
/* zjson_get_element_by_path_vec 如上 
 * zjson_get_element_by_path_vec(json, "group", "linux", "2", "me", 0); */
zjson_t *zjson_get_element_by_path_vec(zjson_t *j, const char *path0, ...);
/* */
zinline zjson_t *zjson_get_parent(zjson_t *j) { return j->parent; }
zjson_t *zjson_get_top(zjson_t *j);

/* memcache client #################################################### */
zmemcache_client_t *zmemcache_client_connect(const char *destination, int cmd_timeout, zbool_t auto_reconnect);
void zmemcache_client_set_cmd_timeout(zmemcache_client_t *mc, int timeout);
void zmemcache_client_set_auto_reconnect(zmemcache_client_t *mc, zbool_t auto_reconnect);
void zmemcache_client_disconnect(zmemcache_client_t *mc);
int zmemcache_client_get(zmemcache_client_t *mc, const char *key, int *flag, zbuf_t *value);
int zmemcache_client_add(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_set(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_replace(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_append(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
int zmemcache_client_prepend(zmemcache_client_t *mc, const char *key, int flag, long timeout, const void *data, int len); 
long zmemcache_client_incr(zmemcache_client_t *mc, const char *key, unsigned long n);
long zmemcache_client_decr(zmemcache_client_t *mc, const char *key, unsigned long n);
int zmemcache_client_del(zmemcache_client_t *mc, const char *key);
int zmemcache_client_flush_all(zmemcache_client_t *mc, long after_second);
int zmemcache_client_version(zmemcache_client_t *mc, zbuf_t *version);

/* redis client ####################################################### */
zredis_client_t *zredis_client_connect(const char *destinations, const char *password, int cmd_timeout, zbool_t auto_reconnect);
zredis_client_t *zredis_client_connect_cluster(const char *destinations, const char *password, int cmd_timeout, zbool_t auto_reconnect);
void zredis_client_set_cmd_timeout(zredis_client_t *rc, int cmd_timeout);
void zredis_client_set_auto_reconnect(zredis_client_t *rc, zbool_t auto_reconnect);
const char * zredis_client_get_error_msg(zredis_client_t *rc);
void zredis_client_free(zredis_client_t *rc);
int zredis_client_get_success(zredis_client_t *rc, const char *redis_fmt, ...);
int zredis_client_get_long(zredis_client_t *rc, long *number_ret, const char *redis_fmt, ...);
int zredis_client_get_string(zredis_client_t *rc, zbuf_t *string_ret, const char *redis_fmt, ...);
int zredis_client_get_vector(zredis_client_t *rc, zvector_t *vector_ret, const char *redis_fmt, ...);
int zredis_client_get_json(zredis_client_t *rc, zjson_t *json_ret, const char *redis_fmt, ...);
int zredis_client_vget(zredis_client_t *rc, long *number_ret, zbuf_t *string_ret, zvector_t *vector_ret, zjson_t *json_ret, const char *redis_fmt, va_list ap);
int zredis_client_scan(zredis_client_t *rc,zvector_t *vector_ret,long *cursor_ret,const char *redis_fmt, ...);
int zredis_client_get_info_dict(zredis_client_t *rc, zdict_t *info);
int zredis_client_subscribe(zredis_client_t *rc, const char *redis_fmt, ...);
int zredis_client_psubscribe(zredis_client_t *rc, const char *redis_fmt, ...);
int zredis_client_fetch_channel_message(zredis_client_t *rc, zvector_t *vector_ret);

/* redis puny server ######################################### */
extern void (*zredis_puny_server_before_service)(void);
extern void (*zredis_puny_server_before_reload)(void);
extern void (*zredis_puny_server_before_exit)(void);
int zredis_puny_server_main(int argc, char **argv);
void zredis_puny_server_exec_cmd(zvector_t *cmd);

/* url ############################################################ */
struct zurl_t {
    char *scheme;
    char *destination;
    char *host;
    char *path;
    char *query;
    char *fragment;
    int port;
};
zurl_t *zurl_parse(const char *url_string);
void zurl_free(zurl_t *url);
void zurl_debug_show(zurl_t *url);

zdict_t *zurl_query_parse(const char *query, zdict_t *query_vars);
char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_result, zbool_t strict);

/* cookie ######################################################### */
zdict_t *zhttp_cookie_parse(const char *raw_cookie, zdict_t *cookies);
char *zhttp_cookie_build_item(const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly, zbuf_t *cookie_result);

/* httpd ########################################################## */
zbool_t zvar_httpd_debug;
zbool_t zvar_httpd_no_cache;

zhttpd_t *zhttpd_open_fd(int sock);
zhttpd_t *zhttpd_open_ssl(SSL *ssl);
void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl);
void zhttpd_run(zhttpd_t *httpd);
void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* set attribute */
void zhttps_set_exception(zhttpd_t *httpd);
void zhttps_set_stop(zhttpd_t *httpd);
void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout);
void zhttpd_set_request_header_timeout(zhttpd_t *httpd, int timeout);
void zhttpd_set_max_length_for_post(zhttpd_t *httpd, int max_length);
void zhttpd_set_tmp_path_for_post(zhttpd_t *httpd, const char *tmp_path);
void zhttpd_set_gzip_file_suffix(zhttpd_t *httpd, const char *suffix);
void zhttpd_enable_form_data(zhttpd_t *httpd);

const char *zhttpd_request_get_method(zhttpd_t *httpd);
const char *zhttpd_request_get_host(zhttpd_t *httpd);
const char *zhttpd_request_get_path(zhttpd_t *httpd);
const char *zhttpd_request_get_uri(zhttpd_t *httpd);
const char *zhttpd_request_get_version(zhttpd_t *httpd);
long zhttpd_request_get_content_length(zhttpd_t *httpd);
zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd);
zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd);

const zdict_t *zhttpd_request_get_headers(zhttpd_t *httpd);
const zdict_t *zhttpd_request_get_query_vars(zhttpd_t *httpd);
const zdict_t *zhttpd_request_get_post_vars(zhttpd_t *httpd);
const zdict_t *zhttpd_request_get_cookies(zhttpd_t *httpd);

const zvector_t *zhttpd_request_get_upload_files(zhttpd_t *httpd); /* zhttpd_upload_file * */

/* response completely */
void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size);
void zhttpd_response_304(zhttpd_t *httpd, const char *etag);
void zhttpd_response_404(zhttpd_t *httpd);
void zhttpd_response_500(zhttpd_t *httpd);

void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size));
void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size));
void zhttpd_set_304_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *etag));
void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* response file */
void zhttpd_response_file_set_max_age(zhttpd_t *httpd, int left_second);
void zhttpd_response_file_set_expires(zhttpd_t *httpd, int left_second);
void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing);
void zhttpd_response_file(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing);
void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing);

/* response header */
void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *initialization);
void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value);
void zhttpd_response_header_date(zhttpd_t *httpd, const char *name, long value);
void zhttpd_response_header_content_type(zhttpd_t *httpd, const char *value, const char *charset);
void zhttpd_response_header_content_length(zhttpd_t *httpd, long length);
void zhttpd_response_header_set_cookie(zhttpd_t *httpd, const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly);
void zhttpd_response_header_unset_cookie(zhttpd_t *httpd, const char *name);
void zhttpd_response_header_over(zhttpd_t *httpd);

/* response body */
void zhttpd_response_write(zhttpd_t *httpd, const void *data, int len);
void zhttpd_response_puts(zhttpd_t *httpd, const char *data);
void zhttpd_response_append(zhttpd_t *httpd, const zbuf_t *bf);
void zhttpd_response_printf_1024(zhttpd_t *httpd, const char *format, ...);
void zhttpd_response_flush(zhttpd_t *httpd);

/* stream */
zstream_t *zhttpd_get_stream(zhttpd_t *httpd);

/* zhttpd_upload_file_t */
const char *zhttpd_upload_file_get_filename(zhttpd_upload_file_t *fo);
const char *zhttpd_upload_file_get_name(zhttpd_upload_file_t *fo);
const char *zhttpd_upload_file_get_saved_pathname(zhttpd_upload_file_t *fo);
int zhttpd_upload_file_get_size(zhttpd_upload_file_t *fo);

/* sqlite3 ################################################## */
/* zsqlite3_proxd based on zevent_server */
extern void (*zsqlite3_proxy_server_before_service)(void);
extern void (*zsqlite3_proxy_server_before_reload)(void);
extern void (*zsqlite3_proxy_server_before_exit)(void);
int zsqlite3_proxy_server_main(int argc, char **argv);

/* client */
zbuf_t *zsqlite3_escape_append(zbuf_t *sql, const void *data, int len);
zsqlite3_proxy_client_t *zsqlite3_proxy_client_connect(const char *destination, zbool_t auto_reconnect);
void zsqlite3_proxy_client_close(zsqlite3_proxy_client_t *client);
int zsqlite3_proxy_client_log(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_exec(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_query(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_get_row(zsqlite3_proxy_client_t *client, zbuf_t ***rows);
int zsqlite3_proxy_client_get_column(zsqlite3_proxy_client_t *client);
const char *zsqlite3_proxy_client_get_error_msg(zsqlite3_proxy_client_t *client);

/* END ################################################################ */
#ifdef ZC_NAMESAPCE_NO_MALLOC
#undef zmalloc
#undef zcalloc
#undef zrealloc
#undef zfree
#undef zstrdup
#undef zstrndup
#undef zmemdup
#undef zmemdupnull
#endif

#undef zinline

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)
#endif /*___ZC_LIB_INCLUDE___ */
