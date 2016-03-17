#include "zc.h"
#include <time.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	ZMPOOL *zmp;
	int i, key, left, ecps, eul, ac, lc;
	char **as;

	if (argc != 5) {
		fprintf(stderr, "USAGE   : %s element_count_per_set element_unused_limit alloc_count left_count\n", argv[0]);
		fprintf(stderr, "NOW RUN : %s 10 25 1000000 10\n", argv[0]);
		char *ARGV[] = { "", "10", "25", "1000000", "100" };
		argv = ARGV;
	}

	ecps = atoi(argv[1]);
	eul = atoi(argv[2]);
	ac = atoi(argv[3]);
	lc = atoi(argv[4]);

	as = (char **)zcalloc(ac, sizeof(char *));
	zmp = zmpool_create(2, ecps, eul);
	srand((int)(time(NULL)));

	for (i = 0; i < ac; i++) {
		as[i] = (char *)zmpool_alloc_one(zmp);
	}
	printf("set: %d\n", zmp->set_sum);
	left = ac;

	while (1) {
		key = ((unsigned int)rand()) % (unsigned int)left;
		zmpool_free_one(zmp, as[key]);
		as[key] = as[left - 1];
		if (left-- < lc) {
			break;
		}
	}
	printf("set: %d\n", zmp->set_sum);

	zmpool_free(zmp);

	zfree(as);

	return 0;
}
