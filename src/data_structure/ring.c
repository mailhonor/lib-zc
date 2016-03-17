/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-28
 * ================================
 */

#include "libzc.h"

void zring_init(zring_t * ring)
{
    ring->next = ring->prev = ring;
}

void zring_append(zring_t * ring, zring_t * entry)
{
    entry->prev = ring->prev;
    entry->next = ring;
    ring->prev->next = entry;
    ring->prev = entry;
}

void zring_prepend(zring_t * ring, zring_t * entry)
{
    entry->next = ring->next;
    entry->prev = ring;
    ring->next->prev = entry;
    ring->next = entry;
}

void zring_detach(zring_t * entry)
{
    zring_t *prev = entry->prev;
    zring_t *next = entry->next;

    next->prev = prev;
    prev->next = next;

    entry->prev = entry->next = 0;
}
