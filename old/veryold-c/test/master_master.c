#include "zc.h"
#include "test_lib.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	char *config_file;
	int test_lock = 0;
	char *lock_fn = "zfile/pid/master.pid";

	if (argc == 1) {
		config_file = "main.cf";
	} else {
		config_file = argv[1];
	}

	if (!strcmp(config_file, "-test_lock")) {
		test_lock = 1;
	}

	if (zmaster_lock_pid(lock_fn)) {
		if (test_lock) {
			exit(0);
		}
	} else {
		if (test_lock) {
			exit(1);
		}
	}

	{
		int i;
		for (i = 0; i < 3; i++) {
			(void)close(i);
		}
		open("/dev/null", O_RDWR, 0);
	}

	zmaster_start(0, config_file);

	return 0;
}
