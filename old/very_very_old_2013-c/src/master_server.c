#include "zyc.h"
#define zmaster_entry_info(m) zmsg_debug("master entry: %s,%s, limit:%d,count:%d,avail:%d", \
	       	m->service,m->cmd, m->proc_limit, m->proc_count, m->proc_avail)

typedef struct {
	char *service;
	int  used;
	int  drop;
	char *name;
	char *type;
	char *ip;
	int port;
	char *path;
	int proc_limit;
	int proc_count;
	int proc_avail;
	int wakeup;
	char *cmd;
	ZARGV *args;
	int sock_fd;
	int status_fd[2];
	int idx;
} MASTER_ENTRY;

typedef struct {
	int avail;
	int use_count;
	MASTER_ENTRY *mnode;
} MASTER_PID;
char *z_master_config_path=0;

#define z_master_entry_max 128
static MASTER_ENTRY *z_master_entry_list =0;
static ZHASH *z_zevent_child_list=0;
static int z_master_lock_fd;
static int z_master_pid;
static int z_master_sig_hup;
static int z_master_sig_child;
static int z_master_daemon_mode=0;
static char *z_master_script_cmd;

static int zmaster_start_child(int rwe, int fd, char *ctx);
static int zmaster_start_wakeup(int rwe, int fd, char *ctx);
static int zmaster_child_status_deal(int rwe, int fd, char *ctx);
static void zmaster_sig_exit(int sig);
static void zmaster_sig_hup(int sig);
static void zmaster_sig_child(int sig);

void zmaster_server_init(void){
	int i;
	MASTER_ENTRY *mnode;

	if(z_master_entry_list){
		return;
	}

	zevent_init();
	z_zevent_child_list=zhash_create(1024);

	z_master_entry_list = (MASTER_ENTRY *)z_malloc(sizeof(MASTER_ENTRY) * z_master_entry_max);
	memset(z_master_entry_list, 0, sizeof(MASTER_ENTRY) * z_master_entry_max);
	for(i=0;i<z_master_entry_max;i++){
		mnode = z_master_entry_list + i;
		mnode->used=0;
		mnode->sock_fd=-1;
		mnode->status_fd[0]=mnode->status_fd[1]=-1;
		mnode->args=zargv_create(5);
	}
	
	/* SIG deal */
	struct sigaction action;
	int sigs[] = { SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGSEGV, SIGTERM};

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	action.sa_handler = zmaster_sig_exit;
	for(i=0; i<sizeof(sigs)/sizeof(sigs[0]);i++){
		if (sigaction(sigs[i], &action, (struct sigaction *) 0) < 0){
			zmsg_fatal("%s: sigaction(%d) : %m", __FUNCTION__, sigs[i]);
		}
	}

	action.sa_handler = zmaster_sig_hup;
	if (sigaction(SIGHUP, &action, (struct sigaction *) 0) < 0){
		zmsg_fatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
	}

	action.sa_handler = zmaster_sig_child;
	if (sigaction(SIGCHLD, &action, (struct sigaction *) 0) < 0){
		zmsg_fatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGCHLD);
	}
}

static void zmaster_sig_exit(int sig){

	if(kill(-z_master_pid, SIGTERM) <0 ){
		zmsg_fatal("zmaster_sig_exit: kill process group: %m");
	}
	zmsg_info("now exit");
	exit(1);
}

static void zmaster_sig_hup(int sig){
	zmsg_info("now reload");
	z_master_sig_hup=sig;
}

static void zmaster_sig_child(int sig){
	z_master_sig_child=sig;
}

static void zmaster_child_list_enter(int pid, MASTER_ENTRY *en){
	char buf[16];
	MASTER_PID *mp;

	mp=(MASTER_PID *)z_malloc(sizeof(MASTER_PID));
	mp->avail=1;
	mp->use_count=0;
	mp->mnode=en;
	sprintf(buf, "%d", pid);
	zhash_enter(z_zevent_child_list, buf, (char *)mp);
}

static void zmaster_child_list_remove(int pid){
	char buf[16], *v;

	sprintf(buf, "%d", pid);
	if(zhash_delete(z_zevent_child_list, buf, &v)){
		z_free(v);
	}
}

static MASTER_PID *zmaster_child_list_get(int pid){
	char buf[16];
	char *v;

	sprintf(buf, "%d", pid);
	if(zhash_find(z_zevent_child_list, buf, &v)){
		return (MASTER_PID *)v;
	}
	return 0;
}

static MASTER_ENTRY *zmaster_load_entry(ZARGV *zag, int line_number){
	char *name,*type,*cmd, *config_file;
	int i, idx=0;
	MASTER_ENTRY *men, *mnode;
	
	men = 0;
	name=zag->argv[0];
	type=zag->argv[1];
	cmd =zag->argv[4];
	config_file =zag->argv[5];
	for(i=0;i<z_master_entry_max;i++){
		mnode=z_master_entry_list+i;
		if(mnode->used==0) continue;
		if(!strcmp(mnode->name, name) && !(strcmp(mnode->type, type))){
			men=mnode;
			idx=i;
			break;
		}
	}
	if(!men){
		for(i=0;i<z_master_entry_max;i++){
			mnode=z_master_entry_list+i;
			if(mnode->used) continue;
			men=mnode;
			idx=i;
			men->proc_count=0;
			men->proc_avail=0;
			break;
		}
	}
	if(!men){
		zmsg_fatal("master.cf config error: too many entrys!!!");
	}
	men->proc_limit=atoi(zag->argv[2]);
	men->wakeup=atoi(zag->argv[3]);
	men->used=1;
	men->drop=0;
	men->idx=idx;

	if(men->service)z_free(men->service);
	if(men->name)z_free(men->name);
	if(men->type)z_free(men->type);
	if(men->cmd)z_free(men->cmd);
	men->service=z_strdup(zstr_str(zstr_fconcatenate("%s:%s", type,name)));
	men->cmd=z_strdup(cmd);
	men->name=z_strdup(name);
	men->type=z_strdup(type);

	zargv_reset(men->args);
	zargv_add(men->args, cmd);
	zargv_add(men->args, zstr_str(zstr_fconcatenate("-t=%s", type)));
	zargv_add(men->args, zstr_str(zstr_fconcatenate("-c=%s", config_file)));

	if(men->path)z_free(men->path);men->path=0;
	if(men->ip)z_free(men->ip);men->ip=0;
	if(!strcmp(type, "unix")){
		men->path=z_strdup(zstr_str(zstr_fconcatenate("zsockets/%s", name)));
	}else if(!strcmp(type, "fifo")){
		men->path=z_strdup(zstr_str(zstr_fconcatenate("zsockets/%s", name)));
	}else if(!strcmp(type, "inet")){
		char *p, *tbuf;
		tbuf=z_strdup(name);
		p=strchr(tbuf, ':');
		if(p){
			*p=0;
			men->ip=z_strdup(tbuf);
			p++;
			men->port=atoi(p);
		}else{
			men->ip=0;
			men->port=atoi(tbuf);
		}
		z_free(tbuf);
	}else{
		zmsg_fatal("master.cf config error at line %d, type error,should be inet/unix/fifo", line_number);
	}
	return(men);
}

static void zmaster_load_config(void){
	FILE *mfp;
	char *buf, *endpoint, *lline;
	int i,line_number;
	ZARGV *zag;
	ZHASH *en_h;
	MASTER_ENTRY *men;

	en_h=zhash_create(128);
	zag=zargv_create(36);
	mfp=fopen("master.cf", "r");
	buf=(char *)malloc(10240);
	endpoint=(char *)malloc(1024);

	for(i=0;i<z_master_entry_max;i++){
		men=z_master_entry_list+i;
		if(men->used==0)continue;
		men->drop=1;
	}
	men=0;
	line_number=0;
	while(!feof(mfp)){
		zargv_reset(zag);
		if(fgets(buf, 10240, mfp)==NULL)
			break;
		line_number++;
		zargv_split_append(zag, buf, " \t\r\n");
		if(ZARGV_LEN(zag) == 0 ) continue;
		else if(zag->argv[0][0]=='#') continue;
		if(*buf==' ' || *buf=='\t'){
			if(!men){
				zmsg_fatal("master.cf error at line: %d, no main config before.", line_number);
			}
			for(i=0;i<ZARGV_LEN(zag);i++){
				if(zag->argv[0][0]=='#') break;
				zargv_add(men->args, zag->argv[i]);
			}
		}else if(ZARGV_LEN(zag) < 5 ) {
			zmsg_fatal("master.cf error at line: %d", line_number);
		}else{
			sprintf(endpoint, "%s:%s", zag->argv[1], zag->argv[0]);
			if(zhash_find(en_h, endpoint, &lline)){
				zmsg_fatal("master.cf error at line: %d, repeated line: %d",line_number, Z_CHAR_PTR_TO_INT(lline));
			}
			zhash_enter(en_h, endpoint, Z_INT_TO_CHAR_PTR(line_number));
			if(ZARGV_LEN(zag)==5) zargv_add(zag, "none");
			men=zmaster_load_entry(zag, line_number);
		}
	}
	zhash_free(en_h, 0, 0);
	zargv_free(zag, 0, 0);
	fclose(mfp);
	free(buf);
	free(endpoint);
}

static void zmaster_start_service(void){
	int i;
	MASTER_ENTRY *mnode;

	for(i=0;i<z_master_entry_max;i++){
		mnode=z_master_entry_list+i;
		if(mnode->used==0) continue;
		if(mnode->sock_fd ==-1){
			if(!strcmp(mnode->type, "inet")){
				mnode->sock_fd=zsocket_inet_listen(mnode->ip, mnode->port, mnode->proc_limit);
			}else if(!strcmp(mnode->type, "unix")){
				mnode->sock_fd=zsocket_unix_listen(mnode->path, mnode->proc_limit);
			}else if(!strcmp(mnode->type, "fifo")){
				mnode->sock_fd=zsocket_fifo_listen(mnode->path);
			}
			zevent_enable_read(mnode->sock_fd, zmaster_start_child, (char *)mnode);
			zmsg_debug("zmaster: start service:%s, cmd:%s", mnode->service, mnode->cmd);
		}
		if(mnode->wakeup){
			zevent_request_timer(mnode->sock_fd, zmaster_start_wakeup, (char *)mnode, 0);
		}else{
			zevent_cancel_timer(mnode->sock_fd, zmaster_start_wakeup, (char *)mnode);
		}
		if(mnode->status_fd[0]!=-1){
			zevent_disable_readwrite(mnode->status_fd[0]);
			zmsg_debug("zmaster: notify: %s to reload", mnode->service);
			close(mnode->status_fd[0]);
			close(mnode->status_fd[1]);
		}
		if(pipe(mnode->status_fd)==-1){
			zmsg_fatal("zmaster: pipe : %m");
		}
		zio_close_on_exec(mnode->status_fd[0], 1); zio_close_on_exec(mnode->status_fd[1], 1);
		zevent_enable_read(mnode->status_fd[0], zmaster_child_status_deal, 0);
		zmsg_verbose("zmaster: monitor: %s'status for avail or taken", mnode->service);
		zmaster_entry_info(mnode);
	}
}

static void zmaster_stop_service(void){
	int i, pid;
	MASTER_ENTRY *mnode;
	MASTER_PID *mpid;
	ZHASH_NODE **pid_list, **pid_list2;

	pid_list = pid_list2 = zhash_list(z_zevent_child_list);
	for(;*pid_list;pid_list++){
		mpid=(MASTER_PID *)(pid_list[0]->value);
		mnode=mpid->mnode;
		if(mnode->used==0)continue;
		if(mnode->drop==0)continue;
		pid=atoi(pid_list[0]->key);
		zmaster_child_list_remove(pid);
		zmsg_debug("send SIGTERM to child %d", pid); 
		kill(pid, SIGTERM);
	}

	for(i=0;i<z_master_entry_max;i++){
		MASTER_ENTRY *mnode;
		mnode=z_master_entry_list+i;
		if(mnode->used==0) continue;
		if(mnode->drop==0) continue;
		if(mnode->sock_fd!=-1){
			zevent_disable_readwrite(mnode->sock_fd);
			zevent_cancel_timer(mnode->sock_fd, zmaster_start_wakeup, (char *)mnode);
			close(mnode->sock_fd);
			mnode->sock_fd=-1;
		}
		if(mnode->status_fd[0]!=-1){
			zevent_disable_readwrite(mnode->status_fd[0]);
			close(mnode->status_fd[0]);
			close(mnode->status_fd[1]);
			mnode->status_fd[0]=mnode->status_fd[1]=-1;
		}
		mnode->used=0;
		mnode->drop=0;
		mnode->proc_limit=0;
		mnode->proc_avail=0;
		mnode->proc_count=0;
	}

	z_free(pid_list2);
}
static int zmaster_start_child(int rwe, int fd, char *ctx){
	int pid;
	MASTER_ENTRY *mnode;

	mnode=(MASTER_ENTRY *)ctx;
	if(mnode->proc_avail){
		zmsg_debug("master: %s proc_avail: %d > 0, ignore the require", mnode->service, mnode->proc_avail);
		zmaster_entry_info(mnode);
		return 0;
	}
	
	pid=fork();
	if(pid==-1){
		return 0;
	}else if(pid){
		mnode->proc_count++;
		mnode->proc_avail++;
		if(mnode->proc_count >= mnode->proc_limit){
			zevent_disable_readwrite(fd);
			zmsg_debug("master: %s, proc_limit up to max, disable_readwrite", mnode->service);
		}else{
		}
		zmaster_entry_info(mnode);
		zmaster_child_list_enter(pid, mnode);
		return 0;
	}
	dup2(fd, ZMASTER_LISTEN_FD); zio_close_on_exec(fd, 1);
	dup2(mnode->status_fd[1], ZMASTER_STATUS_FD); zio_close_on_exec(mnode->status_fd[1], 1);
	execvp(mnode->cmd, mnode->args->argv);
	zmsg_fatal("zmaster_start_child error: %m");
	return 0;
}

static void zmaster_reap_child(void){
	pid_t pid;
	int status;
	MASTER_ENTRY *mnode;
	MASTER_PID *mp;

	while ((pid=waitpid((pid_t) -1, &status, WNOHANG))>0){
		zmsg_verbose("master: child exit, pid: %d", pid);
		mp=zmaster_child_list_get(pid);
		if(mp==0) continue;
		mnode=mp->mnode;
		if(mnode->used == 0 )continue;
		mnode->proc_count--;
		if(mp->avail == 1){
			mnode->proc_avail--;
		}
		zmaster_child_list_remove(pid);
		if(mnode->proc_avail==0){
			zevent_enable_read(mnode->sock_fd, zmaster_start_child, (char *)mnode);
			zmsg_verbose("master: %s, enable_read", mnode->service);
		}
		if(mnode->proc_count == 0 && mnode->wakeup > 0){
			zevent_request_timer(mnode->sock_fd, zmaster_start_wakeup, (char *)mnode, 0);
			zmsg_verbose("master: %s, wakeup", mnode->service);
		}
		zmaster_entry_info(mnode);
	}
}

static int zmaster_child_status_deal(int rwe, int fd, char *ctx){
	struct {
		int pid;
		int magic:24;
		int status:8;
	} _status;
	int n, avail;
	MASTER_PID *mp;
	MASTER_ENTRY *mnode;

	if(rwe==ZEVENT_EXCEPTION){
	}

	n=read(fd, &_status, sizeof(_status));

	if(n==-1){
		zmsg_warning("%s: read: %m", __FUNCTION__);
		return -1;
	}
	if(n==0){
		zmsg_warning("%s: read EOF status", __FUNCTION__);
		return -1;
	}
	if(n!=sizeof(_status)){
		zmsg_warning("%s: read partial status(%d bytes)", __FUNCTION__, n);
		return -1;
	}

	zmsg_verbose("master: get child:%d, status: %d ", _status.pid, _status.status);

	mp=zmaster_child_list_get(_status.pid);
	if(mp==0) return 0;
	mnode=mp->mnode;
	if(mnode->used == 0 ) return 0;
	if( (_status.magic & 0x00FFFFFF) != 0xbeefac){
		zmsg_fatal("%s: read confused, readed magic: %x",__FUNCTION__, _status.magic);
	}
	avail=1-_status.status;
	if(mp->avail == avail){
		return 0;
	}
	mp->avail=avail;
	if(mp->avail == 1){
		mnode->proc_avail++;
	}else{
		mnode->proc_avail--;
	}
	zmaster_entry_info(mnode);
	return 0;
}

static int zmaster_start_wakeup(int rwe, int fd, char *ctx){
	MASTER_ENTRY *mnode;

	mnode=(MASTER_ENTRY *)ctx;

	if(mnode->proc_count){
		return 0;
	}
	zmaster_start_child(0, fd, ctx);
	return 0;
}

static int zmaster_start(void){
	z_master_pid = getpid();
	zmsg_info("zmster_start");

	zmaster_server_init();
	zmaster_load_config();
	zmaster_start_service();
	zmaster_stop_service();

	z_master_sig_hup=0;
	z_master_sig_child=0;

	while(1){
		zevent_loop(-1);
		if(z_master_sig_hup){
			zmsg_info("zmster_reload");
			z_master_sig_hup=0;
			zmaster_load_config();
			zmaster_start_service();
			zmaster_stop_service();
		}
		if(z_master_sig_child){
			z_master_sig_child=0;
			zmaster_reap_child();
		}
	}
	return 0;
}

static void zmaster_usage(char *cmd){
	fprintf(stderr, "\nUSAGE:\n    %s -C workpath  start/stop/reload\n", cmd);
	exit(1);
}

int zmaster_server(int argc, char **argv){
	int i;
	char *arg;
	char pidp[10];
	int pid, ret, killsig;
	char *cmd_name;

	signal(SIGPIPE, SIG_IGN);

	z_master_pid = getpid();

	cmd_name=argv[0];
	z_master_script_cmd = 0;
	for(i=1;i<argc;i++){
		arg=argv[i];
		if(!strcasecmp(arg, "-c")){
			z_master_config_path = z_strdup(argv[i+1]);
			i++;
		}else if(!strcasecmp(arg, "-d")){
			z_master_daemon_mode=1;
		}else{
			if(z_master_script_cmd){
				zmaster_usage(cmd_name);
			}
			z_master_script_cmd=z_strdup(arg);
		}
	}
	if(z_master_script_cmd == 0){
		z_master_script_cmd=z_strdup("start");
	}
	if(z_master_config_path == 0){
		zmaster_usage(cmd_name);
	}

	if(chdir(z_master_config_path)==-1){
		fprintf(stderr, "the system error: cannot chdir: %s  (%m)\n", z_master_config_path);
		exit(1);
	}

	mkdir("zsockets",0770);
	mkdir("zfiles",0770);
	umask(007);

	z_master_lock_fd=open("zfiles/lock", O_RDWR|O_CREAT, 0770);
	if(z_master_lock_fd==-1){
		fprintf(stderr, "master open lock error: %m\n");
		exit(1);
	}
	zio_close_on_exec(z_master_lock_fd, 1);

	ret = flock(z_master_lock_fd, LOCK_EX|LOCK_NB);
	if(ret == 0){
		if(!strcasecmp(z_master_script_cmd, "start")){
			fprintf(stderr, "the system start now!!!\n");
			int wl;
			if(daemon(1,0)); if(setsid());
			dup2(z_master_lock_fd, 3);
			for(i=4;i<100;i++){
				close(i);
			}
			wl=snprintf(pidp, 10, "%d          ", getpid());
			wl=write(z_master_lock_fd, pidp, wl);
			return zmaster_start();
		}else{
			fprintf(stderr, "the system do not run!!!\n");
			exit(1);
		}
	}else{
		if(errno != EWOULDBLOCK){
			fprintf(stderr, "the system error: %m, this is should be a temp fault, please repeat the action.\n");
			exit(1);
		}
	}

	killsig=0;
	if(!strcasecmp(z_master_script_cmd, "start")){
		fprintf(stderr, "the system is already runing!!!\n");
		exit(0);
	}else if(!strcasecmp(z_master_script_cmd, "reload")){
		killsig=SIGHUP;
	}else if(!strcasecmp(z_master_script_cmd, "stop")){
		killsig=SIGTERM;
	}else{
		zmaster_usage(cmd_name);
	}

	if(read(z_master_lock_fd, pidp, 10)==-1);
	pid=atoi(pidp);
	kill(pid, killsig);
	fprintf(stderr, "the system %s ok!!!\n", z_master_script_cmd);
	return 0;
}

