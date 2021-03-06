/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-03
 * ================================
 */

/* postfix source, src/util/chroot_uid.c */

#include "zc.h"
#include <pwd.h>
#include <grp.h>

#define ___zinfo(fmt,args...){errno2=errno;zinfo("chroot_user: "fmt,##args);errno=errno2;}
zbool_t chroot_user(const char *root_dir, const char *user_name)
{
    int errno2;
    struct passwd *pwd;
    uid_t uid;
    gid_t gid;

    /*
     * Look up the uid/gid before entering the jail, and save them so they
     * can't be clobbered. Set up the primary and secondary groups.
     */
    uid = 0;
    if (user_name != 0) {
        if ((pwd = getpwnam(user_name)) == 0) {
            ___zinfo("unknown user: %s", user_name);
            errno = ENONET;
            return 0;
        }
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;
        if (setgid(gid) < 0) {
            ___zinfo("setgid %ld : %m", (long)gid);
            return 0;
        }
        if (initgroups(user_name, gid) < 0) {
            ___zinfo("initgroups: %m");
            return 0;
        }
    }

    /*
     * Enter the jail.
     */
    if (root_dir) {
        if (chroot(root_dir)) {
            ___zinfo("chroot (%s) : %m", root_dir);
            return 0;
        }
        if (chdir("/")) {
            ___zinfo("chdir (/): %m");
            return 0;
        }
    }

    /*
     * Drop the user privileges.
     */
    if (user_name != 0) {
        if (setuid(uid) < 0) {
            ___zinfo("setuid %ld: %m", (long)uid);
            return 0;
        }
    }

    return 1;
}
