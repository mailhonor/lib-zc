# 链表

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

所谓"链表"是一个数据结构, 通用链表算法. **不涉及到内存分配**

## 数据结构

```
/* 链表 */
struct zlink_t {
    zlink_node_t *head;
    zlink_node_t *tail;
};
/* 节点 */
struct zlink_node_t {
    zlink_node_t *prev;
    zlink_node_t *next;
};
```

## 函数方法

### 初始化

```
/* 初始化link指向的指针 */
void zlink_init(zlink_t *link);

/* 反初始化link指向的指针 */
void zlink_fini(zlink_t *link);
```

### 添加节点

```
/* 把node插到before前 */
zlink_node_t *zlink_attach_before(zlink_t *link, zlink_node_t *node, zlink_node_t *before);

/* 把节点追加到尾部 */
zlink_node_t *zlink_push(zlink_t *link, zlink_node_t *node);

/* 把节点追加到首部 */
zlink_node_t *zlink_shift(zlink_t * link);
```

### 移除节点

```
/* 把节点node从link中移除并返回 */
zlink_node_t *zlink_detach(zlink_t *link, zlink_node_t *node);

/* 把尾部节点弹出并返回 */
zlink_node_t *zlink_pop(zlink_t *link);

/* 把首部节点弹出并返回 */
zlink_node_t *zlink_unshift(zlink_t *link, zlink_node_t *node);
```

### 定位节点

```
/* 返回首部节点 */
#define zlink_head(link)          ((link)->head)

/* 返回尾部节点 */
#define zlink_tail(link)          ((link)->tail)

/* 前一个节点 */
#define zlink_node_prev(node)     ((node)->prev)

/* 后一个节点 */
#define zlink_node_next(node)     ((node)->next)
```

### 例子

实现一个简单的列表, 功能很简单:

表尾追加, 表头移除

```
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
    if (!ln) {  return 0; }
    demo->count--;
    demo_node_t *node = ZCONTAINER_OF(ln, demo_node_t, node);
    if (value) { *value = node->value; }
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

```
