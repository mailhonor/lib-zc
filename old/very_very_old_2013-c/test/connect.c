#include <zyc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	ZSTREAM *zsm;
	ZSTR *zs;
	char *default_host="125.88.109.58", *dip;
	int fd;

	zs=zstr_create(1);

	dip=default_host;
	if(argc==2) dip=argv[1];
	printf("dip=%s\n", dip);

	fd=zsocket_inet_connect(dip, 25, 100);
	if(fd<0){
		printf("connect error\n");
		return 0;
	}
	zsm=zstream_fdopen(fd, O_RDWR);
	zstream_set_timeout(zsm, 10);
	if(zsm==NULL){
		printf("zsm fdopen error\n");
		return 0;
	}
	zstream_get_delimiter(zsm, zs, '\n');
	printf("read: %s\n", zstr_str(zs));

	zstream_fprintf(zsm, "quit\n");
	printf("wbuf_len: %d\n", zsm->wbuf_len);
	zstream_fflush(zsm);
	while(1){
		if(zstream_feof(zsm)) break;
		if(zstream_get_delimiter(zsm, zs, '\n')<1) break;
		printf("read: %s", zstr_str(zs));
	}
	zstream_fclose(zsm);
	return 0;
}
