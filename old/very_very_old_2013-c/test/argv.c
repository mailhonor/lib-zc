#include <zyc.h>

int main(int argc, char **argv)
{
	FILE *fp;
	char buf[1024]="abc def\t fjsldkf";
	ZARGV *av;
	int i;
	if(argc == 1){
		av = zargv_split(buf, " \r\t\n");
		for(i=0;i<av->argc;i++){
			printf("argv[%d]=%s\n", i, av->argv[i]);
		}
	}else{
		fp = fopen(argv[1], "r");
		while(fgets(buf, 1024, fp)){
			av = zargv_split((char *)buf, " \t\r$");
			zargv_addn(av, (char *)buf, -1);
			for(i=0;i<av->argc;i++)
				printf("argv[%d]=%s\n", i, av->argv[i]);
			zargv_free(av, 0, 0);
		}
	}
	return(0);
}
