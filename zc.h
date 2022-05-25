/* 
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ___ZC_LIB_INCLUDE___
#define ___ZC_LIB_INCLUDE___

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma pack(push, 4)

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef HEADER_OPENSSL_TYPES_H
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
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
typedef struct zdictlong_node_t zdictlong_node_t;
typedef struct zdictlong_t zdictlong_t;
typedef struct zdict_node_t zdict_node_t;
typedef struct zdict_t zdict_t;
typedef struct zmap_node_t zmap_node_t;
typedef struct zmap_t zmap_t;
typedef struct zlongmap_node_t zlongmap_node_t;
typedef struct zlongmap_t zlongmap_t;
typedef struct zmpool_t zmpool_t;
typedef struct zmqueue_t zmqueue_t;
typedef struct zstream_t zstream_t;
#define zconfig_t zdict_t
typedef struct zmmap_reader_t zmmap_reader_t;
typedef struct zaio_t zaio_t;
typedef struct zaio_base_t zaio_base_t;
typedef struct zmail_t zmail_t;
typedef struct zmime_t zmime_t;
typedef struct ztnef_t ztnef_t;
typedef struct ztnef_mime_t ztnef_mime_t;
typedef struct zjson_t zjson_t;
typedef struct zmemcache_client_t zmemcache_client_t;
typedef struct zredis_client_t zredis_client_t;
typedef struct zurl_t zurl_t;
typedef struct zhttpd_uploaded_file_t zhttpd_uploaded_file_t;
typedef struct zhttpd_t zhttpd_t;
typedef struct zsqlite3_proxy_client_t zsqlite3_proxy_client_t;
typedef struct zcdb_t zcdb_t;
typedef struct zcdb_walker_t zcdb_walker_t;
typedef struct zcdb_builder_t zcdb_builder_t;
typedef struct zmsearch_t zmsearch_t;
typedef struct zmsearch_walker_t zmsearch_walker_t;
typedef struct zpthread_pool_t zpthread_pool_t;

#ifndef zinline
#define zinline inline __attribute__((always_inline))
#endif

zinline int zempty(const void *ptr) { return ((!ptr)||(!(*(const char *)(ptr)))); }
#define ZEMPTY(str)                  (!(str)||!(*((const char *)str)))
#define ZCONTAINER_OF(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))
#define ZCONTAINER_OF2(ptr,app_type,offset) ((app_type *) (((char *) (ptr)) - offset))
#define ZCONVERT_CHAR_PTR(const_void_ptr)   (char *)(void*)(const_void_ptr)

#define zpthread_lock(l)    if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("FATAL mutex:%m");}
#define zpthread_unlock(l)  if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("FATAL mutex:%m");}

union ztype_convert_t {
    const void *CONST_VOID_PTR;
    void *VOID_PTR;
    const char *CONST_CHAR_PTR;
    char *CHAR_PTR;
    const unsigned char *CONST_UCHAR_PTR;
    unsigned char *UCHAR_PTR;
    long LONG;
    unsigned long ULONG;
    int INT;
    unsigned int UINT;
    short int SHORT_INT;
    unsigned short int USHORT_INT;
    char CHAR;
    unsigned char UCHAR;
    size_t SIZE_T;
    ssize_t SSIZE_T;
    mode_t MODE_T;
    off_t OFF_T;
    uid_t UID_T;
    gid_t GID_T;
    struct {
        int fd:30;
        int is_ssl:2;
        int fd_type:8;
    } fdinfo;
};

#define ZCHAR_PTR_TO_INT(_ptr, _int)    {ztype_convert_t _ct;_ct.ptr_char=(_ptr);_int=_ct.i_int;}
#define ZINT_TO_CHAR_PTR(_int, _ptr)    {ztype_convert_t _ct;_ct.i_int=(_int);_ptr=_ct.ptr_char;}

#define ZSTR_N_CASE_EQ(a, b, n)       ((ztoupper((int)a[0]) == ztoupper((int)b[0])) && (!strncasecmp(a,b,n)))
#define ZSTR_CASE_EQ(a, b)            ((ztoupper(a[0]) == ztoupper(b[0])) && (!strcasecmp(a,b)))
#define ZSTR_N_EQ(a, b, n)            ((a[0] == b[0]) && (!strncmp(a,b,n)))
#define ZSTR_EQ(a, b)                 ((a[0] == b[0]) && (!strcmp(a,b)))

/* zinline ######################################################### */
zinline unsigned int zint_unpack(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    n <<= 8; n |= p[3];
    return n;
}

zinline void zint_pack(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[3] = num & 255;
    num >>= 8; p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

zinline unsigned int zint_unpack3(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    return n;
}

zinline void zint_pack3(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

zinline unsigned int zint_unpack2(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8; n |= p[1];
    return n;
}

zinline void zint_pack2(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

/* 最经典的hash函数, 需要更高级的可以考虑 crc16, crc32, crc64, 甚至md5 等*/
zinline unsigned zhash_djb(const void *buf, int len)
{
    register const unsigned char *p = (const unsigned char *)buf;
    register const unsigned char *end = p + len;
    register unsigned hash = 5381;	/* start value */
    while (p < end) {
        hash = (hash + (hash << 5)) ^ *p++;
    }
    return hash;
}

zinline unsigned zhash_djb_with_initial(const void *buf, int len, unsigned int initial)
{
    register const unsigned char *p = (const unsigned char *)buf;
    register const unsigned char *end = p + len;
    register unsigned hash = initial;  /* start value */
    while (p < end) {
        hash = (hash + (hash << 5)) ^ *p++;
    }
    return hash;
}
/* 日志, src/stdlib/log.c ########################################## */

/* 见 zlog_fatal */
extern zbool_t zvar_log_fatal_catch;

/* 见 zdebug */
extern zbool_t zvar_log_debug_enable;

/* 可自定义 zlog_vprintf, 定制日志输出 */
extern void (*zlog_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap);

/* 日志输出 */
void __attribute__((format(printf,3,4))) zlog_info(const char *source_fn, size_t line_number, const char *fmt, ...);

/* 日志输出后, 进程退出, 如果 zvar_log_fatal_catch==1, 则 激活段错误 */
void __attribute__((format(printf,3,4))) zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...);

#define zfatal(fmt, args...) { zlog_fatal(__FILE__, __LINE__, fmt, ##args); }
#define zinfo(fmt, args...) { zlog_info(__FILE__, __LINE__, fmt, ##args); }
#define zdebug(fmt,args...) { if(zvar_log_debug_enable){zinfo(fmt, ##args);} }

void zlog_use_default();

/* 使用syslog, identity/facility 参考 syslog  */
void zlog_use_syslog(const char *identity, int facility);

/* 转换字符串facility, 如: "LOG_MAIL" => LOG_MAIL */
int zlog_get_facility_from_str(const char *facility);

/* 使用 masterlog, master-server 提供的服务 */
/* identity: 程序名; dest: master-server提供的服务地址, domain_socket, udp */
void zlog_use_masterlog(const char *identity, const char *dest);

/* 内存分配, src/malloc/malloc.c ###################################### */
extern char *zblank_buffer;
#define ZFREE(a)                     (zfree(a),a=0)

/* LIB-ZC, 内部开发都使用如下内存操作函数, 理论上 zmalloc等价于malloc, 其他类似 */
#define zmalloc                      zmalloc_20160308
#define zcalloc                      zcalloc_20160308
#define zrealloc                     zrealloc_20160308
#define zfree                        zfree_20160308
#define zstrdup                      zstrdup_20160308
#define zstrndup                     zstrndup_20160308
#define zmemdup                      zmemdup_20160308
#define zmemdupnull                  zmemdupnull_20160308
void *zmalloc(int len);
void *zcalloc(int nmemb, int size);
void *zrealloc(const void *ptr, int len);
void zfree(const void *ptr);
char *zstrdup(const char *ptr);
char *zstrndup(const char *ptr, int n);

/* 复制ptr指向的内存并返回, 长度是n */
char *zmemdup(const void *ptr, int n);

/* 复制ptr指向的内存并返回, 长度是n+1, 复制后追加 '\0' */
char *zmemdupnull(const void *ptr, int n);

/* buf, src/stdlib/buf.c ################################## */
struct zbuf_t {
    char *data;
    int len:31;
    unsigned int static_mode:1;
    int size:31;
    unsigned int unused_flag1:1;
}; 

/* 创建buf, 初始容量为size */
zbuf_t *zbuf_create(int size);

/* 释放 */
void zbuf_free(zbuf_t *bf);

/* 返回数据指针 */
zinline char *zbuf_data(const zbuf_t *bf) { return bf->data; }

/* 返回长度 */
zinline int zbuf_len(const zbuf_t *bf) { return bf->len; }

/* 初始化bf指向的内容为 buf, 初始容量为size */
void zbuf_init(zbuf_t *bf, int size);

/* zbuf_init 的反操作 */
void zbuf_fini(zbuf_t *bf);

/* 调整容量,使剩余容量大于need, 返回实际剩余容量 */
int zbuf_need_space(zbuf_t *bf, int need);

/* 追加字节ch到bf. 返回ch */
/* 不推荐使用zbuf_put_do, 建议使用 zbuf_put */
int zbuf_put_do(zbuf_t *bf, int ch);

/* 追加字节c到b, 建议使用 zbuf_put(b, c) */
#define ZBUF_PUT(b, c)  \
    (((b)->len<(b)->size)?((int)(((unsigned char *)((b)->data))[(b)->len++]=(unsigned char)(c))):(((b)->static_mode?0:zbuf_put_do((b), (c)))))

/* 追加写字节ch到bf. 返回ch */
zinline void zbuf_put(zbuf_t *bf, int ch) { ZBUF_PUT(bf, ch); bf->data[bf->len] = 0; }

/* 重置 */
zinline void zbuf_reset(zbuf_t *bf) { bf->len=0, bf->data[0]=0; }

/* 结尾置 0 */
zinline void zbuf_terminate(zbuf_t *bf) { bf->data[bf->len]=0; }

/* 截短 */
zinline void zbuf_truncate(zbuf_t *bf, int new_len) {
    if ((bf->len>new_len) && (new_len>-1)) { bf->len=new_len; bf->data[bf->len] = 0; }
}

/* 这几个函数, 顾名思义即可 */
void zbuf_strncpy(zbuf_t *bf, const char *src, int len);
void zbuf_strcpy(zbuf_t *bf, const char *src);
void zbuf_strncat(zbuf_t *bf, const char *src, int len);
void zbuf_strcat(zbuf_t *bf, const char *src);
#define zbuf_puts zbuf_strcat
void zbuf_memcpy(zbuf_t *bf, const void *src, int len);
void zbuf_memcat(zbuf_t *bf, const void *src, int len);
zinline void zbuf_append(zbuf_t *bf, zbuf_t *bf2) { return zbuf_memcat(bf, zbuf_data(bf2), zbuf_len(bf2)); }

/* zbuf_printf_1024 意思 { char buf[1024+1], snprintf(buf, 1024, format, ...); zbuf_cat(bf, buf); } */
void zbuf_printf_1024(zbuf_t *bf, const char *format, ...);
void zbuf_vprintf_1024(zbuf_t *bf, const char *format, va_list ap);

/* 删除右侧的\r\n */
void zbuf_trim_right_rn(zbuf_t *bf);

/* STACK_BUF */
#define ZSTACK_BUF(name, _size)    \
    zbuf_t name ## _ZSTACT_BUF_ , *name; \
    name = &name ## _ZSTACT_BUF_; \
    char name ## _databuf_STACK [_size+1]; \
    name->size = _size; name->len = 0; \
    name->data = name ## _databuf_STACK; name->data[0] = 0; \
    name->static_mode = 1;

#define ZSTACK_BUF_FROM(name, _data, _size)    \
    zbuf_t name ## _ZSTACT_BUF_, *name; \
    name = &name ## _ZSTACT_BUF_; \
    name->size = _size; name->len = 0; \
    name->data = (char *)(_data); name->data[0] = 0; \
    name->static_mode = 1;

#define ZREADONLY_BUF(name, _data, _size)    \
    zbuf_t name ## _ZSTACT_BUF_, *name; \
    name = &name ## _ZSTACT_BUF_; \
    name->size = _size; name->len = _size; \
    name->data = (char *)(_data); \
    name->static_mode = 1;

/* size_data ####################################################### */
/* size data 可以用于描述内存buffer */
struct zsize_data_t {
    int size;
    char *data;
};

/* 下面这些函数,在存储或传输格式化数据时可以减少空间占用. src/stdlib/size_data.c */
int zcint_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len);
int zcint_data_unescape_all(const void *src_data, int src_size, zsize_data_t *vec, int vec_size);
void zcint_data_escape(zbuf_t * zb, const void *data, int len);
void zcint_data_escape_int(zbuf_t * zb, int i);
void zcint_data_escape_long(zbuf_t * zb, long i);
void zcint_data_escape_dict(zbuf_t * zb, zdict_t * zd);
void zcint_data_escape_pp(zbuf_t * zb, const char **pp, int size);
int zcint_put(int size, char *buf);

/* char, src/stdlib/string.c ######################################## */
extern unsigned const char zchar_lowercase_vector[];
extern unsigned const char zchar_uppercase_vector[];
extern unsigned const char zchar_isalnum_vector[];
extern unsigned const char zchar_isalpha_vector[];
extern unsigned const char zchar_islower_vector[];
extern unsigned const char zchar_isupper_vector[];
extern unsigned const char zchar_isdigit_vector[];
extern unsigned const char zchar_isxdigit_vector[];
extern unsigned const char zchar_xdigitval_vector[];
extern unsigned const char zchar_istrim_vector[];

/* 在locale 为 "C" 的情况下, ztolower 和 tolower 等价, 其他类似 */
#define ztolower(c)    ((int)zchar_lowercase_vector[(unsigned char)(c)])
#define ztoupper(c)    ((int)zchar_uppercase_vector[(unsigned char)(c)])
#define zisalnum(c)    (zchar_isalnum_vector[(unsigned char)(c)])
#define zisalpha(c)    (zchar_isalpha_vector[(unsigned char)(c)])
#define zislower(c)    (zchar_islower_vector[(unsigned char)(c)])
#define zisupper(c)    (zchar_isupper_vector[(unsigned char)(c)])
#define zisdigit(c)    (zchar_isdigit_vector[(unsigned char)(c)])
#define zisxdigit(c)   (zchar_isxdigit_vector[(unsigned char)(c)])
#define zhexval(c)     (zchar_xdigitval_vector[(unsigned char)(c)])
#define zistrim(c)     (zchar_istrim_vector[(unsigned char)(c)])


/* 把字符串str转换为小写并返回, 直接在str指向内存替换 */
char *zstr_tolower(char *str);
/* 大写, 同上 */
char *zstr_toupper(char *str);

/* trim, 设 str = "\r \t\n \f ABC \r\n"; src/stdlib/string.c ############# */

/* ztrim_left(str) 返回 str+8 */
char *ztrim_left(char *str);

/* ztrim_right(str) 返回 str, 且 str[11]=0; */
char *ztrim_right(char *str);

/* ztrim(str) 返回 str+8, 且 str[11]=0; */
char *ztrim(char *str);

/* skip, 设 str = "\r \t\n \f ABC \r\n", ignores="\r\n \t\f", src/stdlib/string.c ### */

/* zskip_left(str, ignores) 返回 str+8 */
char *zskip_left(const char *str, const char *ignores);

/* zskip_right(str, -1, ignores) 返回 10 */
int zskip_right(const char *str, int len, const char *ignores);

/* zskip(str, -1, ignores, ignores, str) 返回 3, 且 *start = str+8 */
int zskip(const char *str, int len, const char *ignores_left, const char *ignores_right, char **start);

/* strtok */
struct zstrtok_t {
    char *sstr;
    char *str;
    int len;
};
void zstrtok_init(zstrtok_t *k, const char *sstr);
zstrtok_t *zstrtok(zstrtok_t *k, const char *delim);

/* convert to unit */

/* s 是 "0", "n", "N", "no", "NO", "false", "FALSE" 返回 0 */
/* s 是 "1", "y", "Y", "yes", "YES", "true", "TRUE" 返回 1 */
/* 否则 返回 def */
zbool_t zstr_to_bool(const char *s, int def);

/* 转换字符串为秒, 支持 h(小时), m(分), s(秒), d(天), w(周) */
/* 如 "1026S" = > 1026, "8h" => 8 * 3600, "" = > def  */
long zstr_to_second(const char *s, long def);

/* 转换字符串为大小, 支持 g(G), m(兆), k千), b */
/* 如 "9M" = > 9 * 1024 * 1024  */
long zstr_to_size(const char *s, long def);

/* argv, src/stdlib/argv.c ########################################## */
/* argv 是 一组字符串, 例子见 sample/stdlib/argv.c */
struct zargv_t {
    char **argv;
    int argc:31;
    int unused:1;
    int size:31;
    int mpool_used:1;
};

/* 创建argv, 初始容量为size */
zargv_t *zargv_create(int size);

/* 释放 */
void zargv_free(zargv_t *argvp);

/* 个数 */
zinline int zargv_len(const zargv_t *argvp) { return argvp->argc; }
zinline int zargv_argc(const zargv_t *argvp) { return argvp->argc; }

/* 数据指针 */
zinline char **zargv_argv(const zargv_t *argvp) { return argvp->argv; }
zinline char **zargv_data(const zargv_t *argvp) { return argvp->argv; }

/* 把strdup(ns)追加到尾部*/
void zargv_add(zargv_t *argvp, const char *ns);

/* 把strndup(ns, nlen)追加到为尾部 */
void zargv_addn(zargv_t *argvp, const char *ns, int nlen);

/* 把argvp的长度截短为len */
void zargv_truncate(zargv_t *argvp, int len);

/* 重置, 既把argvp的长度截短为 0 */
zinline void zargv_reset(zargv_t *argvp) { zargv_truncate(argvp,0); }

/* 用delim分割string, 并追加到argvp */
zargv_t *zargv_split_append(zargv_t *argvp, const char *string, const char *delim);

/* debug 输出 */
void zargv_debug_show(zargv_t *argvp);

#define ZARGV_WALK_BEGIN(ar, var_your_ptr)   {\
    int zargv_tmpvar_i; const zargv_t *___ar_tmp_ptr = (ar); char *var_your_ptr; \
        for(zargv_tmpvar_i=0;zargv_tmpvar_i<(___ar_tmp_ptr)->argc;zargv_tmpvar_i++){ \
            var_your_ptr = (___ar_tmp_ptr)->argv[zargv_tmpvar_i];
#define ZARGV_WALK_END                       }}

/* mlink ############################################################ */
/* 一组宏, 可实现, 栈, 链表等, 例子见 src/stdlib/list.c */

/* head: 头; tail: 尾; node:节点变量; prev:head/tail 指向的struct成员的属性"前一个" */

/* 追加node到尾部 */
#define ZMLINK_APPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

/* 追加node到首部 */
#define ZMLINK_PREPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

/* 插入node到before前 */
#define ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node, _before_1106 = before;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else if(_before_1106==0){_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    else if(_before_1106==_head_1106){_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    else {_node_1106->prev=_before_1106->prev; _node_1106->next=_before_1106; _before_1106->prev->next=_node_1106; _before_1106->prev=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

/* 去掉节点node */
#define ZMLINK_DETACH(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_node_1106->prev){ _node_1106->prev->next=_node_1106->next; }else{ _head_1106=_node_1106->next; }\
    if(_node_1106->next){ _node_1106->next->prev=_node_1106->prev; }else{ _tail_1106=_node_1106->prev; }\
    head = _head_1106; tail = _tail_1106; \
}

#define ZMLINK_CONCAT(head_1, tail_1, head_2, tail_2, prev, next) {\
    typeof(head_1) _head_1106=head_1,_tail_1106=tail_1,_head_2206=head_2,_tail_2206=tail_2; if(_head_2206){ \
        if(_head_1106){_tail_1106->next=_head_2206;_head_2206->prev=_tail_1106; }else{_head_1106=_head_2206;} \
        _tail_1106=_tail_2206; \
    } head_1 = _head_1106; tail_1 = _tail_1106; \
}
/* link, src/stdlib/link.c ########################################## */
/* 数据结构, 可实现链表等 */

struct zlink_t {
    zlink_node_t *head;
    zlink_node_t *tail;
};
struct zlink_node_t {
    zlink_node_t *prev;
    zlink_node_t *next;
};

/* 初始化link指向的指针 */
zinline void zlink_init(zlink_t *link) { link->head = 0; link->tail = 0; }

/* 反初始化link指向的指针 */
zinline void zlink_fini(zlink_t *link) { }

/* 把node插到before前 */
zlink_node_t *zlink_attach_before(zlink_t *link, zlink_node_t *node, zlink_node_t *before);

/* 把节点node从link中移除 */
zlink_node_t *zlink_detach(zlink_t *link, zlink_node_t *node);

/* 把节点追加到尾部 */
zlink_node_t *zlink_push(zlink_t *link, zlink_node_t *node);

/* 把尾部节点弹出并返回 */
zlink_node_t *zlink_pop(zlink_t *link);

/* 把节点追加到首部 */
zlink_node_t *zlink_unshift(zlink_t *link, zlink_node_t *node);

/* 把首部节点弹出并返回 */
zlink_node_t *zlink_shift(zlink_t *link);

/* 返回首部节点 */
zinline zlink_node_t *zlink_head(const zlink_t *link) { return link->head; }

/* 返回尾部节点 */
zinline zlink_node_t *zlink_tail(const zlink_t *link) { return link->tail; }

/* 前一个节点 */
zinline zlink_node_t *zlink_node_prev(const zlink_node_t *node) { return node->prev; }

/* 后一个节点 */
zinline zlink_node_t *zlink_node_next(const zlink_node_t *node) { return node->next; }

/* vector, src/stdlib/vector.c ###################################### */
/* 一列指针 */
/* 推荐使用 zvector_push, zvector_pop */
/* 不推荐使用 zvector_unshift, zvector_shift, zvector_insert, zvector_delete */
struct zvector_t {
    char **data;
    int len;
    int size;
    int offset:31;
    unsigned int mpool_used:1;
};

/* 创建vector, 初始容量为size */
zvector_t *zvector_create(int size);

/* 释放, 释放自身, 忽略成员的数据 */
void zvector_free(zvector_t *v);

/* 数据指针 */
zinline char **zvector_data(const zvector_t *v) { return v->data; }

/* 个数 */
zinline int zvector_len(const zvector_t *v) { return v->len; }

#define zvector_add zvector_push
/* 追加一个指针val到v的尾部 */
void zvector_push(zvector_t *v, const void *val);

/* 弹出尾部指针并赋值给*val, 存在则返回1, 否则返回 0 */
zbool_t zvector_pop(zvector_t *v, void **val);

/* 追加一个指针val到v的首部 */
void zvector_unshift(zvector_t *v, const void *val);

/* 弹出首部指针并赋值给*val, 存在则返回1, 否则返回 0 */
zbool_t zvector_shift(zvector_t *v, void **val);

/* 把val插到idx处, 远idx及其后元素顺序后移 */
void zvector_insert(zvector_t *v, int idx, void *val);

/* 弹出idx处的指针并赋值给*val, 存在则返回1, 否则返回 0 */
zbool_t zvector_delete(zvector_t *v, int idx, void **val);

/* 重置 */
void zvector_reset(zvector_t *v);

/* 截短到 new_len */
void zvector_truncate(zvector_t *v, int new_len);

/* 宏,遍历 */
#define ZVECTOR_WALK_BEGIN(arr, you_chp_type, var_your_ptr)    {\
     int zvector_tmpvar_i; const zvector_t * _arr_tmp_ptr = (arr); you_chp_type var_your_ptr;\
    for(zvector_tmpvar_i=0;zvector_tmpvar_i<_arr_tmp_ptr->len;zvector_tmpvar_i++){\
        var_your_ptr = (you_chp_type)(_arr_tmp_ptr->data[zvector_tmpvar_i]);
#define ZVECTOR_WALK_END                }}

void zbuf_vector_reset(zvector_t *v);
void zbuf_vector_free(zvector_t *v);

/* list ############################################################# */
/* 双向链表 */
struct zlist_t {
    zlist_node_t *head;
    zlist_node_t *tail;
    int len;
};
struct zlist_node_t {
    zlist_node_t *prev;
    zlist_node_t *next;
    void *value;
};

/* 创建链表 */
zlist_t *zlist_create(void);

/* 释放 */
void zlist_free(zlist_t *list);

/* 重置, 忽略成员数据 */
void zlist_reset(zlist_t *list);

/* 个数 */ 
zinline int zlist_len(const zlist_t *list) { return list->len; }

/* 头部(第一个)节点 */
zinline zlist_node_t *zlist_head(const zlist_t *list) { return list->head; }

/* 尾部(最有一个)节点 */
zinline zlist_node_t *zlist_tail(const zlist_t *list) { return list->tail; }

/* 下一个节点 */
zinline zlist_node_t *zlist_node_next(const zlist_node_t *node) { return node->next; }

/* 上一个节点 */
zinline zlist_node_t *zlist_node_prev(const zlist_node_t *node) { return node->prev; }

/* 节点的值 */
zinline void *zlist_node_value(const zlist_node_t *node) { return node->value; }

/* 把节点node插到before前 */
void zlist_attach_before(zlist_t *list, zlist_node_t *n, zlist_node_t *before);

/* 移除节点 n, 并没有释放n的资源 */
void zlist_detach(zlist_t *list, zlist_node_t *n);

/* 创建一个值为value的节点,插到before前, 并返回 */
zlist_node_t *zlist_add_before(zlist_t *list, const void *value, zlist_node_t *before);

/* 删除节点n, 把n的值赋值给*value, 释放n的资源, 如果n==0返回0, 否则返回1 */
zbool_t zlist_delete(zlist_t *list, zlist_node_t *n, void **value);

/* 创建值为v的节点, 追加到链表尾部, 并返回 */
zinline zlist_node_t *zlist_push(zlist_t *l,const void *v){return zlist_add_before(l,v,0);}

/* 创建值为v的节点, 追加到链表首部部, 并返回 */
zinline zlist_node_t *zlist_unshift(zlist_t *l,const void *v){return zlist_add_before(l,v,l->head);}

/* 弹出尾部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
zinline zbool_t zlist_pop(zlist_t *l, void **v){return zlist_delete(l,l->tail,v);}

/* 弹出首部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
zinline zbool_t zlist_shift(zlist_t *l, void **v){return zlist_delete(l,l->head,v);}

/* 宏, 遍历 */
#define ZLIST_WALK_BEGIN(list, var_your_type, var_your_ptr)  { \
    zlist_node_t *list_current_node=(list)->head; var_your_type var_your_ptr; \
    for(;list_current_node;list_current_node=list_current_node->next){ \
        var_your_ptr = (var_your_type)(void *)(list_current_node->value);
#define ZLIST_WALK_END                          }}

/* 宏, 遍历 */
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

/* dict, src/stdlib/dict.c ########################################## */
/* 词典, 例子见 sample/rbtree/dict_account.c */
struct zdict_t {
    zrbtree_t rbtree;
    int len; /* 节点个数 */
};
struct zdict_node_t {
    zrbtree_node_t rbnode;
    zbuf_t value; /* 值 */
    char *key; /* 键 */
} __attribute__ ((aligned(8)));

/* 创建词典 */
zdict_t *zdict_create(void);

/* 释放 */
void zdict_free(zdict_t *dict);

/* 重置 */
void zdict_reset(zdict_t *dict);

/* 个数 */
zinline int zdict_len(const zdict_t *dict) { return dict->len; }

/* 节点的键 */
zinline char *zdict_node_key(const zdict_node_t *node) { return node->key; }

/* 节点的值 */
zinline zbuf_t *zdict_node_value(const zdict_node_t *node) { return (zbuf_t *)(&(node->value)); }

/* 增加或更新节点,并返回此节点.  此节点键为key, 值为 dup_foo(value) */
zdict_node_t *zdict_update(zdict_t *dict, const char *key, const zbuf_t *value);

/* 增加或更新节点,并返回此节点.  此节点键为key, 值为(len<0?strdup(value):strndup(value, len)) */
zdict_node_t *zdict_update_string(zdict_t *dict, const char *key, const char *value, int len);

/* 查找键为key的节点,并返回. 如果存在则节点的值赋值给 *value */
zdict_node_t *zdict_find(const zdict_t *dict, const char *key, zbuf_t **value);

/* 查找键值小于key且最接近key的节点, 并... */
zdict_node_t *zdict_find_near_prev(const zdict_t *dict, const char *key, zbuf_t **value);

/* 查找键值大于key且最接近key的节点, 并... */
zdict_node_t *zdict_find_near_next(const zdict_t *dict, const char *key, zbuf_t **value);

/* 删除并释放键为key的节点 */
void zdict_delete(zdict_t *dict, const char *key);

/* 删除节点n */
void zdict_delete_node(zdict_t *dict, zdict_node_t *n);

/* 第一个节点 */
zdict_node_t *zdict_first(const zdict_t *dict);

/* 最后一个节点 */
zdict_node_t *zdict_last(const zdict_t *dict);

/* 前一个节点 */
zdict_node_t *zdict_prev(const zdict_node_t *node);

/* 后一个节点 */
zdict_node_t *zdict_next(const zdict_node_t *node);

/* 查找键为name的节点, 如果存在则返回其值, 否则返回def */
char *zdict_get_str(const zdict_t *dict, const char *name, const char *def);

/* 查找键为name的节点, 如果存在则返回zstr_to_bool(其值), 否则返回def */
int zdict_get_bool(const zdict_t *dict, const char *name, int def);

/* 查找键为name的节点, 如果存在则返回foo(其值), 否则返回def; foo 为 atoi */
int zdict_get_int(const zdict_t *dict, const char *name, int def);

/* 如上, foo 为 atol */
long zdict_get_long(const zdict_t *dict, const char *name, long def);

/* 如上, foo 为 zstr_to_second */
long zdict_get_second(const zdict_t *dict, const char *name, long def);

/* 如上, foo 为 zstr_to_size */
long zdict_get_size(const zdict_t *dict, const char *name, long def);

/* debug输出 */
void zdict_debug_show(const zdict_t *dict);

/* 宏, 遍历1 */
#define ZDICT_WALK_BEGIN(dict, var_your_key, var_your_value)  { \
    zdict_node_t *var_your_node; char *var_your_key; zbuf_t *var_your_value; \
    for(var_your_node = zdict_first(dict); var_your_node; var_your_node = zdict_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=&(var_your_node->value); {
#define ZDICT_WALK_END    }}}

/* 宏, 遍历2 */
#define ZDICT_NODE_WALK_BEGIN(dict, var_your_node)  { \
    zdict_node_t *var_your_node; \
    for(var_your_node = zdict_first(dict); var_your_node; var_your_node = zdict_next(var_your_node)) {
#define ZDICT_NODE_WALK_END    }}

/* dictlong, src/stdlib/dictlong.c ##################################### */
/* 参考 dict, 和dict的区别是值的类型是long */
struct zdictlong_t {
    zrbtree_t rbtree;
    int len;
};
struct zdictlong_node_t {
    zrbtree_node_t rbnode;
    long value;
    char *key;
} __attribute__ ((aligned(8)));
zdictlong_t *zdictlong_create(void);
void zdictlong_free(zdictlong_t *dictlong);
void zdictlong_reset(zdictlong_t *dictlong);
zinline int zdictlong_len(const zdictlong_t *dictlong) { return dictlong->len; }
zinline char *zdictlong_node_key(const zdictlong_node_t *node) { return node->key; }
zinline long zdictlong_node_value(const zdictlong_node_t *node) { return node->value; }
zinline void zdictlong_node_set_value(zdictlong_node_t *node, long value) { node->value = value; }
zdictlong_node_t *zdictlong_update(zdictlong_t *dictlong, const char *key, long value);
zdictlong_node_t *zdictlong_find(const zdictlong_t *dictlong, const char *key, long *value);
zdictlong_node_t *zdictlong_find_near_prev(const zdictlong_t *dictlong, const char *key, long *value);
zdictlong_node_t *zdictlong_find_near_next(const zdictlong_t *dictlong, const char *key, long *value);
void zdictlong_delete_node(zdictlong_t *dictlong, zdictlong_node_t *n);
zdictlong_node_t *zdictlong_first(const zdictlong_t *dictlong);
zdictlong_node_t *zdictlong_last(const zdictlong_t *dictlong);
zdictlong_node_t *zdictlong_prev(const zdictlong_node_t *node);
zdictlong_node_t *zdictlong_next(const zdictlong_node_t *node);
void zdictlong_debug_show(const zdictlong_t *dictlong);

#define ZDICTLONG_WALK_BEGIN(dictlong, var_your_key, var_your_value)  { \
    zdictlong_node_t *var_your_node; char *var_your_key; long var_your_value; \
    for(var_your_node = zdictlong_first(dictlong); var_your_node; var_your_node = zdictlong_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=var_your_node->value; {
#define ZDICTLONG_WALK_END    }}}

#define ZDICTLONG_NODE_WALK_BEGIN(dictlong, var_your_node)  { \
    zdictlong_node_t *var_your_node; \
    for(var_your_node = zdictlong_first(dictlong); var_your_node; var_your_node = zdictlong_next(var_your_node)) {
#define ZDICTLONG_NODE_WALK_END    }}

/* map, src/stdlib/map.c ############################################ */
/* 映射, 例子见 sample/rbtree/map_account.c */
struct zmap_t {
    zrbtree_t rbtree;
    int len;
};
struct zmap_node_t {
    char *key; /* 键 */
    void *value; /* 值 */
    zrbtree_node_t rbnode;
} __attribute__ ((aligned(8)));

/* 创建 */
zmap_t *zmap_create(void);
void zmap_free(zmap_t *map);

/* 重置 */
void zmap_reset(zmap_t *map);

/* 新增或更新节点并返回, 这个节点的键为key, 新值为value, 如果旧值存在则赋值给 *old_value */
zmap_node_t *zmap_update(zmap_t *map, const char *key, const void *value, void **old_value);

/* 查找键为key的节点,并返回. 如果存在则节点的值赋值给 *value */
zmap_node_t *zmap_find(const zmap_t *map, const char *key, void **value);

/* 查找键值小于key且最接近key的节点, 并... */
zmap_node_t *zmap_find_near_prev(const zmap_t *map, const char *key, void **value);

/* 查找键值大于key且最接近key的节点, 并... */
zmap_node_t *zmap_find_near_next(const zmap_t *map, const char *key, void **value);

/* 删除并释放键为key的节点, 节点的值赋值给 *old_value */
zbool_t zmap_delete(zmap_t * map, const char *key, void **old_value);

/* 删除并释放节点n, 节点的值赋值给 *old_value */
void zmap_delete_node(zmap_t *map, zmap_node_t *n, void **old_value);

/* 更新节点的值, 节点的旧值赋值给 *old_value */
void zmap_node_update(zmap_node_t *n, const void *value, void **old_value);

/* 第一个 */
zmap_node_t *zmap_first(const zmap_t *map);

/* 最后一个 */
zmap_node_t *zmap_last(const zmap_t *map);

/* 前一个 */
zmap_node_t *zmap_prev(const zmap_node_t *node);

/* 后一个 */
zmap_node_t *zmap_next(const zmap_node_t *node);

/* 节点个数 */
zinline int zmap_len(const zmap_t *map) { return map->len; }

/* 节点的键 */
zinline char *zmap_node_key(const zmap_node_t *node) { return node->key; }

/* 节点的值 */
zinline void *zmap_node_value(const zmap_node_t *node) { return node->value; }

/* 宏, 遍历1 */
#define ZMAP_NODE_WALK_BEGIN(map, var_your_node)  { \
    zmap_node_t *var_your_node; \
    for(var_your_node = zmap_first(map); var_your_node; var_your_node = zmap_next(var_your_node)) {
#define ZMAP_NODE_WALK_END    }}

/* 宏, 遍历2 */
#define ZMAP_WALK_BEGIN(map, var_your_key, var_your_value_type, var_your_value)  { \
    zmap_node_t *var_your_node; char *var_your_key; var_your_value_type var_your_value; \
    (void)var_your_key; (void)var_your_value; \
    for(var_your_node = zmap_first(map); var_your_node; var_your_node = zmap_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=(var_your_value_type)(void *)var_your_node->value; {
#define ZMAP_WALK_END    }}}

/* longmap, src/stdlib/longmap.c ############################################ */
/* 映射, 例子见 sample/rbtree/longmap_account.c */
struct zlongmap_t {
    zrbtree_t rbtree;
    int len;
};
struct zlongmap_node_t {
    long key; /* 键 */
    void *value; /* 值 */
    zrbtree_node_t rbnode;
} __attribute__ ((aligned(8)));

/* 创建 */
zlongmap_t *zlongmap_create(void);
void zlongmap_free(zlongmap_t *longmap);

/* 重置 */
void zlongmap_reset(zlongmap_t *longmap);

/* 新增或更新节点并返回, 这个节点的键为key, 新值为value, 如果旧值存在则赋值给 *old_value */
zlongmap_node_t *zlongmap_update(zlongmap_t *longmap, long key, const void *value, void **old_value);

/* 查找键为key的节点,并返回. 如果存在则节点的值赋值给 *value */
zlongmap_node_t *zlongmap_find(const zlongmap_t *longmap, long key, void **value);

/* 查找键值小于key且最接近key的节点, 并... */
zlongmap_node_t *zlongmap_find_near_prev(const zlongmap_t *longmap, long key, void **value);

/* 查找键值大于key且最接近key的节点, 并... */
zlongmap_node_t *zlongmap_find_near_next(const zlongmap_t *longmap, long key, void **value);

/* 删除并释放键为key的节点, 节点的值赋值给 *old_value */
zbool_t zlongmap_delete(zlongmap_t * longmap, long key, void **old_value);

/* 删除并释放节点n, 节点的值赋值给 *old_value */
void zlongmap_delete_node(zlongmap_t *longmap, zlongmap_node_t *n, void **old_value);

/* 更新节点的值, 节点的旧值赋值给 *old_value */
void zlongmap_node_update(zlongmap_node_t *n, const void *value, void **old_value);

/* 第一个 */
zlongmap_node_t *zlongmap_first(const zlongmap_t *longmap);

/* 最后一个 */
zlongmap_node_t *zlongmap_last(const zlongmap_t *longmap);

/* 前一个 */
zlongmap_node_t *zlongmap_prev(const zlongmap_node_t *node);

/* 后一个 */
zlongmap_node_t *zlongmap_next(const zlongmap_node_t *node);

/* 节点个数 */
zinline int zlongmap_len(const zlongmap_t *longmap) { return longmap->len; }

/* 节点的键 */
zinline long zlongmap_node_key(const zlongmap_node_t *node) { return node->key; }

/* 节点的值 */
zinline void *zlongmap_node_value(const zlongmap_node_t *node) { return node->value; }

/* 宏, 遍历1 */
#define ZLONGMAP_NODE_WALK_BEGIN(longmap, var_your_node)  { \
    zlongmap_node_t *var_your_node; \
    for(var_your_node = zlongmap_first(longmap); var_your_node; var_your_node = zlongmap_next(var_your_node)) {
#define ZLONGMAP_NODE_WALK_END    }}

/* 宏, 遍历2 */
#define ZLONGMAP_WALK_BEGIN(longmap, var_your_key, var_your_value_type, var_your_value)  { \
    zlongmap_node_t *var_your_node; char *var_your_key; var_your_value_type var_your_value; \
    (void)var_your_key; (void)var_your_value; \
    for(var_your_node = zlongmap_first(longmap); var_your_node; var_your_node = zlongmap_next(var_your_node)) { \
        var_your_key=var_your_node->key; var_your_value=(var_your_value_type)(void *)var_your_node->value; {
#define ZLONGMAP_WALK_END    }}}

/* mpool, src/malloc/ ################################################ */
/* 内存池, 不推荐使用 */
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

/* 创建通用型内存池 */
zmpool_t *zmpool_create_common_pool(int *register_size_list);

/* 创建贪婪型内存池 */
zmpool_t *zmpool_create_greedy_pool(int single_buf_size, int once_malloc_max_size);

/* 释放 */
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

/* 内存 queue */
zmqueue_t *zmqueue_create(int element_size, int element_count_per_queue);
void zmqueue_free(zmqueue_t *mq);
void *zmqueue_require_and_push(zmqueue_t *mq);
void zmqueue_release_and_shift(zmqueue_t *mq);
void *zmqueue_get_head(zmqueue_t *mq);

/* encode/decode, src/encode/ ###################################### */
/* 请注意, src_size < 0, 则 src_size = strlen(src) */
/* 函数内部重置 str, zbuf_reset(str) */

/* base64 */
void zbase64_encode(const void *src, int src_size, zbuf_t *str, int mime_flag);
void zbase64_decode(const void *src, int src_size, zbuf_t *str);
int zbase64_decode_get_valid_len(const void *src, int src_size);
int zbase64_encode_get_min_len(int in_len, int mime_flag);
void zuudecode(const void *src, int src_size, zbuf_t *str);

/* quoted-printable */
void zqp_encode_2045(const void *src, int src_size, zbuf_t *result, zbool_t mime_flag);
void zqp_encode_2047(const void *src, int src_size, zbuf_t *result);
void zqp_decode_2045(const void *src, int src_size, zbuf_t *str);
void zqp_decode_2047(const void *src, int src_size, zbuf_t *str);
int zqp_decode_get_valid_len(const void *src, int src_size);

/* hex */
void zhex_encode(const void *src, int src_size, zbuf_t *dest);
void zhex_decode(const void *src, int src_size, zbuf_t *dest);

/* url */
void zurl_hex_decode(const void *src, int src_size, zbuf_t *str);
void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag);

/* 返回写入 wchar 的长度 */
int zncr_decode(int ins, char *wchar);

/* crc, src/hash ################################################### */
/* crc16, crc32, crc64, init_value 默认应该为 0 */
unsigned short int zcrc16(const void *data, int size, unsigned short int init_value);
unsigned int zcrc32(const void *data, int size, unsigned int init_value);
unsigned long zcrc64(const void *data, int size, unsigned long init_value);

/* config, src/stdlib/config.c ##################################### */
/* 一个简单的通用配置文件风格
 * 推荐使用, 不强制使用. 一些内嵌服务和master/server使用此配置风格 
 * 行首第一个非空字符是#, 则忽略本行
 * 每配置行以 "=" 为分隔符
 * 配置项和配置值都需要过滤掉两侧的空白
 * 不支持任何转义
 * 相同配置项, 以后一个为准
 */

extern zconfig_t *zvar_default_config;
zconfig_t *zdefault_config_init();
void zdefault_config_fini();

#define zconfig_create  zdict_create
#define zconfig_free    zdict_free
#define zconfig_update  zdict_update
#define zconfig_update_string  zdict_update_string
#define zconfig_delete  zdict_delete
#define zconfig_reset   zdict_reset
#define zconfig_len     zdict_len
#define zconfig_data    zdict_data
#define zconfig_debug_show    zdict_debug_show

/* 从文件pathname加载配置到cf, 同名则覆盖 */
int zconfig_load_from_pathname(zconfig_t *cf, const char *pathname);

/* 从配置another中加载配置到cf, 同名则覆盖 */
void zconfig_load_annother(zconfig_t *cf, zconfig_t *another);

/* 快速处理大批配置 */
typedef struct {
    const char *name;
    const char *defval;
    const char **target;
} zconfig_str_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
} zconfig_int_table_t;
typedef struct {
    const char *name;
    long defval;
    long *target;
} zconfig_long_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
} zconfig_bool_table_t;
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

/* 宏, 遍历. zconfig_t *cf; char *key; zbuf_t *value;  */
#define ZCONFIG_WALK_BEGIN(cf, key, value) { \
    zdict_node_t *___nd; char *key; zbuf_t *value; \
    for (___nd = zdict_first(cf);___nd;___nd=zdict_next(___nd)) { \
        key = zdict_node_key(___nd); value = zdict_node_value(___nd); {
#define ZCONFIG_WALK_END }}}

/* pthread pool ################################################### */
zpthread_pool_t *zpthread_pool_create();
void zpthread_pool_free(zpthread_pool_t *ptp);
void zpthread_pool_set_debug_flag(zpthread_pool_t *ptp, zbool_t flag);
void zpthread_pool_set_min_max_count(zpthread_pool_t *ptp, int min, int max);
void zpthread_pool_set_idle_timeout(zpthread_pool_t *ptp, int timeout);
int zpthread_pool_get_current_count(zpthread_pool_t *ptp);
int zpthread_pool_get_queue_length(zpthread_pool_t *ptp);
void zpthread_pool_set_pthread_init_handler(zpthread_pool_t *ptp, void (*pthread_init_handler)(zpthread_pool_t *ptp));
void zpthread_pool_set_pthread_fini_handler(zpthread_pool_t *ptp, void (*pthread_fini_handler)(zpthread_pool_t *ptp));
void zpthread_pool_set_context(zpthread_pool_t *ptp, void *ctx);
void *zpthread_pool_get_context(zpthread_pool_t *ptp);
zpthread_pool_t *zpthread_pool_get_current_zpthread_pool();
long zpthread_pool_get_max_running_millisecond(zpthread_pool_t *ptp);
void zpthread_pool_softstop(zpthread_pool_t *ptp);
void zpthread_pool_wait_all_stopped(zpthread_pool_t *ptp, int max_second);
void zpthread_pool_start(zpthread_pool_t *ptp);
void zpthread_pool_job(zpthread_pool_t *ptp, void (*callback)(void *ctx), void *ctx);
void zpthread_pool_timer(zpthread_pool_t *ptp, void (*callback)(void *ctx), void *ctx, int timeout);

/* io ############################################################# */
/* -1: 出错  0: 不可读写, 1: 可读写或socket异常  */
int zrwable(int fd);
int zreadable(int fd);
int zwriteable(int fd);

/* 设置fd非阻塞, 或 阻塞 */
/* 返回 -1: 出错, 0: 现在是阻塞, 1: 现在是非阻塞 */
int znonblocking(int fd, int no);

/* 设置 close_on_exec */
/* 返回 -1: 出错, 0: 没设置, 1: 已经设置 */
int zclose_on_exec(int fd, int on);

/* 检查fd有多少可读字节 */
int zget_readable_count(int fd);

/* 下面这些, 忽略信号EINTR的封装 */
int zopen(const char *pathname, int flags, mode_t mode);
ssize_t zread(int fd, void *buf, size_t count);
ssize_t zwrite(int fd, const void *buf, size_t count);
int zclose(int fd);
int zflock(int fd, int operation);
int zflock_share(int fd);
int zflock_exclusive(int fd);
int zfunlock(int fd);
int zrename(const char *oldpath, const char *newpath);
int zunlink(const char *pathname);
int zlink(const char *oldpath, const char *newpath);

/* 进程间传递fd, 返回 -1: 错, >-1: 成功 */
int zsend_fd(int fd, int sendfd);

/* 进程间接受fd, 返回 -1: 错, 其他: 接受到的fd */
int zrecv_fd(int fd);

/* timed_io ######################################################## */
/* 除非函数名或其他特别标注, 所有timeout单位都是秒, -1表示无限长 */

/* <0: 出错  0: 不可读写, 1: 可读写或socket异常 */
int ztimed_read_write_wait(int fd, int read_write_wait_timeout, int *readable, int *writeable);
int ztimed_read_write_wait_millisecond(int fd, long read_write_wait_timeout, int *readable, int *writeable);

/* <0: 出错  0: 不可读, 1: 可读或socket异常 */
int ztimed_read_wait_millisecond(int fd, long read_wait_timeout);
int ztimed_read_wait(int fd, int read_wait_timeout);

/* <0: 出错, >0: 正常, 0: 不可读 */
int ztimed_read(int fd, void *buf, int size, int read_wait_timeout);

/* <0: 出错  0: 不可写, 1: 可写或socket异常 */
int ztimed_write_wait_millisecond(int fd, long write_wait_timeout);
int ztimed_write_wait(int fd, int write_wait_timeout);

/* <0: 出错, >0: 正常, 0: 不可写 */
int ztimed_write(int fd, const void *buf, int size, int write_wait_timeout);

/* tcp socket ##################################################### */
#define zvar_tcp_listen_type_inet  'i'
#define zvar_tcp_listen_type_unix  'u'
#define zvar_tcp_listen_type_fifo  'f'
/* accept domain socket, 忽略EINTR */
int zunix_accept(int fd);

/* accept socket, 忽略EINTR */
int zinet_accept(int fd);

/* accept, 忽略EINTR */
int zaccept(int sock, int type);

/* listen */
int zunix_listen(char *addr, int backlog);
int zinet_listen(const char *sip, int port, int backlog);
int zlisten(const char *netpath, int *type, int backlog);

int zfifo_listen(const char *path);

/* connect, 忽略EINTR */
int zunix_connect(const char *addr, int timeout);
int zinet_connect(const char *dip, int port, int timeout);
int zhost_connect(const char *host, int port, int timeout);
int zconnect(const char *netpath, int timeout);

/* openssl, src/stdlib/openssl.c #################################### */
extern zbool_t zvar_openssl_debug;

/* zopenssl_init, 初始化openssl环境, 支持线程安全openssl环境 */
void zopenssl_init(void);
void zopenssl_fini(void);

/* 创建服务端 SSL_CTX */
/* cert_file: 证书文件, key_file: 私钥文件 */
SSL_CTX *zopenssl_SSL_CTX_create_server(const char *cert_file, const char *key_file);

/* 创建客户端 SSL_CTX */
SSL_CTX *zopenssl_SSL_CTX_create_client(void);

/* 释放 SSL_CTX */
void zopenssl_SSL_CTX_free(SSL_CTX *ctx);

/* 支持 SNI */
/* get_ssl_ctx_by_server_name 为回调函数, 其参数为servername, 根据servername 返回合适的 SSL_CTX */
void zopenssl_SSL_CTX_support_sni(SSL_CTX *ctx, SSL_CTX *(*get_ssl_ctx_by_server_name)(const char *servername));

/* 获取错误, *ecode: 错误码, buf: 错误信息, buf_len: 错误信息buf长度 */
void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len);

/* 创建 SSL */
SSL *zopenssl_SSL_create(SSL_CTX *ctx, int fd);
void zopenssl_SSL_free(SSL *ssl);

/* 获取 fd */
int zopenssl_SSL_get_fd(SSL *ssl);

/* 带超时的ssl connect, timeout: 秒, 下同, 返回 -1:错/超时, 1:成功 */
int zopenssl_timed_connect(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

/* 带超时的ssl accept, 返回 -1:错/超时, 1:成功 */
int zopenssl_timed_accept(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

/* 带超时的ssl shutdown, 返回 -1:错/超时, 1:成功 */
int zopenssl_timed_shutdown(SSL *ssl, int read_wait_timeout, int write_wait_timeout);

/* 带超时的ssl read, 返回 和 -1:错/超时, 其他请看 ssl_read 帮助文档 */
int zopenssl_timed_read(SSL *ssl, void *buf, int len, int read_wait_timeout, int write_wait_timeout);

/* 带超时的ssl write, 返回 和 -1:错/超时, 其他请看 ssl_write 帮助文档 */
int zopenssl_timed_write(SSL *ssl, const void *buf, int len, int read_wait_timeout, int write_wait_timeout);

/* stream, src/stream/ ############################################## */
/* fd/ssl流实现, 例子见 sample/stream/ */

#define zvar_stream_rbuf_size           4096
#define zvar_stream_wbuf_size           4096

typedef struct zstream_engine_t zstream_engine_t;
struct zstream_engine_t {
    const char *(*get_type)();
    int (*close_fn)(zstream_t *fp, int release_ioctx);
    int (*read_fn)(zstream_t *fp, void *buf, int len);
    int (*write_fn)(zstream_t *fp, const void *buf, int len);
    int (*timed_read_wait_fn)(zstream_t *fp, int read_wait_timeout);
    int (*timed_write_wait_fn)(zstream_t *fp, int write_wait_timeout);
    int (*get_fd_fn)(zstream_t *fp);
};

struct zstream_t {
    int read_wait_timeout;
    int write_wait_timeout;
    void *ioctx;
    zstream_engine_t *engine;
    short int read_buf_p1;
    short int read_buf_p2;
    short int write_buf_len;
    unsigned short int error:1;
    unsigned short int eof:1;
    unsigned short int auto_release_ioctx:1;
    unsigned char read_buf[zvar_stream_rbuf_size];
    unsigned char write_buf[zvar_stream_wbuf_size];
};

/* 宏, 返回读取的下一个字符, -1:错 */
#define ZSTREAM_GETC(fp)            (((fp)->read_buf_p1<(fp)->read_buf_p2)?((int)((fp)->read_buf[(fp)->read_buf_p1++])):(zstream_getc_do(fp)))

/* 宏, 写一个字符ch到fp, -1:错 */
#define ZSTREAM_PUTC(fp, ch)        (((fp)->write_buf_len<zvar_stream_wbuf_size)?((fp)->write_buf[(fp)->write_buf_len++]=(unsigned char)(ch),(int)(ch)):(zstream_putc_do(fp, ch)))

/* 是否出错 */
zinline zbool_t zstream_is_error(zstream_t *fp) { return fp->error; }

/* 是否读到结尾 */
zinline zbool_t zstream_is_eof(zstream_t *fp) { return fp->eof; }

/* 是否异常(错或读到结尾) */
zinline zbool_t zstream_is_exception(zstream_t *fp) { return (fp->eof)||(fp->error); }

/* 可读缓存的长度 */
zinline int zstream_get_read_cache_len(zstream_t *fp) { return ((fp)->read_buf_p2-(fp)->read_buf_p1); }

/* 读缓存的指针 */
zinline char * zstream_get_read_cache(zstream_t *fp) { return (char *)((fp)->read_buf); }

/* 已经写入的缓存的长度 */
zinline int zstream_get_write_cache_len(zstream_t *fp) { return ((fp)->write_buf_len); }

/* 写缓存的指针 */
zinline char * zstream_get_write_cache(zstream_t *fp) {
    fp->write_buf[fp->write_buf_len] = 0; return (char *)((fp)->write_buf);
}

/* 基于文件描述符fd创建stream */
zstream_t *zstream_open_fd(int fd);

/* 基于ssl创建stream */ 
zstream_t *zstream_open_ssl(SSL *ssl);

/* 打开本地文件, mode: "r", "r+", "w", "w+", "a", "a+" */
/* 推荐使用标准库FILE *, 只有在极为特殊的情况下才建议用这个函数, 一般情况就用FILE * */
zstream_t *zstream_open_file(const char *pathname, const char *mode);

/* 打开地址destination, timeout:是超时, 单位秒. destination: 见 zconnect */
zstream_t *zstream_open_destination(const char *destination, int timeout);

/* 关闭stream, release_ioctx: 是否同时关闭相关fd */
int zstream_close(zstream_t *fp, zbool_t release_ioctx);

/* 返回 fd */
int zstream_get_fd(zstream_t *fp);

/* 返回ssl */
SSL *zstream_get_ssl(zstream_t *fp);

/* 发起tls_connect, 返回 -1:错 */
int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx);

/* 发起tls_accept, 返回 -1:错 */
int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx);

/* 通用超时等待可读, 单位秒, timeout<0: 表示无限长 */
void zstream_set_read_wait_timeout(zstream_t *fp, int read_wait_timeout);

/* 通用超时等待可写, 单位秒 */
void zstream_set_write_wait_timeout(zstream_t *fp, int write_wait_timeout);

/* 超时等待可读, 单位秒, timeout<0: 表示无限长 */
/* -1: 出错  0: 不可读, 1: 可读或socket异常 */
int zstream_timed_read_wait(zstream_t *fp, int read_wait_timeout);

/* 超时等待可写, 单位秒 */
/* <0: 出错  0: 不可写, 1: 可写或socket异常 */
int zstream_timed_write_wait(zstream_t *fp, int write_wait_timeout);

/* 读取一个字符, -1: 错误 */
/* 不应该使用这个函数 */
int zstream_getc_do(zstream_t *fp);

/* 读取一个字符, -1: 错误 */
zinline int zstream_getc(zstream_t *fp) { return ZSTREAM_GETC(fp); }

/* 使用条件太苛刻, 不推荐使用 */
void zstream_ungetc(zstream_t *fp);

/* 读 max_len个字节到bf, -1: 错, 0: 不可读, >0: 读取字节数 */
int zstream_read(zstream_t *fp, zbuf_t *bf, int max_len);
int zstream_read_to_mem(zstream_t *fp, void *mem, int max_len);

/* 严格读取strict_len个字符 */
int zstream_readn(zstream_t *fp, zbuf_t *bf, int strict_len);
int zstream_readn_to_mem(zstream_t *fp, void *mem, int strict_len);

/* 读取最多max_len个字符到bf, 读取到delimiter为止 */
int zstream_read_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len);
int zstream_read_delimiter_to_mem(zstream_t *fp, void *mem, int delimiter, int max_len);

/* 读取一行 */
zinline int zstream_gets(zstream_t *fp, zbuf_t *bf, int max_len) {
    return zstream_read_delimiter(fp, bf, '\n', max_len);
}
zinline int zstream_gets_to_mem(zstream_t *fp, void *mem, int max_len) {
    return zstream_read_delimiter_to_mem(fp, mem, '\n', max_len);
}

int zstream_get_cint(zstream_t *fp);

/* 写一个字节ch, 返回-1:错, 返回ch:成功 */
/* 不推荐使用zstream_putc_do, 而是使用 zstream_putc */
int zstream_putc_do(zstream_t *fp, int ch);

/* 写一个字节ch, 返回-1:错, 返回ch:成功 */
zinline int zstream_putc(zstream_t *fp, int c) { return ZSTREAM_PUTC(fp, c); }

/* 写长度为len的buf到fp, 返回-1:失败, 其他:成功 */
int zstream_write(zstream_t *fp, const void *buf, int len);

/* 写一行s 到fp, 返回-1:失败, 其他:成功 */
int zstream_puts(zstream_t *fp, const char *s);

#define zstream_puts_const(fp, s) zstream_write(fp, s, sizeof(s)-1)

/* 写bf到fp, 返回-1:失败, 其他:成功 */
zinline int zstream_append(zstream_t *fp, zbuf_t *bf) {
    return zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
}

/* zstream_printf_1024, 意思是:
 * char buf[1024+1]; sprintf(buf, format, ...); zstream_puts(fp, buf); */
int zstream_printf_1024(zstream_t *fp, const char *format, ...);

int zstream_write_cint(zstream_t *fp, int len);
int zstream_write_cint_and_int(zstream_t *fp, int i);
int zstream_write_cint_and_long(zstream_t *fp, long l);
int zstream_write_cint_and_data(zstream_t *fp, const void *buf, int len);
int zstream_write_cint_and_dict(zstream_t *fp, zdict_t * zd);
int zstream_write_cint_and_pp(zstream_t *fp, const char **pp, int size);

/* flush, 返回-1:错 */
int zstream_flush(zstream_t *fp);

/* time, src/stdlib/time.c ########################################## */
#define zvar_max_timeout_millisecond (3600L * 24 * 365 * 10 * 1000)

/* 返回当前毫秒精度的时间 */
long zmillisecond(void);

/* 睡眠delay毫秒 */
void zsleep_millisecond(int delay);

/* 返回 zmillisecond(void) + timeout */
long ztimeout_set_millisecond(long timeout);

/* 返回 stamp - zmillisecond(void) */
long ztimeout_left_millisecond(long stamp);

#define zvar_max_timeout (3600 * 24 * 365 * 10)

/* 返回当前秒精度的时间 */
long zsecond(void);

/* 睡眠delay秒 */
void zsleep(int delay);

/* 返回 zsecond(void) + timeout */
long ztimeout_set(int timeout);

/* 返回 stamp - zsecond(void) */
int ztimeout_left(long stamp);

/* date, src/stdlib/date.c ########################################## */

#define zvar_rfc1123_date_string_size 32
/* 根据t(秒)生成rfc1123格式的时间字符串,存储在buf, 并返回, buf的长度不小于 zvar_rfc1123_date_string_size */
char *zbuild_rfc1123_date_string(long t, char *buf);

#define zvar_rfc822_date_string_size 38
/* 根据t(秒)生成rfc822格式的时间字符,存储在buf并, 返回, buf的长度不小于 zvar_rfc822_date_string_size */
char *zbuild_rfc822_date_string(long t, char *buf);

/* dns, src/stdlib/dns.c ############################################ */

/* 获取本机ip地址, 返回ip地址个数 */
int zget_localaddr(zargv_t *addrs);

/* 获取host对应的ip地址, 返回ip地址个数 */
int zget_hostaddr(const char *host, zargv_t *addrs);

/* 获取socket文件描述符sockfd,另一端的ip和端口信息; host: struct in_addr */
zbool_t zget_peername(int sockfd, int *host, int *port);

/* ip(struct in_addr)转ip地址(1.1.1.1), 结果存储在ipstr, 并返回 */
char *zget_ipstring(int ip, char *ipstr);

/* 是不是 IP 地址, 返回 1 或 0 */
int zis_ip(const char *ip);

/* zget_ipstring 的反操作 */
int zget_ipint(const char *ipstr);

/* 返回ip的网络地址, masklen是掩码长度(下同) */
int zget_network(int ip, int masklen);

/* 返回子网掩码 */
int zget_netmask(int masklen);

/* 返回ip的广播地址 */
int zget_broadcast(int ip, int masklen);

/* 返回ip所在网段的最小地址 */
int zget_ip_min(int ip, int masklen);

/* 返回ip所在网段的最大地址 */
int zget_ip_max(int ip, int masklen);

/* 是否保留地址 */
int zip_is_intranet(int ip);

/* 是否保留地址 */
int zip_is_intranet2(const char *ip);

/* mime type ######################################################## */
extern const char *zvar_mime_type_application_cotet_stream /* = "application/octet-stream" */;

/* 返回mime类型, 如 txt => text/plain */
const char *zget_mime_type_from_suffix(const char *suffix, const char *def);
const char *zget_mime_type_from_pathname(const char *pathname, const char *def);

/* unique id ######################################################## */
/* 唯一id */
#define zvar_unique_id_size 22
char *zbuild_unique_id(char *buf);

/* 从唯一id中获得时间戳(秒)并返回 */
long zget_time_from_unique_id(const char *buf);

/* 检测是不是唯一id格式 */
zbool_t zis_unique_id(const char *buf);

/* system ########################################################### */
/* 如果user_name非空, 则改变实际用户为user_name */
/* 如果root_dir非空, 改变根(chroot)到root_dir */
/* 返回 -1: 失败, >=0: 成功 */
int zchroot_user(const char *root_dir, const char *user_name);

long zgettid(void);

/* 打开 core, 不建议使用, 推荐操作系统层面设置 */
/* megabyte: core文件大小, 单位 M, -1: 无限, 0: 禁用 */
zbool_t zset_core_file_size(int megabyte);

/* 设置 cgroup 的名字 */
zbool_t zset_cgroup_name(const char *name);

/* file, src/stdlib/file.c ########################################## */
/* -1: 错, >=0: 文件大小 */
int zfile_get_size(const char *pathname);

/* 保存data 到文件pathname, 覆盖pathname -1: 错, 1: 成功 */
int zfile_put_contents(const char *pathname, const void *data, int len);

/* 从文件pathname获取文件内容, 存储到(覆盖)result, -1:错, >= 文件长度 */
int zfile_get_contents(const char *pathname, zbuf_t *result);
/* 同上, 出错exit */
int zfile_get_contents_sample(const char *pathname, zbuf_t *result);

/* 从标准输入读取内容到(覆盖)bf */
int zstdin_get_contents(zbuf_t *bf);

/* mmap reader */
struct zmmap_reader_t {
    int fd;
    int len; /* 映射后, 长度 */
    char *data; /* 映射后, 指针 */
};

/* mmap 只读方式映射一个文件, -1: 错, 1: 成功  */
int zmmap_reader_init(zmmap_reader_t *reader, const char *pathname);
int zmmap_reader_fini(zmmap_reader_t *reader);

/* touch */
int ztouch(const char *pathname);

/* 通过find命令查抄文件, 找到的文件放到 file_argv, 出错exit, file_argv为0则创建 */
zargv_t *zfind_file_sample(zargv_t *file_argv, const char **pathnames, int pathnames_count, const char *pathname_match);

/* 获取mac地址; 返回个数, -1: 错; src/stdlib/mac_address.c */
int zget_mac_address(zargv_t *mac_list);

/* signal */
typedef void (*zsighandler_t)(int);
zsighandler_t zsignal(int signum, zsighandler_t handler);
zsighandler_t zsignal_ignore(int signum);

/* main_parameter, src/stdlib/main_argument.c ########################### */
extern char *zvar_progname;

extern int zvar_main_argc;
extern char **zvar_main_argv;

extern int zvar_memleak_check;
extern int zvar_sigint_flag;

extern int zvar_main_kv_argc;
extern char **zvar_main_kv_argv;

extern char **zvar_main_redundant_argv;
extern int zvar_main_redundant_argc;

/* 处理 main函数 argc/argv, 和 config 无缝结合, 很方便. 默认参数风格如下:
 * ./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 ... arg2 arg3
   执行 zmain_argument_run(argc, argv) 后, 会自动创建一个全局配置词典
   zconfigt_t *zvar_default_config;
  
 * 而且, 逻辑上
   zvar_default_config[name1] = val1
   zvar_default_config[name2] = val2
   zvar_default_config[bool1] = "yes"
   zvar_default_config[bool2] = "yes"
   zvar_main_redundant_argc = 3
   zvar_main_redundant_argv[0] = arg1
   zvar_main_redundant_argv[1] = arg2
   zvar_main_redundant_argv[3] = arg3

 * main_argument_run 最后处理
   如果 zconfig_get_bool(zvar_default_config, "debug", 0) == 1, 则 zvar_log_debug_enable = 1
   如果 zconfig_get_bool(zvar_default_config, "fatal-catch", 0) == 1, 则 zvar_log_fatal_catch = 1

 * 如果参数项是 -config somepath.cf , 则
   会立即加载配置文件somepath.cf到zvar_default_config

 * 遵循规则
   后加载的配置项覆盖先加载的配置项
   命令行上的配置项覆盖配置文件中的配置项
 */
void zmain_argument_run(int argc, char **argv);

/* 注册函数, 系统退出前执行, 按照注册相反的顺序执行 */
void zatexit(void (*func)(void *), void *);

/* license, src/stdlib/license.c ####################################### */
char *zlicense_build(const char *salt, const char *target, char *new_license);
#define  zvar_license_size  16
/* -1: 系统错, 0: 不匹配, 1: 匹配 */
int zlicense_check(const char *salt, const char *license /* target|new_license */);
int zlicense_check_from_config_pathname(const char *salt, const char *config_file, const char *key);
int zlicense_check_from_config_pathnames(const char *salt, const char **config_files, const char *key);
#if 1
/* 基于 MAC 地址的 license */
void zlicense_mac_build(const char *salt, const char *_mac, zbuf_t *result);
/* -1: 系统错, 0: 不匹配, 1: 匹配 */
zinline int zlicense_mac_check(const char *salt, const char *license) { return zlicense_check(salt, license); }
zinline int zlicense_mac_check_from_config_pathname(const char *salt, const char *config_file, const char *key) {
    return zlicense_check_from_config_pathname(salt, config_file, key);
}
#endif

/* event, src/event/ ################################################### */
/* 基于epoll的高并发io模型, 包括 事件, 异步io, 定时器, io映射. 例子见 sample/event/  */

/* 创建 aio */
#define zaio_create zaio_create_by_fd
zaio_t *zaio_create_by_fd(int fd, zaio_base_t *aiobase);
zaio_t *zaio_create_by_ssl(SSL *ssl, zaio_base_t *aiobase);
void zaio_free(zaio_t *aio, int close_fd_and_release_ssl);

/* 设置可读超时 */
void zaio_set_read_wait_timeout(zaio_t *aio, int read_wait_timeout);

/* 设置可写超时 */
void zaio_set_write_wait_timeout(zaio_t *aio, int write_wait_timeout);

/* 重新绑定 aio_base */
void zaio_rebind_aio_base(zaio_t *aio, zaio_base_t *aiobase);

/* 停止 aio, 只能在所属aio_base运行的线程执行 */
void zaio_disable(zaio_t *aio);

/* 获取结果, -2:超时(且没有任何数据), <0: 错, >0: 写成功,或可读的字节数 */
/* 如果是 read cint, >=0: 表示 cint的值 */
int zaio_get_result(zaio_t *aio);

/* 获取 fd */
int zaio_get_fd(zaio_t *aio);

/* 获取 SSL 句柄 */
SSL *zaio_get_ssl(zaio_t *aio);

/* 发起tls连接, 成功/失败/超时后回调执行callback */
void zaio_tls_connect(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio));

/* 发起tls接受, 成功/失败/超时后回调执行callback */
void zaio_tls_accept(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio));

/* 从缓存中获取数据 */
int zaio_get_read_cache_size(zaio_t *aio);
void zaio_get_read_cache(zaio_t *aio, zbuf_t *bf, int strict_len);
void zaio_get_read_cache_to_buf(zaio_t *aio, char *buf, int strict_len);

/* 获取缓存数据的长度 */
int zaio_get_write_cache_size(zaio_t *aio);
void zaio_get_write_cache(zaio_t *aio, zbuf_t *bf, int strict_len);
void zaio_get_write_cache_to_buf(zaio_t *aio, char *buf, int strict_len);

/* 如果可读(或出错)则回调执行函数 callback */
void zaio_readable(zaio_t *aio, void (*callback)(zaio_t *aio));

/* 如果可写(或出错)则回调执行函数 callback */
void zaio_writeable(zaio_t *aio, void (*callback)(zaio_t *aio));

/* 请求读, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
void zaio_read(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio));

/* 请求读, 严格读取strict_len个字节, 成功/失败/超时后回调执行callback */
void zaio_readn(zaio_t *aio, int strict_len, void (*callback)(zaio_t *aio));

/* */
void zaio_get_cint(zaio_t *aio, void (*callback)(zaio_t *aio));
void zaio_get_cint_and_data(zaio_t *aio, void (*callback)(zaio_t *aio));

/* 请求读, 读到delimiter为止, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
void zaio_read_delimiter(zaio_t *aio, int delimiter, int max_len, void (*callback)(zaio_t *aio));

/* 如上, 读行 */
zinline void zaio_gets(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio))
{
    zaio_read_delimiter(aio, '\n', max_len, callback);
}

/* 向缓存写数据, (fmt, ...)不能超过1024个字节 */
void zaio_cache_printf_1024(zaio_t *aio, const char *fmt, ...);

/* 向缓存写数据 */
void zaio_cache_puts(zaio_t *aio, const char *s);

/* 向缓存写数据 */
void zaio_cache_write(zaio_t *aio, const void *buf, int len);

/* */
void zaio_cache_write_cint(zaio_t *aio, int len);
void zaio_cache_write_cint_and_data(zaio_t *aio, const void *data, int len);

/* 向缓存写数据, 不复制buf */
void zaio_cache_write_direct(zaio_t *aio, const void *buf, int len);

/* 请求写, 成功/失败/超时后回调执行callback */
void zaio_cache_flush(zaio_t *aio, void (*callback)(zaio_t *aio));

/* */
int zaio_get_cache_size(zaio_t *aio);

/* 请求sleep, sleep秒后回调执行callback */
void zaio_sleep(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout);

/* 设置/获取上下文 */
void zaio_set_context(zaio_t *aio, const void *ctx);
void *zaio_get_context(zaio_t *aio);

/* 获取 zaio_base_t */
zaio_base_t *zaio_get_aio_base(zaio_t *aio);

/* */
void zaio_list_append(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio);
void zaio_list_detach(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio);

/* event/epoll 运行框架 */

/* 默认aio_base */
extern zaio_base_t *zvar_default_aio_base;

/* 创建 aio_base */
zaio_base_t *zaio_base_create();
void zaio_base_free(zaio_base_t *eb);

/* 获取当前线程运行的 aio_base */
zaio_base_t *zaio_base_get_current_pthread_aio_base();

/* 设置 aio_base 每次epoll循环需要执行的函数 */
void zaio_base_set_loop_fn(zaio_base_t *eb, void (*loop_fn)(zaio_base_t *eb));

/* 运行 aio_base */
void zaio_base_run(zaio_base_t *eb);

/* 通知 aio_base 停止, 既 zaio_base_run 返回 */
void zaio_base_stop_notify(zaio_base_t *eb);

/* 通知 aio_base, 手动打断 epoll_wait */
void zaio_base_touch(zaio_base_t *eb);

/* 设置/获取上下文 */
void zaio_base_set_context(zaio_base_t *eb, const void *ctx);
void *zaio_base_get_context(zaio_base_t *eb);

/* sleep秒后回调执行callback, 只执行一次 callback(ctx) */
void zaio_base_timer(zaio_base_t *eb, void (*callback)(void *ctx), void *ctx, int timeout);

/* iopipe 管道 */
void zaio_iopipe_enter(zaio_t *client, zaio_t *server, zaio_base_t *aiobase, void (*after_close)(void *ctx), void *ctx);

#ifndef ___ZC_LIB_INCLUDE_COROUTINE___
#define ___ZC_LIB_INCLUDE_COROUTINE___

/* coroutine, src/coroutine/ ########################################## */
/* 协程框架, 本协程不得跨线程操作 例子见 sample/coroutine/ */

typedef struct zcoroutine_base_t zcoroutine_base_t;
typedef struct zcoroutine_t zcoroutine_t;
typedef struct zcoroutine_mutex_t zcoroutine_mutex_t;
typedef struct zcoroutine_cond_t zcoroutine_cond_t;

extern int zvar_coroutine_max_fd; /* 10240 */

/* 在线程内初始化协程基础环境 */
zcoroutine_base_t *zcoroutine_base_init();

/* 获取当前协程环境 */
zcoroutine_base_t *zcoroutine_base_get_current();

/* 设置当前协程环境每次epoll循环需要执行的函数 */
void zcoroutine_base_set_loop_fn(void (*loop_fn)(zcoroutine_base_t *cb));

/* 在线程内运行当前协程框架 */
void zcoroutine_base_run(void);

/* 通知协程环境(cobs==0,表示当前)退出, 既 zcoroutine_base_run() 返回 */
void zcoroutine_base_stop_notify(zcoroutine_base_t *cobs);

/* 回收协程框架资源 */
void zcoroutine_base_fini();

/* 进入协程, 然后, 执行 start_job, 参数为 ctx */
/* stack_size: 协程栈大小(单位K), 建议不小于16 */
void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);
/* 同 zcoroutine_go, 指定协程环境 */
void zcoroutine_advanced_go(zcoroutine_base_t *cobs, void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte);

/* 返回当前协程 */
zcoroutine_t * zcoroutine_self();

/* 放弃当前协程使用权 */
void zcoroutine_yield();

/* 主动结束协程 */
void zcoroutine_exit();

/* 返回当前协程ID */
long zcoroutine_getid();

/* 睡眠 */
#define zcoroutine_sleep zsleep
#define zcoroutine_sleep_millisecond zsleep_millisecond

/* 获取当前协程上下文 */
void *zcoroutine_get_context();

/* 设置当前协程上下文 */
void zcoroutine_set_context(const void *ctx);

/* 主动设置fd支持协程切换 */
void zcoroutine_enable_fd(int fd);

/* 主动设置fd不支持协程切换 */
void zcoroutine_disable_fd(int fd);

/* 创建携程锁 */
zcoroutine_mutex_t * zcoroutine_mutex_create();
void zcoroutine_mutex_free(zcoroutine_mutex_t *);

/* 锁 */
void zcoroutine_mutex_lock(zcoroutine_mutex_t *);

/* 解锁 */
void zcoroutine_mutex_unlock(zcoroutine_mutex_t *);

/* 创建条件 */
zcoroutine_cond_t * zcoroutine_cond_create();
void zcoroutine_cond_free(zcoroutine_cond_t *);

/* 条件等待, 参考 pthread_cond_wait */
void zcoroutine_cond_wait(zcoroutine_cond_t *, zcoroutine_mutex_t *);

/* 条件信号, 参考 pthread_cond_signal */
void zcoroutine_cond_signal(zcoroutine_cond_t *);

/* 条件广播, 参考 pthread_cond_broadcast */
void zcoroutine_cond_broadcast(zcoroutine_cond_t *);

/* 禁用 UDP 协程切换 */
extern zbool_t zvar_coroutine_disable_udp/* = 0 */;

/* 禁用 53 端口的 UDP 协程切换 */
extern zbool_t zvar_coroutine_disable_udp_53/* = 0 */;

/* 启用limit个线程池, 用于文件io,和 block_do */
extern int zvar_coroutine_block_pthread_count_limit;

/* 如果
      zvar_coroutine_block_pthread_count_limit > 0 且
      zvar_coroutine_fileio_use_block_pthread == 1
   则 文件io在线程池执行, 否则在当前线程执行 */
extern zbool_t zvar_coroutine_fileio_use_block_pthread;

/* 如果 zvar_coroutine_block_pthread_count_limit > 0
   则 block_func(ctx) 在线程池执行, 否则在当前程直接执行 */
void *zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx);

/* zcoroutine_block_XXX 基于 zcoroutine_block_do 机制 */
int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset);
int zcoroutine_block_write(int fd, const void *data, int len);
long zcoroutine_block_lseek(int fd, long offset, int whence);
int zcoroutine_block_open(const char *pathname, int flags, mode_t mode);
int zcoroutine_block_close(int fd);
int zcoroutine_block_rename(const char *oldpath, const char *newpath);
int zcoroutine_block_unlink(const char *pathname);

#endif

/* io 映射 */
void zcoroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, zcoroutine_base_t *cobs, void (*after_close)(void *ctx), void *ctx);

/* master, src/master/ ################################################ */
/* master/server 进程服务管理框架, 例子见 sample/master/ */

extern zbool_t zvar_master_server_log_debug_enable;
extern int zvar_master_server_reload_signal; /* SIGHUP */

/* master 重新加载各服务配置函数 */
/* zvector_t *cfs, 是 zconfig_t *的vector, 一个zconfig_t 对应一个服务 */
extern void (*zmaster_server_load_config)(zvector_t *cfs);

/* master进入服务管理前执行的函数 */
extern void (*zmaster_server_before_service)();

/* 一个通用的加载一个目录下所有服务配置的函数 */
void zmaster_server_load_config_from_dirname(const char *config_dir_pathname, zvector_t *cfs);

/* master程序主函数 */
int zmaster_server_main(int argc, char **argv);

/* 基于事件(zaio_base_t 模型)的服务模型 #################  */
/* 主线程运行在 zaio_base_t 框架下, event_hase 为 zvar_default_aio_base */

/* 注册服务, service 是服务名, fd继承自master, fd_type: inet/unix/fifo 见(zvar_tcp_listen_type_inter ...) */
extern void (*zaio_server_service_register) (const char *service, int fd, int fd_type);

/* 进入主服务前执行函数 */
extern void (*zaio_server_before_service) (void);

/* 接到master重启命令后, 退出前执行的函数 */
extern void (*zaio_server_before_softstop) (void);

/* 手动通知主程序循环退出 */
void zaio_server_stop_notify(int stop_after_second);

/* 和master服务分离, master程序会以为此进程已经终止 */
/* master 发起reload时, 不会通知此进程 reload */
/* 1小时候后, 此进程强制退出 */
void zaio_server_detach_from_master();

/* 通用的服务注册函数 */
/* fd2 = accept(fd); callback(fd2) */
zaio_t *zaio_server_general_aio_register(zaio_base_t *eb, int fd, int fd_type, void (*callback) (int));

/* 主函数 */
int zaio_server_main(int argc, char **argv);

/* 基于协程(zcoroutine_base_init)的服务模型 #################  */
/* 主线程运行在协程框架下 */

/* 注册服务, service 是服务名, fd继承自master, fd_type: inet/unix/fifo 见(zvar_tcp_listen_type_inet ...) */
extern void (*zcoroutine_server_service_register) (const char *service, int fd, int fd_type);

/* 进入主服务前执行函数 */
extern void (*zcoroutine_server_before_service) (void);

/* 接到master重启命令后, 退出前执行的函数 */
extern void (*zcoroutine_server_before_softstop) (void);

/* 手动通知主程序循环退出 */
void zcoroutine_server_stop_notify(int stop_after_second);

/* 和master服务分离, master程序会以为此进程已经终止 */
/* master 发起reload时, 不会通知此进程 reload */
/* 1小时候后, 此进程强制退出 */
void zcoroutine_server_detach_from_master();

/* 主函数 */
int zcoroutine_server_main(int argc, char **argv);

/* charset, src/charset/ ############################################## */
/* 字符集函数, 例子见 sample/charset/ */

#define zvar_charset_name_max_size          32

extern zbool_t zvar_charset_debug;

/* { "UTF-8", "GB18030", "BIG5", "UTF-7", 0 } */
extern const char *zvar_charset_chinese[];

/* { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0 } */
extern const char *zvar_charset_japanese[];

/* { "UTF-8", "KS_C_5601", "KS_C_5861", "UTF-7", 0 } */
extern const char *zvar_charset_korean[];

/* { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", "UTF-7", 0 } */
extern const char *zvar_charset_cjk[];

/* 修正字符集名称, 如 GBK => GB18030, KS_C_5861 => EUC-KR */
char *zcharset_correct_charset(const char *charset);

/* 探测字符串data是什么字符集, 结果存储在charset_result, 并返回 */
/* charset_list 是 字符集名称的指针输入, 结尾为 0 */
char *zcharset_detect(const char **charset_list, const char *data, int size, char *charset_result);

/* 如上. charset_list = zvar_charset_cjk; */
char *zcharset_detect_cjk(const char *data, int size, char *charset_result);

/* 字符集转码, 有点复杂哈, 建议再次封装 */
/* 返回目标字符串的长度, -1: 错; */
/* from_charset: 原字符集; src, src_len: 原字符串和长度 */
/* to_charset: 目标字符集; result: 目标字符串, zbuf_reset(result) */
/* *src_converted_len = (成功转码的字节数); */
/* omit_invalid_bytes_limit: 设置可忽略的错误字节个数, <0: 无限大 */
/* *omit_invalid_bytes_count = (实际忽略字节数); */

extern int (*zcharset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);

int zcharset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);

extern int zvar_charset_uconv_mode;
void zcharset_convert_use_uconv();
int zcharset_uconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);

void zcharset_convert_to_utf8(const char *from_charset, const char *data, int size, zbuf_t *result);


/* mime utils, src/mime/ ############################################# */
/* 邮件解码工具 例子见 sample/mime/ */

/* 假设邮件头逻辑行最长长度不超过 zvar_mime_header_line_max_length */
#define zvar_mime_header_line_max_length   1024000

/* 邮件用, 字符集转码 */
/* 目标字符集为 UTF-8 */
/* from_charset: 原字符集, 为空则探测字符集, 探测失败则取值 GB18030 */
void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result);

/* 解码原始邮件头行 */
void zmime_raw_header_line_unescape(const char *in_line, int in_len, zbuf_t *result);

/* 获取邮件头行的第一个单词, trim两侧的 " \t<?\"'" */
void zmime_header_line_get_first_token(const char *in_line, int in_len, zbuf_t *result);

/* 邮件头行单词节点 */
typedef struct zmime_header_line_element_t zmime_header_line_element_t;
struct zmime_header_line_element_t {
    const char *charset; /* 可能为空 */
    const char *data;
    int dlen;
    char encode_type; /* 'B':base64, 'Q':qp, 0:unknown */
};

/* 分解邮件头行 */
const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len);
void zmime_header_line_element_vector_free(const zvector_t *element_vector);

/* 对邮件头行做分解并转码, 目标字符集UTF-8, ... 参考 zmime_iconv */
void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result);

/* 对符合RFC2231的邮件头行做分解并转码, 如上 */
void zmime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, int with_charset_flag);

/* 对邮件头行做处理, value 得到 第一个单词, params存储key<=>value */
void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *value, zdict_t *params);

/* 解码 Date 字段 */
long zmime_header_line_decode_date(const char *str);

/* 邮件地址 */
typedef struct zmime_address_t zmime_address_t;
struct zmime_address_t {
    char *name; /* 原始名称 */
    char *address; /* email 地址 */
    char *name_utf8; /* 原始名称转码为UTF-8 字符集 */
};

/* 分解邮件头行为多个邮件地址并返回, 这个时候不处理 name_utf8 */
zvector_t *zmime_header_line_get_address_vector(const char *in_str, int in_len);

/* 分解邮件头行为多个邮件地址并返回 */
zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len);
void zmime_header_line_address_vector_free(zvector_t *address_vector);

/* 邮件解析mime节点函数列表, 全部只读 */

/* type */
const char *zmime_get_type(zmime_t *mime);

/* 编码 */
const char *zmime_get_encoding(zmime_t *mime);

/* 字符集 */
const char *zmime_get_charset(zmime_t *mime);

/* disposition */
const char *zmime_get_disposition(zmime_t *mime);

/* 可读名字, 把mime的name或filename转码为UTF-8 */
const char *zmime_get_show_name(zmime_t *mime);

/* name */
const char *zmime_get_name(zmime_t *mime);

/* name转UTF-8 */
const char *zmime_get_name_utf8(zmime_t *mime);

/* filename */
const char *zmime_get_filename(zmime_t *mime);

/* RFC2231类型的filename;  *with_charset_flag是否带字符集 */
const char *zmime_get_filename2231(zmime_t *mime, zbool_t *with_charset_flag);

/* filename 转 UTF-8 */
const char *zmime_get_filename_utf8(zmime_t *mime);

/* CONTENT-ID */
const char *zmime_get_content_id(zmime_t *mime);

/* boundary */
const char *zmime_get_boundary(zmime_t *mime);

/* imap协议代码, 形如 1.2 或 2.1.6 */
const char *zmime_get_imap_section(zmime_t *mime);

/* mime头部 */
const char *zmime_get_header_data(zmime_t *mime);

/* mime邮件体 */
const char *zmime_get_body_data(zmime_t *mime);

/* mime头部相对于邮件起始偏移 */
int zmime_get_header_offset(zmime_t *mime);

/* mime头部长度 */
int zmime_get_header_len(zmime_t *mime);

/* mime邮件体相对于邮件起始偏移 */
int zmime_get_body_offset(zmime_t *mime);

/* mime 邮件体长度 */
int zmime_get_body_len(zmime_t *mime);

/* 下一个节点 */
zmime_t *zmime_next(zmime_t *mime);

/* 第一个子节点 */
zmime_t *zmime_child(zmime_t *mime);

/* 父节点 */
zmime_t *zmime_parent(zmime_t *mime);

/* 获取mime头 */
const zvector_t *zmime_get_raw_header_line_vector(zmime_t *mime); /* zsize_data_t* */

/* 获取第sn个名称为header_name的原始逻辑行, 返回 -1: 不存在 */
/* sn -1: 倒数第一个, 0:第一个, 1: 第二个, ... 下同 */
int zmime_get_raw_header_line(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

/* 获取第sn个名称为header_name的行的值, 返回 -1: 不存在 */
int zmime_get_header_line_value(zmime_t *mime, const char *header_name, zbuf_t *result, int sn);

/* 获取解码(base64, qp)后的mime邮件体 */
void zmime_get_decoded_content(zmime_t *mime, zbuf_t *result);

/* 获取解码(base64, qp)后并转UTF-8的mime邮件体 */
void zmime_get_decoded_content_utf8(zmime_t *mime, zbuf_t *result);

/* 是不是 tnef(application/ms-tnef) 类型的附件 */
zbool_t zmime_is_tnef(zmime_t *mime);

/* 下面是邮件解析函数 */

/* 创建邮件解析器, default_charset: 默认字符集 */
zmail_t *zmail_create_parser_from_data(const char *mail_data, int mail_data_len, const char *default_charset);
zmail_t *zmail_create_parser_from_pathname(const char *pathname, const char *default_charset);
void zmail_free(zmail_t *parser);

/* debug输出解析结果 */

void zmail_debug_show(zmail_t *parser);

/* 邮件源码data */
const char *zmail_get_data(zmail_t *parser);

/* 邮件源码长度 */
int zmail_get_len(zmail_t *parser);

/* 邮件头data */
const char *zmail_get_header_data(zmail_t *parser);

/* 邮件头偏移 */
int zmail_get_header_offset(zmail_t *parser);

/* 邮件头长度 */
int zmail_get_header_len(zmail_t *parser);

/*  邮件体 */
const char *zmail_get_body_data(zmail_t *parser);

/* 邮件体偏移 */
int zmail_get_body_offset(zmail_t *parser);

/* 邮件体偏移 */
int zmail_get_body_len(zmail_t *parser);

/* Message-ID */
const char *zmail_get_message_id(zmail_t *parser);

/* Subject */
const char *zmail_get_subject(zmail_t *parser);

/* Subject 转 UTF-8 */
const char *zmail_get_subject_utf8(zmail_t *parser);

/* 日期 */
const char *zmail_get_date(zmail_t *parser);

/* 日期, unix时间 */
long zmail_get_date_unix(zmail_t *parser);

/* In-Reply-To */
const char *zmail_get_in_reply_to(zmail_t *parser);

/* From */
const zmime_address_t *zmail_get_from(zmail_t *parser);

/* From并且处理name_utf8 */
const zmime_address_t *zmail_get_from_utf8(zmail_t *parser);

/* Sender */
const zmime_address_t *zmail_get_sender(zmail_t *parser);

/* Reply-To */
const zmime_address_t *zmail_get_reply_to(zmail_t *parser);

/* Disposition-Notification-To */
const zmime_address_t *zmail_get_receipt(zmail_t *parser);

/* To */
const zvector_t *zmail_get_to(zmail_t *parser); /* zmime_address_t* */

/* To并且处理name_utf8 */
const zvector_t *zmail_get_to_utf8(zmail_t *parser);
const zvector_t *zmail_get_cc(zmail_t *parser);
const zvector_t *zmail_get_cc_utf8(zmail_t *parser);
const zvector_t *zmail_get_bcc(zmail_t *parser);
const zvector_t *zmail_get_bcc_utf8(zmail_t *parser);

/* References */
const zargv_t *zmail_get_references(zmail_t *parser);

/* 顶层mime */
const zmime_t *zmail_get_top_mime(zmail_t *parser);

/* 全部mime */
const zvector_t *zmail_get_all_mimes(zmail_t *parser); /* zmime_t* */

/* 全部text/html,text/plain类型的mime */
const zvector_t *zmail_get_text_mimes(zmail_t *parser);

/* 应该在客户端显示的mime, alternative情况首选html */
const zvector_t *zmail_get_show_mimes(zmail_t *parser);

/* 所有附件类型的mime, 包括内嵌图片 */
const zvector_t *zmail_get_attachment_mimes(zmail_t *parser);

/* 获取所有邮件头 */
const zvector_t *zmail_get_raw_header_line_vector(zmail_t *parser); /* zsize_data_t* */

/* 获取第sn个名称为header_name的原始逻辑行, 返回 -1: 不存在 */
/* sn -1: 倒数第一个, 0:第一个, 1: 第二个, ... 下同 */
int zmail_get_raw_header_line(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

/* 获取第sn个名称为header_name的行的值, 返回 -1: 不存在 */
int zmail_get_header_line_value(zmail_t *parser, const char *header_name, zbuf_t *result, int sn);

/* 解析 tnef(application/ms-tnef) */

/* type */
const char *ztnef_mime_get_type(ztnef_mime_t *mime);

/* filename_utf8 */
const char *ztnef_mime_get_show_name(ztnef_mime_t *mime);

/* filename */
const char *ztnef_mime_get_filename(ztnef_mime_t *mime);

/* filename_utf8 */
const char *ztnef_mime_get_filename_utf8(ztnef_mime_t *mime);

/* Content-ID */
const char *ztnef_mime_get_content_id(ztnef_mime_t *mime);

/* mime_body 数据 */
const char *ztnef_mime_get_body_data(ztnef_mime_t *mime);

/* mime_body 长度 */
int ztnef_mime_get_body_len(ztnef_mime_t *mime);

/* 如果是text类型的mime, 则返回其文本字符集 */
const char *ztnef_mime_get_charset(ztnef_mime_t *mime);

/* 创建tnef解析器 */
ztnef_t * ztnef_create_parser_from_data(const char *tnef_data, int tnef_data_len, const char *default_charset);
ztnef_t *ztnef_create_parser_from_pathname(const char *pathname, const char *default_charset);
void ztnef_free(ztnef_t *parser);

/* 原始数据 */
const char *ztnef_get_data(ztnef_t *parser);

/* 原始数据长度 */
int ztnef_get_len(ztnef_t *parser);

/* 全部 mime */
const zvector_t *ztnef_get_all_mimes(ztnef_t *parser);

/* 全部附件 mime */
const zvector_t *ztnef_get_attachment_mimes(ztnef_t *parser);

/* 全部文本(应该作为正文显示) mime, 包括 plain, html, 和 rtf */
const zvector_t *ztnef_get_text_mimes(ztnef_t *parser);

/* debug输出 */
void ztnef_debug_show(ztnef_t *parser);

/* zjson_t, src/json/ #################################################### */
/* json 库, 序列号/反序列化, 例子见 sample/json/  */
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

/* 创建json */
zjson_t *zjson_create(void);

/* 创建undefined/null */
#define zjson_create_null zjson_create

/* 创建bool */
zjson_t *zjson_create_bool(zbool_t b);

/* long */
zjson_t *zjson_create_long(long l);

/* double */
zjson_t *zjson_create_double(double d);

/* array */
zjson_t *zjson_create_array();

/* object */
zjson_t *zjson_create_object();

/* string */
zjson_t *zjson_create_string(const void *s, int len);

/* 释放 */
void zjson_free(zjson_t *j);

/* 重置 */
void zjson_reset(zjson_t *j);

/* 从文件加载并分析(反序列化)数据 */
zbool_t zjson_load_from_pathname(zjson_t *j, const char *pathname);

/* 反序列化数据 */
zbool_t zjson_unserialize(zjson_t *j, const char *s, int len);

/* 序列化json, 结果追加到result */
void zjson_serialize(zjson_t *j, zbuf_t *result, int strict);

/* json的类型 */
zinline int zjson_get_type(zjson_t *j) { return j->type; }
zinline zbool_t zjson_is_null(zjson_t *j)   { return j->type==zvar_json_type_null; }
zinline zbool_t zjson_is_bool(zjson_t *j)   { return j->type==zvar_json_type_bool; }
zinline zbool_t zjson_is_long(zjson_t *j)   { return j->type==zvar_json_type_long; }
zinline zbool_t zjson_is_double(zjson_t *j) { return j->type==zvar_json_type_double; }
zinline zbool_t zjson_is_string(zjson_t *j) { return j->type==zvar_json_type_string; }
zinline zbool_t zjson_is_object(zjson_t *j) { return j->type==zvar_json_type_object; }
zinline zbool_t zjson_is_array(zjson_t *j)  { return j->type==zvar_json_type_array; }

/* 获取bool值的指针; 如果不是bool类型,则首先转换为bool类型, 默认为 0 */
zbool_t *zjson_get_bool_value(zjson_t *j);

/* 获取long值的指针; 如果不是long类型, 则首先转换为long类型, 默认为 0 */
long *zjson_get_long_value(zjson_t *j);

/* 获取double值的指针; 如果不是double类型, 则首先转换为long类型, 默认为 0 */
double *zjson_get_double_value(zjson_t *j);

/* 获取(zbuf_t *)值的指针; 如果不是zbuf_t *类型, 则首先转换为zbuf_t *类型, 值默认为 "" */
zbuf_t **zjson_get_string_value(zjson_t *j);

/* 获取数组值的指针; 如果不是数组类型, 则首先转换为数组类型, 默认为 [] */
const zvector_t *zjson_get_array_value(zjson_t *j); /* <zjson_t *> */

/* 获取对象值的指针; 如果不是对象类型, 则首先转换为对象类型, 默认为 {} */
const zmap_t *zjson_get_object_value(zjson_t *j); /* <char *, zjson_t *> */

/* 如果不是数组,先转为数组, 获取下表为idx的 子json */
zjson_t *zjson_array_get(zjson_t *j, int idx);

/* 如果不是对象,先转为对象, 获取下键为key的子json */
zjson_t *zjson_object_get(zjson_t *j, const char *key);

/* 如果不是数组,先转为数组, 获取数组长度 */
int zjson_array_get_len(zjson_t *j);

/* 如果不是对象,先转为对象, 获取子json个数 */
int zjson_object_get_len(zjson_t *j);

/* 如果不是数组,先转为数组, 在数组后追加element(json). 返回element */
zjson_t *zjson_array_push(zjson_t *j, zjson_t *element);
#define zjson_array_add zjson_array_push

/* 如果不是数组,先转为数组, 存在则返回1, 否则返回 0;
 * element不为0,则pop出来的json赋值给*element, 否则销毁 */
zbool_t zjson_array_pop(zjson_t *j, zjson_t **element);


/* 如果不是数组,先转为数组, 在数组前追加element(json). 返回element */
zjson_t *zjson_array_unshift(zjson_t *j, zjson_t *element);

/* 如果不是数组,先转为数组, 存在则返回1, 否则返回 0;
 * element不为0,则shift出来的json赋值给*element, 否则销毁 */
zbool_t zjson_array_shift(zjson_t *j, zjson_t **element);

/* 已知 json = [1, {}, "ss" "aaa"]
 * 1, zjson_array_update 给键idx设置成员element. 返回element
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

/* 把element插入idx处, idx及其后元素顺序后移 */
zjson_t *zjson_array_insert(zjson_t *j, int idx, zjson_t *element);

/* 移除idx处json,并把其值付给 *old_element, idx后元素属性前移 */
void zjson_array_delete(zjson_t *j, int idx, zjson_t **old_element);

/* 增加或更新键为key对应的json, 新值为element;
 * 旧值如果存在则赋值给*old_element, 如果old_element为了0则销毁  */
zjson_t *zjson_object_update(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element);
#define zjson_object_add zjson_object_update

/* 移除键key及对应的json;
 * json如果存在则赋值给*old_element, 如果old_element为了0则销毁  */
void zjson_object_delete(zjson_t *j, const char *key, zjson_t **old_element);

/* 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
 * zjson_get_element_by_path(json, "group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
zjson_t *zjson_get_element_by_path(zjson_t *j, const char *path);


/* 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
 * zjson_get_element_by_path_vec(json, "group", "linux", "2", "me", 0); 返回的 应该是 {age:18, sex:"male"} */
zjson_t *zjson_get_element_by_path_vec(zjson_t *j, const char *path0, ...);


/* 返回父节点 */
zinline zjson_t *zjson_get_parent(zjson_t *j) { return j->parent; }

/* 返回祖先 */
zjson_t *zjson_get_top(zjson_t *j);

/* debug */
void zjson_debug_show(zjson_t *j);

/* xml */
void zxml_unescape_string(zbuf_t *content, const char *data, int len);

/* memcache client, src/memcache/ ##################################### */
/* memcache 客户端, 例子见 sample/memcache/ */

/* 创建连接器; destination: 见 zconnect; timeout: connect超时,单位秒; */
zmemcache_client_t *zmemcache_client_connect(const char *destination, int connect_timeout);

/* 设置connect超时时间, 单位秒 */
void zmemcache_client_set_connect_timeout(zmemcache_client_t *mc, int connect_timeout);

/* 设置可读超时时间, 单位秒 */
void zmemcache_client_set_read_wait_timeout(zmemcache_client_t *mc, int read_wait_timeout);

/* 设置可写超时时间, 单位秒 */
void zmemcache_client_set_write_wait_timeout(zmemcache_client_t *mc, int write_wait_timeout);

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

/* redis client, src/redis/ ########################################### */
/* redis 客户端, 例子见 sample/redis/ */

/* 创建连接器 */
/* destinations: 见 zconnect */
/* password: 密码, 空: 没有密码 */
/* connect_timeout: 连接超时,单位秒 */
zredis_client_t *zredis_client_connect(const char *destination, const char *password, int connect_timeout);

/* 创建连接器, 同上. 连接集群 */
zredis_client_t *zredis_client_connect_cluster(const char *destination, const char *password, int connect_timeout);

/* 设置命令超时时间, 单位秒 */
void zredis_client_set_connect_timeout(zredis_client_t *rc, int connect_timeout);

/* 设置可读超时时间, 单位秒 */
void zredis_client_set_read_wait_timeout(zredis_client_t *rc, int read_wait_timeout);

/* 设置可写超时时间, 单位秒 */
void zredis_client_set_write_wait_timeout(zredis_client_t *rc, int write_wait_timeout);

/* 设置是否自动重连 */
void zredis_client_set_auto_reconnect(zredis_client_t *rc, zbool_t auto_reconnect);

/* 获取错误信息 */
const char * zredis_client_get_error_msg(zredis_client_t *rc);

/* 释放 */
void zredis_client_disconnect(zredis_client_t *rc);

/* redis命令返回结果可以抽象为json, 绝大部分可以简化为4类:
 * 1: 成功/失败, 2: 整数, 3: 字符串, 4:字符换vecgtor */

/* 下面的 (redis_fmt, ...) 介绍:
 *  s: char *
 *  S: zuf_t *
 *  d: int
 *  l: long int
 *  f: double
 *  L: zlist_t * <zbuf_t *>
 *  V: zvector_t * <zbuf_t *>
 *  A: zargv_t *
 *  P: char **, 0结尾
 *  D: zsize_data_t
 * 例子1: 检查key是否存在:
 *      zredis_client_get_success(rc, "ss", "EXISTS", "somekey");
 * 例子2: 将 key 所储存的值加上增量 increment:
 *      zredis_client_get_long(rc, &number_ret, "ssl", "INRBY", "somekey", some_longint); 或
 *      zredis_client_get_long(rc, &number_ret, "sss", "INRBY", "somekey", "some_longint_string");
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
extern void (*zredis_puny_server_before_softstop)(void);

/* 如果注册且返回1: 表明调用者成功注册了这个服务, 否则启用 redis 服务*/
extern zbool_t (*zredis_puny_server_service_register) (const char *service, int fd, int fd_type);

int zredis_puny_server_main(int argc, char **argv);
void zredis_puny_server_exec_cmd(zvector_t *cmd);

/* url, src/http/ ################################################# */
/* url 解析, 例子见 sample/http/ */
struct zurl_t {
    char *scheme;
    char *destination;
    char *host;
    char *path;
    char *query;
    char *fragment;
    int port;
};

/* 解析url字符换 */
zurl_t *zurl_parse(const char *url_string);
void zurl_free(zurl_t *url);

/* debug输出 */
void zurl_debug_show(zurl_t *url);

/* 解析query部分, 保存在query_vars, 并返回; 如果 query_vars==0, 则创建 */
zdict_t *zurl_query_parse(const char *query, zdict_t *query_vars);

/* 从词典query_vars, 组成query字符串保存在query_result; strict_flag: 是否严格编码 */
char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_result, zbool_t strict);

/* cookie, src/http/ ############################################### */
/* 例子见 sample/http/ */

/* 解析cookie字符串, 保存在cookies, 并返回; 如果 cookies == 0, 则创建 */
zdict_t *zhttp_cookie_parse(const char *raw_cookie, zdict_t *cookies);

/* 生成一个cookie条目, 保存在 cookie_result */
/* name: 名称; */
/* value: 值; 为空则删除 */
/* expires: 设置过期时间, >0 生效 */
/* path: cookie 路径, 为空则忽略 */
/* domain: 作用域, 为空则忽略 */
/* secure: 是否安全 */
/* httponly: 是否httponly */
char *zhttp_cookie_build_item(const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly, zbuf_t *cookie_result);

/* httpd, src/http/ ############################################### */
/* httpd, 例子见 sample/http/ */
extern zbool_t zvar_httpd_no_cache;

/* 从fd, 或ssl 创建http对象 */
zhttpd_t *zhttpd_open_fd(int sock);
zhttpd_t *zhttpd_open_ssl(SSL *ssl);
void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl);

/* 执行 */
void zhttpd_run(zhttpd_t *httpd);

/* 设置handler */
void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_HEAD_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_OPTIONS_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_DELETE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_TRACE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_PATCH_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* 设置/获取上下文 */
void zhttpd_set_context(zhttpd_t *httpd, const void *context);
void *zhttpd_get_context(zhttpd_t *httpd);

/* 停止httpd */
void zhttpd_set_stop(zhttpd_t *httpd);

/* 设置keep-alive超时时间, timeout: 秒 */
void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout);

/* 通用超时等待可读, 单位秒, timeout<0: 表示无限长 */
void zhttpd_set_read_wait_timeout(zhttpd_t *httpd, int read_wait_timeout);

/* 通用超时等待可写, 单位秒 */
void zhttpd_set_write_wait_timeout(zhttpd_t *httpd, int write_wait_timeout);

/* 设置post方式最大长度 */
void zhttpd_set_max_length_for_post(zhttpd_t *httpd, int max_length);

/* 设置post方式临时目录 */
void zhttpd_set_tmp_path_for_post(zhttpd_t *httpd, const char *tmp_path);

/* 设置支持form_data协议, 默认不支持 */
void zhttpd_enable_form_data(zhttpd_t *httpd);

/* 请求的方法: GET/POST/... */
const char *zhttpd_request_get_method(zhttpd_t *httpd);

/* 请求的主机名 */
const char *zhttpd_request_get_host(zhttpd_t *httpd);

/* 请求的url路径 */
const char *zhttpd_request_get_path(zhttpd_t *httpd);

/* 请求的URI */
const char *zhttpd_request_get_uri(zhttpd_t *httpd);

/* http版本 */
const char *zhttpd_request_get_version(zhttpd_t *httpd);

/* 0: 1.0;  1: 1.1 */
int zhttpd_request_get_version_code(zhttpd_t *httpd);

/* 分析http头Content-Lengtgh字段 */
long zhttpd_request_get_content_length(zhttpd_t *httpd);

/* 请求是否是 gzip/deflate */
zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd);
zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd);

/* 全部http头 */
const zdict_t *zhttpd_request_get_headers(zhttpd_t *httpd);

/* url queries, 解析后 */
const zdict_t *zhttpd_request_get_query_vars(zhttpd_t *httpd);

/* post queries, 解析后 */
const zdict_t *zhttpd_request_get_post_vars(zhttpd_t *httpd);

/* cookies, 解析后 */
const zdict_t *zhttpd_request_get_cookies(zhttpd_t *httpd);

/* 获取全部上传文件的信息 */
const zvector_t *zhttpd_request_get_uploaded_files(zhttpd_t *httpd); /* zhttpd_uploaded_file * */

/* 快速回复 */
void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size);
void zhttpd_response_301(zhttpd_t *httpd, const char *url);
void zhttpd_response_302(zhttpd_t *httpd, const char *url);
void zhttpd_response_304(zhttpd_t *httpd, const char *etag);
void zhttpd_response_404(zhttpd_t *httpd);
void zhttpd_response_500(zhttpd_t *httpd);
void zhttpd_response_501(zhttpd_t *httpd);

/* 设置 默认handler */
void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size));
void zhttpd_set_301_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *url));
void zhttpd_set_302_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *url));
void zhttpd_set_304_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *etag));
void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_501_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* 输出 initialization; version: "http/1.0", "http/1.1", 0(采用请求值); status: 如 "200 XXX"  */
void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *version, const char *status);

/* 输出header */
void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value);

/* 输出header, value为date类型 */
void zhttpd_response_header_date(zhttpd_t *httpd, const char *name, long value);

/* 输出Content-Type; charset: 为空则取 UTF-8 */
void zhttpd_response_header_content_type(zhttpd_t *httpd, const char *value, const char *charset);

/* 输出http体长度 */
void zhttpd_response_header_content_length(zhttpd_t *httpd, long length);

/* 设置cookie */
void zhttpd_response_header_set_cookie(zhttpd_t *httpd, const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly);

/* 删除cookie */
void zhttpd_response_header_unset_cookie(zhttpd_t *httpd, const char *name);

/* http头输出完毕 */
void zhttpd_response_header_over(zhttpd_t *httpd);

/* 写http体 */
void zhttpd_response_write(zhttpd_t *httpd, const void *data, int len);
void zhttpd_response_puts(zhttpd_t *httpd, const char *data);
void zhttpd_response_append(zhttpd_t *httpd, const zbuf_t *bf);
void zhttpd_response_printf_1024(zhttpd_t *httpd, const char *format, ...);
void zhttpd_response_flush(zhttpd_t *httpd);

/* 获取http的stream */
zstream_t *zhttpd_get_stream(zhttpd_t *httpd);

/* 上传文件路径名 */
const char *zhttpd_uploaded_file_get_pathname(zhttpd_uploaded_file_t *fo);

/* 上传文件名称 */
const char *zhttpd_uploaded_file_get_name(zhttpd_uploaded_file_t *fo);

/* 上传文件大小; -1: 错 */
int zhttpd_uploaded_file_get_size(zhttpd_uploaded_file_t *fo);

/* 保存上传文件到指定文件路径 */
int zhttpd_uploaded_file_save_to(zhttpd_uploaded_file_t *fo, const char *pathname);

/* 获取上传文件数据 */
int zhttpd_uploaded_file_get_data(zhttpd_uploaded_file_t *fo, zbuf_t *data);

/* 输出一个文件 */
void zhttpd_response_file(zhttpd_t *httpd, const char *pathname, const char *content_type, int max_age);

/* 输出一个文件, 带gzip */
void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *gzip_pathname, const char *content_type, int max_age);

/* 输出一个文件 */
void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *pathname, const char *gzip_pathname, const char *content_type, int max_age);

/* 输出一个(文件)data */
void zhttpd_response_file_data(zhttpd_t *httpd, const void *data, long size, const char *content_type, int max_age, long mtime, const char *etag, zbool_t is_gzip);

/* 日志 */
#define zhttpd_show_log(httpd, fmt, args...) { zinfo("%s " fmt, zhttpd_get_prefix_log_msg(httpd), ##args) }
extern const char *(*zhttpd_get_prefix_log_msg)(zhttpd_t *httpd);
const char *zhttpd_get_prefix_log_msg_default(zhttpd_t *httpd);
zbuf_t *zhttpd_get_prefix_log_msg_buf(zhttpd_t *httpd);

/* sqlite3 ################################################## */
/* zsqlite3_proxd based on zaio_server */
extern void (*zsqlite3_proxy_server_before_service)(void);
extern void (*zsqlite3_proxy_server_before_softstop)(void);

/* 如果注册且返回1: 表明调用者成功注册了这个服务, 否则启用 sqlite3 服务*/
extern zbool_t (*zsqlite3_proxy_server_service_register) (const char *service, int fd, int fd_type);
int zsqlite3_proxy_server_main(int argc, char **argv);

/* client */
/* 因为一般用于本地, 所以没有超时设置 */
zbuf_t *zsqlite3_escape_append(zbuf_t *sql, const void *data, int len);
zsqlite3_proxy_client_t *zsqlite3_proxy_client_connect(const char *destination);
void zsqlite3_proxy_client_close(zsqlite3_proxy_client_t *client);
void zsqlite3_proxy_client_set_auto_reconnect(zsqlite3_proxy_client_t *client, zbool_t auto_reconnect);
int zsqlite3_proxy_client_log(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_exec(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_query(zsqlite3_proxy_client_t *client, const char *sql, int len);
int zsqlite3_proxy_client_get_row(zsqlite3_proxy_client_t *client, zbuf_t ***rows);
int zsqlite3_proxy_client_get_column(zsqlite3_proxy_client_t *client);
const char *zsqlite3_proxy_client_get_error_msg(zsqlite3_proxy_client_t *client);
void zsqlite3_proxy_client_set_error_log(zsqlite3_proxy_client_t *client, int flag);
/* utils */
int zsqlite3_proxy_client_quick_fetch_one_row(zsqlite3_proxy_client_t *db, const char *sql, int len, zbuf_t ***result_row);

/* cdb, src/cdb/ ###################################################### */
/* 一种新的静态db, 不支持修改, 例子见 sample/cdb/ */

#define zvar_cdb_code_version "0001"

/* 打开句柄 */
zcdb_t *zcdb_open(const char *cdb_pathname);
zcdb_t *zcdb_open_from_data(const void *data);
void zcdb_close(zcdb_t *cdb);

/* cdb成员个数 */
int zcdb_get_count(zcdb_t *cdb);

/* -1:出错, 0: 没找到, 1: 找到, 线程安全 */
int zcdb_find(zcdb_t *cdb, const void *key, int klen, void **val, int *vlen);

/* 创建遍历器 */
zcdb_walker_t *zcdb_walker_create(zcdb_t *cdb);
void zcdb_walker_free(zcdb_walker_t *walker);

/* -1:出错, 0: 没找到, 1: 找到, 非线程安全 */
int zcdb_walker_walk(zcdb_walker_t *walker, void **key, int *klen, void **val, int *vlen);

/* 重置 */
void zcdb_walker_reset(zcdb_walker_t *walker);

/* 创建生成器 */
zcdb_builder_t *zcdb_builder_create();
void zcdb_builder_free(zcdb_builder_t *builder);

/* 更新值 */
void zcdb_builder_update(zcdb_builder_t *builder, const void *key, int klen, const void *val, int vlen);

/* 生成zcddb文件, 保存(覆盖写)在 dest_db_pathname */
int zcdb_builder_build(zcdb_builder_t *builder, const char *dest_db_pathname);

zbool_t zcdb_builder_compile(zcdb_builder_t *builder);

const void *zcdb_builder_get_compiled_data(zcdb_builder_t *builder);
int zcdb_builder_get_compiled_len(zcdb_builder_t *builder);

/* msearch ###################################################### */
/* 多字符串匹配 */
extern zbool_t zvar_msearch_error_msg;

zmsearch_t *zmsearch_create();
void zmsearch_free(zmsearch_t *ms);
void zmsearch_add_token(zmsearch_t *ms, const char *word, int len);
void zmsearch_add_over(zmsearch_t *ms);
int zmsearch_match(zmsearch_t *ms, const char *str, int len, const char **matched_ptr, int *matched_len);
int zmsearch_add_token_from_pathname(zmsearch_t *ms, const char *pathname);

zmsearch_t *zmsearch_create_from_data(const void *data);
zmsearch_t *zmsearch_create_from_pathname(const char *pathname);

const void *zmsearch_get_compiled_data(zmsearch_t *ms);
int zmsearch_get_compiled_len(zmsearch_t *ms);
int zmsearch_build(zmsearch_t *ms, const char *dest_db_pathname);

/* 创建遍历器 */
zmsearch_walker_t *zmsearch_walker_create(zmsearch_t *ms);
void zmsearch_walker_free(zmsearch_walker_t *walker);

/* -1:出错, 0: 没找到, 1: 找到, 非线程安全 */
int zmsearch_walker_walk(zmsearch_walker_t *walker, const char **token, int *tlen);

/* 重置 */
void zmsearch_walker_reset(zmsearch_walker_t *walker);

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

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

/* ################################################################## */
/* CPP */
/* ################################################################## */
#ifdef  __cplusplus
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#pragma pack(push, 4)
namespace zcc
{

/* auto_delete ###################################################### */
template <class T>
class auto_delete {
public:
    zinline auto_delete(T *obj) { obj_ = obj; }
    zinline ~auto_delete() { if (obj_) { delete obj_; } }
    T *obj_;
};

/* gloabal init ##################################################### */
class global_init {
public:
    zinline global_init(void (*_init_fn)()) { _init_fn(); }
    zinline global_init(void (*_init_fn)(void *ctx), void *ctx) { _init_fn(ctx); }
    zinline global_init(void (*_init_fn)(int i), int i) { _init_fn(i); }
    zinline ~global_init() { }
public:
    int unused;
};

/* std utils ######################################################## */
std::string &vsprintf_1024(std::string &str, const char *format, va_list ap);
std::string &sprintf_1024(std::string &str, const char *format, ...);
std::string &tolower(std::string &str);
std::string &toupper(std::string &str);
std::string &trim_right(std::string &str, const char *delims=0);
std::vector<std::string> split(const char *s, const char *delims = 0);
inline std::vector<std::string> split(const std::string &s, const char *delims=0) {
    return split(s.c_str(), delims);
}

class string: public std::string
{
public:
    typedef std::string stdstring;
    using stdstring::stdstring;
    using stdstring::append;
    string &printf_1024(const char *format, ...);
    zinline string &append(int i) {return printf_1024("%d", i);}
    zinline string &append(unsigned int i) {return printf_1024("%u", i);}
    zinline string &append(long int i) {return printf_1024("%ld", i);}
    zinline string &append(unsigned long i) {return printf_1024("%lu", i);}
    zinline string &append(double i) {return printf_1024("%f", i);}
    zinline string &append(float i) {return printf_1024("%f", i);}
    zinline string &clear() {std::string::clear(); return *this;}
    zinline string &push_back(char c) {std::string::push_back(c); return *this;}
    zinline string &tolower() {zcc::tolower(*this); return *this;}
    zinline string &toupper() {zcc::toupper(*this);;return *this;}
    zinline string &trim_right(const char *delims=0) {zcc::trim_right(*this, delims); return *this;}
    std::vector<std::string> split(const char *delims=0) {return zcc::split(*this, delims);}
};

/* encode/decode, ################################################## */
void base64_encode(const void *src, int src_size, std::string &str, bool mime_flag = false);
void base64_decode(const void *src, int src_size, std::string &str);
void qp_encode_2045(const void *src, int src_size, std::string &result, bool mime_flag = false);
void qp_encode_2047(const void *src, int src_size, std::string &result);
void qp_decode_2045(const void *src, int src_size, std::string &str);
void qp_decode_2047(const void *src, int src_size, std::string &str);
void hex_encode(const void *src, int src_size, std::string &str);
void hex_decode(const void *src, int src_size, std::string &str);
void url_hex_decode(const void *src, int src_size, std::string &str);
void url_hex_encode(const void *src, int src_size, std::string &str, bool strict_flag = false);
void xml_unescape_string(std::string &content, const char *data, int len);
void uudecode(const void *src, int src_size, std::string &str);

/* config ########################################################## */
class config {
public:
    config();
    config(zconfig_t *cf);
    ~config();
    config &update(const char *key, const char *val);
    config &update(const char *key, std::string &val);
    config &remove(const char *key);
    bool load_from_pathname(const char *pathname);
    config &load_annother(zconfig_t *another);
    config &load_annother(config &another);
    config &debug_show();
    zinline zconfig_t *c_config() { return cf_; }
    bool get_bool(const char *key, bool default_val = false);
private:
    zconfig_t *cf_;
    bool new_flag_;
};
extern config var_default_config;

/* json ############################################################ */
#ifndef ___ZC_LIB_INCLUDE_JSON___
#define ___ZC_LIB_INCLUDE_JSON___
#pragma pack(push, 1)
class json;
const unsigned char json_type_null    = 0;
const unsigned char json_type_string  = 1;
const unsigned char json_type_long    = 2;
const unsigned char json_type_double  = 3;
const unsigned char json_type_object  = 4;
const unsigned char json_type_array   = 5;
const unsigned char json_type_bool    = 6;
const unsigned char json_type_unknown = 7;
class json
{
public:
    json();
    json(const std::string &val);
    json(const char *val, int len = -1);
    json(long val);
    json(double val);
    json(bool val);
    json(const unsigned char type);
    ~json();

    /* 深度递归复制 */
    json *deep_copy();

    /* 首先重置本json为 null, 然后从文件加载json */
    bool load_from_pathname(const char *pathname);

    /* 首先重置本json为 null, 然后从jstr反序列化为json */
    bool unserialize(const char *jstr, int jsize = -1);
    zinline bool unserialize(const std::string &jstr) { return unserialize(jstr.c_str(), (int)(jstr.size())); }

    /* 序列化 */
    json *serialize(std::string &result, bool strict_flag = false);

    /* 类型 */
    zinline int get_type()   { return type_; }
    zinline bool is_string() { return type_==json_type_string; }
    zinline bool is_long()   { return type_==json_type_long; }
    zinline bool is_double() { return type_==json_type_double; }
    zinline bool is_object() { return type_==json_type_object; }
    zinline bool is_array()  { return type_==json_type_array; }
    zinline bool is_bool()   { return type_==json_type_bool; }
    zinline bool is_null()   { return type_==json_type_null; }

    /* 重置为 null */
    json *reset();

    /* 改变类型为 bool, 其他类似 */
    json *used_for_bool();
    json *used_for_long();
    json *used_for_double();
    json *used_for_string();
    json *used_for_array();
    json *used_for_object();

    /* 获取 string 值; 如果不是 string 类型, 则首先转换为 string 类型, 值默认为 "" */
    std::string &get_string_value();

    /* 获取 long 值; 如果不是 long 类型, 则首先转换为 long 类型, 值默认为 0 */
    long &get_long_value();

    /* 获取 double 值; 如果不是 double 类型, 则首先转换为 double 类型, 值默认为 0 */
    double &get_double_value();

    /* 获取 bool 值; 如果不是 bool 类型, 则首先转换为 bool 类型, 值默认为 false */
    bool &get_bool_value();

    /* 如果想操作下面的vector/map, 记得给新 json 设置 set_parent */

    /* 获取 array 值; 如果不是 array 类型, 则首先转换为 array 类型, 默认为 [] */
    const std::vector<json *> &get_array_value();

    /* 获取 object 值; 如果不是 object 类型, 则首先转换为 object 类型, 默认为 {} */
    const std::map<std::string, json *> &get_object_value();

    /* 设置值 */
    json *set_string_value(const char *val, int len = -1);
    zinline json *set_string_value(const std::string &val) { get_string_value() = val; return this; }
    zinline json *set_long_value(long val) { get_long_value() = val; return this; }
    zinline json *set_double_value(double val) { get_double_value() = val; return this; }
    zinline json *set_bool_value(bool val) { get_bool_value() = val; return this; }

    /* 获取下标为 idx 的 json 节点, 如果当前节点不是 array 类型, 则先转为 array 类型 */
    json *array_get(int idx);

    /* 获取 array 节点的个数, 如上 */
    int array_size();

    /* 在 idx 前, 插入 j */
    json *array_insert(int idx, json *j, bool return_child = false);

    /* 在最前面插入 j */
    zinline json *array_unshift(json *j, bool return_child = false) {
        return array_insert(0, j, return_child);
    }

    /* 在尾部追加节点 j */
    zinline json *array_add(json *j, bool return_child = false) {
        return array_insert(-1, j, return_child);
    }
    zinline json *array_push(json *j, bool return_child = false) { return array_add(j, return_child); }
    zinline json *array_add(const char *val, int len) { return array_add(new json(val, len)); }
    zinline json *array_push(const char *val, int len) { return array_add(new json(val, len)); }

    /* 设置下表为 idx 的成员 j, 如果键idx存在则, 则把idx对应的json赋给 *old, 如果old为0, 则销毁 */
    json *array_update(int idx, json *j, json **old, bool return_child = false);
    zinline json *array_update(int idx, json *j, bool return_child = false) {
        return array_update(idx, j, 0, return_child);
    }
    zinline json *array_update(size_t idx, const char *val, int len, bool return_child = false) {
        return array_update(idx, new json(val, len), return_child);
    }
    zinline json *array_update(size_t idx, const char *val, int len, json **old, bool return_child = false) {
        return array_update(idx, new json(val, len), old, return_child);
    }


    /* 删除下标为 idx的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool array_delete(int idx, json **old);

    /* 弹出第一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    zinline bool array_shift(json **old) { return array_delete(0, old); }

    /* 弹出最后一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    zinline bool array_pop(json **old) { return array_delete(-1, old); }

    /* 获取键为 key 的字json节点, 如果当前节点不是 object 类型, 则先转为 object 类型 */
    json *object_get(const char *key);

    /* 获取 object 节点的个数, 如上 */
    int object_size();

    /* 更新键为key的节点, 旧值赋值给 *old, 如果 old为 0, 则销毁 */
    json *object_update(const char *key, json *j, json **old, bool return_child = false);
    json *object_add(const char *key, json *j, json **old, bool return_child = false) {
        return object_update(key, j, old, return_child);
    }

    zinline json *object_update(const char *key, json *j, bool return_child = false) {
        return object_update(key, j, 0, return_child);
    }
    zinline json *object_add(const char *key, json *j, bool return_child = false) {
        return object_update(key, j, 0, return_child);
    }
    zinline json *object_update(const char *key, const char *val, int len, json **old, bool return_child = false) {
        return object_update(key, new json(val, len), old, return_child);
    }
    zinline json *object_add(const char *key, const char *val, int len, json **old, bool return_child = false) {
        return object_update(key, new json(val, len), old, return_child);
    }
    zinline json *object_update(const char *key, const char *val, int len, bool return_child = false) {
        return object_update(key, new json(val, len), return_child);
    }
    zinline json *object_add(const char *key, const char *val, int len, bool return_child = false) {
        return object_update(key, new json(val, len), return_child);
    }

    /* 删除键为key的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool object_delete(const char *key, json **old);

#define ___zcc_json_update(TTT) \
    json *array_insert(int idx, TTT val, bool return_child = false) { \
        return array_insert(idx, new json(val), return_child); \
    } \
    zinline json *array_unshift(TTT val, bool return_child = false) { \
        return array_insert(0, new json(val), return_child); \
    } \
    zinline json *array_add(TTT val, bool return_child = false) { \
        return array_insert(-1, new json(val), return_child); \
    } \
    zinline json *array_push(TTT val, bool return_child = false) { \
        return array_insert(-1, new json(val), return_child); \
    } \
    zinline json *array_update(int idx, TTT val, bool return_child = false) { \
        return array_update(idx, new json(val), return_child); \
    } \
    zinline json *array_update(int idx, TTT val, json **old, bool return_child = false) { \
        return array_update(idx, new json(val), old, return_child); \
    } \
    zinline json *object_update(const char *key, TTT val, bool return_child = false) { \
        return object_update(key, new json(val), return_child); \
    } \
    zinline json *object_update(const char *key, TTT val, json **old, bool return_child = false) { \
        return object_update(key, new json(val), old, return_child); \
    } \
    zinline json *object_add(const char *key, TTT val, bool return_child = false) { \
        return object_update(key, new json(val), return_child); \
    } \
    zinline json *object_add(const char *key, TTT val, json **old, bool return_child = false) { \
        return object_update(key, new json(val), old, return_child); \
    }

    /* 这几组方法类似 array_update, object_update, 只不过参数不同 */
    ___zcc_json_update(const std::string &);
    ___zcc_json_update(const char *);
    ___zcc_json_update(long);
    ___zcc_json_update(double);
    ___zcc_json_update(bool);
#undef ___zcc_json_update

    /* @get_by_path 得到路径path对应的j son, 并返回, 如:
     * 已知 json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
     * get_by_path("group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
    json *get_by_path(const char *path);

    /* 如: get_by_path_vec("group", "linux", "2", "me", 0); */
    json *get_by_path_vec(const char *path0, ...);

    /* 获取 路径 pathname 对应的节点 的 值 */
    bool get_value_by_path(const char *pathname, std::string &value);
    bool get_value_by_path(const char *pathname, long *value);

    /* */
    zinline json *get_parent() { return parent_; }
    json *get_top();
    json *set_parent(json *js);
    json *debug_show();
private:
    unsigned char type_;
    union {
        bool b;
        long l;
        double d;
        char s[sizeof(std::string)];
        std::vector<json *> *v;
        std::map<std::string, json *> *m;
    } val_;
    json *parent_;
};
#pragma pack(pop)
#endif /*___ZC_LIB_INCLUDE_JSON___ */

/* stream ########################################################## */
class basic_stream {
public:
    struct basic_stream_worker {
        short int read_buf_p1;
        short int read_buf_p2;
        short int write_buf_len;
        unsigned short int error:1;
        unsigned short int eof:1;
        unsigned char read_buf[zvar_stream_rbuf_size];
        unsigned char write_buf[zvar_stream_wbuf_size];
    };
    basic_stream();
    virtual ~basic_stream();
    basic_stream &reset();
public:
    zinline bool is_error() { return (basic_worker_->error?true:false); }
    zinline bool is_eof() { return (basic_worker_->eof?true:false); }
    zinline basic_stream &set_error(bool tf = true) { basic_worker_->error = 1; return *this; }
    zinline basic_stream &set_eof(bool tf = true) { basic_worker_->eof = 1; return *this; }
    zinline bool is_exception() {
        return (((basic_worker_->eof)||(basic_worker_->error))?true:false);
    }
    zinline int get_read_cache_len() {
        return (basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1);
    }
    zinline char * get_read_cache() { return (char *)(basic_worker_->read_buf); }
    zinline int get_write_cache_len() { return (basic_worker_->write_buf_len); }
    zinline char * get_write_cache() {
        basic_worker_->write_buf[basic_worker_->write_buf_len] = 0;
        return (char *)(basic_worker_->write_buf);
    }
    zinline int getc() {
        return ((basic_worker_->read_buf_p1<basic_worker_->read_buf_p2)?((int)(basic_worker_->read_buf[basic_worker_->read_buf_p1++])):(getc_do()));
    }
    basic_stream &ungetc();
    int read(zbuf_t *bf, int max_len);
    int read(void *mem, int max_len);
    int read(std::string &str, int max_len);
    int readn(zbuf_t *bf, int strict_len);
    int readn(void *mem, int strict_len);
    int readn(std::string &str, int strict_len);
    int read_delimiter(zbuf_t *bf, int delimiter, int max_len);
    int read_delimiter(void *mem, int delimiter, int max_len);
    int read_delimiter(std::string &str, int delimiter, int max_len);
    zinline int gets(zbuf_t *bf, int max_len) { return read_delimiter(bf, '\n', max_len ); }
    zinline int gets(void *mem, int max_len) { return read_delimiter(mem, '\n', max_len ); }
    zinline int gets(std::string &str, int max_len) { return read_delimiter(str, '\n', max_len ); }
    zinline int putc(int c) {
        return ((basic_worker_->write_buf_len<zvar_stream_wbuf_size)?(basic_worker_->write_buf[basic_worker_->write_buf_len++]=(int)(c),(int)(c)):(putc_do(c)));
    }
    int flush();
    int write(const void *buf, int len);
    int puts(const char *s) { return write(s, -1); }
    zinline int append(zbuf_t *bf) { return write(zbuf_data(bf), zbuf_len(bf)); }
    zinline int append(std::string &str) { return write(str.c_str(), (int)str.size()); }
    int printf_1024(const char *format, ...);
    int get_cint();
    int write_cint(int len);
    int write_cint_and_int(int i);
    int write_cint_and_long(long l);
    int write_cint_and_data(const void *buf, int len);
    zinline int write_cint_and_dict(zdict_t * zd);
    zinline int write_cint_and_pp(const char **pp, int size);
protected:
    virtual int engine_read(void *buf, int len) = 0;
    virtual int engine_write(const void *buf, int len) = 0;
private:
    int getc_do();
    int putc_do(int ch);
protected:
    basic_stream_worker *basic_worker_;
};

class iostream: public basic_stream {
public:
    iostream();
    iostream &open_fd(int fd);
    bool open_file(const char *pathname, const char *mode);
    bool open_destination(const char *destination, int timeout);
    zinline bool connect(const char *destination, int timeout) {
        return open_destination(destination, timeout);
    }
    virtual ~iostream();
public:
    zinline int get_fd() { return fd_; }
    zinline int get_read_wait_timeout() { return read_wait_timeout_; }
    zinline int get_write_wait_timeout() { return write_wait_timeout_; }
    iostream &set_read_wait_timeout(int read_wait_timeout);
    iostream &set_write_wait_timeout(int write_wait_timeout);
    virtual int timed_read_wait(int read_wait_timeout);
    virtual int timed_write_wait(int write_wait_timeout);
    virtual int close(bool close_fd_or_release_ssl = true);
protected:
    virtual int engine_read(void *buf, int len);
    virtual int engine_write(const void *buf, int len);
protected:
    int fd_;
    int read_wait_timeout_;
    int write_wait_timeout_;
};

class ssl_iostream: public iostream {
public:
    ssl_iostream();
    virtual ~ssl_iostream();
    ssl_iostream &open_ssl(SSL *ssl);
public:
    zinline SSL *get_ssl() { return ssl_; }
    int tls_connect(SSL_CTX *ctx);
    int tls_accept(SSL_CTX *ctx);
    virtual int timed_read_wait(int read_wait_timeout);
    virtual int timed_write_wait(int write_wait_timeout);
    virtual int close(bool close_fd_or_release_ssl = true);
protected:
    virtual int engine_read(void *buf, int len);
    virtual int engine_write(const void *buf, int len);
protected:
    SSL *ssl_;
};

/* charset ######################################################### */
inline char *charset_detect(const char **charset_list, const char *data, int size, char *charset_result) {
    return zcharset_detect(charset_list, data, size, charset_result);
}
const char *charset_detect(const char **charset_list, const char *data, int size, std::string &charset_result);
inline char *charset_detect_cjk(const char *data, int size, char *charset_result) {
    return zcharset_detect_cjk(data, size, charset_result);
}
const char *charset_detect_cjk(const char *data, int size, std::string charset_result);
extern int (*charset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
int charset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
void charset_convert_use_uconv();
int charset_uconv(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
void charset_convert_to_utf8(const char *from_charset, const char *data, int size, std::string &result);

/* event, src/event/ ################################################### */
/* 基于epoll的高并发io模型, 包括 事件, 异步io, 定时器, io映射. 例子见 cpp_sample/event/  */

class aio_base;
class aio {
public:
    aio();
    aio(int fd, aio_base *aiobase = 0);
    ~aio();
    void rebind_fd(int fd, aio_base *aiobase = 0);
    void close(bool close_fd_and_release_ssl = true);
    /* 重新绑定 aio_base */
    void rebind_aio_base(aio_base *aiobase = 0);
    /* 设置可读超时 */
    zinline void set_read_wait_timeout(int read_wait_timeout) { zaio_set_read_wait_timeout(engine_, read_wait_timeout); }
    /* 设置可写超时 */
    zinline void set_write_wait_timeout(int write_wait_timeout) { zaio_set_write_wait_timeout(engine_, write_wait_timeout); }
    /* 停止 aio, 只能在所属 aio_base 运行的线程执行 */
    zinline void disable() { zaio_disable(engine_); }
    /* 获取结果, -2:超时(且没有任何数据), <0: 错, >0: 写成功,或可读的字节数 */
    zinline int get_result() { return zaio_get_result(engine_); }
    /* 获取 fd */
    zinline int get_fd() { return zaio_get_fd(engine_); }
    /* 获取 SSL 句柄 */
    zinline int get_read_cache_size() { return zaio_get_read_cache_size(engine_); }
    zinline void get_read_cache(zbuf_t *bf, int strict_len) { zaio_get_read_cache(engine_, bf, strict_len); }
    zinline void get_read_cache(char *buf, int strict_len) { zaio_get_read_cache_to_buf(engine_, buf, strict_len); }
    void get_read_cache(std::string &bf, int strict_len);
    /* 获取缓存数据的长度 */
    zinline int get_write_cache_size() { return zaio_get_write_cache_size(engine_); }
    zinline void get_write_cache(zbuf_t *bf, int strict_len) { zaio_get_write_cache(engine_, bf, strict_len); }
    zinline void get_write_cache(char *buf, int strict_len) { zaio_get_write_cache_to_buf(engine_, buf, strict_len); }
    void get_write_cache(std::string &bf, int strict_len);
    /* 如果可读(或出错)则回调执行函数 callback */
    void readable(std::function<void()> callback);
    /* 如果可写(或出错)则回调执行函数 callback */
    void writeable(std::function<void()> callback);
    /* 请求读, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
    void read(int max_len, std::function<void()> callback);
    /* 请求读, 严格读取strict_len个字节, 成功/失败/超时后回调执行callback */
    void readn(int strict_len, std::function<void()> callback);
    /* */
    /* 请求读, 读到delimiter为止, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
    void read_delimiter(int delimiter, int max_len, std::function<void()> callback);
    /* 如上, 读行 */
    zinline void gets(int max_len, std::function<void()> callback) { read_delimiter('\n', max_len, callback); }
    /* 向缓存写数据, (fmt, ...)不能超过1024个字节 */
    void cache_printf_1024(const char *fmt, ...);
    /* 向缓存写数据 */
    zinline void cache_puts(const char *s) { zaio_cache_puts(engine_, s); }
    /* 向缓存写数据 */
    zinline void cache_write(const void *buf, int len) { zaio_cache_write(engine_, buf, len); }
    zinline void cache_write(const std::string &bf) { zaio_cache_write(engine_, bf.c_str(), (int)(bf.size())); }
    zinline void cache_append(const std::string &bf) { zaio_cache_write(engine_, bf.c_str(), (int)(bf.size())); }
    zinline void cache_write(const zbuf_t *bf) { zaio_cache_write(engine_, zbuf_data(bf), zbuf_len(bf)); }
    zinline void cache_append(const zbuf_t *bf) { zaio_cache_write(engine_, zbuf_data(bf), zbuf_len(bf)); }
    /* */
    /* 向缓存写数据, 不复制buf */
    zinline void cache_write_direct(const void *buf, int len) { zaio_cache_write_direct(engine_, buf, len); }
    /* 请求写, 成功/失败/超时后回调执行callback */
    void cache_flush(std::function<void()> callback);
    /* */
    zinline int get_cache_size() { return zaio_get_cache_size(engine_); }
    /* 请求sleep, sleep秒后回调执行callback */
    void sleep(std::function<void()> callback, int timeout);
    /* 获取 aio_base */
    aio_base *get_aio_base();
protected:
    zaio_t *engine_;
};

class ssl_aio :public aio {
public:
    ssl_aio(SSL *ssl, aio_base *aiobase = 0);
    void rebind_SLL(SSL *ssl, aio_base *aiobase = 0);
    /* 获取 SSL 句柄 */
    virtual SSL *get_ssl();
    /* 发起tls连接, 成功/失败/超时后回调执行callback */
    void tls_connect(SSL_CTX * ctx, std::function<void()> callback);
    /* 发起tls接受, 成功/失败/超时后回调执行callback */
    void tls_accept(SSL_CTX * ctx, std::function<void()> callback);
};

/* event/epoll 运行框架 */
/* 默认aio_base */
extern aio_base *var_default_aio_base;

class aio_base {
public:
    aio_base();
    aio_base(zaio_base_t *ab);
    ~aio_base();
    /* 设置 aio_base 每次epoll循环需要执行的函数 */
    void set_loop_fn(std::function<void()> callback);
    /* 运行 aio_base */
    void run();
    /* 通知 aio_base 停止, 既 zaio_base_run 返回 */
    void stop_notify();
    /* 通知 aio_base, 手动打断 epoll_wait */
    void touch();
    /* sleep秒后回调执行callback, 只执行一次 callback(ctx) */
    void timer(std::function<void()> callback, int timeout);
protected:
    zaio_base_t *engine_;
};
/* 获取当前线程运行的 aio_base */
aio_base *aio_base_get_current_pthread_aio_base();


/* master, cpp_src/master/ ############################################# */
/* master/server 进程服务管理框架, 例子见 cpp_sample/master/ */
class aio_server {
public:
    aio_server();
    virtual ~aio_server();
    void general_service_register(aio_base *ab, int fd, int fd_type, void (*callback) (int));
    zinline void general_service_register(int fd, int fd_type, void (*callback) (int)) {
        general_service_register(0, fd, fd_type, callback);
    }
    virtual void service_register(const char *service, int fd, int fd_type) = 0;
    virtual void before_service(void);
    virtual void before_softstop(void);
    virtual void stop_notify(int stop_after_second = 0);
    virtual void detach_from_master(void);
    virtual int run(int argc, char **argv);
public:
    void *unused_;
};
extern aio_server *var_default_aio_server;

class simple_line_request: public aio {
public:
    simple_line_request();
    ~simple_line_request();
    void clear();
    void cache_flush_and_request();
    void cache_flush_and_stop();
    void request();
    void stop();
    std::string *cmd_name_;
    json json_;
    int extra_data_len_;
    void *extra_data_ptr_;
};

class simple_line_request_aio_server: public aio_server {
public:
    static int var_max_use;
    static int var_concurrency;
    static int var_request_timeout;
    static int var_line_max_len;
    static int var_extra_data_max_len;
public:
    simple_line_request_aio_server();
    ~simple_line_request_aio_server();
    virtual void service_register(const char *service, int fd, int fd_type);
    virtual void before_service(void);
    virtual void before_softstop(void);
    virtual int  run(int argc, char **argv);
    virtual void handler(simple_line_request *slr) = 0;
    virtual void every_request();
    virtual void enter_job(void (*fn)(void *ctx), void *ctx);
    virtual void enter_timer(void (*fn)(void *ctx), void *ctx, int timeout);
};

} /* namespace zcc */
#pragma pack(pop)
#endif /* __cplusplus */

#ifdef ___ZC_DEV_MODE___
#include "cpp_src/cpp_dev.h"
#endif
#endif /*___ZC_LIB_INCLUDE___ */

