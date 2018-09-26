/*
 *  nanddump.c
 *
 *  Copyright (C) 2000 David Woodhouse (dwmw2@infradead.org)
 *                     Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This utility dumps the contents of raw NAND chips or NAND
 *   chips contained in DoC devices.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/types.h>
#include "mtd-user.h"
#include "mtdutil_common.h"
#include "libmtd.h"

// Option variables

static bool		pretty_print = false;	// print nice
static bool		noecc = false;		// don't error correct
static bool		omitoob = true;		// omit oob data
static /*long long*/ int start_addr;		// start address
static /*long long*/ int length;			// dump length
static char		mtddev[16];		// mtd device name
/*static const char	*dumpfile;		// dump file name*/
/*static bool		quiet = false;		// suppress diagnostic output*/
static bool		canonical = false;	// print nice + ascii
/*static bool		forcebinary = false;	// force printing binary to tty*/

static enum {
	padbad,   // dump flash data, substituting 0xFF for any bad blocks
	dumpbad,  // dump flash data, including any bad blocks
	skipbad,  // dump good data, completely skipping any bad blocks
} bb_method = skipbad;

#if 0
static void process_options(int argc, char * const argv[])
{
	int error = 0;
	bool oob_default = true;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "s:f:l:opqnca";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"bb", required_argument, 0, 0},
			{"omitoob", no_argument, 0, 0},
			{"forcebinary", no_argument, 0, 'a'},
			{"canonicalprint", no_argument, 0, 'c'},
			{"file", required_argument, 0, 'f'},
			{"oob", no_argument, 0, 'o'},
			{"prettyprint", no_argument, 0, 'p'},
			{"startaddress", required_argument, 0, 's'},
			{"length", required_argument, 0, 'l'},
			{"noecc", no_argument, 0, 'n'},
			{"quiet", no_argument, 0, 'q'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
			case 0:
				switch (option_index) {
#if 0
					case 0:
						display_help();
						break;
					case 1:
						display_version();
						break;
#endif
					case 2:
						/* Handle --bb=METHOD */
						if (!strcmp(optarg, "padbad"))
							bb_method = padbad;
						else if (!strcmp(optarg, "dumpbad"))
							bb_method = dumpbad;
						else if (!strcmp(optarg, "skipbad"))
							bb_method = skipbad;
						else
							error++;
						break;
					case 3: /* --omitoob */
						if (oob_default) {
							oob_default = false;
							omitoob = true;
						} else {
							errmsg_die("--oob and --oomitoob are mutually exclusive");
						}
						break;
				}
				break;
			case 's':
				start_addr = simple_strtoll(optarg, &error);
				break;
			case 'f':
				if (!(dumpfile = strdup(optarg))) {
					perror("stddup");
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				length = simple_strtoll(optarg, &error);
				break;
			case 'o':
				if (oob_default) {
					oob_default = false;
					omitoob = false;
				} else {
					errmsg_die("--oob and --oomitoob are mutually exclusive");
				}
				break;
			case 'a':
				forcebinary = true;
				break;
			case 'c':
				canonical = true;
			case 'p':
				pretty_print = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 'n':
				noecc = true;
				break;
			case '?':
				error++;
				break;
		}
	}

	if (start_addr < 0)
		errmsg_die("Can't specify negative offset with option -s: %lld",
				start_addr);

	if (length < 0)
		errmsg_die("Can't specify negative length with option -l: %lld", length);

	if (quiet && pretty_print) {
		fprintf(stderr, "The quiet and pretty print options are mutually-\n"
				"exclusive. Choose one or the other.\n");
		exit(EXIT_FAILURE);
	}

	if (forcebinary && pretty_print) {
		fprintf(stderr, "The forcebinary and pretty print options are\n"
				"mutually-exclusive. Choose one or the "
				"other.\n");
		exit(EXIT_FAILURE);
	}

	if ((argc - optind) != 1 || error)
		display_help();

	mtddev = argv[optind];
}
#endif

#define PRETTY_ROW_SIZE 16
#define PRETTY_BUF_LEN 80

/**
 * pretty_dump_to_buffer - formats a blob of data to "hex ASCII" in memory
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @linebuf: where to put the converted data
 * @linebuflen: total size of @linebuf, including space for terminating NULL
 * @pagedump: true - dumping as page format; false - dumping as OOB format
 * @ascii: dump ascii formatted data next to hexdump
 * @prefix: address to print before line in a page dump, ignored if !pagedump
 *
 * pretty_dump_to_buffer() works on one "line" of output at a time, i.e.,
 * PRETTY_ROW_SIZE bytes of input data converted to hex + ASCII output.
 *
 * Given a buffer of unsigned char data, pretty_dump_to_buffer() converts the
 * input data to a hex/ASCII dump at the supplied memory location. A prefix
 * is included based on whether we are dumping page or OOB data. The converted
 * output is always NULL-terminated.
 *
 * e.g.
 *   pretty_dump_to_buffer(data, data_len, prettybuf, linelen, true,
 *                         false, 256);
 * produces:
 *   0x00000100: 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f
 * NOTE: This function was adapted from linux kernel, "lib/hexdump.c"
 */
#if 0
static void pretty_dump_to_buffer(const unsigned char *buf, size_t len,
		char *linebuf, size_t linebuflen, bool pagedump, bool ascii,
		unsigned long long prefix)
{
	static const char hex_asc[] = "0123456789abcdef";
	unsigned char ch;
	unsigned int j, lx = 0, ascii_column;

	if (pagedump)
		lx += sprintf(linebuf, "0x%.8llx: ", prefix);
	else
		lx += sprintf(linebuf, "  OOB Data: ");

	if (!len)
		goto nil;
	if (len > PRETTY_ROW_SIZE)	/* limit to one line at a time */
		len = PRETTY_ROW_SIZE;

	for (j = 0; (j < len) && (lx + 3) <= linebuflen; j++) {
		ch = buf[j];
		linebuf[lx++] = hex_asc[(ch & 0xf0) >> 4];
		linebuf[lx++] = hex_asc[ch & 0x0f];
		linebuf[lx++] = ' ';
	}
	if (j)
		lx--;

	ascii_column = 3 * PRETTY_ROW_SIZE + 14;

	if (!ascii)
		goto nil;

	/* Spacing between hex and ASCII - ensure at least one space */
	lx += sprintf(linebuf + lx, "%*s",
			MAX((int)MIN(linebuflen, ascii_column) - 1 - lx, 1),
			" ");

	linebuf[lx++] = '|';
	for (j = 0; (j < len) && (lx + 2) < linebuflen; j++)
		linebuf[lx++] = (isascii(buf[j]) && isprint(buf[j])) ? buf[j]
			: '.';
	linebuf[lx++] = '|';
nil:
	linebuf[lx++] = '\n';
	linebuf[lx++] = '\0';
}
#endif


/*
 * Main program
 */
static unsigned char readbuf[2048]; // , oobbuf[64];
int nand_dump_main(char *mtdname, unsigned char *buf, int saddr, int rlen)
{
	int ofs, end_addr = 0;
	int blockstart = 1;
	//int i;
        int fd, bs, badblock = 0;
        //int ofd = 0;
	struct mtd_dev_info mtd;
	/*char pretty_buf[PRETTY_BUF_LEN];*/
	int firstblock = 1;
	//struct mtd_ecc_stats stat1, stat2;
	/*bool eccstats = false;*/
	libmtd_t mtd_desc;
        int idx;

        if (rlen <= 0 || rlen < 2048) {
            printf("nand_dump(), rlen is too small(%d)\n", rlen);
            return -1;
        }

	/*process_options(argc, argv);*/
	canonical = true;
	pretty_print = true;
	noecc = true;
        strcpy(mtddev, mtdname);
	length = rlen;
	omitoob = false;
        start_addr = saddr;

	/* Initialize libmtd */
	mtd_desc = libmtd_open();
	if (!mtd_desc) {
		printf("can't initialize libmtd\n");
		return -1;
        }

	/* Open MTD device */
	if ((fd = open(mtddev, O_RDONLY)) == -1) {
		printf("Cannot open %s\n", mtddev);
                return -1;
	}

	/* Fill in MTD device capability structure */
	if (mtd_get_dev_info(mtd_desc, mtddev, &mtd) < 0) {
		printf("mtd_get_dev_info failed\n");
		return -1;
        }

	/* Allocate buffers */
	if (mtd.oob_size > 64) {
	    printf("MTD.oob_size is too big (%d)\n", mtd.oob_size);
            return -1;
        }
	if (mtd.min_io_size > 2048) {
	    printf("MTD.min_io_size is too big (%d)\n", mtd.min_io_size);
            return -1;
        }

#if 0
	if (noecc)  {
		if (ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW) != 0) {
				perror("MTDFILEMODE");
				goto closeall;
		}
	}
        else {
		/* check if we can read ecc stats */
		if (!ioctl(fd, ECCGETSTATS, &stat1)) {
			eccstats = true;
			if (!quiet) {
				fprintf(stderr, "ECC failed: %d\n", stat1.failed);
				fprintf(stderr, "ECC corrected: %d\n", stat1.corrected);
				fprintf(stderr, "Number of bad blocks: %d\n", stat1.badblocks);
				fprintf(stderr, "Number of bbt blocks: %d\n", stat1.bbtblocks);
			}
		} else
			perror("No ECC status information available");
	}
#else
	if (ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW) != 0) {
		perror("MTDFILEMODE");
		goto closeall;
	}
#endif

#if 0
	/* Open output file for writing. If file name is "-", write to standard
	 * output. */
	if (!dumpfile) {
		ofd = STDOUT_FILENO;
	} else if ((ofd = open(dumpfile, O_WRONLY | O_TRUNC | O_CREAT, 0644))== -1) {
		perror(dumpfile);
		goto closeall;
	}
#endif

#if 0
	if (!pretty_print && !forcebinary && isatty(ofd)) {
		fprintf(stderr, "Not printing binary garbage to tty. Use '-a'\n"
				"or '--forcebinary' to override.\n");
		goto closeall;
	}
#endif

	/* Initialize start/end addresses and block size */
	if (start_addr & (mtd.min_io_size - 1)) {
		fprintf(stderr, "the start address (-s parameter) is not page-aligned!\n"
				"The pagesize of this NAND Flash is 0x%x.\n",
				mtd.min_io_size);
		goto closeall;
	}
	if (length)
		end_addr = start_addr + length;
	if (!length || end_addr > mtd.size)
		end_addr = mtd.size;

	bs = mtd.min_io_size;
        idx = 0;

#ifdef _MTD_DEBUG_PRINT_
	/* Print informative message */
	printf("Block size %d, page size %d, OOB size %d\n",
			mtd.eb_size, mtd.min_io_size, mtd.oob_size);
	printf("Dumping data starting at 0x%08x and ending at 0x%08x...\n",
			start_addr, end_addr);
#endif

	/* Dump the flash contents */
	for (ofs = start_addr; ofs < end_addr; ofs += bs) {
		/* Check for bad block */
		if (bb_method == dumpbad) {
			badblock = 0;
		} else if (blockstart != (ofs & (~mtd.eb_size + 1)) ||
				firstblock) {
			blockstart = ofs & (~mtd.eb_size + 1);
			firstblock = 0;
			if ((badblock = mtd_is_bad(&mtd, fd, ofs / mtd.eb_size)) < 0) {
				printf("error, libmtd: mtd_is_bad");
				goto closeall;
			}
		}

		if (badblock) {
			/* skip bad block, increase end_addr */
			if (bb_method == skipbad) {
				end_addr += mtd.eb_size;
				ofs += mtd.eb_size - bs;
				if (end_addr > mtd.size)
					end_addr = mtd.size;
				continue;
			}
			memset(readbuf, 0xff, bs);
		} else {
			/* Read page data and exit on failure */
			if (mtd_read(&mtd, fd, ofs / mtd.eb_size, ofs % mtd.eb_size, readbuf, bs)) {
				printf("error, mtd_read\n");
				goto closeall;
			}
		}

                memcpy(&buf[idx], readbuf, bs);
                idx += bs;

#if 0
		/* ECC stats available ? */
		if (eccstats) {
			if (ioctl(fd, ECCGETSTATS, &stat2)) {
				perror("ioctl(ECCGETSTATS)");
				goto closeall;
			}
			if (stat1.failed != stat2.failed)
				fprintf(stderr, "ECC: %d uncorrectable bitflip(s)"
						" at offset 0x%08llx\n",
						stat2.failed - stat1.failed, ofs);
			if (stat1.corrected != stat2.corrected)
				fprintf(stderr, "ECC: %d corrected bitflip(s) at"
						" offset 0x%08llx\n",
						stat2.corrected - stat1.corrected, ofs);
			stat1 = stat2;
		}
#endif

#if 0
		/* Write out page data */
		//if (pretty_print)
                {
			for (i = 0; i < bs; i += PRETTY_ROW_SIZE) {
				pretty_dump_to_buffer(readbuf + i, PRETTY_ROW_SIZE,
						pretty_buf, PRETTY_BUF_LEN, true, canonical, ofs + i);
				write(ofd, pretty_buf, strlen(pretty_buf));
			}
		}
#endif

#if 0
                else
			write(ofd, readbuf, bs);
#endif

#if 0
		if (omitoob)
			continue;

		if (badblock) {
			memset(oobbuf, 0xff, mtd.oob_size);
		} else {
			/* Read OOB data and exit on failure */
			if (mtd_read_oob(mtd_desc, &mtd, fd, ofs, mtd.oob_size, oobbuf)) {
				errmsg("libmtd: mtd_read_oob");
				goto closeall;
			}
		}

		/* Write out OOB data */
		//if (pretty_print)
                {
			for (i = 0; i < mtd.oob_size; i += PRETTY_ROW_SIZE) {
				pretty_dump_to_buffer(oobbuf + i, mtd.oob_size - i,
						pretty_buf, PRETTY_BUF_LEN, false, canonical, 0);
				write(ofd, pretty_buf, strlen(pretty_buf));
			}
		}
#if 0
                else
			write(ofd, oobbuf, mtd.oob_size);
#endif

#endif // omitoob
	}

	/* Close the output file and MTD device, free memory */
	close(fd);
	//close(ofd);
	//free(oobbuf);
	//free(readbuf);

#ifdef _MTD_DEBUG_PRINT_
	/* Exit happy */
        printf("mtd_nanddump GOOD\n\n");
#endif

	return 0;

closeall:
	close(fd);
	//close(ofd);
	//free(oobbuf);
	//free(readbuf);

        return -1;
}

