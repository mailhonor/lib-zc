#ifndef ___ZYC_INCLUDE__
#include<stdio.h>
int main(){
	fprintf(stderr, "This programe only  provide some basic funcs to other programs used for test in this test-dir.\n");
	return 0;
}
#else
#include <syslog.h>
#include <test.h>
void test_zmsg_output_syslog(int level, int who, char *fmt, va_list ap){
	static int test_zmsg_output_syslog_init=0;
	static ZSTR *test_zmsg_output_syslog_fmt=0;

	if(test_zmsg_output_syslog_init==0){
		test_zmsg_output_syslog_init=1;
		test_zmsg_output_syslog_fmt=zstr_create(1024);
		openlog(zvar_program_name?zvar_program_name:0, LOG_PID|LOG_NDELAY, LOG_SYSLOG);
	}
	zstr_strcpys(test_zmsg_output_syslog_fmt, (who?"S ":"U "), fmt, 0);
	vsyslog(level, ZSTR_STR(test_zmsg_output_syslog_fmt), ap);
}












#endif
