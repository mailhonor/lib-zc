#include <zyc.h>

int main(int argc, char **argv){
	char *fn;

	if(argc < 2){
		fprintf(stderr, "USAGE: %s config_file\n", argv[0]);
		return 1;
	}
	fn=argv[1];

	zgconfig_init();
	zgconfig_load(fn, 0);
	zgconfig_show();
	return 0;
}

