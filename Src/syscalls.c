/*
 * syscalls.c
 *
 * Minimal newlib syscall stubs for bare-metal STM32F4.
 * Silences the 6 "_xxx is not implemented and will always fail" linker
 * messages and routes stdout to USART2.
 */

#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include "uart.h"

/* Called by the C runtime on exit — nothing to do on bare metal. */
int _close(int file)
{
    (void)file;
    return -1;
}

/* Character device: report as a TTY so stdout is line-buffered. */
int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return 0;
}

/* No stdin on this board yet. */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/* Route printf()/puts() output to USART2. */
int _write(int file, char *ptr, int len)
{
    (void)file;
    int i;
    for (i = 0; i < len; i++)
    {
        uart2_write(ptr[i]);
    }
    return len;
}
