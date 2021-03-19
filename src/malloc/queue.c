/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-17
 * ================================
 */

#include "zc.h"

typedef struct zmqueue_node_t zmqueue_node_t;
struct zmqueue_node_t {
    zmqueue_node_t *next;
};

struct zmqueue_t {
    zmqueue_node_t *head;
    zmqueue_node_t *tail;
    short int head_put_idx;
    short int tail_get_idx;
    short int element_size;
    short int element_count_per_queue;
};

zmqueue_t *zmqueue_create(int element_size, int element_count_per_queue)
{
    if (element_count_per_queue > 4096) {
        element_count_per_queue = 4096;
    }
    if (element_count_per_queue < 32) {
        element_count_per_queue = 32;
    }
    zmqueue_t *mq = (zmqueue_t *)zcalloc(1, sizeof(zmqueue_t));
    mq->element_size = element_size;
    mq->element_count_per_queue = element_count_per_queue;
    return mq;
}

void zmqueue_free(zmqueue_t *mq)
{
    if (!mq) {
        return;
    }
    zmqueue_node_t *node, *next;
    for (node = mq->head; node; node = next) {
        next = node->next;
        zfree(node);
    }
    zfree(mq);
}

void *zmqueue_require_and_push(zmqueue_t *mq)
{
    zmqueue_node_t *node;
    char *r;
    if (!mq) {
        return 0;
    }
    if (!(mq->tail)) {
        node = zmalloc(sizeof(zmqueue_node_t) + mq->element_size * mq->element_count_per_queue);
        node->next = 0;
        mq->head = mq->tail = node;
    }
    if (mq->tail_get_idx >= mq->element_count_per_queue) {
        node = zmalloc(sizeof(zmqueue_node_t) + mq->element_size * mq->element_count_per_queue);
        node->next = 0;
        mq->tail->next = node;
        mq->tail = node;
        mq->tail_get_idx = 0;
    }
    r = ((char *)(mq->tail)) + sizeof(zmqueue_node_t) + mq->element_size * mq->tail_get_idx;
    mq->tail_get_idx++;
    return r;
}

void zmqueue_release_and_shift(zmqueue_t *mq)
{
    zmqueue_node_t *node;
    if (!mq) {
        return;
    }
    if (!(mq->head)) {
        return;
    }
    mq->head_put_idx ++;
    if (mq->head != mq->tail) {
        if (mq->head_put_idx >= mq->element_count_per_queue) {
            node = mq->head->next;
            zfree(mq->head);
            mq->head = node;
            if (!(mq->head)) {
                mq->tail = 0;
            }
            mq->head_put_idx = 0;
        }
    } else {
        if (mq->head_put_idx >= mq->tail_get_idx) {
            zfree(mq->head);
            mq->head = mq->tail = 0;
            mq->head_put_idx = 0;
            mq->tail_get_idx = 0;
        }
    }
}

void *zmqueue_get_head(zmqueue_t *mq)
{
    if (!mq) {
        return 0;
    }
    if (!(mq->head)) {
        return 0;
    }
    return (void *)(((char *)(mq->head)) + sizeof(zmqueue_node_t) + mq->element_size*mq->head_put_idx);
}

