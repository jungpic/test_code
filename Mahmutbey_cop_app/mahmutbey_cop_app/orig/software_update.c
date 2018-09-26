#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>

#include "copyfd.h"
#include "safe_read_write.h"
#include "software_update.h"
#include "version.h"

#define IS_APP_UPDATE		100
#define IS_APP_TAR_UPDATE	101
#define IS_FONT_UPDATE		102
#define IS_KERNEL_UPDATE	104
#define IS_UBOOT_UPDATE		105
#define IS_BOOTSTRAP_UPDATE	106

static char tmp_buffer[32];

extern char ftp_file_buffer[CONFIG_COPYBUF_SIZE];
extern const unsigned short crc16_tab[];

static int check_file_validation(char *file_name);
static int update_app_image(char *file_name);
static int update_tar_apps_image(char *file_name);
static int update_kernel_image(char *file_name);
static int update_uboot_image(char *file_name);
static int update_bootstrap_image(char *file_name);

static int run_tar_execlp(char *src_file, char *dst_path);
static int run_del_tar_execlp(char *src_file, char *dst_path);

extern int get_mtd_erase_blk_ctr(char *mtdname);
extern int flash_erase_main(char *mtd_device, unsigned int start, unsigned int eb_cnt);
extern int nandwrite_main(char	*mtd_device, char *img);

static char tmp_buffer[32];
extern int send_all2avc_cycle_data(char *buf, int maxlen, unsigned short errcode, int errset, int errclear);

int software_update(char *image_path)
{
    int mode, ret = -1;

    mode = check_file_validation(image_path);
    if (mode < 0) {
        printf("<< ERROR, UPDATE FILE CHECK >>\n");
        return -1;
    }

    switch (mode) {
        case IS_APP_UPDATE:
            ret = update_app_image(image_path);
            break;
	
        case IS_FONT_UPDATE:
		    ret = update_tar_apps_image(image_path);
	    	break;

		case IS_APP_TAR_UPDATE:
		    ret = update_tar_apps_image(image_path);
	    	break;
        
        case IS_KERNEL_UPDATE:
            ret = update_kernel_image(image_path);
            break;

        case IS_UBOOT_UPDATE:
            ret = update_uboot_image(image_path);
            break;

		case IS_BOOTSTRAP_UPDATE:
			ret = update_bootstrap_image(image_path);
			break;
    }

    if (ret != 0) {
        printf("<< ERROR, UPDATIMG IMAGE >>\n");
        return -1;
    }

    return 0;
}

static int check_file_validation(char *file_name)
{
    int i, idx, ret, status = -1;
    int fd, file_size, rd;
    struct stat st;
    int buffer_size;
    int total = 0;
    unsigned short read_file_cksum, cal_file_cksum;
    unsigned char ch;
    char name[128];
    int mode = -1, is_tar = 0;

    read_file_cksum = 0;
    cal_file_cksum = 0;
    
    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
         printf("check_image_validation(), FILE OPEN ERROR (%s)\n", file_name);
         return -1;
    }

    ret = fstat(fd, &st);
    if (ret != 0) {
        printf("check_image_validation(), Cannot get stat from %s\n", file_name);
        close(fd);
        return -1;
    }

    file_size = st.st_size;
    //printf("check_image_validation(), file_size: %d\n", file_size);

    buffer_size = UPIMG_HEADER_SIZE;
    rd = safe_read(fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
    if (!rd) { /* EOF */
        close(fd);
        return -1;
    }

    idx = 0;
    for (i = 0; i < 33; i++)
        name[i] = ftp_file_buffer[idx++];
    name[i] = 0;

    ret = strncmp(&ftp_file_buffer[TAR_INDICATOR_POS], "tar xfz", 7);
    if (ret == 0)
        is_tar = 1;

    ret = strncmp(name, UPDATE_APP_NAME, strlen(UPDATE_APP_NAME));
    if (ret == 0) {
        if (is_tar)
            mode = IS_APP_TAR_UPDATE;
        else
            mode = IS_APP_UPDATE;
    }

	if (mode < 0) {
        ret = strncmp(name, UPDATE_FONTS_NAME, strlen(UPDATE_FONTS_NAME));
		if (ret == 0)
			mode = IS_FONT_UPDATE;
	}

    if (mode < 0) {
        ret = strncmp(name, UPDATE_KERNEL_NAME, strlen(UPDATE_KERNEL_NAME));
        if (ret == 0)
            mode = IS_KERNEL_UPDATE;
    }

    if (mode < 0) {
        ret = strncmp(name, UPDATE_UBOOT_NAME, strlen(UPDATE_UBOOT_NAME));
        if (ret == 0)
            mode = IS_UBOOT_UPDATE;
    }

    if (mode < 0) {
        ret = strncmp(name, UPDATE_BOOTSTRAP_NAME, strlen(UPDATE_BOOTSTRAP_NAME));
        if (ret == 0)
            mode = IS_BOOTSTRAP_UPDATE;
    }

    if (mode < 0) {
        printf("%s(), PRODUCT NAME ERROR(%s)\n", __func__, name);
        close(fd);
        return -1;
    }


    printf("[ UPDATE PRODUCT NAME : %s ]\n", name);

    for (i = 0; i < rd; i++) {
        ch = ftp_file_buffer[i];
        cal_file_cksum = crc16_tab[((cal_file_cksum>>8) ^ ch) & 0xFF] ^ (cal_file_cksum << 8);
    }

    buffer_size = CONFIG_COPYBUF_SIZE;
    file_size = file_size - UPIMG_HEADER_SIZE - 2;

    while (1) {
        rd = safe_read(fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
        if (!rd) { /* EOF */
            status = 0;
            break;
        }
        if (rd < 0) {
            printf("ERROR, File Read.\n");
            break;
        }
        total += rd;

        for (i = 0; i < rd; i++) {
            ch = ftp_file_buffer[i];
            cal_file_cksum = crc16_tab[((cal_file_cksum>>8) ^ ch) & 0xFF] ^ (cal_file_cksum << 8);
        }

        file_size -= rd;
        if (!file_size) {
            status = 0;
            break;
        }
    }

    if (status != 0) {
        close(fd);
        return -1;
    }

    /* read CRC 16 */
    rd = safe_read(fd, ftp_file_buffer, 2);
    if (rd >= 2) {
        read_file_cksum = ftp_file_buffer[0];
        read_file_cksum |= ftp_file_buffer[1] << 8;
    }
    else
        status = -1;

    if (read_file_cksum == cal_file_cksum) {
        printf("[ UPDATE FILE CHECK OK ]\n");
        ret = 0;
    }
    else {
        printf("< ERROR, FILE CRC, R:0x%04X, C:0x%04X >\n", read_file_cksum, cal_file_cksum);
        ret = -1;
    }

    close(fd);

    if (ret == 0)
        ret = mode;

    return ret;
}

static int update_app_image(char *file_name)
{
    int i, idx, ret, status = -1;
    int src_fd, dst_fd;
    int file_size, file_len;
    int rd, wr = 0;
    int total = 0, total2 = 0;
    struct stat st;
    int buffer_size;
    unsigned short read_img_cksum, cal_img_cksum;
    unsigned char ch;
    char name[128+4];

    read_img_cksum = 0;
    cal_img_cksum = 0;
    
    src_fd = open(file_name, O_RDONLY);
    if (src_fd < 0) {
         printf("update_app_image(), FILE OPEN ERROR (%s)\n", file_name);
         return -1;
    }

    ret = fstat(src_fd, &st);
    if (ret != 0) {
        printf("update_app_image(), Cannot get stat from %s\n", file_name);
        close(src_fd);
        return -1;
    }

    file_size = st.st_size;

    buffer_size = UPIMG_HEADER_SIZE;
    rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
    if (!rd) { /* EOF */
        close(src_fd);
        return -1;
    }

    idx = 32;
    for (i = 0; i < 128; i++)
        name[i] = ftp_file_buffer[idx++];
    name[i] = 0;

    printf("[ UPDATE LOCAL FILE PATH:%s ]\n", name);

    dst_fd = open(name, O_CREAT | O_TRUNC | O_WRONLY);
    if (src_fd < 0) {
        printf("update_app_image(), WRITE OPEN ERROR (%s)\n", name);
        close(src_fd);
        return -1;
    }

    read_img_cksum = ftp_file_buffer[0xA4];
    read_img_cksum |= ftp_file_buffer[0xA5] << 8;

    buffer_size = CONFIG_COPYBUF_SIZE;
    file_size = file_size - UPIMG_HEADER_SIZE - 2;
    file_len = file_size;

    while (1) {
        rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
        if (!rd) { /* EOF */
            status = 0;
            break;
        }
        if (rd < 0) {
            printf("ERROR, UPDATE IMAGE Read.\n");
            break;
        }
        for (i = 0; i < rd; i++) {
            ch = ftp_file_buffer[i];
            cal_img_cksum = crc16_tab[((cal_img_cksum>>8) ^ ch) & 0xFF] ^ (cal_img_cksum << 8);
        }

        wr = full_write(dst_fd, ftp_file_buffer, rd);
        if (wr < rd) {
            printf("ERROR, UPDATE IMAGE WRITE (len: %d).\n", rd);
            break;
        }

        total += rd;
        total2 += rd;

        if (total2 >= 1024*1024) { // Ver 0.71, 2014/01/15
            total2 = 0;
            printf("< 2. Send alive >\n");
            send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0);
        }

        printf("< UPDATE FILE WRITING: %d/%d >\n", total, file_len);

        file_size -= rd;
        if (!file_size) {
            status = 0;
            break;
        }
    }
    printf("\n");

    if (status != 0) {
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    rd = safe_read(src_fd, ftp_file_buffer, 2);
    if (rd < 0) { /* EOF */
        printf("ERROR, UPDATE IMAGE Read.\n");
        close(src_fd);
        close(dst_fd);
        return -1;
    }
    wr = full_write(dst_fd, ftp_file_buffer, rd);
    if (wr < rd) {
        printf("ERROR, UPDATE IMAGE WRITE (len: %d).\n", rd);
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    if (read_img_cksum == cal_img_cksum) {
        printf("[ UPDATE IMAGE CHECK OK ]\n");
        ret = 0;
    }
    else {
        printf("< ERROR, IMAGE CRC, R:0x%04X, C:0x%04X >\n", read_img_cksum, cal_img_cksum);
        ret = -1;
    }

    close(src_fd);
    close(dst_fd);

    return ret;
}

static int update_tar_apps_image(char *file_name)
{
    int i, idx, ret, status = -1;
    int src_fd, dst_fd;
    int file_size, file_len;
    int rd, wr = 0;
    int total = 0, total2 = 0;
    struct stat st;
    int buffer_size;
    unsigned short read_img_cksum, cal_img_cksum;
    unsigned char ch;
    char dst_name[128+4];
    char name[128+4];

    read_img_cksum = 0;
    cal_img_cksum = 0;
    
    src_fd = open(file_name, O_RDONLY);
    if (src_fd < 0) {
         printf("update_app_image(), FILE OPEN ERROR (%s)\n", file_name);
         return -1;
    }

    ret = fstat(src_fd, &st);
    if (ret != 0) {
        printf("update_app_image(), Cannot get stat from %s\n", file_name);
        close(src_fd);
        return -1;
    }

    file_size = st.st_size;

    buffer_size = UPIMG_HEADER_SIZE;
    rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
    if (!rd) { /* EOF */
        close(src_fd);
        return -1;
    }

    idx = 32;
    for (i = 0; i < 128; i++)
        dst_name[i] = ftp_file_buffer[idx++];
    dst_name[i] = 0;

    strcpy(name, dst_name);
    i = strlen(name);
    name[i] = '/';
    strcpy(&name[i+1], DEFAULT_APP_TAR_NAME);

    printf("[ UPDATE APPS TAR FILE PATH:%s ]\n", name);

    dst_fd = open(name, O_CREAT | O_TRUNC | O_WRONLY);
    if (src_fd < 0) {
        printf("update_app_image(), WRITE OPEN ERROR (%s)\n", name);
        close(src_fd);
        return -1;
    }

    read_img_cksum = ftp_file_buffer[0xA4];
    read_img_cksum |= ftp_file_buffer[0xA5] << 8;

    buffer_size = CONFIG_COPYBUF_SIZE;
    file_size = file_size - UPIMG_HEADER_SIZE - 2;
    file_len = file_size;

    while (1) {
        rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
        if (!rd) { /* EOF */
            status = 0;
            break;
        }
        if (rd < 0) {
            printf("ERROR, UPDATE IMAGE Read.\n");
            break;
        }
        for (i = 0; i < rd; i++) {
            ch = ftp_file_buffer[i];
            cal_img_cksum = crc16_tab[((cal_img_cksum>>8) ^ ch) & 0xFF] ^ (cal_img_cksum << 8);
        }

        wr = full_write(dst_fd, ftp_file_buffer, rd);
        if (wr < rd) {
            printf("ERROR, UPDATE IMAGE WRITE (len: %d).\n", rd);
            break;
        }

        total += rd;
        total2 += rd;

        if (total2 >= 1024*1024) { // Ver 0.71, 2014/01/15
            total2 = 0;
            printf("< 3. Send alive >\n");
            send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0);
        }

        printf("< UPDATE FILE WRITING: %d/%d >\n", total, file_len);

        file_size -= rd;
        if (!file_size) {
            status = 0;
            break;
        }
    }
    printf("\n");

    if (status != 0) {
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    rd = safe_read(src_fd, ftp_file_buffer, 2);
    if (rd < 0) { /* EOF */
        printf("ERROR, UPDATE IMAGE Read.\n");
        close(src_fd);
        close(dst_fd);
        return -1;
    }
    wr = full_write(dst_fd, ftp_file_buffer, rd);
    if (wr < rd) {
        printf("ERROR, UPDATE IMAGE WRITE (len: %d).\n", rd);
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    if (read_img_cksum == cal_img_cksum) {
        printf("[ UPDATE IMAGE CHECK OK ]\n");
        ret = 0;
    }
    else {
        printf("< ERROR, IMAGE CRC, R:0x%04X, C:0x%04X >\n", read_img_cksum, cal_img_cksum);
        ret = -1;
    }

    close(src_fd);
    close(dst_fd);

    if (ret != 0)
        return ret;

    send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0);

    /* RUN TAR COMMAND */
    ret = run_tar_execlp(name, dst_name);

    send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0);

	/* RUN DELETE FILE COMMAND */
	ret = run_del_tar_execlp(name, dst_name);

    send_all2avc_cycle_data(tmp_buffer, 8, 0, 0, 0);

    return ret;
}

static int run_del_tar_execlp(char *src_file, char *dst_path)
{
	pid_t childpid;
	childpid = fork();

	if (childpid == -1) {
		printf("Failed to fork for running tar command\n");
		return -1;
	}

	if (childpid == 0) {
		printf("\n[ RUN DEL CMD: rm -rf %s ]\n", src_file, dst_path);
		execlp("rm", "rm", "-rf", src_file, NULL);
        printf("Failed running rm command\n");
        return -1;
    }

	if (childpid != wait(NULL)) {
		printf("Parent failed to wait due to signal or error while running rm command\n");
		return -1;
	}

	return 0;
}

static int run_tar_execlp(char *src_file, char *dst_path)
{
    pid_t childpid;

    childpid = fork();

    if (childpid == -1) {
        printf("Failed to fork for running tar command\n");
        return -1;
    }

    if (childpid == 0) {
        printf("\n[ RUN TAR CMD: tar xfz %s -C %s ]\n", src_file, dst_path);
        execlp("tar", "tar", "xfz", src_file, "-C", dst_path, NULL); 
        printf("Failed running tar command\n");
        return -1;
    }

    if (childpid != wait(NULL)) {
        printf("Parent failed to wait due to signal or error while running tar command\n");
        return -1;
    }

    return 0;
}

static int update_kernel_image(char *file_name)
{
    int ret, idx, rd, i;
    int blk_ctr;
    int src_fd;
    struct stat st;
    char name[128+4];
    int file_size;
    int buffer_size;

    src_fd = open(file_name, O_RDONLY);
    if (src_fd < 0) {
         printf("update_kernel_image(), FILE OPEN ERROR (%s)\n", file_name);
         return -1;
    }

    ret = fstat(src_fd, &st);
    if (ret != 0) {
        printf("update_kernel_image(), Cannot get stat from %s\n", file_name);
        close(src_fd);
        return -1;
    }

    file_size = st.st_size;

    buffer_size = UPIMG_HEADER_SIZE;
    rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
    if (!rd) { /* EOF */
        close(src_fd);
        return -1;
    }
    close(src_fd);

    idx = 32;
    for (i = 0; i < 128; i++)
        name[i] = ftp_file_buffer[idx++];
    name[i] = 0;

    printf("[ UPDATE LOCAL FILE PATH:%s ]\n", /*name*/ __KERNEL_DEVMTDNAME);
    blk_ctr = get_mtd_erase_blk_ctr(/*&name[5]*/__KERNEL_MTDNAME);
    if (blk_ctr < 0) return -1;
    ret = flash_erase_main(/*name*/__KERNEL_DEVMTDNAME, 0, blk_ctr);
    if (ret < 0) return -1;
    ret = nandwrite_main(/*name*/__KERNEL_DEVMTDNAME, file_name);
    if (ret < 0) return -1;

    printf("\n[ UPDATE LOCAL BACKUP FILE PATH:%s ]\n", /*name*/ __KERNEL_BACKUP_DEVMTDNAME);
    blk_ctr = get_mtd_erase_blk_ctr(/*&name[5]*/__KERNEL_BACKUP_MTDNAME);
    if (blk_ctr < 0) return -1;
    ret = flash_erase_main(/*name*/__KERNEL_BACKUP_DEVMTDNAME, 0, blk_ctr);
    if (ret < 0) return -1;
    ret = nandwrite_main(/*name*/__KERNEL_BACKUP_DEVMTDNAME, file_name);
    if (ret < 0) return -1;

    return ret;
}

static int update_uboot_image(char *file_name)
{
    int ret, idx, rd, i;
    int blk_ctr;
    int src_fd;
    struct stat st;
    char name[128+4];
    int file_size;
    int buffer_size;

    src_fd = open(file_name, O_RDONLY);
    if (src_fd < 0) {
         printf("update_uboot_image(), FILE OPEN ERROR (%s)\n", file_name);
         return -1;
    }

    ret = fstat(src_fd, &st);
    if (ret != 0) {
        printf("update_uboot_image(), Cannot get stat from %s\n", file_name);
        close(src_fd);
        return -1;
    }

    file_size = st.st_size;

    buffer_size = UPIMG_HEADER_SIZE;
    rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
    if (!rd) { /* EOF */
        close(src_fd);
        return -1;
    }
    close(src_fd);

    idx = 32;
    for (i = 0; i < 128; i++)
        name[i] = ftp_file_buffer[idx++];
    name[i] = 0;

    printf("[ UPDATE LOCAL FILE PATH:%s ]\n", /*name*/ __UBOOT_DEVMTDNAME);
    blk_ctr = get_mtd_erase_blk_ctr(/*&name[5]*/__UBOOT_MTDNAME);
    if (blk_ctr < 0) return -1;
    ret = flash_erase_main(/*name*/__UBOOT_DEVMTDNAME, 0, blk_ctr);
    if (ret < 0) return -1;
    ret = nandwrite_main(/*name*/__UBOOT_DEVMTDNAME, file_name);
    if (ret < 0) return -1;

    printf("[ UPDATE LOCAL BACKUP FILE PATH:%s ]\n", /*name*/ __UBOOT_BACKUP_DEVMTDNAME);
    blk_ctr = get_mtd_erase_blk_ctr(/*&name[5]*/__UBOOT_BACKUP_MTDNAME);
    if (blk_ctr < 0) return -1;
    ret = flash_erase_main(/*name*/__UBOOT_BACKUP_DEVMTDNAME, 0, blk_ctr);
    if (ret < 0) return -1;
    ret = nandwrite_main(/*name*/__UBOOT_BACKUP_DEVMTDNAME, file_name);
    if (ret < 0) return -1;

    return ret;
}

#define MAXIMUM_READ_SIZE		1024

static int update_bootstrap_image(char *file_name)
{
	int ret, idx, rd, i;
	int blk_ctr;
	int src_fd;
	int dst_fd;
	struct stat st;
	char name[128+4];
	int file_size;
	int buffer_size;
	int readcnt = 0;
	char *dst_file_name = "/tmp/update_bootstrap.bin";
	char temp_buf[MAXIMUM_READ_SIZE];

	src_fd = open(file_name, O_RDONLY);
	if (src_fd < 0) {
		printf("-ERROR- : update_bootstrap_image(), FILE OPEN ERROR (%s)\n", file_name);
		return -1;
	}

	ret = fstat(src_fd, &st);
	if (ret != 0) {
		printf("-ERROR- : update_bootstrap_image(), Cannot get stat from %s\n", file_name);
		close(src_fd);
		return -1;
	}

	file_size = st.st_size;

	buffer_size = UPIMG_HEADER_SIZE;
	rd = safe_read(src_fd, ftp_file_buffer, file_size > buffer_size ? buffer_size : file_size);
	if (!rd) { /* EOF */
		close(src_fd);
		return -1;
	}

    dst_fd = open(dst_file_name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_SYNC, 0644);
	if (dst_fd < 0) {
		printf("-ERROR- : update_bootstrap_image(), FILE CREATE ERROR (%s)\n", dst_file_name);
		return -1;
	}

	do {
		readcnt = read(src_fd, temp_buf, MAXIMUM_READ_SIZE);
		if(readcnt > 0) {
			if(write(dst_fd, temp_buf, readcnt) == -1) {
				printf("%s-%s [%d] : -ERROR- : Bootstrap file copy error...\n", __FILE__, __FUNCTION__, __LINE__);
				close(src_fd);
				close(dst_fd);
				return -1;
			}
		}
	} while(readcnt);

	close(src_fd);
	close(dst_fd);

	idx = 32;
	for (i = 0; i < 128; i++)
		name[i] = ftp_file_buffer[idx++];
	name[i] = 0;

	printf("[ UPDATE LOCAL FILE PATH : %s ]\n", __BOOTSTRAP_DEVMTDNAME);
	blk_ctr = get_mtd_erase_blk_ctr(__BOOTSTRAP_MTDNAME);
	if (blk_ctr < 0) return -1;
	ret = flash_erase_main(__BOOTSTRAP_DEVMTDNAME, 0, blk_ctr);
	if (ret < 0) return -1;
	ret = nandwrite_main(__BOOTSTRAP_DEVMTDNAME, dst_file_name);
	if (ret < 0) return -1;

	return ret;
}
