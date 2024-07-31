/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-12-03
 * ================================
 */

/* postfix source, src/util/chroot_uid.c */

#ifdef __linux__
#include "zcc/zcc_stdlib.h"
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

zcc_namespace_begin;
#define ___zwarning(fmt, args...)                 \
    {                                             \
        errno2 = errno;                           \
        zcc_warning("chroot_user: " fmt, ##args); \
        errno = errno2;                           \
    }
int chroot_user(const char *root_dir, const char *user_name)
{
    int errno2;
    struct passwd *pwd;
    uid_t uid;
    gid_t gid;

    uid = 0;
    if (user_name != 0)
    {
        if ((pwd = getpwnam(user_name)) == 0)
        {
            ___zwarning("unknown user: %s", user_name);
            errno = ENONET;
            return -1;
        }
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;
        if (setgid(gid) < 0)
        {
            ___zwarning("setgid %ld : %m", (long)gid);
            return -1;
        }
        if (initgroups(user_name, gid) < 0)
        {
            ___zwarning("initgroups: %m");
            return -1;
        }
    }

    /*
     * Enter the jail.
     */
    if (root_dir)
    {
        if (chroot(root_dir))
        {
            ___zwarning("chroot (%s) : %m", root_dir);
            return -1;
        }
        if (chdir("/"))
        {
            ___zwarning("chdir (/): %m");
            return -1;
        }
    }

    /*
     * Drop the user privileges.
     */
    if (user_name != 0)
    {
        if (setuid(uid) < 0)
        {
            ___zwarning("setuid %ld: %m", (long)uid);
            return -1;
        }
    }

    return 0;
}

zcc_namespace_end;
#endif // __linux__
