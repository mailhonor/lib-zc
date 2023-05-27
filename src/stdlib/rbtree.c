/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-09-28
 * ================================
 */

/*
    Red Black Trees
    (C) 1999  Andrea Arcangeli <andrea@suse.de>
    (C) 2002  David Woodhouse <dwmw2@infradead.org>
    (C) 2012  Michel Lespinasse <walken@google.com>
    linux kernel 3.10.9  lib/rbtree.c
*/

#include "zc.h"
#include <sys/types.h>

#define zinline inline __attribute__((always_inline))

void __zrbtree_insert_augmented(zrbtree_t * root, zrbtree_node_t * node, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node));
void __zrbtree_erase_color(zrbtree_t * root, zrbtree_node_t * parent, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node));

#define RB_RED          0
#define RB_BLACK        1
#define true    1
#define false   0

#define RB_ROOT    (zrbtree_t) { NULL, }
#define    zrbtree_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)  ((root)->zrbtree_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbree */
#define RB_EMPTY_NODE(node)  \
    ((node)->__zrbtree_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node)  \
    ((node)->__zrbtree_parent_color = (unsigned long)(node))

struct zrbtree_augment_callbacks {
    void (*propagate) (zrbtree_node_t * node, zrbtree_node_t * stop);
    void (*copy) (zrbtree_node_t * old, zrbtree_node_t * new_node);
    void (*rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node);
};

zinline static void zrbtree_insert_augmented(zrbtree_node_t * node, zrbtree_t * root, const struct zrbtree_augment_callbacks *augment)
{
    __zrbtree_insert_augmented(root, node, augment->rotate);
}

#define RB_DECLARE_CALLBACKS(rbstatic, rbname, rbstruct, rbfield,    \
                 rbtype, rbaugmented, rbcompute)        \
zinline static void                            \
rbname ## _propagate(zrbtree_node_t *rb, zrbtree_node_t *stop)        \
{                                    \
    while (rb != stop) {                        \
        rbstruct *node = zrbtree_entry(rb, rbstruct, rbfield);    \
        rbtype augmented = rbcompute(node);            \
        if (node->rbaugmented == augmented)            \
            break;                        \
        node->rbaugmented = augmented;                \
        rb = ZRBTREE_PARENT(&node->rbfield);                \
    }                                \
}                                    \
zinline static void                            \
rbname ## _copy(zrbtree_node_t *zrbtree_old, zrbtree_node_t *zrbtree_new)        \
{                                    \
    rbstruct *old = zrbtree_entry(zrbtree_old, rbstruct, rbfield);        \
    rbstruct *new_node = zrbtree_entry(zrbtree_new, rbstruct, rbfield);        \
    new_node->rbaugmented = old->rbaugmented;                \
}                                    \
static void                                \
rbname ## _rotate(zrbtree_node_t *zrbtree_old, zrbtree_node_t *zrbtree_new)    \
{                                    \
    rbstruct *old = zrbtree_entry(zrbtree_old, rbstruct, rbfield);        \
    rbstruct *new_node = zrbtree_entry(zrbtree_new, rbstruct, rbfield);        \
    new_node->rbaugmented = old->rbaugmented;                \
    old->rbaugmented = rbcompute(old);                \
}                                    \
rbstatic const struct zrbtree_augment_callbacks rbname = {            \
    rbname ## _propagate, rbname ## _copy, rbname ## _rotate    \
};

#define    RB_RED        0
#define    RB_BLACK    1

#define __ZRBTREE_PARENT(pc)    ((zrbtree_node_t *)(pc & ~3))

#define __zrbtree_color(pc)     ((pc) & 1)
#define __zrbtree_is_black(pc)  __zrbtree_color(pc)
#define __zrbtree_is_red(pc)    (!__zrbtree_color(pc))
#define zrbtree_color(rb)       __zrbtree_color((rb)->__zrbtree_parent_color)
#define zrbtree_is_red(rb)      __zrbtree_is_red((rb)->__zrbtree_parent_color)
#define zrbtree_is_black(rb)    __zrbtree_is_black((rb)->__zrbtree_parent_color)

zinline static void zrbtree_set_parent(zrbtree_node_t * rb, zrbtree_node_t * p)
{
    rb->__zrbtree_parent_color = zrbtree_color(rb) | (unsigned long)p;
}

zinline static void zrbtree_set_parent_color(zrbtree_node_t * rb, zrbtree_node_t * p, int color)
{
    rb->__zrbtree_parent_color = (unsigned long)p | color;
}

zinline static void __zrbtree_change_child(zrbtree_node_t * old, zrbtree_node_t * new_node, zrbtree_node_t * parent, zrbtree_t * root)
{
    if (parent) {
        if (parent->zrbtree_left == old)
            parent->zrbtree_left = new_node;
        else
            parent->zrbtree_right = new_node;
    } else
        root->zrbtree_node = new_node;
}

void __zrbtree_erase_color(zrbtree_t * root, zrbtree_node_t * parent, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node));

static __always_inline zrbtree_node_t *__zrbtree_erase_augmented(zrbtree_node_t * node, zrbtree_t * root, const struct zrbtree_augment_callbacks *augment)
{
    zrbtree_node_t *child = node->zrbtree_right, *tmp = node->zrbtree_left;
    zrbtree_node_t *parent, *rebalance;
    unsigned long pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __zrbtree_erase_color() later on.
         */
        pc = node->__zrbtree_parent_color;
        parent = __ZRBTREE_PARENT(pc);
        __zrbtree_change_child(node, child, parent, root);
        if (child) {
            child->__zrbtree_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = __zrbtree_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->zrbtree_left */
        tmp->__zrbtree_parent_color = pc = node->__zrbtree_parent_color;
        parent = __ZRBTREE_PARENT(pc);
        __zrbtree_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        zrbtree_node_t *successor = child, *child2;
        tmp = child->zrbtree_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->zrbtree_right;
            augment->copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->zrbtree_left;
            }
            while (tmp);
            parent->zrbtree_left = child2 = successor->zrbtree_right;
            successor->zrbtree_right = child;
            zrbtree_set_parent(child, successor);
            augment->copy(node, successor);
            augment->propagate(parent, successor);
        }

        successor->zrbtree_left = tmp = node->zrbtree_left;
        zrbtree_set_parent(tmp, successor);

        pc = node->__zrbtree_parent_color;
        tmp = __ZRBTREE_PARENT(pc);
        __zrbtree_change_child(node, successor, tmp, root);
        if (child2) {
            successor->__zrbtree_parent_color = pc;
            zrbtree_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2 = successor->__zrbtree_parent_color;
            successor->__zrbtree_parent_color = pc;
            rebalance = __zrbtree_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    augment->propagate(tmp, NULL);
    return rebalance;
}

static __always_inline void zrbtree_erase_augmented(zrbtree_node_t * node, zrbtree_t * root, const struct zrbtree_augment_callbacks *augment)
{
    zrbtree_node_t *rebalance = __zrbtree_erase_augmented(node, root, augment);
    if (rebalance)
        __zrbtree_erase_color(root, rebalance, augment->rotate);
}

zinline static void zrbtree_set_black(zrbtree_node_t * rb)
{
    rb->__zrbtree_parent_color |= RB_BLACK;
}

zinline static zrbtree_node_t *zrbtree_red_parent(zrbtree_node_t * red)
{
    return (zrbtree_node_t *) red->__zrbtree_parent_color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new_node
 * - old gets assigned new_node as a parent and 'color' as a color.
 */
zinline static void __zrbtree_rotate_set_parents(zrbtree_node_t * old, zrbtree_node_t * new_node, zrbtree_t * root, int color)
{
    zrbtree_node_t *parent = ZRBTREE_PARENT(old);
    new_node->__zrbtree_parent_color = old->__zrbtree_parent_color;
    zrbtree_set_parent_color(old, new_node, color);
    __zrbtree_change_child(old, new_node, parent, root);
}

zinline static void __zrbtree_insert(zrbtree_node_t * node, zrbtree_t * root, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node))
{
    zrbtree_node_t *parent = zrbtree_red_parent(node), *gparent, *tmp;

    while (true) {
        /*
         * Loop invariant: node is red
         *
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as we don't
         * want a red root or two consecutive red nodes.
         */
        if (!parent) {
            zrbtree_set_parent_color(node, NULL, RB_BLACK);
            break;
        } else if (zrbtree_is_black(parent))
            break;

        gparent = zrbtree_red_parent(parent);

        tmp = gparent->zrbtree_right;
        if (parent != tmp) {    /* parent == gparent->zrbtree_left */
            if (tmp && zrbtree_is_red(tmp)) {
                /*
                 * Case 1 - color flips
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            N
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                zrbtree_set_parent_color(tmp, gparent, RB_BLACK);
                zrbtree_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = ZRBTREE_PARENT(node);
                zrbtree_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->zrbtree_right;
            if (node == tmp) {
                /*
                 * Case 2 - left rotate at parent
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                parent->zrbtree_right = tmp = node->zrbtree_left;
                node->zrbtree_left = parent;
                if (tmp)
                    zrbtree_set_parent_color(tmp, parent, RB_BLACK);
                zrbtree_set_parent_color(parent, node, RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->zrbtree_right;
            }

            /*
             * Case 3 - right rotate at gparent
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->zrbtree_left = tmp;    /* == parent->zrbtree_right */
            parent->zrbtree_right = gparent;
            if (tmp)
                zrbtree_set_parent_color(tmp, gparent, RB_BLACK);
            __zrbtree_rotate_set_parents(gparent, parent, root, RB_RED);
            augment_rotate(gparent, parent);
            break;
        } else {
            tmp = gparent->zrbtree_left;
            if (tmp && zrbtree_is_red(tmp)) {
                /* Case 1 - color flips */
                zrbtree_set_parent_color(tmp, gparent, RB_BLACK);
                zrbtree_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = ZRBTREE_PARENT(node);
                zrbtree_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->zrbtree_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                parent->zrbtree_left = tmp = node->zrbtree_right;
                node->zrbtree_right = parent;
                if (tmp)
                    zrbtree_set_parent_color(tmp, parent, RB_BLACK);
                zrbtree_set_parent_color(parent, node, RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->zrbtree_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->zrbtree_right = tmp;   /* == parent->zrbtree_left */
            parent->zrbtree_left = gparent;
            if (tmp)
                zrbtree_set_parent_color(tmp, gparent, RB_BLACK);
            __zrbtree_rotate_set_parents(gparent, parent, root, RB_RED);
            augment_rotate(gparent, parent);
            break;
        }
    }
}

/*
 * Inline version for zrbtree_erase() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
zinline static void ____zrbtree_erase_color(zrbtree_node_t * parent, zrbtree_t * root, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node))
{
    zrbtree_node_t *node = NULL, *sibling, *tmp1, *tmp2;

    while (true) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->zrbtree_right;
        if (node != sibling) {  /* node == parent->zrbtree_left */
            if (zrbtree_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                parent->zrbtree_right = tmp1 = sibling->zrbtree_left;
                sibling->zrbtree_left = parent;
                zrbtree_set_parent_color(tmp1, parent, RB_BLACK);
                __zrbtree_rotate_set_parents(parent, sibling, root, RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->zrbtree_right;
            if (!tmp1 || zrbtree_is_black(tmp1)) {
                tmp2 = sibling->zrbtree_left;
                if (!tmp2 || zrbtree_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    zrbtree_set_parent_color(sibling, parent, RB_RED);
                    if (zrbtree_is_red(parent))
                        zrbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZRBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   Sl
                 *     / \             \
                 *    sl  Sr            s
                 *                       \
                 *                        Sr
                 */
                sibling->zrbtree_left = tmp1 = tmp2->zrbtree_right;
                tmp2->zrbtree_right = sibling;
                parent->zrbtree_right = tmp2;
                if (tmp1)
                    zrbtree_set_parent_color(tmp1, sibling, RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            parent->zrbtree_right = tmp2 = sibling->zrbtree_left;
            sibling->zrbtree_left = parent;
            zrbtree_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                zrbtree_set_parent(tmp2, parent);
            __zrbtree_rotate_set_parents(parent, sibling, root, RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        } else {
            sibling = parent->zrbtree_left;
            if (zrbtree_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                parent->zrbtree_left = tmp1 = sibling->zrbtree_right;
                sibling->zrbtree_right = parent;
                zrbtree_set_parent_color(tmp1, parent, RB_BLACK);
                __zrbtree_rotate_set_parents(parent, sibling, root, RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->zrbtree_left;
            if (!tmp1 || zrbtree_is_black(tmp1)) {
                tmp2 = sibling->zrbtree_right;
                if (!tmp2 || zrbtree_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    zrbtree_set_parent_color(sibling, parent, RB_RED);
                    if (zrbtree_is_red(parent))
                        zrbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZRBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - right rotate at sibling */
                sibling->zrbtree_right = tmp1 = tmp2->zrbtree_left;
                tmp2->zrbtree_left = sibling;
                parent->zrbtree_left = tmp2;
                if (tmp1)
                    zrbtree_set_parent_color(tmp1, sibling, RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - left rotate at parent + color flips */
            parent->zrbtree_left = tmp2 = sibling->zrbtree_right;
            sibling->zrbtree_right = parent;
            zrbtree_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                zrbtree_set_parent(tmp2, parent);
            __zrbtree_rotate_set_parents(parent, sibling, root, RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        }
    }
}

/* Non-inline version for zrbtree_erase_augmented() use */
void __zrbtree_erase_color(zrbtree_t * root, zrbtree_node_t * parent, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node))
{
    ____zrbtree_erase_color(parent, root, augment_rotate);
}

/*
 * Non-augmented rbtree manipulation functions.
 *
 * We use dummy augmented callbacks here, and have the compiler optimize them
 * out of the zrbtree_insert_color() and zrbtree_erase() function definitions.
 */

zinline static void dummy_propagate(zrbtree_node_t * node, zrbtree_node_t * stop)
{
}

zinline static void dummy_copy(zrbtree_node_t * old, zrbtree_node_t * new_node)
{
}

zinline static void dummy_rotate(zrbtree_node_t * old, zrbtree_node_t * new_node)
{
}

static const struct zrbtree_augment_callbacks dummy_callbacks = {
    dummy_propagate, dummy_copy, dummy_rotate
};

void zrbtree_insert_color(zrbtree_t * root, zrbtree_node_t * node)
{
    __zrbtree_insert(node, root, dummy_rotate);
}

void zrbtree_erase(zrbtree_t * root, zrbtree_node_t * node)
{
    zrbtree_node_t *rebalance;
    rebalance = __zrbtree_erase_augmented(node, root, &dummy_callbacks);
    if (rebalance)
        ____zrbtree_erase_color(rebalance, root, dummy_rotate);
}

/*
 * Augmented rbtree manipulation functions.
 *
 * This instantiates the same __always_inline functions as in the non-augmented
 * case, but this time with user-defined callbacks.
 */

void __zrbtree_insert_augmented(zrbtree_t * root, zrbtree_node_t * node, void (*augment_rotate) (zrbtree_node_t * old, zrbtree_node_t * new_node))
{
    __zrbtree_insert(node, root, augment_rotate);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
zrbtree_node_t *zrbtree_first(const zrbtree_t * root)
{
    zrbtree_node_t *n;

    n = root->zrbtree_node;
    if (!n)
        return NULL;
    while (n->zrbtree_left)
        n = n->zrbtree_left;
    return n;
}

zrbtree_node_t *zrbtree_last(const zrbtree_t * root)
{
    zrbtree_node_t *n;

    n = root->zrbtree_node;
    if (!n)
        return NULL;
    while (n->zrbtree_right)
        n = n->zrbtree_right;
    return n;
}

zrbtree_node_t *zrbtree_next(const zrbtree_node_t * node)
{
    zrbtree_node_t *parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->zrbtree_right) {
        node = node->zrbtree_right;
        while (node->zrbtree_left)
            node = node->zrbtree_left;
        return (zrbtree_node_t *) node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = ZRBTREE_PARENT(node)) && node == parent->zrbtree_right)
        node = parent;

    return parent;
}

zrbtree_node_t *zrbtree_prev(const zrbtree_node_t * node)
{
    zrbtree_node_t *parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->zrbtree_left) {
        node = node->zrbtree_left;
        while (node->zrbtree_right)
            node = node->zrbtree_right;
        return (zrbtree_node_t *) node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = ZRBTREE_PARENT(node)) && node == parent->zrbtree_left)
        node = parent;

    return parent;
}

void zrbtree_replace_node(zrbtree_t * root, zrbtree_node_t * victim, zrbtree_node_t * new_node)
{
    zrbtree_node_t *parent = ZRBTREE_PARENT(victim);

    /* Set the surrounding nodes to point to the replacement */
    __zrbtree_change_child(victim, new_node, parent, root);
    if (victim->zrbtree_left)
        zrbtree_set_parent(victim->zrbtree_left, new_node);
    if (victim->zrbtree_right)
        zrbtree_set_parent(victim->zrbtree_right, new_node);

    /* Copy the pointers/colour from the victim to the replacement */
    *new_node = *victim;
}

/* XXX added by zc*/

void zrbtree_init(zrbtree_t * tree, zrbtree_cmp_t cmp_fn)
{
    tree->zrbtree_node = 0;
    tree->cmp_fn = cmp_fn;
}

zrbtree_node_t *zrbtree_parent(const zrbtree_node_t * node)
{
    return ((zrbtree_node_t *) ((node)->__zrbtree_parent_color & ~3));
}

zrbtree_node_t *zrbtree_attach(zrbtree_t * tree, zrbtree_node_t * node)
{
    zrbtree_node_t **new_node = &(tree->zrbtree_node), *parent = 0;
    int cmp_result;

    while (*new_node) {
        parent = *new_node;
        cmp_result = tree->cmp_fn(node, *new_node);
        if (cmp_result < 0) {
            new_node = &((*new_node)->zrbtree_left);
        } else if (cmp_result > 0) {
            new_node = &((*new_node)->zrbtree_right);
        } else {
            return (*new_node);
        }
    }
    zrbtree_link_node(node, parent, new_node);
    zrbtree_insert_color(tree, node);

    return node;
}

zrbtree_node_t *zrbtree_find(const zrbtree_t * tree, zrbtree_node_t * vnode)
{
    zrbtree_node_t *node;
    int cmp_result;

    node = tree->zrbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result < 0) {
            node = node->zrbtree_left;
        } else if (cmp_result > 0) {
            node = node->zrbtree_right;
        } else {
            return node;
        }
    }

    return 0;
}

zrbtree_node_t *zrbtree_detach(zrbtree_t * tree, zrbtree_node_t * node)
{
    zrbtree_erase(tree, node);
    return node;
}

zrbtree_node_t *zrbtree_remove(zrbtree_t * tree, zrbtree_node_t * vnode)
{
    zrbtree_node_t *node;

    node = zrbtree_find(tree, vnode);
    if (node) {
        zrbtree_erase(tree, node);
    }

    return node;
}

void zrbtree_link_node(zrbtree_node_t * node, zrbtree_node_t * parent, zrbtree_node_t ** zrbtree_link)
{
    node->__zrbtree_parent_color = (unsigned long)parent;
    node->zrbtree_left = node->zrbtree_right = 0;

    *zrbtree_link = node;
}

zrbtree_node_t *zrbtree_near_prev(const zrbtree_t * tree, zrbtree_node_t * vnode)
{
    zrbtree_node_t *node, *ret_node;
    int cmp_result;

    ret_node = 0;
    node = tree->zrbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result < 0) {
            node = node->zrbtree_left;
        } else if (cmp_result > 0) {
            ret_node = node;
            node = node->zrbtree_right;
        } else {
            return node;
            return zrbtree_prev(node);
        }
    }
    return ret_node;
}

zrbtree_node_t *zrbtree_near_next(const zrbtree_t * tree, zrbtree_node_t * vnode)
{
    zrbtree_node_t *node, *ret_node;
    int cmp_result;

    ret_node = 0;
    node = tree->zrbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result > 0) {
            ret_node = node;
            node = node->zrbtree_right;
        } else if (cmp_result < 0) {
            node = node->zrbtree_left;
        } else {
            return node;
            return zrbtree_next(node);
        }
    }
    return ret_node;
}

void zrbtree_walk(zrbtree_t * tree, void (*walk_fn) (zrbtree_node_t *, void *), void *ctx)
{
    typedef struct {
        zrbtree_node_t *node;
        unsigned char lrs;
    } _NL;
    zrbtree_node_t *node;
    _NL list[64];
    int idx, lrs;

    if (!walk_fn) {
        return;
    }
    idx = 0;
    node = tree->zrbtree_node;
    list[idx].node = node;
    list[idx].lrs = 0;
    while (1) {
        node = list[idx].node;
        lrs = list[idx].lrs;
        if (!node || lrs == 2) {
            if (node) {
                (*walk_fn) (node, ctx);
            }
            idx--;
            if (idx == -1) {
                break;
            }
            list[idx].lrs++;
            continue;
        }
        idx++;
        list[idx].lrs = 0;
        list[idx].node = ((lrs == 0) ? node->zrbtree_left : node->zrbtree_right);
    }
}
