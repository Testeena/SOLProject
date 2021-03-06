#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAXPIPEBUFF 5
#define MAX_PATH 512
#define MAX_REQUESTS 256
#define DEFAULT_MAXSTORAGE 100000
#define DEFAULT_MAXNFILES 100
#define DEFAULT_NWORKERS 10
#define DEFAULT_SOCKNAME "solsock.sk"
#define DEFAULT_LOGFILENAME "log.json"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

// usage: printf(RED "hello in red!\n" RESET);

// syscalls returns handlers

#define PERRNOTZERO(v)\
    do {\
        if (v != 0) {\
            perror(#v);\
            exit(EXIT_FAILURE);\
        }\
    } while (0);

#define PERRNULL(v)\
    do {\
        if(v == NULL) {\
            perror(#v);\
            exit(EXIT_FAILURE);\
        }\
    } while(0);

#define PERRNEG(v)\
    do {\
        if(v == -1) {\
            perror(#v);\
            exit(EXIT_FAILURE);\
        }\
    } while(0);

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void* buf, size_t size) {
    size_t left = size;
    int r;
    char* bufptr = (char*)buf;
    while (left > 0) {
        if ((r = read((int)fd, bufptr, left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;   // EOF
        left -= r;
        bufptr += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void* buf, size_t size) {
    size_t left = size;
    int r;
    char* bufptr = (char*)buf;
    while (left > 0) {
        if ((r = write((int)fd, bufptr, left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left -= r;
        bufptr += r;
    }
    return 1;
}

static inline int isNumber(const char* s, long* n) {
    if (s == NULL) return 1;
    if (strlen(s) == 0) return 1;
    char* e = NULL;
    errno = 0;
    long val = strtol(s, &e, 10);
    if (errno == ERANGE) return 2;    // overflow/underflow
    if (e != NULL && *e == (char)0) {
        *n = val;
        return 0;   // successo 
    }
    return 1;   // non e' un numero
}

#endif