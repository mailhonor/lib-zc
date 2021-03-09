#include <test.h>
#include <zyc.h>

int main(int argc, char **argv)
{
	int fd;
	ZSTREAM *zsm;
	ZARGV *av;
	int i;
	char buf[1024]="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	buf[1000]=0;
	av=zargv_create(10);
	for(i=0;i<10;i++){
		sprintf(buf, "%s:%d", "abcde", i);
		zargv_add(av, buf);
	}
	fd=open("test/attr.txt", O_RDWR|O_CREAT|O_TRUNC|O_APPEND, 0666);
	zsm=zstream_fdopen(fd, 10);

	zattr_fprint(zsm, ZATTR_DOUBLE, 123.21311, ZATTR_CHAR, 'c',
		       	ZATTR_STR, buf, ZATTR_DATA, buf, 18, ZATTR_ZARGV, av, ZATTR_END);
	ZSTREAM_FFLUSH(zsm);
	printf("error:%d\n", zstream_ferror(zsm));
	zstream_fclose(zsm);

	fd=open("test/attr.txt", O_RDONLY);
	zsm=zstream_fdopen(fd, 10);
	int tint=100;
	char tc;
	double tdou;
	ZSTR *tzbuf, *tdzbuf;
	tzbuf=zstr_create(11);
	tdzbuf=zstr_create(11);
	zargv_reset(av);
	zattr_fscan(zsm, ZATTR_DOUBLE, &tdou, ZATTR_CHAR, &tc,
		       	ZATTR_STR, tzbuf, ZATTR_DATA, tdzbuf, ZATTR_ZARGV, av, ZATTR_END);

	printf("tchar: %c\n", tc);
	printf("tdouble: %f\n", tdou);
	printf("tint: %d\n", tint);
	printf("tzbuf: %s, %d\n", zstr_str(tzbuf), zstr_len(tzbuf));
	printf("tdzbuf: %s, %d\n", zstr_str(tdzbuf), zstr_len(tdzbuf));
	for(i=0;i<ZARGV_LEN(av);i++){
		printf("%s\n", av->argv[i]);
	}
	zstream_fclose(zsm);
	
	return(0);
}
