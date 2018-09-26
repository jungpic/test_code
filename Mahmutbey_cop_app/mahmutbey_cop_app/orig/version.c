#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "version.h"
#include "safe_read_write.h"

#define _VERSION_DEBUG__

extern int nand_dump_main(char *mtdname, unsigned char *buf, int saddr, int rlen);

unsigned short get_entire_version(unsigned short bootver, unsigned short kernelver,
		unsigned short appver)
{
    unsigned short h, l;

    h = (bootver >> 8) + (kernelver >> 8) + (appver >> 8);
    l = (bootver & 0xff) + (kernelver & 0xff) + (appver & 0xff);

    if (l >= 100) {
        h += 1;
        l -= 100;
    }

    if (h >= 100)
        h -= 100;

    return ((h << 8) + (l & 0xff));
}

unsigned short get_uboot_version(unsigned char *tmpbuf, int maxlen)
{
    unsigned short uboot_ver = 0;
    unsigned short h, l;
    int ret, idx, i;
    char ver_str[64];
    char str[64];

    if (maxlen < _NAND_BLOCK_SIZE) {
        printf("[%s(), Buf size is too small ]\n", __func__);
        return 0;
    }

    ret = nand_dump_main(__UBOOT_DEVMTDNAME, tmpbuf, 0, _NAND_BLOCK_SIZE);
    if (ret != 0) {
        printf("< ERROR, Cannot read image (%s) >\n", __UBOOT_DEVMTDNAME);
        return 0;
    }

    idx = 0xb0 + 0x40;
    for (i = 0; i < 32; i++)
        ver_str[i] = tmpbuf[idx++];
    ver_str[i] = 0;
#ifdef _VERSION_DEBUG__
    printf("< READ UBOOT VER: %s >\n", ver_str);
#endif

    strcpy(str, BOOT_VERSION_PREFIX);
    ret = memcmp(ver_str, str, strlen(str));
    if (ret != 0) {
        printf("< ERROR, UBOOT Ver Key >\n");
        return 0;
    }

    idx = strlen(BOOT_VERSION_PREFIX);
    h = (ver_str[idx++] - '0') * 10;
    h += (ver_str[idx++] - '0');
    idx++; /* dot */
    l = (ver_str[idx++] - '0') * 10;
    l += (ver_str[idx++] - '0');

    uboot_ver = (h << 8) + l;
    
    return uboot_ver;
}

unsigned short get_kernel_version(unsigned char *tmpbuf, int maxlen)
{
    unsigned short kernel_ver = 0;
    unsigned short h, l;
    int ret, idx, i;
    char ver_str[32+4];
    char str[32+4];

    if (maxlen < _NAND_BLOCK_SIZE) {
        printf("[%s(), Buf size is too small ]\n", __func__);
        return 0;
    }

    ret = nand_dump_main(__KERNEL_DEVMTDNAME, tmpbuf, 0, _NAND_BLOCK_SIZE);
    if (ret != 0) {
        printf("< ERROR, Cannot read image (%s) >\n", __KERNEL_DEVMTDNAME);
    }

    idx = 0xb0 + 0x20;
    for (i = 0; i < 32; i++)
        ver_str[i] = tmpbuf[idx++];
    ver_str[i] = 0;
#ifdef _VERSION_DEBUG__
    printf("< READ KERNEL VER: %s >\n", ver_str);
#endif

    strcpy(str, KERNEL_VERSION_PREFIX);
    ret = memcmp(ver_str, str, strlen(str));
    if (ret != 0) {
    	ret = nand_dump_main(__KERNEL_BACKUP_DEVMTDNAME, tmpbuf, 0, _NAND_BLOCK_SIZE);
    
        if (ret != 0) {
            printf("< ERROR, Cannot read image (%s) >\n", __KERNEL_DEVMTDNAME);
        }
	
	idx = 0xb0 + 0x20;
        for (i = 0; i < 32; i++)
            ver_str[i] = tmpbuf[idx++];
        ver_str[i] = 0;

    	strcpy(str, KERNEL_VERSION_PREFIX);
	ret = memcmp(ver_str, str, strlen(str));
      
	if (ret != 0){
	    printf("< ERROR, KERNEL Ver Key >\n");
            return 0;
	}
    }

    idx = strlen(KERNEL_VERSION_PREFIX);
    h = (ver_str[idx++] - '0') * 10;
    h += (ver_str[idx++] - '0');
    idx++; /* dot */
    l = (ver_str[idx++] - '0') * 10;
    l += (ver_str[idx++] - '0');

    kernel_ver = (h << 8) + l;
    return (kernel_ver);
}
#if 0
unsigned short get_font_version(unsigned char *tmpbuf, int maxlen)
{
    unsigned short font_ver = 0;
    unsigned short h, l;
    int ret, idx, i;
    int fd;
    char ver_str[32];
    int len;

    fd = open(__FONT_VERSION_FILENAME, O_RDONLY);
    if (fd < 0) {
         printf("get_font_version(), FILE OPEN ERROR (%s)\n", __FONT_VERSION_FILENAME);
         return 0;
    }

    ret = safe_read(fd, tmpbuf, 29);
    if (!ret) { /* EOF */
        close(fd);
        return -1;
    }
    tmpbuf[29] = 0;

#ifdef _VERSION_DEBUG__
    printf("< READ  FONT VER: %s >\n", tmpbuf);
#endif

    idx = 0;
    len = strlen(FONT_VERSION_PREFIX);
    for (i = 0; i < len; i++)
        ver_str[i] = tmpbuf[idx++];
    ver_str[i] = 0;

    ret = strncmp(ver_str, FONT_VERSION_PREFIX, len);
    if (ret != 0) {
        printf("< ERROR, FONT VER PREFIX: %s >\n", ver_str);
        close(fd);
        return 0;
    }

    idx = len;
    h = (tmpbuf[idx++] - '0') * 10;
    h += (tmpbuf[idx++] - '0');
    idx++; /* dot */
    l = (tmpbuf[idx++] - '0') * 10;
    l += (tmpbuf[idx++] - '0');

    font_ver = (h << 8) + l;

    if (tmpbuf[idx] != '#') {
        printf("< ERROR, FONT VER END PREFIX: %c >\n", tmpbuf[idx]);
        close(fd);
        return 0;
    }

    close(fd);

    return (font_ver);
}
#endif
