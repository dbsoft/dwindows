#ifdef __UNIX__
#include <dirent.h>
#else
#ifndef __DIRENT_H__
#define __DIRENT_H__

#include <stdio.h>
#ifdef MAXPATHLEN
	#undef MAXPATHLEN
#endif
#define MAXPATHLEN (FILENAME_MAX*4)
#define MAXNAMLEN FILENAME_MAX

#ifdef __cplusplus
extern "C" {
#endif

/* attribute stuff */
#ifndef A_RONLY
# define A_RONLY   0x01
# define A_HIDDEN  0x02
# define A_SYSTEM  0x04
# define A_LABEL   0x08
# define A_DIR     0x10
# define A_ARCHIVE 0x20
#endif

struct dirent {
    int            d_ino;                 /* Dummy */
    int            d_reclen;		  /* Dummy, same as d_namlen */
    int            d_namlen;              /* length of name */
    char           d_name[MAXNAMLEN + 1];
    unsigned long  d_size;
    unsigned short d_attribute;           /* attributes (see above) */
    unsigned short d_time;                /* modification time */
    unsigned short d_date;                /* modification date */
};

typedef struct _dirdescr DIR;
/* the structs do not have to be defined here */

extern DIR		*_opendir(const char *);
#define opendir(a) _opendir(a)
extern DIR		*_openxdir(const char *, unsigned);
#define openxdir(a, b) _openxdir(a, b)
extern struct dirent	*_readdir(DIR *);
#define readdir(a) _readdir(a)
extern void		_seekdir(DIR *, long);
#define seekdir(a, b) _seekdir(a, b)
extern long		_telldir(DIR *);
#define telldir(a) _telldir(a)
extern void 		_closedir(DIR *);
#define closedir(a) _closedir(a)

#define			rewinddir(dirp) _seekdir(dirp, 0L)
extern char *		_abs_path(const char *name, char *buffer, int len);
#define abs_path(a, b, c) _abs_path(a, b, c)

#ifndef S_IFMT
#define S_IFMT ( S_IFDIR | S_IFREG )
#endif

#ifndef S_ISDIR
#define S_ISDIR( m )                    (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG( m )                    (((m) & S_IFMT) == S_IFREG)
#endif

#ifdef __cplusplus
}
#endif

#endif
#endif
