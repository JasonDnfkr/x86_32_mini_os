#ifndef FS_H
#define FS_H

int sys_open(const char* name, int flags, ...);

int sys_read(int fd, char* ptr, int len);

int sys_write(int fd, char* ptr, int len);

int sys_lseek(int fd, int offset, int dir);

int sys_close(int fd);

#endif