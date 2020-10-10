/*
 * ================================
 * eli960@qq.com
 * https://linuxmail.cn/
 * 2017-06-26
 * ================================
 */

#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wpacked-not-aligned"

# include "./coroutine.h"

#ifndef __x86_64__
#include <ucontext.h>
#endif

/* {{{ include */
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
/* }}} */

/* {{{ macro */

#define zvar_coroutine_max_timeout_millisecond (3600L * 24 * 365 * 10 * 1000)

#define ZEMPTY(str)                  (!(str)||!(*((const char *)str)))

#define ZCONTAINER_OF(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))

#define zinline inline __attribute__((always_inline))

#define zcoroutine_fatal(fmt, args...) { \
    fprintf(stderr, "FATAL "fmt, ##args); \
    fprintf(stderr, " [%s:%u]\n", __FILE__, __LINE__);\
    fflush(stderr); \
    _exit(1); \
}

#define zpthread_lock(l)    {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zcoroutine_fatal("mutex:%m");}}
#define zpthread_unlock(l)  {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zcoroutine_fatal("mutex:%m");}}

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

/* }}} */

#pragma pack(push, 4)

#ifdef  __cplusplus
extern "C" {
#endif

/* {{{ type convert */

typedef union _co_type_convert_t _co_type_convert_t;
union _co_type_convert_t {
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

/* }}} */

/* {{{ malloc */

static void *_co_mem_malloc(int len)
{
    void *r;

    if (len < 0) {
        len = 0;
    }
    if ((r = malloc(len)) == 0) {
        zcoroutine_fatal("_co_mem_malloc: insufficient memory for %d bytes: %m", len);
    }

    return r;
}

static void *_co_mem_calloc(int nmemb, int size)
{
    void *r;

    if (nmemb < 0) {
        nmemb = 0;
    }
    if (size < 0) {
        size = 0;
    }
    if ((r = calloc(nmemb, size)) == 0) {
        zcoroutine_fatal("_co_mem_calloc: insufficient memory for %dx%d bytes: %m", nmemb, size);
    }

    return r;
}

static void _co_mem_free(const void *ptr)
{
    if (ptr) {
        free((void *)ptr);
    }
}
/* }}} */

/* {{{ syscall */
zinline static int _syscall_pipe(int pipefd[2])
{
#ifdef __NR_pipe
    return syscall(__NR_pipe, pipefd);
#else
    return syscall(__NR_pipe2, pipefd, 0);
#endif
}

zinline static int _syscall_pipe2(int pipefd[2], int flags)
{
    return syscall(__NR_pipe2, pipefd, flags);
}

zinline static int _syscall_dup(int oldfd)
{
    return syscall(__NR_dup, oldfd);
}

zinline static int _syscall_dup2(int oldfd, int newfd)
{
#ifdef __NR_dup2
    return syscall(__NR_dup2, oldfd, newfd);
#else
    return syscall(__NR_dup3, oldfd, newfd, 0);
#endif
}

zinline static int _syscall_dup3(int oldfd, int newfd, int flags)
{
    return syscall(__NR_dup3, oldfd, newfd, flags);
}

zinline static int _syscall_socketpair(int domain, int type, int protocol, int sv[2])
{
    return syscall(__NR_socketpair, domain, type, protocol, sv);
}

zinline static int _syscall_socket(int domain, int type, int protocol)
{
    return syscall(__NR_socket, domain, type, protocol);
}

zinline static int _syscall_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    return syscall(__NR_accept, fd, addr, len);
}

zinline static int _syscall_connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return syscall(__NR_connect, socket, address, address_len);
}

zinline static int _syscall_close(int fd)
{
    return syscall(__NR_close, fd);
}

zinline static ssize_t _syscall_read(int fildes, void *buf, size_t nbyte)
{
    return syscall(__NR_read, fildes, buf, nbyte);
}

zinline static ssize_t _syscall_readv(int fd, const struct iovec *iov, int iovcnt)
{
    return syscall(__NR_readv, fd, iov, iovcnt);
}

zinline static ssize_t _syscall_write(int fildes, const void *buf, size_t nbyte)
{
    return syscall(__NR_write, fildes, buf, nbyte);
}

zinline static ssize_t _syscall_writev(int fd, const struct iovec *iov, int iovcnt)
{
    return syscall(__NR_writev, fd, iov, iovcnt);
}

zinline static ssize_t _syscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    return syscall(__NR_sendto, socket, message, length, flags, dest_addr, dest_len);
}

zinline static ssize_t _syscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, address, address_len);
}

zinline static size_t _syscall_send(int socket, const void *buffer, size_t length, int flags)
{
    return syscall(__NR_sendto, socket, buffer, length, flags, 0, 0);
}

zinline static ssize_t _syscall_recv(int socket, void *buffer, size_t length, int flags)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, 0, 0);
}

zinline static int _syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
#ifdef __NR_poll
    return syscall(__NR_poll, fds, nfds, timeout);
#else
    return syscall(__NR_ppoll, fds, nfds, 0, 0);
#endif
}

zinline static int _syscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return syscall(__NR_setsockopt, socket, level, option_name, option_value, option_len);
}

static int _syscall_fcntl(int fildes, int cmd, ...)
{
    int ret = -1;
    va_list args;
    va_start(args,cmd);
    switch(cmd)
    {
        case F_DUPFD:
        case F_SETFD:
        case F_SETFL:
        case F_SETOWN:
            {
                int param = va_arg(args,int);
                ret = syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
            {
                ret = syscall(__NR_fcntl, fildes, cmd);
                break;
            }
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            {
                /* struct flock *param = va_arg(args,struct flock *); */
                void *param = va_arg(args, void *);
                ret = syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
    }
    va_end(args);
    return ret;
}

zinline static pid_t _syscall_gettid(void)
{
    return syscall(__NR_gettid);
}

static int _syscall_open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
#ifdef __NR_open
    return syscall(__NR_open, pathname, flags, mode);
#else
    return syscall(__NR_openat, AT_FDCWD, pathname, flags, mode);
#endif
}

static int _syscall_openat(int dirfd, const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return syscall(__NR_openat, dirfd, pathname, flags, mode);
}

zinline static int _syscall_creat(const char *pathname, mode_t mode)
{
#ifdef __NR_creat
    return syscall(__NR_creat, pathname, mode);
#else
    return syscall(__NR_openat, AT_FDCWD, pathname, O_WRONLY|O_CREAT|O_TRUNC, mode);
#endif
}

zinline static off_t _syscall_lseek(int fd, off_t offset, int whence)
{
    return syscall(__NR_lseek, fd, offset, whence);
}

zinline static int _syscall_fdatasync(int fd)
{
    return syscall(__NR_fdatasync, fd);
}

zinline static int _syscall_fsync(int fd)
{
    return syscall(__NR_fsync, fd);
}

zinline static int _syscall_rename(const char *oldpath, const char *newpath)
{
#ifdef __NR_rename
    return syscall(__NR_rename, oldpath, newpath);
#else
    return syscall(__NR_renameat, AT_FDCWD, oldpath, AT_FDCWD, newpath);
#endif
}

zinline static int _syscall_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
    return syscall(__NR_renameat, olddirfd, oldpath, newdirfd, newpath);
}

zinline static int _syscall_truncate(const char *path, off_t length)
{
    return syscall(__NR_truncate, path, length);
}

zinline static int _syscall_ftruncate(int fd, off_t length)
{
    return syscall(__NR_truncate, fd, length);
}

zinline static int _syscall_rmdir(const char *pathname)
{
#ifdef __NR_rmdir
    return syscall(__NR_rmdir, pathname);
#else
    return syscall(__NR_unlinkat, AT_FDCWD, pathname, AT_REMOVEDIR);
#endif
}

zinline static int _syscall_mkdir(const char *pathname, mode_t mode)
{
#ifdef mkdir
    return syscall(__NR_mkdir, pathname, mode);
#else
    return syscall(__NR_mkdirat, AT_FDCWD, pathname, mode);
#endif
}

zinline static int _syscall_mkdirat(int dirfd, const char *pathname, mode_t mode)
{
    return syscall(__NR_mkdirat, dirfd, pathname, mode);
}

#ifdef __NR_getdents
zinline static int _syscall_getdents(unsigned int fd, void *dirp, unsigned int count)
{
    return syscall(__NR_getdents, fd, dirp, count);
}
#endif

#ifdef __NR_stat
zinline static int _syscall_stat(const char *pathname, struct stat *buf)
{
    return syscall(__NR_stat, pathname, buf);
}
#endif

zinline static int _syscall_fstat(int fd, struct stat *buf)
{
    return syscall(__NR_fstat, fd, buf);
}

#ifdef __NR_lstat
zinline static int _syscall_lstat(const char *pathname, struct stat *buf)
{
    return syscall(__NR_lstat, pathname, buf);
}
#endif

zinline static int _syscall_link(const char *oldpath, const char *newpath)
{
#ifdef __NR_link
    return syscall(__NR_link, oldpath, newpath);
#else
    return syscall(__NR_linkat, AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
#endif
}

zinline static int _syscall_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    return syscall(__NR_linkat, olddirfd, oldpath, newdirfd, newpath, flags);
}

zinline static int _syscall_symlink(const char *target, const char *linkpath)
{
#ifdef __NR_symlink
    return syscall(__NR_symlink, target, linkpath);
#else
    return syscall(__NR_symlinkat, target, AT_FDCWD, linkpath);
#endif
}

zinline static int _syscall_symlinkat(const char *target, int newdirfd, const char *linkpath)
{
    return syscall(__NR_symlinkat, target, newdirfd, linkpath);
}

zinline static ssize_t _syscall_readlink(const char *pathname, char *buf, size_t bufsiz)
{
#ifdef __NR_readlink
    return syscall(__NR_readlink, pathname, buf, bufsiz);
#else
    return syscall(__NR_readlinkat, AT_FDCWD, pathname, buf, bufsiz);
#endif
}

zinline static ssize_t _syscall_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
    return syscall(__NR_readlinkat, dirfd, pathname, buf, bufsiz);
}

zinline static int _syscall_unlink(const char *pathname)
{
#ifdef __NR_unlink
    return syscall(__NR_unlink, pathname);
#else
    return syscall(__NR_unlinkat, AT_FDCWD, pathname);
#endif
}

zinline static int _syscall_unlinkat(int dirfd, const char *pathname, int flags)
{
    return syscall(__NR_unlinkat, dirfd, pathname);
}

#ifdef __NR_chmod
zinline static int _syscall_chmod(const char *pathname, mode_t mode)
{
    return syscall(__NR_chmod, pathname, mode);
}
#endif

zinline static int _syscall_fchmod(int fd, mode_t mode)
{
    return syscall(__NR_fchmod, fd, mode);
}

#ifdef __NR_chown
zinline static int _syscall_chown(const char *pathname, uid_t owner, gid_t group)
{
    return syscall(__NR_chown, pathname, owner, group);
}
#endif

zinline static int _syscall_fchown(int fd, uid_t owner, gid_t group)
{
    return syscall(__NR_fchown, fd, owner, group);
}

#ifdef __NR_lchown
zinline static int _syscall_lchown(const char *pathname, uid_t owner, gid_t group)
{
    return syscall(__NR_lchown, pathname, owner, group);
}
#endif

#ifdef __NR_utime
zinline static int _syscall_utime(const char *filename, const struct utimbuf *times)
{
    return syscall(__NR_utime, filename, times);
}
#endif

#ifdef __NR_utimes
zinline static int _syscall_utimes(const char *filename, const struct timeval times[2])
{
    return syscall(__NR_utimes, filename, times);
}
#endif

#if 0
int _syscall_futimes(int fd, const struct timeval tv[2])
{
    return syscall(__NR_futimes, fd, tv);
}

int _syscall_lutimes(const char *filename, const struct timeval tv[2])
{
    return syscall(__NR_lutimes, filename, tv);
}
#endif

static int zrobust_syscall_close(int fd) {
    int ret;
    do {
        ret = _syscall_close(fd);
    } while((ret<0) && (errno==EINTR));
    return ret;
}

/* }}} */

/* {{{ list */
typedef struct _co_list_t _co_list_t;
typedef struct _co_list_node_t _co_list_node_t;
struct _co_list_t {
    _co_list_node_t *head;
    _co_list_node_t *tail;
    int len;
};
struct _co_list_node_t {
    _co_list_node_t *prev;
    _co_list_node_t *next;
    void *value;
};

static  _co_list_t *_co_list_create(void);
static void _co_list_free(_co_list_t *list);
zinline static int _co_list_len(const _co_list_t *list) { return list->len; }
zinline static _co_list_node_t *_co_list_head(const _co_list_t *list) { return list->head; }

zinline static _co_list_node_t *_co_list_tail(const _co_list_t *list) { return list->tail; }
zinline static _co_list_node_t *_co_list_node_next(const _co_list_node_t *node) { return node->next; }
zinline static _co_list_node_t *_co_list_node_prev(const _co_list_node_t *node) { return node->prev; }
zinline static void *_co_list_node_value(const _co_list_node_t *node) { return node->value; }
static void _co_list_attach_before(_co_list_t *list, _co_list_node_t *n, _co_list_node_t *before);
static void _co_list_detach(_co_list_t *list, _co_list_node_t *n);
static _co_list_node_t *_co_list_add_before(_co_list_t *list, const void *value, _co_list_node_t *before);
static zbool_t _co_list_delete(_co_list_t *list, _co_list_node_t *n, void **value);
zinline static _co_list_node_t *_co_list_push(_co_list_t *l,const void *v){return _co_list_add_before(l,v,0);}
zinline static _co_list_node_t *_co_list_unshift(_co_list_t *l,const void *v){return _co_list_add_before(l,v,l->head);}
zinline static zbool_t _co_list_pop(_co_list_t *l, void **v){return _co_list_delete(l,l->tail,v);}
zinline static zbool_t _co_list_shift(_co_list_t *l, void **v){return _co_list_delete(l,l->head,v);}

/* 宏, 遍历 */
#define ZCOROUTINE_LIST_WALK_BEGIN(list, var_your_type, var_your_ptr)  { \
    _co_list_node_t *list_current_node=(list)->head; var_your_type var_your_ptr; \
    for(;list_current_node;list_current_node=list_current_node->next){ \
        var_your_ptr = (var_your_type)(void *)(list_current_node->value);
#define ZCOROUTINE_LIST_WALK_END                          }}


static void _co_list_init(_co_list_t *list)
{
    memset(list, 0, sizeof(_co_list_t));
}

static void _co_list_fini(_co_list_t *list)
{
    _co_list_node_t *n, *next;
    if (!list) {
        return;
    }
    n = list->head;
    for (; n; n = next) {
        next = n->next;
        _co_mem_free(n);
    }
}

static _co_list_t *_co_list_create(void)
{
    _co_list_t *list = _co_mem_malloc(sizeof(_co_list_t));
    _co_list_init(list);
    return (list);
}

static void _co_list_free(_co_list_t * list)
{
    _co_list_fini(list);
    _co_mem_free(list);
}

static void _co_list_attach_before(_co_list_t * list, _co_list_node_t * n, _co_list_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(list->head, list->tail, n, prev, next, before);
    list->len++;
}

static void _co_list_detach(_co_list_t * list, _co_list_node_t * n)
{
    ZMLINK_DETACH(list->head, list->tail, n, prev, next);
    list->len--;
}

static _co_list_node_t *_co_list_add_before(_co_list_t * list, const void *value, _co_list_node_t * before)
{
    _co_list_node_t *n;

    n = (_co_list_node_t *) _co_mem_calloc(1, sizeof(_co_list_node_t));
    n->value = (char *)value;
    _co_list_attach_before(list, n, before);

    return n;
}

static int _co_list_delete(_co_list_t * list, _co_list_node_t * n, void **value)
{
    if (n == 0) {
        return 0;
    }
    if (value) {
        *value = n->value;
    }
    _co_list_detach(list, n);
    _co_mem_free(n);

    return 1;
}

/* }}} */

/* {{{ rbtree */
typedef struct _co_rbtree_node_t _co_rbtree_node_t;
typedef struct _co_rbtree_t _co_rbtree_t;
typedef int (*_co_rbtree_cmp_t) (_co_rbtree_node_t *node1, _co_rbtree_node_t *node2);
struct _co_rbtree_t {
    _co_rbtree_node_t *_co_rbtree_node;
    _co_rbtree_cmp_t cmp_fn;
};
struct _co_rbtree_node_t {
    unsigned long ___co_rbtree_parent_color;
    _co_rbtree_node_t *_co_rbtree_right;
    _co_rbtree_node_t *_co_rbtree_left;
    /* The alignment might seem pointless, but allegedly CRIS needs it */
} __attribute__ ((aligned(sizeof(long))));

zinline static int _co_rbtree_have_data(_co_rbtree_t *tree) { return ((tree)->_co_rbtree_node?1:0); }
static void _co_rbtree_init(_co_rbtree_t *tree, _co_rbtree_cmp_t cmp_fn);
static void _co_rbtree_insert_color(_co_rbtree_t *, _co_rbtree_node_t *);
static void _co_rbtree_erase(_co_rbtree_t *tree, _co_rbtree_node_t *node);
static _co_rbtree_node_t *_co_rbtree_next(const _co_rbtree_node_t *tree);
static _co_rbtree_node_t *_co_rbtree_first(const _co_rbtree_t *node);
static _co_rbtree_node_t *_co_rbtree_parent(const _co_rbtree_node_t *node);
static _co_rbtree_node_t *_co_rbtree_attach(_co_rbtree_t *tree, _co_rbtree_node_t *node);
static _co_rbtree_node_t *_co_rbtree_detach(_co_rbtree_t *tree, _co_rbtree_node_t *node);
static void _co_rbtree_link_node(_co_rbtree_node_t *node, _co_rbtree_node_t *parent, _co_rbtree_node_t **_co_rbtree_link);

#define ZCOROUTINE_RBTREE_PARENT(node)    ((_co_rbtree_node_t *)((node)->___co_rbtree_parent_color & ~3))

static void ___co_rbtree_insert_augmented(_co_rbtree_t * root, _co_rbtree_node_t * node, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node));
static void ___co_rbtree_erase_color(_co_rbtree_t * root, _co_rbtree_node_t * parent, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node));

#define RB_RED          0
#define RB_BLACK        1
#define true    1
#define false   0

#define RB_ROOT    (_co_rbtree_t) { NULL, }
#define    _co_rbtree_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)  ((root)->_co_rbtree_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbree */
#define RB_EMPTY_NODE(node)  \
    ((node)->___co_rbtree_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node)  \
    ((node)->___co_rbtree_parent_color = (unsigned long)(node))

struct _co_rbtree_augment_callbacks {
    void (*propagate) (_co_rbtree_node_t * node, _co_rbtree_node_t * stop);
    void (*copy) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node);
    void (*rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node);
};

zinline static void _co_rbtree_insert_augmented(_co_rbtree_node_t * node, _co_rbtree_t * root, const struct _co_rbtree_augment_callbacks *augment)
{
    ___co_rbtree_insert_augmented(root, node, augment->rotate);
}

#define RB_DECLARE_CALLBACKS(rbstatic, rbname, rbstruct, rbfield,    \
        rbtype, rbaugmented, rbcompute)        \
        zinline static void                            \
        rbname ## _propagate(_co_rbtree_node_t *rb, _co_rbtree_node_t *stop)        \
{                                    \
    while (rb != stop) {                        \
        rbstruct *node = _co_rbtree_entry(rb, rbstruct, rbfield);    \
        rbtype augmented = rbcompute(node);            \
        if (node->rbaugmented == augmented)            \
        break;                        \
        node->rbaugmented = augmented;                \
        rb = ZCOROUTINE_RBTREE_PARENT(&node->rbfield);                \
    }                                \
}                                    \
zinline static void                            \
rbname ## _copy(_co_rbtree_node_t *_co_rbtree_old, _co_rbtree_node_t *_co_rbtree_new)        \
{                                    \
    rbstruct *old = _co_rbtree_entry(_co_rbtree_old, rbstruct, rbfield);        \
    rbstruct *new_node = _co_rbtree_entry(_co_rbtree_new, rbstruct, rbfield);        \
    new_node->rbaugmented = old->rbaugmented;                \
}                                    \
static void                                \
rbname ## _rotate(_co_rbtree_node_t *_co_rbtree_old, _co_rbtree_node_t *_co_rbtree_new)    \
{                                    \
    rbstruct *old = _co_rbtree_entry(_co_rbtree_old, rbstruct, rbfield);        \
    rbstruct *new_node = _co_rbtree_entry(_co_rbtree_new, rbstruct, rbfield);        \
    new_node->rbaugmented = old->rbaugmented;                \
    old->rbaugmented = rbcompute(old);                \
}                                    \
static rbstatic const struct _co_rbtree_augment_callbacks rbname = {            \
    rbname ## _propagate, rbname ## _copy, rbname ## _rotate    \
};

#define    RB_RED        0
#define    RB_BLACK    1

#define __ZCOROUTINE_RBTREE_PARENT(pc)    ((_co_rbtree_node_t *)(pc & ~3))

#define ___co_rbtree_color(pc)     ((pc) & 1)
#define ___co_rbtree_is_black(pc)  ___co_rbtree_color(pc)
#define ___co_rbtree_is_red(pc)    (!___co_rbtree_color(pc))
#define _co_rbtree_color(rb)       ___co_rbtree_color((rb)->___co_rbtree_parent_color)
#define _co_rbtree_is_red(rb)      ___co_rbtree_is_red((rb)->___co_rbtree_parent_color)
#define _co_rbtree_is_black(rb)    ___co_rbtree_is_black((rb)->___co_rbtree_parent_color)

zinline static void _co_rbtree_set_parent(_co_rbtree_node_t * rb, _co_rbtree_node_t * p)
{
    rb->___co_rbtree_parent_color = _co_rbtree_color(rb) | (unsigned long)p;
}

zinline static void _co_rbtree_set_parent_color(_co_rbtree_node_t * rb, _co_rbtree_node_t * p, int color)
{
    rb->___co_rbtree_parent_color = (unsigned long)p | color;
}

zinline static void ___co_rbtree_change_child(_co_rbtree_node_t * old, _co_rbtree_node_t * new_node, _co_rbtree_node_t * parent, _co_rbtree_t * root)
{
    if (parent) {
        if (parent->_co_rbtree_left == old)
            parent->_co_rbtree_left = new_node;
        else
            parent->_co_rbtree_right = new_node;
    } else
        root->_co_rbtree_node = new_node;
}

static void ___co_rbtree_erase_color(_co_rbtree_t * root, _co_rbtree_node_t * parent, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node));

zinline static _co_rbtree_node_t *___co_rbtree_erase_augmented(_co_rbtree_node_t * node, _co_rbtree_t * root, const struct _co_rbtree_augment_callbacks *augment)
{
    _co_rbtree_node_t *child = node->_co_rbtree_right, *tmp = node->_co_rbtree_left;
    _co_rbtree_node_t *parent, *rebalance;
    unsigned long pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass ___co_rbtree_erase_color() later on.
         */
        pc = node->___co_rbtree_parent_color;
        parent = __ZCOROUTINE_RBTREE_PARENT(pc);
        ___co_rbtree_change_child(node, child, parent, root);
        if (child) {
            child->___co_rbtree_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = ___co_rbtree_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->_co_rbtree_left */
        tmp->___co_rbtree_parent_color = pc = node->___co_rbtree_parent_color;
        parent = __ZCOROUTINE_RBTREE_PARENT(pc);
        ___co_rbtree_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        _co_rbtree_node_t *successor = child, *child2;
        tmp = child->_co_rbtree_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->_co_rbtree_right;
            augment->copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->_co_rbtree_left;
            }
            while (tmp);
            parent->_co_rbtree_left = child2 = successor->_co_rbtree_right;
            successor->_co_rbtree_right = child;
            _co_rbtree_set_parent(child, successor);
            augment->copy(node, successor);
            augment->propagate(parent, successor);
        }

        successor->_co_rbtree_left = tmp = node->_co_rbtree_left;
        _co_rbtree_set_parent(tmp, successor);

        pc = node->___co_rbtree_parent_color;
        tmp = __ZCOROUTINE_RBTREE_PARENT(pc);
        ___co_rbtree_change_child(node, successor, tmp, root);
        if (child2) {
            successor->___co_rbtree_parent_color = pc;
            _co_rbtree_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2 = successor->___co_rbtree_parent_color;
            successor->___co_rbtree_parent_color = pc;
            rebalance = ___co_rbtree_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    augment->propagate(tmp, NULL);
    return rebalance;
}

zinline static void _co_rbtree_erase_augmented(_co_rbtree_node_t * node, _co_rbtree_t * root, const struct _co_rbtree_augment_callbacks *augment)
{
    _co_rbtree_node_t *rebalance = ___co_rbtree_erase_augmented(node, root, augment);
    if (rebalance)
        ___co_rbtree_erase_color(root, rebalance, augment->rotate);
}

zinline static void _co_rbtree_set_black(_co_rbtree_node_t * rb)
{
    rb->___co_rbtree_parent_color |= RB_BLACK;
}

zinline static _co_rbtree_node_t *_co_rbtree_red_parent(_co_rbtree_node_t * red)
{
    return (_co_rbtree_node_t *) red->___co_rbtree_parent_color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new_node
 * - old gets assigned new_node as a parent and 'color' as a color.
 */
zinline static void ___co_rbtree_rotate_set_parents(_co_rbtree_node_t * old, _co_rbtree_node_t * new_node, _co_rbtree_t * root, int color)
{
    _co_rbtree_node_t *parent = ZCOROUTINE_RBTREE_PARENT(old);
    new_node->___co_rbtree_parent_color = old->___co_rbtree_parent_color;
    _co_rbtree_set_parent_color(old, new_node, color);
    ___co_rbtree_change_child(old, new_node, parent, root);
}

zinline static void ___co_rbtree_insert(_co_rbtree_node_t * node, _co_rbtree_t * root, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node))
{
    _co_rbtree_node_t *parent = _co_rbtree_red_parent(node), *gparent, *tmp;

    while (true) {
        /*
         * Loop invariant: node is red
         *
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as we don't
         * want a red root or two consecutive red nodes.
         */
        if (!parent) {
            _co_rbtree_set_parent_color(node, NULL, RB_BLACK);
            break;
        } else if (_co_rbtree_is_black(parent))
            break;

        gparent = _co_rbtree_red_parent(parent);

        tmp = gparent->_co_rbtree_right;
        if (parent != tmp) {    /* parent == gparent->_co_rbtree_left */
            if (tmp && _co_rbtree_is_red(tmp)) {
                /*
                 * Case 1 - color flips
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            N
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                _co_rbtree_set_parent_color(tmp, gparent, RB_BLACK);
                _co_rbtree_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = ZCOROUTINE_RBTREE_PARENT(node);
                _co_rbtree_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->_co_rbtree_right;
            if (node == tmp) {
                /*
                 * Case 2 - left rotate at parent
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                parent->_co_rbtree_right = tmp = node->_co_rbtree_left;
                node->_co_rbtree_left = parent;
                if (tmp)
                    _co_rbtree_set_parent_color(tmp, parent, RB_BLACK);
                _co_rbtree_set_parent_color(parent, node, RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->_co_rbtree_right;
            }

            /*
             * Case 3 - right rotate at gparent
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->_co_rbtree_left = tmp;    /* == parent->_co_rbtree_right */
            parent->_co_rbtree_right = gparent;
            if (tmp)
                _co_rbtree_set_parent_color(tmp, gparent, RB_BLACK);
            ___co_rbtree_rotate_set_parents(gparent, parent, root, RB_RED);
            augment_rotate(gparent, parent);
            break;
        } else {
            tmp = gparent->_co_rbtree_left;
            if (tmp && _co_rbtree_is_red(tmp)) {
                /* Case 1 - color flips */
                _co_rbtree_set_parent_color(tmp, gparent, RB_BLACK);
                _co_rbtree_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = ZCOROUTINE_RBTREE_PARENT(node);
                _co_rbtree_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->_co_rbtree_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                parent->_co_rbtree_left = tmp = node->_co_rbtree_right;
                node->_co_rbtree_right = parent;
                if (tmp)
                    _co_rbtree_set_parent_color(tmp, parent, RB_BLACK);
                _co_rbtree_set_parent_color(parent, node, RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->_co_rbtree_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->_co_rbtree_right = tmp;   /* == parent->_co_rbtree_left */
            parent->_co_rbtree_left = gparent;
            if (tmp)
                _co_rbtree_set_parent_color(tmp, gparent, RB_BLACK);
            ___co_rbtree_rotate_set_parents(gparent, parent, root, RB_RED);
            augment_rotate(gparent, parent);
            break;
        }
    }
}

/*
 * Inline version for _co_rbtree_erase() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
zinline static void _____co_rbtree_erase_color(_co_rbtree_node_t * parent, _co_rbtree_t * root, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node))
{
    _co_rbtree_node_t *node = NULL, *sibling, *tmp1, *tmp2;

    while (true) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->_co_rbtree_right;
        if (node != sibling) {  /* node == parent->_co_rbtree_left */
            if (_co_rbtree_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                parent->_co_rbtree_right = tmp1 = sibling->_co_rbtree_left;
                sibling->_co_rbtree_left = parent;
                _co_rbtree_set_parent_color(tmp1, parent, RB_BLACK);
                ___co_rbtree_rotate_set_parents(parent, sibling, root, RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->_co_rbtree_right;
            if (!tmp1 || _co_rbtree_is_black(tmp1)) {
                tmp2 = sibling->_co_rbtree_left;
                if (!tmp2 || _co_rbtree_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    _co_rbtree_set_parent_color(sibling, parent, RB_RED);
                    if (_co_rbtree_is_red(parent))
                        _co_rbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZCOROUTINE_RBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   Sl
                 *     / \             \
                 *    sl  Sr            s
                 *                       \
                 *                        Sr
                 */
                sibling->_co_rbtree_left = tmp1 = tmp2->_co_rbtree_right;
                tmp2->_co_rbtree_right = sibling;
                parent->_co_rbtree_right = tmp2;
                if (tmp1)
                    _co_rbtree_set_parent_color(tmp1, sibling, RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            parent->_co_rbtree_right = tmp2 = sibling->_co_rbtree_left;
            sibling->_co_rbtree_left = parent;
            _co_rbtree_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                _co_rbtree_set_parent(tmp2, parent);
            ___co_rbtree_rotate_set_parents(parent, sibling, root, RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        } else {
            sibling = parent->_co_rbtree_left;
            if (_co_rbtree_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                parent->_co_rbtree_left = tmp1 = sibling->_co_rbtree_right;
                sibling->_co_rbtree_right = parent;
                _co_rbtree_set_parent_color(tmp1, parent, RB_BLACK);
                ___co_rbtree_rotate_set_parents(parent, sibling, root, RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->_co_rbtree_left;
            if (!tmp1 || _co_rbtree_is_black(tmp1)) {
                tmp2 = sibling->_co_rbtree_right;
                if (!tmp2 || _co_rbtree_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    _co_rbtree_set_parent_color(sibling, parent, RB_RED);
                    if (_co_rbtree_is_red(parent))
                        _co_rbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZCOROUTINE_RBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - right rotate at sibling */
                sibling->_co_rbtree_right = tmp1 = tmp2->_co_rbtree_left;
                tmp2->_co_rbtree_left = sibling;
                parent->_co_rbtree_left = tmp2;
                if (tmp1)
                    _co_rbtree_set_parent_color(tmp1, sibling, RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - left rotate at parent + color flips */
            parent->_co_rbtree_left = tmp2 = sibling->_co_rbtree_right;
            sibling->_co_rbtree_right = parent;
            _co_rbtree_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                _co_rbtree_set_parent(tmp2, parent);
            ___co_rbtree_rotate_set_parents(parent, sibling, root, RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        }
    }
}

/* Non-inline version for _co_rbtree_erase_augmented() use */
static void ___co_rbtree_erase_color(_co_rbtree_t * root, _co_rbtree_node_t * parent, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node))
{
    _____co_rbtree_erase_color(parent, root, augment_rotate);
}

/*
 * Non-augmented rbtree manipulation functions.
 *
 * We use dummy augmented callbacks here, and have the compiler optimize them
 * out of the _co_rbtree_insert_color() and _co_rbtree_erase() function definitions.
 */

zinline static void dummy_propagate(_co_rbtree_node_t * node, _co_rbtree_node_t * stop)
{
}

zinline static void dummy_copy(_co_rbtree_node_t * old, _co_rbtree_node_t * new_node)
{
}

zinline static void dummy_rotate(_co_rbtree_node_t * old, _co_rbtree_node_t * new_node)
{
}

static const struct _co_rbtree_augment_callbacks dummy_callbacks = {
    dummy_propagate, dummy_copy, dummy_rotate
};

static void _co_rbtree_insert_color(_co_rbtree_t * root, _co_rbtree_node_t * node)
{
    ___co_rbtree_insert(node, root, dummy_rotate);
}

static void _co_rbtree_erase(_co_rbtree_t * root, _co_rbtree_node_t * node)
{
    _co_rbtree_node_t *rebalance;
    rebalance = ___co_rbtree_erase_augmented(node, root, &dummy_callbacks);
    if (rebalance)
        _____co_rbtree_erase_color(rebalance, root, dummy_rotate);
}

/*
 * Augmented rbtree manipulation functions.
 *
 * This instantiates the same __always_inline functions as in the non-augmented
 * case, but this time with user-defined callbacks.
 */

static void ___co_rbtree_insert_augmented(_co_rbtree_t * root, _co_rbtree_node_t * node, void (*augment_rotate) (_co_rbtree_node_t * old, _co_rbtree_node_t * new_node))
{
    ___co_rbtree_insert(node, root, augment_rotate);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
static _co_rbtree_node_t *_co_rbtree_first(const _co_rbtree_t * root)
{
    _co_rbtree_node_t *n;

    n = root->_co_rbtree_node;
    if (!n)
        return NULL;
    while (n->_co_rbtree_left)
        n = n->_co_rbtree_left;
    return n;
}

static _co_rbtree_node_t *_co_rbtree_next(const _co_rbtree_node_t * node)
{
    _co_rbtree_node_t *parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->_co_rbtree_right) {
        node = node->_co_rbtree_right;
        while (node->_co_rbtree_left)
            node = node->_co_rbtree_left;
        return (_co_rbtree_node_t *) node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = ZCOROUTINE_RBTREE_PARENT(node)) && node == parent->_co_rbtree_right)
        node = parent;

    return parent;
}

static void _co_rbtree_init(_co_rbtree_t * tree, _co_rbtree_cmp_t cmp_fn)
{
    tree->_co_rbtree_node = 0;
    tree->cmp_fn = cmp_fn;
}

zinline static _co_rbtree_node_t *_co_rbtree_parent(const _co_rbtree_node_t * node)
{
    return ((_co_rbtree_node_t *) ((node)->___co_rbtree_parent_color & ~3));
}

static _co_rbtree_node_t *_co_rbtree_attach(_co_rbtree_t * tree, _co_rbtree_node_t * node)
{
    _co_rbtree_node_t **new_node = &(tree->_co_rbtree_node), *parent = 0;
    int cmp_result;

    while (*new_node) {
        parent = *new_node;
        cmp_result = tree->cmp_fn(node, *new_node);
        if (cmp_result < 0) {
            new_node = &((*new_node)->_co_rbtree_left);
        } else if (cmp_result > 0) {
            new_node = &((*new_node)->_co_rbtree_right);
        } else {
            return (*new_node);
        }
    }
    _co_rbtree_link_node(node, parent, new_node);
    _co_rbtree_insert_color(tree, node);

    return node;
}

static _co_rbtree_node_t *_co_rbtree_detach(_co_rbtree_t * tree, _co_rbtree_node_t * node)
{
    _co_rbtree_erase(tree, node);
    return node;
}

static void _co_rbtree_link_node(_co_rbtree_node_t * node, _co_rbtree_node_t * parent, _co_rbtree_node_t ** _co_rbtree_link)
{
    node->___co_rbtree_parent_color = (unsigned long)parent;
    node->_co_rbtree_left = node->_co_rbtree_right = 0;

    *_co_rbtree_link = node;
}

/* }}} */

/* {{{ nonblocking close_on_exec */

static int _co_nonblocking(int fd, int no)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, no ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        return -1;
    }

    return ((flags & O_NONBLOCK)?1:0);
}

static int _co_close_on_exec(int fd, int on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
        return -1;
    }

    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0) {
        return -1;
    }

    return ((flags & FD_CLOEXEC)?1:0);
}

static long zcoroutine_timeout_set_millisecond(long timeout)
{
    if (timeout < 0) {
        timeout = (3600L * 24 * 365 * 10 * 1000);
    }
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (r+timeout);
}

/* }}} */

/* {{{ struct vars */
static pthread_mutex_t zvar_coroutine_base_mutex = PTHREAD_MUTEX_INITIALIZER;

int zvar_coroutine_block_pthread_count_limit = 0;
zbool_t zvar_coroutine_fileio_use_block_pthread = 0;
zbool_t zvar_coroutine_disable_udp = 0;
zbool_t zvar_coroutine_disable_udp_53 = 0;

int zvar_coroutine_max_fd = 0;

static int zvar_coroutine_block_pthread_count_current = 0;

static void *zcoroutine_hook_fileio_worker(void *arg);

int _syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);

/* ################################################################################# */
#ifdef __x86_64__
typedef struct zcoroutine_sys_context zcoroutine_sys_context;
struct zcoroutine_sys_context {
    void *regs[ 14 ];
    size_t ss_size;
    char *ss_sp;
};
#else
typedef struct ucontext_t zcoroutine_sys_context;
#endif

typedef union zcoroutine_hook_arg_t zcoroutine_hook_arg_t;
typedef struct zcoroutine_hook_fileio_t zcoroutine_hook_fileio_t;

typedef struct zgethostbyname_buf_t zgethostbyname_buf_t;
struct zgethostbyname_buf_t 
{
    struct hostent host;
    char* buf_ptr;
    int buf_size;
    int h_errno2;
};

union zcoroutine_hook_arg_t {
    const void *const_void_ptr_t;
    void *void_ptr_t;
    const char *const_char_ptr_t;
    char *char_ptr_t;
    struct iovec *iovec_t;
    const struct iovec *const_iovec_t;
    long long_t;
    int int_t;
    unsigned uint_t;
    size_t size_size_t;
    ssize_t ssize_ssize_t;
    mode_t mode_mode_t;
    off_t off_off_t;
    uid_t uid_uid_t;
    gid_t gid_gid_t;
    void *(*block_func)(void *);
};
struct zcoroutine_hook_fileio_t {
    zcoroutine_t *current_coroutine;
    zcoroutine_hook_arg_t args[6];
    zcoroutine_hook_fileio_t *prev;
    zcoroutine_hook_fileio_t *next;
    zcoroutine_hook_arg_t retval;
    int co_errno;
    int cmdcode;
    unsigned int is_regular_file:1;
    unsigned int is_block_func:1;
};

static pthread_mutex_t zvar_coroutine_hook_fileio_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t zvar_coroutine_hook_fileio_cond = PTHREAD_COND_INITIALIZER;

#define zvar_coroutine_extern_lock zvar_coroutine_hook_fileio_lock

static zcoroutine_hook_fileio_t *zvar_coroutine_hook_fileio_head = 0;
static zcoroutine_hook_fileio_t *zvar_coroutine_hook_fileio_tail = 0;
static int zvar_coroutine_hook_fileio_count = 0;

enum zcoroutine_hook_fileio_cmd_t {
    zcoroutine_hook_fileio_open = 1,
    zcoroutine_hook_fileio_openat,
    zcoroutine_hook_fileio_close,
    zcoroutine_hook_fileio_read,
    zcoroutine_hook_fileio_readv,
    zcoroutine_hook_fileio_write,
    zcoroutine_hook_fileio_writev,
    zcoroutine_hook_fileio_lseek,
    zcoroutine_hook_fileio_fdatasync,
    zcoroutine_hook_fileio_fsync,
    zcoroutine_hook_fileio_rename,
    zcoroutine_hook_fileio_renameat,
    zcoroutine_hook_fileio_truncate,
    zcoroutine_hook_fileio_ftruncate,
    zcoroutine_hook_fileio_rmdir,
    zcoroutine_hook_fileio_mkdir,
    zcoroutine_hook_fileio_getdents,
    zcoroutine_hook_fileio_stat,
    zcoroutine_hook_fileio_fstat,
    zcoroutine_hook_fileio_lstat,
    zcoroutine_hook_fileio_link,
    zcoroutine_hook_fileio_linkat,
    zcoroutine_hook_fileio_symlink,
    zcoroutine_hook_fileio_symlinkat,
    zcoroutine_hook_fileio_readlink,
    zcoroutine_hook_fileio_readlinkat,
    zcoroutine_hook_fileio_unlink,
    zcoroutine_hook_fileio_unlinkat,
    zcoroutine_hook_fileio_chmod,
    zcoroutine_hook_fileio_fchmod,
    zcoroutine_hook_fileio_chown,
    zcoroutine_hook_fileio_fchown,
    zcoroutine_hook_fileio_lchown,
    zcoroutine_hook_fileio_utime,
    zcoroutine_hook_fileio_utimes,
    zcoroutine_hook_fileio_unknown
};
typedef enum zcoroutine_hook_fileio_cmd_t zcoroutine_hook_fileio_cmd_t;

#define zcoroutine_hook_fileio_run_part0() \
    zcoroutine_base_t *cobs = 0; \
if ((cobs = zcoroutine_base_get_current_inner())==0)

#define zcoroutine_hook_fileio_run_part1() \
    zcoroutine_base_t *cobs = 0; \
if ((!zvar_coroutine_fileio_use_block_pthread) || (zvar_coroutine_block_pthread_count_limit<1) || ((cobs = zcoroutine_base_get_current_inner())==0))

#define zcoroutine_hook_fileio_run_part2(func)  \
    zcoroutine_t *current_coroutine = cobs->current_coroutine; \
zcoroutine_hook_fileio_t fileio;\
fileio.is_block_func = 0; \
fileio.current_coroutine = cobs->current_coroutine; \
fileio.co_errno = 0; \
fileio.cmdcode = zcoroutine_hook_fileio_ ## func;

#define zcoroutine_hook_fileio_run_part3()  \
    zpthread_lock(&zvar_coroutine_hook_fileio_lock); \
ZMLINK_APPEND(zvar_coroutine_hook_fileio_head, zvar_coroutine_hook_fileio_tail, &fileio, prev, next); \
zvar_coroutine_hook_fileio_count++; \
if (zvar_coroutine_block_pthread_count_current < zvar_coroutine_block_pthread_count_limit) { \
    if ((zvar_coroutine_block_pthread_count_current == 0)||(zvar_coroutine_hook_fileio_count > (zvar_coroutine_block_pthread_count_current))) { \
        pthread_t pth; \
        if (pthread_create(&pth, 0, zcoroutine_hook_fileio_worker, 0)) { \
            zcoroutine_fatal("pthread_create error(%m)"); \
        } \
        zvar_coroutine_block_pthread_count_current++; \
    } \
} \
zpthread_unlock(&zvar_coroutine_hook_fileio_lock); \
pthread_cond_signal(&zvar_coroutine_hook_fileio_cond); \
current_coroutine->inner_yield = 1; \
zcoroutine_yield_my(current_coroutine); \
errno = fileio.co_errno; \
zcoroutine_hook_arg_t retval; \
retval.long_t = fileio.retval.long_t; \

/* ######################################## */
static void zcoroutine_yield_my(zcoroutine_t *co);

static int zvar_coroutine_mode_flag = 0;

struct  zcoroutine_t {
    void *(*start_job)(void *ctx);
    void *context;
    long sleep_timeout;
    _co_rbtree_node_t sleep_rbnode;
    /* system */
    zcoroutine_sys_context sys_context;
    zcoroutine_t *prev;
    zcoroutine_t *next;
    zcoroutine_base_t *base; /* FIXME, should be removed */
    _co_list_t *mutex_list; /* zcoroutine_mutex_t* */
    struct __res_state *res_state;
    zgethostbyname_buf_t *gethostbyname;
    long id;
    /* flags */
    unsigned char ended:1;
    unsigned char ep_loop:1;
    unsigned char active_list:1;
    unsigned char inner_yield:1;
};

struct zcoroutine_base_t {
    int epoll_fd;
    int event_fd;
    int epoll_event_size;
    struct epoll_event *epoll_event_vec;
    _co_rbtree_t sleep_zrbtree;
    _co_rbtree_t fd_timeout_zrbtree;
    zcoroutine_t *self_coroutine;
    zcoroutine_t *current_coroutine;
    zcoroutine_t *extern_coroutines_head;
    zcoroutine_t *extern_coroutines_tail;
    zcoroutine_t *fileio_coroutines_head;
    zcoroutine_t *fileio_coroutines_tail;
    zcoroutine_t *active_coroutines_head;
    zcoroutine_t *active_coroutines_tail;
    zcoroutine_t *prepare_coroutines_head;
    zcoroutine_t *prepare_coroutines_tail;
    zcoroutine_t *deleted_coroutine_head;
    zcoroutine_t *deleted_coroutine_tail;
    long id_plus;
    unsigned char ___break:1;
};

typedef struct _co_fd_attribute_t _co_fd_attribute_t;
struct _co_fd_attribute_t {
    long timeout;
    _co_rbtree_node_t rbnode;
    zcoroutine_t *co;
    unsigned int read_timeout;
    unsigned int write_timeout;
    unsigned int revents;
    unsigned short int nonblock:1;
    unsigned short int pseudo_mode:1;
    unsigned short int in_epoll:1;
    unsigned short int by_epoll:1;
    unsigned short int is_regular_file:1;
    unsigned short int is_udp:1;
};

struct  zcoroutine_mutex_t {
    _co_list_t *colist; /* zcoroutine_t* */
};

struct zcoroutine_cond_t {
    _co_list_t *colist; /* zcoroutine_t* */
};

static void zcoroutine_base_remove_coroutine(zcoroutine_base_t *cobs);
static zcoroutine_base_t *zcoroutine_base_create();
/* }}} */

/* {{{ current base */
static __thread zcoroutine_base_t *zvar_coroutine_base_per_pthread = 0;
static _co_fd_attribute_t **_co_fd_attribute_vec = 0;

zinline static zcoroutine_base_t *zcoroutine_base_get_current_inner()
{
    return (zvar_coroutine_mode_flag?zvar_coroutine_base_per_pthread:0);
}

zcoroutine_base_t *zcoroutine_base_get_current()
{
    return (zvar_coroutine_mode_flag?zvar_coroutine_base_per_pthread:0);
}
/* }}} */

/* {{{ context swap */
static int _co_start_wrap(zcoroutine_t *co, void *unused);

#ifdef __x86_64__
asm("\n"
        ".type ___coroutine_context_swap, @function\n"
        "___coroutine_context_swap:\n"
        "\tleaq 8(%rsp),%rax\n"
        "\tleaq 112(%rdi),%rsp\n"
        "\tpushq %rax\n"
        "\tpushq %rbx\n"
        "\tpushq %rcx\n"
        "\tpushq %rdx\n"
        "\tpushq -8(%rax)\n"
        "\tpushq %rsi\n"
        "\tpushq %rdi\n"
        "\tpushq %rbp\n"
        "\tpushq %r8\n"
        "\tpushq %r9\n"
        "\tpushq %r12\n"
        "\tpushq %r13\n"
        "\tpushq %r14\n"
        "\tpushq %r15\n"
        "\tmovq %rsi, %rsp\n"
        "\tpopq %r15\n"
"\tpopq %r14\n"
"\tpopq %r13\n"
"\tpopq %r12\n"
"\tpopq %r9\n"
"\tpopq %r8\n"
"\tpopq %rbp\n"
"\tpopq %rdi\n"
"\tpopq %rsi\n"
"\tpopq %rax\n"
"\tpopq %rdx\n"
"\tpopq %rcx\n"
"\tpopq %rbx\n"
"\tpopq %rsp\n"
"\tpushq %rax\n"
"\txorl %eax, %eax\n"
"\tret\n");
void zcoroutine_context_swap(zcoroutine_sys_context *, zcoroutine_sys_context *) asm("___coroutine_context_swap");
#else
#define zcoroutine_context_swap swapcontext
#endif

/* }}} */

/* {{{ fd attribute */
/* FIXME 是否需要加锁? 不需要! */
zinline static _co_fd_attribute_t * _co_fd_attribute_get(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_coroutine_max_fd) && (zvar_coroutine_mode_flag)) {
        return _co_fd_attribute_vec[fd];
    }
    return 0;
}

static _co_fd_attribute_t *_co_fd_attribute_create(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_coroutine_max_fd) && (zvar_coroutine_mode_flag)) {
        _co_mem_free(_co_fd_attribute_vec[fd]);
        _co_fd_attribute_t *cfa = (_co_fd_attribute_t *)_co_mem_calloc(1, sizeof(_co_fd_attribute_t));
        _co_fd_attribute_vec[fd] = cfa;
        cfa->read_timeout = 10 * 1000  + (fd%1000);
        cfa->write_timeout = 10 * 1000 + (fd%1000);
        return cfa;
    }
    return 0;
}

zinline static void _co_fd_attribute_free(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_coroutine_max_fd) && (zvar_coroutine_mode_flag)) {
        _co_mem_free(_co_fd_attribute_vec[fd]);
        _co_fd_attribute_vec[fd] = 0;
    }
}
/* }}} */

/* {{{ zcoroutine_t */
static zcoroutine_t *zcoroutine_create(zcoroutine_base_t *base, int stack_kilobyte, void *self_buf)
{
    if (stack_kilobyte < 1) {
        stack_kilobyte = 128;
    }
    zcoroutine_t *co = (zcoroutine_t *)(self_buf?self_buf:_co_mem_calloc(1, sizeof (zcoroutine_t)));
    co->base = base;
    co->id = base->id_plus++;
    memset(&(co->sys_context), 0, sizeof(zcoroutine_sys_context));
#ifdef __x86_64__
    co->sys_context.ss_sp = (char *)_co_mem_malloc(stack_kilobyte*1024 + 16 + 10);
    co->sys_context.ss_size = stack_kilobyte*1024;
    char *sp = co->sys_context.ss_sp + co->sys_context.ss_size;
    sp = (char*) ((unsigned long)sp & -16LL  );
    co->sys_context.regs[13] = sp - 8;
    co->sys_context.regs[9] = (char*)_co_start_wrap;
    co->sys_context.regs[7] = (char *)co;
    co->sys_context.regs[8] = 0;
#else
    co->sys_context.uc_stack.ss_sp = (char *)_co_mem_malloc(stack_kilobyte*1024 + 16 + 10);
    co->sys_context.uc_stack.ss_size = stack_kilobyte*1024;
    co->sys_context.uc_link = NULL;
    makecontext(&(co->sys_context), (void (*)(void)) _co_start_wrap, 2, co, 0);
#endif
    return co;
}

static void zcoroutine_free(zcoroutine_t *co)
{
    if (co->res_state) {
        void __res_iclose(struct __res_state *, int);
        __res_iclose(co->res_state, 1);
        _co_mem_free(co->res_state);
    }
    _co_mem_free(co->gethostbyname);
#ifdef __x86_64__
    _co_mem_free(co->sys_context.ss_sp);
#else
    _co_mem_free(co->sys_context.uc_stack.ss_sp);
#endif
    _co_mem_free(co);
}

static void zcoroutine_append_mutex(zcoroutine_t *co, zcoroutine_mutex_t *mutex)
{
    if (co->mutex_list == 0) {
        co->mutex_list = _co_list_create();
        _co_list_push(co->mutex_list, mutex);
        return;
    }

    ZCOROUTINE_LIST_WALK_BEGIN(co->mutex_list, zcoroutine_mutex_t *, m) {
        if (m == mutex) {
            return;
        }
    } ZCOROUTINE_LIST_WALK_END;
    _co_list_push(co->mutex_list, mutex);
}

static void zcoroutine_remove_mutex(zcoroutine_t *co, zcoroutine_mutex_t *mutex)
{
    if (!co->mutex_list) {
        return;
    }
    _co_list_node_t *del = 0;
    ZCOROUTINE_LIST_WALK_BEGIN(co->mutex_list, zcoroutine_mutex_t *, m) {
        if (m == mutex) {
            del = list_current_node;
            break;
        }
    } ZCOROUTINE_LIST_WALK_END;

    if (del) {
        _co_list_delete(co->mutex_list, del, 0);
    }
    if (!_co_list_len(co->mutex_list)) {
        _co_list_free(co->mutex_list);
        co->mutex_list = 0;
    }
}

static void zcoroutine_release_all_mutex(zcoroutine_t *co)
{
    if (!co->mutex_list) {
        return;
    }
    zcoroutine_mutex_t *mutex;
    while(co->mutex_list && _co_list_pop(co->mutex_list, (void **)&mutex) ) {
        zcoroutine_mutex_unlock(mutex);
    }
    _co_list_free(co->mutex_list);
    co->mutex_list = 0;
}

static int _co_start_wrap(zcoroutine_t *co, void *unused)
{
    if (co->start_job) {
        co->start_job(co->context);
    }
    zcoroutine_release_all_mutex(co);
    zcoroutine_base_t *cobs = co->base;
    if (cobs->deleted_coroutine_head) {
        zcoroutine_base_remove_coroutine(cobs);
    }
    co->ended = 1;
    co->inner_yield = 1;
    zcoroutine_yield_my(co);
    return 0;
}

static int base_count = 0;
zcoroutine_base_t *zcoroutine_base_init()
{
    zcoroutine_base_t *cobs = 0;
    pthread_mutex_lock(&zvar_coroutine_base_mutex);
    zvar_coroutine_mode_flag = 1;
    if (_co_fd_attribute_vec == 0) {
        if (zvar_coroutine_max_fd < 1) {
            struct rlimit rlimit;
            if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0) {
                zcoroutine_fatal("getrlimit: %m");
                zvar_coroutine_max_fd = 1024;
            } else {
                zvar_coroutine_max_fd = rlimit.rlim_cur;
            }
            if (zvar_coroutine_max_fd < 1024) {
                zvar_coroutine_max_fd = 1024;
            }
        }
        _co_fd_attribute_vec = (_co_fd_attribute_t **)_co_mem_calloc(zvar_coroutine_max_fd + 1, sizeof(void *));
    }
    if (zvar_coroutine_base_per_pthread == 0) {
        cobs = zcoroutine_base_create();
        base_count++;
    }
    pthread_mutex_unlock(&zvar_coroutine_base_mutex);
    return cobs;
}

void zcoroutine_base_fini()
{
    if (_co_fd_attribute_vec == 0) {
        return;
    }
    pthread_mutex_lock(&zvar_coroutine_base_mutex);
    zcoroutine_base_t *cobs = zvar_coroutine_base_per_pthread;
    if (cobs) {
        if (cobs->deleted_coroutine_head) {
            zcoroutine_base_remove_coroutine(cobs);
        }
        zcoroutine_free(cobs->self_coroutine);
        zrobust_syscall_close(cobs->epoll_fd);
        zrobust_syscall_close(cobs->event_fd);
        _co_mem_free(cobs->epoll_event_vec);
        _co_mem_free(cobs);
        zvar_coroutine_base_per_pthread = 0;
        base_count--;
        if (base_count == 0) {
            _co_mem_free(_co_fd_attribute_vec);
            _co_fd_attribute_vec = 0;
            zvar_coroutine_mode_flag = 0;
        }
    }
    pthread_mutex_unlock(&zvar_coroutine_base_mutex);
}

void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte)
{
    if (start_job == 0) {
        return;
    }
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if (!cobs) {
        zcoroutine_fatal("excute zcoroutine_enable() when the pthread begin");
    }
    if (cobs->deleted_coroutine_head) {
        zcoroutine_base_remove_coroutine(cobs);
    }
    zcoroutine_t *co = zcoroutine_create(cobs, stack_kilobyte, 0);
    ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    co->start_job = start_job;
    co->context = ctx;
}

static void zcoroutine_advanced_go_now(zcoroutine_base_t *cobs, zcoroutine_t *pseudo_co_list)
{
    zcoroutine_t *pseudo_co, *pseudo_next;
    for (pseudo_co = pseudo_co_list; pseudo_co; pseudo_co = pseudo_next) {
        pseudo_next = pseudo_co->next;
        if (cobs->deleted_coroutine_head) {
            zcoroutine_base_remove_coroutine(cobs);
        }

        void *(*start_job)(void *ctx) = pseudo_co->start_job;
        void *ctx = pseudo_co->context;
        int stack_kilobyte = (int)(pseudo_co->sleep_timeout);
        zcoroutine_t *co = zcoroutine_create(cobs, stack_kilobyte, (void *)pseudo_co);
        ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
        co->start_job = start_job;
        co->context = ctx;
    }
}

void zcoroutine_advanced_go(zcoroutine_base_t *cobs, void *(*start_job)(void *ctx), void *ctx, int stack_kilobyte)
{
    if (start_job == 0) {
        return;
    }
    if (cobs == 0) {
        zcoroutine_go(start_job, ctx, stack_kilobyte);
        return;
    }
    zcoroutine_t *pseudo_co = (zcoroutine_t *)_co_mem_calloc(1, sizeof(zcoroutine_t));
    pseudo_co->start_job = start_job;
    pseudo_co->context = ctx;
    pseudo_co->sleep_timeout = stack_kilobyte;

    zpthread_lock(&zvar_coroutine_extern_lock);
    ZMLINK_APPEND(cobs->extern_coroutines_head, cobs->extern_coroutines_tail, pseudo_co, prev, next);
    zpthread_unlock(&zvar_coroutine_extern_lock);
    uint64_t u = 1;
    _syscall_write(cobs->event_fd, &u, sizeof(uint64_t));

}

zcoroutine_t * zcoroutine_self()
{
    zcoroutine_base_t *cobs =  zcoroutine_base_get_current_inner();
    if (cobs == 0) {
        return 0;
    }
    return cobs->current_coroutine;
}

/* 返回当前协程ID */
long zcoroutine_getid()
{
    zcoroutine_base_t *cobs =  zcoroutine_base_get_current_inner();
    if ((cobs == 0)||(cobs->current_coroutine==0) ) {
        return 0;
    }
    return cobs->current_coroutine->id;
}

static void zcoroutine_yield_my(zcoroutine_t *co)
{
    if (!co) {
        co = zcoroutine_self();
    }
    if (!co) {
        return;
    }
    zcoroutine_base_t *cobs = co->base;
    cobs->current_coroutine = 0;
    if (co->ended) {
        ZMLINK_APPEND(cobs->deleted_coroutine_head, cobs->deleted_coroutine_tail, co, prev, next);
    } else if ((co->inner_yield == 0) && (co != cobs->self_coroutine)) {
        ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    }
    zcoroutine_t *next_co = cobs->active_coroutines_head;
    if (next_co == 0) {
        if (co == cobs->self_coroutine) {
            return;
        }
        next_co = cobs->self_coroutine;
    }
    if (next_co != cobs->self_coroutine) {
        ZMLINK_DETACH(cobs->active_coroutines_head, cobs->active_coroutines_tail, next_co, prev, next);
    }
    cobs->current_coroutine = next_co;
    co->inner_yield = 0;
    zcoroutine_context_swap(&(co->sys_context), &(next_co->sys_context));
}

void zcoroutine_yield()
{
    zcoroutine_yield_my(zcoroutine_self());
}

void zcoroutine_exit()
{
    zcoroutine_t *co = zcoroutine_self();
    if (!co) {
        return;
    }
    zcoroutine_release_all_mutex(co);
    co->ended = 1;
    co->inner_yield = 1;
    zcoroutine_yield_my(co);
}

void *zcoroutine_get_context()
{
    zcoroutine_t *co = zcoroutine_self();
    if (!co) {
        return 0;
    }
    return (void *)(co->context);
}

void zcoroutine_set_context(const void *ctx)
{
    zcoroutine_t *co = zcoroutine_self();
    if (!co) {
        return;
    }
    co->context = (void *)ctx;
}

void zcoroutine_enable_fd(int fd)
{
    _co_fd_attribute_create(fd);
}

void zcoroutine_disable_fd(int fd)
{
    _co_fd_attribute_t  *cfa = _co_fd_attribute_get(fd);
    if (cfa) {
        int flags;
        if ((flags = _syscall_fcntl(fd, F_GETFL, 0)) < 0) {
            zcoroutine_fatal("fcntl _co(%m)");
        }
        if (_syscall_fcntl(fd, F_SETFL, (cfa->nonblock?flags | O_NONBLOCK : flags & ~O_NONBLOCK)) < 0) {
            zcoroutine_fatal("fcntl _co(%m)");
        }
        _co_fd_attribute_free(fd);
    }
}
/* }}} */

/* {{{ mutex cond */
zcoroutine_mutex_t * zcoroutine_mutex_create()
{
    zcoroutine_mutex_t *m = (zcoroutine_mutex_t *)_co_mem_calloc(1, sizeof(zcoroutine_mutex_t));
    m->colist = _co_list_create();
    return m;
}

void zcoroutine_mutex_free(zcoroutine_mutex_t *m)
{
    if (m) {
        _co_list_free(m->colist);
        _co_mem_free(m);
    }
}

void zcoroutine_mutex_lock(zcoroutine_mutex_t *m)
{
    if (m == 0) {
        zcoroutine_fatal("not in zcoroutine_t");
    }
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        zcoroutine_fatal("not in zcoroutine_t");
    }
    _co_list_t *colist = m->colist;
    if (!(_co_list_len(colist))) {
        _co_list_push(colist, co);
        zcoroutine_append_mutex(co, m);
        return;
    }
    if ((zcoroutine_t *)_co_list_node_value(_co_list_head(colist)) == co) {
        zcoroutine_append_mutex(co, m);
        return;
    }
    _co_list_push(colist, co);
    co->inner_yield = 1;
    zcoroutine_yield_my(co);
}

void zcoroutine_mutex_unlock(zcoroutine_mutex_t *m)
{
    /* FIXME */
    if (m == 0) {
        zcoroutine_fatal("mutex is null");
        return;
    }
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        return;
    }
    _co_list_t *colist = m->colist;
    if (!(_co_list_len(colist))) {
        return;
    }
    if ((zcoroutine_t *)_co_list_node_value(_co_list_head(colist)) != co) {
        return;
    }
    zcoroutine_remove_mutex(co, m);
    _co_list_shift(colist, 0);
    if (!_co_list_len(colist)) {
        return;
    }
    co = (zcoroutine_t *)_co_list_node_value(_co_list_head(colist));
    ZMLINK_APPEND(co->base->prepare_coroutines_head, co->base->prepare_coroutines_tail, co, prev, next);
    return;
}

zcoroutine_cond_t *zcoroutine_cond_create()
{
    zcoroutine_cond_t *c= (zcoroutine_cond_t *)_co_mem_calloc(1, sizeof(zcoroutine_cond_t));
    c->colist = _co_list_create();
    return c;
}

void zcoroutine_cond_free(zcoroutine_cond_t * c)
{
    if (c) {
        _co_list_free(c->colist);
        _co_mem_free(c);
    }
}

void zcoroutine_cond_wait(zcoroutine_cond_t *cond, zcoroutine_mutex_t * mutex)
{
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        zcoroutine_fatal("not in zcoroutine_t");
    }
    if (!cond) {
        zcoroutine_fatal("cond is null");
    }
    if (!cond) {
        zcoroutine_fatal("mutex is null");
    }

    zcoroutine_mutex_unlock(mutex);

    _co_list_push(cond->colist, co);
    co->inner_yield = 1;
    zcoroutine_yield_my(co);

    zcoroutine_mutex_lock(mutex);
}

void zcoroutine_cond_signal(zcoroutine_cond_t * cond)
{
    if (!cond) {
        zcoroutine_fatal("cond is null");
    }
    zcoroutine_base_t *cobs;
    if ((cobs = zcoroutine_base_get_current_inner()) == 0) {
        zcoroutine_fatal("not in zcoroutine_t");
    }

    zcoroutine_t *co;
    if (!_co_list_shift(cond->colist, (void **)&co)) {
        return;
    }
    ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    zcoroutine_yield_my(cobs->current_coroutine);
}

void zcoroutine_cond_broadcast(zcoroutine_cond_t *cond)
{
    if (!cond) {
        zcoroutine_fatal("cond is null");
    }
    zcoroutine_base_t *cobs;
    if ((cobs = zcoroutine_base_get_current_inner()) == 0) {
        zcoroutine_fatal("not in zcoroutine_t");
    }
    zcoroutine_t *co;
    while(_co_list_shift(cond->colist, (void **)&co)) {
        ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    }
    zcoroutine_yield_my(cobs->current_coroutine);
}

/* }}} */

/* {{{ rbtree cmp*/
static int _co_base_sleep_tree_cmp(_co_rbtree_node_t * n1, _co_rbtree_node_t * n2)
{
    zcoroutine_t *c1, *c2;
    long r;
    c1 = ZCONTAINER_OF(n1, zcoroutine_t, sleep_rbnode);
    c2 = ZCONTAINER_OF(n2, zcoroutine_t, sleep_rbnode);
    r = c1->sleep_timeout - c2->sleep_timeout;
    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}

static int _co_base_fd_timeout_tree_cmp(_co_rbtree_node_t * n1, _co_rbtree_node_t * n2)
{
    _co_fd_attribute_t *c1, *c2;
    long r;
    c1 = ZCONTAINER_OF(n1, _co_fd_attribute_t, rbnode);
    c2 = ZCONTAINER_OF(n2, _co_fd_attribute_t, rbnode);
    r = c1->timeout - c2->timeout;
    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}
/* }}} */

/* {{{ zcoroutine_base_create */
static zcoroutine_base_t *zcoroutine_base_create()
{
    zcoroutine_base_t *cobs;
    cobs = (zcoroutine_base_t *)_co_mem_calloc(1, sizeof(zcoroutine_base_t));
    zvar_coroutine_base_per_pthread = cobs;
    cobs->id_plus = 1;
    cobs->self_coroutine = zcoroutine_create(cobs, 128, 0);
    cobs->epoll_fd = epoll_create(1024);
    _co_close_on_exec(cobs->epoll_fd, 1);
    cobs->epoll_event_size = 256;
    cobs->epoll_event_vec = (struct epoll_event *)_co_mem_malloc(cobs->epoll_event_size*sizeof(struct epoll_event));

    cobs->event_fd = eventfd(0, 0);
    _co_close_on_exec(cobs->event_fd, 1);
    _co_nonblocking(cobs->event_fd, 1);
    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = cobs->event_fd;
    int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_ADD, cobs->event_fd, &epev);
    if (eret < 0) {
        zcoroutine_fatal("epoll_ctl ADD event_fd:%d (%m)", cobs->event_fd);
    }

    _co_rbtree_init(&(cobs->sleep_zrbtree), _co_base_sleep_tree_cmp);
    _co_rbtree_init(&(cobs->fd_timeout_zrbtree), _co_base_fd_timeout_tree_cmp);
    return cobs;
}

static void zcoroutine_base_remove_coroutine(zcoroutine_base_t *cobs)
{
    zcoroutine_t *co, *next_co;
    for (co = cobs->deleted_coroutine_head; co; co = next_co) {
        next_co = co->next;
        zcoroutine_free(co);
    }
    cobs->deleted_coroutine_head = cobs->deleted_coroutine_tail = 0;
}

void zcoroutine_base_stop_notify(zcoroutine_base_t *cobs)
{
    if (!cobs) {
        cobs = zcoroutine_base_get_current_inner();
    }
    if (cobs) {
        cobs->___break = 1;
    }
}

/* }}} */

/* {{{ zcoroutine_base_run */
void zcoroutine_base_run(void (*loop_fn)())
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if (!cobs) {
        zcoroutine_fatal("excute zcoroutine_enable() when the pthread begin");
    }
    zcoroutine_t *co;
    _co_rbtree_node_t *rn;
    _co_fd_attribute_t *cfa;
    long delay, tmp_delay, tmp_ms;

    while(cobs->___break == 0) {
        if (loop_fn) {
            loop_fn();
        }
        if (cobs->deleted_coroutine_head) {
            zcoroutine_base_remove_coroutine(cobs);
        }

        delay = 1000;
        tmp_ms = zcoroutine_timeout_set_millisecond(0) - 1;

        if (cobs->extern_coroutines_head) {
            zcoroutine_t *pseudo_co_list = 0;
            zpthread_lock(&zvar_coroutine_extern_lock);
            if (cobs->extern_coroutines_head) {
                pseudo_co_list = cobs->extern_coroutines_head;
                cobs->extern_coroutines_head = 0;
                cobs->extern_coroutines_tail = 0;
            }
            zpthread_unlock(&zvar_coroutine_extern_lock);
            if (pseudo_co_list) {
                zcoroutine_advanced_go_now(cobs, pseudo_co_list);
            }
        }

        /* fileio list */
        if (cobs->fileio_coroutines_head) {
            zpthread_lock(&zvar_coroutine_hook_fileio_lock);
            if (cobs->fileio_coroutines_head) {
                if (cobs->active_coroutines_head == 0) {
                    cobs->active_coroutines_head = cobs->fileio_coroutines_head;
                    cobs->active_coroutines_tail = cobs->fileio_coroutines_tail;
                } else if (cobs->fileio_coroutines_head) {
                    cobs->active_coroutines_tail->next = cobs->fileio_coroutines_head;
                    cobs->fileio_coroutines_head->prev = cobs->active_coroutines_tail;
                    cobs->active_coroutines_tail = cobs->fileio_coroutines_tail;
                }
                cobs->fileio_coroutines_head = cobs->fileio_coroutines_tail = 0;
            }
            zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
        }

        /* prepared list */
        if (cobs->active_coroutines_head == 0) {
            cobs->active_coroutines_head = cobs->prepare_coroutines_head;
            cobs->active_coroutines_tail = cobs->prepare_coroutines_tail;
        } else if (cobs->prepare_coroutines_head) {
            cobs->active_coroutines_tail->next = cobs->prepare_coroutines_head;
            cobs->prepare_coroutines_head->prev = cobs->active_coroutines_tail;
            cobs->active_coroutines_tail = cobs->prepare_coroutines_tail;
        }
        cobs->prepare_coroutines_head = cobs->prepare_coroutines_tail = 0;

        /* sleep timeout */
        if (_co_rbtree_have_data(&(cobs->sleep_zrbtree))) {
            rn = _co_rbtree_first(&(cobs->sleep_zrbtree));
            co = ZCONTAINER_OF(rn, zcoroutine_t, sleep_rbnode);
            tmp_delay = co->sleep_timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = _co_rbtree_next(rn)) {
                co = ZCONTAINER_OF(rn, zcoroutine_t, sleep_rbnode);
                if (tmp_ms < co->sleep_timeout) {
                    break;
                }
                ZMLINK_APPEND(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, prev, next);
            }
        }

        /* fd timeout */
        if (_co_rbtree_have_data(&(cobs->fd_timeout_zrbtree))) {
            rn = _co_rbtree_first(&(cobs->fd_timeout_zrbtree));
            cfa = ZCONTAINER_OF(rn, _co_fd_attribute_t, rbnode);
            tmp_delay = cfa->timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = _co_rbtree_next(rn)) {
                cfa = ZCONTAINER_OF(rn, _co_fd_attribute_t, rbnode);
                if (tmp_ms < cfa->timeout) {
                    break;
                }
                co = cfa->co;
                if ((co->ep_loop == 1) && (co->active_list == 0)) {
                    ZMLINK_APPEND(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, prev, next);
                    co->active_list = 1;
                }
                co->ep_loop = 1;
            }
        }

        /* epoll_wait */
        if ((delay < 0) || cobs->active_coroutines_head) {
            delay = 0;
        }
        int nfds = epoll_wait(cobs->epoll_fd, cobs->epoll_event_vec, cobs->epoll_event_size, (int)delay);
        if ((nfds == -1) && (errno != EINTR)) {
            zcoroutine_fatal("epoll_wait: %m");
        }
        for (int i = 0; i < nfds; i++) {
            struct epoll_event *epev = cobs->epoll_event_vec + i;
            int fd = epev->data.fd;
            if (fd == cobs->event_fd) {
                uint64_t u;
                _syscall_read(fd, &u, sizeof(uint64_t));
                continue;
            }
            cfa = _co_fd_attribute_get(fd);
            if (!cfa) {
                zcoroutine_fatal("fd:%d be closed unexpectedly", fd);
                continue;
            }
            cfa->by_epoll = 1;
            cfa->revents = epev->events;
            co = cfa->co;
            if (co->active_list == 1) {
                ZMLINK_DETACH(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, prev, next);
                co->active_list = 0;
            }
            if (co->active_list == 0) {
                ZMLINK_APPEND(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, prev, next);
                co->active_list = 1;
            }
        }
        if ((nfds == cobs->epoll_event_size)&&(cobs->epoll_event_size<4096)) {
            _co_mem_free(cobs->epoll_event_vec);
            cobs->epoll_event_size *= 2;
            cobs->epoll_event_vec = (struct epoll_event *)_co_mem_malloc(cobs->epoll_event_size*sizeof(struct epoll_event));
        }

        /* */
        if (cobs->active_coroutines_head == 0) {
            continue;
        }
        zcoroutine_yield_my(cobs->self_coroutine);
    }
    if (cobs->deleted_coroutine_head) {
        zcoroutine_base_remove_coroutine(cobs);
    }
}

/* }}} */

/* {{{ coroutine poll */
static int _co_poll(zcoroutine_t *co, struct pollfd fds[], nfds_t nfds, int timeout)
{
    co->active_list = 0;
    co->ep_loop = 0;

    zcoroutine_base_t *cobs = co->base;
    long now_ms = zcoroutine_timeout_set_millisecond(0);
    int is_epoll_ctl = 0;

    do {
        int fdcount = 0;
        int last_fd = -1;
        for (nfds_t i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            int fd  = fds[i].fd;
            if (fd < 0) {
                continue;
            }
            fdcount++;
            last_fd = fd;
            if (fdcount > 1) {
                break;
            }
        }
        if (fdcount != 1) {
            break;
        }

        _co_fd_attribute_t *cfa = _co_fd_attribute_get(last_fd);
        if (cfa == 0) {
            return _syscall_poll(fds, nfds, timeout);
        }

    } while(0);

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        int fd  = fds[i].fd;
        if (fd < 0) {
            continue;
        }
        _co_fd_attribute_t *cfa = _co_fd_attribute_get(fd);
        if (cfa == 0) {
            cfa = _co_fd_attribute_create(fd);
            cfa->pseudo_mode = 1;
        }
        if (cfa->in_epoll) {
            zcoroutine_fatal("mutli monitor fd:%d", fd);
        }
        short int events = fds[i].events;
        unsigned int ep_events = 0;	
        if(events & POLLIN) 	ep_events |= EPOLLIN;
        if(events & POLLOUT)    ep_events |= EPOLLOUT;
        if(events & POLLHUP) 	ep_events |= EPOLLHUP;
        if(events & POLLERR)	ep_events |= EPOLLERR;
        if(events & POLLRDNORM) ep_events |= EPOLLRDNORM;
        if(events & POLLWRNORM) ep_events |= EPOLLWRNORM;
        struct epoll_event epev;
        epev.events = ep_events;
        epev.data.fd = fd;
        int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_ADD, fd, &epev);
        if ((eret<0) && (errno==EPERM) && (nfds==1)) {
            return _syscall_poll(fds, nfds, timeout);
        }
        is_epoll_ctl = 1;
        cfa->by_epoll = 0;
        cfa->co = co;
        cfa->timeout = now_ms + (timeout<0?zvar_coroutine_max_timeout_millisecond:timeout);
        _co_rbtree_attach(&(cobs->fd_timeout_zrbtree), &(cfa->rbnode));
    }

    if (is_epoll_ctl == 0) {
        co->sleep_timeout = now_ms + (timeout<0?zvar_coroutine_max_timeout_millisecond:timeout);
        _co_rbtree_attach(&(cobs->sleep_zrbtree), &(co->sleep_rbnode));
        co->inner_yield = 1;
        zcoroutine_yield_my(co);
        _co_rbtree_detach(&(cobs->sleep_zrbtree), &(co->sleep_rbnode));
        return 0;
    }

    co->inner_yield = 1;
    zcoroutine_yield_my(co);

    int raise_count = 0;
    for (nfds_t i = 0; i < nfds; i++) {
        int fd  = fds[i].fd;
        if (fd < 0) {
            continue;
        }
        _co_fd_attribute_t *cfa = _co_fd_attribute_get(fd);
        if (!cfa) {
            /* the fd be closed */
        } else {
            cfa->in_epoll = 0;
            unsigned int revents = cfa->revents;
            fds[i].revents = 0;
            if (cfa->by_epoll && revents) {
                raise_count++;
                short int p_events = 0;	
                if(revents & EPOLLIN)     p_events |= POLLIN;
                if(revents & EPOLLOUT)    p_events |= POLLOUT;
                if(revents & EPOLLHUP)    p_events |= POLLHUP;
                if(revents & EPOLLERR)    p_events |= POLLERR;
                if(revents & EPOLLRDNORM) p_events |= POLLRDNORM;
                if(revents & EPOLLWRNORM) p_events |= POLLWRNORM;
                fds[i].revents = p_events;
            }
            int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_DEL, fd, 0);
            if (eret < 0) {
                zcoroutine_fatal("epoll_ctl del fd:%d (%m)", fd);
            }
        }
        _co_rbtree_detach(&(cobs->fd_timeout_zrbtree), &(cfa->rbnode));
    }
    return raise_count;
}
/* }}} */

/* {{{ file io worker pthread */
static void zcoroutine_hook_fileio_worker_do(zcoroutine_hook_fileio_t *cio)
{
    zcoroutine_hook_arg_t *args = cio->args;
    zcoroutine_hook_arg_t *retval = &(cio->retval);
    int errno_bak = 0;
    if (cio->is_block_func) {
        retval->void_ptr_t = args[0].block_func(args[1].void_ptr_t);
        if (errno_bak) {
            cio->co_errno = errno_bak;
        } else {
            cio->co_errno = errno;
        }
        return;
    }

    switch(cio->cmdcode) {
        case zcoroutine_hook_fileio_open:
            retval->int_t = _syscall_open(args[0].char_ptr_t, args[1].int_t, args[2].int_t);
            if (retval->int_t > -1) {
                struct stat st;
                if (_syscall_fstat(retval->int_t, &st) == -1) {
                    errno_bak = errno;
                    zrobust_syscall_close(retval->int_t);
                    retval->int_t = -1;
                } else {
                    if (S_ISFIFO(st.st_mode)) {
                        cio->is_regular_file = 0;
                    } else {
                        cio->is_regular_file = 1;
                    }
                }
            }
            break;
        case zcoroutine_hook_fileio_openat:
            retval->int_t = _syscall_openat(args[0].int_t, args[1].char_ptr_t, args[2].int_t, args[3].int_t);
            if (retval->int_t > -1) {
                struct stat st;
                if (_syscall_fstat(retval->int_t, &st) == -1) {
                    errno_bak = errno;
                    zrobust_syscall_close(retval->int_t);
                    retval->int_t = -1;
                } else {
                    if (S_ISFIFO(st.st_mode)) {
                        cio->is_regular_file = 0;
                    } else {
                        cio->is_regular_file = 1;
                    }
                }
            }
            break;
        case zcoroutine_hook_fileio_read:
            retval->ssize_ssize_t = _syscall_read(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
            break;
        case zcoroutine_hook_fileio_readv:
            retval->ssize_ssize_t = _syscall_readv(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
            break;
        case zcoroutine_hook_fileio_write:
            retval->ssize_ssize_t = _syscall_write(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
            break;
        case zcoroutine_hook_fileio_writev:
            retval->ssize_ssize_t = _syscall_writev(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
            break;
        case zcoroutine_hook_fileio_lseek:
            retval->off_off_t = _syscall_lseek(args[0].int_t, args[1].off_off_t, args[2].int_t);
            break;
        case zcoroutine_hook_fileio_fdatasync:
            retval->int_t = _syscall_fdatasync(args[0].int_t);
            break;
        case zcoroutine_hook_fileio_fsync:
            retval->int_t = _syscall_fsync(args[0].int_t);
            break;
        case zcoroutine_hook_fileio_rename:
            retval->int_t = _syscall_rename(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_renameat:
            retval->int_t = _syscall_renameat(args[0].int_t, args[1].const_char_ptr_t, args[2].int_t, args[3].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_truncate:
            retval->int_t = _syscall_truncate(args[0].const_char_ptr_t, args[1].off_off_t);
            break;
        case zcoroutine_hook_fileio_ftruncate:
            retval->int_t = _syscall_ftruncate(args[0].int_t, args[1].off_off_t);
            break;
        case zcoroutine_hook_fileio_rmdir:
            retval->int_t = _syscall_rmdir(args[0].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_mkdir:
            retval->int_t = _syscall_mkdir(args[0].const_char_ptr_t, args[1].mode_mode_t);
            break;
#ifdef __NR_getdents
        case zcoroutine_hook_fileio_getdents:
            retval->int_t = _syscall_getdents(args[0].uint_t, args[1].void_ptr_t, args[1].uint_t);
            break;
#endif
#ifdef __NR_stat
        case zcoroutine_hook_fileio_stat:
            retval->int_t = _syscall_stat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
            break;
#endif
        case zcoroutine_hook_fileio_fstat:
            retval->int_t = _syscall_fstat(args[0].int_t, (struct stat *)args[0].char_ptr_t);
            break;
#ifdef __NR_lstat
        case zcoroutine_hook_fileio_lstat:
            retval->int_t = _syscall_lstat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
            break;
#endif
        case zcoroutine_hook_fileio_link:
            retval->int_t = _syscall_link(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_linkat:
            retval->int_t = _syscall_linkat(args[0].int_t, args[1].const_char_ptr_t, args[2].int_t, args[3].const_char_ptr_t, args[4].int_t);
            break;
        case zcoroutine_hook_fileio_symlink:
            retval->int_t = _syscall_symlink(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_symlinkat:
            retval->int_t = _syscall_symlinkat(args[0].const_char_ptr_t, args[1].int_t, args[2].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_readlink:
            retval->int_t = _syscall_readlink(args[0].const_char_ptr_t, args[1].char_ptr_t, args[2].size_size_t);
            break;
        case zcoroutine_hook_fileio_readlinkat:
            retval->int_t = _syscall_readlinkat(args[0].int_t, args[1].const_char_ptr_t, args[2].char_ptr_t, args[3].size_size_t);
            break;
        case zcoroutine_hook_fileio_unlink:
            retval->int_t = _syscall_unlink(args[0].const_char_ptr_t);
            break;
        case zcoroutine_hook_fileio_unlinkat:
            retval->int_t = _syscall_unlinkat(args[0].int_t, args[1].const_char_ptr_t, args[2].int_t);
            break;
#ifdef __NR_chmod
        case zcoroutine_hook_fileio_chmod:
            retval->int_t = _syscall_chmod(args[0].const_char_ptr_t, args[1].mode_mode_t);
            break;
#endif
        case zcoroutine_hook_fileio_fchmod:
            retval->int_t = _syscall_fchmod(args[0].int_t, args[1].mode_mode_t);
            break;
#ifdef __NR_chown
        case zcoroutine_hook_fileio_chown:
            retval->int_t = _syscall_chown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
            break;
#endif
        case zcoroutine_hook_fileio_fchown:
            retval->int_t = _syscall_fchown(args[0].int_t, args[1].uid_uid_t, args[2].gid_gid_t);
            break;
#ifdef __NR_lchown
        case zcoroutine_hook_fileio_lchown:
            retval->int_t = _syscall_lchown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
            break;
#endif
#ifdef __NR_utime
        case zcoroutine_hook_fileio_utime:
            retval->int_t = _syscall_utime(args[0].const_char_ptr_t, (const struct utimbuf *)args[1].char_ptr_t);
            break;
#endif
#ifdef __NR_utimes
        case zcoroutine_hook_fileio_utimes:
            retval->int_t = _syscall_utimes(args[0].const_char_ptr_t, (struct timeval *)args[1].char_ptr_t);
            break;
#endif
    }
    if (errno_bak) {
        cio->co_errno = errno_bak;
    } else {
        cio->co_errno = errno;
    }
}

static pthread_key_t gethostbyname_pthread_key;
void gethostbyname_pthread_key_destroy(void *buf)
{
    char **g = (char **)buf;
    if (g) {
        if (*g) {
            _co_mem_free(*g);
        }
        _co_mem_free(g);
    }
    pthread_setspecific(gethostbyname_pthread_key, 0);
}

static void _hook_fileio_worker_init()
{
    pthread_detach(pthread_self());
    pthread_key_create(&gethostbyname_pthread_key, gethostbyname_pthread_key_destroy);
    char **g = (char **)_co_mem_malloc(sizeof(char **));
    *g = 0;
    pthread_setspecific(gethostbyname_pthread_key, g);
}

static void *zcoroutine_hook_fileio_worker(void *arg)
{
    _hook_fileio_worker_init();
    while (1) {
        zpthread_lock(&zvar_coroutine_hook_fileio_lock);
        while(!zvar_coroutine_hook_fileio_head) {
            struct timespec ts;
            ts.tv_sec = time(0) + 1;
            ts.tv_nsec = 0;
            pthread_cond_timedwait(&zvar_coroutine_hook_fileio_cond, &zvar_coroutine_hook_fileio_lock, &ts);
        }
        zcoroutine_hook_fileio_t *cio = zvar_coroutine_hook_fileio_head;
        ZMLINK_DETACH(zvar_coroutine_hook_fileio_head, zvar_coroutine_hook_fileio_tail, cio, prev, next);
        zvar_coroutine_hook_fileio_count--;
        zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
        zcoroutine_hook_fileio_worker_do(cio);

        zpthread_lock(&zvar_coroutine_hook_fileio_lock);
        zcoroutine_t  *co = cio->current_coroutine;
        zcoroutine_base_t *cobs = co->base;
        ZMLINK_APPEND(cobs->fileio_coroutines_head, cobs->fileio_coroutines_tail, co, prev, next);
        zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
        uint64_t u = 1;
        _syscall_write(cobs->event_fd, &u, sizeof(uint64_t));
    }
    return arg;
}
/* }}} */

/* {{{ general read/write wait */
static int _co_general_read_wait(int fd)
{
    _co_fd_attribute_t *cfa =  _co_fd_attribute_get(fd);
    struct pollfd pf;
    pf.fd = fd;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    pf.revents = 0;
    poll(&pf, 1, cfa->read_timeout);
    return pf.revents;
}

static int _co_general_write_wait(int fd)
{
    _co_fd_attribute_t *cfa =  _co_fd_attribute_get(fd);
    struct pollfd pf;
    pf.fd = fd;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    pf.revents = 0;
    poll(&pf, 1, cfa->write_timeout);
    return pf.revents;
}

/* }}} */

/* {{{ sleep */
zinline static long _zmillisecond(void)
{
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return r;
}

unsigned int sleep(unsigned int seconds)
{
    int left = seconds * 1000;
    if (left < 1) {
        return 0;
    }
    long end = _zmillisecond() + left + 1;
    poll(0, 0, (int)left);
    left = end - _zmillisecond();
    if (left <10) {
        return 0;
    }
    return (left/1000);
}

void zcoroutine_sleep_millisecond(int milliseconds)
{
    int left = milliseconds;
    if (left < 1) {
        return;
    }

    long end = _zmillisecond() + left + 1;
    while (left > 0) {
        poll(0, 0, left);
        left = end - _zmillisecond();
    }
}
/* }}} */

/* {{{ hook */
/* {{{ poll hook */
int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    if (timeout == 0) {
        return _syscall_poll(fds, nfds, 0);
    }
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if ((cobs == 0) || (cobs->current_coroutine == 0)) {
        return _syscall_poll(fds, nfds, timeout);
    }
    return  _co_poll(cobs->current_coroutine, fds, nfds, timeout);
}

int __poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return poll(fds, nfds, timeout);
}

/* }}} */

/* {{{ pipe hook */
int pipe(int pipefd[2])
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    int ret = _syscall_pipe(pipefd);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    _co_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    _co_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ pipe2 hook */
int pipe2(int pipefd[2], int flags)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    int ret = _syscall_pipe2(pipefd, flags);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    _co_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    _co_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ dup hook */
int dup(int oldfd)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    int newfd = _syscall_dup(oldfd);
    if (newfd < 0) {
        return newfd;
    }
    if (!cobs) {
        return newfd;
    }
    _co_fd_attribute_t *cfa = _co_fd_attribute_get(oldfd);
    _co_fd_attribute_t *new_cfa = _co_fd_attribute_create(newfd);
    if (cfa) {
        new_cfa->is_regular_file = cfa->is_regular_file;
        new_cfa->pseudo_mode = cfa->pseudo_mode;
        new_cfa->read_timeout = cfa->read_timeout;
        new_cfa->write_timeout = cfa->write_timeout;
        if (cfa && (cfa->pseudo_mode == 0)) {
            fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0));
        }
    }
    return newfd;
}
/* }}} */

/* {{{ dup2 hook */
int dup2(int oldfd, int newfd)
{
    int ret;
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if ((ret = _syscall_dup2(oldfd, newfd)) < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    _co_fd_attribute_t *cfa = _co_fd_attribute_get(oldfd);
    if (cfa && (cfa->pseudo_mode == 0)) {
        cfa = _co_fd_attribute_get(newfd);
        if (cfa) {
            zcoroutine_fatal("the newfd be used by other zcoroutine_t");
#if 0
            /* note: the newfd be used by other zcoroutine_t. */
            _co_fd_attribute_free(newfd);
            _co_fd_attribute_create(newfd);
#endif
        } else {
            _co_fd_attribute_t *new_cfa = _co_fd_attribute_create(newfd);
            new_cfa->is_regular_file = cfa->is_regular_file;
            new_cfa->pseudo_mode = cfa->pseudo_mode;
            new_cfa->read_timeout = cfa->read_timeout;
            new_cfa->write_timeout = cfa->write_timeout;
        }
        fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0));
    }
    return ret;
}
/* }}} */

/* {{{ dup3 hook */
int dup3(int oldfd, int newfd, int flags)
{
    int ret;
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if ((ret = _syscall_dup3(oldfd, newfd, flags)) < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    _co_fd_attribute_t *cfa = _co_fd_attribute_get(oldfd);
    if (cfa && (cfa->pseudo_mode == 0)) {
        cfa = _co_fd_attribute_get(newfd);
        if (cfa) {
            zcoroutine_fatal("the newfd be used by other zcoroutine_t");
#if 0
            /* note: the newfd be used by other zcoroutine_t. */
            _co_fd_attribute_free(newfd);
            _co_fd_attribute_create(newfd);
#endif
        } else {
            _co_fd_attribute_t *new_cfa = _co_fd_attribute_create(newfd);
            new_cfa->is_regular_file = cfa->is_regular_file;
            new_cfa->pseudo_mode = cfa->pseudo_mode;
            new_cfa->read_timeout = cfa->read_timeout;
            new_cfa->write_timeout = cfa->write_timeout;
        }
        fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0));
    }
    return ret;
}
/* }}} */

/* {{{ socketpair hook */
int socketpair(int domain, int type, int protocol, int sv[2])
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    int ret = _syscall_socketpair(domain, type, protocol, sv);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    _co_fd_attribute_create(sv[0]);
    fcntl(sv[0], F_SETFL, fcntl(sv[0], F_GETFL, 0));
    _co_fd_attribute_create(sv[1]);
    fcntl(sv[1], F_SETFL, fcntl(sv[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ open hook */
int open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_open(pathname, flags, mode);
    }
    zcoroutine_hook_fileio_run_part2(open);
    fileio.args[0].void_ptr_t = (void *)pathname;
    fileio.args[1].int_t = flags;
    fileio.args[2].int_t = mode;
    zcoroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        _co_fd_attribute_t *fdatts = _co_fd_attribute_create(retfd);
        fdatts->is_regular_file = fileio.is_regular_file;
        if (fileio.is_regular_file == 0) {
            fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
        }
    }
    return retfd;
}

int openat(int dirid, const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_openat(dirid, pathname, flags, mode);
    }
    zcoroutine_hook_fileio_run_part2(openat);
    fileio.args[0].int_t = dirid;
    fileio.args[1].void_ptr_t = (void *)pathname;
    fileio.args[2].int_t = flags;
    fileio.args[3].int_t = mode;
    zcoroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        _co_fd_attribute_t *fdatts = _co_fd_attribute_create(retfd);
        fdatts->is_regular_file = fileio.is_regular_file;
        if (fileio.is_regular_file == 0) {
            fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
        }
    }
    return retfd;
}
/* }}} */

/* {{{ creat hook */
int creat(const char *pathname, mode_t mode)
{
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC, mode);
}
/* }}} */

/* {{{ socket hook */
int socket(int domain, int type, int protocol)
{
    int fd = _syscall_socket(domain, type, protocol);
    if(fd < 0) {
        return fd;
    }
    if ((zvar_coroutine_disable_udp == 1) && (type & SOCK_DGRAM)) {
        return fd;
    }

    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if (cobs == 0) {
        return fd;
    }

    if ((zvar_coroutine_disable_udp_53 == 1) && (type & SOCK_DGRAM)) {
        _co_fd_attribute_t *cfa = _co_fd_attribute_create(fd);
        if (cfa) {
            cfa->is_udp = 1;
        }
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0));

    return fd;
}
/* }}} */

/* {{{ return_zc_call */
#define return_zc_call_co(fd)  \
    zcoroutine_base_t *cobs = 0; \
    _co_fd_attribute_t *fdatts = 0; \
    if (((cobs = zcoroutine_base_get_current_inner()) ==0) \
            || ((fdatts = _co_fd_attribute_get(fd)) == 0)  \
            || (fdatts->nonblock == 1) \
            || (fdatts->pseudo_mode == 1))

/* }}} */

/* {{{ accept hook */
int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    const int ___accept_timeout = 100 * 1000;
    return_zc_call_co(fd) {
        int sock = _syscall_accept(fd, addr, len);
        if (cobs && (sock > -1)) {
            _co_fd_attribute_create(sock);
            fcntl(sock, F_SETFL, _syscall_fcntl(sock, F_GETFL,0));
        }
        return sock;
    }
    struct pollfd pf;
    memset(&pf,0,sizeof(pf));
    pf.fd = fd;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    poll(&pf, 1, ___accept_timeout);
    if (pf.revents & (POLLERR|POLLHUP)) {
        errno = ECONNABORTED;
        return -1;
    }
    if (!(pf.revents & (POLLIN))) {
        errno = EINTR;
        return -1;
    }

    int sock = _syscall_accept(fd, addr, len);
    if (sock > -1) {
        _co_fd_attribute_create(sock);
        fcntl(sock, F_SETFL, _syscall_fcntl(sock, F_GETFL,0));
    } else {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return sock;
}

/* }}} */

/* {{{ connect hook */
int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    const int ___connect_timeout = 100 * 1000;

    if (zvar_coroutine_disable_udp_53) {
        _co_fd_attribute_t *tmp_fdatts = _co_fd_attribute_get(fd);
        if (tmp_fdatts && tmp_fdatts->is_udp) {
            const struct sockaddr_in *addr = (struct sockaddr_in *)address;
            if ((addr->sin_family == AF_INET) && (ntohs(addr->sin_port) == 53)) {
                zcoroutine_disable_fd(fd);
            }
        }
    }

    int ret = _syscall_connect(fd, address, address_len);
    return_zc_call_co(fd) {
        return ret;
    }

    if (!((ret < 0) && (errno == EINPROGRESS))) {
        return ret;
    }

    struct pollfd pf;
    memset(&pf,0,sizeof(pf));
    pf.fd = fd;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    poll(&pf, 1, ___connect_timeout);
    if (pf.revents & POLLOUT) {
        errno = 0;
        return 0;
    }

    int err = 0;
    socklen_t errlen = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if(err) {
        errno = err;
    } else {
        errno = ETIMEDOUT;
    } 
    return ret;
}
/* }}} */

/* {{{ close hook */
int close(int fd)
{
    int ret;
    zcoroutine_hook_fileio_run_part1() {
        ret = zrobust_syscall_close(fd);
        if (ret > -1) {
            _co_fd_attribute_free(fd);
        }
        return ret;
    }

    _co_fd_attribute_t *fdatts = 0;
    if (((fdatts = _co_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        ret = zrobust_syscall_close(fd);
        if (ret > -1) {
            _co_fd_attribute_free(fd);
        }
        return ret;
    }
    if (fdatts->is_regular_file) {
        zcoroutine_hook_fileio_run_part2(close);
        fileio.args[0].int_t = fd;
        zcoroutine_hook_fileio_run_part3();
        ret = retval.int_t;
        if (ret > -1) {
            _co_fd_attribute_free(fd);
        }
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return ret;
    }
    ret = zrobust_syscall_close(fd);
    if (ret > -1) {
        _co_fd_attribute_free(fd);
    }
    return ret;
}
/* }}} */

/* {{{ read hook */
ssize_t read(int fd, void *buf, size_t nbyte)
{
    zcoroutine_hook_fileio_run_part0() {
        return _syscall_read(fd, buf, nbyte);
    }

    _co_fd_attribute_t *fdatts = 0;
    if (((fdatts = _co_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return _syscall_read(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return _syscall_read(fd, buf, nbyte);
        }
        zcoroutine_hook_fileio_run_part2(read);
        fileio.args[0].int_t = fd;
        fileio.args[1].void_ptr_t = buf;
        fileio.args[2].size_size_t = nbyte;
        zcoroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return _syscall_read(fd, buf, nbyte);
    }
#if 0
    _co_general_read_wait(fd);
    ssize_t readret = _syscall_read(fd,(char*)buf ,nbyte);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return readret;
#else 
    while(1) {
        _co_general_read_wait(fd);
        ssize_t readret = _syscall_read(fd,(char*)buf ,nbyte);
        int ec = errno;
        if ((readret >= 0) || (ec != EAGAIN)) {
            return readret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ readv hook */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    zcoroutine_hook_fileio_run_part0() {
        return _syscall_readv(fd, iov, iovcnt);
    }

    _co_fd_attribute_t *fdatts = 0;
    if (((fdatts = _co_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return _syscall_readv(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return _syscall_readv(fd, iov, iovcnt);
        }
        zcoroutine_hook_fileio_run_part2(readv);
        fileio.args[0].int_t = fd;
        fileio.args[1].const_iovec_t = iov;
        fileio.args[2].int_t = iovcnt;
        zcoroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return _syscall_readv(fd, iov, iovcnt);
    }
#if 0
    _co_general_read_wait(fd);
    ssize_t readret = _syscall_readv(fd, iov, iovcnt);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return readret;
#else
    while(1) {
        _co_general_read_wait(fd);
        ssize_t readret = _syscall_readv(fd, iov, iovcnt);
        int ec = errno;
        if ((readret >= 0) || (ec != EAGAIN)) {
            return readret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ write hook */
ssize_t write(int fd, const void *buf, size_t nbyte)
{
    zcoroutine_hook_fileio_run_part0() {
        return _syscall_write(fd, buf, nbyte);
    }

    _co_fd_attribute_t *fdatts = 0;
    if (((fdatts = _co_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return _syscall_write(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return _syscall_write(fd, buf, nbyte);
        }
        zcoroutine_hook_fileio_run_part2(write);
        fileio.args[0].int_t = fd;
        fileio.args[1].void_ptr_t = (void *)buf;
        fileio.args[2].size_size_t = nbyte;
        zcoroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return _syscall_write(fd, buf, nbyte);
    }

#if 0
    _co_general_write_wait(fd);
    ssize_t writeret = _syscall_write(fd, buf ,nbyte);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return writeret;
#else
    while(1) {
        _co_general_write_wait(fd);
        ssize_t writeret = _syscall_write(fd, buf ,nbyte);
        int ec = errno;
        if ((writeret >= 0) || (ec != EAGAIN)) {
            return writeret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ writev hook */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    zcoroutine_hook_fileio_run_part0() {
        return _syscall_writev(fd, iov, iovcnt);
    }

    _co_fd_attribute_t *fdatts = 0;
    if (((fdatts = _co_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return _syscall_writev(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return _syscall_writev(fd, iov, iovcnt);
        }
        zcoroutine_hook_fileio_run_part2(writev);
        fileio.args[0].int_t = fd;
        fileio.args[1].const_iovec_t = iov;
        fileio.args[2].int_t = iovcnt;
        zcoroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return _syscall_writev(fd, iov, iovcnt);
    }
#if 0
    _co_general_write_wait(fd);
    ssize_t writeret = _syscall_writev(fd, iov, iovcnt);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return writeret;
#else
    while(1) {
        _co_general_write_wait(fd);
        ssize_t writeret = _syscall_writev(fd, iov, iovcnt);
        int ec = errno;
        if ((writeret >= 0) || (ec != EAGAIN)) {
            return writeret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ sendto hook */
ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    int ret;
    return_zc_call_co(socket) {
        return _syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    }
#if 0
    _co_general_write_wait(socket);
    ret = _syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        _co_general_write_wait(socket);
        ret = _syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
        int ec = errno;
        if ((ret >= 0) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ recvfrom hook */
ssize_t recvfrom(int socket, void *buf, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return_zc_call_co(socket) {
        return _syscall_recvfrom(socket,buf,length,flags,address,address_len);
    }
#if 0
    _co_general_read_wait(socket);
    ssize_t ret = _syscall_recvfrom(socket,buf,length,flags,address,address_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        _co_general_read_wait(socket);
        ssize_t ret = _syscall_recvfrom(socket,buf,length,flags,address,address_len);
        int ec = errno;
        if ((ret >= 0) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ send hook */
ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    return_zc_call_co(socket) {
        return _syscall_send(socket,buffer,length,flags);
    }
#if 0
    _co_general_write_wait(socket);
    int ret = _syscall_send(socket,(const char*)buffer, length, flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        _co_general_write_wait(socket);
        int ret = _syscall_send(socket,(const char*)buffer, length, flags);
        int ec = errno;
        if ((ret >= 0) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif

}
/* }}} */

/* {{{ recv hook */
ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    return_zc_call_co(socket) {
        return _syscall_recv(socket,buffer,length,flags);
    }
#if 0
    _co_general_read_wait(socket);
    ssize_t ret = _syscall_recv(socket,buffer,length,flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        _co_general_read_wait(socket);
        ssize_t ret = _syscall_recv(socket,buffer,length,flags);
        int ec = errno;
        if ((ret >= 0) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ setsockopt hook */
int setsockopt(int fd, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return_zc_call_co(fd) {
        return _syscall_setsockopt(fd,level,option_name,option_value,option_len);
    }

    if(SOL_SOCKET == level) {
        struct timeval *val = (struct timeval*)option_value;
        long t = val->tv_sec * 1000 + val->tv_usec/1000;
        if (t > 1000 * 60 * 24 * 10) {
            t = 1000 * 60 * 24 * 10;
        }
        if (t < 0) {
            t = 1;
        }
        if(SO_RCVTIMEO == option_name ) {
            fdatts->read_timeout = t;
        } else if(SO_SNDTIMEO == option_name) {
            fdatts->write_timeout = t;
        }
    }
    return _syscall_setsockopt(fd,level,option_name,option_value,option_len);
}
/* }}} */

/* {{{ fcntl hook */
int fcntl(int fildes, int cmd, ...)
{
    if(fildes < 0) {
        errno = EINVAL;
        return -1;
    }
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();

    va_list args;
    va_start(args,cmd);

    int ret = -1;
    switch(cmd)
    {
        case F_DUPFD:
            {
                int param = va_arg(args,int);
                ret = _syscall_fcntl(fildes,cmd,param);
                if (cobs == 0) {
                    break;
                }
                if (ret > -1) {
                    _co_fd_attribute_t *cfa = _co_fd_attribute_get(fildes);
                    if (cfa && (cfa->pseudo_mode == 0)) {
                        _co_fd_attribute_create(ret);
                        fcntl(ret, F_SETFL, _syscall_fcntl(ret, F_GETFL,0));
                    }
                }
                break;
            }
        case F_GETFD:
            {
                ret = _syscall_fcntl(fildes,cmd);
                break;
            }
        case F_SETFD:
            {
                int param = va_arg(args,int);
                ret = _syscall_fcntl(fildes,cmd,param);
                break;
            }
        case F_GETFL:
            {
                ret = _syscall_fcntl(fildes,cmd);
                break;
            }
        case F_SETFL:
            {
                int param = va_arg(args,int);
                if (cobs == 0) {
                    ret = _syscall_fcntl(fildes,cmd,param);
                    break;
                }
                int flag = param;
                _co_fd_attribute_t *cfa = _co_fd_attribute_get(fildes);
                if (cfa) {
                    flag |= O_NONBLOCK;
                }
                ret = _syscall_fcntl(fildes,cmd,flag);
                if((0 == ret) && cfa) {
                    cfa->nonblock = ((param&O_NONBLOCK)?1:0);
                }
                break;
            }
        case F_GETOWN:
            {
                ret = _syscall_fcntl(fildes,cmd);
                break;
            }
        case F_SETOWN:
            {
                int param = va_arg(args,int);
                ret = _syscall_fcntl(fildes,cmd,param);
                break;
            }
        case F_GETLK:
            {
                struct flock *param = va_arg(args,struct flock *);
                ret = _syscall_fcntl(fildes,cmd,param);
                break;
            }
        case F_SETLK:
            {
                struct flock *param = va_arg(args,struct flock *);
                ret = _syscall_fcntl(fildes,cmd,param);
                break;
            }
        case F_SETLKW:
            {
                struct flock *param = va_arg(args,struct flock *);
                ret = _syscall_fcntl(fildes,cmd,param);
                break;
            }
    }
    va_end(args);

    return ret;
}
/* }}} */
/* }}} */

/* file io hook {{{ */
off_t lseek(int fd, off_t offset, int whence)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_lseek(fd, offset, whence);
    }
    zcoroutine_hook_fileio_run_part2(lseek);
    fileio.args[0].int_t = fd;
    fileio.args[1].off_off_t = offset;
    fileio.args[2].int_t = whence;
    zcoroutine_hook_fileio_run_part3();
    return retval.off_off_t;
}

int fdatasync(int fd)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_fdatasync(fd);
    }
    zcoroutine_hook_fileio_run_part2(fdatasync);
    fileio.args[0].int_t = fd;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fsync(int fd)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_fsync(fd);
    }
    zcoroutine_hook_fileio_run_part2(fsync);
    fileio.args[0].int_t = fd;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int rename(const char *oldpath, const char *newpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_rename(oldpath, newpath);
    }
    zcoroutine_hook_fileio_run_part2(rename);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_renameat(olddirfd, oldpath, newdirfd, newpath);
    }
    zcoroutine_hook_fileio_run_part2(renameat);
    fileio.args[0].int_t = olddirfd;
    fileio.args[1].const_char_ptr_t = newpath;
    fileio.args[2].int_t = olddirfd;
    fileio.args[3].const_char_ptr_t = newpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int truncate(const char *path, off_t length)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_truncate(path, length);
    }
    zcoroutine_hook_fileio_run_part2(truncate);
    fileio.args[0].const_char_ptr_t = path;
    fileio.args[1].long_t = (long)length;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int ftruncate(int fd, off_t length)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_ftruncate(fd, length);
    }
    zcoroutine_hook_fileio_run_part2(ftruncate);
    fileio.args[0].int_t = fd;
    fileio.args[1].long_t = (long)length;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int rmdir(const char *pathname)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_rmdir(pathname);
    }
    zcoroutine_hook_fileio_run_part2(rmdir);
    fileio.args[0].const_char_ptr_t = pathname;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int mkdir(const char *pathname, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_mkdir(pathname, mode);
    }
    zcoroutine_hook_fileio_run_part2(mkdir);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

#ifdef __NR_getdents
int getdents(unsigned int fd, char *dirp, unsigned int count)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_getdents(fd, (void *)dirp, count);
    }
    zcoroutine_hook_fileio_run_part2(getdents);
    fileio.args[0].uint_t = fd;
    fileio.args[1].void_ptr_t = dirp;
    fileio.args[1].uint_t = count;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

#ifdef __NR_stat
int stat(const char *pathname, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_stat(pathname, buf);
    }
    zcoroutine_hook_fileio_run_part2(stat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

int fstat(int fd, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_fstat(fd, buf);
    }
    zcoroutine_hook_fileio_run_part2(fstat);
    fileio.args[0].int_t = fd;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

#ifdef __NR_lstat
int lstat(const char *pathname, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_lstat(pathname, buf);
    }
    zcoroutine_hook_fileio_run_part2(lstat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

int link(const char *oldpath, const char *newpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_link(oldpath, newpath);
    }
    zcoroutine_hook_fileio_run_part2(link);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_linkat(olddirfd, oldpath, newdirfd, newpath, flags);
    }
    zcoroutine_hook_fileio_run_part2(linkat);
    fileio.args[0].int_t = olddirfd;
    fileio.args[1].const_char_ptr_t = oldpath;
    fileio.args[2].int_t = newdirfd;
    fileio.args[3].const_char_ptr_t = newpath;
    fileio.args[4].int_t = flags;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int symlink(const char *target, const char *linkpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_symlink(target, linkpath);
    }
    zcoroutine_hook_fileio_run_part2(symlink);
    fileio.args[0].const_char_ptr_t = target;
    fileio.args[1].const_char_ptr_t = linkpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int symlinkat(const char *target, int newdirfd, const char *linkpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_symlinkat(target, newdirfd, linkpath);
    }
    zcoroutine_hook_fileio_run_part2(symlinkat);
    fileio.args[0].const_char_ptr_t = target;
    fileio.args[1].int_t = newdirfd;
    fileio.args[2].const_char_ptr_t = linkpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_readlink(pathname, buf, bufsiz);
    }
    zcoroutine_hook_fileio_run_part2(readlink);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].char_ptr_t = buf;
    fileio.args[2].size_size_t = bufsiz;
    zcoroutine_hook_fileio_run_part3();
    return retval.ssize_ssize_t;
}

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_readlinkat(dirfd, pathname, buf, bufsiz);
    }
    zcoroutine_hook_fileio_run_part2(readlinkat);
    fileio.args[0].int_t = dirfd;
    fileio.args[1].const_char_ptr_t = pathname;
    fileio.args[2].char_ptr_t = buf;
    fileio.args[3].size_size_t = bufsiz;
    zcoroutine_hook_fileio_run_part3();
    return retval.ssize_ssize_t;
}

int unlink(const char *pathname)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_unlink(pathname);
    }
    zcoroutine_hook_fileio_run_part2(unlink);
    fileio.args[0].const_char_ptr_t = pathname;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int unlinkat(int dirfd, const char *pathname, int flags)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_unlinkat(dirfd, pathname, flags);
    }
    zcoroutine_hook_fileio_run_part2(unlinkat);
    fileio.args[0].int_t = dirfd;
    fileio.args[1].const_char_ptr_t = pathname;
    fileio.args[2].int_t = flags;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

#ifdef __NR_chmod
int chmod(const char *pathname, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_chmod(pathname, mode);
    }
    zcoroutine_hook_fileio_run_part2(chmod);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

int fchmod(int fd, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_fchmod(fd, mode);
    }
    zcoroutine_hook_fileio_run_part2(fchmod);
    fileio.args[0].int_t = fd;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

#ifdef __NR_chown
int chown(const char *pathname, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_chown(pathname, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(chown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

int fchown(int fd, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_fchown(fd, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(fchown);
    fileio.args[0].int_t = fd;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

#ifdef __NR_lchown
int lchown(const char *pathname, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_lchown(pathname, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(lchown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

#ifdef __NR_utime
int utime(const char *filename, const struct utimbuf *times)
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_utime(filename, times);
    }
    zcoroutine_hook_fileio_run_part2(utime);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

#ifdef __NR_utimes
int utimes(const char *filename, const struct timeval times[2])
{
    zcoroutine_hook_fileio_run_part1() {
        return _syscall_utimes(filename, times);
    }
    zcoroutine_hook_fileio_run_part2(utimes);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}
#endif

/* }}} */

/* {{{ __res_state hook */
extern __thread struct __res_state __resp;
struct __res_state *__res_state()
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    if (cobs == 0) {
        if (__resp.options == 0) {
            res_ninit(&__resp);
        }
        return &__resp;
    }
    if (cobs->current_coroutine->res_state == 0) {
        cobs->current_coroutine->res_state = (struct __res_state *)_co_mem_calloc(1, sizeof(struct __res_state));
        if (cobs->current_coroutine->res_state->options == 0) {
            res_ninit(cobs->current_coroutine->res_state);
        }
    }
    return cobs->current_coroutine->res_state;
}
/* }}} */

/* {{{ gethostbyname hook */
struct hostent* gethostbyname2(const char* name, int af)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    char **_hp_char_ptr = 0;
    zgethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(gethostbyname_pthread_key);
        _hp = (zgethostbyname_buf_t *)(*_hp_char_ptr);
    }

    if (_hp && (_hp->buf_size > 1024)) {
        _co_mem_free(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zgethostbyname_buf_t *)_co_mem_malloc(sizeof(zgethostbyname_buf_t) + 1024 + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zgethostbyname_buf_t);
        _hp->buf_size = 1024;
        if (cobs) {
            cobs->current_coroutine->gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    }
    struct hostent *host = &(_hp->host);
    struct hostent *result = NULL;
    int *h_errnop = &(_hp->h_errno2);

    int ret = -1;
    while (((ret = gethostbyname2_r(name, af, host, _hp->buf_ptr, _hp->buf_size, &result, h_errnop)) == ERANGE) && (*h_errnop == NETDB_INTERNAL)) {
        size_t nsize = _hp->buf_size * 2;
        _co_mem_free(_hp);
        _hp = (zgethostbyname_buf_t *)_co_mem_malloc(sizeof(zgethostbyname_buf_t) + nsize + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zgethostbyname_buf_t);
        _hp->buf_size = nsize;
        if (cobs) {
            cobs->current_coroutine->gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    } 
    if ((ret == 0) && (host == result)){
        return host;
    }
    h_errno = _hp->h_errno2;
    return 0;
}

struct hostent *gethostbyname(const char *name)
{
    return gethostbyname2(name, AF_INET);
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current_inner();
    char **_hp_char_ptr = 0;
    zgethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(gethostbyname_pthread_key);
        _hp = (zgethostbyname_buf_t *)(*_hp_char_ptr);
    }

    if (_hp && (_hp->buf_size > 1024)) {
        _co_mem_free(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zgethostbyname_buf_t *)_co_mem_malloc(sizeof(zgethostbyname_buf_t) + 1024 + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zgethostbyname_buf_t);
        _hp->buf_size = 1024;
        if (cobs) {
            cobs->current_coroutine->gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    }
    struct hostent *host = &(_hp->host);
    struct hostent *result = NULL;
    int *h_errnop = &(_hp->h_errno2);

    int ret = -1;
    while (((ret = gethostbyaddr_r(addr, len, type, host, _hp->buf_ptr, _hp->buf_size, &result, h_errnop)) == ERANGE) && (*h_errnop == NETDB_INTERNAL)) {
        size_t nsize = _hp->buf_size * 2;
        _co_mem_free(_hp);
        _hp = (zgethostbyname_buf_t *)_co_mem_malloc(sizeof(zgethostbyname_buf_t) + nsize + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zgethostbyname_buf_t);
        _hp->buf_size = nsize;
        if (cobs) {
            cobs->current_coroutine->gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    } 
    if ((ret == 0) && (host == result)){
        return host;
    }
    h_errno = _hp->h_errno2;
    return 0;
}

/* }}} */

/* {{{ block common utils  */
void *zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx)
{
    zcoroutine_base_t *cobs = 0;
    if ((zvar_coroutine_block_pthread_count_limit<1) || ((cobs = zcoroutine_base_get_current_inner())==0)) {
        return block_func(ctx);
    }
    zcoroutine_hook_fileio_run_part2(unknown);
    fileio.args[0].block_func = block_func;
    fileio.args[1].void_ptr_t = ctx;
    fileio.is_block_func = 1;
    zcoroutine_hook_fileio_run_part3();
    return retval.void_ptr_t;
}

static void *_pwrite(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    int fd = args[1].INT;
    const char *data = args[2].CONST_CHAR_PTR;
    int len = args[3].INT, wlen = 0, ret;
    long offset = args[4].LONG;

    if (offset > 0) {
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            args[0].INT = -1;
            return 0;
        }
    }
    while (len > wlen) {
        ret = _syscall_write(fd, data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
    if (len != wlen) {
        args[0].INT = -1;
    } else {
        args[0].INT = len;
    }

    return 0;
}

int zcoroutine_block_pwrite(int fd, const void *data, int len, long offset)
{
    _co_type_convert_t args[5];
    args[0].INT = -1;
    args[1].INT = fd;
    args[2].CONST_CHAR_PTR = data;
    args[3].INT = len;
    args[4].LONG = offset;
    zcoroutine_block_do(_pwrite, args);
    return args[0].INT;
}

static void *_write(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    int fd = args[1].INT;
    const char *data = args[2].CONST_CHAR_PTR;
    int len = args[3].INT, wlen = 0, ret;

    while (len > wlen) {
        ret = _syscall_write(fd, data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
    if (len != wlen) {
        args[0].INT = -1;
    } else {
        args[0].INT = len;
    }

    return 0;
}

int zcoroutine_block_write(int fd, const void *data, int len)
{
    _co_type_convert_t args[4];
    args[0].INT = -1;
    args[1].INT = fd;
    args[2].CONST_CHAR_PTR = data;
    args[3].INT = len;
    zcoroutine_block_do(_write, args);
    return args[0].INT;
}

static void *_lseek(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    int fd = args[1].INT;
    long offset = args[2].LONG;;
    int whence = args[3].INT;
    args[0].LONG = lseek(fd, offset, whence);
    return 0;
}

long zcoroutine_block_lseek(int fd, long offset, int whence)
{
    _co_type_convert_t args[4];
    args[0].LONG = -1;
    args[1].INT = fd;
    args[2].LONG = offset;
    args[3].INT = whence;
    zcoroutine_block_do(_lseek, args);
    return args[0].LONG;
}

static void *_open(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    const char *pathname = args[1].CONST_CHAR_PTR;
    int flags = args[2].INT;
    mode_t mode = args[3].INT;
    do {
        args[0].INT = _syscall_open(pathname, flags, mode);
    } while ((args[0].INT < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_open(const char *pathname, int flags, mode_t mode)
{
    _co_type_convert_t args[4];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = pathname;
    args[2].INT = flags;
    args[3].INT = mode;
    zcoroutine_block_do(_open, args);
    return args[0].INT;
}

static void *_close(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    int fd = args[1].INT;
    do {
        args[0].INT = _syscall_close(fd);
    } while ((args[0].INT < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_close(int fd)
{
    _co_type_convert_t args[2];
    args[0].INT = -1;
    args[1].INT = fd;
    zcoroutine_block_do(_close, args);
    return args[0].INT;
}

static void *_rename(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    const char *oldpath = args[1].CONST_CHAR_PTR;
    const char *newpath = args[2].CONST_CHAR_PTR;
    do {
        args[0].INT = _syscall_rename(oldpath, newpath);
    } while ((args[0].INT < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_rename(const char *oldpath, const char *newpath)
{
    _co_type_convert_t args[3];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = oldpath;
    args[2].CONST_CHAR_PTR = newpath;
    zcoroutine_block_do(_rename, args);
    return args[0].INT;
}

static void *_unlink(void *ctx)
{
    _co_type_convert_t *args = (_co_type_convert_t *)ctx;
    const char *pathname = args[1].CONST_CHAR_PTR;
    do {
        args[0].INT = _syscall_unlink(pathname);
    } while ((args[0].INT < 0) && (errno==EINTR));
    return 0;
}

int zcoroutine_block_unlink(const char *pathname)
{
    _co_type_convert_t args[2];
    args[0].INT = -1;
    args[1].CONST_CHAR_PTR = pathname;
    zcoroutine_block_do(_unlink, args);
    return args[0].INT;
}

/* }}} */

#ifdef  __cplusplus
}
#endif

#pragma pack(pop)

/* Local variables:
 * End:
 * vim600: fdm=marker
 */
