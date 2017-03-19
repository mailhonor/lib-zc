#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	char *p;
	int i;

	ZLEAK_INIT();

	for (i = 0; i < 100; i++) {
		p = (char *)zmalloc(sizeof(long long));
		if ((i % 9)) {
			zfree(p);
		}
	}

	return 0;
}
