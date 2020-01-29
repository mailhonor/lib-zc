# 词典

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

**zdict_t** 是一个C词典的封装, 基于rbtree实现, 是LIB-ZC基本数据结构

rbtree是一个数据结构, 复制自: [Linux内核源码](https://github.com/torvalds/linux/blob/master/lib/rbtree.c)

LIB-ZC对rbtree的实现做了整理:[https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/rbtree.c](https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/rbtree.c)

PS: Linux内核版rbtree的风格极其经典, **没有内存分配**. 这才是真正的封装

## 数据结构

```
struct zdict_t {
    zrbtree_t rbtree;
    int len; /* 节点个数 */
};
struct zdict_node_t {
    zrbtree_node_t rbnode;
    zbuf_t value;  /* 值 */
    char *key;  /* 键 */
} __attribute__ ((aligned(8)));
```

## 函数介绍

**创建词典** 

```
zdict_t *zdict_create(void);
/* 释放 */
void zdict_free(zdict_t *dict);
```

**重置** 

```
void zdict_reset(zdict_t *dict);
```

**增加或更新节点**

```
/* 返回此节点.  此节点键为key, 值为  value的拷贝 */
zdict_node_t *zdict_update(zdict_t *dict, const char *key, const zbuf_t *value);

/* 并返回此节点.  此节点键为key, 值为(len<0?strdup(value):strndup(value, len)) */
zdict_node_t *zdict_update_string(zdict_t *dict, const char *key, const char *value, int len);
```

**查找节点**

```
/* 查找键为key的节点, 如果存在则节点的值赋值给 *value */
zdict_node_t *zdict_find(const zdict_t *dict, const char *key, zbuf_t **value);

/* 查找键值小于key且最接近key的节点,  如果存在则节点的值赋值给 *value */
zdict_node_t *zdict_find_near_prev(const zdict_t *dict, const char *key, zbuf_t **value);

/* 查找键值大于key且最接近key的节点, ,  如果存在则节点的值赋值给 *value */
zdict_node_t *zdict_find_near_next(const zdict_t *dict, const char *key, zbuf_t **value);

/* 第一个节点 */
zdict_node_t *zdict_first(const zdict_t *dict);

/* 最后一个节点 */
zdict_node_t *zdict_last(const zdict_t *dict);

/* 前一个节点 */
zdict_node_t *zdict_prev(const zdict_node_t *node);

/* 后一个节点 */
zdict_node_t *zdict_next(const zdict_node_t *node);
```

**删除节点**

```
/* 删除并释放键为key的节点 */
void zdict_delete(zdict_t *dict, const char *key);

/* 移除节点 n */
void zdict_delete_node(zdict_t *dict, zdict_node_t *n);
```

**debug**

```
void zdict_debug_show(const zdict_t *dict);
```

**节点个数** 

```
#define zdict_len(dict)                 ((dict)->len)
```

**节点的键**

```
#define zdict_node_key(n)               ((n)->key)
```

**节点的值** 

```
#define zdict_node_value(n)             (&((n)->value))
```

**宏, 遍历1** 

```
#define ZDICT_WALK_BEGIN(dict, var_your_key, var_your_value) 
#define ZDICT_WALK_END 
```

**宏, 遍历2**

```
#define ZDICT_NODE_WALK_BEGIN(dict, var_your_node)
#define ZDICT_NODE_WALK_END
```

**查找(扩展)**

```
/* 查找键为name的节点, 如果存在则返回其值, 否则返回def */
char *zdict_get_str(const zdict_t *dict, const char *name, const char *def);

/* 查找键为name的节点, 如果存在则返回zstr_to_bool(其值), 否则返回def */
int zdict_get_bool(const zdict_t *dict, const char *name, int def);

/* 查找键为name的节点, 如果存在且{ min < foo(其值) < max }则返回foo(其值), 否则返回def; foo 为 atoi */
int zdict_get_int(const zdict_t *dict, const char *name, int def, int min, int max);

/* 如上, foo 为 atol */
long zdict_get_long(const zdict_t *dict, const char *name, long def, long min, long max);

/* 如上, foo 为 zstr_to_second */
/* 支持 s/m/h/d/w, s(second), m(minute), h(hour), d(day), w(week)
   如: 19m => 19*60, 2w => 2*7*24*3600   */
long zdict_get_second(const zdict_t *dict, const char *name, long def, long min, long max);

/* 如上, foo 为 zstr_to_size */
/* 支持 b/k/m/g, b(byte), k(1024), m(1024*1024), g(1024*1024*1024)
   如: 128K => 128*1024, 8M = > 8*1024*1024 */
long zdict_get_size(const zdict_t *dict, const char *name, long def, long min, long max);
```

## 例子

源码见 [https://gitee.com/linuxmail/lib-zc/blob/master/sample/rbtree/dict_demo.c](https://gitee.com/linuxmail/lib-zc/blob/master/sample/rbtree/dict_demo.c)

```
#include "zc.h"

int main(int argc, char **argv)
{
    zdict_t *dict = zdict_create();

    zdict_update_string(dict, "tom", "cat", -1);
    zdict_update_string(dict, "jerry", "mouse", -1);
    zdict_update_string(dict, "Donald", "duck", -1);

    printf("### show items:\n");
    zdict_debug_show(dict);


    printf("\n\n### test find:\n");

    if (zdict_find(dict, "tom", 0)) {
        printf("found tom\n");
    } else {
        printf("not found tom\n");
    }

    zbuf_t *value;
    if (zdict_find(dict, "Donald", &value)) {
        printf("found Donald=>%s\n", zbuf_data(value));
    } else {
        printf("not found Donald\n");
    }

    if (zdict_find(dict, "mike", &value)) {
        printf("found mike=>%s\n", zbuf_data(value));
    } else {
        printf("not found mike\n");
    }

    printf("\n\n## test near next find(jim):\n");
    zdict_node_t *n;
    if ((n=zdict_find_near_next(dict, "jim", 0))) {
        printf("found %s=>%s\n", zdict_node_key(n), zbuf_data(zdict_node_value(n)));
    } else {
        printf("not found near next of (jim)\n");
    }

    printf("\n\n## test near next find(jerry):\n");
    if ((n=zdict_find_near_next(dict, "jerry", 0))) {
        printf("found %s=>%s\n", zdict_node_key(n), zbuf_data(zdict_node_value(n)));
    } else {
        printf("not found near next of (jerry)\n");
    }
    printf("\n\n## test macro:\n");

    ZDICT_NODE_WALK_BEGIN(dict, rn) {
        printf("name: %s, value: %s\n", zdict_node_key(rn), zbuf_data(zdict_node_value(rn)));
    }
    ZDICT_NODE_WALK_END;

    zdict_free(dict);

    return (0);
}
```
