#include <zyc.h>
#include <signal.h>




int main(int argc, char **argv){
	int i,ipnum;
	ZARGV *iplist, *ss;
	char *domain;

	signal(SIGPIPE, SIG_IGN);
	ss=zargv_create(16);
	zargv_add(ss, "local");
	for(i=1;i<argc;i++){
		zargv_add(ss, argv[i]);
	}
	for(i=0;i<ZARGV_LEN(ss);i++){
		domain=ss->argv[i];
		if(i==0){
			iplist=zaddr_get_localaddr();
		}else{
			iplist = zaddr_get_addr(domain);
		}
		printf("%d, domain: %s, iplist: ", i, domain);
		if(iplist==0){
			printf("none\n");
		}else{
			for(ipnum=0;ipnum<ZARGV_LEN(iplist);ipnum++){
				printf(" %s ", iplist->argv[ipnum]);
			}
			printf("\n");
		}
	}
	return 0;
}
