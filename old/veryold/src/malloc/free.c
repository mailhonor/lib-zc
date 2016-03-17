#include "zc.h"

void zfree_structure_string(void *value, void *ctx)
{
	if (value) {
		free(value);
	}
}
