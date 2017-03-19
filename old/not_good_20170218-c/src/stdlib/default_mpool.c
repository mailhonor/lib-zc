/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-07
 * ================================
 */

#include "libzc.h"

typedef struct zmem_default_pool_t zmem_default_pool_t;
typedef struct zmem_default_pool_setgroup_t zmem_default_pool_setgroup_t;
typedef struct zmem_default_pool_set_t zmem_default_pool_set_t;

struct zmem_default_pool_set_t {
    unsigned short int setgroup_id;
    unsigned short int element_unused_id;
    unsigned short int element_unused_sum;
    char *element_list;
    zrbtree_node_t rbnode;
    zlink_node_t linknode;
};

struct zmem_default_pool_setgroup_t {
    unsigned short int setgroup_id;
    unsigned short int element_size;
    unsigned short int element_count_per_set;
    int element_unused_sum;
    zlink_t set_used;
    zlink_t set_unused;
};

struct zmem_default_pool_t {
    unsigned short int setgroup_count;
    zrbtree_t rbtree;
};

typedef struct {
    unsigned short int e_id;
} zmem_default_pool_t_USI;

static int default_register_list[] = {
    4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 0
};

static int zmem_default_pool_set_cmp(zrbtree_node_t * rn1, zrbtree_node_t * rn2)
{
    zmem_default_pool_set_t *set1, *set2;

    set1 = ZCONTAINER_OF(rn1, zmem_default_pool_set_t, rbnode);
    set2 = ZCONTAINER_OF(rn2, zmem_default_pool_set_t, rbnode);

    return (set1->element_list - set2->element_list);
}

static zmem_default_pool_set_t *zmem_default_pool_set_create(zmem_default_pool_t * worker, zmem_default_pool_setgroup_t * setgroup)
{
    int i;
    char *data;
    zmem_default_pool_set_t *set;
    zmem_default_pool_t_USI *usi;

    data = (char *)zmalloc(sizeof(zmem_default_pool_set_t) + setgroup->element_size * setgroup->element_count_per_set);
    set = (zmem_default_pool_set_t *) data;
    memset(set, 0, sizeof(zmem_default_pool_set_t));

    set->element_list = data + sizeof(zmem_default_pool_set_t);
    for (i = 0; i < setgroup->element_count_per_set; i++) {
        usi = (zmem_default_pool_t_USI *) (set->element_list + setgroup->element_size * i);
        usi->e_id = (unsigned short int)(i + 1);
    }
    set->element_unused_id = 0;
    set->element_unused_sum = setgroup->element_count_per_set;

    return set;
}

void *zmem_default_pool_malloc(zmpool_t * mp, int len)
{
    zmem_default_pool_t *worker = (zmem_default_pool_t *) mp->worker;
    zmem_default_pool_setgroup_t *setgroup_list, *setgroup;
    zmem_default_pool_set_t *set;
    zmem_default_pool_t_USI *usi;
    unsigned short int e_id;
    char *ptr;

    setgroup_list = (zmem_default_pool_setgroup_t *)
        ((char *)worker + sizeof(zmem_default_pool_t));
    if (len > (setgroup_list[worker->setgroup_count - 1].element_size)) {
        return zmalloc(len);
    }

    setgroup = 0;
    {
        /* find setgroup */
        int left, right, center;
        zmem_default_pool_setgroup_t *center_node;
        left = 0;
        right = worker->setgroup_count - 1;
        while (1) {
            if (left == right) {
                setgroup = setgroup_list + left;
                break;
            }
            center = (left + right) / 2;
            center_node = setgroup_list + center;

            if (center_node->element_size == len) {
                setgroup = center_node;
                break;
            }
            if (center_node->element_size < len) {
                left = center + 1;
                continue;
            }
            right = right - 1;
        }
    }

    if (setgroup->set_used.head == 0) {
        if (setgroup->set_unused.head) {
            set = ZCONTAINER_OF(setgroup->set_unused.head, zmem_default_pool_set_t, linknode);
            zlink_detach(&(setgroup->set_unused), setgroup->set_unused.head);
        } else {
            set = zmem_default_pool_set_create(worker, setgroup);
            zrbtree_attach(&(worker->rbtree), &(set->rbnode));
            setgroup->element_unused_sum += setgroup->element_count_per_set;
        }
        zlink_push(&(setgroup->set_used), &(set->linknode));
    }
    setgroup->element_unused_sum--;

    set = ZCONTAINER_OF(setgroup->set_used.head, zmem_default_pool_set_t, linknode);
    e_id = set->element_unused_id;
    ptr = (char *)(set->element_list + e_id * setgroup->element_size);
    usi = (zmem_default_pool_t_USI *) ptr;
    set->element_unused_id = usi->e_id;
    set->element_unused_sum--;

    if (set->element_unused_sum == 0) {
        zlink_detach(&(setgroup->set_used), &(set->linknode));
    }

    return ptr;
}

static inline zmem_default_pool_set_t *find_set(zmem_default_pool_t * worker, const void *ptr)
{
    zmem_default_pool_setgroup_t *setgroup_list, *setgroup;
    zmem_default_pool_set_t *set, rs;
    zrbtree_node_t *rn;

    rs.element_list = (char *)ptr + 1;
    rn = zrbtree_near_prev(&(worker->rbtree), &(rs.rbnode));
    if (rn == 0) {
        return 0;
    }

    set = ZCONTAINER_OF(rn, zmem_default_pool_set_t, rbnode);
    setgroup_list = (zmem_default_pool_setgroup_t *)
        ((char *)worker + sizeof(zmem_default_pool_t));
    setgroup = setgroup_list + set->setgroup_id;

    if (((char *)ptr - (char *)(set->element_list)) > ((unsigned int)(setgroup->element_count_per_set) * (unsigned int)(setgroup->element_size))) {
        return 0;
    }

    return set;
}

static inline void free_by_set(zmem_default_pool_t * worker, zmem_default_pool_set_t * set, const void *ptr)
{
    zmem_default_pool_setgroup_t *setgroup_list;
    zmem_default_pool_setgroup_t *setgroup;
    zmem_default_pool_t_USI *usi;

    setgroup_list = (zmem_default_pool_setgroup_t *) ((char *)worker + sizeof(zmem_default_pool_t));
    setgroup = setgroup_list + set->setgroup_id;

    usi = (zmem_default_pool_t_USI *) ptr;
    usi->e_id = set->element_unused_id;
    set->element_unused_id = (unsigned short int)((((char *)ptr) - set->element_list) / setgroup->element_size);
    set->element_unused_sum++;

    setgroup->element_unused_sum++;

    if (set->element_unused_sum == 1) {
        zlink_unshift(&(setgroup->set_used), &(set->linknode));
    } else if (set->element_unused_sum == setgroup->element_count_per_set) {
        zlink_detach(&(setgroup->set_used), &(set->linknode));
        zlink_unshift(&(setgroup->set_unused), &(set->linknode));
    }

    if (setgroup->element_unused_sum > setgroup->element_count_per_set * 2) {
        if (setgroup->set_unused.head) {
            set = ZCONTAINER_OF(setgroup->set_unused.head, zmem_default_pool_set_t, linknode);
            zrbtree_detach(&(worker->rbtree), &(set->rbnode));
            zlink_detach(&(setgroup->set_unused), &(set->linknode));
            zfree(set);
            setgroup->element_unused_sum -= setgroup->element_count_per_set;
        }
    }
}

void *zmem_default_pool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    zmem_default_pool_t *worker = (zmem_default_pool_t *) mp->worker;
    zmem_default_pool_set_t *set;
    zmem_default_pool_setgroup_t *setgroup_list, *setgroup;
    char *rp;

    if (!ptr) {
        return zmem_default_pool_malloc(mp, len);
    }

    set = find_set(worker, ptr);
    if (!set) {
        return zrealloc(ptr, len);
    }

    setgroup_list = (zmem_default_pool_setgroup_t *)
        ((char *)worker + sizeof(zmem_default_pool_t));
    setgroup = setgroup_list + set->setgroup_id;
    if (setgroup->element_size <= len) {
        return (void *)ptr;
    }

    rp = zmem_default_pool_malloc(mp, len);
    memcpy(rp, ptr, setgroup->element_size);
    free_by_set(worker, set, ptr);

    return rp;
}

void zmem_default_pool_free(zmpool_t * mp, const void *ptr)
{
    zmem_default_pool_t *worker = (zmem_default_pool_t *) mp->worker;
    zmem_default_pool_set_t *set;

    if (!ptr) {
        return;
    }

    set = find_set(worker, ptr);
    if (!set) {
        return;
    }

    free_by_set(worker, set, ptr);
}

/* ################################################################## */
static void zmem_default_pool_free_pool(zmpool_t * mp)
{
    zmem_default_pool_t *worker = (zmem_default_pool_t *) mp->worker;
    zrbtree_node_t *rn;
    zmem_default_pool_set_t *set;

    while ((rn = zrbtree_first(&(worker->rbtree)))) {
        set = ZCONTAINER_OF(rn, zmem_default_pool_set_t, rbnode);
        zrbtree_detach(&(worker->rbtree), rn);
        zfree(set);
    }
    zfree(mp);
}

static void zmem_default_pool_reset(zmpool_t * mp)
{
    zmem_default_pool_t *worker = (zmem_default_pool_t *) mp->worker;
    zrbtree_node_t *rn;
    zmem_default_pool_setgroup_t *setgroup, *setgroup_list;
    zmem_default_pool_set_t *set;
    int i;

    while ((rn = zrbtree_first(&(worker->rbtree)))) {
        set = ZCONTAINER_OF(rn, zmem_default_pool_set_t, rbnode);
        zrbtree_detach(&(worker->rbtree), rn);
        zfree(set);
    }
    setgroup_list = (zmem_default_pool_setgroup_t *)
        ((char *)worker + sizeof(zmem_default_pool_t));
    for (i = 0; i < worker->setgroup_count; i++) {
        setgroup = setgroup_list + i;
        setgroup_list->element_unused_sum = 0;
        zlink_init(&(setgroup->set_used));
        zlink_init(&(setgroup->set_unused));
    }
}

/* ################################################################## */
zmpool_t *zmpool_create_default_pool(int *register_list)
{
    zmpool_t *mp;
    zmem_default_pool_t *worker;
    zmem_default_pool_setgroup_t *setgroup, *setgroup_list;
    int *register_one, element_size;
    int i, ecount, register_count;

    if (register_list == 0) {
        register_list = default_register_list;
    }
    register_count = 0;
    register_one = register_list;
    for (; register_one && *register_one; register_one++) {
        element_size = *register_one;
        if (element_size < sizeof(unsigned short int)) {
            zfatal("zmem_default_pool_create: element_size must > %d ", (int)(sizeof(unsigned short int)));
        }
        register_count++;
    }

    mp = (zmpool_t *) zcalloc(1, sizeof(zmpool_t) + sizeof(zmem_default_pool_t) + sizeof(zmem_default_pool_setgroup_t) * register_count);

    worker = (zmem_default_pool_t *) ((char *)mp + sizeof(zmpool_t));
    mp->worker = worker;
    mp->malloc = zmem_default_pool_malloc;
    mp->realloc = zmem_default_pool_realloc;
    mp->free = zmem_default_pool_free;
    mp->free_pool = zmem_default_pool_free_pool;
    mp->reset = zmem_default_pool_reset;

    setgroup_list = (zmem_default_pool_setgroup_t *)
        ((char *)worker + sizeof(zmem_default_pool_t));
    for (i = 0; i < register_count; i++) {
        element_size = register_list[i];
        setgroup = setgroup_list + i;

        setgroup->setgroup_id = i;
        setgroup->element_size = element_size;
        ecount = 10 * 1024 / setgroup->element_size;
        if (ecount < 100) {
            ecount = 100;
        }
        if (ecount > 4096) {
            ecount = 4096;
        }
        setgroup->element_count_per_set = ecount;

        zlink_init(&(setgroup->set_used));
        zlink_init(&(setgroup->set_unused));
    }
    worker->setgroup_count = register_count;
    zrbtree_init(&(worker->rbtree), zmem_default_pool_set_cmp);

    return mp;
}
