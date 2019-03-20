/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */


#include "zc.h"

static long nval;
static zbuf_t *sval, *sval2;
static zvector_t *vval;
static zjson_t *jval;

static void usage()
{
    printf("\n");
    printf("%s [ -server 127.0.0.1:6379 ] redis_cmd arg1 arg2 ...\n", zvar_progname);
    exit(1);
}

static void _test_test(zredis_client_t *rc, const char *cmd, int cmd_ret, size_t line, int test_type)
{
    printf("\n%s\n%-8d", cmd, cmd_ret);
    if (cmd_ret < 0) {
        printf("%s  ### line:%zd", zredis_client_get_error_msg(rc), line);
    } else if (test_type == 'r') {
        if (cmd_ret == 0) {
            printf("none/no/not");
        } else {
            printf("exists/yes/ok/count");
        }
    } else if(test_type == 'n') {
        printf("number: %ld", nval);
    } else if(test_type == 's') {
        printf("string: %s", zbuf_data(sval));
    } else if(test_type == 'v') {
        printf("vector: ");
        ZVECTOR_WALK_BEGIN(vval, zbuf_t *, bf) {
            printf("%s, ", bf?zbuf_data(bf):"null");
        } ZVECTOR_WALK_END;
    } else if(test_type == 'j') {
        zbuf_t *out = zbuf_create(-1);
        zjson_serialize(jval, out, 0);
        printf("json: %s", zbuf_data(out));
        zbuf_free(out);
    }
    printf("\n"); fflush(stdout);
    nval = -1000;
    zbuf_reset(sval);
    zbuf_reset(sval2);
    zbuf_vector_reset(vval);
    zjson_reset(jval);
}

#define _test_return(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'r')
#define _test_number(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'n')
#define _test_string(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 's')
#define _test___list(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'v')
#define _test___json(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'j')

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    zredis_client_t *rc;
    char *server = zconfig_get_str(zvar_default_config, "server", "127.0.0.1:6379");
    int ex = zconfig_get_bool(zvar_default_config, "cluster", 0);
    if (ex) {
        rc = zredis_client_connect_cluster(server, 0, 10, 1);
    } else {
    return 0;
        rc = zredis_client_connect(server, 0, 10, 1);
    }

    sval = zbuf_create(-13);
    sval2 = zbuf_create(-13);
    vval = zvector_create(-1);
    jval = zjson_create();

    if (!rc) {
        printf("ERR can not connect %s\n", server);
        goto over;
    }

    if (zvar_main_redundant_argc > 0 ) {
        _test___json(zredis_client_get_json(rc, jval, "P", zvar_main_redundant_argv));
        goto over;
    }

    _test_return(zredis_client_get_success(rc, "sss", "SET", "abc", "ssssss"));
    _test_string(zredis_client_get_string(rc, sval, "ss", "GET", "abc"));
    _test_string(zredis_client_get_string(rc, sval, "sss", "HGET", "xxx.com_u", "ac.tai"));

    _test_return(zredis_client_get_success(rc, "ss", "STRLEN", "abc"));
    _test_number(zredis_client_get_long(rc, &nval, "ss", "STRLEN", "abc"));

    _test___list(zredis_client_get_vector(rc, vval, "sss", "mget", "abc", "fasdfdsaf"));

    _test___json(zredis_client_get_json(rc, jval, "sss", "MGET", "abc", "sss"));
    _test___json(zredis_client_get_json(rc, jval, "sd", "SCAN", 0));
    _test___json(zredis_client_get_json(rc, jval, "ssd", "EVAL", "return {1,2,{3,'Hello World!'}}", 0));
    _test___json(zredis_client_get_json(rc, jval, "sd", "fffSCAN", 0));
    
over:
    zbuf_free(sval);
    zbuf_free(sval2);
    zbuf_vector_free(vval);
    zjson_free(jval);
    if(rc) {
        zredis_client_free(rc);
    }
    usage();
    return 0;
}
