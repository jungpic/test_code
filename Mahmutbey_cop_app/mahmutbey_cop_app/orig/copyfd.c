
/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2005 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

//#include "libbb.h"

/* Used by NOFORK applets (e.g. cat) - must not use xmalloc.
 * size < 0 means "ignore write errors", used by tar --to-command
 * size = 0 means "copy till EOF"
 */
#include <stdio.h>
#include <stdlib.h>

#include "copyfd.h"

char ftp_file_buffer[CONFIG_COPYBUF_SIZE];

extern int safe_read(int fd, void *buf, int count);
extern int full_write(int fd, const void *buf, int len);

static int bb_full_fd_action(int src_fd, int dst_fd, int size)
{
	int status = -1;
	int total = 0;
	int buffer_size = CONFIG_COPYBUF_SIZE;

	if (size < 0) {
            return -1;
	}

	if (src_fd < 0)
		goto out;

	if (!size) {
		size = buffer_size;
		status = 1; /* copy until eof */
	}

	while (1) {
		int rd;

		rd = safe_read(src_fd, ftp_file_buffer, size > buffer_size ? buffer_size : size);

		if (!rd) { /* eof - all done */
			status = 0;
			break;
		}
		if (rd < 0) {
			printf("ERROR, safe_read\n");
			break;
		}
		/* dst_fd == -1 is a fake, else... */
		if (dst_fd >= 0) {
			int wr = full_write(dst_fd, ftp_file_buffer, rd);
			if (wr < rd) {
				printf("ERROR, full_write\n");
				dst_fd = -1;
				break;
			}
		}
		total += rd;
		if (status < 0) { /* if we aren't copying till EOF... */
			size -= rd;
			if (!size) {
				/* 'size' bytes copied - all done */
				status = 0;
				break;
			}
		}
	}
 out:

	return status ? -1 : total;
}


int  bb_copyfd_size(int fd1, int fd2, int size)
{
	if (size) {
		return bb_full_fd_action(fd1, fd2, size);
	}
	return 0;
}

#if 0
void FAST_FUNC bb_copyfd_exact_size(int fd1, int fd2, off_t size)
{
	off_t sz = bb_copyfd_size(fd1, fd2, size);
	if (sz == (size >= 0 ? size : -size))
		return;
	if (sz != -1)
		bb_error_msg_and_die("short read");
	/* if sz == -1, bb_copyfd_XX already complained */
	xfunc_die();
}
#endif

int  bb_copyfd_eof(int fd1, int fd2)
{
	return bb_full_fd_action(fd1, fd2, 0);
}
