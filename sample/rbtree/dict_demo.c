/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2019-10-25
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zdict_t *dict = zdict_create();

    zdict_update_string(dict, "tom", "cat", -1);
    zdict_update_string(dict, "jerry", "mouse", -1);
    zdict_update_string(dict, "Donald", "duck", -1);

    zprintf("### show items:\n");
    zdict_debug_show(dict);


    zprintf("\n\n### test find:\n");

    if (zdict_find(dict, "tom", 0)) {
        zprintf("found tom\n");
    } else {
        zprintf("not found tom\n");
    }

    zbuf_t *value;
    if (zdict_find(dict, "Donald", &value)) {
        zprintf("found Donald=>%s\n", zbuf_data(value));
    } else {
        zprintf("not found Donald\n");
    }

    if (zdict_find(dict, "mike", &value)) {
        zprintf("found mike=>%s\n", zbuf_data(value));
    } else {
        zprintf("not found mike\n");
    }

    zprintf("\n\n## test near next find(jim):\n");
    zdict_node_t *n;
    if ((n=zdict_find_near_next(dict, "jim", 0))) {
        zprintf("found %s=>%s\n", zdict_node_key(n), zbuf_data(zdict_node_value(n)));
    } else {
        zprintf("not found near next of (jim)\n");
    }

    zprintf("\n\n## test near next find(jerry):\n");
    if ((n=zdict_find_near_next(dict, "jerry", 0))) {
        zprintf("found %s=>%s\n", zdict_node_key(n), zbuf_data(zdict_node_value(n)));
    } else {
        zprintf("not found near next of (jerry)\n");
    }
    zprintf("\n\n## test macro:\n");

    ZDICT_NODE_WALK_BEGIN(dict, rn) {
        zprintf("name: %s, value: %s\n", zdict_node_key(rn), zbuf_data(zdict_node_value(rn)));
    }
    ZDICT_NODE_WALK_END;

    zdict_free(dict);

    return (0);
}
