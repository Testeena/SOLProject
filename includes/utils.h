#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#define DEFAULT_MAXSTORAGE 100000
#define DEFAULT_MAXNFILES 100
#define DEFAULT_NWORKERS 10
#define DEFAULT_SOCKNAME "solsock.sk"
#define DEFAULT_LOGFILENAME "log.json"

// syscalls returns handlers

#define PERRNOTZERO(v)\
    do {\
        if (v != 0) {\
            perror(#v);\
            exit(EXIT_FAILURE);\
        }\
    } while (0);

#define PERRIFNULL(v)\
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