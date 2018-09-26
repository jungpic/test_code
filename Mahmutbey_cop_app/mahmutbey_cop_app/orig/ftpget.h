#ifndef __FTPGET_H__
#define __FTPGET_H__

void ftpget_init(void);
int ftpget_login(char *host_ip, char *user_name, char *passwd);
int ftp_receive(const char *local_path, char *server_path);

#endif // __FTPGET_H__
