#ifndef __VERSION_H__
#define __VERSION_H__


#define BOOT_VERSION_PREFIX "COP-BOOT-"
#define KERNEL_VERSION_PREFIX "COP-KERNEL-"


#define _NAND_BLOCK_SIZE	2048
#define __BOOTSTRAP_MTDNAME		"mtd0"
#define __BOOTSTRAP_DEVMTDNAME	"/dev/mtd0"

#define __UBOOT_MTDNAME		"mtd1"
#define __UBOOT_DEVMTDNAME	"/dev/mtd1"
#define __UBOOT_BACKUP_MTDNAME		"mtd2"
#define __UBOOT_BACKUP_DEVMTDNAME	"/dev/mtd2"

#define __KERNEL_MTDNAME	"mtd5"
#define __KERNEL_DEVMTDNAME	"/dev/mtd5"
#define __KERNEL_BACKUP_MTDNAME	"mtd6"
#define __KERNEL_BACKUP_DEVMTDNAME	"/dev/mtd6"

#define __FONT_VERSION_FILENAME	"/mnt/data/fonts_version.txt"

unsigned short get_uboot_version(unsigned char *tmpbuf, int maxlen);
unsigned short get_kernel_version(unsigned char *tmpbuf, int maxlen);
unsigned short get_entire_version(unsigned short bootver, unsigned short kernelver,
		unsigned short appver);

#endif //__VERSION_H__
