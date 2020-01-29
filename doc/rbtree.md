# zrbtree\_t 

rbtree 是数据结构, 不是容器(没有内容分配), 可实现词典,排序等, 如
[zdict\_](./dict.md), [zmap\_t](./map.md)

rbtree 是从Linux内容2.4版本复制的

---

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

#define ZRBTREE_HAVE_DATA(tree)     ((tree)->zrbtree_node?1:0)
zinline int zrbtree_have_data(zrbtree_t *tree) { return ZRBTREE_HAVE_DATA(tree); }
void zrbtree_init(zrbtree_t *tree, zrbtree_cmp_t cmp_fn);
void zrbtree_insert_color(zrbtree_t *, zrbtree_node_t *);
void zrbtree_erase(zrbtree_t *tree, zrbtree_node_t *node);
void zrbtree_replace_node(zrbtree_t *tree, zrbtree_node_t *victim, zrbtree_node_t *_new);
zrbtree_node_t *zrbtree_prev(const zrbtree_node_t *tree);
zrbtree_node_t *zrbtree_next(const zrbtree_node_t *tree);
zrbtree_node_t *zrbtree_first(const zrbtree_t *node);
zrbtree_node_t *zrbtree_last(const zrbtree_t *node);
zrbtree_node_t *zrbtree_near_prev(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_near_next(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_parent(const zrbtree_node_t *node);
zrbtree_node_t *zrbtree_attach(zrbtree_t *tree, zrbtree_node_t *node);
zrbtree_node_t *zrbtree_find(const zrbtree_t *tree, zrbtree_node_t *vnode);
zrbtree_node_t *zrbtree_detach(zrbtree_t *tree, zrbtree_node_t *node);
void zrbtree_link_node(zrbtree_node_t *node, zrbtree_node_t *parent, zrbtree_node_t **zrbtree_link);

#define ZRBTREE_INIT(tree, _cmp_fn)     ((tree)->zrbtree_node=0, (tree)->cmp_fn = _cmp_fn)
#define ZRBTREE_PARENT(node)    ((zrbtree_node_t *)((node)->__zrbtree_parent_color & ~3))
#define ZRBTREE_ATTACH_PART1(root, node, cmp_node) {                            \
    zrbtree_node_t **___Z_new_pp = &((root)->zrbtree_node), *___Z_parent = 0;            \
    while (*___Z_new_pp) {                                        \
        ___Z_parent = *___Z_new_pp;                                \
        cmp_node = *___Z_new_pp;                                \
        {
#define ZRBTREE_ATTACH_PART2(root, node, cmp_result, return_node)                     \
        }                                            \
        return_node = 0;                                    \
        if (cmp_result < 0) {                                    \
            ___Z_new_pp = &((*___Z_new_pp)->zrbtree_left);                    \
        } else if (cmp_result > 0) {                                \
            ___Z_new_pp = &((*___Z_new_pp)->zrbtree_right);                    \
        } else {                                        \
            return_node = *___Z_new_pp;                            \
            break;                                        \
        }                                            \
    }                                                \
    if(!return_node){                                        \
        zrbtree_link_node(node, ___Z_parent, ___Z_new_pp);                    \
        zrbtree_insert_color(root, node);                            \
        return_node = node;                                    \
    }                                                \
}

#define ZRBTREE_LOOKUP_PART1(root, cmp_node) {                                \
    zrbtree_node_t *___Z_node_tmp = (root)->zrbtree_node;                        \
    while (___Z_node_tmp) {                                        \
        cmp_node = ___Z_node_tmp;                                \
        {
#define ZRBTREE_LOOKUP_PART2(root, cmp_result, return_node)                        \
        }                                            \
        return_node = 0;                                    \
        if (cmp_result < 0) {                                    \
            ___Z_node_tmp = ___Z_node_tmp->zrbtree_left;                    \
        } else if (cmp_result > 0) {                                \
            ___Z_node_tmp = ___Z_node_tmp->zrbtree_right;                    \
        } else {                                        \
            return_node = ___Z_node_tmp;                            \
            break;                                        \
        }                                            \
    }                                                \
}

#define ZRBTREE_DETACH(root, node) {                                    \
    zrbtree_erase(root, node);                                    \
}

#define ZRBTREE_WALK_BEGIN(root, var_your_node) {                            \
    struct { zrbtree_node_t *node; unsigned char lrs; } ___Z_list[64];                \
    zrbtree_node_t *___Z_node = (root)->zrbtree_node;                            \
    int ___Z_idx = 0, ___Z_lrs;                                    \
    ___Z_list[0].node = ___Z_node;                                    \
    ___Z_list[0].lrs = 0;                                        \
    while (1) {                                            \
        zrbtree_node_t *var_your_node = ___Z_node = ___Z_list[___Z_idx].node;                    \
        ___Z_lrs = ___Z_list[___Z_idx].lrs;                            \
        if (!___Z_node || ___Z_lrs == 2) {                            \
            if (___Z_node) {
#define ZRBTREE_WALK_END                                        \
            }                                        \
            ___Z_idx--;                                    \
            if (___Z_idx == -1){                                \
                break;                                    \
                   }                                        \
            ___Z_list[___Z_idx].lrs++;                            \
            continue;                                    \
        }                                            \
        ___Z_idx++;                                        \
        ___Z_list[___Z_idx].lrs = 0;                                \
        ___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->zrbtree_left : ___Z_node->zrbtree_right);\
    }                                                \
}

#define ZRBTREE_WALK_FORWARD_BEGIN(root, var_your_node)     {                    \
    zrbtree_node_t *var_your_node; \
    for (var_your_node = zrbtree_first(root); var_your_node; var_your_node = zrbtree_next(var_your_node)) {
#define ZRBTREE_WALK_FORWARD_END                }}

#define ZRBTREE_WALK_BACK_BEGIN(root, var_your_node)     {                    \
    zrbtree_node_t *var_your_node; \
    for (var_your_node = zrbtree_last(root); var_your_node; var_your_node = zrbtree_prev(var_your_node)) {
#define ZRBTREE_WALK_BACK_END                }}
```

---

## 例子1
见 src/stdlib/dict.c

## 例子2
见 src/stdlib/map.c

## 例子3
见 sample/rbtree/lib_account.c
