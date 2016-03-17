#include "zc.h"

char *zvar_program_name = 0;
int zvar_ipc_timeout = 60;
int zvar_daemon_mode = 0;
int zvar_max_fd_limit = 10240;

ZEVENT_BASE *zvar_default_event_base = 0;
