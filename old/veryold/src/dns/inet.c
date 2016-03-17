#include "zc.h"

void zdns_inet_ntoa_r(struct in_addr *in, char *buf)
{
#if 0
	unsigned char *bytes = (unsigned char *)in;
	sprintf(buf, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
#else
	inet_ntop(AF_INET, in, buf, 16);
#endif
}
