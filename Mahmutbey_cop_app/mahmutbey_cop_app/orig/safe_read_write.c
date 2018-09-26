#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int safe_read(int fd, void *buf, int count)
{
	int n;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

//printf("safe_read(), count:%d, return: %d\n", count, n);
	return n;
}

int safe_write(int fd, const void *buf, int count)
{
	ssize_t n;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

//printf("safe_write(), count: %d, return: %d\n", count, n);
	return n;
}

/*
 * Write all of the supplied buffer out to a file.
 * This does multiple writes as necessary.
 * Returns the amount written, or -1 on an error.
 */
int full_write(int fd, const void *buf, int len)
{
	int cc;
	int total;

	total = 0;

	while (len) {
		cc = safe_write(fd, buf, len);

		if (cc < 0) {
			if (total) {
				/* we already wrote some! */
				/* user can do another write to know the error code */
				return total;
			}
			return cc;  /* write() returns -1 on failure. */
		}

		total += cc;
		buf = ((const char *)buf) + cc;
		len -= cc;
	}

	return total;
}

