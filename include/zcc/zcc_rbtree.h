/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_RBTREE___
#define ZCC_LIB_INCLUDE_RBTREE___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

/* rbtree ########################################################### */
struct rbtree_t;
struct rbtree_node_t;
typedef int (*rbtree_cmp_t)(rbtree_node_t *node1, rbtree_node_t *node2);
struct rbtree_t
{
    rbtree_node_t *rbtree_node;
    rbtree_cmp_t cmp_fn;
};

struct rbtree_node_t
{
    int64_t __rbtree_parent_color;
    rbtree_node_t *rbtree_right;
    rbtree_node_t *rbtree_left;
    /* The alignment might seem pointless, but allegedly CRIS needs it */
}
#ifdef __linux__
__attribute__((aligned(sizeof(int64_t))))
#endif // __linux__
;

#define ZCC_RBTREE_HAVE_DATA(tree) ((tree)->rbtree_node ? 1 : 0)
inline int rbtree_have_data(rbtree_t *tree)
{
    return ZCC_RBTREE_HAVE_DATA(tree);
}
void rbtree_init(rbtree_t *tree, rbtree_cmp_t cmp_fn);
void rbtree_insert_color(rbtree_t *, rbtree_node_t *);
void rbtree_erase(rbtree_t *tree, rbtree_node_t *node);
void rbtree_replace_node(rbtree_t *tree, rbtree_node_t *victim, rbtree_node_t *_new);
rbtree_node_t *rbtree_prev(const rbtree_node_t *tree);
rbtree_node_t *rbtree_next(const rbtree_node_t *tree);
rbtree_node_t *rbtree_first(const rbtree_t *node);
rbtree_node_t *rbtree_last(const rbtree_t *node);
rbtree_node_t *rbtree_near_prev(const rbtree_t *tree, rbtree_node_t *vnode);
rbtree_node_t *rbtree_near_next(const rbtree_t *tree, rbtree_node_t *vnode);
rbtree_node_t *rbtree_parent(const rbtree_node_t *node);
rbtree_node_t *rbtree_attach(rbtree_t *tree, rbtree_node_t *node);
rbtree_node_t *rbtree_find(const rbtree_t *tree, rbtree_node_t *vnode);
rbtree_node_t *rbtree_detach(rbtree_t *tree, rbtree_node_t *node);
void rbtree_link_node(rbtree_node_t *node, rbtree_node_t *parent, rbtree_node_t **rbtree_link);

#define ZCC_RBTREE_INIT(tree, _cmp_fn) ((tree)->rbtree_node = 0, (tree)->cmp_fn = _cmp_fn)
#define ZCC_RBTREE_PARENT(node) ((rbtree_node_t *)((node)->__rbtree_parent_color & ~3))
#define ZCC_RBTREE_ATTACH_PART1(root, node, cmp_node)                           \
    {                                                                           \
        rbtree_node_t **___Z_new_pp = &((root)->rbtree_node), *___Z_parent = 0; \
        while (*___Z_new_pp)                                                    \
        {                                                                       \
            ___Z_parent = *___Z_new_pp;                                         \
            cmp_node = *___Z_new_pp;                                            \
            {
#define ZCC_RBTREE_ATTACH_PART2(root, node, cmp_result, return_node) \
    }                                                                \
    return_node = 0;                                                 \
    if (cmp_result < 0)                                              \
    {                                                                \
        ___Z_new_pp = &((*___Z_new_pp)->rbtree_left);                \
    }                                                                \
    else if (cmp_result > 0)                                         \
    {                                                                \
        ___Z_new_pp = &((*___Z_new_pp)->rbtree_right);               \
    }                                                                \
    else                                                             \
    {                                                                \
        return_node = *___Z_new_pp;                                  \
        break;                                                       \
    }                                                                \
    }                                                                \
    if (!return_node)                                                \
    {                                                                \
        rbtree_link_node(node, ___Z_parent, ___Z_new_pp);            \
        rbtree_insert_color(root, node);                             \
        return_node = node;                                          \
    }                                                                \
    }

#define ZCC_RBTREE_LOOKUP_PART1(root, cmp_node)             \
    {                                                       \
        rbtree_node_t *___Z_node_tmp = (root)->rbtree_node; \
        while (___Z_node_tmp)                               \
        {                                                   \
            cmp_node = ___Z_node_tmp;                       \
            {
#define ZCC_RBTREE_LOOKUP_PART2(root, cmp_result, return_node) \
    }                                                          \
    return_node = 0;                                           \
    if (cmp_result < 0)                                        \
    {                                                          \
        ___Z_node_tmp = ___Z_node_tmp->rbtree_left;            \
    }                                                          \
    else if (cmp_result > 0)                                   \
    {                                                          \
        ___Z_node_tmp = ___Z_node_tmp->rbtree_right;           \
    }                                                          \
    else                                                       \
    {                                                          \
        return_node = ___Z_node_tmp;                           \
        break;                                                 \
    }                                                          \
    }                                                          \
    }

#define ZCC_RBTREE_DETACH(root, node) \
    {                                 \
        rbtree_erase(root, node);     \
    }

#define ZCC_RBTREE_WALK_BEGIN(root, var_your_node)                               \
    {                                                                            \
        struct                                                                   \
        {                                                                        \
            rbtree_node_t *node;                                                 \
            unsigned char lrs;                                                   \
        } ___Z_list[64];                                                         \
        rbtree_node_t *___Z_node = (root)->rbtree_node;                          \
        int ___Z_idx = 0, ___Z_lrs;                                              \
        ___Z_list[0].node = ___Z_node;                                           \
        ___Z_list[0].lrs = 0;                                                    \
        while (1)                                                                \
        {                                                                        \
            rbtree_node_t *var_your_node = ___Z_node = ___Z_list[___Z_idx].node; \
            ___Z_lrs = ___Z_list[___Z_idx].lrs;                                  \
            if (!___Z_node || ___Z_lrs == 2)                                     \
            {                                                                    \
                if (___Z_node)                                                   \
                {
#define ZCC_RBTREE_WALK_END                                                                          \
    }                                                                                                \
    ___Z_idx--;                                                                                      \
    if (___Z_idx == -1)                                                                              \
    {                                                                                                \
        break;                                                                                       \
    }                                                                                                \
    ___Z_list[___Z_idx].lrs++;                                                                       \
    continue;                                                                                        \
    }                                                                                                \
    ___Z_idx++;                                                                                      \
    ___Z_list[___Z_idx].lrs = 0;                                                                     \
    ___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->rbtree_left : ___Z_node->rbtree_right); \
    }                                                                                                \
    }

#define ZCC_RBTREE_WALK_FORWARD_BEGIN(root, var_your_node)                                                  \
    {                                                                                                       \
        rbtree_node_t *var_your_node;                                                                       \
        for (var_your_node = rbtree_first(root); var_your_node; var_your_node = rbtree_next(var_your_node)) \
        {
#define ZCC_RBTREE_WALK_FORWARD_END \
    }                               \
    }

#define ZCC_RBTREE_WALK_BACK_BEGIN(root, var_your_node)                                                    \
    {                                                                                                      \
        rbtree_node_t *var_your_node;                                                                      \
        for (var_your_node = rbtree_last(root); var_your_node; var_your_node = rbtree_prev(var_your_node)) \
        {
#define ZCC_RBTREE_WALK_BACK_END \
    }                            \
    }

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_RBTREE___
