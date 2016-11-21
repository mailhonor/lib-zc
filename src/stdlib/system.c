/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-10-28
 * ================================
 */

/* postfix source, src/util/chroot_uid.c */

#include "libzc.h"
#include <pwd.h>
#include <grp.h>

#define ___zerror_20161028(fmt, args...)    {errno2=errno; zerror(fmt, ##args); errno=errno2;}
int zchroot_user(const char *root_dir, const char *user_name)
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
            ___zerror_20161028("unknown user: %s", user_name);
            errno = ENONET;
            return -1;
        }
        uid = pwd->pw_uid;
        gid = pwd->pw_gid;
        if (setgid(gid) < 0) {
            ___zerror_20161028("setgid %ld : %m", (long)gid);
            return -1;
        }
        if (initgroups(user_name, gid) < 0) {
            ___zerror_20161028("initgroups: %m");
            return -1;
        }
    }

    /*
     * Enter the jail.
     */
    if (root_dir) {
        if (chroot(root_dir)) {
            ___zerror_20161028("chroot (%s) : %m", root_dir);
            return -1;
        }
        if (chdir("/")) {
            ___zerror_20161028("chdir (/): %m");
            return -1;
        }
    }

    /*
     * Drop the user privileges.
     */
    if (user_name != 0) {
        if (setuid(uid) < 0) {
            ___zerror_20161028("setuid %ld: %m", (long)uid);
            return -1;
        }
    }

    /*
     * Give the desperate developer a clue of what is happening.
     */
    zverbose("chroot %s user %s", root_dir ? root_dir : "(none)", user_name ? user_name : "(none)");

    return 0;
}
