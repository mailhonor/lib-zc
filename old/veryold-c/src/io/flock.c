#include "zc.h"

int zio_flock(int fd, int flags)
{
	int ret;
	while ((ret = flock(fd, flags)) < 0 && errno == EINTR)
		sleep(1);

	return ret;
}
