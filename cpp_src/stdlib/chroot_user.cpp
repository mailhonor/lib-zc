/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-12-03
 * ================================
 */

/* postfix source, src/util/chroot_uid.c */

// 仅在 Linux 系统下编译此代码
#ifdef __linux__
// 包含自定义标准库头文件
#include "zcc/zcc_stdlib.h"
// 包含用于获取用户信息的头文件
#include <pwd.h>
// 包含用于获取组信息的头文件
#include <grp.h>
// 包含错误号相关的头文件
#include <errno.h>
// 包含系统类型定义的头文件
#include <sys/types.h>
// 包含 Unix 系统调用的头文件
#include <unistd.h>

zcc_namespace_begin;

/**
 * @brief 定义一个宏，用于记录警告信息并保持 errno 不变
 * @param fmt 格式化字符串，用于指定警告信息的格式
 * @param ... 可变参数，用于填充格式化字符串
 */
#define ___zwarning(fmt, args...)                 \
    {                                             \
        errno2 = errno;                           \
        zcc_warning("chroot_user: " fmt, ##args); \
        errno = errno2;                           \
    }

/**
 * @brief 将当前进程切换到指定的根目录并切换到指定用户
 *
 * 该函数首先根据用户名获取用户的 UID 和 GID，然后设置组 ID 和附属组列表，
 * 接着将进程的根目录切换到指定目录，最后设置用户 ID 以降低权限。
 *
 * @param root_dir 要切换到的根目录路径，如果为 NULL 则不进行根目录切换
 * @param user_name 要切换到的用户名，如果为 NULL 则不进行用户切换
 * @return int 成功返回 0，失败返回 -1，并设置相应的错误号
 */
int chroot_user(const char *root_dir, const char *user_name)
{
    // 用于保存临时的 errno 值
    int errno2;
    // 指向用户密码文件条目的指针
    struct passwd *pwd;
    // 用户的 UID
    uid_t uid;
    // 用户的 GID
    gid_t gid;

    // 初始化 UID 为 0（通常是 root 用户）
    uid = 0;
    // 如果指定了用户名
    if (user_name != 0)
    {
        // 根据用户名获取用户的密码文件条目
        if ((pwd = getpwnam(user_name)) == 0)
        {
            // 记录未知用户的警告信息
            ___zwarning("unknown user: %s", user_name);
            // 设置错误号为 ENONET
            errno = ENONET;
            // 返回 -1 表示失败
            return -1;
        }
        // 获取用户的 UID
        uid = pwd->pw_uid;
        // 获取用户的 GID
        gid = pwd->pw_gid;
        // 设置当前进程的组 ID
        if (setgid(gid) < 0)
        {
            // 记录设置组 ID 失败的警告信息
            ___zwarning("setgid %ld : %m", (long)gid);
            // 返回 -1 表示失败
            return -1;
        }
        // 初始化进程的附属组列表
        if (initgroups(user_name, gid) < 0)
        {
            // 记录初始化附属组列表失败的警告信息
            ___zwarning("initgroups: %m");
            // 返回 -1 表示失败
            return -1;
        }
    }

    /*
     * Enter the jail.
     */
    // 如果指定了根目录
    if (root_dir)
    {
        // 将当前进程的根目录切换到指定目录
        if (chroot(root_dir))
        {
            // 记录根目录切换失败的警告信息
            ___zwarning("chroot (%s) : %m", root_dir);
            // 返回 -1 表示失败
            return -1;
        }
        // 将当前进程的工作目录切换到新根目录的根路径
        if (chdir("/"))
        {
            // 记录工作目录切换失败的警告信息
            ___zwarning("chdir (/): %m");
            // 返回 -1 表示失败
            return -1;
        }
    }

    /*
     * Drop the user privileges.
     */
    // 如果指定了用户名
    if (user_name != 0)
    {
        // 设置当前进程的用户 ID
        if (setuid(uid) < 0)
        {
            // 记录设置用户 ID 失败的警告信息
            ___zwarning("setuid %ld: %m", (long)uid);
            // 返回 -1 表示失败
            return -1;
        }
    }

    // 成功返回 0
    return 0;
}

// 退出自定义命名空间
zcc_namespace_end;
#endif // __linux__
