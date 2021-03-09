#include <zyc.h>


int test_get(){
	int var_au, var_at, var_atp;
	ZCONFIG_TIME tlist[]={
		{"anvil_rate_time_unit", "30s", &var_au},
		{"anvil_status_update_time", "50s", &var_at},
		{"anvil_status_update_dftime", "90w", &var_atp},
		{0}
	};

	zgconfig_time_table(tlist);
	printf("var_au = %d\n", var_au);
	printf("var_at = %d\n", var_at);
	printf("var_atp = %d\n", var_atp);

	return 0;
}
int main(int argc, char **argv)
{
	zgconfig_init();
	zgconfig_load(argv[1], 1);
	test_get();
	zgconfig_show();
	return 0;
}
