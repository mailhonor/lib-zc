#ifndef ___ZC_INCLUDE__
#define ___ZC_INCLUDE__
#define _GNU_SOURCE
#define _GNU_SOURCE

#include <netdb.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define z_string_empty			""
#define ZTRUE 	1
#define ZFALSE	0
#define ZEMPTY(str)			(!(str)||!(*(str)))
#define ZCHAR_PTR_TO_INT(ptr)		((int) (long) (ptr))
#define ZINT_TO_CHAR_PTR(val)		((char *) (long) (val))
#define ZVOID_PTR_TO_INT(ptr)		((int) (long) (ptr))
#define ZINT_TO_VOID_PTR(val)		((void *) (long) (val))
#define ZCONTAINER_OF(ptr,app_type,member)  		\
	((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))

/* RETURN CODE */
#define Z_OK			(-0)
#define Z_ERR			(-1)
#define Z_ERROR			(-1)
#define Z_TIMEOUT		(-2)
#define Z_EXIST			(-3)
#define Z_NONE			(-4)
#define Z_CONFIG		(-5)
#define Z_SOFT			(-6)
#define Z_EOF			(-7)
#define Z_NONE_TYPE		(-8)
#define Z_NONEEXISTS		(-9)

/* TIME */
static inline long zmtime_set_timeout(int timeout)
{
	long r;
	struct timeval tv;

	gettimeofday(&tv, 0);
	r = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;

	return r;
}

static inline int zmtime_left(long timeout)
{
	long r;
	struct timeval tv;

	gettimeofday(&tv, 0);
	r = timeout - (tv.tv_sec * 1000 + tv.tv_usec / 1000);

	return r;
}

#define ZTIME_SUB(x, y, z)				\
	do { 						\
		(x).tv_sec = (y).tv_sec - (z).tv_sec; 	\
		(x).tv_usec = (y).tv_usec - (z).tv_usec;\
		while ((x).tv_usec < 0) { 		\
			(x).tv_usec += 1000000;		\
			(x).tv_sec -= 1;		\
		}					\
		while ((x).tv_usec >= 1000000) {	\
			(x).tv_usec -= 1000000;		\
			(x).tv_sec += 1;		\
		}					\
	} while (0)

typedef void (*ZFN_VOID) (void);
typedef void (*ZFN_VV) (void *);
typedef void *(*ZFN_VVV) (void *);
typedef void (*ZFN_STRUCTURE_FREE_2) (void *, void *);

/* TYPEDEF STRUCT*/
typedef struct ZSDATA ZSDATA;
typedef struct ZSDLIST ZSDLIST;
typedef struct ZBUF ZBUF;
typedef struct ZARGV ZARGV;
typedef struct ZARRAY ZARRAY;
typedef struct ZRING ZRING;
typedef struct ZCHAIN ZCHAIN;
typedef struct ZCHAIN_NODE ZCHAIN_NODE;
typedef struct ZLINK ZLINK;
typedef struct ZLINK_NODE ZLINK_NODE;
typedef struct ZHTABLE ZHTABLE;
typedef struct ZHTABLE_NODE ZHTABLE_NODE;
typedef struct ZRBTREE ZRBTREE;
typedef struct ZRBTREE_NODE ZRBTREE_NODE;
typedef struct ZMPOOL ZMPOOL;
typedef struct ZDICT ZDICT;
typedef struct ZDICT_NODE ZDICT_NODE;
typedef struct ZIDICT ZIDICT;
typedef struct ZIDICT_NODE ZIDICT_NODE;
typedef struct ZSIO ZSIO;
typedef struct ZMAP ZMAP;
typedef struct ZEVENT_BASE ZEVENT_BASE;
typedef struct ZEVENT ZEVENT;
typedef struct ZEVENT_BASE ZEVENT_BASE;
typedef struct ZAIO ZAIO;
typedef struct ZIOPIPE ZIOPIPE;
typedef struct ZIOPIPE_BASE ZIOPIPE_BASE;
typedef struct ZIOPIPE_PART ZIOPIPE_PART;
typedef struct ZTIMER ZTIMER;
typedef struct ZQUEUE ZQUEUE;
typedef struct ZALARM ZALARM;
typedef struct ZSSL_CTX ZSSL_CTX;
typedef struct ZSSL ZSSL;
typedef struct ZSKVLIST ZSKVLIST;
typedef struct ZPLAINLIST ZPLAINLIST;
typedef struct ZKV ZKV;
typedef struct ZCMDER ZCMDER;

/* VARS */
extern int zvar_pthread_safe;
extern char *zvar_program_name;
extern int zvar_ipc_timeout;
extern int zvar_daemon_mode;
extern int zvar_max_fd_limit;
extern ZEVENT_BASE *zvar_default_event_base;

/* LOG */
typedef int (*ZLOG_HANDLER) (int level, const char *fmt, va_list ap);
extern int zvar_log_debug;
extern int zvar_log_verbose;
void zlog_set_output(ZLOG_HANDLER handler);
int zlog_fatal(const char *fmt, ...);
int zlog_error(const char *fmt, ...);
int zlog_warning(const char *fmt, ...);
int zlog_info(const char *fmt, ...);
int zlog_debug(const char *fmt, ...);
#define zlog_err		zlog_error
#define zlog_warn		zlog_warning
#ifdef ZC_DEBUG
#define ZDEBUG			if(zvar_log_debug || zvar_log_verbose)zlog_debug
#define ZVERBOSE		if(zvar_log_verbose)zlog_debug
#else
#define ZDEBUG(...)
#define ZVERBOSE(...)
#endif

/* MALLOC */
#ifdef ZC_DEBUG
#ifndef ZC_MLEAK
#define ZC_MLEAK
#endif
#endif

#ifndef ZC_MLEAK
#define zmalloc		zmalloc_malloc
#define zcalloc		zmalloc_calloc
#define zrealloc	zmalloc_realloc
#define zfree		zmalloc_free
#define zstr_strdup	zmalloc_str_strdup
#define zstr_strndup	zmalloc_str_strndup
#define zstr_memdup	zmalloc_str_memdup
#define zstr_free	zmalloc_str_free
void *zmalloc_malloc(size_t len);
void *zmalloc_calloc(size_t nmemb, size_t size);
void *zmalloc_realloc(void *ptr, size_t len);
void zmalloc_free(void *ptr);
char *zmalloc_str_strdup(const char *ptr);
char *zmalloc_str_strndup(const char *ptr, size_t n);
char *zmalloc_str_memdup(void *ptr, size_t n);
void zmalloc_str_free(void *ptr);
#endif
void zfree_structure_string(void *value, void *ctx);

#ifdef ZC_MLEAK
#define ZLEAK_INIT		zleak_init
#define zmalloc(l)		zleak_malloc((l), __LINE__, __FILE__)
#define zcalloc(n, l)		zleak_calloc(n, l, __LINE__, __FILE__)
#define zrealloc(p, l)		zleak_realloc(p, l, __LINE__, __FILE__)
#define zfree(p)		zleak_free(p, __LINE__, __FILE__)
#define zstr_strdup(p)		zleak_str_strdup(p, __LINE__, __FILE__)
#define zstr_strndup(p, l)	zleak_str_strndup(p, l, __LINE__, __FILE__)
#define zstr_memdup(p, l)	zleak_str_memdup(p, l, __LINE__, __FILE__)
#define zstr_free(p)		zleak_str_free(p, __LINE__, __FILE__)
void zleak_init(void);
void *zleak_malloc(size_t len, int line, char *fn);
void *zleak_calloc(size_t nmemb, size_t size, int line, char *fn);
void *zleak_realloc(void *ptr, size_t len, int line, char *fn);
void zleak_free(void *ptr, int line, char *fn);
char *zleak_str_strdup(const char *ptr, int line, char *fn);
char *zleak_str_strndup(const char *ptr, size_t n, int line, char *fn);
char *zleak_str_memdup(void *ptr, size_t n, int line, char *fn);
void zleak_str_free(void *ptr, int line, char *fn);
#else
#define ZLEAK_INIT()
#endif

/* CHAR */
extern const unsigned char zchar_lowercase_list[];
extern const unsigned char zchar_uppercase_list[];
#define ZCHAR_TOLOWER(c)	((int)zchar_lowercase_list[(unsigned char )(c)])
#define ZCHAR_TOUPPER(c)	((int)zchar_uppercase_list[(unsigned char )(c)])
static inline int zchar_tolower(int c)
{
	return (int)zchar_lowercase_list[(unsigned char)c];
}

static inline int zchar_toupper(int c)
{
	return (int)zchar_uppercase_list[(unsigned char)c];
}

/* STR STRING */
#if 0
char *zstr_strdup(const char *ptr);
char *zstr_strndup(const char *ptr, size_t n);
char *zstr_memdup(void *ptr, size_t n);
void zstr_free(void *ptr);
#endif

int zstr_strncpy(char *dest, char *src, size_t len);

int zstr_snprintf(char *str, size_t size, const char *format, ...);
int zstr_vsnprintf(char *str, size_t size, const char *format, va_list ap);
char *zstr_tolower(char *str);
char *zstr_toupper(char *str);
typedef struct {
	char *sstr;
	char *str;
	int len;
} ZSTRTOK;
void zstr_strtok_create(ZSTRTOK * k, char *sstr);
ZSTRTOK *zstr_strtok(ZSTRTOK * k, const char *delim);
int zstr_to_bool(char *s, int def);
int zstr_to_second(char *s);
int zstr_to_size(char *s);
int zstr_escape(char *src, char *dest, char *from, char *to, char *delete, char *terminate);
char *zstr_trim_left(char *str);
char *zstr_trim_right(char *str);
char *zstr_trim(char *str);

/* BUF */
/* general logic */
extern ZBUF *zvar_zbuf_tmp;
typedef int (*ZBUF_READY_FN) (ZBUF *);
typedef int (*ZBUF_SPACE_FN) (ZBUF *, int);
struct ZBUF {
	int flags;
	char *data;
	int size;
	int len;
	char *ptr;
	ZBUF_READY_FN ready;
	ZBUF_SPACE_FN space;
};
#define ZBUF_FLAG_RD_ERR	(1<<0)
#define ZBUF_FLAG_WR_ERR	(1<<1)
#define ZBUF_FLAG_ERR		(ZBUF_FLAG_RD_ERR | ZBUF_FLAG_WR_ERR)
#define ZBUF_FLAG_EOF		(1<<2)
#define ZBUF_FLAG_RD_TIMEOUT	(1<<3)
#define ZBUF_FLAG_WR_TIMEOUT	(1<<4)
#define ZBUF_FLAG_TIMEOUT	(ZBUF_FLAG_RD_TIMEOUT | ZBUF_FLAG_WR_TIMEOUT)
#define ZBUF_FLAG_EXCEPTION	(ZBUF_FLAG_ERR | ZBUF_FLAG_EOF | ZBUF_FLAG_TIMEOUT)

#define ZBUF_GET(zb)		((zb)->len>0?((zb)->len--,(int)*(zb)->ptr++):(zbuf_get(zb)))
#define ZBUF_PUT(zb, c)		(((zb)->len<(zb)->size)?((zb)->len++,(int)(*(zb)->ptr++=(c))):(zbuf_put((zb), (c))))
#define ZBUF_SPACE(zb, n)	(((zb)->space)?((zb)->space((zb), (n))):ZBUF_EOF)
#define ZBUF_EOF		(-1)

int zbuf_get(ZBUF *);
int zbuf_put(ZBUF *, int);
int zbuf_read(ZBUF *, void *, int);
int zbuf_write(ZBUF *, void *, int);
int zbuf_vprintf(ZBUF * zb, char *format, va_list ap);

/* buf string */
#define ZBUF_DATA(zb)		((zb)->data)
#define ZBUF_PTR(zb)		((zb)->ptr)
#define ZBUF_LEN(zb)		((zb)->len)
#define ZBUF_RESET(zb)          {(zb)->len=0;(zb)->ptr=(zb)->data;((zb)->data[0]=0);}
#define ZBUF_TERMINATE(zb)      (*(zb)->ptr=0)
#define ZBUF_TRUNCATE(zb, len)	{(zb)->len=(len);(zb)->ptr=(zb)->data+(len);*(zb)->ptr=0;}
#define ZBUF_SET_DATA(zb, datad, sizee, extended)	{\
	(zb)->flags = 0; (zb)->size = sizee; (zb)->len = 0;\
       	(zb)->ptr = (zb)->data = (char *)(datad);\
	(zb)->ready=((extended)?zbuf_string_write_ready:0);\
	(zb)->space = zbuf_string_space_ready;\
}
int zbuf_string_space_extend(ZBUF * zb, int incr);
int zbuf_string_write_ready(ZBUF * zb);
int zbuf_string_space_ready(ZBUF * zb, int want);
ZBUF *zbuf_create_buf(void);
void zbuf_free_buf(ZBUF * zb);
ZBUF *zbuf_create(int size);
void zbuf_free(ZBUF * zb);
void zbuf_reset(ZBUF * zb);
void zbuf_terminate(ZBUF * zb);
void zbuf_truncate(ZBUF * zb, int len);
int zbuf_strncpy(ZBUF * zb, char *src, int len);
int zbuf_strcpy(ZBUF * zb, char *src);
int zbuf_strncat(ZBUF * zb, char *src, int len);
int zbuf_strcat(ZBUF * zb, char *src);
int zbuf_memcpy(ZBUF * zb, char *src, int len);
int zbuf_memcat(ZBUF * zb, char *src, int len);
int zbuf_sprintf(ZBUF * zs, char *fmt, ...);
int zbuf_vsprintf(ZBUF * zb, char *format, va_list ap);
void zbuf_sizedata_escape(ZBUF * zb, void *data, int len);
void zbuf_sizedata_escape_int(ZBUF * zb, int i);
void zbuf_sizedata_escape_long(ZBUF * zb, long i);
void zbuf_sizedata_escape_dict(ZBUF * zb, ZDICT * zd);
void zbuf_sizedata_escape_pp(ZBUF * zb, char **pp, int len);

/* stack buf */
#define ZSTACK_BUF(name, size)	\
	ZBUF name ## _ZSTACT_BUF_ ,  *name;\
       	name = &name ## _ZSTACT_BUF_;\
       	char name ## _databuf_STACK [size+1];\
	ZBUF_SET_DATA(name, (name ## _databuf_STACK), size, 0);

/* SIZE DATA SDATA */
struct ZSDATA {
	int size;
	void *data;
};
int zsdata_parse(ZSDATA * sd, void *data, int size);
void zsdata_escape(ZSDATA * sd, ZBUF * zb);

/* ARGV */
struct ZARGV {
	int size;
	int argc;
	char **argv;
};
#define ZARGV_LEN(za)		((za)->argc)
#define ZARGV_ARGC(za)		((za)->argc)
#define ZARGV_ARGV(za)		((za)->argv)
ZARGV *zargv_create(int size);
ZARGV *zargv_free(ZARGV * argvp);
void zargv_add(ZARGV * argvp, char *ns);
void zargv_addn(ZARGV * argvp, char *ns, int nlen);
void zargv_truncate(ZARGV * argvp, int len);
ZARGV *zargv_split(const char *string, const char *delim);
ZARGV *zargv_split_append(ZARGV * argvp, const char *string, const char *delim);
#define ZARGV_REST(a)	(zargv_truncate((a), 0))
#define ZARGV_WALK_BEGIN(za, var_your_chp)	{\
	int  zargv_tmpvar_i; ZARGV *___za_tmp_ptr = za;\
	for(zargv_tmpvar_i=0;zargv_tmpvar_i<(___za_tmp_ptr)->argc;zargv_tmpvar_i++){\
		var_your_chp = (___za_tmp_ptr)->argv[zargv_tmpvar_i];
#define ZARGV_WALK_END				}}

/* ARRAY */
struct ZARRAY {
	int len;
	int size;
	char **data;
};
#define ZARRAY_LEN(za)		((za)->len)
#define ZARRAY_DATA(za)		((za)->data)
ZARRAY *zarray_create(int size);
void zarray_free(ZARRAY * arr, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx);
void *zarray_free_STR(ZARRAY * arr);
void zarray_enter(ZARRAY * arr, void *ns);
void zarray_truncate(ZARRAY * arr, int len, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx);
#define ZARRAY_WALK_BEGIN(za, var_your_chp)	{\
	int  zargv_tmpvar_i;\
	for(zargv_tmpvar_i=0;zargv_tmpvar_i<(za)->len;zargv_tmpvar_i++){\
		var_your_chp = (za)->data[zargv_tmpvar_i];
#define ZARRAY_WALK_END				}}

/* ZRING */
#define ZRING_WALK_BEGIN(head, var_your_node) 		{\
       	for (var_your_node = (head)->next; var_your_node != (head); var_your_node = var_your_node->next){
#define ZRING_WALK_END				}}
struct ZRING {
	ZRING *prev;
	ZRING *next;
};
void zring_init(ZRING * ring);
void zring_append(ZRING * ring, ZRING * entry);
void zring_prepend(ZRING * ring, ZRING * entry);
void zring_detach(ZRING * entry);

/* LINK  MLINK*/
#define ZMLINK_INIT(p)	(p=0)
#define ZMLINK_APPEND(head, tail, node, prev, next) {\
	if(head == 0){node->prev=node->next=0;head=tail=node;}\
	else {tail->next=node;node->prev=tail;node->next=0;tail=node;}\
}
#define ZMLINK_PREPEND(head, tail, node, prev, next) {\
	if(head == 0){node->prev=node->next=0;head=tail=node;}\
	else {head->prev=node;node->next=head;node->prev=0;head=node;}\
}
#define ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before) {\
	if(head == 0){node->prev=node->next=0;head=tail=node;}\
	else if(before==0){tail->next=node;node->prev=tail;node->next=0;tail=node;}\
	else if(before==head){head->prev=node;node->next=head;node->prev=0;head=node;}\
	else {node->prev=before->prev; node->next=before; before->prev->next=node; before->prev=node;}\
}
#define ZMLINK_DETACH(head, tail, node, prev, next) {\
	if(node->prev){ node->prev->next=node->next; }else{ head=node->next; }\
	if(node->next){ node->next->prev=node->prev; }else{ tail=node->prev; }\
}

/* LINK innode */
struct ZLINK {
	ZLINK_NODE *head;
	ZLINK_NODE *tail;
};
struct ZLINK_NODE {
	ZLINK_NODE *prev;
	ZLINK_NODE *next;
};
void zlink_init(ZLINK * link);
ZLINK_NODE *zlink_attach_before(ZLINK * link, ZLINK_NODE * node, ZLINK_NODE * before);
ZLINK_NODE *zlink_detach(ZLINK * link, ZLINK_NODE * node);
ZLINK_NODE *zlink_push(ZLINK * link, ZLINK_NODE * node);
ZLINK_NODE *zlink_unshift(ZLINK * link, ZLINK_NODE * node);
ZLINK_NODE *zlink_pop(ZLINK * link);
ZLINK_NODE *zlink_shift(ZLINK * link);
void zlink_fini(ZLINK * link, void (*fini_fn) (ZLINK_NODE *));
void zlink_walk(ZLINK * link, void (*walk_fn) (ZLINK_NODE *));

/* CHAIN */
#define ZCHAIN_NEXT(n)			((n)->next)
#define ZCHAIN_PREV(n)			((n)->prev)
#define ZCHAIN_HEAD(c)			((c)->head)
#define ZCHAIN_TAIL(c)			((c)->tail)
struct ZCHAIN {
	ZCHAIN_NODE *head;
	ZCHAIN_NODE *tail;
	int len;
};
struct ZCHAIN_NODE {
	ZCHAIN_NODE *prev;
	ZCHAIN_NODE *next;
	char *value;
};
ZCHAIN *zchain_create(void);
void zchain_free(ZCHAIN * zc, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx);
void zchain_free_STR(ZCHAIN * zc);
void zchain_walk(ZCHAIN * zc, void (*walk_fn) (ZCHAIN_NODE *));
int zchain_attach_before(ZCHAIN * zc, ZCHAIN_NODE * n, ZCHAIN_NODE * before);
int zchain_detach(ZCHAIN * zc, ZCHAIN_NODE * n);
ZCHAIN_NODE *zchain_enter_before(ZCHAIN * zc, char *value, ZCHAIN_NODE * before);
ZCHAIN_NODE *zchain_delete(ZCHAIN * zc, ZCHAIN_NODE * n, char **value);
static inline ZCHAIN_NODE *zchain_push(ZCHAIN * zc, char *value)
{
	return zchain_enter_before(zc, value, 0);
}

static inline ZCHAIN_NODE *zchain_unshift(ZCHAIN * zc, char *value)
{
	return zchain_enter_before(zc, value, zc->head);
}

static inline ZCHAIN_NODE *zchain_pop(ZCHAIN * zc, char **value)
{
	return zchain_delete(zc, zc->tail, value);
}

static inline ZCHAIN_NODE *zchain_shift(ZCHAIN * zc, char **value)
{
	return zchain_delete(zc, zc->head, value);
}

#define ZCHAIN_WALK_BEGIN(zc, var_your_node)	{\
	var_your_node=(zc)->head;\
	for(;var_your_node;var_your_node=var_your_node->next){
#define ZCHAIN_WALK_END				}}

/* HTABLE */
typedef int (*ZHTABLE_CMP_FN) (ZHTABLE_NODE * node1, ZHTABLE_NODE * node2);
typedef int (*ZHTABLE_HASH_FN) (ZHTABLE_NODE * node, int size);
struct ZHTABLE_NODE {
	ZHTABLE_NODE *next;
	ZHTABLE_NODE *prev;
};
struct ZHTABLE {
	int size;
	int len;
	ZHTABLE_NODE **data;
	ZHTABLE_CMP_FN cmp_fn;
	ZHTABLE_HASH_FN hash_fn;
};
void zhtable_init(ZHTABLE * table, int size, ZHTABLE_CMP_FN cmp_fn, ZHTABLE_HASH_FN hash_fn);
ZHTABLE_NODE *zhtable_attach(ZHTABLE * table, ZHTABLE_NODE * node);
ZHTABLE_NODE *zhtable_lookup(ZHTABLE * table, ZHTABLE_NODE * vnode);
ZHTABLE_NODE *zhtable_remove(ZHTABLE * table, ZHTABLE_NODE * vnode);
ZHTABLE_NODE *zhtable_detach(ZHTABLE * table, ZHTABLE_NODE * node);
void zhtable_fini(ZHTABLE * table, void (*fini_fn) (ZHTABLE_NODE *, void *), void *ctx);
void zhtable_walk(ZHTABLE * table, void (*walk_fn) (ZHTABLE_NODE *, void *), void *ctx);
ZHTABLE_NODE **zhtable_list(ZHTABLE * table, ZHTABLE_NODE ** list);
#define ZHTABLE_LEN(table)	((table)->len)
#define ZHTABLE_SIZE(table)	((table)->size)
#define ZHTABLE_DATA(table)	((table)->data)
#define ZHTABLE_WALK_BEGIN(table, var_your_node) {							\
	unsigned var_your_node ## __LINE__ ## i = (table)->size;					\
	ZHTABLE_NODE **var_your_node ## __LINE__ ## h = (table)->data;					\
	ZHTABLE_NODE *var_your_node ## __LINE__ ## next;						\
	while (var_your_node ## __LINE__ ## i --){ 							\
		for (var_your_node= *var_your_node ## __LINE__ ## h++;					\
				var_your_node;var_your_node = var_your_node ## __LINE__ ## next){	\
			var_your_node ## __LINE__ ## next = var_your_node->next;
#define ZHTABLE_WALK_END										}}}
#define ZHTABLE_BUF_HASH(result, pre_hash, key, cond)	\
{							\
	unsigned long g;				\
	result = pre_hash;				\
	while (cond) {					\
		result = (result << 4U) + *key++;	\
		if ((g = (result & 0xf0000000)) != 0) { \
			result ^= (g >> 24U);		\
			result ^= g;			\
		}					\
	}						\
}

/* RBTREE */
typedef int (*ZRBTREE_CMP_FN) (ZRBTREE_NODE * node1, ZRBTREE_NODE * node2);
struct ZRBTREE {
	ZRBTREE_NODE *zrbtree_node;
	ZRBTREE_CMP_FN cmp_fn;
};
struct ZRBTREE_NODE {
	unsigned long __zrbtree_parent_color;
	ZRBTREE_NODE *zrbtree_right;
	ZRBTREE_NODE *zrbtree_left;
/* The alignment might seem pointless, but allegedly CRIS needs it */
} __attribute__ ((aligned(sizeof(long))));

static inline int zrbtree_have_data(ZRBTREE * tree)
{
	return (tree->zrbtree_node ? 1 : 0);
}

void zrbtree_init(ZRBTREE * tree, ZRBTREE_CMP_FN cmp_fn);
void zrbtree_insert_color(ZRBTREE *, ZRBTREE_NODE *);
void zrbtree_erase(ZRBTREE * tree, ZRBTREE_NODE * node);
void zrbtree_replace_node(ZRBTREE * tree, ZRBTREE_NODE * victim, ZRBTREE_NODE * _new);
ZRBTREE_NODE *zrbtree_prev(ZRBTREE_NODE * tree);
ZRBTREE_NODE *zrbtree_next(ZRBTREE_NODE * tree);
ZRBTREE_NODE *zrbtree_first(ZRBTREE * node);
ZRBTREE_NODE *zrbtree_last(ZRBTREE * node);
ZRBTREE_NODE *zrbtree_near_prev(ZRBTREE * tree, ZRBTREE_NODE * vnode);
ZRBTREE_NODE *zrbtree_near_next(ZRBTREE * tree, ZRBTREE_NODE * vnode);
static inline ZRBTREE_NODE *zrbtree_parent(ZRBTREE_NODE * node)
{
	return ((ZRBTREE_NODE *) ((node)->__zrbtree_parent_color & ~3));
}

ZRBTREE_NODE *zrbtree_attach(ZRBTREE * tree, ZRBTREE_NODE * node);
ZRBTREE_NODE *zrbtree_lookup(ZRBTREE * tree, ZRBTREE_NODE * vnode);
static inline ZRBTREE_NODE *zrbtree_detach(ZRBTREE * tree, ZRBTREE_NODE * node)
{
	zrbtree_erase(tree, node);
	return node;
}

void zrbtree_walk(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx);
#define zrbtree_fini 	zrbtree_walk
void zrbtree_walk_forward(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx);
void zrbtree_walk_back(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx);

static inline void zrbtree_link_node(ZRBTREE_NODE * node, ZRBTREE_NODE * parent, ZRBTREE_NODE ** zrbtree_link)
{
	node->__zrbtree_parent_color = (unsigned long)parent;
	node->zrbtree_left = node->zrbtree_right = NULL;

	*zrbtree_link = node;
}

#define ZRBTREE_INIT(tree, _cmp_fn) 	((tree)->zrbtree_node=0, (tree)->cmp_fn = _cmp_fn)
#define ZRBTREE_PARENT(node)	((ZRBTREE_NODE *)((node)->__zrbtree_parent_color & ~3))
#define ZRBTREE_ATTACH_PART1(root, node, cmp_node) {							\
	ZRBTREE_NODE ** ___Z_new_pp = &((root)->zrbtree_node), * ___Z_parent = 0;			\
	while (*___Z_new_pp) {										\
		___Z_parent = *___Z_new_pp;								\
		cmp_node = *___Z_new_pp;								\
		{
#define ZRBTREE_ATTACH_PART2(root, node, cmp_result, return_node) 					\
		}											\
		return_node = 0;									\
		if (cmp_result < 0) {									\
			___Z_new_pp = &((*___Z_new_pp)->zrbtree_left);					\
		} else if (cmp_result > 0) {								\
			___Z_new_pp = &((*___Z_new_pp)->zrbtree_right);					\
		} else {										\
			return_node = *___Z_new_pp;							\
			break;										\
		}											\
	}												\
	if(!return_node){										\
		zrbtree_link_node(node, ___Z_parent, ___Z_new_pp);					\
		zrbtree_insert_color(root, node);							\
		return_node = node;									\
	}												\
}

#define ZRBTREE_LOOKUP_PART1(root, cmp_node) {								\
	ZRBTREE_NODE * ___Z_node_tmp = (root)->zrbtree_node;						\
	while (___Z_node_tmp) {										\
		cmp_node = ___Z_node_tmp;								\
		{
#define ZRBTREE_LOOKUP_PART2(root, cmp_result, return_node)						\
		}											\
		return_node = 0;									\
		if (cmp_result < 0) {									\
			___Z_node_tmp = ___Z_node_tmp->zrbtree_left;					\
		} else if (cmp_result > 0) {								\
			___Z_node_tmp = ___Z_node_tmp->zrbtree_right;					\
		} else {										\
			return_node = ___Z_node_tmp;							\
			break;										\
		}											\
	}												\
}

#define ZRBTREE_DETACH(root, node) {									\
	zrbtree_erase(root, node);									\
}

#define ZRBTREE_WALK_BEGIN(root, var_your_node) {							\
	struct { ZRBTREE_NODE *node; unsigned char lrs; } ___Z_list[64];				\
	ZRBTREE_NODE *___Z_node = (root)->zrbtree_node;							\
	int ___Z_idx = 0, ___Z_lrs;									\
	___Z_list[0].node = ___Z_node;									\
	___Z_list[0].lrs = 0;										\
	while (1) {											\
		var_your_node = ___Z_node = ___Z_list[___Z_idx].node;					\
		___Z_lrs = ___Z_list[___Z_idx].lrs;							\
		if (!___Z_node || ___Z_lrs == 2) {							\
			if (___Z_node) {
#define ZRBTREE_WALK_END										\
			}										\
			___Z_idx--;									\
			if (___Z_idx == -1){								\
				break;									\
		       	}										\
			___Z_list[___Z_idx].lrs++;							\
			continue;									\
		}											\
		___Z_idx++;										\
		___Z_list[___Z_idx].lrs = 0;								\
		___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->zrbtree_left : ___Z_node->zrbtree_right);\
	}												\
}

#define ZRBTREE_WALK_FORWARD_BEGIN(root, var_your_node) 	{					\
	for (var_your_node = zrbtree_first(root); var_your_node; var_your_node = zrbtree_next(var_your_node)) {
#define ZRBTREE_WALK_FORWARD_END				}}

#define ZRBTREE_WALK_BACK_BEGIN(root, var_your_node) 	{					\
	for (var_your_node = zrbtree_last(root); var_your_node; var_your_node = zrbtree_prev(var_your_node)) {
#define ZRBTREE_WALK_BACK_END				}}

/* DICT */
struct ZDICT {
	int len;
	ZRBTREE rbtree;
};
struct ZDICT_NODE {
	char *key;
	void *value;
	ZRBTREE_NODE rbnode;
};
ZDICT *zdict_create(void);
ZDICT_NODE *zdict_attach(ZDICT * zd, ZDICT_NODE * node);
ZDICT_NODE *zdict_detach(ZDICT * zd, ZDICT_NODE * node);
ZDICT_NODE *zdict_enter(ZDICT * zd, char *key, void *value, ZDICT_NODE ** ret_node);
ZDICT_NODE *zdict_lookup(ZDICT * zd, char *key, char **value);
ZDICT_NODE *zdict_remove(ZDICT * zd, char *key, char **old_value);
ZDICT_NODE *zdict_enter_STR(ZDICT * zd, char *key, char *str);
ZDICT_NODE *zdict_remove_STR(ZDICT * zd, char *key);
void zdict_walk(ZDICT * zd, void (*walk_fn) (ZDICT_NODE *, void *), void *ctx);
void zdict_free(ZDICT * zd, void (*free_fn) (ZDICT_NODE *, void *), void *ctx);
void zdict_free2(ZDICT * zd, void (*free_fn) (ZDICT_NODE *, void *), void *ctx);
void zdict_free_STR(ZDICT * zd);
void zdict_free2_STR(ZDICT * zd);
ZDICT_NODE *zdict_first(ZDICT * zd);
ZDICT_NODE *zdict_last(ZDICT * zd);
ZDICT_NODE *zdict_prev(ZDICT_NODE * node);
ZDICT_NODE *zdict_next(ZDICT_NODE * node);
int zdict_keys(ZDICT * zd, char **key_list, int size);
#define ZDICT_LEN(zd)			((zd)->len)
#define ZDICT_KEY(n)			((n)->key)
#define ZDICT_VALUE(n)			((n)->value)
#define ZDICT_UPDATE_VALUE(n, v)	((n)->value=(void *)(v))
static inline ZDICT_NODE *zdict_node_create(void)
{
	return (ZDICT_NODE *) zmalloc(sizeof(ZDICT_NODE));
}

static inline void zdict_node_free(ZDICT_NODE * node)
{
	zfree(node);
}

#define ZDICT_WALK_BEGIN(dict, n) { \
	for(n = zdict_first(dict); n; n = zdict_next(n)) {

#define ZDICT_WALK_END	}}

/* dict config */
extern ZDICT *zvar_default_config;
int zdict_parse_line(ZDICT * zd, char *buf, int len);
char *zdict_get_str(ZDICT * zd, char *name, char *def);
int zdict_get_bool(ZDICT * zd, char *name, int def);
int zdict_get_int(ZDICT * zd, char *name, int def);
int zdict_get_time(ZDICT * zd, char *name, int def);
int zdict_get_size(ZDICT * zd, char *name, int def);
void zdict_config_load_file(ZDICT * zd, char *filename);
ZDICT *zdict_config_get_child(ZDICT * zd, char *name);
ZDICT *zdict_config_create_child(ZDICT * zd, char *name);
void zdict_config_free(ZDICT * zd);
void zdict_config_debug(ZDICT * zd);
#define ZDICT_CONFIG_WALK_BEGIN(dict, n, v) { \
	ZDICT_NODE *zdicT_node ## __LINE__; \
	for(zdicT_node ## __LINE__ = zdict_first(dict); zdicT_node ## __LINE__; zdicT_node ## __LINE__ = zdict_next(zdicT_node ## __LINE__)) {\
		n = ZDICT_KEY(zdicT_node ## __LINE__); if (*n=='='){ continue;} \
		v = (char *)(ZDICT_VALUE(zdicT_node ## __LINE__)); { \


#define ZDICT_CONFIG_WALK_END	}}}

#define ZDICT_CONFIG_WALK_CHILD_BEGIN(dict, n, child) { \
	ZDICT_NODE *zdicT_node ## __LINE__; \
	for(zdicT_node ## __LINE__ = zdict_first(dict); zdicT_node ## __LINE__; zdicT_node ## __LINE__ = zdict_next(zdicT_node ## __LINE__)) {\
		n = ZDICT_KEY(zdicT_node ## __LINE__); if (*n++!='='){ continue;} \
		child = (ZDICT *)(ZDICT_VALUE(zdicT_node ## __LINE__)); { \

#define ZDICT_CONFIG_WALK_CHILD_END	}}}

/* IDICT */
struct ZIDICT {
	int len;
	ZRBTREE rbtree;
};
struct ZIDICT_NODE {
	int key;
	void *value;
	ZRBTREE_NODE rbnode;
};
ZIDICT *zidict_create(void);
ZIDICT_NODE *zidict_attach(ZIDICT * zd, ZIDICT_NODE * node);
ZIDICT_NODE *zidict_detach(ZIDICT * zd, ZIDICT_NODE * node);
ZIDICT_NODE *zidict_enter(ZIDICT * zd, int key, void *value, ZIDICT_NODE ** ret_node);
ZIDICT_NODE *zidict_lookup(ZIDICT * zd, int key, char **value);
ZIDICT_NODE *zidict_remove(ZIDICT * zd, int key, char **old_value);
void zidict_walk(ZIDICT * zd, void (*walk_fn) (ZIDICT_NODE *, void *), void *ctx);
void zidict_free(ZIDICT * zd, void (*free_fn) (ZIDICT_NODE *, void *), void *ctx);
void zidict_free2(ZIDICT * zd, void (*free_fn) (ZIDICT_NODE *, void *), void *ctx);
void zidict_free_STR(ZIDICT * zd);
void zidict_free2_STR(ZIDICT * zd);
ZIDICT_NODE *zidict_first(ZIDICT * zd);
ZIDICT_NODE *zidict_last(ZIDICT * zd);
ZIDICT_NODE *zidict_prev(ZIDICT_NODE * node);
ZIDICT_NODE *zidict_next(ZIDICT_NODE * node);
int zidict_keys(ZIDICT * zd, int *key_list, int size);
#define ZIDICT_LEN(zd)			((zd)->len)
#define ZIDICT_KEY(n)			((n)->key)
#define ZIDICT_VALUE(n)			((n)->value)
#define ZIDICT_UPDATE_VALUE(n, v)	((n)->value=(void *)(v))
static inline ZIDICT_NODE *zidict_node_create(void)
{
	return (ZIDICT_NODE *) zmalloc(sizeof(ZIDICT_NODE));
}

static inline void zidict_node_free(ZIDICT_NODE * node)
{
	zfree(node);
}

/* MPOOL MALLOC*/
struct ZMPOOL {
	unsigned short int element_size;
	unsigned short int element_count_per_set;
	unsigned int element_unused_limit;
	unsigned int element_unused_sum;
	unsigned int set_sum;
	ZRING set_ring;
	ZRING set_used_ring;
	ZRING set_unused_ring;
	ZRBTREE rbtree;
};
ZMPOOL *zmpool_create(int element_size, int element_count_per_set, int element_unused_limit);
void *zmpool_alloc_one(ZMPOOL * mp);
void zmpool_free_one(ZMPOOL * mp, void *p);
void zmpool_free(ZMPOOL * mp);

/* SOCKET */
/* inet unix fifo */
#define ZSOCKET_TYPE_INET	'i'
#define ZSOCKET_TYPE_UNIX	'u'
#define ZSOCKET_TYPE_FIFO	'f'

#define ___ziuf_parse(iuf_url, type, host_path, port) \
{ \
       	char *p; \
	type = ZCHAR_TOLOWER(*iuf_url); \
	if(!(p = strchr(iuf_url, ':'))){ \
		type = 0; \
	} else if(p++, !*p) { \
		type = 0; \
	} else { \
		host_path = p; \
		if(!host_path){ \
			type = 0; \
		} else if(type == ZSOCKET_TYPE_INET) { \
			if(!(p = strchr(host_path, ':'))){ \
				port = atoi((const char *)host_path); \
				host_path=0; \
			}else { \
				*p++=0; \
				port = atoi(p); \
			}\
		}\
	}\
}

int zsocket_sane_connect(int sock, struct sockaddr *sa, socklen_t len);
int zsocket_timed_connect(int sock, struct sockaddr *sa, int len, int timeout);
int zsocket_unix_connect(const char *addr, int timeout);
int zsocket_inet_connect(char *dip, int port, int timeout);
int zsocket_sane_accept(int sock, struct sockaddr *sa, socklen_t * len);
int zsocket_unix_accept(int fd);
int zsocket_inet_accept(int fd);
int zsocket_unix_listen(const char *addr, int backlog, int unlinked, mode_t mode);
int zsocket_inet_listen(char *sip, int port, int backlog);
int zsocket_fifo_listen(const char *path, int unlinked, mode_t mode);
int zsocket_inet_connect_host(char *host, int port, int timeout);

int zsocket_parse_path(char *path, int *type, char **host_or_path, int *port);

/* DNS */
void zdns_inet_ntoa_r(struct in_addr *in, char *buf);
int zdns_getaddr_r(char *host, struct in_addr *addr_list, int addr_list_len);
int zdns_getlocaladdr_r(struct in_addr *addr_list, int addr_list_len);
int zdns_getpeer_r(int sockfd, int *host, int *port);

/* IO */
#define ZIO_EOF			(-1)
#define ZIO_ERROR		(-2)
#define ZIO_ERR			(-2)
#define ZIO_TIMEOUT		(-3)

int zio_read_wait(int fd, int timeout);
ssize_t zio_timed_read(int fd, void *buf, size_t len, int timeout);
ssize_t zio_timed_strict_read(int fd, void *buf, size_t len, int timeout);
ssize_t zio_timed_read_delimiter(int fd, void *buf, size_t len, int delimiter, int timeout);
int zio_write_wait(int fd, int timeout);
ssize_t zio_timed_write(int fd, void *buf, size_t len, int timeout);
ssize_t zio_timed_strict_write(int fd, void *buf, size_t len, int timeout);
int zio_readable_or_writeable(int fd, int events);
inline static int zio_readable(int fd)
{
	return zio_readable_or_writeable(fd, POLLIN);
}

inline static int zio_writeable(int fd)
{
	return zio_readable_or_writeable(fd, POLLOUT);
}

int zio_rwable(int fd, int *r, int *w);
int zio_nonblocking(int fd, int on);
int zio_close_on_exec(int fd, int on);
ssize_t zio_peek(int fd);
int zio_flock(int fd, int flags);

/* QUEUE */
struct ZQUEUE {
	pthread_mutex_t locker;
	pthread_cond_t cond;
	ZLINK node_list;
	ZLINK res_list;
	int current_res_count;
};

ZQUEUE *zqueue_create(void);
void zqueue_enter_resource(ZQUEUE * zq, void *res);
void *zqueue_require(ZQUEUE * zq);
void zqueue_release(ZQUEUE * zq, void *res);
void zqueue_free(ZQUEUE * zq);

/* SIGNAL */
/* alarm */
typedef int (*ZALARM_CB_FN) (ZALARM *, void *);
struct ZALARM {
	long timeout;
	ZALARM_CB_FN callback;
	void *context;
	unsigned short int in_time:1;
	ZRBTREE_NODE rbnode_time;
	short int enable_time:1;
};
extern int zvar_alarm_signal;

void zalarm_base_init(int sig, int used_pthread);
void zalarm_base_fini(void);
void zalarm_init(ZALARM * ala);
void zalarm_fini(ZALARM * ala);
int zalarm_set(ZALARM * ala, ZALARM_CB_FN callback, void *context, int timeout);

/* TIMER */
typedef int (*ZTIMER_CB_FN) (ZTIMER *, void *);
struct ZTIMER {
	long timeout;
	ZTIMER_CB_FN callback;
	void *context;
	ZRBTREE_NODE rbnode_time;
	unsigned short int in_time:1;
	unsigned short int enable_time:1;
	ZEVENT_BASE *event_base;
};

void ztimer_base_init(ZRBTREE * timer_tree);
void ztimer_init(ZTIMER * zt, ZEVENT_BASE * eb);
void ztimer_fini(ZTIMER * zt);
int ztimer_check(ZRBTREE * timer_tree);
int ztimer_set(ZTIMER * zt, ZTIMER_CB_FN callback, void *context, int timeout);

/* EVENT*/
#define ZEVENT_TYPE_EVENT 		0x1
#define ZEVENT_TYPE_AIO			0x2

#define ZEVENT_NONE			0x00000000
#define ZEVENT_READ			0x00000001
#define ZEVENT_WRITE			0x00000002
#define ZEVENT_RDWR			0x00000003
#define ZEVENT_PERSIST			0x00000004
/* exception */
#define ZEVENT_RDHUP			0x00000010
#define ZEVENT_HUP			0x00000020
#define ZEVENT_ERROR			0x00000040
#define ZEVENT_ERRORS			0x00000070
#define ZEVENT_TIMEOUT			0x00000080
#define ZEVENT_EXCEPTION		0x000000F0

typedef int (*ZEVENT_CB_FN) (ZEVENT *, void *);

struct ZEVENT {
	unsigned int aio_type:3;
	int fd:29;
	unsigned short int events:8;
	unsigned short int recv_events:8;
	unsigned int in_time:1;
	unsigned int enable_time:1;
	ZEVENT_CB_FN callback;
	void *context;
	long timeout;
	ZRBTREE_NODE rbnode_time;
	ZEVENT_BASE *event_base;
};

static inline ZEVENT_BASE *zevent_get_base(ZEVENT * ev)
{
	return ev->event_base;
}

static inline int zevent_get_fd(ZEVENT * ev)
{
	return ev->fd;
}

static inline int zevent_get_events(ZEVENT * ev)
{
	return ev->recv_events;
}

int zevent_attach(ZEVENT * zev, ZEVENT_CB_FN callback, void *context);
int zevent_detach(ZEVENT * zev);
int zevent_base_notify(ZEVENT_BASE * eb);

void zevent_init(ZEVENT * tg, ZEVENT_BASE * eb, int fd);
void zevent_fini(ZEVENT * tg);
int zevent_set(ZEVENT * zev, int events, ZEVENT_CB_FN callback, void *context, int timeout);
int zevent_unset(ZEVENT * zev);

/* AIO */
typedef struct ZAIO_RWBUF ZAIO_RWBUF;
typedef struct ZAIO_CACHE ZAIO_CACHE;
#define ZAIO_RWBUF_SIZE			1024
struct ZAIO_RWBUF {
	char data[ZAIO_RWBUF_SIZE];
	unsigned int p1:16;
	unsigned int p2:16;
	ZAIO_RWBUF *next;
};
struct ZAIO_CACHE {
	ZAIO_RWBUF *head;
	ZAIO_RWBUF *tail;
	int len;
};

typedef int (*ZAIO_CB_FN) (ZAIO *, void *, char *);

typedef struct ZAIO_SSL ZAIO_SSL;
struct ZAIO_SSL {
	unsigned char server_or_client:1;
	unsigned char session_init:1;
	unsigned char read_want_read:1;
	unsigned char read_want_write:1;
	unsigned char write_want_read:1;
	unsigned char write_want_write:1;
	unsigned char error:1;
	SSL *ssl;
};
struct ZAIO {
	unsigned int aio_type:3;
	int fd:29;
	void *context;
	ZAIO_CB_FN callback;
	unsigned short int events:8;
	unsigned short int recv_events:8;
	unsigned char rw_type;
	unsigned char delimiter;
	int read_magic_len;
	int ret;
	ZAIO_CACHE read_cache;
	ZAIO_CACHE write_cache;
	unsigned char in_loop:1;
	unsigned char want_read:1;
	unsigned char in_time:1;
	unsigned char enable_time:1;
	long timeout;
	ZRBTREE_NODE rbnode_time;
	ZEVENT_BASE *event_base;
	ZAIO_SSL *ssl;
};

typedef int (*ZEVENT_BASE_CB_FN) (ZEVENT_BASE *, void *);
typedef struct ZEVENT_BASE_QUEUE ZEVENT_BASE_QUEUE;
struct ZEVENT_BASE_QUEUE {
	ZEVENT_BASE_CB_FN callback;
	void *context;
	ZEVENT_BASE_QUEUE *prev;
	ZEVENT_BASE_QUEUE *next;
};

int zevent_base_queue_enter(ZEVENT_BASE * eb, ZEVENT_BASE_CB_FN callback, void *context);

struct ZEVENT_BASE {
	int epoll_fd;
	int epoll_event_count;
	struct epoll_event *epoll_event_list;
	void *context;
	pthread_mutex_t locker;
	ZRBTREE timer_tree;
	ZRBTREE event_timer_tree;
	ZRBTREE aio_timer_tree;
	ZEVENT_BASE_QUEUE *event_base_queue_head;
	ZEVENT_BASE_QUEUE *event_base_queue_tail;
	ZRBTREE_NODE *aio_set_list_head;
	ZRBTREE_NODE *aio_set_list_tail;
	ZRBTREE_NODE *event_set_list_head;
	ZRBTREE_NODE *event_set_list_tail;
	ZMPOOL *aio_rwbuf_mpool;
	ZEVENT eventfd_event;
	ZAIO *magic_aio;
};

#define ZAIO_EOF			(-1)
#define ZAIO_ERROR			(-2)
#define ZAIO_ERR			(-2)
#define ZAIO_TIMEOUT			(-3)

static inline ZEVENT_BASE *zaio_get_base(ZAIO * aio)
{
	return aio->event_base;
}

static inline int zaio_get_fd(ZAIO * aio)
{
	return aio->fd;
}

static inline int zaio_get_ret(ZAIO * aio)
{
	return aio->ret;
}

ZEVENT_BASE *zevent_base_create(void);
static inline void zevent_base_set_context(ZEVENT_BASE * eb, void *context)
{
	eb->context = context;
}

static inline void *zevent_base_get_context(ZEVENT_BASE * eb)
{
	return eb->context;
}

void zevent_base_free(ZEVENT_BASE * eb);
void zevent_base_dispatch(ZEVENT_BASE * eb, int delay);

int zaio_attach(ZAIO * aio, ZAIO_CB_FN callback, void *context);
int zaio_detach(ZAIO * aio);
void zaio_init(ZAIO * aio, ZEVENT_BASE * eb, int fd);
void zaio_fini(ZAIO * aio);
void zaio_reset_base(ZAIO * aio, ZEVENT_BASE * eb_new);
int zaio_read(ZAIO * aio, int max_len, ZAIO_CB_FN callback, void *context, int timeout);
int zaio_read_n(ZAIO * aio, int strict_len, ZAIO_CB_FN callback, void *context, int timeout);
int zaio_read_delimiter(ZAIO * aio, char delimiter, int max_len, ZAIO_CB_FN callback, void *context, int timeout);
#define zaio_read_line(a,c,d,e,f) zaio_read_delimiter(a,'\n',c,d,e,f)
int zaio_read_sizedata(ZAIO * aio, ZAIO_CB_FN callback, void *context, int timeout);
int zaio_write_cache_append(ZAIO * aio, void *buf, int len);
int zaio_write_cache_append_sizedata(ZAIO * aio, int len, void *buf);
int zaio_write_cache_flush(ZAIO * aio, ZAIO_CB_FN callback, void *context, int timeout);
int zaio_write_cache_get_len(ZAIO *aio);
int zaio_sleep(ZAIO * aio, ZAIO_CB_FN callback, char *context, int timeout);

int zaio_ssl_init(ZAIO * aio, ZSSL_CTX * ctx, ZAIO_CB_FN callback, void *context, int timeout);
void zaio_ssl_fini(ZAIO * aio);
int zaio_ssl_attach(ZAIO * aio, ZAIO_SSL * zssl);
ZAIO_SSL *zaio_ssl_detach(ZAIO * aio);
SSL *zaio_ssl_detach_ssl(ZAIO_SSL * assl);
int zaio_move(ZAIO * aio, ZEVENT_BASE * neb, ZAIO_CB_FN attach_callback, void *context);

/* ZEVENT connect */
int zevent_inet_connect(ZEVENT * zev, char *dip, int port, ZEVENT_CB_FN callback, void *context, int timeout);
int zevent_unix_connect(ZEVENT * zev, char *path, ZEVENT_CB_FN callback, void *context, int timeout);

/* IOPIPE */
struct ZIOPIPE_PART {
	unsigned int is_client_or_server:1;
	unsigned char read_want_read:1;
	unsigned char read_want_write:1;
	unsigned char write_want_read:1;
	unsigned char write_want_write:1;
	unsigned char ssl_error:1;
	unsigned int old_events:8;
	SSL *ssl;
	int rbuf_p1:16;
	int rbuf_p2:16;
	char *rbuf;
	int fd;
};
struct ZIOPIPE {
	ZIOPIPE_PART client;
	ZIOPIPE_PART server;
};

struct ZIOPIPE_BASE {
	void *context;
	int epoll_fd;
	int epoll_event_count;
	struct epoll_event *epoll_event_list;
	pthread_mutex_t locker;
	void *set_list_head;
	void *set_list_tail;
	ZIOPIPE eventfd_iop;
};
void ziopipe_enter(ZIOPIPE_BASE * iopb, int client_fd, SSL * client_ssl, int server_fd, SSL * server_ssl);
ZIOPIPE_BASE *ziopipe_base_create(void);
void ziopipe_base_dispatch(ZIOPIPE_BASE * iopb, int delay);
void ziopipe_base_free(ZIOPIPE_BASE * iopb);

/* SIO */
typedef ssize_t(*ZSIO_READ_FN) (ZSIO *, void *, size_t, int);
typedef ssize_t(*ZSIO_WRITE_FN) (ZSIO *, void *, size_t, int);

#define ZSIO_RBUF_SIZE       	4096
#define ZSIO_WBUF_SIZE       	4096
#define ZSIO_EOF		ZBUF_EOF
#define ZSIO_ERROR 		ZBUF_EOF
#define ZSIO_ERR 		ZBUF_EOF
#define ZSIO_TIMEOUT 		ZIO_TIMEOUT
#define ZSIO_FLAG_RD_ERR	ZBUF_FLAG_RD_ERR
#define ZSIO_FLAG_WR_ERR	ZBUF_FLAG_WR_ERR
#define ZSIO_FLAG_ERR		ZBUF_FLAG_ERR
#define ZSIO_FLAG_EOF		ZBUF_FLAG_EOF
#define ZSIO_FLAG_RD_TIMEOUT	ZBUF_FLAG_RD_TIMEOUT
#define ZSIO_FLAG_WR_TIMEOUT	ZBUF_FLAG_WR_TIMEOUT
#define ZSIO_FLAG_TIMEOUT	ZBUF_FLAG_TIMEOUT
#define ZSIO_FLAG_EXCEPTION	ZBUF_FLAG_EXCEPTION
struct ZSIO {
	int flags;
	long timeout;
	ZBUF rbuf;
	ZBUF wbuf;
	char rbuf_data[ZSIO_RBUF_SIZE];
	char wbuf_data[ZSIO_WBUF_SIZE];
	void *io_ctx;
	ZSIO_READ_FN read_fn;
	ZSIO_WRITE_FN write_fn;
};

#define ZSIO_FFLUSH(fp)		((fp)->wbuf.len?(zsio_fflush(fp)):0)
#define ZSIO_GET		ZSIO_GETCHAR
#define ZSIO_GETC		ZSIO_GETCHAR
#define ZSIO_GETCHAR(fp)	ZBUF_GET((&(fp)->rbuf))
#define ZSIO_PUT		ZSIO_PUTCHAR
#define ZSIO_PUTC		ZSIO_PUTCHAR
#define ZSIO_PUTCHAR(fp, inch)	ZBUF_PUT((&(fp)->wbuf), (inch))
#define ZSIO_FEOF(fp)		((fp)->flags & ZSIO_FLAG_EOF)
#define ZSIO_FERROR(fp)		((fp)->flags & ZSIO_FLAG_ERR)
#define ZSIO_FEXCEPTION(fp)	((fp)->flags & ZBUF_FLAG_EXCEPTION)
#define ZSIO_FILENO(fp)		((fp)->fd)

#define zsio_get		zsio_getchar
#define zsio_getc		zsio_getchar
#define zsio_put		zsio_putchar
#define zsio_putc		zsio_putchar
ZSIO *zsio_create(int unused);
void *zsio_free(ZSIO * fp);
void zsio_reset(ZSIO * fp);
int zsio_set_ioctx(ZSIO * fp, void *io_ctx, ZSIO_READ_FN read_fn, ZSIO_WRITE_FN write_fn);
int zsio_set_timeout(ZSIO * fp, int timeout);
int zsio_read_ready(ZBUF * rbuf);
int zsio_write_ready(ZBUF * wbuf);
int zsio_fflush(ZSIO * fp);
int zsio_read_cache(ZSIO * fp);
int zsio_getchar(ZSIO * fp);
int zsio_read(ZSIO * fp, void *buf, int len);
int zsio_read_n(ZSIO * fp, void *buf, int len);
int zsio_read_delimiter(ZSIO * fp, void *buf, int len, char delimiter);
int zsio_get_delimiter(ZSIO * fp, ZBUF * zb, char delimiter);
int zsio_get_n(ZSIO * fp, ZBUF * zb, int len);
int zsio_putchar(ZSIO * fp, int inch);
int zsio_write_n(ZSIO * fp, void *buf, int len);
int zsio_vfprintf(ZSIO * fp, char *format, va_list ap);
int zsio_fprintf(ZSIO * fp, char *format, ...);
int zsio_fputs(char *s, ZSIO * fp);
static inline int zsio_read_line(ZSIO * fp, void *buf, int len)
{
	return zsio_read_delimiter(fp, buf, len, '\n');
}

static inline int zsio_get_line(ZSIO * fp, ZBUF * zb)
{
	return zsio_get_delimiter(fp, zb, '\n');
}

/* SIO fd */
int zsio_set_FD(ZSIO * fp, int fd);
int zsio_get_FD(ZSIO * fp);
/* SIO ssl */
int zsio_set_SSL(ZSIO * fp, ZSSL * ssl);

/* MAP */
struct ZMAP {
	void *db;
	char *info;
	int info_size;
	ZTIMER release_timer;
	int release_sec;
	int (*query) (ZMAP * zm, char *query, char *value, int value_len, int timeout);
	int (*query2) (ZMAP * zm, char *query, char **value, int timeout);
	int (*close) (ZMAP * zm);
	int (*set_info) (ZMAP * zm, char *fmt, ...);
};
ZMAP *zmap_create(int flags);
int zmap_open(ZMAP * zm, char *url);
void zmap_free(ZMAP * zm);
int zmap_query(ZMAP * zm, char *query, char *value, int value_len, int timeout);
int zmap_query2(ZMAP * zm, char *query, char **value, int timeout);
char *zmap_get_info(ZMAP * zm);
int zmap_query_expand(ZBUF * zb, char *fmt, char *query, int flags);
#define zmap_set_info(zm, fmt, args...)	((zm)->set_info((zm), fmt, ##args))

/* MASTER */
#define ZMASTER_LISTEN_INET	'i'
#define ZMASTER_LISTEN_UNIX	'u'
#define ZMASTER_LISTEN_FIFO	'f'
#define ZMASTER_STATUS_FD	3

typedef void (*ZMASTER_SERVER_FN) (int fd, void *ctx, int type);
typedef struct {
	char *id;
	ZMASTER_SERVER_FN callback;
	void *ctx;
	int raw_flag;
} ZMASTER_SERVER_SERVICE;

typedef void (*ZMASTER_LOAD_CONFIG) (void *context);
int zmaster_start(ZMASTER_LOAD_CONFIG re_load_config, void *context);
int zmaster_lock_pid(char *lock_fn);
int zmaster_get_pid(char *lock_fn);
int zmaster_server_main(int argc, char **argv, ZMASTER_SERVER_SERVICE * service_list);
void zmaster_server_disconnect(int fd);
void zmaster_server_event_stop(void);

extern ZFN_VOID zmaster_server_before_service;
extern ZFN_VOID zmaster_server_on_reload;
extern int zmaster_server_base_stopped;

/* CMDER */
struct ZCMDER {
	unsigned int background:1;
	pthread_t pth_t;
	ZEVENT_BASE *eb;
	ZDICT *cmd_func_list;
	ZSDLIST *sdlist;
	ZBUF *outbuf;
	ZAIO *aio;
};
typedef int (*ZCMDER_FN) (ZCMDER *);
int zcmder_quit(ZCMDER * cmder);
int zcmder_flush(ZCMDER * cmder, int timeout);
int zcmder_fatal(ZCMDER * cmder);
int zcmder_error(ZCMDER * cmder, char *code, char *msg);
int zcmder_bad(ZCMDER * cmder);
int zcmder_ok_dict(ZCMDER * cmder, int timeout, ZDICT * dict);
ZCMDER *zcmder_create(void);
void zcmder_register(ZCMDER * cmder, char *cmd_name, ZCMDER_FN cmd_func);
void zcmder_run(ZCMDER * cmder, int flag);
int zcmder_append_zbuf(ZCMDER * cmder, ZBUF * zb);
int zcmder_append_buf(ZCMDER * cmder, int size, void *buf);
int zcmder_arguments_lookup(ZCMDER * cmder, char *key, char **value);
int zcmder_loop(ZAIO * aio, void *context, char *unused);
#define zcmder_attahcer		zcmder_loop
/* ZSSL openssl */

struct ZSSL_CTX {
	SSL_CTX *ssl_ctx;
	int server_or_client;
};
struct ZSSL {
	SSL *ssl;
	int fd;
	int server_or_client;
};
int zssl_INIT(int unused_flags);
ZSSL_CTX *zssl_ctx_server_create(int unused_flags);
ZSSL_CTX *zssl_ctx_client_create(int unused_flags);
int zssl_ctx_set_cert(ZSSL_CTX * ssl_ctx, char *cert_file, char *key_file);
void zssl_ctx_free(ZSSL_CTX * ctx);
void zssl_get_error(unsigned long *ecode, char *buf, int buf_len);
ZSSL *zssl_create(ZSSL_CTX * ctx, int fd);
void zssl_free(ZSSL * ssl);
SSL *zssl_detach_ssl(ZSSL * zssl);
int zssl_connect(ZSSL * ssl, int timeout);
int zssl_accept(ZSSL * ssl, int timeout);
int zssl_shutdown(ZSSL * ssl, int timeout);
int zssl_read(ZSSL * ssl, void *buf, int len, int timeout);
int zssl_write(ZSSL * ssl, void *buf, int len, int timeout);

/* SKVLIST */
struct ZSKVLIST {
	char *path;
	ZDICT *dict;
	FILE *list_fp;
	int lock_fd;
	int locks;
	ino_t ino;
	time_t mtime;
	off_t size;
};

ZSKVLIST *zskvlist_create(char *path);
void zskvlist_free(ZSKVLIST * kv);
int zskvlist_begin(ZSKVLIST * kv);
int zskvlist_end(ZSKVLIST * kv);
int zskvlist_changed(ZSKVLIST * kv);
int zskvlist_enter(ZSKVLIST * kv, char *key, char *value);
int zskvlist_delete(ZSKVLIST * kv, char *key);
int zskvlist_lookup(ZSKVLIST * kv, char *key, char **value);
int zskvlist_load(ZSKVLIST * kv);

/* KV */
struct ZKV {
	void *kvdb;
	int (*lookup_fn) (ZKV * zkv, char *key, char **value);
	int (*delete_fn) (ZKV * zkv, char *key);
	int (*update_fn) (ZKV * zkv, char *key, char *value);
	int (*query_fn) (ZKV * zkv, char *query, char **result);
	void (*free_fn) (ZKV * zkv);
};

ZKV *zkv_create(void);
void zkv_free(ZKV * zkv);
static inline int zkv_lookup(ZKV * zkv, char *key, char **value)
{
	return zkv->lookup_fn(zkv, key, value);
}

static inline int zkv_delete(ZKV * zkv, char *key)
{
	return zkv->delete_fn(zkv, key);
}

static inline int zkv_update(ZKV * zkv, char *key, char *value)
{
	return zkv->update_fn(zkv, key, value);
}

static inline int zkv_query(ZKV * zkv, char *query, char **result)
{
	return zkv->query_fn(zkv, query, result);
}

/* SDLIST */
struct ZSDLIST {
	ZSDATA *list;
	int len;
	int size;
	int ST;
};
ZSDLIST *zsdlist_create(int size);
void zsdlist_free(ZSDLIST * sdl);
int zsdlist_add(ZSDLIST * sdl, void *data, int size);
void zsdlist_terminate(ZSDLIST * sdl);
void zsdlist_reset(ZSDLIST * sdl);
int zsdlist_parse_sizedata(ZSDLIST * sdl, void *data, int size);
int zsdlist_escape(ZSDLIST * sdl, ZBUF * zb);
#define ZSTACK_SDLIST(sdl, s) \
       	ZSDLIST sdl ## __STACK__BUF, * sdl; sdl = & sdl ## __STACK__BUF; \
	ZSDATA sdl ## DATA__STACK_BUF[s]; sdl->list = sdl ## DATA__STACK_BUF; \
	sdl->size = s; sdl->len = 0; sdl->ST = 1;

#define ZSDLIST_WALK_BEGIN(sdl, d, s)  { \
	int i_sdl_AFAFD; ZSDATA *sd_sdl_AFAF; for(i_sdl_AFAFD =0;i_sdl_AFAFD < (sdl)->len;i_sdl_AFAFD++){ \
		sd_sdl_AFAF = (sdl)->list+i_sdl_AFAFD; d=sd_sdl_AFAF->data;s=sd_sdl_AFAF->size; {
#define ZSDLIST_WALK_END }}}

/* OVER */

#ifdef ZC_INUSE
#include "src/zc_inuse.h"
#endif

#endif
