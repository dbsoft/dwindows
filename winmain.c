/* Dynamic Windows stub file to allow Win32 applications
 * to use the main() entry point instead of WinMain().
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <process.h>

void Win32_Set_Instance(HINSTANCE hInstance);

char **_convertargs(int *count, char *start, HINSTANCE DWInstance)
{
	char *tmp, *argstart, **argv;
	int loc = 0, inquotes = 0;

	(*count) = 1;

	tmp = start;

	/* Count the number of entries */
	if(*start)
	{
		(*count)++;

		while(*tmp)
		{
			if(*tmp == '"' && inquotes)
				inquotes = 0;
			else if(*tmp == '"' && !inquotes)
				inquotes = 1;
			else if(*tmp == ' ' && !inquotes)
			{
				/* Push past any white space */
				while(*(tmp+1) == ' ')
					tmp++;
				/* If we aren't at the end of the command
				 * line increment the count.
				 */
				if(*(tmp+1))
					(*count)++;
			}
			tmp++;
		}
	}

	argv = (char **)malloc(sizeof(char *) * ((*count)+1));
	argv[0] = malloc(260);
	GetModuleFileName(DWInstance, argv[0], 260);

	argstart = tmp = start;

	if(*start)
	{
		loc = 1;

		while(*tmp)
		{
			if(*tmp == '"' && inquotes)
			{
				*tmp = 0;
				inquotes = 0;
			}
			else if(*tmp == '"' && !inquotes)
			{
				argstart = tmp+1;
				inquotes = 1;
			}
			else if(*tmp == ' ' && !inquotes)
			{
				*tmp = 0;
				argv[loc] = strdup(argstart);

				/* Push past any white space */
				while(*(tmp+1) == ' ')
					tmp++;

				/* Move the start pointer */
				argstart = tmp+1;

				/* If we aren't at the end of the command
				 * line increment the count.
				 */
				if(*(tmp+1))
					loc++;
			}
			tmp++;
		}
		if(*argstart)
			argv[loc] = strdup(argstart);
	}
	argv[loc+1] = NULL;
	return argv;
}

/* Ok this is a really big hack but what the hell ;) */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char **argv;
	int argc;

	Win32_Set_Instance(hInstance);

	argv = _convertargs(&argc, lpCmdLine, hInstance);

	return main(argc, argv);
}
