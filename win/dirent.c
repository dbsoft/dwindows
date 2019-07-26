#undef UNICODE
#undef _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <winsock2.h>
#include <windows.h>

#define _DW_INTERNAL
#include "dwcompat.h"
#include <errno.h>
#include <direct.h>

#define error(rc) errno = 255

struct _dirdescr {
	HANDLE		handle;		/* DosFindFirst handle */
	char		fstype;		/* filesystem type */
	long		count;		/* valid entries in <ffbuf> */
	long		number;		/* absolute number of next entry */
	int		index;		/* relative number of next entry */
	char		name[MAXPATHLEN+3]; /* directory name */
	unsigned	attrmask;	/* attribute mask for seekdir */
	struct dirent	entry;		/* buffer for directory entry */
	WIN32_FIND_DATA data;
};

/*
 * Return first char of filesystem type, or 0 if unknown.
 */
static char getFSType(const char *path)
{
	static char cache[1+26];
	char drive[3];
	UCHAR unit;
	char r;

	if (isalpha(path[0]) && path[1] == ':') {
		unit = toupper(path[0]) - '@';
		path += 2;
	} else {
		return 0;
	}

	if ((path[0] == '\\' || path[0] == '/')
	 && (path[1] == '\\' || path[1] == '/'))
		return 0;

	if (cache [unit])
		return cache [unit];

	drive[0] = '@' + unit;
	drive[1] = ':';
	drive[2] = '\0';

	r = GetDriveType(drive);

	return cache [unit] = r;
}

char * API abs_path(const char *name, char *buffer, int len)
{
	LPTSTR file;

	if(isalpha(name[0]) && name[1] == ':' && name[2] == '\0')
	{
		int drive = _getdrive();
		char newdrive = toupper(name[0]);

		_chdrive((newdrive - 'A')+1);

		if(_getcwd(buffer, len))
		{
			_chdrive(drive);
			return buffer;
		}
		_chdrive(drive);
		return NULL;
	}
	if(GetFullPathName(name, len, buffer, &file))
		return buffer;
	return NULL;
}

DIR * API openxdir(const char *path, unsigned att_mask)
{
	DIR *dir;
	char name[MAXPATHLEN+3];

	dir = malloc(sizeof(DIR));
	if (dir == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	strncpy(name, path, MAXPATHLEN);
	name[MAXPATHLEN] = '\0';
	switch (name[strlen(name)-1]) {
	default:
		strcat(name, "\\");
	case '\\':
	case '/':
	case ':':
		;
	}
	strcat(name, ".");
	if (!abs_path(name, dir->name, MAXPATHLEN+1))
		strcpy(dir->name, name);
	if (dir->name[strlen(dir->name)-1] == '\\')
		strcat(dir->name, "*");
	else
		strcat(dir->name, "\\*");

	dir->fstype = getFSType(dir->name);
	dir->attrmask = att_mask | A_DIR;

	dir->count = 100;
	if((dir->handle = FindFirstFile(dir->name, &dir->data))==INVALID_HANDLE_VALUE)
	{
		free(dir);
		error(rc);
		return NULL;
	}

	dir->number = 0;
	dir->index = 0;

	return (DIR *)dir;
}

DIR * API opendir(const char *pathname)
{
	return openxdir(pathname, 0);
}

struct dirent * API readdir(DIR *dir)
{
	static int dummy_ino = 2;

	if (dir->number)
	{
		dir->count = 100;
		if(!FindNextFile(dir->handle, &(dir->data)))
		{
			error(rc);
			return NULL;
		}

		dir->index = 0;
	}

	strcpy(dir->entry.d_name, dir->data.cFileName);
	dir->entry.d_ino = dummy_ino++;
	dir->entry.d_reclen = (int)strlen(dir->data.cFileName);
	dir->entry.d_namlen = (int)strlen(dir->data.cFileName);
	dir->entry.d_size = dir->data.nFileSizeLow;
	dir->entry.d_attribute = dir->data.dwFileAttributes;
#if 0
	dir->entry.d_time = *(USHORT *)&dir->next->ftimeLastWrite;
	dir->entry.d_date = *(USHORT *)&dir->next->fdateLastWrite;
#endif

	dir->number++;
	dir->index++;
	return &dir->entry;
}

long API telldir(DIR *dir)
{
	return dir->number;
}

void API seekdir(DIR *dir, long off)
{
	if (dir->number > off) {
		char name[MAXPATHLEN+2];

		FindClose(dir->handle);

		strcpy(name, dir->name);
		strcat(name, "*");

		if((dir->handle = FindFirstFile(name, &(dir->data)))==NULL)
		{
			error(rc);
			return;
		}

		dir->number = 0;
		dir->index = 0;
	}

	while (dir->number < off && readdir(dir))
		;
}

void API closedir(DIR *dir)
{
	FindClose(dir->handle);
	free(dir);
}

/*****************************************************************************/

#ifdef TEST

main(int argc, char **argv)
{
	int i;
	DIR *dir;
	struct dirent *ep;

	for (i = 1; i < argc; ++i) {
		dir = opendir(argv[i]);
		if (!dir)
			continue;
		while (ep = readdir(dir))
			if (strchr("\\/:", argv[i] [strlen(argv[i]) - 1]))
				printf("%s%s\n", argv[i], ep->d_name);
			else
				printf("%s/%s\n", argv[i], ep->d_name);
		closedir(dir);
	}

	return 0;
}

#endif
