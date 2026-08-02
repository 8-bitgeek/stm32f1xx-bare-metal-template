#include "syscalls.h"


// int _write(int fd, char *ptr, int len) {
//     (void) fd;
//     (void) ptr;
//     (void) len;
//     errno = ENOSYS;
//     return -1;
// }
int _read(int fd, char * ptr, int len) {
    (void) fd;
    (void) ptr;
    (void) len;
    errno = ENOSYS;
    return -1;
}
int _close(int fd) {
    (void) fd;
    errno = ENOSYS;
    return -1;
}
int _fstat(int fd, struct stat * st) {
    (void) fd;
    (void) st;
    errno = ENOSYS;
    return -1;
}
int _isatty(int fd) {
    (void) fd;
    errno = ENOSYS;
    return -1;
}
int _lseek(int fd, int ptr, int dir) {
    (void) fd;
    (void) ptr;
    (void) dir;
    errno = ENOSYS;
    return -1;
}
int _getpid(void) {
    return -1;
}
int _kill(int pid, int sig) {
    (void) pid;
    (void) sig;
    errno = ENOSYS;
    return -1;
}
