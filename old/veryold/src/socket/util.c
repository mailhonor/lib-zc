#include "zc.h"

int zsocket_parse_path(char *path, int *r_type, char **r_host_path, int *r_port)
{
	int type = 0, port = 0;
	char *host_path = 0, *p = 0;

	type = ZCHAR_TOLOWER(*path);

	if (!(p = strchr(path, ':'))) {
		type = 0;
	} else if ((p++, !*p)) {
		type = 0;
	} else {
		host_path = p;
		if (type == ZSOCKET_TYPE_INET) {
			if (!(p = strchr(host_path, ':'))) {
				port = atoi(host_path);
				host_path = 0;
			} else {
				*p++ = 0;
				port = atoi(p);
			}
		}
	}
	*r_type = type;
	*r_host_path = host_path;
	*r_port = port;

	if (type == 0) {
		return Z_ERR;
	}

	return 0;
}
