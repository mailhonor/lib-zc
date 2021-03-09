#include "zyc.h"

static time_t z_event_present_time=0;

static int z_event_epoll_fd=0;

#define z_event_max_use 	256
struct epoll_event z_event_list[256], _z_epoll_tmp, *z_epoll_tmp=&_z_epoll_tmp;

typedef struct {
	ZEVENT_CALLBACK_FN callback;
	char *context;
	char flag;
} ZEVENT;
#define z_zevent_max_use  	1024
static ZEVENT z_zevent_list[z_zevent_max_use];

#define _ZEVENT_IS_READ(fd)	((z_zevent_list[fd].flag) & ZEVENT_READ)
#define _ZEVENT_IS_WRITE(fd)	((z_zevent_list[fd].flag) & ZEVENT_WRITE)
#define _ZEVENT_IS_RDWR(fd)	((z_zevent_list[fd].flag) & ZEVENT_RDWR == ZEVENT_RDWR)
#define _ZEVENT_IS_MARK(fd)	((z_zevent_list[fd].flag) & ZEVENT_RDWR)
#define _ZEVENT_CLR(fd) 	(z_zevent_list[fd].flag = 0)

typedef struct _ZEVENT_TIMER _ZEVENT_TIMER;
struct _ZEVENT_TIMER {
	int fd;
	ZEVENT_TIMER_CALLBACK_FN callback;
	char *context;
	time_t when;
	int loop_instance;
	_ZEVENT_TIMER *prev;
	_ZEVENT_TIMER *next;
};
static _ZEVENT_TIMER * z_event_timer_head;
static int z_event_timer_num;
static int z_event_loop_instance;
#define _ZEVENT_TIMER_FIRST()	(z_event_timer_head->next!=z_event_timer_head?z_event_timer_head->next:0)

int zevent_init(void){

	if(z_event_epoll_fd)  return 0;

	z_event_epoll_fd = epoll_create(z_event_max_use);
	zio_close_on_exec(z_event_epoll_fd, 1);

	z_event_timer_head=(_ZEVENT_TIMER *)z_malloc(sizeof(_ZEVENT_TIMER));
	z_event_timer_head->prev=z_event_timer_head->next=z_event_timer_head;
	z_event_timer_num=0;
	z_event_loop_instance = 0;
	return 0;
}

int zevent_close(void){
	_ZEVENT_TIMER *node, *nb;

	if(!z_event_epoll_fd)  return 0;
	close(z_event_epoll_fd);
	z_event_epoll_fd=0;
	
	for(node=z_event_timer_head->next;node!=z_event_timer_head;node=nb){
		nb=node->next;
		z_free(node);
	}
	z_free(z_event_timer_head);

	memset(z_zevent_list, 0, sizeof(z_zevent_list));
	return 0;
}

int zevent_enable(int fd, ZEVENT_CALLBACK_FN callback, char *context, int flag){
	int action, events;
	
	if(ZEVENT_IS_READ(flag) && ZEVENT_IS_WRITE(flag)){
		events=EPOLLIN|EPOLLOUT;
	}else if(ZEVENT_IS_READ(flag)){
		events=EPOLLIN;
	}else if(ZEVENT_IS_WRITE(flag)){
		events=EPOLLOUT;
	}else{
		zmsg_error("zevent_enable: fd %d: zevent error", fd);
		return -1;
	}

	z_zevent_list[fd].callback=callback;
	z_zevent_list[fd].context=context;

	if((z_zevent_list[fd].flag & ZEVENT_RDWR) == (flag & ZEVENT_RDWR)){
		return 0;
	}

	action='A';
	if(_ZEVENT_IS_MARK(fd)){
		action='M';
	}
	memset(z_epoll_tmp, 0, sizeof(struct epoll_event));
	z_epoll_tmp->events=EPOLLET | EPOLLRDHUP | events;
	z_epoll_tmp->data.fd=fd;
	if (epoll_ctl(z_event_epoll_fd, (action=='A'?EPOLL_CTL_ADD:EPOLL_CTL_MOD), fd, z_epoll_tmp) == -1) {
		zmsg_fatal("zevent_enable: fd %d: epoll_ctl %s error: %m", fd, (action=='A'?"ADD":"DEL"));
	}
	zmsg_verbose("zevent_enable: fd %d: flag: %d, epoll_ctl %s, events:%X", fd, flag, (action=='A'?"ADD":"DEL"), events);

	z_zevent_list[fd].flag = (flag & ZEVENT_RDWR);

	return 0;
}


int zevent_disable(int fd){

	if(!_ZEVENT_IS_MARK(fd)){
		return 0;
	}
	if (epoll_ctl(z_event_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		zmsg_fatal("zevent_disable: fd %d: epoll_ctl  error: %m", fd);
	}
	_ZEVENT_CLR(fd);
	z_zevent_list[fd].callback=0;
	z_zevent_list[fd].context=0;
	return 0;
}

static void zevent_timer_detach(_ZEVENT_TIMER *timer){
	_ZEVENT_TIMER *next, *prev;
	
	prev=timer->prev;
	next=timer->next;
	
	prev->next=next;
	next->prev=prev;
	timer->next=timer->prev=0;

	z_event_timer_num--;
}

static void zevent_timer_enter(_ZEVENT_TIMER *timer){
	_ZEVENT_TIMER *node;
	
	for(node=z_event_timer_head->next;node!=z_event_timer_head;node=node->next){
		if(timer->when < node->when){
			break;
		}
	}
	timer->next=node;
	timer->prev=node->prev;
	node->prev->next=timer;
	node->prev=timer;
	z_event_timer_num++;
}

time_t zevent_request_timer(int fd, ZEVENT_TIMER_CALLBACK_FN callback, char *context, int delay){
	_ZEVENT_TIMER *timer;

	z_event_present_time=time(0);

	for(timer=z_event_timer_head->next;timer!=z_event_timer_head;timer=timer->next){
		if((timer->fd==fd) && (timer->callback==callback) && (timer->context==context)){
			break;
		}
	}
	if(timer==z_event_timer_head){
		timer=(_ZEVENT_TIMER *)z_malloc(sizeof(_ZEVENT_TIMER));
	}else{
		zevent_timer_detach(timer);
	}

	timer->when=z_event_present_time+delay;
	timer->fd=fd;
	timer->callback=callback;
	timer->context=context;
	timer->loop_instance = z_event_loop_instance;

	zevent_timer_enter(timer);

	return (timer->when);
}

time_t zevent_cancel_timer(int fd, ZEVENT_TIMER_CALLBACK_FN callback, char *context){
	_ZEVENT_TIMER *timer;
	int time_left;

	z_event_present_time=time(0);
	time_left=-1;

	for(timer=z_event_timer_head->next;timer!=z_event_timer_head;timer=timer->next){
		if((timer->fd==fd) && (timer->callback==callback) && (timer->context==context)){
			if((time_left=timer->when-z_event_present_time)<0){
				time_left = 0;
			}
			zevent_timer_detach(timer);
			z_free(timer);
			break;
		}
	}

	return (time_left);
}

int zevent_loop(int delay){
	int nfds,fd,i,zev,ev,epoll_delay,timeout;
	_ZEVENT_TIMER *timer;

	if((timer=_ZEVENT_TIMER_FIRST())){
		z_event_present_time = time(0);
		if((epoll_delay=timer->when - z_event_present_time) < 0){
			epoll_delay = 0;
		}else if(delay >= 0 && epoll_delay > delay){
			epoll_delay=delay;
		}
		zmsg_verbose("zevent_loop: get epoll_delay: %d from zevent_timer", epoll_delay);
	}else{
		epoll_delay=delay;
	}

	if(epoll_delay < 0){
		timeout=-1;
	}else{
		timeout=epoll_delay*1000;
	}

	zmsg_verbose("zevent_loop: delay: %d, epoll_delay: %d ", delay, epoll_delay);
	nfds=epoll_wait(z_event_epoll_fd, z_event_list, z_event_max_use, timeout);
	if(nfds==-1){
		if(errno != EINTR){
			zmsg_fatal("zevent_loop: epoll_wait error: %m");
			return -1;
		}
		return 0;
	}

	z_event_present_time = time(0);
	z_event_loop_instance ++;
	while((timer=_ZEVENT_TIMER_FIRST())){
		if(timer->when > z_event_present_time){
			break;
		}
		if(timer->loop_instance == z_event_loop_instance)
			break;
		zevent_timer_detach(timer);
		timer->callback(ZEVENT_TIMER, timer->fd, timer->context);
		z_free(timer);
	}

	for(i=0;i<nfds;i++){
		fd=z_event_list[i].data.fd;
		ev = z_event_list[i].events;
		zmsg_verbose("zevent_loop: fd: %d, events: %X", fd, ev);
		zev=0;
		if(ev & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)){
			zev=ZEVENT_EXCEPTION;
		}
#ifdef ZYC_ENENT_EXACT_EXCEPTION
		else
#endif
		{
			if((ev & EPOLLIN) && (ev &EPOLLOUT) ){
				zev |= ZEVENT_RDWR;
			}else if(ev & EPOLLIN){
				zev |= ZEVENT_READ;
			}else if(ev & EPOLLOUT){
				zev |= ZEVENT_WRITE;
			}else{
				zev |= ZEVENT_EXCEPTION;
			}
		}
		z_zevent_list[fd].callback(zev, fd, z_zevent_list[fd].context);
	}
	return 0;
}
