#include "zc.h"

int zdns_getpeer_r(int sockfd, int *host, int *port)
{
	struct sockaddr_in sa;
	socklen_t sa_length = sizeof(struct sockaddr);

	if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0) {
		return -1;
	}

	if (host) {
		*host = *((int *)&(sa.sin_addr));
	}

	if (port) {
		*port = ntohs(sa.sin_port);
	}

	return 0;
}
