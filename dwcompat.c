/* $Id: dwcompat.c 1717 2012-05-05 22:44:27Z bsmith $ */
#undef UNICODE
#undef _UNICODE

#ifdef __WIN32__
#include <direct.h>
#endif
#if defined(__OS2__) || defined(__WIN32__)
#include <share.h>
#endif
#define _DW_INTERNAL
#include "dwcompat.h"
#include "dw.h"

#if defined(__UNIX__) || defined(__MAC__)
#if defined(__FreeBSD__) || defined(__MAC__)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#elif defined(__sun__)
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#else
#include <mntent.h>
#include <sys/vfs.h>
#endif
#endif
#include <time.h>
#include <errno.h>

#if defined(__UNIX__) || defined(__MAC__)
void msleep(long period)
{
#ifdef __sun__
	/* usleep() isn't threadsafe on Solaris */
	struct timespec req;

	req.tv_sec = 0;
	if(period >= 1000)
	{
		req.tv_sec = (int)(period / 1000);
		period -= (req.tv_sec * 1000);
	}
	req.tv_nsec = period * 10000000;
	
	nanosleep(&req, NULL);
#else
	usleep((int)(period * 1000));
#endif
}
#endif

int API makedir(char *path)
{
#if defined(__IBMC__) || defined(__WATCOMC__) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	return mkdir(path);
#else
	return mkdir(path,S_IRWXU);
#endif
}

char * API vargs(char *buf, int len, char *format, ...)
{
	va_list args;

	va_start(args, format);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, len, format, args);
#else
	len = len;
	vsprintf(buf, format, args);
#endif
	va_end(args);

	return buf;
}

/* Get around getmntinfo() not being thread safe */
#if defined(__FreeBSD__) || defined(__MAC__)
int _getmntinfo_r(struct statfs **mntbufp, int flags)
{
	static HMTX mutex = 0;
	int result;

	if(!mutex)
		mutex = dw_mutex_new();

	dw_mutex_lock(mutex);
	result = getmntinfo(mntbufp, flags);
	dw_mutex_unlock(mutex);
	return result;
}
#endif

#ifdef __MINGW32__
double API drivefree(int drive)
#else
long double API drivefree(int drive)
#endif
{
#if defined(__EMX__) || defined(__OS2__)
	ULONG   aulFSInfoBuf[40] = {0};
	APIRET  rc               = NO_ERROR;

	DosError(FERR_DISABLEHARDERR);
	rc = DosQueryFSInfo(drive,
						FSIL_ALLOC,
						(PVOID)aulFSInfoBuf,
						sizeof(aulFSInfoBuf));

	DosError(FERR_ENABLEHARDERR);
	if (rc != NO_ERROR)
		return 0;

	return (long double)((double)aulFSInfoBuf[3] * (double)aulFSInfoBuf[1] * (double)aulFSInfoBuf[4]);
#elif defined(__WIN32__) || defined(WINNT)
	char buffer[10] = "C:\\";
	DWORD spc, bps, fc, tc;

	buffer[0] = drive + 'A' - 1;

	if(GetDiskFreeSpace(buffer, &spc, &bps, &fc, &tc) == 0)
		return 0;

	return (long double)((double)spc*(double)bps*(double)fc);
#elif defined(__FreeBSD__) || defined(__MAC__)
	struct statfs *fsp = NULL;
	int entries, index = 1;

	entries = _getmntinfo_r(&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			return (long double)((double)fsp->f_bsize * (double)fsp->f_bavail);
		index++;
	}
	return 0;
#elif defined(__sun__)
	FILE *fp = fopen("/etc/mnttab", "r");
	struct mnttab mnt;
	struct statvfs sfs;
	int index = 1;

	if(fp)
	{
		while((getmntent(fp, &mnt) == 0))
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt.mnt_mountp)
				{
					if(!statvfs(mnt.mnt_mountp, &sfs))
					{
						size = (long double)((double)sfs.f_bsize * (double)sfs.f_bavail);
					}
				}
				fclose(fp);
				return size;
			}
			index++;          
		}
		fclose(fp);
	}
	return 0;
#else
	FILE *fp = setmntent(MOUNTED, "r");
	struct mntent mnt;
	struct statfs sfs;
	char buffer[1024];
	int index = 1;

	if(fp)
	{
		while(getmntent_r(fp, &mnt, buffer, sizeof(buffer)))
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt.mnt_dir)
				{
					if(!statfs(mnt.mnt_dir, &sfs))
					{
						size = (long double)((double)sfs.f_bsize * (double)sfs.f_bavail);
					}
				}
				endmntent(fp);
				return size;
			}
			index++;          
		}
		endmntent(fp);
	}
	return 0;
#endif
}

#ifdef __MINGW32__
double API drivesize(int drive)
#else
long double API drivesize(int drive)
#endif
{
#if defined(__EMX__) || defined(__OS2__)
	ULONG   aulFSInfoBuf[40] = {0};
	APIRET  rc               = NO_ERROR;

	DosError(FERR_DISABLEHARDERR);
	rc = DosQueryFSInfo(drive,
						FSIL_ALLOC,
						(PVOID)aulFSInfoBuf,
						sizeof(aulFSInfoBuf));

	DosError(FERR_ENABLEHARDERR);
	if (rc != NO_ERROR)
		return 0;

	return (long double)((double)aulFSInfoBuf[2] * (double)aulFSInfoBuf[1] * (double)aulFSInfoBuf[4]);
#elif defined(__WIN32__) || defined(WINNT)
	char buffer[10] = "C:\\";
	DWORD spc, bps, fc, tc;

	buffer[0] = drive + 'A' - 1;

	if(GetDiskFreeSpace(buffer, &spc, &bps, &fc, &tc) == 0)
		return 0;

	return (long double)((double)spc*(double)bps*(double)tc);
#elif defined(__FreeBSD__) || defined(__MAC__)
	struct statfs *fsp = NULL;
	int entries, index = 1;

	entries = _getmntinfo_r(&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			return (long double)((double)fsp->f_bsize * (double)fsp->f_blocks);
		index++;
	}
	return 0;
#elif defined(__sun__)
	FILE *fp = fopen("/etc/mnttab", "r");
	struct mnttab mnt;
	struct statvfs sfs;
	int index = 1;

	if(fp)
	{
		while(getmntent(fp, &mnt) == 0)
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt.mnt_mountp)
				{
					if(!statvfs(mnt.mnt_mountp, &sfs))
					{
						size = (long double)((double)sfs.f_bsize * (double)sfs.f_blocks);
					}
				}
				fclose(fp);
				return size;
			}
			index++;          
		}
		fclose(fp);
	}
	return 0;
#else
	FILE *fp = setmntent(MOUNTED, "r");
	struct mntent mnt;
	char buffer[1024];
	struct statfs sfs;
	int index = 1;

	if(fp)
	{
		while(getmntent_r(fp, &mnt, buffer, sizeof(buffer)))
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt.mnt_dir)
				{
					if(!statfs(mnt.mnt_dir, &sfs))
					{
						size = (long double)((double)sfs.f_bsize * (double)sfs.f_blocks);
					}
				}
				endmntent(fp);
				return size;
			}
			index++;          
		}
		endmntent(fp);
	}
	return 0;
#endif
}

int API isdrive(int drive)
{
#if defined(__EMX__) || defined(__OS2__)
	APIRET  rc               = NO_ERROR;
	FSINFO  volinfo;

	DosError(FERR_DISABLEHARDERR);
	rc = DosQueryFSInfo(drive,
			    FSIL_VOLSER,
			    (PVOID)&volinfo,
			    sizeof(FSINFO));

	DosError(FERR_ENABLEHARDERR);
	if (rc == NO_ERROR)
		return 1;

#elif defined(__WIN32__) || defined(WINNT)
	char buffer[10] = "C:\\", volname[100];
	DWORD spc, bps, fc;

	buffer[0] = drive + 'A' - 1;

	if(GetVolumeInformation(buffer, volname, 100, &spc, &bps, &fc, NULL, 0) != 0)
		return 1;
#elif defined(__FreeBSD__) || defined(__MAC__)
	struct statfs *fsp = NULL;
	int entries, index = 1;

	entries = _getmntinfo_r(&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive && fsp->f_blocks)
			return 1;
		index++;
	}
	return 0;
#elif defined(__sun__)
	FILE *fp = fopen("/etc/mnttab", "r");
	struct mnttab mnt;
	struct statvfs sfs;
	int index = 1;

	if(fp)
	{
		while(getmntent(fp, &mnt) == 0)
		{
			if(index == drive)
			{
				fclose(fp);
				if(mnt.mnt_mountp)
				{
					if(!statvfs(mnt.mnt_mountp, &sfs) && sfs.f_blocks)
						return 1;
				}
				return 0;
			}
			index++;          
		}
		fclose(fp);
	}
#else
	FILE *fp = setmntent(MOUNTED, "r");
	struct mntent mnt;
	char buffer[1024];
	struct statfs sfs;
	int index = 1;

	if(fp)
	{
		while(getmntent_r(fp, &mnt, buffer, sizeof(buffer)))
		{
			if(index == drive)
			{
				endmntent(fp);
				if(mnt.mnt_dir)
				{
					if(!statfs(mnt.mnt_dir, &sfs) && sfs.f_blocks)
					{
						return 1;
					}
				}
				return 0;
			}
			index++;          
		}
		endmntent(fp);
	}
#endif
	return 0;
}

void API getfsname(int drive, char *buf, int len)
{
#if defined(__UNIX__) || defined(__MAC__) 
#if defined(__FreeBSD__) || defined(__MAC__)
	struct statfs *fsp = NULL;
	int entries, index = 1;

	strncpy(buf, "Unknown", len);

	entries = _getmntinfo_r(&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			strncpy(buf, fsp->f_mntonname, len);
		index++;
	}
#elif defined(__sun__)
	FILE *fp = fopen("/etc/mnttab", "r");
	struct mnttab mnt;
	int index = 1;

	strncpy(buf, "Unknown", len);

	if(fp)
	{
		while(getmntent(fp, &mnt) == 0)
		{
			if(index == drive && mnt.mnt_mountp)
				strncpy(buf, mnt.mnt_mountp, len);
			index++;          
		}
		fclose(fp);
	}
#else
	FILE *fp = setmntent(MOUNTED, "r");
	struct mntent mnt;
	char buffer[1024];
	int index = 1;

	strncpy(buf, "Unknown", len);

	if(fp)
	{
		while(getmntent_r(fp, &mnt, buffer, sizeof(buffer)))
		{
			if(index == drive && mnt.mnt_dir)
				strncpy(buf, mnt.mnt_dir, len);
			index++;          
		}
		endmntent(fp);
	}
#endif
#elif defined(__OS2__)
	/* No snprintf() on OS/2 ??? */
	len = len;
	sprintf(buf, "Drive %c",  (char)drive + 'A' - 1);
#else
	_snprintf(buf, len, "Drive %c",  (char)drive + 'A' - 1);
#endif
}

void API setfileinfo(char *filename, char *url, char *logfile)
{
	time_t		ltime;
	struct tm	*tm;
    char buffer[200], timebuf[200];
#ifdef __OS2__
	const unsigned fea2listsize = 6000;
	char *pData;
	EAOP2 eaop2;
	PFEA2 pFEA2;
#else
	FILE *urlfile;
#endif

	ltime = time(NULL);

	tm = localtime(&ltime);

	strftime(timebuf, 200, "%c", tm);

	sprintf(buffer, "%s %s", url, timebuf);

#ifdef __OS2__
	logfile = logfile;
	eaop2.fpGEA2List = 0;
	eaop2.fpFEA2List = (PFEA2LIST)malloc(fea2listsize);
	pFEA2 = &eaop2.fpFEA2List->list[0];

	pFEA2->fEA = 0;
    /* .COMMENTS is 9 characters long */
	pFEA2->cbName = 9;

	/* space for the type and length field. */
	pFEA2->cbValue = strlen(buffer)+2*sizeof(USHORT);

	strcpy(pFEA2->szName, ".COMMENTS");
	pData = pFEA2->szName+pFEA2->cbName+1;
	/* data begins at first byte after the name */

	*(USHORT*)pData = EAT_ASCII;             /* type */
	*((USHORT*)pData+1) = strlen(buffer);  /* length */
	strcpy(pData+2*sizeof(USHORT), buffer);/* content */

	pFEA2->oNextEntryOffset = 0;

	eaop2.fpFEA2List->cbList = ((PCHAR)pData+2*sizeof(USHORT)+
									 pFEA2->cbValue)-((PCHAR)eaop2.fpFEA2List);

	DosSetPathInfo((PSZ)filename,
						FIL_QUERYEASIZE,
						&eaop2,
						sizeof(eaop2),
						0);

	free((void *)eaop2.fpFEA2List);
#else

	if((urlfile = fopen(logfile, "a"))!=NULL)
	{
		fprintf(urlfile, "%s\n", buffer);
		fclose(urlfile);
	}
#endif
}

#if defined(__OS2__) || defined(__WIN32__)
typedef struct _fsinfo {
	FILE *fp;
	int fd;
} FSInfo;

FSInfo *FSIRoot = NULL;

#define FSI_MAX 100
#endif

/* Sharable fopen() and fclose() calls. */
FILE * API fsopen(char *path, char *modes)
{
#if (defined(__OS2__) && !defined(__WATCOMC__)) || defined(__WIN32__)
	int z;

	if(!FSIRoot)
		FSIRoot = calloc(sizeof(struct _fsinfo), FSI_MAX);

	for(z=0;z<FSI_MAX;z++)
	{
		if(FSIRoot[z].fd < 1)
		{
			int s, sopenmode = 0, wrmode = 0;

			/* Check the flags passed */
			for(s=0;s<3;s++)
			{
				if(modes[s] == 'b')
					sopenmode |= O_BINARY;
				if(modes[s] == 'r')
					wrmode |= O_RDONLY;
				if(modes[s] == 'w')
					wrmode |= O_WRONLY;
				if(modes[s] == 'a')
					sopenmode |= O_APPEND;
				if(modes[s] == 't')
					sopenmode |= O_TEXT;
			}

			/* Check the read/write request */
			if((wrmode & O_RDONLY) && (wrmode & O_WRONLY))
				sopenmode |= O_RDWR;
			else
				sopenmode |= wrmode;
			FSIRoot[z].fd = _sopen(path, sopenmode, SH_DENYNO, S_IREAD|S_IWRITE);
			if(FSIRoot[z].fd > 0)
			{
				FSIRoot[z].fp = fdopen(FSIRoot[z].fd, modes);

				return FSIRoot[z].fp;
			}
		}
	}
	return NULL;
#else
	return fopen(path, modes);
#endif
}

int API fsclose(FILE *fp)
{
#if defined(__OS2__) || defined(__WIN32__)
	if(FSIRoot)
	{

		int z;
		for(z=0;z<FSI_MAX;z++)
		{
			if(fp == FSIRoot[z].fp)
			{
				int ret = fclose(fp);
				close(FSIRoot[z].fd);
				FSIRoot[z].fd = 0;
				FSIRoot[z].fp = NULL;
				return ret;
			}
		}
	}
#endif
	return fclose(fp);
}

char * API fsgets(char *str, int size, FILE *stream)
{
	return fgets(str, size, stream);
}

int API fsseek(FILE *stream, long offset, int whence)
{
	return fseek(stream, offset, whence);
}

void API nice_strformat(char *dest, long double val, int dec)
{
	char formatbuf[10];
	char format = 0;
	double printval;

	/* 1024 ^ 3 = Gigabytes */
	if(val >= 1073741824L)
	{
		printval = val/(1073741824L);
		format = 'G';
	}
	/* 1024 ^ 2 = Megabytes */
	else if(val >= 1048576)
	{
		printval = val/(1048576);
		format = 'M';
	}
	/* 1024 = Kilobytes */
	else if(val > 1024)
	{
		printval = val/1024;
		format = 'K';
	}
	else
		printval = val;

	/* Generate the format string */
	sprintf(formatbuf, "%%.%df%c", dec, format);
	/* Create the pretty value */
	sprintf(dest, formatbuf, printval);
}

/* Update the current working directory based on the
 * path of the executable being executed.
 */
void API initdir(int argc, char *argv[])
{
	if(argc > 0)
	{
		char *tmpdir = strdup(argv[0]);
		int z, len = (int)strlen(argv[0]);

		for(z=len;z > -1;z--)
		{
			if(tmpdir[z] == '/')
			{
				tmpdir[z+1] = 0;
				setpath(tmpdir);
				free(tmpdir);
				return;
			}
		}
		free(tmpdir);
	}
}

/*
 * Sets	the current directory (and drive) information.
 * Parameters:
 *	 path: A buffer	containing the new path.
 * Returns:
 *	 -1 on error.
 */
int API setpath(char *path)
{
#if defined(__OS2__) || defined(__WIN32__)
	if(strlen(path)	> 2)
	{
		if(path[1] == ':')
		{
			char drive = toupper(path[0]);
			_chdrive((drive - 'A')+1);
		}
	}
#endif
	return chdir(path);
}

static int locale_number = -1, locale_count = 0;
static char **locale_text = NULL;

void _compat_free_locale(void)
{
	if(locale_text)
	{
		int z;

		for(z=0;z<locale_count;z++)
		{
			if(locale_text[z])
				free(locale_text[z]);
		}
		free(locale_text);
		locale_text = NULL;
	}
}

int _stripcrlf(char *buf)
{
	int z, len = (int)strlen(buf);

	for(z=0;z<len;z++)
	{
		if(buf[z] == '\r' || buf[z] == '\n')
		{
			buf[z] = 0;
			return 1;
		}
	}
	return 1;
}

/* Initialize the locale engine
 * Returns: TRUE on success, FALSE on failure.
 */
int API locale_init(char *filename, int my_locale)
{
	FILE *fp = fopen(filename, FOPEN_READ_TEXT);
	static char text[1025];
	int count = 0;

	_compat_free_locale();

	if(fp)
	{
		if(fgets(text, 1024, fp) && strncasecmp(text, "MESSAGES=", 9) == 0 && (count = atoi(&text[9])) > 0)
		{
			int current = -1;

			locale_text = calloc(count, sizeof(char *));

			while(!feof(fp))
			{
				if(fgets(text, 1024, fp) && _stripcrlf(text) &&
				   strncasecmp(text, "LOCALE=", 7) == 0)
				{
					if(current > -1)
					{
						fclose(fp);
						locale_count = count;
						locale_number = my_locale;
						return 1;
					}
					if(atoi(&text[7]) == my_locale)
						current = 0;
				}
				else if(current > -1 && current < count)
				{
					/* Use defaults on blank lines */
					if(text[0])
					{
						int x = 0, z, len = (int)strlen(text);

						locale_text[current] = calloc(1, len + 1);

						for(z=0;z<len;z++)
						{
							if(text[z] == '\\' && (text[z+1] == 'r' || text[z+1] == 'n'
							   || text[z+1] == '\"'  || text[z+1] == '\''))
							{
								switch(text[z+1])
								{
								case 'r':
									locale_text[current][x] = '\r';
                                    break;
								case 'n':
									locale_text[current][x] = '\n';
                                    break;
								case '\"':
									locale_text[current][x] = '\"';
                                    break;
								case '\'':
									locale_text[current][x] = '\'';
                                    break;
								}
								x++;
								z++;
							}
							else
							{
								locale_text[current][x] = text[z];
								x++;
							}
						}
					}
					current++;
				}
			}
		}
		fclose(fp);
	}
	if(locale_text && count)
	{
		locale_count = count;
		locale_number = my_locale;
		return 1;
	}
	return 0;
}

/* Retrieve a localized string if available */
char * API locale_string(char *default_text, int message)
{
	if(locale_number > -1 && message < locale_count && message > -1 && locale_text[message])
		return locale_text[message];
	return default_text;
}

