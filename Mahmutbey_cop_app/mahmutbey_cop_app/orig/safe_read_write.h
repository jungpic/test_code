#ifndef __SAFE_READ_WRITE_H__
#define __SAFE_READ_WRITE_H__

int safe_read(int fd, void *buf, int count);
int safe_write(int fd, const void *buf, int count);
int full_write(int fd, const void *buf, int len);

#endif // __SAFE_READ_WRITE_H__
