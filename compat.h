/* $Id$ */

/* This header includes and defines everything needed for a given OS/compiler */
#if !defined(__EMX__) && !defined(__IBMC__) && !defined(__WIN32__) && !defined(WINNT)
#include "config.h"

#include <sys/stat.h>
#include <unistd.h>
void msleep(long period);
#endif /* Unix */

#ifndef __TARGET__
#define __TARGET__ "dw"
#endif

#include <sys/types.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */

#ifdef DIRSEP
#undef DIRSEP
#endif

#if defined(__EMX__) || defined(__IBMC__) || defined(__WIN32__) || defined(WINNT)
#include <io.h>
#include <process.h>

#define DIRSEP "\\"
#define INIDIR "."
#define TYPDIR "."
#else
#define DIRSEP "/"
#define INIDIR "~/." __TARGET__
#define TYPDIR "/usr/local/" __TARGET__
#endif

/* OS/2 */
#if defined(__EMX__) || defined(__IBMC__)
#define INCL_WIN
#define INCL_GPI
#define INCL_VIO
#define INCL_NLS
#define INCL_DOS
#define INCL_DEV
#define INCL_DOSERRORS

#define msleep(a) DosSleep(a)

#ifdef __EMX__
#include <dirent.h>
#include <sys/stat.h>
#define HAVE_PIPE
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif /* __EMX__ */

#ifndef OS2
#define OS2
#endif /* OS2 */

#include <os2.h>

#ifndef BKS_TABBEDDIALOG
#define BKS_TABBEDDIALOG          0x0800
#endif 

#define PIPENAME "\\socket\\" __TARGET__ "%d"
#define TPIPENAME "\\socket\\" __TARGET__ "%d"
#else
#define PIPENAME "/tmp/" __TARGET__ "%d"
#define TPIPENAME "/tmp/" __TARGET__ "%d"
#endif /* __EMX__ || __IBMC__ */

#ifdef __IBMC__
#define BSD_SELECT

#include <types.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <direct.h>
#include <stdarg.h>
/* For VAC we are using the Mozilla dirent.c */
#include "platform/dirent.h"
#endif

/* Windows */
#if defined(__WIN32__) || defined(WINNT)
#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <process.h>
#include <sys/stat.h>
#ifdef MSVC
#include "platform/dirent.h"
#else
#include <dir.h>
#include <dirent.h>
#endif
#include <stdarg.h>

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#include <sys/un.h>
#endif /* __CYGWIN32__ || __MINGW32__ */

#ifndef __CYGWIN32__
#define NO_DOMAIN_SOCKETS
#endif /* __CYGWIN32__ */

#if defined(_P_NOWAIT) && !defined(P_NOWAIT)
#define P_NOWAIT _P_NOWAIT
#endif

#define strcasecmp stricmp
#define strncasecmp strnicmp
#define msleep Sleep

#endif /* WIN32 */

/* Everything else ;) */
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>

#if !defined(__WIN32__) && !defined(WINNT)
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#if defined(STDC_HEADERS) || defined(__EMX__)
#include <stdarg.h>
#include <string.h>
#endif /* STDC_HEADERS */
#endif /* !WIN32 */
#include <ctype.h>

#ifndef _MAX_PATH
#define _MAX_PATH 255
#endif

/* IBM C doesn't allow "t" in the mode parameter
 * because it violates the ANSI standard.
 */
#ifdef __IBMC__
#define FOPEN_READ_TEXT "r"
#define FOPEN_WRITE_TEXT "w"
#define FOPEN_APPEND_TEXT "a"
#else
#define FOPEN_READ_TEXT "rt"
#define FOPEN_WRITE_TEXT "wt"
#define FOPEN_APPEND_TEXT "at"
#endif
#define FOPEN_READ_BINARY "rb"
#define FOPEN_WRITE_BINARY "wb"
#define FOPEN_APPEND_BINARY "ab"

/* Compatibility layer for IBM C/Winsock */
int	sockread (int a, void *b, int c, int d);
int	sockwrite (int a, void *b, int c, int d);
int	sockclose(int a);
int socksprintf(int fd, char *format, ...);
int sockpipe(int *pipes);
void sockinit(void);
void sockshutdown(void);
int makedir(char *path);
void nonblock(int fd);
void block(int fd);
void setfileinfo(char *filename, char *url, char *logfile);
#if (defined(__IBMC__) && __IBMC__ < 360) || (defined(__WIN32__) && !defined(__CYGWIN32__))
unsigned long drivesize(int drive);
unsigned long drivefree(int drive);
#else
unsigned long long drivefree(int drive);
unsigned long long drivesize(int drive);
#endif
int isdrive(int drive);
void getfsname(int drive, char *buf, int len);
FILE *fsopen(char *path, char *modes);
int fsclose(FILE *fp);
char *fsgets(char *str, int size, FILE *stream);
int fsseek(FILE *stream, long offset, int whence);
int locale_init(char *filename, int my_locale);
char *locale_string(char *default_text, int message);

