#include "zc.h"

int zsocket_inet_connect_host(char *host, int port, int timeout)
{
	struct in_addr addr_list[8];
	char ip[18];;
	int sock, count, i;

	count = zdns_getaddr_r(host, addr_list, 8);
	if (count < 1) {
		return -1;
	}
	for (i = 0; i < count; i++) {
		zdns_inet_ntoa_r(addr_list + i, ip);
		sock = zsocket_inet_connect(ip, port, timeout);
		if (sock < 1) {
			return -2;
		}
		return sock;
	}

	return -2;
}
