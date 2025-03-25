#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

// Function to handle `_close`
int _close(int file) {
    return -1;
}

// Function to handle `_fstat`
int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

// Function to handle `_isatty`
int _isatty(int file) {
    return 1;
}

// Function to handle `_lseek`
off_t _lseek(int file, off_t ptr, int dir) {
    return 0;
}

// Function to handle `_read`
int _read(int file, char *ptr, int len) {
    return 0;
}

// Function to handle `_write`
int _write(int file, char *ptr, int len) {
    int i;
    for (i = 0; i < len; i++) {
        // Implement your UART/Serial output function here
        // Example: HAL_UART_Transmit(&huart2, (uint8_t*)&ptr[i], 1, HAL_MAX_DELAY);
    }
    return len;
}

// Function to handle `_kill`
int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

// Function to handle `_getpid`
int _getpid(void) {
    return 1;
}

//extern UART_HandleTypeDef huart1;  // Change this to your UART instance
//
//int _write(int file, char *ptr, int len) {
//    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
//    return len;
//}
