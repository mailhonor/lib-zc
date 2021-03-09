#include "zyc.h"

volatile int z_msg_fatal_times=0;
#ifdef ZYC_DEV
int zvar_debug=1;
#else
int zvar_debug=0;
#endif
int zvar_verbose=0;
static ZMSG_OUTPUT_FN zmsg_output_fn=0;

void zmsg_set_output(ZMSG_OUTPUT_FN fn){
	zmsg_output_fn=fn;
}

void zmsg_fatal_printf(int level, int who, char *fmt, ...){
	va_list ap;

	if(z_msg_fatal_times++==0){
		va_start(ap, fmt);
		if(zmsg_output_fn){
			zmsg_output_fn(level, who, fmt, ap);
		}else{
			zmsg_vprintf(level, who, fmt, ap);
		}
		va_end(ap);
	}
	sleep(1);
	_exit(1);
}

void zmsg_printf(int level, int who, char *fmt, ...){
	va_list ap;

	va_start(ap, fmt);
	if(zmsg_output_fn){
		zmsg_output_fn(level, who, fmt, ap);
	}else{
		zmsg_vprintf(level, who, fmt, ap);
	}
	va_end(ap);
}

void zmsg_vprintf(int level, int who, char *fmt, va_list ap){
	char *z_msg_attrs[]={"EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG"};
	static char *progname=0;
	static pid_t pid=0;

	if(level > LOG_DEBUG)
		level=LOG_DEBUG;
	if(progname==0){
		progname=zvar_program_name;
		if(progname==NULL){
			progname =getenv("_");
		}
		if(progname==NULL){
			progname = "";
		}
		progname=z_strdup(progname);
	}
	if(pid==0){
		pid=getpid();
	}
	fprintf(stderr, "%s[%d]: %s %s ", progname, pid, (who?"SYSTEM":"USER"), z_msg_attrs[level]);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "%s", "\n");
	fflush(stderr);
}
