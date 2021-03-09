#include <zyc.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

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
	zevent_enable_read(fd, test_accept_after, 0);
	zevent_request_timer(fd, test_timeout_timer, 0, 10);
	zmsg_debug("accept : times: %d  fd:%d", accept_times++,fd);
	return 0;
}

int test_accept_after(int rwe, int fd, char *ctx){

	zmsg_debug("accept after");
	zevent_cancel_timer(fd, test_timeout_timer, 0);
	if(zio_peek(fd)<=0){
		zmsg_debug("test_accept_after: over, fd:%d", fd);
		zevent_disable_readwrite(fd);
		proc_num--;
		close(fd);
		return 0;
	}
	proc_num++;
	if(rwe==ZEVENT_EXCEPTION){
		zmsg_fatal("%s: get exception from zevent, fd:%d", __FUNCTION__, fd);
	}

	ZSTREAM *zsm;
	ZSTR *zs;

	zs=zstr_create(1024);
	zsm=zstream_fdopen(fd, 10);
	zstream_get_delimiter(zsm, zs, '\n');
	if(zstream_feof(zsm)) return (-1);
	zstream_fprintf(zsm, "%s\n", zstr_str(zs));
	zstream_fdclose(zsm);
	zstr_free(zs);
	zevent_request_timer(fd, test_timeout_timer, 0, 10);
	return 0;

}

int test_stdout(int rwe, int fd, char *ctx){
	char buf[1024];
	int len;

	len=sprintf(buf, "now time: %lu\n", time(0));
	len=write(fd, buf, len);
	sleep(1);
	return 0;
}

int test_status_abort(int rwe, int fd, char *ctx){
	zmsg_debug("main receive child's status, should exit");
	sleep(1);
	return 0;
}

int test_status(void){
	int pid, pfd[2];
	char buf[1024];

	if(pipe(pfd));
	pid=fork();
	if(pid){
		close(pfd[0]);
		zevent_enable_read(pfd[1], test_status_abort, 0);
		if(write(pfd[1], "aaaabbbb", 5));
	}else{
		close(pfd[1]);
		if(read(pfd[0], buf, 5));
		buf[5]=0;
		zmsg_debug("child read: %s, now sleep 10s, then close self read end.", buf);
		sleep(10);
		close(pfd[0]);
		sleep(5);
		zmsg_debug("child exist");
		exit(0);
	}
	return 0;
}

int main(int argc, char **argv){
	int test_sock;
	
	zvar_verbose=1;
	signal(SIGHUP, SIG_IGN);

	zevent_init();
	
	test_sock=zsocket_inet_listen("127.0.0.1", 99, 1);
	zevent_enable_read(test_sock, test_accept, 0);

	if(argc>1){
		char *doo=argv[1];
		if(!strcmp(doo, "stdout")){
			zevent_enable_write(1, test_stdout, 0);
		}else if(!strcmp(doo, "status")){
			test_status();
		}
	}

	while(1){
		zmsg_debug("loop");
		zevent_loop(-1);
	}
	return 0;
}

