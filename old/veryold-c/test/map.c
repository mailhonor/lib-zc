#include "zc.h"
#include "test_lib.h"

void _usage()
{
	fprintf(stderr, "USAGE:\n\t%s map_url query_string\n", zvar_program_name);
	exit(1);
}

int main(int argc, char **argv)
{
	ZMAP *zm;
	char *url;
	char *query;
	char result[1024];
	int ret;

	test_init(argc, argv);

	if (argc < 3) {
		_usage();
	}
	url = argv[1];
	query = argv[2];

	zm = zmap_create(0);
	ret = zmap_open(zm, url);
	if (ret < 0) {
		fprintf(stderr, "create map error\n");
		exit(1);
	}

	ret = zmap_query(zm, query, result, 1024, 1000);

	zmap_free(zm);

	if (ret == Z_NONE) {
		printf("NO RESULT!!!\n");
	} else if (ret < 0) {
		printf("ERROR!!!\n");
	} else {
		printf("result: %s\n", result);
	}

	return 0;
}
