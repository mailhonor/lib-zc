#include "zyc.h"

typedef struct{
	char *ip;
	int port;
	int fd;
} ZSERVER_SOCKET_FD;

ZCHAIN *z_server_listen_fd_list=0;
int zserver_monitor_inet(char *sip, int port){
	return 0;
}

int zserver_monitor_unix(char *path){
	return 0;
}

int zserver_init(){
	z_server_listen_fd_list=zchain_create();
	zevent_init();
	return 0;
}

int zserver_start(){
	return 0;
}
