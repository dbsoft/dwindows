/* $Id$ */

#ifndef _COMPAT_H
#define _COMPAT_H

/* This header includes and defines everything needed for a given OS/compiler */
#if defined(__UNIX__) || defined(__MAC__)
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

#if defined(__EMX__) || defined(__OS2__) || defined(__WIN32__) || defined(WINNT)
#include <io.h>
#include <process.h>

#define DIRSEP "\\"
#define TYPDIR "."
#else
#define DIRSEP "/"
#define TYPDIR "/usr/local/" __TARGET__
#endif
#define INIDIR "~/." __TARGET__

/* OS/2 */
#if defined(__EMX__) || defined(__OS2__)
#define INCL_WIN
#define INCL_GPI
#define INCL_VIO
#define INCL_NLS
#define INCL_DOS
#define INCL_DEV
#define INCL_DOSERRORS

#if (defined(__IBMC__) || defined(_System)) && !defined(API)
#define API _System
#endif

#ifndef API
#define API
#endif

#define msleep(a) DosSleep(a)

#ifdef __EMX__
#include "platform/dirent.h"
#include <sys/stat.h>
#define HAVE_PIPE
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024
#endif /* __EMX__ */

#if defined(__EMX__) || defined(__WATCOMC__)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

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

#if defined(__OS2__) && (defined(__IBMC__) || defined(__WATCOMC__))
#define BSD_SELECT

#include <types.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <direct.h>
#include <stdarg.h>
/* For VAC we are using the Mozilla dirent.c */
#ifndef __WATCOMC__
#include "platform/dirent.h"
#endif
#endif

/* Windows */
#if defined(__WIN32__) || defined(WINNT)

#if defined(MSVC) && !defined(API)
# ifdef __MINGW32__
#  ifdef BUILD_DLL
#   define API APIENTRY __declspec(dllexport)
#  else
#   define API APIENTRY __declspec(dllimport)
#  endif
# else
#  define API _cdecl
# endif
#endif

#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <process.h>
#include <sys/stat.h>

#ifdef MSVC
#include "platform/dirent.h"
#define alloca _alloca
#else
#include <dir.h>
#include <dirent.h>
#endif

#include <stdarg.h>

#if defined(__CYGWIN32__) /*|| defined(__MINGW32__)*/
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
#ifndef __IBMC__
#include <arpa/inet.h>
#endif
#if defined(__OS2__) && defined(RES_DEFAULT)
#undef RES_DEFAULT
#endif
#include <stdarg.h>
#include <string.h>
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

#ifndef API
#define API
#endif

/* Compatibility layer for IBM C/Winsock
 * Now using macros so we can allow cross
 * compiler support.
 */

#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
#define sockread(a, b, c, d) recv(a, b, c, d)
#else
#define sockread(a, b, c, d) read(a, b, c)
#endif

#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
#define sockwrite(a, b, c, d) send(a, b, c, d)
#else
#define sockwrite(a, b, c, d) write(a, b, c)
#endif

#ifdef __IBMC__
#define sockclose(a) soclose(a)
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
#define sockclose(a) closesocket(a)
#else
#define sockclose(a) close(a)
#endif

#if defined(__OS2__) && !defined(__EMX__)
#define nonblock(a)	{ int _nonblock = 1; ioctl(a, FIONBIO, (char *)&_nonblock, sizeof(_nonblock)); }
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
#define nonblock(a) { int _nonblock = 1; ioctlsocket(a, FIONBIO, (unsigned long *)&_nonblock); }
#else
#define nonblock(a) fcntl(a, F_SETFL, O_NONBLOCK)
#endif

#if defined(__OS2__) && !defined(__EMX__)
#define block(a) { int _block = 0; ioctl(a, FIONBIO, (char *)&_nonblock, sizeof(_block)); }
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
#define block(a) { int _block = 0; ioctlsocket(a, FIONBIO, (unsigned long *)&_block); }
#else
#define block(a) fcntl(a, F_SETFL, 0)
#endif

#ifdef __IBMC__
#define sockinit() sock_init();
#elif defined(__WIN32__) || defined(WINNT)
#define sockinit() { static WSADATA wsa; WSAStartup(MAKEWORD (1, 1), &wsa); }
#else  /* !WIN32 */
#define sockinit()
#endif

#if defined(__WIN32__) || defined(WINNT)
#define sockshutdown() WSACleanup()
#else /* !WIN32 */
#define sockshutdown()
#endif

#ifdef HAVE_PIPE
#define sockpipe(pipes) { if(pipe(pipes) < 0) pipes[0] = pipes[1] = -1; }
#elif !defined(NO_DOMAIN_SOCKETS)
#define sockpipe(pipes) { \
	struct sockaddr_un un; \
	int tmpsock = socket(AF_UNIX, SOCK_STREAM, 0); \
	pipes[1] = socket(AF_UNIX, SOCK_STREAM, 0); \
	memset(&un, 0, sizeof(un)); \
	un.sun_family=AF_UNIX; \
	sprintf(un.sun_path, PIPENAME, pipes[1]); \
	bind(tmpsock, (struct sockaddr *)&un, sizeof(un)); \
	listen(tmpsock, 0); \
	connect(pipes[1], (struct sockaddr *)&un, sizeof(un)); \
	pipes[0] = accept(tmpsock, 0, 0); \
	sockclose(tmpsock); \
	}
#else
#define sockpipe(pipes) { \
	struct sockaddr_in server_addr; \
	struct sockaddr_in listen_addr = { 0 }; \
	int tmpsock, len = sizeof(struct sockaddr_in); \
	struct hostent *he = gethostbyname("localhost"); \
	pipes[0] = pipes[1] = -1; \
	if(he) \
	{ \
		memset(&server_addr, 0, sizeof(server_addr)); \
		server_addr.sin_family = AF_INET; \
		server_addr.sin_port   = 0; \
		server_addr.sin_addr.s_addr = INADDR_ANY; \
		if ((tmpsock = socket(AF_INET, SOCK_STREAM, 0)) > -1 && bind(tmpsock, (struct sockaddr *)&server_addr, sizeof(server_addr)) > -1 && listen(tmpsock, 0) > -1) \
	    { \
		    memset(&listen_addr, 0, sizeof(listen_addr)); \
		    getsockname(tmpsock, (struct sockaddr *)&listen_addr, &len); \
		    server_addr.sin_family      = AF_INET; \
		    server_addr.sin_port        = listen_addr.sin_port; \
		    server_addr.sin_addr.s_addr = *((unsigned long *)he->h_addr); \
		    if((pipes[1] = socket(AF_INET, SOCK_STREAM, 0)) > -1 && !connect(pipes[1], (struct sockaddr *)&server_addr, sizeof(server_addr))) \
			   pipes[0] = accept(tmpsock, 0, 0); \
	    } \
		sockclose(tmpsock); \
	} \
}
#endif

/* Visual Studio doesn't define this... so just in case */
#ifndef socklen_t
typedef int socklen_t;
#endif

#define socksprint(a, b) sockwrite(a, b, strlen(b), 0)

char * API vargs(char *buf, int len, char *format, ...);
int API makedir(char *path);
void API setfileinfo(char *filename, char *url, char *logfile);
long double API drivesize(int drive);
long double API drivefree(int drive);
int API isdrive(int drive);
void API getfsname(int drive, char *buf, int len);
FILE * API fsopen(char *path, char *modes);
int API fsclose(FILE *fp);
char * API fsgets(char *str, int size, FILE *stream);
int API fsseek(FILE *stream, long offset, int whence);
int API locale_init(char *filename, int my_locale);
char * API locale_string(char *default_text, int message);
void API nice_strformat(char *dest, long double val, int dec);
void API initdir(int argc, char *argv[]);
int API setpath(char *path);

#ifdef __MINGW32__
# undef API
# define API APIENTRY
#endif

#endif
