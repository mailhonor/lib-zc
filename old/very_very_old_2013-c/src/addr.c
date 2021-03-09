#include "zyc.h"

static ZARGV *z_addr_list = 0;

ZARGV *zaddr_get_addr_r(char *host, ZARGV *_addr_list){
	int i;
	struct hostent *hp;
	struct in_addr **addr_list;

	if(z_addr_list == 0){
		z_addr_list= zargv_create(12);
	}
	if(_addr_list==0){
		_addr_list=z_addr_list;
	}
	zargv_reset(_addr_list);

	hp = gethostbyname(host);
	if (hp == NULL) {
		return(NULL);
	} else {
		addr_list = (struct in_addr **)hp->h_addr_list;
		for(i = 0; addr_list[i] != NULL; i++) {
			zargv_add(_addr_list, inet_ntoa(*addr_list[i]));
		}
	}
	return _addr_list;
}

ZARGV *zaddr_get_localaddr_r(ZARGV *_addr_list){
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[20];

	if(z_addr_list == 0){
		z_addr_list = zargv_create(12);
	}
	if(_addr_list==0){
		_addr_list=z_addr_list;
	}
	zargv_reset(_addr_list);

	if (getifaddrs(&ifaddr) == -1) {
		zmsg_fatal("zaddr_get_localaddr: getifaddrs error: %m");
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;

		if (family != AF_INET){
			continue;
		}
		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, 20, NULL, 0, NI_NUMERICHOST);
		if (s != 0) {
			continue;
		}
		zargv_add(_addr_list, host);
	}

	freeifaddrs(ifaddr);
	return _addr_list;
}
