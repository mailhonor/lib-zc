#include "zc.h"

ssize_t zio_peek(int fd)
{
	int count;

	return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
}
