/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "libzc.h"

void *zdichotomy_search(void *list, long element_size, int element_count, int (*cmp_fn) (void *, void *), void *key)
{
    void *node_left, *node_center;
    int position_left, position_right, position_center;
    int ret;

    position_left = 1;
    position_right = element_count;

    while (1)
    {
        if (position_left > position_right)
        {
            return 0;
        }
        node_left = (void *)(((char *)list) + (position_left - 1) * element_size);
        if (position_left == position_right)
        {
            if (!cmp_fn(key, node_left))
            {
                return node_left;
            }
            return 0;
        }

        position_center = (position_left + position_right) / 2;
        node_center = (void *)(((char *)list) + (position_center - 1) * element_size);

        ret = cmp_fn(key, node_center);
        if (ret == 0)
        {
            return node_center;
        }
        if (ret < 0)
        {
            position_right = position_center - 1;
        }
        else
        {
            position_left = position_center + 1;
        }
    }

    return 0;
}
