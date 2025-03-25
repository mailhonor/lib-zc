
## 数据结构: 红黑树(rbtree), [LIB-ZC](./README.md)


[LIB-ZC](./README.md) 支持红黑树(rbtree),
其 STRUCT 类型是 **zrbtree_t**,
是 [LIB-ZC](./README.md) 的基本数据结构

zrbtree_t 是数据结构, 不是容器, **不涉及到<s>内存分配</s>**. 基于 zrbtree_t 可实现 [词典(zdict_t)](./dict.md), [MAP](./map.md) 等

本实现是基于Linux 内核(2.4版本)复制整理而来:

https://github.com/torvalds/linux/blob/master/lib/rbtree.c

## 数据结构
```
typedef struct zrbtree_node_t zrbtree_node_t;
typedef struct zrbtree_t zrbtree_t;

typedef int (*zrbtree_cmp_t) (zrbtree_node_t *node1, zrbtree_node_t *node2);

struct zrbtree_t {
    zrbtree_node_t *zrbtree_node;
    zrbtree_cmp_t cmp_fn;
};
struct zrbtree_node_t {
    unsigned long __zrbtree_parent_color;
    zrbtree_node_t *zrbtree_right;
    zrbtree_node_t *zrbtree_left;
/* The alignment might seem pointless, but allegedly CRIS needs it */
} __attribute__ ((aligned(sizeof(long))));
```

## 函数: 基本操作

### void zrbtree_init(zrbtree_t *tree, zrbtree_cmp_t cmp_fn);

* 初始化 tree 指向的地址
* cmp_fn 是 比较函数, 此函数应该返回 &lt;0, =0 或者 &gt;0

### int zrbtree_have_data(zrbtree_t *tree);

* tree 是否有数据(节点)

## 函数: 插入节点

### zrbtree_node_t *zrbtree_attach(zrbtree_t *tree, zrbtree_node_t *node);

* 插入节点 node, 此 node 不必初始化, 分两种情况:
* 如果 tree 中存在 found_node, 使得(cmp_fn(found_node, node) == 0), 则返回 found_node, 此时 node 没有被加入 tree
* 如果 tree 中不存在 found_node, 使得(cmp_fn(found_node, node) == 0), 则返回 node, 此时 node 被加入 tree


## 函数: 移除节点

### zrbtree_node_t *zrbtree_detach(zrbtree_t *tree, zrbtree_node_t *node);

* 移除节点 node, 并返回, 并不释放 node 的资源

## 函数: 查找节点

### zrbtree_node_t *zrbtree_first(const zrbtree_t *tree);

* 第一个节点

### zrbtree_node_t *zrbtree_last(const zrbtree_t *tree);

* 最后一个节点

### zrbtree_node_t *zrbtree_find(const zrbtree_t *tree, zrbtree_node_t *vnode);

* 查找节点(found_node), 并返回, 此节点满足 (cmp_fn(found_node, vnode) == 0)

### zrbtree_node_t *zrbtree_near_prev(const zrbtree_t *tree, zrbtree_node_t *vnode);

* 查找节点, 并返回, 此节点满足小于或等于 vnode (cmp_fn上下文)且最最大(cmp_fn上下文)的节点

### zrbtree_node_t *zrbtree_near_next(const zrbtree_t *tree, zrbtree_node_t *vnode);

* 查找节点, 并返回, 此节点满足大于或等于 vnode (cmp_fn上下文)且最最小(cmp_fn上下文)的节点

## 函数: 节点属性

### zrbtree_node_t *zrbtree_prev(const zrbtree_node_t *node);

* 前一个节点

### zrbtree_node_t *zrbtree_next(const zrbtree_node_t *node);

* 后一个节点

### zrbtree_node_t *zrbtree_parent(const zrbtree_node_t *node);

* 父节点

## 函数: 遍历

### ZRBTREE_WALK_BEGIN(root, var_your_node)<BR />ZRBTREE_WALK_END

* 宏, 遍历, 速度较快

### ZRBTREE_WALK_FORWARD_BEGIN(zrbtree_t *tree, var_your_node)<BR />ZRBTREE_WALK_FORWARD_END

* 宏, 遍历, 从头到尾

### ZRBTREE_WALK_BACK_BEGIN(root, var_your_node)<BR />ZRBTREE_WALK_BACK_END

* 宏, 遍历, 从尾到头


## 例子

* ../blob/master/src/stdlib/dict.c
* ../blob/master/src/stdlib/map.c
* ../blob/master/sample/rbtree/

## 技巧

请注意, rbtree 操作本身并不涉及到内存分配

### 第一, 节点的结构

```
typedef struct some_struct_t some_struct_t;
struct some_struct_t {
    char *other_att1;
    int   other_att2;
    /* ... */
    zrbtree_node_t rbnode; /* 注意这个成员 */
    int   other_att5;
    /* ... */
};
```

### 第二, 比较函数

```
int some_struct_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    some_struct_cmp *a1, *a2;

    a1 = ZCONTAINER_OF(n1, some_struct_t, rbnode);
    a2 = ZCONTAINER_OF(n2, some_struct_t, rbnode);
    
    /* 这时, 得到了 2 个类型为 some_struct_t 的指针, 接着比较大小 */
    int cmp_ret = 0;
    do {
        /* 自己实现比较方法, 比如: */
        /* cmp_ret = strcmp(a1->other_att1, a2->other_att1); */
        /* cmp_ret = (a1->other_att2 - a2->other_att2); */
    } while(0);
    return cmp_ret;
}
```

### 第三, 得到一个 tree, 并初始化

```
zrbtree_t some_rbtree;
zrbtree_init(&some_rbtree, some_struct_cmp);
/* 或 */
zrbtree_t *some_rbtree_ptr = (zrbtree_t *)malloc(sizeof(some_rbtree));
zrbtree_init(some_rbtree_ptr, some_struct_cmp);
```

### 第四, 业务

* 分配一个节点, zrbtree_attach 到 tree
* 准备一个vnode(虚节点), 各种查找
* 移除节点
* 遍历节点, 等等

### 第五, 释放

* 移除所有节点, 并释放其资源
* 释放 tree 的资源

