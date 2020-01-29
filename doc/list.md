## 简介

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

zlist\_t 是 双向链表的实现, 是LIB-ZC的基本数据结构

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

## 函数

### 创建链表

```
/* 创建 */
zlist_t *zlist_create(void);

/* 释放 */
void zlist_free(zlist_t *list);

/* 重置 */
void zlist_reset(zlist_t *list);
```

### 节点操作

```
/* 把节点node插到before前 */
void zlist_attach_before(zlist_t *list, zlist_node_t *n, zlist_node_t *before);

/* 移除节点 n, 并没有释放n的资源 */
void zlist_detach(zlist_t *list, zlist_node_t *n);
```

### 新增值

```
/* 创建一个值为value的节点,插到before前, 并返回 */
zlist_node_t *zlist_add_before(zlist_t *list, const void *value, zlist_node_t *before);

/* 删除节点n, 把n的值赋值给*value, 释放n的资源, 如果n==0返回0, 否则返回1 */
zbool_t zlist_delete(zlist_t *list, zlist_node_t *n, void **value);

/* 创建值为v的节点, 追加到链表尾部, 并返回 */
zlist_node_t *zlist_push(zlist_t *l,const void *v);

/* 创建值为v的节点, 追加到链表首部部, 并返回 */
zlist_node_t *zlist_unshift(zlist_t *l,const void *v);
```

### 弹出值

```
/* 弹出尾部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
zbool_t zlist_pop(zlist_t *l, void **v);

/* 弹出首部节点, 把值赋值给*v, 释放这个节点, 如果存在则返回1, 否则返回 0 */
zbool_t zlist_shift(zlist_t *l, void **v);
```

### 宏, 遍历1

```
#define ZLIST_WALK_BEGIN(list, var_your_type, var_your_ptr)
#define ZLIST_WALK_END
```

### 宏,遍历2

```
#define ZLIST_NODE_WALK_BEGIN(list, var_your_node)
#define ZLIST_NODE_WALK_END
```

### 宏 

```
/* 头节点 */
#define zlist_head(c)   ((c)->head)
/* 尾节点 */
#define zlist_tail(c)   ((c)->tail)
/* 节点个数 */
#define zlist_len(c)    ((c)->len)
/* 下一个节点 */
#define zlist_node_next(n)   ((n)->next)
/* 上一个节点 */
#define zlist_node_prev(n)   ((n)->prev)
/* 节点的值 */
#define zlist_node_value(n)  ((n)->value)
```

## 例子

```
#include "zc.h"
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
