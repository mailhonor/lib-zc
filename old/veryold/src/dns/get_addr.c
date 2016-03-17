#include "zc.h"

int zdns_getaddr_r(char *host, struct in_addr *addr_list, int addr_list_len)
{
	struct in_addr **addr_list_tmp;
	struct hostent htt, *htr = 0;
	char hbuf[4096], *tmpbuf;
	int tmpbuflen = 4096, hterror;
	int alloc_flag = 0, i;
	int ret_count = 0;

	if (ZEMPTY(host)) {
		return zdns_getlocaladdr_r(addr_list, addr_list_len);
	}

	tmpbuf = hbuf;
	while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror)) {
		if (hterror == NETDB_INTERNAL && errno == ERANGE) {
			tmpbuflen *= 2;
			if (alloc_flag) {
				tmpbuf = (char *)zrealloc(tmpbuf, tmpbuflen);
			} else {
				tmpbuf = (char *)zmalloc(tmpbuflen);
				alloc_flag = 1;
			}
		} else {
			break;
		}
	}
	if (htr) {
		addr_list_tmp = (struct in_addr **)htr->h_addr_list;
		for (i = 0; addr_list_tmp[i] != 0 && i < addr_list_len; i++, addr_list++) {
			addr_list->s_addr = addr_list_tmp[i]->s_addr;
			ret_count++;
		}
	}
	if (alloc_flag) {
		zfree(tmpbuf);
	}

	return ret_count;
}

int zdns_getlocaladdr_r(struct in_addr *addr_list, int addr_list_len)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_in *scin;
	int ret_count = 0;

	if (getifaddrs(&ifaddr) == -1) {
		return 0;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}
		if (ifa->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		scin = (struct sockaddr_in *)(ifa->ifa_addr);
		addr_list[ret_count].s_addr = scin->sin_addr.s_addr;
		ret_count++;
#if 0
#ifdef ZDEBUG
		char host[20];
		if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr), host, 20, NULL, 0, NI_NUMERICHOST)) {
			continue;
		}
		ZVERBOSE("local addr: %s", host);
#endif
#endif
	}

	freeifaddrs(ifaddr);

	return ret_count;
}
