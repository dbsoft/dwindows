/*
 * Dynamic Windows:
 *          A GTK like implementation of the MacOS GUI
 *
 * (C) 2003-2004 Brian Smith <dbsoft@technologist.com>
 *
 */
#include "dw.h"

static void _do_resize(Box *thisbox, int x, int y);

typedef struct _sighandler
{
	struct _sighandler	*next;
	ULONG message;
	HWND window;
	int id;
	void *signalfunction;
	void *data;

} SignalHandler;

SignalHandler *Root = NULL;

typedef struct
{
	ULONG message;
	char name[30];

} SignalList;

const Rect CreationRect = { 0, 0, 2000, 1000 };
WindowRef CreationWindow = 0;
ControlRef RootControl = 0;

/* List of signals and their equivilent MacOS event */
#define SIGNALMAX 15

SignalList SignalTranslate[SIGNALMAX] = {
	{ nullEvent,       DW_SIGNAL_CONFIGURE },
	{ keyUp,           DW_SIGNAL_KEY_PRESS },
	{ mouseDown,       DW_SIGNAL_BUTTON_PRESS },
	{ mouseUp,         DW_SIGNAL_BUTTON_RELEASE },
	{ nullEvent,       DW_SIGNAL_MOTION_NOTIFY },
	{ nullEvent,       DW_SIGNAL_DELETE },
	{ updateEvt,       DW_SIGNAL_EXPOSE },
	{ nullEvent,       DW_SIGNAL_CLICKED },
	{ nullEvent,       DW_SIGNAL_ITEM_ENTER },
	{ nullEvent,       DW_SIGNAL_ITEM_CONTEXT },
	{ nullEvent,       DW_SIGNAL_LIST_SELECT },
	{ nullEvent,       DW_SIGNAL_ITEM_SELECT },
	{ activateEvt,     DW_SIGNAL_SET_FOCUS },
	{ nullEvent,       DW_SIGNAL_VALUE_CHANGED },
	{ nullEvent,       DW_SIGNAL_SWITCH_PAGE }
};

/* This function adds a signal handler callback into the linked list.
 */
void _new_signal(ULONG message, HWND window, int id, void *signalfunction, void *data)
{
	SignalHandler *new = malloc(sizeof(SignalHandler));

	new->message = message;
	new->window = window;
	new->id = id;
	new->signalfunction = signalfunction;
	new->data = data;
	new->next = NULL;

	if (!Root)
		Root = new;
	else
	{
		SignalHandler *prev = NULL, *tmp = Root;
		while(tmp)
		{
			if(tmp->message == message &&
			   tmp->window == window &&
			   tmp->signalfunction == signalfunction)
			{
				tmp->data = data;
				free(new);
				return;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		if(prev)
			prev->next = new;
		else
			Root = new;
	}
}

/* Finds the message number for a given signal name */
static ULONG _findsigmessage(char *signame)
{
	int z;

	for(z=0;z<SIGNALMAX;z++)
	{
		if(strcasecmp(signame, SignalTranslate[z].name) == 0)
			return SignalTranslate[z].message;
	}
	return 0L;
}

static void *_get_window_pointer(HWND handle)
{
	void *ret = NULL;

	if(IsValidWindowRef((WindowRef)handle))
		GetWindowProperty((WindowRef)handle, 0, 'user', sizeof(void *), NULL, &ret);
	else
		GetControlProperty(handle, 0, 'user', sizeof(void *), NULL, &ret);
	return ret;
}

static void _set_window_pointer(HWND handle, void *pointer)
{
	if(IsValidWindowRef((WindowRef)handle))
		SetWindowProperty((WindowRef)handle, 0, 'user', sizeof(void *), &pointer);
	else
		SetControlProperty(handle, 0, 'user', sizeof(void *), &pointer);
}

/* This function will recursively search a box and add up the total height of it */
static void _count_size(HWND box, int type, int *xsize, int *xorigsize)
{
	int size = 0, origsize = 0, z;
	Box *tmp = _get_window_pointer(box);

	if(!tmp)
	{
		*xsize = *xorigsize = 0;
		return;
	}

	if(type == tmp->type)
	{
		/* If the box is going in the direction we want, then we
		 * return the entire sum of the items.
		 */
		for(z=0;z<tmp->count;z++)
		{
			if(tmp->items[z].type == TYPEBOX)
			{
				int s, os;

				_count_size(tmp->items[z].hwnd, type, &s, &os);
				size += s;
				origsize += os;
			}
			else
			{
				size += (type == DW_HORZ ? tmp->items[z].width : tmp->items[z].height);
				origsize += (type == DW_HORZ ? tmp->items[z].origwidth : tmp->items[z].origheight);
			}
		}
	}
	else
	{
		/* If the box is not going in the direction we want, then we only
		 * want to return the maximum value.
		 */
		int tmpsize = 0, tmporigsize = 0;

		for(z=0;z<tmp->count;z++)
		{
			if(tmp->items[z].type == TYPEBOX)
				_count_size(tmp->items[z].hwnd, type, &tmpsize, &tmporigsize);
			else
			{
				tmpsize = (type == DW_HORZ ? tmp->items[z].width : tmp->items[z].height);
				tmporigsize = (type == DW_HORZ ? tmp->items[z].origwidth : tmp->items[z].origheight);
			}

			if(tmpsize > size)
				size = tmpsize;
		}
	}

	*xsize = size;
	*xorigsize = origsize;
}

/* This function calculates how much space the widgets and boxes require
 * and does expansion as necessary.
 */
static int _resize_box(Box *thisbox, int *depth, int x, int y, int *usedx, int *usedy,
				int pass, int *usedpadx, int *usedpady)
{
	int z, currentx = 0, currenty = 0;
	int uymax = 0, uxmax = 0;
	int upymax = 0, upxmax = 0;
	/* Used for the SIZEEXPAND */
	int nux = *usedx, nuy = *usedy;
	int nupx = *usedpadx, nupy = *usedpady;

	(*usedx) += (thisbox->pad * 2);
	(*usedy) += (thisbox->pad * 2);

	for(z=0;z<thisbox->count;z++)
	{
		if(thisbox->items[z].type == TYPEBOX)
		{
			int initialx, initialy;
			Box *tmp = _get_window_pointer(thisbox->items[z].hwnd);

			initialx = x - (*usedx);
			initialy = y - (*usedy);

			if(tmp)
			{
				int newx, newy;
				int nux = *usedx, nuy = *usedy;
				int upx = *usedpadx + (tmp->pad*2), upy = *usedpady + (tmp->pad*2);

				/* On the second pass we know how big the box needs to be and how
				 * much space we have, so we can calculate a ratio for the new box.
				 */
				if(pass == 2)
				{
					int deep = *depth + 1;

					_resize_box(tmp, &deep, x, y, &nux, &nuy, 1, &upx, &upy);

					tmp->upx = upx - *usedpadx;
					tmp->upy = upy - *usedpady;

					newx = x - nux;
					newy = y - nuy;

					tmp->width = thisbox->items[z].width = initialx - newx;
					tmp->height = thisbox->items[z].height = initialy - newy;

					tmp->parentxratio = thisbox->xratio;
					tmp->parentyratio = thisbox->yratio;

					tmp->parentpad = tmp->pad;

					/* Just in case */
					tmp->xratio = thisbox->xratio;
					tmp->yratio = thisbox->yratio;

					if(thisbox->type == DW_VERT)
					{
						if((thisbox->items[z].width-((thisbox->items[z].pad*2)+(tmp->pad*2)))!=0)
							tmp->xratio = ((float)((thisbox->items[z].width * thisbox->xratio)-((thisbox->items[z].pad*2)+(tmp->pad*2))))/((float)(thisbox->items[z].width-((thisbox->items[z].pad*2)+(tmp->pad*2))));
					}
					else
					{
						if((thisbox->items[z].width-tmp->upx)!=0)
							tmp->xratio = ((float)((thisbox->items[z].width * thisbox->xratio)-tmp->upx))/((float)(thisbox->items[z].width-tmp->upx));
					}
					if(thisbox->type == DW_HORZ)
					{
						if((thisbox->items[z].height-((thisbox->items[z].pad*2)+(tmp->pad*2)))!=0)
							tmp->yratio = ((float)((thisbox->items[z].height * thisbox->yratio)-((thisbox->items[z].pad*2)+(tmp->pad*2))))/((float)(thisbox->items[z].height-((thisbox->items[z].pad*2)+(tmp->pad*2))));
					}
					else
					{
						if((thisbox->items[z].height-tmp->upy)!=0)
							tmp->yratio = ((float)((thisbox->items[z].height * thisbox->yratio)-tmp->upy))/((float)(thisbox->items[z].height-tmp->upy));
					}

					nux = *usedx; nuy = *usedy;
					upx = *usedpadx + (tmp->pad*2); upy = *usedpady + (tmp->pad*2);
				}

				(*depth)++;

				_resize_box(tmp, depth, x, y, &nux, &nuy, pass, &upx, &upy);

				(*depth)--;

				newx = x - nux;
				newy = y - nuy;

				tmp->minwidth = thisbox->items[z].width = initialx - newx;
				tmp->minheight = thisbox->items[z].height = initialy - newy;
			}
		}

		if(pass > 1 && *depth > 0)
		{
			if(thisbox->type == DW_VERT)
			{
				if((thisbox->minwidth-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))) == 0)
					thisbox->items[z].xratio = 1.0;
				else
					thisbox->items[z].xratio = ((float)((thisbox->width * thisbox->parentxratio)-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))))/((float)(thisbox->minwidth-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))));
			}
			else
			{
				if(thisbox->minwidth-thisbox->upx == 0)
					thisbox->items[z].xratio = 1.0;
				else
					thisbox->items[z].xratio = ((float)((thisbox->width * thisbox->parentxratio)-thisbox->upx))/((float)(thisbox->minwidth-thisbox->upx));
			}

			if(thisbox->type == DW_HORZ)
			{
				if((thisbox->minheight-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))) == 0)
					thisbox->items[z].yratio = 1.0;
				else
					thisbox->items[z].yratio = ((float)((thisbox->height * thisbox->parentyratio)-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))))/((float)(thisbox->minheight-((thisbox->items[z].pad*2)+(thisbox->parentpad*2))));
			}
			else
			{
				if(thisbox->minheight-thisbox->upy == 0)
					thisbox->items[z].yratio = 1.0;
				else
					thisbox->items[z].yratio = ((float)((thisbox->height * thisbox->parentyratio)-thisbox->upy))/((float)(thisbox->minheight-thisbox->upy));
			}

			if(thisbox->items[z].type == TYPEBOX)
			{
				Box *tmp = _get_window_pointer(thisbox->items[z].hwnd);

				if(tmp)
				{
					tmp->parentxratio = thisbox->items[z].xratio;
					tmp->parentyratio = thisbox->items[z].yratio;
				}
			}
		}
		else
		{
			thisbox->items[z].xratio = thisbox->xratio;
			thisbox->items[z].yratio = thisbox->yratio;
		}

		if(thisbox->type == DW_VERT)
		{
			if((thisbox->items[z].width + (thisbox->items[z].pad*2)) > uxmax)
				uxmax = (thisbox->items[z].width + (thisbox->items[z].pad*2));
			if(thisbox->items[z].hsize != SIZEEXPAND)
			{
				if(((thisbox->items[z].pad*2) + thisbox->items[z].width) > upxmax)
					upxmax = (thisbox->items[z].pad*2) + thisbox->items[z].width;
			}
			else
			{
				if(thisbox->items[z].pad*2 > upxmax)
					upxmax = thisbox->items[z].pad*2;
			}
		}
		else
		{
			if(thisbox->items[z].width == -1)
			{
				/* figure out how much space this item requires */
				/* thisbox->items[z].width = */
			}
			else
			{
				(*usedx) += thisbox->items[z].width + (thisbox->items[z].pad*2);
				if(thisbox->items[z].hsize != SIZEEXPAND)
					(*usedpadx) += (thisbox->items[z].pad*2) + thisbox->items[z].width;
				else
					(*usedpadx) += thisbox->items[z].pad*2;
			}
		}
		if(thisbox->type == DW_HORZ)
		{
			if((thisbox->items[z].height + (thisbox->items[z].pad*2)) > uymax)
				uymax = (thisbox->items[z].height + (thisbox->items[z].pad*2));
			if(thisbox->items[z].vsize != SIZEEXPAND)
			{
				if(((thisbox->items[z].pad*2) + thisbox->items[z].height) > upymax)
					upymax = (thisbox->items[z].pad*2) + thisbox->items[z].height;
			}
			else
			{
				if(thisbox->items[z].pad*2 > upymax)
					upymax = thisbox->items[z].pad*2;
			}
		}
		else
		{
			if(thisbox->items[z].height == -1)
			{
				/* figure out how much space this item requires */
				/* thisbox->items[z].height = */
			}
			else
			{
				(*usedy) += thisbox->items[z].height + (thisbox->items[z].pad*2);
				if(thisbox->items[z].vsize != SIZEEXPAND)
					(*usedpady) += (thisbox->items[z].pad*2) + thisbox->items[z].height;
				else
					(*usedpady) += thisbox->items[z].pad*2;
			}
		}
	}

	(*usedx) += uxmax;
	(*usedy) += uymax;
	(*usedpadx) += upxmax;
	(*usedpady) += upymax;

	currentx += thisbox->pad;
	currenty += thisbox->pad;

	/* The second pass is for expansion and actual placement. */
	if(pass > 1)
	{
		/* Any SIZEEXPAND items should be set to uxmax/uymax */
		for(z=0;z<thisbox->count;z++)
		{
			if(thisbox->items[z].hsize == SIZEEXPAND && thisbox->type == DW_VERT)
				thisbox->items[z].width = uxmax-(thisbox->items[z].pad*2);
			if(thisbox->items[z].vsize == SIZEEXPAND && thisbox->type == DW_HORZ)
				thisbox->items[z].height = uymax-(thisbox->items[z].pad*2);
			/* Run this code segment again to finalize the sized after setting uxmax/uymax values. */
			if(thisbox->items[z].type == TYPEBOX)
			{
				Box *tmp = _get_window_pointer(thisbox->items[z].hwnd);

				if(tmp)
				{
					if(*depth > 0)
					{
						float calcval;

						if(thisbox->type == DW_VERT)
						{
							calcval = (float)(tmp->minwidth-((thisbox->items[z].pad*2)+(thisbox->pad*2)));
							if(calcval == 0.0)
								tmp->xratio = thisbox->xratio;
							else
								tmp->xratio = ((float)((thisbox->items[z].width * thisbox->xratio)-((thisbox->items[z].pad*2)+(thisbox->pad*2))))/calcval;
							tmp->width = thisbox->items[z].width;
						}
						if(thisbox->type == DW_HORZ)
						{
							calcval = (float)(tmp->minheight-((thisbox->items[z].pad*2)+(thisbox->pad*2)));
							if(calcval == 0.0)
								tmp->yratio = thisbox->yratio;
							else
								tmp->yratio = ((float)((thisbox->items[z].height * thisbox->yratio)-((thisbox->items[z].pad*2)+(thisbox->pad*2))))/calcval;
							tmp->height = thisbox->items[z].height;
						}
					}

					(*depth)++;

					_resize_box(tmp, depth, x, y, &nux, &nuy, 3, &nupx, &nupy);

					(*depth)--;

				}
			}
		}

		for(z=0;z<(thisbox->count);z++)
		{
			int height = thisbox->items[z].height;
			int width = thisbox->items[z].width;
			int pad = thisbox->items[z].pad;
			HWND handle = thisbox->items[z].hwnd;
			int vectorx, vectory;

			/* When upxmax != pad*2 then ratios are incorrect. */
			vectorx = (int)((width*thisbox->items[z].xratio)-width);
			vectory = (int)((height*thisbox->items[z].yratio)-height);

			if(width > 0 && height > 0)
			{
				/* This is a hack to fix rounding of the sizing */
				if(*depth == 0)
				{
					vectorx++;
					vectory++;
				}

				/* If this item isn't going to expand... reset the vectors to 0 */
				if(thisbox->items[z].vsize != SIZEEXPAND)
					vectory = 0;
				if(thisbox->items[z].hsize != SIZEEXPAND)
					vectorx = 0;

				MoveControl(handle, currentx + pad, currenty + pad);
				SizeControl(handle, width + vectorx, height + vectory);

				if(thisbox->type == DW_HORZ)
					currentx += width + vectorx + (pad * 2);
				if(thisbox->type == DW_VERT)
					currenty += height + vectory + (pad * 2);
			}
		}
	}
	return 0;
}

static void _do_resize(Box *thisbox, int x, int y)
{
	if(x != 0 && y != 0)
	{
		if(thisbox)
		{
			int usedx = 0, usedy = 0, usedpadx = 0, usedpady = 0, depth = 0;

			_resize_box(thisbox, &depth, x, y, &usedx, &usedy, 1, &usedpadx, &usedpady);

			if(usedx-usedpadx == 0 || usedy-usedpady == 0)
				return;

			thisbox->xratio = ((float)(x-usedpadx))/((float)(usedx-usedpadx));
			thisbox->yratio = ((float)(y-usedpady))/((float)(usedy-usedpady));

			usedx = usedy = usedpadx = usedpady = depth = 0;

			_resize_box(thisbox, &depth, x, y, &usedx, &usedy, 2, &usedpadx, &usedpady);
		}
	}
}

static int _dw_int_pos(HWND hwnd)
{
	int pos = (int)dw_window_get_data(hwnd, "_dw_percent_value");
	int range = dw_percent_get_range(hwnd);
	float fpos = (float)pos;
	float frange = (float)range;
	float fnew = (fpos/1000.0)*frange;
	return (int)fnew;
}

static void _dw_int_set(HWND hwnd, int pos)
{
	int inew, range = dw_percent_get_range(hwnd);
	if(range)
	{
		float fpos = (float)pos;
		float frange = (float)range;
		float fnew = (fpos/frange)*1000.0;
		inew = (int)fnew;
		dw_window_set_data(hwnd, "_dw_percent_value", (void *)inew);
	}
}

static void _changebox(Box *thisbox, int percent, int type)
{
	int z;

	for(z=0;z<thisbox->count;z++)
	{
		if(thisbox->items[z].type == TYPEBOX)
		{
			Box *tmp = _get_window_pointer(thisbox->items[z].hwnd);
			_changebox(tmp, percent, type);
		}
		else
		{
			if(type == DW_HORZ)
			{
				if(thisbox->items[z].hsize == SIZEEXPAND)
					thisbox->items[z].width = (int)(((float)thisbox->items[z].origwidth) * (((float)percent)/((float)100.0)));
			}
			else
			{
				if(thisbox->items[z].vsize == SIZEEXPAND)
					thisbox->items[z].height = (int)(((float)thisbox->items[z].origheight) * (((float)percent)/((float)100.0)));
			}
		}
	}
}

/* Main MacOS Message loop, all events are handled here. */
static void _doEvents(EventRecord *eventStrucPtr)
{
	SignalHandler *tmp = Root;

	while(tmp)
	{
		if(tmp->message == eventStrucPtr->what)
		{
            switch(eventStrucPtr->what)
			{
			case mouseDown:
				break;
			case mouseUp:
				break;
			case keyDown:
				break;
			}
		}
		if(tmp)
			tmp = tmp->next;
	}
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
	CreateNewWindow (kDocumentWindowClass, 0,
					 &CreationRect, &CreationWindow);
	CreateRootControl(CreationWindow, &RootControl);
	HideWindow(CreationWindow);
	return 0;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
	EventRecord eventStructure;
	int gDone = false;

	while(!gDone)
	{
		if(WaitNextEvent(everyEvent, &eventStructure, 180, 0))
			_doEvents(&eventStructure);
	}
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
	double start = (double)clock();

	while(((((clock() - start) / CLOCKS_PER_SEC)/1000)) <= milliseconds)
	{
		EventRecord eventStructure;
		if(WaitNextEvent(everyEvent, &eventStructure, 1, 0))
			_doEvents(&eventStructure);
	}
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
	EventRecord eventStructure;

	if(WaitNextEvent(everyEvent, &eventStructure, 0, 0))
		_doEvents(&eventStructure);
}

/*
 * Free's memory allocated by dynamic windows.
 * Parameters:
 *           ptr: Pointer to dynamic windows allocated
 *                memory to be free()'d.
 */
void API dw_free(void *ptr)
{
	free(ptr);
}

/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog * API dw_dialog_new(void *data)
{
	DWDialog *tmp = malloc(sizeof(DWDialog));

	tmp->eve = dw_event_new();
	dw_event_reset(tmp->eve);
	tmp->data = data;
	tmp->done = FALSE;
	tmp->result = NULL;

	return tmp;
}

/*
 * Accepts a dialog struct and returns the given data to the
 * initial called of dw_dialog_wait().
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 *           result: Data to be returned by dw_dialog_wait().
 */
int API dw_dialog_dismiss(DWDialog *dialog, void *result)
{
	dialog->result = result;
	dw_event_post(dialog->eve);
	dialog->done = TRUE;
	return 0;
}

/*
 * Accepts a dialog struct waits for dw_dialog_dismiss() to be
 * called by a signal handler with the given dialog struct.
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 */
void * API dw_dialog_wait(DWDialog *dialog)
{
	void *tmp;
	EventRecord eventStructure;

	while(!dialog->done)
	{
		if(WaitNextEvent(everyEvent, &eventStructure, 180, 0))
			_doEvents(&eventStructure);
	}
	dw_event_close(&dialog->eve);
	tmp = dialog->result;
	free(dialog);
	return tmp;
}


/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int API dw_messagebox(char *title, int flags, char *format, ...)
{
	va_list args;
	char outbuf[1024];
	AlertStdCFStringAlertParamRec param;
	DialogRef dialog;
	CFStringRef cftext, cftitle;
	DialogItemIndex item;
	int ret = DW_MB_RETURN_OK;
	AlertType alert = kAlertPlainAlert;

	va_start(args, format);
	vsprintf(outbuf, format, args);
	va_end(args);
        
	GetStandardAlertDefaultParams(&param, kStdCFStringAlertVersionOne);
	param.movable = TRUE;
	param.helpButton = FALSE;
	if(flags & DW_MB_INFORMATION)
		alert = kAlertNoteAlert;
	else if(flags & DW_MB_ERROR)
		alert = kAlertStopAlert;
	else if(flags & DW_MB_WARNING)
		alert = kAlertCautionAlert;

	if(flags & DW_MB_OK || flags & DW_MB_OKCANCEL)
	{
		param.defaultText = CFSTR("Ok");
		param.cancelText = flags & DW_MB_OK ? 0 : CFSTR("Cancel");
	}
	else
	{
		param.defaultText = CFSTR("Yes");
		param.cancelText = CFSTR("No");
		param.otherText = CFSTR("Cancel");
	}
	cftext = CFStringCreateWithCString(NULL, outbuf, kCFStringEncodingDOSLatinUS);
	cftitle = CFStringCreateWithCString(NULL, title, kCFStringEncodingDOSLatinUS);
	if(CreateStandardAlert(alert, cftext, cftitle, &param, &dialog) == noErr)
	{
		if(RunStandardAlert(dialog, NULL, &item) == noErr)
		{
			if(item == kAlertStdAlertOtherButton)
				ret = DW_MB_RETURN_CANCEL;
			else if(item == kAlertStdAlertCancelButton)
			{
				if(flags & DW_MB_OK || flags & DW_MB_OKCANCEL)
					ret = DW_MB_RETURN_CANCEL;
				else
					ret = DW_MB_RETURN_NO;
			}
			else if(item == kAlertStdAlertOKButton)
			{
				if(flags & DW_MB_YESNO || flags & DW_MB_YESNOCANCEL)
					ret = DW_MB_RETURN_YES;
			}
		}
        }
	CFRelease(cftext);
	CFRelease(cftitle);
	return ret;
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
	BringToFront((WindowRef)handle);
	return 0;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
	return 0;
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
	ShowWindow((WindowRef)handle);
	return 0;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int API dw_window_minimize(HWND handle)
{
	return 0;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_hide(HWND handle)
{
	HideWindow((WindowRef)handle);
	return 0;
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int API dw_window_destroy(HWND handle)
{
	DisposeWindow((WindowRef)handle);
	return 0;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, char *fontname)
{
	return 0;
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in DW_RGB format or a default color index.
 *          back: Background color in DW_RGB format or a default color index.
 */
int API dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
	return 0;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
	return 0;
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
}

/*
 * Tracks this window movement.
 * Parameters:
 *       handle: Handle to frame to be tracked.
 */
void API dw_window_track(HWND handle)
{
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
	SetCursor(*GetCursor(pointertype));
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
HWND API dw_window_new(HWND hwndOwner, char *title, ULONG flStyle)
{
	WindowRef hwnd = 0;
	ControlRef rootcontrol = 0;
        
	CreateNewWindow (kDocumentWindowClass, flStyle,
					 &CreationRect, &hwnd);
	CreateRootControl(hwnd, &rootcontrol);
	dw_window_set_text((HWND)hwnd, title);
	return (HWND)hwnd;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_box_new(int type, int pad)
{
        ControlRef hwnd = NewControl(CreationWindow, NULL, "\p",
                    true, kControlSupportsEmbedding | kControlHasSpecialBackground, 
                    0, 1, kControlUserPaneProc, (SInt32) 0);
        return (HWND)hwnd;
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 */
HWND API dw_groupbox_new(int type, int pad, char *title)
{
	HWND hwnd = 0;
	CreateRadioGroupControl(CreationWindow, &CreationRect, &hwnd);
	return hwnd;
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long id)
{
	return 0;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_bitmap_new(ULONG id)
{
	HWND hwnd = 0;
	CreateImageWellControl(CreationWindow, &CreationRect, NULL, &hwnd);
	return hwnd;
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG id, int top)
{
	HWND hwnd = 0;
	CreateTabsControl(CreationWindow, &CreationRect, kControlTabSizeSmall, kControlTabDirectionNorth, 1, NULL, &hwnd);
	return hwnd;
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI API dw_menu_new(ULONG id)
{
	return 0;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
	return 0;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
}

/*
 * Adds a menuitem or submenu to an existing menu.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND API dw_menu_append_item(HMENUI menux, char *title, ULONG id, ULONG flags, int end, int check, HMENUI submenu)
{
	return 0;
}

/*
 * Sets the state of a menu item check.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 */
void API dw_menu_item_set_check(HMENUI menux, unsigned long id, int check)
{
}

/*
 * Pops up a context menu at given x and y coordinates.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       parent: Handle to the window initiating the popup.
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_menu_popup(HMENUI *menu, HWND parent, int x, int y)
{
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_pointer_set_pos(long x, long y)
{
}


/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_container_new(ULONG id, int multi)
{
	ListHandle hwnd = 0;
	Point CellSize;
	ListDefSpec def;

	SetPt(&CellSize, 52, 52);
	/*def.u.userProc = listDefinitionFunctionUPP;*/

	CreateCustomList(&CreationRect, &CreationRect, CellSize, &def, CreationWindow, TRUE, TRUE, TRUE, TRUE, &hwnd);
	return (HWND)hwnd;
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_tree_new(ULONG id)
{
	return dw_container_new(id, FALSE);
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateStaticTextControl (CreationWindow, &CreationRect, cftext, NULL, &hwnd);
        CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_status_text_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateStaticTextControl (CreationWindow, &CreationRect, cftext, NULL, &hwnd);
	CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG id)
{
	HWND hwnd = 0;
	CreateScrollingTextBoxControl(CreationWindow, &CreationRect, id, FALSE, 0, 0, 0, &hwnd);
	return hwnd;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateEditTextControl(CreationWindow, &CreationRect, cftext, FALSE, FALSE, NULL, &hwnd);
	CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateEditTextControl(CreationWindow, &CreationRect, cftext, TRUE, FALSE, NULL, &hwnd);
	CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_combobox_new(char *text, ULONG id)
{
	return 0;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_button_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreatePushButtonControl(CreationWindow, &CreationRect, cftext, &hwnd);
	CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CreatePushButtonWithIconControl(CreationWindow, &CreationRect, 0, NULL, kControlPushButtonIconOnLeft, &hwnd);
	return hwnd;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 */
HWND dw_bitmapbutton_new_from_file(char *text, unsigned long id, char *filename)
{
	return 0;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_spinbutton_new(char *text, ULONG id)
{
	return 0;
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_radiobutton_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateRadioButtonControl(CreationWindow, &CreationRect, cftext, 0, FALSE, &hwnd);
	CFRelease(cftext);
	return hwnd;
}


/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_slider_new(int vertical, int increments, ULONG id)
{
	HWND hwnd = 0;
	CreateSliderControl(CreationWindow, &CreationRect, 0, 0, increments, kControlSliderDoesNotPoint, 0, FALSE, 0, &hwnd);
	return hwnd;
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_scrollbar_new(int vertical, ULONG id)
{
	HWND hwnd;
	CreateScrollBarControl(CreationWindow, &CreationRect, 0, 0, 100, 100, FALSE, 0, &hwnd);
	return hwnd;
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG id)
{
	HWND hwnd = 0;
	CreateProgressBarControl(CreationWindow, &CreationRect, 0, 0, 100, FALSE, &hwnd);
	return hwnd;
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(char *text, ULONG id)
{
	HWND hwnd = 0;
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	CreateCheckBoxControl(CreationWindow, &CreationRect, cftext, 0, TRUE, &hwnd);
	CFRelease(cftext);
	return hwnd;
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
HWND API dw_listbox_new(ULONG id, int multi)
{
	HWND hwnd = 0;
	CreateListBoxControl(CreationWindow, &CreationRect, TRUE, 0, 1, FALSE, TRUE, 50, 50, TRUE, NULL, &hwnd);
	return hwnd;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void API dw_window_set_icon(HWND handle, ULONG id)
{
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       filename: a path to a file (Bitmap on OS/2 or
 *                 Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 */
void API dw_window_set_bitmap(HWND handle, unsigned long id, char *filename)
{
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, char *text)
{
	CFStringRef cftext = CFStringCreateWithCString(NULL, text, kCFStringEncodingDOSLatinUS);
	if(IsValidWindowRef((WindowRef)handle))
		SetWindowTitleWithCFString((WindowRef)handle, cftext);
	else
		SetControlTitleWithCFString(handle, cftext);
	CFRelease(cftext);
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       text: The text associsated with a given window.
 */
char * API dw_window_get_text(HWND handle)
{
	CFStringRef cftext;
	char *ret = NULL;

	if(IsValidWindowRef((WindowRef)handle))
		CopyWindowTitleAsCFString((WindowRef)handle, &cftext);
	else
	{
		Str255 str;
        
		GetControlTitle(handle, str);
		cftext = CFStringCreateWithPascalString(NULL, str, CFStringGetSystemEncoding());
	}

	if(cftext)
	{
		int length = CFStringGetLength(cftext) + 1;
		char *ret = malloc(length);
		CFStringGetCString(cftext, ret, length, kCFStringEncodingDOSLatinUS);
		CFRelease(cftext);
	}
    return ret;
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
	DisableControl(handle);
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
	EnableControl(handle);
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
	HWND ret = 0;

#if 0
	ControlID cid = (ControlID)id;
	GetControlByID((WindowRef)handle, &cid, &ret);
#endif
	return ret;
}

/*
 * Pack windows (widgets) into a box from the end (or bottom).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to be back.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
}

/*
 * Sets the size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void API dw_window_set_size(HWND handle, ULONG width, ULONG height)
{
        SizeWindow((WindowRef)handle, (short)width, (short)height, TRUE);
}

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
	return 0;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
	return 0;
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
	return 0;
}


/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
void API dw_window_set_pos(HWND handle, ULONG x, ULONG y)
{
	MoveWindow((WindowRef)handle, (short)x, (short)y, FALSE);
}

/*
 * Sets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
void API dw_window_set_pos_size(HWND handle, ULONG x, ULONG y, ULONG width, ULONG height)
{
	dw_window_set_pos(handle, x, y);
	dw_window_set_size(handle, width, height);
}

/*
 * Gets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
void API dw_window_get_pos_size(HWND handle, ULONG *x, ULONG *y, ULONG *width, ULONG *height)
{
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void API dw_window_set_style(HWND handle, ULONG style, ULONG mask)
{
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 */
unsigned long API dw_notebook_page_new(HWND handle, ULONG flags, int front)
{
	return 0;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned int pageid)
{
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
	return 0;
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned int pageid)
{
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_text(HWND handle, ULONG pageid, char *text)
{
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_status_text(HWND handle, ULONG pageid, char *text)
{
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
void API dw_notebook_pack(HWND handle, ULONG pageid, HWND page)
{
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, char *text)
{
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
int API dw_listbox_count(HWND handle)
{
	return 0;
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void API dw_listbox_set_top(HWND handle, int top)
{
}

/*
 * Copies the given index item's text into buffer.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 *          length: Length of the buffer (including NULL).
 */
void API dw_listbox_get_text(HWND handle, unsigned int index, char *buffer, unsigned int length)
{
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
void API dw_listbox_set_text(HWND handle, unsigned int index, char *buffer)
{
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
unsigned int API dw_listbox_selected(HWND handle)
{
    return 0;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
	return -1;
}

/*
 * Sets the selection state of a given index.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 *          state: TRUE if selected FALSE if unselected.
 */
void API dw_listbox_select(HWND handle, int index, int state)
{
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 */
unsigned int API dw_mle_import(HWND handle, char *buffer, int startpoint)
{
    return 0;
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
void API dw_mle_export(HWND handle, char *buffer, int startpoint, int length)
{
}

/*
 * Obtains information about an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          bytes: A pointer to a variable to return the total bytes.
 *          lines: A pointer to a variable to return the number of lines.
 */
void API dw_mle_get_size(HWND handle, unsigned long *bytes, unsigned long *lines)
{
}

/*
 * Deletes text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be deleted from.
 *          startpoint: Point to start deleting text.
 *          length: Amount of text to be deleted.
 */
void API dw_mle_delete(HWND handle, int startpoint, int length)
{
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void API dw_mle_set_cursor(HWND handle, int point)
{
}

/*
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 */
int API dw_mle_search(HWND handle, char *text, int point, unsigned long flags)
{
	return 0;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
}

/*
 * Returns the range of the percent bar.
 * Parameters:
 *          handle: Handle to the percent bar to be queried.
 */
unsigned int API dw_percent_get_range(HWND handle)
{
	return 0;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
	return 0;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
	return 0;
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
}

/*
 * Sets the scrollbar range.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          range: Maximum range value.
 *          visible: Visible area relative to the range.
 */
void API dw_scrollbar_set_range(HWND handle, unsigned int range, unsigned int visible)
{
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          upper: Upper limit.
 *          lower: Lower limit.
 */
void API dw_spinbutton_set_limits(HWND handle, long upper, long lower)
{
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
}


/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
    return 0;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int API dw_checkbox_get(HWND handle)
{
	return 0;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
}

/*
 * Inserts an item into a tree window (widget) after another item.
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          item: Handle to the item to be positioned after.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 */
HTREEITEM API dw_tree_insert_after(HWND handle, HTREEITEM item, char *title, unsigned long icon, HTREEITEM parent, void *itemdata)
{
	return 0;
}

/*
 * Inserts an item into a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 */
HTREEITEM API dw_tree_insert(HWND handle, char *title, unsigned long icon, HTREEITEM parent, void *itemdata)
{
	return 0;
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void API dw_tree_item_change(HWND handle, HTREEITEM item, char *title, unsigned long icon)
{
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
void API dw_tree_item_set_data(HWND handle, HTREEITEM item, void *itemdata)
{
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
	return NULL;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
}

/*
 * Sets up the container columns.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 *          separator: The column number that contains the main separator.
 *                     (this item may only be used in OS/2)
 */
int API dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator)
{
	return TRUE;
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 */
int API dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count)
{
	char **newtitles = malloc(sizeof(char *) * (count + 2));
	unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 2));

	newtitles[0] = "Icon";
	newtitles[1] = "Filename";

	newflags[0] = DW_CFA_BITMAPORICON | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR;
	newflags[1] = DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

	memcpy(&newtitles[2], titles, sizeof(char *) * count);
	memcpy(&newflags[2], flags, sizeof(unsigned long) * count);

	dw_container_setup(handle, newflags, newtitles, count + 2, count ? 2 : 0);

	free(newtitles);
	free(newflags);
	return TRUE;
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 */
unsigned long API dw_icon_load(unsigned long module, unsigned long id)
{
	return 0;
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
unsigned long API dw_icon_load_from_file(char *filename)
{
	return 0;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(unsigned long handle)
{
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
	return NULL;
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_container_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_container_change_item(HWND handle, int column, int row, void *data)
{
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_set_file(HWND handle, void *pointer, int row, char *filename, unsigned long icon)
{
	dw_container_set_item(handle, pointer, 0, row, (void *)&icon);
	dw_container_set_item(handle, pointer, 1, row, (void *)&filename);
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
	dw_container_set_item(handle, pointer, column + 2, row, data);
}

/*
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
	return 0;
}

/*
 * Gets column type for a filesystem container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_filesystem_get_column_type(HWND handle, int column)
{
	return dw_container_get_column_type( handle, column + 1 );
}

/*
 * Sets the width of a column in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          column: Zero based column of width being set.
 *          width: Width of column in pixels.
 */
void API dw_container_set_column_width(HWND handle, int column, int width)
{
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_set_row_title(void *pointer, int row, char *title)
{
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          rowcount: The number of rows to be inserted.
 */
void API dw_container_insert(HWND handle, void *pointer, int rowcount)
{
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
}

/*
 * Scrolls container up or down.
 * Parameters:
 *       handle: Handle to the window (widget) to be scrolled.
 *       direction: DW_SCROLL_UP, DW_SCROLL_DOWN, DW_SCROLL_TOP or
 *                  DW_SCROLL_BOTTOM. (rows is ignored for last two)
 *       rows: The number of rows to be scrolled.
 */
void API dw_container_scroll(HWND handle, int direction, long rows)
{
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
char * API dw_container_query_start(HWND handle, unsigned long flags)
{
    return NULL;
}

/*
 * Continues an existing query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
char * API dw_container_query_next(HWND handle, unsigned long flags)
{
    return NULL;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor(HWND handle, char *text)
{
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, char *text)
{
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void dw_taskbar_insert(HWND handle, unsigned long icon, char *bubbletext)
{
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void dw_taskbar_delete(HWND handle, unsigned long icon)
{
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
HWND API dw_render_new(unsigned long id)
{
	return 0;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
}

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_draw_point(HWND handle, HPIXMAP pixmap, int x, int y)
{
}

/* Draw a line on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x1: First X coordinate.
 *       y1: First Y coordinate.
 *       x2: Second X coordinate.
 *       y2: Second Y coordinate.
 */
void API dw_draw_line(HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2)
{
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 *       text: Text to be displayed.
 */
void API dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, char *text)
{
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height Pointer to a variable to be filled in with the height.
 */
void API dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, char *text, int *width, int *height)
{
}

/* Draw a rectangle on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       fill: Fill box TRUE or FALSE.
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void API dw_draw_rect(HWND handle, HPIXMAP pixmap, int fill, int x, int y, int width, int height)
{
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void API dw_flush(void)
{
}

/*
 * Creates a pixmap with given parameters.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       width: Width of the pixmap in pixels.
 *       height: Height of the pixmap in pixels.
 *       depth: Color depth of the pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new(HWND handle, unsigned long width, unsigned long height, int depth)
{
	return 0;
}

/*
 * Creates a pixmap from a file.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new_from_file(HWND handle, char *filename)
{
	return 0;
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_grab(HWND handle, ULONG id)
{
	return 0;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
void API dw_pixmap_destroy(HPIXMAP pixmap)
{
}

/*
 * Copies from one item to another.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of area to copy.
 *       height: Height of area to copy.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 */
void API dw_pixmap_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc)
{
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
}

/* Open a shared library and return a handle.
 * Parameters:
 *         name: Base name of the shared library.
 *         handle: Pointer to a module handle,
 *                 will be filled in with the handle.
 */
int API dw_module_load(char *name, HMOD *handle)
{
	return 0;
}

/* Queries the address of a symbol within open handle.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 *         name: Name of the symbol you want the address of.
 *         func: A pointer to a function pointer, to obtain
 *               the address.
 */
int API dw_module_symbol(HMOD handle, char *name, void**func)
{
	return 0;
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int API dw_module_close(HMOD handle)
{
	return 0;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
{
	return 0;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * If we are in a callback we must keep the message loop running
 * while blocking.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_lock(HMTX mutex)
{
}

/*
 * Reliquishes the access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_unlock(HMTX mutex)
{
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
{
	return 0;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_reset(HEV eve)
{
	return TRUE;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_post(HEV eve)
{
	return TRUE;
}


/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_wait(HEV eve, unsigned long timeout)
{
	return 0;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_close(HEV *eve)
{
	return TRUE;
}

/*
 * Creates a new thread with a starting point of func.
 * Parameters:
 *       func: Function which will be run in the new thread.
 *       data: Parameter(s) passed to the function.
 *       stack: Stack size of new thread (OS/2 and Windows only).
 */
DWTID API dw_thread_new(void *func, void *data, int stack)
{
	return (DWTID)-1;
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
	return 0;
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
	exit(exitcode);
}

/*
 * Creates a splitbar window (widget) with given parameters.
 * Parameters:
 *       type: Value can be DW_VERT or DW_HORZ.
 *       topleft: Handle to the window to be top or left.
 *       bottomright:  Handle to the window to be bottom or right.
 * Returns:
 *       A handle to a splitbar window or NULL on failure.
 */
HWND API dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long id)
{
	return 0;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void API dw_splitbar_set(HWND handle, float percent)
{
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
	return 0.0;
}

/*
 * Pack windows (widgets) into a box from the start (or top).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to be back.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 */
void API dw_window_default(HWND window, HWND defaultitem)
{
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND window, HWND next)
{
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
	ULONG Build;
	char verbuf[10];

	if(!env)
		return;

	Gestalt(gestaltSystemVersion, &Build);

	sprintf(verbuf, "%04x", (int)Build);

	strcpy(env->osName,"MacOS");
	env->MajorBuild = atoi(&verbuf[3]);
	verbuf[3] = 0;
	env->MinorVersion = atoi(&verbuf[2]);
	verbuf[2] = 0;
	env->MajorVersion = atoi(verbuf);

	env->MinorBuild = 0;

	strcpy(env->buildDate, __DATE__);
	strcpy(env->buildTime, __TIME__);
	env->DWMajorVersion = DW_MAJOR_VERSION;
	env->DWMinorVersion = DW_MINOR_VERSION;
	env->DWSubVersion = DW_SUB_VERSION;
}

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extention.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE.
 * Returns:
 *       NULL on error. A malloced buffer containing
 *       the file path on success.
 *       
 */
char * API dw_file_browse(char *title, char *defpath, char *ext, int flags)
{
	return NULL;
}

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       -1 on error.
 */
int API dw_exec(char *program, int type, char **params)
{
	return -1;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(char *url)
{
	return -1;
}

/*
 * Returns a pointer to a static buffer which containes the
 * current user directory.  Or the root directory (C:\ on
 * OS/2 and Windows).
 */
char * API dw_user_dir(void)
{
	static char _user_dir[1024] = "";

	if(!_user_dir[0])
	{
		char *home = getenv("HOME");

		if(home)
			strcpy(_user_dir, home);
		else
			strcpy(_user_dir, "/");
	}
	return _user_dir;
}

/*
 * Call a function from the window (widget)'s context.
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
void API dw_window_function(HWND handle, void *function, void *data)
{
}

/* Functions for managing the user data lists that are associated with
 * a given window handle.  Used in dw_window_set_data() and
 * dw_window_get_data().
 */
static UserData *_find_userdata(UserData **root, char *varname)
{
	UserData *tmp = *root;

	while(tmp)
	{
		if(strcasecmp(tmp->varname, varname) == 0)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

static int _new_userdata(UserData **root, char *varname, void *data)
{
	UserData *new = _find_userdata(root, varname);

	if(new)
	{
		new->data = data;
		return TRUE;
	}
	else
	{
		new = malloc(sizeof(UserData));
		if(new)
		{
			new->varname = strdup(varname);
			new->data = data;

			new->next = NULL;

			if (!*root)
				*root = new;
			else
			{
				UserData *prev = NULL, *tmp = *root;
				while(tmp)
				{
					prev = tmp;
					tmp = tmp->next;
				}
				if(prev)
					prev->next = new;
				else
					*root = new;
			}
			return TRUE;
		}
	}
	return FALSE;
}

static int _remove_userdata(UserData **root, char *varname, int all)
{
	UserData *prev = NULL, *tmp = *root;

	while(tmp)
	{
		if(all || strcasecmp(tmp->varname, varname) == 0)
		{
			if(!prev)
			{
				*root = tmp->next;
				free(tmp->varname);
				free(tmp);
				if(!all)
					return 0;
				tmp = *root;
			}
			else
			{
				/* If all is true we should
				 * never get here.
				 */
				prev->next = tmp->next;
				free(tmp->varname);
				free(tmp);
				return 0;
			}
		}
		else
		{
			prev = tmp;
			tmp = tmp->next;
		}
	}
	return 0;
}

/*
 * Add a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void API dw_window_set_data(HWND window, char *dataname, void *data)
{
	WindowData *blah = (WindowData *)_get_window_pointer(window);

	if(!blah)
	{
		if(!dataname)
			return;

		blah = calloc(1, sizeof(WindowData));
		_set_window_pointer(window, blah);
	}

	if(data)
		_new_userdata(&(blah->root), dataname, data);
	else
	{
		if(dataname)
			_remove_userdata(&(blah->root), dataname, FALSE);
		else
			_remove_userdata(&(blah->root), NULL, TRUE);
	}
}

/*
 * Gets a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void *dw_window_get_data(HWND window, char *dataname)
{
	WindowData *blah = (WindowData *)_get_window_pointer(window);

	if(blah && blah->root && dataname)
	{
		UserData *ud = _find_userdata(&(blah->root), dataname);
		if(ud)
			return ud->data;
	}
	return NULL;
}

/*
 * Add a callback to a timer event.
 * Parameters:
 *       interval: Milliseconds to delay between calls.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 * Returns:
 *       Timer ID for use with dw_timer_disconnect(), 0 on error.
 */
int API dw_timer_connect(int interval, void *sigfunc, void *data)
{
	return 0;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(int id)
{
	SignalHandler *prev = NULL, *tmp = Root;

	/* 0 is an invalid timer ID */
	if(!id)
		return;

	while(tmp)
	{
		if(tmp->id == id)
		{
			if(prev)
			{
				prev->next = tmp->next;
				free(tmp);
				tmp = prev->next;
			}
			else
			{
				Root = tmp->next;
				free(tmp);
				tmp = Root;
			}
		}
		else
		{
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

/*
 * Add a callback to a window event.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect(HWND window, char *signame, void *sigfunc, void *data)
{
	ULONG message = 0L;

	if(window && signame && sigfunc)
	{
		if((message = _findsigmessage(signame)) != 0)
			_new_signal(message, window, 0, sigfunc, data);
	}
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_name(HWND window, char *signame)
{
	SignalHandler *prev = NULL, *tmp = Root;
	ULONG message;

	if(!window || !signame || (message = _findsigmessage(signame)) == 0)
		return;

	while(tmp)
	{
		if(tmp->window == window && tmp->message == message)
		{
			if(prev)
			{
				prev->next = tmp->next;
				free(tmp);
				tmp = prev->next;
			}
			else
			{
				Root = tmp->next;
				free(tmp);
				tmp = Root;
			}
		}
		else
		{
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_window(HWND window)
{
	SignalHandler *prev = NULL, *tmp = Root;

	while(tmp)
	{
		if(tmp->window == window)
		{
			if(prev)
			{
				prev->next = tmp->next;
				free(tmp);
				tmp = prev->next;
			}
			else
			{
				Root = tmp->next;
				free(tmp);
				tmp = Root;
			}
		}
		else
		{
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void API dw_signal_disconnect_by_data(HWND window, void *data)
{
	SignalHandler *prev = NULL, *tmp = Root;

	while(tmp)
	{
		if(tmp->window == window && tmp->data == data)
		{
			if(prev)
			{
				prev->next = tmp->next;
				free(tmp);
				tmp = prev->next;
			}
			else
			{
				Root = tmp->next;
				free(tmp);
				tmp = Root;
			}
		}
		else
		{
			prev = tmp;
			tmp = tmp->next;
		}
	}
}


