/* $Id$ */

#ifndef _DWCOMPAT_H
#define _DWCOMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* This header includes and defines everything needed for a given OS/compiler */
#if defined(__UNIX__) || defined(__MAC__) || defined(__IOS__) || defined(__ANDROID__)
/* iOS and Android currently don't use autoconf */
#if !defined(__IOS__) && !defined(__ANDROID__)
#include "dwconfig.h"
#else
#define HAVE_DIRENT_H 1
#define HAVE_PIPE 1
#define HAVE_VSNPRINTF 1
#endif

/* Attempt to include 64 bit file functions on various unix flavors */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#ifndef _LARGE_FILES
#define _LARGE_FILES 1
#endif
#ifndef _DARWIN_USE_64_BIT_INODE
#define _DARWIN_USE_64_BIT_INODE 1
#endif

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

#ifdef __WATCOMC__
#include <alloca.h>
#include <sys/select.h>
#include <sys/stat.h>
#  ifdef _stati64
#    ifdef stat
#    undef stat
#    endif
#  define stat(a, b) _stati64(a, b)
#  define dwstat _stati64
#  endif
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

#ifdef __OS2__
# if (defined(__IBMC__) || defined(__WATCOMC__) || defined(_System)) && !defined(API)
# define API _System
# endif
#endif

#ifndef API
#define API
#endif

#include <stdio.h>

/* Mostly safe but slow snprintf() for compilers that don't have it... 
 * like VisualAge.  So we can write safe code and still use VAC to test.
 */
#if defined(__IBMC__) && !defined(snprintf)
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
static int _dw_snprintf(char *str, size_t size, const char *format, ...)
{
   va_list args;
   char *outbuf = calloc(1, size + strlen(format) + 1024);
   int retval = -1;

   if(outbuf)
   {
      va_start(args, format);
      vsprintf(outbuf, format, args);
      va_end(args);
      retval = strlen(outbuf);
      strncpy(str, outbuf, size);
      free(outbuf);
   }
   return retval;
}
#define snprintf _dw_snprintf
#endif


#define msleep(a) DosSleep(a)

#ifdef __EMX__
#include "platform/dirent.h"
#include <sys/stat.h>
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

#define PIPEROOT "\\socket"
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
# if defined(__MINGW32__) && defined(BUILD_DLL)
#  define API _cdecl __declspec(dllexport)
# else
#  define API _cdecl
# endif
#endif

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <process.h>
#include <sys/stat.h>

#ifdef MSVC
#include "platform/dirent.h"
#undef alloca
#define alloca _alloca
#ifdef __stat64
#undef stat
#define stat(a, b) _stat64(a, b)
#define dwstat __stat64
#endif
#include <direct.h>
#else
#include <dir.h>
#include <dirent.h>
#endif

#include <stdarg.h>

/* Cygwin and Visual Studio 15.4 (SDK 10.0.16299.15) support domain sockets */
#if defined(__CYGWIN32__)
#include <sys/un.h>
#elif defined(_MSC_VER) && _MSC_VER >= 1912
#include <afunix.h>
#define PIPEROOT getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp"
#else
#define NO_DOMAIN_SOCKETS
#endif 

#if defined(_P_NOWAIT) && !defined(P_NOWAIT)
#define P_NOWAIT _P_NOWAIT
#endif

#ifdef _MSC_VER
/* Handle deprecated functions in Visual C */
#  if _MSC_VER < 1500
#  define vsnprintf _vsnprintf
#  endif
#define HAVE_VSNPRINTF
#  if _MSC_VER >= 1400
#  define strcasecmp(a, b) _stricmp(a, b)
#  define strncasecmp(a, b, c) _strnicmp(a, b, c)
#  define strdup(a) _strdup(a)
#  define snprintf _snprintf
#  define unlink(a) _unlink(a)
#  define rmdir(a) _rmdir(a)
#  define close(a) _close(a)
#  define open(a, b) _open(a, b)
#  define read(a, b, c) _read(a, b, c)
#  define fdopen(a, b) _fdopen(a, b)
#  define getcwd(a, b) _getcwd(a, b)
#  define chdir(a) _chdir(a)
#  define getpid() _getpid()
#ifndef _DW_INTERNAL
#  define mkdir(a,b) _mkdir(a)
#endif
#  else
#  define strcasecmp(a, b) stricmp(a, b)
#  define strncasecmp(a, b, c) strnicmp(a, b, c)
#  endif
#endif
#define msleep(a) Sleep(a)

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

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#ifndef __WATCOMC__
#include <sys/time.h>
#endif
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
#endif /* !_MSC_VER */
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

#ifndef PIPEROOT
#define PIPEROOT "/tmp"
#endif

#define PIPENAME "%s%s" __TARGET__ "%d-%d"

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
#define sockinit() { static WSADATA wsa; WSAStartup(MAKEWORD (2, 0), &wsa); }
#else  /* !WIN32 */
#define sockinit()
#endif

#if defined(__WIN32__) || defined(WINNT)
#define sockshutdown() WSACleanup()
#else /* !WIN32 */
#define sockshutdown()
#endif

#define oldsockpipe(pipes) { \
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
		if(tmpsock > -1) \
			sockclose(tmpsock); \
	} \
}

#ifdef HAVE_PIPE
#define sockpipe(pipes) { if(pipe(pipes) < 0) pipes[0] = pipes[1] = -1; }
#elif !defined(NO_DOMAIN_SOCKETS)
#define sockpipe(pipes) { \
	struct sockaddr_un un; \
	int tmpsock = socket(AF_UNIX, SOCK_STREAM, 0); \
	pipes[0] = pipes[1] = -1; \
	if(tmpsock > -1 && (pipes[1] = socket(AF_UNIX, SOCK_STREAM, 0)) > -1) \
	{ \
		memset(&un, 0, sizeof(un)); \
		un.sun_family=AF_UNIX; \
		sprintf(un.sun_path, PIPENAME, PIPEROOT, DIRSEP, (int)getpid(), pipes[1]); \
		unlink(un.sun_path); \
		bind(tmpsock, (struct sockaddr *)&un, sizeof(un)); \
		listen(tmpsock, 0); \
		connect(pipes[1], (struct sockaddr *)&un, sizeof(un)); \
		pipes[0] = accept(tmpsock, 0, 0); \
	} else \
		oldsockpipe(pipes); \
	if(tmpsock > -1) \
		sockclose(tmpsock); \
	}
#else
#define sockpipe(pipes) oldsockpipe(pipes)
#endif

/* Ok Windows and OS/2 both seem to be missing these */
#if defined(__WIN32__) || defined(__OS2__)
typedef int socklen_t;
#ifndef _IN_ADDR_T_DECLARED
typedef unsigned long in_addr_t;
#endif
#endif

/* If dwstat didn't otherwise get defined */
#ifndef dwstat
#define dwstat stat
#endif

/* Make sure O_BINARY is defined */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined(__IBMC__) || defined(__WATCOMC__) || (defined(_MSC_VER) && _MSC_VER < 1400) || defined(__MINGW32__) || defined(__MINGW64__)
#ifndef _DW_INTERNAL
#  undef mkdir
#  define mkdir(a,b) mkdir(a)
#endif
#endif

#define socksprint(a, b) sockwrite(a, b, strlen(b), 0)

char * API vargs(char *buf, int len, char *format, ...);
int API makedir(char *path);
void API setfileinfo(char *filename, char *url, char *logfile);
#ifdef __MINGW32__
double API drivesize(int drive);
double API drivefree(int drive);
#else
long double API drivesize(int drive);
long double API drivefree(int drive);
#endif
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

#ifdef __cplusplus
}
#endif

#endif
