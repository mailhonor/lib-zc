/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-06-26
 * ================================
 */

#pragma GCC diagnostic ignored "-Wredundant-decls"
#include "zc.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#define zpthread_lock(l)    {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}
#define zpthread_unlock(l)  {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}

#pragma pack(push, 4)
/* {{{ syscall */
int zsyscall_pipe(int pipefd[2]);
int zsyscall_pipe2(int pipefd[2], int flags);
int zsyscall_dup(int oldfd);
int zsyscall_dup2(int oldfd, int newfd);
int zsyscall_dup3(int oldfd, int newfd, int flags);
int zsyscall_socketpair(int domain, int type, int protocol, int sv[2]);
int zsyscall_socket(int domain, int type, int protocol);
int zsyscall_accept(int fd, struct sockaddr *addr, socklen_t *len);
int zsyscall_connect(int socket, const struct sockaddr *address, socklen_t address_len);
int zsyscall_close(int fd);
ssize_t zsyscall_read(int fildes, void *buf, size_t nbyte);
ssize_t zsyscall_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t zsyscall_write(int fildes, const void *buf, size_t nbyte);
ssize_t zsyscall_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t zsyscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t zsyscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
size_t zsyscall_send(int socket, const void *buffer, size_t length, int flags);
ssize_t zsyscall_recv(int socket, void *buffer, size_t length, int flags);
int zsyscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);
int zsyscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int zsyscall_fcntl(int fildes, int cmd, ...);
pid_t zsyscall_gettid(void);
int zsyscall_open(const char *pathname, int flags, ...);
int zsyscall_openat(int dirfd, const char *pathname, int flags, ...);
int zsyscall_creat(const char *pathname, mode_t mode);
off_t zsyscall_lseek(int fd, off_t offset, int whence);
int zsyscall_fdatasync(int fd);
int zsyscall_fsync(int fd);
int zsyscall_rename(const char *oldpath, const char *newpath);
int zsyscall_truncate(const char *path, off_t length);
int zsyscall_ftruncate(int fd, off_t length);
int zsyscall_rmdir(const char *pathname);
int zsyscall_mkdir(const char *pathname, mode_t mode);
int zsyscall_getdents(unsigned int fd, void *dirp, unsigned int count);
int zsyscall_stat(const char *pathname, struct stat *buf);
int zsyscall_fstat(int fd, struct stat *buf);
int zsyscall_lstat(const char *pathname, struct stat *buf);
int zsyscall_link(const char *oldpath, const char *newpath);
int zsyscall_symlink(const char *target, const char *linkpath);
ssize_t zsyscall_readlink(const char *pathname, char *buf, size_t bufsiz);
int zsyscall_unlink(const char *pathname);
int zsyscall_chmod(const char *pathname, mode_t mode);
int zsyscall_fchmod(int fd, mode_t mode);
int zsyscall_chown(const char *pathname, uid_t owner, gid_t group);
int zsyscall_fchown(int fd, uid_t owner, gid_t group);
int zsyscall_lchown(const char *pathname, uid_t owner, gid_t group);
int zsyscall_utime(const char *filename, const struct utimbuf *times);
int zsyscall_utimes(const char *filename, const struct timeval times[2]);
int zsyscall_futimes(int fd, const struct timeval tv[2]);
int zsyscall_lutimes(const char *filename, const struct timeval tv[2]);
static int zrobust_syscall_close(int fd) {
    int ret;
    do {
        ret = zsyscall_close(fd);
    } while((ret<0) && (errno==EINTR));
    return ret;
}

/* }}} */
extern pthread_mutex_t *zvar_general_pthread_mutex;

int zvar_coroutine_block_pthread_count_limit = 0;
zbool_t zvar_coroutine_fileio_use_block_pthread = 0;

static int zvar_coroutine_block_pthread_count_current = 0;

static void *zcoroutine_hook_fileio_worker(void *arg);

int zsyscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);

/* ################################################################################# */
typedef struct zcoroutine_sys_context zcoroutine_sys_context;
typedef struct zcoroutine_base_t zcoroutine_base_t;
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
    zcoroutine_hook_arg_t args[4];
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
    zcoroutine_hook_fileio_truncate,
    zcoroutine_hook_fileio_ftruncate,
    zcoroutine_hook_fileio_rmdir,
    zcoroutine_hook_fileio_mkdir,
    zcoroutine_hook_fileio_getdents,
    zcoroutine_hook_fileio_stat,
    zcoroutine_hook_fileio_fstat,
    zcoroutine_hook_fileio_lstat,
    zcoroutine_hook_fileio_link,
    zcoroutine_hook_fileio_symlink,
    zcoroutine_hook_fileio_readlink,
    zcoroutine_hook_fileio_unlink,
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
    if ((cobs = zcoroutine_base_get_current())==0)

#define zcoroutine_hook_fileio_run_part1() \
    zcoroutine_base_t *cobs = 0; \
    if ((!zvar_coroutine_fileio_use_block_pthread) || (zvar_coroutine_block_pthread_count_limit<1) || ((cobs = zcoroutine_base_get_current())==0))

#define zcoroutine_hook_fileio_run_part2(func)  \
    zcoroutine_t *current_coroutine = cobs->current_coroutine; \
    zcoroutine_hook_fileio_t fileio;\
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
                zfatal("pthread_create error(%m)"); \
            } \
            zvar_coroutine_block_pthread_count_current++; \
        } \
    } \
    pthread_cond_signal(&zvar_coroutine_hook_fileio_cond); \
    zpthread_unlock(&zvar_coroutine_hook_fileio_lock); \
    current_coroutine->inner_yield = 1; \
    zcoroutine_yield_my(current_coroutine); \
    errno = fileio.co_errno; \
    zcoroutine_hook_arg_t retval; \
    retval.long_t = fileio.retval.long_t; \

/* ######################################## */
static void zcoroutine_yield_my(zcoroutine_t *co);

static int zvar_coroutine_mode_flag = 0;

struct zcoroutine_sys_context {
	void *regs[ 14 ];
	size_t ss_size;
	char *ss_sp;
};
struct  zcoroutine_t {
    void *(*start_job)(void *ctx);
    void *context;
    long sleep_timeout;
    zrbtree_node_t sleep_rbnode;
    /* system */
    zcoroutine_sys_context sys_context;
    zcoroutine_t *prev;
    zcoroutine_t *next;
    zcoroutine_base_t *base; /* FIXME, should be removed */
    zlist_t *mutex_list; /* zcoroutine_mutex_t* */
    struct __res_state *res_state;
    zgethostbyname_buf_t *gethostbyname;
    /* flags */
    unsigned char ended:1;
    unsigned char ep_loop:1;
    unsigned char active_list:1;
    unsigned char inner_yield:1;
};

#define zvar_epoll_event_size 1024
struct zcoroutine_base_t {
    int epoll_fd;
    int event_fd;
    struct epoll_event epoll_event_vec[zvar_epoll_event_size];
    zrbtree_t sleep_zrbtree;
    zrbtree_t fd_timeout_zrbtree;
    zcoroutine_t *self_coroutine;
    zcoroutine_t *current_coroutine;
    zcoroutine_t *fileio_coroutines_head;
    zcoroutine_t *fileio_coroutines_tail;
    zcoroutine_t *active_coroutines_head;
    zcoroutine_t *active_coroutines_tail;
    zcoroutine_t *prepare_coroutines_head;
    zcoroutine_t *prepare_coroutines_tail;
    zcoroutine_t *deleted_coroutine_head;
    zcoroutine_t *deleted_coroutine_tail;
    unsigned char ___break:1;
};

typedef struct zcoroutine_fd_attribute zcoroutine_fd_attribute;
struct zcoroutine_fd_attribute {
    unsigned short int nonblock:1;
    unsigned short int pseudo_mode:1;
    unsigned short int in_epoll:1;
    unsigned short int by_epoll:1;
    unsigned short int is_regular_file:1;
    unsigned short int read_timeout;
    unsigned short int write_timeout;
    unsigned int revents;
    long timeout;
    zrbtree_node_t rbnode;
    zcoroutine_t *co;
};

struct  zcoroutine_mutex_t {
    zlist_t *colist; /* zcoroutine_t* */
};

struct zcoroutine_cond_t {
    zlist_t *colist; /* zcoroutine_t* */
};

static void zcoroutine_base_remove_coroutine(zcoroutine_base_t *cobs);
static zcoroutine_base_t *zcoroutine_base_create();

/* ################################################################################# */

static __thread zcoroutine_base_t *zvar_coroutine_base_per_pthread = 0;
static zcoroutine_fd_attribute **zcoroutine_fd_attribute_vec = 0;

static inline zcoroutine_base_t * zcoroutine_base_get_current()
{
    return (zvar_coroutine_mode_flag?zvar_coroutine_base_per_pthread:0);
}

/* {{{ zcoroutine_sys_context */
static int zcoroutine_start_wrap(zcoroutine_t *co, void *unused);
static void zcoroutine_sys_context_init(zcoroutine_sys_context *ctx, const void *s)
{
	char *sp = ctx->ss_sp + ctx->ss_size;
	sp = (char*) ((unsigned long)sp & -16LL  );
	memset(ctx->regs, 0, sizeof(ctx->regs));
	ctx->regs[13] = sp - 8;
	ctx->regs[9] = (char*)zcoroutine_start_wrap;
	ctx->regs[7] = (char *)s;
	ctx->regs[8] = 0;
}

static void zcoroutine_sys_context_fini(zcoroutine_sys_context *ctx)
{
}

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

/* }}} */

/* FIXME 是否需要加锁? 不需要! */
/* {{{ zcoroutine_fd_attribute */
static inline zcoroutine_fd_attribute * zcoroutine_fd_attribute_get(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_max_fd) && (zvar_coroutine_mode_flag)) {
        return zcoroutine_fd_attribute_vec[fd];
    }
    return 0;
}

static zcoroutine_fd_attribute *zcoroutine_fd_attribute_create(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_max_fd) && (zvar_coroutine_mode_flag)) {
        zfree(zcoroutine_fd_attribute_vec[fd]);
        zcoroutine_fd_attribute *cfa = (zcoroutine_fd_attribute *)zcalloc(1, sizeof(zcoroutine_fd_attribute));
        zcoroutine_fd_attribute_vec[fd] = cfa;
        cfa->read_timeout = 10 * 1000  + (fd%1000);
        cfa->write_timeout = 10 * 1000 + (fd%1000);
        return cfa;
    }
    return 0;
}

static inline void zcoroutine_fd_attribute_free(int fd)
{
    if ((fd > -1) &&  (fd <= zvar_max_fd) && (zvar_coroutine_mode_flag)) {
        zfree(zcoroutine_fd_attribute_vec[fd]);
        zcoroutine_fd_attribute_vec[fd] = 0;
    }
}
/* }}} */

/* {{{ zcoroutine_t */
zcoroutine_t *zcoroutine_create(zcoroutine_base_t *base, int stack_size)
{
    if (stack_size < 1) {
        stack_size = 128 * 1024;
    }
    zcoroutine_t *co = (zcoroutine_t *)zcalloc(1, sizeof (zcoroutine_t));
    co->base = base;
    co->sys_context.ss_sp = (char *)zmalloc(stack_size + 16 + 10);
    co->sys_context.ss_size = stack_size;
    zcoroutine_sys_context_init(&(co->sys_context), co);
    return co;
}

void zcoroutine_free(zcoroutine_t *co)
{
    void *ptr = co->sys_context.ss_sp;
    zfree(co->res_state);
    zfree(co->gethostbyname);
    zcoroutine_sys_context_fini(&(co->sys_context));
    zfree(ptr);
    zfree(co);
}

static void zcoroutine_append_mutex(zcoroutine_t *co, zcoroutine_mutex_t *mutex)
{
    if (co->mutex_list == 0) {
        co->mutex_list = zlist_create();
        zlist_push(co->mutex_list, mutex);
        return;
    }

    ZLIST_WALK_BEGIN(co->mutex_list, zcoroutine_mutex_t *, m) {
        if (m == mutex) {
            return;
        }
    } ZLIST_WALK_END;
    zlist_push(co->mutex_list, mutex);
}

static void zcoroutine_remove_mutex(zcoroutine_t *co, zcoroutine_mutex_t *mutex)
{
    if (!co->mutex_list) {
        return;
    }
    zlist_node_t *del = 0;
    ZLIST_WALK_BEGIN(co->mutex_list, zcoroutine_mutex_t *, m) {
        if (m == mutex) {
            del = list_current_node;
            break;
        }
    } ZLIST_WALK_END;

    if (del) {
        zlist_delete(co->mutex_list, del, 0);
    }
    if (!zlist_len(co->mutex_list)) {
        zlist_free(co->mutex_list);
        co->mutex_list = 0;
    }
}

static void zcoroutine_release_all_mutex(zcoroutine_t *co)
{
    if (!co->mutex_list) {
        return;
    }
    zcoroutine_mutex_t *mutex;
    while(co->mutex_list && zlist_pop(co->mutex_list, (void **)&mutex) ) {
        zcoroutine_mutex_unlock(mutex);
    }
    zlist_free(co->mutex_list);
    co->mutex_list = 0;
}

static int zcoroutine_start_wrap(zcoroutine_t *co, void *unused)
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
void zcoroutine_base_init()
{
    pthread_mutex_lock(zvar_general_pthread_mutex);
    zvar_coroutine_mode_flag = 1;
    if (zcoroutine_fd_attribute_vec == 0) {
        zcoroutine_fd_attribute_vec = (zcoroutine_fd_attribute **)zcalloc(zvar_max_fd + 1, sizeof(void *));
    }
    if (zvar_coroutine_base_per_pthread == 0) {
        zcoroutine_base_create();
        base_count++;
    }
    pthread_mutex_unlock(zvar_general_pthread_mutex);
}

void zcoroutine_base_fini()
{
    if (zcoroutine_fd_attribute_vec == 0) {
        return;
    }
    pthread_mutex_lock(zvar_general_pthread_mutex);
    zcoroutine_base_t *cobs = zvar_coroutine_base_per_pthread;
    if (cobs) {
        if (cobs->deleted_coroutine_head) {
            zcoroutine_base_remove_coroutine(cobs);
        }
        zcoroutine_free(cobs->self_coroutine);
        zrobust_syscall_close(cobs->epoll_fd);
        zrobust_syscall_close(cobs->event_fd);
        zfree(cobs);
        zvar_coroutine_base_per_pthread = 0;
        base_count--;
        if (base_count == 0) {
            zfree(zcoroutine_fd_attribute_vec);
            zcoroutine_fd_attribute_vec = 0;
            zvar_coroutine_mode_flag = 0;
        }
    }
    pthread_mutex_unlock(zvar_general_pthread_mutex);
}

void zcoroutine_go(void *(*start_job)(void *ctx), void *ctx, int stack_size)
{
    if (start_job == 0) {
        return;
    }
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if (!cobs) {
        zfatal("excute zcoroutine_enable() when the pthread begin");
    }
    if (cobs->deleted_coroutine_head) {
        zcoroutine_base_remove_coroutine(cobs);
    }
    zcoroutine_t *co = zcoroutine_create(cobs, stack_size);
    ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    co->start_job = start_job;
    co->context = ctx;
}

zcoroutine_t * zcoroutine_self()
{
    zcoroutine_base_t *cobs =  zcoroutine_base_get_current();
    if (cobs == 0) {
        return 0;
    }
    return cobs->current_coroutine;
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
    zcoroutine_fd_attribute_create(fd);
}

void zcoroutine_disable_fd(int fd)
{
    zcoroutine_fd_attribute  *cfa = zcoroutine_fd_attribute_get(fd);
    if (cfa) {
        int flags;
        if ((flags = zsyscall_fcntl(fd, F_GETFL, 0)) < 0) {
            zfatal("fcntl _co(%m)");
        }
        if (zsyscall_fcntl(fd, F_SETFL, (cfa->nonblock?flags | O_NONBLOCK : flags & ~O_NONBLOCK)) < 0) {
            zfatal("fcntl _co(%m)");
        }
        zcoroutine_fd_attribute_free(fd);
    }
}
/* }}} */

/* {{{ mutex cond */
zcoroutine_mutex_t * zcoroutine_mutex_create()
{
    zcoroutine_mutex_t *m = (zcoroutine_mutex_t *)zcalloc(1, sizeof(zcoroutine_mutex_t));
    m->colist = zlist_create();
    return m;
}

void zcoroutine_mutex_free(zcoroutine_mutex_t *m)
{
    if (m) {
        zlist_free(m->colist);
        zfree(m);
    }
}

void zcoroutine_mutex_lock(zcoroutine_mutex_t *m)
{
    if (m == 0) {
        zfatal("not in zcoroutine_t");
    }
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        zfatal("not in zcoroutine_t");
    }
    zlist_t *colist = m->colist;
    if (!(zlist_len(colist))) {
        zlist_push(colist, co);
        zcoroutine_append_mutex(co, m);
        return;
    }
    if ((zcoroutine_t *)zlist_node_value(zlist_head(colist)) == co) {
        zcoroutine_append_mutex(co, m);
        return;
    }
    zlist_push(colist, co);
    co->inner_yield = 1;
    zcoroutine_yield_my(co);
}

void zcoroutine_mutex_unlock(zcoroutine_mutex_t *m)
{
    /* FIXME */
    if (m == 0) {
        zfatal("mutex is null");
        return;
    }
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        return;
    }
    zlist_t *colist = m->colist;
    if (!(zlist_len(colist))) {
        return;
    }
    if ((zcoroutine_t *)zlist_node_value(zlist_head(colist)) != co) {
        return;
    }
    zcoroutine_remove_mutex(co, m);
    zlist_shift(colist, 0);
    if (!zlist_len(colist)) {
        return;
    }
    co = (zcoroutine_t *)zlist_node_value(zlist_head(colist));
    ZMLINK_APPEND(co->base->prepare_coroutines_head, co->base->prepare_coroutines_tail, co, prev, next);
    return;
}

zcoroutine_cond_t *zcoroutine_cond_create()
{
    zcoroutine_cond_t *c= (zcoroutine_cond_t *)zcalloc(1, sizeof(zcoroutine_cond_t));
    c->colist = zlist_create();
    return c;
}

void zcoroutine_cond_free(zcoroutine_cond_t * c)
{
    if (c) {
        zlist_free(c->colist);
        zfree(c);
    }
}

void zcoroutine_cond_wait(zcoroutine_cond_t *cond, zcoroutine_mutex_t * mutex)
{
    zcoroutine_t * co = zcoroutine_self();
    if (co == 0) {
        zfatal("not in zcoroutine_t");
    }
    if (!cond) {
        zfatal("cond is null");
    }
    if (!cond) {
        zfatal("mutex is null");
    }

    zcoroutine_mutex_unlock(mutex);

    zlist_push(cond->colist, co);
    co->inner_yield = 1;
    zcoroutine_yield_my(co);

    zcoroutine_mutex_lock(mutex);
}

void zcoroutine_cond_signal(zcoroutine_cond_t * cond)
{
    if (!cond) {
        zfatal("cond is null");
    }
    zcoroutine_base_t *cobs;
    if ((cobs = zcoroutine_base_get_current()) == 0) {
        zfatal("not in zcoroutine_t");
    }

    zcoroutine_t *co;
    if (!zlist_shift(cond->colist, (void **)&co)) {
        return;
    }
    ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    zcoroutine_yield_my(cobs->current_coroutine);
}

void zcoroutine_cond_broadcast(zcoroutine_cond_t *cond)
{
    if (!cond) {
        zfatal("cond is null");
    }
    zcoroutine_base_t *cobs;
    if ((cobs = zcoroutine_base_get_current()) == 0) {
        zfatal("not in zcoroutine_t");
    }
    zcoroutine_t *co;
    while(zlist_shift(cond->colist, (void **)&co)) {
        ZMLINK_APPEND(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, prev, next);
    }
    zcoroutine_yield_my(cobs->current_coroutine);
}

/* }}} */

/* {{{ sleep_zrbtree */
static int zcoroutine_base_sleep_tree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
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
/* }}} */

/* {{{ fd_timeout_zrbtree */
static int zcoroutine_base_fd_timeout_tree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zcoroutine_fd_attribute *c1, *c2;
    long r;
    c1 = ZCONTAINER_OF(n1, zcoroutine_fd_attribute, rbnode);
    c2 = ZCONTAINER_OF(n2, zcoroutine_fd_attribute, rbnode);
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

/* {{{ zcoroutine_base_t */
static zcoroutine_base_t *zcoroutine_base_create()
{
    zcoroutine_base_t *cobs;
    cobs = (zcoroutine_base_t *)zcalloc(1, sizeof(zcoroutine_base_t));
    zvar_coroutine_base_per_pthread = cobs;
    cobs->self_coroutine = zcoroutine_create(cobs, 0);
    cobs->epoll_fd = epoll_create(1024);
    zclose_on_exec(cobs->epoll_fd, 1);

    cobs->event_fd = eventfd(0, 0);
    zclose_on_exec(cobs->event_fd, 1);
    znonblocking(cobs->event_fd, 1);
    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = cobs->event_fd;
    int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_ADD, cobs->event_fd, &epev);
    if (eret < 0) {
        zfatal("epoll_ctl ADD event_fd:%d (%m)", cobs->event_fd);
    }

    zrbtree_init(&(cobs->sleep_zrbtree), zcoroutine_base_sleep_tree_cmp);
    zrbtree_init(&(cobs->fd_timeout_zrbtree), zcoroutine_base_fd_timeout_tree_cmp);
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

void zcoroutine_base_stop_notify()
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if (cobs) {
        cobs->___break = 1;
    }
}

/* }}} */

/* {{{ zcoroutine_base_run */
void zcoroutine_base_run()
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if (!cobs) {
        zfatal("excute zcoroutine_enable() when the pthread begin");
    }
    zcoroutine_t *co;
    zrbtree_node_t *rn;
    zcoroutine_fd_attribute *cfa;
    long delay, tmp_delay, tmp_ms;

    while(1) {
        if(zvar_proc_stop) {
            return;
        }
        if (cobs->deleted_coroutine_head) {
            zcoroutine_base_remove_coroutine(cobs);
        }

        delay = 1000;
        tmp_ms = ztimeout_set_millisecond(0) - 1;

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
        if (zrbtree_have_data(&(cobs->sleep_zrbtree))) {
            if(zvar_proc_stop) {
                return;
            }
            rn = zrbtree_first(&(cobs->sleep_zrbtree));
            co = ZCONTAINER_OF(rn, zcoroutine_t, sleep_rbnode);
            tmp_delay = co->sleep_timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = zrbtree_next(rn)) {
                co = ZCONTAINER_OF(rn, zcoroutine_t, sleep_rbnode);
                if (tmp_ms < co->sleep_timeout) {
                    break;
                }
                ZMLINK_APPEND(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, prev, next);
            }
        }

        /* fd timeout */
        if (zrbtree_have_data(&(cobs->fd_timeout_zrbtree))) {
            rn = zrbtree_first(&(cobs->fd_timeout_zrbtree));
            cfa = ZCONTAINER_OF(rn, zcoroutine_fd_attribute, rbnode);
            tmp_delay = cfa->timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = zrbtree_next(rn)) {
                if(zvar_proc_stop) {
                    return;
                }
                cfa = ZCONTAINER_OF(rn, zcoroutine_fd_attribute, rbnode);
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
        int nfds = epoll_wait(cobs->epoll_fd, cobs->epoll_event_vec, zvar_epoll_event_size, (int)delay);
        if ((nfds == -1) && (errno != EINTR)) {
            zfatal("epoll_wait: %m");
        }
        for (int i = 0; i < nfds; i++) {
            if(zvar_proc_stop) {
                return;
            }
            struct epoll_event *epev = cobs->epoll_event_vec + i;
            int fd = epev->data.fd;
            if (fd == cobs->event_fd) {
                uint64_t u;
                zsyscall_read(fd, &u, sizeof(uint64_t));
                continue;
            }
            cfa = zcoroutine_fd_attribute_get(fd);
            if (!cfa) {
                zfatal("fd:%d be closed unexpectedly", fd);
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

        /* */
        if (cobs->active_coroutines_head == 0) {
            continue;
        }
        zcoroutine_yield_my(cobs->self_coroutine);
        if (cobs->___break) {
            break;
        }
    }
    if (cobs->deleted_coroutine_head) {
        zcoroutine_base_remove_coroutine(cobs);
    }
}

/* }}} */

/* {{{ zcoroutine_poll */
static int zcoroutine_poll(zcoroutine_t *co, struct pollfd fds[], nfds_t nfds, int timeout)
{
    co->active_list = 0;
    co->ep_loop = 0;

    zcoroutine_base_t *cobs = co->base;
    long now_ms = ztimeout_set_millisecond(0);
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

        zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(last_fd);
        if (cfa == 0) {
            return zsyscall_poll(fds, nfds, timeout);
        }

    } while(0);

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        int fd  = fds[i].fd;
        if (fd < 0) {
            continue;
        }
        zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(fd);
        if (cfa == 0) {
            cfa = zcoroutine_fd_attribute_create(fd);
            cfa->pseudo_mode = 1;
        }
        if (cfa->in_epoll) {
            zfatal("mutli monitor fd:%d", fd);
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
            return zsyscall_poll(fds, nfds, timeout);
        }
        is_epoll_ctl = 1;
        cfa->by_epoll = 0;
        cfa->co = co;
        cfa->timeout = now_ms + timeout;
        zrbtree_attach(&(cobs->fd_timeout_zrbtree), &(cfa->rbnode));
    }

    if (is_epoll_ctl == 0) {
        if (timeout < 1) {
            return 0;
        }
        co->sleep_timeout = now_ms + timeout;
        zrbtree_attach(&(cobs->sleep_zrbtree), &(co->sleep_rbnode));
        co->inner_yield = 1;
        zcoroutine_yield_my(co);
        zrbtree_detach(&(cobs->sleep_zrbtree), &(co->sleep_rbnode));
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
        zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(fd);
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
                zfatal("epoll_ctl del fd:%d (%m)", fd);
            }
        }
        zrbtree_detach(&(cobs->fd_timeout_zrbtree), &(cfa->rbnode));
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
    }
    switch(cio->cmdcode) {
    case zcoroutine_hook_fileio_open:
        retval->int_t = zsyscall_open(args[0].char_ptr_t, args[1].int_t, args[2].int_t);
        if (retval->int_t > -1) {
            struct stat st;
            if (zsyscall_fstat(retval->int_t, &st) == -1) {
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
        retval->int_t = zsyscall_openat(args[0].int_t, args[1].char_ptr_t, args[2].int_t, args[3].int_t);
        if (retval->int_t > -1) {
            struct stat st;
            if (zsyscall_fstat(retval->int_t, &st) == -1) {
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
        retval->ssize_ssize_t = zsyscall_read(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
        break;
    case zcoroutine_hook_fileio_readv:
        retval->ssize_ssize_t = zsyscall_readv(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
        break;
    case zcoroutine_hook_fileio_write:
        retval->ssize_ssize_t = zsyscall_write(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
        break;
    case zcoroutine_hook_fileio_writev:
        retval->ssize_ssize_t = zsyscall_writev(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
        break;
    case zcoroutine_hook_fileio_lseek:
        retval->off_off_t = zsyscall_lseek(args[0].int_t, args[1].off_off_t, args[2].int_t);
        break;
    case zcoroutine_hook_fileio_fdatasync:
        retval->int_t = zsyscall_fdatasync(args[0].int_t);
        break;
    case zcoroutine_hook_fileio_fsync:
        retval->int_t = zsyscall_fsync(args[0].int_t);
        break;
    case zcoroutine_hook_fileio_rename:
        retval->int_t = zsyscall_rename(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case zcoroutine_hook_fileio_truncate:
        retval->int_t = zsyscall_truncate(args[0].const_char_ptr_t, args[1].off_off_t);
        break;
    case zcoroutine_hook_fileio_ftruncate:
        retval->int_t = zsyscall_ftruncate(args[0].int_t, args[1].off_off_t);
        break;
    case zcoroutine_hook_fileio_rmdir:
        retval->int_t = zsyscall_rmdir(args[0].const_char_ptr_t);
        break;
    case zcoroutine_hook_fileio_mkdir:
        retval->int_t = zsyscall_mkdir(args[0].const_char_ptr_t, args[1].mode_mode_t);
        break;
    case zcoroutine_hook_fileio_getdents:
        retval->int_t = zsyscall_getdents(args[0].uint_t, args[1].void_ptr_t, args[1].uint_t);
        break;
    case zcoroutine_hook_fileio_stat:
        retval->int_t = zsyscall_stat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
        break;
    case zcoroutine_hook_fileio_fstat:
        retval->int_t = zsyscall_fstat(args[0].int_t, (struct stat *)args[0].char_ptr_t);
        break;
    case zcoroutine_hook_fileio_lstat:
        retval->int_t = zsyscall_lstat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
        break;
    case zcoroutine_hook_fileio_link:
        retval->int_t = zsyscall_link(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case zcoroutine_hook_fileio_symlink:
        retval->int_t = zsyscall_symlink(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case zcoroutine_hook_fileio_readlink:
        retval->int_t = zsyscall_readlink(args[0].const_char_ptr_t, args[1].char_ptr_t, args[2].size_size_t);
        break;
    case zcoroutine_hook_fileio_unlink:
        retval->int_t = zsyscall_unlink(args[0].const_char_ptr_t);
        break;
    case zcoroutine_hook_fileio_chmod:
        retval->int_t = zsyscall_chmod(args[0].const_char_ptr_t, args[1].mode_mode_t);
        break;
    case zcoroutine_hook_fileio_fchmod:
        retval->int_t = zsyscall_fchmod(args[0].int_t, args[1].mode_mode_t);
        break;
    case zcoroutine_hook_fileio_chown:
        retval->int_t = zsyscall_chown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case zcoroutine_hook_fileio_fchown:
        retval->int_t = zsyscall_fchown(args[0].int_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case zcoroutine_hook_fileio_lchown:
        retval->int_t = zsyscall_lchown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case zcoroutine_hook_fileio_utime:
        retval->int_t = zsyscall_utime(args[0].const_char_ptr_t, (const struct utimbuf *)args[1].char_ptr_t);
        break;
    case zcoroutine_hook_fileio_utimes:
        retval->int_t = zsyscall_utimes(args[0].const_char_ptr_t, (struct timeval *)args[1].char_ptr_t);
        break;
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
            zfree(*g);
        }
        zfree(g);
    }
    pthread_setspecific(gethostbyname_pthread_key, 0);
}

static void zcoroutine_hook_fileio_worker_init()
{
    pthread_detach(pthread_self());
    pthread_key_create(&gethostbyname_pthread_key, gethostbyname_pthread_key_destroy);
    char **g = (char **)zmalloc(sizeof(char **));
    *g = 0;
    pthread_setspecific(gethostbyname_pthread_key, g);
}

static void *zcoroutine_hook_fileio_worker(void *arg)
{
    zcoroutine_hook_fileio_worker_init();
    while (1) {
        if (zvar_proc_stop) {
            return arg;
        }
        zpthread_lock(&zvar_coroutine_hook_fileio_lock);
        while(!zvar_coroutine_hook_fileio_head) {
            if (zvar_proc_stop) {
                zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
                return arg;
            }
            struct timespec ts;
            ts.tv_sec = time(0) + 1;
            ts.tv_nsec = 0;
            pthread_cond_timedwait(&zvar_coroutine_hook_fileio_cond, &zvar_coroutine_hook_fileio_lock, &ts);
        }
        zcoroutine_hook_fileio_t *cio = zvar_coroutine_hook_fileio_head;
        ZMLINK_DETACH(zvar_coroutine_hook_fileio_head, zvar_coroutine_hook_fileio_tail, cio, prev, next);
        zvar_coroutine_hook_fileio_count--;
        zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
        if (zvar_proc_stop) {
            return arg;
        }
        zcoroutine_hook_fileio_worker_do(cio);
        if (zvar_proc_stop) {
            return arg;
        }

        zpthread_lock(&zvar_coroutine_hook_fileio_lock);
        zcoroutine_t  *co = cio->current_coroutine;
        zcoroutine_base_t *cobs = co->base;
        ZMLINK_APPEND(cobs->fileio_coroutines_head, cobs->fileio_coroutines_tail, co, prev, next);
        uint64_t u = 1;
        zsyscall_write(cobs->event_fd, &u, sizeof(uint64_t));
        zpthread_unlock(&zvar_coroutine_hook_fileio_lock);
    }
    return arg;
}
/* }}} */

/* {{{ block*/
void *zcoroutine_block_do(void *(*block_func)(void *ctx), void *ctx)
{
    zcoroutine_base_t *cobs = 0;
    if ((zvar_coroutine_block_pthread_count_limit<1) || ((cobs = zcoroutine_base_get_current())==0)) {
        return block_func(ctx);
    }
    zcoroutine_hook_fileio_run_part2(unknown);
    fileio.args[0].block_func = block_func;
    fileio.args[1].void_ptr_t = ctx;
    fileio.is_block_func = 1;
    zcoroutine_hook_fileio_run_part3();
    return retval.void_ptr_t;
}
/* }}} */

/* ############## SYS CALL  HOOK ############################## */
/* {{{ general read/write wait */

static int general_read_wait(int fd)
{
    zcoroutine_fd_attribute *cfa =  zcoroutine_fd_attribute_get(fd);
	struct pollfd pf;
	pf.fd = fd;
	pf.events = (POLLIN | POLLERR | POLLHUP);
    pf.revents = 0;
	poll(&pf, 1, cfa->read_timeout);
    return pf.revents;
}

static int general_write_wait(int fd)
{
    zcoroutine_fd_attribute *cfa =  zcoroutine_fd_attribute_get(fd);
	struct pollfd pf;
	pf.fd = fd;
	pf.events = (POLLOUT | POLLERR | POLLHUP);
    pf.revents = 0;
	poll(&pf, 1, cfa->write_timeout);
    return pf.revents;
}

/* }}} */

/* {{{ sleep */
unsigned int sleep(unsigned int seconds)
{
    zsleep(seconds);
    return 0;
}
/* }}} */

/* {{{ poll hook */
int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    if (timeout < 1) {
        return zsyscall_poll(fds, nfds, 0);
    }
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if ((cobs == 0) || (cobs->current_coroutine == 0)) {
        return zsyscall_poll(fds, nfds, timeout);
    }
    return  zcoroutine_poll(cobs->current_coroutine, fds, nfds, timeout);
}

int __poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return poll(fds, nfds, timeout);
}

/* }}} */

/* {{{ pipe hook */
int pipe(int pipefd[2])
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    int ret = zsyscall_pipe(pipefd);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    zcoroutine_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    zcoroutine_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ pipe2 hook */
int pipe2(int pipefd[2], int flags)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    int ret = zsyscall_pipe2(pipefd, flags);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    zcoroutine_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    zcoroutine_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ dup hook */
int dup(int oldfd)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    int newfd = zsyscall_dup(oldfd);
    if (newfd < 0) {
        return newfd;
    }
    if (!cobs) {
        return newfd;
    }
    zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(oldfd);
    zcoroutine_fd_attribute *new_cfa = zcoroutine_fd_attribute_create(newfd);
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
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if ((ret = zsyscall_dup2(oldfd, newfd)) < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(oldfd);
    if (cfa && (cfa->pseudo_mode == 0)) {
        cfa = zcoroutine_fd_attribute_get(newfd);
        if (cfa) {
            zfatal("the newfd be used by other zcoroutine_t");
#if 0
            /* note: the newfd be used by other zcoroutine_t. */
            zcoroutine_fd_attribute_free(newfd);
            zcoroutine_fd_attribute_create(newfd);
#endif
        } else {
            zcoroutine_fd_attribute *new_cfa = zcoroutine_fd_attribute_create(newfd);
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
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    int ret = zsyscall_socketpair(domain, type, protocol, sv);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    zcoroutine_fd_attribute_create(sv[0]);
    fcntl(sv[0], F_SETFL, fcntl(sv[0], F_GETFL, 0));
    zcoroutine_fd_attribute_create(sv[1]);
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
        return zsyscall_open(pathname, flags, mode);
    }
    zcoroutine_hook_fileio_run_part2(open);
    fileio.args[0].void_ptr_t = (void *)pathname;
    fileio.args[1].int_t = flags;
    fileio.args[2].int_t = mode;
    zcoroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        zcoroutine_fd_attribute *fdatts = zcoroutine_fd_attribute_create(retfd);
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
        return zsyscall_openat(dirid, pathname, flags, mode);
    }
    zcoroutine_hook_fileio_run_part2(open);
    fileio.args[0].int_t = dirid;
    fileio.args[1].void_ptr_t = (void *)pathname;
    fileio.args[2].int_t = flags;
    fileio.args[3].int_t = mode;
    zcoroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        zcoroutine_fd_attribute *fdatts = zcoroutine_fd_attribute_create(retfd);
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
    return open (pathname, O_WRONLY|O_CREAT|O_TRUNC, mode);
}
/* }}} */

/* {{{ socket hook */
int socket(int domain, int type, int protocol)
{
	int fd = zsyscall_socket(domain, type, protocol);
	if(fd < 0) {
		return fd;
	}

    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if (cobs == 0) {
        return fd;
    }

    zcoroutine_fd_attribute_create(fd);
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0));

	return fd;
}
/* }}} */

/* {{{ return_zcc_call */
#define return_zcc_call_co(fd)  \
    zcoroutine_base_t *cobs = 0; \
    zcoroutine_fd_attribute *fdatts = 0; \
    if (((cobs = zcoroutine_base_get_current()) ==0) \
            || ((fdatts = zcoroutine_fd_attribute_get(fd)) == 0)  \
            || (fdatts->nonblock == 1) \
            || (fdatts->pseudo_mode == 1))

/* }}} */

/* {{{ accept hook */
int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    const int ___accept_timeout = 100 * 1000;
    return_zcc_call_co(fd) {
        int sock = zsyscall_accept(fd, addr, len);
        if (cobs && (sock > -1)) {
            zcoroutine_fd_attribute_create(sock);
            fcntl(sock, F_SETFL, zsyscall_fcntl(sock, F_GETFL,0));
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

    int sock = zsyscall_accept(fd, addr, len);
    if (sock > -1) {
        zcoroutine_fd_attribute_create(sock);
        fcntl(sock, F_SETFL, zsyscall_fcntl(sock, F_GETFL,0));
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
    int ret = zsyscall_connect(fd, address, address_len);
    return_zcc_call_co(fd) {
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
            zcoroutine_fd_attribute_free(fd);
        }
        return ret;
    }

    zcoroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcoroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        ret = zrobust_syscall_close(fd);
        if (ret > -1) {
            zcoroutine_fd_attribute_free(fd);
        }
        return ret;
    }
    if (fdatts->is_regular_file) {
        zcoroutine_hook_fileio_run_part2(close);
        fileio.args[0].int_t = fd;
        zcoroutine_hook_fileio_run_part3();
        ret = retval.int_t;
        if (ret > -1) {
            zcoroutine_fd_attribute_free(fd);
        }
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return ret;
    }
    ret = zrobust_syscall_close(fd);
    if (ret > -1) {
        zcoroutine_fd_attribute_free(fd);
    }
    return ret;
}
/* }}} */

/* {{{ read hook */
ssize_t read(int fd, void *buf, size_t nbyte)
{
    zcoroutine_hook_fileio_run_part0() {
        return zsyscall_read(fd, buf, nbyte);
    }

    zcoroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcoroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zsyscall_read(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return zsyscall_read(fd, buf, nbyte);
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
        return zsyscall_read(fd, buf, nbyte);
    }
#if 0
    general_read_wait(fd);
	ssize_t readret = zsyscall_read(fd,(char*)buf ,nbyte);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return readret;
#else 
    while(1) {
        general_read_wait(fd);
        ssize_t readret = zsyscall_read(fd,(char*)buf ,nbyte);
        int ec = errno;
        if ((readret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
        return zsyscall_readv(fd, iov, iovcnt);
    }

    zcoroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcoroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zsyscall_readv(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return zsyscall_readv(fd, iov, iovcnt);
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
        return zsyscall_readv(fd, iov, iovcnt);
    }
#if 0
    general_read_wait(fd);
	ssize_t readret = zsyscall_readv(fd, iov, iovcnt);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return readret;
#else
    while(1) {
        general_read_wait(fd);
        ssize_t readret = zsyscall_readv(fd, iov, iovcnt);
        int ec = errno;
        if ((readret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
        return zsyscall_write(fd, buf, nbyte);
    }

    zcoroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcoroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zsyscall_write(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return zsyscall_write(fd, buf, nbyte);
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
        return zsyscall_write(fd, buf, nbyte);
    }

#if 0
    general_write_wait(fd);
	ssize_t writeret = zsyscall_write(fd, buf ,nbyte);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return writeret;
#else
    while(1) {
        general_write_wait(fd);
        ssize_t writeret = zsyscall_write(fd, buf ,nbyte);
        int ec = errno;
        if ((writeret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
        return zsyscall_writev(fd, iov, iovcnt);
    }

    zcoroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcoroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zsyscall_writev(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zvar_coroutine_block_pthread_count_limit < 1) {
            return zsyscall_writev(fd, iov, iovcnt);
        }
        zcoroutine_hook_fileio_run_part2(write);
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
        return zsyscall_writev(fd, iov, iovcnt);
    }
#if 0
    general_write_wait(fd);
	ssize_t writeret = zsyscall_writev(fd, iov, iovcnt);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return writeret;
#else
    while(1) {
        general_write_wait(fd);
        ssize_t writeret = zsyscall_writev(fd, iov, iovcnt);
        int ec = errno;
        if ((writeret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
    return_zcc_call_co(socket) {
        return zsyscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    }
#if 0
    general_write_wait(socket);
    ret = zsyscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        general_write_wait(socket);
        ret = zsyscall_sendto(socket,message,length,flags,dest_addr,dest_len);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
    return_zcc_call_co(socket) {
		return zsyscall_recvfrom(socket,buf,length,flags,address,address_len);
    }
#if 0
    general_read_wait(socket);
	ssize_t ret = zsyscall_recvfrom(socket,buf,length,flags,address,address_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
	return ret;
#else
    while(1) {
        general_read_wait(socket);
        ssize_t ret = zsyscall_recvfrom(socket,buf,length,flags,address,address_len);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
    return_zcc_call_co(socket) {
		return zsyscall_send(socket,buffer,length,flags);
    }
#if 0
    general_write_wait(socket);
    int ret = zsyscall_send(socket,(const char*)buffer, length, flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        general_write_wait(socket);
        int ret = zsyscall_send(socket,(const char*)buffer, length, flags);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
    return_zcc_call_co(socket) {
		return zsyscall_recv(socket,buffer,length,flags);
    }
#if 0
    general_read_wait(socket);
	ssize_t ret = zsyscall_recv(socket,buffer,length,flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
	return ret;
#else
    while(1) {
        general_read_wait(socket);
        ssize_t ret = zsyscall_recv(socket,buffer,length,flags);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
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
    return_zcc_call_co(fd) {
		return zsyscall_setsockopt(fd,level,option_name,option_value,option_len);
    }

	if(SOL_SOCKET == level) {
		struct timeval *val = (struct timeval*)option_value;
        long t = val->tv_sec * 1000 + val->tv_usec/1000;
        if (t > 256 * 128 -1) {
            t = 256 * 128 -1;
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
	return zsyscall_setsockopt(fd,level,option_name,option_value,option_len);
}
/* }}} */

/* {{{ fcntl hook */
int fcntl(int fildes, int cmd, ...)
{
	if(fildes < 0) {
        errno = EINVAL;
		return -1;
	}
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();

	va_list args;
	va_start(args,cmd);

	int ret = -1;
	switch(cmd)
	{
		case F_DUPFD:
		{
			int param = va_arg(args,int);
			ret = zsyscall_fcntl(fildes,cmd,param);
            if (cobs == 0) {
                break;
            }
            if (ret > -1) {
                zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(fildes);
                if (cfa && (cfa->pseudo_mode == 0)) {
                    zcoroutine_fd_attribute_create(ret);
                    fcntl(ret, F_SETFL, zsyscall_fcntl(ret, F_GETFL,0));
                }
            }
			break;
		}
		case F_GETFD:
		{
			ret = zsyscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETFD:
		{
			int param = va_arg(args,int);
			ret = zsyscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_GETFL:
		{
			ret = zsyscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETFL:
		{
			int param = va_arg(args,int);
            if (cobs == 0) {
                ret = zsyscall_fcntl(fildes,cmd,param);
                break;
            }
			int flag = param;
            zcoroutine_fd_attribute *cfa = zcoroutine_fd_attribute_get(fildes);
            if (cfa) {
				flag |= O_NONBLOCK;
			}
			ret = zsyscall_fcntl(fildes,cmd,flag);
			if((0 == ret) && cfa) {
                cfa->nonblock = ((param&O_NONBLOCK)?1:0);
			}
			break;
		}
		case F_GETOWN:
		{
			ret = zsyscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETOWN:
		{
			int param = va_arg(args,int);
			ret = zsyscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_GETLK:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zsyscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_SETLK:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zsyscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_SETLKW:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zsyscall_fcntl(fildes,cmd,param);
			break;
		}
	}
	va_end(args);

	return ret;
}
/* }}} */

/* {{{ __res_state  hook */
extern __thread struct __res_state *__resp;
res_state __res_state2(void)
{
    printf("__res_state2 AAAAAAAAAAAAAAAAAAAAAAAAAA new\n");
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    if (cobs == 0) {
        return __resp;
    }
    printf("__res_state2 AAAAAAAAAAAAAAAAAAAAAAAAAA new\n");
    if (cobs->current_coroutine->res_state == 0) {
        cobs->current_coroutine->res_state = (struct __res_state *)zcalloc(1, sizeof(struct __res_state));
    }
    return cobs->current_coroutine->res_state;
}
/* }}} */

/* {{{ gethostbyname  hook */
struct hostent *gethostbyname(const char *name)
{
    return gethostbyname2(name, AF_INET);
}

struct hostent* gethostbyname2(const char* name, int af)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    char **_hp_char_ptr = 0;
    zgethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(gethostbyname_pthread_key);
        _hp = (zgethostbyname_buf_t *)(*_hp_char_ptr);
    }
   
    if (_hp && (_hp->buf_size > 1024)) {
        zfree(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zgethostbyname_buf_t *)zmalloc(sizeof(zgethostbyname_buf_t) + 1024 + 1);
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
        zfree(_hp);
        _hp = (zgethostbyname_buf_t *)zmalloc(sizeof(zgethostbyname_buf_t) + nsize + 1);
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

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
    zcoroutine_base_t *cobs = zcoroutine_base_get_current();
    char **_hp_char_ptr = 0;
    zgethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(gethostbyname_pthread_key);
        _hp = (zgethostbyname_buf_t *)(*_hp_char_ptr);
    }
   
    if (_hp && (_hp->buf_size > 1024)) {
        zfree(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zgethostbyname_buf_t *)zmalloc(sizeof(zgethostbyname_buf_t) + 1024 + 1);
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
        zfree(_hp);
        _hp = (zgethostbyname_buf_t *)zmalloc(sizeof(zgethostbyname_buf_t) + nsize + 1);
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

/* file io hook {{{ */
off_t lseek(int fd, off_t offset, int whence)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_lseek(fd, offset, whence);
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
        return zsyscall_fdatasync(fd);
    }
    zcoroutine_hook_fileio_run_part2(fdatasync);
    fileio.args[0].int_t = fd;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fsync(int fd)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_fsync(fd);
    }
    zcoroutine_hook_fileio_run_part2(fsync);
    fileio.args[0].int_t = fd;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int rename(const char *oldpath, const char *newpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_rename(oldpath, newpath);
    }
    zcoroutine_hook_fileio_run_part2(rename);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int truncate(const char *path, off_t length)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_truncate(path, length);
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
        return zsyscall_ftruncate(fd, length);
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
        return zsyscall_rmdir(pathname);
    }
    zcoroutine_hook_fileio_run_part2(rmdir);
    fileio.args[0].const_char_ptr_t = pathname;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int mkdir(const char *pathname, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_mkdir(pathname, mode);
    }
    zcoroutine_hook_fileio_run_part2(mkdir);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int getdents(unsigned int fd, char *dirp, unsigned int count)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_getdents(fd, (void *)dirp, count);
    }
    zcoroutine_hook_fileio_run_part2(getdents);
    fileio.args[0].uint_t = fd;
    fileio.args[1].void_ptr_t = dirp;
    fileio.args[1].uint_t = count;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int stat(const char *pathname, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_stat(pathname, buf);
    }
    zcoroutine_hook_fileio_run_part2(stat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fstat(int fd, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_fstat(fd, buf);
    }
    zcoroutine_hook_fileio_run_part2(fstat);
    fileio.args[0].int_t = fd;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int lstat(const char *pathname, struct stat *buf)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_lstat(pathname, buf);
    }
    zcoroutine_hook_fileio_run_part2(lstat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int link(const char *oldpath, const char *newpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_link(oldpath, newpath);
    }
    zcoroutine_hook_fileio_run_part2(link);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int symlink(const char *target, const char *linkpath)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_symlink(target, linkpath);
    }
    zcoroutine_hook_fileio_run_part2(symlink);
    fileio.args[0].const_char_ptr_t = target;
    fileio.args[1].const_char_ptr_t = linkpath;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_readlink(pathname, buf, bufsiz);
    }
    zcoroutine_hook_fileio_run_part2(readlink);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].char_ptr_t = buf;
    fileio.args[2].size_size_t = bufsiz;
    zcoroutine_hook_fileio_run_part3();
    return retval.ssize_ssize_t;
}

int unlink(const char *pathname)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_unlink(pathname);
    }
    zcoroutine_hook_fileio_run_part2(unlink);
    fileio.args[0].const_char_ptr_t = pathname;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int chmod(const char *pathname, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_chmod(pathname, mode);
    }
    zcoroutine_hook_fileio_run_part2(chmod);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fchmod(int fd, mode_t mode)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_fchmod(fd, mode);
    }
    zcoroutine_hook_fileio_run_part2(fchmod);
    fileio.args[0].int_t = fd;
    fileio.args[1].mode_mode_t = mode;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int chown(const char *pathname, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_chown(pathname, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(chown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_fchown(fd, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(fchown);
    fileio.args[0].int_t = fd;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int lchown(const char *pathname, uid_t owner, gid_t group)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_lchown(pathname, owner, group);
    }
    zcoroutine_hook_fileio_run_part2(lchown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int utime(const char *filename, const struct utimbuf *times)
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_utime(filename, times);
    }
    zcoroutine_hook_fileio_run_part2(utime);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int utimes(const char *filename, const struct timeval times[2])
{
    zcoroutine_hook_fileio_run_part1() {
        return zsyscall_utimes(filename, times);
    }
    zcoroutine_hook_fileio_run_part2(utimes);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    zcoroutine_hook_fileio_run_part3();
    return retval.int_t;
}

/* }}} */

#pragma pack(pop)
/* Local variables:
* End:
* vim600: fdm=marker
*/
