#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

static char dir_name[128];
static char target_name[256];
static char exe_link[256];

int get_pid_by_name(char *pname)
{
    int result = 0;
    DIR *dir_p;
    struct dirent *dir_entry_p;
    int target_result;
    //int errcount = 0;

    dir_p = opendir("/proc");

    while (NULL != (dir_entry_p = readdir(dir_p))) {
        if (strspn(dir_entry_p->d_name, "01234567890") == strlen(dir_entry_p->d_name)) {
            strcpy(dir_name, "/proc/");
            strcat(dir_name, dir_entry_p->d_name);
            strcat(dir_name, "/");
            exe_link[0] = 0;
            strcat(exe_link, dir_name);
            strcat(exe_link, "exe");

            target_result = readlink(exe_link, target_name, sizeof(target_name) - 1);
            if (target_result > 0) {
                target_name[target_result] = 0;
                if (strstr(target_name, pname) != NULL) {
                    result = atoi(dir_entry_p->d_name);
                    printf("< GET PID: %s-%d >\n", pname, result);
                    closedir(dir_p);
                    return result;
                }
            }
        }
    }

    closedir(dir_p);

    printf("< CANNOT GET PID: %s >\n", pname);

    return result;
}

#define UDHCPC_PID_FILE_PATH	"/tmp/udhcpc.pid"
int get_pid_udhcpc(void)
{
    int ret = 0;
    int i, fd, rlen;
    char buf[16], ch;

    fd = open(UDHCPC_PID_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    i = 0;
    while (1) {
        rlen = read(fd, &ch, 1);
        if (rlen <= 0)
            break;

        //printf("%02X ", ch);
        if (ch == 0x0A || ch == 0x0D) {
            buf[i] = 0;
            ret = strtol(buf, NULL, 0);
            printf("< GET PID UDHCPC: %d >\n", ret);
        }
        else 
            buf[i++] = ch;
    }

    return ret;
}

