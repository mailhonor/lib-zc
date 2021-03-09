#include <zyc.h>
#include "test_lib.c"

int main(int argc, char **argv){

	zmsg_set_output(test_zmsg_output_syslog);
	zmaster_server(argc, argv);
	return 0;
}

