/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-11-01
 * ================================
 */

#include "zc.h"

typedef struct demo_t demo_t;
typedef struct demo_node_t demo_node_t;
struct demo_t {
    demo_node_t *head_123;
    demo_node_t *tail_456;
    int len;
};
struct demo_node_t {
    demo_node_t *prev_abc;
    demo_node_t *next_def;
    void *value;
};

void demo_init(demo_t *demo)
{
    memset(demo, 0, sizeof(demo_t));
}

void demo_fini(demo_t *demo)
{
    demo_node_t *n, *next_def;
    n = demo->head_123;
    for (; n; n = next_def) {
        next_def = n->next_def;
        free(n);
    }
}

demo_t *demo_create(void)
{
    demo_t *demo = malloc(sizeof(demo_t));
    demo_init(demo);
    return (demo);
}

void demo_free(demo_t * demo)
{
    demo_fini(demo);
    free(demo);
}

/* 把节点node插到before前 */
void demo_attach_before(demo_t * demo, demo_node_t * n, demo_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(demo->head_123, demo->tail_456, n, prev_abc, next_def, before);
    demo->len++;
}

/* 移除节点 n, 并没有释放n的资源 */
void demo_detach(demo_t * demo, demo_node_t * n)
{
    ZMLINK_DETACH(demo->head_123, demo->tail_456, n, prev_abc, next_def);
    demo->len--;
}

/* 创建一个值为value的节点,插到before前, 并返回 */
demo_node_t *demo_add_before(demo_t * demo, const void *value, demo_node_t * before)
{
    demo_node_t *n;

    n = (demo_node_t *) calloc(1, sizeof(demo_node_t));
    n->value = (void *)value;
    demo_attach_before(demo, n, before);

    return n;
}

/* 删除节点n, 把n的值赋值给*value, 释放n的资源, 如果n==0返回0, 否则返回1 */
int demo_delete(demo_t * demo, demo_node_t * n, void **value)
{
    if (n == 0) {
        return 0;
    }
    if (value) {
        *value = n->value;
    }
    demo_detach(demo, n);
    free(n);

    return 1;
}

/* 创建值为v的节点, 追加到链表尾部, 并返回 */
demo_node_t *demo_push(demo_t *l,const void *v)
{
    return demo_add_before(l,v,0);
}

/* 创建值为v的节点, 追加到链表首部部, 并返回 */
demo_node_t *demo_unshift(demo_t *l,const void *v)
{ 
    return demo_add_before(l,v,l->head_123);
}

/* 弹出尾部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
int demo_pop(demo_t *l, void **v)
{
    return demo_delete(l,l->tail_456,v);
}

/* 弹出首部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
int demo_shift(demo_t *l, void **v)
{
    return demo_delete(l,l->head_123,v);
}

int main(int argc, char **argv)
{
    typedef struct my_struct my_struct;
    struct my_struct {
        int a;
        char b[12];
    };

    my_struct *ms;

    demo_t *demo = demo_create();

    ms = (my_struct *)malloc(sizeof(my_struct));
    ms->a = 1;
    demo_push(demo, ms);

    ms = (my_struct *)malloc(sizeof(my_struct));
    ms->a = 2;
    demo_push(demo, ms);

    ms = (my_struct *)malloc(sizeof(my_struct));
    ms->a = 3;
    demo_unshift(demo, ms);

    if (demo_pop(demo, (void **)&ms)) {
        printf("pop value:%d\n", ms->a);
        free(ms);
    }

    if (demo_shift(demo, (void **)&ms)) {
        printf("shift value:%d\n", ms->a);
        free(ms);
    }

    demo_pop(demo, 0); /* 这里 丢失一段 内存  my_struct */

    demo_shift(demo, 0);

    demo_free(demo);

    return 0;
}
