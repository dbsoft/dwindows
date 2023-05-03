/*
 * Dynamic Windows:
 *          A GTK like implementation of the iOS GUI
 *
 * (C) 2011-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2011-2021 Mark Hessling <mark@rexx.org>
 * (C) 2017 Ralph Shane (Base tree view implementation)
 *
 * Requires 13.0 or later.
 * clang -g -o dwtest -D__IOS__ -I. dwtest.c ios/dw.m -framework UIKit -framework WebKit -framework Foundation -framework UserNotifications
 */
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <UserNotifications/UserNotifications.h>
#import <UniformTypeIdentifiers/UTDefines.h>
#import <UniformTypeIdentifiers/UTType.h>
#import <UniformTypeIdentifiers/UTCoreTypes.h>
#include "dw.h"
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>
#include <spawn.h>

/* Macros to handle local auto-release pools */
#define DW_LOCAL_POOL_IN NSAutoreleasePool *localpool = nil; \
        if(DWThread != (DWTID)-1 && pthread_self() != DWThread) \
            localpool = [[NSAutoreleasePool alloc] init];
#define DW_LOCAL_POOL_OUT if(localpool) [localpool drain];

/* Macros to encapsulate running functions on the main thread */
#define DW_FUNCTION_INIT
#define DW_FUNCTION_DEFINITION(func, rettype, ...)  void _##func(NSPointerArray *_args); \
rettype API func(__VA_ARGS__) { \
    DW_LOCAL_POOL_IN; \
    NSPointerArray *_args = [[NSPointerArray alloc] initWithOptions:NSPointerFunctionsOpaqueMemory]; \
    [_args addPointer:(void *)_##func];
#define DW_FUNCTION_ADD_PARAM1(param1) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM2(param1, param2) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM3(param1, param2, param3) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM4(param1, param2, param3, param4) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM5(param1, param2, param3, param4, param5) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM6(param1, param2, param3, param4, param5, param6) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM7(param1, param2, param3, param4, param5, param6, param7) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM8(param1, param2, param3, param4, param5, param6, param7, param8) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)&param8]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM9(param1, param2, param3, param4, param5, param6, param7, param8, param9) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)&param8]; \
    [_args addPointer:(void *)&param9]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_RESTORE_PARAM1(param1, vartype1) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:2]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:3];
#define DW_FUNCTION_RESTORE_PARAM2(param1, vartype1, param2, vartype2) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:3]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:4];
#define DW_FUNCTION_RESTORE_PARAM3(param1, vartype1, param2, vartype2, param3, vartype3) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:4]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:5];
#define DW_FUNCTION_RESTORE_PARAM4(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:5]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:6];
#define DW_FUNCTION_RESTORE_PARAM5(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:6]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:7];
#define DW_FUNCTION_RESTORE_PARAM6(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:7]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:8];
#define DW_FUNCTION_RESTORE_PARAM7(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:8]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:9];
#define DW_FUNCTION_RESTORE_PARAM8(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    vartype8 param8 = *((vartype8 *)[_args pointerAtIndex:8]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:9]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:10];
#define DW_FUNCTION_RESTORE_PARAM9(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    vartype8 param8 = *((vartype8 *)[_args pointerAtIndex:8]); \
    vartype9 param9 = *((vartype9 *)[_args pointerAtIndex:9]); \
    UIColor *__attribute__((__unused__))_dw_fg_color = (UIColor *)[_args pointerAtIndex:10]; \
    UIColor *__attribute__((__unused__))_dw_bg_color = (UIColor *)[_args pointerAtIndex:11];
#define DW_FUNCTION_END }
#define DW_FUNCTION_NO_RETURN(func) [DWObj safeCall:@selector(callBack:) withObject:_args]; \
    [_args release]; \
    DW_LOCAL_POOL_OUT; } \
void _##func(NSPointerArray *_args) {
#define DW_FUNCTION_RETURN(func, rettype) [DWObj safeCall:@selector(callBack:) withObject:_args]; {\
        void *tmp = [_args pointerAtIndex:[_args count]-1]; \
        rettype myreturn = *((rettype *)tmp); \
        free(tmp); \
        [_args release]; \
        DW_LOCAL_POOL_OUT; \
        return myreturn; }} \
void _##func(NSPointerArray *_args) {
#define DW_FUNCTION_RETURN_THIS(_retvar) { void *_myreturn = malloc(sizeof(_retvar)); \
    memcpy(_myreturn, (void *)&_retvar, sizeof(_retvar)); \
    [_args addPointer:_myreturn]; }}
#define DW_FUNCTION_RETURN_NOTHING }

unsigned long _dw_colors[] =
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
char *_dw_image_exts[NUM_EXTS+1] =
{
   ".png",
   ".ico",
   ".icns",
   ".gif",
   ".jpg",
   ".jpeg",
   ".tiff",
   ".bmp",
    NULL
};

char *_dw_get_image_extension(const char *filename)
{
   char *file = alloca(strlen(filename) + 6);
   int found_ext = 0,i;

   /* Try various extentions */
   for(i = 0; i < NUM_EXTS; i++)
   {
      strcpy(file, filename);
      strcat(file, _dw_image_exts[i]);
      if(access(file, R_OK ) == 0)
      {
         found_ext = 1;
         break;
      }
   }
   if(found_ext == 1)
   {
      return _dw_image_exts[i];
   }
   return NULL;
}

/* Return the RGB color regardless if a predefined color was passed */
unsigned long _dw_get_color(unsigned long thiscolor)
{
    if(thiscolor & DW_RGB_COLOR)
    {
        return thiscolor & ~DW_RGB_COLOR;
    }
    else if(thiscolor < 17)
    {
        return _dw_colors[thiscolor];
    }
    return 0;
}

/* Thread specific storage */
pthread_key_t _dw_pool_key;
pthread_key_t _dw_fg_color_key;
pthread_key_t _dw_bg_color_key;
static int DWOSMajor, DWOSMinor, DWOSBuild;
static char _dw_bundle_path[PATH_MAX+1] = { 0 };
static char _dw_app_id[_DW_APP_ID_SIZE+1]= {0};
static int _dw_dark_mode_state = DW_FEATURE_UNSUPPORTED;
static int _dw_container_mode = DW_CONTAINER_MODE_DEFAULT;
static CGPoint _dw_event_point = {0};
static NSMutableArray *_dw_toplevel_windows;

/* Create a default colors for a thread */
void _dw_init_colors(void)
{
    UIColor *fgcolor = [[UIColor grayColor] retain];
    pthread_setspecific(_dw_fg_color_key, fgcolor);
    pthread_setspecific(_dw_bg_color_key, NULL);
}

typedef struct _dwsighandler
{
    struct _dwsighandler   *next;
    ULONG message;
    HWND window;
    int id;
    void *signalfunction;
    void *discfunction;
    void *data;

} DWSignalHandler;

static DWSignalHandler *DWRoot = NULL;

/* Some internal prototypes */
static void _dw_do_resize(Box *thisbox, int x, int y);
void _dw_handle_resize_events(Box *thisbox);
int _dw_remove_userdata(UserData **root, const char *varname, int all);
int _dw_main_iteration(NSDate *date);
CGContextRef _dw_draw_context(id image, bool antialias);
typedef id (*DWIMP)(id, SEL, ...);

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

DWSignalHandler *_dw_get_handler(HWND window, int messageid)
{
    DWSignalHandler *tmp = DWRoot;

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

} DWSignalList;

/* List of signals */
static DWSignalList DWSignalTranslate[] = {
    { _DW_EVENT_CONFIGURE,       DW_SIGNAL_CONFIGURE },
    { _DW_EVENT_KEY_PRESS,       DW_SIGNAL_KEY_PRESS },
    { _DW_EVENT_BUTTON_PRESS,    DW_SIGNAL_BUTTON_PRESS },
    { _DW_EVENT_BUTTON_RELEASE,  DW_SIGNAL_BUTTON_RELEASE },
    { _DW_EVENT_MOTION_NOTIFY,   DW_SIGNAL_MOTION_NOTIFY },
    { _DW_EVENT_DELETE,          DW_SIGNAL_DELETE },
    { _DW_EVENT_EXPOSE,          DW_SIGNAL_EXPOSE },
    { _DW_EVENT_CLICKED,         DW_SIGNAL_CLICKED },
    { _DW_EVENT_ITEM_ENTER,      DW_SIGNAL_ITEM_ENTER },
    { _DW_EVENT_ITEM_CONTEXT,    DW_SIGNAL_ITEM_CONTEXT },
    { _DW_EVENT_LIST_SELECT,     DW_SIGNAL_LIST_SELECT },
    { _DW_EVENT_ITEM_SELECT,     DW_SIGNAL_ITEM_SELECT },
    { _DW_EVENT_SET_FOCUS,       DW_SIGNAL_SET_FOCUS },
    { _DW_EVENT_VALUE_CHANGED,   DW_SIGNAL_VALUE_CHANGED },
    { _DW_EVENT_SWITCH_PAGE,     DW_SIGNAL_SWITCH_PAGE },
    { _DW_EVENT_TREE_EXPAND,     DW_SIGNAL_TREE_EXPAND },
    { _DW_EVENT_COLUMN_CLICK,    DW_SIGNAL_COLUMN_CLICK },
    { _DW_EVENT_HTML_RESULT,     DW_SIGNAL_HTML_RESULT },
    { _DW_EVENT_HTML_CHANGED,    DW_SIGNAL_HTML_CHANGED },
    { _DW_EVENT_HTML_MESSAGE,    DW_SIGNAL_HTML_MESSAGE }
};

int _dw_event_handler1(id object, id event, int message)
{
    DWSignalHandler *handler = _dw_get_handler(object, message);
    /* NSLog(@"Event handler - type %d\n", message); */

    if(handler)
    {
        switch(message)
        {
            /* Timer event */
            case _DW_EVENT_TIMER:
            {
                int (* API timerfunc)(void *) = (int (* API)(void *))handler->signalfunction;

                if(!timerfunc(handler->data))
                    dw_timer_disconnect(handler->window);
                return 0;
            }
            /* Configure/Resize event */
            case _DW_EVENT_CONFIGURE:
            {
                int (*sizefunc)(HWND, int, int, void *) = handler->signalfunction;
                CGSize size;

                if([object isKindOfClass:[UIWindow class]])
                {
                    UIWindow *window = object;
                    size = [window frame].size;
                }
                else
                {
                    UIView *view = object;
                    size = [view frame].size;
                }

                if(size.width > 0 && size.height > 0)
                {
                    return sizefunc(object, size.width, size.height, handler->data);
                }
                return 0;
            }
            case _DW_EVENT_KEY_PRESS:
            {
                int (*keypressfunc)(HWND, char, int, int, void *, char *) = handler->signalfunction;
                int special = 0;
                NSString *nchar = @"";
                if (@available(iOS 13.4, *)) {
                    UIKey *key = event;
                    
                    nchar = [key charactersIgnoringModifiers];
                    special = (int)[key modifierFlags];
                } else {
                    // Probably won't even get here if not 13.4
                }
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
            case _DW_EVENT_BUTTON_PRESS:
            case _DW_EVENT_BUTTON_RELEASE:
            {
                int (* API buttonfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                /* Event will be nil when handling the context menu events...
                 * So button should default to 2 if event is nil
                 */
                int button = event ? 1 : 2;
                CGPoint p = _dw_event_point;

                if(event && [event isKindOfClass:[UIEvent class]])
                {
                    NSSet *touches = [event allTouches];

                    for(id touch in touches)
                    {
                        if((message == 3 && [touch phase] == UITouchPhaseBegan) ||
                           (message == 4 && [touch phase] == UITouchPhaseEnded))
                        {
                            p = [touch locationInView:[touch view]];
                            
                            if(@available(ios 13.4, *))
                            {
                                if([event buttonMask] & UIEventButtonMaskSecondary)
                                    button = 2;
                            }
                        }
                    }
                }

                return buttonfunc(object, (int)p.x, (int)p.y, button, handler->data);
            }
            /* Motion notify event */
            case _DW_EVENT_MOTION_NOTIFY:
            {
                int (* API motionfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                int button = 1;

                if(event && [event isKindOfClass:[UIEvent class]])
                {
                    NSSet *touches = [event allTouches];

                    for(id touch in touches)
                    {
                        if([touch phase] == UITouchPhaseMoved)
                        {
                            CGPoint p = [touch locationInView:[touch view]];

                            return motionfunc(object, (int)p.x, (int)p.y, button, handler->data);
                        }
                    }
                }
                return -1;
            }
            /* Window close event */
            case _DW_EVENT_DELETE:
            {
                int (* API closefunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;
                return closefunc(object, handler->data);
            }
            /* Window expose/draw event */
            case _DW_EVENT_EXPOSE:
            {
                DWExpose exp;
                int (* API exposefunc)(HWND, DWExpose *, void *) = (int (* API)(HWND, DWExpose *, void *))handler->signalfunction;
                CGRect rect = [object frame];

                exp.x = rect.origin.x;
                exp.y = rect.origin.y;
                exp.width = rect.size.width;
                exp.height = rect.size.height;
                int result = exposefunc(object, &exp, handler->data);
                return result;
            }
            /* Clicked event for buttons and menu items */
            case _DW_EVENT_CLICKED:
            {
                int (* API clickfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return clickfunc(object, handler->data);
            }
            /* Container class selection event */
            case _DW_EVENT_ITEM_ENTER:
            {
                int (*containerselectfunc)(HWND, char *, void *, void *) = handler->signalfunction;

                return containerselectfunc(handler->window, [event pointerAtIndex:0], handler->data,
                                           [event pointerAtIndex:1]);
            }
            /* Container context menu event */
            case _DW_EVENT_ITEM_CONTEXT:
            {
                int (* API containercontextfunc)(HWND, char *, int, int, void *, void *) = (int (* API)(HWND, char *, int, int, void *, void *))handler->signalfunction;
                char *text = (char *)[event pointerAtIndex:0];
                void *user = [event pointerAtIndex:1];
                int x = DW_POINTER_TO_INT([event pointerAtIndex:2]);
                int y = DW_POINTER_TO_INT([event pointerAtIndex:3]);

                return containercontextfunc(handler->window, text, x, y, handler->data, user);
            }
            /* Generic selection changed event for several classes */
            case _DW_EVENT_LIST_SELECT:
            case _DW_EVENT_VALUE_CHANGED:
            {
                int (* API valuechangedfunc)(HWND, int, void *) = (int (* API)(HWND, int, void *))handler->signalfunction;
                int selected = DW_POINTER_TO_INT(event);

                return valuechangedfunc(handler->window, selected, handler->data);;
            }
            /* Tree class selection event */
            case _DW_EVENT_ITEM_SELECT:
            {
                int (* API treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = (int (* API)(HWND, HTREEITEM, char *, void *, void *))handler->signalfunction;
                char *text = NULL;
                void *user = NULL;
                id item = nil;

                if([object isKindOfClass:[UITableView class]] && event)
                {
                    if([event isKindOfClass:[NSPointerArray class]])
                    {
                       /* The NSPointerArray count will be 2 for Containers */
                        text = [event pointerAtIndex:0];
                        user = [event pointerAtIndex:1];

                        /* The NSPointerArray count will be 3 for Trees */
                        if([event count] > 2)
                            item = [event pointerAtIndex:2];
                    }
                }

                return treeselectfunc(handler->window, item, text, handler->data, user);
            }
            /* Set Focus event */
            case _DW_EVENT_SET_FOCUS:
            {
                int (* API setfocusfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return setfocusfunc(handler->window, handler->data);
            }
            /* Notebook page change event */
            case _DW_EVENT_SWITCH_PAGE:
            {
                int (* API switchpagefunc)(HWND, unsigned long, void *) = (int (* API)(HWND, unsigned long, void *))handler->signalfunction;
                int pageid = DW_POINTER_TO_INT(event);

                return switchpagefunc(handler->window, pageid, handler->data);
            }
            /* Tree expand event */
            case _DW_EVENT_TREE_EXPAND:
            {
                int (* API treeexpandfunc)(HWND, HTREEITEM, void *) = (int (* API)(HWND, HTREEITEM, void *))handler->signalfunction;

                return treeexpandfunc(handler->window, (HTREEITEM)event, handler->data);
            }
            /* Column click event */
            case _DW_EVENT_COLUMN_CLICK:
            {
                int (* API clickcolumnfunc)(HWND, int, void *) = handler->signalfunction;
                int column_num = DW_POINTER_TO_INT(event);

                return clickcolumnfunc(handler->window, column_num, handler->data);
            }
            /* HTML result event */
            case _DW_EVENT_HTML_RESULT:
            {
                int (* API htmlresultfunc)(HWND, int, char *, void *, void *) = handler->signalfunction;
                void **params = (void **)event;
                NSString *result = params[0];

                return htmlresultfunc(handler->window, [result length] ? DW_ERROR_NONE : DW_ERROR_UNKNOWN, [result length] ?
                                      (char *)[result UTF8String] : NULL, params[1], handler->data);
            }
            /* HTML changed event */
            case _DW_EVENT_HTML_CHANGED:
            {
                int (* API htmlchangedfunc)(HWND, int, char *, void *) = handler->signalfunction;
                void **params = (void **)event;
                NSString *uri = params[1];

                return htmlchangedfunc(handler->window, DW_POINTER_TO_INT(params[0]), (char *)[uri UTF8String], handler->data);
            }
            /* HTML message event */
            case _DW_EVENT_HTML_MESSAGE:
            {
                int (* API htmlmessagefunc)(HWND, char *, char *, void *) = handler->signalfunction;
                WKScriptMessage *message = (WKScriptMessage *)event;

                return htmlmessagefunc(handler->window, (char *)[[message name] UTF8String], [[message body] isKindOfClass:[NSString class]] ?
                                       (char *)[[message body] UTF8String] : NULL, handler->data);
                }
        }
    }
    return -1;
}

/* Sub function to handle redraws */
int _dw_event_handler(id object, id event, int message)
{
    int ret = _dw_event_handler1(object, event, message);
    if(ret != -1)
        _dw_redraw(nil, FALSE);
    return ret;
}

/* Subclass for the Timer type */
@interface DWTimerHandler : NSObject { }
-(void)runTimer:(id)sender;
@end

@implementation DWTimerHandler
-(void)runTimer:(id)sender { _dw_event_handler(sender, nil, _DW_EVENT_TIMER); }
@end

@interface DWAppDel : UIResponder <UIApplicationDelegate> { }
-(void)applicationWillTerminate:(UIApplication *)application;
-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions;
@end

@implementation DWAppDel
-(void)applicationWillTerminate:(UIApplication *)application
{
    /* On iOS we can't prevent temrination, but send the notificatoin anyway */
    _dw_event_handler(application, nil, _DW_EVENT_DELETE);
}
-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions
{
    return true;
}
@end

static UIApplication *DWApp = nil;
static UIFont *DWDefaultFont;
static DWTimerHandler *DWHandler;
static NSMutableArray *_DWDirtyDrawables;
static DWTID DWThread = (DWTID)-1;
static HEV DWMainEvent;

/* Used for doing bitblts from the main thread */
typedef struct _dw_bitbltinfo
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
@interface DWObject : NSObject
{
    /* A normally hidden window, at the top of the view hierarchy.
     * Since iOS messageboxes and such require a view controller,
     * we show this hidden window when necessary and use it during
     * the creation of alerts and dialog boxes that don't have one.
     */
    UIWindow *hiddenWindow;
}
-(void)uselessThread:(id)sender;
-(void)menuHandler:(id)param;
-(void)doBitBlt:(id)param;
-(void)doFlush:(id)param;
-(UIWindow *)hiddenWindow;
@end

API_AVAILABLE(ios(13.0))
@interface DWMenu : NSObject
{
    UIMenu *menu;
    NSMutableArray *children;
    NSString *title;
    unsigned long tag;
    UIWindow *window;
}
-(id)initWithTag:(unsigned long)newtag;
-(void)setTitle:(NSString *)input;
-(UIMenu *)menu;
-(unsigned long)tag;
-(id)childWithTag:(unsigned long)tag;
-(void)addItem:(id)item;
-(void)removeItem:(id)item;
-(void)dealloc;
@end

API_AVAILABLE(ios(13.0))
@interface DWMenuItem : UIAction
{
    BOOL check, enabled;
    unsigned long tag;
    DWMenu *submenu, *menu;
    void *userdata;
}
-(void)setCheck:(BOOL)input;
-(void)setEnabled:(BOOL)input;
-(void)setTag:(unsigned long)input;
-(void)setSubmenu:(DWMenu *)input;
-(void)setMenu:(DWMenu *)input;
-(DWMenu *)submenu;
-(DWMenu *)menu;
-(BOOL)check;
-(BOOL)enabled;
-(unsigned long)tag;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)dealloc;
@end

/* So basically to implement our event handlers...
 * it looks like we are going to have to subclass
 * basically everything.  Was hoping to add methods
 * to the superclasses but it looks like you can
 * only add methods and no variables, which isn't
 * going to work. -Brian
 */

/* Subclass for a box type */
@interface DWBox : UIView
{
    Box *box;
    void *userdata;
}
-(id)init;
-(void)dealloc;
-(Box *)box;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)keyDown:(UIKey *)key API_AVAILABLE(ios(13.4));
@end

@implementation DWBox
-(id)init
{
    self = [super init];

    if (self)
    {
        box = calloc(1, sizeof(Box));
        box->type = DW_VERT;
        box->vsize = box->hsize = _DW_SIZE_EXPAND;
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
    _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    for(id object in [self subviews])
    {
        NSUInteger rc = [object retainCount];
        [object removeFromSuperview];
        /* TODO: Fix the cause of this...
         * DWButtons have a lower retain count than the other widgets...
         * so this release causes a crash. Figure out why it is lower...
         * and make the others that way too.
         */
        if(rc > 2)
            [object release];
    }
    [super dealloc];
}
-(Box *)box { return box; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)keyDown:(UIKey *)key  API_AVAILABLE(ios(13.4)){ _dw_event_handler(self, key, _DW_EVENT_KEY_PRESS); }
@end

@interface DWWindow : UIWindow
{
    DWMenu *windowmenu, *popupmenu;
    int shown;
    void *userdata;
}
-(void)sendEvent:(UIEvent *)theEvent;
-(void)keyDown:(UIKey *)key API_AVAILABLE(ios(13.4));
-(int)shown;
-(void)setShown:(int)val;
-(void)layoutSubviews;
-(void)setMenu:(DWMenu *)input;
-(void)setPopupMenu:(DWMenu *)input;
-(DWMenu *)menu;
-(DWMenu *)popupMenu;
-(void)updateMenu;
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWWindow
-(void)sendEvent:(UIEvent *)theEvent
{
   int rcode = -1;
   if([theEvent type] == UIEventTypePresses)
   {
      rcode = _dw_event_handler(self, theEvent, _DW_EVENT_KEY_PRESS);
   }
   if ( rcode != TRUE )
      [super sendEvent:theEvent];
}
-(void)keyDown:(UIKey *)key { }
-(int)shown { return shown; }
-(void)setShown:(int)val { shown = val; }
-(void)layoutSubviews { }
-(void)setMenu:(DWMenu *)input { [windowmenu release]; windowmenu = input; [windowmenu retain]; }
-(void)setPopupMenu:(DWMenu *)input { [popupmenu release]; popupmenu = input; [popupmenu retain]; }
-(void)updateMenu
{
    if(windowmenu)
    {
        NSArray *array = [[[self rootViewController] view] subviews];
        UINavigationBar *nav = nil;

        for(id obj in array)
        {
            if([obj isMemberOfClass:[UINavigationBar class]])
                nav = obj;
        }
        if(nav)
        {
            UINavigationItem *item = [[nav items] firstObject];

            if (@available(iOS 14.0, *)) {
                if(item && item.rightBarButtonItem && item.rightBarButtonItem.menu)
                    [item.rightBarButtonItem setMenu:[windowmenu menu]];
            }
        }
    }
}
-(DWMenu *)menu { return windowmenu; }
-(DWMenu *)popupMenu { return popupmenu; }
-(void)closeWindow:(id)sender
{
    if(_dw_event_handler(self, nil, _DW_EVENT_DELETE) < 1)
        dw_window_destroy(self);
}
-(void)dealloc
{
    UserData *root = userdata;
    _dw_remove_userdata(&root, NULL, TRUE);
    if(windowmenu)
        [windowmenu release];
    if(popupmenu)
        [popupmenu release];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
@end

@interface DWImage : NSObject
{
    UIImage *image;
    CGContextRef cgcontext;
}
-(id)initWithSize:(CGSize)size;
-(id)initWithCGImage:(CGImageRef)cgimage;
-(id)initWithUIImage:(UIImage *)newimage;
-(void)setImage:(UIImage *)input;
-(UIImage *)image;
-(CGContextRef)cgcontext;
-(void)dealloc;
@end

/* Subclass for a render area type */
@interface DWRender : UIView <UIContextMenuInteractionDelegate>
{
    void *userdata;
    UIFont *font;
    CGSize size;
    DWImage *cachedImage;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFont:(UIFont *)input;
-(UIFont *)font;
-(void)setSize:(CGSize)input;
-(CGSize)size;
-(DWImage *)cachedImage;
-(void)keyDown:(UIKey *)key API_AVAILABLE(ios(13.4));
-(UIContextMenuConfiguration *)contextMenuInteraction:(UIContextMenuInteraction *)interaction configurationForMenuAtLocation:(CGPoint)location;
@end

@implementation DWRender
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFont:(UIFont *)input { [font release]; font = input; [font retain]; }
-(UIFont *)font { return font; }
-(void)setSize:(CGSize)input {
    size = input;
    if(cachedImage)
    {
        DWImage *oldimage = cachedImage;
        UIImage *newimage;
        UIGraphicsBeginImageContext(self.frame.size);
        [[self layer] renderInContext:UIGraphicsGetCurrentContext()];
        newimage = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        cachedImage = [[DWImage alloc] initWithUIImage:newimage];
        [cachedImage retain];
        [oldimage release];
    }
}
-(CGSize)size { return size; }
-(DWImage *)cachedImage {
    if(!cachedImage)
    {
        UIImage *newimage;
        UIGraphicsBeginImageContext(self.frame.size);
        [[self layer] renderInContext:UIGraphicsGetCurrentContext()];
        newimage = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        cachedImage = [[DWImage alloc] initWithUIImage:newimage];
        [cachedImage retain];
    }
    /* Mark this render dirty if something is requesting it to draw */
    if(![_DWDirtyDrawables containsObject:self])
        [_DWDirtyDrawables addObject:self];
    return cachedImage;
}
-(void)drawRect:(CGRect)rect {
    _dw_event_handler(self, nil, _DW_EVENT_EXPOSE);
    if(cachedImage)
    {
        [[cachedImage image] drawInRect:self.bounds];
        [_DWDirtyDrawables removeObject:self];
        [self setNeedsDisplay];
    }
}
-(void)keyDown:(UIKey *)key  API_AVAILABLE(ios(13.4)){ _dw_event_handler(self, key, _DW_EVENT_KEY_PRESS); }
-(void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    _dw_event_handler(self, event, _DW_EVENT_BUTTON_PRESS);
    [super touchesBegan:touches withEvent:event];
}
-(void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    _dw_event_handler(self, event, _DW_EVENT_BUTTON_RELEASE);
    [super touchesEnded:touches withEvent:event];
}
-(void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    _dw_event_handler(self, event, _DW_EVENT_MOTION_NOTIFY);
    [super touchesMoved:touches withEvent:event];
}
-(UIContextMenuConfiguration *)contextMenuInteraction:(UIContextMenuInteraction *)interaction configurationForMenuAtLocation:(CGPoint)location
{
    DWWindow *window = (DWWindow *)[self window];
    UIContextMenuConfiguration *config = nil;

    _dw_event_point = location;
    _dw_event_handler(self, nil, _DW_EVENT_BUTTON_PRESS);

    if(window && [window popupMenu])
    {
        __block UIMenu *popupmenu = [[[window popupMenu] menu] retain];
        config = [UIContextMenuConfiguration configurationWithIdentifier:nil
                                                         previewProvider:nil
                                                          actionProvider:^(NSArray* suggestedAction){return popupmenu;}];
        [window setPopupMenu:nil];
    }
    return config;
}
-(void)dealloc {
    UserData *root = userdata;
    _dw_remove_userdata(&root, NULL, TRUE);
    [font release];
    dw_signal_disconnect_by_window(self);
    [cachedImage release];
    [_DWDirtyDrawables removeObject:self];
    [super dealloc];
}
-(BOOL)acceptsFirstResponder { return YES; }
@end

/* Subclass page renderer to implement page by page rendering */
@interface DWPrintPageRenderer : UIPrintPageRenderer
{
    int (*drawfunc)(HPRINT, HPIXMAP, int, void *);
    void *drawdata;
    unsigned long flags;
    unsigned int pages;
    HPRINT *print;
}
-(void)setDrawFunc:(void *)input;
-(void)setDrawData:(void *)input;
-(void *)drawData;
-(void)setFlags:(unsigned long)input;
-(unsigned long)flags;
-(void)setPages:(unsigned int)input;
-(void)setPrint:(HPRINT)input;
@end

@implementation DWPrintPageRenderer
-(void)setDrawFunc:(void *)input { drawfunc = input; }
-(void)setDrawData:(void *)input { drawdata = input; }
-(void *)drawData { return drawdata; }
-(void)setFlags:(unsigned long)input { flags = input; }
-(unsigned long)flags { return flags; }
-(void)setPages:(unsigned int)input { pages = input; }
-(void)setPrint:(HPRINT)input { print = input; }
-(void)drawContentForPageAtIndex:(NSInteger)pageIndex inRect:(CGRect)contentRect
{
    HPIXMAP pm = dw_pixmap_new(nil, contentRect.size.width, contentRect.size.height, 32);
    DWImage *image = pm->image;
    CGBlendMode op = kCGBlendModeNormal;

    drawfunc(print, pm, (int)pageIndex, drawdata);
    [[image image] drawInRect:contentRect blendMode:op alpha:1.0];
    dw_pixmap_destroy(pm);
}
-(NSInteger)numberOfPages { return (NSInteger)pages; }
@end

@interface DWFontPickerDelegate : UIResponder <UIFontPickerViewControllerDelegate>
{
    DWDialog *dialog;
}
-(void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)viewController;
-(void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)viewController;
-(void)setDialog:(DWDialog *)newdialog;
@end

@implementation DWFontPickerDelegate
-(void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)viewController
{
    if(viewController.selectedFontDescriptor)
    {
        UIFont *font = [UIFont fontWithDescriptor:viewController.selectedFontDescriptor size:9];
        dw_dialog_dismiss(dialog, font);
    }
    else
        dw_dialog_dismiss(dialog, NULL);
}
-(void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)viewController
{
    dw_dialog_dismiss(dialog, NULL);
}
-(void)setDialog:(DWDialog *)newdialog { dialog = newdialog; }
@end

@interface DWColorPickerDelegate : UIResponder <UIColorPickerViewControllerDelegate>
{
    DWDialog *dialog;
}
-(void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)viewController API_AVAILABLE(ios(14.0));
-(void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)viewController API_AVAILABLE(ios(14.0));
-(void)setDialog:(DWDialog *)newdialog;
@end

@implementation DWColorPickerDelegate
-(void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)viewController API_AVAILABLE(ios(14.0))
{
    if([viewController selectedColor])
    {
        CGFloat red, green, blue;
        [[viewController selectedColor] getRed:&red green:&green blue:&blue alpha:NULL];
        dw_dialog_dismiss(dialog, DW_UINT_TO_POINTER(DW_RGB((int)(red * 255), (int)(green *255), (int)(blue *255))));
    }
    else
        dw_dialog_dismiss(dialog, NULL);
    [viewController dismissViewControllerAnimated:YES completion:nil];
}
-(void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)viewController API_AVAILABLE(ios(14.0))
{
    dw_dialog_dismiss(dialog, NULL);
    [viewController dismissViewControllerAnimated:YES completion:nil];
}
-(void)setDialog:(DWDialog *)newdialog { dialog = newdialog; }
@end

@interface DWDocumentPickerDelegate : UIResponder <UIDocumentPickerDelegate>
{
    DWDialog *dialog;
}
-(void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls;
-(void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller;
-(void)setDialog:(DWDialog *)newdialog;
@end

#define _DW_URL_LIMIT 10
static NSMutableArray *_dw_URLs = nil;

@implementation DWDocumentPickerDelegate
-(void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
    NSURL *url = [urls firstObject];
    char *file = NULL;

    if(url)
    {
        const char *tmp = [url fileSystemRepresentation];

        if(tmp && [url startAccessingSecurityScopedResource])
        {
            /* We need to allow access to the returned URLs on iOS 13
             * but we have no way of knowing when it is no longer needed...
             * since the C API we are connected to does not have that information.
             * So maintain a list of URLs, and when the list exceeds the limit...
             * revoke access to the oldest URL.
             */
            if(!_dw_URLs)
                _dw_URLs = [[[NSMutableArray alloc] init] retain];
            if(_dw_URLs)
            {
                if([_dw_URLs count] >= _DW_URL_LIMIT)
                {
                    NSURL *oldurl = [_dw_URLs firstObject];
                    [oldurl stopAccessingSecurityScopedResource];
                    [_dw_URLs removeObject:oldurl];
                }
                [_dw_URLs addObject:url];
            }
            file = strdup(tmp);
        }
    }
    dw_dialog_dismiss(dialog, file);
}
-(void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
    dw_dialog_dismiss(dialog, NULL);
}
-(void)setDialog:(DWDialog *)newdialog { dialog = newdialog; }
@end

@implementation DWObject
-(id)init
{
    self = [super init];
    /* This previously had the code in delayedIinit: */
    return self;
}
-(void)delayedInit:(id)param
{
    /* When DWObject is initialized, UIApplicationMain() has not yet been called...
     * So the created objects can't interact with the user interface... therefore
     * we wait until UIApplicationMain() has been called and run this then.
     */
    hiddenWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    [hiddenWindow setWindowLevel:UIWindowLevelAlert+1];
    [hiddenWindow setHidden:YES];
    [hiddenWindow setRootViewController:[UIViewController new]];
    /* Handle setting the dark mode state now that DWObj is valid */
    if(_dw_dark_mode_state != DW_FEATURE_UNSUPPORTED)
        dw_feature_set(DW_FEATURE_DARK_MODE, _dw_dark_mode_state);
}
-(UIWindow *)hiddenWindow { return hiddenWindow; };
-(void)uselessThread:(id)sender { /* Thread only to initialize threading */ }
-(void)menuHandler:(id)param
{
    _dw_event_handler(param, nil, _DW_EVENT_CLICKED);
}
-(void)callBack:(NSPointerArray *)params
{
    void (*mycallback)(NSPointerArray *) = [params pointerAtIndex:0];
    if(mycallback)
        mycallback(params);
}
-(void)colorPicker:(NSMutableArray *)params
{
    if (@available(iOS 14.0, *))
    {
        DWDialog *dialog = dw_dialog_new(NULL);
        UIColorPickerViewController *picker = [[UIColorPickerViewController alloc] init];
        DWColorPickerDelegate *delegate = [[DWColorPickerDelegate alloc] init];
        unsigned long color = [[params firstObject] unsignedLongValue];

        /* Setup our picker */
        [picker setSupportsAlpha:NO];
        /* Unhide our hidden window and make it key */
        [hiddenWindow setHidden:NO];
        [hiddenWindow makeKeyAndVisible];
        [delegate setDialog:dialog];
        [picker setDelegate:delegate];
        [picker setSelectedColor:[UIColor colorWithRed: DW_RED_VALUE(color)/255.0 green: DW_GREEN_VALUE(color)/255.0 blue: DW_BLUE_VALUE(color)/255.0 alpha: 1]];
        /* Wait for them to pick a color */
        [[hiddenWindow rootViewController] presentViewController:picker animated:YES completion:nil];
        [params addObject:[NSNumber numberWithUnsignedLong:DW_POINTER_TO_UINT(dw_dialog_wait(dialog))]];
        /* Once the dialog is gone we can rehide our window */
        [hiddenWindow resignKeyWindow];
        [hiddenWindow setHidden:YES];
        [picker release];
        [delegate release];
    } else {
        // Fallback on earlier versions
    };
}
-(void)fontPicker:(NSPointerArray *)params
{
    DWDialog *dialog = dw_dialog_new(NULL);
    UIFontPickerViewController *picker = [[UIFontPickerViewController alloc] init];
    DWFontPickerDelegate *delegate = [[DWFontPickerDelegate alloc] init];
    UIFont *font;

    /* Unhide our hidden window and make it key */
    [hiddenWindow setHidden:NO];
    [hiddenWindow makeKeyAndVisible];
    [delegate setDialog:dialog];
    [picker setDelegate:delegate];
    /* Wait for them to pick a font */
    [[hiddenWindow rootViewController] presentViewController:picker animated:YES completion:nil];
    font = dw_dialog_wait(dialog);
    /* Once the dialog is gone we can rehide our window */
    [hiddenWindow resignKeyWindow];
    [hiddenWindow setHidden:YES];

    if(font)
    {
        NSString *fontname = [font fontName];
        NSString *output = [NSString stringWithFormat:@"%d.%s", (int)[font pointSize], [fontname UTF8String]];
        [params addPointer:strdup([output UTF8String])];
    }
    else
        [params addPointer:NULL];
    [picker release];
    [delegate release];
}
-(void)filePicker:(NSPointerArray *)params
{
    DWDialog *dialog = dw_dialog_new(NULL);
    UIDocumentPickerViewController *picker;
    DWDocumentPickerDelegate *delegate = [[DWDocumentPickerDelegate alloc] init];
    char *defpath = [params pointerAtIndex:0];
    char *ext = [params pointerAtIndex:1];
    int flags = DW_POINTER_TO_INT([params pointerAtIndex:2]);
    char *file = NULL;
    NSArray *UTIs;

    if (@available(iOS 14, *)) {
        UTType *extuti = ext ? [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:ext]] : nil;
        
        /* Try to generate a UTI for our passed extension */
        if(flags & DW_DIRECTORY_OPEN)
            UTIs = @[UTTypeFolder];
        else
        {
            if(extuti)
                UTIs = [NSArray arrayWithObjects:extuti, UTTypeText, nil];
            else
                UTIs = @[UTTypeText];
        }
        picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:UTIs
                                                                             asCopy:(flags & DW_FILE_SAVE ? YES: NO)];
    } else {
        UIDocumentPickerMode mode = UIDocumentPickerModeOpen;
        
        /* Try to generate a UTI for our passed extension */
        if(flags & DW_DIRECTORY_OPEN)
            UTIs = @[@"public.directory"];
        else
        {
            if(ext)
                UTIs = [NSArray arrayWithObjects:[NSString stringWithFormat:@"public.%s", ext], @"public.text", nil];
            else
                UTIs = @[@"public.text"];
        }
        
        /* Setup the picker */
        if(flags & DW_FILE_SAVE)
            mode = UIDocumentPickerModeExportToService;

        picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:UTIs inMode:mode];
    }
    [picker setAllowsMultipleSelection:NO];
    [picker setShouldShowFileExtensions:YES];
    if(defpath)
        [picker setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:defpath]]];
    /* Unhide our hidden window and make it key */
    [hiddenWindow setHidden:NO];
    [hiddenWindow makeKeyAndVisible];
    [delegate setDialog:dialog];
    [picker setDelegate:delegate];
    /* Wait for them to pick a file */
    [[hiddenWindow rootViewController] presentViewController:picker animated:YES completion:nil];
    file = dw_dialog_wait(dialog);
    /* Once the dialog is gone we can rehide our window */
    [hiddenWindow resignKeyWindow];
    [hiddenWindow setHidden:YES];
    [params addPointer:file];
    [picker release];
    [delegate release];
}
-(void)messageBox:(NSMutableArray *)params
{
    __block DWDialog *dialog = dw_dialog_new(NULL);
    NSInteger iResponse;
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:[params objectAtIndex:0]
                                   message:[params objectAtIndex:1]
                                   preferredStyle:[[params objectAtIndex:2] integerValue]];
    UIAlertAction* action = [UIAlertAction actionWithTitle:[params objectAtIndex:3] style:UIAlertActionStyleDefault
                                                   handler:^(UIAlertAction * action) { dw_dialog_dismiss(dialog, DW_INT_TO_POINTER(1)); }];
    [alert addAction:action];
    if([params count] > 4)
    {
        action = [UIAlertAction actionWithTitle:[params objectAtIndex:4] style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) { dw_dialog_dismiss(dialog, DW_INT_TO_POINTER(2)); }];
        [alert addAction:action];
    }
    if([params count] > 5)
    {
        action = [UIAlertAction actionWithTitle:[params objectAtIndex:5] style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) { dw_dialog_dismiss(dialog, DW_INT_TO_POINTER(3)); }];
        [alert addAction:action];
    }

    /* Unhide our hidden window and make it key */
    [hiddenWindow setHidden:NO];
    [hiddenWindow makeKeyAndVisible];
    [[hiddenWindow rootViewController] presentViewController:alert animated:YES completion:nil];
    iResponse = DW_POINTER_TO_INT(dw_dialog_wait(dialog));
    /* Once the dialog is gone we can rehide our window */
    [hiddenWindow resignKeyWindow];
    [hiddenWindow setHidden:YES];
    [params addObject:[NSNumber numberWithInteger:iResponse]];
}
-(void)printDialog:(NSMutableArray *)params
{
    __block DWDialog *dialog = dw_dialog_new(NULL);
    UIPrintInteractionController *pc = [UIPrintInteractionController sharedPrintController];
    UIPrintInfo *pi = [params firstObject];
    DWPrintPageRenderer *renderer = [params objectAtIndex:1];
    /* Not currently using the passed in flags, but when we need them...
     NSNumber *flags = [params objectAtIndex:2];*/
    int result = DW_ERROR_UNKNOWN;

    /* Setup our print dialog */
    [pc setPrintInfo:pi];
    [pc setPrintPageRenderer:renderer];

    /* Present the shared printer dialog */
    [pc presentAnimated:YES completionHandler:^(UIPrintInteractionController * _Nonnull printInteractionController, BOOL completed, NSError * _Nullable error) {
        dw_dialog_dismiss(dialog, DW_INT_TO_POINTER((completed ? DW_ERROR_NONE : DW_ERROR_UNKNOWN)));
    }];
    result = DW_POINTER_TO_INT(dw_dialog_wait(dialog));

    [params addObject:[NSNumber numberWithInteger:result]];
}
-(void)safeCall:(SEL)sel withObject:(id)param
{
    if([self respondsToSelector:sel])
    {
        DWTID curr = pthread_self();

        if(DWThread == (DWTID)-1 || DWThread == curr)
            [self performSelector:sel withObject:param];
        else
            [self performSelectorOnMainThread:sel withObject:param waitUntilDone:YES];
    }
}
-(void)updateMenu:(id)param
{
    for(id obj in _dw_toplevel_windows)
        [obj updateMenu];
}
-(void)doBitBlt:(id)param
{
    NSValue *bi = (NSValue *)param;
    DWBitBlt *bltinfo = (DWBitBlt *)[bi pointerValue];
    id bltdest = bltinfo->dest;
    id bltsrc = bltinfo->src;
    CGContextRef context = _dw_draw_context(bltdest, NO);

    if(context)
        UIGraphicsPushContext(context);

    if(bltdest && [bltsrc isMemberOfClass:[DWImage class]])
    {
        DWImage *rep = bltsrc;
        UIImage *image = [rep image];
        CGBlendMode op = kCGBlendModeNormal;

        if(bltinfo->srcwidth != -1)
        {
            [image drawInRect:CGRectMake(bltinfo->xdest, bltinfo->ydest, bltinfo->width, bltinfo->height)
                     /*fromRect:CGRectMake(bltinfo->xsrc, bltinfo->ysrc, bltinfo->srcwidth, bltinfo->srcheight)*/
                    blendMode:op alpha:1.0];
        }
        else
        {
            [image drawAtPoint:CGPointMake(bltinfo->xdest, bltinfo->ydest)
                      /*fromRect:CGRectMake(bltinfo->xsrc, bltinfo->ysrc, bltinfo->width, bltinfo->height)*/
                     blendMode:op alpha:1.0];
        }
    }
    if(context)
        UIGraphicsPopContext();
    free(bltinfo);
}
-(void)doFlush:(id)param
{
    NSEnumerator *enumerator = [_DWDirtyDrawables objectEnumerator];
    DWRender *rend;

    while(rend = [enumerator nextObject])
        [rend setNeedsDisplay];
    [_DWDirtyDrawables removeAllObjects];
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
-(void)getUserInterfaceStyle:(id)param
{
    NSMutableArray *array = param;
    UIUserInterfaceStyle overridestyle = [hiddenWindow overrideUserInterfaceStyle];
    int retval;
    
    switch(overridestyle)
    {
        case UIUserInterfaceStyleLight:
            retval = DW_DARK_MODE_DISABLED;
            break;
        case UIUserInterfaceStyleDark:
            retval = DW_DARK_MODE_FORCED;
            break;
        default:  /* UIUserInterfaceStyleUnspecified */
        {
            UIUserInterfaceStyle style = [[[hiddenWindow rootViewController] traitCollection] userInterfaceStyle];

            switch(style)
            {
                case UIUserInterfaceStyleLight:
                    retval = DW_DARK_MODE_BASIC;
                    break;
                case UIUserInterfaceStyleDark:
                    retval = DW_DARK_MODE_FULL;
                    break;
                default: /* UIUserInterfaceStyleUnspecified */
                    retval = DW_FEATURE_UNSUPPORTED;
                    break;
            }
        }
    }
    [array addObject:[NSNumber numberWithInt:retval]];
}
-(void)setUserInterfaceStyle:(id)param
{
    NSNumber *number = param;
    UIUserInterfaceStyle style = [number intValue];
    
    [hiddenWindow setOverrideUserInterfaceStyle:style];
}
@end

DWObject *DWObj;

/* Returns TRUE if iOS 12+ is in Dark Mode */
BOOL _dw_is_dark(void)
{
    if([[[[DWObj hiddenWindow] rootViewController] traitCollection] userInterfaceStyle] == UIUserInterfaceStyleDark)
        return YES;
    return NO;
}

@interface DWWebView : WKWebView <WKNavigationDelegate, WKScriptMessageHandler>
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation *)navigation;
-(void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message;
@end

@implementation DWWebView : WKWebView
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_STARTED), [[self URL] absoluteString] };
    _dw_event_handler(self, (id)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_COMPLETE), [[self URL] absoluteString] };
    _dw_event_handler(self, (id)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_LOADING), [[self URL] absoluteString] };
    _dw_event_handler(self, (id)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_REDIRECT), [[self URL] absoluteString] };
    _dw_event_handler(self, (id)params, _DW_EVENT_HTML_CHANGED);
}
-(void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    _dw_event_handler(self, (id)message, _DW_EVENT_HTML_MESSAGE);
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a top-level window */
@interface DWView : DWBox /* <UIWindowDelegate> */
{
    CGSize oldsize;
}
-(void)windowDidBecomeMain:(id)sender;
-(void)menuHandler:(id)sender;
-(UIResponder *)nextResponder;
@end

@implementation DWView
-(void)willMoveToSuperview:(UIView *)newSuperview
{
    if(newSuperview)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeMain:) name:UIWindowDidBecomeKeyNotification object:[newSuperview window]];
}
-(void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(void)windowResized:(CGSize)size;
{
    if(oldsize.width != size.width || oldsize.height != size.height)
    {
        _dw_do_resize(box, size.width, size.height);
        _dw_event_handler([self window], nil, _DW_EVENT_CONFIGURE);
        oldsize.width = size.width;
        oldsize.height = size.height;
        _dw_handle_resize_events(box);
    }
}
-(void)showWindow
{
    CGSize size = [self frame].size;

    if(oldsize.width == size.width && oldsize.height == size.height)
    {
        _dw_do_resize(box, size.width, size.height);
        _dw_handle_resize_events(box);
    }

}
-(void)windowDidBecomeMain:(id)sender
{
    _dw_event_handler([self window], nil, _DW_EVENT_SET_FOCUS);
}
-(void)menuHandler:(id)sender
{
    [DWObj menuHandler:sender];
}
-(UIResponder *)nextResponder { return [[self window] rootViewController]; }
@end

@interface DWViewController : UIViewController
-(void)viewWillLayoutSubviews;
-(UIResponder *)nextResponder;
@end


@implementation DWViewController : UIViewController {}
-(void)viewWillLayoutSubviews
{
    DWWindow *window = (DWWindow *)[[self view] window];
    NSArray *array = [[self view] subviews];
    CGRect frame = [window frame];
    DWView *view = nil;
    UINavigationBar *nav = nil;
    NSInteger sbheight = [[[window windowScene] statusBarManager] statusBarFrame].size.height;

    for(id obj in array)
    {
        if([obj isMemberOfClass:[DWView class]])
            view = obj;
        else if([obj isMemberOfClass:[UINavigationBar class]])
            nav = obj;
    }
    /* Adjust the frame to account for the status bar and navigation bar if it exists */
    if(nav)
    {
        CGRect navrect = [nav frame];
        
        navrect.size.width = frame.size.width;
        navrect.origin.x = 0;
        navrect.origin.y = sbheight;
        sbheight += navrect.size.height;
        [nav setFrame:navrect];
        
        if (@available(iOS 14.0, *)) {
            DWMenu *windowmenu = [window menu];
            UINavigationItem *item = [[nav items] firstObject];

            if(windowmenu && !item.rightBarButtonItem)
            {
                UIMenu *menu = [windowmenu menu];
                UIBarButtonItem *options = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"list.bullet"]
                                                                             menu:menu];
                item.rightBarButtonItem = options;
            }
        } else {
            // Fallback on earlier versions
        }
    }
    frame.size.height -= sbheight;
    /* Account for the special area on iPhone X and iPad Pro
     * https://blog.maxrudberg.com/post/165590234593/ui-design-for-iphone-x-bottom-elements
     */
    frame.size.height -= 24;
    frame.origin.y += sbheight;
    [view setFrame:frame];
    [view windowResized:frame.size];
}
-(UIResponder *)nextResponder { return [[self viewIfLoaded] window]; }
@end

#define _DW_BUTTON_TYPE_NORMAL 0
#define _DW_BUTTON_TYPE_CHECK  1
#define _DW_BUTTON_TYPE_RADIO  2
#define _DW_BUTTON_TYPE_TREE   3

/* Subclass for a button type */
@interface DWButton : UIButton
{
    void *userdata;
    DWBox *parent;
    int type, checkstate;
}
@property(nonatomic, strong) void(^didCheckedChanged)(BOOL checked);
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)buttonClicked:(id)sender;
-(void)setParent:(DWBox *)input;
-(DWBox *)parent;
-(int)type;
-(void)setType:(int)input;
-(int)checkState;
-(void)setCheckState:(int)input;
@end

@implementation DWButton
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)buttonClicked:(id)sender
{
    /* Toggle the button */
    if(type == _DW_BUTTON_TYPE_CHECK || type == _DW_BUTTON_TYPE_TREE)
        [self setCheckState:(checkstate ? FALSE : TRUE)];
    else if(type == _DW_BUTTON_TYPE_RADIO)
        [self setCheckState:TRUE];

    _dw_event_handler(self, nil, _DW_EVENT_CLICKED);

    /* If it is a radio button, uncheck all the other radio buttons in the box */
    if(type == _DW_BUTTON_TYPE_RADIO)
    {
        DWBox *viewbox = [self parent];
        Box *thisbox = [viewbox box];
        int z;

        for(z=0;z<thisbox->count;z++)
        {
            if(thisbox->items[z].type != _DW_TYPE_BOX)
            {
                id object = thisbox->items[z].hwnd;

                if([object isMemberOfClass:[DWButton class]])
                {
                    DWButton *button = object;

                    if(button != self && [button type] == _DW_BUTTON_TYPE_RADIO)
                    {
                        [button setCheckState:FALSE];
                    }
                }
            }
        }
    }
    if(_didCheckedChanged)
        _didCheckedChanged(checkstate);
}
-(void)setParent:(DWBox *)input { parent = input; }
-(DWBox *)parent { return parent; }
-(int)type { return type; }
-(void)setType:(int)input
{
    type = input;
    if(type == _DW_BUTTON_TYPE_NORMAL)
        [self setContentHorizontalAlignment:UIControlContentHorizontalAlignmentCenter];
    else
        [self setContentHorizontalAlignment:UIControlContentHorizontalAlignmentLeft];
    [self updateImage];
}
-(void)updateImage
{
    NSString *imagename = nil;

    switch(type)
    {
        case _DW_BUTTON_TYPE_CHECK:
        {

            if(checkstate)
                imagename = @"checkmark.square";
            else
                imagename = @"square";
        }
        break;
        case _DW_BUTTON_TYPE_RADIO:
        {
            if(checkstate)
                imagename = @"largecircle.fill.circle";
            else
                imagename = @"circle";
        }
        break;
        case _DW_BUTTON_TYPE_TREE:
        {
            if(checkstate)
                imagename = @"chevron.down";
            else
                imagename = @"chevron.forward";
        }
        break;
    }
    if(imagename)
    {
        UIImage *image = [UIImage systemImageNamed:imagename];
        CGSize size = [image size];
        [self setImage:image forState:UIControlStateNormal];
        [self setTitleEdgeInsets:UIEdgeInsetsMake(0,size.width,0,0)];
    }
}
-(int)checkState { return checkstate; }
-(void)setCheckState:(int)input { checkstate = input; [self updateImage]; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a progress type */
@interface DWPercent : UIProgressView
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWPercent
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a menu item type */
@implementation DWMenuItem
-(void)setCheck:(BOOL)input { check = input; }
-(void)setEnabled:(BOOL)input { enabled = input; [self setAttributes:(enabled ? 0 : UIMenuElementAttributesDisabled)]; }
-(void)setSubmenu:(DWMenu *)input { submenu = input; }
-(void)setMenu:(DWMenu *)input { menu = input; }
-(void)setTag:(unsigned long)input { tag = input; }
-(BOOL)check { return check; }
-(BOOL)enabled { return enabled; }
-(DWMenu *)submenu { return submenu; }
-(DWMenu *)menu { return menu; }
-(unsigned long)tag { return tag; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end
/*
 * Encapsulate immutable objects in our own containers,
 * so we can recreate the immutable subobjects as needed.
 * Currently in this category: DWMenu and DWImage
 */
@implementation DWMenu
-(id)initWithTag:(unsigned long)newtag
{
    self = [super init];
    if(self)
    {
        children = [[NSMutableArray alloc] init];
        tag = newtag;
    }
    return self;
}
-(void)setTitle:(NSString *)input { title = input; [title retain]; }
-(UIMenu *)menu
{
    /* Create or recreate the UIMenu recursively */
    UIMenu *oldmenu = menu;
    NSMutableArray *menuchildren = [[NSMutableArray alloc] init];
    NSMutableArray *section = menuchildren;

    for(id child in children)
    {
        if([child isMemberOfClass:[DWMenuItem class]])
        {
            DWMenuItem *menuitem = child;
            DWMenu *submenu = [menuitem submenu];

            if(submenu)
                [section addObject:[submenu menu]];
            else
                [section addObject:child];
        }
        /* NSNull entry tells us to make a new section...
         * we do this by making a new UIMenu inline.
         */
        else if([child isMemberOfClass:[NSNull class]])
        {
            if(section != menuchildren)
            {
                UIMenu *sectionmenu = [UIMenu menuWithTitle:@""
                                                      image:nil
                                                 identifier:nil
                                                    options:UIMenuOptionsDisplayInline
                                                   children:section];
                [menuchildren addObject:sectionmenu];
            }
            section = [[NSMutableArray alloc] init];
        }
    }
    if(section != menuchildren)
    {
        UIMenu *sectionmenu = [UIMenu menuWithTitle:@""
                                              image:nil
                                         identifier:nil
                                            options:UIMenuOptionsDisplayInline
                                           children:section];
        [menuchildren addObject:sectionmenu];
    }
    if(title)
    {
        menu = [UIMenu menuWithTitle:title image:[UIImage systemImageNamed:@"ellipsis"] identifier:nil
                             options:0  children:menuchildren];
    }
    else
    {
        if(@available(iOS 14.0, *))
            menu = [UIMenu menuWithChildren:menuchildren];
        else
            menu = [UIMenu menuWithTitle:@"" children:menuchildren];
    }
    [menu retain];
    [menuchildren release];
    if(oldmenu)
        [oldmenu release];
    return menu;
}
-(void)addItem:(id)item
{
    [children addObject:item];
}
-(void)removeItem:(id)item
{
    [children removeObject:item];
}
-(unsigned long)tag { return tag; }
-(id)childWithTag:(unsigned long)tag
{
    if(tag > 0)
    {
        for(id child in children)
        {
            if([child isMemberOfClass:[DWMenuItem class]] && [child tag] == tag)
                return child;
        }
    }
    return nil;
}
-(void)dealloc
{
    if(menu)
        [menu release];
    [super dealloc];
}
@end

@implementation DWImage
-(id)initWithSize:(CGSize)size
{
    self = [super init];
    if(self)
    {
        CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
        CGContextRef cgcontext = CGBitmapContextCreate(NULL, size.width, size.height, 8, 0, rgb, kCGImageAlphaPremultipliedFirst);
        CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, size.height);
        CGContextConcatCTM(cgcontext, flipVertical);
        CGImageRef cgimage = CGBitmapContextCreateImage(cgcontext);
        image = [UIImage imageWithCGImage:cgimage];
        CGContextRelease(cgcontext);
        [image retain];
    }
    return self;
}
-(id)initWithCGImage:(CGImageRef)cgimage
{
    self = [super init];
    if(self)
    {
        image = [UIImage imageWithCGImage:cgimage];
        [image retain];
    }
    return self;
}
-(id)initWithUIImage:(UIImage *)newimage
{
    self = [super init];
    if(self)
    {
        image = newimage;
        [image retain];
    }
    return self;
}
-(void)setImage:(UIImage *)input
{
    UIImage *oldimage = image;
    image = input;
    [image retain];
    [oldimage release];
}
-(UIImage *)image
{
    /* If our CGContext has been modified... */
    if(cgcontext)
    {
        UIImage *oldimage = image;
        CGImageRef cgimage;

        /* Create a new UIImage from the CGContext via CGImage */
        cgimage = CGBitmapContextCreateImage(cgcontext);
        image = [UIImage imageWithCGImage:cgimage];
        CGContextRelease(cgcontext);
        cgcontext = nil;
        [image retain];
        [oldimage release];
        CGImageRelease(cgimage);
    }
    return image;
}
-(CGContextRef)cgcontext
{
    /* If we don't have an active context, create a bitmap
     * context and copy the image from our UIImage.
     */
    if(!cgcontext)
    {
        CGSize size = [image size];
        CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();

        cgcontext = CGBitmapContextCreate(NULL, size.width, size.height, 8, 0, rgb, kCGImageAlphaPremultipliedFirst);
        CGAffineTransform flipVertical = CGAffineTransformMake(1, 0, 0, -1, 0, size.height);
        CGContextConcatCTM(cgcontext, flipVertical);
        CGContextDrawImage(cgcontext, CGRectMake(0,0,size.width,size.height), [image CGImage]);
    }
    return cgcontext;
}
-(void)dealloc { if(cgcontext) CGContextRelease(cgcontext); if(image) [image release]; [super dealloc]; }
@end

/* Subclass for a scrollbox type */
@interface DWScrollBox : UIScrollView
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
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [box removeFromSuperview]; [box release]; [super dealloc]; }
@end

/* Subclass for a entryfield type */
@interface DWEntryField : UITextField <UITextFieldDelegate>
{
    void *userdata;
    id clickDefault;
    unsigned long limit;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setClickDefault:(id)input;
@end

@implementation DWEntryField
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
-(BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    // Prevent crashing undo bug – see note below.
    if(range.length + range.location > textField.text.length)
        return NO;

    if(limit > 0)
    {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        return newLength <= limit;
    }
    return YES;
}
-(void)setMaximumLength:(unsigned long)length { limit = length; }
-(unsigned long)maximumLength { return limit; }
@end

/* Subclass for a text and status text type */
@interface DWText : UILabel
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
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); [super dealloc]; }
@end

/* Subclass for a Notebook page type */
@interface DWNotebookPage : DWBox
{
    int pageid;
}
-(int)pageid;
-(void)setPageid:(int)input;
@end

/* Subclass for a Notebook control type */
@interface DWNotebook : UIView
{
    UISegmentedControl *tabs;
    void *userdata;
    int pageid;
    NSMutableArray<DWNotebookPage *> *views;
    DWNotebookPage *visible;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(int)pageid;
-(void)setPageid:(int)input;
-(void)setVisible:(DWNotebookPage *)input;
-(UISegmentedControl *)tabs;
-(NSMutableArray<DWNotebookPage *> *)views;
-(void)pageChanged:(id)sender;
@end

@implementation DWNotebook
-(id)init
{
    self = [super init];
    tabs = [[[UISegmentedControl alloc] init] retain];
    views = [[[NSMutableArray alloc] init] retain];
    [self addSubview:tabs];
    return self;
}
-(void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    frame.size.height = 40;
    [tabs setFrame:frame];
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)setVisible:(DWNotebookPage *)input { visible = input; }
-(UISegmentedControl *)tabs { return tabs; }
-(NSMutableArray<DWNotebookPage *> *)views { return views; };
-(void)pageChanged:(id)sender
{
    NSInteger intpageid = [tabs selectedSegmentIndex];
    
    if(intpageid != -1 && intpageid < [views count])
    {
        DWNotebookPage *page = [views objectAtIndex:intpageid];

        /* Hide the previously visible page contents */
        if(page != visible)
            [visible setHidden:YES];

        /* If the new page is a valid box, lay it out */
        if([page isKindOfClass:[DWBox class]])
        {
            Box *box = [page box];
            /* Start with the entire notebook size and then adjust
             * it to account for the segement control's height.
             */
            NSInteger height = [tabs frame].size.height;
            CGRect frame = [self frame];
            frame.origin.y += height;
            frame.size.height -= height;
            [page setFrame:frame];
            [page setHidden:NO];
            visible = page;
            _dw_do_resize(box, frame.size.width, frame.size.height);
            _dw_handle_resize_events(box);
        }
        _dw_event_handler(self, DW_INT_TO_POINTER([page pageid]), _DW_EVENT_SWITCH_PAGE);
    }
    else
        visible = nil;
}
-(void)dealloc
{
    UserData *root = userdata;
    _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    [tabs removeFromSuperview];
    [tabs release];
    [views release];
    [super dealloc];
}
@end

@implementation DWNotebookPage
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a splitbar type */
@interface DWSplitBar : UIView
{
    void *userdata;
    float percent;
    NSInteger Tag;
    id topleft;
    id bottomright;
    int splittype;
}
-(void)setTag:(NSInteger)tag;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(float)percent;
-(void)setPercent:(float)input;
-(id)topLeft;
-(void)setTopLeft:(id)input;
-(id)bottomRight;
-(void)setBottomRight:(id)input;
-(int)type;
-(void)setType:(int)input;
-(void)resize;
@end

@implementation DWSplitBar
-(void)setTag:(NSInteger)tag { Tag = tag; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(float)percent { return percent; }
-(void)setPercent:(float)input
{
    percent = input;
    if(percent > 100.0)
        percent = 100.0;
    else if(percent < 0.0)
        percent = 0.0;
    [self resize];
}
-(id)topLeft { return topleft; }
-(void)setTopLeft:(id)input
{
    topleft = input;
    [self addSubview:topleft];
    [topleft release];
}
-(id)bottomRight { return bottomright; }
-(void)setBottomRight:(id)input
{
    bottomright = input;
    [self addSubview:bottomright];
    [bottomright release];
}
-(int)type { return splittype; }
-(void)setType:(int)input { splittype = input; }
-(void)resize
{
    CGRect rect = self.frame;
    CGRect half = rect;

    half.origin.x = half.origin.y = 0;
    
    if(splittype == DW_HORZ)
        half.size.width = (percent/100.0) * rect.size.width;
    else
        half.size.height = (percent/100.0) * rect.size.height;
    if(topleft)
    {
        DWBox *view = topleft;
        Box *box = [view box];
        [topleft setFrame:half];
        _dw_do_resize(box, half.size.width, half.size.height);
        _dw_handle_resize_events(box);
    }
    if(splittype == DW_HORZ)
    {
        half.origin.x = half.size.width;
        half.size.width = rect.size.width - half.size.width;
    }
    else
    {
        half.origin.y = half.size.height;
        half.size.height = rect.size.height - half.size.height;
    }
    if(bottomright)
    {
        DWBox *view = bottomright;
        Box *box = [view box];
        [bottomright setFrame:half];
        _dw_do_resize(box, half.size.width, half.size.height);
        _dw_handle_resize_events(box);
    }
}
-(void)dealloc
{
    UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    if(topleft)
    {
        [topleft removeFromSuperview];
        [topleft release];
    }
    if(bottomright)
    {
        [bottomright removeFromSuperview];
        [bottomright release];
    }
    [super dealloc];
}
@end

/* Subclass for a slider type */
@interface DWSlider : UISlider
{
    void *userdata;
    BOOL vertical;
}
-(void)setVertical:(BOOL)vert;
-(BOOL)vertical;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)sliderChanged:(id)sender;
@end

@implementation DWSlider
-(void)setVertical:(BOOL)vert
{
    if(vert)
    {
        CGAffineTransform trans = CGAffineTransformMakeRotation(M_PI * 0.5);
        self.transform = trans;
    }
    vertical = vert;
}
-(BOOL)vertical { return vertical; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)sliderChanged:(id)sender { int intVal = (int)[self value]; _dw_event_handler(self, DW_INT_TO_POINTER(intVal), _DW_EVENT_VALUE_CHANGED); }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a MLE type */
@interface DWMLE : UITextView
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWMLE
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass UITableViewCell so we can support multiple columns...
 * Enabled via the DW_FEATURE_CONTAINER_MODE feature setting.
 */
@interface DWTableViewCell : UITableViewCell
{
    NSMutableArray *columndata;
    UIStackView *stack;
    UILabel *label;
    UIImageView *image;
}
-(void)setColumnData:(NSMutableArray *)input;
-(NSMutableArray *)columnData;
-(UIImageView *)image;
-(UILabel *)label;
-(void)columnClicked:(id)sender;
@end

@implementation DWTableViewCell
-(id)init {
    self = [super init];

    image = [[[UIImageView alloc] init] retain];
    [image setTranslatesAutoresizingMaskIntoConstraints:NO];
    [[self contentView] addSubview:image];

    label = [[[UILabel alloc] init] retain];
    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [[self contentView] addSubview:label];

    stack = [[[UIStackView alloc] init] retain];
    [stack setTranslatesAutoresizingMaskIntoConstraints:NO];
    [stack setSpacing:5.0];
    [[self contentView] addSubview:stack];

    /* Let's use the default margins */
    UILayoutGuide *g = [[self contentView] layoutMarginsGuide];

    [NSLayoutConstraint activateConstraints:@[
        /* Constrain label Top/Leading/Trailing to content margins guide */
        [image.topAnchor constraintEqualToAnchor:g.topAnchor constant:0.0],
        [image.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:0.0],
        [image.trailingAnchor constraintEqualToAnchor:label.leadingAnchor constant:-4.0],
        [label.topAnchor constraintEqualToAnchor:g.topAnchor constant:0.0],
        [label.leadingAnchor constraintEqualToAnchor:image.trailingAnchor constant:4.0],
        /* No Height, because we'll use the label's intrinsic size */

        /* Constrain stack view Top to label bottom plus 8-points */
        [stack.topAnchor constraintEqualToAnchor:label.bottomAnchor constant:0.0],
        /* Leading/Trailing/Bottom to content margins guide */
        [stack.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:0.0],
        [stack.bottomAnchor constraintEqualToAnchor:g.bottomAnchor constant:0.0],
        /* No Height, because we'll use the arranged subviews heights */
    ]];
    return self;
}
-(UILabel *)label { return label; }
-(UIImageView *)image { return image; }
-(void)columnClicked:(id)sender
{
    if([sender isMemberOfClass:[UIButton class]])
    {
        id view = [self superview];

        while(view && [view isKindOfClass:[UITableView class]] == NO)
            view = [view superview];

        _dw_event_handler(view, DW_INT_TO_POINTER([sender tag]+1), _DW_EVENT_COLUMN_CLICK);
    }
}
-(void)setColumnData:(NSMutableArray *)input
{
    if(columndata != input)
    {
        NSMutableArray *olddata = columndata;
        columndata = input;
        [columndata retain];
        [olddata release];
    }
    /* If we have columndata and are not in default mode */
    if(columndata && _dw_container_mode > DW_CONTAINER_MODE_DEFAULT)
    {
        bool extra = _dw_container_mode == DW_CONTAINER_MODE_EXTRA;
        id subviews = [stack arrangedSubviews];
        int index = 0;

        /* Extra mode stack is horizontal, multi stack is vertical */
        [stack setAxis:(extra ? UILayoutConstraintAxisHorizontal : UILayoutConstraintAxisVertical)];
        /* For Multi-line, we don't want fill, we want leading */
        if(!extra)
            [stack setAlignment:UIStackViewAlignmentLeading];

        /* Create the stack using columndata, reusing objects when possible */
        for(id object in columndata)
        {
            if([object isKindOfClass:[NSString class]])
            {
                id label = nil;

                /* Check if we already have a view, reuse it if possible.. */
                if(index < [subviews count])
                {
                    id oldview = [subviews objectAtIndex:index];

                    if([oldview isMemberOfClass:(extra ? [UILabel class] : [UIButton class])])
                        label = oldview;
                    else
                        [stack removeArrangedSubview:oldview];
                }
                if(!label)
                {
                    label = extra ? [[UILabel alloc] init] : [UIButton buttonWithType:UIButtonTypeCustom];

                    [label setTag:index];
                    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
                    if(extra)
                        [label setTextAlignment:NSTextAlignmentCenter];
                    else
                    {
                        [label addTarget:self action:@selector(columnClicked:)
                                    forControlEvents:UIControlEventTouchUpInside];
                        [label setTitleColor:[UIColor darkTextColor] forState:UIControlStateNormal];
                    }

                    if(index < [subviews count])
                        [stack insertArrangedSubview:label atIndex:index];
                    else /* Remove the view if it won't work */
                        [stack addArrangedSubview:label];
                }
                /* Set the label or button text/title */
                if(extra)
                    [label setText:object];
                else
                    [label setTitle:object forState:UIControlStateNormal];
                index++;
            }
            else if([object isMemberOfClass:[UIImage class]])
            {
                id image = nil;

                /* Check if we already have a view, reuse it if possible.. */
                if(index < [subviews count])
                {
                    id oldview = [subviews objectAtIndex:index];

                    if([oldview isMemberOfClass:[UIImageView class]])
                        image = oldview;
                    else /* Remove the view if it won't work */
                        [stack removeArrangedSubview:oldview];
                }
                if(!image)
                {
                    image = [[UIImageView alloc] init];

                    [image setTag:index];
                    [image setTranslatesAutoresizingMaskIntoConstraints:NO];

                    if(index < [subviews count])
                        [stack insertArrangedSubview:image atIndex:index];
                    else
                        [stack addArrangedSubview:image];
                }
                /* Set the image view or button image */
                [image setImage:object];

                index++;
            }
        }
    }
}
-(NSMutableArray *)columnData { return columndata; }
-(void)dealloc
{
    [columndata release];
    [image removeFromSuperview];
    [image release];
    [label removeFromSuperview];
    [label release];
    [stack removeFromSuperview];
    [stack release];
    [super dealloc];
}
@end

 /* Create a tableview cell for containers using DW_CONTAINER_MODE_* or listboxes */
DWTableViewCell *_dw_table_cell_view_new(UIImage *icon, NSString *text, NSMutableArray *columndata)
{
    DWTableViewCell *browsercell = [[DWTableViewCell alloc] init];
    [browsercell setAutoresizesSubviews:YES];

    if(icon)
        [[browsercell image] setImage:icon];
    if(text)
        [[browsercell label] setText:text];
    if(columndata)
        [browsercell setColumnData:columndata];
    return browsercell;
}

/* Subclass for a Container/List type */
@interface DWContainer : UITableView <UITableViewDataSource,UITableViewDelegate>
{
    void *userdata;
    NSMutableArray *tvcols;
    NSMutableArray *data;
    NSMutableArray *types;
    NSPointerArray *titles;
    NSPointerArray *rowdatas;
    UIColor *fgcolor, *oddcolor, *evencolor;
    unsigned long dw_oddcolor, dw_evencolor;
    unsigned long _DW_COLOR_ROW_ODD, _DW_COLOR_ROW_EVEN;
    int iLastAddPoint, iLastQueryPoint;
    int filesystem;
    NSTimeInterval lastClick;
    NSInteger lastClickRow;
}
-(NSInteger)tableView:(UITableView *)aTable numberOfRowsInSection:(NSInteger)section;
-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath;
-(void)tableView:(UITableView*)tableView willDisplayCell:(UITableViewCell*)cell forRowAtIndexPath:(NSIndexPath*)indexPath;
-(UIContextMenuConfiguration *)tableView:(UITableView *)tableView contextMenuConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath point:(CGPoint)point;
-(void)addColumn:(NSString *)input andType:(int)type;
-(NSString *)getColumn:(int)col;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFilesystem:(int)input;
-(int)filesystem;
-(int)addRow:(DWTableViewCell *)input;
-(int)addRows:(int)number;
-(void)editCell:(id)input at:(int)row;
-(void)removeRow:(int)row;
-(void)setRow:(int)row title:(const char *)input;
-(void *)getRowTitle:(int)row;
-(id)getRow:(int)row;
-(NSMutableArray *)getColumns;
-(int)cellType:(int)col;
-(int)lastAddPoint;
-(int)lastQueryPoint;
-(void)setLastQueryPoint:(int)input;
-(void)clear;
-(void)setup;
-(CGSize)getsize;
-(void)setForegroundColor:(UIColor *)input;
@end

@implementation DWContainer
-(NSInteger)tableView:(UITableView *)aTable numberOfRowsInSection:(NSInteger)section
{
    /* Ignoring section for now, everything in one section */
    if(tvcols && data)
        return [data count];
    return 0;
}
-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    /* Not reusing cell views, so get the cell from our array */
    int index = (int)indexPath.row;
    id celldata = [data objectAtIndex:index];

    /* The data is already a DWTableViewCell so just return that */
    if([celldata isMemberOfClass:[DWTableViewCell class]])
    {
        DWTableViewCell *result = celldata;
        
        /* Copy the alignment setting from the column,
         * and set the text color from the container.
         */
        if(fgcolor)
            [[result label] setTextColor:fgcolor];

        /* Return the result */
        return result;
    }
    return nil;
}
-(void)tableView:(UITableView*)tableView willDisplayCell:(UITableViewCell*)cell forRowAtIndexPath:(NSIndexPath*)indexPath
{
    if(indexPath.row % 2 == 0)
    {
        if(evencolor)
            [cell setBackgroundColor:evencolor];
        else
            [cell setBackgroundColor:[UIColor clearColor]];
    }
    else
    {
        if(oddcolor)
            [cell setBackgroundColor:oddcolor];
        else
            [cell setBackgroundColor:[UIColor clearColor]];
    }
}
-(CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    int index = (int)indexPath.row;
    id cell = [data objectAtIndex:index];
    CGFloat height = 0.0;

    /* The data is already a DWTableViewCell so just return that */
    if([cell isMemberOfClass:[DWTableViewCell class]])
        height = [[cell contentView] systemLayoutSizeFittingSize:UILayoutFittingCompressedSize].height;
    return height > 0.0 ? height : UITableViewAutomaticDimension;
}
-(CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return _dw_container_mode > DW_CONTAINER_MODE_DEFAULT ? 85.0 : 44.0;
}
-(UIContextMenuConfiguration *)tableView:(UITableView *)tableView contextMenuConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath point:(CGPoint)point
{
    DWWindow *window = (DWWindow *)[self window];
    UIContextMenuConfiguration *config = nil;
    NSPointerArray *params = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory];

    [params addPointer:[self getRowTitle:(int)indexPath.row]];
    [params addPointer:[self getRowData:(int)indexPath.row]];
    [params addPointer:DW_INT_TO_POINTER((int)point.x)];
    [params addPointer:DW_INT_TO_POINTER((int)point.y)];

    _dw_event_point = point;
    _dw_event_handler(self, params, _DW_EVENT_ITEM_CONTEXT);

    if(window && [window popupMenu])
    {
        __block UIMenu *popupmenu = [[[window popupMenu] menu] retain];
        config = [UIContextMenuConfiguration configurationWithIdentifier:nil
                                                         previewProvider:nil
                                                          actionProvider:^(NSArray* suggestedAction){return popupmenu;}];
        [window setPopupMenu:nil];
    }
    return config;
}
-(void)addColumn:(NSString *)input andType:(int)type { if(tvcols) { [tvcols addObject:input]; [types addObject:[NSNumber numberWithInt:type]]; } }
-(NSString *)getColumn:(int)col { if(tvcols) { return [tvcols objectAtIndex:col]; } return nil; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFilesystem:(int)input { filesystem = input; }
-(int)filesystem { return filesystem; }
-(void)refreshColors
{
    UIColor *oldodd = oddcolor;
    UIColor *oldeven = evencolor;
    unsigned long thisodd = dw_oddcolor == DW_CLR_DEFAULT ? _DW_COLOR_ROW_ODD : dw_oddcolor;
    unsigned long _odd = _dw_get_color(thisodd);
    unsigned long thiseven = dw_evencolor == DW_CLR_DEFAULT ? _DW_COLOR_ROW_EVEN : dw_evencolor;
    unsigned long _even = _dw_get_color(thiseven);

    /* Get the UIColor for non-default colors */
    if(thisodd != DW_RGB_TRANSPARENT)
        oddcolor = [[UIColor colorWithRed: DW_RED_VALUE(_odd)/255.0 green: DW_GREEN_VALUE(_odd)/255.0 blue: DW_BLUE_VALUE(_odd)/255.0 alpha: 1] retain];
    else
        oddcolor = NULL;
    if(thiseven != DW_RGB_TRANSPARENT)
        evencolor = [[UIColor colorWithRed: DW_RED_VALUE(_even)/255.0 green: DW_GREEN_VALUE(_even)/255.0 blue: DW_BLUE_VALUE(_even)/255.0 alpha: 1] retain];
    else
        evencolor = NULL;
    [oldodd release];
    [oldeven release];
}
-(void)setRowBgOdd:(unsigned long)oddcol andEven:(unsigned long)evencol
{
    /* Save the set colors in case we get a theme change */
    dw_oddcolor = oddcol;
    dw_evencolor = evencol;
    [self refreshColors];
}
-(void)checkDark
{
    /* Update any system colors based on the Dark Mode */
    _DW_COLOR_ROW_EVEN = DW_RGB_TRANSPARENT;
    if(_dw_is_dark())
        _DW_COLOR_ROW_ODD = DW_RGB(100, 100, 100);
    else
        _DW_COLOR_ROW_ODD = DW_RGB(230, 230, 230);
    /* Only refresh if we've been setup already */
    if(titles)
        [self refreshColors];
}
-(void)viewDidChangeEffectiveAppearance { [self checkDark]; }
-(int)insertRow:(DWTableViewCell *)input at:(int)index
{
    if(data)
    {
        if(index < iLastAddPoint)
        {
            iLastAddPoint++;
        }
        [data insertObject:input atIndex:index];
        [titles insertPointer:NULL atIndex:index];
        [rowdatas insertPointer:NULL atIndex:index];
        return (int)[titles count];
    }
    return 0;
}
-(int)addRow:(DWTableViewCell *)input
{
    if(data)
    {
        iLastAddPoint = (int)[titles count];
        [data addObject:input];
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
        int z;

        iLastAddPoint = (int)[titles count];

        for(z=0;z<number;z++)
        {
            [data addObject:[NSNull null]];
            [titles addPointer:NULL];
            [rowdatas addPointer:NULL];
        }
        return (int)[titles count];
    }
    return 0;
}
-(void)editCell:(id)input at:(int)row
{
    if(tvcols)
    {
        if(row < [data count])
        {
            if(!input)
                input = [NSNull null];
            [data replaceObjectAtIndex:row withObject:input];
        }
    }
}
-(void)removeRow:(int)row
{
    if(tvcols)
    {
        void *oldtitle;

        [data removeObjectAtIndex:row];
        oldtitle = [titles pointerAtIndex:row];
        [titles removePointerAtIndex:row];
        [rowdatas removePointerAtIndex:row];
        if(iLastAddPoint > 0 && iLastAddPoint > row)
        {
            iLastAddPoint--;
        }
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRow:(int)row title:(const char *)input
{
    if(titles && input)
    {
        void *oldtitle = [titles pointerAtIndex:row];
        void *newtitle = input ? (void *)strdup(input) : NULL;
        [titles replacePointerAtIndex:row withPointer:newtitle];
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRowData:(int)row title:(void *)input { if(rowdatas && input) { [rowdatas replacePointerAtIndex:row withPointer:input]; } }
-(void *)getRowTitle:(int)row { if(titles && row > -1) { return [titles pointerAtIndex:row]; } return NULL; }
-(void *)getRowData:(int)row { if(rowdatas && row > -1) { return [rowdatas pointerAtIndex:row]; } return NULL; }
-(id)getRow:(int)row { if(data && [data count]) {  return [data objectAtIndex:row]; } return nil; }
-(NSMutableArray *)getColumns { return tvcols; }
-(int)cellType:(int)col { return [[types objectAtIndex:col] intValue]; }
-(int)lastAddPoint { return iLastAddPoint; }
-(int)lastQueryPoint { return iLastQueryPoint; }
-(void)setLastQueryPoint:(int)input { iLastQueryPoint = input; }
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
    iLastAddPoint = 0;
}
-(void)setup
{
    titles = [[NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory] retain];
    rowdatas = [[NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory] retain];
    tvcols = [[[NSMutableArray alloc] init] retain];
    data = [[[NSMutableArray alloc] init] retain];
    types = [[[NSMutableArray alloc] init] retain];
    if(!dw_oddcolor && !dw_evencolor)
        dw_oddcolor = dw_evencolor = DW_CLR_DEFAULT;
    [self checkDark];
}
-(CGSize)getsize
{
    int cwidth = 0, cheight = 0;

    if(tvcols)
    {
        int colcount = (int)[tvcols count];
        int rowcount = (int)[self numberOfRowsInSection:0];
        int width = 0;

        if(rowcount > 0)
        {
            int x;

            for(x=0;x<rowcount;x++)
            {
                DWTableViewCell *cell = [data objectAtIndex:(x*colcount)];
                int thiswidth = 4, thisheight = 0;
                
                if([cell imageView])
                {
                    thiswidth += [[cell image] image].size.width;
                    thisheight = [[cell image] image].size.height;
                }
                if([cell textLabel])
                {
                    int textheight = [[cell label] intrinsicContentSize].width;
                    thiswidth += [[cell label] intrinsicContentSize].width;
                    if(textheight > thisheight)
                        thisheight = textheight;
                }

                cheight += thisheight;

                if(thiswidth > width)
                {
                    width = thiswidth;
                }
            }
            /* If the image is missing default the optimized width to 16. */
            if(!width && [[types objectAtIndex:0] intValue] & DW_CFA_BITMAPORICON)
            {
                width = 16;
            }
        }
        if(width)
            cwidth += width;
    }
    cwidth += 16;
    cheight += 16;
    return CGSizeMake(cwidth, cheight);
}
-(void)setForegroundColor:(UIColor *)input
{
    UIColor *oldfgcolor = fgcolor;

    fgcolor = input;
    [fgcolor retain];
    [oldfgcolor release];
}
-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSPointerArray *params = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory];

    [params addPointer:[self getRowTitle:(int)indexPath.row]];
    [params addPointer:[self getRowData:(int)indexPath.row]];

    /* If multiple selection is enabled, treat it as selectionChanged: */
    if([self allowsMultipleSelection])
    {
        NSTimeInterval now = [NSDate timeIntervalSinceReferenceDate];
        
        /* Handler for container class... check for double tap */
        if(lastClickRow == indexPath.row && now - lastClick < 0.3)
            _dw_event_handler(self, params, _DW_EVENT_ITEM_ENTER);
        else
            _dw_event_handler(self, params, _DW_EVENT_ITEM_SELECT);
        /* Handler for listbox class */
        _dw_event_handler(self, DW_INT_TO_POINTER((int)indexPath.row), _DW_EVENT_LIST_SELECT);
        /* Update lastClick for double tap check */
        lastClick = now;
        lastClickRow = indexPath.row;
    }
    else /* Otherwise treat it as doubleClicked: */
    {
        /* Handler for container class */
        _dw_event_handler(self, params, _DW_EVENT_ITEM_ENTER);
   }
}
-(void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if([self allowsMultipleSelection])
        [self tableView:tableView didSelectRowAtIndexPath:indexPath];
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Custom tree view subclasses: DWTree, DWTreeItem, DWTreeViewCell, DWTreeViewCellDelegate */
@interface DWTreeItem : NSObject
@property(nonatomic, strong) UIImage *icon;
@property(nonatomic, strong) NSString *title;
@property(nonatomic, assign) void *data;
@property(nonatomic, weak) DWTreeItem *parent;
@property(nonatomic, retain, readonly) NSMutableArray<DWTreeItem *> *children;
@property(nonatomic, assign, readonly) NSUInteger levelDepth;
@property(nonatomic, assign, readonly) BOOL isRoot;
@property(nonatomic, assign, readonly) BOOL hasChildren;
@property(nonatomic, assign) BOOL expanded;
-(instancetype)initWithIcon:(UIImage *)icon Title:(NSString *)title andData:(void *)itemdata;
-(void)insertChildAfter:(DWTreeItem *)treeItem;
-(void)appendChild:(DWTreeItem *)newChild;
-(void)removeFromParent;
-(void)moveToDestination:(DWTreeItem *)destination;
-(BOOL)containsTreeItem:(DWTreeItem *)treeItem;
-(NSArray<DWTreeItem *> *)visibleNodes;
@end

@implementation DWTreeItem
{
    NSArray *_flattenedTreeCache;
}
-(void)dealloc { NSLog(@"DWTreeItem \"%@\" dealloc", self.title); [_title release]; [_icon release]; [super dealloc]; }
-(instancetype)initWithIcon:(UIImage *)icon Title:(NSString *)title andData:(void *)itemdata
{
    if(self = [super init])
    {
        _title = [title retain];
        _icon = [icon retain];
        _data = itemdata;
        _children = [[NSMutableArray alloc] initWithCapacity:1];
    }
    return self;
}
-(NSString *)title
{
    if(_title)
        return _title;
    return self.description;
}
-(UIImage *)icon
{
    if(_icon)
        return _icon;
    return nil;
}
-(NSArray<DWTreeItem *> *)visibleNodes
{
    NSMutableArray *allElements = [[NSMutableArray alloc] init];
    if(![self isRoot])
        [allElements addObject:self];
    if(_expanded)
    {
        for (DWTreeItem *child in _children)
            [allElements addObjectsFromArray:[child visibleNodes]];
    }
    if(_flattenedTreeCache)
        [_flattenedTreeCache release];
    _flattenedTreeCache = allElements;
    return allElements;
}
-(void)insertChildAfter:(DWTreeItem *)treeItem
{
    DWTreeItem *parent = self.parent;
    NSUInteger index = [parent.children indexOfObject:self];
    if(index == NSNotFound)
        index = [parent.children count];
    treeItem.parent = parent;
    [parent.children insertObject:treeItem atIndex:(index+1)];
}
-(void)appendChild:(DWTreeItem *)newChild
{
    newChild.parent = self;
    [_children addObject:newChild];
}
-(void)removeFromParent
{
    DWTreeItem *parent = self.parent;
    if(parent)
    {
        [parent.children removeObject:self];
        self.parent = nil;
    }
}
-(void)moveToDestination:(DWTreeItem *)destination
{
    NSAssert([self containsTreeItem:destination]==NO, @"[self containsTreeItem:destination] something went wrong!");
    if(self == destination || destination == nil)
        return;
    [self removeFromParent];

    [destination insertChildAfter:self];
}
-(BOOL)containsTreeItem:(DWTreeItem *)treeItem
{
    DWTreeItem *parent = treeItem.parent;
    if(parent == nil)
        return NO;
    if(self == parent)
        return YES;
    return [self containsTreeItem:parent];
}
-(NSUInteger)levelDepth
{
    NSUInteger cnt = 0;
    if(_parent != nil)
    {
        cnt += 1;
        cnt += [_parent levelDepth];
    }
    return cnt;
}
-(BOOL)isRoot { return (!_parent); }
-(BOOL)hasChildren { return (_children.count > 0); }
@end

@class DWTreeViewCell;
@protocol DWTreeViewCellDelegate <NSObject>
//@optional
- (BOOL) queryExpandableInTreeViewCell:(DWTreeViewCell *)treeViewCell;
- (void) treeViewCell:(DWTreeViewCell *)treeViewCell expanded:(BOOL)expanded;
@end

@interface DWTreeViewCell : UITableViewCell
@property(nonatomic, strong) UILabel *titleLabel;
@property(nonatomic) NSUInteger level;
@property(nonatomic) BOOL expanded;
@property(nonatomic) BOOL children;
@property(nonatomic, assign) id <DWTreeViewCellDelegate> delegate;
-(instancetype)initWithStyle:(UITableViewCellStyle)style
             reuseIdentifier:(NSString *)reuseIdentifier
                       level:(NSUInteger)level
                    expanded:(BOOL)expanded
                    children:(BOOL)children;
@end

CGRect DWRectInflate(CGRect rect, CGFloat dx, CGFloat dy)
{
    return CGRectMake(rect.origin.x-dx, rect.origin.y-dy, rect.size.width+2*dx, rect.size.height+2*dy);
}

static CGFloat _DW_TREE_IMG_HEIGHT_WIDTH = 20;
static CGFloat _DW_TREE_XOFFSET = 3;

@implementation DWTreeViewCell
{
    DWButton *_arrowImageButton;
}
-(id)initWithStyle:(UITableViewCellStyle)style
   reuseIdentifier:(NSString *)reuseIdentifier
             level:(NSUInteger)level
          expanded:(BOOL)expanded
          children:(BOOL)children
{
    // We should never display the root node
    if(level < 1)
        return nil;

    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];

    if(self)
    {
        _level = level - 1;
        _expanded = expanded;
        _children = children;

        UIView *content = self.contentView;

        UILabel *titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
        titleLabel.numberOfLines = 0;
        titleLabel.textAlignment = NSTextAlignmentLeft;
        [content addSubview:titleLabel];
        _titleLabel = titleLabel;

        DWButton *arrowImageButton = [[DWButton alloc] initWithFrame:CGRectMake(0, 0,
                                                                                _DW_TREE_IMG_HEIGHT_WIDTH,
                                                                                _DW_TREE_IMG_HEIGHT_WIDTH)];
        [arrowImageButton addTarget:arrowImageButton
                             action:@selector(buttonClicked:)
                   forControlEvents:UIControlEventTouchUpInside];

        [arrowImageButton setType:_DW_BUTTON_TYPE_TREE];
        [arrowImageButton setDidCheckedChanged:^(BOOL checked) {
            _expanded = checked;
            if([_delegate respondsToSelector:@selector(treeViewCell:expanded:)])
                [_delegate treeViewCell:self expanded:checked];
        }];
        [arrowImageButton setCheckState:_expanded];
        [arrowImageButton setContentHorizontalAlignment:UIControlContentHorizontalAlignmentCenter];
        if(!children)
            [arrowImageButton setHidden:YES];
        [content addSubview:arrowImageButton];
        _arrowImageButton = arrowImageButton;
    }
    return self;
}
#pragma mark -
#pragma mark Other overrides
-(void)layoutSubviews
{
    [super layoutSubviews];

    CGSize size = self.contentView.bounds.size;
    CGFloat stepSize = size.height;
    CGRect rc = CGRectMake(_level * stepSize, 0, stepSize, stepSize);
    _arrowImageButton.frame = DWRectInflate(rc, -_DW_TREE_XOFFSET, -_DW_TREE_XOFFSET);

    rc = CGRectMake((_level + 1) * stepSize, 0, stepSize, stepSize);
    self.imageView.frame = DWRectInflate(rc, -_DW_TREE_XOFFSET, -_DW_TREE_XOFFSET);
    _titleLabel.frame = CGRectMake((_level + 2) * stepSize, 0, size.width - (_level + 3) * stepSize, stepSize);
}
@end

@class DWTree;

@protocol DWTreeViewDelegate <NSObject>
@required
-(NSInteger)numberOfRowsInTreeView:(DWTree *)treeView;
-(DWTreeItem *)treeView:(DWTree *)treeView treeItemForRow:(NSInteger)row;
-(NSInteger)treeView:(DWTree *)treeView rowForTreeItem:(DWTreeItem *)treeItem;
-(void)treeView:(DWTree *)treeView removeTreeItem:(DWTreeItem *)treeItem;
-(void)treeView:(DWTree *)treeView moveTreeItem:(DWTreeItem *)treeItem to:(DWTreeItem *)to;
-(void)treeView:(DWTree *)treeView addTreeItem:(DWTreeItem *)treeItem;
-(void)treeView:(DWTree *)treeView didSelectForTreeItem:(DWTreeItem *)treeItem;
-(UIContextMenuConfiguration *)treeView:(DWTree *)treeView contextMenuConfigurationForTreeItem:(DWTreeItem *)treeItem point:(CGPoint)point;
-(BOOL)treeView:(DWTree *)treeView queryExpandableInTreeItem:(DWTreeItem *)treeItem;
-(void)treeView:(DWTree *)treeView treeItem:(DWTreeItem *)treeItem expanded:(BOOL)expanded;
@optional
-(BOOL)treeView:(DWTree *)treeView canEditTreeItem:(DWTreeItem *)treeItem;
-(BOOL)treeView:(DWTree *)treeView canMoveTreeItem:(DWTreeItem *)treeItem;
@end

@interface DWTree : UITableView
@property(nonatomic, strong) UIFont *font;
@property(nonatomic, strong) DWTreeItem *treeItem;
@property(nonatomic, weak) id<DWTreeViewDelegate> treeViewDelegate;
-(instancetype)initWithFrame:(CGRect)frame;
-(void)insertTreeItem:(DWTreeItem *)treeItem;
@end

@interface DWTree () <UITableViewDataSource, UITableViewDelegate, DWTreeViewCellDelegate, DWTreeViewDelegate>
@end

@implementation DWTree
{
    DWTreeItem *_rootNode;
    DWTreeItem *_selectedNode;
}
-(instancetype)initWithFrame:(CGRect)frame
{
    if(self = [super initWithFrame:frame])
    {
        self.delegate=self;
        self.dataSource=self;
        _treeViewDelegate=self;
        self.separatorStyle= UITableViewCellSeparatorStyleNone;
        _font = [UIFont systemFontOfSize:16];
        _rootNode = [[[DWTreeItem alloc] initWithIcon:nil Title:@"@Root" andData:NULL] retain];
        _rootNode.expanded = YES;
    }
    return self;
}
-(void)dealloc { [_rootNode release]; [super dealloc]; }
-(void)insertTreeItem:(DWTreeItem *)treeItem
{
    DWTreeItem *targetNode = nil;

    if([self window])
    {
        NSArray<DWTreeViewCell *> *cells = [self visibleCells];

        // Target the selected tree node first if any
        for(DWTreeViewCell *cell in cells)
        {
            DWTreeItem *iter = [self treeItemForTreeViewCell:cell];
            if(iter == _selectedNode)
            {
                targetNode = iter;
                break;
            }
        }
        // Otherwise target first visible node if any
        if(targetNode == nil && [cells count])
            targetNode = [self treeItemForTreeViewCell:cells[0]];
    }
    // Finally put it on the root level
    if(targetNode == nil)
        targetNode = _rootNode;
    // If target is still nil something went horrible wrong
    NSAssert(targetNode, @"targetNode == nil, something went wrong!");
    if(targetNode.isRoot)
        [targetNode appendChild:treeItem];
    else
        [targetNode insertChildAfter:treeItem];

    if([_treeViewDelegate respondsToSelector:@selector(treeView:addTreeItem:)])
        [_treeViewDelegate treeView:self addTreeItem:treeItem];

    [self reloadData];
    [self resetSelection:NO];
}
-(void)setFont:(UIFont *)font
{
    _font = font;
    [self reloadData];
    [self resetSelection:NO];
}
-(void)clear
{
    [[_rootNode children] removeAllObjects];
    [self reloadData];
    [self resetSelection:NO];
}
-(void)resetSelection:(BOOL)delay
{
    NSInteger row = NSNotFound;
    if([_treeViewDelegate respondsToSelector:@selector(treeView:rowForTreeItem:)])
        row = [_treeViewDelegate treeView:self rowForTreeItem:_selectedNode];
    if(row != NSNotFound)
    {
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:row inSection:0];
        dispatch_block_t run = ^ {
            [self selectRowAtIndexPath:indexPath animated:NO scrollPosition:UITableViewScrollPositionNone];
        };
        if(delay)
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), run);
        else
            run();
    }
}
#pragma mark - DWTreeViewDelegate
-(NSInteger)numberOfRowsInTreeView:(DWTree *)treeView { return [_rootNode visibleNodes].count; }
-(void)treeView:(DWTree *)treeView addTreeItem:(DWTreeItem *)treeItem {}
-(void)treeView:(DWTree *)treeView didSelectForTreeItem:(DWTreeItem *)treeItem
{
    if(treeItem)
    {
        const char *title = [[treeItem title] UTF8String];
        NSPointerArray *params = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory];

        [params addPointer:strdup(title ? title : "")];
        [params addPointer:[treeItem data]];
        [params addPointer:treeItem];
        _dw_event_handler(treeView, params, _DW_EVENT_ITEM_SELECT);
        free([params pointerAtIndex:0]);
    }
}
-(UIContextMenuConfiguration *)treeView:(DWTree *)treeView contextMenuConfigurationForTreeItem:(DWTreeItem *)treeItem point:(CGPoint)point
{
    UIContextMenuConfiguration *config = nil;
    DWWindow *window = (DWWindow *)[self window];
    const char *title = [[treeItem title] UTF8String];
    NSPointerArray *params = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory];

    [params addPointer:strdup(title ? title : "")];
    [params addPointer:[treeItem data]];
    [params addPointer:DW_INT_TO_POINTER((int)point.x)];
    [params addPointer:DW_INT_TO_POINTER((int)point.y)];

    _dw_event_point = point;
    _dw_event_handler(self, params, _DW_EVENT_ITEM_CONTEXT);
    free([params pointerAtIndex:0]);

    if(window && [window popupMenu])
    {
        __block UIMenu *popupmenu = [[[window popupMenu] menu] retain];
        config = [UIContextMenuConfiguration configurationWithIdentifier:nil
                                                         previewProvider:nil
                                                          actionProvider:^(NSArray* suggestedAction){return popupmenu;}];
        [window setPopupMenu:nil];
    }
    return config;
}
-(void)treeView:(DWTree *)treeView moveTreeItem:(DWTreeItem *)treeItem to:(DWTreeItem *)to {}
-(BOOL)treeView:(DWTree *)treeView queryExpandableInTreeItem:(DWTreeItem *)treeItem { return YES; }
-(void)treeView:(DWTree *)treeView removeTreeItem:(DWTreeItem *)treeItem {}
-(NSInteger)treeView:(DWTree *)treeView rowForTreeItem:(DWTreeItem *)treeItem {  return [[_rootNode visibleNodes] indexOfObject:treeItem]; }
-(void)treeView:(DWTree *)treeView treeItem:(DWTreeItem *)treeItem expanded:(BOOL)expanded
{
    if(treeItem)
        _dw_event_handler(treeView, (void *)treeItem, _DW_EVENT_TREE_EXPAND);
}
- (DWTreeItem *)treeView:(DWTree *)treeView treeItemForRow:(NSInteger)row { return [[_rootNode visibleNodes] objectAtIndex:row]; }
#pragma mark - UITableViewDataSource
-(NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 1; }
-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    NSInteger count = 0;
    if([_treeViewDelegate respondsToSelector:@selector(numberOfRowsInTreeView:)])
        count = [_treeViewDelegate numberOfRowsInTreeView:self];
    return count;
}
-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    DWTreeViewCell *cell = [[DWTreeViewCell alloc] initWithStyle:UITableViewCellStyleDefault
                                                 reuseIdentifier:CellIdentifier
                                                           level:[treeItem levelDepth]
                                                        expanded:treeItem.expanded
                                                        children:[treeItem hasChildren]];
    cell.titleLabel.text = treeItem.title;
    cell.imageView.image = treeItem.icon;
    cell.titleLabel.font = _font;
    //cell.selectionStyle = UITableViewCellSelectionStyleNone;
    cell.delegate = self;
    return cell;
}
-(BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    if([_treeViewDelegate respondsToSelector:@selector(treeView:canEditTreeItem:)])
        return [_treeViewDelegate treeView:self canEditTreeItem:treeItem];
    return NO;
}
-(void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    if(editingStyle == UITableViewCellEditingStyleDelete)
    {
        [treeItem removeFromParent];
        if([_treeViewDelegate respondsToSelector:@selector(treeView:removeTreeItem:)])
            [_treeViewDelegate treeView:self removeTreeItem:treeItem];
        if(treeItem.expanded && treeItem.hasChildren)
            [self reloadData];
        else
            [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [self resetSelection:YES];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
-(void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
    if([fromIndexPath isEqual:toIndexPath])
        return;
    DWTreeItem *srcNode = [self treeItemForIndexPath:fromIndexPath];
    DWTreeItem *targetNode = [self treeItemForIndexPath:toIndexPath];

    if(srcNode && targetNode)
    {
        [srcNode moveToDestination:targetNode];

        if([_treeViewDelegate respondsToSelector:@selector(treeView:moveTreeItem:to:)])
            [_treeViewDelegate treeView:self moveTreeItem:srcNode to:targetNode];

        [self reloadData];
        [self resetSelection:NO];
    }
}
-(BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    if(treeItem && [_treeViewDelegate respondsToSelector:@selector(treeView:canMoveTreeItem:)])
        return [_treeViewDelegate treeView:self canMoveTreeItem:treeItem];
    return NO;
}
#pragma mark - DWTableViewDelegate
-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    if(treeItem && [_treeViewDelegate respondsToSelector:@selector(treeView:didSelectForTreeItem:)])
        [_treeViewDelegate treeView:self didSelectForTreeItem:treeItem];
    _selectedNode = treeItem;
}
-(UIContextMenuConfiguration *)tableView:(UITableView *)tableView contextMenuConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath point:(CGPoint)point
{
    UIContextMenuConfiguration *config = nil;
    DWTreeItem *treeItem = [self treeItemForIndexPath:indexPath];
    if(treeItem && [_treeViewDelegate respondsToSelector:@selector(treeView:contextMenuConfigurationForTreeItem:point:)])
        config = [_treeViewDelegate treeView:self contextMenuConfigurationForTreeItem:treeItem point:point];

    return config;
}
-(NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath
{
    DWTreeItem *srcNode = [self treeItemForIndexPath:sourceIndexPath];
    DWTreeItem *targetNode = [self treeItemForIndexPath:proposedDestinationIndexPath];
    if([srcNode containsTreeItem:targetNode] || srcNode==targetNode)
        return sourceIndexPath;
    // NSLog(@"Moving to target node \"%@\"", targetNode.title);
    return proposedDestinationIndexPath;
}
#pragma mark - DWTreeViewCellDelegate
-(BOOL)queryExpandableInTreeViewCell:(DWTreeViewCell *)treeViewCell
{
    BOOL allow = YES;
    if([_treeViewDelegate respondsToSelector:@selector(treeView:queryExpandableInTreeItem:)])
    {
        DWTreeItem *treeItem = [self treeItemForTreeViewCell:treeViewCell];
        allow = [_treeViewDelegate treeView:self queryExpandableInTreeItem:treeItem];
    }
    return allow;
}
-(void)treeViewCell:(DWTreeViewCell *)treeViewCell expanded:(BOOL)expanded
{
    DWTreeItem *treeItem = [self treeItemForTreeViewCell:treeViewCell];
    treeItem.expanded = expanded;
    if(treeItem.hasChildren)
    {
        [self reloadData];
        [self resetSelection:NO];
    }
    if([_treeViewDelegate respondsToSelector:@selector(treeView:treeItem:expanded:)])
        [_treeViewDelegate treeView:self treeItem:treeItem expanded:expanded];
}
#pragma mark - retrieve TreeItem object from special cell
-(DWTreeItem *)treeItemForTreeViewCell:(DWTreeViewCell *)treeViewCell
{
    NSIndexPath *indexPath = [self indexPathForCell:treeViewCell];
    return [self treeItemForIndexPath:indexPath];
}
-(DWTreeItem *)treeItemForIndexPath:(NSIndexPath *)indexPath
{
    DWTreeItem *treeItem = nil;
    if([_treeViewDelegate respondsToSelector:@selector(treeView:treeItemForRow:)])
        treeItem = [_treeViewDelegate treeView:self treeItemForRow:indexPath.row];
    NSAssert(treeItem, @"Can't get the Tree Node data");
    return treeItem;
}
@end

/* Subclass for a Calendar type */
@interface DWCalendar : UIDatePicker
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWCalendar
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a stepper component of the spinbutton type */
/* This is a bad way of doing this... but I can't get the other methods to work */
@interface DWStepper : UIStepper
{
    id textfield;
    id parent;
}
-(void)setTextfield:(id)input;
-(id)textfield;
-(void)setParent:(id)input;
-(id)parent;
@end

@implementation DWStepper
-(void)setTextfield:(id)input { textfield = input; }
-(id)textfield { return textfield; }
-(void)setParent:(id)input { parent = input; }
-(id)parent { return parent; }
@end

/* Subclass for a Spinbutton type */
@interface DWSpinButton : UIView <UITextFieldDelegate>
{
    void *userdata;
    UITextField *textfield;
    DWStepper *stepper;
    id clickDefault;
}
-(id)init;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(UITextField *)textfield;
-(UIStepper *)stepper;
-(void)textFieldDidEndEditing:(UITextField *)textField;
-(void)setClickDefault:(id)input;
@end

@implementation DWSpinButton
-(id)init
{
    self = [super init];

    if(self)
    {
        textfield = [[UITextField alloc] init];
        [self addSubview:textfield];
        stepper = [[DWStepper alloc] init];
        [self addSubview:stepper];
        [stepper setParent:self];
        [stepper setTextfield:textfield];
        [textfield setText:[NSString stringWithFormat:@"%ld",(long)[stepper value]]];
        [textfield setDelegate:self];
        [stepper addTarget:self action:@selector(stepperChanged:) forControlEvents:UIControlEventValueChanged];
    }
    return self;
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(UITextField *)textfield { return textfield; }
-(UIStepper *)stepper { return stepper; }
-(void)textFieldDidEndEditing:(UITextField *)textField
{
    long val = [[textfield text] intValue];
    [stepper setValue:(float)val];
    [textfield setText:[NSString stringWithFormat:@"%d", (int)[stepper value]]];
    _dw_event_handler(self, DW_INT_TO_POINTER((int)[stepper value]), _DW_EVENT_VALUE_CHANGED);
}
-(void)stepperChanged:(UIStepper*)theStepper
{
    [textfield setText:[NSString stringWithFormat:@"%d", (int)[theStepper value]]];
    _dw_event_handler(self, DW_INT_TO_POINTER((int)[stepper value]), _DW_EVENT_VALUE_CHANGED);
}
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)dealloc
{
    UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    [textfield removeFromSuperview];
    [textfield release];
    [stepper removeFromSuperview];
    [stepper release];
    [super dealloc];
}
@end

API_AVAILABLE(ios(10.0))
@interface DWUserNotificationCenterDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation DWUserNotificationCenterDelegate
/* Called when a notification is delivered to a foreground app. */
-(void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler API_AVAILABLE(macos(10.14))
{
    completionHandler(UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge);
}
/* Called to let your app know which action was selected by the user for a given notification. */
-(void)userNotificationCenter:(UNUserNotificationCenter *)center didReceiveNotificationResponse:(UNNotificationResponse *)response withCompletionHandler:(void(^)(void))completionHandler API_AVAILABLE(macos(10.14))
{
    NSScanner *objScanner = [NSScanner scannerWithString:response.notification.request.identifier];
    unsigned long long handle;
    HWND notification;

    /* Skip the dw-notification- prefix */
    [objScanner scanString:@"dw-notification-" intoString:nil];
    [objScanner scanUnsignedLongLong:&handle];
    notification = DW_UINT_TO_POINTER(handle);
    
    if ([response.actionIdentifier isEqualToString:UNNotificationDismissActionIdentifier])
    {
        /* The user dismissed the notification without taking action. */
        dw_signal_disconnect_by_window(notification);
    }
    else if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier])
    {
        /* The user launched the app. */
        _dw_event_handler(notification, nil, _DW_EVENT_CLICKED);
        dw_signal_disconnect_by_window(notification);
    }
    completionHandler();
}
@end

@interface DWComboBox : UITextField <UIPickerViewDelegate,UIPickerViewDataSource,UITextFieldDelegate>
{
    UIPickerView* pickerView;
    NSMutableArray* dataArray;
    UIBarStyle toolbarStyle;
    int selectedIndex;
    void *userdata;
}
-(void)setToolbarStyle:(UIBarStyle)style;
-(void)append:(NSString *)item;
-(void)insert:(NSString *)item atIndex:(int)index;
-(void)clear;
-(int)count;
-(NSString *)getTextAtIndex:(int)index;
-(void)setText:(NSString *)item atIndex:(int)index;
-(void)deleteAtIndex:(int)index;
-(int)selectedIndex;
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWComboBox
-(id)init
{
    self = [super init];
    if(self)
    {
        [self setDelegate:self];

        /* Set UI defaults */
        toolbarStyle = UIBarStyleDefault;
        dataArray = [[NSMutableArray alloc] init];

        /* Setup the arrow image */
        UIButton *imageButton = [UIButton buttonWithType:UIButtonTypeCustom];
        UIImage *image = [UIImage systemImageNamed:@"chevron.down"];
        [imageButton setImage:image forState:UIControlStateNormal];
        [self setRightView:imageButton];
        [self setRightViewMode:UITextFieldViewModeAlways];
        [imageButton addTarget:self action:@selector(showPicker:) forControlEvents:UIControlEventTouchUpInside];
        selectedIndex = -1;
    }
    return self;
}
-(void)setToolbarStyle:(UIBarStyle)style { toolbarStyle = style; }
-(void)append:(NSString *)item { if(item) [dataArray addObject:item]; }
-(void)insert:(NSString *)item atIndex:(int)index { if(item) [dataArray insertObject:item atIndex:index]; }
-(void)clear { [dataArray removeAllObjects]; }
-(int)count { return (int)[dataArray count]; }
-(void)selectIndex:(int)index
{
    if(index > -1 && index < (int)[dataArray count])
    {
        selectedIndex = index;
        [self setText:[dataArray objectAtIndex:index]];
    }
}
-(NSString *)getTextAtIndex:(int)index
{
    if(index > -1 && index < [dataArray count])
        return [dataArray objectAtIndex:index];
    return nil;
}
-(void)setText:(NSString *)item atIndex:(int)index
{
    if(item && index > -1 && index < [dataArray count])
        [dataArray replaceObjectAtIndex:index withObject:item];
}
-(void)deleteAtIndex:(int)index { if(index > -1 && index < [dataArray count]) [dataArray removeObjectAtIndex:index]; }
-(NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView { return 1; }
-(void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
    if(row > -1 && row < [dataArray count])
    {
        selectedIndex = (int)row;
        [self setText:[dataArray objectAtIndex:row]];
        [self sendActionsForControlEvents:UIControlEventValueChanged];
        _dw_event_handler(self, DW_INT_TO_POINTER(selectedIndex), _DW_EVENT_LIST_SELECT);
    }
}
-(NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component { return [dataArray count]; }
-(NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
    if(row > -1 && row < [dataArray count])
        return [dataArray objectAtIndex:row];
    return nil;
}
-(void)doneClicked:(id)sender
{
    /* Hides the pickerView */
    [self resignFirstResponder];
    
    if([[self text] length] == 0 || ![dataArray containsObject:[self text]])
    {
        selectedIndex = -1;
    }
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
-(void)cancelClicked:(id)sender
{
    /* Hides the pickerView */
    [self resignFirstResponder];
}
-(void)showPicker:(id)sender
{
    [self resignFirstResponder];

    pickerView = [[UIPickerView alloc] init];
    [pickerView setDataSource:self];
    [pickerView setDelegate:self];

    /* If the text field is empty show the place holder otherwise show the last selected option */
    if([[self text] length] == 0 || ![dataArray containsObject:[self text]])
    {
        [pickerView selectRow:0 inComponent:0 animated:YES];
    }
    else
    {
        if([dataArray containsObject:[self text]])
        {
            [pickerView selectRow:[dataArray indexOfObject:[self text]] inComponent:0 animated:YES];
        }
    }

    UIToolbar* toolbar = [[UIToolbar alloc] init];
    [toolbar setBarStyle:toolbarStyle];
    
    /* Space between buttons */
    UIBarButtonItem *flexibleSpace = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                                                                                   target:nil
                                                                                   action:nil];
    
    UIBarButtonItem *doneButton = [[UIBarButtonItem alloc]
                                   initWithTitle:@"Done"
                                   style:UIBarButtonItemStyleDone
                                   target:self
                                   action:@selector(doneClicked:)];

    UIBarButtonItem *cancelButton = [[UIBarButtonItem alloc]
                                    initWithTitle:@"Cancel"
                                    style:UIBarButtonItemStylePlain
                                    target:self
                                    action:@selector(cancelClicked:)];
        
    [toolbar setItems:[NSArray arrayWithObjects:cancelButton, flexibleSpace, doneButton, nil]];
    [toolbar sizeToFit];

    /* Custom input view */
    [self setInputView:pickerView];
    [self setInputAccessoryView:toolbar];
    [super becomeFirstResponder];
}
-(BOOL)becomeFirstResponder
{
    [self setInputView:nil];
    [self setInputAccessoryView:nil];
    return [super becomeFirstResponder];
}
-(int)selectedIndex { return selectedIndex; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc
{
    UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    [dataArray release];
    [super dealloc];
}
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
void _dw_new_signal(ULONG message, HWND window, int msgid, void *signalfunction, void *discfunc, void *data)
{
    DWSignalHandler *new = malloc(sizeof(DWSignalHandler));

    new->message = message;
    new->window = window;
    new->id = msgid;
    new->signalfunction = signalfunction;
    new->discfunction = discfunc;
    new->data = data;
    new->next = NULL;

    if (!DWRoot)
        DWRoot = new;
    else
    {
        DWSignalHandler *prev = NULL, *tmp = DWRoot;
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
            DWRoot = new;
    }
}

/* Finds the message number for a given signal name */
ULONG _dw_findsigmessage(const char *signame)
{
    int z;

    for(z=0;z<(_DW_EVENT_MAX-1);z++)
    {
        if(strcasecmp(signame, DWSignalTranslate[z].name) == 0)
            return DWSignalTranslate[z].message;
    }
    return 0L;
}

unsigned long _dw_foreground = 0xAAAAAA, _dw_background = 0;

void _dw_handle_resize_events(Box *thisbox)
{
    int z;

    for(z=0;z<thisbox->count;z++)
    {
        id handle = thisbox->items[z].hwnd;

        if(thisbox->items[z].type == _DW_TYPE_BOX)
        {
            Box *tmp = (Box *)[handle box];

            if(tmp)
            {
                _dw_handle_resize_events(tmp);
            }
        }
        else
        {
            if([handle isMemberOfClass:[DWRender class]])
            {
                DWRender *render = (DWRender *)handle;
                CGSize oldsize = [render size];
                CGSize newsize = [render frame].size;

                /* Eliminate duplicate configure requests */
                if(oldsize.width != newsize.width || oldsize.height != newsize.height)
                {
                    if(newsize.width > 0 && newsize.height > 0)
                    {
                        [render setSize:newsize];
                        _dw_event_handler(handle, nil, _DW_EVENT_CONFIGURE);
                    }
                }
            }
            /* Special handling for notebook controls */
            else if([handle isMemberOfClass:[DWNotebook class]])
            {
                DWNotebook *notebook = handle;
                NSInteger intpageid = [[notebook tabs] selectedSegmentIndex];
                
                if(intpageid != -1 && intpageid < [[notebook views] count])
                {
                    DWNotebookPage *page = [[notebook views] objectAtIndex:intpageid];

                    if([page isKindOfClass:[DWBox class]])
                    {
                        Box *box = [page box];
                        _dw_handle_resize_events(box);
                    }
                }
            }
            /* Handle laying out scrollviews... if required space is less
             * than available space, then expand.  Otherwise use required space.
             */
            else if([handle isMemberOfClass:[DWScrollBox class]])
            {
                DWScrollBox *scrollbox = (DWScrollBox *)handle;
                NSArray *subviews = [scrollbox subviews];
                DWBox *contentbox = [subviews firstObject];
                Box *thisbox = [contentbox box];

                /* Get the required space for the box */
                _dw_handle_resize_events(thisbox);
            }
        }
    }
}

/* This function calculates how much space the widgets and boxes require
 * and does expansion as necessary.
 */
static void _dw_resize_box(Box *thisbox, int *depth, int x, int y, int pass)
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

    /* Count up all the space for all items in the box */
    for(z=0;z<thisbox->count;z++)
    {
        int itempad, itemwidth, itemheight;

        if(thisbox->items[z].type == _DW_TYPE_BOX)
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
                    _dw_resize_box(tmp, depth, x, y, pass);

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

            if(thisbox->items[z].hsize != _DW_SIZE_EXPAND)
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
            if(thisbox->items[z].vsize != _DW_SIZE_EXPAND)
                thisbox->usedpady += itemheight;
            else
                thisbox->usedpady += itempad;
        }
        else
        {
            if(itemheight > uymax)
                uymax = itemheight;
            if(thisbox->items[z].vsize != _DW_SIZE_EXPAND)
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
            if(thisbox->items[z].hsize != _DW_SIZE_EXPAND)
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
            if(thisbox->items[z].hsize == _DW_SIZE_EXPAND)
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
            if(thisbox->items[z].vsize == _DW_SIZE_EXPAND)
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
                id handle = thisbox->items[z].hwnd;
                CGRect rect;

                rect.origin.x = currentx + pad;
                rect.origin.y = currenty + pad;
                rect.size.width = width;
                rect.size.height = height;
                [handle setFrame:rect];

                /* After placing a box... place its components */
                if(thisbox->items[z].type == _DW_TYPE_BOX)
                {
                    id box = thisbox->items[z].hwnd;
                    Box *tmp = (Box *)[box box];

                    if(tmp)
                    {
                        (*depth)++;
                        _dw_resize_box(tmp, depth, width, height, pass);
                        (*depth)--;
                    }
                }

                /* Special handling for notebook controls */
                if([handle isMemberOfClass:[DWNotebook class]])
                {
                    DWNotebook *notebook = handle;
                    NSInteger intpageid = [[notebook tabs] selectedSegmentIndex];

                    if(intpageid == -1)
                    {
                        if([[notebook tabs] numberOfSegments] > 0)
                        {
                            DWNotebookPage *notepage = [[notebook views] firstObject];

                            /* If there is no selected segment, select the first one... */
                            [[notebook tabs] setSelectedSegmentIndex:0];
                            intpageid = 0;
                            [notepage setHidden:NO];
                            [notebook setVisible:notepage];
                        }
                    }

                    if(intpageid != -1 && intpageid < [[notebook views] count])
                    {
                        DWNotebookPage *page = [[notebook views] objectAtIndex:intpageid];

                        /* If the new page is a valid box, lay it out */
                        if([page isKindOfClass:[DWBox class]])
                        {
                            Box *box = [page box];
                            /* Start with the entire notebook size and then adjust
                             * it to account for the segement control's height.
                             */
                            NSInteger height = [[notebook tabs] frame].size.height;
                            CGRect frame = [notebook frame];
                            frame.origin.y += height;
                            frame.size.height -= height;
                            [page setFrame:frame];
                            _dw_do_resize(box, frame.size.width, frame.size.height);
                            _dw_handle_resize_events(box);
                        }
                    }
                }
                /* Handle laying out scrollviews... if required space is less
                 * than available space, then expand.  Otherwise use required space.
                 */
                else if([handle isMemberOfClass:[DWScrollBox class]])
                {
                    int depth = 0;
                    DWScrollBox *scrollbox = (DWScrollBox *)handle;
                    NSArray *subviews = [scrollbox subviews];
                    DWBox *contentbox = [subviews firstObject];
                    Box *thisbox = [contentbox box];
                    /* We start with the content being the available size */
                    CGRect frame = CGRectMake(0,0,rect.size.width,rect.size.height);

                    /* Get the required space for the box */
                    _dw_resize_box(thisbox, &depth, x, y, 1);

                    /* Expand the content box to the size of the contents */
                    if(frame.size.width < thisbox->minwidth)
                    {
                        frame.size.width = thisbox->minwidth;
                    }
                    if(frame.size.height < thisbox->minheight)
                    {
                        frame.size.height = thisbox->minheight;
                    }
                    [scrollbox setContentSize:frame.size];
                    [contentbox setFrame:frame];

                    /* Layout the content of the scrollbox */
                    _dw_do_resize(thisbox, frame.size.width, frame.size.height);
                    _dw_handle_resize_events(thisbox);
                }
                /* Special handling for spinbutton controls */
                else if([handle isMemberOfClass:[DWSpinButton class]])
                {
                    DWSpinButton *spinbutton = (DWSpinButton *)handle;
                    UITextField *textfield = [spinbutton textfield];
                    UIStepper *stepper = [spinbutton stepper];
                    NSInteger stepperwidth = [stepper intrinsicContentSize].width;
                    [textfield setFrame:CGRectMake(0,0,rect.size.width-stepperwidth,rect.size.height)];
                    [stepper setFrame:CGRectMake(rect.size.width-stepperwidth,0,stepperwidth,rect.size.height)];
                }
                else if([handle isMemberOfClass:[DWSplitBar class]])
                {
                    DWSplitBar *split = (DWSplitBar *)handle;
                    [split resize];
                }
                else if([handle isMemberOfClass:[DWMLE class]])
                {
                    DWMLE *mle = (DWMLE *)handle;

                    if([[mle textContainer] lineBreakMode] == NSLineBreakByClipping)
                        [[mle textContainer] setSize:CGRectInfinite.size];
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

static void _dw_do_resize(Box *thisbox, int x, int y)
{
    if(x > 0 && y > 0)
    {
        if(thisbox)
        {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, x, y, 1);

            /* Finally place all the boxes and controls */
            _dw_resize_box(thisbox, &depth, x, y, 2);
        }
    }
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
    /* We don't actually run a loop here,
     * we launched a new thread to run the loop there.
     * Just wait for dw_main_quit() on the DWMainEvent.
     */
    dw_event_wait(DWMainEvent, DW_TIMEOUT_INFINITE);
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
    dw_event_post(DWMainEvent);
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
            DWThread = curr;
        }
        /* Process any pending events */
        _dw_main_iteration(until);
        if(orig == (DWTID)-1)
        {
            DWThread = orig;
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
    return [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:date];
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
    DWTID curr = pthread_self();

    if(DWThread == (DWTID)-1)
    {
        DWThread = curr;
        _dw_main_iteration([NSDate distantPast]);
        DWThread = (DWTID)-1;
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
char * API dw_user_dir(void)
{
    static char _user_dir[PATH_MAX+1] = { 0 };

    if(!_user_dir[0])
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                             NSUserDomainMask, YES);
        NSString *nshome = [paths firstObject];
        const char *home;

        if(nshome && (home = [nshome UTF8String]))
            strncpy(_user_dir, home, PATH_MAX);
        else if((nshome = NSHomeDirectory()) && (home = [nshome UTF8String]))
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
 * Sets the application ID used by this Dynamic Windows application instance.
 * Parameters:
 *         appid: A string typically in the form: com.company.division.application
 *         appname: The application name used on Windows or NULL.
 * Returns:
 *         DW_ERROR_NONE after successfully setting the application ID.
 *         DW_ERROR_UNKNOWN if unsupported on this system.
 *         DW_ERROR_GENERAL if the application ID is not allowed.
 * Remarks:
 *          This must be called before dw_init().  If dw_init() is called first
 *          it will create a unique ID in the form: org.dbsoft.dwindows.application
 *          or if the application name cannot be detected: org.dbsoft.dwindows.pid.#
 *          The appname is only required on Windows.  If NULL is passed the detected
 *          application name will be used, but a prettier name may be desired.
 */
int dw_app_id_set(const char *appid, const char *appname)
{
    strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
    return DW_ERROR_NONE;
}

/*
 * Displays a debug message on the console...
 * Parameters:
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
void API dw_debug(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   dw_vdebug(format, args);
   va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
   NSString *nformat = [NSString stringWithUTF8String:format];

   NSLogv(nformat, args);
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: flags to indicate buttons and icon
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int API dw_messagebox(const char *title, int flags, const char *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = dw_vmessagebox(title, flags, format, args);
   va_end(args);
   return rc;
}

int API dw_vmessagebox(const char *title, int flags, const char *format, va_list args)
{
    NSInteger iResponse;
    NSString *button1 = @"OK";
    NSString *button2 = nil;
    NSString *button3 = nil;
    NSString *mtitle = [NSString stringWithUTF8String:title];
    NSString *mtext;
    UIAlertControllerStyle mstyle = UIAlertControllerStyleAlert;
    NSArray *params;
    static int in_mb = FALSE;

    /* Prevent recursion */
    if(in_mb)
        return 0;
    in_mb = TRUE;

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

    mtext = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:format] arguments:args];

    params = [NSMutableArray arrayWithObjects:mtitle, mtext, [NSNumber numberWithInteger:mstyle], button1, button2, button3, nil];
    [DWObj safeCall:@selector(messageBox:) withObject:params];
    in_mb = FALSE;
    iResponse = [[params lastObject] integerValue];

    switch(iResponse)
    {
        case 1:    /* user pressed OK */
            if(flags & DW_MB_YESNO || flags & DW_MB_YESNOCANCEL)
            {
                return DW_MB_RETURN_YES;
            }
            return DW_MB_RETURN_OK;
        case 2:  /* user pressed Cancel */
            if(flags & DW_MB_OKCANCEL)
            {
                return DW_MB_RETURN_CANCEL;
            }
            return DW_MB_RETURN_NO;
        case 3:      /* user pressed the third button */
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
char * API dw_file_browse(const char *title, const char *defpath, const char *ext, int flags)
{
    NSPointerArray *params = [[NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory] retain];
    char *file = NULL;

    [params addPointer:(void *)defpath];
    [params addPointer:(void *)ext];
    [params addPointer:DW_INT_TO_POINTER(flags)];
    [DWObj safeCall:@selector(filePicker:) withObject:params];
    if([params count] > 3)
        file = [params pointerAtIndex:3];

    return file;
}

/*
 * Gets the contents of the default clipboard as text.
 * Parameters:
 *       None.
 * Returns:
 *       Pointer to an allocated string of text or NULL if clipboard empty or contents could not
 *       be converted to text.
 */
char * API dw_clipboard_get_text(void)
{
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    NSString *str = [pasteboard string];
    if(str != nil)
        return strdup([str UTF8String]);
    return NULL;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];

    [pasteboard setString:[NSString stringWithUTF8String:str]];
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
DW_FUNCTION_DEFINITION(dw_box_new, HWND, int type, int pad)
DW_FUNCTION_ADD_PARAM2(type, pad)
DW_FUNCTION_RETURN(dw_box_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(type, int, pad, int)
{
    DW_FUNCTION_INIT;
    DWBox *view = [[[DWBox alloc] init] retain];
    Box *newbox = [view box];
    memset(newbox, 0, sizeof(Box));
    newbox->pad = pad;
    newbox->type = type;
    DW_FUNCTION_RETURN_THIS(view);
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 */
HWND API dw_groupbox_new(int type, int pad, const char *title)
{
    return dw_box_new(type, pad);
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_new, HWND, int type, int pad)
DW_FUNCTION_ADD_PARAM2(type, pad)
DW_FUNCTION_RETURN(dw_scrollbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(type, int, pad, int)
{
    DWScrollBox *scrollbox = [[[DWScrollBox alloc] init] retain];
    DWBox *box = dw_box_new(type, pad);
    DWBox *tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, box, 1, 1, TRUE, TRUE, 0);
    [scrollbox setBox:box];
    [scrollbox addSubview:tmpbox];
    [scrollbox setScrollEnabled:YES];
    [tmpbox release];
    DW_FUNCTION_RETURN_THIS(scrollbox);
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_get_pos, int, HWND handle, int orient)
DW_FUNCTION_ADD_PARAM2(handle, orient)
DW_FUNCTION_RETURN(dw_scrollbox_get_pos, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, orient, int)
{
    DWScrollBox *scrollbox = handle;
    NSArray *subviews = [scrollbox subviews];
    UIView *view = [subviews firstObject];
    CGSize contentsize = [scrollbox contentSize];
    CGPoint contentoffset = [scrollbox contentOffset];
    int range = 0;
    int val = 0;
    if(orient == DW_VERT)
    {
        range = [view bounds].size.height - contentsize.height;
        val = contentoffset.y;
    }
    else
    {
        range = [view bounds].size.width - contentsize.width;
        val = contentoffset.x;
    }
    if(val > range)
    {
        val = range;
    }
    DW_FUNCTION_RETURN_THIS(val);
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_get_range, int, HWND handle, int orient)
DW_FUNCTION_ADD_PARAM2(handle, orient)
DW_FUNCTION_RETURN(dw_scrollbox_get_range, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, orient, int)
{
    DWScrollBox *scrollbox = handle;
    NSArray *subviews = [scrollbox subviews];
    UIView *view = [subviews firstObject];
    int range = 0;
    if(orient == DW_VERT)
    {
        range = [view bounds].size.height;
    }
    else
    {
        range = [view bounds].size.width;
    }
    DW_FUNCTION_RETURN_THIS(range);
}

/* Return the handle to the text object */
id _dw_text_handle(id object)
{
    if([object isMemberOfClass:[DWButton class]])
    {
        DWButton *button = object;
        object = [button titleLabel];
    }
    else if([object isMemberOfClass:[DWSpinButton class]])
    {
        DWSpinButton *spinbutton = object;
        object = [spinbutton textfield];
    }
#if 0 /* TODO: Fix this when we have a groupbox implemented */
    if([object isMemberOfClass:[NSBox class]])
    {
        NSBox *box = object;
        id content = [box contentView];

        if([content isMemberOfClass:[DWText class]])
        {
            object = content;
        }
    }
#endif
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
void _dw_control_size(id handle, int *width, int *height)
{
    int thiswidth = 1, thisheight = 1, extrawidth = 0, extraheight = 0;
    NSString *nsstr = nil;
    id object = _dw_text_handle(handle);

    /* Handle all the different button types */
    if([handle isMemberOfClass:[DWButton class]])
    {
        UIImage *image = [handle imageForState:UIControlStateNormal];

        if(image)
        {
            /* Image button */
            CGSize size = [image size];
            extrawidth = (int)size.width + 1;
            /* Height isn't additive */
            thisheight = (int)size.height + 1;
        }
        /* Text button */
        nsstr = [[handle titleLabel] text];

        if(nsstr && [nsstr length] > 0)
        {
            /* With combined text/image we seem to
             * need a lot of additional width space.
             */
            if(image)
                extrawidth += 30;
            else
                extrawidth += 8;
            extraheight += 4;
        }
    }
    /* If the control is an entryfield set width to 150 */
    else if([object isKindOfClass:[UITextField class]])
    {
        UIFont *font = [object font];

        if([object isEditable])
        {
            /* Spinbutton text doesn't need to be as wide */
            if([handle isMemberOfClass:[DWSpinButton class]])
                thiswidth = 50;
            else
                thiswidth = 150;
        }
        nsstr = [object text];

        if(font)
            thisheight = (int)[font lineHeight];

        /* Spinbuttons need some extra */
        if([handle isMemberOfClass:[DWSpinButton class]])
        {
            DWSpinButton *spinbutton = handle;
            CGSize size = [[spinbutton stepper] intrinsicContentSize];
            
            /* Add the stepper width as extra... */
            extrawidth = size.width;
            /* The height should be the bigger of the two */
            if(size.height > thisheight)
                thisheight = size.height;
        }
    }
    /* Handle the ranged widgets */
    else if([object isMemberOfClass:[DWPercent class]] ||
            [object isMemberOfClass:[DWSlider class]])
    {
        if([object isMemberOfClass:[DWSlider class]] && [object vertical])
        {
            thiswidth = 25;
            thisheight = 100;
        }
        else
        {
            thiswidth = 100;
            thisheight = 25;
        }
    }
    /* Handle bitmap size */
    else if([object isMemberOfClass:[UIImageView class]])
    {
        UIImage *image = (UIImage *)[object image];

        if(image)
        {
            CGSize size = [image size];
            thiswidth = (int)size.width;
            thisheight = (int)size.height;
        }
    }
    /* Handle calendar */
    else if([object isMemberOfClass:[DWCalendar class]])
    {
        DWCalendar *calendar = object;
        CGSize size = [calendar intrinsicContentSize];

        /* If we can't detect the size... */
        if(size.width < 1 || size.height < 1)
        {
            if(DWOSMajor >= 14)
            {
                /* Bigger new style in ios 14 */
                thiswidth = 200;
                thisheight = 200;
            }
            else
            {
                /* Smaller spinner style in iOS 13 and earlier */
                thiswidth = 200;
                thisheight = 100;
            }
        }
        else
        {
            thiswidth = size.width;
            thisheight = size.height;
        }
    }
    /* MLE and Container */
    else if([object isMemberOfClass:[DWMLE class]] ||
            [object isMemberOfClass:[DWContainer class]])
    {
        CGSize size;

        if([object isMemberOfClass:[DWMLE class]])
            size = [object contentSize];
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
    else if([object isKindOfClass:[UILabel class]])
    {
        nsstr = [object text];
        extrawidth = extraheight = 2;
    }
#ifdef DW_INCLUDE_DEPRECATED
    /* Any other control type */
    else if([object isKindOfClass:[UIControl class]])
        nsstr = [object text];
#endif

    /* If we have a string...
     * calculate the size with the current font.
     */
    if(nsstr)
    {
        int textwidth, textheight;

        /* If we have an empty string, use "gT" to get the most height for the font */
        dw_font_text_extents_get(object, NULL, [nsstr length] ? (char *)[nsstr UTF8String] : "gT", &textwidth, &textheight);

        if(textheight > thisheight)
            thisheight = textheight;
        if(textwidth > thiswidth)
            thiswidth = textwidth;
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
    if([object isKindOfClass:[UIWindow class]])
    {
        UIWindow *window = box;
        NSArray *subviews = [[[window rootViewController] view] subviews];
        view = [subviews firstObject];
    }
    else if([object isMemberOfClass:[DWScrollBox class]])
    {
        DWScrollBox *scrollbox = box;
        view = [scrollbox box];
    }

    thisbox = [view box];
    thisitem = thisbox->items;
    object = item;

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
    if([object isKindOfClass:[DWBox class]])
       tmpitem[index].type = _DW_TYPE_BOX;
    else
    {
        if(width == 0 && hsize == FALSE)
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
        if(height == 0 && vsize == FALSE)
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Height and expand Vertical both unset for box: %x item: %x",box,item);

        tmpitem[index].type = _DW_TYPE_ITEM;
    }

    tmpitem[index].hwnd = item;
    tmpitem[index].origwidth = tmpitem[index].width = width;
    tmpitem[index].origheight = tmpitem[index].height = height;
    tmpitem[index].pad = pad;
    tmpitem[index].hsize = hsize ? _DW_SIZE_EXPAND : _DW_SIZE_STATIC;
    tmpitem[index].vsize = vsize ? _DW_SIZE_EXPAND : _DW_SIZE_STATIC;

    /* If either of the parameters are -1 (DW_SIZE_AUTO) ... calculate the size */
    if(width == DW_SIZE_AUTO || height == DW_SIZE_AUTO)
        _dw_control_size(object, width == DW_SIZE_AUTO ? &tmpitem[index].width : NULL, height == DW_SIZE_AUTO ? &tmpitem[index].height : NULL);

    thisbox->items = tmpitem;

    /* Update the item count */
    thisbox->count++;

    /* Add the item to the box */
    [view addSubview:this];
    [this release];

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
}

/*
 * Remove windows (widgets) from the box they are packed into.
 * Parameters:
 *       handle: Window handle of the packed item to be removed.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
DW_FUNCTION_DEFINITION(dw_box_unpack, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_box_unpack, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id object = handle;
    int retval = DW_ERROR_NONE;

    if([object isKindOfClass:[UIView class]] || [object isKindOfClass:[UIControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        if([parent isKindOfClass:[DWBox class]])
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
                retval = DW_ERROR_GENERAL;
            else
            {
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
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Remove windows (widgets) from a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be removed from.
 *       index: 0 based index of packed items.
 * Returns:
 *       Handle to the removed item on success, 0 on failure or padding.
 */
DW_FUNCTION_DEFINITION(dw_box_unpack_at_index, HWND, HWND box, int index)
DW_FUNCTION_ADD_PARAM2(box, index)
DW_FUNCTION_RETURN(dw_box_unpack_at_index, HWND)
DW_FUNCTION_RESTORE_PARAM2(box, HWND, index, int)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWBox *parent = (DWBox *)box;
    id object = nil;

    if([parent isKindOfClass:[DWBox class]])
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
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(object);
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
DW_FUNCTION_DEFINITION(dw_box_pack_at_index, void, HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM8(box, item, index, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_at_index)
DW_FUNCTION_RESTORE_PARAM8(box, HWND, item, HWND, index, int, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    _dw_box_pack(box, item, index, width, height, hsize, vsize, pad, "dw_box_pack_at_index()");
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_box_pack_start, void, HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM7(box, item, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_start)
DW_FUNCTION_RESTORE_PARAM7(box, HWND, item, HWND, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    /* 65536 is the table limit on GTK...
     * seems like a high enough value we will never hit it here either.
     */
    _dw_box_pack(box, item, 65536, width, height, hsize, vsize, pad, "dw_box_pack_start()");
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_box_pack_end, void, HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM7(box, item, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_end)
DW_FUNCTION_RESTORE_PARAM7(box, HWND, item, HWND, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    _dw_box_pack(box, item, 0, width, height, hsize, vsize, pad, "dw_box_pack_end()");
    DW_FUNCTION_RETURN_NOTHING;
}

/* Internal function to create a basic button, used by all button types */
HWND _dw_internal_button_new(const char *text, ULONG cid, UIButtonType type)
{
    DWButton *button = [[DWButton buttonWithType:type] retain];
    if(text)
    {
        [button setTitle:[NSString stringWithUTF8String:text] forState:UIControlStateNormal];
    }
    [button addTarget:button
               action:@selector(buttonClicked:)
     forControlEvents:UIControlEventTouchUpInside];
    [button setTag:cid];
    if(DWDefaultFont)
    {
        [[button titleLabel] setFont:DWDefaultFont];
    }
    return button;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_button_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_button_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWButton *button = _dw_internal_button_new(text, cid, UIButtonTypeSystem);
    [button setContentHorizontalAlignment:UIControlContentHorizontalAlignmentCenter];
    DW_FUNCTION_RETURN_THIS(button);
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_entryfield_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWEntryField *entry = [[[DWEntryField alloc] init] retain];
    [entry setText:[ NSString stringWithUTF8String:text ]];
    [entry setTag:cid];
    [entry setDelegate:entry];
    DW_FUNCTION_RETURN_THIS(entry);
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_password_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_entryfield_password_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWEntryField *entry = dw_entryfield_new(text, cid);
    [entry setSecureTextEntry:YES];
    [entry setDelegate:entry];
    DW_FUNCTION_RETURN_THIS(entry);
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_set_limit, void, HWND handle, unsigned long limit)
DW_FUNCTION_ADD_PARAM2(handle, limit)
DW_FUNCTION_NO_RETURN(dw_entryfield_set_limit)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, limit, unsigned long)
{
    DWEntryField *entry = handle;

    [entry setMaximumLength:limit];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new, HWND, DW_UNUSED(const char *text), ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_bitmapbutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(text), const char *, cid, ULONG)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", cid];
    UIImage *image = [[UIImage alloc] initWithContentsOfFile:filepath];
    DWButton *button = _dw_internal_button_new("", cid, UIButtonTypeCustom);
    if(image)
    {
        [button setImage:image forState:UIControlStateNormal];
        [image release];
    }
    DW_FUNCTION_RETURN_THIS(button);
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
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new_from_file, HWND, DW_UNUSED(const char *text), ULONG cid, const char *filename)
DW_FUNCTION_ADD_PARAM3(text, cid, filename)
DW_FUNCTION_RETURN(dw_bitmapbutton_new_from_file, HWND)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(text), const char *, cid, ULONG, filename, const char *)
{
    char *ext = _dw_get_image_extension(filename);

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    UIImage *image = [[UIImage alloc] initWithContentsOfFile:nstr];

    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[UIImage alloc] initWithContentsOfFile:nstr];
    }
    DWButton *button = _dw_internal_button_new("", cid, UIButtonTypeCustom);
    if(image)
    {
        [button setImage:image forState:UIControlStateNormal];
        [image release];
    }
    DW_FUNCTION_RETURN_THIS(button);
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
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new_from_data, HWND, DW_UNUSED(const char *text), ULONG cid, const char *data, int len)
DW_FUNCTION_ADD_PARAM4(text, cid, data, len)
DW_FUNCTION_RETURN(dw_bitmapbutton_new_from_data, HWND)
DW_FUNCTION_RESTORE_PARAM4(DW_UNUSED(text), const char *, cid, ULONG, data, const char *, len, int)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    UIImage *image = [[UIImage alloc] initWithData:thisdata];
    DWButton *button = _dw_internal_button_new("", cid, UIButtonTypeCustom);
    if(image)
    {
        [button setImage:image forState:UIControlStateNormal];
        [image release];
    }
    DW_FUNCTION_RETURN_THIS(button);
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_spinbutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWSpinButton *spinbutton = [[[DWSpinButton alloc] init] retain];
    UIStepper *stepper = [spinbutton stepper];
    UITextField *textfield = [spinbutton textfield];
    long val = atol(text);

    [stepper setStepValue:1];
    [stepper setTag:cid];
    [stepper setMinimumValue:-65536];
    [stepper setMaximumValue:65536];
    [stepper setValue:(float)val];
    [textfield setText:[NSString stringWithFormat:@"%ld",val]];
    DW_FUNCTION_RETURN_THIS(spinbutton);
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_set_pos, void, HWND handle, long position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_spinbutton_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, long)
{
    DWSpinButton *spinbutton = handle;
    UIStepper *stepper = [spinbutton stepper];
    UITextField *textfield = [spinbutton textfield];
    [stepper setValue:(float)position];
    [textfield setText:[NSString stringWithFormat:@"%ld",position]];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          upper: Upper limit.
 *          lower: Lower limit.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_set_limits, void, HWND handle, long upper, long lower)
DW_FUNCTION_ADD_PARAM3(handle, upper, lower)
DW_FUNCTION_NO_RETURN(dw_spinbutton_set_limits)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, upper, long, lower, long)
{
    DWSpinButton *spinbutton = handle;
    UIStepper *stepper = [spinbutton stepper];
    [stepper setMinimumValue:(double)lower];
    [stepper setMaximumValue:(double)upper];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_get_pos, long, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_spinbutton_get_pos, long)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DWSpinButton *spinbutton = handle;
    long retval;
    UIStepper *stepper = [spinbutton stepper];
    retval = (long)[stepper value];
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_radiobutton_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_radiobutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWButton *button = _dw_internal_button_new(text, cid, UIButtonTypeSystem);
    [button setType:_DW_BUTTON_TYPE_RADIO];
    DW_FUNCTION_RETURN_THIS(button);
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_slider_new, HWND, int vertical, int increments, ULONG cid)
DW_FUNCTION_ADD_PARAM3(vertical, increments, cid)
DW_FUNCTION_RETURN(dw_slider_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(vertical, int, increments, int, cid, ULONG)
{
    DWSlider *slider = [[[DWSlider alloc] init] retain];
    [slider setMaximumValue:(double)increments];
    [slider setMinimumValue:0];
    [slider setContinuous:YES];
    [slider addTarget:slider
               action:@selector(sliderChanged:)
     forControlEvents:UIControlEventValueChanged];
    [slider setTag:cid];
    [slider setVertical:(vertical ? YES : NO)];
    DW_FUNCTION_RETURN_THIS(slider);
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
    DWSlider *slider = handle;
    double val = [slider value];
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
    [slider setValue:(double)position];
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_scrollbar_new, HWND, int vertical, ULONG cid)
DW_FUNCTION_ADD_PARAM2(vertical, cid)
DW_FUNCTION_RETURN(dw_scrollbar_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(vertical, int, cid, ULONG)
{
    DWSlider *slider = dw_slider_new(vertical, 1, cid);
    [slider setMinimumTrackTintColor:[UIColor clearColor]];
    [slider setMaximumTrackTintColor:[UIColor clearColor]];
    DW_FUNCTION_RETURN_THIS(slider);
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
    return dw_slider_get_pos(handle);
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
    dw_slider_set_pos(handle, position);
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
    DWSlider *slider = handle;
    [slider setMaximumValue:(double)range];
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_percent_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_percent_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    DWPercent *percent = [[[DWPercent alloc] init] retain];
    [percent setTag:cid];
    DW_FUNCTION_RETURN_THIS(percent);
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
DW_FUNCTION_DEFINITION(dw_percent_set_pos, void, HWND handle, unsigned int position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_percent_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, unsigned int)
{
    DW_FUNCTION_INIT;
    DWPercent *percent = handle;

    /* Handle indeterminate */
    if(position == DW_PERCENT_INDETERMINATE)
    {
        [percent setProgress:0 animated:NO];
    }
    else
    {
        /* Handle normal */
        [percent setProgress:(float)position/100.0 animated:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_checkbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWButton *button = _dw_internal_button_new(text, cid, UIButtonTypeSystem);
    [button setType:_DW_BUTTON_TYPE_CHECK];
    DW_FUNCTION_RETURN_THIS(button);
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_get, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_checkbox_get, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DWButton *button = handle;
    int retval = FALSE;

    if([button checkState])
        retval = TRUE;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_set, void, HWND handle, int value)
DW_FUNCTION_ADD_PARAM2(handle, value)
DW_FUNCTION_NO_RETURN(dw_checkbox_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, value, int)
{
    DWButton *button = handle;
    [button setCheckState:value];
    DW_FUNCTION_RETURN_NOTHING;
}

/* Internal common function to create containers and listboxes */
HWND _dw_cont_new(ULONG cid, int multi)
{
    DWContainer *cont = [[[DWContainer alloc] init] retain];

    [cont setAllowsMultipleSelection:(multi ? YES : NO)];
    [cont setDataSource:cont];
    [cont setDelegate:cont];
    [cont setTag:cid];
    [cont setRowHeight:UITableViewAutomaticDimension];
    [cont setEstimatedRowHeight:(_dw_container_mode > DW_CONTAINER_MODE_DEFAULT ? 85.0 : 44.0)];
    [cont setRowBgOdd:DW_RGB_TRANSPARENT andEven:DW_RGB_TRANSPARENT];
    return cont;
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
DW_FUNCTION_DEFINITION(dw_listbox_new, HWND, ULONG cid, int multi)
DW_FUNCTION_ADD_PARAM2(cid, multi)
DW_FUNCTION_RETURN(dw_listbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, multi, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = _dw_cont_new(cid, multi);
    [cont setup];
    [cont addColumn:@"" andType:DW_CFA_STRING];
    DW_FUNCTION_RETURN_THIS(cont);
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
DW_FUNCTION_DEFINITION(dw_listbox_append, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_listbox_append)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo append:[NSString stringWithUTF8String:text]];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [NSString stringWithUTF8String:text];

        [cont addRow:_dw_table_cell_view_new(nil, nstr, nil)];
        [cont reloadData];
        [cont setNeedsDisplay];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Inserts the specified text into the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be inserted into.
 *          text: Text to insert into listbox.
 *          pos: 0-based position to insert text
 */
DW_FUNCTION_DEFINITION(dw_listbox_insert, void, HWND handle, const char *text, int pos)
DW_FUNCTION_ADD_PARAM3(handle, text, pos)
DW_FUNCTION_NO_RETURN(dw_listbox_insert)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, text, const char *, pos, int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo insert:[NSString stringWithUTF8String:text] atIndex:pos];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [NSString stringWithUTF8String:text];

        [cont insertRow:_dw_table_cell_view_new(nil, nstr, nil) at:pos];
        [cont reloadData];
        [cont setNeedsDisplay];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Appends the specified text items to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text strings to append into listbox.
 *          count: Number of text strings to append
 */
DW_FUNCTION_DEFINITION(dw_listbox_list_append, void, HWND handle, char **text, int count)
DW_FUNCTION_ADD_PARAM3(handle, text, count)
DW_FUNCTION_NO_RETURN(dw_listbox_list_append)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, text, char **, count, int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int z;

        for(z=0;z<count;z++)
        {
            NSString *nstr = [NSString stringWithUTF8String:text[z]];

            [combo append:nstr];
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int z;

        for(z=0;z<count;z++)
        {
            NSString *nstr = [NSString stringWithUTF8String:text[z]];

            [cont addRow:_dw_table_cell_view_new(nil, nstr, nil)];
        }
        [cont reloadData];
        [cont setNeedsDisplay];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_listbox_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_listbox_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo clear];
    }
    if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont clear];
        [cont reloadData];
        [cont setNeedsDisplay];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_listbox_count, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_listbox_count, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;
    int result = 0;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        result = [combo count];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        result = (int)[cont numberOfRowsInSection:0];
    }
    DW_FUNCTION_RETURN_THIS(result);
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
DW_FUNCTION_DEFINITION(dw_listbox_set_top, void, HWND handle, int top)
DW_FUNCTION_ADD_PARAM2(handle, top)
DW_FUNCTION_NO_RETURN(dw_listbox_set_top)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, top, int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSIndexPath *myIP = [NSIndexPath indexPathForRow:top inSection:0];

        [cont scrollToRowAtIndexPath:myIP atScrollPosition:UITableViewScrollPositionNone animated:NO];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Copies the given index item's text into buffer.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 *          length: Length of the buffer (including NULL).
 */
DW_FUNCTION_DEFINITION(dw_listbox_get_text, void, HWND handle, unsigned int index, char *buffer, unsigned int length)
DW_FUNCTION_ADD_PARAM4(handle, index, buffer, length)
DW_FUNCTION_NO_RETURN(dw_listbox_get_text)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, index, unsigned int, buffer, char *, length, unsigned int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        NSString *nstr = [combo getTextAtIndex:index];

        if(nstr)
            strncpy(buffer, [nstr UTF8String], length - 1);
        else
            *buffer = '\0';
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInSection:0];

        if(index > count)
        {
            *buffer = '\0';
        }
        else
        {
            DWTableViewCell *cell = [cont getRow:index];
            NSString *nstr = [[cell label] text];

            strncpy(buffer, [nstr UTF8String], length - 1);
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
DW_FUNCTION_DEFINITION(dw_listbox_set_text, void, HWND handle, unsigned int index, const char *buffer)
DW_FUNCTION_ADD_PARAM3(handle, index, buffer)
DW_FUNCTION_NO_RETURN(dw_listbox_set_text)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, index, unsigned int, buffer, char *)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo setText:[NSString stringWithUTF8String:buffer] atIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInSection:0];

        if(index <= count)
        {
            NSString *nstr = [NSString stringWithUTF8String:buffer];
            DWTableViewCell *cell = [cont getRow:index];

            [[cell label] setText:nstr];
            [cont reloadData];
            [cont setNeedsDisplay];
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
DW_FUNCTION_DEFINITION(dw_listbox_selected, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_listbox_selected, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;
    int result = -1;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        result = [combo selectedIndex];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSIndexPath *ip = [cont indexPathForSelectedRow];

        if(ip)
            result = (int)ip.row;
    }
    DW_FUNCTION_RETURN_THIS(result);
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
DW_FUNCTION_DEFINITION(dw_listbox_selected_multi, int, HWND handle, int where)
DW_FUNCTION_ADD_PARAM2(handle, where)
DW_FUNCTION_RETURN(dw_listbox_selected_multi, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, where, int)
{
    DW_FUNCTION_INIT;
    id object = handle;
    int retval = -1;

    if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSArray *selected = [cont indexPathsForSelectedRows];
        NSIndexPath *ip = [selected objectAtIndex:(where == -1 ? 0 :where)];

        if(ip)
            retval = (int)ip.row;
    }
    DW_FUNCTION_RETURN_THIS(retval)
}

/*
 * Sets the selection state of a given index.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 *          state: TRUE if selected FALSE if unselected.
 */
DW_FUNCTION_DEFINITION(dw_listbox_select, void, HWND handle, int index, DW_UNUSED(int state))
DW_FUNCTION_ADD_PARAM3(handle, index, state)
DW_FUNCTION_NO_RETURN(dw_listbox_select)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, index, int, DW_UNUSED(state), int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSIndexPath *ip = [NSIndexPath indexPathForRow:index inSection:0];
        
        [cont selectRowAtIndexPath:ip
                          animated:NO
                    scrollPosition:UITableViewScrollPositionNone];
    }
    else if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        [combo selectIndex:index];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
DW_FUNCTION_DEFINITION(dw_listbox_delete, void, HWND handle, int index)
DW_FUNCTION_ADD_PARAM2(handle, index)
DW_FUNCTION_NO_RETURN(dw_listbox_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, index, int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo deleteAtIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont removeRow:index];
        [cont reloadData];
        [cont setNeedsDisplay];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_combobox_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_combobox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWComboBox *combo = [[[DWComboBox alloc] init] retain];
    if(text)
        [combo setText:[NSString stringWithUTF8String:text]];
    [combo setTag:cid];
    DW_FUNCTION_RETURN_THIS(combo);
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_mle_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_mle_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    CGRect frame = CGRectMake(0, 0, 100, 50);
    NSTextContainer *tc = [[NSTextContainer alloc] initWithSize:frame.size];
    NSLayoutManager *lm = [[NSLayoutManager alloc] init];
    NSTextStorage *ts = [[NSTextStorage alloc] init];
    [lm addTextContainer:tc];
    [ts addLayoutManager:lm];
    DWMLE *mle = [[[DWMLE alloc] initWithFrame:frame textContainer:tc] retain];
    CGSize size = [mle intrinsicContentSize];

    size.width = size.height;
    [mle setAutoresizingMask:UIViewAutoresizingFlexibleHeight|UIViewAutoresizingFlexibleWidth];
    [mle setScrollEnabled:YES];
    [mle setTag:cid];
    DW_FUNCTION_RETURN_THIS(mle);
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 */
DW_FUNCTION_DEFINITION(dw_mle_import, unsigned int, HWND handle, const char *buffer, int startpoint)
DW_FUNCTION_ADD_PARAM3(handle, buffer, startpoint)
DW_FUNCTION_RETURN(dw_mle_import, unsigned int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, buffer, const char *, startpoint, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    unsigned int retval;
    NSTextStorage *ts = [mle textStorage];
    NSString *nstr = [NSString stringWithUTF8String:buffer];
    UIColor *fgcolor = [mle textColor];
    UIFont *font = [mle font];
    NSMutableDictionary *attributes = [[NSMutableDictionary alloc] init];
    [attributes setObject:(fgcolor ? fgcolor : [UIColor labelColor]) forKey:NSForegroundColorAttributeName];
    if(font)
        [attributes setObject:font forKey:NSFontAttributeName];
    NSAttributedString *nastr = [[NSAttributedString alloc] initWithString:nstr attributes:attributes];
    NSUInteger length = [ts length];
    if(startpoint < 0)
        startpoint = 0;
    if(startpoint > length)
        startpoint = (int)length;
    [ts insertAttributedString:nastr atIndex:startpoint];
    retval = (unsigned int)strlen(buffer) + startpoint;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
DW_FUNCTION_DEFINITION(dw_mle_export, void, HWND handle, char *buffer, int startpoint, int length)
DW_FUNCTION_ADD_PARAM4(handle, buffer, startpoint, length)
DW_FUNCTION_NO_RETURN(dw_mle_export)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, buffer, char *, startpoint, int, length, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    const char *tmp = [ms UTF8String];
    strncpy(buffer, tmp+startpoint, length);
    buffer[length] = '\0';
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Obtains information about an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          bytes: A pointer to a variable to return the total bytes.
 *          lines: A pointer to a variable to return the number of lines.
 */
DW_FUNCTION_DEFINITION(dw_mle_get_size, void, HWND handle, unsigned long *bytes, unsigned long *lines)
DW_FUNCTION_ADD_PARAM3(handle, bytes, lines)
DW_FUNCTION_NO_RETURN(dw_mle_get_size)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, bytes, unsigned long *, lines, unsigned long *)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
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
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be deleted from.
 *          startpoint: Point to start deleting text.
 *          length: Amount of text to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_mle_delete, void, HWND handle, int startpoint, int length)
DW_FUNCTION_ADD_PARAM3(handle, startpoint, length)
DW_FUNCTION_NO_RETURN(dw_mle_delete)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, startpoint, int, length, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
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
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_mle_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_mle_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    [ms deleteCharactersInRange:NSMakeRange(0, length)];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_visible, void, HWND handle, int line)
DW_FUNCTION_ADD_PARAM2(handle, line)
DW_FUNCTION_NO_RETURN(dw_mle_set_visible)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, line, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger numberOfLines, index, stringLength = [ms length];

    for(index=0, numberOfLines=0; index < stringLength && numberOfLines < line; numberOfLines++)
        index = NSMaxRange([ms lineRangeForRange:NSMakeRange(index, 0)]);

    if(line == numberOfLines)
    {
        [mle scrollRangeToVisible:[ms lineRangeForRange:NSMakeRange(index, 0)]];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_editable, void, HWND handle, int state)
DW_FUNCTION_ADD_PARAM2(handle, state)
DW_FUNCTION_NO_RETURN(dw_mle_set_editable)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, state, int)
{
    DWMLE *mle = handle;
    [mle setEditable:(state ? YES : NO)];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_word_wrap, void, HWND handle, int state)
DW_FUNCTION_ADD_PARAM2(handle, state)
DW_FUNCTION_NO_RETURN(dw_mle_set_word_wrap)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, state, int)
{
    DWMLE *mle = handle;

    [[mle textContainer] setLineBreakMode:(state ? NSLineBreakByWordWrapping : NSLineBreakByClipping)];
    [[mle textContainer] setWidthTracksTextView:(state ? YES : NO)];
    if(!state)
        [[mle textContainer] setSize:CGRectInfinite.size];
    else
    {
        CGSize size = [mle frame].size;
        [[mle textContainer] setSize:CGSizeMake(size.width, CGRectInfinite.size.height)];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
DW_FUNCTION_DEFINITION(dw_mle_set_auto_complete, void, HWND handle, int state)
DW_FUNCTION_ADD_PARAM2(handle, state)
DW_FUNCTION_NO_RETURN(dw_mle_set_auto_complete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, state, int)
{
    DWMLE *mle = handle;
    NSInteger autocorrect = (state & DW_MLE_COMPLETE_TEXT) ? UITextAutocorrectionTypeYes : UITextAutocorrectionTypeNo;
    [mle setAutocorrectionType:autocorrect];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_cursor, void, HWND handle, int point)
DW_FUNCTION_ADD_PARAM2(handle, point)
DW_FUNCTION_NO_RETURN(dw_mle_set_cursor)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, point, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    if(point < 0)
        point = 0;
    if(point > length)
        point = (int)length;
    [mle setSelectedRange: NSMakeRange(point,point)];
    [mle scrollRangeToVisible:NSMakeRange(point,point)];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 */
DW_FUNCTION_DEFINITION(dw_mle_search, int, HWND handle, const char *text, int point, unsigned long flags)
DW_FUNCTION_ADD_PARAM4(handle, text, point, flags)
DW_FUNCTION_RETURN(dw_mle_search, int)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, text, const char *, point, int, flags, unsigned long)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSString *searchForMe = [NSString stringWithUTF8String:text];
    NSRange searchRange = NSMakeRange(point, [ms length] - point);
    NSRange range = NSMakeRange(NSNotFound, 0);
    NSUInteger options = flags ? flags : NSCaseInsensitiveSearch;
    int retval = -1;

    if(ms)
        range = [ms rangeOfString:searchForMe options:options range:searchRange];
    if(range.location == NSNotFound)
        retval = (int)range.location;
    DW_FUNCTION_RETURN_THIS(retval);
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
DW_FUNCTION_DEFINITION(dw_status_text_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_status_text_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWText *textfield = [[[DWText alloc] init] retain];
    [textfield setText:[NSString stringWithUTF8String:text]];
    [textfield setTag:cid];
    if(DWDefaultFont)
    {
        [textfield setFont:DWDefaultFont];
    }
    [textfield layer].borderWidth = 2.0;
    [textfield layer].borderColor = [[UIColor darkGrayColor] CGColor];
    DW_FUNCTION_RETURN_THIS(textfield);
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_text_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_text_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
    DWText *textfield = [[[DWText alloc] init] retain];
    [textfield setText:[NSString stringWithUTF8String:text]];
    [textfield setTag:cid];
    if(DWDefaultFont)
    {
        [textfield setFont:DWDefaultFont];
    }
    DW_FUNCTION_RETURN_THIS(textfield);
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_render_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_render_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    DWRender *render = [[[DWRender alloc] init] retain];
    UIContextMenuInteraction *interaction = [[UIContextMenuInteraction alloc] initWithDelegate:render];
    [render setTag:cid];
    [render addInteraction:interaction];
    DW_FUNCTION_RETURN_THIS(render);
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
DW_FUNCTION_DEFINITION(dw_render_redraw, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_render_redraw)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    DWRender *render = (DWRender *)handle;

    [render setNeedsDisplay];
    DW_FUNCTION_RETURN_NOTHING;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
    UIColor *oldcolor = pthread_getspecific(_dw_fg_color_key);
    UIColor *newcolor;
    DW_LOCAL_POOL_IN;

    _dw_foreground = _dw_get_color(value);

    newcolor = [[UIColor colorWithRed:  DW_RED_VALUE(_dw_foreground)/255.0 green:
                                        DW_GREEN_VALUE(_dw_foreground)/255.0 blue:
                                        DW_BLUE_VALUE(_dw_foreground)/255.0 alpha: 1] retain];
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
    UIColor *oldcolor = pthread_getspecific(_dw_bg_color_key);
    UIColor *newcolor;
    DW_LOCAL_POOL_IN;

    if(value == DW_CLR_DEFAULT)
    {
        pthread_setspecific(_dw_bg_color_key, NULL);
    }
    else
    {
        _dw_background = _dw_get_color(value);

        newcolor = [[UIColor colorWithRed:  DW_RED_VALUE(_dw_background)/255.0 green:
                                            DW_GREEN_VALUE(_dw_background)/255.0 blue:
                                            DW_BLUE_VALUE(_dw_background)/255.0 alpha: 1] retain];
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
    NSMutableArray *params = [NSMutableArray arrayWithObject:[NSNumber numberWithUnsignedLong:_dw_get_color(value)]];
    unsigned long newcolor = value;

    [DWObj safeCall:@selector(colorPicker:) withObject:params];
    if([params count] > 1)
        newcolor = [[params lastObject] unsignedLongValue];

    return newcolor;
}

CGContextRef _dw_draw_context(id source, bool antialiased)
{
    CGContextRef context = nil;

    if([source isMemberOfClass:[DWImage class]])
    {
        DWImage *image = source;

        context = [image cgcontext];
    }
    if(context)
        CGContextSetAllowsAntialiasing(context, antialiased);
    return context;
}

DWImage *_dw_dest_image(HPIXMAP pixmap, id object)
{
    if(pixmap && pixmap->image)
        return pixmap->image;
    if([object isMemberOfClass:[DWRender class]])
        return [object cachedImage];
    return nil;
}

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
DW_FUNCTION_DEFINITION(dw_draw_point, void, HWND handle, HPIXMAP pixmap, int x, int y)
DW_FUNCTION_ADD_PARAM4(handle, pixmap, x, y)
DW_FUNCTION_NO_RETURN(dw_draw_point)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, pixmap, HPIXMAP, x, int, y, int)
{
    DW_FUNCTION_INIT;
    DWImage *bi = _dw_dest_image(pixmap, handle);
    CGContextRef context = _dw_draw_context(bi, NO);
    
    if(context)
        UIGraphicsPushContext(context);

    if(bi)
    {
        UIBezierPath* aPath = [UIBezierPath bezierPath];
        [aPath setLineWidth: 0.5];
        [_dw_fg_color set];

        [aPath moveToPoint:CGPointMake(x, y)];
        [aPath stroke];
    }
    if(context)
        UIGraphicsPopContext();
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_draw_line, void, HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2)
DW_FUNCTION_ADD_PARAM6(handle, pixmap, x1, y1, x2, y2)
DW_FUNCTION_NO_RETURN(dw_draw_line)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, pixmap, HPIXMAP, x1, int, y1, int, x2, int, y2, int)
{
    DW_FUNCTION_INIT;
    DWImage *bi = _dw_dest_image(pixmap, handle);
    CGContextRef context = _dw_draw_context(bi, NO);

    if(context)
        UIGraphicsPushContext(context);

    if(bi)
    {
        UIBezierPath* aPath = [UIBezierPath bezierPath];
        [_dw_fg_color set];

        [aPath moveToPoint:CGPointMake(x1 + 0.5, y1 + 0.5)];
        [aPath addLineToPoint:CGPointMake(x2 + 0.5, y2 + 0.5)];
        [aPath stroke];
    }
    if(context)
        UIGraphicsPopContext();
    DW_FUNCTION_RETURN_NOTHING;
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 *       text: Text to be displayed.
 */
DW_FUNCTION_DEFINITION(dw_draw_text, void, HWND handle, HPIXMAP pixmap, int x, int y, const char *text)
DW_FUNCTION_ADD_PARAM5(handle, pixmap, x, y, text)
DW_FUNCTION_NO_RETURN(dw_draw_text)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pixmap, HPIXMAP, x, int, y, int, text, const char *)
{
    DW_FUNCTION_INIT;
    NSString *nstr = [ NSString stringWithUTF8String:text ];
    DWImage *bi = nil;
    UIFont *font = nil;
    DWRender *render;
    id image = handle;

    if(pixmap)
    {
        bi = (id)pixmap->image;
        font = pixmap->font;
        render = pixmap->handle;
        if(!font && [render isMemberOfClass:[DWRender class]])
        {
            font = [render font];
        }
    }
    else if(image && [image isMemberOfClass:[DWRender class]])
    {
        render = image;
        font = [render font];
        bi = [render cachedImage];
    }
    if(bi)
    {
        CGContextRef context = _dw_draw_context(bi, NO);

        if(context)
            UIGraphicsPushContext(context);

        NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithObjectsAndKeys:_dw_fg_color, NSForegroundColorAttributeName, nil];
        if(_dw_bg_color)
            [dict setValue:_dw_bg_color forKey:NSBackgroundColorAttributeName];
        if(font)
            [dict setValue:font forKey:NSFontAttributeName];
        [nstr drawAtPoint:CGPointMake(x, y) withAttributes:dict];
        [dict release];
        if(context)
            UIGraphicsPopContext();
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height Pointer to a variable to be filled in with the height.
 */
void API dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
{
    id object = handle;
    NSString *nstr;
    UIFont *font = nil;
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
    if(!font && ([object isMemberOfClass:[DWRender class]] || [object isKindOfClass:[UIControl class]]
                 || [object isKindOfClass:[UILabel class]]))
    {
        font = [object font];
    }
    /* If we got a font... add it to the dictionary */
    if(font)
    {
        [dict setValue:font forKey:NSFontAttributeName];
    }
    /* Calculate the size of the string */
    CGSize size = [nstr sizeWithAttributes:dict];
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
DW_FUNCTION_DEFINITION(dw_draw_polygon, void, HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
DW_FUNCTION_ADD_PARAM6(handle, pixmap, flags, npoints, x, y)
DW_FUNCTION_NO_RETURN(dw_draw_polygon)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, pixmap, HPIXMAP, flags, int, npoints, int, x, int *, y, int *)
{
    DW_FUNCTION_INIT;
    DWImage *bi = _dw_dest_image(pixmap, handle);
    CGContextRef context = _dw_draw_context(bi, flags & DW_DRAW_NOAA ? NO : YES);

    if(context)
        UIGraphicsPushContext(context);

    if(bi)
    {
        UIBezierPath* aPath = [UIBezierPath bezierPath];
        int z;

        [_dw_fg_color set];

        [aPath moveToPoint:CGPointMake(*x + 0.5, *y + 0.5)];
        for(z=1;z<npoints;z++)
        {
            [aPath addLineToPoint:CGPointMake(x[z] + 0.5, y[z] + 0.5)];
        }
        [aPath closePath];
        if(flags & DW_DRAW_FILL)
            [aPath fill];
        else
            [aPath stroke];
    }
    if(context)
        UIGraphicsPopContext();
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_draw_rect, void, HWND handle, HPIXMAP pixmap, int flags, int x, int y, int width, int height)
DW_FUNCTION_ADD_PARAM7(handle, pixmap, flags, x, y, width, height)
DW_FUNCTION_NO_RETURN(dw_draw_rect)
DW_FUNCTION_RESTORE_PARAM7(handle, HWND, pixmap, HPIXMAP, flags, int, x, int, y, int, width, int, height, int)
{
    DW_FUNCTION_INIT;
    DWImage *bi = _dw_dest_image(pixmap, handle);
    CGContextRef context = _dw_draw_context(bi, flags & DW_DRAW_NOAA ? NO : YES);

    if(context)
        UIGraphicsPushContext(context);

    if(bi)
    {
        UIBezierPath *bp = [UIBezierPath bezierPathWithRect:CGRectMake(x, y, width, height)];;
        
        [_dw_fg_color set];

        if(flags & DW_DRAW_FILL)
            [bp fill];
        else
            [bp stroke];
    }
    if(context)
        UIGraphicsPopContext();
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_draw_arc, void, HWND handle, HPIXMAP pixmap, int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2)
DW_FUNCTION_ADD_PARAM9(handle, pixmap, flags, xorigin, yorigin, x1, y1, x2, y2)
DW_FUNCTION_NO_RETURN(dw_draw_arc)
DW_FUNCTION_RESTORE_PARAM9(handle, HWND, pixmap, HPIXMAP, flags, int, xorigin, int, yorigin, int, x1, int, y1, int, x2, int, y2, int)
{
    DW_FUNCTION_INIT;
    DWImage *bi = _dw_dest_image(pixmap, handle);
    CGContextRef context = _dw_draw_context(bi, flags & DW_DRAW_NOAA ? NO : YES);

    if(context)
        UIGraphicsPushContext(context);

    if(bi)
    {
        UIBezierPath* aPath;

        [_dw_fg_color set];

        /* Special case of a full circle/oval */
        if(flags & DW_DRAW_FULL)
            aPath = [UIBezierPath bezierPathWithOvalInRect:CGRectMake(x1, y1, x2 - x1, y2 - y1)];
        else
        {
            double a1 = atan2((y1-yorigin), (x1-xorigin));
            double a2 = atan2((y2-yorigin), (x2-xorigin));
            double dx = xorigin - x1;
            double dy = yorigin - y1;
            double r = sqrt(dx*dx + dy*dy);

            /* Prepare to draw */
            aPath = [UIBezierPath bezierPathWithArcCenter:CGPointMake(xorigin, yorigin)
                                                   radius:r startAngle:a1 endAngle:a2 clockwise:YES];
        }
        /* If the fill flag is passed */
        if(flags & DW_DRAW_FILL)
            [aPath fill];
        else
            [aPath stroke];
    }
    if(context)
        UIGraphicsPopContext();
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       A handle to a tree window or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_tree_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_tree_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    DW_FUNCTION_INIT;
    DWTree *tree = [[[DWTree alloc] init] retain];
    [tree setTag:cid];
    DW_FUNCTION_RETURN_THIS(tree);
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
 * Returns:
 *       A handle to a tree item or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_tree_insert_after, HTREEITEM, HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
DW_FUNCTION_ADD_PARAM6(handle, item, title, icon, parent, itemdata)
DW_FUNCTION_RETURN(dw_tree_insert_after, HTREEITEM)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, item, HTREEITEM, title, char *, icon, HICN, parent, HTREEITEM, itemdata, void *)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    DWTreeItem *treeparent = item ? item : parent;
    DWTreeItem *treeitem = [[[DWTreeItem alloc] initWithIcon:icon
                                                       Title:[NSString stringWithUTF8String:(title ? title : "")]
                                                     andData:itemdata] retain];
    if(treeparent)
    {
        if(item)
            [treeparent insertChildAfter:treeitem];
        else
            [treeparent appendChild:treeitem];
    }
    else
        [tree insertTreeItem:treeitem];
    DW_FUNCTION_RETURN_THIS(treeitem);
}

/*
 * Inserts an item into a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 * Returns:
 *       A handle to a tree item or NULL on failure.
 */
HTREEITEM API dw_tree_insert(HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    return dw_tree_insert_after(handle, NULL, title, icon, parent, itemdata);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A malloc()ed buffer of item text to be dw_free()ed or NULL on error.
 */
DW_FUNCTION_DEFINITION(dw_tree_get_title, char *, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_get_title, char *)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(handle), HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    char *title = NULL;
    DWTreeItem *treeitem = item;
    if(treeitem)
        title = strdup([treeitem.title UTF8String]);
    DW_FUNCTION_RETURN_THIS(title);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A handle to a tree item or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_tree_get_parent, HTREEITEM, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_get_parent, HTREEITEM)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(handle), HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    DWTreeItem *treeparent = nil;
    DWTreeItem *treeitem = item;
    if(treeitem)
        treeparent = treeitem.parent;
    if(treeparent && [treeparent isRoot])
        treeparent = NULL;
    DW_FUNCTION_RETURN_THIS(treeparent);
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_change, void, HWND handle, HTREEITEM item, const char *title, HICN icon)
DW_FUNCTION_ADD_PARAM4(handle, item, title, icon)
DW_FUNCTION_NO_RETURN(dw_tree_item_change)
DW_FUNCTION_RESTORE_PARAM4(DW_UNUSED(handle), HWND, item, HTREEITEM, title, const char *, icon, HICN)
{
    DW_FUNCTION_INIT;
    DWTreeItem *treeitem = item;
    if(handle && treeitem)
    {
        [treeitem setIcon:icon];
        [treeitem setTitle:[NSString stringWithUTF8String:(title ? title : "")]];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_set_data, void, HWND handle, HTREEITEM item, void *itemdata)
DW_FUNCTION_ADD_PARAM3(handle, item, itemdata)
DW_FUNCTION_NO_RETURN(dw_tree_item_set_data)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(handle), HWND, item, HTREEITEM, itemdata, void *)
{
    DW_FUNCTION_INIT;
    DWTreeItem *treeitem = item;
    if(handle && treeitem)
        [treeitem setData:itemdata];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A pointer to tree item data or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_get_data, void *, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_item_get_data, void *)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(handle), HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    void *result = NULL;
    DWTreeItem *treeitem = item;
    if(handle && treeitem)
        result = [treeitem data];
    DW_FUNCTION_RETURN_THIS(result);
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_select, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_select)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    DWTreeItem *treeitem = item;

    if(tree && treeitem)
    {
        NSInteger itemIndex = [tree treeView:tree rowForTreeItem:treeitem];
        if(itemIndex > -1)
        {
            NSIndexPath *ip = [NSIndexPath indexPathForRow:(NSUInteger)itemIndex inSection:0];

            [tree selectRowAtIndexPath:ip
                              animated:NO
                        scrollPosition:UITableViewScrollPositionNone];
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_tree_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_tree_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    [tree clear];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_expand, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_expand)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    DWTreeItem *treeitem = item;
    if(![treeitem expanded])
    {
        [treeitem setExpanded:YES];
        [tree reloadData];
        [tree resetSelection:NO];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_collapse, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_collapse)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    DWTreeItem *treeitem = item;
    if([treeitem expanded])
    {
        [treeitem setExpanded:NO];
        [tree reloadData];
        [tree resetSelection:NO];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_delete, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    DWTreeItem *treeitem = item;
    [treeitem removeFromParent];
    [tree reloadData];
    [tree resetSelection:NO];
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
DW_FUNCTION_DEFINITION(dw_container_new, HWND, ULONG cid, int multi)
DW_FUNCTION_ADD_PARAM2(cid, multi)
DW_FUNCTION_RETURN(dw_container_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, multi, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = _dw_cont_new(cid, multi);
    DW_FUNCTION_RETURN_THIS(cont);
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
DW_FUNCTION_DEFINITION(dw_container_setup, int, HWND handle, unsigned long *flags, char **titles, int count, int separator)
DW_FUNCTION_ADD_PARAM5(handle, flags, titles, count, separator)
DW_FUNCTION_RETURN(dw_container_setup, int)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, flags, unsigned long *, titles, char **, count, int, DW_UNUSED(separator), int)
{
    DW_FUNCTION_INIT;
    int z, retval = DW_ERROR_NONE;
    DWContainer *cont = handle;

    [cont setup];

    for(z=0;z<count;z++)
    {
        /* Even though we don't have columns on iOS, we save the data...
         * So we can simulate columns when displaying the data in the list.
         */
        NSString *title = [NSString stringWithUTF8String:titles[z]];

        [cont addColumn:title andType:(int)flags[z]];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Configures the main filesystem columnn title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, const char *title)
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
DW_FUNCTION_DEFINITION(dw_container_alloc, void *, HWND handle, int rowcount)
DW_FUNCTION_ADD_PARAM2(handle, rowcount)
DW_FUNCTION_RETURN(dw_container_alloc, void *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, rowcount, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont addRows:rowcount];
    DW_FUNCTION_RETURN_THIS(cont);
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
DW_FUNCTION_DEFINITION(dw_container_set_item, void, HWND handle, void *pointer, int column, int row, void *data)
DW_FUNCTION_ADD_PARAM5(handle, pointer, column, row, data)
DW_FUNCTION_NO_RETURN(dw_container_set_item)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pointer, void *, column, int, row, int, data, void *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    id icon = nil, text = nil;
    int type = [cont cellType:column];
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
        lastadd = [cont lastAddPoint];

    if(data)
    {
        if(type & DW_CFA_BITMAPORICON)
        {
            icon = *((UIImage **)data);
        }
        else if(type & DW_CFA_STRING)
        {
            char *str = *((char **)data);
            text = [NSString stringWithUTF8String:str];
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

                memset(&curtm, 0, sizeof(curtm));
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
                text = [NSString stringWithUTF8String:textbuffer];
        }
    }

    id object = [cont getRow:(row+lastadd)];
    NSUInteger capacity = [[cont getColumns] count];

    /* If it is a cell, change the content of the cell */
    if([object isMemberOfClass:[DWTableViewCell class]])
    {
        DWTableViewCell *cell = object;

        if(column == 0)
        {
            if(icon)
                [[cell image] setImage:icon];
            else
                [[cell label] setText:text];
        }
        else if(icon || text)
        {
            NSMutableArray *cd = [cell columnData];

            if(column >= capacity)
                capacity = column + 1;

            if(!cd)
                cd = [NSMutableArray arrayWithCapacity:capacity];
            if(cd)
            {
                while([cd count] <= column)
                    [cd addObject:[NSNull null]];
                [cd replaceObjectAtIndex:column withObject:(text ? text : icon)];
            }
            [cell setColumnData:cd];
        }
    }
    else
    {
        NSMutableArray *cd  = [NSMutableArray arrayWithCapacity:(capacity > column ? capacity : column + 1)];
        if(cd)
            [cd replaceObjectAtIndex:column withObject:(text ? text : icon)];
        /* Otherwise replace it with a new cell */
        [cont editCell:_dw_table_cell_view_new(icon, text, cd) at:(row+lastadd)];
    }
    [cont setNeedsDisplay];
    DW_FUNCTION_RETURN_NOTHING;
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
void API dw_filesystem_change_file(HWND handle, int row, const char *filename, HICN icon)
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
DW_FUNCTION_DEFINITION(dw_filesystem_set_file, void, HWND handle, void *pointer, int row, const char *filename, HICN icon)
DW_FUNCTION_ADD_PARAM5(handle, pointer, row, filename, icon)
DW_FUNCTION_NO_RETURN(dw_filesystem_set_file)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pointer, void *, row, int, filename, char *, icon, HICN)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    NSString *text = filename ? [NSString stringWithUTF8String:filename] : nil;
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
        lastadd = [cont lastAddPoint];

    id object = [cont getRow:(row+lastadd)];
    
    /* If it is a cell, change the content of the cell */
    if([object isMemberOfClass:[DWTableViewCell class]])
    {
        DWTableViewCell *cell = object;

        if(icon)
            [[cell image] setImage:icon];
        if(text)
            [[cell label] setText:text];
    }
    else /* Otherwise replace it with a new cell */
        [cont editCell:_dw_table_cell_view_new(icon, text, nil) at:(row+lastadd)];
    [cont setNeedsDisplay];
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_container_get_column_type, int, HWND handle, int column)
DW_FUNCTION_ADD_PARAM2(handle, column)
DW_FUNCTION_RETURN(dw_container_get_column_type, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, column, int)
{
    DW_FUNCTION_INIT;
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
    DW_FUNCTION_RETURN_THIS(rc);
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
DW_FUNCTION_DEFINITION(dw_container_set_stripe, void, HWND handle, unsigned long oddcolor, unsigned long evencolor)
DW_FUNCTION_ADD_PARAM3(handle, oddcolor, evencolor)
DW_FUNCTION_NO_RETURN(dw_container_set_stripe)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, oddcolor, unsigned long, evencolor, unsigned long)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRowBgOdd:oddcolor
              andEven:evencolor];
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_container_set_row_title, void, void *pointer, int row, const char *title)
DW_FUNCTION_ADD_PARAM3(pointer, row, title)
DW_FUNCTION_NO_RETURN(dw_container_set_row_title)
DW_FUNCTION_RESTORE_PARAM3(pointer, void *, row, int, title, char *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRow:(row+lastadd) title:title];
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Sets the data pointer of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
DW_FUNCTION_DEFINITION(dw_container_set_row_data, void, void *pointer, int row, void *data)
DW_FUNCTION_ADD_PARAM3(pointer, row, data)
DW_FUNCTION_NO_RETURN(dw_container_set_row_data)
DW_FUNCTION_RESTORE_PARAM3(pointer, void *, row, int, data, void *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRowData:(row+lastadd) title:data];
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
DW_FUNCTION_DEFINITION(dw_container_change_row_title, void, HWND handle, int row, const char *title)
DW_FUNCTION_ADD_PARAM3(handle, row, title)
DW_FUNCTION_NO_RETURN(dw_container_change_row_title)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, row, int, title, char *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRow:row title:title];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the data pointer of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
DW_FUNCTION_DEFINITION(dw_container_change_row_data, void, HWND handle, int row, void *data)
DW_FUNCTION_ADD_PARAM3(handle, row, data)
DW_FUNCTION_NO_RETURN(dw_container_change_row_data)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, row, int, data, void *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRowData:row title:data];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          rowcount: The number of rows to be inserted.
 */
DW_FUNCTION_DEFINITION(dw_container_insert, void, HWND handle, void *pointer, int rowcount)
DW_FUNCTION_ADD_PARAM3(handle, pointer, rowcount)
DW_FUNCTION_NO_RETURN(dw_container_insert)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, DW_UNUSED(pointer), void *, DW_UNUSED(rowcount), int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont reloadData];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
DW_FUNCTION_DEFINITION(dw_container_clear, void, HWND handle, int redraw)
DW_FUNCTION_ADD_PARAM2(handle, redraw)
DW_FUNCTION_NO_RETURN(dw_container_clear)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, redraw, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont clear];
    if(redraw)
        [cont reloadData];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_container_delete, void, HWND handle, int rowcount)
DW_FUNCTION_ADD_PARAM2(handle, rowcount)
DW_FUNCTION_NO_RETURN(dw_container_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, rowcount, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int x;

    for(x=0;x<rowcount;x++)
    {
        [cont removeRow:0];
    }
    [cont reloadData];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Scrolls container up or down.
 * Parameters:
 *       handle: Handle to the window (widget) to be scrolled.
 *       direction: DW_SCROLL_UP, DW_SCROLL_DOWN, DW_SCROLL_TOP or
 *                  DW_SCROLL_BOTTOM. (rows is ignored for last two)
 *       rows: The number of rows to be scrolled.
 */
DW_FUNCTION_DEFINITION(dw_container_scroll, void, HWND handle, int direction, DW_UNUSED(long rows))
DW_FUNCTION_ADD_PARAM3(handle, direction, rows)
DW_FUNCTION_NO_RETURN(dw_container_scroll)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, direction, int, DW_UNUSED(rows), long)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int rowcount = (int)[cont numberOfRowsInSection:0];
    CGPoint offset = [cont contentOffset];

    /* Safety check */
    if(rowcount < 1)
    {
        return;
    }

    switch(direction)
    {
        case DW_SCROLL_TOP:
        {
            offset.y = 0;
            break;
        }
        case DW_SCROLL_BOTTOM:
        {
            offset.y = [cont contentSize].height - [cont visibleSize].height;
            break;
        }
        /* TODO: Currently scrolling a full page, need to use row parameter instead */
        case DW_SCROLL_UP:
        {
            offset.y -= [cont visibleSize].height;
            break;
        }
        case DW_SCROLL_DOWN:
        {
            offset.y += [cont visibleSize].height;
            break;
        }
    }
    if(offset.y < 0)
        offset.y = 0;
    [cont setContentOffset:offset];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
DW_FUNCTION_DEFINITION(dw_container_query_start, char *, HWND handle, unsigned long flags)
DW_FUNCTION_ADD_PARAM2(handle, flags)
DW_FUNCTION_RETURN(dw_container_query_start, char *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, flags, unsigned long)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    NSArray *selected = [cont indexPathsForSelectedRows];
    NSIndexPath *result = [selected firstObject];
    void *retval = NULL;

    if(result)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result.row];
        else
        {
            char *temp = [cont getRowTitle:(int)result.row];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:1];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Continues an existing query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
DW_FUNCTION_DEFINITION(dw_container_query_next, char *, HWND handle, unsigned long flags)
DW_FUNCTION_ADD_PARAM2(handle, flags)
DW_FUNCTION_RETURN(dw_container_query_next, char *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, flags, unsigned long)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int lastQueryPoint = [cont lastQueryPoint];
    NSArray *selected = [cont indexPathsForSelectedRows];
    NSIndexPath *result = lastQueryPoint < [selected count] ?  [selected objectAtIndex:lastQueryPoint] : nil;
    void *retval = NULL;

    if(result)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result.row];
        else
        {
            char *temp = [cont getRowTitle:(int)result.row];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:(int)lastQueryPoint+1];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_cursor, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_container_cursor)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInSection:0];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            NSIndexPath *ip = [NSIndexPath indexPathForRow:(NSUInteger)x inSection:0];

            [cont selectRowAtIndexPath:ip
                              animated:NO
                        scrollPosition:UITableViewScrollPositionNone];
            x=count;
            break;
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data: Data associated with the row.
 */
DW_FUNCTION_DEFINITION(dw_container_cursor_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_cursor_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInSection:0];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            NSIndexPath *ip = [NSIndexPath indexPathForRow:(NSUInteger)x inSection:0];

            [cont selectRowAtIndexPath:ip
                              animated:NO
                        scrollPosition:UITableViewScrollPositionNone];
            x=count;
            break;
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_delete_row, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_container_delete_row)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInSection:0];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            [cont removeRow:x];
            [cont reloadData];
            x=count;
            break;
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data: Data specified.
 */
DW_FUNCTION_DEFINITION(dw_container_delete_row_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_delete_row_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInSection:0];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            [cont removeRow:x];
            [cont reloadData];
            x=count;
            break;
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
    /* TODO: Not sure if we need to implement this on iOS */
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void API dw_taskbar_insert(HWND handle, HICN icon, const char *bubbletext)
{
    /* Taskbar unsupported on iOS */
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
    /* Taskbar unsupported on iOS */
}

/* Internal function to keep HICNs from getting too big */
UIImage *_dw_icon_resize(UIImage *image)
{
    if(image)
    {
        CGSize size = [image size];
        if(size.width > 24 || size.height > 24)
        {
            if(size.width > 24)
                size.width = 24;
            if(size.height > 24)
                size.height = 24;
            // Pass 0.0 to use the current device's pixel scaling factor (and thus account for Retina resolution).
            // Pass 1.0 to force exact pixel size.
            UIGraphicsBeginImageContextWithOptions(size, NO, 0.0);
            [image drawInRect:CGRectMake(0, 0, size.width, size.height)];
            UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();
            return newImage;
        }
    }
    return image;
}

/* Internal version that does not resize the image */
HICN _dw_icon_load(unsigned long resid)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    UIImage *image = [[UIImage alloc] initWithContentsOfFile:filepath];
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
    UIImage *image = _dw_icon_load(resid);
    return _dw_icon_resize(image);
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_file(const char *filename)
{
    char *ext = _dw_get_image_extension( filename );

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    UIImage *image = [[UIImage alloc] initWithContentsOfFile:nstr];
    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[UIImage alloc] initWithContentsOfFile:nstr];
    }
    return _dw_icon_resize(image);
}

/*
 * Obtains an icon from data
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    UIImage *image = [[UIImage alloc] initWithData:thisdata];
    return _dw_icon_resize(image);
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
    UIImage *image = handle;
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
    DWMDI *mdi = [[[DWMDI alloc] init] retain];
    [mdi setTag:cid];
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
DW_FUNCTION_DEFINITION(dw_splitbar_new, HWND, int type, HWND topleft, HWND bottomright, unsigned long cid)
DW_FUNCTION_ADD_PARAM4(type, topleft, bottomright, cid)
DW_FUNCTION_RETURN(dw_splitbar_new, HWND)
DW_FUNCTION_RESTORE_PARAM4(type, int, topleft, HWND, bottomright, HWND, cid, unsigned long)
{
    DW_FUNCTION_INIT;
    id tmpbox = dw_box_new(DW_VERT, 0);
    DWSplitBar *split = [[DWSplitBar alloc] init];
    dw_box_pack_start(tmpbox, topleft, 0, 0, TRUE, TRUE, 0);
    [split setTopLeft:tmpbox];
    tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, bottomright, 0, 0, TRUE, TRUE, 0);
    [split setBottomRight:tmpbox];
    [split setPercent:50.0];
    [split setType:type];
    [split setTag:cid];
    DW_FUNCTION_RETURN_THIS(split);
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 *       percent: The position of the splitbar.
 */
DW_FUNCTION_DEFINITION(dw_splitbar_set, void, HWND handle, float percent)
DW_FUNCTION_ADD_PARAM2(handle, percent)
DW_FUNCTION_NO_RETURN(dw_splitbar_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, percent, float)
{
    DW_FUNCTION_INIT;
    DWSplitBar *split = handle;
    [split setPercent:percent];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
    DWSplitBar *split = handle;
    float retval = 50.0;

    if(split)
        retval = [split percent];
    return retval;
}

/* Internal function to convert fontname to UIFont */
UIFont *_dw_font_by_name(const char *fontname)
{
    UIFont *font = DWDefaultFont;
    
    if(fontname)
    {
        char *name = strchr(fontname, '.');

        if(name && (name++))
        {
            UIFontDescriptorSymbolicTraits traits = 0;
            UIFontDescriptor* fd;
            int size = atoi(fontname);
            char *Italic = strstr(name, " Italic");
            char *Bold = strstr(name, " Bold");
            size_t len = (Italic ? (Bold ? (Italic > Bold ? (Bold - name) : (Italic - name)) : (Italic - name)) : (Bold ? (Bold - name) : strlen(name)));
            char *newname = alloca(len+1);

            memset(newname, 0, len+1);
            strncpy(newname, name, len);

            if(Bold)
                traits |= UIFontDescriptorTraitBold;
            if(Italic)
                traits |= UIFontDescriptorTraitItalic;

            fd = [UIFontDescriptor
                    fontDescriptorWithFontAttributes:@{UIFontDescriptorFamilyAttribute:[NSString stringWithUTF8String:newname],
                    UIFontDescriptorTraitsAttribute: @{UIFontSymbolicTrait:[NSNumber numberWithInteger:traits]}}];
            NSArray* matches = [fd matchingFontDescriptorsWithMandatoryKeys:
                                  [NSSet setWithObjects:UIFontDescriptorFamilyAttribute, UIFontDescriptorTraitsAttribute, nil]];
            if(matches.count != 0)
                font = [UIFont fontWithDescriptor:matches[0] size:size];
        }
    }
    return font;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_bitmap_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_bitmap_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    DW_FUNCTION_INIT;
    UIImageView *bitmap = [[[UIImageView alloc] init] retain];
    [bitmap setTag:cid];
    DW_FUNCTION_RETURN_THIS(bitmap);
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
    HPIXMAP pixmap = NULL;

    if((pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        pixmap->width = width;
        pixmap->height = height;
        pixmap->handle = handle;
        pixmap->image = [[[DWImage alloc] initWithSize:CGSizeMake(width,height)] retain];
    }
    return pixmap;
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
HPIXMAP API dw_pixmap_new_from_file(HWND handle, const char *filename)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;
    char *ext = _dw_get_image_extension(filename);

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    UIImage *tmpimage = [[UIImage alloc] initWithContentsOfFile:nstr];
    if(!tmpimage && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        tmpimage = [[UIImage alloc] initWithContentsOfFile:nstr];
    }
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    pixmap->width = [tmpimage size].width;
    pixmap->height = [tmpimage size].height;
    pixmap->image = [[[DWImage alloc] initWithUIImage:tmpimage] retain];
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
HPIXMAP API dw_pixmap_new_from_data(HWND handle, const char *data, int len)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    UIImage *tmpimage = [[UIImage alloc] initWithData:thisdata];
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    pixmap->width = [tmpimage size].width;
    pixmap->height = [tmpimage size].height;
    pixmap->image = [[[DWImage alloc] initWithUIImage:tmpimage] retain];
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
 * Note: This does nothing on iOS as transparency
 *       is handled automatically
 */
void API dw_pixmap_set_transparent_color(HPIXMAP pixmap, ULONG color)
{
    /* Don't do anything, all images support transparency on iOS */
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
    UIImage *tmpimage = [[UIImage alloc] initWithContentsOfFile:filepath];

    if(tmpimage)
    {
        pixmap->width = [tmpimage size].width;
        pixmap->height = [tmpimage size].height;
        pixmap->image = [[DWImage alloc] initWithUIImage:tmpimage];
        pixmap->handle = handle;
        DW_LOCAL_POOL_OUT;
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
int API dw_pixmap_set_font(HPIXMAP pixmap, const char *fontname)
{
    if(pixmap)
    {
        UIFont *font = _dw_font_by_name(fontname);

        if(font)
        {
            DW_LOCAL_POOL_IN;
            UIFont *oldfont = pixmap->font;
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
        DWImage *image = (DWImage *)pixmap->image;
        UIFont *font = pixmap->font;
        DW_LOCAL_POOL_IN;
        [image release];
        [font release];
        free(pixmap);
        DW_LOCAL_POOL_OUT;
    }
}

/*
 * Returns the width of the pixmap, same as the DW_PIXMAP_WIDTH() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_width(HPIXMAP pixmap)
{
    return pixmap ? pixmap->width : 0;
}

/*
 * Returns the height of the pixmap, same as the DW_PIXMAP_HEIGHT() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_height(HPIXMAP pixmap)
{
    return pixmap ? pixmap->height : 0;
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
    bltinfo->dest = _dw_dest_image(destp, dest);
    bltinfo->src = src;
    bltinfo->xdest = xdest;
    bltinfo->ydest = ydest;
    bltinfo->width = width;
    bltinfo->height = height;
    bltinfo->xsrc = xsrc;
    bltinfo->ysrc = ysrc;
    bltinfo->srcwidth = srcwidth;
    bltinfo->srcheight = srcheight;

    if(srcp)
    {
        id object = bltinfo->src = (id)srcp->image;
        [object retain];
    }
    [DWObj safeCall:@selector(doBitBlt:) withObject:bi];
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
DW_FUNCTION_DEFINITION(dw_calendar_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_calendar_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
    DWCalendar *calendar = [[[DWCalendar alloc] init] retain];
    if (@available(iOS 14.0, *)) {
        [calendar setPreferredDatePickerStyle:UIDatePickerStyleInline];
    } else {
        // We really want the iOS 14 style to match the other platforms...
        // but just leave it the default spinner style on 13 and earlier.
    }
    [calendar setDatePickerMode:UIDatePickerModeDate];
    [calendar setDate:[NSDate date]];
    [calendar setTag:cid];
    DW_FUNCTION_RETURN_THIS(calendar);
}

/*
 * Sets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
DW_FUNCTION_DEFINITION(dw_calendar_set_date, void, HWND handle, unsigned int year, unsigned int month, unsigned int day)
DW_FUNCTION_ADD_PARAM4(handle, year, month, day)
DW_FUNCTION_NO_RETURN(dw_calendar_set_date)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, year, unsigned int, month, unsigned int, day, unsigned int)
{
    DWCalendar *calendar = handle;
    NSDate *date;
    char buffer[101] = {0};

    snprintf(buffer, 100, "%04d-%02d-%02d", year, month, day);

    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    dateFormatter.dateFormat = @"yyyy-MM-dd";

    date = [dateFormatter dateFromString:[NSString stringWithUTF8String:buffer]];
    [calendar setDate:date];
    [date release];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 */
DW_FUNCTION_DEFINITION(dw_calendar_get_date, void, HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
DW_FUNCTION_ADD_PARAM4(handle, year, month, day)
DW_FUNCTION_NO_RETURN(dw_calendar_get_date)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, year, unsigned int *, month, unsigned int *, day, unsigned int *)
{
    DWCalendar *calendar = handle;
    NSCalendar *mycalendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
    NSDate *date = [calendar date];
    NSDateComponents* components = [mycalendar components:NSCalendarUnitDay|NSCalendarUnitMonth|NSCalendarUnitYear fromDate:date];
    *day = (unsigned int)[components day];
    *month = (unsigned int)[components month];
    *year = (unsigned int)[components year];
    [mycalendar release];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
DW_FUNCTION_DEFINITION(dw_html_action, void, HWND handle, int action)
DW_FUNCTION_ADD_PARAM2(handle, action)
DW_FUNCTION_NO_RETURN(dw_html_action)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, action, int)
{
    DWWebView *html = handle;
    switch(action)
    {
        case DW_HTML_GOBACK:
            [html goBack];
            break;
        case DW_HTML_GOFORWARD:
            [html goForward];
            break;
        case DW_HTML_GOHOME:
            dw_html_url(handle, DW_HOME_URL);
            break;
        case DW_HTML_SEARCH:
            break;
        case DW_HTML_RELOAD:
            [html reload];
            break;
        case DW_HTML_STOP:
            [html stopLoading];
            break;
        case DW_HTML_PRINT:
            break;
    }
    DW_FUNCTION_RETURN_NOTHING;
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
DW_FUNCTION_DEFINITION(dw_html_raw, int, HWND handle, const char *string)
DW_FUNCTION_ADD_PARAM2(handle, string)
DW_FUNCTION_RETURN(dw_html_raw, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, string, const char *)
{
    DWWebView *html = handle;
    int retval = DW_ERROR_NONE;
    [html loadHTMLString:[ NSString stringWithUTF8String:string ] baseURL:nil];
    DW_FUNCTION_RETURN_THIS(retval);
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
DW_FUNCTION_DEFINITION(dw_html_url, int, HWND handle, const char *url)
DW_FUNCTION_ADD_PARAM2(handle, url)
DW_FUNCTION_RETURN(dw_html_url, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, url, const char *)
{
    DWWebView *html = handle;
    int retval = DW_ERROR_NONE;
    [html loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[ NSString stringWithUTF8String:url ]]]];
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Executes the javascript contained in "script" in the HTML window.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       script: Javascript code to execute.
 *       scriptdata: Data passed to the signal handler.
 * Notes: A DW_SIGNAL_HTML_RESULT event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
DW_FUNCTION_DEFINITION(dw_html_javascript_run, int, HWND handle, const char *script, void *scriptdata)
DW_FUNCTION_ADD_PARAM3(handle, script, scriptdata)
DW_FUNCTION_RETURN(dw_html_javascript_run, int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, script, const char *, scriptdata, void *)
{
    DWWebView *html = handle;
    int retval = DW_ERROR_NONE;
    DW_LOCAL_POOL_IN;

    [html evaluateJavaScript:[NSString stringWithUTF8String:script] completionHandler:^(NSString *result, NSError *error)
    {
        void *params[2] = { result, scriptdata };
        _dw_event_handler(html, (id)params, _DW_EVENT_HTML_RESULT);
    }];
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Install a javascript function with name that can call native code.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       name: Javascript function name.
 * Notes: A DW_SIGNAL_HTML_MESSAGE event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
DW_FUNCTION_DEFINITION(dw_html_javascript_add, int, HWND handle, const char *name)
DW_FUNCTION_ADD_PARAM2(handle, name)
DW_FUNCTION_RETURN(dw_html_javascript_add, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, name, const char *)
{
    DWWebView *html = handle;
    WKUserContentController *controller = [[html configuration] userContentController];
    int retval = DW_ERROR_UNKNOWN;

    if(controller && name)
    {
        DW_LOCAL_POOL_IN;
        /* Script to inject that will call the handler we are adding */
        NSString *script = [NSString stringWithFormat:@"function %s(body) {window.webkit.messageHandlers.%s.postMessage(body);}",
                            name, name];
        [controller addScriptMessageHandler:handle name:[NSString stringWithUTF8String:name]];
        [controller addUserScript:[[WKUserScript alloc] initWithSource:script
                injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:NO]];
        DW_LOCAL_POOL_OUT;
        retval = DW_ERROR_NONE;
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Create a new HTML window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_html_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_html_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(DW_UNUSED(cid), ULONG)
{
    DW_FUNCTION_INIT;
    DWWebView *web = [[[DWWebView alloc] init] retain];
    web.navigationDelegate = web;
    [web setTag:cid];
    DW_FUNCTION_RETURN_THIS(web);
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
    if(x)
    {
        *x = (long)_dw_event_point.x;
    }
    if(y)
    {
        *y = (long)_dw_event_point.y;
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
    DWMenu *menu = [[[DWMenu alloc] initWithTag:cid] retain];
    return menu;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
    DWWindow *window = location;
    DWMenu *menu = [[[DWMenu alloc] initWithTag:0] retain];

    [window setMenu:menu];
    return menu;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
    if(menu)
    {
        DWMenu *thismenu = *menu;
        DW_LOCAL_POOL_IN;
        [thismenu release];
        DW_LOCAL_POOL_OUT;
        *menu = NULL;
    }
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
    if(menu)
    {
        DWMenu *thismenu = (DWMenu *)*menu;
        id object = parent;
        DWWindow *window = [object isMemberOfClass:[DWWindow class]] ? object : (DWWindow *)[object window];
        [window setPopupMenu:thismenu];
        *menu = nil;
    }
}

char _dw_removetilde(char *dest, const char *src)
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
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG itemid, ULONG flags, int end, int check, HMENUI submenux)
{
    DWMenu *menu = menux;
    __block DWMenuItem *item = nil;
    if(!title || strlen(title) == 0)
        [menu addItem:[[NSNull alloc] init]];
    else
    {
        char accel[2];
        char *newtitle = malloc(strlen(title)+1);
        NSString *nstr;
        
        accel[0] = _dw_removetilde(newtitle, title);
        accel[1] = 0;

        nstr = [NSString stringWithUTF8String:newtitle];
        free(newtitle);

        item = [DWMenuItem actionWithTitle:nstr image:nil identifier:nil
                                    handler:^(__kindof UIAction * _Nonnull action) {
            if(check)
            {
                UIMenuElementState state = [item state] == UIMenuElementStateOn ? UIMenuElementStateOff : UIMenuElementStateOn;
                [item setState:state];
            }
            [DWObj menuHandler:item];
            if(check)
                [DWObj safeCall:@selector(updateMenu:) withObject:nil];
        }];
        /* Don't set the tag if the ID is 0 or -1 */
        if(itemid != DW_MENU_AUTO && itemid != DW_MENU_POPUP)
            [item setTag:itemid];
        [menu addItem:item];
        
        if(check)
        {
            [item setCheck:YES];
            if(flags & DW_MIS_CHECKED)
                [item setState:UIMenuElementStateOn];
        }
        [item setEnabled:(flags & DW_MIS_DISABLED ? NO : YES)];

        if(submenux)
        {
            DWMenu *submenu = submenux;

            [submenu setTitle:nstr];
            [item setSubmenu:submenu];
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
    DWMenuItem *menuitem = [menu childWithTag:itemid];

    if(menuitem != nil)
    {
        UIMenuElementState newstate = check ? UIMenuElementStateOn : UIMenuElementStateOff;

        /* Only force the menus to repopulate if something changed */
        if(newstate != [menuitem state])
        {
            [menuitem setState:newstate];
            [DWObj safeCall:@selector(updateMenu:) withObject:nil];
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
    DWMenuItem *menuitem = [menu childWithTag:itemid];

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
    DWMenuItem *menuitem = [menu childWithTag:itemid];

    if(menuitem != nil)
    {
        UIMenuElementState oldstate = [menuitem state];
        BOOL oldenabled = [menuitem enabled];

        if(state & DW_MIS_CHECKED)
            [menuitem setState:UIMenuElementStateOn];
        else if(state & DW_MIS_UNCHECKED)
            [menuitem setState:UIMenuElementStateOff];
        if(state & DW_MIS_ENABLED)
            [menuitem setEnabled:YES];
        else if(state & DW_MIS_DISABLED)
            [menuitem setEnabled:NO];

        /* Only force the menus to repopulate if something changed */
        if(oldstate != [menuitem state] || oldenabled != [menuitem enabled])
            [DWObj safeCall:@selector(updateMenu:) withObject:nil];
    }
}

/* Gets the notebook page from associated ID */
DWNotebookPage *_dw_notepage_from_id(DWNotebook *notebook, unsigned long pageid)
{
    NSArray *pages = [notebook views];
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
DW_FUNCTION_DEFINITION(dw_notebook_new, HWND, ULONG cid, DW_UNUSED(int top))
DW_FUNCTION_ADD_PARAM2(cid, top)
DW_FUNCTION_RETURN(dw_notebook_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, DW_UNUSED(top), int)
{
    DWNotebook *notebook = [[[DWNotebook alloc] init] retain];
    [[notebook tabs] addTarget:notebook
                        action:@selector(pageChanged:)
              forControlEvents:UIControlEventValueChanged];
    [notebook setTag:cid];
    DW_FUNCTION_RETURN_THIS(notebook);
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_new, ULONG, HWND handle, DW_UNUSED(ULONG flags), int front)
DW_FUNCTION_ADD_PARAM3(handle, flags, front)
DW_FUNCTION_RETURN(dw_notebook_page_new, ULONG)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, DW_UNUSED(flags), ULONG, front, int)
{
    DWNotebook *notebook = handle;
    NSInteger page = [notebook pageid];
    DWNotebookPage *notepage = [[DWNotebookPage alloc] init];
    NSMutableArray<DWNotebookPage *> *views = [notebook views];
    unsigned long retval;

    [notepage setPageid:(int)page];
    if(front)
    {
        [[notebook tabs] insertSegmentWithTitle:@"" atIndex:(NSInteger)0 animated:NO];
        [views addObject:notepage];
    }
    else
    {
        [[notebook tabs] insertSegmentWithTitle:@"" atIndex:[[notebook tabs] numberOfSegments] animated:NO];
        [views addObject:notepage];
    }
    [notebook addSubview:notepage];
    [notebook setPageid:(int)(page+1)];

    if([views count] != 1)
    {
        /* Otherwise hide the page */
        [notepage setHidden:YES];
    }

    retval = (unsigned long)page;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_destroy, void, HWND handle, unsigned long pageid)
DW_FUNCTION_ADD_PARAM2(handle, pageid)
DW_FUNCTION_NO_RETURN(dw_notebook_page_destroy)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, pageid, unsigned long)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);
    DW_LOCAL_POOL_IN;

    if(notepage != nil)
    {
        NSMutableArray<DWNotebookPage *> *views = [notebook views];
        NSUInteger index = [views indexOfObject:notepage];

        if(index != NSNotFound)
        {
            [[notebook tabs] removeSegmentAtIndex:index animated:NO];
            [notepage removeFromSuperview];
            [views removeObject:notepage];
            index--;
            if(index >= 0 && index < [views count])
               [[notebook tabs] setSelectedSegmentIndex:index];
            else if([views count] > 0)
               [[notebook tabs] setSelectedSegmentIndex:0];
            [notebook pageChanged:nil];
            [notepage release];
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_get, unsigned long, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_notebook_page_get, unsigned long)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DWNotebook *notebook = handle;
    NSInteger index = [[notebook tabs] selectedSegmentIndex];
    unsigned long retval = 0;
    
    if(index != -1)
    {
        NSMutableArray<DWNotebookPage *> *views = [notebook views];
        DWNotebookPage *notepage = [views objectAtIndex:index];

        retval = [notepage pageid];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_set, void, HWND handle, unsigned long pageid)
DW_FUNCTION_ADD_PARAM2(handle, pageid)
DW_FUNCTION_NO_RETURN(dw_notebook_page_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, pageid, unsigned long)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        NSMutableArray<DWNotebookPage *> *views = [notebook views];
        NSUInteger index = [views indexOfObject:notepage];

        if(index != -1)
        {
            [[notebook tabs] setSelectedSegmentIndex:index];
            [notebook pageChanged:nil];
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_set_text, void, HWND handle, ULONG pageid, const char *text)
DW_FUNCTION_ADD_PARAM3(handle, pageid, text)
DW_FUNCTION_NO_RETURN(dw_notebook_page_set_text)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, pageid, ULONG, text, const char *)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        NSMutableArray<DWNotebookPage *> *views = [notebook views];
        NSUInteger index = [views indexOfObject:notepage];

        if(index != -1)
            [[notebook tabs] setTitle:[NSString stringWithUTF8String:text] forSegmentAtIndex:index];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_status_text(HWND handle, ULONG pageid, const char *text)
{
    /* Not supported here... do nothing */
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
DW_FUNCTION_DEFINITION(dw_notebook_pack, void, HWND handle, ULONG pageid, HWND page)
DW_FUNCTION_ADD_PARAM3(handle, pageid, page)
DW_FUNCTION_NO_RETURN(dw_notebook_pack)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, pageid, ULONG, page, HWND)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        dw_box_pack_start(notepage, page, 0, 0, TRUE, TRUE, 0);
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
DW_FUNCTION_DEFINITION(dw_window_new, HWND, DW_UNUSED(HWND hwndOwner), const char *title, ULONG flStyle)
DW_FUNCTION_ADD_PARAM3(hwndOwner, title, flStyle)
DW_FUNCTION_RETURN(dw_window_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(hwndOwner), HWND, title, char *, flStyle, ULONG)
{
    DW_FUNCTION_INIT;
    CGRect screenrect = [[UIScreen mainScreen] bounds];
    DWWindow *window = [[DWWindow alloc] initWithFrame:screenrect];
    DWView *view = [[DWView alloc] init];
    UIUserInterfaceStyle style = [[DWObj hiddenWindow] overrideUserInterfaceStyle];

    /* Copy the overrideUserInterfaceStyle property from the hiddenWindow */
    if(style != UIUserInterfaceStyleUnspecified)
        [window setOverrideUserInterfaceStyle:style];
    [window setWindowLevel:UIWindowLevelNormal];
    [window setRootViewController:[[DWViewController alloc] init]];
    [window setBackgroundColor:[UIColor systemBackgroundColor]];
    [[[window rootViewController] view] addSubview:view];
    [_dw_toplevel_windows addObject:window];

    /* Handle style flags... There is no visible frame...
     * On iOS 13 and higher if a titlebar is requested create a navigation bar.
     */
    if(@available(iOS 13.0, *)) {
        NSString *nstitle = [NSString stringWithUTF8String:title];
        [window setLargeContentTitle:nstitle];
        if(flStyle & DW_FCF_TITLEBAR)
        {
            NSInteger sbheight = [[[window windowScene] statusBarManager] statusBarFrame].size.height;
            CGRect navrect = CGRectMake(0, sbheight, screenrect.size.width, 44);
            UINavigationBar* navbar = [[UINavigationBar alloc] initWithFrame:navrect];
            UINavigationItem* navItem = [[UINavigationItem alloc] initWithTitle:nstitle];

            /* Double check the intrinsic height... */
            CGSize navdefault = [navbar intrinsicContentSize];
            if(navdefault.height != navrect.size.height)
            {
                navrect.size.height = navdefault.height;
                navbar.frame = navrect;
            }

            /* We maintain a list of top-level windows...If there is more than one window,
             * and our window has the close button style, add a Back button that will close it.
             */
            if([_dw_toplevel_windows count] > 1 && flStyle & DW_FCF_CLOSEBUTTON)
            {
                UIBarButtonItem *back = [[UIBarButtonItem alloc] initWithTitle:@"Back"
                                                                         style:UIBarButtonItemStylePlain
                                                                        target:window
                                                                        action:@selector(closeWindow:)];

                navItem.leftBarButtonItem = back;
            }
            [navbar setItems:@[navItem]];
            [[[window rootViewController] view] addSubview:navbar];
        }
    }
    DW_FUNCTION_RETURN_THIS(window);
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
    /* TODO: Only might be possible on Catalyst */
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
DW_FUNCTION_DEFINITION(dw_window_show, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_show, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    NSObject *object = handle;
    int retval = DW_ERROR_NONE;

    if([ object isMemberOfClass:[DWWindow class]])
    {
        DWWindow *window = handle;
        CGRect rect = [window frame];

        /* If we haven't been sized by a call.. */
        if(rect.size.width <= 1 || rect.size.height <= 1)
        {
            /* Determine the contents size */
            dw_window_set_size(handle, 0, 0);
        }
        if(![window shown])
        {
            [window makeKeyAndVisible];
            [window setShown:YES];
        }
        else
            [window setHidden:NO];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
DW_FUNCTION_DEFINITION(dw_window_hide, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_hide, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    NSObject *object = handle;
    int retval = DW_ERROR_NONE;

    if([ object isKindOfClass:[ UIWindow class ] ])
    {
        UIWindow *window = handle;

        [window setHidden:YES];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in DW_RGB format or a default color index.
 *          back: Background color in DW_RGB format or a default color index.
 */
DW_FUNCTION_DEFINITION(dw_window_set_color, int, HWND handle, unsigned long fore, unsigned long back)
DW_FUNCTION_ADD_PARAM3(handle, fore, back)
DW_FUNCTION_RETURN(dw_window_set_color, int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, fore, unsigned long, back, unsigned long)
{
    id object = handle;
    unsigned long _fore = _dw_get_color(fore);
    unsigned long _back = _dw_get_color(back);
    UIColor *fg = NULL;
    UIColor *bg = NULL;
    int retval = DW_ERROR_NONE;

    /* Get the UIColor for non-default colors */
    if(fore != DW_CLR_DEFAULT)
    {
        fg = [UIColor colorWithRed: DW_RED_VALUE(_fore)/255.0 green: DW_GREEN_VALUE(_fore)/255.0 blue: DW_BLUE_VALUE(_fore)/255.0 alpha: 1];
    }
    if(back != DW_CLR_DEFAULT)
    {
        bg = [UIColor colorWithRed: DW_RED_VALUE(_back)/255.0 green: DW_GREEN_VALUE(_back)/255.0 blue: DW_BLUE_VALUE(_back)/255.0 alpha: 1];
    }

    /* Get the textfield from the spinbutton */
    if([object isMemberOfClass:[DWSpinButton class]])
    {
        object = [object textfield];
    }
    /* Get the cell on classes using NSCell */
    if([object isKindOfClass:[UITextField class]])
    {
        [object setTextColor:(fg ? fg : [UIColor labelColor])];
    }
    if([object isMemberOfClass:[DWButton class]])
    {
       [[object titleLabel] setTextColor:(fg ? fg : [UIColor labelColor])];
    }
    if([object isKindOfClass:[UITextField class]] || [object isKindOfClass:[UIButton class]])
    {
        [object setBackgroundColor:(bg ? bg : [UIColor systemBackgroundColor])];
    }
    else if([object isMemberOfClass:[DWBox class]])
    {
        DWBox *box = object;

        [box setBackgroundColor:bg];
    }
    else if([object isKindOfClass:[UITableView class]])
    {
        DWContainer *cont = handle;

        [cont setBackgroundColor:(bg ? bg : [UIColor systemBackgroundColor])];
        [cont setForegroundColor:(fg ? fg : [UIColor labelColor])];
    }
    else if([object isMemberOfClass:[DWMLE class]])
    {
        DWMLE *mle = handle;
        [mle setBackgroundColor:(bg ? bg : [UIColor systemBackgroundColor])];
        [mle setTextColor:(fg ? fg : [UIColor labelColor])];
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
    /* No overlapping windows on iOS, unsupported */
    return DW_ERROR_UNKNOWN;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
DW_FUNCTION_DEFINITION(dw_window_set_style, void, HWND handle, ULONG style, ULONG mask)
DW_FUNCTION_ADD_PARAM3(handle, style, mask)
DW_FUNCTION_NO_RETURN(dw_window_set_style)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, style, ULONG, mask, ULONG)
{
    id object = _dw_text_handle(handle);

    if([object isKindOfClass:[UILabel class]])
    {
        UILabel *label = object;

        [label setTextAlignment:(style & 0xF)];
        if(mask & DW_DT_VCENTER)
            [label setBaselineAdjustment:(style & DW_DT_VCENTER ? UIBaselineAdjustmentAlignCenters : UIBaselineAdjustmentNone)];
        if(mask & DW_DT_WORDBREAK)
        {
            if(style & DW_DT_WORDBREAK)
                [label setLineBreakMode:NSLineBreakByWordWrapping];
            else
                [label setLineBreakMode:NSLineBreakByTruncatingTail];
        }
    }
    else if([object isMemberOfClass:[DWMLE class]])
    {
        DWMLE *mle = handle;
        [mle setTextAlignment:(style & mask)];
    }
    else if([object isMemberOfClass:[DWMenuItem class]])
    {
        DWMenuItem *menuitem = object;
        UIMenuElementState oldstate = [menuitem state];
        BOOL oldenabled = [menuitem enabled];

        if(mask & (DW_MIS_CHECKED | DW_MIS_UNCHECKED))
        {
            if(style & DW_MIS_CHECKED)
                [menuitem setState:UIMenuElementStateOn];
            else if(style & DW_MIS_UNCHECKED)
                [menuitem setState:UIMenuElementStateOff];
        }
        if(mask & (DW_MIS_ENABLED | DW_MIS_DISABLED))
        {
            if(style & DW_MIS_ENABLED)
                [menuitem setEnabled:YES];
            else if(style & DW_MIS_DISABLED)
                [menuitem setEnabled:NO];
        }
        /* Only force the menus to repopulate if something changed */
        if(oldstate != [menuitem state] || oldenabled != [menuitem enabled])
            [DWObj safeCall:@selector(updateMenu:) withObject:nil];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the current focus item for a window/dialog.
 * Parameters:
 *         handle: Handle to the dialog item to be focused.
 * Remarks:
 *          This is for use after showing the window/dialog.
 */
DW_FUNCTION_DEFINITION(dw_window_set_focus, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_set_focus)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    id object = handle;

    [object becomeFirstResponder];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
DW_FUNCTION_DEFINITION(dw_window_default, void, HWND handle, HWND defaultitem)
DW_FUNCTION_ADD_PARAM2(handle, defaultitem)
DW_FUNCTION_NO_RETURN(dw_window_default)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, defaultitem, HWND)
{
    DWWindow *window = handle;
    id object = defaultitem;

    if([window isKindOfClass:[UIWindow class]] && [object isKindOfClass:[UIControl class]])
    {
        [object becomeFirstResponder];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND handle, HWND next)
{
    /* TODO: Figure out how to do this if we should */
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
    /* TODO: Not sure if we should even bother with this */
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
    NSPointerArray *params = [[NSPointerArray pointerArrayWithOptions:NSPointerFunctionsOpaqueMemory] retain];
    char *font = NULL;

    [params addPointer:(void *)currfont];
    [DWObj safeCall:@selector(fontPicker:) withObject:params];
    if([params count] > 1)
        font = [params pointerAtIndex:1];

    return font;
}

/* Internal function to return a pointer to an item struct
 * with information about the packing information regarding object.
 */
Item *_dw_box_item(id object)
{
    /* Find the item within the box it is packed into */
    if([object isKindOfClass:[DWBox class]] || [object isKindOfClass:[UIControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        if([parent isKindOfClass:[DWBox class]])
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
int API dw_window_set_font(HWND handle, const char *fontname)
{
    UIFont *font = fontname ? _dw_font_by_name(fontname) :
    (DWDefaultFont ? DWDefaultFont : [UIFont systemFontOfSize:[UIFont smallSystemFontSize]]);

    if(font)
    {
        id object = _dw_text_handle(handle);
        if([object isMemberOfClass:[DWMLE class]])
        {
            DWMLE *mle = object;
            
            [mle setFont:font];
        }
        else if([object isKindOfClass:[UIControl class]])
        {
            [object setFont:font];
        }
        else if([object isMemberOfClass:[DWRender class]])
        {
            DWRender *render = object;

            [render setFont:font];
        }
        else
            return DW_ERROR_UNKNOWN;
        /* If we changed the text... */
        Item *item = _dw_box_item(handle);

        /* Check to see if any of the sizes need to be recalculated */
        if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
        {
            _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
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
    id object = _dw_text_handle(handle);
    UIFont *font = nil;

    if([object isKindOfClass:[UIControl class]] || [object isMemberOfClass:[DWRender class]])
    {
         font = [object font];
    }
    if(font)
    {
        NSString *fontname = [font fontName];
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
DW_FUNCTION_DEFINITION(dw_window_destroy, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_destroy, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id object = handle;
    int retval = 0;

    /* Handle destroying a top-level window */
    if([object isKindOfClass:[UIWindow class]])
    {
        DWWindow *window = handle;
        [window setHidden:YES];
        [_dw_toplevel_windows removeObject:window];
        for(id object in [[[window rootViewController] view] subviews])
        {
            [object removeFromSuperview];
            [object release];
        }
        [[[window rootViewController] view] removeFromSuperview];
        [window setRootViewController:nil];
    }
    /* Handle removing menu items from menus */
    else if([object isMemberOfClass:[DWMenuItem class]])
    {
        DWMenuItem *item = object;
        DWMenu *menu = [item menu];

        [menu removeItem:object];
    }
    /* Handle destroying a control or box */
    else if([object isKindOfClass:[UIView class]] || [object isKindOfClass:[UIControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        if([parent isKindOfClass:[DWBox class]])
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

            if(index != -1)
            {
                [object removeFromSuperview];
                [object release];

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
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       text: The text associsated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_get_text, char *, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_get_text, char *)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = _dw_text_handle(handle);
    id control = handle;
    NSString *nsstr = nil;
    char *retval = NULL;

    if([control isKindOfClass:[UIButton class]])
    {
        nsstr = [control titleForState:UIControlStateNormal];
    }
    else if([object isKindOfClass:[UILabel class]] || [object isKindOfClass:[UITextField class]])
    {
        nsstr = [object text];
    }
    else if([object isMemberOfClass:[DWWindow class]])
    {
        DWWindow *window = object;
        UIView *view = [[window rootViewController] view];
        NSArray *array = [view subviews];

        for(id obj in array)
        {
            if([obj isMemberOfClass:[UINavigationBar class]])
            {
                UINavigationBar *nav = obj;
                UINavigationItem *item = [[nav items] firstObject];

                nsstr = [item title];
            }
        }
    }
    if(nsstr)
        retval = strdup([nsstr UTF8String]);
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_set_text, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_window_set_text)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    id object = _dw_text_handle(handle);
    id control = handle;
    Item *item = NULL;

    if([control isKindOfClass:[UIButton class]])
    {
        [control setTitle:[NSString stringWithUTF8String:text] forState:UIControlStateNormal];
        item = _dw_box_item(handle);
    }
    else if([object isKindOfClass:[UILabel class]] || [object isKindOfClass:[UITextField class]])
    {
        [object setText:[NSString stringWithUTF8String:text]];
        item = _dw_box_item(handle);
    }
    else if([object isMemberOfClass:[DWWindow class]])
    {
        DWWindow *window = object;
        UIView *view = [[window rootViewController] view];
        NSArray *array = [view subviews];
        NSString *nstr = [NSString stringWithUTF8String:text];

        [window setLargeContentTitle:nstr];

        for(id obj in array)
        {
            if([obj isMemberOfClass:[UINavigationBar class]])
            {
                UINavigationBar *nav = obj;
                UINavigationItem *item = [[nav items] firstObject];

                [item setTitle:nstr];
            }
        }
    }
    /* If we changed the text...
     * Check to see if any of the sizes need to be recalculated
     */
    if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
    {
      int newwidth, newheight;

      _dw_control_size(handle, &newwidth, &newheight);

      /* Only update the item and redraw the window if it changed */
      if((item->origwidth == DW_SIZE_AUTO && item->width != newwidth) ||
         (item->origheight == DW_SIZE_AUTO && item->height != newheight))
      {
         if(item->origwidth == DW_SIZE_AUTO)
            item->width = newwidth;
         if(item->origheight == DW_SIZE_AUTO)
            item->height = newheight;
         /* Queue a redraw on the top-level window */
         _dw_redraw([object window], TRUE);
      }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, const char *bubbletext)
{
    /* Tooltips don't exist on iOS */
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
DW_FUNCTION_DEFINITION(dw_window_disable, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_disable)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[UIScrollView class]])
    {
        UIScrollView *sv = handle;
        NSArray *subviews = [sv subviews];
        object = [subviews firstObject];
    }
    if([object isKindOfClass:[UIControl class]] || [object isMemberOfClass:[DWMenuItem class]])
    {
        [object setEnabled:NO];
    }
    if([object isKindOfClass:[UITextView class]])
    {
        UITextView *mle = object;

        [mle setEditable:NO];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
DW_FUNCTION_DEFINITION(dw_window_enable, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_enable)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[UIScrollView class]])
    {
        UIScrollView *sv = handle;
        NSArray *subviews = [sv subviews];
        object = [subviews firstObject];
    }
    if([object isKindOfClass:[UIControl class]] || [object isMemberOfClass:[DWMenuItem class]])
    {
        [object setEnabled:YES];
    }
    if([object isKindOfClass:[UITextView class]])
    {
        UITextView *mle = object;

        [mle setEditable:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
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
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long cid, const char *data, int len)
{
    id object = handle;
    int retval = DW_ERROR_UNKNOWN;

    if([object isKindOfClass:[UIImageView class]] || [object isMemberOfClass:[DWButton class]])
    {
        if(data)
        {
            DW_LOCAL_POOL_IN;
            NSData *thisdata = [NSData dataWithBytes:data length:len];
            UIImage *pixmap = [[UIImage alloc] initWithData:thisdata];

            if(pixmap)
            {
                if([object isMemberOfClass:[DWButton class]])
                {
                    DWButton *button = object;
                    [button setImage:pixmap forState:UIControlStateNormal];
                }
                else
                {
                    UIImageView *iv = object;
                    [iv setImage:pixmap];
                }
                /* If we changed the bitmap... */
                Item *item = _dw_box_item(handle);

                /* Check to see if any of the sizes need to be recalculated */
                if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
                {
                    _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
                    /* Queue a redraw on the top-level window */
                    _dw_redraw([object window], TRUE);
                }
                retval = DW_ERROR_NONE;
            }
            else
                retval = DW_ERROR_GENERAL;
            DW_LOCAL_POOL_OUT;
        }
        else
            return dw_window_set_bitmap(handle, cid, NULL);
    }
    return retval;
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
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap(HWND handle, unsigned long resid, const char *filename)
{
    id object = handle;
    int retval = DW_ERROR_UNKNOWN;
    DW_LOCAL_POOL_IN;

    if([object isKindOfClass:[UIImageView class]] || [object isMemberOfClass:[DWButton class]])
    {
        UIImage *bitmap = nil;

        if(filename)
        {
            char *ext = _dw_get_image_extension( filename );
            NSString *nstr = [ NSString stringWithUTF8String:filename ];

            bitmap = [[UIImage alloc] initWithContentsOfFile:nstr];

            if(!bitmap && ext)
            {
                nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
                bitmap = [[UIImage alloc] initWithContentsOfFile:nstr];
            }
            if(!bitmap)
                retval = DW_ERROR_GENERAL;
        }
        if(!bitmap && resid > 0 && resid < 65536)
        {
            bitmap = _dw_icon_load(resid);
            if(!bitmap)
                retval = DW_ERROR_GENERAL;
        }

        if(bitmap)
        {
            if([object isMemberOfClass:[DWButton class]])
            {
                DWButton *button = object;
                [button setImage:bitmap forState:UIControlStateNormal];
            }
            else
            {
                UIImageView *iv = object;
                [iv setImage:bitmap];
            }

            /* If we changed the bitmap... */
            Item *item = _dw_box_item(handle);

            /* Check to see if any of the sizes need to be recalculated */
            if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
            {
                _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
                /* Queue a redraw on the top-level window */
                _dw_redraw([object window], TRUE);
            }
            retval = DW_ERROR_NONE;
        }
    }
    DW_LOCAL_POOL_OUT;
    return retval;
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
    UIView *view = handle;
    if([ object isKindOfClass:[ UIWindow class ] ])
    {
        UIWindow *window = handle;
        NSArray *subviews = [window subviews];
        view = [subviews firstObject];
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
    /* TODO: Not sure if we should do anything here on iOS */
    return DW_ERROR_GENERAL;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
    DWWindow *window = handle;
    NSArray *array = [[[window rootViewController] view] subviews];

    for(id obj in array)
    {
        if([obj isMemberOfClass:[DWView class]])
        {
            DWView *view = obj;
            [view showWindow];
        }
    }

    [window setShown:YES];
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
    /* TODO: Not sure if we should do anything here on iOS */
    return DW_ERROR_GENERAL;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
    /* TODO: Not sure if we should do anything here on iOS */
    return DW_ERROR_GENERAL;
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
    /* On iOS the window usually takes up the full screen */
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

    if([object isMemberOfClass:[DWBox class]])
    {
        Box *thisbox;

        if((thisbox = (Box *)[object box]))
        {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, 0, 0, 1);

            /* Return what was requested */
            if(width) *width = thisbox->minwidth;
            if(height) *height = thisbox->minheight;
        }
    }
    else
        _dw_control_size(handle, width, height);
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

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
void API dw_window_set_pos(HWND handle, LONG x, LONG y)
{
    /* iOS windows take up the whole screen */
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
    /* iOS windows take up the whole screen */
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

    if([object isKindOfClass:[UIWindow class]])
    {
        UIWindow *window = handle;
        CGRect rect = [window frame];
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
    else if([object isKindOfClass:[UIControl class]])
    {
        UIControl *control = handle;
        CGRect rect = [control frame];
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
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    return screenRect.size.width;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    return screenRect.size.height;
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
    /* iOS always runs in 32bit depth */
    return 32;
}

/*
 * Creates a new system notification if possible.
 * Parameters:
 *         title: The short title of the notification.
 *         imagepath: Path to an image to display or NULL if none.
 *         description: A longer description of the notification,
 *                      or NULL if none is necessary.
 * Returns:
 *         A handle to the notification which can be used to attach a "clicked" event if desired,
 *         or NULL if it fails or notifications are not supported by the system.
 * Remarks:
 *          This will create a system notification that will show in the notifaction panel
 *          on supported systems, which may be clicked to perform another task.
 */
HWND API dw_notification_new(const char *title, const char *imagepath, const char *description, ...)
{
    char outbuf[1025] = {0};
    HWND retval = NULL;

    if(description)
    {
        va_list args;

        va_start(args, description);
        vsnprintf(outbuf, 1024, description, args);
        va_end(args);
    }

    /* Configure the notification's payload. */
    if (@available(iOS 10.0, *))
    {
        if([[NSBundle mainBundle] bundleIdentifier] != nil)
        {
            UNMutableNotificationContent* notification = [[UNMutableNotificationContent alloc] init];

            if(notification)
            {
                notification.title = [NSString stringWithUTF8String:title];
                if(description)
                    notification.body = [NSString stringWithUTF8String:outbuf];
                if(imagepath)
                {
                    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:imagepath]];
                    NSError *error;
                    UNNotificationAttachment *attachment = [UNNotificationAttachment attachmentWithIdentifier:@"imageID"
                                                                URL:url
                                                            options:nil
                                                              error:&error];
                    if(attachment)
                        notification.attachments = @[attachment];
                }
                retval = notification;
            }
        }
    }
    return retval;
}

/*
 * Sends a notification created by dw_notification_new() after attaching signal handler.
 * Parameters:
 *         notification: The handle to the notification returned by dw_notification_new().
 * Returns:
 *         DW_ERROR_NONE on success, DW_ERROR_UNKNOWN on error or not supported.
 */
int API dw_notification_send(HWND notification)
{
    if(notification)
    {
        NSString *notid = [NSString stringWithFormat:@"dw-notification-%llu", DW_POINTER_TO_ULONGLONG(notification)];
        
        /* Schedule the notification. */
        if (@available(iOS 10.0, *))
        {
            if([[NSBundle mainBundle] bundleIdentifier] != nil)
            {
                UNMutableNotificationContent* content = (UNMutableNotificationContent *)notification;
                UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:notid content:content trigger:nil];

                UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
                [center addNotificationRequest:request withCompletionHandler:nil];
            }
        }
        return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}


/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
    memset(env, '\0', sizeof(DWEnv));
    strcpy(env->osName, "iOS");

    strncpy(env->buildDate, __DATE__, sizeof(env->buildDate)-1);
    strncpy(env->buildTime, __TIME__, sizeof(env->buildTime)-1);
    strncpy(env->htmlEngine, "WEBKIT", sizeof(env->htmlEngine)-1);
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
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
    AudioServicesPlayAlertSound((SystemSoundID)1104);
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
UserData *_dw_find_userdata(UserData **root, const char *varname)
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

int _dw_new_userdata(UserData **root, const char *varname, void *data)
{
    UserData *new = _dw_find_userdata(root, varname);

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

int _dw_remove_userdata(UserData **root, const char *varname, int all)
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
DW_FUNCTION_DEFINITION(dw_window_set_data, void, HWND window, const char *dataname, void *data)
DW_FUNCTION_ADD_PARAM3(window, dataname, data)
DW_FUNCTION_NO_RETURN(dw_window_set_data)
DW_FUNCTION_RESTORE_PARAM3(window, HWND, dataname, const char *, data, void *)
{
    id object = window;
    if([object isMemberOfClass:[DWWindow class]])
    {
        UIWindow *win = window;
        NSArray *subviews = [win subviews];
        for(id obj in subviews)
        {
            if([obj isMemberOfClass:[DWView class]])
            {
                object = obj;
                break;
            }
        }
    }
    else if([object isMemberOfClass:[UIScrollView class]])
    {
        UIScrollView *sv = window;
        NSArray *subviews = [sv subviews];
        object = [subviews firstObject];
    }
    /* Failsafe so we don't crash */
    if(![object respondsToSelector:NSSelectorFromString(@"userdata")])
    {
#ifdef DEBUG
        NSLog(@"WARNING: Object class %@ does not support dw_window_set_data()\n", [object className]);
#endif
    }
    else
    {
        WindowData *blah = (WindowData *)[object userdata];

        if(!blah)
        {
            if(!dataname)
                return;

            blah = calloc(1, sizeof(WindowData));
            [object setUserdata:blah];
        }

        if(data)
            _dw_new_userdata(&(blah->root), dataname, data);
        else
        {
            if(dataname)
                _dw_remove_userdata(&(blah->root), dataname, FALSE);
            else
                _dw_remove_userdata(&(blah->root), NULL, TRUE);
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
DW_FUNCTION_DEFINITION(dw_window_get_data, void *, HWND window, const char *dataname)
DW_FUNCTION_ADD_PARAM2(window, dataname)
DW_FUNCTION_RETURN(dw_window_get_data, void *)
DW_FUNCTION_RESTORE_PARAM2(window, HWND, dataname, const char *)
{
    id object = window;
    void *retval = NULL;

    if([object isMemberOfClass:[DWWindow class]])
    {
        UIWindow *win = window;
        NSArray *subviews = [win subviews];
        for(id obj in subviews)
        {
            if([obj isMemberOfClass:[DWView class]])
            {
                object = obj;
                break;
            }
        }
    }
    else if([object isMemberOfClass:[UIScrollView class]])
    {
        UIScrollView *sv = window;
        NSArray *subviews = [sv subviews];
        object = [subviews firstObject];
    }
    /* Failsafe so we don't crash */
    if(![object respondsToSelector:NSSelectorFromString(@"userdata")])
    {
#ifdef DEBUG
        NSLog(@"WARNING: Object class %@ does not support dw_window_get_data()\n", [object className]);
#endif
    }
    else
    {
        WindowData *blah = (WindowData *)[object userdata];

        if(blah && blah->root && dataname)
        {
            UserData *ud = _dw_find_userdata(&(blah->root), dataname);
            if(ud)
                retval = ud->data;
        }
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Compare two window handles.
 * Parameters:
 *       window1: First window handle to compare.
 *       window2: Second window handle to compare.
 * Returns:
 *       TRUE if the windows are the same object, FALSE if not.
 */
int API dw_window_compare(HWND window1, HWND window2)
{
    /* If anything special is require to compare... do it
     * here otherwise just compare the handles.
     */
    if(window1 && window2 && window1 == window2)
        return TRUE;
    return FALSE;
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
DW_FUNCTION_DEFINITION(dw_timer_connect, HTIMER, int interval, void *sigfunc, void *data)
DW_FUNCTION_ADD_PARAM3(interval, sigfunc, data)
DW_FUNCTION_RETURN(dw_timer_connect, HTIMER)
DW_FUNCTION_RESTORE_PARAM3(interval, int, sigfunc, void *, data, void *)
{
    HTIMER retval = 0;

    if(sigfunc)
    {
        NSTimeInterval seconds = (double)interval / 1000.0;

        retval = [NSTimer scheduledTimerWithTimeInterval:seconds target:DWHandler selector:@selector(runTimer:) userInfo:nil repeats:YES];
        _dw_new_signal(0, retval, 0, sigfunc, NULL, data);
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
DW_FUNCTION_DEFINITION(dw_timer_disconnect, void, HTIMER timerid)
DW_FUNCTION_ADD_PARAM1(timerid)
DW_FUNCTION_NO_RETURN(dw_timer_disconnect)
DW_FUNCTION_RESTORE_PARAM1(timerid, HTIMER)
{
    DWSignalHandler *prev = NULL, *tmp = DWRoot;

    /* 0 is an invalid timer ID */
    if(timerid)
    {
        NSTimer *thistimer = timerid;

        [thistimer invalidate];

        while(tmp)
        {
            if(tmp->window == thistimer)
            {
                if(prev)
                {
                    prev->next = tmp->next;
                    free(tmp);
                    tmp = prev->next;
                }
                else
                {
                    DWRoot = tmp->next;
                    free(tmp);
                    tmp = DWRoot;
                }
            }
            else
            {
                prev = tmp;
                tmp = tmp->next;
            }
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Add a callback to a window event.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect(HWND window, const char *signame, void *sigfunc, void *data)
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
void API dw_signal_connect_data(HWND window, const char *signame, void *sigfunc, void *discfunc, void *data)
{
    ULONG message = 0, msgid = 0;

    /* Handle special case of application delete signal */
    if(!window && signame && strcmp(signame, DW_SIGNAL_DELETE) == 0)
    {
        window = DWApp;
    }

    if(window && signame && sigfunc)
    {
        if((message = _dw_findsigmessage(signame)) != 0)
        {
            _dw_new_signal(message, window, (int)msgid, sigfunc, discfunc, data);
        }
    }
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_name(HWND window, const char *signame)
{
    DWSignalHandler *prev = NULL, *tmp = DWRoot;
    ULONG message;

    if(!window || !signame || (message = _dw_findsigmessage(signame)) == 0)
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
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
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
    DWSignalHandler *prev = NULL, *tmp = DWRoot;

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
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
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
    DWSignalHandler *prev = NULL, *tmp = DWRoot;

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
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

void _dw_my_strlwr(char *buf)
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
int API dw_module_load(const char *name, HMOD *handle)
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
   _dw_my_strlwr(newname);

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
int API dw_module_symbol(HMOD handle, const char *name, void**func)
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
int API dw_module_close(HMOD handle)
{
   if(handle)
      return dlclose(handle);
   return 0;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
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
void API dw_mutex_close(HMTX mutex)
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
void API dw_mutex_lock(HMTX mutex)
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
            _dw_main_iteration([NSDate dateWithTimeIntervalSinceNow:0.01]);
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
void API dw_mutex_unlock(HMTX mutex)
{
   pthread_mutex_unlock(mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
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
int API dw_event_reset(HEV eve)
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
int API dw_event_post(HEV eve)
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
int API dw_event_wait(HEV eve, unsigned long timeout)
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
int API dw_event_close(HEV *eve)
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

struct _dw_seminfo {
   int fd;
   int waiting;
};

static void _dw_handle_sem(int *tmpsock)
{
   fd_set rd;
   struct _dw_seminfo *array = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo));
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
         struct _dw_seminfo *newarray;
            int newfd = accept(listenfd, 0, 0);

         if(newfd > -1)
         {
            /* Add new connections to the set */
            newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount+1));
            memcpy(newarray, array, sizeof(struct _dw_seminfo)*(connectcount));

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
               struct _dw_seminfo *newarray = NULL;

               /* Remove this connection from the set */
               if(connectcount > 1)
               {
                   newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount-1));
                   if(!z)
                       memcpy(newarray, &array[1], sizeof(struct _dw_seminfo)*(connectcount-1));
                   else
                   {
                       memcpy(newarray, array, sizeof(struct _dw_seminfo)*z);
                       if(z!=(connectcount-1))
                           memcpy(&newarray[z], &array[z+1], sizeof(struct _dw_seminfo)*(connectcount-(z+1)));
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
HEV API dw_named_event_new(const char *name)
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
    pthread_create(&dwthread, NULL, (void *)_dw_handle_sem, (void *)tmpsock);
    eve->alive = ev;
    return eve;
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_get(const char *name)
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
int API dw_named_event_reset(HEV eve)
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
int API dw_named_event_post(HEV eve)
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
int API dw_named_event_wait(HEV eve, unsigned long timeout)
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
int API dw_named_event_close(HEV eve)
{
   /* Finally close the domain socket,
    * cleanup will continue in _dw_handle_sem.
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
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
    pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
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
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
    _dw_init_colors();
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
    UIColor *color;

    /* Release the pool when we are done so we don't leak */
    color = pthread_getspecific(_dw_fg_color_key);
    [color release];
    color = pthread_getspecific(_dw_bg_color_key);
    [color release];
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
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
void API dw_font_set_default(const char *fontname)
{
    UIFont *oldfont = DWDefaultFont;
    DWDefaultFont = nil;
    if(fontname)
    {
        DWDefaultFont = _dw_font_by_name(fontname);
        [DWDefaultFont retain];
    }
    [oldfont release];
}

/* Thread start stub to launch our main */
void _dw_main_launch(void **data)
{
    int (*_dwmain)(int argc, char **argv) = (int (*)(int, char **))data[0];
    int argc = DW_POINTER_TO_INT(data[1]);
    char **argv = (char **)data[2];

    _dwmain(argc, argv);
    free(data);
}

/* The iOS main thread... all this does is handle messages */
void _dw_main_thread(int argc, char *argv[])
{
    DWMainEvent = dw_event_new();
    dw_event_reset(DWMainEvent);
    /* We wait for the dw_init() in the user's main function */
    dw_event_wait(DWMainEvent, DW_TIMEOUT_INFINITE);
    DWThread = dw_thread_id();
    DWObj = [[DWObject alloc] init];
    /* Create object for handling timers */
    DWHandler = [[DWTimerHandler alloc] init];
    UIApplicationMain(argc, argv, nil, NSStringFromClass([DWAppDel class]));
    /* Shouldn't get here, but ... just in case */
    DWThread = (DWTID)-1;
}

/* If DWApp is uninitialized, initialize it */
void _dw_app_init(void)
{
    /* The UIApplication singleton won't be created until we call
     * UIApplicationMain() which blocks indefinitely.. so we have
     * a thread to just run UIApplicationMain() .. and wait until
     * we get a non-nil sharedApplication result.
     */
    while(!DWApp)
    {
        static int posted = FALSE;

        if(DWMainEvent && !posted)
        {
            /* Post the DWMainEvent so the main thread
             * will call UIApplicationMain() creating
             * the UIApplication sharedApplication.
             */
            posted = TRUE;
            dw_event_post(DWMainEvent);
        }
        pthread_yield_np();
        DWApp = [UIApplication sharedApplication];
    }
    dw_event_reset(DWMainEvent);
    [DWObj performSelectorOnMainThread:@selector(delayedInit:) withObject:nil waitUntilDone:YES];
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
        char *binname = strrchr(pathcopy, '/');

        if(binname && (binname++) && !_dw_app_id[0])
        {
            /* If we have a binary name, use that for the Application ID instead. */
            snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.%s", DW_APP_DOMAIN_DEFAULT, binname);
        }
        if(app)
        {
            char pathbuf[PATH_MAX+1] = { 0 };
            size_t len = (size_t)(app - pathcopy);

            if(len > 0)
            {
                strncpy(_dw_bundle_path, pathcopy, len + 4);
            }
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
    _dw_app_init();
    pthread_key_create(&_dw_pool_key, NULL);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    _dw_toplevel_windows = [[NSMutableArray new] retain];
    pthread_setspecific(_dw_pool_key, pool);
    pthread_key_create(&_dw_fg_color_key, NULL);
    pthread_key_create(&_dw_bg_color_key, NULL);
    _dw_init_colors();
    DWDefaultFont = nil;
    if (@available(iOS 10.0, *))
    {
        if([[NSBundle mainBundle] bundleIdentifier] != nil)
        {
            UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
            if(center)
            {
                [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert|UNAuthorizationOptionSound
                        completionHandler:^(BOOL granted, NSError * _Nullable error) {
                            if (granted)
                            {
                                center.delegate = [[DWUserNotificationCenterDelegate alloc] init];
                            }
                            else
                            {
                                NSLog(@"WARNING: Unable to get notification permission. %@", error.localizedDescription);
                            }
                }];
            }
        }
    }
    _DWDirtyDrawables = [[NSMutableArray alloc] init];
    /* Use NSThread to start a dummy thread to initialize the threading subsystem */
    NSThread *thread = [[ NSThread alloc] initWithTarget:DWObj selector:@selector(uselessThread:) object:nil];
    [thread start];
    [thread release];
    if(!_dw_app_id[0])
    {
        /* Generate an Application ID based on the PID if all else fails. */
        snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, getpid());
    }
    return DW_ERROR_NONE;
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         handle: A pointer to receive a SHM identifier.
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 */
HSHM API dw_named_memory_new(void **dest, int size, const char *name)
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
HSHM API dw_named_memory_get(void **dest, int size, const char *name)
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
int API dw_named_memory_free(HSHM handle, void *ptr)
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
DWTID API dw_thread_new(void *func, void *data, int stack)
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
void API dw_thread_end(void)
{
   pthread_exit(NULL);
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
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
 *       DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_exec(const char *program, int type, char **params)
{
    int retval = DW_ERROR_UNKNOWN;
    pid_t pid;

    /* Launch the application directly using posix_spawnp()
     * Might need to use openURLs to open DW_EXEC_GUI.
     */
    if(posix_spawnp(&pid, program, NULL, NULL, params, NULL) == 0)
    {
        if(pid > 0)
            retval = pid;
        else
            retval = DW_ERROR_NONE;
    }
    return retval;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(const char *url)
{
    NSURL *myurl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    [DWApp openURL:myurl options:@{}
           completionHandler:^(BOOL success) {}];
    return DW_ERROR_NONE;
}

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
HPRINT API dw_print_new(const char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
    if(drawfunc)
    {
        UIPrintInfo *pi = [UIPrintInfo alloc];
        DWPrintPageRenderer *renderer = [DWPrintPageRenderer new];
        NSMutableArray *pa = [[[NSMutableArray alloc] initWithObjects:pi, renderer, nil] retain];

        if(!jobname)
            jobname = "Dynamic Windows Print Job";

        [pi setOutputType:UIPrintInfoOutputGeneral];
        [pi setJobName:[NSString stringWithUTF8String:jobname]];
        [pi setOrientation:UIPrintInfoOrientationPortrait];
        [renderer setDrawFunc:drawfunc];
        [renderer setDrawData:drawdata];
        [renderer setPages:pages];
        [renderer setFlags:flags];
        [renderer setPrint:pa];
        return pa;
    }
    return NULL;
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
    int retval = DW_ERROR_UNKNOWN;

    if(print)
    {
        NSMutableArray *pa = print;

        [pa addObject:[NSNumber numberWithUnsignedLong:flags]];
        [DWObj safeCall:@selector(printDialog:) withObject:pa];
        retval = (int)[[pa lastObject] integerValue];
        [pa release];
    }
    return retval;
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
    if(print)
    {
        NSMutableArray *pa = print;
        UIPrintInteractionController *pc = [UIPrintInteractionController sharedPrintController];

        [pc dismissAnimated:YES];
        [pa release];
    }
}

/*
 * Converts a UTF-8 encoded string into a wide string.
 * Parameters:
 *       utf8string: UTF-8 encoded source string.
 * Returns:
 *       Wide string that needs to be freed with dw_free()
 *       or NULL on failure.
 */
wchar_t * API dw_utf8_to_wchar(const char *utf8string)
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
char * API dw_wchar_to_utf8(const wchar_t *wstring)
{
    size_t bufflen = 8 * wcslen(wstring) + 1;
    char *temp = malloc(bufflen);
    if(temp)
        wcstombs(temp, wstring, bufflen);
    return temp;
}

/*
 * Gets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_FEATURE_DISABLED if the feature is supported but disabled.
 *       DW_FEATURE_ENABLED if the feature is supported and enabled.
 *       Other value greater than 1, same as enabled but with extra info.
 */
int API dw_feature_get(DWFEATURE feature)
{
    switch(feature)
    {
        case DW_FEATURE_NOTIFICATION:
        case DW_FEATURE_MLE_AUTO_COMPLETE:
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_TREE:
        case DW_FEATURE_RENDER_SAFE:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_CONTAINER_MODE:
            return _dw_container_mode;
        case DW_FEATURE_DARK_MODE:
        {
            if(DWObj)
            {
                NSMutableArray *array = [[NSMutableArray alloc] init];
                int retval = DW_FEATURE_UNSUPPORTED;
                NSNumber *number;

                [DWObj safeCall:@selector(getUserInterfaceStyle:) withObject:array];
                number = [array lastObject];
                if(number)
                    retval = [number intValue];
                return retval;
            }
            return _dw_dark_mode_state;
        }
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}

/*
 * Sets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 *       state: DW_FEATURE_DISABLED, DW_FEATURE_ENABLED or any value greater than 1.
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_ERROR_NONE if the feature is supported and successfully configured.
 *       DW_ERROR_GENERAL if the feature is supported but could not be configured.
 * Remarks: 
 *       These settings are typically used during dw_init() so issue before 
 *       setting up the library with dw_init().
 */
int API dw_feature_set(DWFEATURE feature, int state)
{
    switch(feature)
    {
        /* These features are supported but not configurable */
        case DW_FEATURE_NOTIFICATION:
        case DW_FEATURE_MLE_AUTO_COMPLETE:
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_TREE:
        case DW_FEATURE_RENDER_SAFE:
            return DW_ERROR_GENERAL;
        case DW_FEATURE_CONTAINER_MODE:
        {
            if(state >= DW_CONTAINER_MODE_DEFAULT && state < DW_CONTAINER_MODE_MAX)
            {
                _dw_container_mode = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
        case DW_FEATURE_DARK_MODE:
        {
            /* Make sure DWObj is initialized */
            if(DWObj)
            {
                /* Disabled forces the non-dark aqua theme */
                if(state == DW_FEATURE_DISABLED)
                    [DWObj safeCall:@selector(setUserInterfaceStyle:) withObject:[NSNumber numberWithInt:UIUserInterfaceStyleLight]];
                /* Enabled lets the OS decide the mode */
                else if(state == DW_FEATURE_ENABLED || state == DW_DARK_MODE_FULL)
                    [DWObj safeCall:@selector(setUserInterfaceStyle:) withObject:[NSNumber numberWithInt:UIUserInterfaceStyleUnspecified]];
                /* 2 forces dark mode aqua appearance */
                else if(state == DW_DARK_MODE_FORCED)
                    [DWObj safeCall:@selector(setUserInterfaceStyle:) withObject:[NSNumber numberWithInt:UIUserInterfaceStyleDark]];
            }
            else /* Save the requested state for dw_init() */
                _dw_dark_mode_state = state;
            return DW_ERROR_NONE;
        }
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}
