#ifndef ___ZYC_INCLUDE__
#define ___ZYC_INCLUDE__
#define _GNU_SOURCE

#ifdef ZYC_INUSE

#include <arpa/inet.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <db.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <iconv.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <time.h>
#include <unistd.h>
#include <ifaddrs.h>

#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define ZERROR_TIMEOUT			(-2)
/* vars */
extern int z_errno;
extern char *zvar_program_name;
extern int zvar_ipc_timeout;
extern int zvar_daemon_mode;

typedef struct ZSTR ZSTR;
#define ZTRUE 	1
#define ZFALSE	0
#define Z_STREMPTY(ptr)	((!ptr)||!((ptr)[0]))
#define Z_STRNONEMPTY(ptr)	((ptr) && ((ptr)[0]))
#define Z_CHAR_PTR_TO_INT(cptr)      ((int) (long) (cptr))
#define Z_INT_TO_CHAR_PTR(ival)      ((char *) (long) (ival))

/* string utils */
void *z_malloc(ssize_t len);
void *z_realloc(char *ptr, ssize_t len);
void z_free(void *ptr);

char *z_strdup(const char *ptr);
char *z_strndup(const char *ptr,size_t n);
char *z_memdup(const char *ptr, size_t n);

typedef struct {
	char *sstr;
	char *str;
	int len;
} ZSTRTOK;
void z_strtok_create(ZSTRTOK *k,  char *sstr);
ZSTRTOK *z_strtok(ZSTRTOK *k, const char *delim);

int z_fget_delimiter(FILE *fp, ZSTR *zs, char delimiter);
int z_fget_delimiter(FILE *fp, ZSTR *zs, char delimiter);
int z_fget_delimiter_bound(FILE *fp, ZSTR *zs, char delimiter, int bound);

/* RAND */
int z_rand(void);

/* realpath */
char *z_pathjoin(char *path1, char *path2);

/* mkdir */
int z_mkdir(const char *path);

/* STRCASE */
char *z_uppercase(char *str);
char *z_lowercase(char *str);

/* STR */
typedef struct ZSTR ZSTR;
typedef int (*ZSTR_PUT_READY_FN) (ZSTR *);
struct ZSTR {
	char *str;
	int len;
	int size;
#if 0
	int (*put_ready) (ZSTR *);
	int (*get_ready) (ZSTR *);
#endif
};

ZSTR * zstr_create(int len);
void zstr_free(ZSTR *zs);
inline void zstr_space(ZSTR *zs, int space);
inline void zstr_put(ZSTR *zs, int ch);
void zstr_truncate(ZSTR *zs, int len);
ZSTR *zstr_strncpy(ZSTR *zs, char *src, int len);
ZSTR *zstr_strcpy(ZSTR *zs, char *src);
ZSTR *zstr_strncat(ZSTR *zs, char *src, int len);
ZSTR *zstr_strcat(ZSTR *zs, char *src);
ZSTR *zstr_strcats(ZSTR *zs, ...);
ZSTR *zstr_strcpys(ZSTR *zs, ...);
ZSTR *zstr_memcpy(ZSTR *zs, char *src, int len);
ZSTR *zstr_memcat(ZSTR *zs, char *src, int len);
int zstr_sprintf_append(ZSTR *zs, char *fmt, ...);
int zstr_vsprintf_append(ZSTR *zs, char *format, va_list ap);
int zstr_snprintf_append(ZSTR *zs, int len, char *fmt, ...);
int zstr_vsnprintf_append(ZSTR *zs, int len, char *fmt, va_list ap);
int zstr_sprintf(ZSTR *zs, char *fmt, ...);
int zstr_snprintf(ZSTR *zs, int len, char *fmt, ...);
inline int zstr_vsprintf(ZSTR *zs, char *format, va_list ap);
inline int zstr_vsnprintf(ZSTR *zs, int len, char *fmt, va_list ap);
ZSTR *zstr_fconcatenate(char *fmt, ...);
ZSTR *zstr_concatenate(char *src0, ...);

#define zstr_str(zs) 		((zs)->str)
#define zstr_len(zs) 		((zs)->len)
#define ZSTR_STR		zstr_str
#define ZSTR_LEN		zstr_len
#define ZSTR_PUT		zstr_put
#define ZSTR_SPACE		zstr_space
#define zstr_terminate(zs)	((zs)->str[(zs)->len]=0)
#define zstr_reset(zs)		((zs)->str[0]=0,(zs)->len=0)

/* MSG or SYSLOG */
extern int zvar_debug;
extern int zvar_verbose;
typedef void (*ZMSG_OUTPUT_FN)(int, int, char*, va_list);
void zmsg_set_output(ZMSG_OUTPUT_FN fn);
void zmsg_fatal_printf(int level, int who, char *fmt, ...);
void zmsg_printf(int level, int is_zyclib, char *fmt, ...);
void zmsg_vprintf(int level, int is_zyclib, char *fmt, va_list ap);
#ifdef ZYC_INUSE
#define ZYC_ZMSG_WHO 1
#else
#define ZYC_ZMSG_WHO 0
#endif

#define zmsg_fatal(fmt, args...)	zmsg_fatal_printf(LOG_CRIT, ZYC_ZMSG_WHO, (fmt), ##args)
#define zmsg_err(fmt, args...)		zmsg_printf(LOG_ERR, ZYC_ZMSG_WHO, (fmt), ##args)
#define zmsg_warning(fmt, args...)	zmsg_printf(LOG_WARNING, ZYC_ZMSG_WHO, (fmt), ##args)
#define zmsg_info(fmt, args...)		zmsg_printf(LOG_INFO, ZYC_ZMSG_WHO, (fmt), ##args)
#define zmsg_debug(fmt, args...)	((zvar_debug||zvar_verbose)?(zmsg_printf(LOG_DEBUG, ZYC_ZMSG_WHO, (fmt), ##args)):0)
#define zmsg_verbose(fmt, args...)	(zvar_verbose?(zmsg_printf(LOG_DEBUG, ZYC_ZMSG_WHO, (fmt), ##args)):0)

#ifdef ZYC_INUSE
#ifndef ZYC_DEV
#undef zmsg_debug
#undef zmsg_verbose
#define zmsg_debug(fmt, args...)
#define zmsg_verbose(fmt, args...)
#endif
#endif

#define zmsg_error				zmsg_err
#define zmsg_warn				zmsg_warning

/* ARGV ARGC */
typedef struct{
	ssize_t len;
	ssize_t argc;
	char  **argv;
} ZARGV;

#define ZARGV_END     ((char *) 0)
#define ZARGV_LEN(argvp) ((argvp)->argc) 
#define zargv_len(argvp) ((argvp)->argc) 

ZARGV *zargv_free(ZARGV * argvp, void(*free_fn)(void *, void *), char *ptr);
ZARGV *zargv_create(int len);
void zargv_add(ZARGV * argvp, char *ns);
void zargv_adds(ZARGV * argvp, ...);
void zargv_addn(ZARGV * argvp, char *ns, int len);
void zargv_truncate(ZARGV * argvp, ssize_t len);
ZARGV *zargv_split(const char *string, const char *delim);
ZARGV *zargv_split_append(ZARGV * argvp, const char *string, const char *delim);
void zargv_add_node(ZARGV * argvp, char *node);

#define zargv_reset(za) (zargv_truncate((za), 0))

/* CHAIN */
typedef struct ZCHAIN ZCHAIN;
typedef struct ZCHAIN_NODE ZCHAIN_NODE;
struct ZCHAIN_NODE{
	char *value;
	ZCHAIN_NODE *prev;
	ZCHAIN_NODE *next;
};
struct ZCHAIN{
	ZCHAIN_NODE *head;
	ZCHAIN_NODE *tail;
	int len;
};
ZCHAIN *zchain_create(void);
void zchain_free(ZCHAIN *zc, void(*free_fn)(void *, char *), char *ptr);
ZCHAIN_NODE *zchain_insert_before(ZCHAIN *zc, char *value, ZCHAIN_NODE *before);
ZCHAIN_NODE *zchain_push(ZCHAIN *zc, char *value);
ZCHAIN_NODE *zchain_unshift(ZCHAIN *zc, char *value);
ZCHAIN_NODE *zchain_detach(ZCHAIN *zc, ZCHAIN_NODE *n);
ZCHAIN_NODE *zchain_remove(ZCHAIN *zc, ZCHAIN_NODE *n);
ZCHAIN_NODE *zchain_pop(ZCHAIN *zc, char **value);
ZCHAIN_NODE *zchain_shift(ZCHAIN *zc, char **value);

#define ZCHAIN_WALK(zc, n)	for(n=(zc)->loop->next;n!=(zc)->loop,n=>n->next)
#define ZCHAIN_FIRST(zc)	((zc->loop->next!=z->loop?zc->loop->next:0)
#define ZCHAIN_LAST(zc)		((zc->loop->prev!=z->loop?zc->loop->prev:0)
/* HASH */
typedef struct ZHASH ZHASH;
typedef struct ZHASH_NODE ZHASH_NODE;
struct ZHASH_NODE {
	char	*key;
	char	*value;
	ZHASH_NODE *next;
	ZHASH_NODE *prev;
};

struct ZHASH {
	int	size;
	int	len;
	ZHASH_NODE **data;
};


ZHASH *zhash_create(int size);
ZHASH_NODE *zhash_enter(ZHASH *table, const char *key, char *value);
ZHASH_NODE *zhash_enter_unique(ZHASH *table, const char *key, char *value, char **old_value);
ZHASH_NODE *zhash_find(ZHASH *table, const char *key, char **value);
ZHASH_NODE *zhash_delete(ZHASH *table, const char *key, char **value);
void zhash_delete_node(ZHASH *table, ZHASH_NODE *node, char **value);
void zhash_free(ZHASH *table, void (*free_fn) (char *,char*), char *ptr);
void zhash_walk(ZHASH *table, void (*walk_fn) (ZHASH_NODE *, char *), char *ptr) ;
ZHASH_NODE **zhash_list(ZHASH *table);



/* IO */
#define zio_set_block(fd) zio_non_blocking(fd, 0)
#define zio_set_nonblock(fd) zio_non_blocking(fd, 1)
#define zio_readable(fd) zio_readable_or_writeable(fd, 1)
#define zio_writeable(fd) zio_readable_or_writeable(fd, 0)
int zio_read_wait(int fd, int timeout);
ssize_t zio_timed_read(int fd, void *buf, size_t len, int timeout);
ssize_t zio_timed_strict_read(int fd, void *buf, size_t len, int timeout);
int zio_write_wait(int fd, int timeout);
ssize_t zio_timed_write(int fd, void *buf, size_t len, int timeout);
ssize_t zio_timed_strict_write(int fd, void *buf, size_t len, int timeout);
int zio_non_blocking(int fd, int on);
int zio_close_on_exec(int fd, int on);
ssize_t zio_peek(int fd);
int zio_readable_or_writeable(int fd, int rw);




/* STREAM */
#define ZSTREAM_RBUF_SIZE	1024
#define ZSTREAM_WBUF_SIZE	1024
typedef struct {
	int fd;
	int flag;
	int error;
	int timeout;
	char rbuf[ZSTREAM_RBUF_SIZE+6];
	int rbuf_start;
	int rbuf_len;
	char wbuf[ZSTREAM_WBUF_SIZE+6];
	int wbuf_len;
}ZSTREAM;


ZSTREAM * zstream_fdopen(int fd, int timeout);
int zstream_fclose(ZSTREAM *zsm);
int zstream_fdclose(ZSTREAM *zsm);
ZSTREAM * zstream_fopen(const char *path, int flag, ...);
int zstream_fflush(ZSTREAM *zsm);
inline int zstream_get_char(ZSTREAM *zsm);
int zstream_read(ZSTREAM *zsm, ZSTR *zbuf, int len);
int zstream_write(ZSTREAM *zsm, char *buf, int len);
int zstream_get_delimiter(ZSTREAM *zsm, ZSTR *zs, char delimiter);
int zstream_get_nodelimiter(ZSTREAM *zsm, ZSTR *zs, char delimiter);
int zstream_get_delimiter_bound(ZSTREAM *zsm, ZSTR *zs, char delimiter, int bound);
int zstream_get_nodelimiter_bound(ZSTREAM *zsm, ZSTR *zs, char delimiter, int bound);
inline int zstream_put_char(ZSTREAM *zsm, int inch);
int zstream_fputs(char *buf, ZSTREAM *zsm);
int zstream_fnprintf(ZSTREAM *zsm, int len, char *fmt, ...);
int zstream_vfnprintf(ZSTREAM *zsm, int len, char *fmt, va_list ap);
int zstream_vfprintf(ZSTREAM *zsm, char *format, va_list ap);
int zstream_fprintf(ZSTREAM *zsm, char *fmt, ...);

#define zstream_set_timeout(zsm, to)	((zsm)->timeout=(to))
#define zstream_set_flush(zsm, to)	((zsm)->timeout=(to))

#define zstream_ferror(zsm)	((zsm)->error)
#define zstream_feof(zsm)	((zsm)->error)
#define zstream_fileno(zsm)	((zsm)->fd)

#define ZSTREAM_EOF 		(-1)
#define ZSTREAM_ERROR 		(-1)

#define zstream_rbuf_left(zsm)	((zsm)->rbuf_len - (zsm)->rbuf_start)
#define ZSTREAM_GET_CHAR(zsm)		(((zsm)->rbuf_start==(zsm)->rbuf_len)?zstream_get_char(zsm):(unsigned char)(zsm->rbuf[zsm->rbuf_start++]))
#define ZSTREAM_GET		ZSTREAM_GET_CHAR
#define ZSTREAM_PUT		ZSTREAM_PUT_CHAR
#define ZSTREAM_PUT_CHAR(zsm, inch)	\
       	(((zsm)->wbuf_len==ZSTREAM_WBUF_SIZE)?zstream_put_char(zsm, inch):(unsigned char)((zsm)->wbuf[zsm->wbuf_len++]=(inch)))
#define ZSTREAM_FFLUSH		zstream_fflush

#define zstream_get_nl(zsm, zs)		(zstream_get_delimiter(zsm, zs, '\n'))
#define zstream_get_nonl(zsm, zs)	(zstream_get_nodelimiter(zsm, zs, '\n'))
#define zstream_get_null(zsm, zs)	(zstream_get_delimiter(zsm, zs, '\0'))
#define zstream_get_nonull(zsm, zs)	(zstream_get_nodelimiter(zsm, zs, '\0'))


/* ATTR */

typedef int (*ZATTR_PRINT_FN) (ZSTREAM *, char *);
#define ZATTR_END	0
#define ZATTR_CHAR	1
#define ZATTR_INT	2
#define ZATTR_LONG	3
#define ZATTR_FLOAT	4
#define ZATTR_DOUBLE	5
#define ZATTR_STR	6
#define ZATTR_DATA	7
#define ZATTR_ZARGV	8
#define ZATTR_FUNC	9

int zattr_fprint(ZSTREAM *zsm, ...);
int zattr_vfprint(ZSTREAM *zsm, va_list ap);
int zattr_fscan(ZSTREAM *zsm, ...);
int zattr_vfscan(ZSTREAM *zsm, va_list ap);


/* ADDR */
ZARGV *zaddr_get_addr_r(char *host, ZARGV *hza);
ZARGV *zaddr_get_localaddr_r(ZARGV *hza);
#define zaddr_get_addr(host)		(zaddr_get_addr_r((host), 0))
#define zaddr_get_localaddr()	(zaddr_get_localaddr_r(0))

/* SOCKET */
int zsocket_sane_connect(int sock, struct sockaddr * sa, socklen_t len);
int zsocket_timed_connect(int sock, struct sockaddr * sa, int len, int timeout);
int zsocket_unix_connect(const char *addr, int timeout);
int zsocket_inet_connect(char *dip, int port, int timeout);
int zsocket_unix_listen(const char *addr, int backlog);
int zsocket_inet_listen(char *sip, int port, int backlog);
int zsocket_sane_accept(int sock, struct sockaddr * sa, socklen_t *len);
int zsocket_unix_accept(int fd);
int zsocket_inet_accept(int fd);
int zsocket_inet_connect_host(char *host, int port, int timeout);
int zsocket_fifo_listen(const char *path);


 

/* DICT */
/*
 * FIXME
 * IGNORE ERROR
 */
typedef struct ZDICT ZDICT;
struct ZDICT {
	void *info;
	int (*lookup) (ZDICT *, char *);
};

#define zdict_lookup(zd,name)	((zd)->lookup(zd, name))
#define zdict_find		zdict_lookup

#define ZMAPS ZARGV
#define zmaps_free(zm)	(zargv_free(zm, 0, 0))
#define zmaps_find	zmaps_lookup

#define ZMAPS_LOOKUP_ERROR 			(-1)
#define ZMAPS_LOOKUP_SKIP_AFTER_ERROR		(1<<8)
#define ZMAPS_LOOKUP_RETURN_AFTER_ERROR		(1<<9)
#define ZMAPS_LOOKUP_UPPERCASE			(1<<10)
#define ZMAPS_LOOKUP_LOWERCASE			(1<<11)
extern ZSTR *zvar_maps_query;
extern ZSTR *zvar_maps_result;
extern ZSTR *zvar_maps_status;

int zdict_register(char *describe);
int zmaps_lookup(ZMAPS *zm, char *name, int flag);
ZMAPS * zmaps_create(char *str);
ZDICT * zdict_locate_dict(char *describe);

/* hash, btree */
extern int zvar_dict_bdb_cache;
int zdict_hash_create(char *describe, char *path, ZDICT *dict);
int zdict_btree_create(char *describe, char *path, ZDICT *dict);

/* static */
int zdict_static_create(char *describe, char *path, ZDICT *dict);

/* inet */
extern int zvar_dict_inet_try_limit;
extern int zvar_dict_inet_timeout;
extern int zvar_dict_unix_try_limit;
extern int zvar_dict_unix_timeout;
int zdict_inet_create(char *describe, char *path, ZDICT *dict);
int zdict_unix_create(char *describe, char *path, ZDICT *dict);

/* proxy */
int zdict_proxy_create(char *describe, char *path, ZDICT *dict);
extern int zvar_dict_proxy_try_limit;
extern int zvar_dict_proxy_timeout;
extern char *zvar_dict_proxy_path;

/* CONFIG */
#define ZCONFIG ZHASH
extern ZCONFIG *z_global_config;
typedef struct {
	char *name;
	char *defval;
	char **target;
}ZCONFIG_STR;
typedef struct {
	char *name;
	int defval;
	int *target;
}ZCONFIG_INT;
#define ZCONFIG_BOOL ZCONFIG_INT
typedef struct {
	char *name;
	char *defval;
	int *target;
}ZCONFIG_TIME;

ZCONFIG *zconfig_create(void);
void zconfig_free(ZCONFIG *zc);
void zconfig_show(ZCONFIG *zc);
int zconfig_split_nameval(char *buf, char **name, char **value);
int zconfig_load(ZCONFIG *zconfig, char *path, int flag);
void zconfig_update(ZCONFIG *zc, char *name, char *value);
char *zconfig_get_str(ZCONFIG *zc, char *name, char *def);
int zconfig_get_bool(ZCONFIG *zc, char *name, int def);
int zconfig_get_int(ZCONFIG *zc, char *name, int def);
int zconfig_get_time(ZCONFIG *zc, char *name, char *def);
void zconfig_str_table(ZCONFIG *zc, ZCONFIG_STR *t);
void zconfig_int_table(ZCONFIG *zc, ZCONFIG_INT *t);
void zconfig_bool_table(ZCONFIG *zc, ZCONFIG_BOOL *t);
void zconfig_time_table(ZCONFIG *zc, ZCONFIG_TIME *t);

#define zgconfig_create() 		(z_global_config?z_global_config:(z_global_config=zconfig_create()))
#define zgconfig_init			zgconfig_create
#define zgconfig_show() 		zconfig_show(z_global_config)
#define zgconfig_free() 		zconfig_free(z_global_config)
#define zgconfig_load(path, flag) 	zconfig_load(z_global_config, path, flag)
#define zgconfig_update(name, value) 	zconfig_update(z_global_config, name, value)
#define zgconfig_get_str(name, def) 	zconfig_get_str(z_global_config, name, def)
#define zgconfig_get_bool(name, def) 	zconfig_get_bool(z_global_config, name, def)
#define zgconfig_get_int(name, def) 	zconfig_get_int(z_global_config, name, def)
#define zgconfig_get_time(name, def) 	zconfig_get_time(z_global_config, name, def)
#define zgconfig_str_table(t) 		zconfig_str_table(z_global_config, t)
#define zgconfig_int_table(t) 		zconfig_int_table(z_global_config, t)
#define zgconfig_bool_table(t) 		zconfig_bool_table(z_global_config, t)
#define zgconfig_time_table(t) 		zconfig_time_table(z_global_config, t)

/* EVENT */
#define ZEVENT_READ		(1)
#define ZEVENT_WRITE		(1<<1)
#define ZEVENT_RDWR		(ZEVENT_READ|ZEVENT_WRITE)
#define ZEVENT_TIMER		(1<<3)
#define ZEVENT_EXCEPTION	(1<<4)
#define ZEVENT_ERROR		ZEVENT_EXCEPTION
#define ZEVENT_ERR		ZEVENT_EXCEPTION

#define ZEVENT_IS_READ(ev)	((ev) & ZEVENT_READ)
#define ZEVENT_IS_WRITE(ev)	((ev) & ZEVENT_WRITE)
#define ZEVENT_IS_TIMER(ev)	((ev) & ZEVENT_TIMER)
#define ZEVENT_IS_EXCEPTION(ev)	((ev) & ZEVENT_EXCEPTION)
#define ZEVENT_IS_ERROR		ZEVENT_IS_EXCEPTION
#define ZEVENT_IS_ERR		ZEVENT_IS_EXCEPTION

typedef int (*ZEVENT_CALLBACK_FN) (int, int, char *);
#define ZEVENT_TIMER_CALLBACK_FN ZEVENT_CALLBACK_FN
int zevent_init(void);
int zevent_close(void);
int zevent_enable(int fd, ZEVENT_CALLBACK_FN callback, char *context, int flag);
int zevent_disable(int fd);
int zevent_loop(int timeout);
time_t zevent_request_timer(int fd, ZEVENT_TIMER_CALLBACK_FN callback, char *context, int delay);
time_t zevent_cancel_timer(int fd, ZEVENT_TIMER_CALLBACK_FN callback, char *context);
#define zevent_enable_read(fd, cb, ctx)		(zevent_enable(fd, cb, ctx, ZEVENT_READ))
#define zevent_enable_write(fd, cb, ctx)	(zevent_enable(fd, cb, ctx, ZEVENT_WRITE))
#define zevent_enable_readwrite(fd, cb, ctx)	(zevent_enable(fd, cb, ctx, ZEVENT_RDWR))
#define zevent_enable_readonly			zevent_enable_read
#define zevent_enable_writeonly			zevent_enable_write
#define zevent_disable_readwrite		zevent_disable

/* MASTER */
#ifdef  ZYC_INUSE
#define ZMASTER_STATUS_FD  3
#define ZMASTER_LISTEN_FD  4
#endif
int zmaster_server(int argc, char **argv);
int zmaster_proxymap(int argc, char **argv);


int zmaster_client_server(int, char **, int, void (*)(void), void(*)(int, ZSTREAM *));
int zmaster_single_server(int argc, char **argv, void (*service_pre)(void), void(*service)(int fd, ZSTREAM *stream));
/* zmaster_multi_server mode :
 * first of all, client must send msg first.
 * mainly used for inner.
 */
int zmaster_multi_server(int argc, char **argv, void (*service_pre)(void), void(*service)(int fd, ZSTREAM *stream));
void zmaster_multi_server_disconnect(int fd, ZSTREAM *zsm);


#endif
