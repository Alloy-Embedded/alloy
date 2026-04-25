#include <errno.h>
#include <sys/stat.h>

extern "C" {

int _write(int, char*, int)     { errno = ENOSYS; return -1; }
int _close(int)                 { errno = ENOSYS; return -1; }
int _fstat(int, struct stat*)   { errno = ENOSYS; return -1; }
int _isatty(int)                { return 0; }
int _lseek(int, int, int)       { errno = ENOSYS; return -1; }
int _read(int, char*, int)      { errno = ENOSYS; return -1; }
void* _sbrk(int)                { errno = ENOMEM; return (void*)-1; }
int _getpid()                   { return 1; }
int _kill(int, int)             { errno = ENOSYS; return -1; }

}  // extern "C"
