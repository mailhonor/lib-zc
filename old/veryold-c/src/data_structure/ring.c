#include "zc.h"

void zring_init(ZRING * ring)
{
	ring->next = ring->prev = ring;
}

void zring_append(ZRING * ring, ZRING * entry)
{
	entry->prev = ring->prev;
	entry->next = ring;
	ring->prev->next = entry;
	ring->prev = entry;
}

void zring_prepend(ZRING * ring, ZRING * entry)
{
	entry->next = ring->next;
	entry->prev = ring;
	ring->next->prev = entry;
	ring->next = entry;
}

void zring_detach(ZRING * entry)
{
	ZRING *prev = entry->prev;
	ZRING *next = entry->next;

	next->prev = prev;
	prev->next = next;

	entry->prev = entry->next = 0;
}
