#include "zc.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "test_lib.h"

int alarm_cb(ZALARM * ala, void *context)
{
	int sn;

	sn = ZVOID_PTR_TO_INT(context);

	zlog_info("alarm_cb : %d", sn);

	if (sn == 5) {
		exit(0);
	}

	return 0;
}

void alarm_entry(void)
{
	ZALARM *ala;
	int i;

	for (i = 1; i < 6; i++) {
		ala = (ZALARM *) zmalloc(sizeof(ZALARM));
		zalarm_init(ala);
		zalarm_set(ala, alarm_cb, ZINT_TO_VOID_PTR(i), i * 1111);
	}
}

int main(int argc, char **argv)
{
	/* zvar_alarm_signal = 32+5;  default signal */

	zalarm_base_init(0, 0);
	alarm_entry();

	while (1) {
		sleep(10);
	}
	exit(0);
}
