/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-06-26
 * ================================
 */

#if 0
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
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>

extern int zsyscall_pipe(int pipefd[2]);
extern int zsyscall_pipe2(int pipefd[2], int flags);
extern int zsyscall_dup(int oldfd);
extern int zsyscall_dup2(int oldfd, int newfd);
extern int zsyscall_dup3(int oldfd, int newfd, int flags);
extern int zsyscall_socketpair(int domain, int type, int protocol, int sv[2]);
extern int zsyscall_socket(int domain, int type, int protocol);
extern int zsyscall_accept(int fd, struct sockaddr *addr, socklen_t *len);
extern int zsyscall_connect(int socket, const struct sockaddr *address, socklen_t address_len);
extern int zsyscall_close(int fd);
extern ssize_t zsyscall_read(int fildes, void *buf, size_t nbyte);
extern ssize_t zsyscall_readv(int fd, const struct iovec *iov, int iovcnt);
extern ssize_t zsyscall_write(int fildes, const void *buf, size_t nbyte);
extern ssize_t zsyscall_writev(int fd, const struct iovec *iov, int iovcnt);
extern ssize_t zsyscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
extern ssize_t zsyscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
extern size_t zsyscall_send(int socket, const void *buffer, size_t length, int flags);
extern ssize_t zsyscall_recv(int socket, void *buffer, size_t length, int flags);
extern int zsyscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);
extern int zsyscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
extern int zsyscall_fcntl(int fildes, int cmd, ...);
extern pid_t zsyscall_gettid(void);
extern int zsyscall_open(const char *pathname, int flags, ...);
extern int zsyscall_openat(int dirfd, const char *pathname, int flags, ...);
extern int zsyscall_creat(const char *pathname, mode_t mode);
extern off_t zsyscall_lseek(int fd, off_t offset, int whence);
extern int zsyscall_fdatasync(int fd);
extern int zsyscall_fsync(int fd);
extern int zsyscall_rename(const char *oldpath, const char *newpath);
extern int zsyscall_truncate(const char *path, off_t length);
extern int zsyscall_ftruncate(int fd, off_t length);
extern int zsyscall_rmdir(const char *pathname);
extern int zsyscall_mkdir(const char *pathname, mode_t mode);
extern int zsyscall_getdents(unsigned int fd, void *dirp, unsigned int count);
extern int zsyscall_stat(const char *pathname, struct stat *buf);
extern int zsyscall_fstat(int fd, struct stat *buf);
extern int zsyscall_lstat(const char *pathname, struct stat *buf);
extern int zsyscall_link(const char *oldpath, const char *newpath);
extern int zsyscall_symlink(const char *target, const char *linkpath);
extern ssize_t zsyscall_readlink(const char *pathname, char *buf, size_t bufsiz);
extern int zsyscall_unlink(const char *pathname);
extern int zsyscall_chmod(const char *pathname, mode_t mode);
extern int zsyscall_fchmod(int fd, mode_t mode);
extern int zsyscall_chown(const char *pathname, uid_t owner, gid_t group);
extern int zsyscall_fchown(int fd, uid_t owner, gid_t group);
extern int zsyscall_lchown(const char *pathname, uid_t owner, gid_t group);
extern int zsyscall_utime(const char *filename, const struct utimbuf *times);
extern int zsyscall_utimes(const char *filename, const struct timeval times[2]);

/* io */
int zsyscall_pipe(int pipefd[2])
{
    return syscall(__NR_pipe, pipefd);
}

int zsyscall_pipe2(int pipefd[2], int flags)
{
    return syscall(__NR_pipe2, pipefd, flags);
}

int zsyscall_dup(int oldfd)
{
    return syscall(__NR_dup, oldfd);
}

int zsyscall_dup2(int oldfd, int newfd)
{
    return syscall(__NR_dup2, oldfd, newfd);
}

int zsyscall_dup3(int oldfd, int newfd, int flags)
{
    return syscall(__NR_dup3, oldfd, newfd, flags);
}

int zsyscall_socketpair(int domain, int type, int protocol, int sv[2])
{
    return syscall(__NR_socketpair, domain, type, protocol, sv);
}

int zsyscall_socket(int domain, int type, int protocol)
{
    return syscall(__NR_socket, domain, type, protocol);
}

int zsyscall_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    return syscall(__NR_accept, fd, addr, len);
}

int zsyscall_connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return syscall(__NR_connect, socket, address, address_len);
}

int zsyscall_close(int fd)
{
    return syscall(__NR_close, fd);
}

ssize_t zsyscall_read(int fildes, void *buf, size_t nbyte)
{
    return syscall(__NR_read, fildes, buf, nbyte);
}

ssize_t zsyscall_readv(int fd, const struct iovec *iov, int iovcnt)
{
    return syscall(__NR_readv, fd, iov, iovcnt);
}

ssize_t zsyscall_write(int fildes, const void *buf, size_t nbyte)
{
    return syscall(__NR_write, fildes, buf, nbyte);
}

ssize_t zsyscall_writev(int fd, const struct iovec *iov, int iovcnt)
{
    return syscall(__NR_writev, fd, iov, iovcnt);
}

ssize_t zsyscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    return syscall(__NR_sendto, socket, message, length, flags, dest_addr, dest_len);
}

ssize_t zsyscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, address, address_len);
}

size_t zsyscall_send(int socket, const void *buffer, size_t length, int flags)
{
    return syscall(__NR_sendto, socket, buffer, length, flags, 0, 0);
}

ssize_t zsyscall_recv(int socket, void *buffer, size_t length, int flags)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, 0, 0);
}

int zsyscall_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return syscall(__NR_poll, fds, nfds, timeout);
}

int zsyscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return syscall(__NR_setsockopt, socket, level, option_name, option_value, option_len);
}

int zsyscall_fcntl(int fildes, int cmd, ...)
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

pid_t zsyscall_gettid(void)
{
    return syscall(__NR_gettid);
}

int zsyscall_open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return syscall(__NR_open, pathname, flags, mode);
}

int zsyscall_openat(int dirfd, const char *pathname, int flags, ...)
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

int zsyscall_creat(const char *pathname, mode_t mode)
{
    return syscall(__NR_creat, pathname, mode);
}

off_t zsyscall_lseek(int fd, off_t offset, int whence)
{
    return syscall(__NR_lseek, fd, offset, whence);
}

int zsyscall_fdatasync(int fd)
{
    return syscall(__NR_fdatasync, fd);
}

int zsyscall_fsync(int fd)
{
    return syscall(__NR_fsync, fd);
}

int zsyscall_rename(const char *oldpath, const char *newpath)
{
    return syscall(__NR_rename, oldpath, newpath);
}

int zsyscall_truncate(const char *path, off_t length)
{
    return syscall(__NR_truncate, path, length);
}

int zsyscall_ftruncate(int fd, off_t length)
{
    return syscall(__NR_truncate, fd, length);
}

int zsyscall_rmdir(const char *pathname)
{
    return syscall(__NR_rmdir, pathname);
}

int zsyscall_mkdir(const char *pathname, mode_t mode)
{
    return syscall(__NR_mkdir, pathname, mode);
}

int zsyscall_getdents(unsigned int fd, void *dirp, unsigned int count)
{
    return syscall(__NR_getdents, fd, dirp, count);
}

int zsyscall_stat(const char *pathname, struct stat *buf)
{
    return syscall(__NR_stat, pathname, buf);
}

int zsyscall_fstat(int fd, struct stat *buf)
{
    return syscall(__NR_fstat, fd, buf);
}

int zsyscall_lstat(const char *pathname, struct stat *buf)
{
    return syscall(__NR_lstat, pathname, buf);
}

int zsyscall_link(const char *oldpath, const char *newpath)
{
    return syscall(__NR_link, oldpath, newpath);
}

int zsyscall_symlink(const char *target, const char *linkpath)
{
    return syscall(__NR_symlink, target, linkpath);
}

ssize_t zsyscall_readlink(const char *pathname, char *buf, size_t bufsiz)
{
    return syscall(__NR_readlink, pathname, buf, bufsiz);
}

int zsyscall_unlink(const char *pathname)
{
    return syscall(__NR_unlink, pathname);
}

int zsyscall_chmod(const char *pathname, mode_t mode)
{
    return syscall(__NR_chmod, pathname, mode);
}

int zsyscall_fchmod(int fd, mode_t mode)
{
    return syscall(__NR_fchmod, fd, mode);
}

int zsyscall_chown(const char *pathname, uid_t owner, gid_t group)
{
    return syscall(__NR_chown, pathname, owner, group);
}

int zsyscall_fchown(int fd, uid_t owner, gid_t group)
{
    return syscall(__NR_fchown, fd, owner, group);
}

int zsyscall_lchown(const char *pathname, uid_t owner, gid_t group)
{
    return syscall(__NR_lchown, pathname, owner, group);
}

int zsyscall_utime(const char *filename, const struct utimbuf *times)
{
    return syscall(__NR_utime, filename, times);
}

int zsyscall_utimes(const char *filename, const struct timeval times[2])
{
    return syscall(__NR_utimes, filename, times);
}

int zsyscall_futimes(int fd, const struct timeval tv[2])
{
    return syscall(__NR_futimes, fd, tv);
}

int zsyscall_lutimes(const char *filename, const struct timeval tv[2])
{
    return syscall(__NR_lutimes, filename, tv);
}
#endif // 0

#ifdef _WIN32
#include <pthread.h>
ssize_t zgettid(void)
{
    return (ssize_t)pthread_self();
}
#else // _WIN32
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

ssize_t zgettid(void)
{
#ifdef __NR_gettid
    return (ssize_t)syscall(__NR_gettid);
#else
    errno = ENOSYS;
    return -1;
#endif
}
#endif // _WIN32

