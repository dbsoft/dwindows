#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <windows.h>

#include "dirent.h"
#include <errno.h>

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
static char
getFSType(const char *path)
{
	static char cache[1+26];
	char drive[3];
	ULONG unit;
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

char *
abs_path(const char *name, char *buffer, int len)
{
	char buf[4];
	if (isalpha(name[0]) && name[1] == ':' && name[2] == '\0') {
		buf[0] = name[0];
		buf[1] = name[1];
		buf[2] = '.';
		buf[3] = '\0';
		name = buf;
	}
	if (GetLongPathName(name, buffer, len))
		return NULL;
	return buffer;
}

DIR *
openxdir(const char *path, unsigned att_mask)
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
	if((dir->handle = FindFirstFile(dir->name, &dir->data))==NULL)
	{
		free(dir);
		error(rc);
		return NULL;
	}

	dir->number = 0;
	dir->index = 0;

	return (DIR *)dir;
}

DIR *
opendir(const char *pathname)
{
	return openxdir(pathname, 0);
}

struct dirent *
readdir(DIR *dir)
{
	static int dummy_ino = 2;

	if (dir->number)
	{
		ULONG rc;
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
	dir->entry.d_reclen = strlen(dir->data.cFileName);
	dir->entry.d_namlen = strlen(dir->data.cFileName);
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

long
telldir(DIR *dir)
{
	return dir->number;
}

void
seekdir(DIR *dir, long off)
{
	if (dir->number > off) {
		char name[MAXPATHLEN+2];
		ULONG rc;

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

void
closedir(DIR *dir)
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
