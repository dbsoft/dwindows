/*
 * Dynamic Windows:
 *          A GTK like implementation of the MacOS GUI using Cocoa
 *
 * (C) 2011-2016 Brian Smith <brian@dbsoft.org>
 * (C) 2011 Mark Hessling <mark@rexx.org>
 *
 * Requires 10.5 or later.
 * clang -std=c99 -g -o dwtest -D__MAC__ -I. dwtest.c mac/dw.m -framework Cocoa -framework WebKit
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
#include <math.h>

/* Create a define to let us know to include Snow Leopard specific features */
#if defined(MAC_OS_X_VERSION_10_6) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_SNOW_LEOPARD
#endif

/* Macros to protect access to thread unsafe classes */
#define  DW_MUTEX_LOCK { \
    if(DWThread != (DWTID)-1 && pthread_self() != DWThread && pthread_self() != _dw_mutex_locked) { \
        dw_mutex_lock(DWThreadMutex); \
        _dw_mutex_locked = pthread_self(); \
        dw_mutex_lock(DWThreadMutex2); \
        [DWObj performSelectorOnMainThread:@selector(synchronizeThread:) withObject:nil waitUntilDone:NO]; \
        dw_mutex_lock(DWRunMutex); \
        _locked_by_me = TRUE; } }
#define  DW_MUTEX_UNLOCK { \
    if(pthread_self() != DWThread && _locked_by_me == TRUE) { \
        dw_mutex_unlock(DWRunMutex); \
        dw_mutex_unlock(DWThreadMutex2); \
        _dw_mutex_locked = (pthread_t)-1; \
        dw_mutex_unlock(DWThreadMutex); \
        _locked_by_me = FALSE; } }

/* Macros to handle local auto-release pools */
#define DW_LOCAL_POOL_IN NSAutoreleasePool *localpool = nil; \
        if(DWThread != (DWTID)-1 && pthread_self() != DWThread) \
            localpool = [[NSAutoreleasePool alloc] init];
#define DW_LOCAL_POOL_OUT if(localpool) [localpool drain];

/* Handle deprecation of several constants in 10.10...
 * the replacements are not available in earlier versions.
 */
#if defined(MAC_OS_X_VERSION_10_9) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_9) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWModalResponseOK NSModalResponseOK
#define DWModalResponseCancel NSModalResponseCancel
#define DWPaperOrientationPortrait NSPaperOrientationPortrait
#else
#define DWModalResponseOK NSOKButton
#define DWModalResponseCancel NSCancelButton
#define DWPaperOrientationPortrait NSPortraitOrientation
#endif

unsigned long _colors[] =
{
    0x00000000,   /* 0  black */
    0x000000bb,   /* 1  red */
    0x0000bb00,   /* 2  green */
    0x0000aaaa,   /* 3  yellow */
    0x00cc0000,   /* 4  blue */
    0x00bb00bb,   /* 5  magenta */
    0x00bbbb00,   /* 6  cyan */
    0x00bbbbbb,   /* 7  white */
    0x00777777,   /* 8  grey */
    0x000000ff,   /* 9  bright red */
    0x0000ff00,   /* 10 bright green */
    0x0000eeee,   /* 11 bright yellow */
    0x00ff0000,   /* 12 bright blue */
    0x00ff00ff,   /* 13 bright magenta */
    0x00eeee00,   /* 14 bright cyan */
    0x00ffffff,   /* 15 bright white */
    0xff000000    /* 16 default color */
};

/*
 * List those icons that have transparency first
 */
#define NUM_EXTS 8
char *image_exts[NUM_EXTS] =
{
   ".png",
   ".ico",
   ".icns",
   ".gif",
   ".jpg",
   ".jpeg",
   ".tiff",
   ".bmp"
};

char *_dw_get_image_extension( char *filename )
{
   char *file = alloca(strlen(filename) + 6);
   int found_ext = 0,i;

   /* Try various extentions */
   for ( i = 0; i < NUM_EXTS; i++ )
   {
      strcpy( file, filename );
      strcat( file, image_exts[i] );
      if ( access( file, R_OK ) == 0 )
      {
         found_ext = 1;
         break;
      }
   }
   if ( found_ext == 1 )
   {
      return image_exts[i];
   }
   return NULL;
}

unsigned long _get_color(unsigned long thiscolor)
{
    if(thiscolor & DW_RGB_COLOR)
    {
        return thiscolor & ~DW_RGB_COLOR;
    }
    else if(thiscolor < 17)
    {
        return _colors[thiscolor];
    }
    return 0;
}

/* Thread specific storage */
#if !defined(GARBAGE_COLLECT)
pthread_key_t _dw_pool_key;
#endif
pthread_key_t _dw_fg_color_key;
pthread_key_t _dw_bg_color_key;
int DWOSMajor, DWOSMinor, DWOSBuild;
static char _dw_bundle_path[PATH_MAX+1] = { 0 };

/* Create a default colors for a thread */
void _init_colors(void)
{
    NSColor *fgcolor = [[NSColor grayColor] retain];

    pthread_setspecific(_dw_fg_color_key, fgcolor);
    pthread_setspecific(_dw_bg_color_key, NULL);
}

typedef struct _sighandler
{
    struct _sighandler   *next;
    ULONG message;
    HWND window;
    int id;
    void *signalfunction;
    void *discfunction;
    void *data;

} SignalHandler;

SignalHandler *Root = NULL;

/* Some internal prototypes */
static void _do_resize(Box *thisbox, int x, int y);
void _handle_resize_events(Box *thisbox);
int _remove_userdata(UserData **root, char *varname, int all);
int _dw_main_iteration(NSDate *date);

/* Internal function to queue a window redraw */
void _dw_redraw(id window, int skip)
{
    static id lastwindow = nil;
    id redraw = lastwindow;

    if(skip && window == nil)
        return;

    lastwindow = window;
    if(redraw != lastwindow && redraw != nil)
    {
        dw_window_redraw(redraw);
    }
}

SignalHandler *_get_handler(HWND window, int messageid)
{
    SignalHandler *tmp = Root;

    /* Find any callbacks for this function */
    while(tmp)
    {
        if(tmp->message == messageid && window == tmp->window)
        {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

typedef struct
{
    ULONG message;
    char name[30];

} SignalList;

/* List of signals */
#define SIGNALMAX 17

SignalList SignalTranslate[SIGNALMAX] = {
    { 1,    DW_SIGNAL_CONFIGURE },
    { 2,    DW_SIGNAL_KEY_PRESS },
    { 3,    DW_SIGNAL_BUTTON_PRESS },
    { 4,    DW_SIGNAL_BUTTON_RELEASE },
    { 5,    DW_SIGNAL_MOTION_NOTIFY },
    { 6,    DW_SIGNAL_DELETE },
    { 7,    DW_SIGNAL_EXPOSE },
    { 8,    DW_SIGNAL_CLICKED },
    { 9,    DW_SIGNAL_ITEM_ENTER },
    { 10,   DW_SIGNAL_ITEM_CONTEXT },
    { 11,   DW_SIGNAL_LIST_SELECT },
    { 12,   DW_SIGNAL_ITEM_SELECT },
    { 13,   DW_SIGNAL_SET_FOCUS },
    { 14,   DW_SIGNAL_VALUE_CHANGED },
    { 15,   DW_SIGNAL_SWITCH_PAGE },
    { 16,   DW_SIGNAL_TREE_EXPAND },
    { 17,   DW_SIGNAL_COLUMN_CLICK }
};

int _event_handler1(id object, NSEvent *event, int message)
{
    SignalHandler *handler = _get_handler(object, message);
    /* NSLog(@"Event handler - type %d\n", message); */

    if(handler)
    {
        switch(message)
        {
            /* Timer event */
            case 0:
            {
                int (* API timerfunc)(void *) = (int (* API)(void *))handler->signalfunction;

                if(!timerfunc(handler->data))
                    dw_timer_disconnect(handler->id);
                return 0;
            }
            /* Configure/Resize event */
            case 1:
            {
                int (*sizefunc)(HWND, int, int, void *) = handler->signalfunction;
                NSSize size;

                if([object isKindOfClass:[NSWindow class]])
                {
                    NSWindow *window = object;
                    size = [[window contentView] frame].size;
                }
                else
                {
                    NSView *view = object;
                    size = [view frame].size;
                }

                if(size.width > 0 && size.height > 0)
                {
                    return sizefunc(object, size.width, size.height, handler->data);
                }
                return 0;
            }
            case 2:
            {
                int (*keypressfunc)(HWND, char, int, int, void *, char *) = handler->signalfunction;
                NSString *nchar = [event charactersIgnoringModifiers];
                int special = (int)[event modifierFlags];
                unichar vk = [nchar characterAtIndex:0];
                char *utf8 = NULL, ch = '\0';

                /* Handle a valid key */
                if([nchar length] == 1)
                {
                    char *tmp = (char *)[nchar UTF8String];
                    if(tmp && strlen(tmp) == 1)
                    {
                        ch = tmp[0];
                    }
                    utf8 = tmp;
                }

                return keypressfunc(handler->window, ch, (int)vk, special, handler->data, utf8);
            }
            /* Button press and release event */
            case 3:
            case 4:
            {
                int (* API buttonfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                NSPoint p = [NSEvent mouseLocation];
                int button = 1;

                if([event isMemberOfClass:[NSEvent class]])
                {
                    id view = [[[event window] contentView] superview];
                    NSEventType type = [event type];

                    p = [view convertPoint:[event locationInWindow] toView:object];

#ifdef MAC_OS_X_VERSION_10_12
                    if(type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseUp)
#else
                    if(type == NSRightMouseDown || type == NSRightMouseUp)
#endif
                    {
                        button = 2;
                    }
#ifdef MAC_OS_X_VERSION_10_12
                    else if(type == NSEventTypeOtherMouseDown || type == NSEventTypeOtherMouseDown)
#else
                    else if(type == NSOtherMouseDown || type == NSOtherMouseUp)
#endif
                    {
                        button = 3;
                    }
#ifdef MAC_OS_X_VERSION_10_12
                    else if([event modifierFlags] & NSEventModifierFlagControl)
#else
                    else if([event modifierFlags] & NSControlKeyMask)
#endif
                    {
                        button = 2;
                    }
                }

                return buttonfunc(object, (int)p.x, (int)p.y, button, handler->data);
            }
            /* Motion notify event */
            case 5:
            {
                int (* API motionfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                id view = [[[event window] contentView] superview];
                NSPoint p = [view convertPoint:[event locationInWindow] toView:object];
                SEL spmb = NSSelectorFromString(@"pressedMouseButtons");
                IMP ipmb = [[NSEvent class] respondsToSelector:spmb] ? [[NSEvent class] methodForSelector:spmb] : 0;
                int buttonmask = ipmb ? (int)ipmb([NSEvent class], spmb) : (1 << [event buttonNumber]);

                return motionfunc(object, (int)p.x, (int)p.y, buttonmask, handler->data);
            }
            /* Window close event */
            case 6:
            {
                int (* API closefunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;
                return closefunc(object, handler->data);
            }
            /* Window expose/draw event */
            case 7:
            {
                DWExpose exp;
                int (* API exposefunc)(HWND, DWExpose *, void *) = (int (* API)(HWND, DWExpose *, void *))handler->signalfunction;
                NSRect rect = [object frame];

                exp.x = rect.origin.x;
                exp.y = rect.origin.y;
                exp.width = rect.size.width;
                exp.height = rect.size.height;
                int result = exposefunc(object, &exp, handler->data);
                [[object window] flushWindow];
                return result;
            }
            /* Clicked event for buttons and menu items */
            case 8:
            {
                int (* API clickfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return clickfunc(object, handler->data);
            }
            /* Container class selection event */
            case 9:
            {
                int (*containerselectfunc)(HWND, char *, void *, void *) = handler->signalfunction;
                void **params = (void **)event;

                return containerselectfunc(handler->window, params[0], handler->data, params[1]);
            }
            /* Container context menu event */
            case 10:
            {
                int (* API containercontextfunc)(HWND, char *, int, int, void *, void *) = (int (* API)(HWND, char *, int, int, void *, void *))handler->signalfunction;
                char *text = (char *)event;
                void *user = NULL;
                LONG x,y;

                /* Fill in both items for the tree */
                if([object isKindOfClass:[NSOutlineView class]])
                {
                    id item = event;
                    NSString *nstr = [item objectAtIndex:1];
                    text = (char *)[nstr UTF8String];
                    NSValue *value = [item objectAtIndex:2];
                    if(value && [value isKindOfClass:[NSValue class]])
                    {
                        user = [value pointerValue];
                    }
                }

                dw_pointer_query_pos(&x, &y);

                return containercontextfunc(handler->window, text, (int)x, (int)y, handler->data, user);
            }
            /* Generic selection changed event for several classes */
            case 11:
            case 14:
            {
                int (* API valuechangedfunc)(HWND, int, void *) = (int (* API)(HWND, int, void *))handler->signalfunction;
                int selected = DW_POINTER_TO_INT(event);

                return valuechangedfunc(handler->window, selected, handler->data);;
            }
            /* Tree class selection event */
            case 12:
            {
                int (* API treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = (int (* API)(HWND, HTREEITEM, char *, void *, void *))handler->signalfunction;
                char *text = NULL;
                void *user = NULL;
                id item = nil;

                if([object isKindOfClass:[NSOutlineView class]])
                {
                    item = (id)event;
                    NSString *nstr = [item objectAtIndex:1];

                    if(nstr)
                    {
                        text = strdup([nstr UTF8String]);
                    }

                    NSValue *value = [item objectAtIndex:2];
                    if(value && [value isKindOfClass:[NSValue class]])
                    {
                        user = [value pointerValue];
                    }
                    int result = treeselectfunc(handler->window, item, text, handler->data, user);
                    if(text)
                    {
                        free(text);
                    }
                    return result;
                }
                else if(event)
                {
                    void **params = (void **)event;

                    text = params[0];
                    user = params[1];
                }

                return treeselectfunc(handler->window, item, text, handler->data, user);
            }
            /* Set Focus event */
            case 13:
            {
                int (* API setfocusfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return setfocusfunc(handler->window, handler->data);
            }
            /* Notebook page change event */
            case 15:
            {
                int (* API switchpagefunc)(HWND, unsigned long, void *) = (int (* API)(HWND, unsigned long, void *))handler->signalfunction;
                int pageid = DW_POINTER_TO_INT(event);

                return switchpagefunc(handler->window, pageid, handler->data);
            }
            case 16:
            {
                int (* API treeexpandfunc)(HWND, HTREEITEM, void *) = (int (* API)(HWND, HTREEITEM, void *))handler->signalfunction;

                return treeexpandfunc(handler->window, (HTREEITEM)event, handler->data);
            }
            case 17:
            {
                int (*clickcolumnfunc)(HWND, int, void *) = handler->signalfunction;
                int column_num = DW_POINTER_TO_INT(event);

                return clickcolumnfunc(handler->window, column_num, handler->data);
            }
        }
    }
    return -1;
}

/* Sub function to handle redraws */
int _event_handler(id object, NSEvent *event, int message)
{
    int ret = _event_handler1(object, event, message);
    if(ret != -1)
        _dw_redraw(nil, FALSE);
    return ret;
}

/* Subclass for the Timer type */
@interface DWTimerHandler : NSObject { }
-(void)runTimer:(id)sender;
@end

@implementation DWTimerHandler
-(void)runTimer:(id)sender { _event_handler(sender, nil, 0); }
@end

NSApplication *DWApp;
NSMenu *DWMainMenu;
NSFont *DWDefaultFont;
DWTimerHandler *DWHandler;
#if !defined(GARBAGE_COLLECT)
NSAutoreleasePool *pool;
#endif
HWND _DWLastDrawable;
HMTX DWRunMutex;
HMTX DWThreadMutex;
HMTX DWThreadMutex2;
DWTID DWThread = (DWTID)-1;
DWTID _dw_mutex_locked = (DWTID)-1;

/* Send fake event to make sure the loop isn't stuck */
void _dw_wakeup_app()
{
#ifdef MAC_OS_X_VERSION_10_12
    [DWApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
#else
    [DWApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
#endif
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:NULL
                                         subtype:0
                                           data1:0
                                           data2:0]
             atStart:NO];
}

/* Used for doing bitblts from the main thread */
typedef struct _bitbltinfo
{
    id src;
    id dest;
    int xdest;
    int ydest;
    int width;
    int height;
    int xsrc;
    int ysrc;
    int srcwidth;
    int srcheight;
} DWBitBlt;

/* Subclass for a test object type */
@interface DWObject : NSObject {}
-(void)uselessThread:(id)sender;
-(void)synchronizeThread:(id)param;
-(void)menuHandler:(id)param;
-(void)doBitBlt:(id)param;
-(void)doFlush:(id)param;
@end

@interface DWMenuItem : NSMenuItem
{
    int check;
}
-(void)setCheck:(int)input;
-(int)check;
-(void)dealloc;
@end

@implementation DWObject
-(void)uselessThread:(id)sender { /* Thread only to initialize threading */ }
-(void)synchronizeThread:(id)param
{
    pthread_mutex_unlock(DWRunMutex);
    pthread_mutex_lock(DWThreadMutex2);
    pthread_mutex_unlock(DWThreadMutex2);
    pthread_mutex_lock(DWRunMutex);
}
-(void)menuHandler:(id)param
{
    DWMenuItem *item = param;
    if([item check])
    {
        if([item state] == NSOnState)
            [item setState:NSOffState];
        else
            [item setState:NSOnState];
    }
    _event_handler(param, nil, 8);
}
-(void)doBitBlt:(id)param
{
    NSValue *bi = (NSValue *)param;
    DWBitBlt *bltinfo = (DWBitBlt *)[bi pointerValue];
    id bltdest = bltinfo->dest;
    id bltsrc = bltinfo->src;

    if([bltdest isMemberOfClass:[NSBitmapImageRep class]])
    {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext
                                              graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:bltdest] graphicsPort] flipped:YES]];
        [[[NSDictionary alloc] initWithObjectsAndKeys:bltdest, NSGraphicsContextDestinationAttributeName, nil] autorelease];
    }
    else
    {
        if([bltdest lockFocusIfCanDraw] == NO)
        {
            free(bltinfo);
            return;
        }
        _DWLastDrawable = bltinfo->dest;
    }
    if([bltsrc isMemberOfClass:[NSBitmapImageRep class]])
    {
        NSBitmapImageRep *rep = bltsrc;
        NSImage *image = [NSImage alloc];
        SEL siwc = NSSelectorFromString(@"initWithCGImage");
#ifdef MAC_OS_X_VERSION_10_12
        NSCompositingOperation op = NSCompositingOperationSourceOver;
#else
        NSCompositingOperation op = NSCompositeSourceOver;
#endif

        if([image respondsToSelector:siwc])
        {
            IMP iiwc = [image methodForSelector:siwc];
            image = iiwc(image, siwc, [rep CGImage], NSZeroSize);
        }
        else
        {
            image = [image initWithSize:[rep size]];
            [image addRepresentation:rep];
        }
        if(bltinfo->srcwidth != -1)
        {
            [image drawInRect:NSMakeRect(bltinfo->xdest, bltinfo->ydest, bltinfo->width, bltinfo->height)
                     fromRect:NSMakeRect(bltinfo->xsrc, bltinfo->ysrc, bltinfo->srcwidth, bltinfo->srcheight)
                     operation:op fraction:1.0];
        }
        else
        {
            [image drawAtPoint:NSMakePoint(bltinfo->xdest, bltinfo->ydest)
                      fromRect:NSMakeRect(bltinfo->xsrc, bltinfo->ysrc, bltinfo->width, bltinfo->height)
                     operation:op fraction:1.0];
        }
        [bltsrc release];
        [image release];
    }
    if([bltdest isMemberOfClass:[NSBitmapImageRep class]])
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [bltdest unlockFocus];
    }
    free(bltinfo);
}
-(void)doFlush:(id)param
{
    if(_DWLastDrawable)
    {
        id object = _DWLastDrawable;
        NSWindow *window = [object window];
        [window flushWindow];
    }
}
-(void)doWindowFunc:(id)param
{
    NSValue *v = (NSValue *)param;
    void **params = (void **)[v pointerValue];
    void (* windowfunc)(void *);

    if(params)
    {
        windowfunc = params[0];
        if(windowfunc)
        {
            windowfunc(params[1]);
        }
    }
}
@end

DWObject *DWObj;

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
    Box *box;
    void *userdata;
    NSColor *bgcolor;
}
-(id)init;
-(void)dealloc;
-(Box *)box;
-(id)contentView;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)drawRect:(NSRect)rect;
-(BOOL)isFlipped;
-(void)mouseDown:(NSEvent *)theEvent;
-(void)mouseUp:(NSEvent *)theEvent;
-(NSMenu *)menuForEvent:(NSEvent *)theEvent;
-(void)rightMouseUp:(NSEvent *)theEvent;
-(void)otherMouseDown:(NSEvent *)theEvent;
-(void)otherMouseUp:(NSEvent *)theEvent;
-(void)keyDown:(NSEvent *)theEvent;
-(void)setColor:(unsigned long)input;
@end

@implementation DWBox
-(id)init
{
    self = [super init];

    if (self)
    {
        box = calloc(1, sizeof(Box));
        box->type = DW_VERT;
        box->vsize = box->hsize = SIZEEXPAND;
        box->width = box->height = 1;
    }
    return self;
}
-(void)dealloc
{
    UserData *root = userdata;
    if(box->items)
        free(box->items);
    free(box);
    _remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(Box *)box { return box; }
-(id)contentView { return self; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)drawRect:(NSRect)rect
{
    if(bgcolor)
    {
        [bgcolor set];
        NSRectFill([self bounds]);
    }
}
-(BOOL)isFlipped { return YES; }
-(void)mouseDown:(NSEvent *)theEvent { _event_handler(self, (void *)1, 3); }
-(void)mouseUp:(NSEvent *)theEvent { _event_handler(self, (void *)1, 4); }
-(NSMenu *)menuForEvent:(NSEvent *)theEvent { _event_handler(self, (void *)2, 3); return nil; }
-(void)rightMouseUp:(NSEvent *)theEvent { _event_handler(self, (void *)2, 4); }
-(void)otherMouseDown:(NSEvent *)theEvent { _event_handler(self, (void *)3, 3); }
-(void)otherMouseUp:(NSEvent *)theEvent { _event_handler(self, (void *)3, 4); }
-(void)keyDown:(NSEvent *)theEvent { _event_handler(self, theEvent, 2); }
-(void)setColor:(unsigned long)input
{
    id orig = bgcolor;

    if(input == _colors[DW_CLR_DEFAULT])
    {
        bgcolor = nil;
    }
    else
    {
        bgcolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(input)/255.0 green: DW_GREEN_VALUE(input)/255.0 blue: DW_BLUE_VALUE(input)/255.0 alpha: 1] retain];
        [bgcolor set];
        NSRectFill([self bounds]);
    }
    [self setNeedsDisplay:YES];
    [orig release];
}
@end

/* Subclass for a group box type */
@interface DWGroupBox : NSBox
{
    void *userdata;
    NSColor *bgcolor;
    NSSize borderSize;
}
-(Box *)box;
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWGroupBox
-(Box *)box { return [[self contentView] box]; }
-(void *)userdata { return userdata; }
-(NSSize)borderSize { return borderSize; }
-(NSSize)initBorder
{
    NSSize frameSize = [self frame].size;

    if(frameSize.height < 20 || frameSize.width < 20)
    {
        frameSize.width = frameSize.height = 100;
        [self setFrameSize:frameSize];
    }
    NSSize contentSize = [[self contentView] frame].size;
    NSSize titleSize = [self titleRect].size;

    borderSize.width = 100-contentSize.width;
    borderSize.height = (100-contentSize.height)-titleSize.height;
    return borderSize;
}
-(void)setUserdata:(void *)input { userdata = input; }
@end

@interface DWWindow : NSWindow
{
    int redraw;
    int shown;
}
-(void)sendEvent:(NSEvent *)theEvent;
-(void)keyDown:(NSEvent *)theEvent;
-(void)mouseDragged:(NSEvent *)theEvent;
-(int)redraw;
-(void)setRedraw:(int)val;
-(int)shown;
-(void)setShown:(int)val;
@end

@implementation DWWindow
-(void)sendEvent:(NSEvent *)theEvent
{
   int rcode = -1;
#ifdef MAC_OS_X_VERSION_10_12
   if([theEvent type] == NSEventTypeKeyDown)
#else
   if([theEvent type] == NSKeyDown)
#endif
   {
      rcode = _event_handler(self, theEvent, 2);
   }
   if ( rcode != TRUE )
      [super sendEvent:theEvent];
}
-(void)keyDown:(NSEvent *)theEvent { }
-(void)mouseDragged:(NSEvent *)theEvent { _event_handler(self, theEvent, 5); }
-(int)redraw { return redraw; }
-(void)setRedraw:(int)val { redraw = val; }
-(int)shown { return shown; }
-(void)setShown:(int)val { shown = val; }
-(void)dealloc { dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a render area type */
@interface DWRender : NSControl
{
    void *userdata;
    NSFont *font;
    NSSize size;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFont:(NSFont *)input;
-(NSFont *)font;
-(void)setSize:(NSSize)input;
-(NSSize)size;
-(void)mouseDown:(NSEvent *)theEvent;
-(void)mouseUp:(NSEvent *)theEvent;
-(NSMenu *)menuForEvent:(NSEvent *)theEvent;
-(void)rightMouseUp:(NSEvent *)theEvent;
-(void)otherMouseDown:(NSEvent *)theEvent;
-(void)otherMouseUp:(NSEvent *)theEvent;
-(void)drawRect:(NSRect)rect;
-(void)keyDown:(NSEvent *)theEvent;
-(BOOL)isFlipped;
@end

@implementation DWRender
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFont:(NSFont *)input { [font release]; font = input; [font retain]; }
-(NSFont *)font { return font; }
-(void)setSize:(NSSize)input { size = input; }
-(NSSize)size { return size; }
-(void)mouseDown:(NSEvent *)theEvent
{
#ifdef MAC_OS_X_VERSION_10_12
    if(![theEvent isMemberOfClass:[NSEvent class]] || !([theEvent modifierFlags] & NSEventModifierFlagControl))
#else
    if(![theEvent isMemberOfClass:[NSEvent class]] || !([theEvent modifierFlags] & NSControlKeyMask))
#endif
        _event_handler(self, theEvent, 3);
}
-(void)mouseUp:(NSEvent *)theEvent { _event_handler(self, theEvent, 4); }
-(NSMenu *)menuForEvent:(NSEvent *)theEvent { _event_handler(self, theEvent, 3); return nil; }
-(void)rightMouseUp:(NSEvent *)theEvent { _event_handler(self, theEvent, 4); }
-(void)otherMouseDown:(NSEvent *)theEvent { _event_handler(self, theEvent, 3); }
-(void)otherMouseUp:(NSEvent *)theEvent { _event_handler(self, theEvent, 4); }
-(void)mouseDragged:(NSEvent *)theEvent { _event_handler(self, theEvent, 5); }
-(void)drawRect:(NSRect)rect { _event_handler(self, nil, 7); }
-(void)keyDown:(NSEvent *)theEvent { _event_handler(self, theEvent, 2); }
-(BOOL)isFlipped { return YES; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); [font release]; dw_signal_disconnect_by_window(self); [super dealloc]; }
-(BOOL)acceptsFirstResponder { return YES; }
@end

/* Subclass for the application class */
@interface DWAppDel : NSObject
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSApplicationDelegate>
#endif
{
}
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation DWAppDel
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if(_event_handler(sender, nil, 6) > 0)
        return NSTerminateCancel;
    return NSTerminateNow;
}
@end

/* Subclass for a top-level window */
@interface DWView : DWBox
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSWindowDelegate>
#endif
{
    NSMenu *windowmenu;
    NSSize oldsize;
}
-(BOOL)windowShouldClose:(id)sender;
-(void)setMenu:(NSMenu *)input;
-(void)windowDidBecomeMain:(id)sender;
-(void)menuHandler:(id)sender;
-(void)mouseDragged:(NSEvent *)theEvent;
@end

@implementation DWView
-(BOOL)windowShouldClose:(id)sender
{
    if(_event_handler(sender, nil, 6) > 0)
        return NO;
    return YES;
}
-(void)viewDidMoveToWindow
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:[self window]];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeMain:) name:NSWindowDidBecomeMainNotification object:[self window]];
}
-(void)dealloc
{
    if(windowmenu)
    {
        [windowmenu release];
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(void)windowResized:(NSNotification *)notification;
{
    NSSize size = [self frame].size;

    if(oldsize.width != size.width || oldsize.height != size.height)
    {
        _do_resize(box, size.width, size.height);
        _event_handler([self window], nil, 1);
        oldsize.width = size.width;
        oldsize.height = size.height;
        _handle_resize_events(box);
    }
}
-(void)showWindow
{
    NSSize size = [self frame].size;

    if(oldsize.width == size.width && oldsize.height == size.height)
    {
        _do_resize(box, size.width, size.height);
        _handle_resize_events(box);
    }

}
-(void)windowDidBecomeMain:(id)sender
{
    if(windowmenu)
    {
        [DWApp setMainMenu:windowmenu];
    }
    else
    {
        [DWApp setMainMenu:DWMainMenu];
    }
    _event_handler([self window], nil, 13);
}
-(void)setMenu:(NSMenu *)input { windowmenu = input; [windowmenu retain]; }
-(void)menuHandler:(id)sender
{
    id menu = [sender menu];

    /* Find the highest menu for this item */
    while([menu supermenu])
    {
        menu = [menu supermenu];
    }

    /* Only perform the delay if this item is a child of the main menu */
    if([DWApp mainMenu] == menu)
        [DWObj performSelector:@selector(menuHandler:) withObject:sender afterDelay:0];
    else
        [DWObj menuHandler:sender];
    _dw_wakeup_app();
}
-(void)mouseDragged:(NSEvent *)theEvent { _event_handler(self, theEvent, 5); }
-(void)mouseMoved:(NSEvent *)theEvent
{
    id hit = [self hitTest:[theEvent locationInWindow]];

    if([hit isMemberOfClass:[DWRender class]])
    {
        _event_handler(hit, theEvent, 5);
    }
}
@end

/* Subclass for a button type */
@interface DWButton : NSButton
{
    void *userdata;
    NSButtonType buttonType;
    DWBox *parent;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)buttonClicked:(id)sender;
-(void)setButtonType:(NSButtonType)input;
-(NSButtonType)buttonType;
-(void)setParent:(DWBox *)input;
-(DWBox *)parent;
-(NSColor *)textColor;
-(void)setTextColor:(NSColor *)textColor;
@end

@implementation DWButton
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)buttonClicked:(id)sender
{
    _event_handler(self, nil, 8);
    if([self buttonType] == NSRadioButton)
    {
        DWBox *viewbox = [self parent];
        Box *thisbox = [viewbox box];
        int z;

        for(z=0;z<thisbox->count;z++)
        {
            if(thisbox->items[z].type != TYPEBOX)
            {
                id object = thisbox->items[z].hwnd;

                if([object isMemberOfClass:[DWButton class]])
                {
                    DWButton *button = object;

                    if(button != self && [button buttonType] == NSRadioButton)
                    {
                        [button setState:NSOffState];
                    }
                }
            }
        }
    }
}
-(void)setButtonType:(NSButtonType)input { buttonType = input; [super setButtonType:input]; }
-(NSButtonType)buttonType { return buttonType; }
-(void)setParent:(DWBox *)input { parent = input; }
-(DWBox *)parent { return parent; }
-(NSColor *)textColor
{
    NSAttributedString *attrTitle = [self attributedTitle];
    NSUInteger len = [attrTitle length];
    NSRange range = NSMakeRange(0, MIN(len, 1));
    NSDictionary *attrs = [attrTitle fontAttributesInRange:range];
    NSColor *textColor = [NSColor controlTextColor];
    if (attrs) {
        textColor = [attrs objectForKey:NSForegroundColorAttributeName];
    }
    return textColor;
}
-(void)setTextColor:(NSColor *)textColor
{
    NSMutableAttributedString *attrTitle = [[NSMutableAttributedString alloc]
                                            initWithAttributedString:[self attributedTitle]];
    NSUInteger len = [attrTitle length];
    NSRange range = NSMakeRange(0, len);
    [attrTitle addAttribute:NSForegroundColorAttributeName
                      value:textColor
                      range:range];
    [attrTitle fixAttributesInRange:range];
    [self setAttributedTitle:attrTitle];
    [attrTitle release];
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(vk == VK_RETURN || vk == VK_SPACE)
    {
        if(buttonType == NSSwitchButton)
            [self setState:([self state] ? NSOffState : NSOnState)];
        else if(buttonType == NSRadioButton)
            [self setState:NSOnState];
        [self buttonClicked:self];
    }
    else
    {
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
        [super keyDown:theEvent];
    }
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
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
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a menu item type */
@implementation DWMenuItem
-(void)setCheck:(int)input { check = input; }
-(int)check { return check; }
-(void)dealloc { dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a scrollbox type */
@interface DWScrollBox : NSScrollView
{
    void *userdata;
    id box;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setBox:(void *)input;
-(id)box;
@end

@implementation DWScrollBox
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setBox:(void *)input { box = input; }
-(id)box { return box; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a textfield that supports vertical centering */
@interface DWTextFieldCell : NSTextFieldCell
{
    BOOL vcenter;
}
-(NSRect)drawingRectForBounds:(NSRect)theRect;
-(void)setVCenter:(BOOL)input;
@end

@implementation DWTextFieldCell
-(NSRect)drawingRectForBounds:(NSRect)theRect
{
    /* Get the parent's idea of where we should draw */
    NSRect newRect = [super drawingRectForBounds:theRect];

    /* If we are being vertically centered */
    if(vcenter)
    {
        /* Get our ideal size for current text */
        NSSize textSize = [self cellSizeForBounds:theRect];

        /* Center that in the proposed rect */
        float heightDelta = newRect.size.height - textSize.height;	
        if (heightDelta > 0)
        {
            newRect.size.height -= heightDelta;
            newRect.origin.y += (heightDelta / 2);
        }
    }
	
    return newRect;
}
-(void)setVCenter:(BOOL)input { vcenter = input; }
@end

@interface DWEntryFieldFormatter : NSFormatter
{
    int maxLength;
}
- (void)setMaximumLength:(int)len;
- (int)maximumLength;
@end

/* This formatter subclass will allow us to limit
 * the text length in an entryfield.
 */
@implementation DWEntryFieldFormatter
-(id)init
{
    self = [super init];
    maxLength = INT_MAX;
    return self;
}
-(void)setMaximumLength:(int)len { maxLength = len; }
-(int)maximumLength { return maxLength; }
-(NSString *)stringForObjectValue:(id)object { return (NSString *)object; }
-(BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error { *object = string; return YES; }
-(BOOL)isPartialStringValid:(NSString **)partialStringPtr
       proposedSelectedRange:(NSRangePointer)proposedSelRangePtr
              originalString:(NSString *)origString
       originalSelectedRange:(NSRange)origSelRange
            errorDescription:(NSString **)error
{
    if([*partialStringPtr length] > maxLength)
    {
        return NO;
    }
    return YES;
}
-(NSAttributedString *)attributedStringForObjectValue:(id)anObject withDefaultAttributes:(NSDictionary *)attributes { return nil; }
@end

/* Subclass for a entryfield type */
@interface DWEntryField : NSTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setClickDefault:(id)input;
@end

@implementation DWEntryField
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(clickDefault && vk == VK_RETURN)
    {
        if([clickDefault isKindOfClass:[NSButton class]])
            [clickDefault buttonClicked:self];
        else
            [[self window] makeFirstResponder:clickDefault];
    } else
    {
        [super keyUp:theEvent];
    }
}
-(BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
#ifdef MAC_OS_X_VERSION_10_12
    if(([theEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask) == NSEventModifierFlagCommand)
#else
    if(([theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask) == NSCommandKeyMask)
#endif
    {
        if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"x"])
            return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"c"])
            return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"v"])
            return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"a"])
            return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
    }
    return [super performKeyEquivalent:theEvent];
}
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a text and status text type */
@interface DWText : NSTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWText
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); [super dealloc]; }
@end


/* Subclass for a entryfield password type */
@interface DWEntryFieldPassword : NSSecureTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setClickDefault:(id)input;
@end

@implementation DWEntryFieldPassword
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        if([clickDefault isKindOfClass:[NSButton class]])
            [clickDefault buttonClicked:self];
        else
            [[self window] makeFirstResponder:clickDefault];
    }
    else
    {
        [super keyUp:theEvent];
    }
}
-(BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
#ifdef MAC_OS_X_VERSION_10_12
    if(([theEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask) == NSEventModifierFlagControl)
#else
    if(([theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask) == NSCommandKeyMask)
#endif
    {
        if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"x"])
            return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"c"])
            return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"v"])
            return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"a"])
            return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
    }
    return [super performKeyEquivalent:theEvent];
}
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a Notebook control type */
@interface DWNotebook : NSTabView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTabViewDelegate>
#endif
{
    void *userdata;
    int pageid;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(int)pageid;
-(void)setPageid:(int)input;
-(void)tabView:(NSTabView *)notebook didSelectTabViewItem:(NSTabViewItem *)notepage;
@end

/* Subclass for a Notebook page type */
@interface DWNotebookPage : NSTabViewItem
{
    void *userdata;
    int pageid;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(int)pageid;
-(void)setPageid:(int)input;
@end

@implementation DWNotebook
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)tabView:(NSTabView *)notebook didSelectTabViewItem:(NSTabViewItem *)notepage
{
    id object = [notepage view];
    DWNotebookPage *page = (DWNotebookPage *)notepage;

    if([object isMemberOfClass:[DWBox class]])
    {
        DWBox *view = object;
        Box *box = [view box];
        NSSize size = [view frame].size;
        _do_resize(box, size.width, size.height);
        _handle_resize_events(box);
    }
    _event_handler(self, DW_INT_TO_POINTER([page pageid]), 15);
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    else if(vk == NSLeftArrowFunctionKey)
    {
        NSArray *pages = [self tabViewItems];
        DWNotebookPage *page = (DWNotebookPage *)[self selectedTabViewItem];
        NSUInteger index = [pages indexOfObject:page];

        if(index != NSNotFound)
        {
            if(index > 0)
               [self selectTabViewItem:[pages objectAtIndex:(index-1)]];
            else
               [self selectTabViewItem:[pages objectAtIndex:0]];

        }
    }
    else if(vk == NSRightArrowFunctionKey)
    {
        NSArray *pages = [self tabViewItems];
        DWNotebookPage *page = (DWNotebookPage *)[self selectedTabViewItem];
        NSUInteger index = [pages indexOfObject:page];
        NSUInteger count = [pages count];

        if(index != NSNotFound)
        {
            if(index + 1 < count)
                [self selectTabViewItem:[pages objectAtIndex:(index+1)]];
            else
                [self selectTabViewItem:[pages objectAtIndex:(count-1)]];

        }
    }
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

@implementation DWNotebookPage
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a color chooser type */
@interface DWColorChoose : NSColorPanel
{
    DWDialog *dialog;
    NSColor *pickedcolor;
}
-(void)changeColor:(id)sender;
-(void)setDialog:(DWDialog *)input;
-(DWDialog *)dialog;
@end

@implementation DWColorChoose
-(void)changeColor:(id)sender
{
    if(!dialog)
        [self close];
    else
        pickedcolor = [self color];
}
-(BOOL)windowShouldClose:(id)window
{
    if(dialog)
    {
        DWDialog *d = dialog;
        dialog = nil;
        dw_dialog_dismiss(d, pickedcolor);
    }
    [self close];
    return NO;
}
-(void)setDialog:(DWDialog *)input { dialog = input; }
-(DWDialog *)dialog { return dialog; }
@end

/* Subclass for a font chooser type */
@interface DWFontChoose : NSFontPanel
{
    DWDialog *dialog;
    NSFontManager *fontManager;
}
-(void)setDialog:(DWDialog *)input;
-(void)setFontManager:(NSFontManager *)input;
-(DWDialog *)dialog;
@end

@implementation DWFontChoose
-(BOOL)windowShouldClose:(id)window
{
    DWDialog *d = dialog; dialog = nil;
    NSFont *pickedfont = [fontManager selectedFont];
    dw_dialog_dismiss(d, pickedfont);
    [window orderOut:nil];
    return NO;
}
-(void)setDialog:(DWDialog *)input { dialog = input; }
-(void)setFontManager:(NSFontManager *)input { fontManager = input; }
-(DWDialog *)dialog { return dialog; }
@end

/* Subclass for a splitbar type */
@interface DWSplitBar : NSSplitView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSSplitViewDelegate>
#endif
{
    void *userdata;
    float percent;
}
-(void)splitViewDidResizeSubviews:(NSNotification *)aNotification;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(float)percent;
-(void)setPercent:(float)input;
@end

@implementation DWSplitBar
-(void)splitViewDidResizeSubviews:(NSNotification *)aNotification
{
    NSArray *views = [self subviews];
    id object;

    for(object in views)
    {
        if([object isMemberOfClass:[DWBox class]])
        {
            DWBox *view = object;
            Box *box = [view box];
            NSSize size = [view frame].size;
            _do_resize(box, size.width, size.height);
            _handle_resize_events(box);
        }
    }
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(float)percent { return percent; }
-(void)setPercent:(float)input { percent = input; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a slider type */
@interface DWSlider : NSSlider
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)sliderChanged:(id)sender;
@end

@implementation DWSlider
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)sliderChanged:(id)sender { _event_handler(self, (void *)[self integerValue], 14); }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a slider type */
@interface DWScrollbar : NSScroller
{
    void *userdata;
    double range;
    double visible;
    int vertical;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(float)range;
-(float)visible;
-(int)vertical;
-(void)setVertical:(int)value;
-(void)setRange:(double)input1 andVisible:(double)input2;
-(void)scrollerChanged:(id)sender;
@end

@implementation DWScrollbar
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(float)range { return range; }
-(float)visible { return visible; }
-(int)vertical { return vertical; }
-(void)setVertical:(int)value { vertical = value; }
-(void)setRange:(double)input1 andVisible:(double)input2 { range = input1; visible = input2; }
-(void)scrollerChanged:(id)sender
{
    double max = (range - visible);
    double result = ([self doubleValue] * max);
    double newpos = result;

    switch ([sender hitPart])
    {

        case NSScrollerDecrementLine:
            if(newpos > 0)
            {
                newpos--;
            }
            break;

        case NSScrollerIncrementLine:
            if(newpos < max)
            {
                newpos++;
            }
            break;

        case NSScrollerDecrementPage:
            newpos -= visible;
            if(newpos < 0)
            {
                newpos = 0;
            }
            break;

        case NSScrollerIncrementPage:
            newpos += visible;
            if(newpos > max)
            {
                newpos = max;
            }
            break;

        default:
            ; /* do nothing */
    }
    int newposi = (int)newpos;
    newpos = (newpos - (double)newposi) > 0.5 ? (double)(newposi + 1) : (double)newposi;
    if(newpos != result)
    {
        [self setDoubleValue:(newpos/max)];
    }
    _event_handler(self, DW_INT_TO_POINTER((int)newpos), 14);
}
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a MLE type */
@interface DWMLE : NSTextView
{
    void *userdata;
    id scrollview;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(id)scrollview;
-(void)setScrollview:(id)input;
@end

@implementation DWMLE
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(id)scrollview { return scrollview; }
-(void)setScrollview:(id)input { scrollview = input; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass NSTextFieldCell for displaying image and text */
@interface DWImageAndTextCell : NSTextFieldCell
{
@private
    NSImage	*image;
}
-(void)setImage:(NSImage *)anImage;
-(NSImage *)image;
-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
-(NSSize)cellSize;
@end

@implementation DWImageAndTextCell
-(void)dealloc
{
    [image release];
    image = nil;
    [super dealloc];
}
-copyWithZone:(NSZone *)zone
{
    DWImageAndTextCell *cell = (DWImageAndTextCell *)[super copyWithZone:zone];
    cell->image = [image retain];
    return cell;
}
-(void)setImage:(NSImage *)anImage
{
    if(anImage != image)
    {
        [image release];
        image = [anImage retain];
    }
}
-(NSImage *)image
{
    return [[image retain] autorelease];
}
-(NSRect)imageFrameForCellFrame:(NSRect)cellFrame
{
    if(image != nil)
    {
        NSRect imageFrame;
        imageFrame.size = [image size];
        imageFrame.origin = cellFrame.origin;
        imageFrame.origin.x += 3;
        imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);
        return imageFrame;
    }
    else
        return NSZeroRect;
}
-(void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [image size].width, NSMinXEdge);
    [super editWithFrame: textFrame inView: controlView editor:textObj delegate:anObject event: theEvent];
}
-(void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [image size].width, NSMinXEdge);
    [super selectWithFrame: textFrame inView: controlView editor:textObj delegate:anObject start:selStart length:selLength];
}
-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    if(image != nil)
    {
        NSSize	imageSize;
        NSRect	imageFrame;
        SEL sdir = NSSelectorFromString(@"drawInRect:fromRect:operation:fraction:respectFlipped:hints:");
#ifdef MAC_OS_X_VERSION_10_12
        NSCompositingOperation op = NSCompositingOperationSourceOver;
#else
        NSCompositingOperation op = NSCompositeSourceOver;
#endif

        imageSize = [image size];
        NSDivideRect(cellFrame, &imageFrame, &cellFrame, 3 + imageSize.width, NSMinXEdge);
        if ([self drawsBackground])
        {
            [[self backgroundColor] set];
            NSRectFill(imageFrame);
        }
        imageFrame.origin.x += 3;
        imageFrame.size = imageSize;

        /* New method for 10.6 and later */
        if([image respondsToSelector:sdir])
        {
            IMP idir = [image methodForSelector:sdir];

            imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);

            idir(image, sdir, imageFrame, NSZeroRect, op, 1.0, YES, nil);
        }
        else
        {
            /* Old method for 10.5 */
            SEL sctp = NSSelectorFromString(@"compositeToPoint:operation:");

            if ([controlView isFlipped])
                imageFrame.origin.y += ceil((cellFrame.size.height + imageFrame.size.height) / 2);
            else
                imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);

            if([image respondsToSelector:sctp])
            {
                IMP ictp = [image methodForSelector:sctp];

                ictp(image, sctp, imageFrame.origin, op);
            }
        }
    }
    [super drawWithFrame:cellFrame inView:controlView];
}
-(NSSize)cellSize
{
    NSSize cellSize = [super cellSize];
    cellSize.width += (image ? [image size].width : 0) + 3;
    return cellSize;
}
@end

/* Subclass for a Container/List type */
@interface DWContainer : NSTableView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTableViewDataSource,NSTableViewDelegate>
#endif
{
    void *userdata;
    NSMutableArray *tvcols;
    NSMutableArray *data;
    NSMutableArray *types;
    NSPointerArray *titles;
    NSPointerArray *rowdatas;
    NSColor *fgcolor, *oddcolor, *evencolor;
    int lastAddPoint, lastQueryPoint;
    id scrollview;
    int filesystem;
}
-(NSInteger)numberOfRowsInTableView:(NSTableView *)aTable;
-(id)tableView:(NSTableView *)aTable objectValueForTableColumn:(NSTableColumn *)aCol row:(NSInteger)aRow;
-(BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFilesystem:(int)input;
-(int)filesystem;
-(id)scrollview;
-(void)setScrollview:(id)input;
-(void)addColumn:(NSTableColumn *)input andType:(int)type;
-(NSTableColumn *)getColumn:(int)col;
-(int)addRow:(NSArray *)input;
-(int)addRows:(int)number;
-(void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
-(void)editCell:(id)input at:(int)row and:(int)col;
-(void)removeRow:(int)row;
-(void)setRow:(int)row title:(void *)input;
-(void *)getRowTitle:(int)row;
-(id)getRow:(int)row and:(int)col;
-(int)cellType:(int)col;
-(int)lastAddPoint;
-(int)lastQueryPoint;
-(void)setLastQueryPoint:(int)input;
-(void)clear;
-(void)setup;
-(void)optimize;
-(NSSize)getsize;
-(void)setForegroundColor:(NSColor *)input;
-(void)doubleClicked:(id)sender;
-(void)keyUp:(NSEvent *)theEvent;
-(void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn;
-(void)selectionChanged:(id)sender;
-(NSMenu *)menuForEvent:(NSEvent *)event;
@end

@implementation DWContainer
-(NSInteger)numberOfRowsInTableView:(NSTableView *)aTable
{
    if(tvcols && data)
    {
        int cols = (int)[tvcols count];
        int total = (int)[data count];
        if(cols && total)
        {
            return total / cols;
        }
    }
    return 0;
}
-(id)tableView:(NSTableView *)aTable objectValueForTableColumn:(NSTableColumn *)aCol row:(NSInteger)aRow
{
    if(tvcols && data)
    {
        int z, col = -1;
        int count = (int)[tvcols count];

        for(z=0;z<count;z++)
        {
            if([tvcols objectAtIndex:z] == aCol)
            {
                col = z;
                break;
            }
        }
        if(col != -1)
        {
            int index = (int)(aRow * count) + col;
            if(index < [data count])
            {
                id this = [data objectAtIndex:index];
                return ([this isKindOfClass:[NSNull class]]) ? nil : this;
            }
        }
    }
    return nil;
}
-(BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex { return NO; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFilesystem:(int)input { filesystem = input; }
-(int)filesystem { return filesystem; }
-(id)scrollview { return scrollview; }
-(void)setScrollview:(id)input { scrollview = input; }
-(void)addColumn:(NSTableColumn *)input andType:(int)type { if(tvcols) { [tvcols addObject:input]; [types addObject:[NSNumber numberWithInt:type]]; } }
-(NSTableColumn *)getColumn:(int)col { if(tvcols) { return [tvcols objectAtIndex:col]; } return nil; }
-(void)setRowBgOdd:(unsigned long)oddcol andEven:(unsigned long)evencol
{
    NSColor *oldodd = oddcolor;
    NSColor *oldeven = evencolor;
    unsigned long _odd = _get_color(oddcol);
    unsigned long _even = _get_color(evencol);

    /* Get the NSColor for non-default colors */
    if(oddcol != DW_RGB_TRANSPARENT)
        oddcolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(_odd)/255.0 green: DW_GREEN_VALUE(_odd)/255.0 blue: DW_BLUE_VALUE(_odd)/255.0 alpha: 1] retain];
    else
        oddcolor = NULL;
    if(evencol != DW_RGB_TRANSPARENT)
        evencolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(_even)/255.0 green: DW_GREEN_VALUE(_even)/255.0 blue: DW_BLUE_VALUE(_even)/255.0 alpha: 1] retain];
    else
        evencolor = NULL;
    [oldodd release];
    [oldeven release];
}
-(int)insertRow:(NSArray *)input at:(int)index
{
    if(data)
    {
        unsigned long start = [tvcols count] * index;
        NSIndexSet *set = [[NSIndexSet alloc] initWithIndexesInRange:NSMakeRange(start, start + [tvcols count])];
        if(index < lastAddPoint)
        {
            lastAddPoint++;
        }
        [data insertObjects:input atIndexes:set];
        [titles insertPointer:NULL atIndex:index];
        [rowdatas insertPointer:NULL atIndex:index];
        [set release];
        return (int)[titles count];
    }
    return 0;
}
-(int)addRow:(NSArray *)input
{
    if(data)
    {
        lastAddPoint = (int)[titles count];
        [data addObjectsFromArray:input];
        [titles addPointer:NULL];
        [rowdatas addPointer:NULL];
        return (int)[titles count];
    }
    return 0;
}
-(int)addRows:(int)number
{
    if(tvcols)
    {
        int count = (int)(number * [tvcols count]);
        int z;

        lastAddPoint = (int)[titles count];

        for(z=0;z<count;z++)
        {
            [data addObject:[NSNull null]];
        }
        for(z=0;z<number;z++)
        {
            [titles addPointer:NULL];
            [rowdatas addPointer:NULL];
        }
        return (int)[titles count];
    }
    return 0;
}
-(void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    DWImageAndTextCell *bcell = cell;

    /* Handle drawing image and text if necessary */
    if([cell isMemberOfClass:[DWImageAndTextCell class]])
    {
        int index = (int)(row * [tvcols count]);
        DWImageAndTextCell *browsercell = [data objectAtIndex:index];
        NSImage *img = [browsercell image];
        [bcell setImage:img];
    }
    if([cell isKindOfClass:[NSTextFieldCell class]])
    {
        /* Handle drawing alternating row colors if enabled */
        if ((row % 2) == 0)
        {
            if(evencolor)
            {
                [bcell setDrawsBackground:YES];
                [bcell setBackgroundColor:evencolor];
            }
            else
                [bcell setDrawsBackground:NO];
        }
        else
        {
            if(oddcolor)
            {
                [bcell setDrawsBackground:YES];
                [bcell setBackgroundColor:oddcolor];
            }
            else
                [bcell setDrawsBackground:NO];
        }
    }
}
-(void)editCell:(id)input at:(int)row and:(int)col
{
    if(tvcols)
    {
        int index = (int)(row * [tvcols count]) + col;
        if(index < [data count])
        {
            if(!input)
                input = [NSNull null];
            [data replaceObjectAtIndex:index withObject:input];
        }
    }
}
-(void)removeRow:(int)row
{
    if(tvcols)
    {
        int z, start, end;
        int count = (int)[tvcols count];
        void *oldtitle;

        start = (count * row);
        end = start + count;

        for(z=start;z<end;z++)
        {
            [data removeObjectAtIndex:start];
        }
        oldtitle = [titles pointerAtIndex:row];
        [titles removePointerAtIndex:row];
        [rowdatas removePointerAtIndex:row];
        if(lastAddPoint > 0 && lastAddPoint > row)
        {
            lastAddPoint--;
        }
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRow:(int)row title:(void *)input
{
    if(titles && input)
    {
        void *oldtitle = [titles pointerAtIndex:row];
        void *newtitle = input ? (void *)strdup((char *)input) : NULL;
        [titles replacePointerAtIndex:row withPointer:newtitle];
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRowData:(int)row title:(void *)input { if(rowdatas && input) { [rowdatas replacePointerAtIndex:row withPointer:input]; } }
-(void *)getRowTitle:(int)row { if(titles && row > -1) { return [titles pointerAtIndex:row]; } return NULL; }
-(void *)getRowData:(int)row { if(rowdatas && row > -1) { return [rowdatas pointerAtIndex:row]; } return NULL; }
-(id)getRow:(int)row and:(int)col { if(data) { int index = (int)(row * [tvcols count]) + col; return [data objectAtIndex:index]; } return nil; }
-(int)cellType:(int)col { return [[types objectAtIndex:col] intValue]; }
-(int)lastAddPoint { return lastAddPoint; }
-(int)lastQueryPoint { return lastQueryPoint; }
-(void)setLastQueryPoint:(int)input { lastQueryPoint = input; }
-(void)clear
{
    if(data)
    {
        [data removeAllObjects];
        while([titles count])
        {
            void *oldtitle = [titles pointerAtIndex:0];
            [titles removePointerAtIndex:0];
            [rowdatas removePointerAtIndex:0];
            if(oldtitle)
                free(oldtitle);
        }
    }
    lastAddPoint = 0;
}
-(void)setup
{
    SEL swopa = NSSelectorFromString(@"pointerArrayWithWeakObjects");

    if(![[NSPointerArray class] respondsToSelector:swopa])
        swopa = NSSelectorFromString(@"weakObjectsPointerArray");
    if(![[NSPointerArray class] respondsToSelector:swopa])
        return;

    IMP iwopa = [[NSPointerArray class] methodForSelector:swopa];

    titles = iwopa([NSPointerArray class], swopa);
    [titles retain];
    rowdatas = iwopa([NSPointerArray class], swopa);
    [rowdatas retain];
    tvcols = [[[NSMutableArray alloc] init] retain];
    data = [[[NSMutableArray alloc] init] retain];
    types = [[[NSMutableArray alloc] init] retain];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(selectionChanged:) name:NSTableViewSelectionDidChangeNotification object:self];
}
-(NSSize)getsize
{
    int cwidth = 0, cheight = 0;

    if(tvcols)
    {
        int z;
        int colcount = (int)[tvcols count];
        int rowcount = (int)[self numberOfRowsInTableView:self];

        for(z=0;z<colcount;z++)
        {
            NSTableColumn *column = [tvcols objectAtIndex:z];
            int width = [column width];

            if(rowcount > 0)
            {
                int x;

                for(x=0;x<rowcount;x++)
                {
                    NSCell *cell = [self preparedCellAtColumn:z row:x];
                    int thiswidth = [cell cellSize].width;

                    /* If on the first column... add up the heights */
                    if(z == 0)
                        cheight += [cell cellSize].height;

                    /* Check the image inside the cell to get its width */
                    if([cell isMemberOfClass:[NSImageCell class]])
                    {
                        int index = (int)(x * [tvcols count]) + z;
                        NSImage *image = [data objectAtIndex:index];
                        if([image isMemberOfClass:[NSImage class]])
                        {
                            thiswidth = [image size].width;
                        }
                    }

                    if(thiswidth > width)
                    {
                        width = thiswidth;
                    }
                }
                /* If the image is missing default the optimized width to 16. */
                if(!width && [[types objectAtIndex:z] intValue] & DW_CFA_BITMAPORICON)
                {
                    width = 16;
                }
            }
            if(width)
                cwidth += width;
        }
    }
    cwidth += 16;
    cheight += 16;
    return NSMakeSize(cwidth, cheight);
}
-(void)optimize
{
    if(tvcols)
    {
        int z;
        int colcount = (int)[tvcols count];
        int rowcount = (int)[self numberOfRowsInTableView:self];

        for(z=0;z<colcount;z++)
        {
            NSTableColumn *column = [tvcols objectAtIndex:z];
            if([column resizingMask] != NSTableColumnNoResizing)
            {
                if(rowcount > 0)
                {
                    int x;
                    NSCell *colcell = [column headerCell];
                    int width = [colcell cellSize].width;

                    for(x=0;x<rowcount;x++)
                    {
                        NSCell *cell = [self preparedCellAtColumn:z row:x];
                        int thiswidth = [cell cellSize].width;

                        /* Check the image inside the cell to get its width */
                        if([cell isMemberOfClass:[NSImageCell class]])
                        {
                            int index = (int)(x * [tvcols count]) + z;
                            NSImage *image = [data objectAtIndex:index];
                            if([image isMemberOfClass:[NSImage class]])
                            {
                                thiswidth = [image size].width;
                            }
                        }

                        if(thiswidth > width)
                        {
                            width = thiswidth;
                        }
                    }
                    /* If the image is missing default the optimized width to 16. */
                    if(!width && [[types objectAtIndex:z] intValue] & DW_CFA_BITMAPORICON)
                    {
                        width = 16;
                    }
                    /* Sanity check... don't set the width to 0 */
                    if(width)
                    {
                        [column setWidth:width];
                    }
                }
                else
                {
                    [column sizeToFit];
                }
            }
        }
    }
}
-(void)setForegroundColor:(NSColor *)input
{
    int z, count = (int)[tvcols count];

    fgcolor = input;
    [fgcolor retain];

    for(z=0;z<count;z++)
    {
        NSTableColumn *tableColumn = [tvcols objectAtIndex:z];
        NSTextFieldCell *cell = [tableColumn dataCell];
        [cell setTextColor:fgcolor];
    }
}
-(void)doubleClicked:(id)sender
{
    void *params[2];

    params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
    params[1] = (void *)[self getRowData:(int)[self selectedRow]];

    /* Handler for container class */
    _event_handler(self, (NSEvent *)params, 9);
}
-(void)keyUp:(NSEvent *)theEvent
{
    if([[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        void *params[2];

        params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
        params[1] = (void *)[self getRowData:(int)[self selectedRow]];

        _event_handler(self, (NSEvent *)params, 9);
    }
    [super keyUp:theEvent];
}

-(void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn
{
    NSUInteger index = [tvcols indexOfObject:tableColumn];

    /* Handler for column click class */
    _event_handler(self, (NSEvent *)index, 17);
}
-(void)selectionChanged:(id)sender
{
    void *params[2];

    params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
    params[1] = (void *)[self getRowData:(int)[self selectedRow]];

    /* Handler for container class */
    _event_handler(self, (NSEvent *)params, 12);
    /* Handler for listbox class */
    _event_handler(self, DW_INT_TO_POINTER((int)[self selectedRow]), 11);
}
-(NSMenu *)menuForEvent:(NSEvent *)event
{
    int row;
    NSPoint where = [self convertPoint:[event locationInWindow] fromView:nil];
    row = (int)[self rowAtPoint:where];
    _event_handler(self, (NSEvent *)[self getRowTitle:row], 10);
    return nil;
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Dive into the tree freeing all desired child nodes */
void _free_tree_recurse(NSMutableArray *node, NSMutableArray *item)
{
    if(node && ([node isKindOfClass:[NSArray class]]))
    {
        int count = (int)[node count];
        NSInteger index = -1;
        int z;

        if(item)
            index = [node indexOfObject:item];

        for(z=0;z<count;z++)
        {
            NSMutableArray *pnt = [node objectAtIndex:z];
            NSMutableArray *children = nil;

            if(pnt && [pnt isKindOfClass:[NSArray class]])
            {
                children = (NSMutableArray *)[pnt objectAtIndex:3];
            }

            if(z == index)
            {
                _free_tree_recurse(children, NULL);
                [node removeObjectAtIndex:z];
                count = (int)[node count];
                index = -1;
                z--;
            }
            else if(item == NULL)
            {
                NSString *oldstr = [pnt objectAtIndex:1];
                [oldstr release];
                _free_tree_recurse(children, item);
            }
            else
                _free_tree_recurse(children, item);
        }
    }
    if(!item)
    {
        [node release];
    }
}

/* Subclass for a Tree type */
@interface DWTree : NSOutlineView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSOutlineViewDataSource,NSOutlineViewDelegate>
#endif
{
    void *userdata;
    NSTableColumn *treecol;
    NSMutableArray *data;
    /* Each data item consists of a linked lists of tree item data.
     * NSImage *, NSString *, Item Data *, NSMutableArray * of Children
     */
    id scrollview;
    NSColor *fgcolor;
}
-(id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item;
-(BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item;
-(int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item;
-(id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item;
-(BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item;
-(void)addTree:(NSMutableArray *)item and:(NSMutableArray *)parent after:(NSMutableArray *)after;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)treeSelectionChanged:(id)sender;
-(void)treeItemExpanded:(NSNotification *)notification;
-(NSScrollView *)scrollview;
-(void)setScrollview:(NSScrollView *)input;
-(void)deleteNode:(NSMutableArray *)item;
-(void)setForegroundColor:(NSColor *)input;
-(void)clear;
@end

@implementation DWTree
-(id)init
{
    self = [super init];

    if (self)
    {
        treecol = [[NSTableColumn alloc] init];
        DWImageAndTextCell *browsercell = [[[DWImageAndTextCell alloc] init] autorelease];
        [treecol setDataCell:browsercell];
        [self addTableColumn:treecol];
        [self setOutlineTableColumn:treecol];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(treeSelectionChanged:) name:NSOutlineViewSelectionDidChangeNotification object:self];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(treeItemExpanded:) name:NSOutlineViewItemDidExpandNotification object:self];
    }
    return self;
}
-(id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
    if(item)
    {
        NSMutableArray *array = [item objectAtIndex:3];
        return ([array isKindOfClass:[NSNull class]]) ? nil : [array objectAtIndex:index];
    }
    else
    {
        return [data objectAtIndex:index];
    }
}
-(BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
    return [self outlineView:outlineView numberOfChildrenOfItem:item] != 0;
}
-(int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    if(item)
    {
        if([item isKindOfClass:[NSMutableArray class]])
        {
            NSMutableArray *array = [item objectAtIndex:3];
            return ([array isKindOfClass:[NSNull class]]) ? 0 : (int)[array count];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return data ? (int)[data count] : 0;
    }
}
-(id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if(item)
    {
        if([item isKindOfClass:[NSMutableArray class]])
        {
            NSMutableArray *this = (NSMutableArray *)item;
            return [this objectAtIndex:1];
        }
        else
        {
            return nil;
        }
    }
    return @"List Root";
}
-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if([cell isMemberOfClass:[DWImageAndTextCell class]])
    {
        NSMutableArray *this = (NSMutableArray *)item;
        NSImage *img = [this objectAtIndex:0];
        if([img isKindOfClass:[NSImage class]])
            [(DWImageAndTextCell*)cell setImage:img];
    }
}
-(BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item { return NO; }
-(void)addTree:(NSMutableArray *)item and:(NSMutableArray *)parent after:(NSMutableArray *)after
{
    NSMutableArray *children = data;
    if(parent)
    {
        children = [parent objectAtIndex:3];
        if([children isKindOfClass:[NSNull class]])
        {
            children = [[[NSMutableArray alloc] init] retain];
            [parent replaceObjectAtIndex:3 withObject:children];
        }
    }
    else
    {
        if(!data)
        {
            children = data = [[[NSMutableArray alloc] init] retain];
        }
    }
    if(after)
    {
        NSInteger index = [children indexOfObject:after];
        int count = (int)[children count];
        if(index != NSNotFound && (index+1) < count)
            [children insertObject:item atIndex:(index+1)];
        else
            [children addObject:item];
    }
    else
    {
        [children addObject:item];
    }
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)treeSelectionChanged:(id)sender
{
    /* Handler for tree class */
    id item = [self itemAtRow:[self selectedRow]];

    if(item)
    {
        _event_handler(self, (void *)item, 12);
    }
}
-(void)treeItemExpanded:(NSNotification *)notification
{
    id item = [[notification userInfo ] objectForKey: @"NSObject"];

    if(item)
    {
        _event_handler(self, (void *)item, 16);
    }
}
-(NSMenu *)menuForEvent:(NSEvent *)event
{
    int row;
    NSPoint where = [self convertPoint:[event locationInWindow] fromView:nil];
    row = (int)[self rowAtPoint:where];
    id item = [self itemAtRow:row];
    _event_handler(self, (NSEvent *)item, 10);
    return nil;
}
-(NSScrollView *)scrollview { return scrollview; }
-(void)setScrollview:(NSScrollView *)input { scrollview = input; }
-(void)deleteNode:(NSMutableArray *)item { _free_tree_recurse(data, item); }
-(void)setForegroundColor:(NSColor *)input
{
    NSTextFieldCell *cell = [treecol dataCell];
    fgcolor = input;
    [fgcolor retain];
    [cell setTextColor:fgcolor];
}
-(void)clear { NSMutableArray *toclear = data; data = nil; _free_tree_recurse(toclear, NULL); [self reloadData]; }
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc
{
    UserData *root = userdata;
    _remove_userdata(&root, NULL, TRUE);
    _free_tree_recurse(data, NULL);
    [treecol release];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
@end

/* Subclass for a Calendar type */
@interface DWCalendar : NSDatePicker
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWCalendar
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a Combobox type */
@interface DWComboBox : NSComboBox
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSComboBoxDelegate>
#endif
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)comboBoxSelectionDidChange:(NSNotification *)not;
-(void)setClickDefault:(id)input;
@end

@implementation DWComboBox
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)comboBoxSelectionDidChange:(NSNotification *)not { _event_handler(self, (void *)[self indexOfSelectedItem], 11); }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        [[self window] makeFirstResponder:clickDefault];
    } else
    {
        [super keyUp:theEvent];
    }
}
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a stepper component of the spinbutton type */
/* This is a bad way of doing this... but I can't get the other methods to work */
@interface DWStepper : NSStepper
{
    id textfield;
    id parent;
}
-(void)setTextfield:(id)input;
-(id)textfield;
-(void)setParent:(id)input;
-(id)parent;
-(void)mouseDown:(NSEvent *)event;
-(void)mouseUp:(NSEvent *)event;
@end

@implementation DWStepper
-(void)setTextfield:(id)input { textfield = input; }
-(id)textfield { return textfield; }
-(void)setParent:(id)input { parent = input; }
-(id)parent { return parent; }
-(void)mouseDown:(NSEvent *)event
{
    [super mouseDown:event];
#ifdef MAC_OS_X_VERSION_10_12
    if([[NSApp currentEvent] type] == NSEventTypeLeftMouseUp)
#else
    if([[NSApp currentEvent] type] == NSLeftMouseUp)
#endif
        [self mouseUp:event];
}
-(void)mouseUp:(NSEvent *)event
{
    [textfield takeIntValueFrom:self];
    _event_handler(parent, (void *)[self integerValue], 14);
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(vk == VK_UP || vk == VK_DOWN)
    {
        if(vk == VK_UP)
            [self setIntegerValue:([self integerValue]+[self increment])];
        else
            [self setIntegerValue:([self integerValue]-[self increment])];
        [self mouseUp:theEvent];
    }
    else
    {
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
        [super keyDown:theEvent];
    }
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
@end

/* Subclass for a Spinbutton type */
@interface DWSpinButton : NSView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTextFieldDelegate>
#endif
{
    void *userdata;
    NSTextField *textfield;
    DWStepper *stepper;
    id clickDefault;
}
-(id)init;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(NSTextField *)textfield;
-(NSStepper *)stepper;
-(void)controlTextDidChange:(NSNotification *)aNotification;
-(void)setClickDefault:(id)input;
@end

@implementation DWSpinButton
-(id)init
{
    self = [super init];

    if(self)
    {
        textfield = [[[NSTextField alloc] init] autorelease];
        [self addSubview:textfield];
        stepper = [[[DWStepper alloc] init] autorelease];
        [self addSubview:stepper];
        [stepper setParent:self];
        [stepper setTextfield:textfield];
        [textfield takeIntValueFrom:stepper];
        [textfield setDelegate:self];
    }
    return self;
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(NSTextField *)textfield { return textfield; }
-(NSStepper *)stepper { return stepper; }
-(void)controlTextDidChange:(NSNotification *)aNotification
{
    [stepper takeIntValueFrom:textfield];
    [textfield takeIntValueFrom:stepper];
    _event_handler(self, (void *)[stepper integerValue], 14);
}
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        [[self window] makeFirstResponder:clickDefault];
    }
    else
    {
        [super keyUp:theEvent];
    }
}
-(void)performClick:(id)sender { [textfield performClick:sender]; }
-(void)dealloc { UserData *root = userdata; _remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a MDI type
 * This is just a box for display purposes... but it is a
 * unique class so it can be identified when creating windows.
 */
@interface DWMDI : DWBox {}
@end

@implementation DWMDI
@end

/* This function adds a signal handler callback into the linked list.
 */
void _new_signal(ULONG message, HWND window, int msgid, void *signalfunction, void *discfunc, void *data)
{
    SignalHandler *new = malloc(sizeof(SignalHandler));

    new->message = message;
    new->window = window;
    new->id = msgid;
    new->signalfunction = signalfunction;
    new->discfunction = discfunc;
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
               tmp->id == msgid &&
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

void _handle_resize_events(Box *thisbox)
{
    int z;

    for(z=0;z<thisbox->count;z++)
    {
        id handle = thisbox->items[z].hwnd;

        if(thisbox->items[z].type == TYPEBOX)
        {
            Box *tmp = (Box *)[handle box];

            if(tmp)
            {
                _handle_resize_events(tmp);
            }
        }
        else
        {
            if([handle isMemberOfClass:[DWRender class]])
            {
                DWRender *render = (DWRender *)handle;
                NSSize oldsize = [render size];
                NSSize newsize = [render frame].size;
                NSWindow *window = [render window];

                if([window preferredBackingLocation] != NSWindowBackingLocationVideoMemory)
                {
                    [window setPreferredBackingLocation:NSWindowBackingLocationVideoMemory];
                }

                /* Eliminate duplicate configure requests */
                if(oldsize.width != newsize.width || oldsize.height != newsize.height)
                {
                    if(newsize.width > 0 && newsize.height > 0)
                    {
                        [render setSize:newsize];
                        _event_handler(handle, nil, 1);
                    }
                }
            }
            /* Special handling for notebook controls */
            else if([handle isMemberOfClass:[DWNotebook class]])
            {
                DWNotebook *notebook = (DWNotebook *)handle;
                DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
                id view = [notepage view];

                if([view isMemberOfClass:[DWBox class]])
                {
                    Box *box = (Box *)[view box];
                    _handle_resize_events(box);
                }
            }
            /* Handle laying out scrollviews... if required space is less
             * than available space, then expand.  Otherwise use required space.
             */
            else if([handle isMemberOfClass:[DWScrollBox class]])
            {
                DWScrollBox *scrollbox = (DWScrollBox *)handle;
                DWBox *contentbox = [scrollbox documentView];
                Box *thisbox = [contentbox box];

                /* Get the required space for the box */
                _handle_resize_events(thisbox);
            }
        }
    }
}

/* This function calculates how much space the widgets and boxes require
 * and does expansion as necessary.
 */
static void _resize_box(Box *thisbox, int *depth, int x, int y, int pass)
{
    /* Current item position */
    int z, currentx = thisbox->pad, currenty = thisbox->pad;
    /* Used x, y and padding maximum values...
     * These will be used to find the widest or
     * tallest items in a box.
     */
    int uymax = 0, uxmax = 0;
    int upymax = 0, upxmax = 0;

    /* Reset the box sizes */
    thisbox->minwidth = thisbox->minheight = thisbox->usedpadx = thisbox->usedpady = thisbox->pad * 2;

    /* Handle special groupbox case */
    if(thisbox->grouphwnd)
    {
        /* Only calculate the size on the first pass...
         * use the cached values on second.
         */
        if(pass == 1)
        {
            DWGroupBox *groupbox = thisbox->grouphwnd;
            NSSize borderSize = [groupbox borderSize];
            NSRect titleRect;

            if(borderSize.width == 0 || borderSize.height == 0)
            {
                borderSize = [groupbox initBorder];
            }
            /* Get the title size for a more accurate groupbox padding size */
            titleRect = [groupbox titleRect];

            thisbox->grouppadx = borderSize.width;
            thisbox->grouppady = borderSize.height + titleRect.size.height;
        }

        thisbox->minwidth += thisbox->grouppadx;
        thisbox->usedpadx += thisbox->grouppadx;
        thisbox->minheight += thisbox->grouppady;
        thisbox->usedpady += thisbox->grouppady;
    }

    /* Count up all the space for all items in the box */
    for(z=0;z<thisbox->count;z++)
    {
        int itempad, itemwidth, itemheight;

        if(thisbox->items[z].type == TYPEBOX)
        {
            id box = thisbox->items[z].hwnd;
            Box *tmp = (Box *)[box box];

            if(tmp)
            {
                /* On the first pass calculate the box contents */
                if(pass == 1)
                {
                    (*depth)++;

                    /* Save the newly calculated values on the box */
                    _resize_box(tmp, depth, x, y, pass);

                    /* Duplicate the values in the item list for use below */
                    thisbox->items[z].width = tmp->minwidth;
                    thisbox->items[z].height = tmp->minheight;

                    /* If the box has no contents but is expandable... default the size to 1 */
                    if(!thisbox->items[z].width && thisbox->items[z].hsize)
                       thisbox->items[z].width = 1;
                    if(!thisbox->items[z].height && thisbox->items[z].vsize)
                       thisbox->items[z].height = 1;

                    (*depth)--;
                }
            }
        }

        /* Precalculate these values, since they will
         * be used used repeatedly in the next section.
         */
        itempad = thisbox->items[z].pad * 2;
        itemwidth = thisbox->items[z].width + itempad;
        itemheight = thisbox->items[z].height + itempad;

        /* Calculate the totals and maximums */
        if(thisbox->type == DW_VERT)
        {
            if(itemwidth > uxmax)
                uxmax = itemwidth;

            if(thisbox->items[z].hsize != SIZEEXPAND)
            {
                if(itemwidth > upxmax)
                    upxmax = itemwidth;
            }
            else
            {
                if(itempad > upxmax)
                    upxmax = itempad;
            }
            thisbox->minheight += itemheight;
            if(thisbox->items[z].vsize != SIZEEXPAND)
                thisbox->usedpady += itemheight;
            else
                thisbox->usedpady += itempad;
        }
        else
        {
            if(itemheight > uymax)
                uymax = itemheight;
            if(thisbox->items[z].vsize != SIZEEXPAND)
            {
                if(itemheight > upymax)
                    upymax = itemheight;
            }
            else
            {
                if(itempad > upymax)
                    upymax = itempad;
            }
            thisbox->minwidth += itemwidth;
            if(thisbox->items[z].hsize != SIZEEXPAND)
                thisbox->usedpadx += itemwidth;
            else
                thisbox->usedpadx += itempad;
        }
    }

    /* Add the maximums which were calculated in the previous loop */
    thisbox->minwidth += uxmax;
    thisbox->minheight += uymax;
    thisbox->usedpadx += upxmax;
    thisbox->usedpady += upymax;

    /* The second pass is for actual placement. */
    if(pass > 1)
    {
        for(z=0;z<(thisbox->count);z++)
        {
            int height = thisbox->items[z].height;
            int width = thisbox->items[z].width;
            int itempad = thisbox->items[z].pad * 2;
            int thispad = thisbox->pad * 2;

            /* Calculate the new sizes */
            if(thisbox->items[z].hsize == SIZEEXPAND)
            {
                if(thisbox->type == DW_HORZ)
                {
                    int expandablex = thisbox->minwidth - thisbox->usedpadx;

                    if(expandablex)
                        width = (int)(((float)width / (float)expandablex) * (float)(x - thisbox->usedpadx));
                }
                else
                    width = x - (itempad + thispad + thisbox->grouppadx);
            }
            if(thisbox->items[z].vsize == SIZEEXPAND)
            {
                if(thisbox->type == DW_VERT)
                {
                    int expandabley = thisbox->minheight - thisbox->usedpady;

                    if(expandabley)
                        height = (int)(((float)height / (float)expandabley) * (float)(y - thisbox->usedpady));
                }
                else
                    height = y - (itempad + thispad + thisbox->grouppady);
            }

            /* If the calculated size is valid... */
            if(height > 0 && width > 0)
            {
                int pad = thisbox->items[z].pad;
                NSView *handle = thisbox->items[z].hwnd;
                NSPoint point;
                NSSize size;

                point.x = currentx + pad;
                point.y = currenty + pad;
                size.width = width;
                size.height = height;
                [handle setFrameOrigin:point];
                [handle setFrameSize:size];

                /* After placing a box... place its components */
                if(thisbox->items[z].type == TYPEBOX)
                {
                    id box = thisbox->items[z].hwnd;
                    Box *tmp = (Box *)[box box];

                    if(tmp)
                    {
                        (*depth)++;
                        _resize_box(tmp, depth, width, height, pass);
                        (*depth)--;
                    }
                }

                /* Special handling for notebook controls */
                if([handle isMemberOfClass:[DWNotebook class]])
                {
                    DWNotebook *notebook = (DWNotebook *)handle;
                    DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
                    id view = [notepage view];

                    if([view isMemberOfClass:[DWBox class]])
                    {
                        Box *box = (Box *)[view box];
                        NSSize size = [view frame].size;
                        _do_resize(box, size.width, size.height);
                        _handle_resize_events(box);
                    }
                }
                /* Handle laying out scrollviews... if required space is less
                 * than available space, then expand.  Otherwise use required space.
                 */
                else if([handle isMemberOfClass:[DWScrollBox class]])
                {
                    int depth = 0;
                    DWScrollBox *scrollbox = (DWScrollBox *)handle;
                    DWBox *contentbox = [scrollbox documentView];
                    Box *thisbox = [contentbox box];
                    NSSize contentsize = [scrollbox contentSize];

                    /* Get the required space for the box */
                    _resize_box(thisbox, &depth, x, y, 1);

                    if(contentsize.width < thisbox->minwidth)
                    {
                        contentsize.width = thisbox->minwidth;
                    }
                    if(contentsize.height < thisbox->minheight)
                    {
                        contentsize.height = thisbox->minheight;
                    }
                    [contentbox setFrameSize:contentsize];

                    /* Layout the content of the scrollbox */
                    _do_resize(thisbox, contentsize.width, contentsize.height);
                    _handle_resize_events(thisbox);
                }
                /* Special handling for spinbutton controls */
                else if([handle isMemberOfClass:[DWSpinButton class]])
                {
                    DWSpinButton *spinbutton = (DWSpinButton *)handle;
                    NSTextField *textfield = [spinbutton textfield];
                    NSStepper *stepper = [spinbutton stepper];
                    [textfield setFrameOrigin:NSMakePoint(0,0)];
                    [textfield setFrameSize:NSMakeSize(size.width-20,size.height)];
                    [stepper setFrameOrigin:NSMakePoint(size.width-20,0)];
                    [stepper setFrameSize:NSMakeSize(20,size.height)];
                }
                else if([handle isMemberOfClass:[DWSplitBar class]])
                {
                    DWSplitBar *split = (DWSplitBar *)handle;
                    DWWindow *window = (DWWindow *)[split window];
                    float percent = [split percent];

                    if(percent > 0 && size.width > 20 && size.height > 20)
                    {
                        dw_splitbar_set(handle, percent);
                        [split setPercent:0];
                    }
                    else if([window redraw])
                    {
                        [split splitViewDidResizeSubviews:nil];
                    }
                }

                /* Advance the current position in the box */
                if(thisbox->type == DW_HORZ)
                    currentx += width + (pad * 2);
                if(thisbox->type == DW_VERT)
                    currenty += height + (pad * 2);
            }
        }
    }
}

static void _do_resize(Box *thisbox, int x, int y)
{
    if(x > 0 && y > 0)
    {
        if(thisbox)
        {
            int depth = 0;

            /* Calculate space requirements */
            _resize_box(thisbox, &depth, x, y, 1);

            /* Finally place all the boxes and controls */
            _resize_box(thisbox, &depth, x, y, 2);
        }
    }
}

NSMenu *_generate_main_menu()
{
    NSString *applicationName = nil;

    /* This only works on 10.6 so we have a backup method */
#ifdef BUILDING_FOR_SNOW_LEOPARD
    applicationName = [[NSRunningApplication currentApplication] localizedName];
#endif
    if(applicationName == nil)
    {
        applicationName = [[NSProcessInfo processInfo] processName];
    }

    /* Create the main menu */
    NSMenu * mainMenu = [[[NSMenu alloc] initWithTitle:@"MainMenu"] autorelease];

    NSMenuItem * mitem = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
    NSMenu * menu = [[[NSMenu alloc] initWithTitle:@"Apple"] autorelease];

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6)
    [DWApp performSelector:@selector(setAppleMenu:) withObject:menu];
#endif

    /* Setup the Application menu */
    NSMenuItem * item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
                                        action:@selector(orderFrontStandardAboutPanel:)
                                 keyEquivalent:@""];
    [item setTarget:DWApp];

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
#ifdef MAC_OS_X_VERSION_10_12
    [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
#else
    [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
#endif
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

    return mainMenu;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
    dw_mutex_lock(DWRunMutex);
    DWThread = dw_thread_id();
    /* Make sure any queued redraws are handled */
    _dw_redraw(0, FALSE);
    [DWApp run];
    DWThread = (DWTID)-1;
    dw_mutex_unlock(DWRunMutex);
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
    [DWApp stop:nil];
    _dw_wakeup_app();
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
    DWTID curr = pthread_self();

    if(DWThread == (DWTID)-1 || DWThread == curr)
    {
        DWTID orig = DWThread;
        NSDate *until = [NSDate dateWithTimeIntervalSinceNow:(milliseconds/1000.0)];

        if(orig == (DWTID)-1)
        {
            dw_mutex_lock(DWRunMutex);
            DWThread = curr;
        }
       /* Process any pending events */
        while(_dw_main_iteration(until))
        {
            /* Just loop */
        }
        if(orig == (DWTID)-1)
        {
            DWThread = orig;
            dw_mutex_unlock(DWRunMutex);
        }
    }
    else
    {
        usleep(milliseconds * 1000);
    }
}

/* Internal version that doesn't lock the run mutex */
int _dw_main_iteration(NSDate *date)
{
#ifdef MAC_OS_X_VERSION_10_12
    NSEvent *event = [DWApp nextEventMatchingMask:NSEventMaskAny
#else
    NSEvent *event = [DWApp nextEventMatchingMask:NSAnyEventMask
#endif
                                        untilDate:date
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if(event)
    {
        [DWApp sendEvent:event];
        [DWApp updateWindows];
        return 1;
    }
    return 0;
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
    DWTID curr = pthread_self();

    if(DWThread == (DWTID)-1)
    {
        dw_mutex_lock(DWRunMutex);
        DWThread = curr;
        _dw_main_iteration([NSDate distantPast]);
        DWThread = (DWTID)-1;
        dw_mutex_unlock(DWRunMutex);
    }
    else if(DWThread == curr)
    {
        _dw_main_iteration([NSDate distantPast]);
    }
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
#if !defined(GARBAGE_COLLECT)
    pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
#endif
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
    dw_shutdown();
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
    static char _user_dir[PATH_MAX+1] = { 0 };

    if(!_user_dir[0])
    {
        char *home = getenv("HOME");

        if(home)
            strncpy(_user_dir, home, PATH_MAX);
        else
            strcpy(_user_dir, "/");
    }
    return _user_dir;
}

/*
 * Returns a pointer to a static buffer which containes the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    return _dw_bundle_path;
}

/*
 * Displays a debug message on the console...
 * Parameters:
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
void API dw_debug(char *format, ...)
{
   va_list args;
   char outbuf[1025] = {0};

   va_start(args, format);
   vsnprintf(outbuf, 1024, format, args);
   va_end(args);

   NSLog(@"%s", outbuf);
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
    NSAlert *alert;
    NSInteger iResponse;
    NSString *button1 = @"OK";
    NSString *button2 = nil;
    NSString *button3 = nil;
    va_list args;

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

    va_start(args, format);
    alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:title]];
    [alert setInformativeText:[[[NSString alloc] initWithFormat:[NSString stringWithUTF8String:format] arguments:args] autorelease]];
    [alert addButtonWithTitle:button1];
    if(button2)
        [alert addButtonWithTitle:button2];
    if(button3)
        [alert addButtonWithTitle:button3];
    va_end(args);

#ifdef MAC_OS_X_VERSION_10_12
    if(flags & DW_MB_ERROR)
        [alert setAlertStyle:NSAlertStyleCritical];
    else if(flags & DW_MB_INFORMATION)
        [alert setAlertStyle:NSAlertStyleInformational];
    else
        [alert setAlertStyle:NSAlertStyleWarning];
#else
    if(flags & DW_MB_ERROR)
        [alert setAlertStyle:NSCriticalAlertStyle];
    else if(flags & DW_MB_INFORMATION)
        [alert setAlertStyle:NSInformationalAlertStyle];
    else
        [alert setAlertStyle:NSWarningAlertStyle];
#endif
    iResponse = [alert runModal];
    [alert release];

    switch(iResponse)
    {
        case NSAlertFirstButtonReturn:    /* user pressed OK */
            if(flags & DW_MB_YESNO || flags & DW_MB_YESNOCANCEL)
            {
                return DW_MB_RETURN_YES;
            }
            return DW_MB_RETURN_OK;
        case NSAlertSecondButtonReturn:  /* user pressed Cancel */
            if(flags & DW_MB_OKCANCEL)
            {
                return DW_MB_RETURN_CANCEL;
            }
            return DW_MB_RETURN_NO;
        case NSAlertThirdButtonReturn:      /* user pressed the third button */
            return DW_MB_RETURN_CANCEL;
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
    char temp[PATH_MAX+1];
    char *file = NULL, *path = NULL;
    DW_LOCAL_POOL_IN;

    /* Figure out path information...
     * These functions are only support in Snow Leopard and later...
     */
    if(defpath && *defpath && DWOSMinor > 5)
    {
        struct stat buf;

        /* Get an absolute path */
        if(!realpath(defpath, temp))
            strcpy(temp, defpath);

        /* Check if the defpath exists */
        if(stat(temp, &buf) == 0)
        {
            /* Can be a directory or file */
            if(buf.st_mode & S_IFDIR)
                path = temp;
            else
                file = temp;
        }
        /* If it wasn't a directory... check if there is a path */
        if(!path && strchr(temp, '/'))
        {
            unsigned long x = strlen(temp) - 1;

            /* Trim off the filename */
            while(x > 0 && temp[x] != '/')
            {
                x--;
            }
            if(temp[x] == '/')
            {
                temp[x] = 0;
                /* Check to make sure the trimmed piece is a directory */
                if(stat(temp, &buf) == 0)
                {
                    if(buf.st_mode & S_IFDIR)
                    {
                        /* We now have it split */
                        path = temp;
                        file = &temp[x+1];
                    }
                }
            }
        }
    }

    if(flags == DW_FILE_OPEN || flags == DW_DIRECTORY_OPEN)
    {
        /* Create the File Open Dialog class. */
        NSOpenPanel* openDlg = [NSOpenPanel openPanel];

        if(path)
        {
            SEL ssdu = NSSelectorFromString(@"setDirectoryURL");

            if([openDlg respondsToSelector:ssdu])
            {
                IMP isdu = [openDlg methodForSelector:ssdu];
                isdu(openDlg, ssdu, [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]]);
            }
        }

        /* Enable the selection of files in the dialog. */
        if(flags == DW_FILE_OPEN)
        {
            [openDlg setCanChooseFiles:YES];
            [openDlg setCanChooseDirectories:NO];
        }
        else
        {
            [openDlg setCanChooseFiles:NO];
            [openDlg setCanChooseDirectories:YES];
        }

        /* Handle file types */
        if(ext && *ext)
        {
            NSArray* fileTypes = [[[NSArray alloc] initWithObjects:[NSString stringWithUTF8String:ext], nil] autorelease];
            [openDlg setAllowedFileTypes:fileTypes];
        }

        /* Disable multiple selection */
        [openDlg setAllowsMultipleSelection:NO];

        /* Display the dialog.  If the OK button was pressed,
         * process the files.
         */
        if([openDlg runModal] == DWModalResponseOK)
        {
            /* Get an array containing the full filenames of all
             * files and directories selected.
             */
            NSArray *files = [openDlg URLs];
            NSString *fileName = [[files objectAtIndex:0] path];
            if(fileName)
            {
                char *ret = strdup([ fileName UTF8String ]);
                DW_LOCAL_POOL_OUT;
                return ret;
            }
        }
    }
    else
    {
        /* Create the File Save Dialog class. */
        NSSavePanel* saveDlg = [NSSavePanel savePanel];

        if(path)
        {
            SEL ssdu = NSSelectorFromString(@"setDirectoryURL");

            if([saveDlg respondsToSelector:ssdu])
            {
                IMP isdu = [saveDlg methodForSelector:ssdu];
                isdu(saveDlg, ssdu, [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]]);
            }
        }
        if(file)
        {
            SEL ssnfsv = NSSelectorFromString(@"setNameFieldStringValue");

            if([saveDlg respondsToSelector:ssnfsv])
            {
                IMP isnfsv = [saveDlg methodForSelector:ssnfsv];
                isnfsv(saveDlg, ssnfsv, [NSString stringWithUTF8String:file]);
            }
        }

        /* Enable the creation of directories in the dialog. */
        [saveDlg setCanCreateDirectories:YES];

        /* Handle file types */
        if(ext && *ext)
        {
            NSArray* fileTypes = [[[NSArray alloc] initWithObjects:[NSString stringWithUTF8String:ext], nil] autorelease];
            [saveDlg setAllowedFileTypes:fileTypes];
        }

        /* Display the dialog.  If the OK button was pressed,
         * process the files.
         */
        if([saveDlg runModal] == NSFileHandlingPanelOKButton)
        {
            /* Get an array containing the full filenames of all
             * files and directories selected.
             */
            NSString* fileName = [[saveDlg URL] path];
            if(fileName)
            {
                char *ret = strdup([ fileName UTF8String ]);
                DW_LOCAL_POOL_OUT;
                return ret;
            }
        }
    }
    DW_LOCAL_POOL_OUT;
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
    SEL scc = NSSelectorFromString(@"clearContents");

    if([pasteboard respondsToSelector:scc])
    {
        IMP icc = [pasteboard methodForSelector:scc];
        icc(pasteboard, scc);
    }

    [pasteboard setString:[ NSString stringWithUTF8String:str ] forType:NSStringPboardType];
}


/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog * API dw_dialog_new(void *data)
{
    DWDialog *tmp = malloc(sizeof(DWDialog));

    if(tmp)
    {
        tmp->eve = dw_event_new();
        dw_event_reset(tmp->eve);
        tmp->data = data;
        tmp->done = FALSE;
        tmp->result = NULL;
    }
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
    void *tmp = NULL;

    if(dialog)
    {
        while(!dialog->done)
        {
            _dw_main_iteration([NSDate dateWithTimeIntervalSinceNow:0.01]);
        }
        dw_event_close(&dialog->eve);
        tmp = dialog->result;
        free(dialog);
    }
    return tmp;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_box_new(int type, int pad)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWBox *view = [[DWBox alloc] init];
    Box *newbox = [view box];
    memset(newbox, 0, sizeof(Box));
    newbox->pad = pad;
    newbox->type = type;
    DW_MUTEX_UNLOCK;
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
    NSBox *groupbox = [[DWGroupBox alloc] init];
    DWBox *thisbox = dw_box_new(type, pad);
    Box *box = [thisbox box];

    [groupbox setBorderType:NSBezelBorder];
    [groupbox setTitle:[NSString stringWithUTF8String:title]];
    box->grouphwnd = groupbox;
    [groupbox setContentView:thisbox];
    [thisbox autorelease];
    return groupbox;
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_scrollbox_new( int type, int pad )
{
    DWScrollBox *scrollbox = [[DWScrollBox alloc] init];
    DWBox *box = dw_box_new(type, pad);
    DWBox *tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, box, 1, 1, TRUE, TRUE, 0);
    [scrollbox setHasVerticalScroller:YES];
    [scrollbox setHasHorizontalScroller:YES];
    [scrollbox setBorderType:NSNoBorder];
    [scrollbox setDrawsBackground:NO];
    [scrollbox setBox:box];
    [scrollbox setDocumentView:tmpbox];
    [tmpbox autorelease];
    return scrollbox;
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_pos(HWND handle, int orient)
{
    DWScrollBox *scrollbox = handle;
    NSView *view = [scrollbox documentView];
    NSSize contentsize = [scrollbox contentSize];
    NSScroller *scrollbar;
    int range = 0;
    int val = 0;
    if(orient == DW_VERT)
    {
        scrollbar = [scrollbox verticalScroller];
        range = [view bounds].size.height - contentsize.height;
    }
    else
    {
        scrollbar = [scrollbox horizontalScroller];
        range = [view bounds].size.width - contentsize.width;
    }
    if(range > 0)
    {
        val = [scrollbar floatValue] * range;
    }
    return val;
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_range(HWND handle, int orient)
{
    DWScrollBox *scrollbox = handle;
    NSView *view = [scrollbox documentView];
    int range = 0;
    if(orient == DW_VERT)
    {
        range = [view bounds].size.height;
    }
    else
    {
        range = [view bounds].size.width;
    }
    return range;
}

/* Return the handle to the text object */
id _text_handle(id object)
{
    if([object isMemberOfClass:[ DWSpinButton class]])
    {
        DWSpinButton *spinbutton = object;
        object = [spinbutton textfield];
    }
    if([object isMemberOfClass:[ NSBox class]])
    {
        NSBox *box = object;
        id content = [box contentView];

        if([content isMemberOfClass:[ DWText class]])
        {
            object = content;
        }
    }
    return object;
}

/* Internal function to calculate the widget's required size..
 * These are the general rules for widget sizes:
 *
 * Render/Unspecified: 1x1
 * Scrolled(Container,Tree,MLE): Guessed size clamped to min and max in dw.h
 * Entryfield/Combobox/Spinbutton: 150x(maxfontheight)
 * Spinbutton: 50x(maxfontheight)
 * Text/Status: (textwidth)x(textheight)
 * Ranged: 100x14 or 14x100 for vertical.
 * Buttons/Bitmaps: Size of text or image and border.
*/
void _control_size(id handle, int *width, int *height)
{
    int thiswidth = 1, thisheight = 1, extrawidth = 0, extraheight = 0;
    NSString *nsstr = nil;
    id object = _text_handle(handle);

    /* Handle all the different button types */
    if([ object isKindOfClass:[ NSButton class ] ])
    {
        switch([object buttonType])
        {
            case NSSwitchButton:
            case NSRadioButton:
                extrawidth = 24;
                extraheight = 4;
                nsstr = [object title];
                break;
            default:
            {
                NSImage *image = [object image];

                if(image)
                {
                    /* Image button */
                    NSSize size = [image size];
                    thiswidth = (int)size.width;
                    thisheight = (int)size.height;
                    if([object isBordered])
                    {
                        extrawidth = 4;
                        extraheight = 4;
                    }
                }
                else
                {
                    /* Text button */
                    nsstr = [object title];

                    if([object isBordered])
                    {
                        extrawidth = 30;
                        extraheight = 8;
                    }
                    else
                    {
                        extrawidth = 8;
                        extraheight = 4;
                    }
                }
                break;
            }
        }
    }
    /* If the control is an entryfield set width to 150 */
    else if([object isKindOfClass:[ NSTextField class ]])
    {
        NSFont *font = [object font];

        if([object isEditable])
        {
            /* Spinbuttons don't need to be as wide */
            if([object isMemberOfClass:[ DWSpinButton class]])
                thiswidth = 50;
            else
                thiswidth = 150;
            /* Comboboxes need some extra height for the border */
            if([object isMemberOfClass:[ DWComboBox class]])
                extraheight = 4;
            /* Yosemite and higher requires even more border space */
            if(DWOSMinor > 9)
                extraheight += 4;
        }
        else
            nsstr = [object stringValue];

        if(font)
            thisheight = (int)[font boundingRectForFont].size.height;
    }
    /* Handle the ranged widgets */
    else if([ object isMemberOfClass:[DWPercent class] ] ||
            [ object isMemberOfClass:[DWSlider class] ])
    {
        thiswidth = 100;
        thisheight = 20;
    }
    /* Handle the ranged widgets */
    else if([ object isMemberOfClass:[DWScrollbar class] ])
    {
        if([object vertical])
        {
            thiswidth = 14;
            thisheight = 100;
        }
        else
        {
            thiswidth = 100;
            thisheight = 14;
        }
    }
    /* Handle bitmap size */
    else if([ object isMemberOfClass:[NSImageView class] ])
    {
        NSImage *image = [object image];

        if(image)
        {
            NSSize size = [image size];
            thiswidth = (int)size.width;
            thisheight = (int)size.height;
        }
    }
    /* Handle calendar */
    else if([ object isMemberOfClass:[DWCalendar class] ])
    {
        NSCell *cell = [object cell];

        if(cell)
        {
            NSSize size = [cell cellSize];

            thiswidth = size.width;
            thisheight = size.height;
        }
    }
    /* MLE and Container */
    else if([ object isMemberOfClass:[DWMLE class] ] ||
            [ object isMemberOfClass:[DWContainer class] ])
    {
        NSSize size;

        if([ object isMemberOfClass:[DWMLE class] ])
        {
            NSScrollView *sv = [object scrollview];
            NSSize frame = [sv frame].size;
            BOOL hscroll = [sv hasHorizontalScroller];

            /* Make sure word wrap is off for the first part */
            if(!hscroll)
            {
                [[object textContainer] setWidthTracksTextView:NO];
                [[object textContainer] setContainerSize:[object maxSize]];
                [object setHorizontallyResizable:YES];
                [sv setHasHorizontalScroller:YES];
            }
            /* Size the text view to fit */
            [object sizeToFit];
            size = [object bounds].size;
            size.width += 2.0;
            size.height += 2.0;
            /* Re-enable word wrapping if it was on */
            if(!hscroll)
            {
                [[object textContainer] setWidthTracksTextView:YES];
                [sv setHasHorizontalScroller:NO];

                /* If the un wrapped it is beyond the bounds... */
                if(size.width > _DW_SCROLLED_MAX_WIDTH)
                {
                    NSSize max = [object maxSize];

                    /* Set the max size to the limit */
                    [object setMaxSize:NSMakeSize(_DW_SCROLLED_MAX_WIDTH, max.height)];
                    /* Recalculate the size */
                    [object sizeToFit];
                    size = [object bounds].size;
                    size.width += 2.0;
                    size.height += 2.0;
                    [object setMaxSize:max];
                }
            }
            [sv setFrameSize:frame];
            /* Take into account the horizontal scrollbar */
            if(hscroll && size.width > _DW_SCROLLED_MAX_WIDTH)
                size.height += 16.0;
        }
        else
            size = [object getsize];

        thiswidth = size.width;
        thisheight = size.height;

        if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
            thiswidth = _DW_SCROLLED_MIN_WIDTH;
        if(thiswidth > _DW_SCROLLED_MAX_WIDTH)
            thiswidth = _DW_SCROLLED_MAX_WIDTH;
        if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
            thisheight = _DW_SCROLLED_MIN_HEIGHT;
        if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
            thisheight = _DW_SCROLLED_MAX_HEIGHT;
    }
    /* Tree */
    else if([ object isMemberOfClass:[DWTree class] ])
    {
        thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
        thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
    }
    /* Any other control type */
    else if([ object isKindOfClass:[ NSControl class ] ])
        nsstr = [object stringValue];

    /* If we have a string...
     * calculate the size with the current font.
     */
    if(nsstr && [nsstr length])
        dw_font_text_extents_get(object, NULL, (char *)[nsstr UTF8String], &thiswidth, &thisheight);

    /* Handle static text fields */
    if([object isKindOfClass:[ NSTextField class ]] && ![object isEditable])
    {
        id border = handle;

        extrawidth = 10;

        /* Handle status bar field */
        if([border isMemberOfClass:[ NSBox class ] ])
        {
            extrawidth += 2;
            extraheight = 8;
        }
    }

    /* Set the requested sizes */
    if(width)
        *width = thiswidth + extrawidth;
    if(height)
        *height = thisheight + extraheight;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = box;
    DWBox *view = box;
    DWBox *this = item;
    Box *thisbox;
    int z, x = 0;
    Item *tmpitem, *thisitem;

    /*
     * If you try and pack an item into itself VERY bad things can happen; like at least an
     * infinite loop on GTK! Lets be safe!
     */
    if(box == item)
    {
        dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
        return;
    }

    /* Query the objects */
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = box;
        view = [window contentView];
    }
    else if([ object isMemberOfClass:[ DWScrollBox class ] ])
    {
        DWScrollBox *scrollbox = box;
        view = [scrollbox box];
    }

    thisbox = [view box];
    thisitem = thisbox->items;
    object = item;

    /* Query the objects */
    if([ object isMemberOfClass:[ DWContainer class ] ] ||
       [ object isMemberOfClass:[ DWTree class ] ] ||
       [ object isMemberOfClass:[ DWMLE class ] ])
    {
        this = item = [object scrollview];
    }

    /* Do some sanity bounds checking */
    if(!thisitem)
        thisbox->count = 0;
    if(index < 0)
        index = 0;
    if(index > thisbox->count)
        index = thisbox->count;

    /* Duplicate the existing data */
    tmpitem = calloc(sizeof(Item), (thisbox->count+1));

    for(z=0;z<thisbox->count;z++)
    {
        if(z == index)
            x++;
        tmpitem[x] = thisitem[z];
        x++;
    }

    /* Sanity checks */
    if(vsize && !height)
       height = 1;
    if(hsize && !width)
       width = 1;

    /* Fill in the item data appropriately */
    if([object isKindOfClass:[DWBox class]] || [object isMemberOfClass:[DWGroupBox class]])
       tmpitem[index].type = TYPEBOX;
    else
    {
        if ( width == 0 && hsize == FALSE )
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
        if ( height == 0 && vsize == FALSE )
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Height and expand Vertical both unset for box: %x item: %x",box,item);

        tmpitem[index].type = TYPEITEM;
    }

    tmpitem[index].hwnd = item;
    tmpitem[index].origwidth = tmpitem[index].width = width;
    tmpitem[index].origheight = tmpitem[index].height = height;
    tmpitem[index].pad = pad;
    tmpitem[index].hsize = hsize ? SIZEEXPAND : SIZESTATIC;
    tmpitem[index].vsize = vsize ? SIZEEXPAND : SIZESTATIC;

    /* If either of the parameters are -1 ... calculate the size */
    if(width == -1 || height == -1)
        _control_size(object, width == -1 ? &tmpitem[index].width : NULL, height == -1 ? &tmpitem[index].height : NULL);

    thisbox->items = tmpitem;

    /* Update the item count */
    thisbox->count++;

    /* Add the item to the box */
    [view addSubview:this];
    /* Enable autorelease on the item...
     * so it will get destroyed when the parent is.
     */
    [this autorelease];
    /* If we are packing a button... */
    if([this isMemberOfClass:[DWButton class]])
    {
        DWButton *button = (DWButton *)this;

        /* Save the parent box so radio
         * buttons can use it later.
         */
        [button setParent:view];
    }
    /* Queue a redraw on the top-level window */
    _dw_redraw([view window], TRUE);

    /* Free the old data */
    if(thisitem)
       free(thisitem);
    DW_MUTEX_UNLOCK;
}

/*
 * Remove windows (widgets) from the box they are packed into.
 * Parameters:
 *       handle: Window handle of the packed item to be removed.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_box_unpack(HWND handle)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isKindOfClass:[NSView class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if(([object isKindOfClass:[NSTableView class]] || [object isMemberOfClass:[DWMLE class]])
           && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            id window = [object window];
            Box *thisbox = [parent box];
            int z, index = -1;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            if(!thisitem)
                thisbox->count = 0;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    index = z;
            }

            if(index == -1)
            {
                DW_MUTEX_UNLOCK;
                DW_LOCAL_POOL_OUT;
                return DW_ERROR_GENERAL;
            }

            [object retain];
            [object removeFromSuperview];

            if(thisbox->count > 1)
            {
                tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                /* Copy all but the current entry to the new list */
                for(z=0;z<index;z++)
                {
                    tmpitem[z] = thisitem[z];
                }
                for(z=index+1;z<thisbox->count;z++)
                {
                    tmpitem[z-1] = thisitem[z];
                }
            }

            thisbox->items = tmpitem;
            if(thisitem)
                free(thisitem);
            if(tmpitem)
                thisbox->count--;
            else
                thisbox->count = 0;
            /* Queue a redraw on the top-level window */
            _dw_redraw(window, TRUE);
        }
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
    return DW_ERROR_NONE;
}

/*
 * Remove windows (widgets) from a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be removed from.
 *       index: 0 based index of packed items.
 * Returns:
 *       Handle to the removed item on success, 0 on failure or padding.
 */
HWND API dw_box_unpack_at_index(HWND box, int index)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    DWBox *parent = (DWBox *)box;
    id object = nil;

    if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
    {
        id window = [parent window];
        Box *thisbox = [parent box];

        if(thisbox && index > -1 && index < thisbox->count)
        {
            int z;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            object = thisitem[index].hwnd;

            if(object)
            {
                [object retain];
                [object removeFromSuperview];
            }

            if(thisbox->count > 1)
            {
                tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                /* Copy all but the current entry to the new list */
                for(z=0;thisitem && z<index;z++)
                {
                    tmpitem[z] = thisitem[z];
                }
                for(z=index+1;z<thisbox->count;z++)
                {
                    tmpitem[z-1] = thisitem[z];
                }
            }

            thisbox->items = tmpitem;
            if(thisitem)
                free(thisitem);
            if(tmpitem)
                thisbox->count--;
            else
                thisbox->count = 0;

            /* Queue a redraw on the top-level window */
            _dw_redraw(window, TRUE);
        }
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
    return object;
}

/*
 * Pack windows (widgets) into a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       index: 0 based index of packed items.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_at_index(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, index, width, height, hsize, vsize, pad, "dw_box_pack_at_index()");
}

/*
 * Pack windows (widgets) into a box from the start (or top).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    /* 65536 is the table limit on GTK...
     * seems like a high enough value we will never hit it here either.
     */
    _dw_box_pack(box, item, 65536, width, height, hsize, vsize, pad, "dw_box_pack_start()");
}

/*
 * Pack windows (widgets) into a box from the end (or bottom).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
void API dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, 0, width, height, hsize, vsize, pad, "dw_box_pack_end()");
}

HWND _button_new(char *text, ULONG cid)
{
    DWButton *button = [[DWButton alloc] init];
    if(text)
    {
        [button setTitle:[ NSString stringWithUTF8String:text ]];
    }
    [button setTarget:button];
    [button setAction:@selector(buttonClicked:)];
    [button setTag:cid];
    [button setButtonType:NSMomentaryPushInButton];
#ifdef MAC_OS_X_VERSION_10_12
    [button setBezelStyle:NSBezelStyleRegularSquare];
#else
    [button setBezelStyle:NSThickerSquareBezelStyle];
#endif
    /* TODO: Reenable scaling in the future if it is possible on other platforms.
    [[button cell] setImageScaling:NSImageScaleProportionallyDown]; */
    if(DWDefaultFont)
    {
        [[button cell] setFont:DWDefaultFont];
    }
    return button;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_button_new(char *text, ULONG cid)
{
    DWButton *button = _button_new(text, cid);
    [button setButtonType:NSMomentaryPushInButton];
    [button setBezelStyle:NSRoundedBezelStyle];
    [button setImagePosition:NSNoImage];
#ifdef MAC_OS_X_VERSION_10_12
    [button setAlignment:NSTextAlignmentCenter];
#else
    [button setAlignment:NSCenterTextAlignment];
#endif
    [[button cell] setControlTint:NSBlueControlTint];
    return button;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_new(char *text, ULONG cid)
{
    DWEntryField *entry = [[DWEntryField alloc] init];
    [entry setStringValue:[ NSString stringWithUTF8String:text ]];
    [entry setTag:cid];
    [[entry cell] setScrollable:YES];
    [[entry cell] setWraps:NO];
    return entry;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(char *text, ULONG cid)
{
    DWEntryFieldPassword *entry = [[DWEntryFieldPassword alloc] init];
    [entry setStringValue:[ NSString stringWithUTF8String:text ]];
    [entry setTag:cid];
    [[entry cell] setScrollable:YES];
    [[entry cell] setWraps:NO];
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
    DWEntryField *entry = handle;
    DWEntryFieldFormatter *formatter = [[[DWEntryFieldFormatter alloc] init] autorelease];

    [formatter setMaximumLength:(int)limit];
    [[entry cell] setFormatter:formatter];
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(char *text, ULONG resid)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:filepath];
    DWButton *button = _button_new("", resid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
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
HWND API dw_bitmapbutton_new_from_file(char *text, unsigned long cid, char *filename)
{
    char *ext = _dw_get_image_extension( filename );

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:nstr];

    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[NSImage alloc] initWithContentsOfFile:nstr];
    }
    DWButton *button = _button_new("", cid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
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
HWND API dw_bitmapbutton_new_from_data(char *text, unsigned long cid, char *data, int len)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *image = [[NSImage alloc] initWithData:thisdata];
    DWButton *button = _button_new("", cid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_spinbutton_new(char *text, ULONG cid)
{
    DWSpinButton *spinbutton = [[DWSpinButton alloc] init];
    NSStepper *stepper = [spinbutton stepper];
    NSTextField *textfield = [spinbutton textfield];
    [stepper setIncrement:1];
    [stepper setTag:cid];
    [stepper setMinValue:-65536];
    [stepper setMaxValue:65536];
    [stepper setIntValue:atoi(text)];
    [textfield takeIntValueFrom:stepper];
    return spinbutton;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    NSTextField *textfield = [spinbutton textfield];
    [stepper setIntValue:(int)position];
    [textfield takeIntValueFrom:stepper];
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
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    [stepper setMinValue:(double)lower];
    [stepper setMaxValue:(double)upper];
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    return (long)[stepper integerValue];
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_radiobutton_new(char *text, ULONG cid)
{
    DWButton *button = _button_new(text, cid);
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
HWND API dw_slider_new(int vertical, int increments, ULONG cid)
{
    DWSlider *slider = [[DWSlider alloc] init];
    [slider setMaxValue:(double)increments];
    [slider setMinValue:0];
    [slider setContinuous:YES];
    [slider setTarget:slider];
    [slider setAction:@selector(sliderChanged:)];
    [slider setTag:cid];
    return slider;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
    DWSlider *slider = handle;
    double val = [slider doubleValue];
    return (int)val;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
    DWSlider *slider = handle;
    [slider setDoubleValue:(double)position];
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_scrollbar_new(int vertical, ULONG cid)
{
    DWScrollbar *scrollbar;
    if(vertical)
    {
        scrollbar = [[DWScrollbar alloc] init];
        [scrollbar setVertical:YES];
    }
    else
    {
        scrollbar = [[DWScrollbar alloc] initWithFrame:NSMakeRect(0,0,100,5)];
    }
    [scrollbar setArrowsPosition:NSScrollerArrowsDefaultSetting];
    [scrollbar setRange:0.0 andVisible:0.0];
    [scrollbar setKnobProportion:1.0];
    [scrollbar setTarget:scrollbar];
    [scrollbar setAction:@selector(scrollerChanged:)];
    [scrollbar setTag:cid];
    [scrollbar setEnabled:YES];
    return scrollbar;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
    DWScrollbar *scrollbar = handle;
    float range = [scrollbar range];
    float fresult = [scrollbar doubleValue] * range;
    return (int)fresult;
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
    DWScrollbar *scrollbar = handle;
    double range = [scrollbar range];
    double visible = [scrollbar visible];
    double newpos = (double)position/(range-visible);
    [scrollbar setDoubleValue:newpos];
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
    DWScrollbar *scrollbar = handle;
    double knob = (double)visible/(double)range;
    [scrollbar setRange:(double)range andVisible:(double)visible];
    [scrollbar setKnobProportion:knob];
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG cid)
{
    DWPercent *percent = [[DWPercent alloc] init];
    [percent setStyle:NSProgressIndicatorBarStyle];
    [percent setBezeled:YES];
    [percent setMaxValue:100];
    [percent setMinValue:0];
    [percent incrementBy:1];
    [percent setIndeterminate:NO];
    [percent setDoubleValue:0];
    /*[percent setTag:cid]; Why doesn't this work? */
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

    /* Handle indeterminate */
    if(position == DW_PERCENT_INDETERMINATE)
    {
        [percent setIndeterminate:YES];
        [percent startAnimation:percent];
    }
    else
    {
        /* Handle normal */
        if([percent isIndeterminate])
        {
            [percent stopAnimation:percent];
            [percent setIndeterminate:NO];
        }
        [percent setDoubleValue:(double)position];
    }
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(char *text, ULONG cid)
{
    DWButton *button = _button_new(text, cid);
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
    DWButton *button = handle;
    if([button state])
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
    DWButton *button = handle;
    if(value)
    {
        [button setState:NSOnState];
    }
    else
    {
        [button setState:NSOffState];
    }

}

/* Common code for containers and listboxes */
HWND _cont_new(ULONG cid, int multi)
{
    NSScrollView *scrollview  = [[NSScrollView alloc] init];
    DWContainer *cont = [[DWContainer alloc] init];

    [cont setScrollview:scrollview];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];

    if(multi)
    {
        [cont setAllowsMultipleSelection:YES];
    }
    else
    {
        [cont setAllowsMultipleSelection:NO];
    }
    [cont setDataSource:cont];
    [cont setDelegate:cont];
    [scrollview setDocumentView:cont];
    [cont setTag:cid];
    [cont autorelease];
    return cont;
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
HWND API dw_listbox_new(ULONG cid, int multi)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = _cont_new(cid, multi);
    [cont setHeaderView:nil];
    int type = DW_CFA_STRING;
    [cont setup];
    NSTableColumn *column = [[[NSTableColumn alloc] init] autorelease];
    [column setEditable:NO];
    [cont addTableColumn:column];
    [cont addColumn:column andType:type];
    DW_MUTEX_UNLOCK;
    return cont;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, char *text)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo addItemWithObjectValue:[ NSString stringWithUTF8String:text ]];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [ NSString stringWithUTF8String:text ];
        NSArray *newrow = [NSArray arrayWithObject:nstr];

        [cont addRow:newrow];
        /*[cont performSelectorOnMainThread:@selector(addRow:)
                               withObject:newrow
                            waitUntilDone:YES];*/
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo insertItemWithObjectValue:[ NSString stringWithUTF8String:text ] atIndex:pos];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [ NSString stringWithUTF8String:text ];
        NSArray *newrow = [NSArray arrayWithObject:nstr];

        [cont insertRow:newrow at:pos];
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int z;

        for(z=0;z<count;z++)
        {
            [combo addItemWithObjectValue:[ NSString stringWithUTF8String:text[z] ]];
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int z;

        for(z=0;z<count;z++)
        {
            NSString *nstr = [NSString stringWithUTF8String:text[z]];
            NSArray *newrow = [NSArray arrayWithObjects:nstr,nil];

            [cont addRow:newrow];
        }
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo removeAllItems];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont clear];
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
int API dw_listbox_count(HWND handle)
{
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        return (int)[combo numberOfItems];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        int _locked_by_me = FALSE;
        DW_MUTEX_LOCK;
        DWContainer *cont = handle;
        int result = (int)[cont numberOfRowsInTableView:cont];
        DW_MUTEX_UNLOCK;
        return result;
    }
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo scrollItemAtIndexToTop:top];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont scrollRowToVisible:top];
    }
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int count = (int)[combo numberOfItems];

        if(index > count)
        {
            *buffer = '\0';
        }
        else
        {
            NSString *nstr = [combo itemObjectValueAtIndex:index];
            strncpy(buffer, [ nstr UTF8String ], length - 1);
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInTableView:cont];

        if(index > count)
        {
            *buffer = '\0';
        }
        else
        {
            NSString *nstr = [cont getRow:index and:0];

            strncpy(buffer, [ nstr UTF8String ], length - 1);
        }
    }
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int count = (int)[combo numberOfItems];

        if(index <= count)
        {
            [combo removeItemAtIndex:index];
            [combo insertItemWithObjectValue:[ NSString stringWithUTF8String:buffer ] atIndex:index];
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInTableView:cont];

        if(index <= count)
        {
            NSString *nstr = [ NSString stringWithUTF8String:buffer ];

            [cont editCell:nstr at:index and:0];
            [cont reloadData];
            [cont setNeedsDisplay:YES];
        }
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
int API dw_listbox_selected(HWND handle)
{
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        return (int)[combo indexOfSelectedItem];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        int _locked_by_me = FALSE;
        DW_MUTEX_LOCK;
        DWContainer *cont = handle;
        int result = (int)[cont selectedRow];
        DW_MUTEX_UNLOCK;
        return result;
    }
    return -1;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;
    int retval = -1;

    if([object isMemberOfClass:[DWContainer class]])
    {
        NSUInteger result;
        DWContainer *cont = handle;
        NSIndexSet *selected = [cont selectedRowIndexes];
        if( where == -1 )
           result = [selected indexGreaterThanOrEqualToIndex:0];
        else
           result = [selected indexGreaterThanIndex:where];

        if(result != NSNotFound)
        {
            retval = (int)result;
        }
    }
    DW_MUTEX_UNLOCK;
    return retval;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        if(state)
            [combo selectItemAtIndex:index];
        else
            [combo deselectItemAtIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)index];

        [cont selectRowIndexes:selected byExtendingSelection:YES];
        [selected release];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo removeItemAtIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont removeRow:index];
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_combobox_new(char *text, ULONG cid)
{
    DWComboBox *combo = [[DWComboBox alloc] init];
    [combo setStringValue:[NSString stringWithUTF8String:text]];
    [combo setDelegate:combo];
    [combo setTag:cid];
    return combo;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG cid)
{
    DWMLE *mle = [[DWMLE alloc] init];
    NSScrollView *scrollview  = [[NSScrollView alloc] init];
    NSSize size = [mle maxSize];

    size.width = size.height;
    [mle setMaxSize:size];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];
    [scrollview setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [scrollview setDocumentView:mle];
    [mle setVerticallyResizable:YES];
    [mle setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [mle setScrollview:scrollview];
    [mle setAutomaticQuoteSubstitutionEnabled:NO];
    [mle setAutomaticDashSubstitutionEnabled:NO];
    [mle setAutomaticTextReplacementEnabled:NO];
    /* [mle setTag:cid]; Why doesn't this work? */
    [mle autorelease];
    return mle;
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
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSString *nstr = [NSString stringWithUTF8String:buffer];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    if(startpoint < 0)
        startpoint = 0;
    if(startpoint > length)
        startpoint = (int)length;
    [ms insertString:nstr atIndex:startpoint];
    DW_MUTEX_UNLOCK;
    return (unsigned int)strlen(buffer) + startpoint;
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
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    const char *tmp = [ms UTF8String];
    strncpy(buffer, tmp+startpoint, length);
    buffer[length] = '\0';
    DW_MUTEX_UNLOCK;
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
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger numberOfLines, index, stringLength = [ms length];

    if(bytes)
        *bytes = stringLength;
    if(lines)
    {
        for(index=0, numberOfLines=0; index < stringLength; numberOfLines++)
            index = NSMaxRange([ms lineRangeForRange:NSMakeRange(index, 0)]);

        *lines = numberOfLines;
    }
    DW_MUTEX_UNLOCK;
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
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger mslength = [ms length];
    if(startpoint < 0)
        startpoint = 0;
    if(startpoint > mslength)
        startpoint = (int)mslength;
    if(startpoint + length > mslength)
        length = (int)mslength - startpoint;
    [ms deleteCharactersInRange:NSMakeRange(startpoint, length)];
    DW_MUTEX_UNLOCK;
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    [ms deleteCharactersInRange:NSMakeRange(0, length)];
    DW_MUTEX_UNLOCK;
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger numberOfLines, index, stringLength = [ms length];

    for(index=0, numberOfLines=0; index < stringLength && numberOfLines < line; numberOfLines++)
        index = NSMaxRange([ms lineRangeForRange:NSMakeRange(index, 0)]);

    if(line == numberOfLines)
    {
        [mle scrollRangeToVisible:[ms lineRangeForRange:NSMakeRange(index, 0)]];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
    DWMLE *mle = handle;
    if(state)
    {
        [mle setEditable:YES];
    }
    else
    {
        [mle setEditable:NO];
    }
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
    DWMLE *mle = handle;
    NSScrollView *sv = [mle scrollview];

    if(state)
    {
        [[mle textContainer] setWidthTracksTextView:YES];
        [sv setHasHorizontalScroller:NO];
    }
    else
    {
        [[mle textContainer] setWidthTracksTextView:NO];
        [[mle textContainer] setContainerSize:[mle maxSize]];
        [mle setHorizontallyResizable:YES];
        [sv setHasHorizontalScroller:YES];
    }
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void API dw_mle_set_cursor(HWND handle, int point)
{
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    if(point < 0)
        point = 0;
    if(point > length)
        point = (int)length;
    [mle setSelectedRange: NSMakeRange(point,point)];
    [mle scrollRangeToVisible:NSMakeRange(point,point)];
    DW_MUTEX_UNLOCK;
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
    DWMLE *mle = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSString *searchForMe = [NSString stringWithUTF8String:text];
    NSRange searchRange = NSMakeRange(point, [ms length] - point);
    NSRange range = NSMakeRange(NSNotFound, 0);
    NSUInteger options = flags ? flags : NSCaseInsensitiveSearch;

    if(ms)
    {
        range = [ms rangeOfString:searchForMe options:options range:searchRange];
    }
    DW_MUTEX_UNLOCK;
    if(range.location != NSNotFound)
    {
        return -1;
    }
    return (int)range.location;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
    /* Don't think this is necessary */
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
    /* Don't think this is necessary */
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_status_text_new(char *text, ULONG cid)
{
    NSBox *border = [[NSBox alloc] init];
    NSTextField *textfield = dw_text_new(text, cid);

    [border setBorderType:NSGrooveBorder];
    //[border setBorderType:NSLineBorder];
    [border setTitlePosition:NSNoTitle];
    [border setContentView:textfield];
    [border setContentViewMargins:NSMakeSize(1.5,1.5)];
    [textfield autorelease];
    [textfield setBackgroundColor:[NSColor clearColor]];
    [[textfield cell] setVCenter:YES];
    return border;
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(char *text, ULONG cid)
{
    DWText *textfield = [[DWText alloc] init];
    [textfield setEditable:NO];
    [textfield setSelectable:NO];
    [textfield setBordered:NO];
    [textfield setDrawsBackground:NO];
    [textfield setStringValue:[ NSString stringWithUTF8String:text ]];
    [textfield setTag:cid];
    if(DWDefaultFont)
    {
        [[textfield cell] setFont:DWDefaultFont];
    }
    [[textfield cell] setWraps:NO];
    return textfield;
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
HWND API dw_render_new(unsigned long cid)
{
    DWRender *render = [[DWRender alloc] init];
    [render setTag:cid];
    return render;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
    NSColor *oldcolor = pthread_getspecific(_dw_fg_color_key);
    NSColor *newcolor;
    DW_LOCAL_POOL_IN;

    _foreground = _get_color(value);

    newcolor = [[NSColor colorWithDeviceRed:    DW_RED_VALUE(_foreground)/255.0 green:
                                                DW_GREEN_VALUE(_foreground)/255.0 blue:
                                                DW_BLUE_VALUE(_foreground)/255.0 alpha: 1] retain];
    pthread_setspecific(_dw_fg_color_key, newcolor);
    [oldcolor release];
    DW_LOCAL_POOL_OUT;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
    NSColor *oldcolor = pthread_getspecific(_dw_bg_color_key);
    NSColor *newcolor;
    DW_LOCAL_POOL_IN;

    if(value == DW_CLR_DEFAULT)
    {
        pthread_setspecific(_dw_bg_color_key, NULL);
    }
    else
    {
        _background = _get_color(value);

        newcolor = [[NSColor colorWithDeviceRed:    DW_RED_VALUE(_background)/255.0 green:
                                                    DW_GREEN_VALUE(_background)/255.0 blue:
                                                    DW_BLUE_VALUE(_background)/255.0 alpha: 1] retain];
        pthread_setspecific(_dw_bg_color_key, newcolor);
    }
    [oldcolor release];
    DW_LOCAL_POOL_OUT;
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
    /* Create the Color Chooser Dialog class. */
    DWColorChoose *colorDlg;
    DWDialog *dialog;
    DW_LOCAL_POOL_IN;

    if(![DWColorChoose sharedColorPanelExists])
    {
        colorDlg = (DWColorChoose *)[DWColorChoose sharedColorPanel];
        /* Set defaults for the dialog. */
        [colorDlg setContinuous:NO];
        [colorDlg setTarget:colorDlg];
        [colorDlg setAction:@selector(changeColor:)];
    }
    else
        colorDlg = (DWColorChoose *)[DWColorChoose sharedColorPanel];

    /* If someone is already waiting just return */
    if([colorDlg dialog])
    {
        DW_LOCAL_POOL_OUT;
        return value;
    }

    unsigned long tempcol = _get_color(value);
    NSColor *color = [[NSColor colorWithDeviceRed: DW_RED_VALUE(tempcol)/255.0 green: DW_GREEN_VALUE(tempcol)/255.0 blue: DW_BLUE_VALUE(tempcol)/255.0 alpha: 1] retain];
    [colorDlg setColor:color];

    dialog = dw_dialog_new(colorDlg);
    [colorDlg setDialog:dialog];
    [colorDlg makeKeyAndOrderFront:nil];

    /* Wait for them to pick a color */
    color = (NSColor *)dw_dialog_wait(dialog);

    /* Figure out the value of what they returned */
    CGFloat red, green, blue;
    [color getRed:&red green:&green blue:&blue alpha:NULL];
    value = DW_RGB((int)(red * 255), (int)(green *255), (int)(blue *255));
    DW_LOCAL_POOL_OUT;
    return value;
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
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;
    if(pixmap)
    {
        image = (id)pixmap->image;
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext
                                              graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort] flipped:YES]];
    }
    else
    {
        if([image lockFocusIfCanDraw] == NO)
        {
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
        _DWLastDrawable = handle;
    }
    NSBezierPath* aPath = [NSBezierPath bezierPath];
    [aPath setLineWidth: 0.5];
    NSColor *color = pthread_getspecific(_dw_fg_color_key);
    [color set];

    [aPath moveToPoint:NSMakePoint(x, y)];
    [aPath stroke];
    if(pixmap)
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [image unlockFocus];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
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
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;
    if(pixmap)
    {
        image = (id)pixmap->image;
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext
                                              graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort] flipped:YES]];
    }
    else
    {
        if([image lockFocusIfCanDraw] == NO)
        {
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
        _DWLastDrawable = handle;
    }
    NSBezierPath* aPath = [NSBezierPath bezierPath];
    NSColor *color = pthread_getspecific(_dw_fg_color_key);
    [color set];

    [aPath moveToPoint:NSMakePoint(x1 + 0.5, y1 + 0.5)];
    [aPath lineToPoint:NSMakePoint(x2 + 0.5, y2 + 0.5)];
    [aPath stroke];

    if(pixmap)
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [image unlockFocus];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
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
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;
    NSString *nstr = [ NSString stringWithUTF8String:text ];
    if(image)
    {
        if([image isMemberOfClass:[DWRender class]])
        {
            DWRender *render = handle;
            NSFont *font = [render font];
            if([image lockFocusIfCanDraw] == NO)
            {
                DW_MUTEX_UNLOCK;
                DW_LOCAL_POOL_OUT;
                return;
            }
            NSColor *fgcolor = pthread_getspecific(_dw_fg_color_key);
            NSColor *bgcolor = pthread_getspecific(_dw_bg_color_key);
            NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithObjectsAndKeys:fgcolor, NSForegroundColorAttributeName, nil];
            if(bgcolor)
            {
                [dict setValue:bgcolor forKey:NSBackgroundColorAttributeName];
            }
            if(font)
            {
                [dict setValue:font forKey:NSFontAttributeName];
            }
            [nstr drawAtPoint:NSMakePoint(x, y) withAttributes:dict];
            [image unlockFocus];
            [dict release];
        }
        _DWLastDrawable = handle;
    }
    if(pixmap)
    {
        NSFont *font = pixmap->font;
        DWRender *render = pixmap->handle;
        if(!font && [render isMemberOfClass:[DWRender class]])
        {
            font = [render font];
        }
        image = (id)pixmap->image;
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext
                                              graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort] flipped:YES]];
        NSColor *fgcolor = pthread_getspecific(_dw_fg_color_key);
        NSColor *bgcolor = pthread_getspecific(_dw_bg_color_key);
        NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithObjectsAndKeys:fgcolor, NSForegroundColorAttributeName, nil];
        if(bgcolor)
        {
            [dict setValue:bgcolor forKey:NSBackgroundColorAttributeName];
        }
        if(font)
        {
            [dict setValue:font forKey:NSFontAttributeName];
        }
        [nstr drawAtPoint:NSMakePoint(x, y) withAttributes:dict];
        [NSGraphicsContext restoreGraphicsState];
        [dict release];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
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
    id object = handle;
    NSString *nstr;
    NSFont *font = nil;
    DW_LOCAL_POOL_IN;

    nstr = [NSString stringWithUTF8String:text];

    /* Check the pixmap for associated object or font */
    if(pixmap)
    {
        object = pixmap->handle;
        font = pixmap->font;
    }
    NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
    /* If we didn't get a font from the pixmap... try the associated object */
    if(!font && ([object isMemberOfClass:[DWRender class]] || [object isKindOfClass:[NSControl class]]))
    {
        font = [object font];
    }
    /* If we got a font... add it to the dictionary */
    if(font)
    {
        [dict setValue:font forKey:NSFontAttributeName];
    }
    /* Calculate the size of the string */
    NSSize size = [nstr sizeWithAttributes:dict];
    [dict release];
    /* Return whatever information we can */
    if(width)
    {
        *width = size.width;
    }
    if(height)
    {
        *height = size.height;
    }
    DW_LOCAL_POOL_OUT;
}

/* Internal function to create an image graphics context...
 * with or without antialiasing enabled.
 */
id _create_gc(id image, bool antialiased)
{
    CGContextRef  context = (CGContextRef)[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort];
    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
    [gc setShouldAntialias:antialiased];
    CGContextSetAllowsAntialiasing(context, antialiased);
    return gc;
}

/* Draw a polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void API dw_draw_polygon( HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y )
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;
    int z;
    if(pixmap)
    {
        image = (id)pixmap->image;
        id gc = _create_gc(image, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }
    else
    {
        if([image lockFocusIfCanDraw] == NO)
        {
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
        [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
        _DWLastDrawable = handle;
    }
    NSBezierPath* aPath = [NSBezierPath bezierPath];
    NSColor *color = pthread_getspecific(_dw_fg_color_key);
    [color set];

    [aPath moveToPoint:NSMakePoint(*x + 0.5, *y + 0.5)];
    for(z=1;z<npoints;z++)
    {
        [aPath lineToPoint:NSMakePoint(x[z] + 0.5, y[z] + 0.5)];
    }
    [aPath closePath];
    if(flags & DW_DRAW_FILL)
    {
        [aPath fill];
    }
    [aPath stroke];
    if(pixmap)
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [image unlockFocus];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
}

/* Draw a rectangle on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the box or DW_DRAW_DEFAULT (0).
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void API dw_draw_rect(HWND handle, HPIXMAP pixmap, int flags, int x, int y, int width, int height)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;
    if(pixmap)
    {
        image = (id)pixmap->image;
        id gc = _create_gc(image, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }
    else
    {
        if([image lockFocusIfCanDraw] == NO)
        {
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
        [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
        _DWLastDrawable = handle;
    }
    NSColor *color = pthread_getspecific(_dw_fg_color_key);
    [color set];

    if(flags & DW_DRAW_FILL)
        [NSBezierPath fillRect:NSMakeRect(x, y, width, height)];
    else
        [NSBezierPath strokeRect:NSMakeRect(x, y, width, height)];
    if(pixmap)
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [image unlockFocus];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
}

/* Draw an arc on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the arc or DW_DRAW_DEFAULT (0).
 *              DW_DRAW_FULL will draw a complete circle/elipse.
 *       xorigin: X coordinate of center of arc.
 *       yorigin: Y coordinate of center of arc.
 *       x1: X coordinate of first segment of arc.
 *       y1: Y coordinate of first segment of arc.
 *       x2: X coordinate of second segment of arc.
 *       y2: Y coordinate of second segment of arc.
 */
void API dw_draw_arc(HWND handle, HPIXMAP pixmap, int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id image = handle;

    if(pixmap)
    {
        image = (id)pixmap->image;
        id gc = _create_gc(image, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }
    else
    {
        if([image lockFocusIfCanDraw] == NO)
        {
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
        [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
        _DWLastDrawable = handle;
    }
    NSBezierPath* aPath = [NSBezierPath bezierPath];
    NSColor *color = pthread_getspecific(_dw_fg_color_key);
    [color set];

    /* Special case of a full circle/oval */
    if(flags & DW_DRAW_FULL)
    {
        [aPath appendBezierPathWithOvalInRect:NSMakeRect(x1, y1, x2 - x1, y2 - y1)];
    }
    else
    {
        double a1 = atan2((y1-yorigin), (x1-xorigin));
        double a2 = atan2((y2-yorigin), (x2-xorigin));
        double dx = xorigin - x1;
        double dy = yorigin - y1;
        double r = sqrt(dx*dx + dy*dy);

        /* Convert to degrees */
        a1 *= (180.0 / M_PI);
        a2 *= (180.0 / M_PI);

        /* Prepare to draw */
        [aPath appendBezierPathWithArcWithCenter:NSMakePoint(xorigin, yorigin)
                                          radius:r startAngle:a1 endAngle:a2];
    }
    /* If the fill flag is passed */
    if(flags & DW_DRAW_FILL)
    {
        [aPath fill];
    }
    /* Actually do the drawing */
    [aPath stroke];
    if(pixmap)
    {
        [NSGraphicsContext restoreGraphicsState];
    }
    else
    {
        [image unlockFocus];
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_tree_new(ULONG cid)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSScrollView *scrollview  = [[NSScrollView alloc] init];
    DWTree *tree = [[DWTree alloc] init];

    [tree setScrollview:scrollview];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];

    [tree setAllowsMultipleSelection:NO];
    [tree setDataSource:tree];
    [tree setDelegate:tree];
    [scrollview setDocumentView:tree];
    [tree setHeaderView:nil];
    [tree setTag:cid];
    [tree autorelease];
    DW_MUTEX_UNLOCK;
    return tree;
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
HTREEITEM API dw_tree_insert_after(HWND handle, HTREEITEM item, char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    NSString *nstr = [[NSString stringWithUTF8String:title] retain];
    NSMutableArray *treenode = [[[NSMutableArray alloc] init] retain];
    if(icon)
        [treenode addObject:icon];
    else
        [treenode addObject:[NSNull null]];
    [treenode addObject:nstr];
    [treenode addObject:[NSValue valueWithPointer:itemdata]];
    [treenode addObject:[NSNull null]];
    [tree addTree:treenode and:parent after:item];
    if(parent)
        [tree reloadItem:parent reloadChildren:YES];
    else
        [tree reloadData];
    DW_MUTEX_UNLOCK;
    return treenode;
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
HTREEITEM API dw_tree_insert(HWND handle, char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    return dw_tree_insert_after(handle, NULL, title, icon, parent, itemdata);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSMutableArray *array = (NSMutableArray *)item;
    NSString *nstr = (NSString *)[array objectAtIndex:1];
    DW_MUTEX_UNLOCK;
    return strdup([nstr UTF8String]);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
HTREEITEM API dw_tree_get_parent(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    HTREEITEM parent;
    DWTree *tree = handle;

    DW_MUTEX_LOCK;
    parent = [tree parentForItem:item];
    DW_MUTEX_UNLOCK;
    return parent;
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void API dw_tree_item_change(HWND handle, HTREEITEM item, char *title, HICN icon)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    NSMutableArray *array = (NSMutableArray *)item;
    DW_LOCAL_POOL_IN;

    if(title)
    {
        NSString *oldstr = [array objectAtIndex:1];
        NSString *nstr = [[NSString stringWithUTF8String:title] retain];
        [array replaceObjectAtIndex:1 withObject:nstr];
        [oldstr release];
    }
    if(icon)
    {
        [array replaceObjectAtIndex:0 withObject:icon];
    }
    [tree reloadData];
    DW_LOCAL_POOL_OUT;
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSMutableArray *array = (NSMutableArray *)item;
    [array replaceObjectAtIndex:2 withObject:[NSValue valueWithPointer:itemdata]];
    DW_MUTEX_UNLOCK;
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    void *result = NULL;
    DW_MUTEX_LOCK;
    NSMutableArray *array = (NSMutableArray *)item;
    NSValue *value = [array objectAtIndex:2];
    if(value)
    {
        result = [value pointerValue];
    }
    DW_MUTEX_UNLOCK;
    return result;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    NSInteger itemIndex = [tree rowForItem:item];
    if(itemIndex > -1)
    {
        [tree selectRowIndexes:[NSIndexSet indexSetWithIndex:itemIndex] byExtendingSelection:NO];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    [tree clear];
    DW_MUTEX_UNLOCK;
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    [tree expandItem:item];
    DW_MUTEX_UNLOCK;
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    [tree collapseItem:item];
    DW_MUTEX_UNLOCK;
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWTree *tree = handle;
    [tree deleteNode:item];
    [tree reloadData];
    DW_MUTEX_UNLOCK;
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_container_new(ULONG cid, int multi)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = _cont_new(cid, multi);
    NSScrollView *scrollview = [cont scrollview];
    [scrollview setHasHorizontalScroller:YES];
    NSTableHeaderView *header = [[[NSTableHeaderView alloc] init] autorelease];
    [cont setHeaderView:header];
    [cont setTarget:cont];
    [cont setDoubleAction:@selector(doubleClicked:)];
    DW_MUTEX_UNLOCK;
    return cont;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    int z;
    DWContainer *cont = handle;

    [cont setup];

    for(z=0;z<count;z++)
    {
        NSTableColumn *column = [[NSTableColumn alloc] init];
        [[column headerCell] setStringValue:[ NSString stringWithUTF8String:titles[z] ]];
        if(flags[z] & DW_CFA_BITMAPORICON)
        {
            NSImageCell *imagecell = [[NSImageCell alloc] init];
            [column setDataCell:imagecell];
            [imagecell release];
        }
        else if(flags[z] & DW_CFA_STRINGANDICON)
        {
            DWImageAndTextCell *browsercell = [[DWImageAndTextCell alloc] init];
            [column setDataCell:browsercell];
            [browsercell release];
        }
        /* Defaults to left justified so just handle right and center */
#ifdef MAC_OS_X_VERSION_10_12
        if(flags[z] & DW_CFA_RIGHT)
        {
            [(NSCell *)[column dataCell] setAlignment:NSTextAlignmentRight];
            [(NSCell *)[column headerCell] setAlignment:NSTextAlignmentRight];
        }
        else if(flags[z] & DW_CFA_CENTER)
        {
            [(NSCell *)[column dataCell] setAlignment:NSTextAlignmentCenter];
            [(NSCell *)[column headerCell] setAlignment:NSTextAlignmentCenter];
        }
#else
        if(flags[z] & DW_CFA_RIGHT)
        {
            [(NSCell *)[column dataCell] setAlignment:NSRightTextAlignment];
            [(NSCell *)[column headerCell] setAlignment:NSRightTextAlignment];
        }
        else if(flags[z] & DW_CFA_CENTER)
        {
            [(NSCell *)[column dataCell] setAlignment:NSCenterTextAlignment];
            [(NSCell *)[column headerCell] setAlignment:NSCenterTextAlignment];
        }
#endif
        [column setEditable:NO];
        [cont addTableColumn:column];
        [cont addColumn:column andType:(int)flags[z]];
        [column release];
    }
    DW_MUTEX_UNLOCK;
    return DW_ERROR_NONE;
}

/*
 * Configures the main filesystem columnn title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, char *title)
{
	char *newtitle = strdup(title ? title : "");
	
	dw_window_set_data(handle, "_dw_coltitle", newtitle);
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
    char **newtitles = malloc(sizeof(char *) * (count + 1));
    unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 1));
    char *coltitle = (char *)dw_window_get_data(handle, "_dw_coltitle");
    DWContainer *cont = handle;

    newtitles[0] = coltitle ? coltitle : "Filename";

    newflags[0] = DW_CFA_STRINGANDICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

    memcpy(&newtitles[1], titles, sizeof(char *) * count);
    memcpy(&newflags[1], flags, sizeof(unsigned long) * count);

    dw_container_setup(handle, newflags, newtitles, count + 1, 0);
    [cont setFilesystem:YES];

    if(coltitle)
    {
        dw_window_set_data(handle, "_dw_coltitle", NULL);
        free(coltitle);
    }
    free(newtitles);
    free(newflags);
    return DW_ERROR_NONE;
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont addRows:rowcount];
    DW_MUTEX_UNLOCK;
    return cont;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    id object = nil;
    int type = [cont cellType:column];
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
    {
        lastadd = [cont lastAddPoint];
    }

    if(data)
    {
        if(type & DW_CFA_BITMAPORICON)
        {
            object = *((NSImage **)data);
        }
        else if(type & DW_CFA_STRING)
        {
            char *str = *((char **)data);
            object = [ NSString stringWithUTF8String:str ];
        }
        else
        {
            char textbuffer[101] = {0};

            if(type & DW_CFA_ULONG)
            {
                ULONG tmp = *((ULONG *)data);

                snprintf(textbuffer, 100, "%lu", tmp);
            }
            else if(type & DW_CFA_DATE)
            {
                struct tm curtm;
                CDATE cdate = *((CDATE *)data);

                memset( &curtm, 0, sizeof(curtm) );
                curtm.tm_mday = cdate.day;
                curtm.tm_mon = cdate.month - 1;
                curtm.tm_year = cdate.year - 1900;

                strftime(textbuffer, 100, "%x", &curtm);
            }
            else if(type & DW_CFA_TIME)
            {
                struct tm curtm;
                CTIME ctime = *((CTIME *)data);

                memset( &curtm, 0, sizeof(curtm) );
                curtm.tm_hour = ctime.hours;
                curtm.tm_min = ctime.minutes;
                curtm.tm_sec = ctime.seconds;

                strftime(textbuffer, 100, "%X", &curtm);
            }
            if(textbuffer[0])
                object = [ NSString stringWithUTF8String:textbuffer ];
        }
    }

    [cont editCell:object at:(row+lastadd) and:column];
    [cont setNeedsDisplay:YES];
    DW_MUTEX_UNLOCK;
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
    dw_container_set_item(handle, NULL, column, row, data);
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
    dw_container_change_item(handle, column+1, row, data);
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
void API dw_filesystem_change_file(HWND handle, int row, char *filename, HICN icon)
{
    dw_filesystem_set_file(handle, NULL, row, filename, icon);
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
void API dw_filesystem_set_file(HWND handle, void *pointer, int row, char *filename, HICN icon)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    DWImageAndTextCell *browsercell;
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
    {
        lastadd = [cont lastAddPoint];
    }

    browsercell = [[[DWImageAndTextCell alloc] init] autorelease];
    [browsercell setImage:icon];
    [browsercell setStringValue:[ NSString stringWithUTF8String:filename ]];
    [cont editCell:browsercell at:(row+lastadd) and:0];
    [cont setNeedsDisplay:YES];
    DW_MUTEX_UNLOCK;
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
    dw_container_set_item(handle, pointer, column+1, row, data);
}

/*
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    int rc;
    int flag = [cont cellType:column];
    if(flag & DW_CFA_BITMAPORICON)
        rc = DW_CFA_BITMAPORICON;
    else if(flag & DW_CFA_STRING)
        rc = DW_CFA_STRING;
    else if(flag & DW_CFA_ULONG)
        rc = DW_CFA_ULONG;
    else if(flag & DW_CFA_DATE)
        rc = DW_CFA_DATE;
    else if(flag & DW_CFA_TIME)
        rc = DW_CFA_TIME;
    else
        rc = 0;
    DW_MUTEX_UNLOCK;
    return rc;
}

/*
 * Gets column type for a filesystem container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_filesystem_get_column_type(HWND handle, int column)
{
    return dw_container_get_column_type(handle, column+1);
}

/*
 * Sets the alternating row colors for container window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          oddcolor: Odd row background color in DW_RGB format or a default color index.
 *          evencolor: Even row background color in DW_RGB format or a default color index.
 *                    DW_RGB_TRANSPARENT will disable coloring rows.
 *                    DW_CLR_DEFAULT will use the system default alternating row colors.
 */
void API dw_container_set_stripe(HWND handle, unsigned long oddcolor, unsigned long evencolor)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont setRowBgOdd:(oddcolor == DW_CLR_DEFAULT ? DW_RGB(230,230,230) : oddcolor)
              andEven:(evencolor == DW_CLR_DEFAULT ? DW_RGB_TRANSPARENT : evencolor)];
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    if([cont filesystem])
    {
        column++;
    }
    NSTableColumn *col = [cont getColumn:column];

    [col setWidth:width];
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRow:(row+lastadd) title:title];
    DW_MUTEX_UNLOCK;
}


/*
 * Sets the data pointer of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_set_row_data(void *pointer, int row, void *data)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRowData:(row+lastadd) title:data];
    DW_MUTEX_UNLOCK;
}


/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_change_row_title(HWND handle, int row, char *title)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont setRow:row title:title];
    DW_MUTEX_UNLOCK;
}

/*
 * Sets the data pointer of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_change_row_data(HWND handle, int row, void *data)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont setRowData:row title:data];
    DW_MUTEX_UNLOCK;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont reloadData];
    DW_MUTEX_UNLOCK;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont clear];
    if(redraw)
    {
        [cont reloadData];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    int x;

    for(x=0;x<rowcount;x++)
    {
        [cont removeRow:0];
    }
    [cont reloadData];
    DW_MUTEX_UNLOCK;
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
    DWContainer *cont = handle;
    NSScrollView *sv = [cont scrollview];
    NSScroller *scrollbar = [sv verticalScroller];
    int rowcount = (int)[cont numberOfRowsInTableView:cont];
    float currpos = [scrollbar floatValue];
    float change;

    /* Safety check */
    if(rowcount < 1)
    {
        return;
    }

    change = (float)rows/(float)rowcount;

    switch(direction)
    {
        case DW_SCROLL_TOP:
        {
            [scrollbar setFloatValue:0];
            break;
        }
        case DW_SCROLL_BOTTOM:
        {
            [scrollbar setFloatValue:1];
            break;
        }
        case DW_SCROLL_UP:
        {
            float newpos = currpos - change;
            if(newpos < 0)
            {
                newpos = 0;
            }
            [scrollbar setFloatValue:newpos];
            break;
        }
        case DW_SCROLL_DOWN:
        {
            float newpos = currpos + change;
            if(newpos > 1)
            {
                newpos = 1;
            }
            [scrollbar setFloatValue:newpos];
            break;
        }
    }
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    NSIndexSet *selected = [cont selectedRowIndexes];
    NSUInteger result = [selected indexGreaterThanOrEqualToIndex:0];
    void *retval = NULL;

    if(result != NSNotFound)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result];
        else
        {
            char *temp = [cont getRowTitle:(int)result];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:(int)result];
    }
    DW_MUTEX_UNLOCK;
    return retval;
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    int lastQueryPoint = [cont lastQueryPoint];
    NSIndexSet *selected = [cont selectedRowIndexes];
    NSUInteger result = [selected indexGreaterThanIndex:lastQueryPoint];
    void *retval = NULL;

    if(result != NSNotFound)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result];
        else
        {
            char *temp = [cont getRowTitle:(int)result];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:(int)result];
    }
    DW_MUTEX_UNLOCK;
    return retval;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor(HWND handle, char *text)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)x];

            [cont selectRowIndexes:selected byExtendingSelection:YES];
            [selected release];
            [cont scrollRowToVisible:x];
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data: Data associated with the row.
 */
void API dw_container_cursor_by_data(HWND handle, void *data)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)x];

            [cont selectRowIndexes:selected byExtendingSelection:YES];
            [selected release];
            [cont scrollRowToVisible:x];
            DW_MUTEX_UNLOCK;
            DW_LOCAL_POOL_OUT;
            return;
        }
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, char *text)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            [cont removeRow:x];
            [cont reloadData];
            DW_MUTEX_UNLOCK;
            return;
        }
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data: Data specified.
 */
void API dw_container_delete_row_by_data(HWND handle, void *data)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            [cont removeRow:x];
            [cont reloadData];
            DW_MUTEX_UNLOCK;
            return;
        }
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWContainer *cont = handle;
    [cont optimize];
    DW_MUTEX_UNLOCK;
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void API dw_taskbar_insert(HWND handle, HICN icon, char *bubbletext)
{
    NSStatusItem *item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
    NSImage *image = icon;
    [item setImage:image];
    if(bubbletext)
        [item setToolTip:[NSString stringWithUTF8String:bubbletext]];
    [item setTarget:handle];
    [item setEnabled:YES];
    [item setHighlightMode:YES];
#ifdef MAC_OS_X_VERSION_10_12
    [item sendActionOn:(NSEventMaskLeftMouseUp|NSEventMaskLeftMouseDown|NSEventMaskRightMouseUp|NSEventMaskRightMouseDown)];
#else
    [item sendActionOn:(NSLeftMouseUpMask|NSLeftMouseDownMask|NSRightMouseUpMask|NSRightMouseDownMask)];
#endif
    [item setAction:@selector(mouseDown:)];
    dw_window_set_data(handle, "_dw_taskbar", item);
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
    NSStatusItem *item = dw_window_get_data(handle, "_dw_taskbar");
    DW_LOCAL_POOL_IN;
    [item release];
    DW_LOCAL_POOL_OUT;
}

/* Internal function to keep HICNs from getting too big */
void _icon_resize(NSImage *image)
{
    if(image)
    {
        NSSize size = [image size];
        if(size.width > 24 || size.height > 24)
        {
            if(size.width > 24)
                size.width = 24;
            if(size.height > 24)
                size.height = 24;
            [image setSize:size];
        }
    }
}

/* Internal version that does not resize the image */
HICN _dw_icon_load(unsigned long resid)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:filepath];
    return image;
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 */
HICN API dw_icon_load(unsigned long module, unsigned long resid)
{
    NSImage *image = _dw_icon_load(resid);
    _icon_resize(image);
    return image;
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_file(char *filename)
{
    char *ext = _dw_get_image_extension( filename );

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:nstr];
    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[NSImage alloc] initWithContentsOfFile:nstr];
    }
    _icon_resize(image);
    return image;
}

/*
 * Obtains an icon from data
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_data(char *data, int len)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *image = [[NSImage alloc] initWithData:thisdata];
    _icon_resize(image);
    return image;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
    NSImage *image = handle;
    DW_LOCAL_POOL_IN;
    [image release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long cid)
{
    /* There isn't anything like quite like MDI on MacOS...
     * However we will make floating windows that hide
     * when the application is deactivated to simulate
     * similar behavior.
     */
    DWMDI *mdi = [[DWMDI alloc] init];
    /* [mdi setTag:cid]; Why doesn't this work? */
    return mdi;
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
HWND API dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long cid)
{
    id tmpbox = dw_box_new(DW_VERT, 0);
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    DWSplitBar *split = [[DWSplitBar alloc] init];
    [split setDelegate:split];
    dw_box_pack_start(tmpbox, topleft, 0, 0, TRUE, TRUE, 0);
    [split addSubview:tmpbox];
    [tmpbox autorelease];
    tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, bottomright, 0, 0, TRUE, TRUE, 0);
    [split addSubview:tmpbox];
    [tmpbox autorelease];
    if(type == DW_VERT)
    {
        [split setVertical:NO];
    }
    else
    {
        [split setVertical:YES];
    }
    /* Set the default percent to 50% split */
    [split setPercent:50.0];
    /* [split setTag:cid]; Why doesn't this work? */
    DW_MUTEX_UNLOCK;
    return split;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 *       percent: The position of the splitbar.
 */
void API dw_splitbar_set(HWND handle, float percent)
{
    DWSplitBar *split = handle;
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSRect rect = [split frame];
    float pos;
    /* Calculate the position based on the size */
    if([split isVertical])
    {
        pos = rect.size.width * (percent / 100.0);
    }
    else
    {
        pos = rect.size.height * (percent / 100.0);
    }
    if(pos > 0)
    {
        [split setPosition:pos ofDividerAtIndex:0];
    }
    else
    {
        /* If we have no size.. wait until the resize
         * event when we get an actual size to try
         * to set the splitbar again.
         */
        [split setPercent:percent];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
    DWSplitBar *split = handle;
    NSRect rect1 = [split frame];
    NSArray *subviews = [split subviews];
    NSView *view = [subviews objectAtIndex:0];
    NSRect rect2 = [view frame];
    float pos, total, retval = 0.0;
    if([split isVertical])
    {
        total = rect1.size.width;
        pos = rect2.size.width;
    }
    else
    {
        total = rect1.size.height;
        pos = rect2.size.height;
    }
    if(total > 0)
    {
        retval = pos / total;
    }
    return retval;
}

/* Internal function to convert fontname to NSFont */
NSFont *_dw_font_by_name(char *fontname)
{
    char *fontcopy = strdup(fontname);
    char *name = strchr(fontcopy, '.');
    NSFont *font = nil;

    if(name)
    {
        int size = atoi(fontcopy);
        *name = 0;
        name++;
        font = [NSFont fontWithName:[ NSString stringWithUTF8String:name ] size:(float)size];
    }
    free(fontcopy);
    return font;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_bitmap_new(ULONG cid)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSImageView *bitmap = [[NSImageView alloc] init];
    [bitmap setImageFrameStyle:NSImageFrameNone];
    [bitmap setImageScaling:NSImageScaleNone];
    [bitmap setEditable:NO];
    [bitmap setTag:cid];
    DW_MUTEX_UNLOCK;
    return bitmap;
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
    HPIXMAP pixmap;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
        return NULL;
    pixmap->width = width;
    pixmap->height = height;
    pixmap->handle = handle;
    pixmap->image = [[NSBitmapImageRep alloc]
                                    initWithBitmapDataPlanes:NULL
                                    pixelsWide:width
                                    pixelsHigh:height
                                    bitsPerSample:8
                                    samplesPerPixel:4
                                    hasAlpha:YES
                                    isPlanar:NO
                                    colorSpaceName:NSDeviceRGBColorSpace
                                    bytesPerRow:0
                                    bitsPerPixel:0];
    return pixmap;
}

/* Function takes an NSImage and copies it into a flipped NSBitmapImageRep */
void _flip_image(NSImage *tmpimage, NSBitmapImageRep *image, NSSize size)
{
#ifdef MAC_OS_X_VERSION_10_12
    NSCompositingOperation op =NSCompositingOperationSourceOver;
#else
    NSCompositingOperation op =NSCompositeSourceOver;
#endif
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext
                                          graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort]
                                          flipped:YES]];
    [[[NSDictionary alloc] initWithObjectsAndKeys:image, NSGraphicsContextDestinationAttributeName, nil] autorelease];
    /* Make a new transform */
    NSAffineTransform *t = [NSAffineTransform transform];

    /* By scaling Y negatively, we effectively flip the image */
    [t scaleXBy:1.0 yBy:-1.0];

    /* But we also have to translate it back by its height */
    [t translateXBy:0.0 yBy:-size.height];

    /* Apply the transform */
    [t concat];
    [tmpimage drawAtPoint:NSMakePoint(0, 0) fromRect:NSMakeRect(0, 0, size.width, size.height)
                operation:op fraction:1.0];
    [NSGraphicsContext restoreGraphicsState];
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
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;
    char *ext = _dw_get_image_extension( filename );

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *tmpimage = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
    if(!tmpimage && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        tmpimage = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
    }
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSSize size = [tmpimage size];
    NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                               initWithBitmapDataPlanes:NULL
                               pixelsWide:size.width
                               pixelsHigh:size.height
                               bitsPerSample:8
                               samplesPerPixel:4
                               hasAlpha:YES
                               isPlanar:NO
                               colorSpaceName:NSDeviceRGBColorSpace
                               bytesPerRow:0
                               bitsPerPixel:0];
    _flip_image(tmpimage, image, size);
    pixmap->width = size.width;
    pixmap->height = size.height;
    pixmap->image = image;
    pixmap->handle = handle;
    DW_LOCAL_POOL_OUT;
    return pixmap;
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
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *tmpimage = [[[NSImage alloc] initWithData:thisdata] autorelease];
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSSize size = [tmpimage size];
    NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                               initWithBitmapDataPlanes:NULL
                               pixelsWide:size.width
                               pixelsHigh:size.height
                               bitsPerSample:8
                               samplesPerPixel:4
                               hasAlpha:YES
                               isPlanar:NO
                               colorSpaceName:NSDeviceRGBColorSpace
                               bytesPerRow:0
                               bitsPerPixel:0];
    _flip_image(tmpimage, image, size);
    pixmap->width = size.width;
    pixmap->height = size.height;
    pixmap->image = image;
    pixmap->handle = handle;
    DW_LOCAL_POOL_OUT;
    return pixmap;
}

/*
 * Sets the transparent color for a pixmap
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  transparent color
 * Note: This does nothing on Mac as transparency
 *       is handled automatically
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
    /* Don't do anything */
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_grab(HWND handle, ULONG resid)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }

    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *temp = [[NSImage alloc] initWithContentsOfFile:filepath];

    if(temp)
    {
        NSSize size = [temp size];
        NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                                   initWithBitmapDataPlanes:NULL
                                   pixelsWide:size.width
                                   pixelsHigh:size.height
                                   bitsPerSample:8
                                   samplesPerPixel:4
                                   hasAlpha:YES
                                   isPlanar:NO
                                   colorSpaceName:NSDeviceRGBColorSpace
                                   bytesPerRow:0
                                   bitsPerPixel:0];
        _flip_image(temp, image, size);
        pixmap->width = size.width;
        pixmap->height = size.height;
        pixmap->image = image;
        pixmap->handle = handle;
        [temp release];
        return pixmap;
    }
    free(pixmap);
    DW_LOCAL_POOL_OUT;
    return NULL;
}

/*
 * Sets the font used by a specified pixmap.
 * Normally the pixmap font is obtained from the associated window handle.
 * However this can be used to override that, or for pixmaps with no window.
 * Parameters:
 *          pixmap: Handle to a pixmap returned by dw_pixmap_new() or
 *                  passed to the application via a callback.
 *          fontname: Name and size of the font in the form "size.fontname"
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_pixmap_set_font(HPIXMAP pixmap, char *fontname)
{
    if(pixmap)
    {
        NSFont *font = _dw_font_by_name(fontname);

        if(font)
        {
            DW_LOCAL_POOL_IN;
            NSFont *oldfont = pixmap->font;
            [font retain];
            pixmap->font = font;
            if(oldfont)
                [oldfont release];
            DW_LOCAL_POOL_OUT;
            return DW_ERROR_NONE;
        }
    }
    return DW_ERROR_GENERAL;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
void API dw_pixmap_destroy(HPIXMAP pixmap)
{
    if(pixmap)
    {
        NSBitmapImageRep *image = (NSBitmapImageRep *)pixmap->image;
        NSFont *font = pixmap->font;
        DW_LOCAL_POOL_IN;
        [image release];
        [font release];
        free(pixmap);
        DW_LOCAL_POOL_OUT;
    }
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
    dw_pixmap_stretch_bitblt(dest, destp, xdest, ydest, width, height, src, srcp, xsrc, ysrc, -1, -1);
}

/*
 * Copies from one surface to another allowing for stretching.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of the target area.
 *       height: Height of the target area.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 *       srcwidth: Width of area to copy.
 *       srcheight: Height of area to copy.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_pixmap_stretch_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc, int srcwidth, int srcheight)
{
    DWBitBlt *bltinfo;
    NSValue* bi;
    DW_LOCAL_POOL_IN;

    /* Sanity checks */
    if((!dest && !destp) || (!src && !srcp) ||
       ((srcwidth == -1 || srcheight == -1) && srcwidth != srcheight))
    {
        DW_LOCAL_POOL_OUT;
        return DW_ERROR_GENERAL;
    }

    bltinfo = calloc(1, sizeof(DWBitBlt));
    bi = [NSValue valueWithPointer:bltinfo];

    /* Fill in the information */
    bltinfo->dest = dest;
    bltinfo->src = src;
    bltinfo->xdest = xdest;
    bltinfo->ydest = ydest;
    bltinfo->width = width;
    bltinfo->height = height;
    bltinfo->xsrc = xsrc;
    bltinfo->ysrc = ysrc;
    bltinfo->srcwidth = srcwidth;
    bltinfo->srcheight = srcheight;

    if(destp)
    {
        bltinfo->dest = (id)destp->image;
    }
    if(srcp)
    {
        id object = bltinfo->src = (id)srcp->image;
        [object retain];
    }
    if(DWThread == (DWTID)-1)
        [DWObj doBitBlt:bi];
    else
        [DWObj performSelectorOnMainThread:@selector(doBitBlt:) withObject:bi waitUntilDone:YES];
    DW_LOCAL_POOL_OUT;
    return DW_ERROR_NONE;
}

/*
 * Create a new static text window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_calendar_new(ULONG cid)
{
    DWCalendar *calendar = [[DWCalendar alloc] init];
    [calendar setDatePickerMode:NSSingleDateMode];
    [calendar setDatePickerStyle:NSClockAndCalendarDatePickerStyle];
    [calendar setDatePickerElements:NSYearMonthDayDatePickerElementFlag];
    [calendar setTag:cid];
    [calendar setDateValue:[NSDate date]];
    return calendar;
}

/*
 * Sets the current date of a calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
void dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
    DWCalendar *calendar = handle;
    NSDate *date;
    char buffer[101];
    DW_LOCAL_POOL_IN;

    snprintf(buffer, 100, "%04d-%02d-%02d", year, month, day);

    NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
    dateFormatter.dateFormat = @"yyyy-mm-dd";

    date = [dateFormatter dateFromString:[NSString stringWithUTF8String:buffer]];
    [calendar setDateValue:date];
    [date release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
    DWCalendar *calendar = handle;
    DW_LOCAL_POOL_IN;
    NSDate *date = [calendar dateValue];
    NSDateFormatter *df = [[NSDateFormatter alloc] init];
    [df setDateStyle:NSDateFormatterShortStyle];
    NSString *nstr = [df stringFromDate:date];
    sscanf([ nstr UTF8String ], "%d/%d/%d", month, day, year);
    if(*year < 70)
    {
        *year += 2000;
    }
    else if(*year < 100)
    {
        *year += 1900;
    }
    [df release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
    WebView *html = handle;
    switch(action)
    {
        case DW_HTML_GOBACK:
            [html goBack];
            break;
        case DW_HTML_GOFORWARD:
            [html goForward];
            break;
        case DW_HTML_GOHOME:
            break;
        case DW_HTML_SEARCH:
            break;
        case DW_HTML_RELOAD:
            [html reload:html];
            break;
        case DW_HTML_STOP:
            [html stopLoading:html];
            break;
        case DW_HTML_PRINT:
            break;
    }
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
HWND API dw_html_new(unsigned long cid)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    WebView *web = [[WebView alloc] init];
    /* [web setTag:cid]; Why doesn't this work? */
    DW_MUTEX_UNLOCK;
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
        *y = [[NSScreen mainScreen] frame].size.height - mouseLoc.y;
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
    /* From what I have read this isn't possible, agaist human interface rules */
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI API dw_menu_new(ULONG cid)
{
    NSMenu *menu = [[NSMenu alloc] init];
    [menu setAutoenablesItems:NO];
    /* [menu setTag:cid]; Why doesn't this work? */
    return menu;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
    NSWindow *window = location;
    NSMenu *windowmenu = _generate_main_menu();
    [[window contentView] setMenu:windowmenu];
    return (HMENUI)windowmenu;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
    NSMenu *thismenu = *menu;
    DW_LOCAL_POOL_IN;
    [thismenu release];
    DW_LOCAL_POOL_OUT;
}

/* Handle deprecation of convertScreenToBase in 10.10 yet still supporting
 * 10.6 and earlier since convertRectFromScreen was introduced in 10.7.
 */
NSPoint _windowPointFromScreen(id window, NSPoint p)
{
    SEL crfs = NSSelectorFromString(@"convertRectFromScreen:");

    if([window respondsToSelector:crfs])
    {
        NSRect (* icrfs)(id, SEL, NSRect) = (NSRect (*)(id, SEL, NSRect))[window methodForSelector:crfs];
        NSRect rect = icrfs(window, crfs, NSMakeRect(p.x, p.y, 1, 1));
        return rect.origin;
    }
    else
    {
        SEL cstb = NSSelectorFromString(@"convertScreenToBase:");

        if([window respondsToSelector:cstb])
        {
            NSPoint (* icstb)(id, SEL, NSPoint) = (NSPoint (*)(id, SEL, NSPoint))[window methodForSelector:cstb];
            return icstb(window, cstb, p);
        }
    }
    return NSMakePoint(0,0);
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
    NSMenu *thismenu = (NSMenu *)*menu;
    id object = parent;
    NSView *view = [object isKindOfClass:[NSWindow class]] ? [object contentView] : parent;
    NSWindow *window = [view window];
    NSEvent *event = [DWApp currentEvent];
    if(!window)
        window = [event window];
    [thismenu autorelease];
    NSPoint p = NSMakePoint(x, [[NSScreen mainScreen] frame].size.height - y);
#ifdef MAC_OS_X_VERSION_10_12
    NSEvent* fake = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown
#else
    NSEvent* fake = [NSEvent mouseEventWithType:NSRightMouseDown
#endif
                                       location:_windowPointFromScreen(window, p)
                                  modifierFlags:0
                                      timestamp:[event timestamp]
                                   windowNumber:[window windowNumber]
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
            accel = src[z+1];
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
HWND API dw_menu_append_item(HMENUI menux, char *title, ULONG itemid, ULONG flags, int end, int check, HMENUI submenux)
{
    NSMenu *menu = menux;
    NSMenu *submenu = submenux;
    DWMenuItem *item = NULL;
    if(strlen(title) == 0)
    {
        [menu addItem:[DWMenuItem separatorItem]];
    }
    else
    {
        char accel[2];
        char *newtitle = malloc(strlen(title)+1);
        NSString *nstr;

        accel[0] = _removetilde(newtitle, title);
        accel[1] = 0;

        nstr = [ NSString stringWithUTF8String:newtitle ];
        free(newtitle);

        item = [[[DWMenuItem alloc] initWithTitle:nstr
                            action:@selector(menuHandler:)
                            keyEquivalent:[ NSString stringWithUTF8String:accel ]] autorelease];
        [menu addItem:item];

        [item setTag:itemid];
        if(check)
        {
            [item setCheck:YES];
            if(flags & DW_MIS_CHECKED)
            {
                [item setState:NSOnState];
            }
        }
        if(flags & DW_MIS_DISABLED)
        {
            [item setEnabled:NO];
        }

        if(submenux)
        {
            [submenu setTitle:nstr];
            [menu setSubmenu:submenu forItem:item];
        }
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
void API dw_menu_item_set_check(HMENUI menux, unsigned long itemid, int check)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        if(check)
        {
            [menuitem setState:NSOnState];
        }
        else
        {
            [menuitem setState:NSOffState];
        }
    }
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menux, unsigned long itemid)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        [menu removeItem:menuitem];
        return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       flags: DW_MIS_ENABLED/DW_MIS_DISABLED
 *              DW_MIS_CHECKED/DW_MIS_UNCHECKED
 */
void API dw_menu_item_set_state(HMENUI menux, unsigned long itemid, unsigned long state)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        if(state & DW_MIS_CHECKED)
        {
            [menuitem setState:NSOnState];
        }
        else if(state & DW_MIS_UNCHECKED)
        {
            [menuitem setState:NSOffState];
        }
        if(state & DW_MIS_ENABLED)
        {
            [menuitem setEnabled:YES];
        }
        else if(state & DW_MIS_DISABLED)
        {
            [menuitem setEnabled:NO];
        }
    }
}

/* Gets the notebook page from associated ID */
DWNotebookPage *_notepage_from_id(DWNotebook *notebook, unsigned long pageid)
{
    NSArray *pages = [notebook tabViewItems];
    for(DWNotebookPage *notepage in pages)
    {
        if([notepage pageid] == pageid)
        {
            return notepage;
        }
    }
    return nil;
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG cid, int top)
{
    DWNotebook *notebook = [[DWNotebook alloc] init];
    [notebook setDelegate:notebook];
    /* [notebook setTag:cid]; Why doesn't this work? */
    return notebook;
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
    DWNotebook *notebook = handle;
    NSInteger page = [notebook pageid];
    DWNotebookPage *notepage = [[DWNotebookPage alloc] initWithIdentifier:[NSString stringWithFormat: @"pageid:%d", (int)page]];
    [notepage setPageid:(int)page];
    if(front)
    {
        [notebook insertTabViewItem:notepage atIndex:(NSInteger)0];
    }
    else
    {
        [notebook addTabViewItem:notepage];
    }
    [notepage autorelease];
    [notebook setPageid:(int)(page+1)];
    return (unsigned long)page;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned int pageid)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _notepage_from_id(notebook, pageid);
    DW_LOCAL_POOL_IN;

    if(notepage != nil)
    {
        [notebook removeTabViewItem:notepage];
    }
    DW_LOCAL_POOL_OUT;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
    return [notepage pageid];
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned int pageid)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        [notebook selectTabViewItem:notepage];
    }
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
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        [notepage setLabel:[ NSString stringWithUTF8String:text ]];
    }
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
    /* Note supported here... do nothing */
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
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        HWND tmpbox = dw_box_new(DW_VERT, 0);
        DWBox *box = tmpbox;

        dw_box_pack_start(tmpbox, page, 0, 0, TRUE, TRUE, 0);
        [notepage setView:box];
        [box autorelease];
    }
}

#ifndef NSWindowCollectionBehaviorFullScreenPrimary
#define NSWindowCollectionBehaviorFullScreenPrimary (1 << 7)
#endif

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
HWND API dw_window_new(HWND hwndOwner, char *title, ULONG flStyle)
{
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSRect frame = NSMakeRect(1,1,1,1);
    DWWindow *window = [[DWWindow alloc]
                        initWithContentRect:frame
                        styleMask:(flStyle)
                        backing:NSBackingStoreBuffered
                        defer:false];

    [window setTitle:[ NSString stringWithUTF8String:title ]];

    DWView *view = [[DWView alloc] init];

    [window setContentView:view];
    [window setDelegate:view];
    [window setAutorecalculatesKeyViewLoop:YES];
    [window setAcceptsMouseMovedEvents:YES];
    [window setReleasedWhenClosed:YES];
    [view autorelease];

    /* Enable full screen mode on resizeable windows */
    if(flStyle & DW_FCF_SIZEBORDER)
    {
        [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    }

    /* If it isn't a toplevel window... */
    if(hwndOwner)
    {
        id object = hwndOwner;

        /* Check to see if the parent is an MDI window */
        if([object isMemberOfClass:[DWMDI class]])
        {
            /* Set the window level to be floating */
            [window setLevel:NSFloatingWindowLevel];
            [window setHidesOnDeactivate:YES];
        }
    }
    DW_MUTEX_UNLOCK;
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
    void **params = calloc(2, sizeof(void *));
    NSValue *v;
    DW_LOCAL_POOL_IN;
    v = [NSValue valueWithPointer:params];
    params[0] = function;
    params[1] = data;
    [DWObj performSelectorOnMainThread:@selector(doWindowFunc:) withObject:v waitUntilDone:YES];
    free(params);
    DW_LOCAL_POOL_OUT;
}


/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
    id object = handle;

    if([ object isKindOfClass:[ NSView class ] ])
    {
        NSView *view = handle;

        if(pointertype == DW_POINTER_DEFAULT)
        {
            [view discardCursorRects];
        }
        else if(pointertype == DW_POINTER_ARROW)
        {
            NSRect rect = [view frame];
            NSCursor *cursor = [NSCursor arrowCursor];

            [view addCursorRect:rect cursor:cursor];
        }
        /* No cursor for DW_POINTER_CLOCK? */
    }
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        NSRect rect = [[window contentView] frame];
        id defaultitem = [window initialFirstResponder];

        if([window isMiniaturized])
        {
            [window deminiaturize:nil];
        }
        /* If we haven't been sized by a call.. */
        if(rect.size.width <= 1 || rect.size.height <= 1)
        {
            /* Determine the contents size */
            dw_window_set_size(handle, 0, 0);
        }
        /* If the position was not set... generate a default
         * default one in a similar pattern to SHELLPOSITION.
         */
        if(![window shown])
        {
            static int defaultx = 0, defaulty = 0;
            int cx = dw_screen_width(), cy = dw_screen_height();
            int maxx = cx / 4, maxy = cy / 4;
            NSPoint point;

            rect = [window frame];

            defaultx += 20;
            defaulty += 20;
            if(defaultx > maxx)
                defaultx = 20;
            if(defaulty > maxy)
                defaulty = 20;

            point.x = defaultx;
            /* Take into account menu bar and inverted Y */
            point.y = cy - defaulty - (int)rect.size.height - 22;

            [window setFrameOrigin:point];
            [window setShown:YES];
        }
        [[window contentView] showWindow];
        [window makeKeyAndOrderFront:nil];

#ifdef MAC_OS_X_VERSION_10_12
        if(!([window styleMask] & NSWindowStyleMaskResizable))
#else
        if(!([window styleMask] & NSResizableWindowMask))
#endif
        {
            /* Fix incorrect repeat in displaying textured windows */
            [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMinYEdge];
            [window setContentBorderThickness:0.0 forEdge:NSMinYEdge];
        }
        if(defaultitem)
        {
            /* If there is a default item set, make it first responder */
            [window makeFirstResponder:defaultitem];
        }
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
    NSObject *object = handle;

    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = handle;

        [window orderOut:nil];
    }
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
    id object = handle;
    unsigned long _fore = _get_color(fore);
    unsigned long _back = _get_color(back);
    NSColor *fg = NULL;
    NSColor *bg = NULL;

    /* Get the NSColor for non-default colors */
    if(fore != DW_CLR_DEFAULT)
    {
        fg = [NSColor colorWithDeviceRed: DW_RED_VALUE(_fore)/255.0 green: DW_GREEN_VALUE(_fore)/255.0 blue: DW_BLUE_VALUE(_fore)/255.0 alpha: 1];
    }
    if(back != DW_CLR_DEFAULT)
    {
        bg = [NSColor colorWithDeviceRed: DW_RED_VALUE(_back)/255.0 green: DW_GREEN_VALUE(_back)/255.0 blue: DW_BLUE_VALUE(_back)/255.0 alpha: 1];
    }

    /* Get the textfield from the spinbutton */
    if([object isMemberOfClass:[DWSpinButton class]])
    {
        object = [object textfield];
    }
    /* Get the cell on classes using NSCell */
    if([object isKindOfClass:[NSTextField class]])
    {
        id cell = [object cell];

        [object setTextColor:(fg ? fg : [NSColor controlTextColor])];
        [cell setTextColor:(fg ? fg : [NSColor controlTextColor])];
    }
    if([object isMemberOfClass:[DWButton class]])
    {
        [object setTextColor:(fg ? fg : [NSColor controlTextColor])];
    }
    if([object isKindOfClass:[NSTextField class]] || [object isKindOfClass:[NSButton class]])
    {
        id cell = [object cell];

        [cell setBackgroundColor:(bg ? bg : [NSColor controlColor])];
    }
    else if([object isMemberOfClass:[DWBox class]])
    {
        DWBox *box = object;

        [box setColor:_back];
    }
    else if([object isKindOfClass:[NSTableView class]])
    {
        DWContainer *cont = handle;

        [cont setBackgroundColor:(bg ? bg : [NSColor controlBackgroundColor])];
        [cont setForegroundColor:(fg ? fg : [NSColor controlTextColor])];
    }
    else if([object isMemberOfClass:[DWMLE class]])
    {
        DWMLE *mle = handle;
        [mle setBackgroundColor:(bg ? bg : [NSColor controlBackgroundColor])];
        NSTextStorage *ts = [mle textStorage];
        [ts setForegroundColor:(fg ? fg : [NSColor controlTextColor])];
    }
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
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void API dw_window_set_style(HWND handle, ULONG style, ULONG mask)
{
    id object = _text_handle(handle);

    if([object isMemberOfClass:[DWWindow class]])
    {
        DWWindow *window = object;
        SEL sssm = NSSelectorFromString(@"setStyleMask");

        if([window respondsToSelector:sssm])
        {
            IMP issm = [window methodForSelector:sssm];
            int currentstyle = (int)[window styleMask];
            int tmp;

            tmp = currentstyle | (int)mask;
            tmp ^= mask;
            tmp |= style;

            issm(window, sssm, tmp);
        }
    }
    else if([object isKindOfClass:[NSTextField class]])
    {
        NSTextField *tf = object;
        DWTextFieldCell *cell = [tf cell];

        [cell setAlignment:(style & 0xF)];
        if(mask & DW_DT_VCENTER && [cell isMemberOfClass:[DWTextFieldCell class]])
        {
            [cell setVCenter:(style & DW_DT_VCENTER ? YES : NO)];
        }
        if(mask & DW_DT_WORDBREAK && [cell isMemberOfClass:[DWTextFieldCell class]])
        {
            [cell setWraps:(style & DW_DT_WORDBREAK ? YES : NO)];
        }
    }
    else if([object isMemberOfClass:[NSTextView class]])
    {
        NSTextView *tv = handle;
        [tv setAlignment:(style & mask)];
    }
    else if([object isMemberOfClass:[DWButton class]])
    {
        DWButton *button = handle;

        if(mask & DW_BS_NOBORDER)
        {
            if(style & DW_BS_NOBORDER)
                [button setBordered:NO];
            else
                [button setBordered:YES];
        }
    }
    else if([object isMemberOfClass:[DWMenuItem class]])
    {
        if(mask & (DW_MIS_CHECKED | DW_MIS_UNCHECKED))
        {
            if(style & DW_MIS_CHECKED)
                [object setState:NSOnState];
            else if(style & DW_MIS_UNCHECKED)
                [object setState:NSOffState];
        }
        if(mask & (DW_MIS_ENABLED | DW_MIS_DISABLED))
        {
            if(style & DW_MIS_ENABLED)
                [object setEnabled:YES];
            else if(style & DW_MIS_DISABLED)
                [object setEnabled:NO];
        }
    }
}

/*
 * Sets the current focus item for a window/dialog.
 * Parameters:
 *         handle: Handle to the dialog item to be focused.
 * Remarks:
 *          This is for use after showing the window/dialog.
 */
void API dw_window_set_focus(HWND handle)
{
    id object = handle;

    [[object window] makeFirstResponder:object];
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND handle, HWND defaultitem)
{
    DWWindow *window = handle;
    id object = defaultitem;

    if([window isKindOfClass:[NSWindow class]] && [object isKindOfClass:[NSControl class]])
    {
        [window setInitialFirstResponder:defaultitem];
    }
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND handle, HWND next)
{
    id object = handle;
    id control = next;

    if([object isMemberOfClass:[DWWindow class]])
    {
        if([control isMemberOfClass:[DWButton class]])
        {
            NSWindow *window = object;

            [window setDefaultButtonCell:control];
        }
    }
    else
    {
        if([control isMemberOfClass:[DWSpinButton class]])
        {
            control = [control textfield];
        }
        else if([control isMemberOfClass:[DWComboBox class]])
        {
            /* TODO: Figure out why the combobox can't be
             * focused using makeFirstResponder method.
             */
            control = [control textfield];
        }
        [object setClickDefault:control];
    }
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
    /* Don't do anything for now */
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
    /* Don't do anything for now */
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
    id object = handle;

    if([object isMemberOfClass:[DWWindow class]])
    {
        /* We can't actually reparent on MacOS but if the
         * new parent is an MDI window, change to be a
         * floating window... otherwise set it to normal.
         */
        NSWindow *window = handle;

        /* If it isn't a toplevel window... */
        if(newparent)
        {
            object = newparent;

            /* Check to see if the parent is an MDI window */
            if([object isMemberOfClass:[DWMDI class]])
            {
                /* Set the window level to be floating */
                [window setLevel:NSFloatingWindowLevel];
                [window setHidesOnDeactivate:YES];
                return;
            }
        }
        /* Set the window back to a normal window */
        [window setLevel:NSNormalWindowLevel];
        [window setHidesOnDeactivate:NO];
    }
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(char *currfont)
{
    /* Create the Color Chooser Dialog class. */
    static DWFontChoose *fontDlg = nil;
    static NSFontManager *fontManager = nil;
    DWDialog *dialog;
    NSFont *font = nil;

    if(currfont)
        font = _dw_font_by_name(currfont);

    if(fontDlg)
    {
        dialog = [fontDlg dialog];
        /* If someone is already waiting just return */
        if(dialog)
        {
            return NULL;
        }
    }
    else
    {
        [NSFontManager setFontPanelFactory:[DWFontChoose class]];
        fontManager = [NSFontManager sharedFontManager];
        fontDlg = (DWFontChoose *)[fontManager fontPanel:YES];
    }

    dialog = dw_dialog_new(fontDlg);
    if(font)
        [fontManager setSelectedFont:font isMultiple:NO];
    else
        [fontManager setSelectedFont:[NSFont fontWithName:@"Helvetica" size:9.0] isMultiple:NO];
    [fontDlg setDialog:dialog];
    [fontDlg setFontManager:fontManager];
    [fontManager orderFrontFontPanel:fontManager];


    /* Wait for them to pick a color */
    font = (NSFont *)dw_dialog_wait(dialog);
    if(font)
    {
        NSString *fontname = [font displayName];
        NSString *output = [NSString stringWithFormat:@"%d.%s", (int)[font pointSize], [fontname UTF8String]];
        return strdup([output UTF8String]);
    }
    return NULL;
}

/* Internal function to return a pointer to an item struct
 * with information about the packing information regarding object.
 */
Item *_box_item(id object)
{
    /* Find the item within the box it is packed into */
    if([object isKindOfClass:[DWBox class]] || [object isKindOfClass:[DWGroupBox class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if([object isKindOfClass:[NSTableView class]] && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            Box *thisbox = [parent box];
            Item *thisitem = thisbox->items;
            int z;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    return &thisitem[z];
            }
        }
    }
    return NULL;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, char *fontname)
{
    NSFont *font = _dw_font_by_name(fontname);

    if(font)
    {
        id object = _text_handle(handle);
        if([object window])
        {
            [object lockFocus];
            [font set];
            [object unlockFocus];
        }
        if([object isMemberOfClass:[DWGroupBox class]])
        {
            [object setTitleFont:font];
        }
        else if([object isKindOfClass:[NSControl class]])
        {
            [object setFont:font];
            [[object cell] setFont:font];
        }
        else if([object isMemberOfClass:[DWRender class]])
        {
            DWRender *render = object;

            [render setFont:font];
        }
        else
            return DW_ERROR_UNKNOWN;
        /* If we changed the text... */
        Item *item = _box_item(handle);

        /* Check to see if any of the sizes need to be recalculated */
        if(item && (item->origwidth == -1 || item->origheight == -1))
        {
            _control_size(handle, item->origwidth == -1 ? &item->width : NULL, item->origheight == -1 ? &item->height : NULL);
            /* Queue a redraw on the top-level window */
            _dw_redraw([object window], TRUE);
        }
        return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Returns the current font for the specified window
 * Parameters:
 *           handle: The window handle from which to obtain the font.
 */
char * API dw_window_get_font(HWND handle)
{
    id object = _text_handle(handle);
    NSFont *font = nil;

    if([object isMemberOfClass:[DWGroupBox class]])
    {
        font = [object titleFont];
    }
    else if([object isKindOfClass:[NSControl class]] || [object isMemberOfClass:[DWRender class]])
    {
         font = [object font];
    }
    if(font)
    {
        NSString *fontname = [font displayName];
        NSString *output = [NSString stringWithFormat:@"%d.%s", (int)[font pointSize], [fontname UTF8String]];
        return strdup([output UTF8String]);
    }
    return NULL;
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int API dw_window_destroy(HWND handle)
{
    int _locked_by_me = FALSE;
    DW_LOCAL_POOL_IN;
    DW_MUTEX_LOCK;
    id object = handle;

    /* Handle destroying a top-level window */
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        DWWindow *window = handle;
        [window close];
    }
    /* Handle removing menu items from menus */
    else if([ object isKindOfClass:[NSMenuItem class]])
    {
        NSMenu *menu = [object menu];

        [menu removeItem:object];
    }
    /* Handle destroying a control or box */
    else if([object isKindOfClass:[NSView class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if(([object isKindOfClass:[NSTableView class]] || [object isMemberOfClass:[DWMLE class]])
            && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            id window = [object window];
            Box *thisbox = [parent box];
            int z, index = -1;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            if(!thisitem)
                thisbox->count = 0;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    index = z;
            }

            if(index == -1)
            {
                DW_MUTEX_UNLOCK;
                DW_LOCAL_POOL_OUT;
                return 0;
            }

            [object removeFromSuperview];

            if(thisbox->count > 1)
            {
                tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                /* Copy all but the current entry to the new list */
                for(z=0;z<index;z++)
                {
                    tmpitem[z] = thisitem[z];
                }
                for(z=index+1;z<thisbox->count;z++)
                {
                    tmpitem[z-1] = thisitem[z];
                }
            }

            thisbox->items = tmpitem;
            if(thisitem)
                free(thisitem);
            if(tmpitem)
                thisbox->count--;
            else
                thisbox->count = 0;

            /* Queue a redraw on the top-level window */
            _dw_redraw(window, TRUE);
        }
    }
    DW_MUTEX_UNLOCK;
    DW_LOCAL_POOL_OUT;
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
    id object = _text_handle(handle);

    if([ object isKindOfClass:[ NSWindow class ] ] || [ object isKindOfClass:[ NSButton class ] ])
    {
        id window = object;
        NSString *nsstr = [ window title];

        return strdup([ nsstr UTF8String ]);
    }
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = object;
        NSString *nsstr = [ control stringValue];

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
    id object = _text_handle(handle);

    if([ object isKindOfClass:[ NSWindow class ] ] || [ object isKindOfClass:[ NSButton class ] ])
        [object setTitle:[ NSString stringWithUTF8String:text ]];
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = object;
        [control setStringValue:[ NSString stringWithUTF8String:text ]];
    }
    else if([object isMemberOfClass:[DWGroupBox class]])
    {
       DWGroupBox *groupbox = object;
       [groupbox setTitle:[NSString stringWithUTF8String:text]];
    }
    else
        return;
    /* If we changed the text... */
    Item *item = _box_item(handle);

    /* Check to see if any of the sizes need to be recalculated */
    if(item && (item->origwidth == -1 || item->origheight == -1))
    {
      int newwidth, newheight;

      _control_size(handle, &newwidth, &newheight);

      /* Only update the item and redraw the window if it changed */
      if((item->origwidth == -1 && item->width != newwidth) ||
         (item->origheight == -1 && item->height != newheight))
      {
         if(item->origwidth == -1)
            item->width = newwidth;
         if(item->origheight == -1)
            item->height = newheight;
         /* Queue a redraw on the top-level window */
         _dw_redraw([object window], TRUE);
      }
    }
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, char *bubbletext)
{
    id object = handle;
    if(bubbletext && *bubbletext)
        [object setToolTip:[NSString stringWithUTF8String:bubbletext]];
    else
        [object setToolTip:nil];
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
    id object = handle;

    if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = handle;
        object = [sv documentView];
    }
    if([object isKindOfClass:[NSControl class]] || [object isKindOfClass:[NSMenuItem class]])
    {
        [object setEnabled:NO];
    }
    if([object isKindOfClass:[NSTextView class]])
    {
        NSTextView *mle = object;

        [mle setEditable:NO];
    }
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
    id object = handle;

    if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = handle;
        object = [sv documentView];
    }
    if([object isKindOfClass:[NSControl class]] || [object isKindOfClass:[NSMenuItem class]])
    {
        [object setEnabled:YES];
    }
    if([object isKindOfClass:[NSTextView class]])
    {
        NSTextView *mle = object;

        [mle setEditable:YES];
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
void API dw_window_set_bitmap_from_data(HWND handle, unsigned long cid, char *data, int len)
{
    id object = handle;

    if([ object isKindOfClass:[ NSImageView class ] ] || [ object isKindOfClass:[ NSButton class ]])
    {
        if(data)
        {
            DW_LOCAL_POOL_IN;
            NSData *thisdata = [NSData dataWithBytes:data length:len];
            NSImage *pixmap = [[[NSImage alloc] initWithData:thisdata] autorelease];

            if(pixmap)
            {
                [object setImage:pixmap];
            }
            /* If we changed the bitmap... */
            Item *item = _box_item(handle);

            /* Check to see if any of the sizes need to be recalculated */
            if(item && (item->origwidth == -1 || item->origheight == -1))
            {
                _control_size(handle, item->origwidth == -1 ? &item->width : NULL, item->origheight == -1 ? &item->height : NULL);
                /* Queue a redraw on the top-level window */
                _dw_redraw([object window], TRUE);
            }
            DW_LOCAL_POOL_OUT;
        }
        else
            dw_window_set_bitmap(handle, cid, NULL);
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
void API dw_window_set_bitmap(HWND handle, unsigned long resid, char *filename)
{
    id object = handle;
    DW_LOCAL_POOL_IN;

    if([ object isKindOfClass:[ NSImageView class ] ] || [ object isKindOfClass:[ NSButton class ]])
    {
        NSImage *bitmap = nil;

        if(filename)
        {
            char *ext = _dw_get_image_extension( filename );
            NSString *nstr = [ NSString stringWithUTF8String:filename ];

            bitmap = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];

            if(!bitmap && ext)
            {
                nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
                bitmap = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
            }
        }
        if(!bitmap && resid > 0 && resid < 65536)
        {
            bitmap = _dw_icon_load(resid);
        }

        if(bitmap)
        {
            [object setImage:bitmap];

            /* If we changed the bitmap... */
            Item *item = _box_item(handle);

            /* Check to see if any of the sizes need to be recalculated */
            if(item && (item->origwidth == -1 || item->origheight == -1))
            {
                _control_size(handle, item->origwidth == -1 ? &item->width : NULL, item->origheight == -1 ? &item->height : NULL);
                /* Queue a redraw on the top-level window */
                _dw_redraw([object window], TRUE);
            }
        }
    }
    DW_LOCAL_POOL_OUT;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void API dw_window_set_icon(HWND handle, HICN icon)
{
    /* This isn't needed, it is loaded from the bundle */
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int cid)
{
    NSObject *object = handle;
    NSView *view = handle;
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = handle;
        view = [window contentView];
    }
    return [view viewWithTag:cid];
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
    DWWindow *window = handle;
    [window setRedraw:YES];
    [[window contentView] showWindow];
    [window flushWindow];
    [window setRedraw:NO];
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        Box *thisbox;
        NSRect content, frame = NSMakeRect(0, 0, width, height);

        /* Convert the external frame size to internal content size */
        content = [NSWindow contentRectForFrameRect:frame styleMask:[window styleMask]];

        /*
         * The following is an attempt to dynamically size a window based on the size of its
         * children before realization. Only applicable when width or height is less than one.
         */
        if((width < 1 || height < 1) && (thisbox = (Box *)[[window contentView] box]))
        {
            int depth = 0;

            /* Calculate space requirements */
            _resize_box(thisbox, &depth, (int)width, (int)height, 1);

            /* Update components that need auto-sizing */
            if(width < 1) content.size.width = thisbox->minwidth;
            if(height < 1) content.size.height = thisbox->minheight;
        }

        /* Finally set the size */
        [window setContentSize:content.size];
    }
    DW_MUTEX_UNLOCK;
}

/*
 * Gets the size the system thinks the widget should be.
 * Parameters:
 *       handle: Window handle of the item to be back.
 *       width: Width in pixels of the item or NULL if not needed.
 *       height: Height in pixels of the item or NULL if not needed.
 */
void API dw_window_get_preferred_size(HWND handle, int *width, int *height)
{
    id object = handle;

    if([object isMemberOfClass:[DWWindow class]])
    {
        Box *thisbox;

        if((thisbox = (Box *)[[object contentView] box]))
        {
            int depth = 0;
            NSRect frame;

            /* Calculate space requirements */
            _resize_box(thisbox, &depth, 0, 0, 1);

            /* Figure out the border size */
            frame = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, thisbox->minwidth, thisbox->minheight) styleMask:[object styleMask]];

            /* Return what was requested */
            if(width) *width = frame.size.width;
            if(height) *height = frame.size.height;
        }
    }
    else if([object isMemberOfClass:[DWBox class]])
    {
        Box *thisbox;

        if((thisbox = (Box *)[object box]))
        {
            int depth = 0;

            /* Calculate space requirements */
            _resize_box(thisbox, &depth, 0, 0, 1);

            /* Return what was requested */
            if(width) *width = thisbox->minwidth;
            if(height) *height = thisbox->minheight;
        }
    }
    else
        _control_size(handle, width, height);
}

/*
 * Sets the gravity of a given window (widget).
 * Gravity controls which corner of the screen and window the position is relative to.
 * Parameters:
 *          handle: Window (widget) handle.
 *          horz: DW_GRAV_LEFT (default), DW_GRAV_RIGHT or DW_GRAV_CENTER.
 *          vert: DW_GRAV_TOP (default), DW_GRAV_BOTTOM or DW_GRAV_CENTER.
 */
void API dw_window_set_gravity(HWND handle, int horz, int vert)
{
    dw_window_set_data(handle, "_dw_grav_horz", DW_INT_TO_POINTER(horz));
    dw_window_set_data(handle, "_dw_grav_vert", DW_INT_TO_POINTER(vert));
}

/* Convert the coordinates based on gravity */
void _handle_gravity(HWND handle, long *x, long *y, unsigned long width, unsigned long height)
{
    int horz = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_horz"));
    int vert = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_vert"));
    id object = handle;

    /* Do any gravity calculations */
    if(horz || (vert & 0xf) != DW_GRAV_BOTTOM)
    {
        long newx = *x, newy = *y;

        /* Handle horizontal center gravity */
        if((horz & 0xf) == DW_GRAV_CENTER)
            newx += ((dw_screen_width() / 2) - (width / 2));
        /* Handle right gravity */
        else if((horz & 0xf) == DW_GRAV_RIGHT)
            newx = dw_screen_width() - width - *x;
        /* Handle vertical center gravity */
        if((vert & 0xf) == DW_GRAV_CENTER)
            newy += ((dw_screen_height() / 2) - (height / 2));
        else if((vert & 0xf) == DW_GRAV_TOP)
            newy = dw_screen_height() - height - *y;

        /* Save the new values */
        *x = newx;
        *y = newy;
    }
    /* Adjust the values to avoid Dock/Menubar if requested */
    if((horz | vert) & DW_GRAV_OBSTACLES)
    {
        NSRect visiblerect = [[object screen] visibleFrame];
        NSRect totalrect = [[object screen] frame];

        if(horz & DW_GRAV_OBSTACLES)
        {
            if((horz & 0xf) == DW_GRAV_LEFT)
                *x += visiblerect.origin.x;
            else if((horz & 0xf) == DW_GRAV_RIGHT)
                *x -= (totalrect.origin.x + totalrect.size.width) - (visiblerect.origin.x + visiblerect.size.width);
        }
        if(vert & DW_GRAV_OBSTACLES)
        {
            if((vert & 0xf) == DW_GRAV_BOTTOM)
                *y += visiblerect.origin.y;
            else if((vert & 0xf) == DW_GRAV_TOP)
                *y -= (totalrect.origin.y + totalrect.size.height) - (visiblerect.origin.y + visiblerect.size.height);
        }
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
    int _locked_by_me = FALSE;
    DW_MUTEX_LOCK;
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        NSPoint point;
        NSSize size = [[window contentView] frame].size;

        /* Can't position an unsized window, so attempt to auto-size */
        if(size.width <= 1 || size.height <= 1)
        {
            /* Determine the contents size */
            dw_window_set_size(handle, 0, 0);
        }

        size = [window frame].size;
        _handle_gravity(handle, &x, &y, (unsigned long)size.width, (unsigned long)size.height);

        point.x = x;
        point.y = y;

        [window setFrameOrigin:point];
        /* Position set manually... don't auto-position */
        [window setShown:YES];
    }
    DW_MUTEX_UNLOCK;
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
    dw_window_set_size(handle, width, height);
    dw_window_set_pos(handle, x, y);
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
            *y = [[window screen] frame].size.height - rect.origin.y - rect.size.height;
        if(width)
            *width = rect.size.width;
        if(height)
            *height = rect.size.height;
        return;
    }
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = handle;
        NSRect rect = [control frame];
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

    uname(&name);
    strcpy(env->osName, "MacOS");

    strcpy(env->buildDate, __DATE__);
    strcpy(env->buildTime, __TIME__);
    env->DWMajorVersion = DW_MAJOR_VERSION;
    env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
    env->DWSubVersion = VER_REV;
#else
    env->DWSubVersion = DW_SUB_VERSION;
#endif

    env->MajorVersion = DWOSMajor;
    env->MinorVersion = DWOSMinor;
    env->MajorBuild = DWOSBuild;
    env->MinorBuild = 0;
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
    /* This may need to be thread specific */
    [DWObj performSelectorOnMainThread:@selector(doFlush:) withObject:nil waitUntilDone:NO];
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
                UserData *prev = *root, *tmp = prev->next;

                while(tmp)
                {
                    prev = tmp;
                    tmp = tmp->next;
                }
                prev->next = new;
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
    if([object isMemberOfClass:[DWWindow class]])
    {
        NSWindow *win = window;
        object = [win contentView];
    }
    else if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = window;
        object = [sv documentView];
    }
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
    if([object isMemberOfClass:[DWWindow class]])
    {
        NSWindow *win = window;
        object = [win contentView];
    }
    else if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = window;
        object = [sv documentView];
    }
    WindowData *blah = (WindowData *)[object userdata];

    if(blah && blah->root && dataname)
    {
        UserData *ud = _find_userdata(&(blah->root), dataname);
        if(ud)
            return ud->data;
    }
    return NULL;
}

#define DW_TIMER_MAX 64
NSTimer *DWTimers[DW_TIMER_MAX];

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
    int z;

    for(z=0;z<DW_TIMER_MAX;z++)
    {
        if(!DWTimers[z])
        {
            break;
        }
    }

    if(sigfunc && !DWTimers[z])
    {
        NSTimeInterval seconds = (double)interval / 1000.0;
        NSTimer *thistimer = DWTimers[z] = [NSTimer scheduledTimerWithTimeInterval:seconds target:DWHandler selector:@selector(runTimer:) userInfo:nil repeats:YES];
        _new_signal(0, thistimer, z+1, sigfunc, NULL, data);
        return z+1;
    }
    return 0;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(int timerid)
{
    SignalHandler *prev = NULL, *tmp = Root;
    NSTimer *thistimer;

    /* 0 is an invalid timer ID */
    if(timerid < 1 || !DWTimers[timerid-1])
        return;

    thistimer = DWTimers[timerid-1];
    DWTimers[timerid-1] = nil;

    [thistimer invalidate];

    while(tmp)
    {
        if(tmp->id == timerid && tmp->window == thistimer)
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
    dw_signal_connect_data(window, signame, sigfunc, NULL, data);
}

/*
 * Add a callback to a window event with a closure callback.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       discfunc: The pointer to the function called when this handler is removed.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect_data(HWND window, char *signame, void *sigfunc, void *discfunc, void *data)
{
    ULONG message = 0, msgid = 0;

    /* Handle special case of application delete signal */
    if(!window && signame && strcmp(signame, DW_SIGNAL_DELETE) == 0)
    {
        window = DWApp;
    }

    if(window && signame && sigfunc)
    {
        if((message = _findsigmessage(signame)) != 0)
        {
            _new_signal(message, window, (int)msgid, sigfunc, discfunc, data);
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
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

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
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

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
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

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
   int z, len = (int)strlen(buf);

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
   char errorbuf[1025];


   if(!handle)
      return -1;

   if((len = (int)strlen(name)) == 0)
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
    /* We need to handle locks from the main thread differently...
     * since we can't stop message processing... otherwise we
     * will deadlock... so try to acquire the lock and continue
     * processing messages in between tries.
     */
    if(DWThread == pthread_self())
    {
        while(pthread_mutex_trylock(mutex) != 0)
        {
            /* Process any pending events */
            while(_dw_main_iteration([NSDate dateWithTimeIntervalSinceNow:0.01]))
            {
                /* Just loop */
            }
        }
    }
    else
    {
        pthread_mutex_lock(mutex);
    }
}

/*
 * Tries to gain access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 * Returns:
 *       DW_ERROR_NONE on success, DW_ERROR_TIMEOUT if it is already locked.
 */
int API dw_mutex_trylock(HMTX mutex)
{
    if(pthread_mutex_trylock(mutex) == 0)
        return DW_ERROR_NONE;
    return DW_ERROR_TIMEOUT;
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
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   pthread_cond_init (&(eve->event), NULL);
   eve->posted = 0;
   pthread_mutex_unlock (&(eve->mutex));
   return DW_ERROR_NONE;
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

    if(!eve)
        return DW_ERROR_NON_INIT;

    pthread_mutex_lock (&(eve->mutex));

    if(eve->posted)
    {
        pthread_mutex_unlock (&(eve->mutex));
        return DW_ERROR_NONE;
    }

    if(timeout != -1)
    {
        struct timeval now;
        struct timespec timeo;

        gettimeofday(&now, 0);
        timeo.tv_sec = now.tv_sec + (timeout / 1000);
        timeo.tv_nsec = now.tv_usec * 1000;
        rc = pthread_cond_timedwait(&(eve->event), &(eve->mutex), &timeo);
    }
    else
        rc = pthread_cond_wait(&(eve->event), &(eve->mutex));
    pthread_mutex_unlock (&(eve->mutex));
    if(!rc)
        return DW_ERROR_NONE;
    if(rc == ETIMEDOUT)
        return DW_ERROR_TIMEOUT;
    return DW_ERROR_GENERAL;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int dw_event_close(HEV *eve)
{
   if(!eve || !(*eve))
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&((*eve)->mutex));
   pthread_cond_destroy (&((*eve)->event));
   pthread_mutex_unlock (&((*eve)->mutex));
   pthread_mutex_destroy (&((*eve)->mutex));
   free(*eve);
   *eve = NULL;

   return DW_ERROR_NONE;
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
      {
          free(array);
          return;
      }

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
            if((bytesread = (int)read(array[z].fd, &command, 1)) < 1)
            {
               struct _seminfo *newarray = NULL;

               /* Remove this connection from the set */
               if(connectcount > 1)
               {
                   newarray = (struct _seminfo *)malloc(sizeof(struct _seminfo)*(connectcount-1));
                   if(!z)
                       memcpy(newarray, &array[1], sizeof(struct _seminfo)*(connectcount-1));
                   else
                   {
                       memcpy(newarray, array, sizeof(struct _seminfo)*z);
                       if(z!=(connectcount-1))
                           memcpy(&newarray[z], &array[z+1], sizeof(struct _seminfo)*(z-connectcount-1));
                   }
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
   struct timeval tv, *useme = NULL;
   int retval = 0;
   char tmp;

   if(!eve || eve->alive < 0)
      return DW_ERROR_NON_INIT;

   /* Set the timout or infinite */
   if(timeout != -1)
   {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (int)timeout % 1000;

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

/* Mac specific function to cause garbage collection */
void _dw_pool_drain(void)
{
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
    pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
    /* If we aren't using garbage collection we need autorelease pools */
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif
    _init_colors();
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
    NSColor *color;

    /* Release the pool when we are done so we don't leak */
    color = pthread_getspecific(_dw_fg_color_key);
    [color release];
    color = pthread_getspecific(_dw_bg_color_key);
    [color release];
#if !defined(GARBAGE_COLLECT)
    pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
#endif
}

/*
 * Setup thread independent pools.
 */
void _dwthreadstart(void *data)
{
    void (*threadfunc)(void *) = NULL;
    void **tmp = (void **)data;

    _dw_init_thread();

    threadfunc = (void (*)(void *))tmp[0];

    /* Start our thread function */
    threadfunc(tmp[1]);

    free(tmp);

    _dw_deinit_thread();
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(char *fontname)
{
    NSFont *oldfont = DWDefaultFont;
    DWDefaultFont = _dw_font_by_name(fontname);
    [DWDefaultFont retain];
    [oldfont release];
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
    char *lang = getenv("LANG");

    /* Correct the startup path if run from a bundle */
    if(argc > 0 && argv[0])
    {
        char *pathcopy = strdup(argv[0]);
        char *app = strstr(pathcopy, ".app/");

        if(app)
        {
            char pathbuf[PATH_MAX+1] = { 0 };
            size_t len = (size_t)(app - pathcopy);

            if(len > 0)
                strncpy(_dw_bundle_path, pathcopy, len + 4);
            *app = 0;

            getcwd(pathbuf, PATH_MAX);

            /* If run from a bundle the path seems to be / */
            if(strcmp(pathbuf, "/") == 0)
            {
                char *pos = strrchr(pathcopy, '/');

                if(pos)
                {
                    strncpy(pathbuf, pathcopy, (size_t)(pos - pathcopy));
                    chdir(pathbuf);
                }
            }
        }
        if(pathcopy)
            free(pathcopy);
    }

    /* Just in case we can't obtain a path */
    if(!_dw_bundle_path[0])
        getcwd(_dw_bundle_path, PATH_MAX);

    /* Get the operating system version */
    NSString *version = [[NSProcessInfo processInfo] operatingSystemVersionString];
    const char *versionstr = [version UTF8String];
    sscanf(versionstr, "Version %d.%d.%d", &DWOSMajor, &DWOSMinor, &DWOSBuild);
    /* Set the locale... if it is UTF-8 pass it
     * directly, otherwise specify UTF-8 explicitly.
     */
    setlocale(LC_ALL, lang && strstr(lang, ".UTF-8") ? lang : "UTF-8");
    /* Create the application object */
    DWApp = [NSApplication sharedApplication];
    DWAppDel *del = [[DWAppDel alloc] init];
    [DWApp setDelegate:del];
    /* Create object for handling timers */
    DWHandler = [[DWTimerHandler alloc] init];
    /* If we aren't using garbage collection we need autorelease pools */
#if !defined(GARBAGE_COLLECT)
    pthread_key_create(&_dw_pool_key, NULL);
    pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif
    pthread_key_create(&_dw_fg_color_key, NULL);
    pthread_key_create(&_dw_bg_color_key, NULL);
    _init_colors();
    /* Create a default main menu, with just the application menu */
    DWMainMenu = _generate_main_menu();
    [DWMainMenu retain];
    [DWApp setMainMenu:DWMainMenu];
    DWObj = [[DWObject alloc] init];
    DWDefaultFont = nil;
    /* Create mutexes for thread safety */
    DWRunMutex = dw_mutex_new();
    DWThreadMutex = dw_mutex_new();
    DWThreadMutex2 = dw_mutex_new();
    /* Use NSThread to start a dummy thread to initialize the threading subsystem */
    NSThread *thread = [[ NSThread alloc] initWithTarget:DWObj selector:@selector(uselessThread:) object:nil];
    [thread start];
    [thread release];
    [NSTextField setCellClass:[DWTextFieldCell class]];
    return 0;
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
   char namebuf[1025] = {0};
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

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
   char namebuf[1025];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

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

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       -1 on error.
 */
int dw_exec(char *program, int type, char **params)
{
    int ret = -1;

    if(type == DW_EXEC_GUI)
    {
        if(params && params[0] && params[1])
        {
            [[NSWorkspace sharedWorkspace] openFile:[NSString stringWithUTF8String:params[1]]
                                    withApplication:[NSString stringWithUTF8String:program]];
        }
        else
        {
            [[NSWorkspace sharedWorkspace] launchApplication:[NSString stringWithUTF8String:program]];
        }
        return 0;
    }

    if((ret = fork()) == 0)
    {
        int i;

        for (i = 3; i < 256; i++)
            close(i);
        setsid();

        if(type == DW_EXEC_CON)
        {
            char **tmpargs;

            if(!params)
            {
                tmpargs = malloc(sizeof(char *));
                tmpargs[0] = NULL;
            }
            else
            {
                int z = 0;

                while(params[z])
                {
                    z++;
                }
                tmpargs = malloc(sizeof(char *)*(z+3));
                z=0;
                tmpargs[0] = "xterm";
                tmpargs[1] = "-e";
                while(params[z])
                {
                    tmpargs[z+2] = params[z];
                    z++;
                }
                tmpargs[z+2] = NULL;
            }
            execvp("xterm", tmpargs);
            free(tmpargs);
        }
        /* If we got here exec failed */
        _exit(-1);
    }
    return ret;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int dw_browse(char *url)
{
    NSURL *myurl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    [[NSWorkspace sharedWorkspace] openURL:myurl];
    return DW_ERROR_NONE;
}

typedef struct _dwprint
{
    NSPrintInfo *pi;
    int (*drawfunc)(HPRINT, HPIXMAP, int, void *);
    void *drawdata;
    unsigned long flags;
} DWPrint;

/*
 * Creates a new print object.
 * Parameters:
 *       jobname: Name of the print job to show in the queue.
 *       flags: Flags to initially configure the print object.
 *       pages: Number of pages to print.
 *       drawfunc: The pointer to the function to be used as the callback.
 *       drawdata: User data to be passed to the handler function.
 * Returns:
 *       A handle to the print object or NULL on failure.
 */
HPRINT API dw_print_new(char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
    DWPrint *print;
    NSPrintPanel *panel;
    PMPrintSettings settings;
    NSPrintInfo *pi;

    if(!drawfunc || !(print = calloc(1, sizeof(DWPrint))))
    {
        return NULL;
    }

    if(!jobname)
        jobname = "Dynamic Windows Print Job";

    print->drawfunc = drawfunc;
    print->drawdata = drawdata;
    print->flags = flags;

    /* Get the page range */
    pi = [NSPrintInfo sharedPrintInfo];
    [pi setHorizontalPagination:NSFitPagination];
    [pi setHorizontallyCentered:YES];
    [pi setVerticalPagination:NSFitPagination];
    [pi setVerticallyCentered:YES];
    [pi setOrientation:DWPaperOrientationPortrait];
    [pi setLeftMargin:0.0];
    [pi setRightMargin:0.0];
    [pi setTopMargin:0.0];
    [pi setBottomMargin:0.0];

    settings = [pi PMPrintSettings];
    PMSetPageRange(settings, 1, pages);
    PMSetFirstPage(settings, 1, true);
    PMSetLastPage(settings, pages, true);
    PMPrintSettingsSetJobName(settings, (CFStringRef)[NSString stringWithUTF8String:jobname]);
    [pi updateFromPMPrintSettings];

    /* Create and show the print panel */
    panel = [NSPrintPanel printPanel];
    if(!panel || [panel runModalWithPrintInfo:pi] == DWModalResponseCancel)
    {
        free(print);
        return NULL;
    }
    /* Put the print info from the panel into the operation */
    print->pi = pi;

    return print;
}

/*
 * Runs the print job, causing the draw page callbacks to fire.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 *       flags: Flags to run the print job.
 * Returns:
 *       DW_ERROR_UNKNOWN on error or DW_ERROR_NONE on success.
 */
int API dw_print_run(HPRINT print, unsigned long flags)
{
    DWPrint *p = print;
    NSBitmapImageRep *rep, *rep2;
    NSPrintInfo *pi;
    NSPrintOperation *po;
    HPIXMAP pixmap, pixmap2;
    NSImage *image, *flipped;
    NSImageView *iv;
    NSSize size;
    PMPrintSettings settings;
    int x, result = DW_ERROR_UNKNOWN;
    UInt32 start, end;

    if(!p)
        return result;

    DW_LOCAL_POOL_IN;

    /* Figure out the printer/paper size */
    pi = p->pi;
    size = [pi paperSize];

    /* Get the page range */
    settings = [pi PMPrintSettings];
    PMGetFirstPage(settings, &start);
    if(start > 0)
        start--;
    PMGetLastPage(settings, &end);
    PMSetPageRange(settings, 1, 1);
    PMSetFirstPage(settings, 1, true);
    PMSetLastPage(settings, 1, true);
    [pi updateFromPMPrintSettings];

    /* Create an image view to print and a pixmap to draw into */
    iv = [[NSImageView alloc] init];
    pixmap = dw_pixmap_new(iv, (int)size.width, (int)size.height, 8);
    rep = pixmap->image;
    pixmap2 = dw_pixmap_new(iv, (int)size.width, (int)size.height, 8);
    rep2 = pixmap2->image;

    /* Create an image with the data from the pixmap
     * to go into the image view.
     */
    image = [[NSImage alloc] initWithSize:[rep size]];
    flipped = [[NSImage alloc] initWithSize:[rep size]];
    [image addRepresentation:rep];
    [flipped addRepresentation:rep2];
    [iv setImage:flipped];
    [iv setImageScaling:NSImageScaleProportionallyDown];
    [iv setFrameOrigin:NSMakePoint(0,0)];
    [iv setFrameSize:size];

    /* Create the print operation using the image view and
     * print info obtained from the panel in the last call.
     */
    po = [NSPrintOperation printOperationWithView:iv printInfo:pi];
    [po setShowsPrintPanel:NO];

    /* Cycle through each page */
    for(x=start; x<end && p->drawfunc; x++)
    {
        /* Call the application's draw function */
        p->drawfunc(print, pixmap, x, p->drawdata);
        if(p->drawfunc)
        {
           /* Internal representation is flipped... so flip again so we can print */
           _flip_image(image, rep2, size);
   #ifdef DEBUG_PRINT
           /* Save it to file to see what we have */
           NSData *data = [rep2 representationUsingType: NSPNGFileType properties: nil];
           [data writeToFile: @"print.png" atomically: NO];
   #endif
           /* Print the image view */
           [po runOperation];
           /* Fill the pixmap with white in case we are printing more pages */
           dw_color_foreground_set(DW_CLR_WHITE);
           dw_draw_rect(0, pixmap, TRUE, 0, 0, (int)size.width, (int)size.height);
        }
    }
    if(p->drawfunc)
        result = DW_ERROR_NONE;
    /* Free memory */
    [image release];
    [flipped release];
    dw_pixmap_destroy(pixmap);
    dw_pixmap_destroy(pixmap2);
    free(p);
    [iv release];
    DW_LOCAL_POOL_OUT;
    return result;
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
    DWPrint *p = print;

    if(p)
        p->drawfunc = NULL;
}

/*
 * Converts a UTF-8 encoded string into a wide string.
 * Parameters:
 *       utf8string: UTF-8 encoded source string.
 * Returns:
 *       Wide string that needs to be freed with dw_free()
 *       or NULL on failure.
 */
wchar_t * API dw_utf8_to_wchar(char *utf8string)
{
    size_t buflen = strlen(utf8string) + 1;
    wchar_t *temp = malloc(buflen * sizeof(wchar_t));
    if(temp)
        mbstowcs(temp, utf8string, buflen);
    return temp;
}

/*
 * Converts a wide string into a UTF-8 encoded string.
 * Parameters:
 *       wstring: Wide source string.
 * Returns:
 *       UTF-8 encoded string that needs to be freed with dw_free()
 *       or NULL on failure.
 */
char * API dw_wchar_to_utf8(wchar_t *wstring)
{
    size_t bufflen = 8 * wcslen(wstring) + 1;
    char *temp = malloc(bufflen);
    if(temp)
        wcstombs(temp, wstring, bufflen);
    return temp;
}
