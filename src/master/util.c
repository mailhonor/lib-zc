/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-20
 * ================================
 */

#ifdef __linux__

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zc.h"
#include "master.h"
#include <dirent.h>

void zmaster_load_global_config_from_dir_inner(zconfig_t *cf, const char *config_path)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];

    dir = opendir(config_path);
    if (!dir) {
        return;
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list)) {
        fn = ent.d_name;
        if (fn[0] == '.') {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "gcf"))) {
            continue;
        }
        zsnprintf(pn, 4096, "%s/%s", config_path, fn);
        zconfig_load_from_pathname(cf, pn);
    }
    closedir(dir);
}

void zmaster_log_use_inner(char *progname, char *log_uri)
{
    if (zempty(log_uri)) {
        return;
    }
    zargv_t *largv = zargv_create(-1);
    zargv_split_append(largv, log_uri, ", \t");
    char **lv = zargv_data(largv);
    int llen = zargv_len(largv);

    char *type = lv[0];
    if ((llen== 0) || zempty(type)) {
        goto over;
    }
    char *facility = 0;
    char *identity = progname;
    identity = strrchr(identity, '/');
    if (identity) {
        identity++;
    } else {
        identity = progname;
    }
    if (!strcmp(type, "syslog")) {
        if ((llen <2) || (llen > 3)) {
            zfatal("syslog mode, value: syslog,facility,identity or syslog,facility");
        }
        facility = lv[1];
        if (llen == 3) {
            identity = lv[2];
        }
        zlog_use_syslog(identity, zlog_get_facility_from_str(facility));
    } else if (!strcmp(type, "masterlog")) {
        if (llen != 2) {
            zfatal("masterlog mode, value: masterlog,log_socket");
        }
        zlog_use_masterlog(identity, lv[1]);
    } else if (!strcmp(type, "stdout")) {
        void zlog_use_stdout();
        zlog_use_stdout();
    }
over:
    zargv_free(largv);
}

#endif // __linux__


