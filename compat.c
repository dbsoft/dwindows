/* $Id$ */

#include "compat.h"
#if defined(__OS2__) || defined(__WIN32__)
#include <share.h>
#endif

#if defined(__UNIX__) || defined(__MAC__)
#if defined(__FreeBSD__) || defined(__MAC__)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#elif defined(__sun__)
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#else
#include <mntent.h>
#include <sys/vfs.h>
#endif
#endif
#include <time.h>

#if defined(__UNIX__) || defined(__MAC__)
void msleep(long period)
{
#ifdef __sun__
	/* usleep() isn't threadsafe on Solaris */
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = period * 10000000;

	nanosleep(&req, NULL);
#else
	usleep(period * 1000);
#endif
}
#endif

int	API sockread (int a, void *b, int c, int d)
{
#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return recv(a,b,c,d);
#else
	return read(a,b,c);
#endif
}

int	API sockwrite (int a, void *b, int c, int d)
{
#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return send(a,b,c,d);
#else
	return write(a,b,c);
#endif
}

int	API sockclose(int a)
{
#ifdef __IBMC__
	return soclose(a);
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	return closesocket(a);
#else
	return close(a);
#endif
}

int API makedir(char *path)
{
#if defined(__IBMC__) || defined(__WATCOMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return mkdir(path);
#else
	return mkdir(path,S_IRWXU);
#endif
}

void API nonblock(int fd)
{
#if defined(__OS2__) && !defined(__EMX__)
	static int _nonblock = 1;

	ioctl(fd, FIONBIO, (char *)&_nonblock, sizeof(_nonblock));
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	static unsigned long _nonblock = 1;

	ioctlsocket(fd, FIONBIO, &_nonblock);
#else
	fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
}

void API block(int fd)
{
#if defined(__OS2__) && !defined(__EMX__)
	static int _nonblock = 0;

	ioctl(fd, FIONBIO, (char *)&_nonblock, sizeof(_nonblock));
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	static unsigned long _nonblock = 0;

	ioctlsocket(fd, FIONBIO, &_nonblock);
#else
	fcntl(fd, F_SETFL, 0);
#endif
}

int API socksprintf(int fd, char *format, ...)
{
	va_list args;
	char outbuf[1024];
	int len;

	va_start(args, format);
	vsprintf(outbuf, format, args);
	va_end(args);

	len = strlen(outbuf);
	sockwrite(fd, outbuf, len, 0);

	return len;
}

void API sockinit(void)
{
#ifdef __IBMC__
	sock_init();
#elif defined(__WIN32__) || defined(WINNT)
    WSADATA wsa;

    WSAStartup(MAKEWORD (1, 1), &wsa);
#endif /* !WIN32 */
}

void API sockshutdown(void)
{
#if defined(__WIN32__) || defined(WINNT)
    WSACleanup();
#endif /* !WIN32 */
}

int API sockpipe(int *pipes)
{
#ifndef NO_DOMAIN_SOCKETS
#ifndef HAVE_PIPE
	struct sockaddr_un un;
#endif
#else
	struct sockaddr_in server_addr;
	struct sockaddr_in listen_addr = { 0 };
	int len = sizeof(struct sockaddr_in);
	struct hostent *he;
#endif
#ifndef HAVE_PIPE
	int tmpsock;
#endif	

#ifdef HAVE_PIPE
    return pipe(pipes);
#elif !defined(NO_DOMAIN_SOCKETS)
	static int instance = -1;

	instance++;

	/* Use UNIX domain sockets to pass messages */
	tmpsock = socket(AF_UNIX, SOCK_STREAM, 0);
	pipes[1] = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&un, 0, sizeof(un));
	un.sun_family=AF_UNIX;
	sprintf(un.sun_path, PIPENAME, instance);
	bind(tmpsock, (struct sockaddr *)&un, sizeof(un));
	listen(tmpsock, 0);
	connect(pipes[1], (struct sockaddr *)&un, sizeof(un));
	pipes[0] = accept(tmpsock, 0, 0);
	sockclose(tmpsock);
#else
	/* Use localhost socket to pass messages if no domain sockets */
	he = gethostbyname("localhost");

	if(he)
	{
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port   = 0;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		if ((tmpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||  bind(tmpsock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 || listen(tmpsock, 0) < 0)
			return -1;

		memset(&listen_addr, 0, sizeof(listen_addr));
		getsockname(tmpsock, (struct sockaddr *)&listen_addr, &len);

		server_addr.sin_family      = AF_INET;
		server_addr.sin_port        = listen_addr.sin_port;
		server_addr.sin_addr.s_addr = *((unsigned long *)he->h_addr);
		if((pipes[1] = socket(AF_INET, SOCK_STREAM, 0)) < 0 || connect(pipes[1], (struct sockaddr *)&server_addr, sizeof(server_addr)))
			return -1;
		else
			pipes[0] = accept(tmpsock, 0, 0);
		sockclose(tmpsock);
	}
	else
		return -1;
#endif
	if(pipes[0] < 0 || pipes[1] < 0)
		return -1;
	return 0;
}

long double API drivefree(int drive)
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
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

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
	struct statfs sfs;
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
					statfs(mnt.mnt_mountp, &sfs, sizeof(struct statfs), 0);
					size = (long double)((double)sfs.f_bsize * (double)sfs.f_bfree);
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
	struct mntent *mnt;
	struct statfs sfs;
	int index = 1;

	if(fp)
	{
		while((mnt = getmntent(fp)))
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt->mnt_dir)
				{
					statfs(mnt->mnt_dir, &sfs);
					size = (long double)((double)sfs.f_bsize * (double)sfs.f_bavail);
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

long double API drivesize(int drive)
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
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

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
	struct statfs sfs;
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
					statfs(mnt.mnt_mountp, &sfs, sizeof(struct statfs), 0);
					size = (long double)((double)sfs.f_bsize * (double)sfs.f_blocks);
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
	struct mntent *mnt;
	struct statfs sfs;
	int index = 1;

	if(fp)
	{
		while((mnt = getmntent(fp)))
		{
			if(index == drive)
			{
				long double size = 0;

				if(mnt->mnt_dir)
				{
					statfs(mnt->mnt_dir, &sfs);
					size = (long double)((double)sfs.f_bsize * (double)sfs.f_blocks);
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
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

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
	struct statfs sfs;
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
					statfs(mnt.mnt_mountp, &sfs, sizeof(struct statfs), 0);
					if(sfs.f_blocks)
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
	struct mntent *mnt;
	struct statfs sfs;
	int index = 1;

	if(fp)
	{
		while((mnt = getmntent(fp)))
		{
			if(index == drive)
			{
				endmntent(fp);
				if(mnt->mnt_dir)
				{
					statfs(mnt->mnt_dir, &sfs);
					if(sfs.f_blocks)
						return 1;
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
	struct statfs *fsp;
	int entries, index = 1;

	strncpy(buf, "Unknown", len);

	entries = getmntinfo (&fsp, MNT_NOWAIT);

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
	struct mntent *mnt;
	int index = 1;

	strncpy(buf, "Unknown", len);

	if(fp)
	{
		while((mnt = getmntent(fp)))
		{
			if(index == drive && mnt->mnt_dir)
				strncpy(buf, mnt->mnt_dir, len);
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

	DosSetPathInfo(filename,
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
		int z, len = strlen(argv[0]);

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

void _free_locale(void)
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

void _stripcrlf(char *buf)
{
	int z, len = strlen(buf);

	for(z=0;z<len;z++)
	{
		if(buf[z] == '\r' || buf[z] == '\n')
		{
			buf[z] = 0;
			return;
		}
	}
}

#ifdef __WIN32__
#define LOCALE_CHARACTERS 62
static char locale_table[LOCALE_CHARACTERS * 2] = {
	0xc0, 0xb7, 0xc1, 0xb5, 0xc2, 0xb6, 0xc3, 0xc7, 0xc4, 0x8e, 0xc5, 0x8f,
	0xc6, 0x92, 0xc7, 0x80, 0xc8, 0xd4, 0xc9, 0x90, 0xcb, 0xd3, 0xcc, 0xde,
	0xcd, 0xd6, 0xce, 0xd7, 0xcf, 0xd8, 0xd0, 0xd1, 0xd1, 0xa5, 0xd2, 0xe3,
	0xd3, 0xe0, 0xd4, 0xe2, 0xd5, 0xe5, 0xd6, 0x99, 0xd8, 0x9d, 0xd9, 0xeb,
	0xda, 0xe9, 0xdb, 0xea, 0xdc, 0x9a, 0xde, 0xed, 0xde, 0xe8, 0xdf, 0xe1,
	0xe0, 0x85, 0xe1, 0xa0, 0xe2, 0x83, 0xe3, 0xc6, 0xe4, 0x84, 0xe5, 0x86,
	0xe6, 0x91, 0xe7, 0x87, 0xe8, 0x8a, 0xe9, 0x82, 0xea, 0x88, 0xeb, 0x89,
	0xec, 0x8d, 0xed, 0xa1, 0xee, 0x8c, 0xef, 0x8b, 0xf0, 0xd0, 0xf1, 0xa4,
	0xf2, 0x95, 0xf3, 0xa3, 0xf4, 0x93, 0xf5, 0xe4, 0xf6, 0x94, 0xf7, 0xf6,
	0xf8, 0x9b, 0xf9, 0x97, 0xfa, 0xa3, 0xfb, 0x96, 0xfc, 0x81, 0xfd, 0xec,
    0xfe, 0xe7, 0xff, 0x9e

};

char locale_convert(int codepage, char c)
{
	int z;

	for(z=0;z<LOCALE_CHARACTERS;z++)
	{
		if(locale_table[(z*2)+1] == c)
			return locale_table[z*2];
	}
	return c;
}
#endif

/* Initialize the locale engine
 * Returns: TRUE on success, FALSE on failure.
 */
int API locale_init(char *filename, int my_locale)
{
	FILE *fp = fopen(filename, FOPEN_READ_TEXT);
	static char text[1025];
	int count = 0;

	_free_locale();

	if(fp)
	{

		fgets(text, 1024, fp);
		if(strncasecmp(text, "MESSAGES=", 9) == 0 && (count = atoi(&text[9])) > 0)
		{
			int current = -1;

			locale_text = calloc(count, sizeof(char *));

			while(!feof(fp))
			{
				fgets(text, 1024, fp);
				_stripcrlf(text);

				if(strncasecmp(text, "LOCALE=", 7) == 0)
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
						int x = 0, z, len = strlen(text);

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
#ifdef __WIN32__
								locale_text[current][x] = locale_convert(1252, text[z]);
#else
								locale_text[current][x] = text[z];
#endif
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

