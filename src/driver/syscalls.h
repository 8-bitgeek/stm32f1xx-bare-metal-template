#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

int _write(int fd, char * ptr, int len);
int _read(int fd, char * ptr, int len);
int _close(int fd);
int _fstat(int fd, struct stat * st);
int _isatty(int fd);
int _lseek(int fd, int ptr, int dir);
int _getpid(void);
int _kill(int pid, int sig);

#endif
