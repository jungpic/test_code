/*
 *  nandwrite.c
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *		  2003 Thomas Gleixner (tglx@linutronix.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a NAND flash
 *   chip or NAND chips contained in DoC devices. This is the
 *   "inverse operation" of nanddump.
 *
 * tglx: Major rewrite to handle bad blocks, write data with or without ECC
 *	 write oob data only on request
 *
 * Bug/ToDo:
 */

#define PROGRAM_NAME "nandwrite"

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd-user.h"
#include "mtdutil_common.h"
#include "libmtd.h"

static char tmp_buffer[32];
extern int send_all2avc_cycle_data(char *buf, int maxlen, unsigned short errcode, int errset, int errclear);

//static const char	*standard_input = "-";
static long long	mtdoffset = 0;
static bool		quiet = false;
//static bool		writeoob = false;
static bool		onlyoob = false;
static bool		markbad = false;
//static bool		noecc = false;
//static bool		autoplace = false;
static int		blockalign = 1; /* default to using actual block size */

static bool		pad = false;

static void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0) {
		memset(buffer, kEraseByte, size);
	}
}

/*
 * Main program
 */
int nandwrite_main(char	*mtd_device, char *img)
{
        int ctr = 0; // jongkkim
	int cnt = 0;
	int fd = -1;
	int ifd = -1;
	int imglen = 0, pagelen;
	bool baderaseblock = false;
	long long blockstart = -1;
	struct mtd_dev_info mtd;
	long long offs;
	int ret;
	bool failed = true;
	/* contains all the data read from the file so far for the current eraseblock */
	unsigned char *filebuf = NULL;
	size_t filebuf_max = 0;
	size_t filebuf_len = 0;
	/* points to the current page inside filebuf */
	unsigned char *writebuf = NULL;
	/* points to the OOB for the current page in filebuf */
	//unsigned char *oobbuf = NULL;
	libmtd_t mtd_desc;
	int ebsize_aligned;
	uint8_t write_mode;
	pad = true;

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		printf("Cannnot open %s", mtd_device);
                return -1;
        }

	mtd_desc = libmtd_open();
	if (!mtd_desc) {
		printf("can't initialize libmtd");
                return -1;
        }
	/* Fill in MTD device capability structure */
	if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0) {
		printf("mtd_get_dev_info failed");
                return -1;
        }

	/*
	 * Pretend erasesize is specified number of blocks - to match jffs2
	 *   (virtual) block size
	 * Use this value throughout unless otherwise necessary
	 */
	ebsize_aligned = mtd.eb_size * blockalign;

	mtdoffset = 0; // 2014-12-27

	if (mtdoffset & (mtd.min_io_size - 1)) {
		printf("The start address is not page-aligned !\n"
			   "The pagesize of this NAND Flash is 0x%x.\n",
			   mtd.min_io_size);
                return -1;
        }

#if 0
	/* Select OOB write mode */
	if (noecc)
		write_mode = MTD_OPS_RAW;
	else if (autoplace)
		write_mode = MTD_OPS_AUTO_OOB;
	else
#endif
		write_mode = MTD_OPS_PLACE_OOB;

#if 0
	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW);
		if (ret) {
			switch (errno) {
			case ENOTTY:
				errmsg_die("ioctl MTDFILEMODE is missing");
			default:
				sys_errmsg_die("MTDFILEMODE");
			}
		}
	}
#endif

	/* Determine if we are reading from standard input or from a file. */
	ifd = open(img, O_RDONLY);
	if (ifd == -1) {
		printf("Cannnot open %s\n", img);
		goto closeall;
	}

	//pagelen = mtd.min_io_size + ((writeoob) ? mtd.oob_size : 0);
	pagelen = mtd.min_io_size;

	imglen = lseek(ifd, 0, SEEK_END);
	lseek(ifd, 0, SEEK_SET);

#if 0
	/* Check, if file is page-aligned */
	if ((!pad) && ((imglen % pagelen) != 0)) {
		fprintf(stderr, "Input file is not page-aligned. Use the padding "
				 "option.\n");
		goto closeall;
	}
#endif

	/* Check, if length fits into device */
	if (((imglen / pagelen) * mtd.min_io_size) > (mtd.size - mtdoffset)) {
		fprintf(stderr, "Image %d bytes, NAND page %d bytes, OOB area %d"
				" bytes, device size %lld bytes\n",
				imglen, pagelen, mtd.oob_size, mtd.size);
		printf("Input file does not fit into device");
		goto closeall;
	}

	/*
	 * Allocate a buffer big enough to contain all the data (OOB included)
	 * for one eraseblock. The order of operations here matters; if ebsize
	 * and pagelen are large enough, then "ebsize_aligned * pagelen" could
	 * overflow a 32-bit data type.
	 */
	filebuf_max = ebsize_aligned / mtd.min_io_size * pagelen;
	filebuf = xmalloc(filebuf_max);
	erase_buffer(filebuf, filebuf_max);

	/*
	 * Get data from input and write to the device while there is
	 * still input to read and we are still within the device
	 * bounds. Note that in the case of standard input, the input
	 * length is simply a quasi-boolean flag whose values are page
	 * length or zero.
	 */
	while (((imglen > 0) || (writebuf < (filebuf + filebuf_len)))
		&& (mtdoffset < mtd.size)) {
		/*
		 * New eraseblock, check for bad block(s)
		 * Stay in the loop to be sure that, if mtdoffset changes because
		 * of a bad block, the next block that will be written to
		 * is also checked. Thus, we avoid errors if the block(s) after the
		 * skipped block(s) is also bad (number of blocks depending on
		 * the blockalign).
		 */
		while (blockstart != (mtdoffset & (~ebsize_aligned + 1))) {
			blockstart = mtdoffset & (~ebsize_aligned + 1);
			offs = blockstart;

			/*
			 * if writebuf == filebuf, we are rewinding so we must
			 * not reset the buffer but just replay it
			 */
			if (writebuf != filebuf) {
				erase_buffer(filebuf, filebuf_len);
				filebuf_len = 0;
				writebuf = filebuf;
			}

			baderaseblock = false;
			if (!quiet)
				fprintf(stdout, "Writing data to block %lld at offset 0x%llx\n",
						 blockstart / ebsize_aligned, blockstart);

ctr++;
if (ctr >= 4) {
    ctr = 0;
    printf("%s(), send alive\n", __func__);
    send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0); // Ver 0.71, 2014/01/15
}

			/* Check all the blocks in an erase block for bad blocks */
			do {
				if ((ret = mtd_is_bad(&mtd, fd, offs / ebsize_aligned)) < 0) {
					printf("%s: MTD get bad block failed", mtd_device);
					goto closeall;
				} else if (ret == 1) {
					baderaseblock = true;
					if (!quiet)
						fprintf(stderr, "Bad block at %llx, %u block(s) "
								"from %llx will be skipped\n",
								offs, blockalign, blockstart);
				}

				if (baderaseblock) {
					mtdoffset = blockstart + ebsize_aligned;
				}
				offs +=  ebsize_aligned / blockalign;
			} while (offs < blockstart + ebsize_aligned);

		}

		/* Read more data from the input if there isn't enough in the buffer */
		if ((writebuf + mtd.min_io_size) > (filebuf + filebuf_len)) {
			int readlen = mtd.min_io_size;

			int alreadyread = (filebuf + filebuf_len) - writebuf;
			int tinycnt = alreadyread;

			while (tinycnt < readlen) {
				cnt = read(ifd, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { /* EOF */
					break;
				} else if (cnt < 0) {
					printf("File I/O error on input");
					goto closeall;
				}
				tinycnt += cnt;
			}

			/* No padding needed - we are done */
			if (tinycnt == 0)
				break;

			/* Padding */
			if (tinycnt < readlen) {
#if 0
				if (!pad) {
					fprintf(stderr, "Unexpected EOF. Expecting at least "
							"%d more bytes. Use the padding option.\n",
							readlen - tinycnt);
					goto closeall;
				}
#endif
				erase_buffer(writebuf + tinycnt, readlen - tinycnt);
			}

			filebuf_len += readlen - alreadyread;
			if (ifd != STDIN_FILENO) {
				imglen -= tinycnt - alreadyread;
			}
			else if (cnt == 0) {
				/* No more bytes - we are done after writing the remaining bytes */
				imglen = 0;
			}
		}

#if 0
		if (writeoob) {
			oobbuf = writebuf + mtd.min_io_size;

			/* Read more data for the OOB from the input if there isn't enough in the buffer */
			if ((oobbuf + mtd.oob_size) > (filebuf + filebuf_len)) {
				int readlen = mtd.oob_size;
				int alreadyread = (filebuf + filebuf_len) - oobbuf;
				int tinycnt = alreadyread;

				while (tinycnt < readlen) {
					cnt = read(ifd, oobbuf + tinycnt, readlen - tinycnt);
					if (cnt == 0) { /* EOF */
						break;
					} else if (cnt < 0) {
						perror("File I/O error on input");
						goto closeall;
					}
					tinycnt += cnt;
				}

				if (tinycnt < readlen) {
					fprintf(stderr, "Unexpected EOF. Expecting at least "
							"%d more bytes for OOB\n", readlen - tinycnt);
					goto closeall;
				}

				filebuf_len += readlen - alreadyread;
				if (ifd != STDIN_FILENO) {
					imglen -= tinycnt - alreadyread;
				}
				else if (cnt == 0) {
					/* No more bytes - we are done after writing the remaining bytes */
					imglen = 0;
				}
			}
		}
#endif

		/* Write out data */
		ret = mtd_write(mtd_desc, &mtd, fd, mtdoffset / mtd.eb_size,
				mtdoffset % mtd.eb_size,
				onlyoob ? NULL : writebuf,
				onlyoob ? 0 : mtd.min_io_size,
				/*writeoob ? oobbuf :*/ NULL,
				/*writeoob ? mtd.oob_size :*/ 0,
				write_mode);
		if (ret) {
			int i;
			if (errno != EIO) {
				printf("%s: MTD write failure", mtd_device);
				goto closeall;
			}

			/* Must rewind to blockstart if we can */
			writebuf = filebuf;

			fprintf(stderr, "Erasing failed write from %#08llx to %#08llx\n",
				blockstart, blockstart + ebsize_aligned - 1);
			for (i = blockstart; i < blockstart + ebsize_aligned; i += mtd.eb_size) {
				if (mtd_erase(mtd_desc, &mtd, fd, i / mtd.eb_size)) {
					int errno_tmp = errno;
					printf("%s: MTD Erase failure", mtd_device);
					if (errno_tmp != EIO) {
						goto closeall;
					}
				}
			}

			if (markbad) {
				fprintf(stderr, "Marking block at %08llx bad\n",
						mtdoffset & (~mtd.eb_size + 1));
				if (mtd_mark_bad(&mtd, fd, mtdoffset / mtd.eb_size)) {
					printf("%s: MTD Mark bad block failure", mtd_device);
					goto closeall;
				}
			}
			mtdoffset = blockstart + ebsize_aligned;

			continue;
		}
		mtdoffset += mtd.min_io_size;
		writebuf += pagelen;
	}

	failed = false;

closeall:
	close(ifd);
	libmtd_close(mtd_desc);
	free(filebuf);
	close(fd);

	if (failed || ((ifd != STDIN_FILENO) && (imglen > 0))
		   || (writebuf < (filebuf + filebuf_len))) {
		printf("Data was only partially written due to error");
                return -1;
        }

	/* Return happy */
	//return EXIT_SUCCESS;
	return 0;
}

