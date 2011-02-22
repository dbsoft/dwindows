/*
 * Dynamic Windows:
 *          A GTK like implementation of the MacOS GUI using Cocoa
 *
 * (C) 2011 Brian Smith <brian@dbsoft.org>
 *
 * Using garbage collection so requires 10.5 or later.
 * clang -std=c99 -g -o dwtest -D__MAC__ -I. mac/dw.m -framework Cocoa -fobjc-gc-only
 */
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h> 
#include "dw.h"
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>

static void _do_resize(Box *thisbox, int x, int y);

/* So basically to implement our event handlers...
 * it looks like we are going to have to subclass
 * basically everything.  Was hoping to add methods
 * to the superclasses but it looks like you can 
 * only add methods and no variables, which isn't 
 * going to work. -Brian
 */

/* Subclass for a box type */
@interface DWBox : NSView 
{
	Box box;
	void *userdata;
	NSColor *bgcolor;
}
-(Box)box;
-(void *)userdata;
-(void)setBox:(Box)input;
-(void)setUserdata:(void *)input;
-(void)drawRect:(NSRect)rect;
@end

@implementation DWBox
-(Box)box { return box; }
-(void *)userdata { return userdata; }
-(void)setBox:(Box)input { box = input; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)drawRect:(NSRect)rect 
{
	if(bgcolor)
	{
		[bgcolor set];
		NSRectFill( [self bounds] );
	}
}
@end

/* Subclass for a top-level window */
@interface DWView : DWBox  { }
@end

@implementation DWView
-(void)windowWillClose:(NSNotification *)note 
{
    [[NSApplication sharedApplication] terminate:self];
}
- (void)viewDidMoveToWindow
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:[self window]];
}
- (void)dealloc
{
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [super dealloc];
}
- (void)windowResized:(NSNotification *)notification;
{
        NSSize size = [self frame].size;
		_do_resize(&box, size.width, size.height);
}
@end

/* Subclass for a button type */
@interface DWButton : NSButton
{
	void *userdata; 
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWButton
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
@end

/* Subclass for a progress type */
@interface DWPercent : NSProgressIndicator
{
	void *userdata; 
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWPercent
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
@end

/* Subclass for a entryfield type */
@interface DWEntryField : NSTextField
{
	void *userdata; 
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWEntryField
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
@end

/* Subclass for a entryfield password type */
@interface DWEntryFieldPassword : NSSecureTextField
{
	void *userdata; 
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWEntryFieldPassword
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
@end

NSApplication *DWApp;
NSRunLoop *DWRunLoop;

typedef struct _sighandler
{
	struct _sighandler   *next;
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

/* List of signals */
#define SIGNALMAX 16

SignalList SignalTranslate[SIGNALMAX] = {
	{ 1,	DW_SIGNAL_CONFIGURE },
	{ 2,	DW_SIGNAL_KEY_PRESS },
	{ 3,	DW_SIGNAL_BUTTON_PRESS },
	{ 4,	DW_SIGNAL_BUTTON_RELEASE },
	{ 5,	DW_SIGNAL_MOTION_NOTIFY },
	{ 6,	DW_SIGNAL_DELETE },
	{ 7,	DW_SIGNAL_EXPOSE },
	{ 8,	DW_SIGNAL_CLICKED },
	{ 9,	DW_SIGNAL_ITEM_ENTER },
	{ 10,	DW_SIGNAL_ITEM_CONTEXT },
	{ 11,	DW_SIGNAL_LIST_SELECT },
	{ 12,	DW_SIGNAL_ITEM_SELECT },
	{ 13,	DW_SIGNAL_SET_FOCUS },
	{ 14,	DW_SIGNAL_VALUE_CHANGED },
	{ 15,	DW_SIGNAL_SWITCH_PAGE },
	{ 16,	DW_SIGNAL_TREE_EXPAND }
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
			   tmp->id == id &&
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
ULONG _findsigmessage(char *signame)
{
	int z;
	
	for(z=0;z<SIGNALMAX;z++)
	{
		if(strcasecmp(signame, SignalTranslate[z].name) == 0)
			return SignalTranslate[z].message;
	}
	return 0L;
}

unsigned long _foreground = 0xAAAAAA, _background = 0;

/* This function will recursively search a box and add up the total height of it */
static void _count_size(HWND thisbox, int type, int *xsize, int *xorigsize)
{
   int size = 0, origsize = 0, z;
   DWBox *box = thisbox;
   Box _tmp = [box box];
   Box *tmp = &_tmp;

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
		DWBox *box = thisbox->items[z].hwnd;
		Box _tmp = [box box];
		Box *tmp = &_tmp;

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
			[box setBox:_tmp];

            (*depth)++;

            _resize_box(tmp, depth, x, y, &nux, &nuy, pass, &upx, &upy);

            (*depth)--;

            newx = x - nux;
            newy = y - nuy;

            tmp->minwidth = thisbox->items[z].width = initialx - newx;
            tmp->minheight = thisbox->items[z].height = initialy - newy;
			[box setBox:_tmp];
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
			DWBox *box = thisbox->items[z].hwnd;
			Box _tmp = [box box];
			Box *tmp = &_tmp;

            if(tmp)
            {
               tmp->parentxratio = thisbox->items[z].xratio;
               tmp->parentyratio = thisbox->items[z].yratio;
            }
			[box setBox:_tmp];
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
			DWBox *box = thisbox->items[z].hwnd;
			Box _tmp = [box box];
			Box *tmp = &_tmp;

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
			   [box setBox:_tmp];

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
         NSView *handle = thisbox->items[z].hwnd;
		 NSPoint point;
		 NSSize size;
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

			point.x = currentx + pad;
			point.y = currenty + pad;
			size.width = width + vectorx;
			size.height = height + vectory;
			[handle setFrameOrigin:point];
			[handle setFrameSize:size];

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

static void _changebox(Box *thisbox, int percent, int type)
{
   int z;

   for(z=0;z<thisbox->count;z++)
   {
      if(thisbox->items[z].type == TYPEBOX)
      {
		 DWBox *box = thisbox->items[z].hwnd;
		 Box _tmp = [box box];
		 Box *tmp = &_tmp;
         _changebox(tmp, percent, type);
		 [box setBox:_tmp];
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

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
	/* Create the application object */
	DWApp = [NSApplication sharedApplication];
	/* Create a run loop for doing manual loops */
	DWRunLoop = [NSRunLoop alloc];
	
	/* This only works on 10.6 so we have a backup method */
	NSString * applicationName = [[NSRunningApplication currentApplication] localizedName];
	if(applicationName == nil)
	{
		applicationName = [[NSProcessInfo processInfo] processName];
	}
	
	/* Create the main menu */
	NSMenu * mainMenu = [[[NSMenu alloc] initWithTitle:@"MainMenu"] autorelease];
	
	NSMenuItem * mitem = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
	NSMenu * menu = [[[NSMenu alloc] initWithTitle:@"Apple"] autorelease];

	[DWApp setMainMenu:mainMenu];

	[DWApp performSelector:@selector(setAppleMenu:) withObject:menu];
	
	/* Setup the Application menu */
	NSMenuItem * item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
						   action:@selector(orderFrontStandardAboutPanel:)
					keyEquivalent:@""];
	[item setTarget:DWApp];
	
	[menu addItem:[NSMenuItem separatorItem]];
	
	item = [menu addItemWithTitle:NSLocalizedString(@"Preferences...", nil)
						   action:NULL
					keyEquivalent:@","];
	
	[menu addItem:[NSMenuItem separatorItem]];
	
	item = [menu addItemWithTitle:NSLocalizedString(@"Services", nil)
						   action:NULL
					keyEquivalent:@""];
	NSMenu * servicesMenu = [[[NSMenu alloc] initWithTitle:@"Services"] autorelease];
	[menu setSubmenu:servicesMenu forItem:item];
	[DWApp setServicesMenu:servicesMenu];
	
	[menu addItem:[NSMenuItem separatorItem]];
	
	item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Hide", nil), applicationName]
						   action:@selector(hide:)
					keyEquivalent:@"h"];
	[item setTarget:DWApp];
	
	item = [menu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
						   action:@selector(hideOtherApplications:)
					keyEquivalent:@"h"];
	[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
	[item setTarget:DWApp];
	
	item = [menu addItemWithTitle:NSLocalizedString(@"Show All", nil)
						   action:@selector(unhideAllApplications:)
					keyEquivalent:@""];
	[item setTarget:DWApp];
	
	[menu addItem:[NSMenuItem separatorItem]];
	
	item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Quit", nil), applicationName]
						   action:@selector(terminate:)
					keyEquivalent:@"q"];
	[item setTarget:DWApp];
	
	[mainMenu setSubmenu:menu forItem:mitem];
	return 0;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
    [DWApp run];
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
	double seconds = (double)milliseconds/1000.0;
	NSDate *time = [[NSDate alloc] initWithTimeIntervalSinceNow:seconds];
	[DWRunLoop runUntilDate:time];
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
	[DWRunLoop limitDateForMode:NSDefaultRunLoopMode];
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
 * Returns a pointer to a static buffer which containes the
 * current user directory.  Or the root directory (C:\ on
 * OS/2 and Windows).
 */
char *dw_user_dir(void)
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
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: flags to indicate buttons and icon
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int API dw_messagebox(char *title, int flags, char *format, ...)
{
	int iResponse;
	NSString *button1 = @"OK";
	NSString *button2 = nil;
	NSString *button3 = nil;
	va_list args;
	char outbuf[1000];

	va_start(args, format);
	vsprintf(outbuf, format, args);
	va_end(args);
	
	if(flags & DW_MB_OKCANCEL)
	{
		button2 = @"Cancel";
	}
	else if(flags & DW_MB_YESNO)
	{
		button1 = @"Yes";
		button2 = @"No";
	}
	else if(flags & DW_MB_YESNOCANCEL)
	{
		button1 = @"Yes";
		button2 = @"No";
		button3 = @"Cancel";
	}
	
	if(flags & DW_MB_ERROR)
	{
		iResponse =
		NSRunCriticalAlertPanel([ NSString stringWithUTF8String:title ], 
								[ NSString stringWithUTF8String:outbuf ],
								button1, button2, button3);	}
	else 
	{
		iResponse =
		NSRunAlertPanel([ NSString stringWithUTF8String:title ], 
						[ NSString stringWithUTF8String:outbuf ],
						button1, button2, button3);
	}
	
	switch(iResponse) 
	{
		case NSAlertDefaultReturn:    /* user pressed OK */
			if(flags & DW_MB_YESNO || flags & DW_MB_YESNOCANCEL)
			{
				return DW_MB_RETURN_YES;
			}
			return DW_MB_RETURN_OK;
		case NSAlertAlternateReturn:  /* user pressed Cancel */
			if(flags & DW_MB_OKCANCEL)
			{
				return DW_MB_RETURN_CANCEL;
			}
			return DW_MB_RETURN_NO;
		case NSAlertOtherReturn:      /* user pressed the third button */
			return DW_MB_RETURN_CANCEL;
		case NSAlertErrorReturn:      /* an error occurred */
			break;
	}
	return 0;
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
	if(flags == DW_FILE_OPEN)
	{
		/* Create the File Open Dialog class. */
		NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	
		/* Enable the selection of files in the dialog. */
		[openDlg setCanChooseFiles:YES];
		[openDlg setCanChooseDirectories:NO];
	
		/* Disable multiple selection */
		[openDlg setAllowsMultipleSelection:NO];
	
		/* Display the dialog.  If the OK button was pressed,
		 * process the files.
		 */
		if([openDlg runModal] == NSOKButton)
		{
			/* Get an array containing the full filenames of all
			 * files and directories selected.
			 */
			NSArray* files = [openDlg filenames];
			NSString* fileName = [files objectAtIndex:0];
			return strdup([ fileName UTF8String ]);		
		}
	}
	else
	{
		/* Create the File Save Dialog class. */
		NSSavePanel* saveDlg = [NSSavePanel savePanel];
		
		/* Enable the selection of files in the dialog. */
		[saveDlg setCanCreateDirectories:YES];
		
		/* Display the dialog.  If the OK button was pressed,
		 * process the files.
		 */
		if([saveDlg runModal] == NSFileHandlingPanelOKButton)
		{
			/* Get an array containing the full filenames of all
			 * files and directories selected.
			 */
			NSString* fileName = [saveDlg filename];
			return strdup([ fileName UTF8String ]);		
		}		
	}

	return NULL;
}

/*
 * Gets the contents of the default clipboard as text.
 * Parameters:
 *       None.
 * Returns:
 *       Pointer to an allocated string of text or NULL if clipboard empty or contents could not
 *       be converted to text.
 */
char *dw_clipboard_get_text()
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *str = [pasteboard stringForType:NSStringPboardType];
	if(str != nil)
	{
		return strdup([ str UTF8String ]);
	}
	return NULL;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void dw_clipboard_set_text( char *str, int len)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	
	[pasteboard clearContents];
	
	[pasteboard setString:[ NSString stringWithUTF8String:str ] forType:NSStringPboardType];
}


/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog * API dw_dialog_new(void *data)
{
	NSLog(@"dw_dialog_new() unimplemented\n");
	return NULL;
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
	NSLog(@"dw_dialog_dismiss() unimplemented\n");
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
	NSLog(@"dw_dialog_wait() unimplemented\n");
	return NULL;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_box_new(int type, int pad)
{
    DWBox *view = [[DWBox alloc] init];  
	Box newbox;
	memset(&newbox, 0, sizeof(Box));
   	newbox.pad = pad;
   	newbox.type = type;
	[view setBox:newbox];
	return view;
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
	DWBox *box = dw_box_new(type, pad);
	[box setFocusRingType:NSFocusRingTypeExterior];
	return box;
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
	NSObject *object = box;
	DWBox *view = box;
	DWBox *this = item;
	Box thisbox;
    int z;
    Item *tmpitem, *thisitem;

	/* Query the objects */
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = box;
		view = [window contentView];
	}
	
	thisbox = [view box];
	thisitem = thisbox.items;
	object = item;

	/* Duplicate the existing data */
    tmpitem = malloc(sizeof(Item)*(thisbox.count+1));

    for(z=0;z<thisbox.count;z++)
    {
       tmpitem[z+1] = thisitem[z];
    }

	/* Sanity checks */
    if(vsize && !height)
       height = 1;
    if(hsize && !width)
       width = 1;

	/* Fill in the item data appropriately */
	if([ object isKindOfClass:[ DWBox class ] ])
       tmpitem[0].type = TYPEBOX;
    else
       tmpitem[0].type = TYPEITEM;

    tmpitem[0].hwnd = item;
    tmpitem[0].origwidth = tmpitem[0].width = width;
    tmpitem[0].origheight = tmpitem[0].height = height;
    tmpitem[0].pad = pad;
    if(hsize)
       tmpitem[0].hsize = SIZEEXPAND;
    else
       tmpitem[0].hsize = SIZESTATIC;

    if(vsize)
       tmpitem[0].vsize = SIZEEXPAND;
    else
       tmpitem[0].vsize = SIZESTATIC;

    thisbox.items = tmpitem;

	/* Update the item count */
    thisbox.count++;

	/* Add the item to the box */
	[view setBox:thisbox];
	[view addSubview:this];
	
	/* Free the old data */
    if(thisbox.count)
       free(thisitem);
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
	NSObject *object = box;
	DWBox *view = box;
	DWBox *this = item;
	Box thisbox;
    int z;
    Item *tmpitem, *thisitem;

	/* Query the objects */
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = box;
		view = [window contentView];
	}
	
	thisbox = [view box];
	thisitem = thisbox.items;
	object = item;

	/* Duplicate the existing data */
    tmpitem = malloc(sizeof(Item)*(thisbox.count+1));

    for(z=0;z<thisbox.count;z++)
    {
       tmpitem[z] = thisitem[z];
    }

	/* Sanity checks */
    if(vsize && !height)
       height = 1;
    if(hsize && !width)
       width = 1;

	/* Fill in the item data appropriately */
	if([ object isKindOfClass:[ DWBox class ] ])
       tmpitem[thisbox.count].type = TYPEBOX;
    else
       tmpitem[thisbox.count].type = TYPEITEM;

    tmpitem[thisbox.count].hwnd = item;
    tmpitem[thisbox.count].origwidth = tmpitem[thisbox.count].width = width;
    tmpitem[thisbox.count].origheight = tmpitem[thisbox.count].height = height;
    tmpitem[thisbox.count].pad = pad;
    if(hsize)
       tmpitem[thisbox.count].hsize = SIZEEXPAND;
    else
       tmpitem[thisbox.count].hsize = SIZESTATIC;

    if(vsize)
       tmpitem[thisbox.count].vsize = SIZEEXPAND;
    else
       tmpitem[thisbox.count].vsize = SIZESTATIC;

	thisbox.items = tmpitem;

	/* Update the item count */
    thisbox.count++;

	/* Add the item to the box */
	[view setBox:thisbox];
	[view addSubview:this];
	
	/* Free the old data */
    if(thisbox.count)
       free(thisitem);
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_button_new(char *text, ULONG id)
{
	DWButton *button = [[DWButton alloc] init];
	[button setTitle:[ NSString stringWithUTF8String:text ]];
	[button setButtonType:NSMomentaryPushInButton];
	[button setBezelStyle:NSThickerSquareBezelStyle];
	/*[button setGradientType:NSGradientConvexWeak];*/
	[button setTag:id];
	return button;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_new(char *text, ULONG id)
{
	DWEntryField *entry = [[DWEntryField alloc] init];
	[entry setStringValue:[ NSString stringWithUTF8String:text ]];
	[entry setTag:id];
	return entry;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(char *text, ULONG id)
{
	DWEntryFieldPassword *entry = [[DWEntryFieldPassword alloc] init];
	[entry setStringValue:[ NSString stringWithUTF8String:text ]];
	[entry setTag:id];
	return entry;
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
	NSLog(@"dw_entryfield_set_limit() unimplemented\n");
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(char *text, ULONG id)
{
	NSLog(@"dw_bitmapbutton_new() unimplemented\n");
	return HWND_DESKTOP;
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
HWND API dw_bitmapbutton_new_from_file(char *text, unsigned long id, char *filename)
{
	NSLog(@"dw_bitmapbutton_new_from_file() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: The contents of the image
 *            (BMP or ICO on OS/2 or Windows, XPM on Unix)
 *       len: length of str
 */
HWND API dw_bitmapbutton_new_from_data(char *text, unsigned long id, char *data, int len)
{
	NSLog(@"dw_bitmapbutton_new_from_data() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_spinbutton_new(char *text, ULONG id)
{
	NSLog(@"dw_spinbutton_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
	NSLog(@"dw_spinbutton_set_pos() unimplemented\n");
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
	NSLog(@"dw_spinbutton_set_limits() unimplemented\n");
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
	NSLog(@"dw_spinbutton_get_pos() unimplemented\n");
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
	DWButton *button = dw_button_new(text, id);
	[button setButtonType:NSRadioButton];
	return button;
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
	NSLog(@"dw_slider_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
	NSLog(@"dw_slider_get_pos() unimplemented\n");
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
	NSLog(@"dw_slider_set_pos() unimplemented\n");
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
	NSLog(@"dw_scrollbar_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
	NSLog(@"dw_scrollbar_get_pos() unimplemented\n");
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
	NSLog(@"dw_scrollbar_set_pos() unimplemented\n");
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
	NSLog(@"dw_scrollbar_set_range() unimplemented\n");
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG id)
{
	DWPercent *percent = [[DWPercent alloc] init];
	[percent setBezeled:YES];
	[percent setMaxValue:100];
	[percent setMinValue:0];
	/*[percent setTag:id]; Why doesn't this work? */
	return percent;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
	DWPercent *percent = handle;
	[percent setDoubleValue:(double)position];
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(char *text, ULONG id)
{
	DWButton *button = dw_button_new(text, id);
	[button setButtonType:NSSwitchButton];
	[button setBezelStyle:NSRegularSquareBezelStyle];
	return button;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int API dw_checkbox_get(HWND handle)
{
	NSLog(@"dw_checkbox_set() unimplemented\n");
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
	NSLog(@"dw_checkbox_set() unimplemented\n");
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
HWND API dw_listbox_new(ULONG id, int multi)
{
	NSLog(@"dw_listbox_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, char *text)
{
	NSLog(@"dw_listbox_append() unimplemented\n");
}

/*
 * Inserts the specified text into the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be inserted into.
 *          text: Text to insert into listbox.
 *          pos: 0-based position to insert text
 */
void API dw_listbox_insert(HWND handle, char *text, int pos)
{
	NSLog(@"dw_listbox_insert() unimplemented\n");
}

/*
 * Appends the specified text items to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text strings to append into listbox.
 *          count: Number of text strings to append
 */
void API dw_listbox_list_append(HWND handle, char **text, int count)
{
	NSLog(@"dw_listbox_list_append() unimplemented\n");
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
	NSLog(@"dw_listbox_clear() unimplemented\n");
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
int API dw_listbox_count(HWND handle)
{
	NSLog(@"dw_listbox_count() unimplemented\n");
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
	NSLog(@"dw_listbox_set_top() unimplemented\n");
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
	NSLog(@"dw_listbox_get_text() unimplemented\n");
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
	NSLog(@"dw_listbox_set_text() unimplemented\n");
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
unsigned int API dw_listbox_selected(HWND handle)
{
	NSLog(@"dw_listbox_selected() unimplemented\n");
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
	NSLog(@"dw_listbox_selected_multi() unimplemented\n");
	return 0;
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
	NSLog(@"dw_listbox_select() unimplemented\n");
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
	NSLog(@"dw_listbox_delete() unimplemented\n");
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_combobox_new(char *text, ULONG id)
{
	NSLog(@"dw_combobox_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG id)
{
	NSLog(@"dw_mle_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_mle_import() unimplemented\n");
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
	NSLog(@"dw_mle_export() unimplemented\n");
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
	NSLog(@"dw_mle_get_size() unimplemented\n");
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
	NSLog(@"dw_mle_delete() unimplemented\n");
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
	NSLog(@"dw_mle_clear() unimplemented\n");
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
	NSLog(@"dw_mle_clear() unimplemented\n");
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
	NSLog(@"dw_mle_set_editable() unimplemented\n");
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
	NSLog(@"dw_mle_set_word_wrap() unimplemented\n");
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void API dw_mle_set_cursor(HWND handle, int point)
{
	NSLog(@"dw_mle_set_cursor() unimplemented\n");
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
	NSLog(@"dw_mle_search() unimplemented\n");
	return 0;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
	NSLog(@"dw_mle_freeze() unimplemented\n");
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
	NSLog(@"dw_mle_thaw() unimplemented\n");
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_status_text_new(char *text, ULONG id)
{
	NSLog(@"dw_status_text_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(char *text, ULONG id)
{
	NSLog(@"dw_text_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_render_new() unimplemented\n");
	return HWND_DESKTOP;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
	_foreground = value;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
	_background = value;
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
	NSLog(@"dw_color_choose() unimplemented\n");
	return 0;
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
	NSLog(@"dw_draw_point() unimplemented\n");
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
	NSLog(@"dw_draw_line() unimplemented\n");
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
	NSLog(@"dw_draw_line() unimplemented\n");
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
	NSLog(@"dw_font_text_extents_get() unimplemented\n");
}

/* Draw a polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       fill: Fill box TRUE or FALSE.
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void API dw_draw_polygon( HWND handle, HPIXMAP pixmap, int fill, int npoints, int *x, int *y )
{
	NSLog(@"dw_draw_polygon() unimplemented\n");
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
	NSLog(@"dw_draw_rect() unimplemented\n");
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_tree_new(ULONG id)
{
	NSLog(@"dw_tree_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_tree_insert_item_after() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_tree_insert_item() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_get_title() unimplemented\n");
	return NULL;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
HTREEITEM API dw_tree_get_parent(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_get_parent() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_tree_item_change() unimplemented\n");
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
	NSLog(@"dw_tree_item_set_data() unimplemented\n");
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_item_get_data() unimplemented\n");
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
	NSLog(@"dw_tree_item_select() unimplemented\n");
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
	NSLog(@"dw_tree_clear() unimplemented\n");
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_item_expand() unimplemented\n");
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_item_collapse() unimplemented\n");
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
	NSLog(@"dw_tree_item_delete() unimplemented\n");
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_container_new(ULONG id, int multi)
{
	NSLog(@"dw_container_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_container_setup() unimplemented\n");
	return 0;
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
	NSLog(@"dw_filesystem_setup() unimplemented\n");
	return 0;
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
	NSLog(@"dw_container_alloc() unimplemented\n");
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
	NSLog(@"dw_container_set_item() unimplemented\n");
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
	NSLog(@"dw_container_change_item() unimplemented\n");
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_item(HWND handle, int column, int row, void *data)
{
	NSLog(@"dw_filesystem_change_item() unimplemented\n");
}

/*
 * Changes an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_file(HWND handle, int row, char *filename, unsigned long icon)
{
	NSLog(@"dw_filesystem_change_file() unimplemented\n");
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
	NSLog(@"dw_filesystem_set_file() unimplemented\n");
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
	NSLog(@"dw_filesystem_set_item() unimplemented\n");
}

/*
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
	NSLog(@"dw_container_get_column_type() unimplemented\n");
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
	NSLog(@"dw_filesystem_get_column_type() unimplemented\n");
	return 0;
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
	NSLog(@"dw_container_set_column_width() unimplemented\n");
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
	NSLog(@"dw_container_set_row_title() unimplemented\n");
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
	NSLog(@"dw_container_insert() unimplemented\n");
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
	NSLog(@"dw_container_clear() unimplemented\n");
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
	NSLog(@"dw_container_delete() unimplemented\n");
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
	NSLog(@"dw_container_scroll() unimplemented\n");
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
	NSLog(@"dw_container_query_start() unimplemented\n");
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
	NSLog(@"dw_container_query_next() unimplemented\n");
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
	NSLog(@"dw_container_cursor() unimplemented\n");
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, char *text)
{
	NSLog(@"dw_container_delete_row() unimplemented\n");
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
	NSLog(@"dw_container_optimize() unimplemented\n");
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void API dw_taskbar_insert(HWND handle, unsigned long icon, char *bubbletext)
{
	NSLog(@"dw_taskbar_insert() unimplemented\n");
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, unsigned long icon)
{
	NSLog(@"dw_taskbar_delete() unimplemented\n");
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
	NSLog(@"dw_icon_load() unimplemented\n");
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
	NSLog(@"dw_icon_load_from_file() unimplemented\n");
	return 0;
}

/*
 * Obtains an icon from data
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
unsigned long API dw_icon_load_from_data(char *data, int len)
{
	NSLog(@"dw_icon_load_from_data() unimplemented\n");
	return 0;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(unsigned long handle)
{
	NSLog(@"dw_icon_free() unimplemented\n");
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long id)
{
	NSLog(@"dw_mdi_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_splitbar_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void API dw_splitbar_set(HWND handle, float percent)
{
	NSLog(@"dw_splitbar_set() unimplemented\n");
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
	NSLog(@"dw_splitbar_get() unimplemented\n");
	return 0.0;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_bitmap_new(ULONG id)
{
	NSLog(@"dw_bitmap_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_pixmap_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_pixmap_new_from_file() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Creates a pixmap from memory.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of the image data
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       le: length of data
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new_from_data(HWND handle, char *data, int len)
{
	NSLog(@"dw_pixmap_new_from_data() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Creates a bitmap mask for rendering bitmaps with transparent backgrounds
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
	NSLog(@"dw_pixmap_set_transparent_color() unimplemented\n");
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
	NSLog(@"dw_pixmap_grab() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
void API dw_pixmap_destroy(HPIXMAP pixmap)
{
	NSLog(@"dw_pixmap_destroy() unimplemented\n");
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
	NSLog(@"dw_pixmap_bitblt() unimplemented\n");
}

/*
 * Create a new static text window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_calendar_new(ULONG id)
{
	NSLog(@"dw_calendar_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Sets the current date of a calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
void dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
	NSLog(@"dw_calendar_set_date() unimplemented\n");
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
	NSLog(@"dw_calendar_get_date() unimplemented\n");
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
	NSLog(@"dw_html_action() unimplemented\n");
}

/*
 * Render raw HTML code in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       string: String buffer containt HTML code to
 *               be rendered.
 * Returns:
 *       0 on success.
 */
int API dw_html_raw(HWND handle, char *string)
{
	WebView *html = handle;
	[[html mainFrame] loadHTMLString:[ NSString stringWithUTF8String:string ] baseURL:nil];
	return 0;
}

/*
 * Render file or web page in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       url: Universal Resource Locator of the web or
 *               file object to be rendered.
 * Returns:
 *       0 on success.
 */
int API dw_html_url(HWND handle, char *url)
{
	WebView *html = handle;
	[[html mainFrame] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[ NSString stringWithUTF8String:url ]]]];
	return 0;
}

/*
 * Create a new HTML window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_html_new(unsigned long id)
{
	WebView *web = [[WebView alloc] init];
	return web;
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
    NSPoint mouseLoc; 
    mouseLoc = [NSEvent mouseLocation];
	if(x)
	{
		*x = mouseLoc.x;
	}
	if(y)
	{
		*y = mouseLoc.y;
	}
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_pointer_set_pos(long x, long y)
{
	NSLog(@"dw_pointer_set_pos() unimplemented\n");
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI API dw_menu_new(ULONG id)
{
	NSMenu *menu = [[[NSMenu alloc] initWithTitle:@"Apple"] autorelease];
	return menu;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
	NSLog(@"dw_menubar_new() unimplemented\n");
	return HWND_DESKTOP;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
	NSMenu *thismenu = *menu;
	[thismenu dealloc];
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
	NSMenu *thismenu = (NSMenu *)menu;
	NSView *view = parent;
	NSEvent* fake = [[NSEvent alloc]
					mouseEventWithType:NSLeftMouseDown
					location:NSMakePoint(x,y)
					modifierFlags:0
					timestamp:1
					windowNumber:[[view window] windowNumber]
					context:[NSGraphicsContext currentContext]
					eventNumber:1
					clickCount:1
					pressure:0.0];
	[NSMenu popUpContextMenu:thismenu withEvent:fake forView:view];
}

char _removetilde(char *dest, char *src)
{
	int z, cur=0;
	char accel = '\0';
	
	for(z=0;z<strlen(src);z++)
	{
		if(src[z] != '~')
		{
			dest[cur] = src[z];
			cur++;
		}
		else
		{
			dest[cur] = '_';
			accel = src[z+1];
			cur++;
		}
	}
	dest[cur] = 0;
	return accel;
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
 *       flags: Extended attributes to set on the menu.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND API dw_menu_append_item(HMENUI menux, char *title, ULONG id, ULONG flags, int end, int check, HMENUI submenu)
{
	NSMenu *menu = menux;
	NSMenuItem *item = NULL;
	if(strlen(title) == 0)
	{
		[menu addItem:[NSMenuItem separatorItem]];
	}
	else 
	{
		char accel[2];
		char *newtitle = malloc(strlen(title)+1);
	
		accel[0] = _removetilde(newtitle, title);
		accel[1] = 0;
	
		item = [menu addItemWithTitle:[ NSString stringWithUTF8String:newtitle ]
													action:NULL
													keyEquivalent:[ NSString stringWithUTF8String:accel ]];
		free(newtitle);
		return item;
	}
	return item;
}

/*
 * Sets the state of a menu item check.
 * Deprecated; use dw_menu_item_set_state()
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 */
void API dw_menu_item_set_check(HMENUI menux, unsigned long id, int check)
{
	NSLog(@"dw_menu_item_set_check() unimplemented\n");
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       flags: DW_MIS_ENABLED/DW_MIS_DISABLED
 *              DW_MIS_CHECKED/DW_MIS_UNCHECKED
 */
void API dw_menu_item_set_state( HMENUI menux, unsigned long id, unsigned long state)
{
	NSLog(@"dw_menu_item_set_state() unimplemented\n");
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG id, int top)
{
	NSLog(@"dw_notebook_new() unimplemented\n");
	return HWND_DESKTOP;
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
	NSLog(@"dw_notebook_page_new() unimplemented\n");
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
	NSLog(@"dw_notebook_page_destroy() unimplemented\n");
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
	NSLog(@"dw_notebook_page_get() unimplemented\n");
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
	NSLog(@"dw_notebook_page_set() unimplemented\n");
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
	NSLog(@"dw_notebook_page_set_text() unimplemented\n");
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
	NSLog(@"dw_notebook_page_set_status_text() unimplemented\n");
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
	NSLog(@"dw_notebook_pack() unimplemented\n");
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
	NSRect frame = NSMakeRect(1,1,1,1);
    NSWindow *window = [[NSWindow alloc]
						initWithContentRect:frame
						styleMask:flStyle
						backing:NSBackingStoreBuffered
						defer:false];

    [window setTitle:[ NSString stringWithUTF8String:title ]];
	
    DWView *view = [[DWView alloc] init];
	
    [window setContentView:view];
    [window setDelegate:view];
    [window makeKeyAndOrderFront:nil];
	
	return (HWND)window;
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
	void (* windowfunc)(void *);
	
	windowfunc = function;
	
	if(windowfunc)
		windowfunc(data);	
}


/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
	NSLog(@"dw_window_set_pointer() unimplemented\n");
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
	NSObject *object = handle;
	
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		if([window isMiniaturized])
		{
			[window deminiaturize:nil];
		}
		[[window contentView] windowResized:nil];
	}
	return 0;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_hide(HWND handle)
{
	NSLog(@"dw_window_hide() unimplemented\n");
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
	NSLog(@"dw_window_set_color() unimplemented\n");
	return -1;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
	NSLog(@"dw_window_set_border() unimplemented\n");
	return 0;
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
	NSLog(@"dw_window_set_style() unimplemented\n");
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 */
void API dw_window_default(HWND window, HWND defaultitem)
{
	NSLog(@"dw_window_default() unimplemented\n");
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND window, HWND next)
{
	NSLog(@"dw_window_click_default() unimplemented\n");
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
	NSLog(@"dw_window_capture() unimplemented\n");
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
	NSLog(@"dw_window_release() unimplemented\n");
}

/*
 * Tracks this window movement.
 * Parameters:
 *       handle: Handle to frame to be tracked.
 */
void API dw_window_track(HWND handle)
{
	NSLog(@"dw_window_track() unimplemented\n");
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
	/* Is this even possible? */
	NSLog(@"dw_window_reparent() unimplemented\n");
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, char *fontname)
{
	NSLog(@"dw_window_set_font() unimplemented\n");
	return 0;
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int API dw_window_destroy(HWND handle)
{
	NSObject *object = handle;
	
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		[window close];
	}
	return 0;
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
	NSObject *object = handle;
	
	if([ object isKindOfClass:[ NSControl class ] ])
	{
		NSControl *control = handle;
		NSString *nsstr = [ control stringValue];
		
		return strdup([ nsstr UTF8String ]);
	}
	else if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		NSString *nsstr = [ window title];
	
		return strdup([ nsstr UTF8String ]);
	}
	return NULL;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, char *text)
{
	NSObject *object = handle;
	
	if([ object isKindOfClass:[ NSControl class ] ])
	{
		NSControl *control = handle;
		[control setStringValue:[ NSString stringWithUTF8String:text ]];
	}
	else if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		[window setTitle:[ NSString stringWithUTF8String:text ]];
	}
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
	NSObject *object = handle;
	if([ object isKindOfClass:[ NSControl class ] ])
	{
		NSControl *control = handle;
		[control setEnabled:NO];
	}
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
	NSObject *object = handle;
	if([ object isKindOfClass:[ NSControl class ] ])
	{
		NSControl *control = handle;
		[control setEnabled:YES];
	}
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
void API dw_window_set_bitmap_from_data(HWND handle, unsigned long id, char *data, int len)
{
	NSLog(@"dw_window_set_bitmap_from_data() unimplemented\n");
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
	NSLog(@"dw_window_set_bitmap() unimplemented\n");
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void API dw_window_set_icon(HWND handle, ULONG id)
{
	NSLog(@"dw_window_set_icon() unimplemented\n");
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
	NSObject *object = handle;
	NSView *view = handle;
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		view = [window contentView];
	}
	return [view viewWithTag:id];
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int API dw_window_minimize(HWND handle)
{
	NSWindow *window = handle;
	[window miniaturize:nil];
	return 0;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
	NSWindow *window = handle;
	[window flushWindow];
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
	NSWindow *window = handle;
	[window orderFront:nil];
	return 0;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
	NSWindow *window = handle;
	[window orderBack:nil];
	return 0;
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
	NSObject *object = handle;
	NSSize size;
	size.width = width;
	size.height = height; 
	
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		[window setContentSize:size];
	}
}

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
void API dw_window_set_pos(HWND handle, LONG x, LONG y)
{
	NSObject *object = handle;
	NSPoint point;
	point.x = x;
	point.y = y; 
	
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		[window setFrameOrigin:point];
	}
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
void API dw_window_set_pos_size(HWND handle, LONG x, LONG y, ULONG width, ULONG height)
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
void API dw_window_get_pos_size(HWND handle, LONG *x, LONG *y, ULONG *width, ULONG *height)
{
	NSObject *object = handle;
	
	if([ object isKindOfClass:[ NSWindow class ] ])
	{
		NSWindow *window = handle;
		NSRect rect = [window frame];
		if(x)
			*x = rect.origin.x;
		if(y)
			*y = rect.origin.y;
		if(width)
			*width = rect.size.width;
		if(height)
			*height = rect.size.height;
		return; 
	}
}

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
	NSRect screenRect = [[NSScreen mainScreen] frame]; 
	return screenRect.size.width;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
	NSRect screenRect = [[NSScreen mainScreen] frame]; 
	return screenRect.size.height;
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
	NSWindowDepth screenDepth = [[NSScreen mainScreen] depth];
	return NSBitsPerPixelFromDepth(screenDepth);
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void dw_environment_query(DWEnv *env)
{
	struct utsname name;
	char tempbuf[100];
	int len, z;
	
	uname(&name);
	strcpy(env->osName, name.sysname);
	strcpy(tempbuf, name.release);
	
	env->MajorBuild = env->MinorBuild = 0;
	
	len = strlen(tempbuf);
	
	strcpy(env->buildDate, __DATE__);
	strcpy(env->buildTime, __TIME__);
	env->DWMajorVersion = DW_MAJOR_VERSION;
	env->DWMinorVersion = DW_MINOR_VERSION;
	env->DWSubVersion = DW_SUB_VERSION;
	
	for(z=1;z<len;z++)
	{
		if(tempbuf[z] == '.')
		{
			tempbuf[z] = '\0';
			env->MajorVersion = atoi(&tempbuf[z-1]);
			env->MinorVersion = atoi(&tempbuf[z+1]);
			return;
		}
	}
	env->MajorVersion = atoi(tempbuf);
	env->MinorVersion = 0;
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
	NSBeep();
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void API dw_flush(void)
{
}

/* Functions for managing the user data lists that are associated with
 * a given window handle.  Used in dw_window_set_data() and
 * dw_window_get_data().
 */
UserData *_find_userdata(UserData **root, char *varname)
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

int _new_userdata(UserData **root, char *varname, void *data)
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

int _remove_userdata(UserData **root, char *varname, int all)
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
void dw_window_set_data(HWND window, char *dataname, void *data)
{
	id object = window;
	WindowData *blah = (WindowData *)[object userdata];
	
	if(!blah)
	{
		if(!dataname)
			return;
		
		blah = calloc(1, sizeof(WindowData));
		[object setUserdata:blah];
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
	id object = window;
	WindowData *blah = (WindowData *)[object userdata];
	
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
	NSLog(@"dw_timer_connect() unimplemented\n");
	if(sigfunc)
	{
		int timerid = 0;
		/* = WinStartTimer(dwhab, NULLHANDLE, 0, interval)*/
		
		if(timerid)
		{
			_new_signal(0, HWND_DESKTOP, timerid, sigfunc, data);
			return timerid;
		}
	}
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
	NSLog(@"dw_timer_disconnect() unimplemented\n");
	
	/* 0 is an invalid timer ID */
	if(!id)
		return;
	
	/*WinStopTimer(dwhab, NULLHANDLE, id);*/
	
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
	ULONG message = 0, id = 0;
	
	if(window && signame && sigfunc)
	{
		if((message = _findsigmessage(signame)) != 0)
		{
			_new_signal(message, window, id, sigfunc, data);
		}
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

void _my_strlwr(char *buf)
{
   int z, len = strlen(buf);

   for(z=0;z<len;z++)
   {
      if(buf[z] >= 'A' && buf[z] <= 'Z')
         buf[z] -= 'A' - 'a';
   }
}

/* Open a shared library and return a handle.
 * Parameters:
 *         name: Base name of the shared library.
 *         handle: Pointer to a module handle,
 *                 will be filled in with the handle.
 */
int dw_module_load(char *name, HMOD *handle)
{
   int len;
   char *newname;
   char errorbuf[1024];


   if(!handle)
      return -1;

   if((len = strlen(name)) == 0)
      return   -1;

   /* Lenth + "lib" + ".dylib" + NULL */
   newname = malloc(len + 10);

   if(!newname)
      return -1;

   sprintf(newname, "lib%s.dylib", name);
   _my_strlwr(newname);

   *handle = dlopen(newname, RTLD_NOW);
   if(*handle == NULL)
   {
      strncpy(errorbuf, dlerror(), 1024);
      printf("%s\n", errorbuf);
      sprintf(newname, "lib%s.dylib", name);
      *handle = dlopen(newname, RTLD_NOW);
   }

   free(newname);

   return (NULL == *handle) ? -1 : 0;
}

/* Queries the address of a symbol within open handle.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 *         name: Name of the symbol you want the address of.
 *         func: A pointer to a function pointer, to obtain
 *               the address.
 */
int dw_module_symbol(HMOD handle, char *name, void**func)
{
   if(!func || !name)
      return   -1;

   if(strlen(name) == 0)
      return   -1;

   *func = (void*)dlsym(handle, name);
   return   (NULL == *func);
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int dw_module_close(HMOD handle)
{
   if(handle)
      return dlclose(handle);
   return 0;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX dw_mutex_new(void)
{
   HMTX mutex = malloc(sizeof(pthread_mutex_t));

   pthread_mutex_init(mutex, NULL);
   return mutex;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void dw_mutex_close(HMTX mutex)
{
   if(mutex)
   {
      pthread_mutex_destroy(mutex);
      free(mutex);
   }
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void dw_mutex_lock(HMTX mutex)
{
   pthread_mutex_lock(mutex);
}

/*
 * Reliquishes the access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void dw_mutex_unlock(HMTX mutex)
{
   pthread_mutex_unlock(mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV dw_event_new(void)
{
   HEV eve = (HEV)malloc(sizeof(struct _dw_unix_event));

   if(!eve)
      return NULL;

   /* We need to be careful here, mutexes on Linux are
    * FAST by default but are error checking on other
    * systems such as FreeBSD and OS/2, perhaps others.
    */
   pthread_mutex_init (&(eve->mutex), NULL);
   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_init (&(eve->event), NULL);

   pthread_mutex_unlock (&(eve->mutex));
   eve->alive = 1;
   eve->posted = 0;

   return eve;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int dw_event_reset (HEV eve)
{
   if(!eve)
      return FALSE;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   pthread_cond_init (&(eve->event), NULL);
   eve->posted = 0;
   pthread_mutex_unlock (&(eve->mutex));
   return 0;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int dw_event_post (HEV eve)
{
   if(!eve)
      return FALSE;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   eve->posted = 1;
   pthread_mutex_unlock (&(eve->mutex));
   return 0;
}

/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int dw_event_wait(HEV eve, unsigned long timeout)
{
   int rc;
   struct timeval now;
   struct timespec timeo;

   if(!eve)
      return FALSE;

   if(eve->posted)
      return 0;

   pthread_mutex_lock (&(eve->mutex));
   gettimeofday(&now, 0);
   timeo.tv_sec = now.tv_sec + (timeout / 1000);
   timeo.tv_nsec = now.tv_usec * 1000;
   rc = pthread_cond_timedwait (&(eve->event), &(eve->mutex), &timeo);
   pthread_mutex_unlock (&(eve->mutex));
   if(!rc)
      return 1;
   if(rc == ETIMEDOUT)
      return -1;
   return 0;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int dw_event_close(HEV *eve)
{
   if(!eve || !(*eve))
      return FALSE;

   pthread_mutex_lock (&((*eve)->mutex));
   pthread_cond_destroy (&((*eve)->event));
   pthread_mutex_unlock (&((*eve)->mutex));
   pthread_mutex_destroy (&((*eve)->mutex));
   free(*eve);
   *eve = NULL;

   return TRUE;
}

struct _seminfo {
   int fd;
   int waiting;
};

static void _handle_sem(int *tmpsock)
{
   fd_set rd;
   struct _seminfo *array = (struct _seminfo *)malloc(sizeof(struct _seminfo));
   int listenfd = tmpsock[0];
   int bytesread, connectcount = 1, maxfd, z, posted = 0;
   char command;
   sigset_t mask;

   sigfillset(&mask); /* Mask all allowed signals */
   pthread_sigmask(SIG_BLOCK, &mask, NULL);

   /* problems */
   if(tmpsock[1] == -1)
   {
      free(array);
      return;
   }

   array[0].fd = tmpsock[1];
   array[0].waiting = 0;

   /* Free the memory allocated in dw_named_event_new. */
   free(tmpsock);

   while(1)
   {
      FD_ZERO(&rd);
      FD_SET(listenfd, &rd);

      maxfd = listenfd;

      /* Added any connections to the named event semaphore */
      for(z=0;z<connectcount;z++)
      {
         if(array[z].fd > maxfd)
            maxfd = array[z].fd;

         FD_SET(array[z].fd, &rd);
      }

      if(select(maxfd+1, &rd, NULL, NULL, NULL) == -1)
         return;

      if(FD_ISSET(listenfd, &rd))
      {
         struct _seminfo *newarray;
            int newfd = accept(listenfd, 0, 0);

         if(newfd > -1)
         {
            /* Add new connections to the set */
            newarray = (struct _seminfo *)malloc(sizeof(struct _seminfo)*(connectcount+1));
            memcpy(newarray, array, sizeof(struct _seminfo)*(connectcount));

            newarray[connectcount].fd = newfd;
            newarray[connectcount].waiting = 0;

            connectcount++;

            /* Replace old array with new one */
            free(array);
            array = newarray;
         }
      }

      /* Handle any events posted to the semaphore */
      for(z=0;z<connectcount;z++)
      {
         if(FD_ISSET(array[z].fd, &rd))
         {
            if((bytesread = read(array[z].fd, &command, 1)) < 1)
            {
               struct _seminfo *newarray;

               /* Remove this connection from the set */
               newarray = (struct _seminfo *)malloc(sizeof(struct _seminfo)*(connectcount-1));
               if(!z)
                  memcpy(newarray, &array[1], sizeof(struct _seminfo)*(connectcount-1));
               else
               {
                  memcpy(newarray, array, sizeof(struct _seminfo)*z);
                  if(z!=(connectcount-1))
                     memcpy(&newarray[z], &array[z+1], sizeof(struct _seminfo)*(z-connectcount-1));
               }
               connectcount--;

               /* Replace old array with new one */
               free(array);
               array = newarray;
            }
            else if(bytesread == 1)
            {
               switch(command)
               {
               case 0:
                  {
                  /* Reset */
                  posted = 0;
                  }
                  break;
               case 1:
                  /* Post */
                  {
                     int s;
                     char tmp = (char)0;

                     posted = 1;

                     for(s=0;s<connectcount;s++)
                     {
                        /* The semaphore has been posted so
                         * we tell all the waiting threads to
                         * continue.
                         */
                        if(array[s].waiting)
                           write(array[s].fd, &tmp, 1);
                     }
                  }
                  break;
               case 2:
                  /* Wait */
                  {
                     char tmp = (char)0;

                     array[z].waiting = 1;

                     /* If we are posted exit immeditately */
                     if(posted)
                        write(array[z].fd, &tmp, 1);
                  }
                  break;
               case 3:
                  {
                     /* Done Waiting */
                     array[z].waiting = 0;
                  }
                  break;
               }
            }
         }
      }

   }

}

/* Using domain sockets on unix for IPC */
/* Create a named event semaphore which can be
 * opened from other processes.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV dw_named_event_new(char *name)
{
	struct sockaddr_un un;
	int ev, *tmpsock = (int *)malloc(sizeof(int)*2);
	HEV eve;
	DWTID dwthread;

	if(!tmpsock)
		return NULL;

	eve = (HEV)malloc(sizeof(struct _dw_unix_event));

	if(!eve)
	{
		free(tmpsock);
     	return NULL;
	}
	
	tmpsock[0] = socket(AF_UNIX, SOCK_STREAM, 0);
	ev = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&un, 0, sizeof(un));
	un.sun_family=AF_UNIX;
	mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
	strcpy(un.sun_path, "/tmp/.dw/");
	strcat(un.sun_path, name);

	/* just to be safe, this should be changed
	 * to support multiple instances.
	 */
	remove(un.sun_path);

	bind(tmpsock[0], (struct sockaddr *)&un, sizeof(un));
	listen(tmpsock[0], 0);
	connect(ev, (struct sockaddr *)&un, sizeof(un));
	tmpsock[1] = accept(tmpsock[0], 0, 0);

	if(tmpsock[0] < 0 || tmpsock[1] < 0 || ev < 0)
	{
		if(tmpsock[0] > -1)
			close(tmpsock[0]);
		if(tmpsock[1] > -1)
			close(tmpsock[1]);
		if(ev > -1)
			close(ev);
		free(tmpsock);
		free(eve);
		return NULL;
	}

	/* Create a thread to handle this event semaphore */
	pthread_create(&dwthread, NULL, (void *)_handle_sem, (void *)tmpsock);
	eve->alive = ev;
	return eve;
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV dw_named_event_get(char *name)
{
	struct sockaddr_un un;
	HEV eve;
	int ev = socket(AF_UNIX, SOCK_STREAM, 0);
	if(ev < 0)
		return NULL;

	eve = (HEV)malloc(sizeof(struct _dw_unix_event));

	if(!eve)
	{
		close(ev);
     	return NULL;
	}

	un.sun_family=AF_UNIX;
	mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
	strcpy(un.sun_path, "/tmp/.dw/");
	strcat(un.sun_path, name);
	connect(ev, (struct sockaddr *)&un, sizeof(un));
	eve->alive = ev;
	return eve;
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int dw_named_event_reset(HEV eve)
{
   /* signal reset */
   char tmp = (char)0;

   if(!eve || eve->alive < 0)
      return 0;

   if(write(eve->alive, &tmp, 1) == 1)
      return 0;
   return 1;
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int dw_named_event_post(HEV eve)
{

   /* signal post */
   char tmp = (char)1;

   if(!eve || eve->alive < 0)
      return 0;

   if(write(eve->alive, &tmp, 1) == 1)
      return 0;
   return 1;
}

/* Waits on the specified semaphore until it becomes
 * posted, or returns immediately if it already is posted.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 *         timeout: Number of milliseconds before timing out
 *                  or -1 if indefinite.
 */
int dw_named_event_wait(HEV eve, unsigned long timeout)
{
   fd_set rd;
   struct timeval tv, *useme;
   int retval = 0;
   char tmp;

   if(!eve || eve->alive < 0)
      return DW_ERROR_NON_INIT;

   /* Set the timout or infinite */
   if(timeout == -1)
      useme = NULL;
   else
   {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;

      useme = &tv;
   }

   FD_ZERO(&rd);
   FD_SET(eve->alive, &rd);

   /* Signal wait */
   tmp = (char)2;
   write(eve->alive, &tmp, 1);

   retval = select(eve->alive+1, &rd, NULL, NULL, useme);

   /* Signal done waiting. */
   tmp = (char)3;
   write(eve->alive, &tmp, 1);

   if(retval == 0)
      return DW_ERROR_TIMEOUT;
   else if(retval == -1)
      return DW_ERROR_INTERRUPT;

   /* Clear the entry from the pipe so
    * we don't loop endlessly. :)
    */
   read(eve->alive, &tmp, 1);
   return 0;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int dw_named_event_close(HEV eve)
{
   /* Finally close the domain socket,
    * cleanup will continue in _handle_sem.
    */
	if(eve)
	{
		close(eve->alive);
		free(eve);
	}
	return 0;
}

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
   void (*threadfunc)(void *) = NULL;
   void **tmp = (void **)data;

   threadfunc = (void (*)(void *))tmp[0];

   threadfunc(tmp[1]);
   free(tmp);
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         handle: A pointer to receive a SHM identifier.
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 */
HSHM dw_named_memory_new(void **dest, int size, char *name)
{
   char namebuf[1024];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   sprintf(namebuf, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
   {
      free(handle);
      return NULL;
   }

   ftruncate(handle->fd, size);

   /* attach the shared memory segment to our process's address space. */
   *dest = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

   if(*dest == MAP_FAILED)
   {
      close(handle->fd);
      *dest = NULL;
      free(handle);
      return NULL;
   }

   handle->size = size;
   handle->sid = getsid(0);
   handle->path = strdup(namebuf);

   return handle;
}

/*
 * Aquires shared memory region with a name.
 * Parameters:
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to requested.
 *         name: A string pointer to a unique memory name.
 */
HSHM dw_named_memory_get(void **dest, int size, char *name)
{
   char namebuf[1024];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   sprintf(namebuf, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_RDWR)) < 0)
   {
      free(handle);
      return NULL;
   }

   /* attach the shared memory segment to our process's address space. */
   *dest = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

   if(*dest == MAP_FAILED)
   {
      close(handle->fd);
      *dest = NULL;
      free(handle);
      return NULL;
   }

   handle->size = size;
   handle->sid = -1;
   handle->path = NULL;

   return handle;
}

/*
 * Frees a shared memory region previously allocated.
 * Parameters:
 *         handle: Handle obtained from DB_named_memory_allocate.
 *         ptr: The memory address aquired with DB_named_memory_allocate.
 */
int dw_named_memory_free(HSHM handle, void *ptr)
{
   struct _dw_unix_shm *h = handle;
   int rc = munmap(ptr, h->size);

   close(h->fd);
   if(h->path)
   {
      /* Only remove the actual file if we are the
       * creator of the file.
       */
      if(h->sid != -1 && h->sid == getsid(0))
         remove(h->path);
      free(h->path);
   }
   return rc;
}
/*
 * Creates a new thread with a starting point of func.
 * Parameters:
 *       func: Function which will be run in the new thread.
 *       data: Parameter(s) passed to the function.
 *       stack: Stack size of new thread (OS/2 and Windows only).
 */
DWTID dw_thread_new(void *func, void *data, int stack)
{
   DWTID thread;
   void **tmp = malloc(sizeof(void *) * 2);
   int rc;

   tmp[0] = func;
   tmp[1] = data;

   rc = pthread_create(&thread, NULL, (void *)_dwthreadstart, (void *)tmp);
   if(rc == 0)
      return thread;
   return (DWTID)-1;
}

/*
 * Ends execution of current thread immediately.
 */
void dw_thread_end(void)
{
   pthread_exit(NULL);
}

/*
 * Returns the current thread's ID.
 */
DWTID dw_thread_id(void)
{
   return (DWTID)pthread_self();
}

#ifdef DWTEST
int main(int argc, char *argv[]) 
{
	HWND window, box, vbox, hbox, button, text, checkbox, html;
	LONG x, y;
	ULONG width, height;
	
	dw_init(TRUE, argc, argv);
	
	window = dw_window_new(HWND_DESKTOP, "Dynamic Windows Test", DW_FCF_TITLEBAR | DW_FCF_SYSMENU | DW_FCF_MINMAX | DW_FCF_SIZEBORDER);
	box = dw_box_new(DW_VERT, 0);
	vbox = dw_groupbox_new(DW_VERT, 4, "Checks");
	checkbox = dw_checkbox_new("Checkbox 1", 0);
	dw_box_pack_start(vbox, checkbox, 100, 25, TRUE, FALSE, 2);
	checkbox = dw_checkbox_new("Checkbox 2", 0);
	dw_box_pack_start(vbox, checkbox, 100, 25, TRUE, FALSE, 2);
	checkbox = dw_checkbox_new("Checkbox 3", 0);
	dw_box_pack_start(vbox, checkbox, 100, 25, TRUE, FALSE, 2);
	hbox = dw_box_new(DW_HORZ, 0);
	button = dw_button_new("Test Button", 0);
	/*dw_window_disable(button);*/
	text = dw_entryfield_new("Entry", 0);
	dw_box_pack_start(hbox, button, 100, 40, TRUE, FALSE, 2);
	dw_box_pack_start(hbox, text, 100, 40, TRUE, FALSE, 2);
	dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
	html = dw_html_new(0);
	dw_html_url(html, "http://dbsoft.org");
	dw_box_pack_start(vbox, html, 0, 0, TRUE, TRUE, 0);
	dw_box_pack_start(box, vbox, 0, 0, TRUE, TRUE, 0);
	dw_box_pack_start(window, box, 0, 0, TRUE, TRUE, 0);
	dw_window_show(window);
	dw_window_set_pos_size(window, 400, 400, 500, 500);
	dw_window_get_pos_size(window, &x, &y, &width, &height);
	dw_messagebox("Dynamic Windows Information", DW_MB_OK | DW_MB_INFORMATION, "%d %d %d %d %d %d %d\n", (int)x, (int)y, (int)width, (int)height, (int)dw_screen_width(), (int)dw_screen_height(), (int)dw_color_depth_get());
	dw_messagebox("File selection", DW_MB_OK | DW_MB_INFORMATION, "%s", dw_file_browse("Choose file", "", "", DW_FILE_OPEN));
	dw_main();
	
	return 0;
}
#endif