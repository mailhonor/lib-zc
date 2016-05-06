/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "libzc.h"

static void cmd_usage(zkvlist_t * kv)
{
    printf("ERR usage:\n");
    printf("add key=value\n");
    printf("delete key\n");
    printf("list\n");
    printf("reload\n");
    printf("exit\n");
    printf("\n");
}

static void cmd_exit(zkvlist_t * kv, char *cmdline)
{
    printf("OK exit BYE\n");
}

static void cmd_reload(zkvlist_t * kv, char *cmdline)
{
    zkvlist_load(kv);
    printf("OK reload\n");
}

static void cmd_list(zkvlist_t * kv, char *cmdline)
{
    zdict_t *dict;
    zdict_node_t *n;

    dict = zkvlist_get_dict(kv);
    for (n = zdict_first(dict); n; n = zdict_next(n)) {
        printf("%s = %s\n", n->key, (char *)(n->value));
    }
    printf("OK list\n");
}

static void ___parse_kv(char *line, char **key, char **value)
{
    char *k, *v, *p;

    k = 0;
    v = 0;
    if (*line == ' ') {
        line++;
        k = line;
        p = strchr(line, '=');
        if (p) {
            *p = 0;
            v = p + 1;
        }
    }
    if (key) {
        *key = k;
    }
    if (value) {
        *value = v;
    }
}

static void cmd_add(zkvlist_t * kv, char *cmdline)
{
    char *n, *v;

    n = 0;
    v = 0;
    ___parse_kv(cmdline + 3, &n, &v);
    if (n == 0 || *n == 0) {
        return cmd_usage(kv);
    }
    if (v == 0 || *v == 0) {
        v = "";
    }

    zkvlist_add(kv, n, v);
    printf("OK add\n");
}

static void cmd_delete(zkvlist_t * kv, char *cmdline)
{
    char *n, *v;

    n = 0;
    v = 0;
    ___parse_kv(cmdline + 6, &n, &v);
    if (n == 0 || *n == 0) {
        return cmd_usage(kv);
    }
    if (v && *v) {
        return cmd_usage(kv);
    }

    zkvlist_delete(kv, n);
    printf("OK delete\n");
}

static void cmd_unknown(zkvlist_t * kv, char *cmdline)
{
    return cmd_usage(kv);
}

static int cmd_do(zkvlist_t * kv, char *cmdline)
{
    if (!strncasecmp(cmdline, "exit", 4)) {
        cmd_exit(kv, cmdline);
        return 0;
    } else if (!strncasecmp(cmdline, "reload", 6)) {
        cmd_reload(kv, cmdline);
    } else if (!strncasecmp(cmdline, "list", 4)) {
        cmd_list(kv, cmdline);
    } else if (!strncasecmp(cmdline, "add", 3)) {
        cmd_add(kv, cmdline);
    } else if (!strncasecmp(cmdline, "delete", 6)) {
        cmd_delete(kv, cmdline);
    } else {
        cmd_unknown(kv, cmdline);
    }
    return 1;
}

int main(int argc, char **argv)
{
    zkvlist_t *kv;
    char *dn = "testdb.kv";
    char buf[102401];
    int len;

    zvar_progname = argv[0];
    if (argc < 2) {
        printf("USAGE: %s kvlist.d\n", zvar_progname);
        exit(1);
    }
    dn = argv[1];
    kv = zkvlist_create(dn);
    zkvlist_load(kv);

    while (fgets(buf, 102400, stdin)) {
        len = strlen(buf);
        if ((len > 0) && (buf[len - 1] == '\n')) {
            len--;
        }
        if ((len > 0) && (buf[len - 1] == '\r')) {
            len--;
        }
        buf[len] = 0;

        if (cmd_do(kv, buf) == 0) {
            break;
        }
    }

    return 0;
}
