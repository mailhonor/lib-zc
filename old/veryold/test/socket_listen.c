#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	int sock;
	int fd;
	FILE *fp;
	char buf[1024];

	sock = zsocket_inet_listen(0, 9099, -1);
	while (1) {
		fd = zsocket_inet_accept(sock);
		if (fd < 0) {
            printf("accept error\n");
			continue;
		}
		fp = fdopen(fd, "r+");
		int i = 0;
        printf("while brefor\n");
		while (1) {
			if (fgets(buf, 1000, fp)) ;
			fprintf(fp, "your input: %s\n", buf);
			if (!strncmp(buf, "exit", 4)) {
				fclose(fp);
				break;
			}
			if (i++ == 0) {
				close(fd);
			}
		}
	}

	return 0;
}
