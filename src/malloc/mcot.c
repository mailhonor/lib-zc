/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-01-04
 * ================================
 */

#include "libzc.h"

zmcot_t *zmcot_create(int element_size)
{
    int list[2];
    zmcot_t *cot;

    cot = (zmcot_t *)zcalloc(1, sizeof(zmcot_t));

    list[0] = element_size;
    list[1] = 0;
    cot->mpool = zmpool_create_default_pool(list);
    cot->element_size = element_size;

    return cot;
}

void zmcot_free(zmcot_t * cot)
{
    zmpool_free_pool(cot->mpool);
    zfree(cot);
}

void *zmcot_alloc_one(zmcot_t * cot)
{
    return cot->mpool->malloc(cot->mpool, cot->element_size);
}

void zmcot_free_one(zmcot_t * cot, void *ptr)
{
    cot->mpool->free(cot->mpool, ptr);
}
