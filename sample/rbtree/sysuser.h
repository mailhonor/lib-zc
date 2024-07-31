/*
 * ================================
 * eli960@163.com
 * http:/linuxmail.cn/
 * 2015-12-02
 * ================================
 */

#include "../../zc.h"

typedef struct zsdata_t  zsdata_t;
struct zsdata_t
{
    int size;
    char *data;
};

/* system user from /etc/passwd*/
typedef struct sysuser_t sysuser_t;
struct sysuser_t {
    char *login_name;
    char *passwd;
    char *user_id;
    char *group_id;
    char *name;
    char *home;
    char *shell;
};

sysuser_t *sysuser_list;
static int sysuser_count;

static int sysuser_parseline(char *linebuf, zsdata_t * list)
{
    char *ps, *p;
    int i;

    ps = linebuf;
    i = 0;
    while (1) {
        list[i].data = ps;
        p = strchr(ps, ':');
        if (p) {
            list[i].size = p - ps;
            ps = p + 1;
            i++;
        } else {
            list[i].size = strlen(ps);
            break;
        }
        if (i >= 7) {
            zfatal("/etc/passwd line error: %s", linebuf);
        }
    }

    return i + 1;
}

static void sysuser_load(void)
{
    FILE *fp = 0;
    char linebuf[102404];
    zsdata_t sdlist[7];
    int i;
    sysuser_t *user, ulist[10240];
    int count = 0;

    fp = fopen("/etc/passwd", "rb");
    while (fgets(linebuf, 102400, fp)) {
        user = ulist + count;
        if (sysuser_parseline(linebuf, sdlist) != 7) {
            continue;
        }
#define ___USDP(a)  {user->a=zstrndup(sdlist[i].data, sdlist[i].size);i++;}
        i = 0;
        ___USDP(login_name);
        ___USDP(passwd);
        ___USDP(user_id);
        ___USDP(group_id);
        ___USDP(name);
        ___USDP(home);
        ___USDP(shell);
        count++;
    }
    fclose(fp);

    sysuser_count = count;
    sysuser_list = (sysuser_t *) zmemdup(ulist, count * sizeof(sysuser_t));
}

static void sysuser_unload(void)
{
    int i;

    for (i = 0; i < sysuser_count; i++) {
#define ___FR(a)    {zfree(sysuser_list[i].a);}
        ___FR(login_name);
        ___FR(passwd);
        ___FR(user_id);
        ___FR(group_id);
        ___FR(name);
        ___FR(home);
        ___FR(shell);
    }
    if (sysuser_list) {
        zfree(sysuser_list);
    }
}
