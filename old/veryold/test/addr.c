#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	char *host = 0;
	struct in_addr addr_list[128];
	char ip[18];;
	int count, i;

	if (argc > 1) {
		host = argv[1];
	}
	count = zdns_getaddr_r(host, addr_list, 128);
	if (count < 1) {
		printf("none\n");
		return -1;
	}
	for (i = 0; i < count; i++) {
		zdns_inet_ntoa_r(addr_list + i, ip);
		printf("%s\n", ip);
	}

	return 0;
}
