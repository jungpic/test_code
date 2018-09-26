/*
 */
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

#define MTD_SYS_PATH	"/sys/class/mtd/"

static int get_mtd_erase_size(char *mtdname);
static int get_mtd_size(char *mtdname);

int get_mtd_erase_blk_ctr(char *mtdname)
{
    int size, erasesize;
    int erasenum;

    size = get_mtd_size(mtdname);
    if (size < 0)
        return -1;

    erasesize = get_mtd_erase_size(mtdname);
    if (erasesize < 0)
        return -1;

    erasenum = size / erasesize;

    printf("erasenum: %d\n", erasenum);


    return erasenum;
}

static int get_mtd_size(char *mtdname)
{
    char path[64];
    char buf[64];
    int size;
    int len, idx;
    int fd;

    idx = 0;
    strcpy(&path[idx], MTD_SYS_PATH);
    idx += strlen(MTD_SYS_PATH);

    strcpy(&path[idx], mtdname);
    idx += strlen(mtdname);

    strcpy(&path[idx], "/size");

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", path);
        return -1;
    }

    len = read(fd, buf, 64);
    buf[len] = 0;

    size = atoi(buf);

    //printf("erasesize: %d\n", buf, size);

    return size;
}

static int get_mtd_erase_size(char *mtdname)
{
    char path[64];
    char buf[64];
    int size;
    int len, idx;
    int fd;

    idx = 0;
    strcpy(&path[idx], MTD_SYS_PATH);
    idx += strlen(MTD_SYS_PATH);

    strcpy(&path[idx], mtdname);
    idx += strlen(mtdname);

    strcpy(&path[idx], "/erasesize");

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", path);
        return -1;
    }

    len = read(fd, buf, 64);
    buf[len] = 0;

    size = atoi(buf);

    //printf("erasesize: %d\n", buf, size);

    return size;
}

