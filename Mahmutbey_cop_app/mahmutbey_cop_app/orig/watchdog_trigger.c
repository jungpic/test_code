/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LED_TRIGGER_BASE_DIR	"/sys/class/leds/wd_trigger/"

/*static int cur_brightness;
static int fd_brightness;
static char trigger_str[2][8];*/

#define __BUF_SIZE	128
static char file_name[64], buf[__BUF_SIZE];

static int set_trigger_function(char *trig_name);

#if 0
int init_trigger_hw_watchdog(void)
{
    int fd;
    int l, ret;

    fd_brightness = -1;

    /* get brightness */
    strcpy(&file_name[0], LED_TRIGGER_BASE_DIR);
    l = strlen(LED_TRIGGER_BASE_DIR);
    strcpy(&file_name[l], "brightness");

    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        printf("Cannot open %s\n", file_name);
        return -1;
    }
    fd_brightness = fd;

    strcpy(&trigger_str[0][0], "0\n");


    /* get max_brightness */
    strcpy(&file_name[0], LED_TRIGGER_BASE_DIR);
    l = strlen(LED_TRIGGER_BASE_DIR);
    strcpy(&file_name[l], "max_brightness");

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", file_name);
        fd_brightness = -1;
        return -1;
    }

    ret = read(fd, buf, __BUF_SIZE);
    buf[ret] = 0;
    close(fd);

    strcpy(&trigger_str[1][0], buf);

    cur_brightness = 0;

    return 0;
}
#else
int init_trigger_hw_watchdog(void)
{
    return 0;
}
#endif

int set_watchdog_trigger_none(void)
{
    return set_trigger_function("none");
}

int set_watchdog_trigger_heartbeat(void)
{
    return set_trigger_function("heartbeat");
}

static int set_trigger_function(char *trig_name)
{
    int l, fd;
    int ret;

    /* get max_brightness */
    strcpy(&file_name[0], LED_TRIGGER_BASE_DIR);
    l = strlen(LED_TRIGGER_BASE_DIR);
    strcpy(&file_name[l], "trigger");

    fd = open(file_name, O_WRONLY);
    if (fd < 0) {
        //printf("Cannot open %s\n", file_name);
        return -1;
    }

    strcpy(buf, trig_name);
    l = strlen(buf);
    ret = write(fd, buf, l);
    if (ret != l) {
        printf("<ERROR, write trigger, ret: %d>\n", ret);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

#if 0
int set_trigger_heartbeat(void)
{
    int l, fd;
    int ret;

    /* get max_brightness */
    strcpy(&file_name[0], LED_TRIGGER_BASE_DIR);
    l = strlen(LED_TRIGGER_BASE_DIR);
    strcpy(&file_name[l], "trigger");

    fd = open(file_name, O_WRONLY);
    if (fd < 0) {
        printf("Cannot open %s\n", file_name);
        return -1;
    }

    strcpy(buf, "heartbeat\n");
    l = strlen(buf);
    ret = write(fd, buf, l);
    close(fd);

    return 0;
}

int do_trigger_hw_watchdog(void)
{
    int l, ret;
    char *ptr;

    ptr = &trigger_str[cur_brightness][0];
    l = strlen(ptr);
    ret = write(fd_brightness, ptr, l);
    if (ret != l) {
        printf("<ERROR, write brightness, ret: %d>\n", ret);
        return -1;
    }

    cur_brightness ^= 1;

    return 0;
}
#endif

