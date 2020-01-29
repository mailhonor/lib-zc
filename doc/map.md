# map映射

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

**zmap_t** 是一个map(映射)的封装, 基于rbtree实现, 是LIB-ZC基本数据结构

rbtree是一个数据结构, 复制自: [Linux内核源码](https://github.com/torvalds/linux/blob/master/lib/rbtree.c)

LIB-ZC对rbtree的实现做了整理:[https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/rbtree.c](https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/rbtree.c)

## 数据结构

```
struct zmap_t {
    zrbtree_t rbtree;
    int len;
};
struct zmap_node_t {
    char *key; /* 键 */ 
    void *value; /* 值 */
    zrbtree_node_t rbnode;
} __attribute__ ((aligned(8)));
```

## 函数介绍

### 创建map

```
/* 创建 */
zmap_t *zmap_create(void);

/* 释放 */
void zmap_free(zmap_t *map);

/* 重置 */
void zdict_reset(zdict_t *dict);
```

### 增加节点

```
/* 新增或更新节点并返回 */
/* 这个节点的键为key, 新值为value, 如果旧值存在则赋值给 *old_value */
zmap_node_t *zmap_update(zmap_t *map, const char *key, const void *value, void **old_value);
```

### 查找节点

```
/* 查找键为key的节点,并返回. 如果存在则节点的值赋值给 *value */
zmap_node_t *zmap_find(const zmap_t *map, const char *key, void **value);

/* 查找键值小于key且最接近key的节点, 并... */
zmap_node_t *zmap_find_near_prev(const zmap_t *map, const char *key, void **value);

/* 查找键值大于key且最接近key的节点, 并... */
zmap_node_t *zmap_find_near_next(const zmap_t *map, const char *key, void **value);

/* 第一个 */
zmap_node_t *zmap_first(const zmap_t *map);

/* 最后一个 */
zmap_node_t *zmap_last(const zmap_t *map);

/* 前一个 */
zmap_node_t *zmap_prev(const zmap_node_t *node);

/* 后一个 */
zmap_node_t *zmap_next(const zmap_node_t *node);
```

### 删除节点

```
/* 删除并释放键为key的节点, 节点的值赋值给 *old_value */
zbool_t zmap_delete(zmap_t * map, const char *key, void **old_value);

/* 删除并释放键节点n, 节点的值赋值给 *old_value */
void zmap_delete_node(zmap_t *map, zmap_node_t *n, void **old_value);

```

### 节点个数 

```
#define zmap_len(map)                 ((map)->len)
```

### 节点的键

```
#define zmap_node_key(n)              ((n)->key)
```

### 节点的值

```
#define zmap_node_value(n)            ((n)->value)
```

### 宏, 遍历1

```
#define ZMAP_NODE_WALK_BEGIN(map, var_your_node)
#define ZMAP_NODE_WALK_END
```

### 宏, 遍历2

```
 #define ZMAP_WALK_BEGIN(map, var_your_key, var_your_value_type, var_your_value)
#define ZMAP_WALK_END
```

## 例子

```
#include "zc.h"
typedef struct mystruct mystruct;
struct mystruct {
    int a;
    int b;
};
int main(int argc, char **argv)
{
    zmap_t *map = zmap_create();
    mystruct *ms;

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 1;
    zmap_update(map, "key1", ms, 0); 

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 2;
    zmap_update(map, "key2", ms, 0); 

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 3;
    zmap_update(map, "key3", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 5;
    /* 因为 key2 已经存在, 但第四个参数为 0, 所以 ms->a == 2的 mystruct 内存泄露了 */
    zmap_update(map, "key2", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 6;
    mystruct *ms2;
    /* 尽管 key3 已经存在, 但第四个参数为ms2, 所以 ms->a == 3的 mystruct 赋值给了 ms2  */
    zmap_update(map, "key3", ms, (void **)&ms2);
    if (ms2) {
        free(ms2);
    }

    ZMAP_WALK_BEGIN(map, key, mystruct *, ms6) {
        printf("key=%s, a=%d\n", key, ms6->a);
        free(ms6);
    } ZMAP_WALK_END;

    zmap_free(map);
    return 0;
}

```
