/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_errno.h"
#ifdef _WIN64
#include <WinSock2.h>
#include <handleapi.h>
#include <io.h>
#else // _WIN64
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif // __linux__
#include <poll.h>
#endif // _WIN64

zcc_namespace_begin;

#ifdef __linux__
/**
 * @brief 通过Unix域套接字发送文件描述符
 * @param fd 用于发送的套接字描述符
 * @param sendfd 要发送的文件描述符
 * @return 成功返回1，失败返回-1
 */
/* postfix src/util/unix_send_fd.c */
int send_fd(int fd, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union
    {
        struct cmsghdr just_for_alignment;
        char control[CMSG_SPACE(sizeof(sendfd))];
    } control_un;
    struct cmsghdr *cmptr;

    // 初始化消息头
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(sendfd));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;

    int *int_ptr = (int *)(CMSG_DATA(cmptr));
    *int_ptr = sendfd;

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    iov->iov_base = var_blank_buffer;
    iov->iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int sendmsg_ret;
    do
    {
        // 调用sendmsg函数发送消息
        sendmsg_ret = ::sendmsg(fd, &msg, 0);
    } while ((sendmsg_ret < 0) && (errno == EINTR));
    if (sendmsg_ret >= 0)
    {
        return 1;
    }

    return (-1);
}

/**
 * @brief 通过Unix域套接字接收文件描述符
 * @param fd 用于接收的套接字描述符
 * @return 接收到的文件描述符，失败返回-1
 */
/* postfix src/util/unix_recv_fd.c */
int recv_fd(int fd)
{
    struct msghdr msg;
    int newfd;
    struct iovec iov[1];
    char buf[1];

    union
    {
        struct cmsghdr just_for_alignment;
        char control[CMSG_SPACE(sizeof(newfd))];
    } control_un;
    struct cmsghdr *cmptr;

    // 初始化消息头
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    iov->iov_base = buf;
    iov->iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int recvmsg_ret;
    do
    {
        // 调用recvmsg函数接收消息
        recvmsg_ret = ::recvmsg(fd, &msg, 0);
    } while ((recvmsg_ret < 0) && (errno == EINTR));
    if (recvmsg_ret < 0)
    {
        return -1;
    }

    if (((cmptr = CMSG_FIRSTHDR(&msg)) != 0) && (cmptr->cmsg_len == CMSG_LEN(sizeof(newfd))))
    {
        if (cmptr->cmsg_level != SOL_SOCKET)
        {
            // 输出错误信息
            zcc_fatal("control level %d != SOL_SOCKET", cmptr->cmsg_level);
        }
        if (cmptr->cmsg_type != SCM_RIGHTS)
        {
            // 输出错误信息
            zcc_fatal("control type %d != SCM_RIGHTS", cmptr->cmsg_type);
        }
        int *int_ptr = (int *)(CMSG_DATA(cmptr));
        newfd = *int_ptr;
        return newfd;
    }

    return (-1);
}
#endif // __linux__

zcc_namespace_end;
