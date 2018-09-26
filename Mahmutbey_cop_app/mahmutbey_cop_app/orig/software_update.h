#ifndef __SOFTWARE_UPDATE_H__
#define __SOFTWARE_UPDATE_H__

#define AVC_FTP_LOGIN_NAME	"wbpa"
#define AVC_FTP_LOGIN_PASSWD	"dowon1222"

#define DEFAULT_LOCAL_PATH	"/tmp/update_firmware.bin"

#define DEFAULT_APP_TAR_NAME	"mahmutbey_cop_apps.tar.gz"

#define UPDATE_APP_NAME     	"MAHMUTBEY_COP_APP-"
#define UPDATE_FONTS_NAME		"MAHMUTBEY_COP_FONTS-"
#define UPDATE_KERNEL_NAME		"MAHMUTBEY_COP_KERNEL-"
#define UPDATE_UBOOT_NAME		"MAHMUTBEY_COP_UBOOT-"
#define UPDATE_BOOTSTRAP_NAME	"MAHMUTBEY_COP_BOOT-"

#define UPIMG_HEADER_SIZE	(32 + 128 + 16)

#define TAR_INDICATOR_POS	(0xA8)
#define TAR_INDICATOR_STRING	"tar xfz"

int software_update(char *image_path);

enum sw_up_status {
    FTP_LOGIN = 1,
    FTP_FILE_DOWNLOADING,
    FTP_FILE_UPDATE,
    FTP_UPDATE_DONE,
    FTP_ERROR,
};

#endif // __SOFTWARE_UPDATE_H__

