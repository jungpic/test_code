/* flash_erase.c -- erase MTD devices

   Copyright (C) 2000 Arcom Control System Ltd
   Copyright (C) 2010 Mike Frysinger <vapier@gentoo.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define PROGRAM_NAME "flash_erase"

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "mtdutil_common.h"
//#include "mtd_crc32.h"
#include "libmtd.h"

#include "mtd-user.h"

//static int quiet;		/* true -- don't output progress */

//static struct jffs2_unknown_node cleanmarker;
int target_endian = __BYTE_ORDER;

static char tmp_buffer[32];
extern int send_all2avc_cycle_data(char *buf, int maxlen, unsigned short errcode, int errset, int errclear);

static void show_progress(struct mtd_dev_info *mtd, uint64_t start, int eb,
			  int eb_start, int eb_cnt)
{
	printf("\rErasing %d Kibyte @ %"PRIx64" -- %2i %% complete ",
		mtd->eb_size / 1024, start, ((eb - eb_start) * 100) / eb_cnt);
	//fflush(stdout);
}

int flash_erase_main(char *mtd_device, unsigned int start, unsigned int eb_cnt)
{
	libmtd_t mtd_desc;
	struct mtd_dev_info mtd;
	int fd; //, clmpos = 0, clmlen = 8;
	unsigned int eb, eb_start;
	int isNAND;
	//int error = 0;
	unsigned int offset = 0;
        //int noskipbad = 0; /* do not skip bad blocks */
        //int ret = 0;
        //int unlock = 0;	/* unlock sectors before erasing */

        int ctr = 0;

	/*
	 * Locate MTD and prepare for erasure
	 */
	mtd_desc = libmtd_open();
	if (mtd_desc == NULL) {
		printf("flash_erase, Can't initialize libmtd");
		return -1;
        }

	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		printf("flash_erase, Cannot open %s", mtd_device);
                return -1;
        }

	if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0) {
		printf("mtd_get_dev_info failed");
		return -1;
        }

	eb_start = start / mtd.eb_size;

	isNAND = mtd.type == MTD_NANDFLASH ? 1 : 0;

	/*
	 * Now do the actual erasing of the MTD device
	 */
	if (eb_cnt == 0)
		eb_cnt = (mtd.size / mtd.eb_size) - eb_start;

	for (eb = eb_start; eb < eb_start + eb_cnt; eb++) {
		offset = eb * mtd.eb_size;

		int ret = mtd_is_bad(&mtd, fd, eb);
		if (ret > 0)
			continue;

		show_progress(&mtd, offset, eb, eb_start, eb_cnt);

#if 0
		if (unlock) {
			if (mtd_unlock(&mtd, fd, eb) != 0) {
				sys_errmsg("%s: MTD unlock failure", mtd_device);
				continue;
			}
		}
#endif

		if (mtd_erase(mtd_desc, &mtd, fd, eb) != 0) {
			printf("%s: MTD Erase failure", mtd_device);
			//continue;
		}

		/* format for JFFS2 ?
		if (!jffs2)
			continue;
                */

#if 0
		/* write cleanmarker */
		if (isNAND) {
			if (mtd_write_oob(mtd_desc, &mtd, fd, offset + clmpos, clmlen, &cleanmarker) != 0) {
				sys_errmsg("%s: MTD writeoob failure", mtd_device);
				continue;
			}
		} else {
			if (lseek(fd, (loff_t)offset, SEEK_SET) < 0) {
				sys_errmsg("%s: MTD lseek failure", mtd_device);
				continue;
			}
			if (write(fd, &cleanmarker, sizeof(cleanmarker)) != sizeof(cleanmarker)) {
				sys_errmsg("%s: MTD write failure", mtd_device);
				continue;
			}
		}
		printf(" Cleanmarker written at %"PRIx64, offset);
#endif

                ctr++;
                if (ctr >= 10) {
                    ctr = 0;
                    printf("< 1. Send alive >\n");
                    send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0); // Ver 0.71, 2014/01/15
                }
	}
	show_progress(&mtd, offset, eb, eb_start, eb_cnt);
	printf("\n");

	return 0;
}

