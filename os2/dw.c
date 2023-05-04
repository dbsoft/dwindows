/*
 * Dynamic Windows:
 *          A GTK like implementation of the PM GUI
 *
 * (C) 2000-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2003-2021 Mark Hessling <mark@rexx.org>
 * (C) 2000 Achim Hasenmueller <achimha@innotek.de>
 * (C) 2000 Peter Nielsen <peter@pmview.com>
 * (C) 2007 Alex Taylor (some code borrowed from clipuni)
 * (C) 1998 Sergey I. Yevtushenko (some code borrowed from cell toolkit)
 *
 */
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI
#define INCL_DEV
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_SPLERRORS

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <process.h>
#include <time.h>
#include <io.h>
#include <math.h>
#ifndef __EMX__
#include <direct.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#ifdef __WATCOMC__
#include <alloca.h>
#endif
#include <fcntl.h>
#ifdef UNICODE
#include <uconv.h>
#include <unikbd.h>
#endif
#include "dw.h"

#define QWP_USER 0

/* The toolkit headers don't seem to have this */
BOOL APIENTRY WinStretchPointer(HPS hps, LONG x, LONG y, LONG cx, LONG cy, HPOINTER hptr, ULONG fs);

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifdef __IBMC__
#define API_FUNC * API
#else
#define API_FUNC API *
#endif

MRESULT EXPENTRY _dw_run_event(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY _dw_wndproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY _dw_scrollwndproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void _dw_do_resize(Box *thisbox, int x, int y);
void _dw_handle_splitbar_resize(HWND hwnd, float percent, int type, int x, int y);
int _dw_load_bitmap_file(char *file, HWND handle, HBITMAP *hbm, HDC *hdc, HPS *hps, unsigned long *width, unsigned long *height, int *depth, unsigned long backrgb);
void _dw_free_menu_data(HWND menu);
BOOL (API_FUNC _WinQueryDesktopWorkArea)(HWND hwndDesktop, PWRECT pwrcWorkArea) = 0;
/* PMPrintf support for dw_debug() */
ULONG (API_FUNC _PmPrintfString)(char *String) = 0;
/* GBM (Generalize Bitmap Module) support for file loading */
#pragma pack(4)
typedef struct
{
    int w, h, bpp;
    unsigned char priv[2000];
} GBM;
typedef struct { unsigned char r, g, b; } GBMRGB;
#pragma pack()
int (API_FUNC _gbm_init)(void) = 0;
int (API_FUNC _gbm_deinit)(void) = 0;
int (API_FUNC _gbm_query_n_filetypes)(int *count) = 0;
int (API_FUNC _gbm_io_open)(const char *fn, int mode) = 0;
int (API_FUNC _gbm_io_close)(int fd) = 0;
int (API_FUNC _gbm_read_header)(const char *fn, int fd, int ft, GBM *gbm, const char *info) = 0;
int (API_FUNC _gbm_read_palette)(int fd, int ft, GBM *gbm, GBMRGB *gbmrgb) = 0;
int (API_FUNC _gbm_read_data)(int fd, int ft, GBM *gbm, unsigned char *data) = 0;
const char * (API_FUNC _gbm_err)(int rc) = 0;
/*
 * GBM List of supported formats: BMP, PNG, JPEG, Targa, TIFF and XPM.
 */
#define NUM_EXTS 8
char *_dw_image_exts[NUM_EXTS] =
{
   ".bmp",
   ".png",
   ".jpg",
   ".jpeg",
   ".tga",
   ".tif",
   ".tiff",
   ".xpm"
};


char ClassName[] = "dynamicwindows";
char SplitbarClassName[] = "dwsplitbar";
char ScrollClassName[] = "dwscroll";
char CalendarClassName[] = "dwcalendar";
char *DefaultFont = "9.WarpSans";

HAB dwhab = 0;
HMQ dwhmq = 0;
DWTID _dwtid = 0;
LONG _dw_foreground = 0xAAAAAA, _dw_background = DW_CLR_DEFAULT;

HWND _dw_app = NULLHANDLE, _dw_bubble = NULLHANDLE, _dw_bubble_last = NULLHANDLE, _dw_emph = NULLHANDLE;
HWND _dw_tray = NULLHANDLE, _dw_task_bar = NULLHANDLE;

PRECORDCORE _dw_core_emph = NULL;
ULONG _dw_ver_buf[4];
HWND _dw_lasthcnr = 0, _dw_lastitem = 0, _dw_popup = 0, _dw_desktop;
HMOD _dw_wpconfig = 0, _dw_pmprintf = 0, _dw_pmmerge = 0, _dw_gbm = 0;
static char _dw_exec_dir[MAX_PATH+1] = {0};
static int _dw_render_safe_mode = DW_FEATURE_DISABLED;
static HWND _dw_render_expose = DW_NOHWND;

/* Return TRUE if it is safe to draw on the window handle.
 * Either we are in unsafe mode, or we are in an EXPOSE
 * event for the requested render window handle.
 */
int _dw_render_safe_check(HWND handle)
{
    if(_dw_render_safe_mode == DW_FEATURE_DISABLED || 
       (handle && _dw_render_expose == handle))
           return TRUE;
    return FALSE;
}

int _dw_is_render(HWND handle)
{
   if(dw_window_get_data(handle, "_dw_render"))
       return TRUE;
   return FALSE;
}

#ifdef UNICODE
/* Atom for "text/unicode" clipboard format */
ATOM  Unicode;
KHAND Keyboard;
UconvObject Uconv;                      /* conversion object */
#endif

unsigned long _dw_colors[] = {
   CLR_BLACK,
   CLR_DARKRED,
   CLR_DARKGREEN,
   CLR_BROWN,
   CLR_DARKBLUE,
   CLR_DARKPINK,
   CLR_DARKCYAN,
   CLR_PALEGRAY,
   CLR_DARKGRAY,
   CLR_RED,
   CLR_GREEN,
   CLR_YELLOW,
   CLR_BLUE,
   CLR_PINK,
   CLR_CYAN,
   CLR_WHITE
};

#define DW_OS2_NEW_WINDOW        1

#define IS_WARP4() (_dw_ver_buf[0] == 20 && _dw_ver_buf[1] >= 40)

#ifndef min
#define min(a, b) (((a < b) ? a : b))
#endif

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

DWSignalHandler *Root = NULL;

typedef struct
{
   ULONG message;
   char name[30];

} DWSignalList;

/* List of signals and their equivilent OS/2 message */
#define SIGNALMAX 16

DWSignalList DWSignalTranslate[SIGNALMAX] = {
   { WM_SIZE,         DW_SIGNAL_CONFIGURE },
   { WM_CHAR,         DW_SIGNAL_KEY_PRESS },
   { WM_BUTTON1DOWN,  DW_SIGNAL_BUTTON_PRESS },
   { WM_BUTTON1UP,    DW_SIGNAL_BUTTON_RELEASE },
   { WM_MOUSEMOVE,    DW_SIGNAL_MOTION_NOTIFY },
   { WM_CLOSE,        DW_SIGNAL_DELETE },
   { WM_PAINT,        DW_SIGNAL_EXPOSE },
   { WM_COMMAND,      DW_SIGNAL_CLICKED },
   { CN_ENTER,        DW_SIGNAL_ITEM_ENTER },
   { CN_CONTEXTMENU,  DW_SIGNAL_ITEM_CONTEXT },
   { LN_SELECT,       DW_SIGNAL_LIST_SELECT },
   { CN_EMPHASIS,     DW_SIGNAL_ITEM_SELECT },
   { WM_SETFOCUS,     DW_SIGNAL_SET_FOCUS },
   { SLN_SLIDERTRACK, DW_SIGNAL_VALUE_CHANGED },
   { BKN_PAGESELECTED,DW_SIGNAL_SWITCH_PAGE },
   { CN_EXPANDTREE,   DW_SIGNAL_TREE_EXPAND }
};

/* Internal function to keep a semi-unique ID within valid range */
USHORT _dw_global_id(void)
{
    static USHORT GlobalID = 9999;

    GlobalID++;
    if(GlobalID >= 65534)
    {
        GlobalID = 10000;
    }
    return GlobalID;
}

/* This function adds a signal handler callback into the linked list.
 */
void _dw_new_signal(ULONG message, HWND window, int id, void *signalfunction, void *discfunc, void *data)
{
   DWSignalHandler *new = malloc(sizeof(DWSignalHandler));

   new->message = message;
   new->window = window;
   new->id = id;
   new->signalfunction = signalfunction;
   new->discfunction = discfunc;
   new->data = data;
   new->next = NULL;

   if (!Root)
      Root = new;
   else
   {
      DWSignalHandler *prev = NULL, *tmp = Root;
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
ULONG _dw_findsigmessage(const char *signame)
{
   int z;

   for(z=0;z<SIGNALMAX;z++)
   {
      if(stricmp(signame, DWSignalTranslate[z].name) == 0)
         return DWSignalTranslate[z].message;
   }
   return 0L;
}

typedef struct _CNRITEM
{
   MINIRECORDCORE rc;
   HPOINTER       hptrIcon;
   PVOID          user;
   HTREEITEM      parent;

} CNRITEM, *PCNRITEM;


int _dw_null_key(HWND DW_UNUSED(window), int DW_UNUSED(key), void * DW_UNUSED(data))
{
   return TRUE;
}

/* Internal function to queue a window redraw */
void _dw_redraw(HWND window, int skip)
{
    static HWND lastwindow = 0;
    HWND redraw = lastwindow;

    if(skip && !window)
        return;

    lastwindow = window;
    if(redraw != lastwindow && redraw)
    {
        dw_window_redraw(redraw);
    }
}

/* Find the desktop window handle */
HWND _dw_toplevel_window(HWND handle)
{
   HWND box, lastbox = WinQueryWindow(handle, QW_PARENT);

   if(lastbox == _dw_desktop)
       return handle;

   /* Find the toplevel window */
   while((box = WinQueryWindow(lastbox, QW_PARENT)) != _dw_desktop && box)
   {
      lastbox = box;
   }
   if(box)
   {
       char tmpbuf[100] = {0};

       WinQueryClassName(lastbox, 99, (PCH)tmpbuf);
       if(strncmp(tmpbuf, "#1", 3) == 0)
           return lastbox;
   }
   return NULLHANDLE;
}


/* Returns height of specified window. */
int _dw_get_height(HWND handle)
{
   unsigned long height;
   dw_window_get_pos_size(handle, NULL, NULL, NULL, &height);
   return (int)height;
}

/* Find the height of the frame a desktop style window is sitting on */
int _dw_get_frame_height(HWND handle)
{
   while(handle)
   {
      HWND client;
      if((client = WinWindowFromID(handle, FID_CLIENT)) != NULLHANDLE)
      {
            return _dw_get_height(WinQueryWindow(handle, QW_PARENT));
      }
        handle = WinQueryWindow(handle, QW_PARENT);
   }
   return dw_screen_height();
}

/* A "safe" WinSendMsg() that tries multiple times in case the
 * queue is blocked for one reason or another.
 */
MRESULT _dw_send_msg(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2, int failure)
{
   MRESULT res;
   int z = 0;

   while((int)(res = WinSendMsg(hwnd, msg, mp1, mp2)) == failure)
   {
      z++;
      if(z > 5000000)
         return (MRESULT)failure;
      dw_main_sleep(1);
   }
   return res;
}

/* Used in the slider and percent classes internally */
unsigned int _dw_percent_get_range(HWND handle)
{
   return SHORT2FROMMP(WinSendMsg(handle, SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE), 0));
}

/* Return the entryfield child of a window */
HWND _dw_find_entryfield(HWND handle)
{
   HENUM henum;
   HWND child, entry = 0;

   henum = WinBeginEnumWindows(handle);
   while((child = WinGetNextWindow(henum)) != NULLHANDLE)
   {
      char tmpbuf[100] = {0};

      WinQueryClassName(child, 99, (PCH)tmpbuf);

      if(strncmp(tmpbuf, "#6", 3)==0)  /* Entryfield */
      {
         entry = child;
         break;
      }
   }
   WinEndEnumWindows(henum);
   return entry;
}

/* This function changes the owner of buttons in to the
 * dynamicwindows handle to fix a problem in notebooks.
 */
void _dw_fix_button_owner(HWND handle, HWND dw)
{
   HENUM henum;
   HWND child;

   henum = WinBeginEnumWindows(handle);
   while((child = WinGetNextWindow(henum)) != NULLHANDLE)
   {
      char tmpbuf[100] = {0};

      WinQueryClassName(child, 99, (PCH)tmpbuf);

      if(strncmp(tmpbuf, "#3", 3)==0 && dw)  /* Button */
         WinSetOwner(child, dw);
      else if(strncmp(tmpbuf, "dynamicwindows", 14) == 0)
         dw = child;

      _dw_fix_button_owner(child, dw);
   }
   WinEndEnumWindows(henum);
   return;
}

/* Free bitmap data associated with a window */
void _dw_free_bitmap(HWND handle)
{
   HBITMAP hbm = (HBITMAP)dw_window_get_data(handle, "_dw_bitmap");
   HPS hps = (HPS)dw_window_get_data(handle, "_dw_hps");
   HDC hdc = (HDC)dw_window_get_data(handle, "_dw_hdc");
   HPIXMAP pixmap = (HPIXMAP)dw_window_get_data(handle, "_dw_hpixmap");
   HPIXMAP disable = (HPIXMAP)dw_window_get_data(handle, "_dw_hpixmap_disabled");
   HPOINTER icon = (HPOINTER)dw_window_get_data(handle, "_dw_button_icon");

   /* For safety purposes, reset all the window data */
   dw_window_set_data(handle, "_dw_bitmap", NULL);
   dw_window_set_data(handle, "_dw_hps", NULL);
   dw_window_set_data(handle, "_dw_hdc", NULL);
   dw_window_set_data(handle, "_dw_hpixmap", NULL);
   dw_window_set_data(handle, "_dw_hpixmap_disabled", NULL);
   dw_window_set_data(handle, "_dw_button_icon", NULL);

   if(icon)
      WinDestroyPointer(icon);

   if(pixmap)
      dw_pixmap_destroy(pixmap);

   if(disable)
      dw_pixmap_destroy(disable);

   if(hps)
   {
      GpiSetBitmap(hps, NULLHANDLE);
      GpiAssociate(hps, NULLHANDLE);
      GpiDestroyPS(hps);
   }

   if(hdc)
      DevCloseDC(hdc);

   if(hbm)
      GpiDeleteBitmap(hbm);
}

/* This function removes any handlers on windows and frees
 * the user memory allocated to it.
 */
void _dw_free_window_memory(HWND handle)
{
   HENUM henum;
   HWND child;
   void *ptr = (void *)WinQueryWindowPtr(handle, QWP_USER);

   dw_signal_disconnect_by_window(handle);

   if((child = WinWindowFromID(handle, FID_MENU)) != NULLHANDLE)
      _dw_free_menu_data(child);

   if((child = WinWindowFromID(handle, FID_CLIENT)) != NULLHANDLE)
   {
      Box *box = (Box *)WinQueryWindowPtr(child, QWP_USER);

      if(box && !dw_window_get_data(handle, "_dw_box"))
      {
         if(box->count && box->items)
            free(box->items);

         WinSetWindowPtr(child, QWP_USER, 0);
         free(box);
      }
   }

   if(ptr)
   {
      WindowData *wd = (WindowData *)ptr;
      char tmpbuf[100] = {0};

      WinQueryClassName(handle, 99, (PCH)tmpbuf);

      if(strncmp(tmpbuf, "ColorSelectClass", 17)!=0)
      {
         /* If this window has an associate bitmap destroy it. */
         _dw_free_bitmap(handle);

         if(strncmp(tmpbuf, "#1", 3)==0 && !WinWindowFromID(handle, FID_CLIENT))
         {
            Box *box = (Box *)ptr;

            if(box->count && box->items)
               free(box->items);
         }
         else if(strncmp(tmpbuf, SplitbarClassName, strlen(SplitbarClassName)+1)==0)
         {
            void *data = dw_window_get_data(handle, "_dw_percent");

            if(data)
               free(data);
         }
         else if(strncmp(tmpbuf, "#37", 4)==0)
         {
            char *coltitle = (char *)dw_window_get_data(handle, "_dw_coltitle");
            PFIELDINFO first;

            dw_container_clear(handle, FALSE);
            if(wd && dw_window_get_data(handle, "_dw_container"))
            {
               void *oldflags = wd->data;
               wd->data = NULL;
               free(oldflags);
            }
            /* Free memory allocated for the container column titles */
            while((first = (PFIELDINFO)WinSendMsg(handle, CM_QUERYDETAILFIELDINFO,  0, MPFROMSHORT(CMA_FIRST))) != NULL)
            {
                if(first->pTitleData)
                    free(first->pTitleData);
                WinSendMsg(handle, CM_REMOVEDETAILFIELDINFO, (MPARAM)&first, MPFROM2SHORT(1, CMA_FREE));
            }
            if(coltitle)
               free(coltitle);
         }

         if(wd->oldproc)
            WinSubclassWindow(handle, wd->oldproc);

         dw_window_set_data(handle, NULL, NULL);
         WinSetWindowPtr(handle, QWP_USER, 0);
         free(ptr);
      }
   }

   henum = WinBeginEnumWindows(handle);
   while((child = WinGetNextWindow(henum)) != NULLHANDLE)
      _dw_free_window_memory(child);

   WinEndEnumWindows(henum);
   return;
}

void _dw_free_menu_data(HWND menu)
{
   int i, count = (int)WinSendMsg(menu, MM_QUERYITEMCOUNT, 0, 0);

   dw_signal_disconnect_by_name(menu, DW_SIGNAL_CLICKED);
   _dw_free_window_memory(menu);

   for(i=0;i<count;i++)
   {
      SHORT menuid = (SHORT)(LONG)WinSendMsg(menu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0);
      MENUITEM mi;

      /* Free the data associated with the ID */
      if(menuid >= 30000)
      {
         char buffer[31] = {0};

         sprintf(buffer, "_dw_id%d", menuid);
         dw_window_set_data( _dw_app, buffer, NULL );
         sprintf(buffer, "_dw_checkable%d", menuid);
         dw_window_set_data( _dw_app, buffer, NULL );
         sprintf(buffer, "_dw_ischecked%d", menuid);
         dw_window_set_data( _dw_app, buffer, NULL );
         sprintf(buffer, "_dw_isdisabled%d", menuid);
         dw_window_set_data( _dw_app, buffer, NULL );
      }

      /* Check any submenus */
      if(WinSendMsg(menu, MM_QUERYITEM, MPFROMSHORT(menuid), MPFROMP(&mi))
         && mi.hwndSubMenu)
         _dw_free_menu_data(mi.hwndSubMenu);
   }
}

/* This function returns 1 if the window (widget) handle
 * passed to it is a valid window that can gain input focus.
 */
int _dw_validate_focus(HWND handle)
{
   char tmpbuf[100] = {0};

   if(!handle)
      return 0;

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

   if(!WinIsWindowEnabled(handle) ||
      (strncmp(tmpbuf, "ColorSelectClass", 17) && dw_window_get_data(handle, "_dw_disabled")))
      return 0;

   /* These are the window classes which can
    * obtain input focus.
    */
   if(strncmp(tmpbuf, "#2", 3)==0 ||  /* Combobox */
      strncmp(tmpbuf, "#3", 3)==0 ||  /* Button */
      strncmp(tmpbuf, "#6", 3)==0 ||  /* Entryfield */
      strncmp(tmpbuf, "#7", 3)==0 ||  /* List box */
      strncmp(tmpbuf, "#10", 4)==0 || /* MLE */
      strncmp(tmpbuf, "#32", 4)==0 || /* Spinbutton */
      strncmp(tmpbuf, "#37", 4)==0 || /* Container */
      strncmp(tmpbuf, "#38", 4)== 0)  /* Slider */
      return 1;
   if(strncmp(tmpbuf, "#40", 4)==0)   /* Notebook */
      return 2;
   return 0;
}

#define _DW_DIRECTION_FORWARD -1
#define _DW_DIRECTION_BACKWARD 1

int _dw_focus_check_box(Box *box, HWND handle, int start, int direction, HWND defaultitem);

/* Internal comparision function */
int _dw_focus_comp(int direction, int z, int end)
{
   if(direction == _DW_DIRECTION_FORWARD)
      return z > -1;
   return z < end;
}
int _dw_focus_notebook(HWND hwnd, HWND handle, int start, int direction, HWND defaultitem)
{
   Box *notebox;
   HWND page = (HWND)WinSendMsg(hwnd, BKM_QUERYPAGEWINDOWHWND,
                         (MPARAM)dw_notebook_page_get(hwnd), 0);

   if(page)
   {
      notebox = (Box *)WinQueryWindowPtr(page, QWP_USER);

      if(notebox && _dw_focus_check_box(notebox, handle, start == 3 ? 3 : 0, direction, defaultitem))
         return 1;
   }
   return 0;
}

int _dw_focus_check_box(Box *box, HWND handle, int start, int direction, HWND defaultitem)
{
   int z;
   static HWND lasthwnd, firsthwnd;
   static int finish_searching;
   int beg = (direction == _DW_DIRECTION_FORWARD) ? box->count-1 : 0;
   int end = (direction == _DW_DIRECTION_FORWARD) ? -1 : box->count;

   /* Start is 2 when we have cycled completely and
    * need to set the focus to the last widget we found
    * that was valid.
    */
   if(start == 2)
   {
      if(lasthwnd)
         WinSetFocus(HWND_DESKTOP, lasthwnd);
      return 0;
   }

   /* Start is 1 when we are entering the function
    * for the first time, it is zero when entering
    * the function recursively.
    */
   if(start == 1)
   {
      lasthwnd = handle;
      finish_searching = 0;
      firsthwnd = 0;
   }

   for(z=beg;_dw_focus_comp(direction, z, end);z+=direction)
   {
      if(box->items[z].type == _DW_TYPE_BOX)
      {
         Box *thisbox = WinQueryWindowPtr(box->items[z].hwnd, QWP_USER);

         if(thisbox && _dw_focus_check_box(thisbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
            return 1;
      }
      else
      {
         int type = _dw_validate_focus(box->items[z].hwnd);

         /* Special case notebook, can focus and contains items */
         if(type == 2 && direction == _DW_DIRECTION_FORWARD && _dw_focus_notebook(box->items[z].hwnd, handle, start, direction, defaultitem))
            return 1;
         if(box->items[z].hwnd == handle)
         {
            if(lasthwnd == handle && firsthwnd)
               WinSetFocus(HWND_DESKTOP, firsthwnd);
            else if(lasthwnd == handle && !firsthwnd)
               finish_searching = 1;
            else
               WinSetFocus(HWND_DESKTOP, lasthwnd);

            /* If we aren't looking for the last handle,
             * return immediately.
             */
            if(!finish_searching)
               return 1;
         }
         if(_dw_validate_focus(box->items[z].hwnd))
         {
            /* Start is 3 when we are looking for the
             * first valid item in the layout.
             */
            if(start == 3)
            {
               if(!defaultitem || (defaultitem && defaultitem == box->items[z].hwnd))
               {
                  WinSetFocus(HWND_DESKTOP, box->items[z].hwnd);
                  return 1;
               }
            }

            lasthwnd = box->items[z].hwnd;

            if(!firsthwnd)
               firsthwnd = lasthwnd;
         }
         else
         {
            char tmpbuf[100] = {0};

            WinQueryClassName(box->items[z].hwnd, 99, (PCH)tmpbuf);
            if(strncmp(tmpbuf, SplitbarClassName, strlen(SplitbarClassName)+1)==0)
            {
               /* Then try the bottom or right box */
                HWND mybox = (HWND)dw_window_get_data(box->items[z].hwnd, (direction == _DW_DIRECTION_FORWARD) ? "_dw_bottomright" : "_dw_topleft");

               if(mybox)
               {
                  Box *splitbox = (Box *)WinQueryWindowPtr(mybox, QWP_USER);

                  if(splitbox && _dw_focus_check_box(splitbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                     return 1;
               }

               /* Try the top or left box */
               mybox = (HWND)dw_window_get_data(box->items[z].hwnd, (direction == _DW_DIRECTION_FORWARD) ? "_dw_topleft" : "_dw_bottomright");

               if(mybox)
               {
                  Box *splitbox = (Box *)WinQueryWindowPtr(mybox, QWP_USER);

                  if(splitbox && _dw_focus_check_box(splitbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                     return 1;
               }
            }
            else if(strncmp(tmpbuf, ScrollClassName, strlen(ScrollClassName)+1)==0) /* Scrollbox */
            {
                /* Get the box window handle */
                HWND mybox = (HWND)dw_window_get_data(box->items[z].hwnd, "_dw_box");

                if(mybox)
                {
                    Box *scrollbox = (Box *)WinQueryWindowPtr(mybox, QWP_USER);

                    if(scrollbox && _dw_focus_check_box(scrollbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                        return 1;
                }
            }
         }
         /* Special case notebook, can focus and contains items */
         if(type == 2 && direction == _DW_DIRECTION_BACKWARD && _dw_focus_notebook(box->items[z].hwnd, handle, start, direction, defaultitem))
            return 1;
      }
   }
   return 0;
}

/* This function finds the first widget in the
 * layout and moves the current focus to it.
 */
int _dw_initial_focus(HWND handle)
{
   Box *thisbox = NULL;
   HWND box;

   box = WinWindowFromID(handle, FID_CLIENT);
   if(box)
      thisbox = WinQueryWindowPtr(box, QWP_USER);
   else
      return 1;

   if(thisbox)
      _dw_focus_check_box(thisbox, handle, 3, _DW_DIRECTION_FORWARD, thisbox->defaultitem);
   return 0;
}

/* This function finds the current widget in the
 * layout and moves the current focus to the next item.
 */
void _dw_shift_focus(HWND handle, int direction)
{
   Box *thisbox;
   HWND box, lastbox = _dw_toplevel_window(handle);

   box = WinWindowFromID(lastbox, FID_CLIENT);
   if(box)
      thisbox = WinQueryWindowPtr(box, QWP_USER);
   else
      thisbox = WinQueryWindowPtr(lastbox, QWP_USER);

   if(thisbox)
   {
      if(_dw_focus_check_box(thisbox, handle, 1, direction, 0)  == 0)
         _dw_focus_check_box(thisbox, handle, 2, direction, 0);
   }
}

/* This function will recursively search a box and add up the total height of it */
void _dw_count_size(HWND box, int type, int *xsize, int *xorigsize)
{
   int size = 0, origsize = 0, z;
   Box *tmp = WinQueryWindowPtr(box, QWP_USER);

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
         if(tmp->items[z].type == _DW_TYPE_BOX)
         {
            int s, os;

            _dw_count_size(tmp->items[z].hwnd, type, &s, &os);
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
         if(tmp->items[z].type == _DW_TYPE_BOX)
            _dw_count_size(tmp->items[z].hwnd, type, &tmpsize, &tmporigsize);
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


/* Function: TrackRectangle
 * Abstract: Tracks given rectangle.
 *
 * If rclBounds is NULL, then track rectangle on entire desktop.
 * rclTrack is in window coorditates and will be mapped to
 * desktop.
 */

BOOL _dw_track_rectangle(HWND hwndBase, RECTL* rclTrack, RECTL* rclBounds)
{
   TRACKINFO track;
   APIRET rc;

   track.cxBorder = 1;
   track.cyBorder = 1;
   track.cxGrid   = 1;
   track.cyGrid   = 1;
   track.cxKeyboard = 8;
   track.cyKeyboard = 8;

   if(!rclTrack)
      return FALSE;

   if(rclBounds)
   {
      track.rclBoundary = *rclBounds;
   }
   else
   {
      track.rclBoundary.yTop    =
         track.rclBoundary.xRight  = 3000;
      track.rclBoundary.yBottom =
         track.rclBoundary.xLeft   = -3000;
   }

   track.rclTrack = *rclTrack;

   WinMapWindowPoints(hwndBase,
                  HWND_DESKTOP,
                  (PPOINTL)&track.rclTrack,
                  2);

   track.ptlMinTrackSize.x = track.rclTrack.xRight
      - track.rclTrack.xLeft;
   track.ptlMinTrackSize.y = track.rclTrack.yTop
      - track.rclTrack.yBottom;
   track.ptlMaxTrackSize.x = track.rclTrack.xRight
      - track.rclTrack.xLeft;
   track.ptlMaxTrackSize.y = track.rclTrack.yTop
      - track.rclTrack.yBottom;

   track.fs = TF_MOVE | TF_ALLINBOUNDARY;

   rc = WinTrackRect(HWND_DESKTOP, 0, &track);

   if(rc)
      *rclTrack = track.rclTrack;

   return rc;
}

void _dw_check_resize_notebook(HWND hwnd)
{
   char tmpbuf[100] = {0};

   WinQueryClassName(hwnd, 99, (PCH)tmpbuf);

   /* If we have a notebook we resize the page again. */
   if(strncmp(tmpbuf, "#40", 4)==0)
   {
      long x, y;
      unsigned long width, height;
      ULONG page = (ULONG)WinSendMsg(hwnd, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_FIRST, BKA_MAJOR));

      while(page)
      {
         HWND pagehwnd = (HWND)WinSendMsg(hwnd, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(page), 0);
         RECTL rc;

         Box *pagebox = (Box *)WinQueryWindowPtr(pagehwnd, QWP_USER);
         if(pagebox)
         {
            dw_window_get_pos_size(hwnd, &x, &y, &width, &height);

            rc.xLeft = x;
            rc.yBottom = y;
            rc.xRight = x + width;
            rc.yTop = y + height;

            WinSendMsg(hwnd, BKM_CALCPAGERECT, (MPARAM)&rc, (MPARAM)TRUE);

            _dw_do_resize(pagebox, rc.xRight - rc.xLeft, rc.yTop - rc.yBottom);
         }
         page = (ULONG)WinSendMsg(hwnd, BKM_QUERYPAGEID, (MPARAM)page, MPFROM2SHORT(BKA_NEXT, BKA_MAJOR));
      }

   }
}

/* Return the OS/2 color from the DW color */
unsigned long _dw_internal_color(unsigned long color)
{
   if(color < 16)
      return _dw_colors[color];
   return color;
}

unsigned long _dw_os2_color(unsigned long color)
{
   return DW_RED_VALUE(color) << 16 | DW_GREEN_VALUE(color) << 8 | DW_BLUE_VALUE(color);
}

BOOL _dw_set_window_pos(HWND hwnd, HWND parent, HWND behind, LONG x, LONG y, LONG cx, LONG cy, ULONG fl)
{
   int height = _dw_get_height(parent);

   return WinSetWindowPos(hwnd, behind, x, height - y - cy, cx, cy, fl);
}

#ifdef UNICODE
#define MAX_CP_NAME     12      /* maximum length of a codepage name */
#define MAX_CP_SPEC     64      /* maximum length of a UconvObject codepage specifier */

char *_dw_WideToUTF8(UniChar *unistr)
{
    /* Convert text to UTF-8 codepage */
    char *retval = NULL;
    /* Now do the conversion */
    ULONG ulBufLen = (UniStrlen(unistr) * 4) + 1;
    char *s, *pszLocalText = (char *)malloc(ulBufLen);

    if(UniStrFromUcs(Uconv, pszLocalText,
                     unistr, ulBufLen) == ULS_SUCCESS)
    {
        /* (some codepages use 0x1A for substitutions; replace with ?) */
        while((s = strchr(pszLocalText, 0x1A)) != NULL) *s = '?';
        /* Output the converted text */
        retval = pszLocalText;
    }
    else if(pszLocalText)
        free(pszLocalText);
    return retval;
}

UniChar *_dw_UTF8toWide(char *utf8str)
{
    /* Convert text to Unicode */
    UniChar *retval = NULL;
    /* Now do the conversion */
    UniChar *buf = calloc(strlen(utf8str) + 1, sizeof(UniChar));

    if(UniStrToUcs(Uconv, buf,
                   utf8str, strlen(utf8str) * sizeof(UniChar)) == ULS_SUCCESS)
    {
        /* Output the converted text */
        retval = buf;
    }
    else if(buf)
        free(buf);
    return retval;
}
#endif

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

   if(thisbox->grouphwnd)
   {
      /* Only calculate the size on the first pass...
       * use the cached values on second.
       */
      if(pass == 1)
      {
         char *text = dw_window_get_text(thisbox->grouphwnd);

         thisbox->grouppady = 9;

         if(text)
         {
            if(*text)
               dw_font_text_extents_get(thisbox->grouphwnd, 0, text, NULL, &thisbox->grouppady);
            dw_free(text);
         }
         /* If the string height is less than 9...
          * set it to 9 anyway since that is the minimum.
          */
         if(thisbox->grouppady < 9)
            thisbox->grouppady = 9;

         if(thisbox->grouppady)
            thisbox->grouppady += 3;
         else
            thisbox->grouppady = 6;

         thisbox->grouppadx = 6;
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

      if(thisbox->items[z].type == _DW_TYPE_BOX)
      {
         Box *tmp = WinQueryWindowPtr(thisbox->items[z].hwnd, QWP_USER);

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

   /* Move the groupbox start past the group border */
   if(thisbox->grouphwnd)
   {
      currentx += 3;
      currenty += thisbox->grouppady - 3;
   }

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
         if(width > 0 && height > 0)
         {
            int pad = thisbox->items[z].pad;
            HWND handle = thisbox->items[z].hwnd;
            char tmpbuf[100] = {0};

            WinQueryClassName(handle, 99, (PCH)tmpbuf);

            if(strncmp(tmpbuf, "#2", 3)==0)
            {
               HWND frame = (HWND)dw_window_get_data(handle, "_dw_combo_box");
               /* Make the combobox big enough to drop down. :) */
               WinSetWindowPos(handle, HWND_TOP, 0, -100,
                           width, height + 100, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
               _dw_set_window_pos(frame, thisbox->hwnd, HWND_TOP, currentx + pad, currenty + pad,
                           width, height, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
            }
            else if(strncmp(tmpbuf, "#6", 3)==0)
            {
               /* Entryfields on OS/2 have a thick border that isn't on Windows and GTK */
               _dw_set_window_pos(handle, thisbox->hwnd, HWND_TOP, (currentx + pad) + 3, (currenty + pad) + 3,
                           width - 6, height - 6, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
            }
            else if(strncmp(tmpbuf, "#40", 5)==0)
            {
               _dw_set_window_pos(handle, thisbox->hwnd, HWND_TOP, currentx + pad, currenty + pad,
                           width, height, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
               _dw_check_resize_notebook(handle);
            }
            else if(strncmp(tmpbuf, ScrollClassName, strlen(ScrollClassName)+1)==0)
            {
                /* Handle special case of scrollbox */
                int cx, cy, depth = 0;
                HWND box = (HWND)dw_window_get_data(handle, "_dw_resizebox");
                HWND client = WinWindowFromID(handle, FID_CLIENT);
                HWND vscroll = WinWindowFromID(handle, FID_VERTSCROLL);
                HWND hscroll = WinWindowFromID(handle, FID_HORZSCROLL);
                Box *contentbox = (Box *)WinQueryWindowPtr(box, QWP_USER);
                int origx, origy;
                unsigned int hpos = (unsigned int)WinSendMsg(hscroll, SBM_QUERYPOS, 0, 0);
                unsigned int vpos = (unsigned int)WinSendMsg(vscroll, SBM_QUERYPOS, 0, 0);

                /* Position the scrollbox parts */
                _dw_set_window_pos(handle, thisbox->hwnd, HWND_TOP, currentx + pad, currenty + pad, width, height, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
                WinSetWindowPos(client, HWND_TOP, 0, WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL), width - WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL), height - WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL), SWP_MOVE | SWP_SIZE | SWP_ZORDER);
                WinSetWindowPos(hscroll, HWND_TOP, 0, 0, width - WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL), WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL), SWP_MOVE | SWP_SIZE | SWP_ZORDER);
                WinSetWindowPos(vscroll, HWND_TOP, width - WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL), WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL), WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL), height - WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL), SWP_MOVE | SWP_SIZE | SWP_ZORDER);

                origx = cx = width - WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
                origy = cy = height - WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);

                /* Get the required space for the box */
                _dw_resize_box(contentbox, &depth, cx, cy, 1);

                if(cx < contentbox->minwidth)
                {
                    cx = contentbox->minwidth;
                }
                if(cy < contentbox->minheight)
                {
                    cy = contentbox->minheight;
                }

                /* Setup vertical scroller */
                WinSendMsg(vscroll, SBM_SETSCROLLBAR, (MPARAM)vpos, MPFROM2SHORT(0, (unsigned short)contentbox->minheight - origy));
                WinSendMsg(vscroll, SBM_SETTHUMBSIZE, MPFROM2SHORT((unsigned short)origy, contentbox->minheight), 0);
                if(vpos > contentbox->minheight)
                {
                    vpos = contentbox->minheight;
                    WinSendMsg(vscroll, SBM_SETPOS, (MPARAM)vpos, 0);
                }

                /* Setup horizontal scroller */
                WinSendMsg(hscroll, SBM_SETSCROLLBAR, (MPARAM)hpos, MPFROM2SHORT(0, (unsigned short)contentbox->minwidth - origx));
                WinSendMsg(hscroll, SBM_SETTHUMBSIZE, MPFROM2SHORT((unsigned short)origx, contentbox->minwidth), 0);
                if(hpos > contentbox->minwidth)
                {
                    hpos = contentbox->minwidth;
                    WinSendMsg(hscroll, SBM_SETPOS, (MPARAM)hpos, 0);
                }

                /* Position the scrolled box */
                WinSetWindowPos(box, HWND_TOP, -hpos, -(cy - origy - vpos), cx, cy, SWP_MOVE | SWP_SIZE | SWP_ZORDER);

                dw_window_set_data(handle, "_dw_cy", (void *)(cy - origy));

                /* Layout the content of the scrollbox */
                _dw_do_resize(contentbox, cx, cy);
            }
            else if(strncmp(tmpbuf, SplitbarClassName, strlen(SplitbarClassName)+1)==0)
            {
               /* Then try the bottom or right box */
               float *percent = (float *)dw_window_get_data(handle, "_dw_percent");
               int type = (int)dw_window_get_data(handle, "_dw_type");

               _dw_set_window_pos(handle, thisbox->hwnd, HWND_TOP, currentx + pad, currenty + pad,
                           width, height, SWP_MOVE | SWP_SIZE | SWP_ZORDER);

               if(percent)
                  _dw_handle_splitbar_resize(handle, *percent, type, width, height);
            }
            else
            {
               /* Everything else */
               _dw_set_window_pos(handle, thisbox->hwnd, HWND_TOP, currentx + pad, currenty + pad,
                           width, height, SWP_MOVE | SWP_SIZE | SWP_ZORDER);

               /* After placing a box... place its components */
               if(thisbox->items[z].type == _DW_TYPE_BOX)
               {
                  Box *boxinfo = WinQueryWindowPtr(handle, QWP_USER);

                  if(boxinfo)
                  {
                     if(boxinfo->grouphwnd)
                     {
                        /* Move the group border into place */
                        WinSetWindowPos(boxinfo->grouphwnd, HWND_TOP, 0, 0,
                                       width, height, SWP_MOVE | SWP_SIZE);
                     }
                     /* Dive into the box */
                     (*depth)++;
                     _dw_resize_box(boxinfo, depth, width, height, pass);
                     (*depth)--;
                  }
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

void _dw_do_resize(Box *thisbox, int x, int y)
{
   if(x != 0 && y != 0)
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

/* This procedure handles WM_QUERYTRACKINFO requests from the frame */
MRESULT EXPENTRY _dw_sizeproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   PFNWP *blah = WinQueryWindowPtr(hWnd, QWP_USER);
   Box *thisbox = NULL;
   HWND box;

   box = WinWindowFromID(hWnd, FID_CLIENT);
   if(box)
      thisbox = WinQueryWindowPtr(box, QWP_USER);

   if(thisbox && !thisbox->titlebar)
   {
      switch(msg)
      {
      case WM_QUERYTRACKINFO:
         {
            if(blah && *blah)
            {
               PTRACKINFO ptInfo;
               int res;
               PFNWP myfunc = *blah;
               res = (int)myfunc(hWnd, msg, mp1, mp2);

               ptInfo = (PTRACKINFO)(mp2);

               ptInfo->ptlMinTrackSize.y = 8;
               ptInfo->ptlMinTrackSize.x = 8;

               return (MRESULT)res;
            }
         }
      }
   }

   if(blah && *blah)
   {
      PFNWP myfunc = *blah;
      return myfunc(hWnd, msg, mp1, mp2);
   }

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

void _dw_top(HPS hpsPaint, RECTL rclPaint)
{
   POINTL ptl1, ptl2;

   ptl1.x = rclPaint.xLeft;
   ptl2.y = ptl1.y = rclPaint.yTop - 1;
   ptl2.x = rclPaint.xRight - 1;
   GpiMove(hpsPaint, &ptl1);
   GpiLine(hpsPaint, &ptl2);
}

/* Left hits the bottom */
void _dw_left(HPS hpsPaint, RECTL rclPaint)
{
   POINTL ptl1, ptl2;

   ptl2.x = ptl1.x = rclPaint.xLeft;
   ptl1.y = rclPaint.yTop - 1;
   ptl2.y = rclPaint.yBottom;
   GpiMove(hpsPaint, &ptl1);
   GpiLine(hpsPaint, &ptl2);
}

void _dw_bottom(HPS hpsPaint, RECTL rclPaint)
{
   POINTL ptl1, ptl2;

   ptl1.x = rclPaint.xRight - 1;
   ptl1.y = ptl2.y = rclPaint.yBottom;
   ptl2.x = rclPaint.xLeft;
   GpiMove(hpsPaint, &ptl1);
   GpiLine(hpsPaint, &ptl2);
}

/* Right hits the top */
void _dw_right(HPS hpsPaint, RECTL rclPaint)
{
   POINTL ptl1, ptl2;

   ptl2.x = ptl1.x = rclPaint.xRight - 1;
   ptl1.y = rclPaint.yBottom + 1;
   ptl2.y = rclPaint.yTop - 1;
   GpiMove(hpsPaint, &ptl1);
   GpiLine(hpsPaint, &ptl2);
}

void _dw_drawtext(HWND hWnd, HPS hpsPaint)
{
    RECTL rclPaint;
    int len = WinQueryWindowTextLength(hWnd);
    ULONG style = WinQueryWindowULong(hWnd, QWL_STYLE) & (DT_TOP|DT_VCENTER|DT_BOTTOM|DT_LEFT|DT_CENTER|DT_RIGHT|DT_WORDBREAK);
    char *tempbuf = alloca(len + 2);
    ULONG fcolor = DT_TEXTATTRS, bcolor = DT_TEXTATTRS;

    WinQueryWindowText(hWnd, len + 1, (PSZ)tempbuf);
    WinQueryWindowRect(hWnd, &rclPaint);

    if(WinQueryPresParam(hWnd, PP_BACKGROUNDCOLOR, 0, NULL, sizeof(bcolor), &bcolor, QPF_NOINHERIT) ||
       WinQueryPresParam(hWnd, PP_BACKGROUNDCOLORINDEX, 0, NULL, sizeof(bcolor), &bcolor, QPF_NOINHERIT))
        GpiSetBackColor(hpsPaint, bcolor);
    if(WinQueryPresParam(hWnd, PP_FOREGROUNDCOLOR, 0, NULL, sizeof(fcolor), &fcolor, QPF_NOINHERIT) ||
       WinQueryPresParam(hWnd, PP_FOREGROUNDCOLORINDEX, 0, NULL, sizeof(fcolor), &fcolor, QPF_NOINHERIT))
        GpiSetColor(hpsPaint, fcolor);
    if(style & DT_WORDBREAK)
    {
        int thisheight;
        LONG drawn, totaldrawn = 0;

        dw_font_text_extents_get(hWnd, NULL, tempbuf, NULL, &thisheight);

        /* until all chars drawn */
        for(; totaldrawn !=  len; rclPaint.yTop -= thisheight)
        {
            /* draw the text */
            drawn = WinDrawText(hpsPaint, len -  totaldrawn, (PCH)tempbuf +  totaldrawn,
                                &rclPaint, DT_TEXTATTRS, DT_TEXTATTRS, style | DT_TEXTATTRS | DT_ERASERECT);
            if(drawn)
                totaldrawn += drawn;
            else
                break;
        }
    }
    else
        WinDrawText(hpsPaint, -1, (PCH)tempbuf, &rclPaint, DT_TEXTATTRS, DT_TEXTATTRS, style | DT_TEXTATTRS | DT_ERASERECT);
}

/* Function: BubbleProc
 * Abstract: Subclass procedure for bubble help
 */
MRESULT EXPENTRY _dw_bubbleproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   PFNWP proc = (PFNWP)WinQueryWindowPtr(hwnd, QWL_USER);

   if(msg == WM_PAINT)
   {
      POINTL ptl;
      HPS hpsTemp;
      RECTL rcl;
      int height, width;

      WinQueryWindowRect(hwnd, &rcl);
      height = rcl.yTop - rcl.yBottom - 1;
      width = rcl.xRight - rcl.xLeft - 1;

      /* Draw a border around the bubble help */
      hpsTemp = WinBeginPaint(hwnd, 0, 0);

      _dw_drawtext(hwnd, hpsTemp);
      GpiSetColor(hpsTemp, CLR_BLACK);
      ptl.x = ptl.y = 0;
      GpiMove(hpsTemp, &ptl);
      ptl.x = 0;
      ptl.y = height;
      GpiLine(hpsTemp, &ptl);
      ptl.x = ptl.y = 0;
      GpiMove(hpsTemp, &ptl);
      ptl.y = 0;
      ptl.x = width;
      GpiLine(hpsTemp, &ptl);
      ptl.x = width;
      ptl.y = height;
      GpiMove(hpsTemp, &ptl);
      ptl.x = 0;
      ptl.y = height;
      GpiLine(hpsTemp, &ptl);
      ptl.x = width;
      ptl.y = height;
      GpiMove(hpsTemp, &ptl);
      ptl.y = 0;
      ptl.x = width;
      GpiLine(hpsTemp, &ptl);
      WinEndPaint(hpsTemp);
      return (MRESULT)TRUE;
   }
   if(proc)
      return proc(hwnd, msg, mp1, mp2);
   return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

/* Subclass WC_STATIC to draw a bitmap centered */
MRESULT EXPENTRY _dw_bitmapproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    WindowData *blah = (WindowData *)WinQueryWindowPtr(hwnd, QWL_USER);

    if(msg == WM_PAINT)
    {
        HPS hps = WinBeginPaint(hwnd, 0, 0);
        HBITMAP hbm = (HBITMAP)dw_window_get_data(hwnd, "_dw_bitmap");
        RECTL rcl;

        WinQueryWindowRect(hwnd, &rcl) ;
        WinFillRect(hps, &rcl, CLR_PALEGRAY);

        /* If we have a bitmap... draw it */
        if(hbm)
        {
            BITMAPINFOHEADER sl;

            sl.cbFix = sizeof(BITMAPINFOHEADER);

            /* Check the bitmap size */
            if(GpiQueryBitmapParameters(hbm, &sl))
            {
               /* Figure out the window size before clobbering the data */
                int width = rcl.xRight - rcl.xLeft, height = rcl.yTop - rcl.yBottom;

                /* If the control is bigger than the bitmap, center it */
               if(width > sl.cx)
                   rcl.xLeft = (width-sl.cx)/2;
               if(height > sl.cy)
                   rcl.yBottom = (height-sl.cy)/2;

           }
            /* Draw the bitmap unscaled at the desired location */
            WinDrawBitmap(hps, hbm, NULL, (PPOINTL) &rcl,
                          CLR_NEUTRAL, CLR_BACKGROUND, DBM_NORMAL);
        }

       WinEndPaint(hps);
       return 0;
    }
    if(blah && blah->oldproc)
        return blah->oldproc(hwnd, msg, mp1, mp2);
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

/* Function to handle tooltip messages from a variety of procedures */
MRESULT EXPENTRY _dw_tooltipproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2, WindowData *blah)
{
    static HWND hstart, hend;

    switch(msg)
    {
    case 0x041f:
        /* Mouse has left the area.. remove tooltip and stop timer */
        if(_dw_bubble)
        {
            WinDestroyWindow(_dw_bubble);
            _dw_bubble = 0;
        }
        if(hstart)
            WinStopTimer(dwhab, hstart, 1);
        if(hend)
            WinStopTimer(dwhab, hend, 2);
        hstart = hend = 0;
        break;

    case 0x041e:
        /* Mouse has entered... stop any pending timer...
         * then start a new timer to creat the tooltip delayed.
         */
        if(hstart)
            WinStopTimer(dwhab, hstart, 1);
        /* Two seconds to create */
        WinStartTimer(dwhab, hwnd, 1, 2000);
        hstart = hwnd;
        break;
    case WM_TIMER:
        if((int)mp1 == 1 || (int)mp1 == 2)
        {
            if(_dw_bubble)
            {
                WinDestroyWindow(_dw_bubble);
                _dw_bubble = 0;
            }
            /* Either starting or ending... remove tooltip and timers */
            if(hstart)
                WinStopTimer(dwhab, hstart, 1);
            if(hend)
                WinStopTimer(dwhab, hend, 2);
            hstart = hend = 0;
            /* If we are starting... create a new tooltip */
            if((int)mp1 == 1)
            {
                HPS   hpsTemp = 0;
                LONG  lHight;
                LONG  lWidth;
                POINTL txtPointl[TXTBOX_COUNT];
                POINTL ptlWork = {0,0};
                ULONG ulColor = CLR_YELLOW;
                void *bubbleproc;

                _dw_bubble_last   = hwnd;
                _dw_bubble = WinCreateWindow(HWND_DESKTOP,
                                             WC_STATIC,
                                             NULL,
                                             SS_TEXT |
                                             DT_CENTER |
                                             DT_VCENTER,
                                             0,0,0,0,
                                             HWND_DESKTOP,
                                             HWND_TOP,
                                             0,
                                             NULL,
                                             NULL);

                WinSetPresParam(_dw_bubble,
                                PP_FONTNAMESIZE,
                                strlen(DefaultFont)+1,
                                DefaultFont);


                WinSetPresParam(_dw_bubble,
                                PP_BACKGROUNDCOLORINDEX,
                                sizeof(ulColor),
                                &ulColor);

                WinSetWindowText(_dw_bubble,
                                 (PSZ)blah->bubbletext);

                WinMapWindowPoints(hwnd, HWND_DESKTOP, &ptlWork, 1);

                hpsTemp = WinGetPS(_dw_bubble);
                GpiQueryTextBox(hpsTemp,
                                strlen(blah->bubbletext),
                                (PCH)blah->bubbletext,
                                TXTBOX_COUNT,
                     txtPointl);
                WinReleasePS(hpsTemp);

                lWidth = txtPointl[TXTBOX_TOPRIGHT].x -
                    txtPointl[TXTBOX_TOPLEFT ].x + 8;

                lHight = txtPointl[TXTBOX_TOPLEFT].y -
                    txtPointl[TXTBOX_BOTTOMLEFT].y + 8;

                ptlWork.y -= lHight + 2;

                /* Make sure it is visible on the screen */
                if(ptlWork.x + lWidth > dw_screen_width())
                {
                    ptlWork.x = dw_screen_width() - lWidth;
                    if(ptlWork.x < 0)
                        ptlWork.x = 0;
                }

                bubbleproc = (void *)WinSubclassWindow(_dw_bubble, _dw_bubbleproc);

                if(bubbleproc)
                    WinSetWindowPtr(_dw_bubble, QWP_USER, bubbleproc);

                WinSetWindowPos(_dw_bubble,
                                HWND_TOP,
                                ptlWork.x,
                                ptlWork.y,
                                lWidth,
                                lHight,
                                SWP_SIZE | SWP_MOVE | SWP_SHOW);

                /* Start a timer to remove it after 15 seconds */
                WinStartTimer(dwhab, hwnd, 2, 15000);
                hend = hwnd;
            }
        }
        break;
	}
    return (MRESULT)FALSE;
}

#define CALENDAR_BORDER 3
#define CALENDAR_ARROW 8

/* Returns a rectangle for a single day on the calendar */
RECTL _dw_calendar_day_rect(int position, RECTL rclPaint)
{
    int height = rclPaint.yTop - rclPaint.yBottom - (CALENDAR_BORDER*2);
    int width = rclPaint.xRight - rclPaint.xLeft - (CALENDAR_BORDER*2);
    /* There are 7 rows... 5 for the day numbers...
     * 1 for the Month/Year and 1 for the day names.
     */
    int row = position / 7;
    int col = position % 7;
    int cellwidth = width / 7;
    int cellheight = height / 8;

    /* Create a new box */
    rclPaint.xLeft = (cellwidth * col) + CALENDAR_BORDER;
    rclPaint.xRight = rclPaint.xLeft + cellwidth;
    /* We only handle 6 of the 7 rows */
    rclPaint.yBottom = (cellheight * (6-row)) + CALENDAR_BORDER;
    rclPaint.yTop = rclPaint.yBottom + cellheight;
    return rclPaint;
}

/* These will be filled in during dw_init() */
static char months[12][20];
static char daysofweek[7][20];

/* This procedure handles drawing of a status border */
MRESULT EXPENTRY _dw_calendarproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   /* How many days are in each month usually (not including leap years) */
   static int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
   {
      oldproc = blah->oldproc;

      if(blah->bubbletext[0])
          _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

      switch(msg)
      {
      case WM_BUTTON1DOWN:
      case WM_BUTTON2DOWN:
      case WM_BUTTON3DOWN:
          {
              POINTS pts = (*((POINTS*)&mp1));
              int day = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_day"));
              int month = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_month"));
              int year = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_year"));
              int dayofweek = 1, x, height, isleapyear = ((year+1) % 4 == 0);
              int daysthismonth = days[month] + (isleapyear && month == 1);
              RECTL rclArea;

              /* Figure out what day of the week the first day of the month falls on */
              for(x=0;x<month;x++)
                  dayofweek += days[x] + (isleapyear && x == 1);
              dayofweek += (year/4) + year - 1;
              dayofweek = dayofweek % 7;

              WinQueryWindowRect(hWnd, &rclArea);
              height = ((rclArea.yTop - (CALENDAR_BORDER*2))/7);

              /* Check for the left arrow */
              if(pts.x > rclArea.xLeft + CALENDAR_BORDER && pts.x < rclArea.xLeft + CALENDAR_BORDER + CALENDAR_ARROW &&
                 pts.y > rclArea.yTop - (CALENDAR_BORDER + height) && pts.y < rclArea.yTop - CALENDAR_BORDER)
              {
                  int daysthismonth;

                  if(month == 0)
                  {
                      month = 11;
                      year--;
                      dw_window_set_data(hWnd, "_dw_year", DW_INT_TO_POINTER(year));
                  }
                  else if(month > 0)
                  {
                      month--;
                  }
                  dw_window_set_data(hWnd, "_dw_month", DW_INT_TO_POINTER(month));
                  /* Make sure we aren't higher than the number of days
                   * in our new month... keeping track of leap years.
                   */
                  daysthismonth = days[month] + (isleapyear && month == 1);
                  if(day >= daysthismonth)
                  {
                      day = daysthismonth - 1;
                      dw_window_set_data(hWnd, "_dw_day", DW_INT_TO_POINTER(day));
                  }
                  WinInvalidateRect(hWnd, &rclArea, FALSE);
                  WinPostMsg(hWnd, WM_PAINT, 0, 0);
                  return (MRESULT)TRUE;
              }

              /* Check for the right arrow */
              if(pts.x < rclArea.xRight - CALENDAR_BORDER && pts.x > rclArea.xRight - CALENDAR_BORDER - CALENDAR_ARROW &&
                 pts.y > rclArea.yTop - (CALENDAR_BORDER + height) && pts.y < rclArea.yTop - CALENDAR_BORDER)
              {
                  int daysthismonth;

                  if(month == 11)
                  {
                      month = 0;
                      year++;
                      dw_window_set_data(hWnd, "_dw_year", DW_INT_TO_POINTER(year));
                  }
                  else if(month < 11)
                  {
                      month++;
                  }
                  dw_window_set_data(hWnd, "_dw_month", DW_INT_TO_POINTER(month));
                  /* Make sure we aren't higher than the number of days
                   * in our new month... keeping track of leap years.
                   */
                  daysthismonth = days[month] + (isleapyear && month == 1);
                  if(day >= daysthismonth)
                  {
                      day = daysthismonth - 1;
                      dw_window_set_data(hWnd, "_dw_day", DW_INT_TO_POINTER(day));
                  }
                  WinInvalidateRect(hWnd, &rclArea, FALSE);
                  WinPostMsg(hWnd, WM_PAINT, 0, 0);
                  return (MRESULT)TRUE;
              }

              /* Check all the valid days of the month */
              for(x=dayofweek+7;x<(daysthismonth+dayofweek+7);x++)
              {
                  RECTL rclThis = _dw_calendar_day_rect(x, rclArea);
                  if(pts.x < rclThis.xRight && pts.x > rclThis.xLeft && pts.y < rclThis.yTop && pts.y > rclThis.yBottom)
                  {
                      dw_window_set_data(hWnd, "_dw_day", DW_INT_TO_POINTER((x-(dayofweek+7))));
                      WinInvalidateRect(hWnd, &rclArea, FALSE);
                      WinPostMsg(hWnd, WM_PAINT, 0, 0);
                      return (MRESULT)TRUE;
                  }
              }
          }
          break;
      case WM_PAINT:
         {
            HPS hpsPaint;
            RECTL rclPaint, rclDraw;
            char buf[100];
            int day = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_day"));
            int month = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_month"));
            int year = DW_POINTER_TO_INT(dw_window_get_data(hWnd, "_dw_year"));
            int dayofweek = 1, x, lastmonth = 11, height, isleapyear = ((year+1) % 4 == 0);
            POINTL pptl[3];

            /* Figure out the previous month for later use */
            if(month > 0)
                lastmonth = month - 1;

            /* Make the title */
            sprintf(buf, "%s, %d", months[month], year + 1);

            /* Figure out what day of the week the first day of the month falls on */
            for(x=0;x<month;x++)
                dayofweek += days[x] + (isleapyear && x == 1);
            dayofweek += (year/4) + year - 1;
            dayofweek = dayofweek % 7;

            /* Actually draw the control */
            hpsPaint = WinBeginPaint(hWnd, 0, 0);
            WinQueryWindowRect(hWnd, &rclPaint);
            WinFillRect(hpsPaint, &rclPaint, CLR_PALEGRAY);
            height = ((rclPaint.yTop - (CALENDAR_BORDER*2))/7);

            /* Draw the Month and Year at the top */
            GpiSetColor(hpsPaint, CLR_BLACK);
            rclDraw = rclPaint;
            rclDraw.yBottom = height * 6;
            WinDrawText(hpsPaint, -1, (PCH)buf, &rclDraw, DT_TEXTATTRS, DT_TEXTATTRS, DT_VCENTER | DT_CENTER | DT_TEXTATTRS);

            /* Draw the left arrow */
            GpiSetColor(hpsPaint, CLR_DARKGRAY);
            GpiBeginArea(hpsPaint, 0);
            pptl[2].x = rclDraw.xLeft + CALENDAR_BORDER + CALENDAR_ARROW;
            pptl[2].y = rclDraw.yTop - CALENDAR_BORDER;
            GpiMove(hpsPaint, &pptl[2]);
            pptl[0].x = rclDraw.xLeft + CALENDAR_BORDER;
            pptl[0].y = rclDraw.yTop - (CALENDAR_BORDER+ (height/2));
            pptl[1].x = rclDraw.xLeft + CALENDAR_BORDER + CALENDAR_ARROW;
            pptl[1].y = rclDraw.yTop - CALENDAR_BORDER - height;
            GpiPolyLine(hpsPaint, 3, pptl);
            GpiEndArea(hpsPaint);

            /* Draw the left arrow */
            GpiBeginArea(hpsPaint, 0);
            pptl[2].x = rclDraw.xRight - CALENDAR_BORDER - CALENDAR_ARROW;
            pptl[2].y = rclDraw.yTop - CALENDAR_BORDER;
            GpiMove(hpsPaint, &pptl[2]);
            pptl[0].x = rclDraw.xRight - CALENDAR_BORDER;
            pptl[0].y = rclDraw.yTop - (CALENDAR_BORDER + (height/2));
            pptl[1].x = rclDraw.xRight - CALENDAR_BORDER - CALENDAR_ARROW;
            pptl[1].y = rclDraw.yTop - CALENDAR_BORDER - height;
            GpiPolyLine(hpsPaint, 3, pptl);
            GpiEndArea(hpsPaint);

            /* Draw a border around control */
            _dw_top(hpsPaint, rclPaint);
            _dw_left(hpsPaint, rclPaint);
            /* With shadow */
            GpiSetColor(hpsPaint, CLR_WHITE);
            _dw_right(hpsPaint, rclPaint);
            _dw_bottom(hpsPaint, rclPaint);

            /* Draw the days of the week */
            GpiSetColor(hpsPaint, CLR_BLACK);
            for(x=0;x<7;x++)
            {
                char *title = daysofweek[x];

                rclDraw = _dw_calendar_day_rect(x, rclPaint);

                if(rclDraw.xRight - rclDraw.xLeft < 60)
                {
                    buf[0] = daysofweek[x][0]; buf[1] = daysofweek[x][1]; buf[2] = 0;
                    title = buf;
                }
                WinDrawText(hpsPaint, -1, (PCH)title, &rclDraw, DT_TEXTATTRS, DT_TEXTATTRS, DT_VCENTER | DT_CENTER | DT_TEXTATTRS);
            }

            /* Go through all the days */
            for(x=0;x<42;x++)
            {
                int daysthismonth = days[month] + (isleapyear && month == 1);
                int dayslastmonth = days[lastmonth] + (isleapyear && lastmonth == 1);

                rclDraw = _dw_calendar_day_rect(x+7, rclPaint);
                if(x < dayofweek)
                {
                    GpiSetColor(hpsPaint, CLR_DARKGRAY);
                    sprintf(buf, "%d", dayslastmonth - (dayofweek - x - 1));
                }
                else if(x - dayofweek + 1 > daysthismonth)
                {
                    GpiSetColor(hpsPaint, CLR_DARKGRAY);
                    sprintf(buf, "%d", x - dayofweek - daysthismonth + 1);
                }
                else
                {
                    GpiSetColor(hpsPaint, CLR_DARKBLUE);
                    sprintf(buf, "%d", x - dayofweek + 1);
                }
                WinDrawText(hpsPaint, -1, (PCH)buf, &rclDraw, DT_TEXTATTRS, DT_TEXTATTRS, DT_VCENTER | DT_CENTER | DT_TEXTATTRS);
            }

            /* Draw a border around selected day */
            rclPaint = _dw_calendar_day_rect(day + dayofweek + 7, rclPaint);
            GpiSetColor(hpsPaint, CLR_DARKGRAY);
            _dw_top(hpsPaint, rclPaint);
            _dw_left(hpsPaint, rclPaint);
            /* With shadow */
            GpiSetColor(hpsPaint, CLR_WHITE);
            _dw_right(hpsPaint, rclPaint);
            _dw_bottom(hpsPaint, rclPaint);

            WinEndPaint(hpsPaint);

            return (MRESULT)TRUE;
         }
      }
      if(oldproc)
          return oldproc(hWnd, msg, mp1, mp2);
   }

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}


/* This procedure handles drawing of a status border */
MRESULT EXPENTRY _dw_statusproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;

   if(msg == WM_MOUSEMOVE && _dw_wndproc(hWnd, msg, mp1, mp2))
      return MPFROMSHORT(FALSE);

   if(blah)
   {
      oldproc = blah->oldproc;

      if(blah->bubbletext[0])
          _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

      if(blah->bubbletext[0])
          _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

      switch(msg)
      {
      case WM_PAINT:
         {
            HPS hpsPaint;
            RECTL rclPaint;
            char buf[1024];

            hpsPaint = WinBeginPaint(hWnd, 0, 0);
            WinQueryWindowRect(hWnd, &rclPaint);
            WinFillRect(hpsPaint, &rclPaint, CLR_PALEGRAY);

            GpiSetColor(hpsPaint, CLR_DARKGRAY);
            _dw_top(hpsPaint, rclPaint);
            _dw_left(hpsPaint, rclPaint);

            GpiSetColor(hpsPaint, CLR_WHITE);
            _dw_right(hpsPaint, rclPaint);
            _dw_bottom(hpsPaint, rclPaint);

            WinQueryWindowText(hWnd, 1024, (PSZ)buf);
            rclPaint.xLeft += 3;
            rclPaint.xRight--;
            rclPaint.yTop--;
            rclPaint.yBottom++;

            GpiSetColor(hpsPaint, CLR_BLACK);
            WinDrawText(hpsPaint, -1, (PCH)buf, &rclPaint, DT_TEXTATTRS, DT_TEXTATTRS, DT_VCENTER | DT_LEFT | DT_TEXTATTRS);
            WinEndPaint(hpsPaint);

            return (MRESULT)TRUE;
         }
      }
      if(oldproc)
          return oldproc(hWnd, msg, mp1, mp2);
   }

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/* This procedure handles pointer changes */
MRESULT EXPENTRY _dw_textproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   if(msg == WM_MOUSEMOVE &&_dw_wndproc(hWnd, msg, mp1, mp2))
      return MPFROMSHORT(FALSE);

   if(blah && blah->oldproc)
   {
      PFNWP myfunc = blah->oldproc;

      switch(msg)
      {
      case WM_PAINT:
          {
              HPS hpsPaint = WinBeginPaint(hWnd, 0, 0);

              _dw_drawtext(hWnd, hpsPaint);
              WinEndPaint(hpsPaint);
              return (MRESULT)TRUE;
          }
      default:
          return myfunc(hWnd, msg, mp1, mp2);
      }
   }

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/* This procedure handles scrollbox */
MRESULT EXPENTRY _dw_scrollwndproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch(msg)
    {
    case WM_PAINT:
        {
            HPS hpsPaint;
            RECTL rclPaint;

            hpsPaint = WinBeginPaint(hWnd, 0, 0);
            WinQueryWindowRect(hWnd, &rclPaint);
            WinFillRect(hpsPaint, &rclPaint, CLR_PALEGRAY);
            WinEndPaint(hpsPaint);
            break;
        }
    case WM_HSCROLL:
    case WM_VSCROLL:
        {
            MPARAM res;
            int *pos, min, max, page, which = SHORT2FROMMP(mp2);
            HWND handle, client = WinWindowFromID(hWnd, FID_CLIENT);
            HWND box = (HWND)dw_window_get_data(hWnd, "_dw_resizebox");
            HWND hscroll = WinWindowFromID(hWnd, FID_HORZSCROLL);
            HWND vscroll = WinWindowFromID(hWnd, FID_VERTSCROLL);
            int hpos = dw_scrollbar_get_pos(hscroll);
            int vpos = dw_scrollbar_get_pos(vscroll);
            int cy = (int)dw_window_get_data(hWnd, "_dw_cy");
            RECTL rect;

            WinQueryWindowRect(client, &rect);

            if(msg == WM_VSCROLL)
            {
                page = rect.yTop - rect.yBottom;
                handle = vscroll;
                pos = &vpos;
            }
            else
            {
                page = rect.xRight - rect.xLeft;
                handle = hscroll;
                pos = &hpos;
            }

            res = WinSendMsg(handle, SBM_QUERYRANGE, 0, 0);
            min = SHORT1FROMMP(res);
            max = SHORT2FROMMP(res);

            switch(which)
            {
            case SB_SLIDERTRACK:
                *pos = SHORT1FROMMP(mp2);
                break;
            case SB_LINEUP:
                (*pos)--;
                if(*pos < min)
                    *pos = min;
                break;
            case SB_LINEDOWN:
                (*pos)++;
                if(*pos > max)
                    *pos = max;
                break;
            case SB_PAGEUP:
                (*pos) -= page;
                if(*pos < min)
                    *pos = min;
                break;
            case SB_PAGEDOWN:
                (*pos) += page;
                if(*pos > max)
                    *pos = max;
                break;
            }
            WinSendMsg(handle, SBM_SETPOS, (MPARAM)*pos, 0);
            /* Position the scrolled box */
            WinSetWindowPos(box, HWND_TOP, -hpos, -(cy - vpos), 0, 0, SWP_MOVE);
            break;
        }
    }
    return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

void _dw_click_default(HWND handle)
{
   char tmpbuf[100] = {0};

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

   /* These are the window classes which can
    * obtain input focus.
    */
   if(strncmp(tmpbuf, "#3", 3)==0)
   {
      /* Generate click on default item */
      DWSignalHandler *tmp = Root;

      /* Find any callbacks for this function */
      while(tmp)
      {
         if(tmp->message == WM_COMMAND)
         {
            int (API_FUNC clickfunc)(HWND, void *) = (int (API_FUNC)(HWND, void *))tmp->signalfunction;

            /* Make sure it's the right window, and the right ID */
            if(tmp->window == handle)
            {
               clickfunc(tmp->window, tmp->data);
               tmp = NULL;
            }
         }
         if(tmp)
            tmp= tmp->next;
      }
   }
   else
      WinSetFocus(HWND_DESKTOP, handle);
}

#define ENTRY_CUT    60901
#define ENTRY_COPY   60902
#define ENTRY_PASTE  60903
#define ENTRY_DELETE 60904
#define ENTRY_UNDO   60905
#define ENTRY_SALL   60906

#ifdef UNICODE
void _dw_combine_text(HWND handle, USHORT pos1, char *text, char *pastetext)
{
    char *combined = calloc((text ? strlen(text) : 0) + strlen(pastetext) + 1, 1);
    SHORT newsel = pos1 + strlen(pastetext);

    /* Combine the two strings into 1... or just use pastetext if no text */
    if(text)
        strncpy(combined, text, pos1);
    strcat(combined, pastetext);
    if(text && pos1 < strlen(text))
        strcat(combined, &text[pos1]);

    /* Set the new combined text to the entryfield */
    dw_window_set_text(handle, combined);
    /* Move the cursor to the old selection start plus paste length */
    WinSendMsg(handle, EM_SETSEL, MPFROM2SHORT(newsel, newsel), 0);
    /* Free temporary memory */
    free(combined);
}

/* Internal function to handle Unicode enabled MLE cut, copy and paste */
void _dw_mle_copy_paste(HWND hWnd, int command)
{
    /* MLE insertion points (for querying selection) */
    IPT ipt1, ipt2;

    /* Get the selected text */
    ipt1 = (IPT)WinSendMsg(hWnd, MLM_QUERYSEL, MPFROMSHORT(MLFQS_MINSEL), 0);
    ipt2 = (IPT)WinSendMsg(hWnd, MLM_QUERYSEL, MPFROMSHORT(MLFQS_MAXSEL), 0);

    /* Get the selection and put on clipboard for copy and cut */
    if(command != ENTRY_PASTE)
    {
        char *text = (char *)malloc((ULONG)WinSendMsg(hWnd, MLM_QUERYFORMATTEXTLENGTH, MPFROMLONG(ipt1), MPFROMLONG(ipt2 - ipt1)) + 1);
        ULONG ulCopied = (ULONG)WinSendMsg(hWnd, MLM_QUERYSELTEXT, MPFROMP(text), 0);

        dw_clipboard_set_text(text, ulCopied);
        free(text);
    }
    /* Clear selection for cut and paste */
    if(command != ENTRY_COPY)
        WinSendMsg(hWnd, MLM_CLEAR, 0, 0);
    if(command == ENTRY_PASTE)
    {
        char *text = dw_clipboard_get_text();

        if(text)
        {
            WinSendMsg(hWnd, MLM_INSERT, MPFROMP(text), 0);
            dw_free(text);
        }
    }
}

/* Internal function to handle Unicode enabled Entryfield cut, copy and paste */
void _dw_entry_copy_paste(HWND handle, int command)
{
    /* Get the selected text */
    char *text = dw_window_get_text(handle);
    ULONG sel = (ULONG)WinSendMsg(handle, EM_QUERYSEL, 0, 0);
    SHORT pos1 = SHORT1FROMMP(sel), pos2 = SHORT2FROMMP(sel);

    /* Get the selection and put on clipboard for copy and cut */
    if(text)
    {
        if(command != ENTRY_PASTE)
        {
            if(pos2 > pos1)
            {
                text[pos2] = 0;

                dw_clipboard_set_text(&text[pos1], pos2 - pos1);
            }
        }
        free(text);
    }
    /* Clear selection for cut and paste */
    if(command != ENTRY_COPY)
        WinSendMsg(handle, EM_CLEAR, 0, 0);
    text = dw_window_get_text(handle);
    if(command == ENTRY_PASTE)
    {
        char *pastetext = dw_clipboard_get_text();

        if(pastetext)
        {
            _dw_combine_text(handle, pos1, text, pastetext);
            /* Free temporary memory */
            dw_free(pastetext);
        }
    }
    if(text)
        free(text);
}
#endif

/* Originally just intended for entryfields, it now serves as a generic
 * procedure for handling TAB presses to change input focus on controls.
 */
MRESULT EXPENTRY _dw_entryproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;
   char tmpbuf[100] = {0};

   if(blah)
      oldproc = blah->oldproc;

   WinQueryClassName(hWnd, 99, (PCH)tmpbuf);

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   /* These are the window classes which should get a menu */
   if(strncmp(tmpbuf, "#2", 3)==0 ||  /* Combobox */
      strncmp(tmpbuf, "#6", 3)==0 ||  /* Entryfield */
      strncmp(tmpbuf, "#10", 4)==0 || /* MLE */
      strncmp(tmpbuf, "#32", 4)==0)   /* Spinbutton */
   {
      switch(msg)
      {
#ifdef UNICODE
      case MLM_PASTE:
          _dw_mle_copy_paste(hWnd, ENTRY_PASTE);
          return (MRESULT)TRUE;
      case MLM_CUT:
          _dw_mle_copy_paste(hWnd, ENTRY_CUT);
          return (MRESULT)TRUE;
      case MLM_COPY:
          _dw_mle_copy_paste(hWnd, ENTRY_COPY);
          return (MRESULT)TRUE;
      case EM_PASTE:
          _dw_entry_copy_paste(hWnd, ENTRY_PASTE);
          return (MRESULT)TRUE;
      case EM_CUT:
          _dw_entry_copy_paste(hWnd, ENTRY_CUT);
          return (MRESULT)TRUE;
      case EM_COPY:
          _dw_entry_copy_paste(hWnd, ENTRY_COPY);
          return (MRESULT)TRUE;
#endif
      case WM_CONTEXTMENU:
         {
            HMENUI hwndMenu = dw_menu_new(0L);
            long x, y;
            unsigned long style = 0L;
            int is_mle = FALSE;

            if(strncmp(tmpbuf, "#10", 4)==0)
               is_mle = TRUE;

            /* When readonly, disable: Undo, Cut, Paste, Delete */
            if(is_mle && WinSendMsg(hWnd, MLM_QUERYREADONLY, 0, 0))
               style = DW_MIS_DISABLED;

            /* Undo is also disabled if it isn't an MLE */
            dw_menu_append_item(hwndMenu, "Undo", ENTRY_UNDO, style | (is_mle ? 0 : DW_MIS_DISABLED), TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, -1, 0L);

            /* Also check if non-MLE windows are disabled */
            if(!is_mle && dw_window_get_data(hWnd, "_dw_disabled"))
               style = DW_MIS_DISABLED;

            dw_menu_append_item(hwndMenu, "Cut", ENTRY_CUT, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Copy", ENTRY_COPY, 0L, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Paste", ENTRY_PASTE, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Delete", ENTRY_DELETE, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Select All", ENTRY_SALL, 0L, TRUE, -1, 0L);

            WinSetFocus(HWND_DESKTOP, hWnd);
            dw_pointer_query_pos(&x, &y);
            dw_menu_popup(&hwndMenu, hWnd, x, y);
         }
         break;
      case WM_COMMAND:
         {
            ULONG command = COMMANDMSG(&msg)->cmd;

            /* MLE */
            if(strncmp(tmpbuf, "#10", 4)==0)
            {
               switch(command)
               {
               case ENTRY_CUT:
                  return WinSendMsg(hWnd, MLM_CUT, 0, 0);
               case ENTRY_COPY:
                  return WinSendMsg(hWnd, MLM_COPY, 0, 0);
               case ENTRY_PASTE:
                  return WinSendMsg(hWnd, MLM_PASTE, 0, 0);
               case ENTRY_DELETE:
                  return WinSendMsg(hWnd, MLM_CLEAR, 0, 0);
               case ENTRY_UNDO:
                  return WinSendMsg(hWnd, MLM_UNDO, 0, 0);
               case ENTRY_SALL:
                  {
                     ULONG len = (ULONG)WinSendMsg(hWnd, MLM_QUERYTEXTLENGTH, 0, 0);
                     return WinSendMsg(hWnd, MLM_SETSEL, 0, (MPARAM)len);
                  }
               }
            }
            else /* Other */
            {
               HWND handle = hWnd;

               /* Get the entryfield handle from multi window controls */
               if(strncmp(tmpbuf, "#2", 3)==0)
                  handle = WinWindowFromID(hWnd, 667);

               if(handle)
               {
                  switch(command)
                  {
                  case ENTRY_CUT:
                     return WinSendMsg(handle, EM_CUT, 0, 0);
                  case ENTRY_COPY:
                     return WinSendMsg(handle, EM_COPY, 0, 0);
                  case ENTRY_PASTE:
                     return WinSendMsg(handle, EM_PASTE, 0, 0);
                  case ENTRY_DELETE:
                     return WinSendMsg(handle, EM_CLEAR, 0, 0);
                  case ENTRY_SALL:
                     {
                        LONG len = WinQueryWindowTextLength(hWnd);
                        return WinSendMsg(hWnd, EM_SETSEL, MPFROM2SHORT(0, (SHORT)len), 0);
                     }
                  }
               }
            }
         }
         break;
      }
   }

   switch(msg)
   {
   case WM_BUTTON1DOWN:
   case WM_BUTTON2DOWN:
   case WM_BUTTON3DOWN:
      {
         if(strncmp(tmpbuf, "#32", 4)==0)
            _dw_run_event(hWnd, WM_SETFOCUS, (MPARAM)FALSE, (MPARAM)TRUE);
      }
      break;
   case WM_CONTROL:
       {
           if(strncmp(tmpbuf, "#38", 4)==0)
               _dw_run_event(hWnd, msg, mp1, mp2);
       }
       break;
   case WM_SETFOCUS:
      _dw_run_event(hWnd, msg, mp1, mp2);
      break;
   case WM_CHAR:
      if(_dw_run_event(hWnd, msg, mp1, mp2) == (MRESULT)TRUE)
         return (MRESULT)TRUE;
      if(SHORT1FROMMP(mp2) == '\t')
      {
         if(CHARMSG(&msg)->fs & KC_SHIFT)
            _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      else if(SHORT1FROMMP(mp2) == '\r' && blah && blah->clickdefault)
         _dw_click_default(blah->clickdefault);
      /* When you hit escape we get this value and the
       * window hangs for reasons unknown. (in an MLE)
       */
      else if(SHORT1FROMMP(mp2) == 283)
         return (MRESULT)TRUE;
#ifdef UNICODE
      else if(!SHORT1FROMMP(mp2))
      {
          UniChar uc[2] = {0};
          VDKEY vdk;
          BYTE bscan;
          char *utf8;

          UniTranslateKey(Keyboard, SHORT1FROMMP(mp1) & KC_SHIFT  ? 1 : 0, CHAR4FROMMP(mp1), uc, &vdk, &bscan);

          if((utf8 = _dw_WideToUTF8(uc)) != NULL)
          {
              if(*utf8)
              {
                  /* MLE */
                  if(strncmp(tmpbuf, "#10", 4)==0)
                  {
                      WinSendMsg(hWnd, MLM_INSERT, MPFROMP(utf8), 0);
                  }
                  else /* Other */
                  {
                      HWND handle = hWnd;

                      /* Get the entryfield handle from multi window controls */
                      if(strncmp(tmpbuf, "#2", 3)==0)
                          handle = WinWindowFromID(hWnd, 667);

                      if(handle)
                      {
                          char *text = dw_window_get_text(handle);
                          ULONG sel = (ULONG)WinSendMsg(hWnd, EM_QUERYSEL, 0, 0);
                          SHORT pos1 = SHORT1FROMMP(sel);

                           WinSendMsg(handle, EM_CLEAR, 0, 0);
                          _dw_combine_text(handle, pos1, text, utf8);

                          if(text)
                              free(text);
                      }
                  }
              }
              free(utf8);
          }
          return (MRESULT)TRUE;
      }
#endif
      break;
   case WM_SIZE:
      {
         /* If it's a slider... make sure it shows the correct value */
         if(strncmp(tmpbuf, "#38", 4)==0)
            WinPostMsg(hWnd, WM_USER+7, 0, 0);
      }
      break;
   case WM_USER+7:
      {
         int pos = (int)dw_window_get_data(hWnd, "_dw_slider_value");
         WinSendMsg(hWnd, SLM_SETSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), (MPARAM)pos);
      }
      break;
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   }

   if(oldproc)
      return oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/*  Deal with combobox specifics and enhancements */
MRESULT EXPENTRY _dw_comboentryproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_CONTEXTMENU:
   case WM_COMMAND:
#ifdef UNICODE
   case EM_PASTE:
   case EM_CUT:
   case EM_COPY:
#endif
      return _dw_entryproc(hWnd, msg, mp1, mp2);
   case WM_SETFOCUS:
      _dw_run_event(hWnd, msg, mp1, mp2);
      break;
   case WM_CHAR:
      if(_dw_run_event(hWnd, msg, mp1, mp2) == (MRESULT)TRUE)
         return (MRESULT)TRUE;
      /* A Similar problem to the MLE, if ESC just return */
      if(SHORT1FROMMP(mp2) == 283)
         return (MRESULT)TRUE;
      break;
   }

   if(blah && blah->oldproc)
      return blah->oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/* Enhance the standard OS/2 MLE control */
MRESULT EXPENTRY _dw_mleproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch(msg)
   {
   case WM_VSCROLL:
      if(SHORT2FROMMP(mp2) == SB_SLIDERTRACK)
      {
         USHORT pos = SHORT1FROMMP(mp2);

         WinSendMsg(hWnd, msg, mp1, MPFROM2SHORT(pos, SB_SLIDERPOSITION));
      }
      break;
   }
   return _dw_entryproc(hWnd, msg, mp1, mp2);
}

/* Handle special messages for the spinbutton's entryfield */
MRESULT EXPENTRY _dw_spinentryproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
      oldproc = blah->oldproc;

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_CONTEXTMENU:
   case WM_COMMAND:
#ifdef UNICODE
   case EM_PASTE:
   case EM_CUT:
   case EM_COPY:
#endif
       return _dw_entryproc(hWnd, msg, mp1, mp2);
   }

   if(oldproc)
      return oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

int _dw_int_pos(HWND hwnd)
{
   int pos = (int)dw_window_get_data(hwnd, "_dw_percent_value");
   int range = _dw_percent_get_range(hwnd);
   float fpos = (float)pos;
   float frange = (float)range;
   float fnew = (fpos/1000.0)*frange;
   return (int)fnew;
}

void _dw_int_set(HWND hwnd, int pos)
{
   int inew, range = _dw_percent_get_range(hwnd);
   if(range)
   {
      float fpos = (float)pos;
      float frange = (float)range;
      float fnew = (fpos/frange)*1000.0;
      inew = (int)fnew;
      dw_window_set_data(hwnd, "_dw_percent_value", (void *)inew);
   }
}

/* Handle size changes in the percent class */
MRESULT EXPENTRY _dw_percentproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
      oldproc = blah->oldproc;

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_SIZE:
      WinPostMsg(hWnd, WM_USER+7, 0, 0);
      break;
   case WM_USER+7:
      WinSendMsg(hWnd, SLM_SETSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE), (MPARAM)_dw_int_pos(hWnd));
      break;
   }

   if(oldproc)
      return oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/* Handle correct painting of a combobox with the WS_CLIPCHILDREN
 * flag enabled, and also handle TABs to switch input focus.
 */
MRESULT EXPENTRY _dw_comboproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = WinQueryWindowPtr(hWnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
      oldproc = blah->oldproc;

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hWnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_CHAR:
      if(SHORT1FROMMP(mp2) == '\t')
      {
         if(CHARMSG(&msg)->fs & KC_SHIFT)
            _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      else if(SHORT1FROMMP(mp2) == '\r' && blah && blah->clickdefault)
         _dw_click_default(blah->clickdefault);
      break;
   case WM_BUTTON1DBLCLK:
   case WM_BUTTON2DBLCLK:
   case WM_BUTTON3DBLCLK:
      if(dw_window_get_data(hWnd, "_dw_disabled"))
         return (MRESULT)TRUE;
      break;
   case WM_BUTTON1DOWN:
   case WM_BUTTON2DOWN:
   case WM_BUTTON3DOWN:
      if(_dw_run_event(hWnd, msg, mp1, mp2) == (MRESULT)TRUE)
         return (MRESULT)TRUE;
      _dw_run_event(hWnd, WM_SETFOCUS, (MPARAM)FALSE, (MPARAM)TRUE);
      break;
   case WM_SETFOCUS:
      _dw_run_event(hWnd, msg, mp1, mp2);
      break;
   case WM_PAINT:
      {
         HWND entry, frame = (HWND)dw_window_get_data(hWnd, "_dw_combo_box"), parent = WinQueryWindow(frame, QW_PARENT);
         HPS hpsPaint;
         POINTL ptl;
         unsigned long width, height, thumbheight = 0;
         ULONG color;

         if((entry = (HWND)dw_window_get_data(hWnd, "_dw_comboentry")) != NULLHANDLE)
            dw_window_get_pos_size(entry, 0, 0, 0, &thumbheight);

         if(!thumbheight)
            thumbheight = WinQuerySysValue(HWND_DESKTOP, SV_CYVSCROLLARROW);

         /* Add 6 because it has a thick border like the entryfield */
         thumbheight += 6;

         color = (ULONG)dw_window_get_data(parent, "_dw_fore");
         dw_window_get_pos_size(hWnd, 0, 0, &width, &height);

         if(height > thumbheight)
         {
            hpsPaint = WinGetPS(hWnd);
            if(color)
               GpiSetColor(hpsPaint, _dw_internal_color(color-1));
            else
               GpiSetColor(hpsPaint, CLR_PALEGRAY);

            ptl.x = ptl.y = 0;
            GpiMove(hpsPaint, &ptl);

            ptl.x = width;
            ptl.y = height - thumbheight;
            GpiBox(hpsPaint, DRO_FILL, &ptl, 0, 0);

            WinReleasePS(hpsPaint);
         }
      }
      break;
   }
   if(oldproc)
      return oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

void _dw_get_pp_font(HWND hwnd, char *buff)
{
    ULONG AttrFound;
    BYTE  AttrValue[128];
    ULONG cbRetLen;

    cbRetLen = WinQueryPresParam(hwnd,
                                 PP_FONTNAMESIZE,
                                 0,
                                 &AttrFound,
                                 sizeof(AttrValue),
                                 &AttrValue,
                                 QPF_NOINHERIT);

    if(PP_FONTNAMESIZE == AttrFound && cbRetLen)
    {
        memcpy(buff, AttrValue, cbRetLen);
    }
}

int _dw_handle_scroller(HWND handle, int pos, int which)
{
   MPARAM res;
   int min, max, page;

   if(which == SB_SLIDERTRACK)
      return pos;

   pos = dw_scrollbar_get_pos(handle);
   res = WinSendMsg(handle, SBM_QUERYRANGE, 0, 0);

   min = SHORT1FROMMP(res);
   max = SHORT2FROMMP(res);
   page = (int)dw_window_get_data(handle, "_dw_scrollbar_visible");

   switch(which)
   {
   case SB_LINEUP:
      pos = pos - 1;
      if(pos < min)
         pos = min;
      dw_scrollbar_set_pos(handle, pos);
      return pos;
   case SB_LINEDOWN:
      pos = pos + 1;
      if(pos > max)
         pos = max;
      dw_scrollbar_set_pos(handle, pos);
      return pos;
   case SB_PAGEUP:
      pos = pos - page;
      if(pos < min)
         pos = min;
      dw_scrollbar_set_pos(handle, pos);
      return pos;
   case SB_PAGEDOWN:
      pos = pos + page;
      if(pos > max)
         pos = max;
      dw_scrollbar_set_pos(handle, pos);
      return pos;
   }
   return -1;
}

void _dw_clear_emphasis(void)
{
   if(_dw_emph && WinIsWindow(dwhab, _dw_emph) && _dw_core_emph)
      WinSendMsg(_dw_emph, CM_SETRECORDEMPHASIS, _dw_core_emph, MPFROM2SHORT(FALSE, CRA_SOURCE));
   _dw_emph = NULLHANDLE;
   _dw_core_emph = NULL;
}

/* Find the desktop window handle */
HWND _dw_menu_owner(HWND handle)
{
   HWND menuowner = NULLHANDLE, lastowner = (HWND)dw_window_get_data(handle, "_dw_owner");
   int menubar = (int)dw_window_get_data(handle, "_dw_menubar");

   /* Find the toplevel window */
   while(!menubar && (menuowner = (HWND)dw_window_get_data(lastowner, "_dw_owner")) != NULLHANDLE)
   {
      menubar = (int)dw_window_get_data(lastowner, "_dw_menubar");
      lastowner = menuowner;
   }
   if(menuowner && menubar)
   {
      HWND client = WinWindowFromID(menuowner, FID_CLIENT);

      return client ? client : menuowner;
   }
   return NULLHANDLE;
}

MRESULT EXPENTRY _dw_run_event(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   int result = -1;
   DWSignalHandler *tmp = Root;
   ULONG origmsg = msg;

   if(msg == WM_BUTTON2DOWN || msg == WM_BUTTON3DOWN)
      msg = WM_BUTTON1DOWN;
   if(msg == WM_BUTTON2UP || msg == WM_BUTTON3UP)
      msg = WM_BUTTON1UP;
   if(msg == WM_VSCROLL || msg == WM_HSCROLL)
      msg = WM_CONTROL;

   /* Find any callbacks for this function */
   while(tmp)
   {
      if(tmp->message == msg || msg == WM_CONTROL || tmp->message == WM_USER+1)
      {
         switch(msg)
         {
         case WM_SETFOCUS:
            {
               if((mp2 && tmp->message == WM_SETFOCUS) || (!mp2 && tmp->message == WM_USER+1))
               {
                  int (API_FUNC setfocusfunc)(HWND, void *) = (int (API_FUNC)(HWND, void *))tmp->signalfunction;

                  if(hWnd == tmp->window || WinWindowFromID(tmp->window, FID_CLIENT) == hWnd)
                  {
                     result = setfocusfunc(tmp->window, tmp->data);
                     tmp = NULL;
                  }
               }
            }
            break;
         case WM_TIMER:
            {
               int (API_FUNC timerfunc)(void *) = (int (API_FUNC)(void *))tmp->signalfunction;
               if(tmp->id == (int)mp1)
               {
                  if(!timerfunc(tmp->data))
                     dw_timer_disconnect(tmp->id);
                  tmp = NULL;
               }
               result = 0;
            }
            break;
         case WM_SIZE:
            {
               int (API_FUNC sizefunc)(HWND, int, int, void *) = (int (API_FUNC)(HWND, int, int, void *))tmp->signalfunction;

               if((hWnd == tmp->window || WinWindowFromID(tmp->window, FID_CLIENT) == hWnd) && SHORT1FROMMP(mp2) && SHORT2FROMMP(mp2))
               {
                  result = sizefunc(tmp->window, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2), tmp->data);
                  tmp = NULL;
               }
            }
            break;
         case WM_BUTTON1DOWN:
            {
               POINTS pts = (*((POINTS*)&mp1));
               int (API_FUNC buttonfunc)(HWND, int, int, int, void *) = (int (API_FUNC)(HWND, int, int, int, void *))tmp->signalfunction;

               if(hWnd == tmp->window || WinWindowFromID(tmp->window, FID_CLIENT) == hWnd || WinQueryCapture(HWND_DESKTOP) == tmp->window)
               {
                  int button = 0;

                  switch(origmsg)
                  {
                  case WM_BUTTON1DOWN:
                     button = 1;
                     break;
                  case WM_BUTTON2DOWN:
                     button = 2;
                     break;
                  case WM_BUTTON3DOWN:
                     button = 3;
                     break;
                  }

                  result = buttonfunc(tmp->window, pts.x, WinQueryWindow(tmp->window, QW_PARENT) == HWND_DESKTOP ? dw_screen_height() - pts.y : _dw_get_height(tmp->window) - pts.y, button, tmp->data);
                  tmp = NULL;
               }
            }
            break;
         case WM_BUTTON1UP:
            {
               POINTS pts = (*((POINTS*)&mp1));
               int (API_FUNC buttonfunc)(HWND, int, int, int, void *) = (int (API_FUNC)(HWND, int, int, int, void *))tmp->signalfunction;

               if(hWnd == tmp->window || WinWindowFromID(tmp->window, FID_CLIENT) == hWnd || WinQueryCapture(HWND_DESKTOP) == tmp->window)
               {
                  int button = 0;

                  switch(origmsg)
                  {
                  case WM_BUTTON1UP:
                     button = 1;
                     break;
                  case WM_BUTTON2UP:
                     button = 2;
                     break;
                  case WM_BUTTON3UP:
                     button = 3;
                     break;
                  }

                  result = buttonfunc(tmp->window, pts.x, WinQueryWindow(tmp->window, QW_PARENT) == HWND_DESKTOP ? dw_screen_height() - pts.y : _dw_get_height(tmp->window) - pts.y, button, tmp->data);
                  tmp = NULL;
               }
            }
            break;
         case WM_MOUSEMOVE:
            {
               int (API_FUNC motionfunc)(HWND, int, int, int, void *) = (int (API_FUNC)(HWND, int, int, int, void *))tmp->signalfunction;

               if(hWnd == tmp->window || WinWindowFromID(tmp->window, FID_CLIENT) == hWnd || WinQueryCapture(HWND_DESKTOP) == tmp->window)
               {
                  int keys = 0;
                  SHORT x = SHORT1FROMMP(mp1), y = SHORT2FROMMP(mp1);

                  if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON1) & 0x8000)
                     keys = DW_BUTTON1_MASK;
                  if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON2) & 0x8000)
                     keys |= DW_BUTTON2_MASK;
                  if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON3) & 0x8000)
                     keys |= DW_BUTTON3_MASK;

                  result = motionfunc(tmp->window, x, _dw_get_height(hWnd) - y, keys, tmp->data);
                  tmp = NULL;
               }
            }
            break;
         case WM_CHAR:
            {
                int (API_FUNC keypressfunc)(HWND, char, int, int, void *, char *) = (int (API_FUNC)(HWND, char, int, int, void *, char *))tmp->signalfunction;

                if((hWnd == tmp->window || _dw_toplevel_window(hWnd) == tmp->window) && !(SHORT1FROMMP(mp1) & KC_KEYUP))
                {
                  int vk;
                  char ch[2] = {0};
                  char *utf8 = NULL;
#ifdef UNICODE
                  UniChar uc[2] = {0};
                  VDKEY vdk;
                  BYTE bscan;
                  UniTranslateKey(Keyboard, SHORT1FROMMP(mp1) & KC_SHIFT  ? 1 : 0, CHAR4FROMMP(mp1), uc, &vdk, &bscan);
                  utf8 = _dw_WideToUTF8(uc);
#endif

                  if(SHORT1FROMMP(mp1) & KC_CHAR)
                     ch[0] = (char)SHORT1FROMMP(mp2);
                  if(SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
                     vk = SHORT2FROMMP(mp2);
                  else
                     vk = SHORT1FROMMP(mp2) + 128;

                  /* This is a hack to fix shift presses showing
                   * up as tabs!
                   */
                  if(ch[0] == '\t' && !(SHORT1FROMMP(mp1) & KC_CHAR))
                  {
                     ch[0] = 0;
                     vk = VK_SHIFT;
                  }

                  result = keypressfunc(tmp->window, ch[0], vk,
                                        SHORT1FROMMP(mp1) & (KC_ALT | KC_SHIFT | KC_CTRL), tmp->data, utf8 ? utf8 : ch);
                  tmp = NULL;

                  if(utf8)
                      free(utf8);
               }
            }
            break;
         case WM_CLOSE:
            {
               int (API_FUNC closefunc)(HWND, void *) = (int (API_FUNC)(HWND, void *))tmp->signalfunction;

               if(hWnd == tmp->window || hWnd == WinWindowFromID(tmp->window, FID_CLIENT))
               {
                  result = closefunc(tmp->window, tmp->data);
                  if(result)
                     result = FALSE;
                  tmp = NULL;
               }
            }
            break;
         case WM_PAINT:
            {
               HPS hps;
               DWExpose exp;
               int (API_FUNC exposefunc)(HWND, DWExpose *, void *) = (int (API_FUNC)(HWND, DWExpose *, void *))tmp->signalfunction;
               RECTL  rc;

               if(hWnd == tmp->window)
               {
                  int height = _dw_get_height(hWnd);
                  HWND oldrender = _dw_render_expose;

                  hps = WinBeginPaint(hWnd, 0L, &rc);
                  exp.x = rc.xLeft;
                  exp.y = height - rc.yTop;
                  exp.width = rc.xRight - rc. xLeft;
                  exp.height = rc.yTop - rc.yBottom;
                  if(_dw_render_safe_mode == DW_FEATURE_ENABLED && _dw_is_render(hWnd))
                     _dw_render_expose = hWnd;
                  result = exposefunc(hWnd, &exp, tmp->data);
                  _dw_render_expose = oldrender;
                  WinEndPaint(hps);
               }
            }
            break;
         case WM_COMMAND:
            {
               int (API_FUNC clickfunc)(HWND, void *) = (int (API_FUNC)(HWND, void *))tmp->signalfunction;
               ULONG command = COMMANDMSG(&msg)->cmd;

               if(tmp->id && command == tmp->id)
               {
                  HWND menuowner = _dw_menu_owner(tmp->window);

                  if(menuowner == hWnd || menuowner == NULLHANDLE)
                  {
                     result = clickfunc((HWND)tmp->id, tmp->data);
                     tmp = NULL;
                  }
               }
               else if(tmp->window < 65536 && command == tmp->window)
               {
                  result = clickfunc(_dw_popup ?  _dw_popup : tmp->window, tmp->data);
                  tmp = NULL;
               }
            }
            break;
         case WM_CONTROL:
            if(origmsg == WM_VSCROLL || origmsg == WM_HSCROLL || tmp->message == SHORT2FROMMP(mp1) ||
               (tmp->message == SLN_SLIDERTRACK && (SHORT2FROMMP(mp1) == SLN_CHANGE || SHORT2FROMMP(mp1) == SPBN_CHANGE)))
            {
               int svar = SLN_SLIDERTRACK;
               int id = SHORT1FROMMP(mp1);
               HWND notifyhwnd = dw_window_from_id(hWnd, id);

               if(origmsg == WM_CONTROL)
               {
                   svar = SHORT2FROMMP(mp1);
                   if(!notifyhwnd && WinIsWindow(dwhab, (HWND)mp2))
                       notifyhwnd = (HWND)mp2;
               }

               switch(svar)
               {
               case CN_ENTER:
                  {
                     int (API_FUNC containerselectfunc)(HWND, char *, void *, void *) = (int (API_FUNC)(HWND, char *, void *, void *))tmp->signalfunction;
                     char *text = NULL;
                     void *data = NULL;

                     if(mp2)
                     {
                        PRECORDCORE pre;

                        pre = ((PNOTIFYRECORDENTER)mp2)->pRecord;
                        if(pre)
                        {
                            text = (char *)pre->pszIcon;
                            data = (void *)pre->pszText;
                        }
                     }

                     if(tmp->window == notifyhwnd)
                     {
                        result = containerselectfunc(tmp->window, text, tmp->data, data);
                        tmp = NULL;
                     }
                  }
                  break;
               case CN_EXPANDTREE:
                  {
                     int (API_FUNC treeexpandfunc)(HWND, HTREEITEM, void *) = (int (API_FUNC)(HWND, HTREEITEM, void *))tmp->signalfunction;

                     if(tmp->window == notifyhwnd)
                     {
                        result = treeexpandfunc(tmp->window, (HTREEITEM)mp2, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case CN_CONTEXTMENU:
                  {
                     int (API_FUNC containercontextfunc)(HWND, char *, int, int, void *, void *) = (int (API_FUNC)(HWND, char *, int, int, void *, void *))tmp->signalfunction;
                     char *text = NULL;
                     void *user = NULL;
                     LONG x,y;

                     dw_pointer_query_pos(&x, &y);

                     if(tmp->window == notifyhwnd)
                     {
                        int container = (int)dw_window_get_data(tmp->window, "_dw_container");

                        if(mp2)
                        {
                           PCNRITEM pci = (PCNRITEM)mp2;

                           text = (char *)pci->rc.pszIcon;

                           if(!container)
                           {
                              NOTIFYRECORDEMPHASIS pre;

                              dw_tree_item_select(tmp->window, (HTREEITEM)mp2);
                              pre.pRecord = mp2;
                              pre.fEmphasisMask = CRA_CURSORED;
                              pre.hwndCnr = tmp->window;
                              _dw_run_event(hWnd, WM_CONTROL, MPFROM2SHORT(0, CN_EMPHASIS), (MPARAM)&pre);
                              pre.pRecord->flRecordAttr |= CRA_CURSORED;
                              user = pci->user;
                           }
                           else
                           {
                              PRECORDCORE rc = (PRECORDCORE)mp2;

                              if(_dw_core_emph)
                                 _dw_clear_emphasis();
                              _dw_emph = tmp->window;
                              _dw_core_emph = mp2;
                              WinSendMsg(tmp->window, CM_SETRECORDEMPHASIS, mp2, MPFROM2SHORT(TRUE, CRA_SOURCE));
                              user = rc->pszText;
                           }
                        }
                        result = containercontextfunc(tmp->window, text, x, y, tmp->data, user);
                        tmp = NULL;
                     }
                  }
                  break;
               case CN_EMPHASIS:
                  {
                     PNOTIFYRECORDEMPHASIS pre = (PNOTIFYRECORDEMPHASIS)mp2;
                     static int emph_recurse = 0;

                     if(!emph_recurse)
                     {
                        emph_recurse = 1;

                        if(mp2)
                        {
                           if(tmp->window == pre->hwndCnr)
                           {
                              /* PCNRITEM for Tree PRECORDCORE for Container */
                              PCNRITEM pci = (PCNRITEM)pre->pRecord;
                              PRECORDCORE prc = pre->pRecord;

                              if(pci && pre->fEmphasisMask & CRA_CURSORED && (pci->rc.flRecordAttr & CRA_CURSORED))
                              {
                                 int (API_FUNC treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = (int (API_FUNC)(HWND, HTREEITEM, char *, void *, void *))tmp->signalfunction;

                                 if(dw_window_get_data(tmp->window, "_dw_container"))
                                    result = treeselectfunc(tmp->window, 0, (char *)prc->pszIcon, tmp->data, (void *)prc->pszText);
                                 else
                                 {
                                    if(_dw_lasthcnr == tmp->window && _dw_lastitem == (HWND)pci)
                                    {
                                       _dw_lasthcnr = 0;
                                       _dw_lastitem = 0;
                                    }
                                    else
                                    {
                                       _dw_lasthcnr = tmp->window;
                                       _dw_lastitem = (HWND)pci;
                                       result = treeselectfunc(tmp->window, (HTREEITEM)pci, (char *)pci->rc.pszIcon, tmp->data, pci->user);
                                    }
                                 }
                                 tmp = NULL;
                              }
                           }
                        }
                        emph_recurse = 0;
                     }
                  }
                  break;
               case LN_SELECT:
                  {
                     char classbuf[100];

                     WinQueryClassName(tmp->window, 99, (PCH)classbuf);

                     /* Slider/Percent */
                     if(strncmp(classbuf, "#38", 4) == 0)
                     {
                        int (API_FUNC valuechangedfunc)(HWND, int, void *) = (int (API_FUNC)(HWND, int, void *))tmp->signalfunction;

                        if(tmp->window == hWnd || tmp->window == notifyhwnd)
                        {
                           static int lastvalue = -1;
                           static HWND lasthwnd = NULLHANDLE;
                           int ulValue = (int)WinSendMsg(tmp->window, SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), 0);
                           if(lastvalue != ulValue || lasthwnd != tmp->window)
                           {
                              result = valuechangedfunc(tmp->window, ulValue, tmp->data);
                              lastvalue = ulValue;
                              lasthwnd = tmp->window;
                           }
                           tmp = NULL;
                        }
                     }
                     else
                     {
                        int (API_FUNC listboxselectfunc)(HWND, int, void *) = (int (API_FUNC)(HWND, int, void *))tmp->signalfunction;
                        static int _recursing = 0;

                        if(_recursing == 0 && (tmp->window == notifyhwnd || (!id && tmp->window == (HWND)mp2)))
                        {
                           char buf1[500];
                           int index = dw_listbox_selected(tmp->window);

                           dw_listbox_get_text(tmp->window, index, buf1, 500);

                           _recursing = 1;

                           /* Combobox */
                           if(id && strncmp(classbuf, "#2", 3)==0)
                           {
                              char *buf2;

                              buf2 = dw_window_get_text(tmp->window);

                              /* This is to make sure the listboxselect function doesn't
                               * get called if the user is modifying the entry text.
                               */
                              if(buf2 && *buf2 && *buf1 && strncmp(buf1, buf2, 500) == 0)
                                 result = listboxselectfunc(tmp->window, index, tmp->data);

                              if(buf2)
                                 free(buf2);
                           }
                           else
                              result = listboxselectfunc(tmp->window, index, tmp->data);

                           _recursing = 0;
                           tmp = NULL;
                        }
                     }
                  }
                  break;
               case SPBN_CHANGE:
                  {
                     int (API_FUNC valuechangedfunc)(HWND, int, void *) = (int (API_FUNC)(HWND, int, void *))tmp->signalfunction;

                     if(origmsg == WM_CONTROL && tmp->message == SLN_SLIDERTRACK)
                     {
                        /* Handle Spinbutton control */
                        if(tmp->window == hWnd || tmp->window == notifyhwnd)
                        {
                            int position = dw_spinbutton_get_pos(tmp->window);
                            result = valuechangedfunc(tmp->window, position, tmp->data);
                            tmp = NULL;
                        }
                     }
                  }
                  break;
               case SLN_SLIDERTRACK:
                  {
                     int (API_FUNC valuechangedfunc)(HWND, int, void *) = (int (API_FUNC)(HWND, int, void *))tmp->signalfunction;

                     if(origmsg == WM_CONTROL)
                     {
                        /* Handle Slider control */
                        if(tmp->window == hWnd || tmp->window == notifyhwnd)
                        {
                           static int lastvalue = -1;
                           static HWND lasthwnd = NULLHANDLE;
                           int ulValue = (int)WinSendMsg(tmp->window, SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), 0);
                           if(lastvalue != ulValue || lasthwnd != tmp->window)
                           {
                              dw_window_set_data(tmp->window, "_dw_slider_value", (void *)ulValue);
                              result = valuechangedfunc(tmp->window, ulValue, tmp->data);
                              lastvalue = ulValue;
                              lasthwnd = tmp->window;
                           }
                           tmp = NULL;
                        }
                     }
                     else
                     {
                        /* Handle scrollbar control */
                        if(tmp->window > 65535 && tmp->window == notifyhwnd)
                        {
                           int pos = _dw_handle_scroller(tmp->window, (int)SHORT1FROMMP(mp2), (int)SHORT2FROMMP(mp2));;

                           if(pos > -1)
                           {
                              dw_window_set_data(tmp->window, "_dw_scrollbar_value", (void *)pos);
                              result = valuechangedfunc(tmp->window, pos, tmp->data);
                           }
                           result = 0;
                           tmp = NULL;
                        }
                     }
                  }
                  break;
               case BKN_PAGESELECTED:
                  {
                     PAGESELECTNOTIFY *psn = (PAGESELECTNOTIFY *)mp2;

                     if(psn && tmp->window == psn->hwndBook)
                     {
                        int (API_FUNC switchpagefunc)(HWND, unsigned long, void *) = (int (API_FUNC)(HWND, unsigned long, void *))tmp->signalfunction;

                        result = switchpagefunc(tmp->window, psn->ulPageIdNew, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               }
            }
            break;
         }
      }

      if(tmp)
         tmp = tmp->next;

   }
   if(result != -1)
   {
      /* Make sure any queued redraws are handled */
      _dw_redraw(0, FALSE);
      /* Then finally return */
   }
   return (MRESULT)result;
}

/* Gets a DW_RGB value from the three spinbuttons */
unsigned long _dw_color_spin_get(HWND window)
{
   HWND button = (HWND)dw_window_get_data(window, "_dw_red_spin");
   long red, green, blue;

   red = dw_spinbutton_get_pos(button);
   button = (HWND)dw_window_get_data(window, "_dw_green_spin");
   green = dw_spinbutton_get_pos(button);
   button = (HWND)dw_window_get_data(window, "_dw_blue_spin");
   blue = dw_spinbutton_get_pos(button);

   return DW_RGB(red, green, blue);
}

/* Set the three spinbuttons from a DW_RGB value */
void _dw_color_spin_set(HWND window, unsigned long value)
{
   HWND button = (HWND)dw_window_get_data(window, "_dw_red_spin");
   dw_window_set_data(window, "_dw_updating", (void *)1);
   dw_spinbutton_set_pos(button, DW_RED_VALUE(value));
   button = (HWND)dw_window_get_data(window, "_dw_green_spin");
   dw_spinbutton_set_pos(button, DW_GREEN_VALUE(value));
   button = (HWND)dw_window_get_data(window, "_dw_blue_spin");
   dw_spinbutton_set_pos(button, DW_BLUE_VALUE(value));
   dw_window_set_data(window, "_dw_updating", NULL);
}

/* Sets the color selection control to be a DW_RGB value */
void _dw_col_set(HWND col, unsigned long value)
{
   WinSendMsg(col, 0x0602, MPFROMLONG(_dw_os2_color(value)), 0);
   if(!IS_WARP4())
      WinSendMsg(col, 0x1384, MPFROMLONG(_dw_os2_color(value)), 0);
}

/* Handles control messages sent to the box (owner). */
MRESULT EXPENTRY _dw_controlproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   Box *blah = WinQueryWindowPtr(hWnd, QWP_USER);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hWnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_VSCROLL:
   case WM_HSCROLL:
      if(_dw_run_event(hWnd, msg, mp1, mp2))
      {
         HWND window = WinWindowFromID(hWnd, (ULONG)mp1);
         _dw_handle_scroller(window, (int)SHORT1FROMMP(mp2), (int)SHORT2FROMMP(mp2));
      }
      break;
      /* Handles Color Selection control messages */
   case 0x0601:
   case 0x130C:
      {
         HWND window = (HWND)dw_window_get_data(hWnd, "_dw_window");
         unsigned long val = (unsigned long)mp1;

         if(window)
            _dw_color_spin_set(window, DW_RGB((val & 0xFF0000) >> 16, (val & 0xFF00) >> 8, val & 0xFF));
      }
      break;
   case WM_USER:
       _dw_run_event(hWnd, WM_CONTROL, mp1, mp2);
       break;
   case WM_CONTROL:
       {
           char tmpbuf[100];

           WinQueryClassName((HWND)mp2, 99, (PCH)tmpbuf);
           /* Don't set the ownership if it's an entryfield or spinbutton  */
           if(strncmp(tmpbuf, "#32", 4)==0)
           {
               if((SHORT2FROMMP(mp1) == SPBN_CHANGE || SHORT2FROMMP(mp1) == SPBN_ENDSPIN))
               {
                   HWND window = (HWND)dw_window_get_data(hWnd, "_dw_window");

                   if(window && !dw_window_get_data(window, "_dw_updating"))
                   {
                       unsigned long val = _dw_color_spin_get(window);
                       HWND col = (HWND)dw_window_get_data(window, "_dw_col");

                       _dw_col_set(col, val);
                   }
               }
               if(!dw_window_get_data((HWND)mp2, "_dw_updating"))
                   WinPostMsg(hWnd, WM_USER, mp1, mp2);
           }
           else
               _dw_run_event(hWnd, msg, mp1, mp2);
      }
      break;
   }

   if(blah && blah->oldproc)
      return blah->oldproc(hWnd, msg, mp1, mp2);

   return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

/* The main window procedure for Dynamic Windows, all the resizing code is done here. */
MRESULT EXPENTRY _dw_wndproc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   int result = -1;
   static int command_active = 0;
   void (API_FUNC windowfunc)(PVOID) = 0L;

   if(!command_active)
   {
        /* Make sure we don't end up in infinite recursion */
      command_active = 1;

      if(msg == WM_ACTIVATE)
          result = (int)_dw_run_event((HWND)mp2, WM_SETFOCUS, 0, mp1);
      else
          result = (int)_dw_run_event(hWnd, msg, mp1, mp2);

      command_active = 0;
   }

   /* Now that any handlers are done... do normal processing */
   switch( msg )
   {
   case WM_ERASEBACKGROUND:
      return 0;

   case WM_PAINT:
      {
      HPS    hps;
      RECTL  rc;

      hps = WinBeginPaint( hWnd, 0L, &rc );
      WinEndPaint( hps );
      break;
      }

   case WM_SIZE:
      {
         Box *mybox = (Box *)WinQueryWindowPtr(hWnd, QWP_USER);

         if(!SHORT1FROMMP(mp2) && !SHORT2FROMMP(mp2))
            return (MPARAM)TRUE;

         if(mybox && mybox->flags != DW_MINIMIZED)
         {
            /* Hide the window when recalculating to reduce
             * CPU load.
             */
            WinShowWindow(hWnd, FALSE);

            if(mybox->items)
                WinSetWindowPos(mybox->items[0].hwnd, HWND_TOP, 0, 0, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2), SWP_MOVE | SWP_SIZE);

            _dw_do_resize(mybox, SHORT1FROMMP(mp2), SHORT2FROMMP(mp2));

            WinShowWindow(hWnd, TRUE);
         }
      }
      break;
   case WM_MINMAXFRAME:
      {
         Box *mybox = (Box *)WinQueryWindowPtr(hWnd, QWP_USER);
         SWP *swp = (SWP *)mp1;

         if(mybox && (swp->fl & SWP_MINIMIZE))
            mybox->flags = DW_MINIMIZED;

         if(mybox && (swp->fl & SWP_RESTORE))
         {
            if(!mybox->titlebar && mybox->hwndtitle)
               WinSetParent(mybox->hwndtitle, HWND_OBJECT, FALSE);
            mybox->flags = 0;
         }

         if(mybox && (swp->fl & (SWP_MAXIMIZE | SWP_RESTORE)))
         {
            int z;
            SWP swp2;

            WinQueryWindowPos(swp->hwnd, &swp2);

            if(swp2.cx == swp->cx && swp2.cy == swp->cy)
               return FALSE;

            mybox->flags = 0;

            /* Hide the window when recalculating to reduce
             * CPU load.
             */
            WinShowWindow(hWnd, FALSE);

            _dw_do_resize(mybox, swp->cx, swp->cy);

            if(mybox->count == 1 && mybox->items[0].type == _DW_TYPE_BOX)
            {
               mybox = (Box *)WinQueryWindowPtr(mybox->items[0].hwnd, QWP_USER);

               for(z=0;z<mybox->count;z++)
                  _dw_check_resize_notebook(mybox->items[z].hwnd);

            }

            WinShowWindow(hWnd, TRUE);
         }
      }
      break;
   case WM_CONTROL:
      switch(SHORT2FROMMP(mp1))
      {
      case BKN_PAGESELECTEDPENDING:
         {
            PAGESELECTNOTIFY *psn = (PAGESELECTNOTIFY *)mp2;
            HWND pagehwnd = (HWND)WinSendMsg(psn->hwndBook, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(psn->ulPageIdNew), 0);
            Box *pagebox = (Box *)WinQueryWindowPtr(pagehwnd, QWP_USER);
            long x, y;
            unsigned long width, height;
            RECTL rc;

            if(pagebox && psn->ulPageIdNew != psn->ulPageIdCur)
            {
               dw_window_get_pos_size(psn->hwndBook, &x, &y, &width, &height);

               rc.xLeft = x;
               rc.yBottom = y;
               rc.xRight = x + width;
               rc.yTop = y + height;

               WinSendMsg(psn->hwndBook, BKM_CALCPAGERECT, (MPARAM)&rc, (MPARAM)TRUE);

               _dw_do_resize(pagebox, rc.xRight - rc.xLeft, rc.yTop - rc.yBottom);
            }
         }
         break;
      }
      break;
   case WM_CLOSE:
      if(result == -1)
      {
         dw_window_destroy(WinQueryWindow(hWnd, QW_PARENT));
         return (MRESULT)TRUE;
      }
      break;
   case WM_MOUSEMOVE:
      {
         HPOINTER pointer;

         if((pointer = (HPOINTER)dw_window_get_data(hWnd, "_dw_pointer")) ||
            (pointer = (HPOINTER)dw_window_get_data(_dw_toplevel_window(hWnd), "_dw_pointer")))
         {
            WinSetPointer(HWND_DESKTOP, pointer);
            return MRFROMSHORT(TRUE);
         }
      }
      return MRFROMSHORT(FALSE);
   case WM_USER:
      windowfunc = (void (API_FUNC)(void *))mp1;

      if(windowfunc)
         windowfunc((void *)mp2);
      break;
   case WM_CHAR:
      if(SHORT1FROMMP(mp2) == '\t')
      {
         if(CHARMSG(&msg)->fs & KC_SHIFT)
            _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      break;
   case WM_DESTROY:
      {
         HWND parent = WinQueryWindow(hWnd, QW_PARENT);

         /* Free memory before destroying */
         if(parent && WinWindowFromID(parent, FID_CLIENT) == hWnd)
            _dw_free_window_memory(parent);
         else
            _dw_free_window_memory(hWnd);
      }
      break;
   case WM_MENUEND:
      /* Delay removing the signal until we've executed
       * the signal handler.
       */
      WinPostMsg(hWnd, WM_USER+2, mp1, mp2);
      break;
   case WM_DDE_INITIATEACK:
       /* aswer dde server */
       _dw_tray = (HWND)mp1;
       break;
   case WM_BUTTON1DOWN | 0x2000:
   case WM_BUTTON2DOWN | 0x2000:
   case WM_BUTTON3DOWN | 0x2000:
   case WM_BUTTON1UP | 0x2000:
   case WM_BUTTON2UP | 0x2000:
   case WM_BUTTON3UP | 0x2000:
       if(_dw_task_bar)
           result = (int)_dw_run_event(_dw_task_bar, msg & ~0x2000, mp1, mp2);
       break;
   case WM_USER+2:
      _dw_clear_emphasis();
      if(dw_window_get_data((HWND)mp2, "_dw_popup"))
         _dw_free_menu_data((HWND)mp2);
      break;
   }

   if(result != -1)
      return (MRESULT)result;
   else
      return WinDefWindowProc(hWnd, msg, mp1, mp2);
}

void _dw_change_box(Box *thisbox, int percent, int type)
{
   int z;

   for(z=0;z<thisbox->count;z++)
   {
      if(thisbox->items[z].type == _DW_TYPE_BOX)
      {
         Box *tmp = WinQueryWindowPtr(thisbox->items[z].hwnd, QWP_USER);
         _dw_change_box(tmp, percent, type);
      }
      else
      {
         if(type == DW_HORZ)
         {
            if(thisbox->items[z].hsize == _DW_SIZE_EXPAND)
               thisbox->items[z].width = (int)(((float)thisbox->items[z].origwidth) * (((float)percent)/((float)100.0)));
         }
         else
         {
            if(thisbox->items[z].vsize == _DW_SIZE_EXPAND)
               thisbox->items[z].height = (int)(((float)thisbox->items[z].origheight) * (((float)percent)/((float)100.0)));
         }
      }
   }
}

void _dw_handle_splitbar_resize(HWND hwnd, float percent, int type, int x, int y)
{
   float ratio = (float)percent/(float)100.0;
   HWND handle1 = (HWND)dw_window_get_data(hwnd, "_dw_topleft");
   HWND handle2 = (HWND)dw_window_get_data(hwnd, "_dw_bottomright");
   Box *tmp = WinQueryWindowPtr(handle1, QWP_USER);

   WinShowWindow(handle1, FALSE);
   WinShowWindow(handle2, FALSE);

   if(type == DW_HORZ)
   {
      int newx = (int)((float)x * ratio) - (_DW_SPLITBAR_WIDTH/2);

      WinSetWindowPos(handle1, NULLHANDLE, 0, 0, newx, y, SWP_MOVE | SWP_SIZE);
      _dw_do_resize(tmp, newx - 1, y - 1);

      dw_window_set_data(hwnd, "_dw_start", (void *)newx);

      tmp = WinQueryWindowPtr(handle2, QWP_USER);

      newx = x - newx - _DW_SPLITBAR_WIDTH;

      WinSetWindowPos(handle2, NULLHANDLE, x - newx, 0, newx, y, SWP_MOVE | SWP_SIZE);
      _dw_do_resize(tmp, newx - 1, y - 1);
   }
   else
   {
      int newy = (int)((float)y * ratio) - (_DW_SPLITBAR_WIDTH/2);

      WinSetWindowPos(handle1, NULLHANDLE, 0, y - newy, x, newy, SWP_MOVE | SWP_SIZE);
      _dw_do_resize(tmp, x - 1, newy - 1);

      tmp = WinQueryWindowPtr(handle2, QWP_USER);

      newy = y - newy - _DW_SPLITBAR_WIDTH;

      WinSetWindowPos(handle2, NULLHANDLE, 0, 0, x, newy, SWP_MOVE | SWP_SIZE);
      _dw_do_resize(tmp, x - 1, newy - 1);

      dw_window_set_data(hwnd, "_dw_start", (void *)newy);
   }

   WinShowWindow(handle1, TRUE);
   WinShowWindow(handle2, TRUE);
}


/* This handles any activity on the splitbars (sizers) */
MRESULT EXPENTRY _dw_splitwndproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
   case WM_ACTIVATE:
   case WM_SETFOCUS:
      return (MRESULT)(FALSE);

   case WM_PAINT:
      {
         HPS hps;
         POINTL ptl[2];
         RECTL rcl;
         int type = (int)dw_window_get_data(hwnd, "_dw_type");
         int start = (int)dw_window_get_data(hwnd, "_dw_start");

         hps = WinBeginPaint(hwnd, 0, 0);

         WinQueryWindowRect(hwnd, &rcl);

         if(type == DW_HORZ)
         {
            ptl[0].x = rcl.xLeft + start;
            ptl[0].y = rcl.yBottom;
            ptl[1].x = rcl.xRight + start + 3;
            ptl[1].y = rcl.yTop;
         }
         else
         {
            ptl[0].x = rcl.xLeft;
            ptl[0].y = rcl.yBottom + start;
            ptl[1].x = rcl.xRight;
            ptl[1].y = rcl.yTop + start + 3;
         }


         GpiSetColor(hps, CLR_PALEGRAY);
         GpiMove(hps, &ptl[0]);
         GpiBox(hps, DRO_OUTLINEFILL, &ptl[1], 0, 0);
         WinEndPaint(hps);
      }
      return MRFROMSHORT(FALSE);

   case WM_MOUSEMOVE:
      {
         int type = (int)dw_window_get_data(hwnd, "_dw_type");

         if(type == DW_HORZ)
            WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                     SPTR_SIZEWE,
                                     FALSE));
         else
            WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                     SPTR_SIZENS,
                                     FALSE));
      }
      return MRFROMSHORT(FALSE);
   case WM_BUTTON1DOWN:
      {
         APIRET rc;
         RECTL  rclFrame;
         RECTL  rclBounds;
         float *percent = (float *)dw_window_get_data(hwnd, "_dw_percent");
         int type = (int)dw_window_get_data(hwnd, "_dw_type");
         int start = (int)dw_window_get_data(hwnd, "_dw_start");

         WinQueryWindowRect(hwnd, &rclFrame);
         WinQueryWindowRect(hwnd, &rclBounds);

         WinMapWindowPoints(hwnd, HWND_DESKTOP,
                        (PPOINTL)&rclBounds, 2);


         if(type == DW_HORZ)
         {
            rclFrame.xLeft = start;
            rclFrame.xRight = start + _DW_SPLITBAR_WIDTH;
         }
         else
         {
            rclFrame.yBottom = start;
            rclFrame.yTop = start + _DW_SPLITBAR_WIDTH;
         }

         if(percent)
         {
            rc = _dw_track_rectangle(hwnd, &rclFrame, &rclBounds);

            if(rc == TRUE)
            {
               int width = (rclBounds.xRight - rclBounds.xLeft);
               int height = (rclBounds.yTop - rclBounds.yBottom);

               if(type == DW_HORZ)
               {
                  start = rclFrame.xLeft - rclBounds.xLeft;
                  if(width - _DW_SPLITBAR_WIDTH > 1 && start < width - _DW_SPLITBAR_WIDTH)
                     *percent = ((float)start / (float)(width - _DW_SPLITBAR_WIDTH)) * 100.0;
               }
               else
               {
                  start = rclFrame.yBottom - rclBounds.yBottom;
                  if(height - _DW_SPLITBAR_WIDTH > 1 && start < height - _DW_SPLITBAR_WIDTH)
                     *percent = 100.0 - (((float)start / (float)(height - _DW_SPLITBAR_WIDTH)) * 100.0);
               }
               _dw_handle_splitbar_resize(hwnd, *percent, type, width, height);
               _dw_handle_splitbar_resize(hwnd, *percent, type, width, height);
            }
         }
      }
      return MRFROMSHORT(FALSE);
   }
   return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY _dw_button_draw(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2, PFNWP oldproc, int indent)
{
   HPIXMAP pixmap = (HPIXMAP)dw_window_get_data(hwnd, "_dw_hpixmap");
   HPIXMAP disable = (HPIXMAP)dw_window_get_data(hwnd, "_dw_hpixmap_disabled");
   HPOINTER icon = (HPOINTER)dw_window_get_data(hwnd, "_dw_button_icon");
   MRESULT res;
   unsigned long width, height;
   int x = 5, y = 5;
   ULONG style = WinQueryWindowULong(hwnd, QWL_STYLE);

   dw_window_get_pos_size(hwnd, NULL, NULL, &width, &height);

   if(!oldproc)
      res = WinDefWindowProc(hwnd, msg, mp1, mp2);
   res = oldproc(hwnd, msg, mp1, mp2);

   if(icon)
   {
      ULONG halftone = DP_NORMAL;
      HPS hps = WinGetPS(hwnd);
      POINTERINFO pi;
      int cx, cy;

      if(dw_window_get_data(hwnd, "_dw_disabled"))
         halftone = DP_HALFTONED;

      /* If there is a border take that into account */
      if(style & BS_NOBORDER)
      {
          cx = width;
          cy = height;
      }
      else
      {
          cx = width - 8;
          cy = height - 8;
      }

      if(WinQueryPointerInfo(icon, &pi))
      {
         BITMAPINFOHEADER sl;
         int newcx = cx, newcy = cy;

         sl.cbFix = sizeof(BITMAPINFOHEADER);

         /* Check the mini icon first */
         if(GpiQueryBitmapParameters(pi.hbmMiniColor, &sl))
         {
             if(sl.cx && sl.cy && cx > sl.cx && cy > sl.cy)
             {
                 newcx = sl.cx;
                 newcy = sl.cy;
             }
         }
         /* Check the normal icon second */
         if(GpiQueryBitmapParameters(pi.hbmColor, &sl))
         {
             if(sl.cx && sl.cy)
             {
                 if(cx > sl.cx && cy > sl.cy)
                 {
                     newcx = sl.cx;
                     newcy = sl.cy;
                 }
                 /* In case there was no mini icon... cut it in half */
                 else if(cx >= (sl.cx/2) && cy >= (sl.cy/2))
                 {
                     newcx = sl.cx/2;
                     newcy = sl.cy/2;
                 }
             }
         }
         cx = newcx; cy = newcy;
         /* Safety check to avoid icon dimension stretching */
         if(cx != cy)
         {
             if(cx > cy)
                 cx = cy;
             else
                 cy = cx;
         }
         /* Finally center it in the window */
         x = (width - cx)/2;
         y = (height - cy)/2;
      }
      WinStretchPointer(hps, x + indent, y - indent, cx, cy, icon, halftone);
      WinReleasePS(hps);
   }
   else if(pixmap)
   {
      x = (width - pixmap->width)/2;
      y = (height - pixmap->height)/2;

      if(disable && dw_window_get_data(hwnd, "_dw_disabled"))
         dw_pixmap_bitblt(hwnd, 0, x + indent, y + indent, pixmap->width, pixmap->height, 0, disable, 0, 0);
      else
         dw_pixmap_bitblt(hwnd, 0, x + indent, y + indent, pixmap->width, pixmap->height, 0, pixmap, 0, 0);
   }
   return res;
}

/* Function: BtProc
 * Abstract: Subclass procedure for buttons
 */

MRESULT EXPENTRY _dw_buttonproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = WinQueryWindowPtr(hwnd, QWL_USER);
   PFNWP oldproc;
   int retval = -1;

   if(!blah)
      return WinDefWindowProc(hwnd, msg, mp1, mp2);

   oldproc = blah->oldproc;

   if(blah->bubbletext[0])
       _dw_tooltipproc(hwnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hwnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_PAINT:
      return _dw_button_draw(hwnd, msg, mp1, mp2, oldproc, 0);
   case BM_SETHILITE:
      return _dw_button_draw(hwnd, msg, mp1, mp2, oldproc, (int)mp1);
   case WM_SETFOCUS:
      if(mp2)
         _dw_run_event(hwnd, msg, mp1, mp2);
      else
          WinSendMsg(hwnd, BM_SETDEFAULT, 0, 0);
      /*  FIX: Borderless buttons not displaying properly after gaining focus */
      if((WinQueryWindowULong(hwnd, QWL_STYLE) & BS_NOBORDER))
      {
          RECTL rcl;

          WinQueryWindowRect(hwnd, &rcl);

          WinInvalidateRect(hwnd, &rcl, FALSE);
          WinPostMsg(hwnd, WM_PAINT, 0, 0);
      }
      break;
   case WM_BUTTON1DOWN:
   case WM_BUTTON2DOWN:
   case WM_BUTTON3DOWN:
   case WM_BUTTON1DBLCLK:
   case WM_BUTTON2DBLCLK:
   case WM_BUTTON3DBLCLK:
      if(dw_window_get_data(hwnd, "_dw_disabled"))
         return (MRESULT)FALSE;
      break;
   case WM_BUTTON1UP:
      {
         DWSignalHandler *tmp = Root;

         if(WinIsWindowEnabled(hwnd) && !dw_window_get_data(hwnd, "_dw_disabled"))
         {
            /* Find any callbacks for this function */
            while(tmp)
            {
               if(tmp->message == WM_COMMAND)
               {
                  /* Make sure it's the right window, and the right ID */
                  if(tmp->window == hwnd)
                  {
                     /* Due to the fact that if we run the function
                      * here, finishing actions on the button will occur
                      * after we run the signal handler.  So we post the
                      * message so the button can finish what it needs to
                      * do before we run our handler.
                      */
                     WinPostMsg(hwnd, WM_USER, (MPARAM)tmp, 0);
                     tmp = NULL;
                  }
               }
               if(tmp)
                  tmp= tmp->next;
            }
         }
      }
      break;
   case WM_USER:
      {
         DWSignalHandler *tmp = (DWSignalHandler *)mp1;
         int (API_FUNC clickfunc)(HWND, void *) = NULL;

         if(tmp)
         {
            clickfunc = (int (API_FUNC)(HWND, void *))tmp->signalfunction;

            retval = clickfunc(tmp->window, tmp->data);
         }
      }
        break;
   case WM_CHAR:
      {
         /* A button press should also occur for an ENTER or SPACE press
          * while the button has the active input focus.
          */
         if(SHORT1FROMMP(mp2) == '\r' || SHORT1FROMMP(mp2) == ' ')
         {
            DWSignalHandler *tmp = Root;

            /* Find any callbacks for this function */
            while(tmp)
            {
               if(tmp->message == WM_COMMAND)
               {
                  /* Make sure it's the right window, and the right ID */
                  if(tmp->window == hwnd)
                  {
                     WinPostMsg(hwnd, WM_USER, (MPARAM)tmp, 0);
                     tmp = NULL;
                  }
               }
               if(tmp)
                  tmp= tmp->next;
            }
         }
         if(SHORT1FROMMP(mp2) == '\t')
         {
            if(CHARMSG(&msg)->fs & KC_SHIFT)
               _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
            else
               _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
            WinSendMsg(hwnd, BM_SETDEFAULT, 0, 0);
            return FALSE;
         }
         else if(!(CHARMSG(&msg)->fs & KC_KEYUP) && (CHARMSG(&msg)->vkey == VK_LEFT || CHARMSG(&msg)->vkey == VK_UP))
         {
            _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
            return FALSE;
         }
         else if(!(CHARMSG(&msg)->fs & KC_KEYUP) && (CHARMSG(&msg)->vkey == VK_RIGHT || CHARMSG(&msg)->vkey == VK_DOWN))
         {
            _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
            return FALSE;
         }
      }
      break;
   }

   /* Make sure windows are up-to-date */
   if(retval != -1)
      _dw_redraw(0, FALSE);
   if(!oldproc)
      return WinDefWindowProc(hwnd, msg, mp1, mp2);
   return oldproc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY _RendProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hwnd, QWP_USER);
   int res = (int)_dw_run_event(hwnd, msg, mp1, mp2);

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hwnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hwnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_BUTTON1DOWN:
   case WM_BUTTON2DOWN:
   case WM_BUTTON3DOWN:
      if(res == -1)
          WinSetFocus(HWND_DESKTOP, hwnd);
      else if(res)
         return (MPARAM)TRUE;
   }
   return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY _dw_treeproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hwnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
      oldproc = blah->oldproc;

   if(blah && blah->bubbletext[0])
       _dw_tooltipproc(hwnd, msg, mp1, mp2, blah);

   switch(msg)
   {
   case WM_MOUSEMOVE:
      if(_dw_wndproc(hwnd, msg, mp1, mp2))
         return MPFROMSHORT(FALSE);
      break;
   case WM_PAINT:
      {
         HPS hps;
         RECTL rcl;
         POINTL ptl[2];

         if(oldproc)
            oldproc(hwnd, msg, mp1, mp2);

         hps = WinBeginPaint(hwnd, 0, 0);
         WinQueryWindowRect(hwnd, &rcl);
         ptl[0].x = rcl.xLeft + 1;
         ptl[0].y = rcl.yBottom + 1;
         ptl[1].x = rcl.xRight - 1;
         ptl[1].y = rcl.yTop - 1;

         GpiSetColor(hps, CLR_BLACK);
         GpiMove(hps, &ptl[0]);
         GpiBox(hps, DRO_OUTLINE, &ptl[1], 0, 0);
         WinEndPaint(hps);
      }
      return MRFROMSHORT(FALSE);
   case WM_SETFOCUS:
      _dw_run_event(hwnd, msg, mp1, mp2);
      break;
   case WM_CHAR:
      if(SHORT1FROMMP(mp2) == '\t')
      {
         if(CHARMSG(&msg)->fs & KC_SHIFT)
            _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      break;
   }

   _dw_run_event(hwnd, msg, mp1, mp2);

   if(oldproc)
      return oldproc(hwnd, msg, mp1, mp2);

   return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY _dw_notebookproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(hwnd, QWP_USER);
   PFNWP oldproc = 0;

   if(blah)
      oldproc = blah->oldproc;

   switch(msg)
   {
   case WM_PAINT:
      if(!WinSendMsg(hwnd, BKM_QUERYPAGECOUNT, 0, (MPARAM)BKA_END))
      {
         HPS hpsPaint;
         RECTL rclPaint;

         hpsPaint = WinBeginPaint(hwnd, 0, &rclPaint);
         WinFillRect(hpsPaint, &rclPaint, CLR_PALEGRAY);
         WinEndPaint(hpsPaint);
      }
      break;
   case WM_CHAR:
      if(SHORT1FROMMP(mp2) == '\t')
      {
         if(CHARMSG(&msg)->fs & KC_SHIFT)
            _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      break;
   }

   if(oldproc)
      return oldproc(hwnd, msg, mp1, mp2);

   return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

#ifdef UNICODE
/* Internal function to detect the active keyboard layout */
UniChar *_dw_detect_keyb(void)
{
    HFILE handle;
    struct
    {
        USHORT length;
        USHORT codepage;
        CHAR   strings[8];
    } kd;
    ULONG action;
    UniChar *buf = NULL;

    if(DosOpen((PSZ)"KBD$", &handle, &action, 0, 0,
               OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE,
               NULL) == 0)
    {
        ULONG plen = 0, dlen = sizeof(kd);

        kd.length = dlen;

        if(DosDevIOCtl(handle, 4, 0x7b, NULL, plen, &plen,
                       &kd, dlen, &dlen) == 0 && strlen(kd.strings) > 0)
        {

            /* Convert to Unicode */
            buf = _dw_UTF8toWide(kd.strings);
        }
        DosClose (handle);
    }
    return buf;
}
#endif

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
   APIRET rc;
   char objnamebuf[300] = "";
   int x;
   struct tm thistm = { 0 };

   /* Setup the private data directory */
   if(argc > 0 && argv[0])
   {
      char *pos = strrchr(argv[0], '\\');

      /* Just to be safe try the unix style */
      if(!pos)
         pos = strrchr(argv[0], '/');

      if(pos)
         strncpy(_dw_exec_dir, argv[0], (size_t)(pos - argv[0]));
   }
   /* If that failed... just get the current directory */
   if(!_dw_exec_dir[0])
      _getcwd(_dw_exec_dir, MAX_PATH);

   if(newthread)
   {
#ifdef UNICODE
      UniChar *kbd;
      UniChar  suCodepage[MAX_CP_SPEC];    /* conversion specifier */
#endif
      dwhab = WinInitialize(0);
      dwhmq = WinCreateMsgQueue(dwhab, 0);
#ifdef UNICODE
      /* Create the conversion object */
      UniMapCpToUcsCp(1208, suCodepage, MAX_CP_NAME);
      UniStrcat(suCodepage, (UniChar *) L"@map=cdra,path=no");
      UniCreateUconvObject(suCodepage, &Uconv);
      /* Create the Unicode atom for copy and paste */
      Unicode = WinAddAtom(WinQuerySystemAtomTable(), (PSZ)"text/unicode");
      /* Figure out how to determine the correct keyboard here */
      kbd = _dw_detect_keyb();
      /* Default to US if could not detect */
      UniCreateKeyboard(&Keyboard, (UniChar *)kbd ? kbd : L"us", 0);
      /* Free temporary memory */
      if(kbd)
          free(kbd);
      /* Set the codepage to 1208 (UTF-8) */
      WinSetCp(dwhmq, 1208);
#endif
   }

   rc = WinRegisterClass(dwhab, (PSZ)ClassName, _dw_wndproc, CS_SIZEREDRAW | CS_CLIPCHILDREN, 32);
   rc = WinRegisterClass(dwhab, (PSZ)SplitbarClassName, _dw_splitwndproc, 0L, 32);
   rc = WinRegisterClass(dwhab, (PSZ)ScrollClassName, _dw_scrollwndproc, 0L, 32);
   rc = WinRegisterClass(dwhab, (PSZ)CalendarClassName, _dw_calendarproc, 0L, 32);

   /* Fill in the the calendar fields */
   for(x=0;x<7;x++)
   {
       thistm.tm_wday = x;
       strftime(daysofweek[x], 19, "%A", &thistm);
   }
   for(x=0;x<12;x++)
   {
       thistm.tm_mon = x;
       strftime(months[x], 19, "%B", &thistm);
   }

   /* Get the OS/2 version. */
   DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_MS_COUNT,(void *)_dw_ver_buf, 4*sizeof(ULONG));

   _dw_desktop = WinQueryDesktopWindow(dwhab, NULLHANDLE);

   if(!IS_WARP4())
      DefaultFont = strdup("8.Helv");
   else
      DefaultFont = strdup(DefaultFont);

   /* This is a window that hangs around as long as the
    * application does and handles menu messages.
    */
   _dw_app = dw_window_new(HWND_OBJECT, "", 0);
   /* Attempt to locate a tray server */
   WinDdeInitiate(_dw_app, (PSZ)"SystrayServer", (PSZ)"TRAY", NULL);

   /* Load DLLs for providing extra functionality if available */
   DosLoadModule((PSZ)objnamebuf, sizeof(objnamebuf), (PSZ)"WPCONFIG", &_dw_wpconfig);
   if(!DosLoadModule((PSZ)objnamebuf, sizeof(objnamebuf), (PSZ)"PMPRINTF", &_dw_pmprintf))
       DosQueryProcAddr(_dw_pmprintf, 0, (PSZ)"PmPrintfString", (PFN*)&_PmPrintfString);
   if(!DosLoadModule((PSZ)objnamebuf, sizeof(objnamebuf), (PSZ)"PMMERGE", &_dw_pmmerge))
       DosQueryProcAddr(_dw_pmmerge, 5469, NULL, (PFN*)&_WinQueryDesktopWorkArea);
   if(!DosLoadModule((PSZ)objnamebuf, sizeof(objnamebuf), (PSZ)"GBM", &_dw_gbm))
   {
       /* Load the _System versions of the functions from the library */
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_err", (PFN*)&_gbm_err);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_init", (PFN*)&_gbm_init);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_deinit", (PFN*)&_gbm_deinit);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_io_open", (PFN*)&_gbm_io_open);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_io_close", (PFN*)&_gbm_io_close);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_read_data", (PFN*)&_gbm_read_data);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_read_header", (PFN*)&_gbm_read_header);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_read_palette", (PFN*)&_gbm_read_palette);
       DosQueryProcAddr(_dw_gbm, 0, (PSZ)"Gbm_query_n_filetypes", (PFN*)&_gbm_query_n_filetypes);
       /* If we got the functions, try to initialize the library */
       if(!_gbm_init || _gbm_init())
       {
           /* Otherwise clear out the function pointers */
           _gbm_init=0;_gbm_deinit=0;_gbm_io_open=0;_gbm_io_close=0;_gbm_query_n_filetypes=0;
           _gbm_read_header=0;_gbm_read_palette=0;_gbm_read_data=0;_gbm_err=0;
       }
   }
   return rc;
}

static int _dw_main_running = FALSE;

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
   QMSG qmsg;

   _dwtid = dw_thread_id();
   /* Make sure any queued redraws are handled */
   _dw_redraw(0, FALSE);

   /* Set the running flag to TRUE */
   _dw_main_running = TRUE;

   /* Run the loop until the flag is unset... or error */
   while(_dw_main_running && WinGetMsg(dwhab, &qmsg, 0, 0, 0))
   {
      if(qmsg.msg == WM_TIMER && qmsg.hwnd == NULLHANDLE)
         _dw_run_event(qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
      WinDispatchMsg(dwhab, &qmsg);
   }
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
    _dw_main_running = FALSE;
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
   QMSG qmsg;
#ifdef __EMX__
   struct timeval tv, start;

   gettimeofday(&start, NULL);
   gettimeofday(&tv, NULL);

   while(((tv.tv_sec - start.tv_sec)*1000) + ((tv.tv_usec - start.tv_usec)/1000) <= milliseconds)
   {
      if(WinPeekMsg(dwhab, &qmsg, 0, 0, 0, PM_NOREMOVE))
      {
         WinGetMsg(dwhab, &qmsg, 0, 0, 0);
         if(qmsg.msg == WM_TIMER && qmsg.hwnd == NULLHANDLE)
            _dw_run_event(qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
         WinDispatchMsg(dwhab, &qmsg);
      }
      else
         DosSleep(1);
      gettimeofday(&tv, NULL);
   }
#else
   double start = (double)clock();

   while(((clock() - start) / (CLOCKS_PER_SEC/1000)) <= milliseconds)
   {
      if(WinPeekMsg(dwhab, &qmsg, 0, 0, 0, PM_NOREMOVE))
      {
         WinGetMsg(dwhab, &qmsg, 0, 0, 0);
         if(qmsg.msg == WM_TIMER && qmsg.hwnd == NULLHANDLE)
            _dw_run_event(qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
         WinDispatchMsg(dwhab, &qmsg);
      }
      else
         DosSleep(1);
   }
#endif
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
   QMSG qmsg;

   _dwtid = dw_thread_id();

   if(WinGetMsg(dwhab, &qmsg, 0, 0, 0))
   {
      if(qmsg.msg == WM_TIMER && qmsg.hwnd == NULLHANDLE)
         _dw_run_event(qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
      WinDispatchMsg(dwhab, &qmsg);
   }
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
   QMSG qmsg;
   void *tmp;

   if(!dialog)
      return NULL;

   while (WinGetMsg(dwhab, &qmsg, 0, 0, 0))
   {
      if(qmsg.msg == WM_TIMER && qmsg.hwnd == NULLHANDLE)
         _dw_run_event(qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
      WinDispatchMsg(dwhab, &qmsg);
      if(dialog->done)
         break;
   }
   dw_event_close(&dialog->eve);
   tmp = dialog->result;
   free(dialog);
   return tmp;
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
   vfprintf(stderr, format, args);
   va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
   char outbuf[1025] = { 0 };

#if defined(__IBMC__)
   vsprintf(outbuf, format, args);
#else
   vsnprintf(outbuf, 1024, format, args);
#endif

   if(_PmPrintfString)
   {
       int len = strlen(outbuf);

       /* Trim off trailing newline for PMPrintf */
       if(len > 0 && outbuf[len-1] == '\n')
           outbuf[len-1] = 0;
       _PmPrintfString(outbuf);
   }
   else
       fprintf(stderr, "%s", outbuf);
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
   char outbuf[1025] = { 0 };
   int rc;

#if defined(__IBMC__)
   vsprintf(outbuf, format, args);
#else
   vsnprintf(outbuf, 1024, format, args);
#endif

   rc = WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, (PSZ)outbuf, (PSZ)title, 0, flags | MB_MOVEABLE);
   if(rc == MBID_OK)
      return DW_MB_RETURN_OK;
   else if(rc == MBID_YES)
      return DW_MB_RETURN_YES;
   else if(rc == MBID_NO)
      return DW_MB_RETURN_NO;
   else if(rc == MBID_CANCEL)
      return DW_MB_RETURN_CANCEL;
   else return 0;
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
   return WinSetWindowPos(handle, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
   return WinSetWindowPos(handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_ZORDER);
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
   int rc = WinShowWindow(handle, TRUE);
   HSWITCH hswitch;
   SWCNTRL swcntrl;

   _dw_fix_button_owner(_dw_toplevel_window(handle), 0);
   WinSetFocus(HWND_DESKTOP, handle);
   _dw_initial_focus(handle);

   /* If this window has a  switch list entry make sure it is visible */
   hswitch = WinQuerySwitchHandle(handle, 0);
   if(hswitch)
   {
      WinQuerySwitchEntry(hswitch, &swcntrl);
      swcntrl.uchVisibility = SWL_VISIBLE;
      WinChangeSwitchEntry(hswitch, &swcntrl);
   }
   if(WinWindowFromID(handle, FID_CLIENT))
   {
      WindowData *blah = WinQueryWindowPtr(handle, QWP_USER);

      if(blah && !(blah->flags & DW_OS2_NEW_WINDOW))
      {
         /* Handle auto-positioning and auto-sizing */
         ULONG cx = dw_screen_width(), cy = dw_screen_height();
         HWND parent = WinQueryWindow(handle, QW_PARENT);
         int newx, newy, changed = 0;
         SWP swp;

         /* If it is an MDI window...
          * find the MDI area.
          */
         if(parent && parent != _dw_desktop)
         {
             WinQueryWindowPos(parent, &swp);
             cx = swp.cx;
             cy = swp.cy;
             /* If the MDI parent isn't visible...
              * we can't calculate. Drop out.
              */
             if(cx < 1 || cy < 1)
             {
                 WinSetWindowPos(handle, NULLHANDLE, 0, 0, 0, 0, SWP_MOVE);
                 return rc;
             }
         }

         blah->flags |= DW_OS2_NEW_WINDOW;

         WinQueryWindowPos(handle, &swp);

         /* If the size is 0 then auto-size */
         if(swp.cx == 0 || swp.cy == 0)
         {
             dw_window_set_size(handle, 0, 0);
             WinQueryWindowPos(handle, &swp);
         }

         /* If the position was not set... generate a default
          * default one in a similar pattern to SHELLPOSITION.
          */
         if(swp.x == -2000 || swp.y == -2000)
         {
             static int defaultx = 0, defaulty = 0;
             int maxx = cx / 4, maxy = cy / 4;

             defaultx += 20;
             defaulty += 20;

             if(defaultx > maxx)
                 defaultx = 20;
             if(defaulty > maxy)
                 defaulty = 20;

             newx = defaultx;
             /* Account for flipped Y */
             newy = cy - defaulty - swp.cy;
             changed = 1;
         }
         else
         {
             newx = swp.x;
             newy = swp.y;
         }

         /* Make sure windows shown for the first time are
          * completely visible if possible.
          */
         if(swp.cx < cx && (newx+swp.cx) > cx)
         {
            newx = (cx - swp.cx)/2;
            changed = 1;
         }
         if(swp.cy < cy && (newy+swp.cy) > cy)
         {
            newy = (cy - swp.cy)/2;
            changed = 1;
         }

         if(changed)
            WinSetWindowPos(handle, NULLHANDLE, newx, newy, 0, 0, SWP_MOVE);
      }
   }
   return rc;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int API dw_window_minimize(HWND handle)
{
   HWND hwndclient = WinWindowFromID(handle, FID_CLIENT);

   if(hwndclient)
   {
      Box *box = (Box *)WinQueryWindowPtr(hwndclient, QWP_USER);

      if(box)
      {
         if(!box->titlebar && box->hwndtitle)
            WinSetParent(box->hwndtitle, handle, FALSE);
      }
   }

   return WinSetWindowPos(handle, NULLHANDLE, 0, 0, 0, 0, SWP_MINIMIZE);
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_hide(HWND handle)
{
   HSWITCH hswitch;
   SWCNTRL swcntrl;

   /* If this window has a  switch list entry make sure it is invisible */
   hswitch = WinQuerySwitchHandle(handle, 0);
   if(hswitch)
   {
      WinQuerySwitchEntry(hswitch, &swcntrl);
      swcntrl.uchVisibility = SWL_INVISIBLE;
      WinChangeSwitchEntry(hswitch, &swcntrl);
   }
   return WinShowWindow(handle, FALSE);
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int API dw_window_destroy(HWND handle)
{
   HWND frame, menu, parent;

   if(!handle)
      return DW_ERROR_UNKNOWN;

   /* Handle special case for menu handle */
   if(handle < 65536)
   {
      char buffer[30];

      sprintf(buffer, "_dw_id%ld", handle);
      menu = (HWND)dw_window_get_data(_dw_app, buffer);

      if(menu && WinIsWindow(dwhab, menu))
          return dw_menu_delete_item((HMENUI)menu, handle);
      return DW_ERROR_UNKNOWN;
   }

   parent = WinQueryWindow(handle, QW_PARENT);
   frame = (HWND)dw_window_get_data(handle, "_dw_combo_box");

   if((menu = WinWindowFromID(handle, FID_MENU)) != NULLHANDLE)
      _dw_free_menu_data(menu);

   /* If it is a desktop window let WM_DESTROY handle it */
   if(parent != _dw_desktop)
   {
      dw_box_unpack(handle);
      _dw_free_window_memory(frame ? frame : handle);
   }
   return WinDestroyWindow(frame ? frame : handle);
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
   HWND client = WinWindowFromID(handle, FID_CLIENT);
   HWND window = client ? client : handle;
   Box *mybox = (Box *)WinQueryWindowPtr(window, QWP_USER);

   _dw_fix_button_owner(_dw_toplevel_window(handle), 0);
   if(window && mybox)
   {
      unsigned long width, height;

      dw_window_get_pos_size(window, NULL, NULL, &width, &height);

      if(mybox->items)
          WinSetWindowPos(mybox->items[0].hwnd, HWND_TOP, 0, 0, width, height, SWP_MOVE | SWP_SIZE);

      WinShowWindow(client && mybox->items ? mybox->items[0].hwnd : handle, FALSE);
      _dw_do_resize(mybox, width, height);
      WinShowWindow(client && mybox->items ? mybox->items[0].hwnd : handle, TRUE);
   }
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
void API dw_render_redraw(HWND handle)
{
   WinInvalidateRect(handle, NULL, FALSE);
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
   HWND blah = WinWindowFromID(newparent, FID_CLIENT);
   WinSetParent(handle, blah ? blah : newparent, TRUE);
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
    FONTDLG fd = { 0 };
    char *buf = calloc(1,100);
    int size = 9;

    /* Fill in the family name if possible */
    if(currfont)
    {
        char *name = strchr(currfont, '.');
        if(name)
        {
            int newsize = atoi(currfont);
            if(newsize > 0)
                size = newsize;
            name++;
            strcpy(buf, name);
            strcpy(fd.fAttrs.szFacename, name);
        }
        else
        {
            strcpy(buf, currfont);
            strcpy(fd.fAttrs.szFacename, currfont);
        }
    }
#ifdef UNICODE
    fd.fAttrs.usCodePage = 1208;
#endif
    fd.fAttrs.usRecordLength = sizeof(FATTRS);

    /* Fill in the font dialog struct */
    fd.cbSize = sizeof(fd);
    fd.hpsScreen = WinGetScreenPS(HWND_DESKTOP);
    fd.pszTitle = (PSZ)"Choose Font";
    fd.clrFore = CLR_BLACK;
    fd.clrBack = CLR_WHITE;
    fd.pszFamilyname = (PSZ)buf;
    fd.usFamilyBufLen = 100;
    fd.fxPointSize = MAKEFIXED(size,0);
    fd.fl = FNTS_INITFROMFATTRS;

    /* Show the dialog and wait for a response */
    if(!WinFontDlg(HWND_DESKTOP, HWND_OBJECT, &fd) || fd.lReturn != DID_OK)
    {
        WinReleasePS(fd.hpsScreen);
        free(buf);
        return NULL;
    }
    WinReleasePS(fd.hpsScreen);
    /* Figure out what the user selected and return that */
    size = FIXEDINT(fd.fxPointSize);
    sprintf(buf, "%d.%s", size, fd.fAttrs.szFacename);
    return buf;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
    char *oldfont = DefaultFont;

    DefaultFont = fontname ? strdup(fontname) : NULL;
    if(oldfont)
        free(oldfont);
}

/* Internal function to return a pointer to an item struct
 * with information about the packing information regarding object.
 */
Item *_dw_box_item(HWND handle)
{
   HWND parent = WinQueryWindow(handle, QW_PARENT);
   Box *thisbox = (Box *)WinQueryWindowPtr(parent, QWP_USER);

   /* If it is a desktop window let WM_DESTROY handle it */
   if(parent != HWND_DESKTOP)
   {
      if(thisbox && thisbox->count)
      {
         int z;
         Item *thisitem = thisbox->items;

         for(z=0;z<thisbox->count;z++)
         {
            if(thisitem[z].hwnd == handle)
               return &thisitem[z];
         }
      }
   }
   return NULL;
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
void _dw_control_size(HWND handle, int *width, int *height)
{
   int thiswidth = 1, thisheight = 1, extrawidth = 0, extraheight = 0;
   char tmpbuf[100] = {0}, *buf = dw_window_get_text(handle);
   static char testtext[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

    /* If we have a string...
     * calculate the size with the current font.
     */
    if(buf)
    {
       if(*buf)
          dw_font_text_extents_get(handle, NULL, buf, &thiswidth, &thisheight);
       dw_free(buf);
    }

   /* Combobox */
   if(strncmp(tmpbuf, "#2", 3)==0)
   {
      dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
      thiswidth = 150;
      extraheight = 4;
      if(thisheight < 18)
        thisheight = 18;
   }
   /* Calendar */
   else if(strncmp(tmpbuf, CalendarClassName, strlen(CalendarClassName)+1)==0)
   {
       thiswidth = 200;
       thisheight = 150;
   }
   /* Bitmap/Static */
   else if(strncmp(tmpbuf, "#5", 3)==0)
   {
       HBITMAP hbm = (HBITMAP)dw_window_get_data(handle, "_dw_bitmap");

       /* If we got a bitmap handle */
       if(hbm)
       {
            BITMAPINFOHEADER2 bmp;
            bmp.cbFix = sizeof(BITMAPINFOHEADER2);
            /* Get the parameters of the bitmap */
            if(GpiQueryBitmapInfoHeader(hbm, &bmp))
            {
               thiswidth = bmp.cx;
               thisheight = bmp.cy;
            }
       }
       else
       {
            if(thiswidth == 1 && thisheight == 1)
               dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
            if(dw_window_get_data(handle, "_dw_status"))
            {
               extrawidth = 4;
               extraheight = 4;
            }
       }
   }
   /* Ranged: Slider/Percent */
   else if(strncmp(tmpbuf, "#38", 4)== 0)
   {
       thiswidth = 100;
       thisheight = 20;
   }
   /* Scrollbar */
   else if(strncmp(tmpbuf, "#8", 3)== 0)
   {
      /* Check for vertical scrollbar */
      if(WinQueryWindowULong(handle, QWL_STYLE) & SBS_VERT)
      {
         thiswidth = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
         thisheight = 100;
      }
      else
      {
         thiswidth = 100;
         thisheight = WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);
      }
   }
   /* Spinbutton */
   else if(strncmp(tmpbuf, "#32", 4)==0)
   {
      dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
      thiswidth = 50;
      extraheight = 6;
   }
   /* Entryfield */
   else if(strncmp(tmpbuf, "#6", 3)==0)
   {
      dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
      thiswidth = 150;
      extraheight = 6;
   }
   /* MLE */
   else if(strncmp(tmpbuf, "#10", 4)==0)
   {
       unsigned long bytes;
       int height, width;
       char *buf, *ptr;
       int basicwidth;
       int wrap = (int)WinSendMsg(handle, MLM_QUERYWRAP, 0,0);

       thisheight = 8;
       basicwidth = thiswidth = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL) + 8;

       dw_mle_get_size(handle, &bytes, NULL);

       ptr = buf = alloca(bytes + 2);
       dw_mle_export(handle, buf, 0, (int)bytes);
       buf[bytes] = 0;
       strcat(buf, "\n");

       /* MLE */
       while((ptr = strstr(buf, "\n")) != NULL)
       {
           ptr[0] = 0;
           width = 0;
           if(strlen(buf))
               dw_font_text_extents_get(handle, NULL, buf, &width, &height);
           else
               dw_font_text_extents_get(handle, NULL, testtext, NULL, &height);

           width += basicwidth;

           if(wrap && width > _DW_SCROLLED_MAX_WIDTH)
           {
               thiswidth = _DW_SCROLLED_MAX_WIDTH;
               thisheight += height * (width / _DW_SCROLLED_MAX_WIDTH);

           }
           else
           {
               if(width > thiswidth)
                   thiswidth = width > _DW_SCROLLED_MAX_WIDTH ? _DW_SCROLLED_MAX_WIDTH : width;
           }
           thisheight += height;
           buf = &ptr[1];
       }

       if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
           thiswidth = _DW_SCROLLED_MIN_WIDTH;
       if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
           thisheight = _DW_SCROLLED_MIN_HEIGHT;
       if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
           thisheight = _DW_SCROLLED_MAX_HEIGHT;
   }
   /* Listbox */
   else if(strncmp(tmpbuf, "#7", 3)==0)
   {
      char buf[1025] = {0};
      int x, count = dw_listbox_count(handle);
      int basicwidth = thiswidth = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL) + 8;

      thisheight = 8;

      for(x=0;x<count;x++)
      {
         int height, width = 0;

         dw_listbox_get_text(handle, x, buf, 1024);

         if(strlen(buf))
            dw_font_text_extents_get(handle, NULL, buf, &width, &height);
         else
            dw_font_text_extents_get(handle, NULL, testtext, NULL, &height);

         width += basicwidth;

         if(width > thiswidth)
            thiswidth = width > _DW_SCROLLED_MAX_WIDTH ? _DW_SCROLLED_MAX_WIDTH : width;
         thisheight += height;
      }

      if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
         thiswidth = _DW_SCROLLED_MIN_WIDTH;
      if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
         thisheight = _DW_SCROLLED_MIN_HEIGHT;
      if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
         thisheight = _DW_SCROLLED_MAX_HEIGHT;
   }
   /* Container and Tree */
   else if(strncmp(tmpbuf, "#37", 4)==0)
   {
       /* Container */
       if(dw_window_get_data(handle, "_dw_container"))
       {
           CNRINFO ci;

           if(WinSendMsg(handle, CM_QUERYCNRINFO, MPFROMP(&ci), MPFROMSHORT(sizeof(CNRINFO))))
           {
               RECTL item;
               PRECORDCORE pCore = NULL;
               int right = FALSE, max = 0;
               /* Get the left title window */
               HWND title = WinWindowFromID(handle, 32752);

               thiswidth = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
               thisheight = WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);

               /* If the pFieldInfoList is filled in we want to look at the right side */
               if(ci.pFieldInfoLast)
               {
                   right = TRUE;
                   /* Left side include splitbar position */
                   thiswidth += ci.xVertSplitbar + 4;
                   /* If split... find the right side */
                   title = WinWindowFromID(handle, 32753);
               }

               /* If there are column titles ... */
               if(title)
               {
                   unsigned long height = 0;

                   dw_window_get_pos_size(title, 0, 0, 0, &height);
                   if(height)
                       thisheight += height;
                   else
                       thisheight += 28;
               }

               /* Cycle through all the records finding the maximums */
               pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

               /* Method 1: With items in container */
               if(pCore)
               {
                   while(pCore)
                   {
                       QUERYRECORDRECT qrr;
                       int vector;

                       qrr.cb = sizeof(QUERYRECORDRECT);
                       qrr.pRecord = pCore;
                       qrr.fRightSplitWindow = right;
                       qrr.fsExtent = CMA_TEXT;

                       WinSendMsg(handle, CM_QUERYRECORDRECT, (MPARAM)&item, (MPARAM)&qrr);

                       vector = item.xRight - item.xLeft;

                       if(vector > max)
                           max = vector;

                       thisheight += (item.yTop - item.yBottom);

                       pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
                   }

                   /* Add the widest item to the width */
                   thiswidth += max;
               }
               else
               {
                   /* Method 2: No items */
                   unsigned long width, height;
                   HWND hscroll = WinWindowFromID(handle, right ? 32756 : 32755);
                   MRESULT mr;

                   /* Save the original size */
                   dw_window_get_pos_size(handle, 0, 0, &width, &height);

                   /* Set the size to the minimum */
                   dw_window_set_size(handle, _DW_SCROLLED_MIN_WIDTH, _DW_SCROLLED_MIN_HEIGHT);

                   /* With the minimum size check to see what the scrollbar says */
                   mr = WinSendMsg(hscroll, SBM_QUERYRANGE, 0, 0);
                   if(right)
                       thiswidth += SHORT2FROMMP(mr);
                   else if(SHORT2FROMMP(mr) != _DW_SCROLLED_MIN_HEIGHT)
                       thiswidth += SHORT2FROMMP(mr) + _DW_SCROLLED_MIN_HEIGHT + WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);

                   /* Reload the original size */
                   dw_window_set_size(handle, width, height);
               }

               /* Clamp to min and max */
               if(thiswidth > _DW_SCROLLED_MAX_WIDTH)
                   thiswidth = _DW_SCROLLED_MAX_WIDTH;
               if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
                   thiswidth = _DW_SCROLLED_MIN_WIDTH;
               if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
                   thisheight = _DW_SCROLLED_MIN_HEIGHT;
               if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
                   thisheight = _DW_SCROLLED_MAX_HEIGHT;
           }
       }
       else
       {
           /* Tree */
           thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
           thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
       }
   }
   /* Button */
   else if(strncmp(tmpbuf, "#3", 3)==0)
   {
      ULONG style = WinQueryWindowULong(handle, QWL_STYLE);

      if(style & BS_AUTOCHECKBOX || style & BS_AUTORADIOBUTTON)
      {
         extrawidth = 24;
         extraheight = 4;
      }
      else
      {
          /* Handle bitmap buttons */
          if(dw_window_get_data(handle, "_dw_bitmapbutton"))
          {
              HPOINTER hpr = (HPOINTER)dw_window_get_data(handle, "_dw_button_icon");
              HBITMAP hbm = 0;
              int iconwidth = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_button_icon_width"));
              HPIXMAP pixmap = (HPIXMAP)dw_window_get_data(handle, "_dw_hpixmap");

              /* Handle case of icon resource */
              if(hpr)
              {
                  if(iconwidth)
                      thisheight = thiswidth = iconwidth;
                  else
                  {
                      POINTERINFO pi;

                      /* Get the internal HBITMAP handles */
                      if(WinQueryPointerInfo(hpr, &pi))
                          hbm = pi.hbmColor ? pi.hbmColor : pi.hbmPointer;
                  }
              }
              /* Handle case of pixmap resource */
              else if(pixmap)
              {
                  thiswidth = pixmap->width;
                  thisheight = pixmap->height;
              }

              /* If we didn't load it from the icon... */
              if(!hbm && !iconwidth)
              {
                  WNDPARAMS wp;
                  BTNCDATA bcd;

                  wp.fsStatus = WPM_CTLDATA;
                  wp.cbCtlData = sizeof(BTNCDATA);
                  wp.pCtlData = &bcd;

                  /* Query the button's bitmap */
                  if(WinSendMsg(handle, WM_QUERYWINDOWPARAMS, (MPARAM)&wp, MPVOID) && bcd.hImage)
                      hbm = bcd.hImage;
              }

              /* If we got a bitmap handle */
              if(hbm)
              {
                  BITMAPINFOHEADER2 bmp;
                  bmp.cbFix = sizeof(BITMAPINFOHEADER2);
                  /* Get the parameters of the bitmap */
                  if(GpiQueryBitmapInfoHeader(hbm, &bmp))
                  {
                      thiswidth = bmp.cx;
                      thisheight = bmp.cy;
                  }
              }
          }
          else
          {
              extrawidth = 4;
              extraheight = 4;
          }
          if(!(style & BS_NOBORDER))
          {
              extrawidth += 4;
              extraheight += 4;
          }
      }
   }

   /* Set the requested sizes */
   if(width)
      *width = thiswidth + extrawidth;
   if(height)
      *height = thisheight + extraheight;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, const char *fontname)
{
   HWND group = (HWND)dw_window_get_data(handle, "_dw_buddy");
   const char *font = fontname ? fontname : DefaultFont;

   /* If we changed the font... */
   if((!font && WinRemovePresParam(group ? group : handle, PP_FONTNAMESIZE)) ||
      (font && WinSetPresParam(group ? group : handle, PP_FONTNAMESIZE, strlen(font)+1, (void *)font)))
   {
      Item *item = _dw_box_item(handle);

      /* Check to see if any of the sizes need to be recalculated */
      if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
      {
         _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
          /* Queue a redraw on the top-level window */
         _dw_redraw(_dw_toplevel_window(handle), TRUE);
      }
      return DW_ERROR_NONE;
   }
   return DW_ERROR_UNKNOWN;
}

/*
 * Gets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
char * API dw_window_get_font(HWND handle)
{
   char *str = (char *)alloca(50);
   HWND group = (HWND)dw_window_get_data(handle, "_dw_buddy");
   if(str && WinQueryPresParam(group ? group : handle, PP_FONTNAMESIZE, 0, NULL, 50, str, QPF_NOINHERIT))
      return strdup(str);
   return NULL;
}

/* Internal function to handle transparent children */
void _dw_handle_transparent(HWND handle)
{
    ULONG pcolor, which = PP_BACKGROUNDCOLOR;;


    if(!WinQueryPresParam(handle, which, 0, NULL, sizeof(pcolor), &pcolor, QPF_NOINHERIT))
        which = PP_BACKGROUNDCOLORINDEX;

    if(which == PP_BACKGROUNDCOLOR ||
       WinQueryPresParam(handle, which, 0, NULL, sizeof(pcolor), &pcolor, QPF_NOINHERIT))
    {
        HWND child;
        HENUM henum = WinBeginEnumWindows(handle);

        while((child = WinGetNextWindow(henum)) != NULLHANDLE)
        {
            if(dw_window_get_data(child, "_dw_transparent"))
            {
                WinSetPresParam(child, which, sizeof(pcolor), &pcolor);
            }
        }
        WinEndEnumWindows(henum);
    }
}

/* Internal version */
int _dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
    /* Handle foreground */
   if((fore & DW_RGB_COLOR) == DW_RGB_COLOR)
   {
      RGB2 rgb2;

      rgb2.bBlue = DW_BLUE_VALUE(fore);
      rgb2.bGreen = DW_GREEN_VALUE(fore);
      rgb2.bRed = DW_RED_VALUE(fore);
      rgb2.fcOptions = 0;

      WinSetPresParam(handle, PP_FOREGROUNDCOLOR, sizeof(RGB2), &rgb2);

   }
   else if(fore != DW_CLR_DEFAULT)
   {
      fore = _dw_internal_color(fore);

      WinSetPresParam(handle, PP_FOREGROUNDCOLORINDEX, sizeof(ULONG), &fore);
   }
   else if(fore == DW_CLR_DEFAULT)
   {
      WinRemovePresParam(handle, PP_FOREGROUNDCOLOR);
      WinRemovePresParam(handle, PP_FOREGROUNDCOLORINDEX);
   }
   /* Handle background */
   if(back == DW_RGB_TRANSPARENT)
   {
       /* Special case for setting transparent */
       ULONG pcolor;
       HWND parent = WinQueryWindow(handle, QW_PARENT);

       dw_window_set_data(handle, "_dw_transparent", DW_INT_TO_POINTER(1));

       if(WinQueryPresParam(parent, PP_BACKGROUNDCOLOR, 0, NULL,
                            sizeof(pcolor), &pcolor, QPF_NOINHERIT | QPF_PURERGBCOLOR))
           WinSetPresParam(handle, PP_BACKGROUNDCOLOR, sizeof(pcolor), &pcolor);
       else if(WinQueryPresParam(parent, PP_BACKGROUNDCOLORINDEX, 0, NULL,
                            sizeof(pcolor), &pcolor, QPF_NOINHERIT))
           WinSetPresParam(handle, PP_BACKGROUNDCOLORINDEX, sizeof(pcolor), &pcolor);
   }
   else if((back & DW_RGB_COLOR) == DW_RGB_COLOR)
   {
      RGB2 rgb2;

      rgb2.bBlue = DW_BLUE_VALUE(back);
      rgb2.bGreen = DW_GREEN_VALUE(back);
      rgb2.bRed = DW_RED_VALUE(back);
      rgb2.fcOptions = 0;

      WinSetPresParam(handle, PP_BACKGROUNDCOLOR, sizeof(RGB2), &rgb2);
      dw_window_set_data(handle, "_dw_transparent", NULL);
   }
   else if(back != DW_CLR_DEFAULT)
   {
      back = _dw_internal_color(back);

      WinSetPresParam(handle, PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &back);
      dw_window_set_data(handle, "_dw_transparent", NULL);
   }
   else if(back == DW_CLR_DEFAULT)
   {
      WinRemovePresParam(handle, PP_BACKGROUNDCOLOR);
      WinRemovePresParam(handle, PP_BACKGROUNDCOLORINDEX);
   }
   /* If this is a box... check if any of the children are transparent */
   _dw_handle_transparent(handle);
   return DW_ERROR_NONE;
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
   dw_window_set_data(handle, "_dw_fore", (void *)(fore+1));
   dw_window_set_data(handle, "_dw_back", (void *)(back+1));

   return _dw_window_set_color(handle, fore, back);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
   WinSendMsg(handle, WM_SETBORDERSIZE, MPFROMSHORT(border), MPFROMSHORT(border));
   return 0;
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
   WinSetCapture(HWND_DESKTOP, handle);
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
   WinSetCapture(HWND_DESKTOP, NULLHANDLE);
}

/*
 * Tracks this window movement.
 * Parameters:
 *       handle: Handle to frame to be tracked.
 */
void API dw_window_track(HWND handle)
{
   WinSendMsg(handle, WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), 0);
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
   HPOINTER pointer = pointertype < 65535 ?
      WinQuerySysPointer(HWND_DESKTOP, pointertype, FALSE)
      : (HPOINTER)pointertype;

   if(!pointertype)
      dw_window_set_data(handle, "_dw_pointer", 0);
   else
   {
      WinSetPointer(HWND_DESKTOP, pointer);

      if(handle != HWND_DESKTOP)
         dw_window_set_data(handle, "_dw_pointer", (void *)pointer);
   }
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
HWND API dw_window_new(HWND hwndOwner, const char *title, ULONG flStyle)
{
   HWND hwndframe;
   Box *newbox = calloc(1, sizeof(Box));
   WindowData *blah = calloc(1, sizeof(WindowData));
   ULONG winStyle = 0L;

   newbox->type = DW_VERT;
   newbox->vsize = newbox->hsize = _DW_SIZE_EXPAND;

   flStyle |= FCF_NOBYTEALIGN;

   if(flStyle & DW_FCF_TITLEBAR)
      newbox->titlebar = 1;
   else
      flStyle |= FCF_TITLEBAR;

   if(flStyle & WS_MAXIMIZED)
   {
      winStyle |= WS_MAXIMIZED;
      flStyle &= ~WS_MAXIMIZED;
   }
   if(flStyle & WS_MINIMIZED)
   {
      winStyle |= WS_MINIMIZED;
      flStyle &= ~WS_MINIMIZED;
   }

   /* Then create the real window window without FCF_SHELLPOSITION */
   flStyle &= ~FCF_SHELLPOSITION;
   hwndframe = WinCreateStdWindow(hwndOwner, winStyle, &flStyle, (PSZ)ClassName, (PSZ)title, 0L, NULLHANDLE, 0L, &newbox->hwnd);
   /* Default the window to a ridiculus place so it can't possibly be intentional */
   WinSetWindowPos(hwndframe, NULLHANDLE, -2000, -2000, 0, 0, SWP_MOVE);
   newbox->hwndtitle = WinWindowFromID(hwndframe, FID_TITLEBAR);
   if(!newbox->titlebar && newbox->hwndtitle)
      WinSetParent(newbox->hwndtitle, HWND_OBJECT, FALSE);
   blah->oldproc = WinSubclassWindow(hwndframe, _dw_sizeproc);
   WinSetWindowPtr(hwndframe, QWP_USER, blah);
   WinSetWindowPtr(newbox->hwnd, QWP_USER, newbox);
   return hwndframe;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_box_new(int type, int pad)
{
   Box *newbox = calloc(1, sizeof(Box));

   newbox->pad = pad;
   newbox->type = type;
   newbox->count = 0;
   newbox->grouphwnd = NULLHANDLE;

   newbox->hwnd = WinCreateWindow(HWND_OBJECT,
                           WC_FRAME,
                           NULL,
                           WS_VISIBLE | WS_CLIPCHILDREN |
                           FS_NOBYTEALIGN,
                           0,0,2000,1000,
                           NULLHANDLE,
                           HWND_TOP,
                           0L,
                           NULL,
                           NULL);

   newbox->oldproc = WinSubclassWindow(newbox->hwnd, _dw_controlproc);
   WinSetWindowPtr(newbox->hwnd, QWP_USER, newbox);
   dw_window_set_color(newbox->hwnd, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
   return newbox->hwnd;
}

/*
 * Create a new scroll Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_scrollbox_new(int type, int pad)
{
   HWND hwndframe, box = dw_box_new(type, pad);
   HWND client, tmpbox = dw_box_new(DW_VERT, 0);
   Box *blah = calloc(sizeof(Box), 1);
   dw_box_pack_start(tmpbox, box, 1, 1, TRUE, TRUE, 0);
   hwndframe = WinCreateWindow(HWND_OBJECT, (PSZ)ScrollClassName, NULL, WS_VISIBLE | WS_CLIPCHILDREN,
                               0, 0, 2000, 1000, NULLHANDLE, HWND_TOP, 0, NULL, NULL);
   WinCreateWindow(hwndframe, WC_SCROLLBAR, NULL, WS_VISIBLE | SBS_AUTOTRACK | SBS_VERT,
                   0,0,2000,1000, hwndframe, HWND_TOP, FID_VERTSCROLL, NULL, NULL);
   WinCreateWindow(hwndframe, WC_SCROLLBAR, NULL, WS_VISIBLE | SBS_AUTOTRACK | SBS_HORZ,
                   0,0,2000,1000, hwndframe, HWND_TOP, FID_HORZSCROLL, NULL, NULL);
   client = WinCreateWindow(hwndframe, WC_FRAME, NULL, WS_VISIBLE | WS_CLIPCHILDREN,
                      0,0,2000,1000, NULLHANDLE, HWND_TOP, FID_CLIENT, NULL, NULL);
   WinSetParent(tmpbox, client, FALSE);
   WinSetWindowPtr(client, QWP_USER, blah);
   dw_window_set_data(hwndframe, "_dw_resizebox", (void *)tmpbox);
   dw_window_set_data(hwndframe, "_dw_box", (void *)box);
   dw_window_set_color(hwndframe, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
   dw_window_set_color(client, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
   return hwndframe;
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_pos( HWND handle, int orient )
{
   HWND scroll;

   if(orient == DW_VERT)
   {
      scroll = WinWindowFromID(handle, FID_VERTSCROLL);
   }
   else
   {
      scroll = WinWindowFromID(handle, FID_HORZSCROLL);
   }
   return (int)WinSendMsg(scroll, SBM_QUERYPOS, 0, 0);
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_range( HWND handle, int orient )
{
   HWND scroll;

   if(orient == DW_VERT)
   {
      scroll = WinWindowFromID(handle, FID_VERTSCROLL);
   }
   else
   {
      scroll = WinWindowFromID(handle, FID_HORZSCROLL);
   }
   return SHORT2FROMMP(WinSendMsg(scroll, SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE), 0));
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
   Box *newbox = calloc(1, sizeof(Box));
   newbox->pad = pad;
   newbox->type = type;
   newbox->count = 0;

   newbox->hwnd = WinCreateWindow(HWND_OBJECT,
                           WC_FRAME,
                           NULL,
                           WS_VISIBLE |
                           FS_NOBYTEALIGN,
                           0,0,2000,1000,
                           NULLHANDLE,
                           HWND_TOP,
                           0L,
                           NULL,
                           NULL);

   newbox->grouphwnd = WinCreateWindow(newbox->hwnd,
                              WC_STATIC,
                              (PSZ)title,
                              WS_VISIBLE | SS_GROUPBOX |
                              WS_GROUP,
                              0,0,2000,1000,
                              NULLHANDLE,
                              HWND_TOP,
                              0L,
                              NULL,
                              NULL);

   WinSetWindowPtr(newbox->hwnd, QWP_USER, newbox);
   dw_window_set_color(newbox->hwnd, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
   dw_window_set_color(newbox->grouphwnd, DW_CLR_BLACK, DW_CLR_PALEGRAY);
   dw_window_set_font(newbox->grouphwnd, DefaultFont);
   dw_window_set_data(newbox->hwnd, "_dw_buddy", (void *)newbox->grouphwnd);
   return newbox->hwnd;
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long id)
{
   HWND hwndframe;
   ULONG back = CLR_DARKGRAY;

   hwndframe = WinCreateWindow(HWND_OBJECT,
                        WC_FRAME,
                        NULL,
                        WS_VISIBLE | WS_CLIPCHILDREN |
                        FS_NOBYTEALIGN,
                        0,0,0,0,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   /* Make the MDI Client area the same color as Windows and Unix */
   WinSetPresParam(hwndframe, PP_BACKGROUNDCOLORINDEX, sizeof(ULONG), &back);
   return hwndframe;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_bitmap_new(ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                              WC_STATIC,
                              NULL,
                              WS_VISIBLE | SS_TEXT,
                              0,0,0,0,
                              NULLHANDLE,
                              HWND_TOP,
                              id,
                              NULL,
                              NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_bitmapproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   return tmp;
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG id, int top)
{
   ULONG flags;
   HWND tmp;
   WindowData *blah = calloc(1, sizeof(WindowData));

   if(top)
      flags = BKS_MAJORTABTOP;
   else
      flags = BKS_MAJORTABBOTTOM;

   tmp = WinCreateWindow(HWND_OBJECT,
                    WC_NOTEBOOK,
                    NULL,
                    WS_VISIBLE |
#ifdef BKS_TABBEDDIALOG
                    BKS_TABBEDDIALOG |
#endif
                    flags,
                    0,0,2000,1000,
                    NULLHANDLE,
                    HWND_TOP,
                    id,
                    NULL,
                    NULL);

   /* Fix tab sizes on Warp 3 */
   if(!IS_WARP4())
   {
      /* best sizes to be determined by trial and error */
      WinSendMsg(tmp, BKM_SETDIMENSIONS,MPFROM2SHORT(102, 28), MPFROMSHORT( BKA_MAJORTAB));
   }

   blah->oldproc = WinSubclassWindow(tmp, _dw_notebookproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI API dw_menu_new(ULONG id)
{
   HMENUI tmp = WinCreateWindow(HWND_OBJECT,
                         WC_MENU,
                         NULL,
                         WS_VISIBLE,
                         0,0,2000,1000,
                         NULLHANDLE,
                         HWND_TOP,
                         id,
                         NULL,
                         NULL);
   return tmp;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
   HMENUI tmp = WinCreateWindow(location,
                         WC_MENU,
                         NULL,
                         WS_VISIBLE | MS_ACTIONBAR,
                         0,0,2000,1000,
                         location,
                         HWND_TOP,
                         FID_MENU,
                         NULL,
                         NULL);
   dw_window_set_data(tmp, "_dw_owner", (void *)location);
   dw_window_set_data(tmp, "_dw_menubar", (void *)location);
   return tmp;
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
      WinDestroyWindow(*menu);
      *menu = 0;
   }
}

/* Internal function to make sure menu ID isn't in use */
int _dw_menuid_allocated(int id)
{
   DWSignalHandler *tmp = Root;

   while(tmp)
   {
     if(tmp->id == id)
        return TRUE;
     tmp = tmp->next;
   }
   return FALSE;
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
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG id, ULONG flags, int end, int check, HMENUI submenu)
{
   MENUITEM miSubMenu;
   char buffer[30];
   int is_checked, is_disabled;

   if ( !menux || !WinIsWindow(dwhab, menux) )
      return NULLHANDLE;

   if ( end )
      miSubMenu.iPosition=MIT_END;
   else
      miSubMenu.iPosition=0;
   /*
    * Handle flags
   */
   miSubMenu.afAttribute = 0;
   is_checked = (flags & DW_MIS_CHECKED) ? 1 : 0;
   if ( is_checked )
      miSubMenu.afAttribute |= MIA_CHECKED;
   is_disabled = (flags & DW_MIS_DISABLED) ? 1 : 0;
   if ( is_disabled )
      miSubMenu.afAttribute |= MIA_DISABLED;

   if (title && *title)
   {
      /* Code to autogenerate a menu ID if not specified or invalid
       * First pool is smaller for transient popup menus
       */
      if(id == (ULONG)-1)
      {
         static ULONG tempid = 61000;

         tempid++;
         id = tempid;

         if(tempid > 65500)
            tempid = 61000;
      }
      /* Special internal case - 60001 to 60999 reserved for DW internal use */
      else if(id > 60000 && id < 61000 && check == -1)
      {
          check = 0;
      }
      /* Second pool is larger for more static windows */
      else if(!id || id >= 30000)
      {
         static ULONG menuid = 30000;

         do
         {
            menuid++;
            if(menuid > 60000)
               menuid = 30000;
         }
         while(_dw_menuid_allocated(menuid));
         id = menuid;
      }
      miSubMenu.afStyle = MIS_TEXT;
   }
   else
      miSubMenu.afStyle = MIS_SEPARATOR;
   miSubMenu.id=id;
   miSubMenu.hwndSubMenu = submenu;
   miSubMenu.hItem=NULLHANDLE;

   WinSendMsg(menux, MM_INSERTITEM, MPFROMP(&miSubMenu), MPFROMP(title));

   sprintf(buffer, "_dw_id%d", (int)id);
   dw_window_set_data(_dw_app, buffer, (void *)menux);
   sprintf(buffer, "_dw_checkable%ld", id);
   dw_window_set_data( _dw_app, buffer, (void *)check );
   sprintf(buffer, "_dw_ischecked%ld", id);
   dw_window_set_data( _dw_app, buffer, (void *)is_checked );
   sprintf(buffer, "_dw_isdisabled%ld", id);
   dw_window_set_data( _dw_app, buffer, (void *)is_disabled );

   if ( submenu )
      dw_window_set_data(submenu, "_dw_owner", (void *)menux);
   return (HWND)id;
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
   if ( check )
      WinSendMsg(menux, MM_SETITEMATTR, MPFROM2SHORT(id, TRUE),MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
   else
      WinSendMsg(menux, MM_SETITEMATTR, MPFROM2SHORT(id, TRUE),MPFROM2SHORT(MIA_CHECKED, 0));
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
   char buffer1[30],buffer2[30];
   int check;
   int disabled;
   USHORT fAttribute=0;

   sprintf( buffer1, "_dw_ischecked%ld", id );
   check = (int)dw_window_get_data( _dw_app, buffer1 );
   sprintf( buffer2, "_dw_isdisabled%ld", id );
   disabled = (int)dw_window_get_data( _dw_app, buffer2 );

   if ( (state & DW_MIS_CHECKED) || (state & DW_MIS_UNCHECKED) )
   {
      /*
       * If we are changing state of "checked" base our setting on the passed flag...
       */
      if ( state & DW_MIS_CHECKED )
      {
         fAttribute |= MIA_CHECKED;
         check = 1;
      }
      else
      {
         check = 0;
      }
   }
   else
   {
      /*
       * ...otherwise base our setting on the current "checked" state.
       */
      if ( check )
      {
         fAttribute |= MIA_CHECKED;
      }
   }
   if ( (state & DW_MIS_ENABLED) || (state & DW_MIS_DISABLED) )
   {
      if ( state & DW_MIS_DISABLED )
      {
         fAttribute |= MIA_DISABLED;
         disabled = 1;
      }
      else
      {
         disabled = 0;
      }
   }
   else
   {
      /*
       * ...otherwise base our setting on the current "disabled" state.
       */
      if ( disabled )
      {
         fAttribute |= MIA_DISABLED;
      }
   }
   WinSendMsg( menux, MM_SETITEMATTR, MPFROM2SHORT(id, TRUE), MPFROM2SHORT( MIA_CHECKED|MIA_DISABLED, fAttribute ) );
   /*
    * Keep our internal checked state consistent
    */
   dw_window_set_data( _dw_app, buffer1, (void *)check );
   dw_window_set_data( _dw_app, buffer2, (void *)disabled );
}

/*
 * Deletes the menu item specified
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menux, unsigned long id)
{
   if(id < 65536 && menux)
   {
      WinSendMsg(menux, MM_DELETEITEM, MPFROM2SHORT(id, FALSE), 0);
      /* If the ID was autogenerated it is safe to remove it */
      if(id >= 30000)
         dw_signal_disconnect_by_window((HWND)id);
      return DW_ERROR_NONE;
   }
   return DW_ERROR_UNKNOWN;
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
      _dw_popup = parent;
      dw_window_set_data(*menu, "_dw_popup", DW_INT_TO_POINTER(1));
      WinPopupMenu(HWND_DESKTOP, parent, *menu, x, dw_screen_height() - y, 0, PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_VCONSTRAIN | PU_HCONSTRAIN);
      *menu = 0;
   }
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
   POINTL ptl;

   WinQueryPointerPos(HWND_DESKTOP, &ptl);
   if(x && y)
   {
      *x = ptl.x;
      *y = dw_screen_height() - ptl.y;
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
   WinSetPointerPos(HWND_DESKTOP, x, dw_screen_height() - y);
}


/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_container_new(ULONG id, int multi)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_CONTAINER,
                        NULL,
                        WS_VISIBLE | CCS_READONLY |
                        (multi ? CCS_EXTENDSEL : CCS_SINGLESEL) |
                        CCS_AUTOPOSITION,
                        0,0,0,0,
                        NULLHANDLE,
                        HWND_TOP,
                        id ? id : _dw_global_id(),
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_treeproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_data(tmp, "_dw_container", (void *)1);
   return tmp;
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_tree_new(ULONG id)
{
   CNRINFO cnrinfo;
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_CONTAINER,
                        NULL,
                        WS_VISIBLE | CCS_READONLY |
                        CCS_SINGLESEL | CCS_AUTOPOSITION,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id ? id : _dw_global_id(),
                        NULL,
                        NULL);

   cnrinfo.flWindowAttr = CV_TREE | CA_TREELINE;
   cnrinfo.slBitmapOrIcon.cx = 16;
   cnrinfo.slBitmapOrIcon.cy = 16;
   cnrinfo.cyLineSpacing = 0;
   cnrinfo.cxTreeIndent = 16;
   cnrinfo.cxTreeLine = 1;

   WinSendMsg(tmp, CM_SETCNRINFO, &cnrinfo, MPFROMLONG(CMA_FLWINDOWATTR | CMA_SLBITMAPORICON |
                                          CMA_LINESPACING | CMA_CXTREEINDENT | CMA_CXTREELINE));
   blah->oldproc = WinSubclassWindow(tmp, _dw_treeproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(sizeof(WindowData), 1);
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_STATIC,
                        (PSZ)text,
                        WS_VISIBLE | SS_TEXT,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_textproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_RGB_TRANSPARENT);
   return tmp;
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_status_text_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(sizeof(WindowData), 1);
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_STATIC,
                        (PSZ)text,
                        WS_VISIBLE | SS_TEXT,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_statusproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_PALEGRAY);
   dw_window_set_data(tmp, "_dw_status", DW_INT_TO_POINTER(1));
   return tmp;
}

#ifndef MLS_LIMITVSCROLL
#define MLS_LIMITVSCROLL           0x00000080L
#endif

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_MLE,
                        NULL,
                        WS_VISIBLE | MLS_WORDWRAP |
                        MLS_BORDER | MLS_IGNORETAB |
                        MLS_VSCROLL | MLS_LIMITVSCROLL,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   WinSendMsg(tmp, MLM_FORMAT, (MPARAM)MLFIE_NOTRANS, 0);
   blah->oldproc = WinSubclassWindow(tmp, _dw_mleproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_new(const char *text, ULONG id)
{

   WindowData *blah = calloc(1, sizeof(WindowData));
   ENTRYFDATA efd = { sizeof(ENTRYFDATA), 32000, 0, 0 };
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_ENTRYFIELD,
                        (PSZ)text,
                        WS_VISIBLE | ES_MARGIN |
                        ES_AUTOSCROLL | WS_TABSTOP,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        (PVOID)&efd,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_WHITE);
   return tmp;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_ENTRYFIELD,
                        (PSZ)text,
                        WS_VISIBLE | ES_MARGIN | ES_UNREADABLE |
                        ES_AUTOSCROLL | WS_TABSTOP,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_WHITE);
   return tmp;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_combobox_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND frame = dw_box_new(DW_HORZ, 0);
   HWND tmp = WinCreateWindow(frame,
                        WC_COMBOBOX,
                        (PSZ)text,
                        WS_VISIBLE | CBS_DROPDOWN | WS_GROUP,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id ? id : _dw_global_id(),
                        NULL,
                        NULL);
   HENUM henum = WinBeginEnumWindows(tmp);
   HWND child, last = NULLHANDLE;

   while((child = WinGetNextWindow(henum)) != NULLHANDLE)
   {
      WindowData *moreblah = calloc(1, sizeof(WindowData));
      moreblah->oldproc = WinSubclassWindow(child, _dw_comboentryproc);
      WinSetWindowPtr(child, QWP_USER, moreblah);
      dw_window_set_color(child, DW_CLR_BLACK, DW_CLR_WHITE);
      last = child;
   }
   WinEndEnumWindows(henum);
   blah->oldproc = WinSubclassWindow(tmp, _dw_comboproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_WHITE);
   dw_window_set_data(tmp, "_dw_comboentry", (void *)last);
   dw_window_set_data(tmp, "_dw_combo_box", (void *)frame);
   WinSetOwner(tmp, frame);
   return tmp;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_button_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_BUTTON,
                        (PSZ)text,
                        WS_VISIBLE,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);

   blah->oldproc = WinSubclassWindow(tmp, _dw_buttonproc);

   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_PALEGRAY);
   return tmp;
}

/* Function: GenResIDStr
** Abstract: Generate string '#nnnn' for a given ID for using with Button
**           controls
*/

void _dw_gen_res_id_str(CHAR *buff, ULONG ulID)
{
   char *str;
   int  slen = 0;

   *buff++ = '#';

   str = buff;

   do
   {
      *str++ = (ulID % 10) + '0';
      ulID /= 10;
      slen++;
   }
   while(ulID);

   *str-- = 0;

   for(; str > buff; str--, buff++)
   {
      *buff ^= *str;
      *str  ^= *buff;
      *buff ^= *str;
   }
}


/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(const char *text, ULONG id)
{
   char idbuf[256], *name = NULL;
   HWND tmp;
   WindowData *blah = calloc(1, sizeof(WindowData));
   HPOINTER icon = WinLoadPointer(HWND_DESKTOP, 0L, id);

   if(!icon)
   {
      name = idbuf;
      _dw_gen_res_id_str(idbuf, id);
   }

   tmp = WinCreateWindow(HWND_OBJECT,
                    WC_BUTTON,
                    (PSZ)name,
                    WS_VISIBLE | BS_PUSHBUTTON |
                    BS_NOPOINTERFOCUS | BS_AUTOSIZE |
                    (icon ? 0 : BS_BITMAP),
                    0,0,2000,1000,
                    NULLHANDLE,
                    HWND_TOP,
                    id,
                    NULL,
                    NULL);

   if(text)
      strncpy(blah->bubbletext, text, BUBBLE_HELP_MAX - 1);
   blah->oldproc = WinSubclassWindow(tmp, _dw_buttonproc);

   WinSetWindowPtr(tmp, QWP_USER, blah);

   if(icon)
   {
       PVOID ResPtr;
       ULONG ResSize;

       /* Since WinLoadPointer() can change the size of the icon...
        * We will query the resource directly and check the size ourselves.
        */
       if(DosQueryResourceSize(NULLHANDLE, RT_POINTER, id, &ResSize) == NO_ERROR && ResSize &&
          DosGetResource(NULLHANDLE, RT_POINTER, id, &ResPtr) == NO_ERROR)
       {
           PBITMAPFILEHEADER2 header = ResPtr;

           /* We can only check for icons and pointers */
           if(header->usType == 0x4943 /* Icon 'CI' */ ||
              header->usType == 0x5043 /* Pointer 'CP' */)
           {
               /* Check the new style header */
               if(header->bmp2.cbFix == sizeof(BITMAPINFOHEADER2))
                   dw_window_set_data(tmp, "_dw_button_icon_width", DW_INT_TO_POINTER(header->bmp2.cx));
               else if(header->bmp2.cbFix == sizeof(BITMAPINFOHEADER))
               {
                   /* Check the old style header */
                   PBITMAPINFOHEADER bi = (PBITMAPINFOHEADER)&(header->bmp2);

                   dw_window_set_data(tmp, "_dw_button_icon_width", DW_INT_TO_POINTER(((int)bi->cx)));
               }
           }
           DosFreeResource(ResPtr);
       }
       dw_window_set_data(tmp, "_dw_button_icon", DW_POINTER(icon));
   }
   dw_window_set_data(tmp, "_dw_bitmapbutton", DW_INT_TO_POINTER(1));
   return tmp;
}

/* Internal function to create a disabled version of a pixmap */
HPIXMAP _dw_create_disabled(HWND handle, HPIXMAP pixmap)
{
    /* Create a disabled style pixmap */
    HPIXMAP disabled = dw_pixmap_new(handle, pixmap->width, pixmap->height, dw_color_depth_get());
    LONG fore = _dw_foreground;
    int z, j, lim;

    dw_pixmap_bitblt(0, disabled, 0, 0, pixmap->width, pixmap->height, 0, pixmap, 0, 0);

    dw_color_foreground_set(DW_CLR_PALEGRAY);
    lim = pixmap->width/2;
    for(j=0;j<pixmap->height;j++)
    {
        int mod = j%2;

        for(z=0;z<lim;z++)
            dw_draw_point(0, disabled, (z*2)+mod, j);
    }
    _dw_foreground = fore;
    return disabled;
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
HWND API dw_bitmapbutton_new_from_file(const char *text, unsigned long id, const char *filename)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_BUTTON,
                        NULL,
                        WS_VISIBLE | BS_PUSHBUTTON |
                        BS_AUTOSIZE | BS_NOPOINTERFOCUS,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   char *file = alloca(strlen(filename) + 6);
   HPIXMAP pixmap = NULL, disabled = NULL;
   HPOINTER icon = 0;

   if(file && (pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      int z, len;

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) == 0)
      {
         len = strlen( file );
         if(len > 4)
         {
            if(stricmp(file + len - 4, ".ico") == 0)
               icon = WinLoadFileIcon((PSZ)file, FALSE);
            else
               _dw_load_bitmap_file(file, tmp, &pixmap->hbm, &pixmap->hdc, &pixmap->hps, &pixmap->width, &pixmap->height, &pixmap->depth, DW_CLR_DEFAULT);
         }
      }
      else
      {
         /* Try with .ico extension first...*/
         strcat(file, ".ico");
         if(access(file, 04) == 0)
            icon = WinLoadFileIcon((PSZ)file, FALSE);
         else
         {
            for(z=0;z<(_gbm_init?NUM_EXTS:1);z++)
            {
                strcpy(file, filename);
                strcat(file, _dw_image_exts[z]);
                if(access(file, 04) == 0 &&
                   _dw_load_bitmap_file(file, tmp, &pixmap->hbm, &pixmap->hdc, &pixmap->hps, &pixmap->width, &pixmap->height, &pixmap->depth, DW_CLR_DEFAULT))
                    break;
            }
         }
      }

      if(icon)
      {
         free(pixmap);
         pixmap = NULL;
      }
      else
      {
          disabled = _dw_create_disabled(tmp, pixmap);
      }
   }

   if(text)
      strncpy(blah->bubbletext, text, BUBBLE_HELP_MAX - 1);
   blah->oldproc = WinSubclassWindow(tmp, _dw_buttonproc);

   WinSetWindowPtr(tmp, QWP_USER, blah);

   if(icon)
      dw_window_set_data(tmp, "_dw_button_icon", (void *)icon);
   else
   {
      dw_window_set_data(tmp, "_dw_hpixmap", (void *)pixmap);
      dw_window_set_data(tmp, "_dw_hpixmap_disabled", (void *)disabled);
   }
   dw_window_set_data(tmp, "_dw_bitmapbutton", (void *)1);
   return tmp;
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
HWND API dw_bitmapbutton_new_from_data(const char *text, unsigned long id, const char *data, int len)
{
   FILE *fp;
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_BUTTON,
                        NULL,
                        WS_VISIBLE | BS_PUSHBUTTON |
                        BS_AUTOSIZE | BS_NOPOINTERFOCUS,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   char *file;
   HPIXMAP pixmap = NULL, disabled = NULL;
   HPOINTER icon = 0;

   if((pixmap = calloc(1, sizeof(struct _hpixmap))) != NULL)
   {
      file = tmpnam( NULL );
      if ( file != NULL )
      {
         fp = fopen( file, "wb" );
         if ( fp != NULL )
         {
            fwrite( data, 1, len, fp );
            fclose( fp );
            if(!_dw_load_bitmap_file( file, tmp, &pixmap->hbm, &pixmap->hdc, &pixmap->hps, &pixmap->width, &pixmap->height, &pixmap->depth, DW_CLR_DEFAULT))
            {
               icon = WinLoadFileIcon((PSZ)file, FALSE);
            }
         }
         else
         {
            unlink( file );
            return 0;
         }
         unlink( file );
      }

      if ( icon )
      {
         free(pixmap);
         pixmap = NULL;
      }
      else
      {
          disabled = _dw_create_disabled(tmp, pixmap);
      }
   }

   if(text)
      strncpy(blah->bubbletext, text, BUBBLE_HELP_MAX - 1);
   blah->oldproc = WinSubclassWindow(tmp, _dw_buttonproc);

   WinSetWindowPtr(tmp, QWP_USER, blah);

   if(icon)
      dw_window_set_data(tmp, "_dw_button_icon", (void *)icon);
   else
   {
      dw_window_set_data(tmp, "_dw_hpixmap", (void *)pixmap);
      dw_window_set_data(tmp, "_dw_hpixmap_disabled", (void *)disabled);
   }
   dw_window_set_data(tmp, "_dw_bitmapbutton", (void *)1);
   return tmp;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_spinbutton_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(sizeof(WindowData), 1);
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_SPINBUTTON,
                        (PSZ)text,
                        WS_VISIBLE | SPBS_MASTER,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   HWND entry = _dw_find_entryfield(tmp);
   WinSendMsg(tmp, SPBM_SETLIMITS, MPFROMLONG(65536), MPFROMLONG(-65536));
   WinSendMsg(tmp, SPBM_SETCURRENTVALUE, MPFROMLONG(atoi(text)), 0L);
   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   blah = calloc(sizeof(WindowData), 1);
   blah->oldproc = WinSubclassWindow(entry, _dw_spinentryproc);
   WinSetWindowPtr(entry, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(entry, DW_CLR_BLACK, DW_CLR_WHITE);
   dw_window_set_data(tmp, "_dw_buddy", (void *)entry);
   return tmp;
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_radiobutton_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(sizeof(WindowData), 1);
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_BUTTON,
                        (PSZ)text,
                        WS_VISIBLE |
                        BS_AUTORADIOBUTTON,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_RGB_TRANSPARENT);
   return tmp;
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
   WindowData *blah = calloc(1, sizeof(WindowData));
   SLDCDATA sldcData = { 0, 0, 0, 0, 0 };
   HWND tmp;

   sldcData.cbSize = sizeof(SLDCDATA);
   sldcData.usScale1Increments = increments;

   tmp = WinCreateWindow(HWND_OBJECT,
                    WC_SLIDER,
                    NULL,
                    WS_VISIBLE | SLS_SNAPTOINCREMENT |
                    (vertical ? SLS_VERTICAL : SLS_HORIZONTAL),
                    0,0,2000,1000,
                    NULLHANDLE,
                    HWND_TOP,
                    id ? id : _dw_global_id(),
                    &sldcData,
                    NULL);

   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   return tmp;
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
   return WinCreateWindow(HWND_OBJECT,
                     WC_SCROLLBAR,
                     NULL,
                     WS_VISIBLE | SBS_AUTOTRACK |
                     (vertical ? SBS_VERT : SBS_HORZ),
                     0,0,2000,1000,
                     NULLHANDLE,
                     HWND_TOP,
                     id ? id : _dw_global_id(),
                     NULL,
                     NULL);
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_SLIDER,
                        NULL,
                        WS_VISIBLE | SLS_READONLY
                        | SLS_RIBBONSTRIP,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id ? id : _dw_global_id(),
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_percentproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_disable(tmp);
   return tmp;
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(const char *text, ULONG id)
{
   WindowData *blah = calloc(1, sizeof(WindowData));
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_BUTTON,
                        (PSZ)text,
                        WS_VISIBLE | BS_AUTOCHECKBOX,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_buttonproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_RGB_TRANSPARENT);
   return tmp;
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
HWND API dw_listbox_new(ULONG id, int multi)
{
   WindowData *blah = calloc(sizeof(WindowData), 1);
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        WC_LISTBOX,
                        NULL,
                        WS_VISIBLE | LS_NOADJUSTPOS |
                        (multi ? LS_MULTIPLESEL : 0),
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id ? id : _dw_global_id(),
                        NULL,
                        NULL);
   blah->oldproc = WinSubclassWindow(tmp, _dw_entryproc);
   WinSetWindowPtr(tmp, QWP_USER, blah);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_BLACK, DW_CLR_WHITE);
   return tmp;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void API dw_window_set_icon(HWND handle, HICN icon)
{
   HPOINTER hptr = icon < 65536 ? WinLoadPointer(HWND_DESKTOP,NULLHANDLE,icon) : (HPOINTER)icon;
   WinSendMsg(handle, WM_SETICON, (MPARAM)hptr, 0);
}

/* Code from GBM to convert to 24bpp if it isn't currently */
static int _dw_to_24_bit(GBM *gbm, GBMRGB *gbmrgb, BYTE **ppbData)
{
    unsigned long stride     = (((unsigned long)gbm -> w * gbm -> bpp + 31)/32) * 4;
    unsigned long new_stride = (((unsigned long)gbm -> w * 3 + 3) & ~3);
    unsigned long bytes;
    int y;
    unsigned char *pbDataNew;

    if ( gbm -> bpp == 24 )
    {
        return ( TRUE );
    }

    bytes = new_stride * gbm -> h;
    /* Allocate a buffer to store the image */
    if(DosAllocMem((PPVOID)&pbDataNew, (ULONG)bytes, PAG_READ | PAG_WRITE | PAG_COMMIT) != NO_ERROR)
    {
        return ( FALSE );
    }

    for ( y = 0; y < gbm -> h; y++ )
    {
        unsigned char *src = *ppbData + y * stride;
        unsigned char *dest = pbDataNew + y * new_stride;
        int   x;

        switch ( gbm -> bpp )
        {
        case 1:
            {
                unsigned char  c = 0;
                for ( x = 0; x < gbm -> w; x++ )
                {
                    if ( (x & 7) == 0 )
                        c = *src++;
                    else
                        c <<= 1;

                    *dest++ = gbmrgb [(c & 0x80) != 0].b;
                    *dest++ = gbmrgb [(c & 0x80) != 0].g;
                    *dest++ = gbmrgb [(c & 0x80) != 0].r;
                }
            }
            break;

        case 4:
            for ( x = 0; x + 1 < gbm -> w; x += 2 )
            {
                unsigned char c = *src++;

                *dest++ = gbmrgb [c >> 4].b;
                *dest++ = gbmrgb [c >> 4].g;
                *dest++ = gbmrgb [c >> 4].r;
                *dest++ = gbmrgb [c & 15].b;
                *dest++ = gbmrgb [c & 15].g;
                *dest++ = gbmrgb [c & 15].r;
            }

            if ( x < gbm -> w )
            {
                unsigned char c = *src;

                *dest++ = gbmrgb [c >> 4].b;
                *dest++ = gbmrgb [c >> 4].g;
                *dest++ = gbmrgb [c >> 4].r;
            }
            break;

        case 8:
            for ( x = 0; x < gbm -> w; x++ )
            {
                unsigned char c = *src++;

                *dest++ = gbmrgb [c].b;
                *dest++ = gbmrgb [c].g;
                *dest++ = gbmrgb [c].r;
            }
            break;
        }
    }
    DosFreeMem(*ppbData);
    *ppbData = pbDataNew;
    gbm->bpp = 24;

    return ( TRUE );
}

/* GBM seems to be compiled with VisualAge which defines O_BINARY and O_RDONLY
 * as follows... but other compilers (GCC and Watcom at least) define them
 * differently... so we add defines that are compatible with VAC here.
 */
#define GBM_O_BINARY        0x00008000
#define GBM_O_RDONLY        0x00000004

/* Internal function to load a bitmap from a file and return handles
 * to the bitmap, presentation space etc.
 */
int _dw_load_bitmap_file(char *file, HWND handle, HBITMAP *hbm, HDC *hdc, HPS *hps, unsigned long *width, unsigned long *height, int *depth, unsigned long backrgb)
{
    PBITMAPINFOHEADER2 pBitmapInfoHeader;
    /* pointer to the first byte of bitmap data  */
    PBYTE BitmapFileBegin, BitmapBits;
    ULONG ulFlags;
    SIZEL sizl = { 0 };
    HPS hps1;
    HDC hdc1;

    /* If we have GBM support open the file using GBM */
    if(_gbm_init)
    {
        int fd, z, err = -1, ft = 0;
        GBM gbm;
        GBMRGB *gbmrgb = NULL;
        ULONG byteswidth;

        /* Try to open the file */
        if((fd = _gbm_io_open(file, GBM_O_RDONLY|GBM_O_BINARY)) == -1)
        {
#ifdef DEBUG
            dw_debug("Failed to open file %s\n", file);
#endif
            return 0;
        }

        /* guess the source file type from the source filename */
        _gbm_query_n_filetypes(&ft);

        for(z=0;z<ft;z++)
        {
            /* Using CLR_PALEGRAY as a default alpha background... we can
             * change this to use WinQuerySysColor() later, but pale gray is
             * already hardcoded elsewhere so just continue using it here.
             */
            char options[101] = "back_rgb=52224_52224_52224";

            /* Ask the control if it has another color set */
            if(backrgb == DW_CLR_DEFAULT && handle)
            {
                RGB rgb = {0};

                if(WinQueryPresParam(handle, PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX, NULL, sizeof(rgb), &rgb, QPF_NOINHERIT | QPF_PURERGBCOLOR | QPF_ID2COLORINDEX))
                    sprintf(options, "back_rgb=%d_%d_%d", rgb.bRed * 256, rgb.bGreen * 256, rgb.bBlue * 256);
            }
            else if(backrgb & DW_RGB_COLOR)
            {
                sprintf(options, "back_rgb=%d_%d_%d", (int)DW_RED_VALUE(backrgb) * 256, (int)DW_GREEN_VALUE(backrgb) * 256, (int)DW_BLUE_VALUE(backrgb) * 256);
            }

            /* Read the file header */
            if((err = _gbm_read_header(file, fd, z, &gbm, options)) == 0)
                break;
        }

        /* If we failed to load the header */
        if(err)
        {
#ifdef DEBUG
            dw_debug("GBM: Read header type %d \"%s\" %d %s\n", z, file, err, _gbm_err(err));
#endif
            _gbm_io_close(fd);
            return 0;
        }

        /* if less than 24-bit, then have palette */
        if(gbm.bpp < 24)
        {
            gbmrgb = alloca(sizeof(GBMRGB) * 256);
            /* Read the palette from the file */
            if((err = _gbm_read_palette(fd, z, &gbm, gbmrgb)) != 0)
            {
#ifdef DEBUG
                dw_debug("GBM: Read palette type %d \"%s\" %d %s\n", z, file, err, _gbm_err(err));
#endif
                _gbm_io_close(fd);
                return 0;
            }
        }

        /* Save the dimension for return */
        *width = gbm.w;
        *height = gbm.h;
        *depth = gbm.bpp;
        byteswidth = (((gbm.w*gbm.bpp + 31)/32)*4);

        /* Allocate a buffer to store the image */
        DosAllocMem((PPVOID)&BitmapFileBegin, (ULONG)byteswidth * gbm.h,
                    PAG_READ | PAG_WRITE | PAG_COMMIT);

        /* Read the data into our buffer */
        if((err = _gbm_read_data(fd, z, &gbm, BitmapFileBegin)) != 0)
        {
#ifdef DEBUG
            dw_debug("GBM: Read data type %d \"%s\" %d %s\n", z, file, err, _gbm_err(err));
#endif
            _gbm_io_close(fd);
            DosFreeMem(BitmapFileBegin);
            return 0;
        }

        /* Close the file */
        _gbm_io_close(fd);

        /* Convert to 24bpp for use in the application */
        if(_dw_to_24_bit(&gbm, gbmrgb, &BitmapFileBegin))
            *depth = 24;
        else
        {
#ifdef DEBUG
            dw_debug("GBM: Failed 24bpp conversion \"%s\"\n", file);
#endif
            DosFreeMem(BitmapFileBegin);
            return 0;
        }

        pBitmapInfoHeader = alloca(sizeof(BITMAPINFOHEADER2));
        memset(pBitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER2));
        pBitmapInfoHeader->cbFix     = sizeof(BITMAPINFOHEADER2);
        pBitmapInfoHeader->cx        = (SHORT)gbm.w;
        pBitmapInfoHeader->cy        = (SHORT)gbm.h;
        pBitmapInfoHeader->cPlanes   = (SHORT)1;
        pBitmapInfoHeader->cBitCount = (SHORT)gbm.bpp;

        /* Put the bitmap bits into the destination */
        BitmapBits = BitmapFileBegin;
    }
    else
    {
        HFILE BitmapFileHandle = NULLHANDLE; /* handle for the file */
        ULONG OpenAction = 0;
        FILESTATUS BitmapStatus;
        ULONG cbRead;
        PBITMAPFILEHEADER2 pBitmapFileHeader;

        /* open bitmap file */
        DosOpen((PSZ)file, &BitmapFileHandle, &OpenAction, 0L,
                FILE_ARCHIVED | FILE_NORMAL | FILE_READONLY,
                OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY |
                OPEN_FLAGS_NOINHERIT, 0L);

        if(!BitmapFileHandle)
            return 0;

        /* find out how big the file is  */
        DosQueryFileInfo(BitmapFileHandle, 1, &BitmapStatus,
                         sizeof(BitmapStatus));

        /* allocate memory to load the bitmap */
        DosAllocMem((PPVOID)&BitmapFileBegin, (ULONG)BitmapStatus.cbFile,
                    PAG_READ | PAG_WRITE | PAG_COMMIT);

        /* read bitmap file into memory buffer */
        DosRead(BitmapFileHandle, (PVOID)BitmapFileBegin,
                BitmapStatus.cbFile, &cbRead);

        /* access first bytes as bitmap header */
        pBitmapFileHeader = (PBITMAPFILEHEADER2)BitmapFileBegin;

        /* check if it's a valid bitmap data file */
        if((pBitmapFileHeader->usType != BFT_BITMAPARRAY) &&
           (pBitmapFileHeader->usType != BFT_BMAP))
        {
            /* free memory of bitmap file buffer */
            DosFreeMem(BitmapFileBegin);
            /* close the bitmap file */
            DosClose(BitmapFileHandle);
            return 0;
        }

        /* check if it's a file with multiple bitmaps */
        if(pBitmapFileHeader->usType == BFT_BITMAPARRAY)
        {
            /* we'll just use the first bitmap and ignore the others */
            pBitmapFileHeader = &(((PBITMAPARRAYFILEHEADER2)BitmapFileBegin)->bfh2);
        }

        /* set pointer to bitmap information block */
        pBitmapInfoHeader = &pBitmapFileHeader->bmp2;

        /* find out if it's the new 2.0 format or the old format */
        /* and query number of lines */
        if(pBitmapInfoHeader->cbFix == sizeof(BITMAPINFOHEADER))
        {
            *height = (ULONG)((PBITMAPINFOHEADER)pBitmapInfoHeader)->cy;
            *width = (ULONG)((PBITMAPINFOHEADER)pBitmapInfoHeader)->cx;
            *depth = (int)((PBITMAPINFOHEADER)pBitmapInfoHeader)->cBitCount;
        }
        else
        {
            *height = pBitmapInfoHeader->cy;
            *width = pBitmapInfoHeader->cx;
            *depth = pBitmapInfoHeader->cBitCount;
        }

        /* Put the bitmap bits into the destination */
        BitmapBits = BitmapFileBegin + pBitmapFileHeader->offBits;

        /* close the bitmap file */
        DosClose(BitmapFileHandle);
    }

    /* now we need a presentation space, get it from static control */
    hps1 = WinGetPS(handle);

    hdc1    = GpiQueryDevice(hps1);
    ulFlags = GpiQueryPS(hps1, &sizl);

    *hdc = DevOpenDC(dwhab, OD_MEMORY, (PSZ)"*", 0L, NULL, hdc1);
    *hps = GpiCreatePS (dwhab, *hdc, &sizl, ulFlags | GPIA_ASSOC);

    /* create bitmap now using the parameters from the info block */
    *hbm = GpiCreateBitmap(*hps, pBitmapInfoHeader, 0L, NULL, NULL);

    /* select the new bitmap into presentation space */
    GpiSetBitmap(*hps, *hbm);

    /* now copy the bitmap data into the bitmap */
    GpiSetBitmapBits(*hps, 0L, *height,
                     BitmapBits,
                     (PBITMAPINFO2)pBitmapInfoHeader);

    WinReleasePS(hps1);

    /* free memory of bitmap file buffer */
    if(BitmapFileBegin)
        DosFreeMem(BitmapFileBegin);
    return 1;
}

/* Internal function to change the button bitmap */
int _dw_window_set_bitmap(HWND handle, HBITMAP hbm, HDC hdc, HPS hps, unsigned long width, unsigned long height, int depth, HPOINTER icon)
{
   char tmpbuf[100] = {0};

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

   /* Button */
   if(strncmp(tmpbuf, "#3", 3)==0)
   {
       WNDPARAMS   wp = {0};
       BTNCDATA    bcd = {0};
       RECTL       rect;

       wp.fsStatus = WPM_CTLDATA;
       wp.pCtlData = &bcd;
       wp.cbCtlData = bcd.cb = sizeof(BTNCDATA);

       /* Clear any existing icon */
       WinSendMsg(handle, WM_SETWINDOWPARAMS, (MPARAM)&wp, NULL);

       if(icon)
       {
           dw_window_set_data(handle, "_dw_button_icon", DW_POINTER(icon));
       }
       else
       {
           HPIXMAP disabled, pixmap = calloc(1,sizeof(struct _hpixmap));

           pixmap->hbm = hbm;
           pixmap->hdc = hdc;
           pixmap->hps = hps;
           pixmap->width = width;
           pixmap->height = height;
           disabled = _dw_create_disabled(handle, pixmap);

           dw_window_set_data(handle, "_dw_hpixmap", DW_POINTER(pixmap));
           dw_window_set_data(handle, "_dw_hpixmap_disabled", DW_POINTER(disabled));
       }
       dw_window_set_data(handle, "_dw_bitmapbutton", DW_POINTER(1));
       /* Make sure we invalidate the button so it redraws */
       WinQueryWindowRect(handle, &rect);
       WinInvalidateRect(handle, &rect, TRUE);
   }
   else
      return DW_ERROR_UNKNOWN;

   /* If we changed the bitmap... */
   {
      Item *item = _dw_box_item(handle);

      /* Check to see if any of the sizes need to be recalculated */
      if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
      {
         _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
         /* Queue a redraw on the top-level window */
         _dw_redraw(_dw_toplevel_window(handle), TRUE);
      }
   }
   return DW_ERROR_NONE;
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
int API dw_window_set_bitmap(HWND handle, unsigned long id, const char *filename)
{
   HBITMAP hbm = 0;
   HPS     hps = 0;
   HDC     hdc = 0;
   HPOINTER icon = 0;
   unsigned long width = 0, height = 0;
   int depth = 0;

   /* Destroy any old bitmap data */
   _dw_free_bitmap(handle);

   /* If id is non-zero use the resource */
   if(id)
   {
      hps = WinGetPS(handle);
      hbm = GpiLoadBitmap(hps, NULLHANDLE, id, 0, 0);
      WinReleasePS(hps);
   }
   else if(filename)
   {
      char *file = alloca(strlen(filename) + 6);

      if(!file)
         return DW_ERROR_GENERAL;

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) != 0)
      {
         /* Try with .ico extension first...*/
         strcat(file, ".ico");
         if(access(file, 04) == 0)
            icon = WinLoadFileIcon((PSZ)file, FALSE);
         else
         {
             int z;

             /* Try with supported extensions */
             for(z=0;z<(_gbm_init?NUM_EXTS:1);z++)
             {
                 strcpy(file, filename);
                 strcat(file, _dw_image_exts[z]);
                 if(access(file, 04) == 0 &&
                    _dw_load_bitmap_file(file, handle, &hbm, &hdc, &hps, &width, &height, &depth, DW_CLR_DEFAULT))
                     break;
             }
         }
      }
      else
      {
         int len = strlen(file);
         if(len > 4)
         {
            if(stricmp(file + len - 4, ".ico") == 0)
               icon = WinLoadFileIcon((PSZ)file, FALSE);
            else
               _dw_load_bitmap_file(file, handle, &hbm, &hdc, &hps, &width, &height, &depth, DW_CLR_DEFAULT);
         }
      }

      if(!hdc && !icon)
         return DW_ERROR_GENERAL;

      dw_window_set_data(handle, "_dw_hps", (void *)hps);
      dw_window_set_data(handle, "_dw_hdc", (void *)hdc);
      dw_window_set_data(handle, "_dw_width", (void *)width);
      dw_window_set_data(handle, "_dw_height", (void *)height);
   }
   else
      return DW_ERROR_UNKNOWN;

   dw_window_set_data(handle, "_dw_bitmap", (void *)hbm);

   return _dw_window_set_bitmap(handle, hbm, hdc, hps, width, height, depth, icon);
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
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long id, const char *data, int len)
{
   HBITMAP hbm;
   HPS     hps;
   HDC hdc;
   unsigned long width, height;
   char *file;
   FILE *fp;
   int depth;

   /* Destroy any old bitmap data */
   _dw_free_bitmap(handle);

   if(data)
   {
      file = tmpnam(NULL);
      if(file != NULL)
      {
         fp = fopen(file, "wb");
         if(fp != NULL)
         {
            fwrite(data, 1, len, fp);
            fclose(fp);
            if(!_dw_load_bitmap_file(file, handle, &hbm, &hdc, &hps, &width, &height, &depth, DW_CLR_DEFAULT))
            {
               /* can't use ICO ? */
               unlink(file);
               return DW_ERROR_GENERAL;
            }
         }
         else
         {
            unlink(file);
            return DW_ERROR_GENERAL;
         }
         unlink(file);
      }

      dw_window_set_data(handle, "_dw_hps", (void *)hps);
      dw_window_set_data(handle, "_dw_hdc", (void *)hdc);
      dw_window_set_data(handle, "_dw_width", (void *)width);
      dw_window_set_data(handle, "_dw_height", (void *)height);
   }
   /* If id is non-zero use the resource */
   else if(id)
   {
      hps = WinGetPS(handle);
      hbm = GpiLoadBitmap( hps, NULLHANDLE, id, 0, 0 );
      WinReleasePS(hps);
   }
   else
      return DW_ERROR_UNKNOWN;

   dw_window_set_data(handle, "_dw_bitmap", (void *)hbm);

   return _dw_window_set_bitmap(handle, hbm, hdc, hps, width, height, depth, 0);
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, const char *text)
{
   HWND entryfield = (HWND)dw_window_get_data(handle, "_dw_buddy");
   WinSetWindowText(entryfield ? entryfield : handle, (PSZ)text);
   /* If we changed the text... */
   {
      Item *item = _dw_box_item(handle);

      /* Check to see if any of the sizes need to be recalculated */
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
            _dw_redraw(_dw_toplevel_window(handle), TRUE);
         }
      }
   }
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, const char *bubbletext)
{
   HWND buddy = (HWND)dw_window_get_data(handle, "_dw_comboentry");
   WindowData *blah = (WindowData *)WinQueryWindowPtr(buddy ? buddy : handle, QWP_USER);
   const char *text = bubbletext ? bubbletext : "";

   buddy = (HWND)dw_window_get_data(handle, "_dw_buddy");

   if(blah)
       strncpy(blah->bubbletext, text, BUBBLE_HELP_MAX - 1);
   if(buddy && (blah = (WindowData *)WinQueryWindowPtr(buddy, QWP_USER)))
       strncpy(blah->bubbletext, text, BUBBLE_HELP_MAX - 1);
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
   HWND entryfield = (HWND)dw_window_get_data(handle, "_dw_buddy");
   int len = WinQueryWindowTextLength(entryfield ? entryfield : handle);
   char *tempbuf = calloc(1, len + 2);

   WinQueryWindowText(entryfield ? entryfield : handle, len + 1, (PSZ)tempbuf);

   return tempbuf;
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
   char tmpbuf[100] = {0};

   if(handle < 65536)
   {
      char buffer[30];
      HMENUI mymenu;

      sprintf(buffer, "_dw_id%ld", handle);
      mymenu = (HMENUI)dw_window_get_data(_dw_app, buffer);

      if(mymenu && WinIsWindow(dwhab, mymenu))
          dw_menu_item_set_state(mymenu, handle, DW_MIS_DISABLED);
      return;
   }

   if(dw_window_get_data(handle, "_dw_disabled"))
      return;

   WinQueryClassName(handle, 99, (PCH)tmpbuf);
   dw_window_set_data(handle, "_dw_disabled", (void *)1);

   if(tmpbuf[0] == '#')
   {
      int val = atoi(&tmpbuf[1]);
      HWND hwnd;

      switch(val)
      {
      case 2:
      case 6:
      case 10:
      case 32:
      case 7:
         hwnd = _dw_find_entryfield(handle);
         _dw_window_set_color(hwnd ? hwnd : handle, DW_CLR_BLACK, DW_CLR_PALEGRAY);
         dw_signal_connect(hwnd ? hwnd : handle, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_dw_null_key), (void *)100);
            if(val == 2)
            dw_signal_connect(handle, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_dw_null_key), (void *)100);
         if(hwnd)
            dw_window_set_data(hwnd, "_dw_disabled", (void *)1);
         return;
      case 3:
         if(dw_window_get_data(handle, "_dw_bitmapbutton") && !dw_window_get_data(handle, "_dw_hpixmap"))
            WinEnableWindow(handle, FALSE);
         else if(dw_window_get_data(handle, "_dw_bitmapbutton") && dw_window_get_data(handle, "_dw_hpixmap_disabled"))
            WinInvalidateRect(handle, NULL, FALSE);
         else
            _dw_window_set_color(handle, DW_CLR_DARKGRAY, DW_CLR_PALEGRAY);
         dw_signal_connect(handle, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_dw_null_key), (void *)100);
         dw_signal_connect(handle, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_dw_null_key), (void *)100);
         return;
      }
   }
   WinEnableWindow(handle, FALSE);
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
   ULONG fore = (ULONG)dw_window_get_data(handle, "_dw_fore");
   ULONG back = (ULONG)dw_window_get_data(handle, "_dw_back");
   HWND hwnd = _dw_find_entryfield(handle);

   if(handle < 65536)
   {
      char buffer[30];
      HMENUI mymenu;

      sprintf(buffer, "_dw_id%ld", handle);
      mymenu = (HMENUI)dw_window_get_data(_dw_app, buffer);

      if(mymenu && WinIsWindow(dwhab, mymenu))
          dw_menu_item_set_state(mymenu, handle, DW_MIS_ENABLED);
      return;
   }

   dw_window_set_data(handle, "_dw_disabled", 0);
   if(hwnd)
      dw_window_set_data(hwnd, "_dw_disabled", 0);
   if(fore && back)
      _dw_window_set_color(hwnd ? hwnd : handle, fore-1, back-1);
   dw_signal_disconnect_by_data(handle, (void *)100);
   WinEnableWindow(handle, TRUE);
   if(dw_window_get_data(handle, "_dw_bitmapbutton") && dw_window_get_data(handle, "_dw_hpixmap_disabled"))
      WinInvalidateRect(handle, NULL, FALSE);
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
   HENUM henum;
   HWND child;
   char tmpbuf[100] = {0};

   henum = WinBeginEnumWindows(handle);
   while((child = WinGetNextWindow(henum)) != NULLHANDLE)
   {
      int windowid = WinQueryWindowUShort(child, QWS_ID);
      HWND found;

      WinQueryClassName(child, 99, (PCH)tmpbuf);

      /* If the child is a box (frame) then recurse into it */
      if(strncmp(tmpbuf, "#1", 3)==0)
         if((found = dw_window_from_id(child, id)) != NULLHANDLE)
            return found;

      if(windowid && windowid == id)
      {
         WinEndEnumWindows(henum);
         return child;
      }
   }
   WinEndEnumWindows(henum);
   return NULLHANDLE;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
   Box *thisbox;

      /*
       * If you try and pack an item into itself VERY bad things can happen; like at least an
       * infinite loop on GTK! Lets be safe!
       */
   if(box == item)
   {
      dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
      return;
   }

   if(WinWindowFromID(box, FID_CLIENT))
   {
      HWND intbox = (HWND)dw_window_get_data(box, "_dw_box");
      if(intbox)
      {
         box = intbox;
      }
      else
      {
         box = WinWindowFromID(box, FID_CLIENT);
      }
   }

   thisbox = WinQueryWindowPtr(box, QWP_USER);

   if(thisbox)
   {
      int z, x = 0;
      Item *tmpitem, *thisitem = thisbox->items;
      char tmpbuf[100] = {0};
      HWND frame = (HWND)dw_window_get_data(item, "_dw_combo_box");

      /* Do some sanity bounds checking */
      if(!thisitem)
        thisbox->count = 0;
      if(index < 0)
        index = 0;
      if(index > thisbox->count)
        index = thisbox->count;

      tmpitem = calloc(sizeof(Item), (thisbox->count+1));

      for(z=0;z<thisbox->count;z++)
      {
         if(z == index)
            x++;
         tmpitem[x] = thisitem[z];
         x++;
      }

      WinQueryClassName(item, 99, (PCH)tmpbuf);

      if(vsize && !height)
         height = 1;
      if(hsize && !width)
         width = 1;

      if(strncmp(tmpbuf, "#1", 3)==0 && !dw_window_get_data(item, "_dw_render"))
         tmpitem[index].type = _DW_TYPE_BOX;
      else
      {
         if ( width == 0 && hsize == FALSE )
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
         if ( height == 0 && vsize == FALSE )
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
         _dw_control_size(item, width == DW_SIZE_AUTO ? &tmpitem[index].width : NULL, height == DW_SIZE_AUTO ? &tmpitem[index].height : NULL);

      thisbox->items = tmpitem;

      if(thisitem)
         free(thisitem);

      thisbox->count++;

      WinQueryClassName(item, 99, (PCH)tmpbuf);
      /* Don't set the ownership if it's an entryfield
       * NOTE: spinbutton used to be in this list but it was preventing value change events
       * from firing, so I removed it.  If spinbuttons cause problems revisit this.
       */
      if(strncmp(tmpbuf, "#6", 3)!=0 && /*strncmp(tmpbuf, "#32", 4)!=0 &&*/ strncmp(tmpbuf, "#2", 3)!=0)
          WinSetOwner(item, box);
      WinSetParent(frame ? frame : item, box, FALSE);
      _dw_handle_transparent(box);
      /* Queue a redraw on the top-level window */
      _dw_redraw(_dw_toplevel_window(box), TRUE);
   }
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
   HWND parent = WinQueryWindow(handle, QW_PARENT);

   if(parent != _dw_desktop)
   {
      Box *thisbox = WinQueryWindowPtr(parent, QWP_USER);

      /* If the parent box has items...
       * try to remove it from the layout
       */
      if(thisbox && thisbox->count)
      {
         int z, index = -1;
         Item *tmpitem = NULL, *thisitem = thisbox->items;

         if(!thisitem)
            thisbox->count = 0;

         for(z=0;z<thisbox->count;z++)
         {
            if(thisitem[z].hwnd == handle)
               index = z;
         }

         if(index == -1)
            return DW_ERROR_GENERAL;

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

         /* If it isn't padding, reset the parent */
         if(handle)
            WinSetParent(handle, HWND_OBJECT, FALSE);
         /* Queue a redraw on the top-level window */
         _dw_redraw(_dw_toplevel_window(parent), TRUE);
         return DW_ERROR_NONE;
      }
   }
   return DW_ERROR_GENERAL;
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
   Box *thisbox = WinQueryWindowPtr(box, QWP_USER);

   /* Try to remove it from the layout */
   if(thisbox && index > -1 && index < thisbox->count)
   {
      int z;
      Item *tmpitem = NULL, *thisitem = thisbox->items;
      HWND handle = thisitem[index].hwnd;

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

      /* If it isn't padding, reset the parent */
      if(handle)
         WinSetParent(handle, HWND_OBJECT, FALSE);
      /* Queue a redraw on the top-level window */
      _dw_redraw(_dw_toplevel_window(box), TRUE);
      return handle;
   }
   return 0;
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

/*
 * The following is an attempt to dynamically size a window based on the size of its
 * children before realization. Only applicable when width or height is less than one.
 */
void _get_window_for_size(HWND handle, unsigned long *width, unsigned long *height)
{
   HWND box = WinWindowFromID(handle, FID_CLIENT);
   Box *thisbox = WinQueryWindowPtr(box, QWP_USER);

   if(thisbox)
   {
      int depth = 0;
      RECTL rect = { 0 };

      /* Calculate space requirements */
      _dw_resize_box(thisbox, &depth, *width, *height, 1);

      rect.xRight = thisbox->minwidth;
      rect.yTop = thisbox->minheight;

      /* Take into account the window border and menu here */
      WinCalcFrameRect(handle, &rect, FALSE);

      if(*width < 1) *width = rect.xRight - rect.xLeft;
      if(*height < 1) *height = rect.yTop - rect.yBottom;
   }
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
   /* Attempt to auto-size */
   if ( width < 1 || height < 1 )
      _get_window_for_size(handle, &width, &height);

   /* Finally set the size */
   WinSetWindowPos(handle, NULLHANDLE, 0, 0, width, height, SWP_SIZE);
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
   char tmpbuf[100] = {0};

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

   if(strncmp(tmpbuf, "#1", 3)==0)
   {
      HWND box = WinWindowFromID(handle, FID_CLIENT);

      if(box)
      {
         unsigned long thiswidth = 0, thisheight = 0;

         /* Get the size with the border */
         _get_window_for_size(handle, &thiswidth, &thisheight);

         /* Return what was requested */
         if(width) *width = (int)thiswidth;
         if(height) *height = (int)thisheight;
      }
      else
      {
         Box *thisbox = WinQueryWindowPtr(handle, QWP_USER);

         if(thisbox)
         {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, 0, 0, 1);

            /* Return what was requested */
            if(width) *width = thisbox->minwidth;
            if(height) *height = thisbox->minheight;
         }
      }
   }
   else
      _dw_control_size(handle, width, height);
}

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
   return WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN);
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
   return WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
   HDC hdc = WinOpenWindowDC(HWND_DESKTOP);
   long colors;

   DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1, &colors);
   DevCloseDC(hdc);
   return colors;
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
void _dw_handle_gravity(HWND handle, long *x, long *y, unsigned long width, unsigned long height)
{
   int horz = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_horz"));
   int vert = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_vert"));

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
   /* Adjust the values to avoid WarpCenter/XCenter/eCenter if requested */
   if(_WinQueryDesktopWorkArea && (horz | vert) & DW_GRAV_OBSTACLES)
   {
       RECTL rect;

       _WinQueryDesktopWorkArea(HWND_DESKTOP, &rect);

       if(horz & DW_GRAV_OBSTACLES)
       {
           if((horz & 0xf) == DW_GRAV_LEFT)
               *x += rect.xLeft;
           else if((horz & 0xf) == DW_GRAV_RIGHT)
               *x -= dw_screen_width() - rect.xRight;
       }
       if(vert & DW_GRAV_OBSTACLES)
       {
           if((vert & 0xf) == DW_GRAV_BOTTOM)
               *y += rect.yBottom;
           else if((vert & 0xf) == DW_GRAV_TOP)
               *y -= dw_screen_height() - rect.yTop;
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
   unsigned long width, height;

   dw_window_get_pos_size(handle, NULL, NULL, &width, &height);
   /* Can't position an unsized window, so attempt to auto-size */
   if(width == 0 || height == 0)
   {
      dw_window_set_size(handle, 0, 0);
      dw_window_get_pos_size(handle, NULL, NULL, &width, &height);
   }
   _dw_handle_gravity(handle, &x, &y, width, height);
   WinSetWindowPos(handle, NULLHANDLE, x, y, 0, 0, SWP_MOVE);
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
   /* Attempt to auto-size */
   if ( width < 1 || height < 1 )
      _get_window_for_size(handle, &width, &height);

   _dw_handle_gravity(handle, &x, &y, width, height);
   /* Finally set the size */
   WinSetWindowPos(handle, NULLHANDLE, x, y, width, height, SWP_MOVE | SWP_SIZE);
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
   SWP swp;
   WinQueryWindowPos(handle, &swp);
   if(x)
      *x = swp.x;
   if(y)
      *y = _dw_get_frame_height(handle) - (swp.y + swp.cy);
   if(width)
      *width = swp.cx;
   if(height)
      *height = swp.cy;
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
   if(handle < 65536)
   {
      char buffer[30];
      HMENUI mymenu;

      sprintf(buffer, "_dw_id%ld", handle);
      mymenu = (HMENUI)dw_window_get_data(_dw_app, buffer);

      if(mymenu && WinIsWindow(dwhab, mymenu))
          dw_menu_item_set_state(mymenu, handle, style & mask);
   }
   else
      WinSetWindowBits(handle, QWL_STYLE, style, mask);
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
   ULONG retval = (ULONG)WinSendMsg(handle, BKM_INSERTPAGE, 0L,
                                    MPFROM2SHORT((BKA_STATUSTEXTON | BKA_AUTOPAGESIZE | BKA_MAJOR | flags), front ? BKA_FIRST : BKA_LAST));
   RECTL rect;
   WinQueryWindowRect(handle, &rect);
   WinInvalidateRect(handle, &rect, TRUE);
   return retval;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned long pageid)
{
   HWND pagehwnd = (HWND)WinSendMsg(handle, BKM_QUERYPAGEWINDOWHWND,
                                    MPFROMLONG(pageid), 0L);
   RECTL rect;
   WinSendMsg(handle, BKM_DELETEPAGE,
            MPFROMLONG(pageid),  (MPARAM)BKA_SINGLE);
   if(pagehwnd)
      dw_window_destroy(pagehwnd);
   WinQueryWindowRect(handle, &rect);
   WinInvalidateRect(handle, &rect, TRUE);
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
   return (unsigned long)WinSendMsg(handle, BKM_QUERYPAGEID,0L, MPFROM2SHORT(BKA_TOP, BKA_MAJOR));
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned long pageid)
{
   WinSendMsg(handle, BKM_TURNTOPAGE, MPFROMLONG(pageid), 0L);
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_text(HWND handle, ULONG pageid, const char *text)
{
   WinSendMsg(handle, BKM_SETTABTEXT,
            MPFROMLONG(pageid),  MPFROMP(text));
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
   WinSendMsg(handle, BKM_SETSTATUSLINETEXT,
            MPFROMLONG(pageid),  MPFROMP(text));
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
   HWND tmpbox = dw_box_new(DW_VERT, 0);

   dw_box_pack_start(tmpbox, page, 0, 0, TRUE, TRUE, 0);
   WinSubclassWindow(tmpbox, _dw_wndproc);
   WinSendMsg(handle, BKM_SETPAGEWINDOWHWND,
            MPFROMLONG(pageid),  MPFROMHWND(tmpbox));
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, const char *text)
{
   WinSendMsg(handle,
            LM_INSERTITEM,
            MPFROMSHORT(LIT_END),
            MPFROMP(text));
}

/*
 * Inserts the specified text into the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be inserted into.
 *          text: Text to insert into listbox.
 *          pos: 0-based position to insert text
 */
void API dw_listbox_insert(HWND handle, const char *text, int pos)
{
   WinSendMsg(handle,
            LM_INSERTITEM,
            MPFROMSHORT(pos),
            MPFROMP(text));
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
   int i;
   for(i=0;i<count;i++)
      WinSendMsg(handle,
               LM_INSERTITEM,
               MPFROMSHORT(LIT_END),
               MPFROMP(text[i]));
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
   WinSendMsg(handle,
            LM_DELETEALL, 0L, 0L);
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
int API dw_listbox_count(HWND handle)
{
   return (int)WinSendMsg(handle,
                     LM_QUERYITEMCOUNT,0L, 0L);
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void API dw_listbox_set_top(HWND handle, int top)
{
   WinSendMsg(handle,
            LM_SETTOPINDEX,
            MPFROMSHORT(top),
            0L);
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
   WinSendMsg(handle, LM_QUERYITEMTEXT, MPFROM2SHORT(index, length), (MPARAM)buffer);
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
void API dw_listbox_set_text(HWND handle, unsigned int index, const char *buffer)
{
   WinSendMsg(handle, LM_SETITEMTEXT, MPFROMSHORT(index), (MPARAM)buffer);
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
int API dw_listbox_selected(HWND handle)
{
      return (unsigned int)WinSendMsg(handle,
                              LM_QUERYSELECTION,
                              MPFROMSHORT(LIT_CURSOR),
                              0);
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
   int place = where;

   if(where == -1)
      place = LIT_FIRST;

   place = (int)WinSendMsg(handle,
                     LM_QUERYSELECTION,
                     MPFROMSHORT(place),0L);
   if(place == LIT_NONE)
      return -1;
   return place;
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
   char tmpbuf[100] = {0};

   WinSendMsg(handle, LM_SELECTITEM, MPFROMSHORT(index), (MPARAM)state);

   WinQueryClassName(handle, 99, (PCH)tmpbuf);

   /* If we are setting a combobox call the event handler manually */
   if(strncmp(tmpbuf, "#6", 3)==0)
      _dw_run_event(handle, WM_CONTROL, MPFROM2SHORT(0, LN_SELECT), (MPARAM)handle);
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
   WinSendMsg(handle, LM_DELETEITEM, MPFROMSHORT(index), 0);
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 */
unsigned int API dw_mle_import(HWND handle, const char *buffer, int startpoint)
{
   long point = startpoint < 0 ? 0 : startpoint;
   PBYTE mlebuf;

   /* Work around 64K limit */
   if(!DosAllocMem((PPVOID) &mlebuf, 65536, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_TILE))
   {
      int amount, len = strlen(buffer), written = 0;

      while(written < len)
      {
         int z, x = 0;

         if((len - written) > 65535)
            amount = 65535;
         else
            amount = len - written;

         /* Remove Carriage Returns \r */
         for(z=0;z<amount;z++)
         {
             if(buffer[z] != '\r')
             {
                 mlebuf[x] = buffer[z];
                 x++;
             }
         }

         if(point < 0)
             point = 0;
         WinSendMsg(handle, MLM_SETIMPORTEXPORT, MPFROMP(mlebuf), MPFROMLONG(x));
         WinSendMsg(handle, MLM_IMPORT, MPFROMP(&point), MPFROMLONG(x));

         written += amount;
      }
      DosFreeMem(mlebuf);
   }
   return point;
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
   PBYTE mlebuf;

   /* Work around 64K limit */
   if(!DosAllocMem((PPVOID) &mlebuf, 65535, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_TILE))
   {
      int amount, copied, written = 0;

      while(written < length)
      {
         if((length - written) > 65535)
            amount = 65535;
         else
            amount = length - written;

         WinSendMsg(handle, MLM_SETIMPORTEXPORT, MPFROMP(mlebuf), MPFROMLONG(amount));
         copied = (int)WinSendMsg(handle, MLM_EXPORT, MPFROMP(&startpoint), MPFROMLONG(&amount));

         if(copied)
         {
            memcpy(&buffer[written], mlebuf, copied);

            written += copied;
         }
         else
            break;
      }
      DosFreeMem(mlebuf);
   }
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
   if(bytes)
      *bytes = (unsigned long)WinSendMsg(handle, MLM_QUERYTEXTLENGTH, 0, 0);
   if(lines)
      *lines = (unsigned long)WinSendMsg(handle, MLM_QUERYLINECOUNT, 0, 0);
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
   WinSendMsg(handle, MLM_DELETE, MPFROMLONG(startpoint), MPFROMLONG(length));
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
   unsigned long bytes;

   dw_mle_get_size(handle, &bytes, NULL);

   WinSendMsg(handle, MLM_DELETE, MPFROMLONG(0), MPFROMLONG(bytes));
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
   int tmppnt = (int)WinSendMsg(handle, MLM_CHARFROMLINE, MPFROMLONG(line), 0);
   WinSendMsg(handle, MLM_SETSEL, MPFROMLONG(tmppnt), MPFROMLONG(tmppnt));
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
   WinSendMsg(handle, MLM_SETREADONLY, MPFROMLONG(state ? FALSE : TRUE), 0);
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
   WinSendMsg(handle, MLM_SETWRAP, MPFROMLONG(state), 0);
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
void API dw_mle_set_auto_complete(HWND handle, int state)
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
   WinSendMsg(handle, MLM_SETSEL, MPFROMLONG(point), MPFROMLONG(point));
}

/*
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 */
int API dw_mle_search(HWND handle, const char *text, int point, unsigned long flags)
{
   MLE_SEARCHDATA msd;

   /* This code breaks with structure packing set to 1 (/Sp1 in VAC)
    * if this is needed we need to add a pragma here.
    */
   msd.cb = sizeof(msd);
   msd.pchFind = (char *)text;
   msd.pchReplace = NULL;
   msd.cchFind = strlen(text);
   msd.cchReplace = 0;
   msd.iptStart = point;
   msd.iptStop = -1;

   if(WinSendMsg(handle, MLM_SEARCH, MPFROMLONG(MLFSEARCH_SELECTMATCH | flags), (MPARAM)&msd))
      return (int)WinSendMsg(handle, MLM_QUERYSEL,(MPARAM)MLFQS_MAXSEL, 0);
   return 0;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
   WinSendMsg(handle, MLM_DISABLEREFRESH, 0, 0);
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
   WinSendMsg(handle, MLM_ENABLEREFRESH, 0, 0);
}

/* Internal version that can be called from _dw_percent_thread */
void _dw_percent_set_pos(HWND handle, unsigned int position)
{
   int range = _dw_percent_get_range(handle);

   if(range)
   {
      int mypos = (((float)position)/100)*range;

      if(mypos >= range)
          mypos = range - 1;

      _dw_int_set(handle, mypos);
      WinSendMsg(handle, SLM_SETSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_RANGEVALUE), (MPARAM)mypos);
   }
}

/* Move the percentage bar backwards to simulate indeterminate */
void _dw_percent_thread(void *data)
{
   HWND percent = (HWND)data;

   if(percent)
   {
       HAB thishab = WinInitialize(0);
       HMQ thishmq = WinCreateMsgQueue(dwhab, 0);

       int pos = 100;

       do
       {
           pos--;
           if(pos < 1)
               pos = 100;
           _dw_percent_set_pos(percent, pos);
           DosSleep(100);
       }
       while(dw_window_get_data(percent, "_dw_ind"));

       WinDestroyMsgQueue(thishmq);
       WinTerminate(thishab);
   }
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
   /* OS/2 doesn't really support indeterminate... */
   if(position == DW_PERCENT_INDETERMINATE)
   {
       if(!dw_window_get_data(handle, "_dw_ind"))
       {
           /* So we fake it with a thread */
           dw_window_set_data(handle, "_dw_ind", (void *)1);
           _beginthread(_dw_percent_thread, NULL, 100, (void *)handle);
       }
   }
   else
   {
       /* Make sure we are no longer indeterminate */
       dw_window_set_data(handle, "_dw_ind", NULL);
      /* Otherwise set the position as usual */
       _dw_percent_set_pos(handle, position);
   }
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
   return (unsigned int)WinSendMsg(handle, SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), 0);
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
   dw_window_set_data(handle, "_dw_slider_value", (void *)position);
   WinSendMsg(handle, SLM_SETSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE), (MPARAM)position);
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
   return (unsigned int)WinSendMsg(handle, SBM_QUERYPOS, 0, 0);
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
   dw_window_set_data(handle, "_dw_scrollbar_value", (void *)position);
   WinSendMsg(handle, SBM_SETPOS, (MPARAM)position, 0);
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
   unsigned int pos = (unsigned int)dw_window_get_data(handle, "_dw_scrollbar_value");
   WinSendMsg(handle, SBM_SETSCROLLBAR, (MPARAM)pos, MPFROM2SHORT(0, (unsigned short)range - visible));
   WinSendMsg(handle, SBM_SETTHUMBSIZE, MPFROM2SHORT((unsigned short)visible, range), 0);
   dw_window_set_data(handle, "_dw_scrollbar_visible", (void *)visible);
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
   dw_window_set_data(handle, "_dw_updating", (void *)1);
   WinSendMsg(handle, SPBM_SETCURRENTVALUE, MPFROMLONG((long)position), 0L);
   dw_window_set_data(handle, "_dw_updating", NULL);
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
   WinSendMsg(handle, SPBM_SETLIMITS, MPFROMLONG(upper), MPFROMLONG(lower));
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
   WinSendMsg(handle, EM_SETTEXTLIMIT, (MPARAM)limit, (MPARAM)0);
}


/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
   long tmpval = 0L;

   WinSendMsg(handle, SPBM_QUERYVALUE, (MPARAM)&tmpval,0L);
   return tmpval;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int API dw_checkbox_get(HWND handle)
{
   return (int)WinSendMsg(handle,BM_QUERYCHECK,0,0);
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
   WinSendMsg(handle,BM_SETCHECK,MPFROMSHORT(value),0);
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
HTREEITEM API dw_tree_insert_after(HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   ULONG        cbExtra;
   PCNRITEM     pci;
   RECORDINSERT ri;

   if(!item)
      item = (HTREEITEM)CMA_FIRST;

   /* Calculate extra bytes needed for each record besides that needed for the
    * MINIRECORDCORE structure
    */

   cbExtra = sizeof(CNRITEM) - sizeof(MINIRECORDCORE);

   /* Allocate memory for the parent record */

   if((pci = (PCNRITEM)_dw_send_msg(handle, CM_ALLOCRECORD, MPFROMLONG(cbExtra), MPFROMSHORT(1), 0)) == 0)
      return 0;

   /* Fill in the parent record data */

   pci->rc.cb          = sizeof(MINIRECORDCORE);
   pci->rc.pszIcon     = (PSZ)strdup(title);
   pci->rc.hptrIcon    = icon;

   pci->hptrIcon       = icon;
   pci->user           = itemdata;
   pci->parent         = parent;

   memset(&ri, 0, sizeof(RECORDINSERT));

   ri.cb                 = sizeof(RECORDINSERT);
   ri.pRecordOrder       = (PRECORDCORE)item;
   ri.zOrder             = (USHORT)CMA_TOP;
   ri.cRecordsInsert     = 1;
   ri.fInvalidateRecord  = TRUE;

   /* We are about to insert the child records. Set the parent record to be
    * the one we just inserted.
    */
   ri.pRecordParent = (PRECORDCORE)parent;

   /* Insert the record */
   WinSendMsg(handle, CM_INSERTRECORD, MPFROMP(pci), MPFROMP(&ri));

   return (HTREEITEM)pci;
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
HTREEITEM API dw_tree_insert(HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   return dw_tree_insert_after(handle, (HTREEITEM)CMA_END, title, icon, parent, itemdata);
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void API dw_tree_item_change(HWND handle, HTREEITEM item, const char *title, HICN icon)
{
   PCNRITEM pci = (PCNRITEM)item;

   if(!pci)
      return;

   if(pci->rc.pszIcon)
      free(pci->rc.pszIcon);

   pci->rc.pszIcon     = (PSZ)strdup(title);
   pci->rc.hptrIcon    = icon;

   pci->hptrIcon       = icon;

   WinSendMsg(handle, CM_INVALIDATERECORD, (MPARAM)&pci, MPFROM2SHORT(1, CMA_TEXTCHANGED));
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
char * API dw_tree_get_title(HWND DW_UNUSED(handle), HTREEITEM item)
{
   PCNRITEM pci = (PCNRITEM)item;

   if(pci && pci->rc.pszIcon)
      return strdup((char *)pci->rc.pszIcon);
   return NULL;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
HTREEITEM API dw_tree_get_parent(HWND DW_UNUSED(handle), HTREEITEM item)
{
   PCNRITEM pci = (PCNRITEM)item;

   if(pci)
      return pci->parent;
   return (HTREEITEM)0;
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
void API dw_tree_item_set_data(HWND DW_UNUSED(handle), HTREEITEM item, void *itemdata)
{
   PCNRITEM pci = (PCNRITEM)item;

   if(!pci)
      return;

   pci->user = itemdata;
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void * API dw_tree_item_get_data(HWND DW_UNUSED(handle), HTREEITEM item)
{
   PCNRITEM pci = (PCNRITEM)item;

   if(!pci)
      return NULL;
   return pci->user;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if(pCore->flRecordAttr & CRA_SELECTED)
         WinSendMsg(handle, CM_SETRECORDEMPHASIS, (MPARAM)pCore, MPFROM2SHORT(FALSE, CRA_SELECTED | CRA_CURSORED));
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }
   WinSendMsg(handle, CM_SETRECORDEMPHASIS, (MPARAM)item, MPFROM2SHORT(TRUE, CRA_SELECTED | CRA_CURSORED));
   _dw_lastitem = 0;
   _dw_lasthcnr = 0;
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
   dw_container_clear(handle, TRUE);
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
   WinSendMsg(handle, CM_EXPANDTREE, MPFROMP(item), 0);
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
   WinSendMsg(handle, CM_COLLAPSETREE, MPFROMP(item), 0);
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
   PCNRITEM     pci = (PCNRITEM)item;

   if(!item)
      return;

   if(pci->rc.pszIcon)
   {
      free(pci->rc.pszIcon);
      pci->rc.pszIcon = 0;
   }

   WinSendMsg(handle, CM_REMOVERECORD, (MPARAM)&pci, MPFROM2SHORT(1, CMA_INVALIDATE | CMA_FREE));
}

/* Some OS/2 specific container structs */
typedef struct _dwcontainerinfo {
   int count;
   void *data;
   HWND handle;
} DWContainerInfo;

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
   PFIELDINFO details, first, left = NULL;
   FIELDINFOINSERT detin;
   CNRINFO cnri;
   int z;
   ULONG size = sizeof(RECORDCORE);
   ULONG *offStruct = malloc(count * sizeof(ULONG));
   ULONG *tempflags = malloc((count+1) * sizeof(ULONG));
   WindowData *blah = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   ULONG *oldflags = blah ? blah->data : 0;

   if(!offStruct || !tempflags)
      return FALSE;

   memcpy(tempflags, flags, count * sizeof(ULONG));
   tempflags[count] = 0;

   blah->data = tempflags;
   blah->flags = separator;

   if(oldflags)
      free(oldflags);

   while((first = (PFIELDINFO)WinSendMsg(handle, CM_QUERYDETAILFIELDINFO,  0, MPFROMSHORT(CMA_FIRST))) != NULL)
   {
      WinSendMsg(handle, CM_REMOVEDETAILFIELDINFO, (MPARAM)&first, MPFROM2SHORT(1, CMA_FREE));
   }

   /* Figure out the offsets to the items in the struct */
   for(z=0;z<count;z++)
   {
      offStruct[z] = size;
      if(flags[z] & DW_CFA_BITMAPORICON)
         size += sizeof(HPOINTER);
      else if(flags[z] & DW_CFA_STRING)
         size += sizeof(char *);
      else if(flags[z] & DW_CFA_ULONG)
         size += sizeof(ULONG);
      else if(flags[z] & DW_CFA_DATE)
         size += sizeof(CDATE);
      else if(flags[z] & DW_CFA_TIME)
         size += sizeof(CTIME);
   }

   first = details = (PFIELDINFO)WinSendMsg(handle, CM_ALLOCDETAILFIELDINFO, MPFROMLONG(count), 0L);

   if(!first)
   {
      free(offStruct);
      return FALSE;
   }

   for(z=0;z<count;z++)
   {
      if(z==separator-1)
         left=details;
      details->cb = sizeof(FIELDINFO);
      details->flData = flags[z];
      details->flTitle = CFA_FITITLEREADONLY;
      details->pTitleData = strdup(titles[z]);
      details->offStruct = offStruct[z];
      details = details->pNextFieldInfo;
   }

   detin.cb = sizeof(FIELDINFOINSERT);
   detin.fInvalidateFieldInfo = FALSE;
   detin.pFieldInfoOrder = (PFIELDINFO) CMA_FIRST;
   detin.cFieldInfoInsert = (ULONG)count;

   WinSendMsg(handle, CM_INSERTDETAILFIELDINFO, MPFROMP(first), MPFROMP(&detin));

   if(count > separator && separator > 0)
   {
      cnri.cb = sizeof(CNRINFO);
      cnri.pFieldInfoLast = left;
      cnri.xVertSplitbar  = 150;

      WinSendMsg(handle, CM_SETCNRINFO, MPFROMP(&cnri),  MPFROMLONG(CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR));
   }

   cnri.flWindowAttr = CV_DETAIL | CV_MINI | CA_DETAILSVIEWTITLES;
   cnri.slBitmapOrIcon.cx = 16;
   cnri.slBitmapOrIcon.cy = 16;

   WinSendMsg(handle, CM_SETCNRINFO, &cnri, MPFROMLONG(CMA_FLWINDOWATTR | CMA_SLBITMAPORICON));

   free(offStruct);
   return DW_ERROR_NONE;
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
   char **newtitles = malloc(sizeof(char *) * (count + 2));
   unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 2));
   char *coltitle = (char *)dw_window_get_data(handle, "_dw_coltitle");

   newtitles[0] = "";
   newtitles[1] = coltitle ? coltitle : "Filename";

   newflags[0] = DW_CFA_BITMAPORICON | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR;
   newflags[1] = DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

   memcpy(&newtitles[2], titles, sizeof(char *) * count);
   memcpy(&newflags[2], flags, sizeof(unsigned long) * count);

   dw_container_setup(handle, newflags, newtitles, count + 2, count ? 2 : 0);

   free(newtitles);
   free(newflags);
   return DW_ERROR_NONE;
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 */
HICN API dw_icon_load(unsigned long module, unsigned long id)
{
   return WinLoadPointer(HWND_DESKTOP,module,id);
}

#if 0
/* Internal function to create pointer/icon masks */
void _dw_create_mask(HPIXMAP src, HPIXMAP mask, unsigned long backrgb)
{
    LONG maskcolor = (DW_RED_VALUE(backrgb) << 16) | (DW_GREEN_VALUE(backrgb) << 8) | DW_BLUE_VALUE(backrgb);
    int x, y;

    for(x=0; x < src->width; x++)
    {
        for(y=0; y < src->height; y++)
        {
            POINTL pt = {x, y};
            LONG color = GpiQueryPel(src->hps, &pt);

            dw_debug("Mask color %x (%dx%d) %x\n", (int)maskcolor, x, y, (int)color);
            if(color == maskcolor)
            {
                GpiSetColor(mask->hps, CLR_WHITE);
                GpiSetPel(mask->hps, &pt);
                pt.y = y + src->height;
                GpiSetPel(mask->hps, &pt);
            }
        }
    }
}
#endif

/* Internal function to create an icon from an existing pixmap */
HICN _dw_create_icon(HPIXMAP src, unsigned long backrgb)
{
    HPIXMAP pntr = dw_pixmap_new(_dw_app, WinQuerySysValue(HWND_DESKTOP, SV_CXICON), WinQuerySysValue(HWND_DESKTOP, SV_CYICON), src->depth);
    HPIXMAP mask = dw_pixmap_new(_dw_app, pntr->width, pntr->height*2, 1);
    HPIXMAP minipntr = dw_pixmap_new(_dw_app, pntr->width/2, pntr->height/2, src->depth);
    HPIXMAP minimask = dw_pixmap_new(_dw_app, minipntr->width, minipntr->height*2, 1);
    ULONG oldcol = _dw_foreground;
    POINTERINFO pi = {0};

    /* Create the color pointers, stretching it to the necessary size */
    dw_pixmap_stretch_bitblt(0, pntr, 0, 0, pntr->width, pntr->height, 0, src, 0, 0, src->width, src->height);
    dw_pixmap_stretch_bitblt(0, minipntr, 0, 0, minipntr->width, minipntr->height, 0, src, 0, 0, src->width, src->height);

    /* Create the masks, all in black */
    dw_color_foreground_set(DW_CLR_BLACK);
    dw_draw_rect(0, mask, DW_DRAW_FILL, 0, 0, mask->width, mask->height);
    dw_draw_rect(0, minimask, DW_DRAW_FILL, 0, 0, minimask->width, minimask->height);
#if 0
    /* If we have a background color... create masks */
    if(backrgb & DW_RGB_COLOR)
    {
        _dw_create_mask(pntr, mask, backrgb);
        _dw_create_mask(minipntr, minimask, backrgb);
    }
#endif
    _dw_foreground = oldcol;

    /* Assemble the Pointer Info structure */
    pi.hbmPointer = mask->hbm;
    pi.hbmColor = pntr->hbm;
    pi.hbmMiniPointer = minimask->hbm;
    pi.hbmMiniColor = minipntr->hbm;

    /* Destroy the temporary pixmaps */
    mask->hbm = pntr->hbm = minimask->hbm = minipntr->hbm = 0;
    dw_pixmap_destroy(mask);
    dw_pixmap_destroy(pntr);
    dw_pixmap_destroy(minimask);
    dw_pixmap_destroy(minipntr);

    /* Generate the icon */
    return WinCreatePointerIndirect(HWND_DESKTOP, &pi);
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
   char *file = alloca(strlen(filename) + 6);
   HPIXMAP src = alloca(sizeof(struct _hpixmap));
   HICN icon = 0;
   unsigned long defcol = DW_RGB(204, 204, 204);

   if(!file || !src)
      return 0;

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if(access(file, 04) != 0)
   {
       int z;

       /* Try with .ico extention */
       strcat(file, ".ico");
       if(access(file, 04) == 0)
           return WinLoadFileIcon((PSZ)file, FALSE);

       /* Try with supported extensions */
       for(z=0;z<(_gbm_init?NUM_EXTS:1);z++)
       {
           strcpy(file, filename);
           strcat(file, _dw_image_exts[z]);
           if(access(file, 04) == 0 &&
              _dw_load_bitmap_file(file, _dw_app, &src->hbm, &src->hdc, &src->hps, &src->width, &src->height, &src->depth, defcol))
           {
               icon = _dw_create_icon(src, defcol);
               break;
           }
       }
   }
   else if(_dw_load_bitmap_file(file, _dw_app, &src->hbm, &src->hdc, &src->hps, &src->width, &src->height, &src->depth, defcol))
       icon = _dw_create_icon(src, defcol);
   /* Free temporary resources if in use */
   if(icon)
   {
       GpiSetBitmap(src->hps, NULLHANDLE);
       GpiDeleteBitmap(src->hbm);
       GpiAssociate(src->hps, NULLHANDLE);
       GpiDestroyPS(src->hps);
       DevCloseDC(src->hdc);
   }
   /* Otherwise fall back to the classic method */
   return icon ? icon : WinLoadFileIcon((PSZ)file, FALSE);
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
   HICN icon=0;
   char *file;
   FILE *fp;

   if ( !data )
      return 0;
   file = tmpnam( NULL );
   if ( file != NULL )
   {
      fp = fopen( file, "wb" );
      if ( fp != NULL )
      {
         fwrite( data, 1, len, fp );
         fclose( fp );
         icon = dw_icon_load_from_file(file);
      }
      unlink( file );
   }
   return icon;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
   WinDestroyPointer(handle);
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
   WindowData *wd = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   ULONG *flags = wd ? wd->data : 0;
   int z, size = 0, totalsize, count = 0;
   PRECORDCORE temp;
   DWContainerInfo *ci;
   void *blah = NULL;

   if(!flags || rowcount < 1)
      return NULL;

   while(flags[count])
      count++;

   /* Figure out the offsets to the items in the struct */
   for(z=0;z<count;z++)
   {
      if(flags[z] & DW_CFA_BITMAPORICON)
         size += sizeof(HPOINTER);
      else if(flags[z] & DW_CFA_STRING)
         size += sizeof(char *);
      else if(flags[z] & DW_CFA_ULONG)
         size += sizeof(ULONG);
      else if(flags[z] & DW_CFA_DATE)
         size += sizeof(CDATE);
      else if(flags[z] & DW_CFA_TIME)
         size += sizeof(CTIME);
   }

   totalsize = size + sizeof(RECORDCORE);

   z = 0;

   if(!(blah = (void *)_dw_send_msg(handle, CM_ALLOCRECORD, MPFROMLONG(size), MPFROMLONG(rowcount), 0)))
      return NULL;

   temp = (PRECORDCORE)blah;

   for(z=0;z<rowcount;z++)
   {
      temp->cb = totalsize;
      temp = temp->preccNextRecord;
   }

   ci = malloc(sizeof(struct _dwcontainerinfo));

   ci->count = rowcount;
   ci->data = blah;
   ci->handle = handle;

   return (void *)ci;
}

/* Internal function that does the work for set_item and change_item */
void _dw_container_set_item(HWND handle, PRECORDCORE temp, int column, int row, void *data)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   ULONG totalsize, size = 0, *flags = blah ? blah->data : 0;
   int z, currentcount;
   CNRINFO cnr;
   void *dest;

   if(!flags)
      return;

   if(!_dw_send_msg(handle, CM_QUERYCNRINFO, (MPARAM)&cnr, MPFROMSHORT(sizeof(CNRINFO)), 0))
      return;

   currentcount = cnr.cRecords;

   /* Figure out the offsets to the items in the struct */
   for(z=0;z<column;z++)
   {
      if(flags[z] & DW_CFA_BITMAPORICON)
         size += sizeof(HPOINTER);
      else if(flags[z] & DW_CFA_STRING)
         size += sizeof(char *);
      else if(flags[z] & DW_CFA_ULONG)
         size += sizeof(ULONG);
      else if(flags[z] & DW_CFA_DATE)
         size += sizeof(CDATE);
      else if(flags[z] & DW_CFA_TIME)
         size += sizeof(CTIME);
   }

   totalsize = size + sizeof(RECORDCORE);

   for(z=0;z<(row-currentcount);z++)
      temp = temp->preccNextRecord;

   dest = (void *)(((ULONG)temp)+((ULONG)totalsize));

   if(flags[column] & DW_CFA_BITMAPORICON)
   {
       if(data)
           memcpy(dest, data, sizeof(HPOINTER));
       else
           memset(dest, 0, sizeof(HPOINTER));
   }
   else if(flags[column] & DW_CFA_STRING)
   {
      char **newstr = (char **)data, **str = dest;

      if(*str)
         free(*str);

      if(newstr && *newstr)
         *str = strdup(*newstr);
      else
         *str = NULL;
   }
   else if(flags[column] & DW_CFA_ULONG)
   {
       if(data)
           memcpy(dest, data, sizeof(ULONG));
       else
           memset(dest, 0, sizeof(ULONG));
   }
   else if(flags[column] & DW_CFA_DATE)
   {
       if(data)
           memcpy(dest, data, sizeof(CDATE));
       else
           memset(dest, 0, sizeof(CDATE));
   }
   else if(flags[column] & DW_CFA_TIME)
   {
       if(data)
           memcpy(dest, data, sizeof(CTIME));
       else
           memset(dest, 0, sizeof(CTIME));
   }
}

/* Internal function that free()s any strings allocated for a container item */
void _dw_container_free_strings(HWND handle, PRECORDCORE temp)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   ULONG totalsize, size = 0, *flags = blah ? blah->data : 0;
   int z, count = 0;
   char *oldtitle = (char *)temp->pszIcon;

   if(!flags)
      return;

   while(flags[count])
      count++;

   /* Empty and free the title memory */
   temp->pszIcon = temp->pszText = NULL;
   if(oldtitle)
        free(oldtitle);

   /* Figure out the offsets to the items in the struct */
   for(z=0;z<count;z++)
   {
      if(flags[z] & DW_CFA_BITMAPORICON)
         size += sizeof(HPOINTER);
      else if(flags[z] & DW_CFA_STRING)
      {
         char **str;

         totalsize = size + sizeof(RECORDCORE);

         str = (char **)(((ULONG)temp)+((ULONG)totalsize));

         if(*str)
         {
            free(*str);
            *str = NULL;
         }
         size += sizeof(char *);
      }
      else if(flags[z] & DW_CFA_ULONG)
         size += sizeof(ULONG);
      else if(flags[z] & DW_CFA_DATE)
         size += sizeof(CDATE);
      else if(flags[z] & DW_CFA_TIME)
         size += sizeof(CTIME);
   }
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
   DWContainerInfo *ci = (DWContainerInfo *)pointer;

   if(!ci)
      return;

   _dw_container_set_item(handle, (PRECORDCORE)ci->data, column, row, data);
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
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
   int count = 0;

   while(pCore)
   {
      if(count == row)
      {
         _dw_container_set_item(handle, pCore, column, 0, data);
         WinSendMsg(handle, CM_INVALIDATERECORD, (MPARAM)&pCore, MPFROM2SHORT(1, CMA_NOREPOSITION | CMA_TEXTCHANGED));
         return;
      }
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
      count++;
   }
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
   dw_container_change_item(handle, column + 2, row, data);
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
   dw_container_change_item(handle, 0, row, (void *)&icon);
   dw_container_change_item(handle, 1, row, (void *)&filename);
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
void API dw_filesystem_set_file(HWND handle, void *pointer, int row, const char *filename, HICN icon)
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
   WindowData *blah = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   ULONG *flags = blah ? blah->data : 0;
   int rc;

   if(!flags)
      return 0;

   if(flags[column] & DW_CFA_BITMAPORICON)
      rc = DW_CFA_BITMAPORICON;
   else if(flags[column] & DW_CFA_STRING)
      rc = DW_CFA_STRING;
   else if(flags[column] & DW_CFA_ULONG)
      rc = DW_CFA_ULONG;
   else if(flags[column] & DW_CFA_DATE)
      rc = DW_CFA_DATE;
   else if(flags[column] & DW_CFA_TIME)
      rc = DW_CFA_TIME;
   else
      rc = 0;
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
   return dw_container_get_column_type( handle, column + 2 );
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
void API dw_container_set_stripe(HWND DW_UNUSED(handle), unsigned long DW_UNUSED(oddcolor), unsigned long DW_UNUSED(evencolor))
{
    /* Don't think this is possible on OS/2 */
}

/*
 * Sets the width of a column in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          column: Zero based column of width being set.
 *          width: Width of column in pixels.
 */
void API dw_container_set_column_width(HWND DW_UNUSED(handle), int DW_UNUSED(column), int DW_UNUSED(width))
{
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_set_row_title(void *pointer, int row, const char *title)
{
   DWContainerInfo *ci = (DWContainerInfo *)pointer;
   PRECORDCORE temp;
   int z, currentcount;
   CNRINFO cnr;
   char *newtitle;

   if(!ci)
      return;

   temp = (PRECORDCORE)ci->data;

   z = 0;

   if(!_dw_send_msg(ci->handle, CM_QUERYCNRINFO, (MPARAM)&cnr, MPFROMSHORT(sizeof(CNRINFO)), 0))
      return;

   currentcount = cnr.cRecords;

   for(z=0;z<(row-currentcount);z++)
      temp = temp->preccNextRecord;

   newtitle = title ? strdup(title) : NULL;
   temp->pszName = temp->pszIcon = (PSZ)newtitle;
}

/*
 * Changes the title of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_change_row_title(HWND handle, int row, const char *title)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
   int count = 0;

   while(pCore)
   {
      if(count == row)
      {
         char *oldtitle = (char *)pCore->pszIcon;
         char *newtitle = title ? strdup(title) : NULL;
         pCore->pszName = pCore->pszIcon = (PSZ)newtitle;

         WinSendMsg(handle, CM_INVALIDATERECORD, (MPARAM)&pCore, MPFROM2SHORT(1, CMA_NOREPOSITION | CMA_TEXTCHANGED));

         if(oldtitle)
            free(oldtitle);
         return;
      }
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
      count++;
   }
}

/*
 * Sets the data of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_set_row_data(void *pointer, int row, void *data)
{
   DWContainerInfo *ci = (DWContainerInfo *)pointer;
   PRECORDCORE temp;
   int z, currentcount;
   CNRINFO cnr;

   if(!ci)
      return;

   temp = (PRECORDCORE)ci->data;

   z = 0;

   if(!_dw_send_msg(ci->handle, CM_QUERYCNRINFO, (MPARAM)&cnr, MPFROMSHORT(sizeof(CNRINFO)), 0))
      return;

   currentcount = cnr.cRecords;

   for(z=0;z<(row-currentcount);z++)
      temp = temp->preccNextRecord;

   temp->pszText = (PSZ)data;
}

/*
 * Changes the data of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void API dw_container_change_row_data(HWND handle, int row, void *data)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
   int count = 0;

   while(pCore)
   {
      if(count == row)
      {
         pCore->pszText = (PSZ)data;

         WinSendMsg(handle, CM_INVALIDATERECORD, (MPARAM)&pCore, MPFROM2SHORT(1, CMA_NOREPOSITION | CMA_TEXTCHANGED));
         return;
      }
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
      count++;
   }
}

/* Internal function to get the first item with given flags */
PRECORDCORE _dw_container_start(HWND handle, unsigned long flags)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   if(pCore)
   {
       while(pCore)
       {
           if(pCore->flRecordAttr & flags)
           {
               return pCore;
           }
           pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
       }
   }
   return NULL;
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
   RECORDINSERT recin;
   DWContainerInfo *ci = (DWContainerInfo *)pointer;
   PRECORDCORE pCore;

   if(!ci)
      return;

   recin.cb = sizeof(RECORDINSERT);
   recin.pRecordOrder = (PRECORDCORE)CMA_END;
   recin.pRecordParent = NULL;
   recin.zOrder = CMA_TOP;
   recin.fInvalidateRecord = TRUE;
   recin.cRecordsInsert = rowcount;

   _dw_send_msg(handle, CM_INSERTRECORD, MPFROMP(ci->data), MPFROMP(&recin), 0);

   free(ci);

   if((pCore = _dw_container_start(handle, CRA_CURSORED)) != NULL)
   {
       NOTIFYRECORDEMPHASIS pre;

       pre.pRecord = pCore;
       pre.fEmphasisMask = CRA_CURSORED;
       pre.hwndCnr = handle;
       _dw_run_event(handle, WM_CONTROL, MPFROM2SHORT(0, CN_EMPHASIS), (MPARAM)&pre);
       pre.pRecord->flRecordAttr |= CRA_CURSORED;
   }
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
   PCNRITEM pCore;
   int container = (int)dw_window_get_data(handle, "_dw_container");

   if(_dw_emph == handle)
      _dw_clear_emphasis();

   pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if(container)
         _dw_container_free_strings(handle, (PRECORDCORE)pCore);
      else
      {
         /* Free icon text */
         if(pCore->rc.pszIcon)
         {
            free(pCore->rc.pszIcon);
            pCore->rc.pszIcon = 0;
         }
      }
      pCore = (PCNRITEM)pCore->rc.preccNextRecord;/*WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));*/
   }
   _dw_send_msg(handle, CM_REMOVERECORD, (MPARAM)0L, MPFROM2SHORT(0, (redraw ? CMA_INVALIDATE : 0) | CMA_FREE), -1);
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
   RECORDCORE *last, **prc = malloc(sizeof(RECORDCORE *) * rowcount);
   int current = 1;

   prc[0] = last = (RECORDCORE *)WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(last && current < rowcount)
   {
      _dw_container_free_strings(handle, last);
      prc[current] = last = (RECORDCORE *)WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)last, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
      current++;
   }

   _dw_send_msg(handle, CM_REMOVERECORD, (MPARAM)prc, MPFROM2SHORT(current, CMA_INVALIDATE | CMA_FREE), -1);

   free(prc);
}

/*
 * Scrolls container up or down.
 * Parameters:
 *       handle: Handle to the window (widget) to be scrolled.
 *       direction: DW_SCROLL_UP, DW_SCROLL_DOWN, DW_SCROLL_TOP or
 *                  DW_SCROLL_BOTTOM. (rows is ignored for last two)
 *       rows: The number of rows to be scrolled.
 */
void API dw_container_scroll(HWND handle, int direction, long DW_UNUSED(rows))
{
   switch(direction)
   {
   case DW_SCROLL_TOP:
      WinSendMsg(handle, CM_SCROLLWINDOW, MPFROMSHORT(CMA_VERTICAL), MPFROMLONG(-10000000));
        break;
   case DW_SCROLL_BOTTOM:
      WinSendMsg(handle, CM_SCROLLWINDOW, MPFROMSHORT(CMA_VERTICAL), MPFROMLONG(10000000));
      break;
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
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   if(pCore)
   {
      if(flags)
      {
         while(pCore)
         {
            if(pCore->flRecordAttr & flags)
            {
               dw_window_set_data(handle, "_dw_pcore", (void *)pCore);
               if(flags & DW_CR_RETDATA)
                  return (char *)pCore->pszText;
               return pCore->pszIcon ? strdup((char *)pCore->pszIcon) : NULL;
            }
            pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }
      }
      else
      {
         dw_window_set_data(handle, "_dw_pcore", (void *)pCore);
         if(flags & DW_CR_RETDATA)
            return (char *)pCore->pszText;
         return pCore->pszIcon ? strdup((char *)pCore->pszIcon) : NULL;
      }
   }
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
   PRECORDCORE pCore = (PRECORDCORE)dw_window_get_data(handle, "_dw_pcore");

   pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));

   if(pCore)
   {
      if(flags)
      {
         while(pCore)
         {
            if(pCore->flRecordAttr & flags)
            {
               dw_window_set_data(handle, "_dw_pcore", (void *)pCore);
               if(flags & DW_CR_RETDATA)
                  return (char *)pCore->pszText;
               return pCore->pszIcon ? strdup((char *)pCore->pszIcon) : NULL;
            }

            pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
         }
      }
      else
      {
         dw_window_set_data(handle, "_dw_pcore", (void *)pCore);
         if(flags & DW_CR_RETDATA)
            return (char *)pCore->pszText;
         return pCore->pszIcon ? strdup((char *)pCore->pszIcon) : NULL;
      }
   }
    return NULL;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor(HWND handle, const char *text)
{
   RECTL viewport, item;
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if(pCore->pszIcon && strcmp((char *)pCore->pszIcon, text) == 0)
      {
         QUERYRECORDRECT qrr;
         int scrollpixels = 0, midway;

         qrr.cb = sizeof(QUERYRECORDRECT);
         qrr.pRecord = pCore;
         qrr.fRightSplitWindow = 0;
         qrr.fsExtent = CMA_TEXT;

         WinSendMsg(handle, CM_SETRECORDEMPHASIS, (MPARAM)pCore, MPFROM2SHORT(TRUE, CRA_CURSORED));
         WinSendMsg(handle, CM_QUERYVIEWPORTRECT, (MPARAM)&viewport, MPFROM2SHORT(CMA_WORKSPACE, FALSE));
         WinSendMsg(handle, CM_QUERYRECORDRECT, (MPARAM)&item, (MPARAM)&qrr);

         midway = (viewport.yTop - viewport.yBottom)/2;
         scrollpixels = viewport.yTop - (item.yTop + midway);

         WinSendMsg(handle, CM_SCROLLWINDOW, MPFROMSHORT(CMA_VERTICAL),  MPFROMLONG(scrollpixels));
         return;
      }

      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, const char *text)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if(pCore->pszIcon && strcmp((char *)pCore->pszIcon, text) == 0)
      {
         WinSendMsg(handle, CM_REMOVERECORD, (MPARAM)&pCore, MPFROM2SHORT(1, CMA_FREE | CMA_INVALIDATE));
         return;
      }
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_cursor_by_data(HWND handle, void *data)
{
   RECTL viewport, item;
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if((void *)pCore->pszText == data)
      {
         QUERYRECORDRECT qrr;
         int scrollpixels = 0, midway;

         qrr.cb = sizeof(QUERYRECORDRECT);
         qrr.pRecord = pCore;
         qrr.fRightSplitWindow = 0;
         qrr.fsExtent = CMA_TEXT;

         WinSendMsg(handle, CM_SETRECORDEMPHASIS, (MPARAM)pCore, MPFROM2SHORT(TRUE, CRA_CURSORED));
         WinSendMsg(handle, CM_QUERYVIEWPORTRECT, (MPARAM)&viewport, MPFROM2SHORT(CMA_WORKSPACE, FALSE));
         WinSendMsg(handle, CM_QUERYRECORDRECT, (MPARAM)&item, (MPARAM)&qrr);

         midway = (viewport.yTop - viewport.yBottom)/2;
         scrollpixels = viewport.yTop - (item.yTop + midway);

         WinSendMsg(handle, CM_SCROLLWINDOW, MPFROMSHORT(CMA_VERTICAL),  MPFROMLONG(scrollpixels));
         return;
      }

      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_delete_row_by_data(HWND handle, void *data)
{
   PRECORDCORE pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while(pCore)
   {
      if((void *)pCore->pszText == data)
      {
         WinSendMsg(handle, CM_REMOVERECORD, (MPARAM)&pCore, MPFROM2SHORT(1, CMA_FREE | CMA_INVALIDATE));
         return;
      }
      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(handle, QWP_USER);
   RECTL item;
   PRECORDCORE pCore = NULL;
   int max = 0;

   if(blah && !blah->flags)
      return;

   pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)0L, MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
   while(pCore)
   {
      QUERYRECORDRECT qrr;
      int vector;

      qrr.cb = sizeof(QUERYRECORDRECT);
      qrr.pRecord = pCore;
      qrr.fRightSplitWindow = 0;
      qrr.fsExtent = CMA_TEXT;

      WinSendMsg(handle, CM_QUERYRECORDRECT, (MPARAM)&item, (MPARAM)&qrr);

      vector = item.xRight - item.xLeft;

      if(vector > max)
         max = vector;

      pCore = WinSendMsg(handle, CM_QUERYRECORD, (MPARAM)pCore, MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
   }

   if(max)
   {
      CNRINFO cnri;

      cnri.cb = sizeof(CNRINFO);
      cnri.xVertSplitbar  = max;

      WinSendMsg(handle, CM_SETCNRINFO, MPFROMP(&cnri),  MPFROMLONG(CMA_XVERTSPLITBAR));
   }
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
    /* Make sure we have our server */
    if(!_dw_tray)
        return;

    WinSendMsg(_dw_app, WM_SETICON, (MPARAM)icon, 0);
    _dw_task_bar = handle;
    WinPostMsg(_dw_tray, WM_USER+1, (MPARAM)_dw_app, (MPARAM)icon);
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
    /* Make sure we have our server */
    if(!_dw_tray)
        return;

    WinPostMsg(_dw_tray, WM_USER+2, (MPARAM)_dw_app, (MPARAM)0);
    _dw_task_bar = NULLHANDLE;
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
   HWND hwndframe = WinCreateWindow(HWND_OBJECT,
                            WC_FRAME,
                            NULL,
                            WS_VISIBLE |
                            FS_NOBYTEALIGN,
                            0,0,2000,1000,
                            NULLHANDLE,
                            HWND_TOP,
                            id,
                            NULL,
                            NULL);
   WinSubclassWindow(hwndframe, _RendProc);
   dw_window_set_data(hwndframe, "_dw_render", DW_INT_TO_POINTER(1));
   return hwndframe;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
   _dw_foreground = value;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
   _dw_background = value;
}

int DWSIGNAL _dw_color_cancel_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;
   HMTX mtx = (HMTX)dw_window_get_data((HWND)dwwait->data, "_dw_mutex");
   void *val;

   window = (HWND)dwwait->data;
   val = dw_window_get_data(window, "_dw_val");

   dw_mutex_lock(mtx);
   dw_mutex_close(mtx);
   dw_window_destroy(window);
   dw_dialog_dismiss((DWDialog *)data, val);
   return FALSE;
}

int DWSIGNAL _dw_color_ok_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;
   HMTX mtx = (HMTX)dw_window_get_data((HWND)dwwait->data, "_dw_mutex");
   unsigned long val;

   window = (HWND)dwwait->data;
   val = _dw_color_spin_get(window);

   dw_mutex_lock(mtx);
   dw_mutex_close(mtx);
   dw_window_destroy(window);
   dw_dialog_dismiss((DWDialog *)data, (void *)val);
   return FALSE;
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
   HWND window, hbox, vbox, col, button, text;
   DWDialog *dwwait;
   HMTX mtx = dw_mutex_new();

   window = dw_window_new( HWND_DESKTOP, "Choose Color", FCF_TITLEBAR | FCF_DLGBORDER | FCF_CLOSEBUTTON | FCF_SYSMENU);

   vbox = dw_box_new(DW_VERT, 5);

   dw_box_pack_start(window, vbox, 0, 0, TRUE, TRUE, 0);

   hbox = dw_box_new(DW_HORZ, 0);

   dw_box_pack_start(vbox, hbox, 0, 0, FALSE, FALSE, 0);
   dw_window_set_style(hbox, 0, WS_CLIPCHILDREN);

   col = WinCreateWindow(vbox, (PSZ)"ColorSelectClass", NULL, WS_VISIBLE | WS_GROUP, 0, 0, 390, 300, vbox, HWND_TOP, 266, NULL,NULL);
   dw_box_pack_start(hbox, col, 390, 300, FALSE, FALSE, 0);

   dw_window_set_data(hbox, "_dw_window", (void *)window);
   dw_window_set_data(window, "_dw_mutex", (void *)mtx);
   dw_window_set_data(window, "_dw_col", (void *)col);
   dw_window_set_data(window, "_dw_val", (void *)value);

   hbox = dw_box_new(DW_HORZ, 0);
   dw_window_set_data(hbox, "_dw_window", (void *)window);

   dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);

   text = dw_text_new("Red:", 0);
   dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
   dw_box_pack_start(hbox, text, 30, 20, FALSE, FALSE, 3);

   button = dw_spinbutton_new("", 1001L);
   dw_spinbutton_set_limits(button, 255, 0);
   dw_box_pack_start(hbox, button, 20, 20, TRUE, FALSE, 3);
   WinSetOwner(button, hbox);
   dw_window_set_data(window, "_dw_red_spin", (void *)button);

   text = dw_text_new("Green:", 0);
   dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
   dw_box_pack_start(hbox, text, 30, 20, FALSE, FALSE, 3);

   button = dw_spinbutton_new("", 1002L);
   dw_spinbutton_set_limits(button, 255, 0);
   dw_box_pack_start(hbox, button, 20, 20, TRUE, FALSE, 3);
   WinSetOwner(button, hbox);
   dw_window_set_data(window, "_dw_green_spin", (void *)button);

   text = dw_text_new("Blue:", 0);
   dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
   dw_box_pack_start(hbox, text, 30, 20, FALSE, FALSE, 3);

   button = dw_spinbutton_new("", 1003L);
   dw_spinbutton_set_limits(button, 255, 0);
   dw_box_pack_start(hbox, button, 20, 20, TRUE, FALSE, 3);
   WinSetOwner(button, hbox);
   dw_window_set_data(window, "_dw_blue_spin", (void *)button);

   hbox = dw_box_new(DW_HORZ, 0);

   dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
   dw_box_pack_start(hbox, 0, 100, 1, TRUE, FALSE, 0);

   button = dw_button_new("Ok", 1001L);
   dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);

   dwwait = dw_dialog_new((void *)window);

   dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_color_ok_func), (void *)dwwait);

   button = dw_button_new("Cancel", 1002L);
   dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);

   dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_color_cancel_func), (void *)dwwait);
   dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_dw_color_cancel_func), (void *)dwwait);

   dw_window_set_size(window, 400, 400);

   _dw_col_set(col, value);
   _dw_color_spin_set(window, value);

   dw_window_show(window);

   return (unsigned long)dw_dialog_wait(dwwait);
}

HPS _dw_set_hps(HPS hps)
{
   LONG alTable[2];

   alTable[0] = DW_RED_VALUE(_dw_foreground) << 16 | DW_GREEN_VALUE(_dw_foreground) << 8 | DW_BLUE_VALUE(_dw_foreground);
   alTable[1] = DW_RED_VALUE(_dw_background) << 16 | DW_GREEN_VALUE(_dw_background) << 8 | DW_BLUE_VALUE(_dw_background);

   GpiCreateLogColorTable(hps,
                     LCOL_RESET,
                     LCOLF_CONSECRGB,
                     16,
                     2,
                     alTable);
   if(_dw_foreground & DW_RGB_COLOR)
      GpiSetColor(hps, 16);
   else
      GpiSetColor(hps, _dw_internal_color(_dw_foreground));
   if(_dw_background & DW_RGB_COLOR)
      GpiSetBackColor(hps, 17);
   else
      GpiSetBackColor(hps, _dw_internal_color(_dw_background));
   return hps;
}

HPS _dw_set_colors(HWND handle)
{
   HPS hps = WinGetPS(handle);

   _dw_set_hps(hps);
   return hps;
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
   HPS hps;
   int height;
   POINTL ptl;

   if(handle && _dw_render_safe_check(handle))
   {
      hps = _dw_set_colors(handle);
      height = _dw_get_height(handle);
   }
   else if(pixmap)
   {
      hps = _dw_set_hps(pixmap->hps);
      height = pixmap->height;
   }
   else
      return;

   ptl.x = x;
   ptl.y = height - y - 1;

   GpiSetPel(hps, &ptl);
   if(handle)
      WinReleasePS(hps);
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
   HPS hps;
   int height;
   POINTL ptl[2];

   if(handle && _dw_render_safe_check(handle))
   {
      hps = _dw_set_colors(handle);
      height = _dw_get_height(handle);
   }
   else if(pixmap)
   {
      hps = _dw_set_hps(pixmap->hps);
      height = pixmap->height;
   }
   else
      return;

   ptl[0].x = x1;
   ptl[0].y = height - y1 - 1;
   ptl[1].x = x2;
   ptl[1].y = height - y2 - 1;

   GpiMove(hps, &ptl[0]);
   GpiLine(hps, &ptl[1]);

   if(handle)
       WinReleasePS(hps);
}


void _dw_copy_font_settings(HPS hpsSrc, HPS hpsDst)
{
   FONTMETRICS fm;
   FATTRS fat;
   SIZEF sizf;

   GpiQueryFontMetrics(hpsSrc, sizeof(FONTMETRICS), &fm);

   memset(&fat, 0, sizeof(fat));

   fat.usRecordLength  = sizeof(FATTRS);
   fat.lMatch          = fm.lMatch;
#ifdef UNICODE
   fat.usCodePage      = 1208;
#endif
   strcpy(fat.szFacename, fm.szFacename);

   GpiCreateLogFont(hpsDst, 0, 1L, &fat);
   GpiSetCharSet(hpsDst, 1L);

   sizf.cx = MAKEFIXED(fm.lEmInc,0);
   sizf.cy = MAKEFIXED(fm.lMaxBaselineExt,0);
   GpiSetCharBox(hpsDst, &sizf );
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 *       text: Text to be displayed.
 */
void API dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, const char *text)
{
    HPS hps;
    int z, height;
    RECTL rcl;
    char fontname[128];
    POINTL aptl[TXTBOX_COUNT];

    if(handle && _dw_render_safe_check(handle))
    {
        hps = _dw_set_colors(handle);
        height = _dw_get_height(handle);
        _dw_get_pp_font(handle, fontname);
    }
    else if(pixmap)
    {
        HPS pixmaphps = WinGetPS(pixmap->font ? pixmap->font : pixmap->handle);

        hps = _dw_set_hps(pixmap->hps);
        height = pixmap->height;
        _dw_get_pp_font(pixmap->font ? pixmap->font : pixmap->handle, fontname);
        _dw_copy_font_settings(pixmaphps, hps);
        WinReleasePS(pixmaphps);
    }
    else
        return;

    for(z=0;z<strlen(fontname);z++)
    {
        if(fontname[z]=='.')
            break;
    }

    GpiQueryTextBox(hps, strlen(text), (PCH)text, TXTBOX_COUNT, aptl);

    rcl.xLeft = x;
    rcl.yTop = height - y;
    rcl.yBottom = rcl.yTop - (aptl[TXTBOX_TOPLEFT].y - aptl[TXTBOX_BOTTOMLEFT].y);
    rcl.xRight = rcl.xLeft + (aptl[TXTBOX_TOPRIGHT].x - aptl[TXTBOX_TOPLEFT].x);

    if(_dw_background == DW_CLR_DEFAULT)
        WinDrawText(hps, -1, (PCH)text, &rcl, DT_TEXTATTRS, DT_TEXTATTRS, DT_VCENTER | DT_LEFT | DT_TEXTATTRS);
    else
        WinDrawText(hps, -1, (PCH)text, &rcl, _dw_internal_color(_dw_foreground), _dw_internal_color(_dw_background), DT_VCENTER | DT_LEFT | DT_ERASERECT);

    if(handle)
        WinReleasePS(hps);
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
   HPS hps;
   POINTL aptl[TXTBOX_COUNT];

   if(handle)
   {
      hps = _dw_set_colors(handle);
   }
   else if(pixmap)
   {
      HPS pixmaphps = WinGetPS(pixmap->font ? pixmap->font : pixmap->handle);

      hps = _dw_set_hps(pixmap->hps);
      _dw_copy_font_settings(pixmaphps, hps);
      WinReleasePS(pixmaphps);
   }
   else
      return;

   GpiQueryTextBox(hps, strlen(text), (PCH)text, TXTBOX_COUNT, aptl);

   if(width)
      *width = aptl[TXTBOX_TOPRIGHT].x - aptl[TXTBOX_TOPLEFT].x;

   if(height)
      *height = aptl[TXTBOX_TOPLEFT].y - aptl[TXTBOX_BOTTOMLEFT].y;

   if(handle)
      WinReleasePS(hps);
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
void API dw_draw_polygon(HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
{
   HPS hps;
   int thisheight;
   POINTL *pptl;
   POINTL start;
   int i;

   if(handle && _dw_render_safe_check(handle))
   {
      hps = _dw_set_colors(handle);
      thisheight = _dw_get_height(handle);
   }
   else if(pixmap)
   {
      hps = _dw_set_hps(pixmap->hps);
      thisheight = pixmap->height;
   }
   else
      return;
   if ( npoints == 0 )
      return;
   pptl = (POINTL *)malloc(sizeof(POINTL)*npoints);
   if ( pptl == NULL )
      return;
   /*
    * For a filled polygon we need to start an area
    */
   if ( flags & DW_DRAW_FILL )
      GpiBeginArea( hps, 0L );
   if ( npoints )
   {
      /*
       * Move to the first point of the polygon
       */
      start.x = x[0];
      start.y = thisheight - y[0] - 1;
      GpiMove( hps, &start );
      /*
       * Convert the remainder of the x and y points
       */
      for ( i = 1; i < npoints; i++ )
      {
         pptl[i-1].x = x[i];
         pptl[i-1].y = thisheight - y[i] - 1;
      }
      GpiPolyLine( hps, npoints-1, pptl );

      if ( flags & DW_DRAW_FILL )
         GpiEndArea( hps );
   }
   if(handle)
      WinReleasePS(hps);
   free( pptl );
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
   HPS hps;
   int thisheight;
   POINTL ptl[2];

   if(handle && _dw_render_safe_check(handle))
   {
      hps = _dw_set_colors(handle);
      thisheight = _dw_get_height(handle);
   }
   else if(pixmap)
   {
      hps = _dw_set_hps(pixmap->hps);
      thisheight = pixmap->height;
   }
   else
      return;

   ptl[0].x = x;
   ptl[0].y = thisheight - y - 1;
   ptl[1].x = x + width - 1;
   ptl[1].y = thisheight - y - height;

   GpiMove(hps, &ptl[0]);
   GpiBox(hps, (flags & DW_DRAW_FILL) ? DRO_OUTLINEFILL : DRO_OUTLINE, &ptl[1], 0, 0);

   if(handle)
      WinReleasePS(hps);
}

/* VisualAge doesn't seem to have this */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

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
   HPS hps;
   int thisheight;
   ARCPARAMS ap = { 1, 1, 0, 0 };
   POINTL pts[2];
   double r, a1, a2, a;

   if(handle && _dw_render_safe_check(handle))
   {
      hps = _dw_set_colors(handle);
      thisheight = _dw_get_height(handle);
   }
   else if(pixmap)
   {
      hps = _dw_set_hps(pixmap->hps);
      thisheight = pixmap->height;
   }
   else
      return;

   /* Handle full circle/ellipse */
   if(flags & DW_DRAW_FULL)
   {
       pts[0].x = xorigin;
       pts[0].y = thisheight - yorigin - 1;
       GpiMove(hps, pts);
       ap.lP = (x2 - x1)/2;
       ap.lQ = (y2 - y1)/2;
       /* Setup the arc info on the presentation space */
       GpiSetArcParams(hps, &ap);
       GpiFullArc(hps, (flags & DW_DRAW_FILL) ? DRO_OUTLINEFILL : DRO_OUTLINE, MAKEFIXED(1, 1));
   }
   else
   {
       /* For a filled arc we need to start an area */
       if(flags & DW_DRAW_FILL)
           GpiBeginArea(hps, 0L);

       /* Setup the default arc info on the presentation space */
       GpiSetArcParams(hps, &ap);
       pts[0].x = x1;
       pts[0].y = thisheight - y1 - 1;
       /* Move to the initial position */
       GpiMove(hps, pts);
       /* Calculate the midpoint */
       r = 0.5 * (hypot((double)(y1 - yorigin), (double)(x1 - xorigin)) +
                  hypot((double)(y2 - yorigin), (double)(x2 - xorigin)));
       a1 = atan2((double)(y1 - yorigin), (double)(x1 - xorigin));
       a2 = atan2((double)(y2 - yorigin), (double)(x2 - xorigin));
       if(a2 < a1)
           a2 += M_PI * 2;
       a = (a1 + a2) / 2.;
       /* Prepare to draw */
       pts[0].x = (int)(xorigin + r * cos(a));
       pts[0].y = thisheight - (int)(yorigin + r * sin(a)) - 1;
       pts[1].x = x2;
       pts[1].y = thisheight - y2 - 1;
       /* Actually draw the arc */
       GpiPointArc(hps, pts);
       if(flags & DW_DRAW_FILL)
           GpiEndArea(hps);
   }

   if(handle)
      WinReleasePS(hps);
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
   BITMAPINFOHEADER bmih;
   SIZEL sizl = { 0, 0 };
   HPIXMAP pixmap;
   HDC hdc;
   HPS hps;
   ULONG ulFlags;
   LONG cPlanes, cBitCount;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   hps = WinGetPS(handle);

   hdc     = GpiQueryDevice(hps);
   ulFlags = GpiQueryPS(hps, &sizl);

   pixmap->handle = handle;
   pixmap->hdc = DevOpenDC(dwhab, OD_MEMORY, (PSZ)"*", 0L, NULL, hdc);
   pixmap->hps = GpiCreatePS (dwhab, pixmap->hdc, &sizl, ulFlags | GPIA_ASSOC);

   DevQueryCaps(hdc, CAPS_COLOR_PLANES  , 1L, &cPlanes);
   if (!depth)
   {
      DevQueryCaps(hdc, CAPS_COLOR_BITCOUNT, 1L, &cBitCount);
      depth = cBitCount;
   }

   memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
   bmih.cbFix     = sizeof(BITMAPINFOHEADER);
   bmih.cx        = (SHORT)width;
   bmih.cy        = (SHORT)height;
   bmih.cPlanes   = (SHORT)cPlanes;
   bmih.cBitCount = (SHORT)depth;

   pixmap->width = width; pixmap->height = height;
   pixmap->transcolor = DW_CLR_DEFAULT;
   pixmap->depth = depth;

   pixmap->hbm = GpiCreateBitmap(pixmap->hps, (PBITMAPINFOHEADER2)&bmih, 0L, NULL, NULL);

   GpiSetBitmap(pixmap->hps, pixmap->hbm);

   if (depth>8)
      GpiCreateLogColorTable(pixmap->hps, LCOL_PURECOLOR, LCOLF_RGB, 0, 0, NULL );

   WinReleasePS(hps);

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
   char *file = alloca(strlen(filename) + 5);

   if ( !file || !(pixmap = calloc(1,sizeof(struct _hpixmap))) )
      return NULL;

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if ( access(file, 04) != 0 )
   {
       int z;

       /* Try with supported extensions */
       for(z=0;z<(_gbm_init?NUM_EXTS:1);z++)
       {
           strcpy(file, filename);
           strcat(file, _dw_image_exts[z]);
           if(access(file, 04) == 0 &&
              _dw_load_bitmap_file(file, handle, &pixmap->hbm, &pixmap->hdc, &pixmap->hps, &pixmap->width, &pixmap->height, &pixmap->depth, DW_CLR_DEFAULT))
               break;
       }
   }

   /* Try to load the bitmap from file */
   if(!pixmap->hbm)
   {
      free(pixmap);
      return NULL;
   }

   /* Success fill in other values */
   pixmap->handle = handle;
   pixmap->transcolor = DW_CLR_DEFAULT;

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
   char *file;
   FILE *fp;

   if ( !(pixmap = calloc(1,sizeof(struct _hpixmap))) )
      return NULL;

   file = tmpnam( NULL );
   if ( file != NULL )
   {
      fp = fopen( file, "wb" );
      if ( fp != NULL )
      {
         fwrite( data, 1, len, fp );
         fclose( fp );
         if(!_dw_load_bitmap_file(file, handle, &pixmap->hbm, &pixmap->hdc, &pixmap->hps, &pixmap->width, &pixmap->height, &pixmap->depth, DW_CLR_DEFAULT))
         {
            /* can't use ICO ? */
            unlink( file );
            return NULL;
         }
      }
      else
      {
         unlink( file );
         return NULL;
      }
      unlink( file );
   }

   /* Success fill in other values */
   pixmap->handle = handle;
   pixmap->transcolor = DW_CLR_DEFAULT;

   return pixmap;
}

/*
 * Creates a bitmap mask for rendering bitmaps with transparent backgrounds
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
   if ( pixmap )
   {
      pixmap->transcolor = _dw_internal_color(color);
   }
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
   BITMAPINFOHEADER bmih;
   SIZEL sizl = { 0, 0 };
   HPIXMAP pixmap;
   HDC hdc;
   HPS hps;
   ULONG ulFlags;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   hps = WinGetPS(handle);

   hdc     = GpiQueryDevice(hps);
   ulFlags = GpiQueryPS(hps, &sizl);

   pixmap->hdc = DevOpenDC(dwhab, OD_MEMORY, (PSZ)"*", 0L, NULL, hdc);
   pixmap->hps = GpiCreatePS (dwhab, pixmap->hdc, &sizl, ulFlags | GPIA_ASSOC);

   pixmap->hbm = GpiLoadBitmap(pixmap->hps, NULLHANDLE, id, 0, 0);

   GpiQueryBitmapParameters(pixmap->hbm, &bmih);

   GpiSetBitmap(pixmap->hps, pixmap->hbm);

   pixmap->width = bmih.cx; pixmap->height = bmih.cy;
   pixmap->transcolor = DW_CLR_DEFAULT;

   WinReleasePS(hps);

   return pixmap;
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
    if(pixmap && fontname && *fontname)
    {
        if(!pixmap->font)
            pixmap->font = WinCreateWindow(HWND_OBJECT, WC_FRAME, NULL, 0,0,0,1,1, NULLHANDLE, HWND_TOP,0, NULL, NULL);
        WinSetPresParam(pixmap->font, PP_FONTNAMESIZE, strlen(fontname)+1, (void *)fontname);
        return DW_ERROR_NONE;
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
    if(pixmap->font)
        WinDestroyWindow(pixmap->font);
    GpiSetBitmap(pixmap->hps, NULLHANDLE);
    GpiDeleteBitmap(pixmap->hbm);
    GpiAssociate(pixmap->hps, NULLHANDLE);
    GpiDestroyPS(pixmap->hps);
    DevCloseDC(pixmap->hdc);
    free(pixmap);
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
   HPS hpsdest;
   HPS hpssrc;
   POINTL ptl[4];
   int dheight, sheight;
   int count = 3;

   /* Do some sanity checks */
   if((srcheight == -1 || srcwidth == -1) && srcheight != srcwidth)
      return DW_ERROR_GENERAL;

   if(dest && _dw_render_safe_check(dest))
   {
      hpsdest = WinGetPS(dest);
      dheight = _dw_get_height(dest);
   }
   else if(destp)
   {
      hpsdest = destp->hps;
      dheight = destp->height;
   }
   else
      return DW_ERROR_GENERAL;

   if(src)
   {
      hpssrc = WinGetPS(src);
      sheight = _dw_get_height(src);
   }
   else if(srcp)
   {
      hpssrc = srcp->hps;
      sheight = srcp->height;
   }
   else
   {
      if(dest)
         WinReleasePS(hpsdest);
      return DW_ERROR_GENERAL;
   }

   ptl[0].x = xdest;
   ptl[0].y = dheight - (ydest + height);
   ptl[1].x = xdest + width;
   ptl[1].y = dheight - ydest;
   ptl[2].x = xsrc;
   ptl[2].y = sheight - (ysrc + (srcheight != -1 ? srcheight : height));
   if(srcwidth != -1 && srcheight != -1)
   {
      count = 4;
      ptl[3].x = xsrc + srcwidth;
      ptl[3].y = sheight - ysrc;
   }

   /* Handle transparency if requested */
   if(srcp && srcp->transcolor != DW_CLR_DEFAULT)
   {
       IMAGEBUNDLE newIb, oldIb;
       /* Transparent color is put into the background color */
       GpiSetBackColor(hpsdest, srcp->transcolor);
       GpiQueryAttrs(hpsdest, PRIM_IMAGE, IBB_BACK_MIX_MODE, (PBUNDLE)&oldIb);
       newIb.usBackMixMode = BM_SRCTRANSPARENT;
       GpiSetAttrs(hpsdest, PRIM_IMAGE, IBB_BACK_MIX_MODE, 0, (PBUNDLE)&newIb);
       GpiBitBlt(hpsdest, hpssrc, count, ptl, ROP_SRCCOPY, BBO_IGNORE);
       GpiSetAttrs(hpsdest, PRIM_IMAGE, IBB_BACK_MIX_MODE, 0, (PBUNDLE)&oldIb);
   }
   else
   {
       /* Otherwise use the regular BitBlt call */
       GpiBitBlt(hpsdest, hpssrc, count, ptl, ROP_SRCCOPY, BBO_IGNORE);
   }

   if(dest)
      WinReleasePS(hpsdest);
   if(src)
      WinReleasePS(hpssrc);
   return DW_ERROR_NONE;
}

/* Run DosBeep() in a separate thread so it doesn't block */
void _dw_beep_thread(void *data)
{
   int *info = (int *)data;

   if(data)
   {
      DosBeep(info[0], info[1]);
      free(data);
   }
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
   int *info = malloc(sizeof(int) * 2);

   if(info)
   {
      info[0] = freq;
      info[1] = dur;

      _beginthread(_dw_beep_thread, NULL, 100, (void *)info);
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
   char objnamebuf[300] = "";

   return DosLoadModule((PSZ)objnamebuf, sizeof(objnamebuf), (PSZ)name, handle);
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
   return DosQueryProcAddr(handle, 0, (PSZ)name, (PFN*)func);
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int API dw_module_close(HMOD handle)
{
   DosFreeModule(handle);
   return DW_ERROR_NONE;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
{
   HMTX mutex;

   DosCreateMutexSem(NULL, &mutex, 0, FALSE);
   return mutex;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
   DosCloseMutexSem(mutex);
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
   if(_dwtid == dw_thread_id())
   {
      int rc = DosRequestMutexSem(mutex, SEM_IMMEDIATE_RETURN);

      while(rc == ERROR_TIMEOUT)
      {
         dw_main_sleep(10);
         rc = DosRequestMutexSem(mutex, SEM_IMMEDIATE_RETURN);
      }
   }
    else
      DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
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
    if(DosRequestMutexSem(mutex, SEM_IMMEDIATE_RETURN) == NO_ERROR)
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
   DosReleaseMutexSem(mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
{
   HEV blah;

   if(DosCreateEventSem (NULL, &blah, 0L, FALSE))
      return 0;

   return blah;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_reset(HEV eve)
{
   ULONG count;

   if(DosResetEventSem(eve, &count))
      return DW_ERROR_GENERAL;
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
   if(DosPostEventSem(eve))
      return DW_ERROR_GENERAL;
   return DW_ERROR_NONE;
}


/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_wait(HEV eve, unsigned long timeout)
{
   int rc = DosWaitEventSem(eve, timeout);
   if(!rc)
      return DW_ERROR_NONE;
   if(rc == ERROR_TIMEOUT)
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
   if(!eve || ~DosCloseEventSem(*eve))
      return DW_ERROR_GENERAL;
   return DW_ERROR_NONE;
}

/* Create a named event semaphore which can be
 * opened from other processes.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_new(const char *name)
{
   char *semname = malloc(strlen(name)+8);
   HEV ev = 0;

   if(!semname)
      return 0;

   strcpy(semname, "\\sem32\\");
   strcat(semname, name);

   DosCreateEventSem((PSZ)semname, &ev, 0L, FALSE);

   free(semname);
   return ev;
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_get(const char *name)
{
   char *semname = malloc(strlen(name)+8);
   HEV ev;

   if(!semname)
      return 0;

   strcpy(semname, "\\sem32\\");
   strcat(semname, name);

   DosOpenEventSem((PSZ)semname, &ev);

   free(semname);
   return ev;
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_reset(HEV eve)
{
   ULONG count;

   return DosResetEventSem(eve, &count);
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_post(HEV eve)
{
   return DosPostEventSem(eve);
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
   int rc;

   rc = DosWaitEventSem(eve, timeout);
   switch (rc)
   {
   case ERROR_INVALID_HANDLE:
      rc = DW_ERROR_NON_INIT;
      break;
   case ERROR_NOT_ENOUGH_MEMORY:
      rc = DW_ERROR_NO_MEM;
      break;
   case ERROR_INTERRUPT:
      rc = DW_ERROR_INTERRUPT;
      break;
   case ERROR_TIMEOUT:
      rc = DW_ERROR_TIMEOUT;
      break;
   }

   return rc;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_close(HEV eve)
{
   int rc;

   rc = DosCloseEventSem(eve);
   switch (rc)
   {
   case ERROR_INVALID_HANDLE:
      rc = DW_ERROR_NON_INIT;
      break;

   case ERROR_SEM_BUSY:
      rc = DW_ERROR_INTERRUPT;
      break;
   }

   return rc;
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
   char namebuf[1024];

   sprintf(namebuf, "\\sharemem\\%s", name);

   if(DosAllocSharedMem((void *)dest, (PSZ)namebuf, size, PAG_COMMIT | PAG_WRITE | PAG_READ) != NO_ERROR)
      return 0;

   return 1;
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
   char namebuf[1024];

   size = size;
   sprintf(namebuf, "\\sharemem\\%s", name);

   if(DosGetNamedSharedMem((void *)dest, (PSZ)namebuf, PAG_READ | PAG_WRITE) != NO_ERROR)
      return 0;

   return 1;
}

/*
 * Frees a shared memory region previously allocated.
 * Parameters:
 *         handle: Handle obtained from DB_named_memory_allocate.
 *         ptr: The memory address aquired with DB_named_memory_allocate.
 */
int API dw_named_memory_free(HSHM handle, void *ptr)
{
   handle = handle;

   if(DosFreeMem(ptr) != NO_ERROR)
      return -1;
   return 0;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void **_dw_init_thread2(void)
{
   HAB thishab = WinInitialize(0);
   HMQ thishmq = WinCreateMsgQueue(thishab, 0);
   void **threadinfo = (void **)malloc(sizeof(void *) * 2);

   threadinfo[0] = (void *)thishab;
   threadinfo[1] = (void *)thishmq;

#ifndef __WATCOMC__
   *_threadstore() = (void *)threadinfo;
#endif

#ifdef UNICODE
   /* Set the codepage to 1208 (UTF-8) */
   WinSetCp(thishmq, 1208);
#endif
   return threadinfo;
}

void API _dw_init_thread(void)
{
   _dw_init_thread2();
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void _dw_deinit_thread2(void **threadinfo)
{
#ifndef __WATCOMC__
   if(!threadinfo)
      threadinfo = (void **)*_threadstore();
#endif
	
   if(threadinfo)
   {
      HAB thishab = (HAB)threadinfo[0];
      HMQ thishmq = (HMQ)threadinfo[1];

      WinDestroyMsgQueue(thishmq);
      WinTerminate(thishab);
      free(threadinfo);
   }
}

void API _dw_deinit_thread(void)
{
   _dw_deinit_thread2(NULL);
}

/*
 * Encapsulate the message queues on OS/2.
 */
void _dwthreadstart(void *data)
{
   void (API_FUNC threadfunc)(void *) = NULL;
   void **tmp = (void **)data;
   void **threadinfo = _dw_init_thread2();

   threadfunc = (void (API_FUNC)(void *))tmp[0];
   threadfunc(tmp[1]);

   free(tmp);

   _dw_deinit_thread2(threadinfo);
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
    void **tmp = malloc(sizeof(void *) * 2);
    int z = 1;

    /* Clamp the stack size to 4K blocks...
     * since some CRTs require it (VAC)
     */
    while(stack > (4096*z))
        z++;

    tmp[0] = func;
    tmp[1] = data;

    return (DWTID)_beginthread((void (*)(void *))_dwthreadstart, NULL, (z*4096), (void *)tmp);
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
   _dw_deinit_thread();
   _endthread();
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
   return (DWTID)_threadid;
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
   /* Destroy the menu message window */
   dw_window_destroy(_dw_app);

   /* In case we are in a signal handler, don't
    * try to free memory that could possibly be
    * free()'d by the runtime already.
    */
   Root = NULL;

   /* Deinit the GBM */
   if(_gbm_deinit)
   {
       _gbm_deinit();
       _gbm_deinit = NULL;
   }

#ifdef UNICODE
   /* Free the conversion object */
   UniFreeUconvObject(Uconv);
   /* Deregister the Unicode clipboard format */
   WinDeleteAtom(WinQuerySystemAtomTable(), Unicode);
#endif

   /* Destroy the main message queue and anchor block */
   WinDestroyMsgQueue(dwhmq);
   WinTerminate(dwhab);

   /* Free any in use modules */
   DosFreeModule(_dw_wpconfig);
   DosFreeModule(_dw_pmprintf);
   DosFreeModule(_dw_pmmerge);
   DosFreeModule(_dw_gbm);
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
   dw_shutdown();
   /* And finally exit */
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
   HWND tmp = WinCreateWindow(HWND_OBJECT,
                        (PSZ)SplitbarClassName,
                        NULL,
                        WS_VISIBLE | WS_CLIPCHILDREN,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
   if(tmp)
   {
      HWND tmpbox = dw_box_new(DW_VERT, 0);
        float *percent = malloc(sizeof(float));

      dw_box_pack_start(tmpbox, topleft, 1, 1, TRUE, TRUE, 0);
      WinSetParent(tmpbox, tmp, FALSE);
      dw_window_set_data(tmp, "_dw_topleft", (void *)tmpbox);

      tmpbox = dw_box_new(DW_VERT, 0);
      dw_box_pack_start(tmpbox, bottomright, 1, 1, TRUE, TRUE, 0);
      WinSetParent(tmpbox, tmp, FALSE);
      *percent = 50.0;
      dw_window_set_data(tmp, "_dw_bottomright", (void *)tmpbox);
      dw_window_set_data(tmp, "_dw_percent", (void *)percent);
      dw_window_set_data(tmp, "_dw_type", (void *)type);
   }
   return tmp;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void API dw_splitbar_set(HWND handle, float percent)
{
   float *mypercent = (float *)dw_window_get_data(handle, "_dw_percent");
   int type = (int)dw_window_get_data(handle, "_dw_type");
    unsigned long width, height;

   if(mypercent)
      *mypercent = percent;

   dw_window_get_pos_size(handle, NULL, NULL, &width, &height);

   _dw_handle_splitbar_resize(handle, percent, type, width, height);
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
   float *percent = (float *)dw_window_get_data(handle, "_dw_percent");

   if(percent)
      return *percent;
   return 0.0;
}

/* The following two functions graciously contributed by Peter Nielsen. */
static ULONG _ParseBuildLevel (char* pchBuffer, ULONG ulSize) {
   char* pchStart = pchBuffer;
   char* pchEnd = pchStart + ulSize - 2;

   while (pchEnd >= pchStart)
   {
      if ((pchEnd[0] == '#') && (pchEnd[1] == '@'))
      {
         *pchEnd-- = '\0';
         while (pchEnd >= pchStart)
         {
            if ((pchEnd[0] == '@') && (pchEnd[1] == '#'))
            {
               ULONG ulMajor = 0;
               ULONG ulMinor = 0;

               char* pch = pchEnd + 2;
               while (!isdigit ((int)*pch) && *pch)
                  pch++;

               while (isdigit ((int)*pch))
                  ulMajor = ulMajor * 10 + *pch++ - '0';

               if (*pch == '.')
               {
                  while (isdigit ((int)*++pch))
                     ulMinor = ulMinor * 10 + *pch - '0';
               }
               return ((ulMajor << 16) | ulMinor);
            }
            pchEnd--;
         }
      }
      pchEnd--;
   }
   return (0);
}

ULONG _dw_get_system_build_level(void) {
   /* The build level info is normally available in the end of the OS2KRNL file. However, this is not the case in some beta versions of OS/2.
    * We first try to find the info in the 256 last bytes of the file. If that fails, we load the entire file and search it completely.
    */
   ULONG ulBootDrive = 0;
   ULONG ulBuild = 0;
   if (DosQuerySysInfo (QSV_BOOT_DRIVE, QSV_BOOT_DRIVE, &ulBootDrive, sizeof (ulBootDrive)) == NO_ERROR)
   {
      char achFileName[11] = "C:\\OS2KRNL";
      HFILE hfile;
      ULONG ulResult;

        achFileName[0] = (char)('A'+ulBootDrive-1);

      if (DosOpen ((PSZ)achFileName, &hfile, &ulResult, 0, 0, OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS, OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY, NULL) == NO_ERROR)
      {
         ULONG ulFileSize = 0;
         if (DosSetFilePtr (hfile, 0, FILE_END, &ulFileSize) == NO_ERROR)
         {
            const ULONG ulFirstTry = min (256, ulFileSize);
            if (DosSetFilePtr (hfile, -(LONG)ulFirstTry, FILE_END, &ulResult) == NO_ERROR)
            {
               char *pchBuffer = malloc(ulFirstTry);
               if (DosRead (hfile, pchBuffer, ulFirstTry, &ulResult) == NO_ERROR)
               {
                  ulBuild = _ParseBuildLevel (pchBuffer, ulFirstTry);
                  if (ulBuild == 0)
                  {
                     if (DosSetFilePtr (hfile, 0, FILE_BEGIN, &ulResult) == NO_ERROR)
                     {
                        free(pchBuffer);
                        pchBuffer = malloc(ulFileSize);

                        if (DosRead (hfile, pchBuffer, ulFileSize, &ulResult) == NO_ERROR)
                           ulBuild = _ParseBuildLevel (pchBuffer, ulFileSize);
                     }
                  }
               }
               free(pchBuffer);
            }
         }
         DosClose (hfile);
      }
   }
   return (ulBuild);
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
    WinSetFocus(HWND_DESKTOP, handle);
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND window, HWND defaultitem)
{
   Box *thisbox = NULL;
   HWND box;

   box = WinWindowFromID(window, FID_CLIENT);
   if(box)
      thisbox = WinQueryWindowPtr(box, QWP_USER);

   if(thisbox)
      thisbox->defaultitem = defaultitem;
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND window, HWND next)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(window, QWP_USER);

   if(blah)
      blah->clickdefault = next;
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
    char *retbuf = NULL;
#ifdef UNICODE
    UniChar *unistr;
#endif

    WinOpenClipbrd(dwhab);

#ifdef UNICODE
    if(!Unicode || !(unistr = (UniChar *)WinQueryClipbrdData(dwhab, Unicode)) ||
       !(retbuf = _dw_WideToUTF8(unistr)))
#endif
    {
        ULONG fmtInfo;

        if (WinQueryClipbrdFmtInfo(dwhab, CF_TEXT, &fmtInfo)) /* Text data in clipboard */
        {
            PSZ pszClipText = (PSZ)WinQueryClipbrdData(dwhab, CF_TEXT); /* Query data handle */
            retbuf = strdup((char *)pszClipText);
        }
    }
    WinCloseClipbrd(dwhab);
    return retbuf;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
    static PVOID shared;
    PVOID old = shared, src = (PVOID)str;
    int buflen = len + 1;

    WinOpenClipbrd(dwhab); /* Open clipboard */
    WinEmptyClipbrd(dwhab); /* Empty clipboard */

    /* Ok, clipboard wants giveable unnamed shared memory */
    shared = NULL;
    if(!DosAllocSharedMem(&shared, NULL, buflen, OBJ_GIVEABLE | PAG_COMMIT | PAG_READ | PAG_WRITE))
    {
        memcpy(shared, src, buflen);

        WinSetClipbrdData(dwhab, (ULONG)shared, CF_TEXT, CFI_POINTER);
    }
    if(old)
        DosFreeMem(old);

#ifdef UNICODE
    if(Unicode)
    {
        UniChar *unistr = NULL;
        static PVOID ushared;
        PVOID uold = ushared;

        src = calloc(len + 1, 1);

        memcpy(src, str, len);
        unistr = _dw_UTF8toWide((char *)src);
        free(src);

        if(unistr)
        {
            buflen = (UniStrlen(unistr) + 1) * sizeof(UniChar);
            src = unistr;
            /* Ok, clipboard wants giveable unnamed shared memory */
            ushared = NULL;
            if(!DosAllocSharedMem(&ushared, NULL, buflen, OBJ_GIVEABLE | PAG_COMMIT | PAG_READ | PAG_WRITE))
            {
                memcpy(ushared, src, buflen);

                WinSetClipbrdData(dwhab, (ULONG)ushared, Unicode, CFI_POINTER);
            }
            free(unistr);
        }
        if(uold)
            DosFreeMem(uold);
    }
#endif

    WinCloseClipbrd(dwhab); /* Close clipboard */
    return;
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
HWND API dw_notification_new(const char * DW_UNUSED(title), const char * DW_UNUSED(imagepath), const char * DW_UNUSED(description), ...)
{
   return 0;
}

/*
 * Sends a notification created by dw_notification_new() after attaching signal handler.
 * Parameters:
 *         notification: The handle to the notification returned by dw_notification_new().
 * Returns:
 *         DW_ERROR_NONE on success, DW_ERROR_UNKNOWN on error or not supported.
 */
int API dw_notification_send(HWND DW_UNUSED(notification))
{
   return DW_ERROR_UNKNOWN;
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
   ULONG Build;

   if(!env)
      return;

   /* The default is OS/2 2.0 */
   strcpy(env->osName,"OS/2");
   env->MajorVersion = 2;
   env->MinorVersion = 0;

   Build = _dw_get_system_build_level();
   env->MinorBuild =  Build & 0xFFFF;
   env->MajorBuild =  Build >> 16;

   if (_dw_ver_buf[0] == 20)
   {
      int i = (unsigned int)_dw_ver_buf[1];
      if (i > 20)
      {
         strcpy(env->osName,"Warp");
         env->MajorVersion = (int)i/10;
         env->MinorVersion = i-(((int)i/10)*10);
      }
      else if (i == 10)
         env->MinorVersion = 1;
   }
   strcpy(env->buildDate, __DATE__);
   strcpy(env->buildTime, __TIME__);
   strcpy(env->htmlEngine, "N/A");
   env->DWMajorVersion = DW_MAJOR_VERSION;
   env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
   env->DWSubVersion = VER_REV;
#else
   env->DWSubVersion = DW_SUB_VERSION;
#endif
}

/* The next few functions are support functions for the OS/2 folder browser */
void _dw_populate_directory(HWND tree, HTREEITEM parent, char *path)
{
   FILEFINDBUF3 ffbuf;
   HTREEITEM item;
   ULONG count = 1;
   HDIR hdir = HDIR_CREATE;

   if(DosFindFirst((PSZ)path, &hdir, FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_ARCHIVED | MUST_HAVE_DIRECTORY,
               &ffbuf, sizeof(FILEFINDBUF3), &count, FIL_STANDARD) == NO_ERROR)
   {
      while(DosFindNext(hdir, &ffbuf, sizeof(FILEFINDBUF3), &count) == NO_ERROR)
      {
         if(strcmp(ffbuf.achName, ".") && strcmp(ffbuf.achName, ".."))
         {
            int len = strlen(path);
            char *folder = malloc(len + ffbuf.cchName + 2);
            HTREEITEM tempitem;

            strcpy(folder, path);
            strcpy(&folder[len-1], ffbuf.achName);

            item = dw_tree_insert(tree, ffbuf.achName, WinLoadFileIcon((PSZ)folder, TRUE), parent, (void *)parent);
            tempitem = dw_tree_insert(tree, "", 0, item, 0);
            dw_tree_item_set_data(tree, item, (void *)tempitem);
         }
      }
      DosFindClose(hdir);
   }
}

void API _dw_populate_tree_thread(void *data)
{
   HWND window = (HWND)data, tree = (HWND)dw_window_get_data(window, "_dw_tree");
   HMTX mtx = (HMTX)dw_window_get_data(window, "_dw_mutex");
   int drive;
   HTREEITEM items[26];
   FSINFO  volinfo;

   DosError(FERR_DISABLEHARDERR);

   dw_mutex_lock(mtx);
   for(drive=0;drive<26;drive++)
   {
      if(DosQueryFSInfo(drive+1, FSIL_VOLSER,(PVOID)&volinfo, sizeof(FSINFO)) == NO_ERROR)
      {
         char folder[5] = "C:\\", name[9] = "Drive C:";
         HTREEITEM tempitem;

         folder[0] = name[6] = 'A' + drive;

         items[drive] = dw_tree_insert(tree, name, WinLoadFileIcon((PSZ)folder, TRUE), NULL, 0);
         tempitem = dw_tree_insert(tree, "", 0, items[drive], 0);
         dw_tree_item_set_data(tree, items[drive], (void *)tempitem);
      }
      else
         items[drive] = 0;
   }
   dw_mutex_unlock(mtx);

   DosError(FERR_ENABLEHARDERR);
}

int DWSIGNAL _dw_ok_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;
   HMTX mtx = (HMTX)dw_window_get_data((HWND)dwwait->data, "_dw_mutex");
   void *treedata;

   window = window;
   if(!dwwait)
      return FALSE;

   dw_mutex_lock(mtx);
   treedata = dw_window_get_data((HWND)dwwait->data, "_dw_tree_selected");
   dw_mutex_close(mtx);
   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, treedata);
   return FALSE;
}

int DWSIGNAL _dw_cancel_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;
   HMTX mtx = (HMTX)dw_window_get_data((HWND)dwwait->data, "_dw_mutex");

   window = window;
   if(!dwwait)
      return FALSE;

   dw_mutex_lock(mtx);
   dw_mutex_close(mtx);
   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, NULL);
   return FALSE;
}

char *_dw_tree_folder(HWND tree, HTREEITEM item)
{
   char *folder=strdup("");
   HTREEITEM parent = item;

   while(parent)
   {
      char *temp, *text = dw_tree_get_title(tree, parent);

      if(text)
      {
         if(strncmp(text, "Drive ", 6) == 0)
            text = &text[6];

         temp = malloc(strlen(text) + strlen(folder) + 3);
         strcpy(temp, text);
         strcat(temp, "\\");
         strcat(temp, folder);
         free(folder);
         folder = temp;
      }
      parent = dw_tree_get_parent(tree, parent);
   }
   return folder;
}

int DWSIGNAL _dw_item_select(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
   DWDialog *dwwait = (DWDialog *)data;
   char *treedata = (char *)dw_window_get_data((HWND)dwwait->data, "_dw_tree_selected");

   text = text; itemdata = itemdata;
   if(treedata)
      free(treedata);

   treedata = _dw_tree_folder(window, item);
   dw_window_set_data((HWND)dwwait->data, "_dw_tree_selected", (void *)treedata);

   return FALSE;
}

int DWSIGNAL _tree_expand(HWND window, HTREEITEM item, void *data)
{
   HTREEITEM tempitem = (HTREEITEM)dw_tree_item_get_data(window, item);

   data = data;
   if(tempitem)
   {
      char *folder = _dw_tree_folder(window, item);

      dw_tree_item_set_data(window, item, 0);
      dw_tree_item_delete(window, tempitem);

      if(*folder)
      {
         strcat(folder, "*");
         _dw_populate_directory(window, item, folder);
      }
      free(folder);
   }

   return FALSE;
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
   if(flags == DW_DIRECTORY_OPEN)
   {
      HWND window, hbox, vbox, tree, button;
      DWDialog *dwwait;
      HMTX mtx = dw_mutex_new();

      window = dw_window_new( HWND_DESKTOP, title, FCF_TITLEBAR | FCF_SIZEBORDER | FCF_MINMAX);

      vbox = dw_box_new(DW_VERT, 5);

      dw_box_pack_start(window, vbox, 0, 0, TRUE, TRUE, 0);

      tree = dw_tree_new(60);

      dw_box_pack_start(vbox, tree, 1, 1, TRUE, TRUE, 0);
      dw_window_set_data(window, "_dw_mutex", (void *)mtx);
      dw_window_set_data(window, "_dw_tree", (void *)tree);

      hbox = dw_box_new(DW_HORZ, 0);

      dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);

      dwwait = dw_dialog_new((void *)window);

      dw_signal_connect(tree, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_dw_item_select), (void *)dwwait);
      dw_signal_connect(tree, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_tree_expand), (void *)dwwait);

      button = dw_button_new("Ok", 1001L);
      dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);
      dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_ok_func), (void *)dwwait);

      button = dw_button_new("Cancel", 1002L);
      dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);
      dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_cancel_func), (void *)dwwait);
      dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_dw_cancel_func), (void *)dwwait);

      dw_window_set_size(window, 225, 300);
      dw_window_show(window);

      dw_thread_new((void *)_dw_populate_tree_thread, (void *)window, 0xff);
      return (char *)dw_dialog_wait(dwwait);
   }
   else
   {
      FILEDLG fild = { 0 };
      HWND hwndFile;
      int len;
      struct stat buf;

      if(defpath)
      {
          if(DosQueryPathInfo((PSZ)defpath, FIL_QUERYFULLNAME, fild.szFullFile, sizeof(fild.szFullFile)))
              strcpy(fild.szFullFile, defpath);
      };

      len = strlen(fild.szFullFile);

      /* If we have a defpath */
      if(len)
      {
          /* Check to see if it exists */
          if(stat(fild.szFullFile, &buf) == 0)
          {
              /* If it is a directory... make sure there is a trailing \ */
              if(buf.st_mode & S_IFDIR)
              {
                  if(fild.szFullFile[len-1] != '\\')
                      strcat(fild.szFullFile, "\\");
                  /* Set len to 0 so the wildcard gets added below */
                  len = 0;
              }
          }
      }

      /* If we need a wildcard (defpath isn't a file) */
      if(!len)
      {
          /* Add a * to get all files... */
          strcat(fild.szFullFile, "*");

          /* If an extension was requested... */
          if(ext)
          {
              /* Limit the results further */
              strcat(fild.szFullFile, ".");
              strcat(fild.szFullFile, ext);
          }
      }

      /* Setup the structure */
      fild.cbSize = sizeof(FILEDLG);
      fild.fl = FDS_CENTER | FDS_OPEN_DIALOG;
      fild.pszTitle = (PSZ)title;
      fild.pszOKButton = (PSZ)((flags & DW_FILE_SAVE) ? "Save" : "Open");
      fild.pfnDlgProc = (PFNWP)WinDefFileDlgProc;

      hwndFile = WinFileDlg(HWND_DESKTOP, HWND_DESKTOP, &fild);
      if(hwndFile)
      {
         switch(fild.lReturn)
         {
         case DID_OK:
            return strdup(fild.szFullFile);
         case DID_CANCEL:
            return NULL;
         }
      }
   }
   return NULL;
}

/* Internal function to set drive and directory */
int _dw_set_path(char *path)
{
#ifndef __WATCOMC__
   if(strlen(path)   > 2)
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

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       -1 on error.
 */
int API dw_exec(const char *program, int DW_UNUSED(type), char **params)
{
#ifdef __EMX__
   return spawnvp(P_NOWAIT, program, (char * const *)params);
#else
   return spawnvp(P_NOWAIT, program, (const char * const *)params);
#endif
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(const char *url)
{
   char *execargs[3], browser[1024], *olddir, *newurl = NULL;
   int len, ret;

   olddir = _getcwd(NULL, 1024);

   PrfQueryProfileString(HINI_USERPROFILE, (PSZ)"WPURLDEFAULTSETTINGS",
                    (PSZ)"DefaultWorkingDir", NULL, (PSZ)browser, 1024);

   if(browser[0])
      _dw_set_path(browser);

   PrfQueryProfileString(HINI_USERPROFILE, (PSZ)"WPURLDEFAULTSETTINGS",
                    (PSZ)"DefaultBrowserExe", NULL, (PSZ)browser, 1024);

   len = strlen(browser) - strlen("explore.exe");

   execargs[0] = browser;
   execargs[1] = (char *)url;
   execargs[2] = NULL;

   /* Special case for Web Explorer, it requires file:/// instead
    * of file:// so I am handling it here.
    */
   if(len > 0)
   {
      if(stricmp(&browser[len], "explore.exe") == 0 && stricmp(url, "file://") == 0)
      {
         int newlen, z;
         newurl = malloc(strlen(url) + 2);
         sprintf(newurl, "file:///%s", &url[7]);
         newlen = strlen(newurl);
         for(z=8;z<(newlen-8);z++)
         {
            if(newurl[z] == '|')
               newurl[z] = ':';
            if(newurl[z] == '/')
               newurl[z] = '\\';
         }
         execargs[1] = newurl;
      }
   }

   ret = dw_exec(browser, DW_EXEC_GUI, execargs);

   if(olddir)
   {
      _dw_set_path(olddir);
      free(olddir);
   }
   if(newurl)
      free(newurl);
   return ret;
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND DW_UNUSED(handle), int DW_UNUSED(action))
{
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
int API dw_html_raw(HWND DW_UNUSED(handle), const char * DW_UNUSED(string))
{
   return DW_ERROR_UNKNOWN;
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
int API dw_html_url(HWND DW_UNUSED(handle), const char * DW_UNUSED(url))
{
   return DW_ERROR_UNKNOWN;
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
int dw_html_javascript_run(HWND DW_UNUSED(handle), const char * DW_UNUSED(script), void * DW_UNUSED(scriptdata))
{
   return DW_ERROR_UNKNOWN;
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
int API dw_html_javascript_add(HWND handle, const char *name)
{
    return DW_ERROR_UNKNOWN;
}

/*
 * Create a new HTML window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_html_new(unsigned long DW_UNUSED(id))
{
   dw_debug("HTML widget not available; OS/2 currently does not support it.\n");
   return 0;
}

typedef struct _dwprint
{
    HDC hdc;
    char *printername;
    int (API_FUNC drawfunc)(HPRINT, HPIXMAP, int, void *);
    void *drawdata;
    unsigned long flags;
    unsigned int startpage, endpage;
    char *jobname;
} DWPrint;

/* Internal functions to handle the print dialog */
int DWSIGNAL _dw_printer_cancel_func(HWND window, void *data)
{
    DWPrint *print = (DWPrint *)data;
    DWDialog *dwwait = (DWDialog *)print->printername;

    window = (HWND)dwwait->data;

    dw_window_destroy(window);
    dw_dialog_dismiss(dwwait, NULL);
    return FALSE;
}

int DWSIGNAL _dw_printer_ok_func(HWND window, void *data)
{
    DWPrint *print = (DWPrint *)data;
    DWDialog *dwwait = (DWDialog *)print->printername;
    HWND printerlist, startspin, endspin;
    char *result = NULL;

    window = (HWND)dwwait->data;
    printerlist = (HWND)dw_window_get_data(window, "_dw_list");
    startspin = (HWND)dw_window_get_data(window, "_dw_start_spin");
    endspin = (HWND)dw_window_get_data(window, "_dw_end_spin");
    if(printerlist)
    {
        char printername[32] = "";
        int selected = dw_listbox_selected(printerlist);

        /* Get the name of the selected printer */
        if(selected != DW_ERROR_UNKNOWN)
        {
            dw_listbox_get_text(printerlist, selected, printername, 32);
            if(printername[0])
                print->printername = result = strdup(printername);
        }
        /* Get the start and end positions */
        print->startpage = (unsigned int)dw_spinbutton_get_pos(startspin);
        print->endpage = (unsigned int)dw_spinbutton_get_pos(endspin);

        /* If the start is bigger than end... swap them */
        if(print->startpage > print->endpage)
        {
            print->endpage = print->startpage;
            print->startpage = (unsigned int)dw_spinbutton_get_pos(endspin);
        }
    }

    dw_window_destroy(window);
    dw_dialog_dismiss(dwwait, (void *)result);
    return FALSE;
}

/* Borrowed functions which should probably be rewritten */
BOOL _dw_extract_log_address(char * LogAddress, char * DetailStr)
{
    char *p;

    p = DetailStr;
    while(*p++ != ';'); /* Gets to first ';' and one char beyond */
    while(*p++ != ';'); /* Gets to second ';' and one char beyond */
    while(*p != ';') *LogAddress++ = *p++;
    *LogAddress = '\0';
    return TRUE;
}

BOOL _dw_extract_driver_name(char * DriverName, char * DetailStr)
{
    char *p;

    p = DetailStr;
    while(*p++ != ';'); /* Gets to first ';' and one char beyond */
    while(*p != '.' && *p != ';' && *p != ',')
        *DriverName++ = *p++;
    *DriverName = '\0';
    return TRUE;
}

/* EMX Doesn't seem to define this? */
#ifndef NERR_BufTooSmall
#define NERR_BufTooSmall 2123
#endif

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
    char printername[32], tmpbuf[20];
    HWND window, hbox, vbox, printerlist, button, text;
    DWDialog *dwwait;
    DWPrint *print;
    PVOID pBuf = NULL;
    ULONG fsType = SPL_PR_QUEUE;
    ULONG cbBuf, cRes, cTotal, cbNeeded;
    SPLERR splerr = 0 ;
    PPRINTERINFO pRes ;    /* Check the default printer for now... want a printer list in the future */
    int cb = PrfQueryProfileString(HINI_PROFILE, (PSZ)"PM_SPOOLER", (PSZ)"PRINTER", (PSZ)"", printername, 32);

    if(!drawfunc || !(print = calloc(1, sizeof(DWPrint))))
        return NULL;

    print->drawfunc = (int (API_FUNC)(HPRINT, HPIXMAP, int, void *))drawfunc;
    print->drawdata = drawdata;
    print->jobname = strdup(jobname ? jobname : "Dynamic Windows Print Job");
    print->startpage = 1;
    print->endpage = pages;
    print->flags = flags;

    /* Check to see how much space we need for the printer list */
    splerr = SplEnumPrinter(NULL, 0, fsType, NULL, 0, &cRes, &cTotal, &cbNeeded ,NULL);

    if(splerr == ERROR_MORE_DATA || splerr == NERR_BufTooSmall)
    {
        /* Allocate memory for the buffer using the count of bytes that were returned in cbNeeded. */
        DosAllocMem(&pBuf, cbNeeded, PAG_READ|PAG_WRITE|PAG_COMMIT);

        /* Set count of bytes in buffer to value used to allocate buffer. */
        cbBuf = cbNeeded;

        /* Call function again with the correct buffer size. */
        splerr = SplEnumPrinter(NULL, 0, fsType, pBuf, cbBuf, &cRes, &cTotal, &cbNeeded, NULL);
    }

    /* Make sure we got a valid result */
    if(cb > 2)
        printername[cb-2] = '\0';
    else
        printername[0] = '\0';

    /* If we didnt' get a printer list or default printer abort */
    if(!cRes && !printername[0])
    {
        /* Show an error and return failure */
        dw_messagebox("Printing", DW_MB_ERROR | DW_MB_OK, "No printers detected.");
        free(print);
        return NULL;
    }

    /* Create the print dialog */
    window = dw_window_new(HWND_DESKTOP, "Choose Printer", FCF_TITLEBAR | FCF_DLGBORDER | FCF_CLOSEBUTTON | FCF_SYSMENU);

    vbox = dw_box_new(DW_VERT, 5);

    dw_box_pack_start(window, vbox, 0, 0, TRUE, TRUE, 0);

    printerlist = dw_listbox_new(0, FALSE);
    dw_box_pack_start(vbox, printerlist, 1, 1, TRUE, TRUE, 0);

    /* If there are any returned structures in the buffer... */
    if(pBuf && cRes)
    {
        int count = 0;

        pRes = (PPRINTERINFO)pBuf ;
        while(cRes--)
        {
            dw_listbox_append(printerlist, (char *)pRes[cRes].pszPrintDestinationName);
            /* If this is the default printer... select it by default */
            if(strcmp((char *)pRes[cRes].pszPrintDestinationName, printername) == 0)
                dw_listbox_select(printerlist, count, TRUE);
            count++;
        }
    }
    else
    {
        /* Otherwise just add the default */
        dw_listbox_append(printerlist, printername);
        dw_listbox_select(printerlist, 0, TRUE);
    }

    /* Free any unneeded memory */
    if(pBuf)
        DosFreeMem(pBuf);

    dw_window_set_data(window, "_dw_list", (void *)printerlist);

    /* Start spinbutton */
    hbox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);

    text = dw_text_new("Start page:", 0);
    dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
    dw_box_pack_start(hbox, text, 70, 20, FALSE, FALSE, 3);

    button = dw_spinbutton_new("1", 0);
    dw_spinbutton_set_limits(button, 1, pages);
    dw_box_pack_start(hbox, button, 20, 20, TRUE, FALSE, 3);
    dw_window_set_data(window, "_dw_start_spin", (void *)button);

    /* End spinbutton */
    hbox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);

    text = dw_text_new("End page:", 0);
    dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
    dw_box_pack_start(hbox, text, 70, 20, FALSE, FALSE, 3);

    sprintf(tmpbuf, "%d", pages);
    button = dw_spinbutton_new(tmpbuf, 0);
    dw_spinbutton_set_limits(button, 1, pages);
    dw_box_pack_start(hbox, button, 20, 20, TRUE, FALSE, 3);
    dw_window_set_data(window, "_dw_end_spin", (void *)button);

    /* Ok and Cancel buttons */
    hbox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(vbox, hbox, 0, 0, TRUE, FALSE, 0);
    dw_box_pack_start(hbox, 0, 100, 1, TRUE, FALSE, 0);

    button = dw_button_new("Ok", 0);
    dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);

    dwwait = dw_dialog_new((void *)window);
    /* Save it temporarily there until we need it */
    print->printername = (char *)dwwait;

    dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_printer_ok_func), (void *)print);

    button = dw_button_new("Cancel", 0);
    dw_box_pack_start(hbox, button, 50, 30, TRUE, FALSE, 3);

    dw_signal_connect(button, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_printer_cancel_func), (void *)print);
    dw_signal_connect(window, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_dw_printer_cancel_func), (void *)print);

    dw_window_set_size(window, 300, 400);

    dw_window_show(window);

    print->printername = dw_dialog_wait(dwwait);

    /* The user picked a printer */
    if(print->printername)
    {
        char PrintDetails[256];
        char DriverName[32];
        char LogAddress[32];
        DEVOPENSTRUC dop;

        /* Get the printer information string */
        cb = PrfQueryProfileString(HINI_PROFILE, (PSZ)"PM_SPOOLER_PRINTER", (PSZ)print->printername, (PSZ)"", PrintDetails, 256);
        _dw_extract_log_address(LogAddress, PrintDetails);
        _dw_extract_driver_name(DriverName, PrintDetails);
        dop.pszDriverName = (PSZ)DriverName;
        dop.pszLogAddress = (PSZ)LogAddress;
        dop.pdriv = NULL;
        dop.pszDataType = (PSZ)"PM_Q_STD";
        /* Attempt to open a device context and return a handle to it */
        print->hdc = DevOpenDC(dwhab, OD_QUEUED, (PSZ)"*", 4L, (PDEVOPENDATA) &dop, (HDC)NULL);
        if(print->hdc)
            return print;
    }
    /* The user canceled */
    if(print->printername)
        free(print->printername);
    free(print);
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
    DWPrint *p = print;
    HPIXMAP pixmap;
    int x, result = DW_ERROR_UNKNOWN;
    SIZEL sizl = { 0, 0 };

    if(!p)
        return result;

    if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
        return result;

    /* Start the job */
    DevEscape(p->hdc, DEVESC_STARTDOC, strlen(p->jobname), (PBYTE)p->jobname, NULL, NULL);

    pixmap->font = WinCreateWindow(HWND_OBJECT, WC_FRAME, NULL, 0,0,0,1,1, NULLHANDLE, HWND_TOP,0, NULL, NULL);
    pixmap->hdc = p->hdc;
    pixmap->hps = GpiCreatePS(dwhab, p->hdc, &sizl, PU_PELS | GPIF_DEFAULT | GPIT_NORMAL | GPIA_ASSOC);
    pixmap->transcolor = DW_RGB_TRANSPARENT;
    pixmap->width = sizl.cx;
    pixmap->height = sizl.cy;
    dw_pixmap_set_font(pixmap, DefaultFont);

    /* Cycle through each page */
    for(x=p->startpage-1; x<p->endpage && p->drawfunc; x++)
    {
        p->drawfunc(print, pixmap, x, p->drawdata);
        /* Next page */
        DevEscape(p->hdc, DEVESC_NEWFRAME, 0, NULL, NULL, NULL);
    }
    /* Determine the completion code */
    if(p->drawfunc)
    {
        result = DW_ERROR_NONE;
        /* Signal that we are done */
        DevEscape(p->hdc, DEVESC_ENDDOC, 0, NULL, NULL, NULL);
    }
    else
        DevEscape(p->hdc, DEVESC_ABORTDOC, 0, NULL, NULL, NULL);
    /* Free memory */
    dw_pixmap_destroy(pixmap);
    if(p->jobname)
        free(p->jobname);
    if(p->printername)
        free(p->printername);
    free(p);
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
 * Returns a pointer to a static buffer which containes the
 * current user directory.  Or the root directory (C:\ on
 * OS/2 and Windows).
 */
char * API dw_user_dir(void)
{
   static char _user_dir[MAX_PATH+1] = {0};

   if(!_user_dir[0])
   {
      char *home = getenv("HOME");

      if(home)
         strcpy(_user_dir, home);
      else
         strcpy(_user_dir, "C:\\");
   }
   return _user_dir;
}

/*
 * Returns a pointer to a static buffer which containes the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    return _dw_exec_dir;
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
int API dw_app_id_set(const char * DW_UNUSED(appid), const char * DW_UNUSED(appname))
{
    return DW_ERROR_UNKNOWN;
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
   WinSendMsg(_dw_toplevel_window(handle), WM_USER, (MPARAM)function, (MPARAM)data);
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
      if(stricmp(tmp->varname, varname) == 0)
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
      if(all || stricmp(tmp->varname, varname) == 0)
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
void API dw_window_set_data(HWND window, const char *dataname, void *data)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(window, QWP_USER);

   if(!blah)
   {
      if(!dataname)
         return;

      blah = calloc(1, sizeof(WindowData));
      WinSetWindowPtr(window, QWP_USER, blah);
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

/*
 * Gets a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void * API dw_window_get_data(HWND window, const char *dataname)
{
   WindowData *blah = (WindowData *)WinQueryWindowPtr(window, QWP_USER);

   if(blah && blah->root && dataname)
   {
      UserData *ud = _dw_find_userdata(&(blah->root), dataname);
      if(ud)
         return ud->data;
   }
   return NULL;
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
HTIMER API dw_timer_connect(int interval, void *sigfunc, void *data)
{
   if(sigfunc)
   {
      HTIMER timerid = WinStartTimer(dwhab, NULLHANDLE, 0, interval);

      if(timerid)
      {
         _dw_new_signal(WM_TIMER, NULLHANDLE, timerid, sigfunc, NULL, data);
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
void API dw_timer_disconnect(HTIMER id)
{
   DWSignalHandler *prev = NULL, *tmp = Root;

   /* 0 is an invalid timer ID */
   if(!id)
      return;

   WinStopTimer(dwhab, NULLHANDLE, id);

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
   ULONG message = 0, id = 0;

   if(window && signame && sigfunc)
   {
      if((message = _dw_findsigmessage(signame)) != 0)
      {
         /* Handle special case of the menu item */
         if(message == WM_COMMAND && window < 65536)
         {
            char buffer[15];
            HWND owner;

            sprintf(buffer, "_dw_id%d", (int)window);
            owner = (HWND)dw_window_get_data(_dw_app, buffer);

            /* Make sure there are no dupes from popups */
            dw_signal_disconnect_by_window(window);

            if(owner)
            {
               id = window;
               window = owner;
            }
         }
         _dw_new_signal(message, window, id, sigfunc, discfunc, data);
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
   DWSignalHandler *prev = NULL, *tmp = Root;
   ULONG message;

   if(!window || !signame || (message = _dw_findsigmessage(signame)) == 0)
      return;

   while(tmp)
   {
      if(((window < 65536 && tmp->id == window) || tmp->window == window) && tmp->message == message)
      {
         void (API_FUNC discfunc)(HWND, void *) = (void (API_FUNC)(HWND, void *))tmp->discfunction;

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
   DWSignalHandler *prev = NULL, *tmp = Root;

   while(tmp)
   {
      if((window < 65536 && tmp->id == window) || tmp->window == window)
      {
         void (API_FUNC discfunc)(HWND, void *) = (void (API_FUNC)(HWND, void *))tmp->discfunction;

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
   DWSignalHandler *prev = NULL, *tmp = Root;

   while(tmp)
   {
      if(((window < 65536 && tmp->id == window) || tmp->window == window) && tmp->data == data)
      {
         void (API_FUNC discfunc)(HWND, void *) = (void (API_FUNC)(HWND, void *))tmp->discfunction;

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
 * Create a new calendar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created calendar or NULL on error.
 */
HWND API dw_calendar_new(ULONG id)
{
    WindowData *blah = calloc(sizeof(WindowData), 1);
    DATETIME dt;
    HWND tmp = WinCreateWindow(HWND_OBJECT,
                        (PSZ)CalendarClassName,
                        NULL,
                        WS_VISIBLE,
                        0,0,2000,1000,
                        NULLHANDLE,
                        HWND_TOP,
                        id,
                        NULL,
                        NULL);
    WinSetWindowPtr(tmp, QWP_USER, blah);
    dw_window_set_font(tmp, DefaultFont);
    if(!DosGetDateTime(&dt))
        dw_calendar_set_date(tmp, dt.year, dt.month, dt.day);
    return tmp;
}

/*
 * Sets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year, month, day: To set the calendar to display.
 */
void API dw_calendar_set_date( HWND window, unsigned int year, unsigned int month, unsigned int day )
{
    /* Need to be 0 based */
    if(year > 0)
        year--;
    if(month > 0)
        month--;
    if(day > 0)
        day--;

    dw_window_set_data(window, "_dw_year", DW_INT_TO_POINTER(year));
    dw_window_set_data(window, "_dw_month", DW_INT_TO_POINTER(month));
    dw_window_set_data(window, "_dw_day", DW_INT_TO_POINTER(day));
    /* Make it redraw */
    WinPostMsg(window, WM_PAINT, 0, 0);
}

/*
 * Gets the year, month and day set in the calendar widget.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year: Variable to store the year or NULL.
 *       month: Variable to store the month or NULL.
 *       day: Variable to store the day or NULL.
 */
void API dw_calendar_get_date( HWND window, unsigned int *year, unsigned int *month, unsigned int *day )
{
    if(year)
        *year = DW_POINTER_TO_UINT(dw_window_get_data(window, "_dw_year")) + 1;
    if(month)
        *month = DW_POINTER_TO_UINT(dw_window_get_data(window, "_dw_month")) + 1;
    if(day)
        *day = DW_POINTER_TO_UINT(dw_window_get_data(window, "_dw_day")) + 1;
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
#ifdef UNICODE
    return _dw_UTF8toWide((char *)utf8string);
#else
    return NULL;
#endif
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
#ifdef UNICODE
    return _dw_WideToUTF8((wchar_t *)wstring);
#else
    return NULL;
#endif
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
#ifdef UNICODE
        case DW_FEATURE_UTF8_UNICODE:
#endif
        case DW_FEATURE_WINDOW_BORDER:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_NOTEBOOK_STATUS_TEXT:
        case DW_FEATURE_MDI:
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_TASK_BAR:
        {
            if(_dw_tray)
                return DW_FEATURE_ENABLED;
            return DW_FEATURE_UNSUPPORTED;
        }
        case DW_FEATURE_RENDER_SAFE:
            return _dw_render_safe_mode;
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
#ifdef UNICODE
        case DW_FEATURE_UTF8_UNICODE:
#endif
        case DW_FEATURE_WINDOW_BORDER:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_NOTEBOOK_STATUS_TEXT:
        case DW_FEATURE_MDI:
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_ERROR_GENERAL;
        case DW_FEATURE_TASK_BAR:
        {
            if(_dw_tray)
                return DW_ERROR_GENERAL;
            return DW_FEATURE_UNSUPPORTED;
        }
        /* These features are supported and configurable */
        case DW_FEATURE_RENDER_SAFE:
        {
            if(state == DW_FEATURE_ENABLED || state == DW_FEATURE_DISABLED)
            {
                _dw_render_safe_mode = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}
