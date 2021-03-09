#include <zyc.h>
#include <test.h>
#include "test_lib.c"

void test_output(int level, int is_zyclib, char *fmt, va_list ap){
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "%s\n", " already change output function");
}

int main()
{
	zmsg_info("this is first log");
	zmsg_set_output(test_output);
	zmsg_info("this is seccond log");
	zmsg_set_output(test_zmsg_output_syslog);
	zmsg_info("this is seccond log ...");
	return(0);
}
