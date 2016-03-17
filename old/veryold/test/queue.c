#include "zc.h"
#include "test_lib.h"

static ZQUEUE *qu;

void *do_run(void *arg)
{
	void *res;
	int i, id, sn;

	sn = ZVOID_PTR_TO_INT(arg);
	usleep(sn * 10 * 1000);
	printf("pthread start: %d\n", sn);

	for (i = 0; i < 2; i++) {
		res = zqueue_require(qu);
		id = ZVOID_PTR_TO_INT(res);
		printf("res id: %d, phtread sn: %d\n", id, sn);
		sleep(1);
		zqueue_release(qu, res);
		sleep(3);
	}

	return arg;
}

int main(int argc, char **argv)
{

	int i;
	pthread_t pths[32];

	qu = zqueue_create();
	for (i = 0; i < 5; i++) {
		zqueue_enter_resource(qu, ZINT_TO_VOID_PTR(i));
	}

	for (i = 0; i < 30; i++) {
		pthread_create(&(pths[i]), 0, do_run, ZINT_TO_VOID_PTR(i));
	}

	for (i = 0; i < 30; i++) {
		pthread_join(pths[i], 0);
	}

	zqueue_free(qu);

	return 0;
}
