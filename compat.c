/* $Id$ */

#include "compat.h"
#if defined(__OS2__) || defined(__WIN32__)
#include <share.h>
#endif

#ifdef __UNIX__
#ifdef __FreeBSD__
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

#ifdef __UNIX__
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

int	sockread (int a, void *b, int c, int d)
{
#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return recv(a,b,c,d);
#else
	return read(a,b,c);
#endif
}

int	sockwrite (int a, void *b, int c, int d)
{
#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return send(a,b,c,d);
#else
	return write(a,b,c);
#endif
}

int	sockclose(int a)
{
#ifdef __IBMC__
	return soclose(a);
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	return closesocket(a);
#else
	return close(a);
#endif
}

int makedir(char *path)
{
#if defined(__IBMC__) || (defined(__WIN32__) && !defined(__CYGWIN32__))
	return mkdir(path);
#else
	return mkdir(path,S_IRWXU);
#endif
}

void nonblock(int fd)
{
#ifdef __IBMC__
	static int _nonblock = 1;

	ioctl(fd, FIONBIO, (char *)&_nonblock, sizeof(_nonblock));
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	static unsigned long _nonblock = 1;

	ioctlsocket(fd, FIONBIO, &_nonblock);
#else
	fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
}

void block(int fd)
{
#ifdef __IBMC__
	static int _nonblock = 0;

	ioctl(fd, FIONBIO, (char *)&_nonblock, sizeof(_nonblock));
#elif defined(__WIN32__) && !defined(__CYGWIN32__)
	static unsigned long _nonblock = 0;

	ioctlsocket(fd, FIONBIO, &_nonblock);
#else
	fcntl(fd, F_SETFL, 0);
#endif
}

int socksprintf(int fd, char *format, ...)
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

void sockinit(void)
{
#ifdef __IBMC__
	sock_init();
#elif defined(__WIN32__) || defined(WINNT)
    WSADATA wsa;

    WSAStartup(MAKEWORD (1, 1), &wsa);
#endif /* !WIN32 */
}

void sockshutdown(void)
{
#if defined(__WIN32__) || defined(WINNT)
    WSACleanup();
#endif /* !WIN32 */
}

int sockpipe(int *pipes)
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

/* Return in K to avoid big problems exceeding an
   unsigned long when no 64bit integers are available */
#if (defined(__IBMC__) && __IBMC__ < 360) || (defined(__WIN32__) && !defined(__CYGWIN32__))
unsigned long drivefree(int drive)
#else
unsigned long long drivefree(int drive)
#endif
{
#if defined(__EMX__) || defined(__OS2__)
	ULONG   aulFSInfoBuf[40] = {0};
	APIRET  rc               = NO_ERROR;
	ULONG kbytes;

	DosError(FERR_DISABLEHARDERR);
	rc = DosQueryFSInfo(drive,
						FSIL_ALLOC,
						(PVOID)aulFSInfoBuf,
						sizeof(aulFSInfoBuf));

	DosError(FERR_ENABLEHARDERR);
	if (rc != NO_ERROR)
		return 0;

	kbytes = aulFSInfoBuf[3]/1024;

	return (kbytes * aulFSInfoBuf[1] * aulFSInfoBuf[4]);
#elif defined(__WIN32__) || defined(WINNT)
	char buffer[10] = "C:\\";
	DWORD spc, bps, fc, tc;
	ULONG kbytes;

	buffer[0] = drive + 'A' - 1;

	if(GetDiskFreeSpace(buffer, &spc, &bps, &fc, &tc) == 0)
		return 0;

	kbytes = fc/1024;

	return (spc*bps*kbytes);
#elif defined(__FreeBSD__)
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			return (fsp->f_bsize * fsp->f_bavail) / 1024;
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
				long long size = 0;

				if(mnt.mnt_mountp)
				{
					statfs(mnt.mnt_mountp, &sfs, sizeof(struct statfs), 0);
					size = sfs.f_bsize * (sfs.f_bfree / 1024);
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
				long long size = 0;

				if(mnt->mnt_dir)
				{
					statfs(mnt->mnt_dir, &sfs);
					size = sfs.f_bsize * (sfs.f_bavail / 1024);
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

/* Return in K to avoid big problems exceeding an
   unsigned long when no 64bit integers are available */
#if (defined(__IBMC__) && __IBMC__  < 360) || (defined(__WIN32__) && !defined(__CYGWIN32__))
unsigned long drivesize(int drive)
#else
unsigned long long drivesize(int drive)
#endif
{
#if defined(__EMX__) || defined(__OS2__)
	ULONG   aulFSInfoBuf[40] = {0};
	APIRET  rc               = NO_ERROR;
	ULONG kbytes;

	DosError(FERR_DISABLEHARDERR);
	rc = DosQueryFSInfo(drive,
						FSIL_ALLOC,
						(PVOID)aulFSInfoBuf,
						sizeof(aulFSInfoBuf));

	DosError(FERR_ENABLEHARDERR);
	if (rc != NO_ERROR)
		return 0;

	kbytes = aulFSInfoBuf[2]/1024;

	return (kbytes * aulFSInfoBuf[1] * aulFSInfoBuf[4]);
#elif defined(__WIN32__) || defined(WINNT)
	char buffer[10] = "C:\\";
	DWORD spc, bps, fc, tc;
	ULONG kbytes;

	buffer[0] = drive + 'A' - 1;

	if(GetDiskFreeSpace(buffer, &spc, &bps, &fc, &tc) == 0)
		return 0;

	kbytes = tc/1024;

	return (spc*bps*kbytes);
#elif defined(__FreeBSD__)
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			return (fsp->f_bsize * fsp->f_blocks) / 1024;
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
				long long size = 0;

				if(mnt.mnt_mountp)
				{
					statfs(mnt.mnt_mountp, &sfs, sizeof(struct statfs), 0);
					size = sfs.f_bsize * (sfs.f_blocks / 1024);
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
				long long size = 0;

				if(mnt->mnt_dir)
				{
					statfs(mnt->mnt_dir, &sfs);
					size = sfs.f_bsize * (sfs.f_blocks / 1024);
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

int isdrive(int drive)
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
#elif defined(__FreeBSD__)
	struct statfs *fsp;
	int entries, index = 1;

	entries = getmntinfo (&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive && fsp->f_blocks)
			return 1;
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

void getfsname(int drive, char *buf, int len)
{
#ifdef __UNIX__
#ifdef __FreeBSD__
	struct statfs *fsp;
	int entries, index = 1;

	strncpy(buf, "Unknown", len);

	entries = getmntinfo (&fsp, MNT_NOWAIT);

	for (; entries-- > 0; fsp++)
	{
		if(index == drive)
			strncpy(buf, fsp->f_mntonname, len);
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
	sprintf(buf, "Drive %c",  (char)drive + 'A' - 1);
#else
	_snprintf(buf, len, "Drive %c",  (char)drive + 'A' - 1);
#endif
}

void setfileinfo(char *filename, char *url, char *logfile)
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
FILE *fsopen(char *path, char *modes)
{
#if defined(__OS2__) || defined(__WIN32__)
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

int fsclose(FILE *fp)
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

char *fsgets(char *str, int size, FILE *stream)
{
	return fgets(str, size, stream);
}

int fsseek(FILE *stream, long offset, int whence)
{
	return fseek(stream, offset, whence);
}
