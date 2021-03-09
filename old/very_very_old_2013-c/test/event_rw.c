#include <zyc.h>
#include <test.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

int test_accept_after(int rwe, int fd, char *ctx);
int accept_times=1;
int proc_num=0;

int test_timeout_timer(int rwe, int fd, char *ctx){
	zmsg_debug("timeout: colse socket fd: %d", fd);
	zevent_disable_readwrite(fd);
	close(fd);
	return 0;
}

int test_accept(int rwe, int sock_fd, char *ctx){
	int fd;

	zmsg_debug("accept ready");
	fd = zsocket_inet_accept(sock_fd);
	zio_set_nonblock(fd);
	zevent_enable_readwrite(fd, test_accept_after, 0);
	zevent_request_timer(fd, test_timeout_timer, 0, 60);
	zmsg_debug("accept : times: %d  fd:%d", accept_times++,fd);
	return 0;
}

int test_accept_after(int rwe, int fd, char *ctx){

	zmsg_debug("accept after");
	zevent_cancel_timer(fd, test_timeout_timer, 0);
	if(ZEVENT_IS_EXCEPTION(rwe)){
		zmsg_debug("test_accept_after: exception: %X, fd:%d", rwe, fd);
		zevent_disable_readwrite(fd);
		proc_num--;
		close(fd);
		return 0;
	}
	proc_num++;
	ZSTREAM *zsm;
	ZSTR *zs;

	zs=zstr_create(1024);
	zsm=zstream_fdopen(fd, 10);
	if(ZEVENT_IS_WRITE(rwe)){
		zstream_fprintf(zsm, "welcome:\n");
	}
	zstream_get_delimiter(zsm, zs, '\n');
	if(zstream_feof(zsm)) return (-1);
	zstream_fprintf(zsm, "%s\n", zstr_str(zs));
	zstream_fdclose(zsm);
	zstr_free(zs);
	zevent_request_timer(fd, test_timeout_timer, 0, 10);
	return 0;

}

int main(int argc, char **argv){
	int test_sock;
	
	zvar_verbose=1;
	signal(SIGHUP, SIG_IGN);

	zevent_init();
	
	test_sock=zsocket_inet_listen("127.0.0.1", 99, 1);
	zevent_enable_read(test_sock, test_accept, 0);

	while(1){
		zmsg_debug("loop");
		zevent_loop(-1);
	}
	return 0;
}

