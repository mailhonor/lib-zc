
## 双向链表, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持双向链表,
其 STRUCT 类型是 **zlist_t**,
是 [LIB-ZC](./README.md) 的基本数据结构

## 数据结构

```
struct zlist_t {
    zlist_node_t *head;
    zlist_node_t *tail;
    int len; 
};
struct zlist_node_t {
    zlist_node_t *prev;
    zlist_node_t *next;
    void *value;
};
```

## 函数: 基本操作

### zlist_t *zlist_create(void);

* 创建链表

### void zlist_free(zlist_t *list);

* 释放

### void zlist_reset(zlist_t *list);

* 重置

### int zlist_len(zlist_t *list);

* 宏, 节点个数


## 函数: 增加节点

### void zlist_attach_before(zlist_t *list, zlist_node_t *node, zlist_node_t *before);

* 把节点 node 插到 before 前

### zlist_node_t *zlist_add_before(zlist_t *list, const void *value, zlist_node_t *before);

* 创建一个值为 value 的节点, 插到 before 前, 并返回

### zlist_node_t *zlist_push(zlist_t *list, const void *value);

* 创建值为 value 的节点, 追加到链表尾部, 并返回

### zlist_node_t *zlist_unshift(zlist_t *list, const void *value);

* 创建值为 value 的节点, 追加到链表首部, 并返回

## 函数: 删除节点

### void zlist_detach(zlist_t *list, zlist_node_t *node);

* 移除节点 node, 注意: 没有释放 node

### zbool_t zlist_delete(zlist_t *list, zlist_node_t *node, void **value);

* 删除节点 node, 把 node 的值赋值给 *value, 释放 node,
* 如果 node==0 则返回 0, 否则返回 1

### zbool_t zlist_pop(zlist_t *list, void **value);

* 弹出尾部节点, 把值赋值给 *value, 释放这个节点
* 如果存在则返回 1, 否则返回 0 

### zbool_t zlist_shift(zlist_t *list, void **value);

* 弹出首部节点, 把值赋值给*value, 释放这个节点
* 如果存在则返回 1, 否则返回 0

## 函数: 特殊节点

### zlist_node_t *zlist_head(zlist_t *list)

* 宏, 头节点

### zlist_node_t *zlist_tail(zlist_t *list)

* 宏, 尾节点

## 函数: 节点属性

### zlist_node_t *zlist_node_next(zlist_node_t *node);

* 宏, 下一个节点

### zlist_node_t *zlist_node_prev(zlist_node_t *node);

* 宏, 上一个节点

### void *zlist_node_value(zlist_node_t *node);

* 宏, 节点的值

## 函数: 遍历

<i>当然, 可以根据数据结构自己遍历</i>

### ZLIST_WALK_BEGIN(zlist_t *list, var_your_type, var_your_ptr);<BR />ZLIST_WALK_END

* 宏, 遍历

### ZLIST_NODE_WALK_BEGIN(zlist_t *list, var_your_node);<BR />ZLIST_NODE_WALK_END

* 宏, 遍历

## 例子: 1

```
#include "zc.h"

typedef struct mystruct mystruct;
struct mystruct {
    int a;
};

int main(int argc, char **argv)
{
    zlist_t *list = zlist_create();
    mystruct *ms;

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 1;
    zlist_push(list, ms);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 2;
    zlist_push(list, ms);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 8;
    zlist_unshift(list, ms);

    ZLIST_WALK_BEGIN(list, mystruct *, ptr) {
        printf("walk a: %d\n", ptr->a);
    } ZLIST_WALK_END;

    while (zlist_len(list)) {
        if (zlist_pop(list, (void **)&ms)) {
            printf("pop: %d\n", ms->a);
        }   
        free(ms);
    }   
    zlist_pop(list, (void **)&ms);
    zlist_pop(list, 0);
    
    zlist_free(list);
    return 0;
}
```

## 例子: 2

* ../blob/master/sample/stdlib/list.c

