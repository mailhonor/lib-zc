/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-12-27
 * ================================
 */

#include "zc.h"

static void _test_core()
{
    zprintf("\nTEST CORE\n");
    if (!zconfig_get_bool(zvar_default_config, "test-core", 0)) {
        zprintf("NEED parameters --test-core -core-file-size 123M\n");
        return;
    }
    char *p = 0;
    *p = 0;
}

static void _test_max_memory()
{
    zprintf("\nTEST MAX MEMORY\n");
    if (!zconfig_get_bool(zvar_default_config, "test-memory", 0)) {
        zprintf("NEED parameters --test-memory -max-memory 123M\n");
        return;
    }
    if (!zconfig_get_str(zvar_default_config, "max-memory", 0)) {
        zprintf("NEED parameters --test-memory -max-memory 123M\n");
        return;
    }
    zprintf("now, malloc 1024M and memset\n");
    char *p = (char *)zmalloc(1024L*1024*1024 + 1);
    memset(p, 1, 1024L*1024*1024);
    zprintf("now malloc 1024M and memset, over\n");
    zprintf("top -p %ld, see memory info\n", (long)getpid());
    zprintf("enter for continue\n");
    getchar();
}

static void _test_config()
{
    zprintf("\nTEST CONFIG\n");
    if (!zconfig_get_bool(zvar_default_config, "test-config", 0)) {
        zprintf("NEED parameters --test-config\n");
        return;
    }
    zconfig_debug_show(zvar_default_config);
}


int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    _test_core();
    _test_max_memory();
    _test_config();
    return 0;
}

