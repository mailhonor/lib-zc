#include "zc.h"
/*
	Red Black Trees
	(C) 1999  Andrea Arcangeli <andrea@suse.de>
	(C) 2002  David Woodhouse <dwmw2@infradead.org>
	(C) 2012  Michel Lespinasse <walken@google.com>
	linux kernel 3.10.9  lib/rbtree.c
*/

void __zrbtree_insert_augmented(ZRBTREE * root, ZRBTREE_NODE * node, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new));
void __zrbtree_erase_color(ZRBTREE * root, ZRBTREE_NODE * parent, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new));

#define RB_RED          0
#define RB_BLACK        1
#define true    1
#define false   0

#define RB_ROOT	(ZRBTREE) { NULL, }
#define	zrbtree_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)  ((root)->zrbtree_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbree */
#define RB_EMPTY_NODE(node)  \
	((node)->__zrbtree_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node)  \
	((node)->__zrbtree_parent_color = (unsigned long)(node))

struct zrbtree_augment_callbacks {
	void (*propagate) (ZRBTREE_NODE * node, ZRBTREE_NODE * stop);
	void (*copy) (ZRBTREE_NODE * old, ZRBTREE_NODE * new);
	void (*rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new);
};

static inline void zrbtree_insert_augmented(ZRBTREE_NODE * node, ZRBTREE * root, const struct zrbtree_augment_callbacks *augment)
{
	__zrbtree_insert_augmented(root, node, augment->rotate);
}

#define RB_DECLARE_CALLBACKS(rbstatic, rbname, rbstruct, rbfield,	\
			     rbtype, rbaugmented, rbcompute)		\
static inline void							\
rbname ## _propagate(ZRBTREE_NODE *rb, ZRBTREE_NODE *stop)		\
{									\
	while (rb != stop) {						\
		rbstruct *node = zrbtree_entry(rb, rbstruct, rbfield);	\
		rbtype augmented = rbcompute(node);			\
		if (node->rbaugmented == augmented)			\
			break;						\
		node->rbaugmented = augmented;				\
		rb = ZRBTREE_PARENT(&node->rbfield);				\
	}								\
}									\
static inline void							\
rbname ## _copy(ZRBTREE_NODE *zrbtree_old, ZRBTREE_NODE *zrbtree_new)		\
{									\
	rbstruct *old = zrbtree_entry(zrbtree_old, rbstruct, rbfield);		\
	rbstruct *new = zrbtree_entry(zrbtree_new, rbstruct, rbfield);		\
	new->rbaugmented = old->rbaugmented;				\
}									\
static void								\
rbname ## _rotate(ZRBTREE_NODE *zrbtree_old, ZRBTREE_NODE *zrbtree_new)	\
{									\
	rbstruct *old = zrbtree_entry(zrbtree_old, rbstruct, rbfield);		\
	rbstruct *new = zrbtree_entry(zrbtree_new, rbstruct, rbfield);		\
	new->rbaugmented = old->rbaugmented;				\
	old->rbaugmented = rbcompute(old);				\
}									\
rbstatic const struct zrbtree_augment_callbacks rbname = {			\
	rbname ## _propagate, rbname ## _copy, rbname ## _rotate	\
};

#define	RB_RED		0
#define	RB_BLACK	1

#define __ZRBTREE_PARENT(pc)    ((ZRBTREE_NODE *)(pc & ~3))

#define __zrbtree_color(pc)     ((pc) & 1)
#define __zrbtree_is_black(pc)  __zrbtree_color(pc)
#define __zrbtree_is_red(pc)    (!__zrbtree_color(pc))
#define zrbtree_color(rb)       __zrbtree_color((rb)->__zrbtree_parent_color)
#define zrbtree_is_red(rb)      __zrbtree_is_red((rb)->__zrbtree_parent_color)
#define zrbtree_is_black(rb)    __zrbtree_is_black((rb)->__zrbtree_parent_color)

static inline void zrbtree_set_parent(ZRBTREE_NODE * rb, ZRBTREE_NODE * p)
{
	rb->__zrbtree_parent_color = zrbtree_color(rb) | (unsigned long)p;
}

static inline void zrbtree_set_parent_color(ZRBTREE_NODE * rb, ZRBTREE_NODE * p, int color)
{
	rb->__zrbtree_parent_color = (unsigned long)p | color;
}

static inline void __zrbtree_change_child(ZRBTREE_NODE * old, ZRBTREE_NODE * new, ZRBTREE_NODE * parent, ZRBTREE * root)
{
	if (parent) {
		if (parent->zrbtree_left == old)
			parent->zrbtree_left = new;
		else
			parent->zrbtree_right = new;
	} else
		root->zrbtree_node = new;
}

void __zrbtree_erase_color(ZRBTREE * root, ZRBTREE_NODE * parent, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new));

static __always_inline ZRBTREE_NODE *__zrbtree_erase_augmented(ZRBTREE_NODE * node, ZRBTREE * root, const struct zrbtree_augment_callbacks *augment)
{
	ZRBTREE_NODE *child = node->zrbtree_right, *tmp = node->zrbtree_left;
	ZRBTREE_NODE *parent, *rebalance;
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
		ZRBTREE_NODE *successor = child, *child2;
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
			} while (tmp);
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

static __always_inline void zrbtree_erase_augmented(ZRBTREE_NODE * node, ZRBTREE * root, const struct zrbtree_augment_callbacks *augment)
{
	ZRBTREE_NODE *rebalance = __zrbtree_erase_augmented(node, root, augment);
	if (rebalance)
		__zrbtree_erase_color(root, rebalance, augment->rotate);
}

static inline void zrbtree_set_black(ZRBTREE_NODE * rb)
{
	rb->__zrbtree_parent_color |= RB_BLACK;
}

static inline ZRBTREE_NODE *zrbtree_red_parent(ZRBTREE_NODE * red)
{
	return (ZRBTREE_NODE *) red->__zrbtree_parent_color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
static inline void __zrbtree_rotate_set_parents(ZRBTREE_NODE * old, ZRBTREE_NODE * new, ZRBTREE * root, int color)
{
	ZRBTREE_NODE *parent = ZRBTREE_PARENT(old);
	new->__zrbtree_parent_color = old->__zrbtree_parent_color;
	zrbtree_set_parent_color(old, new, color);
	__zrbtree_change_child(old, new, parent, root);
}

static __always_inline void __zrbtree_insert(ZRBTREE_NODE * node, ZRBTREE * root, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new))
{
	ZRBTREE_NODE *parent = zrbtree_red_parent(node), *gparent, *tmp;

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
		if (parent != tmp) {	/* parent == gparent->zrbtree_left */
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
			gparent->zrbtree_left = tmp;	/* == parent->zrbtree_right */
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
			gparent->zrbtree_right = tmp;	/* == parent->zrbtree_left */
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
static __always_inline void ____zrbtree_erase_color(ZRBTREE_NODE * parent, ZRBTREE * root, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new))
{
	ZRBTREE_NODE *node = NULL, *sibling, *tmp1, *tmp2;

	while (true) {
		/*
		 * Loop invariants:
		 * - node is black (or NULL on first iteration)
		 * - node is not the root (parent is not NULL)
		 * - All leaf paths going through parent and node have a
		 *   black node count that is 1 lower than other leaf paths.
		 */
		sibling = parent->zrbtree_right;
		if (node != sibling) {	/* node == parent->zrbtree_left */
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
void __zrbtree_erase_color(ZRBTREE * root, ZRBTREE_NODE * parent, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new))
{
	____zrbtree_erase_color(parent, root, augment_rotate);
}

/*
 * Non-augmented rbtree manipulation functions.
 *
 * We use dummy augmented callbacks here, and have the compiler optimize them
 * out of the zrbtree_insert_color() and zrbtree_erase() function definitions.
 */

static inline void dummy_propagate(ZRBTREE_NODE * node, ZRBTREE_NODE * stop)
{
}

static inline void dummy_copy(ZRBTREE_NODE * old, ZRBTREE_NODE * new)
{
}

static inline void dummy_rotate(ZRBTREE_NODE * old, ZRBTREE_NODE * new)
{
}

static const struct zrbtree_augment_callbacks dummy_callbacks = {
	dummy_propagate, dummy_copy, dummy_rotate
};

void zrbtree_insert_color(ZRBTREE * root, ZRBTREE_NODE * node)
{
	__zrbtree_insert(node, root, dummy_rotate);
}

void zrbtree_erase(ZRBTREE * root, ZRBTREE_NODE * node)
{
	ZRBTREE_NODE *rebalance;
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

void __zrbtree_insert_augmented(ZRBTREE * root, ZRBTREE_NODE * node, void (*augment_rotate) (ZRBTREE_NODE * old, ZRBTREE_NODE * new))
{
	__zrbtree_insert(node, root, augment_rotate);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
ZRBTREE_NODE *zrbtree_first(ZRBTREE * root)
{
	ZRBTREE_NODE *n;

	n = root->zrbtree_node;
	if (!n)
		return NULL;
	while (n->zrbtree_left)
		n = n->zrbtree_left;
	return n;
}

ZRBTREE_NODE *zrbtree_last(ZRBTREE * root)
{
	ZRBTREE_NODE *n;

	n = root->zrbtree_node;
	if (!n)
		return NULL;
	while (n->zrbtree_right)
		n = n->zrbtree_right;
	return n;
}

ZRBTREE_NODE *zrbtree_next(ZRBTREE_NODE * node)
{
	ZRBTREE_NODE *parent;

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
		return (ZRBTREE_NODE *) node;
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

ZRBTREE_NODE *zrbtree_prev(ZRBTREE_NODE * node)
{
	ZRBTREE_NODE *parent;

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
		return (ZRBTREE_NODE *) node;
	}

	/*
	 * No left-hand children. Go up till we find an ancestor which
	 * is a right-hand child of its parent.
	 */
	while ((parent = ZRBTREE_PARENT(node)) && node == parent->zrbtree_left)
		node = parent;

	return parent;
}

void zrbtree_replace_node(ZRBTREE * root, ZRBTREE_NODE * victim, ZRBTREE_NODE * new)
{
	ZRBTREE_NODE *parent = ZRBTREE_PARENT(victim);

	/* Set the surrounding nodes to point to the replacement */
	__zrbtree_change_child(victim, new, parent, root);
	if (victim->zrbtree_left)
		zrbtree_set_parent(victim->zrbtree_left, new);
	if (victim->zrbtree_right)
		zrbtree_set_parent(victim->zrbtree_right, new);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;
}

/* XXX added by zc*/

void zrbtree_init(ZRBTREE * tree, ZRBTREE_CMP_FN cmp_fn)
{
	tree->zrbtree_node = 0;
	tree->cmp_fn = cmp_fn;
}

ZRBTREE_NODE *zrbtree_attach(ZRBTREE * tree, ZRBTREE_NODE * node)
{
	ZRBTREE_NODE **new = &(tree->zrbtree_node), *parent = 0;
	int cmp_result;

	while (*new) {
		parent = *new;
		cmp_result = tree->cmp_fn(node, *new);
		if (cmp_result < 0) {
			new = &((*new)->zrbtree_left);
		} else if (cmp_result > 0) {
			new = &((*new)->zrbtree_right);
		} else {
			return (*new);
		}
	}
	zrbtree_link_node(node, parent, new);
	zrbtree_insert_color(tree, node);

	return node;
}

ZRBTREE_NODE *zrbtree_lookup(ZRBTREE * tree, ZRBTREE_NODE * vnode)
{
	ZRBTREE_NODE *node;
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

ZRBTREE_NODE *zrbtree_remove(ZRBTREE * tree, ZRBTREE_NODE * vnode)
{
	ZRBTREE_NODE *node;

	node = zrbtree_lookup(tree, vnode);
	if (node) {
		zrbtree_erase(tree, node);
	}

	return node;
}

ZRBTREE_NODE *zrbtree_near_prev(ZRBTREE * tree, ZRBTREE_NODE * vnode)
{
	ZRBTREE_NODE *node, *ret_node;
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

ZRBTREE_NODE *zrbtree_near_next(ZRBTREE * tree, ZRBTREE_NODE * vnode)
{
	ZRBTREE_NODE *node, *ret_node;
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

void zrbtree_walk(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx)
{
	typedef struct {
		ZRBTREE_NODE *node;
		unsigned char lrs;
	} _NL;
	ZRBTREE_NODE *node;
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

void zrbtree_walk_forward(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx)
{
	ZRBTREE_NODE *node, *next;

	if (!walk_fn) {
		return;
	}
	for (node = zrbtree_first(tree); node; node = next) {
		next = zrbtree_next(node);
		(*walk_fn) (node, ctx);
	}
}

void zrbtree_walk_back(ZRBTREE * tree, void (*walk_fn) (ZRBTREE_NODE *, void *), void *ctx)
{
	ZRBTREE_NODE *node, *next;

	if (!walk_fn) {
		return;
	}
	for (node = zrbtree_last(tree); node; node = next) {
		next = zrbtree_prev(node);
		(*walk_fn) (node, ctx);
	}
}
