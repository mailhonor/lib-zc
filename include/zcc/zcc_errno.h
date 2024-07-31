/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_ERRNO___
#define ZCC_LIB_INCLUDE_ERRNO___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

#define ZCC_EPERM 1
#define ZCC_ENOENT 2
#define ZCC_ESRCH 3
#define ZCC_EINTR 4
#define ZCC_EIO 5
#define ZCC_ENXIO 6
#define ZCC_E2BIG 7
#define ZCC_ENOEXEC 8
#define ZCC_EBADF 9
#define ZCC_ECHILD 10
#define ZCC_EAGAIN 11
#define ZCC_ENOMEM 12
#define ZCC_EACCES 13
#define ZCC_EFAULT 14
#define ZCC_EBUSY 16
#define ZCC_EEXIST 17
#define ZCC_EXDEV 18
#define ZCC_ENODEV 19
#define ZCC_ENOTDIR 20
#define ZCC_EISDIR 21
#define ZCC_EINVAL 22
#define ZCC_ENFILE 23
#define ZCC_EMFILE 24
#define ZCC_ENOTTY 25
#define ZCC_EFBIG 27
#define ZCC_ENOSPC 28
#define ZCC_ESPIPE 29
#define ZCC_EROFS 30
#define ZCC_EMLINK 31
#define ZCC_EPIPE 32
#define ZCC_EDOM 33
#define ZCC_ERANGE 34
#define ZCC_EDEADLK 36
#define ZCC_ENAMETOOLONG 38
#define ZCC_ENOLCK 39
#define ZCC_ENOSYS 40
#define ZCC_ENOTEMPTY 41
#define ZCC_EILSEQ 42
#define ZCC_STRUNCATE 80
#define ZCC_EDEADLOCK ZCC_EDEADLK
#define ZCC_EADDRINUSE 100
#define ZCC_EADDRNOTAVAIL 101
#define ZCC_EAFNOSUPPORT 102
#define ZCC_EALREADY 103
#define ZCC_EBADMSG 104
#define ZCC_ECANCELED 105
#define ZCC_ECONNABORTED 106
#define ZCC_ECONNREFUSED 107
#define ZCC_ECONNRESET 108
#define ZCC_EDESTADDRREQ 109
#define ZCC_EHOSTUNREACH 110
#define ZCC_EIDRM 111
#define ZCC_EINPROGRESS 112
#define ZCC_EISCONN 113
#define ZCC_ELOOP 114
#define ZCC_EMSGSIZE 115
#define ZCC_ENETDOWN 116
#define ZCC_ENETRESET 117
#define ZCC_ENETUNREACH 118
#define ZCC_ENOBUFS 119
#define ZCC_ENODATA 120
#define ZCC_ENOLINK 121
#define ZCC_ENOMSG 122
#define ZCC_ENOPROTOOPT 123
#define ZCC_ENOSR 124
#define ZCC_ENOSTR 125
#define ZCC_ENOTCONN 126
#define ZCC_ENOTRECOVERABLE 127
#define ZCC_ENOTSOCK 128
#define ZCC_ENOTSUP 129
#define ZCC_EOPNOTSUPP 130
#define ZCC_EOTHER 131
#define ZCC_EOVERFLOW 132
#define ZCC_EOWNERDEAD 133
#define ZCC_EPROTO 134
#define ZCC_EPROTONOSUPPORT 135
#define ZCC_EPROTOTYPE 136
#define ZCC_ETIME 137
#define ZCC_ETIMEDOUT 138
#define ZCC_ETXTBSY 139
#define ZCC_EWOULDBLOCK 140

int get_errno(int code = 0);
void set_errno(int code);

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_ERRNO___
