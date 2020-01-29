/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-10-31
 * ================================
 */

#include "zc.h"

typedef struct demo_t demo_t;
typedef struct demo_node_t demo_node_t;

struct demo_t {
    zlink_t link;
    int count;
};
struct demo_node_t {
    zlink_node_t node;
    int value;
};

demo_node_t *demo_add(demo_t *demo, int value)
{
    demo_node_t *node = (demo_node_t *)malloc(sizeof(demo_node_t));
    node->value = value;
    zlink_push(&(demo->link), &(node->node));
    demo->count++;
    return node;
}

demo_node_t *demo_shift(demo_t *demo, int *value)
{
    zlink_node_t *ln = zlink_shift(&(demo->link));
    if (!ln) {
        return 0;
    }
    demo->count--;
    demo_node_t *node = ZCONTAINER_OF(ln, demo_node_t, node);
    if (value) {
        *value = node->value;
    }
    free(node);
    return node;
}

int main(int argc, char **argv)
{
    demo_t *demo = (demo_t *)malloc(sizeof(demo_t));
    zlink_init(&(demo->link));

    demo_add(demo, 2);
    demo_add(demo, 3);
    demo_add(demo, 5);

    int value;
    if (demo_shift(demo, &value)) {
        printf("shift value:%d\n", value);
    }
    demo_shift(demo, 0);
    demo_shift(demo, 0);
    demo_shift(demo, 0);

    zlink_fini(&(demo->link));
    free(demo);

    return 0;
}
