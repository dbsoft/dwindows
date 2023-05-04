/*
 * Dynamic Windows:
 *          A GTK like implementation of the Win32 GUI
 *
 * (C) 2000-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2003-2021 Mark Hessling <mark@rexx.org>
 *
 */

/* Check that the compiler can support AEROGLASS */
#if defined(_MSC_VER) && _MSC_VER < 1600 && defined(AEROGLASS)
#pragma message ( "WARNING: Disabling AEROGLASS support, upgrade to Visual C 2010 and Windows XP." )
#undef AEROGLASS
#endif

#ifdef AEROGLASS
#define _WIN32_IE 0x0501
#define WINVER 0x501
#else
#define _WIN32_IE 0x0500
#define WINVER 0x500
#endif
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <userenv.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <malloc.h>
#include <io.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "dw.h"
#ifdef BUILD_DLL
#include "XBrowseForFolder.h"
#endif
#ifdef AEROGLASS
#include <uxtheme.h>
#endif
#include <richedit.h>

#ifdef RICHEDIT
int _DW_MLE_RICH_EDIT = DW_FEATURE_UNSUPPORTED;
#endif

/* For backwards compatibility BUILD_DLL will imply BUILD_HTML 
 * unless it is explicitly denied by defining NO_BUILD_HTML
 */
#if defined(BUILD_DLL) && !defined(NO_BUILD_HTML) && !defined(BUILD_HTML)
#define BUILD_HTML
#endif

#define STATICCLASSNAME TEXT("STATIC")
#define COMBOBOXCLASSNAME TEXT("COMBOBOX")
#define LISTBOXCLASSNAME TEXT("LISTBOX")
#define BUTTONCLASSNAME TEXT("BUTTON")
#define POPUPMENUCLASSNAME TEXT("POPUPMENU")
#define EDITCLASSNAME TEXT("EDIT")
#define FRAMECLASSNAME TEXT("FRAME")
#define SCROLLBARCLASSNAME TEXT("SCROLLBAR")

#define ClassName TEXT("dynamicwindows")
#define SplitbarClassName TEXT("dwsplitbar")
#define ObjectClassName TEXT("dwobjectclass")
#define BrowserClassName TEXT("dwbrowserclass")
#define ScrollClassName TEXT("dwscrollclass")
#define StatusbarClassName TEXT("dwstatusbar")
#define DefaultFont NULL

#ifdef GDIPLUS
/* GDI+ Headers are not C compatible... so define what we need here instead */
struct GdiplusStartupInput
{
    UINT32 GdiplusVersion;
    void *DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
};

typedef enum  {
  Ok                          = 0,
  GenericError                = 1,
  InvalidParameter            = 2,
  OutOfMemory                 = 3,
  ObjectBusy                  = 4,
  InsufficientBuffer          = 5,
  NotImplemented              = 6,
  Win32Error                  = 7,
  WrongState                  = 8,
  Aborted                     = 9,
  FileNotFound                = 10,
  ValueOverflow               = 11,
  AccessDenied                = 12,
  UnknownImageFormat          = 13,
  FontFamilyNotFound          = 14,
  FontStyleNotFound           = 15,
  NotTrueTypeFont             = 16,
  UnsupportedGdiplusVersion   = 17,
  GdiplusNotInitialized       = 18,
  PropertyNotFound            = 19,
  PropertyNotSupported        = 20,
  ProfileNotFound             = 21
} GpStatus;

typedef enum {
  UnitWorld,
  UnitDisplay,
  UnitPixel,
  UnitPoint,
  UnitInch,
  UnitDocument,
  UnitMillimeter
} GpUnit;

typedef enum  {
  FlushIntentionFlush   = 0,
  FlushIntentionSync    = 1
} GpFlushIntention;

typedef enum {
  QualityModeInvalid  = -1,
  QualityModeDefault  = 0,
  QualityModeLow      = 1,
  QualityModeHigh     = 2
} QualityMode;

typedef enum  {
  SmoothingModeInvalid     = QualityModeInvalid,
  SmoothingModeDefault     = QualityModeDefault,
  SmoothingModeHighSpeed   = QualityModeLow,
  SmoothingModeHighQuality = QualityModeHigh,
  SmoothingModeNone,
  SmoothingModeAntiAlias
} SmoothingMode;

typedef void GpGraphics;
typedef void GpPen;
typedef void GpBrush;
typedef void GpBitmap;
typedef void GpImage;
typedef POINT GpPoint;
typedef DWORD ARGB;
typedef float REAL;

#define ALPHA_SHIFT 24
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0
#define ALPHA_MASK  ((ARGB) 0xff << ALPHA_SHIFT)

#define MAKEARGB(a, r, g, b) \
                (((ARGB) ((a) & 0xff) << ALPHA_SHIFT)| \
                 ((ARGB) ((r) & 0xff) << RED_SHIFT)  | \
                 ((ARGB) ((g) & 0xff) << GREEN_SHIFT)| \
                 ((ARGB) ((b) & 0xff) << BLUE_SHIFT))

/* Graphic conversion functions */
GpStatus WINAPI GdipCreateBitmapFromFile(const WCHAR* filename, GpBitmap **bitmap);
GpStatus WINAPI GdipCreateHBITMAPFromBitmap(GpBitmap *bitmap, HBITMAP* hbmReturn, DWORD background);
GpStatus WINAPI GdipCreateHICONFromBitmap(GpBitmap *bitmap, HICON *hbmReturn);
GpStatus WINAPI GdipDisposeImage(GpImage *image);
GpStatus WINAPI GdipGetImagePixelFormat(GpImage *image, INT *format);
/* Startup and Shutdown */
GpStatus WINAPI GdiplusStartup(ULONG_PTR *token, const struct GdiplusStartupInput *input, void *output);
VOID WINAPI GdiplusShutdown(ULONG_PTR token);
/* Drawing functions */
GpStatus WINAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics);
GpStatus WINAPI GdipCreateFromHWND(HWND hwnd, GpGraphics **graphics);
GpStatus WINAPI GdipDeleteGraphics(GpGraphics *graphics);
GpStatus WINAPI GdipSetSmoothingMode(GpGraphics *graphics, SmoothingMode smoothingMode);
GpStatus WINAPI GdipCreatePen1(ARGB color, REAL width, GpUnit unit, GpPen **pen);
GpStatus WINAPI GdipDeletePen(GpPen *pen);
GpStatus WINAPI GdipCreateSolidFill(ARGB color, GpBrush **brush);
GpStatus WINAPI GdipDeleteBrush(GpBrush *brush);
GpStatus WINAPI GdipSetSolidFillColor(GpBrush *brush, ARGB color);
GpStatus WINAPI GdipDrawLineI(GpGraphics *graphics, GpPen *pen, INT x1, INT y1, INT x2, INT y2);
GpStatus WINAPI GdipDrawLinesI(GpGraphics *graphics, GpPen *pen, const GpPoint *points, INT count);
GpStatus WINAPI GdipDrawRectangleI(GpGraphics *graphics, GpPen *pen, INT x, INT y, INT width, INT height);
GpStatus WINAPI GdipFillRectangleI(GpGraphics *graphics, GpBrush *brush, INT x, INT y, INT width, INT height);
GpStatus WINAPI GdipDrawArcI(GpGraphics *graphics, GpPen *pen, INT x, INT y, INT width, INT height, REAL startAngle, REAL sweepAngle);
GpStatus WINAPI GdipDrawPolygonI(GpGraphics *graphics, GpPen *pen, const GpPoint *points, INT count);
GpStatus WINAPI GdipFillPolygon2I(GpGraphics *graphics, GpBrush *brush, const GpPoint *points, INT count);
GpStatus WINAPI GdipDrawEllipseI(GpGraphics *graphics, GpPen *pen, INT x, INT y, INT width, INT height);
GpStatus WINAPI GdipFillEllipseI(GpGraphics *graphics, GpBrush *brush, INT x, INT y, INT width, INT height);
GpStatus WINAPI GdipFlush(GpGraphics *graphics, GpFlushIntention intention);

/* Pixel format information */
#define    PixelFormatIndexed      0x00010000
#define    PixelFormatGDI          0x00020000
#define    PixelFormatAlpha        0x00040000
#define    PixelFormatPAlpha       0x00080000
#define    PixelFormatExtended     0x00100000
#define    PixelFormatCanonical    0x00200000

#define    PixelFormatUndefined       0
#define    PixelFormatDontCare        0

#define    PixelFormat1bppIndexed     (1 | ( 1 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat4bppIndexed     (2 | ( 4 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat8bppIndexed     (3 | ( 8 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat16bppGrayScale  (4 | (16 << 8) | PixelFormatExtended)
#define    PixelFormat16bppRGB555     (5 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppRGB565     (6 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppARGB1555   (7 | (16 << 8) | PixelFormatAlpha | PixelFormatGDI)
#define    PixelFormat24bppRGB        (8 | (24 << 8) | PixelFormatGDI)
#define    PixelFormat32bppRGB        (9 | (32 << 8) | PixelFormatGDI)
#define    PixelFormat32bppARGB       (10 | (32 << 8) | PixelFormatAlpha | PixelFormatGDI | PixelFormatCanonical)
#define    PixelFormat32bppPARGB      (11 | (32 << 8) | PixelFormatAlpha | PixelFormatPAlpha | PixelFormatGDI)
#define    PixelFormat48bppRGB        (12 | (48 << 8) | PixelFormatExtended)
#define    PixelFormat64bppARGB       (13 | (64 << 8) | PixelFormatAlpha  | PixelFormatCanonical | PixelFormatExtended)
#define    PixelFormat64bppPARGB      (14 | (64 << 8) | PixelFormatAlpha  | PixelFormatPAlpha | PixelFormatExtended)
#define    PixelFormat32bppCMYK       (15 | (32 << 8))

/* Token to the GDI+ Instance */
ULONG_PTR gdiplusToken;
#endif

#ifdef AEROGLASS
HRESULT (WINAPI *_DwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS *pMarInset) = 0;
HRESULT (WINAPI *_DwmIsCompositionEnabled)(BOOL *pfEnabled) = 0;
HRESULT (WINAPI *_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD) = 0;
BOOL (WINAPI *_DwmDefWindowProc)(HWND, UINT, WPARAM, LPARAM, LRESULT *) = 0;
HTHEME (WINAPI *_OpenThemeData)(HWND hwnd, LPCWSTR pszClassList) = 0;
HPAINTBUFFER (WINAPI *_BeginBufferedPaint)(HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc) = 0;
HRESULT (WINAPI *_BufferedPaintSetAlpha)(HPAINTBUFFER hBufferedPaint, const RECT *prc, BYTE alpha) = 0;
HRESULT (WINAPI *_DrawThemeTextEx)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwFlags, LPRECT pRect, const DTTOPTS *pOptions) = 0;
HRESULT (WINAPI *_EndBufferedPaint)(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget) = 0;
HRESULT (WINAPI *_CloseThemeData)(HTHEME hTheme) = 0;
HRESULT (WINAPI *_GetThemeSysFont)(HTHEME hTheme, int iFontId, LOGFONTW *plf) = 0;
DWORD(WINAPI *_GetImmersiveColorFromColorSetEx)(UINT dwImmersiveColorSet, UINT dwImmersiveColorType, BOOL bIgnoreHighContrast, UINT dwHighContrastCacheMode) = 0;
int (WINAPI *_GetImmersiveColorTypeFromName)(const WCHAR *name) = 0;
int (WINAPI *_GetImmersiveUserColorSetPreference)(BOOL bForceCheckRegistry, BOOL bSkipCheckOnFail) = 0;
BOOL _dw_composition = FALSE;
COLORREF _dw_transparencykey = RGB(200,201,202);
HANDLE hdwm = 0;
#endif
/* Aero related but separate functions and handles */
HRESULT (WINAPI *_SetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) = 0;
HANDLE huxtheme = 0;

/* Needed for Rich Edit controls */
HANDLE hrichedit = 0;
HANDLE hmsftedit = 0;

/*
 * MinGW Is missing a bunch of definitions
 * so #define them here...
 */

#if !defined( _tstol )
#if defined(UNICODE)
# define _tstol _wtol
#else
# define _tstol atol
#endif
#endif
#if !defined(PBS_MARQUEE)
# define PBS_MARQUEE 0x08
#endif
#if !defined(PBM_SETMARQUEE)
# define PBM_SETMARQUEE (WM_USER+10)
#endif
#if !defined(LVS_EX_DOUBLEBUFFER)
# define LVS_EX_DOUBLEBUFFER 0x10000
#endif

HWND _dw_popup = (HWND)NULL, DW_HWND_OBJECT = (HWND)NULL, _dw_tooltip = (HWND)NULL;

HINSTANCE _DWInstance = NULL;

DWORD _dwVersion = 0, _dwComctlVer = 0;
DWTID _dwtid = -1;
SECURITY_DESCRIPTOR _dwsd;

#define PACKVERSION(major,minor) MAKELONG(minor,major)

#define IS_XPPLUS (_dwComctlVer >= PACKVERSION(5,82))
#define IS_VISTAPLUS (_dwComctlVer >= PACKVERSION(6,10))
#define IS_WIN7PLUS (PACKVERSION(LOBYTE(LOWORD(_dwVersion)),HIBYTE(LOWORD(_dwVersion))) >= PACKVERSION(6,1))
#define IS_WIN8PLUS (PACKVERSION(LOBYTE(LOWORD(_dwVersion)),HIBYTE(LOWORD(_dwVersion))) >= PACKVERSION(6,2))
#define IS_WIN81PLUS (PACKVERSION(LOBYTE(LOWORD(_dwVersion)),HIBYTE(LOWORD(_dwVersion))) >= PACKVERSION(6,3))
#define IS_WIN10PLUS (PACKVERSION(LOBYTE(LOWORD(_dwVersion)),HIBYTE(LOWORD(_dwVersion))) >= PACKVERSION(10,0))

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define _DW_DATA_TYPE_STRING  0
#define _DW_DATA_TYPE_POINTER 1

/*
 * For the dw*from_data() functions, a temporary file is required to write
 * the contents of the image to so it can be loaded by the Win32 API
 * We use _tempnam() which uses TMP env variable by default. It can be passed
 * an alternate temporary directory if TMP is not set, so we get the value
 * of TEMP and store it here.
 */
static char _dw_alternate_temp_dir[MAX_PATH+1] = {0};
static char _dw_exec_dir[MAX_PATH+1] = {0};
static char _dw_app_id[_DW_APP_ID_SIZE+1]= {0};
static char _dw_app_name[_DW_APP_ID_SIZE+1]= {0};
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
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, ObjectClassName, _tcslen(ObjectClassName)+1) == 0)
       return TRUE;
   return FALSE;
}

int main(int argc, char *argv[]);

#define ICON_INDEX_LIMIT 200
HICON _dw_icon_lookup[200];
HIMAGELIST _dw_image_list_small  = 0, _dw_image_list_large = 0;

/* Special flag used for internal tracking */
#define DW_CFA_RESERVED (1 << 30)

DWORD _dw_foreground;
DWORD _dw_background;
DWORD _dw_hpen;
DWORD _dw_hbrush;
#ifdef GDIPLUS
DWORD _dw_gppen;
DWORD _dw_gpbrush;
#endif

BYTE _dw_red[] = {   0x00, 0xbb, 0x00, 0xaa, 0x00, 0xbb, 0x00, 0xaa, 0x77,
           0xff, 0x00, 0xee, 0x00, 0xff, 0x00, 0xff, 0xaa, 0x00 };
BYTE _dw_green[] = { 0x00, 0x00, 0xbb, 0xaa, 0x00, 0x00, 0xbb, 0xaa, 0x77,
           0x00, 0xff, 0xee, 0x00, 0x00, 0xee, 0xff, 0xaa, 0x00 };
BYTE _dw_blue[] = {  0x00, 0x00, 0x00, 0x00, 0xcc, 0xbb, 0xbb, 0xaa, 0x77,
            0x00, 0x00, 0x00, 0xff, 0xff, 0xee, 0xff, 0xaa, 0x00};

HBRUSH _dw_colors[18];

HFONT _DWDefaultFont = NULL;

#ifdef BUILD_HTML
LRESULT CALLBACK _dw_browserwndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef BUILD_EDGE
BOOL _dw_edge_detect(LPWSTR AppID);
BOOL _DW_EDGE_DETECTED = FALSE;
LRESULT CALLBACK _dw_edgewndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
#endif
#ifdef BUILD_TOAST
void _dw_toast_init(LPWSTR AppName, LPWSTR AppID);
void *_dw_notification_new(LPWSTR title, LPWSTR image, LPWSTR description);
int _dw_notification_send(void *notification);
BOOL _dw_toast_is_compatible(void);
#endif 
LRESULT CALLBACK _dw_colorwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
void _dw_resize_notebook_page(HWND handle, int pageid);
void _dw_handle_splitbar_resize(HWND hwnd, float percent, int type, int x, int y);
int _dw_lookup_icon(HWND handle, HICON hicon, int type);
HFONT _dw_acquire_font(HWND handle, const char *fontname);
void _dw_click_default(HWND handle);
void _dw_do_resize(Box *thisbox, int x, int y, int xborder, int yborder);

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

static int in_checkbox_handler = 0;

/* List of signals and their equivalent Win32 message */
DWSignalList DWSignalTranslate[] = {
   { WM_SIZE,         DW_SIGNAL_CONFIGURE },
   { WM_CHAR,         DW_SIGNAL_KEY_PRESS },
   { WM_LBUTTONDOWN,  DW_SIGNAL_BUTTON_PRESS },
   { WM_LBUTTONUP,    DW_SIGNAL_BUTTON_RELEASE },
   { WM_MOUSEMOVE,    DW_SIGNAL_MOTION_NOTIFY },
   { WM_CLOSE,        DW_SIGNAL_DELETE },
   { WM_PAINT,        DW_SIGNAL_EXPOSE },
   { WM_COMMAND,      DW_SIGNAL_CLICKED },
   { NM_DBLCLK,       DW_SIGNAL_ITEM_ENTER },
   { NM_RCLICK,       DW_SIGNAL_ITEM_CONTEXT },
   { LBN_SELCHANGE,   DW_SIGNAL_LIST_SELECT },
   { TVN_SELCHANGED,  DW_SIGNAL_ITEM_SELECT },
   { WM_SETFOCUS,     DW_SIGNAL_SET_FOCUS },
   { WM_VSCROLL,      DW_SIGNAL_VALUE_CHANGED },
   { TCN_SELCHANGE,   DW_SIGNAL_SWITCH_PAGE },
   { LVN_COLUMNCLICK, DW_SIGNAL_COLUMN_CLICK },
   { TVN_ITEMEXPANDED,DW_SIGNAL_TREE_EXPAND },
   { WM_USER+100,     DW_SIGNAL_HTML_RESULT },
   { WM_USER+101,     DW_SIGNAL_HTML_CHANGED },
   { WM_USER+103,     DW_SIGNAL_HTML_MESSAGE },
   { 0,               "" }
};

#ifdef BUILD_DLL
/* Old function to store the instance handle, kept for compatibilty. */
void Win32_Set_Instance(HINSTANCE hInstance)
{
   _DWInstance = hInstance;
}
#endif

/*
 * Internal function to convert WinMain arguments into main() style.
 * Also saves the handle to the instance passed from WinMain().
 */
char ** API _dw_convertargs(int *count, char *start, HINSTANCE hInstance)
{
   char *tmp, *argstart, **argv;
   int loc = 0, inquotes = 0;

   _DWInstance = hInstance;
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

   argv = (char **)calloc(sizeof(char *), ((*count)+1));
   argv[0] = calloc(261, 1);
   GetModuleFileNameA(_DWInstance, argv[0], 260);

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
            argv[loc] = _strdup(argstart);

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
         argv[loc] = _strdup(argstart);
   }
   argv[loc+1] = NULL;
   return argv;
}

#ifdef UNICODE
/* Macro and internal function to convert UTF8 to Unicode wide characters */
#define UTF8toWide(a) _dw_UTF8toWide(a, a ? _alloca(MultiByteToWideChar(CP_UTF8, 0, a, -1, NULL, 0) * sizeof(WCHAR)) : NULL)
LPWSTR _dw_UTF8toWide(const char *utf8string, void *outbuf)
{
   LPWSTR retbuf = outbuf;

   if(retbuf)
      MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, retbuf, MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0) * sizeof(WCHAR));
   return retbuf;
}
#define WideToUTF8(a) _dw_WideToUTF8(a, a ? _alloca(WideCharToMultiByte(CP_UTF8, 0, a, -1, NULL, 0, NULL, NULL)) : NULL)
char *_dw_WideToUTF8(LPCWSTR widestring, void *outbuf)
{
   char *retbuf = outbuf;

   if(retbuf)
      WideCharToMultiByte(CP_UTF8, 0, widestring, -1, retbuf, WideCharToMultiByte(CP_UTF8, 0, widestring, -1, NULL, 0, NULL, NULL), NULL, NULL);
   return retbuf;
}
#else
#define UTF8toWide(a) a
#define WideToUTF8(a) a
#endif

DWORD _dw_get_dll_version(LPCTSTR lpszDllName)
{

   HINSTANCE hinstDll;
   DWORD dwVersion = 0;

   hinstDll = LoadLibrary(lpszDllName);

   if(hinstDll)
   {
      DLLGETVERSIONPROC pDllGetVersion;

      pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

      /* Because some DLLs might not implement this function, you
       * must test for it explicitly. Depending on the particular
       * DLL, the lack of a DllGetVersion function can be a useful
       * indicator of the version.
       */
      if(pDllGetVersion)
      {
         DLLVERSIONINFO dvi;
         HRESULT hr;

         ZeroMemory(&dvi, sizeof(dvi));
         dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

         if(SUCCEEDED(hr))
         {
            dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
         }
      }

      FreeLibrary(hinstDll);
   }
    return dwVersion;
}

#ifdef GDIPLUS
/*
 * List those icons that have transparency first
 * GDI+ List of supported formats: BMP, ICON, GIF, JPEG, Exif, PNG, TIFF, WMF, and EMF.
 * Not sure if we should include all these or not... maybe we should add TIFF and GIF?
 */
#define NUM_EXTS 8
char *image_exts[NUM_EXTS+1] =
{
   "",
   ".png",
   ".ico",
   ".gif",
   ".jpg",
   ".jpeg",
   ".tiff",
   ".bmp",
   NULL
};

/* Section for loading files of types besides BMP and ICO and return HBITMAP or HICON */
void *_dw_load_gpbitmap(const char *filename)
{
   int i, wclen = (int)(strlen(filename) + 6) * sizeof(wchar_t);
   char *file = _alloca(strlen(filename) + 6);
   wchar_t *wfile = _alloca(wclen);
   void *image;

   /* Try various extentions */
   for ( i = 0; i < NUM_EXTS; i++ )
   {
      strcpy( file, filename );
      strcat( file, image_exts[i] );
      if ( _access( file, 04 ) == 0 )
      {
         /* Convert to wide format */
         MultiByteToWideChar(CP_ACP, 0, file, (int)strlen(file)+1, wfile, wclen);
         if(!GdipCreateBitmapFromFile(wfile, &image))
             return image;
      }
   }
   return NULL;
}

/* Try to load the appropriate image and return the HBITMAP handle */
HBITMAP _dw_load_bitmap(const char *filename, unsigned long *depth)
{
    void *bitmap = _dw_load_gpbitmap(filename);
    if(bitmap)
    {
        HBITMAP hbm = NULL;

        if(!GdipCreateHBITMAPFromBitmap(bitmap, &hbm, 0))
        {
            if(depth)
            {
                INT pf;

                /* Default to 0 */
                *depth = 0;

                /* Query the pixel format so we can determine the depth */
                if(!GdipGetImagePixelFormat(bitmap, &pf))
                {
                    switch(pf)
                    {
                    case PixelFormat1bppIndexed:
                        *depth = 1;
                        break;
                    case PixelFormat4bppIndexed:
                        *depth = 4;
                        break;
                    case PixelFormat8bppIndexed:
                        *depth = 8;
                        break;
                    case PixelFormat16bppGrayScale:
                    case PixelFormat16bppRGB555:
                    case PixelFormat16bppRGB565:
                    case PixelFormat16bppARGB1555:
                        *depth = 16;
                        break;
                    case PixelFormat24bppRGB:
                        *depth = 24;
                        break;
                    case PixelFormat32bppRGB:
                    case PixelFormat32bppARGB:
                    case PixelFormat32bppPARGB:
                    case PixelFormat32bppCMYK:
                        *depth = 32;
                        break;
                    case PixelFormat48bppRGB:
                        *depth = 48;
                        break;
                    case PixelFormat64bppARGB:
                    case PixelFormat64bppPARGB:
                        *depth = 64;
                        break;
                    }
                }
            }
        }
        GdipDisposeImage(bitmap);
        return hbm;
    }
    return NULL;
}

/* Try to load the appropriate image and return the HICON handle */
HICON _dw_load_icon(const char *filename)
{
    void *bitmap = _dw_load_gpbitmap(filename);
    if(bitmap)
    {
        HICON hicon = NULL;

        GdipCreateHICONFromBitmap(bitmap, &hicon);
        GdipDisposeImage(bitmap);
        return hicon;
    }
    return NULL;
}
#endif

#ifdef AEROGLASS
/* Set _DW_DARK_MODE_ALLOWED to FALSE to disable dark mode.
 * Set _DW_DARK_MODE_ALLOWED to TRUE for basic dark mode.
 * Set _DW_DARK_MODE_ALLOWED to 2 for full dark mode.
 * Set _DW_DARK_MODE_ALLOWED to 3 for forced full dark mode.
 */
int _DW_DARK_MODE_ALLOWED = TRUE;
int _DW_DARK_MODE_SUPPORTED = FALSE;
int _DW_DARK_MODE_ENABLED = FALSE;

typedef enum IMMERSIVE_HC_CACHE_MODE
{
   IHCM_USE_CACHED_VALUE,
   IHCM_REFRESH
} IMMERSIVE_HC_CACHE_MODE;

typedef enum _PreferredAppMode
{
   _Default,
   _AllowDark,
   _ForceDark,
   _ForceLight,
   _Max
} _PreferredAppMode;

typedef enum _WINDOWCOMPOSITIONATTRIB
{
   WCA_UNDEFINED = 0,
   WCA_NCRENDERING_ENABLED = 1,
   WCA_NCRENDERING_POLICY = 2,
   WCA_TRANSITIONS_FORCEDISABLED = 3,
   WCA_ALLOW_NCPAINT = 4,
   WCA_CAPTION_BUTTON_BOUNDS = 5,
   WCA_NONCLIENT_RTL_LAYOUT = 6,
   WCA_FORCE_ICONIC_REPRESENTATION = 7,
   WCA_EXTENDED_FRAME_BOUNDS = 8,
   WCA_HAS_ICONIC_BITMAP = 9,
   WCA_THEME_ATTRIBUTES = 10,
   WCA_NCRENDERING_EXILED = 11,
   WCA_NCADORNMENTINFO = 12,
   WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
   WCA_VIDEO_OVERLAY_ACTIVE = 14,
   WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
   WCA_DISALLOW_PEEK = 16,
   WCA_CLOAK = 17,
   WCA_CLOAKED = 18,
   WCA_ACCENT_POLICY = 19,
   WCA_FREEZE_REPRESENTATION = 20,
   WCA_EVER_UNCLOAKED = 21,
   WCA_VISUAL_OWNER = 22,
   WCA_HOLOGRAPHIC = 23,
   WCA_EXCLUDED_FROM_DDA = 24,
   WCA_PASSIVEUPDATEMODE = 25,
   WCA_USEDARKMODECOLORS = 26,
   WCA_LAST = 27
} _WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA
{
   _WINDOWCOMPOSITIONATTRIB Attrib;
   PVOID pvData;
   SIZE_T cbData;
} _WINDOWCOMPOSITIONATTRIBDATA;

HTHEME (WINAPI * _OpenNcThemeData)(HWND, LPCWSTR) = NULL;
VOID (WINAPI * _RefreshImmersiveColorPolicyState)(VOID) = NULL;
BOOL (WINAPI * _GetIsImmersiveColorUsingHighContrast)(IMMERSIVE_HC_CACHE_MODE) = NULL;
BOOL (WINAPI * _ShouldAppsUseDarkMode)(VOID) = NULL;
BOOL (WINAPI * _AllowDarkModeForWindow)(HWND, BOOL) = NULL;
BOOL (WINAPI * _AllowDarkModeForApp)(BOOL) = NULL;
_PreferredAppMode (WINAPI * _SetPreferredAppMode)(_PreferredAppMode) = NULL;
BOOL (WINAPI * _IsDarkModeAllowedForWindow)(HWND) = NULL;
BOOL (WINAPI * _ShouldSystemUseDarkMode)(VOID) = NULL;
BOOL (WINAPI* _SetWindowCompositionAttribute)(HWND, _WINDOWCOMPOSITIONATTRIBDATA *) = NULL;

BOOL _dw_is_high_contrast(VOID)
{
   HIGHCONTRASTW highContrast = { sizeof(highContrast) };
   if(SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
      return highContrast.dwFlags & HCF_HIGHCONTRASTON;
   return FALSE;
}

/* Our own ShouldAppsUseDarkMode() that handles the forced option */
BOOL _dw_should_apps_use_dark_mode(void)
{
    if(_DW_DARK_MODE_ALLOWED == DW_DARK_MODE_FORCED)
        return TRUE;
    return (_ShouldAppsUseDarkMode() && !_dw_is_high_contrast());
}

void _dw_init_dark_mode(void)
{
   if(_DW_DARK_MODE_ALLOWED && _dwVersion && huxtheme)
   {
      /* Dark mode is introduced in Windows 10 (1809) build 17763 */
      if(LOBYTE(LOWORD(_dwVersion)) >= 10 && HIWORD(_dwVersion) >= 17763)
      {
         _OpenNcThemeData = (HTHEME (WINAPI *)(HWND, LPCWSTR))GetProcAddress(huxtheme, MAKEINTRESOURCEA(49));
         _RefreshImmersiveColorPolicyState = (VOID (WINAPI *)(VOID))GetProcAddress(huxtheme, MAKEINTRESOURCEA(104));
         _GetIsImmersiveColorUsingHighContrast = (BOOL (WINAPI *)(IMMERSIVE_HC_CACHE_MODE))GetProcAddress(huxtheme, MAKEINTRESOURCEA(106));
         _ShouldAppsUseDarkMode = (BOOL (WINAPI *)(VOID))GetProcAddress(huxtheme, MAKEINTRESOURCEA(132));
         _AllowDarkModeForWindow = (BOOL (WINAPI *)(HWND, BOOL))GetProcAddress(huxtheme, MAKEINTRESOURCEA(133));
         if(HIWORD(_dwVersion) < 18362)
            _AllowDarkModeForApp = (BOOL (WINAPI *)(BOOL))GetProcAddress(huxtheme, MAKEINTRESOURCEA(135));
         else
            _SetPreferredAppMode = (_PreferredAppMode (WINAPI *)(_PreferredAppMode))GetProcAddress(huxtheme, MAKEINTRESOURCEA(135));
         _IsDarkModeAllowedForWindow = (BOOL (WINAPI *)(HWND))GetProcAddress(huxtheme, MAKEINTRESOURCEA(137));
         _ShouldSystemUseDarkMode = (BOOL (WINAPI *)(VOID))GetProcAddress(huxtheme, MAKEINTRESOURCEA(138));
         _SetWindowCompositionAttribute = (BOOL (WINAPI*)(HWND, _WINDOWCOMPOSITIONATTRIBDATA*))GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetWindowCompositionAttribute");
      }
      /* Make sure we were able to load all the Dark Mode functions */
      if(_OpenNcThemeData && _RefreshImmersiveColorPolicyState && _ShouldAppsUseDarkMode && _AllowDarkModeForWindow &&
         (_AllowDarkModeForApp || _SetPreferredAppMode) && _IsDarkModeAllowedForWindow &&
         (_DwmSetWindowAttribute || _SetWindowCompositionAttribute))
      {
         _DW_DARK_MODE_SUPPORTED = TRUE;
         if(_AllowDarkModeForApp)
            _AllowDarkModeForApp(TRUE);
         else
            _SetPreferredAppMode(_AllowDark);
         _RefreshImmersiveColorPolicyState();
         _DW_DARK_MODE_ENABLED = _dw_should_apps_use_dark_mode();
      }
   }
}

/* Return a margins struct based on the calculated window rect */
MARGINS _dw_rect_to_margins(RECT rect)
{
   /* Left, Right, Top, Bottom */
   MARGINS mar = { 1, 1, rect.top, 1 }, none = {0};
   
   if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
      return mar;
   return none;
}

BOOL _dw_can_theme_window(HWND window)
{
   TCHAR tmpbuf[100] = {0};
   LONG_PTR style = GetWindowLongPtr(window, GWL_STYLE);

   GetClassName(window, tmpbuf, 99);

   /* Some controls don't display properly with visual styles enabled */
   if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1) == 0 &&
      (style & (BS_AUTOCHECKBOX | BS_CHECKBOX | BS_RADIOBUTTON)))
      return FALSE;
#ifdef TOOLBAR
   else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
   {
     /* If we aren't in full dark mode */
      if(_DW_DARK_MODE_ALLOWED < DW_DARK_MODE_FULL)
      {
         /* Enable or disable visual themes */
         if(_SetWindowTheme)
            _SetWindowTheme(window, (style & TBSTYLE_FLAT) ? L"" : NULL, (style & TBSTYLE_FLAT) ? L"" : NULL);
      }
      return FALSE;
   }
#endif
   return TRUE;
}

BOOL _dw_allow_dark_mode_for_window(HWND window, BOOL allow)
{
   if(_DW_DARK_MODE_SUPPORTED)
   {
      if(_dw_can_theme_window(window))
      {
         if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC)
         {
            if(_DW_DARK_MODE_ENABLED)
               _SetWindowTheme(window, L"DarkMode_Explorer", NULL);
            else
               _SetWindowTheme(window, L"Explorer", NULL);
         }
      }
      return _AllowDarkModeForWindow(window, allow);
   }
   return FALSE;
}

BOOL _dw_is_color_scheme_change_message(LPARAM lParam)
{
   BOOL is = FALSE;
   if(lParam && _wcsicmp((LPCWCH)lParam, L"ImmersiveColorSet") == 0)
   {
      _RefreshImmersiveColorPolicyState();
      is = TRUE;
   }
   _GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
   return is;
}

void _dw_refresh_titlebar_theme_color(HWND window)
{
   BOOL dark = FALSE;
   if (_IsDarkModeAllowedForWindow(window) && _dw_should_apps_use_dark_mode())
      dark = TRUE;
   if(HIWORD(_dwVersion) < 18362)
      SetProp(window, TEXT("UseImmersiveDarkModeColors"), (HANDLE)DW_INT_TO_POINTER(dark));
   else if (_SetWindowCompositionAttribute)
   {
      _WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
      _SetWindowCompositionAttribute(window, &data);
   } else
      _DwmSetWindowAttribute(window, 19, &dark, sizeof(dark));
}

/* Call this on a window to apply the style */
BOOL CALLBACK _dw_set_child_window_theme(HWND window, LPARAM lParam)
{
   if(_DW_DARK_MODE_SUPPORTED)
   {
      _dw_allow_dark_mode_for_window(window, _DW_DARK_MODE_ENABLED);
      SendMessageW(window, WM_THEMECHANGED, 0, 0);
   }
   return TRUE;
}

#define RECTWIDTH(rc)   (rc.right - rc.left)
#define RECTHEIGHT(rc)  (rc.bottom - rc.top)

/* Our function to draw the titlebar in dark mode */
void _dw_draw_dark_mode_titlebar(HWND hWnd, HDC hdc)
{
   if(_OpenThemeData && _CloseThemeData && _DrawThemeTextEx && _GetThemeSysFont && 
      _GetImmersiveColorFromColorSetEx && _GetImmersiveColorTypeFromName && _GetImmersiveUserColorSetPreference)
   {
      HTHEME hTheme = _OpenThemeData(NULL, L"CompositedWindow::Window");
      RECT rcClient;

      GetClientRect(hWnd, &rcClient);

      if(hTheme)
      {
         HDC hdcPaint = CreateCompatibleDC(hdc);

         if(hdcPaint)
         {
            int cx = RECTWIDTH(rcClient);
            int cy = RECTHEIGHT(rcClient);
            HBITMAP hbm;

            /* Define the BITMAPINFO structure used to draw text.
             * Note that biHeight is negative. This is done because
             * DrawThemeTextEx() needs the bitmap to be in top-to-bottom
             * order.
             */
            BITMAPINFO dib = { 0 };
            dib.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
            dib.bmiHeader.biWidth           = cx;
            dib.bmiHeader.biHeight          = -cy;
            dib.bmiHeader.biPlanes          = 1;
            dib.bmiHeader.biBitCount        = 32;
            dib.bmiHeader.biCompression     = BI_RGB;

            if((hbm = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0)))
            {
               HBITMAP hbmOld = (HBITMAP)SelectObject(hdcPaint, hbm);
               LOGFONT lgFont;
               HFONT hFontOld = NULL;
               RECT rcPaint = rcClient;
               TCHAR *tempbuf;
               HICON icon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
               ColorInfo *cinfo = (ColorInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
               LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
               int len;
               DWORD nColor = _GetImmersiveColorFromColorSetEx(_GetImmersiveUserColorSetPreference(FALSE, FALSE), 
                                                               _GetImmersiveColorTypeFromName(L"ImmersiveApplicationText"), FALSE, 0);

               /* Setup the theme drawing options. */
               DTTOPTS DttOpts = {sizeof(DTTOPTS)};
               DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE | DTT_TEXTCOLOR;
               DttOpts.iGlowSize = 15;
               DttOpts.crText = RGB(0xFF & nColor, (0xFF00 & nColor) >> 8, (0xFF0000 & nColor) >> 16);

               /* Select a font. */
               if(_GetThemeSysFont(hTheme, 801 /*TMT_CAPTIONFONT*/, &lgFont) == S_OK)
               {
                  HFONT hFont = CreateFontIndirect(&lgFont);
                  hFontOld = (HFONT)SelectObject(hdcPaint, hFont);
               }

               /* Draw the title. */
               len = GetWindowTextLength(hWnd) + 1;
               if((tempbuf = _alloca(len * sizeof(TCHAR))))
                  GetWindowText(hWnd, tempbuf, len);
               rcPaint.top += 8;
               rcPaint.right -= 125;
               rcPaint.left += 8;
               if(style & WS_SYSMENU)
                  rcPaint.left += 24;
               if(cinfo && cinfo->hmenu)
                  rcPaint.left += 24;
               rcPaint.bottom = 50;
               _DrawThemeTextEx(hTheme, 
                               hdcPaint, 
                               0, 0, 
                               tempbuf ? tempbuf : TEXT("<no title>"), 
                               -1, 
                               DT_LEFT | DT_WORD_ELLIPSIS, 
                               &rcPaint, 
                               &DttOpts);

               if(style & WS_SYSMENU)
               {
                  /* Draw the application icon sysmenu */
                  DrawIconEx(hdcPaint, 8, 8, icon ? icon : LoadIcon(NULL, MAKEINTRESOURCE(32512)), 16, 16, 0, NULL, DI_NORMAL);
               }
               if(cinfo && cinfo->hmenu)
               {
                  /* Draw an icon to use as a menu click location */
                  DrawIconEx(hdcPaint, 8 + ((style & WS_SYSMENU) ? 24 : 0), 8, LoadIcon(NULL, MAKEINTRESOURCE(32516)), 16, 16, 0, NULL, DI_NORMAL);
               }

               /* Blit text to the frame. */
               BitBlt(hdc, 0, 0, cx, cy, hdcPaint, 0, 0, SRCCOPY);

               SelectObject(hdcPaint, hbmOld);
               if(hFontOld)
                  SelectObject(hdcPaint, hFontOld);
               DeleteObject(hbm);
            }
            DeleteDC(hdcPaint);
         }
         _CloseThemeData(hTheme);
      }
   }
}
#endif

/* Special wrappers for GetSysColor*() since they currently don't support
 * dark mode, we will have to return modified colors when dark mode is enabled.
 */
DWORD _dw_get_syscolor(int nIndex)
{
   DWORD retval = GetSysColor(nIndex);
#ifdef AEROGLASS
   if(_DW_DARK_MODE_ALLOWED > 1 && _DW_DARK_MODE_ENABLED)
   {
      const COLORREF darkBkColor = 0x383838;
      const COLORREF darkTextColor = 0xFFFFFF;

      switch(nIndex)
      {
         case COLOR_3DFACE:
         case COLOR_WINDOW:
            retval = darkBkColor;
         break;
         case COLOR_WINDOWTEXT:
            retval = darkTextColor;
         break;
      }
   }
#endif
   return retval;
}

HBRUSH _dw_get_syscolor_brush(int nIndex)
{
   HBRUSH retval = GetSysColorBrush(nIndex);
#ifdef AEROGLASS
   static HBRUSH darkBkColorBrush = 0;

   if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_ENABLED)
   {
      if(!darkBkColorBrush)
         darkBkColorBrush = CreateSolidBrush(0x383838);

      switch(nIndex)
      {
         case COLOR_3DFACE:
         case COLOR_WINDOW:
            retval = darkBkColorBrush;
         break;
         case COLOR_WINDOWTEXT:
            retval = _dw_colors[DW_CLR_WHITE];
         break;
      }
   }
#endif
   return retval;
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
   {
      Root = new;
   }
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
   int z = 0;

   while(DWSignalTranslate[z].message)
   {
      if(_stricmp(signame, DWSignalTranslate[z].name) == 0)
         return DWSignalTranslate[z].message;
      z++;
   }
   return 0L;
}

/* Internal function to get the ColorInfo struct from a widget/window */
ColorInfo *_dw_window_get_cinfo(HWND handle)
{
   return (ColorInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);
}

/* Internal function to add ColorInfo and subclass a widget/window */
ColorInfo *_dw_window_new_cinfo(HWND handle, BOOL subclass)
{
   ColorInfo *cinfo = calloc(1, sizeof(ColorInfo));

   if(cinfo)
   {
      cinfo->back = cinfo->fore = -1;
      if(subclass)
         cinfo->pOldProc = SubclassWindow(handle, _dw_colorwndproc);
      SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)cinfo);
   }
   return cinfo;
}

/* This function removes any handlers on windows and frees
 * the user memory and resources allocated to it.
 */
BOOL CALLBACK _dw_free_window_memory(HWND handle, LPARAM lParam)
{
   ColorInfo *thiscinfo = _dw_window_get_cinfo(handle);
   HFONT oldfont = (HFONT)SendMessage(handle, WM_GETFONT, 0, 0);
   HICON oldicon = (HICON)SendMessage(handle, WM_GETICON, 0, 0);
   TCHAR tmpbuf[100] = {0};

   TOOLINFO ti = { 0 };

   ti.cbSize = sizeof(TOOLINFO);
   ti.hwnd = handle;
   ti.hinst = _DWInstance;

   SendMessage(_dw_tooltip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

   GetClassName(handle, tmpbuf, 99);

   /* Don't try to free memory from an embedded IE or Edge/Chromium window */
   if(_tcsncmp(tmpbuf, TEXT("Internet Explorer_Server"), 25) == 0 ||
	   _tcsncmp(tmpbuf, TEXT("Chrome_WidgetWin_"), 17) == 0)
      return TRUE;

   /* Delete font, icon and bitmap GDI objects in use */
   if(oldfont)
      DeleteObject(oldfont);
   if(oldicon)
      DeleteObject(oldicon);

   if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1)==0)
   {
      HBITMAP oldbitmap = (HBITMAP)SendMessage(handle, STM_GETIMAGE, IMAGE_BITMAP, 0);

      if(oldbitmap)
         DeleteObject(oldbitmap);
   }
   if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1)==0)
   {
      HBITMAP oldbitmap = (HBITMAP)SendMessage(handle, BM_GETIMAGE, IMAGE_BITMAP, 0);
      HICON oldicon = (HICON)SendMessage(handle, BM_GETIMAGE, IMAGE_ICON, 0);

      if(oldbitmap)
         DeleteObject(oldbitmap);
      if(oldicon)
         DestroyIcon(oldicon);
   }
#ifdef TOOLBAR
   /* Bitmap Buttons */
   else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
   {
      HIMAGELIST imlist = (HIMAGELIST)SendMessage(handle, TB_GETIMAGELIST, 0, 0);
      HIMAGELIST dimlist = (HIMAGELIST)SendMessage(handle, TB_GETDISABLEDIMAGELIST, 0, 0);

      SendMessage(handle, TB_SETIMAGELIST, 0, 0);
      SendMessage(handle, TB_SETDISABLEDIMAGELIST, 0, 0);

      if(imlist)
         ImageList_Destroy(imlist);
      if(dimlist)
         ImageList_Destroy(dimlist);
   }
#endif
   else if(_tcsnicmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1)==0)
   {
      Box *box = (Box *)thiscinfo;

      if(box && box->count && box->items)
         free(box->items);
   }
   else if(_tcsnicmp(tmpbuf, SplitbarClassName, _tcslen(SplitbarClassName)+1)==0)
   {
      void *data = dw_window_get_data(handle, "_dw_percent");

      if(data)
         free(data);
   }
   else if(_tcsnicmp(tmpbuf, WC_TREEVIEW, _tcslen(WC_TREEVIEW)+1)==0)
   {
      dw_tree_clear(handle);
   }
   else if(_tcsnicmp(tmpbuf, WC_TABCONTROL, _tcslen(WC_TABCONTROL)+1)==0) /* Notebook */
   {
      NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");

      if(array)
      {
         int z;

         for(z=0;z<256;z++)
         {
            if(array[z])
            {
               _dw_free_window_memory(array[z]->hwnd, 0);
               EnumChildWindows(array[z]->hwnd, _dw_free_window_memory, 0);
               DestroyWindow(array[z]->hwnd);
               free(array[z]);
            }
         }
         free(array);
      }
   }
   else if(_tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1)==0)
   {
      /* for spinbuttons, we need to get the spinbutton's "buddy", the text window associated and destroy it */
      ColorInfo *cinfo = _dw_window_get_cinfo(handle);

      if(cinfo && cinfo->buddy)
         DestroyWindow(cinfo->buddy);
   }
   /* Some Edge Windows have an empty class.. abort. */
   else if(_tcslen(tmpbuf) == 0)
      return TRUE;

   dw_signal_disconnect_by_window(handle);

   if(thiscinfo)
   {
      if(thiscinfo->pOldProc)
         SetWindowLongPtr(handle, GWLP_WNDPROC, (LPARAM)(WNDPROC)thiscinfo->pOldProc);

      /* Delete the brush so as not to leak GDI objects */
      if(thiscinfo->hbrush)
         DeleteObject(thiscinfo->hbrush);

      /* Free user data linked list memory */
      if(thiscinfo->root)
         dw_window_set_data(handle, NULL, NULL);

      SetWindowLongPtr(handle, GWLP_USERDATA, 0);
      free(thiscinfo);
   }
   return TRUE;
}

void _dw_free_menu_data(HMENU menu)
{
    int i, count = GetMenuItemCount(menu);

    for(i=0;i<count;i++)
    {
        MENUITEMINFO mii;

        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_SUBMENU | MIIM_ID;

        if ( GetMenuItemInfo( menu, i, TRUE, &mii ) )
        {
           /* Free the data associated with the ID */
           if(mii.wID >= 30000)
           {
              char buffer[31] = {0};

              _snprintf(buffer, 30, "_dw_id%u", mii.wID);
              dw_window_set_data( DW_HWND_OBJECT, buffer, NULL );
              _snprintf(buffer, 30, "_dw_checkable%u", mii.wID);
              dw_window_set_data( DW_HWND_OBJECT, buffer, NULL );
              _snprintf(buffer, 30, "_dw_ischecked%u", mii.wID);
              dw_window_set_data( DW_HWND_OBJECT, buffer, NULL );
              _snprintf(buffer, 30, "_dw_isdisabled%u", mii.wID);
              dw_window_set_data( DW_HWND_OBJECT, buffer, NULL );
           }

           /* Check any submenus */
           if( mii.hSubMenu )
              _dw_free_menu_data(mii.hSubMenu);
        }
    }
    dw_signal_disconnect_by_name((HWND)menu, DW_SIGNAL_CLICKED);
}

/* Convert to our internal color scheme */
ULONG _dw_internal_color(ULONG color)
{
   if(color == DW_CLR_DEFAULT)
      return DW_RGB_TRANSPARENT;
   if(color < 18)
      return DW_RGB(_dw_red[color], _dw_green[color], _dw_blue[color]);
   return color;
}

/* This function returns 1 if the window (widget) handle
 * passed to it is a valid window that can gain input focus.
 */
int _dw_validate_focus(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   if(!handle)
      return 0;

   if(!IsWindowEnabled(handle))
      return 0;

   GetClassName(handle, tmpbuf, 99);

   /* These are the window classes which can
    * obtain input focus.
    */
   if(_tcsnicmp(tmpbuf, EDITCLASSNAME, _tcslen(EDITCLASSNAME)+1)==0 ||          /* Entryfield */
      _tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1)==0 ||      /* Button */
#ifdef TOOLBAR
      _tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0 ||  /* Toolbar */
#endif
      _tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0 ||  /* Combobox */
      _tcsnicmp(tmpbuf, LISTBOXCLASSNAME, _tcslen(LISTBOXCLASSNAME)+1)==0 ||    /* List box */
      _tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1)==0 ||            /* Spinbutton */
      _tcsnicmp(tmpbuf, TRACKBAR_CLASS, _tcslen(TRACKBAR_CLASS)+1)==0 ||        /* Slider */
      _tcsnicmp(tmpbuf, WC_LISTVIEW, _tcslen(WC_LISTVIEW)+1)== 0 ||             /* Container */
      _tcsnicmp(tmpbuf, WC_TREEVIEW, _tcslen(WC_TREEVIEW)+1)== 0)               /* Tree */
      return 1;
   /* Special case for the notebook, can get focus and contains other items */
   if(_tcsnicmp(tmpbuf, WC_TABCONTROL, _tcslen(WC_TABCONTROL))==0)              /* Notebook */
      return 2;
   return 0;
}

HWND _dw_normalize_handle(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);
   if(_tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS))==0) /* Spinner */
   {
      ColorInfo *cinfo = _dw_window_get_cinfo(handle);

      if(cinfo && cinfo->buddy)
         return cinfo->buddy;
   }
   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME))==0) /* Combobox */
   {
      ColorInfo *cinfo = _dw_window_get_cinfo(handle);

      if(cinfo && cinfo->buddy)
         return cinfo->buddy;
   }
   return handle;
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
   NotebookPage **array = (NotebookPage **)dw_window_get_data(hwnd, "_dw_array");
   int pageid = TabCtrl_GetCurSel(hwnd);

   if(pageid > -1 && array && array[pageid])
   {
      Box *notebox;

      if(array[pageid]->hwnd)
      {
         notebox = (Box *)GetWindowLongPtr(array[pageid]->hwnd, GWLP_USERDATA);

         if(notebox && _dw_focus_check_box(notebox, handle, start == 3 ? 3 : 0, direction, defaultitem))
            return 1;
      }
   }
   return 0;
}

/* Handle box focus traversal in either direction */
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
         SetFocus(lasthwnd);
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
         Box *thisbox = (Box *)GetWindowLongPtr(box->items[z].hwnd, GWLP_USERDATA);

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
               SetFocus(firsthwnd);
            else if(lasthwnd == handle && !firsthwnd)
               finish_searching = 1;
            else
               SetFocus(lasthwnd);

            /* If we aren't looking for the last handle,
             * return immediately.
             */
            if(!finish_searching)
               return 1;
         }
         if(type > 0)
         {
            /* Start is 3 when we are looking for the
             * first valid item in the layout.
             */
            if(start == 3)
            {
               if(!defaultitem || (defaultitem && box->items[z].hwnd == defaultitem))
               {
                  SetFocus(_dw_normalize_handle(box->items[z].hwnd));
                  return 1;
               }
            }

            lasthwnd = _dw_normalize_handle(box->items[z].hwnd);

            if(!firsthwnd)
               firsthwnd = lasthwnd;
         }
         else
         {
            /* Handle controls that contain other items */
            TCHAR tmpbuf[100] = {0};

            GetClassName(box->items[z].hwnd, tmpbuf, 99);

            if(_tcsncmp(tmpbuf, SplitbarClassName, _tcslen(SplitbarClassName)+1)==0)
            {
               /* Then try the bottom or right box */
               HWND mybox = (HWND)dw_window_get_data(box->items[z].hwnd, (direction == _DW_DIRECTION_FORWARD) ? "_dw_bottomright" : "_dw_topleft");

               if(mybox)
               {
                  Box *splitbox = (Box *)GetWindowLongPtr(mybox, GWLP_USERDATA);

                  if(splitbox && _dw_focus_check_box(splitbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                     return 1;
               }

               /* Try the top or left box */
               mybox = (HWND)dw_window_get_data(box->items[z].hwnd, (direction == _DW_DIRECTION_FORWARD) ? "_dw_topleft" : "_dw_bottomright");

               if(mybox)
               {
                  Box *splitbox = (Box *)GetWindowLongPtr(mybox, GWLP_USERDATA);

                  if(splitbox && _dw_focus_check_box(splitbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                     return 1;
               }
            }
            else if(_tcsnicmp(tmpbuf, ScrollClassName, _tcslen(ScrollClassName))==0) /* Scroll Box */
            {
                ColorInfo *cinfo = _dw_window_get_cinfo(handle);
                Box *scrollbox = (Box *)GetWindowLongPtr(cinfo->combo, GWLP_USERDATA);

                if(scrollbox && _dw_focus_check_box(scrollbox, handle, start == 3 ? 3 : 0, direction, defaultitem))
                   return 1;
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
void _dw_initial_focus(HWND handle)
{
   Box *thisbox = NULL;
   TCHAR tmpbuf[100] = {0};

   if(!handle)
      return;

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, ClassName, _tcslen(ClassName)+1)!=0)
      return;


   if(handle)
      thisbox = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);

   if(thisbox)
   {
      _dw_focus_check_box(thisbox, handle, 3, _DW_DIRECTION_FORWARD, thisbox->defaultitem);
   }
}

HWND _dw_toplevel_window(HWND handle)
{
   HWND box, lastbox = GetParent(handle);

   if(!lastbox)
      lastbox = handle;

   /* Find the toplevel window */
   while((box = GetParent(lastbox)))
   {
      /* If it hasn't been packed yet.. */
      if(box == DW_HWND_OBJECT)
         return 0;
      lastbox = box;
   }
   if(lastbox)
   {
      TCHAR tmpbuf[100] = {0};

      GetClassName(lastbox, tmpbuf, 99);

      if(_tcsncmp(tmpbuf, ClassName, _tcslen(ClassName)+1)==0)
         return lastbox;
   }
   return 0;
}

/* This function finds the current widget in the
 * layout and moves the current focus to the next item.
 */
void _dw_shift_focus(HWND handle, int direction)
{
   Box *thisbox;

   HWND box, lastbox = GetParent(handle);

   /* Find the toplevel window */
   while((box = GetParent(lastbox)))
   {
      lastbox = box;
   }

   thisbox = (Box *)GetWindowLongPtr(lastbox, GWLP_USERDATA);
   if(thisbox)
   {
      if(_dw_focus_check_box(thisbox, handle, 1, direction, 0)  == 0)
         _dw_focus_check_box(thisbox, handle, 2, direction, 0);
   }
}

/* This function calculates how much space the widgets and boxes require
 * and does expansion as necessary.
 */
static void _dw_resize_box(Box *thisbox, int *depth, int x, int y, int xborder, int yborder, int pass)
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
         Box *tmp = (Box *)GetWindowLongPtr(thisbox->items[z].hwnd, GWLP_USERDATA);

         if(tmp)
         {
            /* On the first pass calculate the box contents */
            if(pass == 1)
            {
               (*depth)++;

               /* Save the newly calculated values on the box */
               _dw_resize_box(tmp, depth, x, y, 0, 0, pass);

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
            TCHAR tmpbuf[100] = {0};

            GetClassName(handle, tmpbuf, 99);

            if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
            {
               /* Handle special case Combobox */
               MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder,
                        width, height + 400, FALSE);
            }
#ifdef TOOLBAR
            /* Bitmap Buttons */
            else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
            {
               SendMessage(handle, TB_SETBUTTONSIZE, 0, MAKELPARAM(width, height));

               MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder, width, height, FALSE);
            }
#endif
            else if(_tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1)==0)
            {
               /* Handle special case Spinbutton */
               ColorInfo *cinfo = _dw_window_get_cinfo(handle);

               MoveWindow(handle, currentx + pad + (width - 20) + xborder, currenty + pad + yborder,
                        20, height, FALSE);

               if(cinfo)
               {
                  MoveWindow(cinfo->buddy, currentx + pad + xborder, currenty + pad + yborder,
                           width - 20, height, FALSE);
               }
            }
            else if(_tcsncmp(tmpbuf, ScrollClassName, _tcslen(ScrollClassName)+1)==0)
            {
                /* Handle special case of scrollbox */
                ColorInfo *cinfo = _dw_window_get_cinfo(handle);
                int cx, cy, depth = 0;
                Box *thisbox = (Box *)GetWindowLongPtr(cinfo->combo, GWLP_USERDATA);
                SCROLLINFO hsi, vsi;
                RECT rect;

                vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
                vsi.fMask = hsi.fMask = SIF_POS;

                /* Save the current scroll positions */
                GetScrollInfo(handle, SB_HORZ, &hsi);
                GetScrollInfo(handle, SB_VERT, &vsi);

                /* Position the scrollbox */
                MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder, width, height, FALSE);

                GetClientRect(handle, &rect);
                cx = rect.right;
                cy = rect.bottom;


                /* Get the required space for the box */
                _dw_resize_box(thisbox, &depth, cx, cy, 0, 0, 1);

                if(cx < thisbox->minwidth)
                {
                    cx = thisbox->minwidth;
                }
                if(cy < thisbox->minheight)
                {
                    cy = thisbox->minheight;
                }

                /* Position the scrolled box */
                vsi.fMask = hsi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
                vsi.nMin = hsi.nMin = vsi.nMax = hsi.nMax = 0;
                if(rect.bottom < thisbox->minheight)
                {
                    vsi.nMax = thisbox->minheight - 1;
                    vsi.nPage = rect.bottom;
                    if(vsi.nPos > vsi.nMax)
                    {
                        vsi.nPos = vsi.nMax;
                    }
                }
                if(rect.right < thisbox->minwidth)
                {
                    hsi.nMax = thisbox->minwidth - 1;
                    hsi.nPage = rect.right;
                    if(hsi.nPos > hsi.nMax)
                    {
                        hsi.nPos = hsi.nMax;
                    }
                }
                MoveWindow(cinfo->combo, -hsi.nPos, -vsi.nPos, cx, cy, FALSE);
                SetScrollInfo(handle, SB_HORZ, &hsi, TRUE);
                SetScrollInfo(handle, SB_VERT, &vsi, TRUE);

                /* Layout the content of the scrollbox */
                _dw_do_resize(thisbox, cx, cy, 0, 0);
            }
            else if(_tcsncmp(tmpbuf, SplitbarClassName, _tcslen(SplitbarClassName)+1)==0)
            {
               /* Then try the bottom or right box */
               float *percent = (float *)dw_window_get_data(handle, "_dw_percent");
               int type = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_type"));

               MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder,
                        width, height, FALSE);

               if(percent && width > 0 && height > 0)
                  _dw_handle_splitbar_resize(handle, *percent, type, width, height);
            }
            else if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1)==0)
            {
               /* Handle special case Vertically Center static text */
               ColorInfo *cinfo = _dw_window_get_cinfo(handle);

               if(cinfo && cinfo->style & DW_DT_VCENTER)
               {
                  /* We are centered so calculate a new position */
                  TCHAR tmpbuf[1024] = {0}, *thisbuf = tmpbuf;
                  int textheight, diff, total = height;

                  GetWindowText(handle, thisbuf, 1023);

                  /* Figure out how big the text is */
                  dw_font_text_extents_get(handle, 0, WideToUTF8(thisbuf), 0, &textheight);

                  diff = (total - textheight) / 2;

                  MoveWindow(handle, currentx + pad + xborder, currenty + pad + diff + yborder,
                           width, height - diff, FALSE);
               }
               else
               {
                  MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder,
                           width, height, FALSE);
               }
            }
            else
            {
               /* Everything else */
               if(*depth)
                  MoveWindow(handle, currentx + pad + xborder, currenty + pad + yborder, width, height, FALSE);
               else /* FIXME: This is a hack to generate WM_PAINT messages for items on the top-level */
                  SetWindowPos(handle, HWND_TOP, currentx + pad + xborder, currenty + pad + yborder, width, height, 0);

               /* After placing a box... place its components */
               if(thisbox->items[z].type == _DW_TYPE_BOX)
               {
                  Box *boxinfo = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);

                  if(boxinfo)
                  {
                     /* Move the group border into place */
                     if(boxinfo->grouphwnd)
                     {
                        MoveWindow(boxinfo->grouphwnd, 0, 0,
                                 width, height, FALSE);
                     }
                     /* Dive into the box */
                     (*depth)++;
                     _dw_resize_box(boxinfo, depth, width, height, 0, 0, pass);
                     (*depth)--;
                  }
               }
            }

            /* Notebook dialog requires additional processing */
            if(_tcsncmp(tmpbuf, WC_TABCONTROL, _tcslen(WC_TABCONTROL))==0)
            {
               RECT rect;
               NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
               int pageid = TabCtrl_GetCurSel(handle);

               if(pageid > -1 && array && array[pageid])
               {
                  GetClientRect(handle,&rect);
                  TabCtrl_AdjustRect(handle,FALSE,&rect);
                  MoveWindow(array[pageid]->hwnd, rect.left, rect.top,
                           rect.right - rect.left, rect.bottom-rect.top, FALSE);
                  dw_window_redraw(array[pageid]->hwnd);
               }
            }
            /* So does the List View... handle delayed cursoring */
            if(_tcsnicmp(tmpbuf, WC_LISTVIEW, _tcslen(WC_LISTVIEW)+1)==0 && width > 10 && height > 10)
            {
                int index = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_cursor"));

                if(index > 0)
                    ListView_EnsureVisible(handle, index, TRUE);
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

void _dw_do_resize(Box *thisbox, int x, int y, int xborder, int yborder)
{
   if(x != 0 && y != 0)
   {
      if(thisbox)
      {
         int depth = 0;

         /* Calculate space requirements */
         _dw_resize_box(thisbox, &depth, x, y, xborder, yborder, 1);

         /* Finally place all the boxes and controls */
         _dw_resize_box(thisbox, &depth, x, y, xborder, yborder, 2);
      }
   }
}

int _dw_handle_scroller(HWND handle, int bar, int pos, int which)
{
   SCROLLINFO si;

   ZeroMemory( &si, sizeof(si) );
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_ALL;

   SendMessage(handle, SBM_GETSCROLLINFO, 0, (LPARAM)&si);

   switch(which)
   {
   case SB_THUMBTRACK:
      return pos;
   /*case SB_PAGEUP:*/
   case SB_PAGELEFT:
      pos = si.nPos - si.nPage;
      if(pos < si.nMin)
         pos = si.nMin;
      return pos;
   /*case SB_PAGEDOWN:*/
   case SB_PAGERIGHT:
      pos = si.nPos + si.nPage;
      if(pos > (int)(si.nMax - si.nPage) + 1)
         pos = (si.nMax - si.nPage) + 1;
      return pos;
   /*case SB_LINEUP:*/
   case SB_LINELEFT:
      pos = si.nPos - 1;
      if(pos < si.nMin)
         pos = si.nMin;
      return pos;
   /*case SB_LINEDOWN:*/
   case SB_LINERIGHT:
      pos = si.nPos + 1;
      if(pos > (int)(si.nMax - si.nPage) + 1)
         pos = (si.nMax - si.nPage) + 1;
      return pos;
   }
   return -1;
}

HMENU _dw_get_owner(HMENU menu)
{
   MENUINFO mi;

   mi.cbSize = sizeof(MENUINFO);
   mi.fMask = MIM_MENUDATA;

   if ( GetMenuInfo( menu, &mi ) )
      return (HMENU)mi.dwMenuData;
   return (HMENU)0;
}

/* Find the desktop window handle */
HMENU _dw_menu_owner(HMENU handle)
{
   HMENU menuowner = 0, lastowner = _dw_get_owner(handle);

   /* Find the toplevel menu */
   while((menuowner = _dw_get_owner(lastowner)) != 0)
   {
      if(menuowner == (HMENU)1)
         return lastowner;
      lastowner = menuowner;
   }
   return (HMENU)0;
}

/*
 * Determine if this is a checkable menu. If it is get the current state
 * and toggle it. Windows doesn't do this automatically :-(
 */
static void _dw_toggle_checkable_menu_item( HWND window, int id )
{
   char buffer[40];
   int checkable;
   sprintf( buffer, "_dw_checkable%d", id );
   checkable = DW_POINTER_TO_INT(dw_window_get_data(DW_HWND_OBJECT, buffer));
   if ( checkable )
   {
      int is_checked;
      sprintf( buffer, "_dw_ischecked%d", id );
      is_checked = DW_POINTER_TO_INT(dw_window_get_data(DW_HWND_OBJECT, buffer));
      is_checked = (is_checked) ? DW_MIS_UNCHECKED : DW_MIS_CHECKED;
      dw_menu_item_set_state( window, id, is_checked );
   }
}

#ifdef AEROGLASS
void _dw_show_margins(HWND handle, MARGINS mar, int line)
{
#ifdef DEBUG
   char *title = dw_window_get_text(handle);
   dw_debug("_DwmExtendFrameIntoClientArea(\"%s\",%d,%d,%d,%d) line %d\n", title,
            mar.cxLeftWidth, mar.cxRightWidth,
            mar.cyTopHeight, mar.cyBottomHeight, line);
   dw_free(title);
#endif
}
#endif

/* The main window procedure for Dynamic Windows, all the resizing code is done here. */
LRESULT CALLBACK _dw_wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   int result = -1, taskbar = FALSE;
   DWSignalHandler *tmp = Root;
   void (DWSIGNAL *windowfunc)(PVOID);
   ULONG origmsg = msg;

#ifdef DARK_MODE_TITLEBAR_MENU
   /* Expand the client area into the titlebar so we can draw our alternate dark mode button
    * which when clicked will display the window's menubar menu. Since the menubar cannot be
    * made dark, hide it and add the button to the titlebar instead.
    */
   if(msg == WM_NCCALCSIZE && mp2 && _DW_DARK_MODE_SUPPORTED && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC)
   {
      ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);

      if(cinfo)
      {
         NCCALCSIZE_PARAMS* sz = (NCCALCSIZE_PARAMS*)mp2;

         sz->rgrc[0].left += cinfo->rect.left;
         sz->rgrc[0].right -= cinfo->rect.right;
         sz->rgrc[0].bottom -= cinfo->rect.bottom;
         return 0;
      }
   }
   else if(msg == WM_NCPAINT && _DW_DARK_MODE_SUPPORTED && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC)
   {
      /* Handle close/minimize/maximize/help button */
      LRESULT lResult;

      if(_DwmDefWindowProc && _DwmDefWindowProc(hWnd, msg, mp1, mp2, &lResult))
         return lResult;
   }
   else if(msg == WM_NCHITTEST && _DW_DARK_MODE_SUPPORTED && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC)
   {
      /* Handle close/minimize/maximize/help button */
      LRESULT lResult;

      if(_DwmDefWindowProc && _DwmDefWindowProc(hWnd, msg, mp1, mp2, &lResult))
         return lResult;

      /* Do default processing, except change the result for caption area */
      lResult = DefWindowProc(hWnd, msg, mp1, mp2);
      if(lResult == HTCLIENT)
      {
         ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);

         if(cinfo)
         {
            POINT pt = { LOWORD(mp2), HIWORD(mp2) };

            ScreenToClient(hWnd, &pt);
            /* The top border should be the same as the bottom. */
            if (pt.y < cinfo->rect.bottom) return HTTOP;
            /* The caption is any part of the top besides the border */
            if (pt.y < cinfo->rect.top) 
            {
               ColorInfo *cinfo = (ColorInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
               LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
               int pos = 32;

               if(style & WS_SYSMENU)
               {
                  if(pt.x < (pos + cinfo->rect.left))
                     return HTSYSMENU;
                  pos += 24;
               }
               if(cinfo && cinfo->hmenu)
               {
                  if(pt.x < (pos + cinfo->rect.left))
                  {
                     if(cinfo->hmenu)
                     {
                        TrackPopupMenu(cinfo->hmenu, 0, LOWORD(mp2), HIWORD(mp2), 0, hWnd, NULL);
                        return HTNOWHERE;
                     }
                  }
               }
               return HTCAPTION;
            }
         }
      }
      return lResult;
   }
   else if(msg == WM_ACTIVATE && _DW_DARK_MODE_SUPPORTED && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC)
   {
      RECT rect;
      GetWindowRect(hWnd, &rect);
      PostMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rect.right-rect.left, rect.bottom-rect.top));
   }
   else if(msg == WM_PAINT && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED && GetParent(hWnd) == HWND_DESKTOP)
   {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      _dw_draw_dark_mode_titlebar(hWnd, hdc);
      EndPaint(hWnd, &ps);
   }
#endif

   /* Deal with translating some messages */
   if (msg == WM_USER+2)
   {
      taskbar = TRUE;
      origmsg = msg = (UINT)mp2; /* no else here */
   }
   if (msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN)
      msg = WM_LBUTTONDOWN;
   else if (msg == WM_RBUTTONUP || msg == WM_MBUTTONUP)
      msg = WM_LBUTTONUP;
   else if (msg == WM_HSCROLL)
      msg = WM_VSCROLL;
   else if (msg == WM_KEYDOWN) /* && mp1 >= VK_F1 && mp1 <= VK_F24) allow ALL special keys */
      msg = WM_CHAR;

   if (result == -1)
   {
      /* Find any callbacks for this function */
      while (tmp)
      {
         if (tmp->message == msg || msg == WM_COMMAND || msg == WM_NOTIFY || tmp->message == WM_USER+1 || msg == WM_USER+102)
         {
            switch (msg)
            {
               case WM_TIMER:
                  {
                     if (!hWnd)
                     {
                        int (DWSIGNAL *timerfunc)(void *) = tmp->signalfunction;
                        if (tmp->id == (int)mp1)
                        {
                           if (!timerfunc(tmp->data))
                           {
                              dw_timer_disconnect(tmp->id);
                           }
                           tmp = NULL;
                        }
                     }
                     result = 0;
                  }
                  break;
               case WM_SETFOCUS:
                  {
                     int (DWSIGNAL *setfocusfunc)(HWND, void *) = (int (*)(HWND, void *))tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        result = setfocusfunc(tmp->window, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_SIZE:
                  {
                     int (DWSIGNAL *sizefunc)(HWND, int, int, void *) = tmp->signalfunction;
                     if(hWnd == tmp->window)
                     {
                        result = sizefunc(tmp->window, LOWORD(mp2), HIWORD(mp2), tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_LBUTTONDOWN:
                  {
                     int (DWSIGNAL *buttonfunc)(HWND, int, int, int, void *) = (int (*)(HWND, int, int, int, void *))tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        int button=0;

                        switch(origmsg)
                        {
                        case WM_LBUTTONDOWN:
                           button = DW_BUTTON1_MASK;
                           break;
                        case WM_RBUTTONDOWN:
                           button = DW_BUTTON2_MASK;
                           break;
                        case WM_MBUTTONDOWN:
                           button = DW_BUTTON3_MASK;
                           break;
                        }
                        if(taskbar)
                        {
                           POINT ptl;
                           GetCursorPos(&ptl);
                           result = buttonfunc(tmp->window, ptl.x, ptl.y, button, tmp->data);
                        }
                        else
                        {
                           POINTS pts = MAKEPOINTS(mp2);
                           result = buttonfunc(tmp->window, pts.x, pts.y, button, tmp->data);
                        }
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_LBUTTONUP:
                  {
                     int (DWSIGNAL *buttonfunc)(HWND, int, int, int, void *) = (int (*)(HWND, int, int, int, void *))tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        int button=0;

                        switch(origmsg)
                        {
                        case WM_LBUTTONUP:
                           button = DW_BUTTON1_MASK;
                           break;
                        case WM_RBUTTONUP:
                           button = DW_BUTTON2_MASK;
                           break;
                        case WM_MBUTTONUP:
                           button = DW_BUTTON3_MASK;
                           break;
                        }
                        if(taskbar)
                        {
                           POINT ptl;
                           GetCursorPos(&ptl);
                           result = buttonfunc(tmp->window, ptl.x, ptl.y, button, tmp->data);
                        }
                        else
                        {
                           POINTS pts = MAKEPOINTS(mp2);
                           result = buttonfunc(tmp->window, pts.x, pts.y, button, tmp->data);
                        }
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_MOUSEMOVE:
                  {
                     POINTS pts = MAKEPOINTS(mp2);
                     int (DWSIGNAL *motionfunc)(HWND, int, int, int, void *) = (int (*)(HWND, int, int, int, void *))tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        int keys = 0;

                        if (mp1 & MK_LBUTTON)
                           keys = DW_BUTTON1_MASK;
                        if (mp1 & MK_RBUTTON)
                           keys |= DW_BUTTON2_MASK;
                        if (mp1 & MK_MBUTTON)
                           keys |= DW_BUTTON3_MASK;

                        result = motionfunc(tmp->window, pts.x, pts.y, keys, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_CHAR:
                  {
                     int (DWSIGNAL *keypressfunc)(HWND, char, int, int, void *, char *) = tmp->signalfunction;

                     if(hWnd == tmp->window || _dw_toplevel_window(hWnd) == tmp->window)
                     {
                        int special = 0;
                        char *utf8 = NULL, ch[2] = {0};
#ifdef UNICODE
                        WCHAR uc[2] = { 0 };

                        uc[0] = (WCHAR)mp1;
                        utf8 = WideToUTF8(&uc[0]);
#endif

                        if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
                           special |= KC_SHIFT;
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                           special |= KC_CTRL;
                               if(mp2 & (1 << 29))
                           special |= KC_ALT;

                        if(origmsg == WM_CHAR && mp1 < 128)
                           ch[0] = (char)mp1;

                        result = keypressfunc(tmp->window, ch[0], (int)mp1, special, tmp->data, utf8 ? utf8 : ch);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_CLOSE:
                  {
                     int (DWSIGNAL *closefunc)(HWND, void *) = tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        result = closefunc(tmp->window, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_PAINT:
                  {
                     PAINTSTRUCT ps;
                     DWExpose exp;
                     int (DWSIGNAL *exposefunc)(HWND, DWExpose *, void *) = tmp->signalfunction;

                     if(hWnd == tmp->window)
                     {
                        HWND oldrender = _dw_render_expose;

                        BeginPaint(hWnd, &ps);
                        exp.x = ps.rcPaint.left;
                        exp.y = ps.rcPaint.top;
                        exp.width = ps.rcPaint.right - ps.rcPaint.left;
                        exp.height = ps.rcPaint.bottom - ps.rcPaint.top;
                        if(_dw_render_safe_mode == DW_FEATURE_ENABLED && _dw_is_render(hWnd))
                            _dw_render_expose = hWnd;
                        result = exposefunc(hWnd, &exp, tmp->data);
                        _dw_render_expose = oldrender;
                        EndPaint(hWnd, &ps);
                     }
                  }
                  break;
               case WM_NOTIFY:
                  {
                     if(tmp->message == TVN_SELCHANGED ||
                        tmp->message == NM_RCLICK ||
                        tmp->message == TVN_ITEMEXPANDED)
                     {
                        NMTREEVIEW FAR *tem=(NMTREEVIEW FAR *)mp2;
                        NMLISTVIEW FAR *lem=(NMLISTVIEW FAR *)mp2;
                        TCHAR tmpbuf[100] = {0};

                        GetClassName(tem->hdr.hwndFrom, tmpbuf, 99);

                        if(_tcsnicmp(tmpbuf, WC_TREEVIEW, _tcslen(WC_TREEVIEW))==0)
                        {
                           if(tem->hdr.code == TVN_SELCHANGED && tmp->message == TVN_SELCHANGED)
                           {
                              if(tmp->window == tem->hdr.hwndFrom && !dw_window_get_data(tmp->window, "_dw_select_item"))
                              {
                                 int (DWSIGNAL *treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = tmp->signalfunction;
                                 TVITEM tvi;
                                 TCHAR textbuf[1025] = {0}, *textptr = textbuf;

                                 tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
                                 tvi.hItem = tem->itemNew.hItem;
                                 tvi.pszText = textbuf;
                                 tvi.cchTextMax = 1024;

                                 TreeView_GetItem(tmp->window, &tvi);

                                 result = treeselectfunc(tmp->window, tem->itemNew.hItem, WideToUTF8(textptr), tmp->data, (void *)tvi.lParam);

                                 tmp = NULL;
                              }
                           }
                           else if(tem->hdr.code == TVN_ITEMEXPANDED && tmp->message == TVN_ITEMEXPANDED)
                           {
                              if(tmp->window == tem->hdr.hwndFrom && tem->action == TVE_EXPAND)
                              {
                                 int (DWSIGNAL *treeexpandfunc)(HWND, HTREEITEM, void *) = tmp->signalfunction;

                                 result = treeexpandfunc(tmp->window, tem->itemNew.hItem, tmp->data);
                                 tmp = NULL;
                              }
                           }
                           else if(tem->hdr.code == NM_RCLICK && tmp->message == NM_RCLICK)
                           {
                              if(tmp->window == tem->hdr.hwndFrom)
                              {
                                 int (DWSIGNAL *containercontextfunc)(HWND, char *, int, int, void *, void *) = tmp->signalfunction;
                                 HTREEITEM hti;
                                 TVITEM tvi;
                                 TVHITTESTINFO thi;
                                 void *itemdata = NULL;
                                 LONG x, y;
                                 TCHAR textbuf[1025] = {0}, *textptr = textbuf;

                                 dw_pointer_query_pos(&x, &y);

                                 thi.pt.x = x;
                                 thi.pt.y = y;

                                 MapWindowPoints(HWND_DESKTOP, tmp->window, &thi.pt, 1);

                                 hti = TreeView_HitTest(tmp->window, &thi);

                                 if(hti)
                                 {
                                    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
                                    tvi.hItem = hti;
                                    tvi.pszText = textbuf;
                                    tvi.cchTextMax = 1024;

                                    TreeView_GetItem(tmp->window, &tvi);
                                    TreeView_SelectItem(tmp->window, hti);

                                    itemdata = (void *)tvi.lParam;
                                 }
                                 containercontextfunc(tmp->window, WideToUTF8(textptr), x, y, tmp->data, itemdata);
                                 tmp = NULL;
                              }
                           }
                        }
                        else if(_tcsnicmp(tmpbuf, WC_LISTVIEW, _tcslen(WC_LISTVIEW)+1)==0)
                        {
                           if((lem->hdr.code == LVN_ITEMCHANGED && (lem->uChanged & LVIF_STATE)) && tmp->message == TVN_SELCHANGED)
                           {
                              if(tmp->window == tem->hdr.hwndFrom)
                              {
                                 LV_ITEM lvi;
                                 int iItem;

                                 iItem = ListView_GetNextItem(tmp->window, -1, LVNI_SELECTED);

                                 memset(&lvi, 0, sizeof(LV_ITEM));

                                 if(iItem > -1)
                                 {
                                    int (DWSIGNAL *treeselectfunc)(HWND, HWND, char *, void *, void *) = tmp->signalfunction;
                                    void **params;

                                    lvi.iItem = iItem;
                                    lvi.mask = LVIF_PARAM;

                                    ListView_GetItem(tmp->window, &lvi);

                                    params = (void **)lvi.lParam;

                                    treeselectfunc(tmp->window, 0, params ? (char *)params[_DW_DATA_TYPE_STRING] : NULL, tmp->data, params ? params[_DW_DATA_TYPE_POINTER] : NULL);
                                    tmp = NULL;
                                 }
                              }
                           }
                        }
                     }
                     else if(tmp->message == TCN_SELCHANGE)
                     {
                        NMHDR FAR *tem=(NMHDR FAR *)mp2;
                        if(tmp->window == tem->hwndFrom && tem->code == tmp->message)
                        {
                           int (DWSIGNAL *switchpagefunc)(HWND, unsigned long, void *) = tmp->signalfunction;
                           unsigned long num=dw_notebook_page_get(tem->hwndFrom);
                           result = switchpagefunc(tem->hwndFrom, num, tmp->data);
                           tmp = NULL;
                        }
                     }
                     else if(tmp->message == LVN_COLUMNCLICK)
                     {
                        NMLISTVIEW FAR *tem=(NMLISTVIEW FAR *)mp2;
                        if(tmp->window == tem->hdr.hwndFrom && tem->hdr.code == tmp->message)
                        {
                           int (DWSIGNAL *columnclickfunc)(HWND, int, void *) = tmp->signalfunction;
                           result = columnclickfunc(tem->hdr.hwndFrom, tem->iSubItem, tmp->data);
                           tmp = NULL;
                        }
                     }
                     else if(tmp->message == WM_VSCROLL)
                     {
                        NMUPDOWN FAR *tem=(NMUPDOWN FAR *)mp2;
                        if(tmp->window == tem->hdr.hwndFrom && tem->hdr.code == UDN_DELTAPOS)
                        {
                           int (DWSIGNAL *valuechangefunc)(HWND, int, void *) = tmp->signalfunction;
                           result = valuechangefunc(tmp->window, tem->iPos + tem->iDelta, tmp->data);
                           tmp = NULL;
                        }
                     }
                  }
                  break;
               case WM_COMMAND:
                  {
                     int (DWSIGNAL *clickfunc)(HWND, void *) = tmp->signalfunction;
                     HWND command;
                     ULONG passthru = (ULONG)LOWORD(mp1);
                     ULONG message = (ULONG)HIWORD(mp1);

                     command = (HWND)(uintptr_t)passthru;

                     if (message == LBN_SELCHANGE || message == CBN_SELCHANGE)
                     {
                        int (DWSIGNAL *listboxselectfunc)(HWND, int, void *) = tmp->signalfunction;

                        if (tmp->message == LBN_SELCHANGE && tmp->window == (HWND)mp2)
                        {
                           result = listboxselectfunc(tmp->window, dw_listbox_selected(tmp->window), tmp->data);
                           tmp = NULL;
                        }
                     }
#ifdef TOOLBAR
                     else if (message == BN_CLICKED && tmp->message == WM_COMMAND && tmp->window == (HWND)mp2)
                     {
                        TCHAR tmpbuf[100] = {0};

                        GetClassName((HWND)mp2, tmpbuf, 99);

                        /* Make sure this isn't a button, because it will have already been handled */
                        if (_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1) != 0)
                        {
                           result = clickfunc(tmp->window, tmp->data);
                           tmp = NULL;
                        }
                     }
#endif
                     else if (tmp->id && passthru == tmp->id)
                     {
                        HMENU hwndmenu = GetMenu(hWnd), menuowner = _dw_menu_owner((HMENU)tmp->window);

                        if (menuowner == hwndmenu || !menuowner)
                        {
                           _dw_toggle_checkable_menu_item( tmp->window, tmp->id );
                           /*
                            * Call the user supplied callback
                            */
                           result = clickfunc((HWND)(uintptr_t)tmp->id, tmp->data);
                           tmp = NULL;
                        }
                     } /* this fires for checkable menu items */
                     else if ( tmp->window < (HWND)65536 && command == tmp->window && tmp->message != WM_TIMER )
                     {
                        _dw_toggle_checkable_menu_item( _dw_popup ? _dw_popup : tmp->window, DW_POINTER_TO_INT(tmp->data) );
                        result = clickfunc(_dw_popup ? _dw_popup : tmp->window, tmp->data);
                        tmp = NULL;
                     }
                  }
                  break;
               case WM_HSCROLL:
               case WM_VSCROLL:
                  {
                     TCHAR tmpbuf[100] = {0};
                     HWND handle = (HWND)mp2;
                     int (DWSIGNAL *valuechangefunc)(HWND, int, void *) = tmp->signalfunction;

                     if(!GetClassName(handle, tmpbuf, 99))
                     {
                         GetClassName(hWnd, tmpbuf, 99);
                     }

                     if (_tcsnicmp(tmpbuf, TRACKBAR_CLASS, _tcslen(TRACKBAR_CLASS)+1)==0)
                     {

                        if (handle == tmp->window)
                        {
                           int value = (int)SendMessage(handle, TBM_GETPOS, 0, 0);
                           int max = (int)SendMessage(handle, TBM_GETRANGEMAX, 0, 0);
                           ULONG currentstyle = GetWindowLong(handle, GWL_STYLE);

                           if(currentstyle & TBS_VERT)
                              result = valuechangefunc(tmp->window, max - value, tmp->data);
                           else
                              result = valuechangefunc(tmp->window, value, tmp->data);
                           tmp = NULL;
                        }
                     }
                     else if(_tcsnicmp(tmpbuf, SCROLLBARCLASSNAME, _tcslen(SCROLLBARCLASSNAME)+1)==0)
                     {
                        if(handle == tmp->window)
                        {
                           int bar = (origmsg == WM_HSCROLL) ? SB_HORZ : SB_VERT;
                           int value = _dw_handle_scroller(handle, bar, (int)HIWORD(mp1), (int)LOWORD(mp1));

                           if(value > -1)
                           {
                              dw_scrollbar_set_pos(tmp->window, value);
                              result = valuechangefunc(tmp->window, value, tmp->data);
                           }
                           tmp = NULL;
                           msg = 0;
                        }
                     }
                  }
                  break;
               case WM_USER+100:
                  {
                     if(hWnd == tmp->window)
                     {
                        int (DWSIGNAL *htmlresultfunc)(HWND, int, char *, void *, void *) = tmp->signalfunction;
                        void** params = (void**)mp1;

                        return htmlresultfunc(tmp->window, DW_POINTER_TO_INT(params[1]), (char *)params[0], (void *)mp2, tmp->data);
                     }
                  }
                  break;
               case WM_USER+101:
                  {
                     if(hWnd == tmp->window)
                     {
                        int (DWSIGNAL *htmlchangedfunc)(HWND, int, char *, void *) = tmp->signalfunction;

                        return htmlchangedfunc(tmp->window, DW_POINTER_TO_INT(mp1), (char *)mp2, tmp->data);
                     }
                  }
                  break;
               case WM_USER+102:
                  {
                     if(hWnd == tmp->window)
                     {
                        int (DWSIGNAL *clickfunc)(HWND, void *) = tmp->signalfunction;

                        return clickfunc(tmp->window, tmp->data);
                     }
                  }
                  break;
               case WM_USER+103:
                  {
                     if(hWnd == tmp->window)
                     {
                        int (DWSIGNAL *htmlmessagefunc)(HWND, char *, char *, void *) = tmp->signalfunction;

                        return htmlmessagefunc(tmp->window, (char *)mp1, (char *)mp2, tmp->data);
                     }
                  }
                  break;
            }
         }
         if(tmp)
            tmp = tmp->next;
      }
   }

   /* Now that any handlers are done... do normal processing */
   switch( msg )
   {
#ifdef AEROGLASS
   case WM_DWMCOMPOSITIONCHANGED:
      {
         ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);
         
         if(_DwmIsCompositionEnabled)
            _DwmIsCompositionEnabled(&_dw_composition);
         
         /* If we are no longer compositing... */
         if(!_dw_composition)
         {
            MARGINS mar = {0};

            SetLayeredWindowAttributes(hWnd, _dw_transparencykey, 255, LWA_ALPHA);
            if(_DwmExtendFrameIntoClientArea)
               _DwmExtendFrameIntoClientArea(hWnd, &mar);
            _dw_show_margins(hWnd, mar, __LINE__);
         }
         /* If we have started compositing... */
         else
         {
            MARGINS mar = {-1,-1,-1,-1};

            if(cinfo && (cinfo->style & DW_FCF_COMPOSITED) && !IS_WIN8PLUS)
               SetLayeredWindowAttributes(hWnd, _dw_transparencykey, 0, LWA_COLORKEY);
            else
            {
#ifdef DARK_MODE_TITLEBAR_MENU
               if(cinfo)
                  mar = _dw_rect_to_margins(cinfo->rect);
               else
#endif
                  memset(&mar, 0, sizeof(MARGINS));
               SetLayeredWindowAttributes(hWnd, _dw_transparencykey, 255, LWA_ALPHA);
            }
            if(_DwmExtendFrameIntoClientArea)
               _DwmExtendFrameIntoClientArea(hWnd, &mar);
            _dw_show_margins(hWnd, mar, __LINE__);
         }
      }
      break;
   case WM_SETTINGCHANGE:
   {
      if(_DW_DARK_MODE_SUPPORTED && _dw_is_color_scheme_change_message(mp2))
      {
         _DW_DARK_MODE_ENABLED = _dw_should_apps_use_dark_mode();

         _dw_refresh_titlebar_theme_color(hWnd);
         _dw_set_child_window_theme(hWnd, 0);
         EnumChildWindows(hWnd, _dw_set_child_window_theme, 0);
      }
   }
   break;
#endif
   case WM_PAINT:
      {
         PAINTSTRUCT ps;

         BeginPaint(hWnd, &ps);
         EndPaint(hWnd, &ps);
      }
      break;
   case WM_SIZE:
      {
         int x = LOWORD(mp2), y = HIWORD(mp2);

         Box *mybox = (Box *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

         if(mybox && mybox->count)
         {
            static int lastx = -1, lasty = -1;
            static HWND lasthwnd = 0;
            int xborder = 0, yborder = 0;

#ifdef DARK_MODE_TITLEBAR_MENU
            if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
            {
               ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);

               if(cinfo)
               {
                  x -= (cinfo->rect.left + cinfo->rect.right);
                  y -= (cinfo->rect.top + cinfo->rect.bottom);
                  xborder = cinfo->rect.left;
                  yborder = cinfo->rect.top;
               }
            }
#endif

            /* Simple check to prevent duplicate sizing calls */
            if(lastx != x || lasty != y || lasthwnd != hWnd)
            {
               lastx = x;
               lasty = y;
               lasthwnd = hWnd;

               ShowWindow(mybox->items[0].hwnd, SW_HIDE);
               _dw_do_resize(mybox, x, y, xborder, yborder);
               ShowWindow(mybox->items[0].hwnd, SW_SHOW);
               return 0;
            }
         }
      }
      break;
   case WM_USER:
      windowfunc = (void *)mp1;

      if(windowfunc)
         windowfunc((void *)mp2);
      break;
   case WM_USER+5:
      _dw_free_menu_data((HMENU)mp1);
      DestroyMenu((HMENU)mp1);
      break;
   case WM_NOTIFY:
      {
         NMHDR FAR *tem=(NMHDR FAR *)mp2;

         if(tem->code == TCN_SELCHANGING)
         {
            int num=TabCtrl_GetCurSel(tem->hwndFrom);
            NotebookPage **array = (NotebookPage **)dw_window_get_data(tem->hwndFrom, "_dw_array");

            if(num > -1 && array && array[num])
               SetParent(array[num]->hwnd, DW_HWND_OBJECT);

         }
         else if(tem->code == TCN_SELCHANGE)
         {
            int num=TabCtrl_GetCurSel(tem->hwndFrom);
            NotebookPage **array = (NotebookPage **)dw_window_get_data(tem->hwndFrom, "_dw_array");

            if(num > -1 && array && array[num])
               SetParent(array[num]->hwnd, tem->hwndFrom);

            _dw_resize_notebook_page(tem->hwndFrom, num);
         }
         else if(tem->code == LVN_DELETEITEM)
         {
            NMLISTVIEW FAR *lem=(NMLISTVIEW FAR *)mp2;
            LV_ITEM lvi;
            void **params;

            memset(&lvi, 0, sizeof(LV_ITEM));

            lvi.iItem = lem->iItem;
            lvi.mask = LVIF_PARAM;

            ListView_GetItem(lem->hdr.hwndFrom, &lvi);
            params = (void **)lvi.lParam;

            /* Free row data */
            if(params)
            {
               if(params[_DW_DATA_TYPE_STRING])
                  free(params[_DW_DATA_TYPE_STRING]);
               free(params);
            }
         }
      }
      break;
   case WM_HSCROLL:
   case WM_VSCROLL:
      {
         HWND handle = (HWND)mp2;
         int bar = (origmsg == WM_HSCROLL) ? SB_HORZ : SB_VERT;

         if(dw_window_get_data(handle, "_dw_scrollbar"))
         {
            int value = _dw_handle_scroller(handle, bar, (int)HIWORD(mp1), (int)LOWORD(mp1));

            if(value > -1)
               dw_scrollbar_set_pos(handle, value);
         }
         else
         {
            TCHAR tmpbuf[100] = {0};

            GetClassName( hWnd, tmpbuf, 99 );
            if ( _tcsnicmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1 ) == 0 )
            {
               _dw_handle_scroller(hWnd, bar, (int)HIWORD(mp1), (int)LOWORD(mp1));
            }
         }
      }
      break;
   case WM_GETMINMAXINFO:
      {
         MINMAXINFO *info = (MINMAXINFO *)mp2;
         info->ptMinTrackSize.x = 8;
         info->ptMinTrackSize.y = 8;
         return 0;
      }
      break;
   case WM_DESTROY:
      {
         HMENU menu = GetMenu(hWnd);
         ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);

         if(menu || (cinfo && (menu = cinfo->hmenu)))
            _dw_free_menu_data(menu);

         /* Free memory before destroying */
         _dw_free_window_memory(hWnd, 0);
         EnumChildWindows(hWnd, _dw_free_window_memory, 0);
      }
      break;
   case WM_MOUSEMOVE:
      {
         HCURSOR cursor;

         if((cursor = (HCURSOR)dw_window_get_data(hWnd, "_dw_cursor")) ||
            (cursor = (HCURSOR)dw_window_get_data(_dw_toplevel_window(hWnd), "_dw_cursor")))
         {
            SetCursor(cursor);
         }
      }
      break;
   case WM_CTLCOLORSTATIC:
   case WM_CTLCOLORLISTBOX:
   case WM_CTLCOLORBTN:
   case WM_CTLCOLOREDIT:
   case WM_CTLCOLORMSGBOX:
   case WM_CTLCOLORSCROLLBAR:
   case WM_CTLCOLORDLG:
      return _dw_colorwndproc(hWnd, msg, mp1, mp2);
   }
   if(result != -1)
   {
      /* Make sure any queued redraws are handled */
      _dw_redraw(0, FALSE);
      /* Then finally return */
      return result;
   }
   return DefWindowProc(hWnd, msg, mp1, mp2);
}

VOID CALLBACK _dw_timerproc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
   _dw_wndproc(hwnd, msg, (WPARAM)idEvent, 0);
}

LRESULT CALLBACK _dw_framewndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   switch( msg )
   {
   case WM_LBUTTONDOWN:
   case WM_MBUTTONDOWN:
   case WM_RBUTTONDOWN:
      SetActiveWindow(hWnd);
      SetFocus(hWnd);
      break;
   case WM_COMMAND:
   case WM_NOTIFY:
   case WM_MOUSEMOVE:
      _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
#ifdef AEROGLASS
   case WM_THEMECHANGED:
      if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
      {
         SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)_dw_get_syscolor_brush(COLOR_3DFACE));
         InvalidateRect(hWnd, NULL, TRUE);
      }
      break;
   case WM_ERASEBKGND:
      if(_dw_composition)
      {
         ColorInfo *cinfo = _dw_window_get_cinfo(_dw_toplevel_window(hWnd));

         if(cinfo && cinfo->style & DW_FCF_COMPOSITED && !IS_WIN8PLUS)
         {
            static HBRUSH hbrush = 0;
            RECT rect;

            if(!hbrush)
               hbrush = CreateSolidBrush(_dw_transparencykey);

            GetClientRect(hWnd, &rect);
            FillRect((HDC)mp1, &rect, hbrush);
            return TRUE;
         }
      }
      break;
#endif
   case WM_PAINT:
      {
         ColorInfo *thiscinfo = _dw_window_get_cinfo(hWnd);

         if(thiscinfo && thiscinfo->fore != -1 && thiscinfo->back != -1)
         {
            PAINTSTRUCT ps;
            HDC hdcPaint = BeginPaint(hWnd, &ps);
            int success = FALSE;

            if(thiscinfo && thiscinfo->fore != -1 && thiscinfo->back != -1)
            {
               /* Handle foreground */
               if(thiscinfo->fore > -1 && thiscinfo->fore < 18)
               {
                  if(thiscinfo->fore != DW_CLR_DEFAULT)
                  {
                     SetTextColor((HDC)mp1, RGB(_dw_red[thiscinfo->fore],
                                          _dw_green[thiscinfo->fore],
                                          _dw_blue[thiscinfo->fore]));
                  }
               }
               else if((thiscinfo->fore & DW_RGB_COLOR) == DW_RGB_COLOR)
               {
                  SetTextColor((HDC)mp1, RGB(DW_RED_VALUE(thiscinfo->fore),
                                       DW_GREEN_VALUE(thiscinfo->fore),
                                       DW_BLUE_VALUE(thiscinfo->fore)));
               }
               /* Handle background */
               if(thiscinfo->back > -1 && thiscinfo->back < 18)
               {
                  if(thiscinfo->back != DW_CLR_DEFAULT)
                  {
                     SetBkColor((HDC)mp1, RGB(_dw_red[thiscinfo->back],
                                        _dw_green[thiscinfo->back],
                                        _dw_blue[thiscinfo->back]));
                     if(thiscinfo->hbrush)
                        DeleteObject(thiscinfo->hbrush);
                     thiscinfo->hbrush = CreateSolidBrush(RGB(_dw_red[thiscinfo->back],
                                                    _dw_green[thiscinfo->back],
                                                    _dw_blue[thiscinfo->back]));
                     SelectObject(hdcPaint, thiscinfo->hbrush);
                     Rectangle(hdcPaint, ps.rcPaint.left - 1, ps.rcPaint.top - 1, ps.rcPaint.right + 1, ps.rcPaint.bottom + 1);
                     success = TRUE;
                  }
               }
               else if((thiscinfo->back & DW_RGB_COLOR) == DW_RGB_COLOR)
               {
                  SetBkColor((HDC)mp1, RGB(DW_RED_VALUE(thiscinfo->back),
                                     DW_GREEN_VALUE(thiscinfo->back),
                                     DW_BLUE_VALUE(thiscinfo->back)));
                  if(thiscinfo->hbrush)
                     DeleteObject(thiscinfo->hbrush);
                  thiscinfo->hbrush = CreateSolidBrush(RGB(DW_RED_VALUE(thiscinfo->back),
                                                 DW_GREEN_VALUE(thiscinfo->back),
                                                 DW_BLUE_VALUE(thiscinfo->back)));
                  SelectObject(hdcPaint, thiscinfo->hbrush);
                  Rectangle(hdcPaint, ps.rcPaint.left - 1, ps.rcPaint.top - 1, ps.rcPaint.right + 1, ps.rcPaint.bottom + 1);
                  success = TRUE;
               }
            }

            EndPaint(hWnd, &ps);
            if(success)
               return FALSE;
         }

      }
      break;
   }
   return DefWindowProc(hWnd, msg, mp1, mp2);
}

LRESULT CALLBACK _dw_rendwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   LRESULT rcode = TRUE;

   switch( msg )
   {
   case WM_LBUTTONDOWN:
   case WM_MBUTTONDOWN:
   case WM_RBUTTONDOWN:
      SetFocus(hWnd);
      rcode = _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
   case WM_MOUSEMOVE:
      /* call our standard Windows procedure */
      rcode = _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
   case WM_USER+2:
   case WM_LBUTTONUP:
   case WM_MBUTTONUP:
   case WM_RBUTTONUP:
      rcode = _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
   case WM_PAINT:
   case WM_SIZE:
   case WM_COMMAND:
   case WM_CHAR:
   case WM_KEYDOWN:
      rcode = _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
   default:
      break;
   }
   /* only call the default Windows process if the user hasn't handled the message themselves */
   if ( rcode != 0 )
      rcode = DefWindowProc(hWnd, msg, mp1, mp2);
   return rcode;
}

LRESULT CALLBACK _spinnerwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);

   if(msg == WM_MOUSEMOVE)
      _dw_wndproc(hWnd, msg, mp1, mp2);

   if(cinfo)
   {
      switch( msg )
      {
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_KEYDOWN:
         {
            LRESULT ret;

            if(!cinfo || !cinfo->pOldProc)
               ret = DefWindowProc(hWnd, msg, mp1, mp2);
            else
               ret = CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);

            /* Tell the edit control that a buttonpress has
             * occured and to update it's window title.
             */
            if(cinfo && cinfo->buddy)
               SendMessage(cinfo->buddy, WM_USER+10, 0, 0);

            SetTimer(hWnd, 100, 100, (TIMERPROC)NULL);

            return ret;
         }
         break;
      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
      case WM_KEYUP:
         {
            LRESULT ret;

            if(!cinfo || !cinfo->pOldProc)
               ret = DefWindowProc(hWnd, msg, mp1, mp2);
            ret = CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);

            /* Tell the edit control that a buttonpress has
             * occured and to update it's window title.
             */
            if(cinfo && cinfo->buddy)
               SendMessage(cinfo->buddy, WM_USER+10, 0, 0);

            if(hWnd)
               KillTimer(hWnd, 100);

            return ret;
         }
         break;
      case WM_TIMER:
         {
            if(mp1 == 100)
            {
               LRESULT ret;

               if(cinfo && cinfo->buddy)
                  SendMessage(cinfo->buddy, WM_USER+10, 0, 0);

               if(!cinfo || !cinfo->pOldProc)
                  ret = DefWindowProc(hWnd, msg, mp1, mp2);
               ret = CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);

               /* Tell the edit control that a buttonpress has
                * occured and to update it's window title.
                */
               if(cinfo && cinfo->buddy)
                  SendMessage(cinfo->buddy, WM_USER+10, 0, 0);

               return ret;
            }
         }
         break;
      case WM_USER+10:
         {
            if(cinfo->buddy)
            {
               TCHAR tempbuf[100] = { 0 };
               long position;

               GetWindowText(cinfo->buddy, tempbuf, 99);

               position = _tstol(tempbuf);

               SendMessage(hWnd, UDM_SETPOS32, 0, (LPARAM)position);
            }
         }
         break;
      }
   }

   if(!cinfo || !cinfo->pOldProc)
      return DefWindowProc(hWnd, msg, mp1, mp2);
   return CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);
}

void _dw_click_default(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   /* These are the window classes which can
    * obtain input focus.
    */
   if (_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1)==0
#ifdef TOOLBAR
    || _tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0
#endif
      )
   {
      /* Generate click on default item */
      DWSignalHandler *tmp = Root;

      /* Find any callbacks for this function */
      while (tmp)
      {
         if (tmp->message == WM_COMMAND)
         {
            /* Make sure it's the right window, and the right ID */
            if (tmp->window == handle)
            {
               int (DWSIGNAL *clickfunc)(HWND, void *) = tmp->signalfunction;
               clickfunc(tmp->window, tmp->data);
               tmp = NULL;
            }
         }
         if (tmp)
            tmp= tmp->next;
      }
   }
   else
      SetFocus(handle);
}

/* Subclass function that will handle setting colors on controls */
LRESULT CALLBACK _dw_colorwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   ColorInfo *cinfo = _dw_window_get_cinfo(hWnd);
   TCHAR tmpbuf[100] = {0};
   WNDPROC pOldProc = 0;
   LRESULT ret = -1;

   GetClassName(hWnd, tmpbuf, 99);
   if(_tcsncmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1) == 0)
      cinfo = &(((Box *)cinfo)->cinfo);

   if ( msg == WM_MOUSEMOVE || msg == WM_USER+2 )
      ret = _dw_wndproc(hWnd, msg, mp1, mp2);

   if (cinfo)
   {
      pOldProc = cinfo->pOldProc;

      switch( msg )
      {
      case WM_SETFOCUS:
         if(cinfo->combo)
            ret = _dw_wndproc(cinfo->combo, msg, mp1, mp2);
         else
            ret = _dw_wndproc(hWnd, msg, mp1, mp2);
         break;
      case WM_VSCROLL:
      case WM_HSCROLL:
            ret = _dw_wndproc(hWnd, msg, mp1, mp2);
         break;
      case WM_KEYDOWN:
      case WM_KEYUP:
         {
            if (hWnd && (mp1 == VK_UP || mp1 == VK_DOWN))
            {
               if (!cinfo || !cinfo->pOldProc)
                  ret = DefWindowProc(hWnd, msg, mp1, mp2);
               ret = CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);

               /* Tell the spinner control that a keypress has
                * occured and to update it's internal value.
                */
               if (cinfo && cinfo->buddy && !cinfo->combo)
                  PostMessage(hWnd, WM_USER+10, 0, 0);

               if(msg == WM_KEYDOWN)
                  SetTimer(hWnd, 101, 100, (TIMERPROC)NULL);
               else
                  KillTimer(hWnd, 101);

               return ret;
            }
         }
         break;
      case WM_TIMER:
         {
            if(mp1 == 101)
            {
               if(!cinfo || !cinfo->pOldProc)
                  ret = DefWindowProc(hWnd, msg, mp1, mp2);
               ret = CallWindowProc(cinfo->pOldProc, hWnd, msg, mp1, mp2);

               /* Tell the spinner control that a keypress has
                * occured and to update it's internal value.
                */
               if(cinfo && cinfo->buddy && !cinfo->combo)
                  PostMessage(hWnd, WM_USER+10, 0, 0);

               return ret;
            }
         }
         break;
      case WM_CHAR:
         ret = _dw_wndproc(hWnd, msg, mp1, mp2);
         if (ret != TRUE && LOWORD(mp1) == '\t')
         {
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
               if (cinfo->combo)
                  _dw_shift_focus(cinfo->combo, _DW_DIRECTION_BACKWARD);
               else if(cinfo->buddy)
                  _dw_shift_focus(cinfo->buddy, _DW_DIRECTION_BACKWARD);
               else
                  _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
            }
            else
            {
               if (cinfo->combo)
                  _dw_shift_focus(cinfo->combo, _DW_DIRECTION_FORWARD);
               else if(cinfo->buddy)
                  _dw_shift_focus(cinfo->buddy, _DW_DIRECTION_FORWARD);
               else
                  _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
            }
            return FALSE;
         }
         else if(LOWORD(mp1) == '\r')
         {

            if ( cinfo->clickdefault )
            {
               _dw_click_default(cinfo->clickdefault);
               return (LRESULT)TRUE;
            }
            else
            {
               /*
                * Find the toplevel window for the current window and check if it
                * has a default click set
                */
               HWND tl = _dw_toplevel_window( hWnd );
               ColorInfo *mycinfo = _dw_window_get_cinfo(tl);

               if(mycinfo && mycinfo->clickdefault)
               {
                  _dw_click_default(mycinfo->clickdefault);
                  return (LRESULT)TRUE;
               }
            }
         }

         /* Tell the spinner control that a keypress has
          * occured and to update it's internal value.
          */
         if (cinfo->buddy && !cinfo->combo)
         {
            PostMessage(cinfo->buddy, WM_USER+10, 0, 0);
         }
         break;
      case WM_USER+10:
         {
            if(cinfo->buddy)
            {
               long val;

               val = (long)SendMessage(cinfo->buddy, UDM_GETPOS32, 0, 0);

               _sntprintf(tmpbuf, 99, TEXT("%ld"), val);
               SetWindowText(hWnd, tmpbuf);
            }
         }
         break;
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORBTN:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORMSGBOX:
      case WM_CTLCOLORSCROLLBAR:
      case WM_CTLCOLORDLG:
         {
            ColorInfo *thiscinfo = _dw_window_get_cinfo((HWND)mp2);

            if(msg == WM_CTLCOLORBTN)
            {
               /* Groupbox color info is on the frame window it is attached to */
               if(GetWindowLongPtr((HWND)mp2, GWL_STYLE) & BS_GROUPBOX)
               {
                  Box *framebox = (Box *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                  if(framebox)
                     thiscinfo = &framebox->cinfo;
               }
            }

            if(thiscinfo && thiscinfo->fore != -1 && thiscinfo->back != -1)
            {
               int thisback = thiscinfo->back;

               /* Handle foreground */
               if(thiscinfo->fore != DW_CLR_DEFAULT)
               {
                  int fore = _dw_internal_color(thiscinfo->fore);

                  SetTextColor((HDC)mp1, RGB(DW_RED_VALUE(fore),
                                       DW_GREEN_VALUE(fore),
                                       DW_BLUE_VALUE(fore)));
               }
#ifdef AEROGLASS
               else if(thiscinfo->fore == DW_CLR_DEFAULT && _DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_ENABLED)
                  SetTextColor((HDC)mp1, _dw_get_syscolor(COLOR_WINDOWTEXT));
#endif
               /* Handle background */
               if(thiscinfo->back == DW_RGB_TRANSPARENT)
               {
                  ColorInfo *parentcinfo = _dw_window_get_cinfo(hWnd);

                  if(parentcinfo && parentcinfo->back != -1)
                     thisback = parentcinfo->back;
               }
               if(thisback == DW_CLR_DEFAULT)
               {
                  HBRUSH hbr = _dw_get_syscolor_brush(COLOR_3DFACE);

                  SetBkColor((HDC)mp1, _dw_get_syscolor(COLOR_3DFACE));

                  SelectObject((HDC)mp1, hbr);
                  return (LRESULT)(intptr_t)hbr;
               }
               else if(thisback != -1 && thisback != DW_RGB_TRANSPARENT)
               {
                  int back = _dw_internal_color(thisback);

                  SetBkColor((HDC)mp1, RGB(DW_RED_VALUE(back),
                                     DW_GREEN_VALUE(back),
                                     DW_BLUE_VALUE(back)));
                  if(thiscinfo->hbrush)
                     DeleteObject(thiscinfo->hbrush);
                  thiscinfo->hbrush = CreateSolidBrush(RGB(DW_RED_VALUE(back),
                                                 DW_GREEN_VALUE(back),
                                                 DW_BLUE_VALUE(back)));
                  SelectObject((HDC)mp1, thiscinfo->hbrush);
                  return (LRESULT)(intptr_t)thiscinfo->hbrush;
               }
            }
 #ifdef AEROGLASS
            /* First handle the transparent or layered cases */
            switch(msg)
            {
               case WM_CTLCOLORSTATIC:
               case WM_CTLCOLORBTN:
               case WM_CTLCOLORDLG:
                  {
                     ColorInfo *tlcinfo = _dw_window_get_cinfo(_dw_toplevel_window(hWnd));

                     if((_dw_composition && tlcinfo && (tlcinfo->style & DW_FCF_COMPOSITED)) && !IS_WIN8PLUS &&
                        (!thiscinfo || (thiscinfo && (thiscinfo->back == -1 || thiscinfo->back == DW_RGB_TRANSPARENT))))
                     {
                        if(!(msg == WM_CTLCOLORSTATIC && SendMessage((HWND)mp2, STM_GETIMAGE, IMAGE_BITMAP, 0)))
                        {
                           SetBkColor((HDC)mp1, _dw_transparencykey);
                           if(thiscinfo && thiscinfo->hbrush)
                              DeleteObject(thiscinfo->hbrush);
                           thiscinfo->hbrush = CreateSolidBrush(_dw_transparencykey);
                           SelectObject((HDC)mp1, thiscinfo->hbrush);
                           return (LRESULT)thiscinfo->hbrush;
                        }
                     }
                  }
            }
            /* Second we handle the dark mode cases */
            switch(msg)
            {
               case WM_CTLCOLORSTATIC:
               case WM_CTLCOLORLISTBOX:
               case WM_CTLCOLORBTN:
               case WM_CTLCOLOREDIT:
               case WM_CTLCOLORMSGBOX:
               case WM_CTLCOLORSCROLLBAR:
               case WM_CTLCOLORDLG:
               {
                  if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_ENABLED)
                  {
                     ColorInfo *parentcinfo = _dw_window_get_cinfo(hWnd);
                     int thisback = thiscinfo ? thiscinfo->back : -1;

                     if(thisback == DW_RGB_TRANSPARENT && parentcinfo)
                        thisback = parentcinfo->back;

                     if(!thiscinfo || (thiscinfo && (thiscinfo->fore == DW_CLR_DEFAULT || thiscinfo->fore == -1)))
                        SetTextColor((HDC)mp1, _dw_get_syscolor(COLOR_WINDOWTEXT));
                     if(!thiscinfo || (thiscinfo && (thisback == DW_CLR_DEFAULT || thisback == -1 || thisback == DW_RGB_TRANSPARENT)))
                     {
                        HBRUSH hbr = _dw_get_syscolor_brush(COLOR_3DFACE);

                        SetBkColor((HDC)mp1, _dw_get_syscolor(COLOR_3DFACE));

                        SelectObject((HDC)mp1, hbr);
                        return (LRESULT)(intptr_t)hbr;
                     }
                  }
               }
            }
#endif
         }
         break;
      }
   }

   if(ret != TRUE)
   {
       if(!pOldProc)
          return DefWindowProc(hWnd, msg, mp1, mp2);
       return CallWindowProc(pOldProc, hWnd, msg, mp1, mp2);
   }
   return ret;
}

/* Window procedure for container/listview */
LRESULT CALLBACK _dw_containerwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   ContainerInfo *continfo = (ContainerInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   switch( msg )
   {
   case WM_COMMAND:
   case WM_NOTIFY:
   case WM_MOUSEMOVE:
      _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
#ifdef AEROGLASS
   case WM_THEMECHANGED:
      if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
      {
         if(!continfo || continfo->cinfo.back == -1 || continfo->cinfo.back == DW_CLR_DEFAULT)
         {
            COLORREF bk = _dw_get_syscolor(COLOR_WINDOW);

            ListView_SetBkColor(hWnd, bk);
            ListView_SetTextBkColor(hWnd, bk);
         }
         if(!continfo || continfo->cinfo.fore == -1 || continfo->cinfo.fore == DW_CLR_DEFAULT)
            ListView_SetTextColor(hWnd, _dw_get_syscolor(COLOR_WINDOWTEXT));
      }
      break;
#endif
   case WM_PAINT:
       if(continfo->cinfo.pOldProc && (continfo->even != DW_RGB_TRANSPARENT || continfo->odd != DW_RGB_TRANSPARENT))
       {
            RECT rectUpd, rectDestin, rectThis, *rect = &rectThis;
            int iItems, iTop, i;
            COLORREF c, odd, even;
            unsigned long temp = _dw_internal_color(continfo->odd);

            /* Create the colors based on the current theme */
            if(continfo->odd == DW_CLR_DEFAULT)
            {
#ifdef AEROGLASS
               if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_ENABLED)
                  odd = RGB(100,100,100);
               else
#endif
                  odd = RGB(230, 230, 230);
            }
            else
                  odd = RGB(DW_RED_VALUE(temp), DW_GREEN_VALUE(temp), DW_BLUE_VALUE(temp));
            temp = _dw_internal_color(continfo->even);
            if(continfo->even == DW_CLR_DEFAULT)
               even = DW_RGB_TRANSPARENT;
            else
               even = RGB(DW_RED_VALUE(temp), DW_GREEN_VALUE(temp), DW_BLUE_VALUE(temp));

            /* Load the default background color for the first pass */
            ListView_SetTextBkColor(hWnd, continfo->cinfo.back != -1 ? continfo->cinfo.back : ListView_GetBkColor(hWnd));
            /* get the rectangle to be updated */
            GetUpdateRect(hWnd, &rectUpd, FALSE);
            /* allow default processing first */
            CallWindowProc(continfo->cinfo.pOldProc, hWnd, msg, 0, 0);
            /* number of displayed rows */
            iItems = ListView_GetCountPerPage(hWnd);
            /* first visible row */
            iTop = ListView_GetTopIndex(hWnd);

            for(i=iTop; i<=(iTop+iItems+1); i++)
            {
                /* if row rectangle intersects update rectangle then it requires re-drawing */
                if(ListView_GetItemRect(hWnd, i, rect, LVIR_BOUNDS) && IntersectRect(&rectDestin, &rectUpd, rect))
                {
                    /* change text background colour accordingly */
                    c = (i % 2) ? odd : even;

                    if(c != DW_RGB_TRANSPARENT)
                    {
                        ListView_SetTextBkColor(hWnd, c);
                        /* invalidate the row rectangle then... */
                        InvalidateRect(hWnd, &rectDestin, FALSE);
                        /* ...force default processing */
                        CallWindowProc(continfo->cinfo.pOldProc, hWnd, msg, 0, 0);
                    }
                }
            }
       }
       break;
   case WM_LBUTTONDBLCLK:
   case WM_CHAR:
      {
         LV_ITEM lvi;
         int iItem;
         void **params = NULL;

         if(LOWORD(mp1) == '\t')
         {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
               _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
            else
               _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
            return FALSE;
         }

         if(msg == WM_CHAR && (char)mp1 != '\r')
            break;

         iItem = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED);

         memset(&lvi, 0, sizeof(LV_ITEM));

         if(iItem > -1)
         {
            lvi.iItem = iItem;
            lvi.mask = LVIF_PARAM;

            ListView_GetItem(hWnd, &lvi);
            params = (void **)lvi.lParam;
         }

         {
            DWSignalHandler *tmp = Root;

            while(tmp)
            {
               if(tmp->message == NM_DBLCLK && tmp->window == hWnd)
               {
                  int (DWSIGNAL *containerselectfunc)(HWND, char *, void *, void *) = tmp->signalfunction;

                  containerselectfunc(tmp->window, params ? params[_DW_DATA_TYPE_STRING] : NULL, tmp->data, params ? params[_DW_DATA_TYPE_POINTER] : NULL);
                  tmp = NULL;
               }
               if(tmp)
                  tmp = tmp->next;
            }
         }
      }
      break;
   case WM_CONTEXTMENU:
      {
         DWSignalHandler *tmp = Root;
         void **params = NULL;
         while(tmp)
         {
            if(tmp->message == NM_RCLICK && tmp->window == hWnd)
            {
               int (DWSIGNAL *containercontextfunc)(HWND, char *, int, int, void *, void *) = tmp->signalfunction;
               LONG x,y;
               LV_ITEM lvi;
               int iItem;
               LVHITTESTINFO lhi;
               TCHAR textbuf[1025] = {0};

               dw_pointer_query_pos(&x, &y);

               lhi.pt.x = x;
               lhi.pt.y = y;

               MapWindowPoints(HWND_DESKTOP, tmp->window, &lhi.pt, 1);

               iItem = ListView_HitTest(tmp->window, &lhi);

               memset(&lvi, 0, sizeof(LV_ITEM));

               if(iItem > -1)
               {
                  lvi.iItem = iItem;
                  lvi.pszText = textbuf;
                  lvi.cchTextMax = 1024;
                  lvi.mask = LVIF_PARAM | LVIF_TEXT;

                  ListView_GetItem(tmp->window, &lvi);
                  ListView_SetSelectionMark(tmp->window, iItem);
                  params = (void **)lvi.lParam;
               }

               containercontextfunc(tmp->window, params ? params[_DW_DATA_TYPE_STRING] : NULL, x, y, tmp->data, params ? params[_DW_DATA_TYPE_POINTER] : NULL);
               tmp = NULL;
            }
            if(tmp)
               tmp = tmp->next;
         }
      }
      break;
   }

   if(!continfo || !continfo->cinfo.pOldProc)
      return DefWindowProc(hWnd, msg, mp1, mp2);
   return CallWindowProc(continfo->cinfo.pOldProc, hWnd, msg, mp1, mp2);
}

LRESULT CALLBACK _dw_simplewndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   ContainerInfo *cinfo;
   LRESULT ret = -1;

   cinfo = (ContainerInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   switch( msg )
   {
   case WM_MOUSEMOVE:
      ret = _dw_wndproc(hWnd, msg, mp1, mp2);
      break;
   case WM_CHAR:
      ret = _dw_wndproc(hWnd, msg, mp1, mp2);
      if(ret != TRUE && LOWORD(mp1) == '\t')
      {
         if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
            _dw_shift_focus(hWnd, _DW_DIRECTION_BACKWARD);
         else
            _dw_shift_focus(hWnd, _DW_DIRECTION_FORWARD);
         return FALSE;
      }
      break;
   }

   if(ret != TRUE)
   {
       if(!cinfo || !cinfo->cinfo.pOldProc)
          return DefWindowProc(hWnd, msg, mp1, mp2);
       return CallWindowProc(cinfo->cinfo.pOldProc, hWnd, msg, mp1, mp2);
   }
   return ret;
}

#ifdef RICHEDIT
#define ENTRY_CUT    60901
#define ENTRY_COPY   60902
#define ENTRY_PASTE  60903
#define ENTRY_DELETE 60904
#define ENTRY_UNDO   60905
#define ENTRY_SALL   60906


/* Special WM_NOTIFY handler for Rich Edit controls, to implement the context menu. */
LRESULT CALLBACK _dw_richeditwndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
    switch(msg)
    {    
        case WM_CONTEXTMENU:
        {
            HMENUI hwndMenu = dw_menu_new(0L);
            long x, y;
            unsigned long style = 0L;

            /* When readonly, disable: Undo, Cut, Paste, Delete */
            if(GetWindowLongPtr(hWnd, GWL_STYLE) & ES_READONLY)
                style = DW_MIS_DISABLED;

            dw_menu_append_item(hwndMenu, "Undo", ENTRY_UNDO, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Cut", ENTRY_CUT, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Copy", ENTRY_COPY, 0L, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Paste", ENTRY_PASTE, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Delete", ENTRY_DELETE, style, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, -1, 0L);
            dw_menu_append_item(hwndMenu, "Select All", ENTRY_SALL, 0L, TRUE, -1, 0L);

            dw_pointer_query_pos(&x, &y);
            dw_menu_popup(&hwndMenu, hWnd, x, y);
            return TRUE;
        }
        break;
        case WM_COMMAND:
        {
            if(HIWORD(mp1) == 0)
            {
                switch(LOWORD(mp1))
                {
                    case ENTRY_CUT:
                    SendMessage(hWnd, WM_CUT, 0, 0);
                    break;
                    case ENTRY_COPY:
                    SendMessage(hWnd, WM_COPY, 0, 0);
                    break;
                    case ENTRY_PASTE:
                    SendMessage(hWnd, WM_PASTE, 0, 0);
                    break;
                    case ENTRY_DELETE:
                    SendMessage(hWnd, WM_CLEAR, 0, 0);
                    break;
                    case ENTRY_UNDO:
                    SendMessage(hWnd, EM_UNDO, 0, 0);
                    break;
                    case ENTRY_SALL:
                    SendMessage(hWnd, EM_SETSEL, 0, (LPARAM)-1);
                    break;
                }
            }
        }
        break;
    }
    return _dw_simplewndproc(hWnd, msg, mp1, mp2);
}
#endif


LRESULT CALLBACK _dw_treewndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
#ifdef AEROGLASS
   if(msg == WM_THEMECHANGED)
   {
      if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
      {
         ContainerInfo *continfo = (ContainerInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

         if(!continfo || continfo->cinfo.back == -1 || continfo->cinfo.back == DW_CLR_DEFAULT)
            TreeView_SetBkColor(hWnd, _dw_get_syscolor(COLOR_WINDOW));
         if(!continfo || continfo->cinfo.fore == -1 || continfo->cinfo.fore == DW_CLR_DEFAULT)
            TreeView_SetTextColor(hWnd, _dw_get_syscolor(COLOR_WINDOWTEXT));
      }
   }
#endif
   return _dw_simplewndproc(hWnd, msg, mp1, mp2);
}

void _dw_change_box(Box *thisbox, int percent, int type)
{
   int z;

   for(z=0;z<thisbox->count;z++)
   {
      if(thisbox->items[z].type == _DW_TYPE_BOX)
      {
         Box *tmp = (Box*)GetWindowLongPtr(thisbox->items[z].hwnd, GWLP_USERDATA);
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
   HWND handle1 = (HWND)dw_window_get_data(hwnd, "_dw_topleft");
   HWND handle2 = (HWND)dw_window_get_data(hwnd, "_dw_bottomright");

   ShowWindow(handle1, SW_HIDE);
   ShowWindow(handle2, SW_HIDE);

   if(type == DW_HORZ)
   {
      int newx = x;
      float ratio = (float)percent/(float)100.0;
      Box *tmp = (Box *)GetWindowLongPtr(handle1, GWLP_USERDATA);

      newx = (int)((float)newx * ratio) - (_DW_SPLITBAR_WIDTH/2);

      MoveWindow(handle1, 0, 0, newx, y, FALSE);
      _dw_do_resize(tmp, newx - 1, y - 1, 0, 0);

      tmp = (Box *)GetWindowLongPtr(handle2, GWLP_USERDATA);

      newx = x - newx - _DW_SPLITBAR_WIDTH;

      MoveWindow(handle2, x - newx, 0, newx, y, FALSE);
      _dw_do_resize(tmp, newx - 1, y - 1, 0, 0);

      dw_window_set_data(hwnd, "_dw_start", DW_INT_TO_POINTER(newx));
   }
   else
   {
      int newy = y;
      float ratio = (float)(100.0-percent)/(float)100.0;
      Box *tmp = (Box *)GetWindowLongPtr(handle2, GWLP_USERDATA);

      newy = (int)((float)newy * ratio) - (_DW_SPLITBAR_WIDTH/2);

      MoveWindow(handle2, 0, y - newy, x, newy, FALSE);
      _dw_do_resize(tmp, x - 1, newy - 1, 0, 0);

      tmp = (Box *)GetWindowLongPtr(handle1, GWLP_USERDATA);

      newy = y - newy - _DW_SPLITBAR_WIDTH;

      MoveWindow(handle1, 0, 0, x, newy, FALSE);
      _dw_do_resize(tmp, x - 1, newy - 1, 0, 0);

      dw_window_set_data(hwnd, "_dw_start", DW_INT_TO_POINTER(newy));
   }

   ShowWindow(handle1, SW_SHOW);
   ShowWindow(handle2, SW_SHOW);
}

/* This handles any activity on the scrollbox */
LRESULT CALLBACK _dw_scrollwndproc(HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   switch (msg)
   {
       case WM_HSCROLL:
       case WM_VSCROLL:
       {
            ColorInfo *cinfo = _dw_window_get_cinfo(hwnd);
            SCROLLINFO hsi, vsi, *si = &hsi;
            int bar = SB_HORZ;
            int which = LOWORD(mp1);

            /* Initialize the scroll info structs */
            vsi.cbSize = hsi.cbSize = sizeof(SCROLLINFO);
            vsi.fMask = hsi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;

            /* Save the current scroll positions */
            GetScrollInfo(hwnd, SB_HORZ, &hsi);
            GetScrollInfo(hwnd, SB_VERT, &vsi);

            if(msg == WM_VSCROLL)
            {
                bar = SB_VERT;
                si = &vsi;
            }

            switch(which)
            {
            case SB_THUMBTRACK:
                si->nPos = HIWORD(mp1);
                break;
            /*case SB_PAGEDOWN:*/
            case SB_PAGELEFT:
                si->nPos = si->nPos - si->nPage;
                if(si->nPos < 0)
                    si->nPos = 0;
                break;
            /*case SB_PAGEUP:*/
            case SB_PAGERIGHT:
                si->nPos = si->nPos + si->nPage;
                if(si->nPos > (int)(si->nMax - si->nPage) + 1)
                    si->nPos = (si->nMax - si->nPage) + 1;
                break;
            /*case SB_LINEDOWN:*/
            case SB_LINELEFT:
                si->nPos = si->nPos - 1;
                if(si->nPos < si->nMin)
                    si->nPos = si->nMin;
                break;
            /*case SB_LINEUP:*/
            case SB_LINERIGHT:
                si->nPos = si->nPos + 1;
                if(si->nPos > (int)(si->nMax - si->nPage) + 1)
                    si->nPos = (si->nMax - si->nPage) + 1;
                break;
            }

            /* Position the scrolled box */
            vsi.fMask = hsi.fMask = SIF_POS | SIF_DISABLENOSCROLL;
            SetWindowPos(cinfo->combo, 0, -hsi.nPos, -vsi.nPos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            SetScrollInfo(hwnd, bar, si, TRUE);
       }
       break;
   }
   return DefWindowProc(hwnd, msg, mp1, mp2);
}

/* This handles any activity on the splitbars (sizers) */
LRESULT CALLBACK _dw_splitwndproc(HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   switch (msg)
   {
   case WM_ACTIVATE:
   case WM_SETFOCUS:
      return FALSE;
#ifdef AEROGLASS
   case WM_THEMECHANGED:
      if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
         InvalidateRect(hwnd, NULL, TRUE);
      break;
#endif
   case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdcPaint;
         int type = DW_POINTER_TO_INT(dw_window_get_data(hwnd, "_dw_type"));
         int start = DW_POINTER_TO_INT(dw_window_get_data(hwnd, "_dw_start"));

         BeginPaint(hwnd, &ps);

         if((hdcPaint = GetDC(hwnd)) != NULL)
         {
            unsigned long cx, cy;
            HBRUSH oldBrush = SelectObject(hdcPaint, _dw_get_syscolor_brush(COLOR_3DFACE));
            HPEN oldPen = SelectObject(hdcPaint, CreatePen(PS_SOLID, 1, _dw_get_syscolor(COLOR_3DFACE)));

            dw_window_get_pos_size(hwnd, NULL, NULL, &cx, &cy);

            if(type == DW_HORZ)
               Rectangle(hdcPaint, cx - start - _DW_SPLITBAR_WIDTH, 0, cx - start, cy);
            else
               Rectangle(hdcPaint, 0, start, cx, start + _DW_SPLITBAR_WIDTH);

            SelectObject(hdcPaint, oldBrush);
            DeleteObject(SelectObject(hdcPaint, oldPen));
            ReleaseDC(hwnd, hdcPaint);
         }
         EndPaint(hwnd, &ps);
      }
      break;
   case WM_LBUTTONDOWN:
      {
         SetCapture(hwnd);
         break;
      }
   case WM_LBUTTONUP:
      {
            if(GetCapture() == hwnd)
            ReleaseCapture();
      }
      break;
   case WM_MOUSEMOVE:
      {
         float *percent = (float *)dw_window_get_data(hwnd, "_dw_percent");
         int type = DW_POINTER_TO_INT(dw_window_get_data(hwnd, "_dw_type"));
         int start;

         if(type == DW_HORZ)
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
         else
            SetCursor(LoadCursor(NULL, IDC_SIZENS));

         if(GetCapture() == hwnd && percent)
         {
            POINT point;
            RECT rect;
            static POINT lastpoint;

            GetCursorPos(&point);
            GetWindowRect(hwnd, &rect);

            if(memcmp(&point, &lastpoint, sizeof(POINT)))
            {
               if(PtInRect(&rect, point))
               {
                  int width = (rect.right - rect.left);
                  int height = (rect.bottom - rect.top);

                  if(type == DW_HORZ)
                  {
                     start = point.x - rect.left;
                     if(width - _DW_SPLITBAR_WIDTH > 1 && start < width - _DW_SPLITBAR_WIDTH)
                        *percent = ((float)start / (float)(width - _DW_SPLITBAR_WIDTH)) * (float)100.0;
                  }
                  else
                  {
                     start = point.y - rect.top;
                     if(height - _DW_SPLITBAR_WIDTH > 1 && start < height - _DW_SPLITBAR_WIDTH)
                        *percent = ((float)start / (float)(height - _DW_SPLITBAR_WIDTH)) * (float)100.0;
                  }
                  _dw_handle_splitbar_resize(hwnd, *percent, type, width, height);
               }
               memcpy(&lastpoint, &point, sizeof(POINT));
            }
         }
         break;
      }
   }
   return DefWindowProc(hwnd, msg, mp1, mp2);
}

/* This handles drawing the status text areas */
LRESULT CALLBACK _dw_statuswndproc(HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
   switch (msg)
   {
   case WM_MOUSEMOVE:
      _dw_wndproc(hwnd, msg, mp1, mp2);
      break;
   case WM_SETTEXT:
      {
         /* Make sure the control redraws when there is a text change */
         int ret = (int)DefWindowProc(hwnd, msg, mp1, mp2);

         InvalidateRgn(hwnd, NULL, TRUE);
         return ret;
      }
#ifdef AEROGLASS
   case WM_THEMECHANGED:
      if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
         InvalidateRect(hwnd, NULL, TRUE);
      break;
#endif
   case WM_PAINT:
      {
         HDC hdcPaint;
         PAINTSTRUCT ps;
         RECT rc;
         unsigned long cx, cy;
         TCHAR tempbuf[1025] = { 0 };
         ColorInfo *cinfo = _dw_window_get_cinfo(hwnd);
         HFONT hfont = _dw_acquire_font(hwnd, cinfo ? cinfo->fontname : NULL);
         HFONT oldfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);

         dw_window_get_pos_size(hwnd, NULL, NULL, &cx, &cy);
         GetWindowText(hwnd, tempbuf, 1024);

         hdcPaint = BeginPaint(hwnd, &ps);
         if(hfont)
            oldfont = (HFONT)SelectObject(hdcPaint, hfont);

         SetRect(&rc, 0, 0, cx, cy);

         /* If we are in full dark mode, or we have custom colors selected...
          * we will draw the status window ourselves... otherwise DrawStatusText()
          */
         if(
#ifdef AEROGLASS
            (_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_ENABLED) ||
#endif
            (cinfo && cinfo->fore != -1 && cinfo->fore != DW_CLR_DEFAULT &&
             cinfo->back !=- -1 && cinfo->back != DW_CLR_DEFAULT))
         {
            ULONG fore = (cinfo && (cinfo->fore != -1 && cinfo->fore != DW_CLR_DEFAULT)) ? _dw_internal_color(cinfo->fore) : DW_RGB_TRANSPARENT;
            ULONG back = (cinfo && (cinfo->back != -1 && cinfo->back != DW_CLR_DEFAULT)) ? _dw_internal_color(cinfo->back) : DW_RGB_TRANSPARENT;
            HBRUSH hbrback, hbrfore;
            hbrfore = fore != DW_RGB_TRANSPARENT ? CreateSolidBrush(RGB(DW_RED_VALUE(fore), DW_GREEN_VALUE(fore), DW_BLUE_VALUE(fore))) : NULL;
            hbrback = back != DW_RGB_TRANSPARENT ? CreateSolidBrush(RGB(DW_RED_VALUE(back), DW_GREEN_VALUE(back), DW_BLUE_VALUE(back))) : NULL;
            /* Fill with the background color */
            FillRect(hdcPaint, &rc, hbrback ? hbrback : _dw_get_syscolor_brush(COLOR_3DFACE));
            /* Dwaw a border around it */
            FrameRect(hdcPaint, &rc, hbrfore ? hbrfore : _dw_get_syscolor_brush(COLOR_WINDOWTEXT));
            SetRect(&rc, 3, 1, cx -1, cy - 1);
            SetTextColor(hdcPaint, fore != DW_RGB_TRANSPARENT ? RGB(DW_RED_VALUE(fore), DW_GREEN_VALUE(fore), DW_BLUE_VALUE(fore)) : _dw_get_syscolor(COLOR_WINDOWTEXT));
            SetBkMode(hdcPaint, TRANSPARENT);
            /* Draw the text in the middle */
            ExtTextOut(hdcPaint, 3, 1, ETO_CLIPPED, &rc, tempbuf, (UINT)_tcslen(tempbuf), NULL);
            if(hbrfore)
               DeleteObject(hbrfore);
            if(hbrback)
               DeleteObject(hbrback);
         }
         else
            DrawStatusText(hdcPaint, &rc, tempbuf, 0);
         if(hfont && oldfont)
            SelectObject(hdcPaint, oldfont);
         if(hfont)
            DeleteObject(hfont);
         EndPaint(hwnd, &ps);
      }
      return FALSE;
   }
   return DefWindowProc(hwnd, msg, mp1, mp2);
}

#ifdef AEROGLASS
/* Window procedure to handle drawing themed text when in composited mode */
LRESULT CALLBACK _dw_staticwndproc(HWND hwnd, ULONG msg, WPARAM mp1, LPARAM mp2)
{
   ColorInfo *parentcinfo, *tlcinfo, *cinfo = _dw_window_get_cinfo(hwnd);
   WNDPROC pOldProc;

   if (!cinfo)
      return DefWindowProc(hwnd, msg, mp1, mp2);

   /* Need the parent to do the check completely */
   parentcinfo = _dw_window_get_cinfo(GetParent(hwnd));
   tlcinfo = _dw_window_get_cinfo(_dw_toplevel_window(hwnd));

   /* If we don't require themed drawing */
   if(((cinfo->back != -1 && cinfo->back != DW_RGB_TRANSPARENT) || (parentcinfo && parentcinfo->back != -1))
       || !_dw_composition || !tlcinfo || !(tlcinfo->style & DW_FCF_COMPOSITED) || IS_WIN8PLUS)
      return _dw_colorwndproc(hwnd, msg, mp1, mp2);

   pOldProc = cinfo->pOldProc;

   switch(msg)
   {
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hwnd, &ps);

         if(hdc)
         {
            /* Figure out how to draw */
            HDC hdcPaint = NULL;
            RECT rcClient;
            LONG_PTR dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
            LONG_PTR dwStyleEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            HTHEME hTheme = _OpenThemeData(NULL, L"ControlPanelStyle");

            GetClientRect(hwnd, &rcClient);

            if(hTheme)
            {
               /* Create an in memory image to draw to */
               HPAINTBUFFER hBufferedPaint = _BeginBufferedPaint(hdc, &rcClient, BPBF_TOPDOWNDIB, NULL, &hdcPaint);

               if(hdcPaint)
               {
                  LONG_PTR dwStaticStyle = dwStyle & 0x1F;
                  HFONT hFontOld = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
                  int iLen = GetWindowTextLength(hwnd), fore = _dw_internal_color(cinfo->fore);
                  DTTOPTS DttOpts = { sizeof(DTTOPTS) };
                  static HBRUSH hbrush = 0;

                  /* Make sure we have a transparency brush */
                  if(!hbrush)
                     hbrush = CreateSolidBrush(_dw_transparencykey);

                  /* Fill the background with the transparency color */
                  FillRect(hdcPaint, &rcClient, hbrush);
                  _BufferedPaintSetAlpha(hBufferedPaint, &ps.rcPaint, 0x00);

                  /* Setup how we will draw the text */
                  DttOpts.dwFlags = DTT_COMPOSITED | DTT_GLOWSIZE | DTT_TEXTCOLOR;
                  DttOpts.crText = cinfo->fore == -1 ? RGB(255, 255, 255) : RGB(DW_RED_VALUE(fore), DW_GREEN_VALUE(fore), DW_BLUE_VALUE(fore));
                  DttOpts.iGlowSize = 12;

                  SetBkMode(hdcPaint, TRANSPARENT);

                  if(hFontOld)
                     hFontOld = (HFONT)SelectObject(hdcPaint, hFontOld);

                  /* Make sure there is text to draw */
                  if(iLen)
                  {
                     LPWSTR szText = (LPWSTR)_alloca(sizeof(WCHAR)*(iLen+5));

                     if(szText)
                     {
                        iLen = GetWindowTextW(hwnd, szText, iLen+5);
                        if(iLen)
                        {
                           DWORD dwFlags = DT_WORDBREAK;

                           switch (dwStaticStyle)
                           {
                              case SS_CENTER:
                                 dwFlags |= DT_CENTER;
                                 break;
                              case SS_RIGHT:
                                 dwFlags |= DT_RIGHT;
                                 break;
                              case SS_LEFTNOWORDWRAP:
                                 dwFlags &= ~DT_WORDBREAK;
                                 break;
                           }

                           if(dwStyle & SS_CENTERIMAGE)
                           {
                              dwFlags |= DT_VCENTER;
                              dwFlags &= ~DT_WORDBREAK;
                           }


                           if(dwStyle & SS_ENDELLIPSIS)
                              dwFlags |= DT_END_ELLIPSIS|DT_MODIFYSTRING;
                           else if(dwStyle & SS_PATHELLIPSIS)
                              dwFlags |= DT_PATH_ELLIPSIS|DT_MODIFYSTRING;
                           else if(dwStyle & SS_WORDELLIPSIS)
                              dwFlags |= DT_WORD_ELLIPSIS|DT_MODIFYSTRING;

                           if (dwStyleEx&WS_EX_RIGHT)
                              dwFlags |= DT_RIGHT;

                           if(dwStyle & SS_NOPREFIX)
                              dwFlags |= DT_NOPREFIX;

                           /* Draw the text! */
                           _DrawThemeTextEx(hTheme, hdcPaint, 0, 0,
                                            szText, -1, dwFlags, &rcClient, &DttOpts);
                        }
                     }
                  }

                  /* Cleanup */
                  if (hFontOld)
                     SelectObject(hdcPaint, hFontOld);
                  _EndBufferedPaint(hBufferedPaint, TRUE);
               }
               _CloseThemeData(hTheme);
            }
         }

         EndPaint(hwnd, &ps);
         return TRUE;
      }
      break;
   }

   if ( !pOldProc )
      return DefWindowProc(hwnd, msg, mp1, mp2);
   return CallWindowProc(pOldProc, hwnd, msg, mp1, mp2);
}
#endif

/* Function: _dw_buttonwndproc
 * Abstract: Subclass procedure for buttons
 */

LRESULT CALLBACK _dw_buttonwndproc(HWND hwnd, ULONG msg, WPARAM mp1, LPARAM mp2)
{
   ColorInfo *cinfo = _dw_window_get_cinfo(hwnd);
   WNDPROC pOldProc;
   int retval = -1;

   if ( !cinfo )
      return DefWindowProc(hwnd, msg, mp1, mp2);

   /* We must save a pointer to the old
    * window procedure because if a signal
    * handler attached here destroys this
    * window it will then be invalid.
    */
   pOldProc = cinfo->pOldProc;

   switch(msg)
   {
   case WM_CTLCOLORSTATIC:
   case WM_CTLCOLORLISTBOX:
   case WM_CTLCOLORBTN:
   case WM_CTLCOLOREDIT:
   case WM_CTLCOLORMSGBOX:
   case WM_CTLCOLORSCROLLBAR:
   case WM_CTLCOLORDLG:
      return _dw_colorwndproc(hwnd, msg, mp1, mp2);
   case WM_SETFOCUS:
      _dw_wndproc(hwnd, msg, mp1, mp2);
      break;
   case WM_LBUTTONUP:
      {
         DWSignalHandler *tmp = Root;

         /* Find any callbacks for this function */
         while(tmp)
         {
            if(tmp->message == WM_COMMAND)
            {
               int (DWSIGNAL *clickfunc)(HWND, void *) = tmp->signalfunction;

               /* Make sure it's the right window, and the right ID */
               if(tmp->window == hwnd)
               {
                  int checkbox = DW_POINTER_TO_INT(dw_window_get_data(hwnd, "_dw_checkbox"));

                  if(checkbox)
                     in_checkbox_handler = 1;

                  retval = clickfunc(tmp->window, tmp->data);

                  if(checkbox)
                     in_checkbox_handler = 0;
                  tmp = NULL;
               }
            }
            if(tmp)
               tmp= tmp->next;
         }
      }
      break;
   case WM_CHAR:
      {
         /* A button press should also occur for an ENTER or SPACE press
          * while the button has the active input focus.
          */
         if(LOWORD(mp1) == '\r' || LOWORD(mp1) == ' ')
         {
            DWSignalHandler *tmp = Root;

            /* Find any callbacks for this function */
            while(tmp)
            {
               if(tmp->message == WM_COMMAND)
               {
                  int (DWSIGNAL *clickfunc)(HWND, void *) = tmp->signalfunction;

                  /* Make sure it's the right window, and the right ID */
                  if(tmp->window == hwnd)
                  {
                     retval = clickfunc(tmp->window, tmp->data);
                     tmp = NULL;
                  }
               }
               if(tmp)
                  tmp= tmp->next;
            }
         }
         if(LOWORD(mp1) == '\t')
         {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
               _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
            else
               _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
            return FALSE;
         }
      }
      break;
   case WM_KEYDOWN:
      if(mp1 == VK_LEFT || mp1 == VK_UP)
         _dw_shift_focus(hwnd, _DW_DIRECTION_BACKWARD);
      if(mp1 == VK_RIGHT || mp1 == VK_DOWN)
         _dw_shift_focus(hwnd, _DW_DIRECTION_FORWARD);
      break;
   }

   /* Make sure windows are up-to-date */
   if(retval != -1)
      _dw_redraw(0, FALSE);
   if ( !pOldProc )
      return DefWindowProc(hwnd, msg, mp1, mp2);
   return CallWindowProc(pOldProc, hwnd, msg, mp1, mp2);
}

/* This function recalculates a notebook page for example
 * during switching of notebook pages.
 */
void _dw_resize_notebook_page(HWND handle, int pageid)
{
   RECT rect;
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");

   if(array && array[pageid])
   {
      Box *box = (Box *)GetWindowLongPtr(array[pageid]->hwnd, GWLP_USERDATA);

      GetClientRect(handle,&rect);
      TabCtrl_AdjustRect(handle,FALSE,&rect);
      MoveWindow(array[pageid]->hwnd, rect.left, rect.top,
               rect.right - rect.left, rect.bottom-rect.top, TRUE);
      if(box && box->count)
      {
         ShowWindow(box->items[0].hwnd, SW_HIDE);
         _dw_do_resize(box, rect.right - rect.left, rect.bottom - rect.top, 0, 0);
         ShowWindow(box->items[0].hwnd, SW_SHOW);
      }

      ShowWindow(array[pageid]->hwnd, SW_SHOWNORMAL);
   }
}

void _dw_create_tooltip(HWND handle, const char *text)
{
    TOOLINFO ti = { 0 };

    ti.cbSize = sizeof(TOOLINFO);
    ti.hwnd = handle;
    ti.hinst = _DWInstance;

    SendMessage(_dw_tooltip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    if(text)
    {
        /* Set up "tool" information.
         * In this case, the "tool" is the entire parent window.
         */
        ti.uFlags = TTF_SUBCLASS;
        ti.lpszText = UTF8toWide(text);
        ti.rect.right = ti.rect.bottom = 2000;

        /* Associate the tooltip with the "tool" window. */
        SendMessage(_dw_tooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    }
}

#ifndef GDIPLUS
/* This function determines the handle for a supplied image filename
 */
int _dw_get_image_handle(char *filename, HANDLE *icon, HBITMAP *hbitmap)
{
   int len, windowtype = 0;
   char *file = malloc(strlen(filename) + 5);

   *hbitmap = 0;
   *icon = 0;

   if (!file)
   {
      return 0;
   }

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if (access(file, 04) == 0)
   {
      len = strlen( file );
      if ( len < 4 )
      {
         free(file);
         return 0;
      }
      if ( stricmp( file + len - 4, ".ico" ) == 0 )
      {
         *icon = LoadImage(NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
         windowtype = BS_ICON;
      }
      else if ( stricmp( file + len - 4, ".bmp" ) == 0 )
      {
         *hbitmap = (HBITMAP)LoadImage(NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
         windowtype = BS_BITMAP;
      }
      else if ( stricmp( file + len - 4, ".png" ) == 0 )
      {
         *hbitmap = (HBITMAP)LoadImage(NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
         windowtype = BS_BITMAP;
      }
      free(file);
   }
   else
   {
      /* Try with .ico extension first...*/
      strcat(file, ".ico");
      if (access(file, 04) == 0)
      {
         *icon = LoadImage(NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
         windowtype = BS_ICON;
      }
      else
      {
         strcpy(file, filename);
         strcat(file, ".bmp");
         if (access(file, 04) == 0)
         {
            *hbitmap = (HBITMAP)LoadImage(NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            windowtype = BS_BITMAP;
         }
      }
      free(file);
   }
   return windowtype;
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
   WNDCLASS wc;
   int z;
   INITCOMMONCONTROLSEX icc;
   char *alttmpdir;
#ifdef GDIPLUS
   struct GdiplusStartupInput si;
#endif

   /* Setup the private data directory */
   if(argc > 0 && argv[0])
   {
      char *pos = strrchr(argv[0], '\\');

      /* Just to be safe try the unix style */
      if(!pos)
         pos = strrchr(argv[0], '/');

      if(pos)
      {
         strncpy(_dw_exec_dir, argv[0], (size_t)(pos - argv[0]));
         if((pos++) && !_dw_app_id[0])
         {
            /* If we have a binary name, use that for the Application ID instead. */
            _snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.%s", DW_APP_DOMAIN_DEFAULT, pos);
            strncpy(_dw_app_name, pos, _DW_APP_ID_SIZE);
         }
      }
   }
   /* If that failed... just get the current directory */
   if(!_dw_exec_dir[0])
      GetCurrentDirectoryA(MAX_PATH, _dw_exec_dir);

   /* Initialize our thread local storage */
   _dw_foreground = TlsAlloc();
   _dw_background = TlsAlloc();
   _dw_hpen = TlsAlloc();
   _dw_hbrush = TlsAlloc();
#ifdef GDIPLUS
   _dw_gppen = TlsAlloc();
   _dw_gpbrush = TlsAlloc();
#endif

   icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icc.dwICC = ICC_WIN95_CLASSES|ICC_DATE_CLASSES;

   InitCommonControlsEx(&icc);

   memset(_dw_icon_lookup, 0, sizeof(HICON) * ICON_INDEX_LIMIT);

   /* We need the version to check capability like up-down controls */
   _dwVersion = GetVersion();
   _dwComctlVer = _dw_get_dll_version(TEXT("comctl32.dll"));

   /* We need to initialize dark mode, and thus the aero/theme subsystems before registering our window classes */
   if((huxtheme = LoadLibrary(TEXT("uxtheme"))))
      _SetWindowTheme = (HRESULT (WINAPI *)(HWND, LPCWSTR, LPCWSTR ))GetProcAddress(huxtheme, "SetWindowTheme");
#ifdef AEROGLASS
   /* Attempt to load the Desktop Window Manager and Theme library */
   if(huxtheme && (hdwm = LoadLibrary(TEXT("dwmapi"))))
   {
      _DwmExtendFrameIntoClientArea = (HRESULT (WINAPI *)(HWND, const MARGINS *))GetProcAddress(hdwm, "DwmExtendFrameIntoClientArea");
      _DwmSetWindowAttribute = (HRESULT (WINAPI *)(HWND, DWORD, LPCVOID, DWORD))GetProcAddress(hdwm, "DwmSetWindowAttribute");
      _DwmDefWindowProc = (BOOL (WINAPI *)(HWND, UINT, WPARAM, LPARAM, LRESULT *))GetProcAddress(hdwm, "DwmDefWindowProc");
      if((_DwmIsCompositionEnabled = (HRESULT (WINAPI *)(BOOL *))GetProcAddress(hdwm, "DwmIsCompositionEnabled")))
         _DwmIsCompositionEnabled(&_dw_composition);
      _OpenThemeData = (HTHEME (WINAPI *)(HWND, LPCWSTR))GetProcAddress(huxtheme, "OpenThemeData");
      _GetThemeSysFont = (HRESULT (WINAPI *)(HTHEME, int, LOGFONTW *))GetProcAddress(huxtheme, "GetThemeSysFont");
      _GetImmersiveColorFromColorSetEx = (DWORD (WINAPI *)(UINT, UINT, BOOL, UINT))GetProcAddress(huxtheme, MAKEINTRESOURCEA(95));
      _GetImmersiveColorTypeFromName = (int (WINAPI *)(const WCHAR *))GetProcAddress(huxtheme, MAKEINTRESOURCEA(96));
      _GetImmersiveUserColorSetPreference = (int (WINAPI *)(BOOL, BOOL))GetProcAddress(huxtheme, MAKEINTRESOURCEA(98));
      _BeginBufferedPaint = (HPAINTBUFFER (WINAPI *)(HDC, const RECT *, BP_BUFFERFORMAT, BP_PAINTPARAMS *, HDC *))GetProcAddress(huxtheme, "BeginBufferedPaint");
      _BufferedPaintSetAlpha = (HRESULT (WINAPI *)(HPAINTBUFFER, const RECT *, BYTE))GetProcAddress(huxtheme, "BufferedPaintSetAlpha");
      _DrawThemeTextEx = (HRESULT (WINAPI *)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, LPRECT, const DTTOPTS *))GetProcAddress(huxtheme, "DrawThemeTextEx");
      _EndBufferedPaint = (HRESULT (WINAPI *)(HPAINTBUFFER, BOOL))GetProcAddress(huxtheme, "EndBufferedPaint");
      _CloseThemeData = (HRESULT (WINAPI *)(HTHEME))GetProcAddress(huxtheme, "CloseThemeData");
      _dw_init_dark_mode();
   }
   /* In case of error close the library if needed */
   else if(hdwm)
   {
      FreeLibrary(hdwm);
      hdwm = 0;
   }
#endif

   /* Register the generic Dynamic Windows class */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_DBLCLKS;
   wc.lpfnWndProc = (WNDPROC)_dw_wndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 32;
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = ClassName;

   RegisterClass(&wc);

   /* Register the splitbar control */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = (WNDPROC)_dw_splitwndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = SplitbarClassName;

   RegisterClass(&wc);

   /* Register the scroller control */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_DBLCLKS;
   wc.lpfnWndProc = (WNDPROC)_dw_scrollwndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 32;
   wc.hbrBackground = NULL;
   wc.lpszMenuName = NULL;
   wc.lpszClassName = ScrollClassName;

   RegisterClass(&wc);

   /* Register a frame control like on OS/2 */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_DBLCLKS;
   wc.lpfnWndProc = (WNDPROC)_dw_framewndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 32;
   wc.hbrBackground = (HBRUSH)_dw_get_syscolor_brush(COLOR_3DFACE);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = FRAMECLASSNAME;

   RegisterClass(&wc);

   /* Register a status bar control */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = CS_DBLCLKS;
   wc.lpfnWndProc = (WNDPROC)_dw_statuswndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 32;
   wc.hbrBackground = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = StatusbarClassName;

   RegisterClass(&wc);

   /* Create a set of brushes using the default OS/2 and DOS colors */
   for(z=0;z<18;z++)
      _dw_colors[z] = CreateSolidBrush(RGB(_dw_red[z],_dw_green[z],_dw_blue[z]));

   /* Register an Object Windows class like OS/2 and Win2k+
    * so similar functionality can be used on earlier releases
    * of Windows.
    */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.style = 0;
   wc.lpfnWndProc = (WNDPROC)_dw_wndproc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hbrBackground = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = ObjectClassName;

   RegisterClass(&wc);

   /* Since Windows 95/98/NT don't have a HWND_OBJECT class
    * also known as a input only window, I will create a
    * temporary window that isn't visible and really does nothing
    * except temporarily hold the child windows before they are
    * packed into their correct parent.
    */

   DW_HWND_OBJECT = CreateWindow(ObjectClassName, TEXT("HWND_OBJECT"), 0, 0, 0,
                          0, 0, HWND_DESKTOP, NULL, _DWInstance, NULL);

   if(!DW_HWND_OBJECT)
   {
      dw_messagebox("Dynamic Windows", DW_MB_OK|DW_MB_ERROR, "Could not initialize the object window. error code %d", GetLastError());
      exit(1);
   }

   if(!_dw_app_id[0])
   {
      /* Generate an Application ID based on the PID if all else fails. */
      _snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, _getpid());
   }
   if(!_dw_app_name[0])
   {
      /* If we still don't have an app name, get the executable name */
      char fullpath[MAX_PATH+1] = {0}, *pos;
      GetModuleFileNameA(_DWInstance, fullpath, MAX_PATH);
      pos = strrchr(fullpath, '\\');
      if(pos)
         pos++;
      strncpy(_dw_app_name, pos ? pos : fullpath, _DW_APP_ID_SIZE);
   }
    
#ifdef BUILD_HTML
   /* Register HTML renderer class */
   memset(&wc, 0, sizeof(WNDCLASS));
   wc.lpszClassName = BrowserClassName;
   wc.style = CS_HREDRAW | CS_VREDRAW;
#ifdef BUILD_EDGE
   /* Check if Microsoft Edge (Chromium) is installed */
   if ((_DW_EDGE_DETECTED = _dw_edge_detect(UTF8toWide(_dw_app_id))))
   {
      wc.lpfnWndProc = (WNDPROC)_dw_edgewndproc;
      wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   }
   else
#endif
   {
      wc.lpfnWndProc = (WNDPROC)_dw_browserwndproc;
   }
   RegisterClass(&wc);
#endif

   /* Create a tooltip. */
   _dw_tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, DW_HWND_OBJECT, NULL, _DWInstance, NULL);

   SetWindowPos(_dw_tooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   /* Create empty box data */
   SetWindowLongPtr(DW_HWND_OBJECT, GWLP_USERDATA, (LONG_PTR)calloc(sizeof(Box), 1));

   /* Initialize Security for named events and memory */
   InitializeSecurityDescriptor(&_dwsd, SECURITY_DESCRIPTOR_REVISION);
   SetSecurityDescriptorDacl(&_dwsd, TRUE, (PACL) NULL, FALSE);

   OleInitialize(NULL);

   /*
    * Get an alternate temporary directory in case TMP doesn't exist
    */
   if ( (alttmpdir = getenv( "TEMP" ) ) == NULL )
   {
      strcpy( _dw_alternate_temp_dir, "c:\\tmp" );
   }
   else
   {
      strncpy( _dw_alternate_temp_dir, alttmpdir, MAX_PATH );
   }

#ifdef GDIPLUS
   /* Setup GDI+ */
   si.GdiplusVersion = 1;
   si.DebugEventCallback = NULL;
   si.SuppressBackgroundThread = FALSE;
   si.SuppressExternalCodecs = FALSE;
   GdiplusStartup(&gdiplusToken, &si, NULL);
#endif

   /* GDI+ Needs to be initialized before calling _dw_init_thread(); */
   _dw_init_thread();

#ifdef RICHEDIT
   /* Attempt to load rich edit library: 4.1, 3/2.0 and 1.0 */
   if(!(hmsftedit = LoadLibrary(TEXT("msftedit"))))
   {
      if(!(hrichedit = LoadLibrary(TEXT("riched20"))))
         hrichedit = LoadLibrary(TEXT("riched32"));
   }
   /* Enable Rich Edit by default if supported */
   if(_DW_MLE_RICH_EDIT == DW_FEATURE_UNSUPPORTED && (hmsftedit || hrichedit))
      _DW_MLE_RICH_EDIT = DW_FEATURE_ENABLED;
#endif
#ifdef BUILD_TOAST
   _dw_toast_init(UTF8toWide(_dw_app_name), UTF8toWide(_dw_app_id));
#endif
   return 0;
}

static int _dw_main_running = FALSE;

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
   MSG msg;

   _dwtid = dw_thread_id();
   /* Make sure any queued redraws are handled */
   _dw_redraw(0, FALSE);

   /* Set the running flag to TRUE */
   _dw_main_running = TRUE;

   /* Run the loop until the flag is unset... or error */
   while(_dw_main_running && GetMessage(&msg, NULL, 0, 0))
   {
      if(msg.hwnd == NULL && msg.message == WM_TIMER)
         _dw_wndproc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
      else
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
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
   MSG msg;
   double start = (double)clock();

   while(((clock() - start) / (CLOCKS_PER_SEC/1000)) <= milliseconds)
   {
      if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
      {
         GetMessage(&msg, NULL, 0, 0);
         if(msg.hwnd == NULL && msg.message == WM_TIMER)
            _dw_wndproc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
         else
         {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }
      else
         Sleep(1);
   }
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
   MSG msg;

   _dwtid = dw_thread_id();

   if(GetMessage(&msg, NULL, 0, 0))
   {
      if(msg.hwnd == NULL && msg.message == WM_TIMER)
         _dw_wndproc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
      else
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
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
   MSG msg;
   void *tmp;

   if(!dialog)
      return NULL;

   while (GetMessage(&msg,NULL,0,0))
   {
      if(msg.hwnd == NULL && msg.message == WM_TIMER)
         _dw_wndproc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
      else
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
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
   dw_vdebug(format, args);
   va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
   char outbuf[1025] = {0}, *thisbuf = outbuf;

   vsnprintf(outbuf, 1024, format, args);
   OutputDebugString(UTF8toWide(thisbuf));
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
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
   char outbuf[1025] = { 0 }, *thisbuf = outbuf;
   int rc;

   vsnprintf(outbuf, 1024, format, args);

   rc = MessageBox(HWND_DESKTOP, UTF8toWide(thisbuf), UTF8toWide(title), flags);
   if(rc == IDOK)
      return DW_MB_RETURN_OK;
   else if(rc == IDYES)
      return DW_MB_RETURN_YES;
   else if(rc == IDNO)
      return DW_MB_RETURN_NO;
   else if(rc == IDCANCEL)
      return DW_MB_RETURN_CANCEL;
   else return 0;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int API dw_window_minimize(HWND handle)
{
   return ShowWindow(handle, SW_MINIMIZE);
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
   return SetWindowPos(handle, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
   return SetWindowPos(handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
   int rc;
   RECT rect;

#ifdef AEROGLASS
   if(_DW_DARK_MODE_SUPPORTED)
   {
      /* Try to enable dark mode support if our OS supports it */
      _dw_set_child_window_theme(handle, 0);
      EnumChildWindows(handle, _dw_set_child_window_theme, 0);
      if(GetParent(handle) == HWND_DESKTOP)
         _dw_refresh_titlebar_theme_color(handle);
   }
#endif

   GetClientRect(handle, &rect);

   /* If the client area is 0x0 then call the autosize routine */
   if((rect.bottom - rect.top) == 0 || (rect.right - rect.left) == 0)
      dw_window_set_size(handle, 0, 0);

   rc = ShowWindow(handle, SW_SHOW);
   SetFocus(handle);
   _dw_initial_focus(handle);
   return rc;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_hide(HWND handle)
{
   return ShowWindow(handle, SW_HIDE);
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int API dw_window_destroy(HWND handle)
{
   HWND parent;
   HMENU menu;

   /* Handle special case for menu handle */
   if(handle < (HWND)65536)
   {
      char buffer[31] = {0};
      ULONG id = (ULONG)(uintptr_t)handle;

      _snprintf(buffer, 30, "_dw_id%ld", id);
      menu = (HMENU)dw_window_get_data(DW_HWND_OBJECT, buffer);

      if(menu && IsMenu(menu))
         return dw_menu_delete_item((HMENUI)menu, id);
      return DW_ERROR_UNKNOWN;
   }

   parent = GetParent(handle);
   menu = GetMenu(handle);

   if(menu)
      _dw_free_menu_data(menu);

   /* If it is a desktop window let WM_DESTROY handle it */
   if(parent != HWND_DESKTOP)
   {
      dw_box_unpack(handle);
      _dw_free_window_memory(handle, 0);
      EnumChildWindows(handle, _dw_free_window_memory, 0);
   }
   return DestroyWindow(handle);
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
   Box *mybox = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);

   if(mybox)
   {
      RECT rect;
      int istoplevel = (GetParent(handle) == HWND_DESKTOP);

      GetClientRect(handle, &rect);

      ShowWindow(istoplevel ? mybox->items[0].hwnd : handle, SW_HIDE);
      _dw_do_resize(mybox, rect.right - rect.left, rect.bottom - rect.top, 0, 0);
      ShowWindow(istoplevel ? mybox->items[0].hwnd : handle, SW_SHOW);
   }
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
   SetParent(handle, newparent);
}

LOGFONT _get_logfont(HDC hdc, const char *fontname)
{
   char  *Italic, *Bold, *myFontName = strchr(fontname, '.');
   int size = atoi(fontname);
   LOGFONT lf = {0};

   /* If we found a '.' use the location after the . */
   if(myFontName)
       myFontName = _strdup(++myFontName);
   else /* Otherwise use the whole fontname and default size of 9 */
       myFontName = _strdup(fontname);

   lf.lfHeight = -MulDiv(size ? size : 9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
   Italic = strstr(myFontName, " Italic");
   Bold = strstr(myFontName, " Bold");
   lf.lfWidth = 0;
   lf.lfEscapement = 0;
   lf.lfOrientation = 0;
   lf.lfItalic = Italic ? TRUE : FALSE;
   lf.lfUnderline = 0;
   lf.lfStrikeOut = 0;
   lf.lfWeight = Bold ? FW_BOLD : FW_NORMAL;
   lf.lfCharSet = DEFAULT_CHARSET;
   lf.lfOutPrecision = 0;
   lf.lfClipPrecision = 0;
   lf.lfQuality = DEFAULT_QUALITY;
   lf.lfPitchAndFamily = DEFAULT_PITCH | FW_DONTCARE;
   if(Italic)
      *Italic = 0;
   if(Bold)
      *Bold = 0;
   _tcsncpy(lf.lfFaceName, UTF8toWide(myFontName), (sizeof(lf.lfFaceName)/sizeof(TCHAR))-1);
   free(myFontName);
   return lf;
}

/* Create a duplicate of an existing font handle
 * that is safe to call DeleteObject() on.
 */
HFONT _dw_dup_font_handle(HFONT hfont)
{
    LOGFONT lf = {0};
    return GetObject(hfont, sizeof(lf), &lf) ? CreateFontIndirect(&lf) : NULL;
}

/* Create a font handle from specified font name..
 * or return a handle to the default font.
 */
HFONT _dw_acquire_font2(HDC hdc, const char *fontname)
{
   HFONT hfont = 0;

   if(fontname != DefaultFont && fontname[0])
   {
      LOGFONT lf = _get_logfont(hdc, fontname);
      hfont = CreateFontIndirect(&lf);
   }
   if(!hfont && _DWDefaultFont)
      hfont = _dw_dup_font_handle(_DWDefaultFont);
   if(!hfont)
      hfont = GetStockObject(DEFAULT_GUI_FONT);
   return hfont;
}

/* Create a font handle from specified font name..
 * or return a handle to the default font.
 */
HFONT _dw_acquire_font(HWND handle, const char *fontname)
{
   HDC hdc = GetDC(handle);
   HFONT hfont = _dw_acquire_font2(hdc, fontname);
   ReleaseDC(handle, hdc);
   return hfont;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
    HFONT oldfont = _DWDefaultFont;

    _DWDefaultFont = _dw_acquire_font(HWND_DESKTOP, fontname);
    if(oldfont)
    {
        DeleteObject(oldfont);
    }
}

/* Internal function to return a pointer to an item struct
 * with information about the packing information regarding object.
 */
Item *_dw_box_item(HWND handle)
{
   HWND parent = GetParent(handle);
   Box *thisbox = (Box *)GetWindowLongPtr(parent, GWLP_USERDATA);

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
   TCHAR tmpbuf[100] = {0};
   static char testtext[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
   HBITMAP hbm = 0;
   HICON hic = 0;
   ICONINFO ii = {0};

   GetClassName(handle, tmpbuf, 99);

   /* Attempt to get icon from classes that can have them */
   if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1) == 0)
      hic = (HICON)SendMessage(handle, STM_GETIMAGE, IMAGE_ICON, 0);
   else if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1) == 0)
      hic = (HICON)SendMessage(handle, BM_GETIMAGE, IMAGE_ICON, 0);

   /* If we got an icon, pull out the internal bitmap */
   if(hic)
   {
      if(GetIconInfo(hic, &ii))
         hbm = ii.hbmMask ? ii.hbmMask : ii.hbmColor;
   }

   /* If we weren't able to get the bitmap from the icon... */
   if(!hbm)
   {
       /* Attempt to get bitmap from classes that can have them */
      if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1) == 0)
         hbm = (HBITMAP)SendMessage(handle, STM_GETIMAGE, IMAGE_BITMAP, 0);
      else if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1) == 0)
         hbm = (HBITMAP)SendMessage(handle, BM_GETIMAGE, IMAGE_BITMAP, 0);
   }

   /* If we got an image... set the sizes appropriately */
   if(hbm)
   {
      BITMAP bmi = { 0 };

      GetObject(hbm, sizeof(BITMAP), &bmi);
      thiswidth = bmi.bmWidth;
      thisheight = bmi.bmHeight;
   }
   else
   {
      char *buf = dw_window_get_text(handle);

      /* If we have a string...
       * calculate the size with the current font.
       */
      if(buf)
      {
         if(*buf)
            dw_font_text_extents_get(handle, NULL, buf, &thiswidth, &thisheight);
         else if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1) == 0 ||
                 _tcsnicmp(tmpbuf, StatusbarClassName, _tcslen(StatusbarClassName)+1) == 0)
            dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
         dw_free(buf);
      }
   }

   /* Combobox */
   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1) == 0)
   {
      dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
      thiswidth = 150;
      extraheight = 4;
      if(thisheight < 18)
        thisheight = 18;
   }
   /* Ranged: Percent, Slider, Scrollbar */
   else if(_tcsnicmp(tmpbuf, PROGRESS_CLASS, _tcslen(PROGRESS_CLASS)+1) == 0 ||
           _tcsnicmp(tmpbuf, TRACKBAR_CLASS, _tcslen(TRACKBAR_CLASS)+1) == 0 ||
           _tcsnicmp(tmpbuf, SCROLLBARCLASSNAME, _tcslen(SCROLLBARCLASSNAME)+1) == 0)
   {
      if(_tcsnicmp(tmpbuf, SCROLLBARCLASSNAME, _tcslen(SCROLLBARCLASSNAME)+1) == 0 &&
        GetWindowLong(handle, GWL_STYLE) & SBS_VERT)
      {
         /* Vertical */
         thiswidth = GetSystemMetrics(SM_CXVSCROLL);
         thisheight = 100;
      }
      else
      {
         /* Horizontal */
         thiswidth = 100;
         thisheight = GetSystemMetrics(SM_CYHSCROLL);
      }
   }
   /* Spinbuttons */
   else if(_tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1) == 0)
   {
      dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
      thiswidth = 50;
      extraheight = 6;
   }
#ifdef TOOLBAR
   /* Bitmap Buttons */
   else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
   {
      HIMAGELIST imlist = (HIMAGELIST)SendMessage(handle, TB_GETIMAGELIST, 0, 0);
      int minsize = 24;

      if(imlist)
         ImageList_GetIconSize(imlist, &thiswidth, &thisheight);

      /* If we are flat the size can be smaller */
      if(GetWindowLong(handle, GWL_STYLE) & TBSTYLE_FLAT)
         minsize = 20;
      else
      {
         thiswidth += 4;
         thisheight += 4;
      }

      if(thiswidth < minsize)
         thiswidth = minsize;
      if(thisheight < minsize)
         thisheight = minsize;
   }
#endif
   /* Listbox */
   else if(_tcsnicmp(tmpbuf, LISTBOXCLASSNAME, _tcslen(LISTBOXCLASSNAME)+1) == 0)
   {
      char buf[1025] = {0};
      int x, count = dw_listbox_count(handle);
      int basicwidth = thiswidth = GetSystemMetrics(SM_CXVSCROLL) + 8;

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
   /* Entryfields and MLE */
   else if(_tcsnicmp(tmpbuf, EDITCLASSNAME, _tcslen(EDITCLASSNAME)+1) == 0
#ifdef RICHEDIT
        || _tcsnicmp(tmpbuf, RICHEDIT_CLASS, _tcslen(RICHEDIT_CLASS)+1) == 0
        || _tcsnicmp(tmpbuf, MSFTEDIT_CLASS, _tcslen(MSFTEDIT_CLASS)+1) == 0
#endif
           )
   {
      LONG style = GetWindowLong(handle, GWL_STYLE);
      if((style & ES_MULTILINE))
      {
         unsigned long bytes;
         int height, width;
         char *buf, *ptr;
         int basicwidth;

         if(style & ES_AUTOHSCROLL)
            thisheight = GetSystemMetrics(SM_CYHSCROLL) + 8;
         else
            thisheight = 8;
         basicwidth = thiswidth = GetSystemMetrics(SM_CXVSCROLL) + 8;

         dw_mle_get_size(handle, &bytes, NULL);

         ptr = buf = _alloca(bytes + 2);
         dw_mle_export(handle, buf, 0, (int)bytes);
         buf[bytes] = 0;
         strcat(buf, "\n");

         /* MLE */
         while((ptr = strstr(buf, "\n")))
         {
            ptr[0] = 0;
            width = 0;
            if(strlen(buf))
               dw_font_text_extents_get(handle, NULL, buf, &width, &height);
            else
               dw_font_text_extents_get(handle, NULL, testtext, NULL, &height);

            width += basicwidth;

            if(!(style & ES_AUTOHSCROLL) && width > _DW_SCROLLED_MAX_WIDTH)
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
      else
      {
         /* Entryfield */
         dw_font_text_extents_get(handle, NULL, testtext, NULL, &thisheight);
         thiswidth = 150;
         extraheight = 6;
      }
   }
   /* Container */
   else if(_tcsnicmp(tmpbuf, WC_LISTVIEW, _tcslen(WC_LISTVIEW)+1)== 0)
   {
      DWORD result = ListView_ApproximateViewRect(handle, _DW_SCROLLED_MAX_WIDTH, _DW_SCROLLED_MAX_HEIGHT, -1);

      thiswidth = LOWORD(result);
      thisheight = HIWORD(result);

      if(thiswidth > _DW_SCROLLED_MAX_WIDTH)
         thiswidth = _DW_SCROLLED_MAX_WIDTH;
      if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
         thiswidth = _DW_SCROLLED_MIN_WIDTH;
      if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
         thisheight = _DW_SCROLLED_MIN_HEIGHT;
      if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
         thisheight = _DW_SCROLLED_MAX_HEIGHT;
   }
   /* Tree */
   else if(_tcsnicmp(tmpbuf, WC_TREEVIEW, _tcslen(WC_TREEVIEW)+1)== 0)
   {
      thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
      thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
   }
   /* Buttons */
   else if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1) == 0)
   {
      ULONG style = GetWindowLong(handle, GWL_STYLE);

      /* Bitmap buttons */
      if(hbm)
      {
         extrawidth = 5;
         extraheight = 5;
      }
      /* Checkbox or radio button */
      else if(style & BS_AUTOCHECKBOX || style & BS_AUTORADIOBUTTON)
      {
         extrawidth = 24;
         extraheight = 4;
      }
      /* Text buttons */
      else
      {
         extrawidth = 8;
         extraheight = 8;
      }
   }
   else if(_tcsnicmp(tmpbuf, StatusbarClassName, _tcslen(StatusbarClassName)+1) == 0)
   {
      extrawidth = 4;
      extraheight = 2;
   }

   /* Set the requested sizes */
   if(width)
      *width = thiswidth + extrawidth;
   if(height)
      *height = thisheight + extraheight;

   /* Free temporary bitmaps */
   if(ii.hbmColor)
      DeleteObject(ii.hbmColor);
   if(ii.hbmMask)
      DeleteObject(ii.hbmMask);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, const char *fontname)
{
    HFONT hfont = NULL, oldfont = NULL;
    ColorInfo *cinfo = _dw_window_get_cinfo(handle);
    TCHAR tmpbuf[100] = {0};

    GetClassName(handle, tmpbuf, 99);
    if ( _tcsnicmp( tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1) == 0 )
    {
        /* groupbox */
        Box *thisbox = (Box *)GetWindowLongPtr( handle, GWLP_USERDATA );
        if ( thisbox && thisbox->grouphwnd != (HWND)NULL )
        {
            handle = thisbox->grouphwnd;
        }
    }
#ifdef RICHEDIT
    if (_tcsnicmp(tmpbuf, RICHEDIT_CLASS, _tcslen(RICHEDIT_CLASS)+1) == 0 ||
        _tcsnicmp(tmpbuf, MSFTEDIT_CLASS, _tcslen(MSFTEDIT_CLASS)+1) == 0)
    {
       if(fontname)
       {
            CHARFORMAT2 cf = {0};
            char  *Italic, *Bold, *myFontName = strchr(fontname, '.');
            int size = atoi(fontname);

            /* If we found a '.' use the location after the . */
            if(myFontName)
               myFontName = _strdup(++myFontName);
            else /* Otherwise use the whole fontname and default size of 9 */
               myFontName = _strdup(fontname);

            Italic = strstr(myFontName, " Italic");
            Bold = strstr(myFontName, " Bold");
            if(Italic)
                *Italic = 0;
            if(Bold)
                *Bold = 0;

            cf.cbSize = sizeof(cf);
            SendMessage(handle, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
            cf.dwMask = CFM_FACE | CFM_SIZE | CFM_ITALIC | CFM_BOLD;
            cf.dwEffects = (Italic ? CFE_ITALIC : 0) | (Bold ? CFE_BOLD : 0);
            _tcsncpy(cf.szFaceName, UTF8toWide(myFontName), (sizeof(cf.szFaceName)/sizeof(TCHAR))-1);
            /* yHeight is apparently the point size * 20, defaulting to 9 (180) */
            cf.yHeight = (size > 0) ? (size * 20) : 180;
            SendMessage(handle, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
       }
       else
       {
           /* On testing what the default is, the szFaceName is empty,
            * yHeight is 180 (9 point). So use that to reset to default.
            */
           CHARFORMAT2 cf = {0};
           cf.cbSize = sizeof(cf);
           cf.yHeight = 180;
           cf.dwMask = CFM_FACE | CFM_SIZE | CFM_ITALIC | CFM_BOLD;
           SendMessage(handle, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
       }
    }
    else
#endif
    {
        /* This needs to be after we get the correct handle */
        oldfont = (HFONT)SendMessage(handle, WM_GETFONT, 0, 0);
        hfont = _dw_acquire_font(handle, fontname);
    }

    if(fontname)
    {
        if(cinfo || (cinfo = _dw_window_new_cinfo(handle, TRUE)))
        {
            strncpy(cinfo->fontname, fontname, 127);
            if(hfont)
            {
                if(!oldfont)
                    oldfont = cinfo->hfont;
                cinfo->hfont = hfont;
            }
        }
    }
    /* If we changed the font... */
    if(hfont)
    {
       Item *item = _dw_box_item(handle);

       SendMessage(handle, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);
       if(oldfont)
          DeleteObject(oldfont);

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

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
   CHOOSEFONT cf = { 0 };
   LOGFONT lf = { 0 };
   char *str = NULL;
   char *bold = "";
   char *italic = "";

   if(currfont && *currfont)
      lf = _get_logfont(NULL, currfont);

   cf.lStructSize = sizeof(cf);
   cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
   cf.lpLogFont = &lf;

   if(ChooseFont(&cf))
   {
      str = (char *)calloc( 101, 1 );
      if ( str )
      {
         int height;
         HDC hdc = GetDC(NULL);

         if ( lf.lfWeight > FW_MEDIUM )
            bold = " Bold";
         if ( lf.lfItalic )
            italic = " Italic";
         height = MulDiv(abs(lf.lfHeight), 72,  GetDeviceCaps (hdc, LOGPIXELSY));
         ReleaseDC(NULL, hdc);
         _snprintf( str, 100, "%d.%s%s%s", height, WideToUTF8(lf.lfFaceName), bold, italic );
      }
   }
   return str;
}

/*
 * Gets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
char * API dw_window_get_font(HWND handle)
{
   HFONT oldfont = NULL;
   char *str = NULL;
   char *bold = "";
   char *italic = "";
   LOGFONT lf = { 0 };
   Box *thisbox;
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);
   if ( _tcsnicmp( tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1) == 0 )
   {
      /* groupbox */
      thisbox = (Box *)GetWindowLongPtr( handle, GWLP_USERDATA );
      if ( thisbox && thisbox->grouphwnd != (HWND)NULL )
      {
         handle = thisbox->grouphwnd;
      }
   }
   oldfont = (HFONT)SendMessage(handle, WM_GETFONT, 0, 0);
   if ( GetObject( oldfont, sizeof(lf), &lf ) )
   {
      str = (char *)calloc( 100, 1 );
      if ( str )
      {
         int height;
         HDC hdc = GetDC(handle);

         if ( lf.lfWeight > FW_MEDIUM )
            bold = " Bold";
         if ( lf.lfItalic )
            italic = " Italic";
         height = MulDiv(abs(lf.lfHeight), 72,  GetDeviceCaps (hdc, LOGPIXELSY));
         ReleaseDC(handle, hdc);
         _snprintf( str, 100, "%d.%s%s%s", height, WideToUTF8(lf.lfFaceName), bold, italic );
      }
   }
   if ( oldfont )
      DeleteObject( oldfont );
   return str;
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in RGB format.
 *          back: Background color in RGB format.
 */
int API dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
   ColorInfo *cinfo = _dw_window_get_cinfo(handle);
   Box *thisbox;
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, WC_LISTVIEW, _tcslen(WC_LISTVIEW))==0)
   {
      cinfo->fore = fore = _dw_internal_color(fore);
      cinfo->back = back = _dw_internal_color(back);

      ListView_SetTextColor(handle, RGB(DW_RED_VALUE(fore),
                                DW_GREEN_VALUE(fore),
                                DW_BLUE_VALUE(fore)));
      ListView_SetTextBkColor(handle, RGB(DW_RED_VALUE(back),
                                 DW_GREEN_VALUE(back),
                                 DW_BLUE_VALUE(back)));
      ListView_SetBkColor(handle, RGB(DW_RED_VALUE(back),
                              DW_GREEN_VALUE(back),
                              DW_BLUE_VALUE(back)));
      InvalidateRgn(handle, NULL, TRUE);
      return TRUE;
   }
   else if ( _tcsnicmp( tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)) == 0 )
   {
      /* groupbox */
      thisbox = (Box *)GetWindowLongPtr( handle, GWLP_USERDATA );
      if ( thisbox && thisbox->grouphwnd != (HWND)NULL )
      {
         thisbox->cinfo.fore = fore;
         thisbox->cinfo.back = back;
      }
   }
#ifdef RICHEDIT
   else if (_tcsnicmp(tmpbuf, RICHEDIT_CLASS, _tcslen(RICHEDIT_CLASS)+1) == 0 ||
            _tcsnicmp(tmpbuf, MSFTEDIT_CLASS, _tcslen(MSFTEDIT_CLASS)+1) == 0)
   {
        ULONG _fore = _dw_internal_color(fore);
        ULONG _back = _dw_internal_color(back);
        CHARFORMAT2 cf;

        SendMessage(handle, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.dwEffects = 0;
        cf.crTextColor = RGB(DW_RED_VALUE(_fore), DW_GREEN_VALUE(_fore), DW_BLUE_VALUE(_fore)); 
        SendMessage(handle, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

        SendMessage(handle, EM_SETBKGNDCOLOR, (back == DW_CLR_DEFAULT || back == DW_RGB_TRANSPARENT) ? 1 : 0, 
                    RGB(DW_RED_VALUE(_back), DW_GREEN_VALUE(_back), DW_BLUE_VALUE(_back)));
   }
#endif

   if(cinfo || (cinfo = _dw_window_new_cinfo(handle, TRUE)))
   {
      cinfo->fore = fore;
      cinfo->back = back;
   }
   InvalidateRgn(handle, NULL, TRUE);
   return TRUE;
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
   SetCapture(handle);
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
   ReleaseCapture();
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
   HCURSOR cursor = pointertype < 65536 ? LoadCursor(NULL, MAKEINTRESOURCE(pointertype)) : (HCURSOR)(intptr_t)pointertype;

   if(!pointertype)
      dw_window_set_data(handle, "_dw_cursor", 0);
    else
   {
      dw_window_set_data(handle, "_dw_cursor", DW_POINTER(cursor));
      SetCursor(cursor);
   }
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the DW reference.
 */
HWND API dw_window_new(HWND hwndOwner, const char *title, ULONG flStyle)
{
   HWND hwndframe;
   Box *newbox = calloc(sizeof(Box), 1);
   ULONG flStyleEx = 0;
#ifdef AEROGLASS
   flStyleEx = WS_EX_LAYERED;
#endif

   newbox->type = DW_VERT;
   newbox->vsize = newbox->hsize = _DW_SIZE_EXPAND;
   newbox->cinfo.fore = newbox->cinfo.back = -1;
   newbox->cinfo.style = flStyle;

   if(!(flStyle & WS_CAPTION))
      flStyle |= WS_POPUPWINDOW;

   if(flStyle & DW_FCF_TASKLIST ||
      flStyle & WS_VSCROLL /* This is deprecated and should go away in version 3? */)
   {
      hwndframe = CreateWindowEx(flStyleEx, ClassName, UTF8toWide(title), (flStyle | WS_CLIPCHILDREN) & 0xffdf0000, CW_USEDEFAULT, CW_USEDEFAULT,
                           0, 0, hwndOwner, NULL, _DWInstance, NULL);
   }
   else
   {
      flStyleEx |= WS_EX_TOOLWINDOW;

      hwndframe = CreateWindowEx(flStyleEx, ClassName, UTF8toWide(title), (flStyle | WS_CLIPCHILDREN) & 0xffff0000, CW_USEDEFAULT, CW_USEDEFAULT,
                           0, 0, hwndOwner, NULL, _DWInstance, NULL);
   }

   SetWindowLongPtr(hwndframe, GWLP_USERDATA, (LONG_PTR)newbox);

   if(hwndOwner)
      SetParent(hwndframe, hwndOwner);

#ifdef AEROGLASS
   /* Determine the borders of the default window frame */
   AdjustWindowRectEx(&(newbox->cinfo.rect), flStyle, FALSE, flStyleEx);
#ifdef DEBUG
   dw_debug("AdjustWindowRectExt(%d,%d,%d,%d)\n", newbox->cinfo.rect.left, newbox->cinfo.rect.right,
                                                  newbox->cinfo.rect.top, newbox->cinfo.rect.bottom);
#endif
   newbox->cinfo.rect.top *= -1;
   newbox->cinfo.rect.left = newbox->cinfo.rect.right = newbox->cinfo.rect.bottom = 1;

   if((flStyle & DW_FCF_COMPOSITED) && !IS_WIN8PLUS)
   {
      /* Attempt to enable Aero glass background on the entire window */
      if(_DwmExtendFrameIntoClientArea && _dw_composition)
      {
         MARGINS fullmar = {-1,-1,-1,-1};

         _DwmExtendFrameIntoClientArea(hwndframe, &fullmar);
         _dw_show_margins(hwndframe, fullmar, __LINE__);
      }
      SetLayeredWindowAttributes(hwndframe, _dw_transparencykey, 0, LWA_COLORKEY);
   }
   else
   {
      if(_DwmExtendFrameIntoClientArea && _dw_composition)
      {
         MARGINS mar = {0};

#ifdef DARK_MODE_TITLEBAR_MENU
         mar = _dw_rect_to_margins(newbox->cinfo.rect);
#endif

         _DwmExtendFrameIntoClientArea(hwndframe, &mar);
         _dw_show_margins(hwndframe, mar, __LINE__);
      }
      SetLayeredWindowAttributes(hwndframe, _dw_transparencykey, 255, LWA_ALPHA);
   }
#endif

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
   Box *newbox = calloc(sizeof(Box), 1);
   HWND hwndframe;

   newbox->pad = pad;
   newbox->type = type;
   newbox->count = 0;
   newbox->grouphwnd = (HWND)NULL;
   newbox->cinfo.fore = newbox->cinfo.back = -1;

   hwndframe = CreateWindow(FRAMECLASSNAME,
                      NULL,
                      WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                      0,0,0,0,
                      DW_HWND_OBJECT,
                      NULL,
                      _DWInstance,
                      NULL);

   newbox->cinfo.pOldProc = SubclassWindow(hwndframe, _dw_colorwndproc);
   newbox->cinfo.fore = newbox->cinfo.back = -1;

   SetWindowLongPtr(hwndframe, GWLP_USERDATA, (LONG_PTR)newbox);
   return hwndframe;
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
   HWND tmpbox = dw_box_new(DW_VERT, 0);
   ColorInfo *cinfo;

   dw_box_pack_start(tmpbox, box, 1, 1, TRUE, TRUE, 0);

   hwndframe = CreateWindow(ScrollClassName,
                     NULL,
                     WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     NULL,
                     _DWInstance,
                     NULL);

   if((cinfo = _dw_window_new_cinfo(hwndframe, FALSE)))
   {
      cinfo->buddy = box;
      cinfo->combo = tmpbox;
   }
   SetParent(tmpbox, hwndframe);
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
    SCROLLINFO si;
    int bar = SB_HORZ;

    if(orient == DW_VERT)
    {
        bar = SB_VERT;
    }

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;

    /* Get the current scroll positions */
    if(!GetScrollInfo(handle, bar, &si))
    {
        return -1;
    }
    return si.nPos;
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_range( HWND handle, int orient )
{
    SCROLLINFO si;
    int bar = SB_HORZ;

    if(orient == DW_VERT)
    {
        bar = SB_VERT;
    }

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE;

    /* Get the current scroll positions */
    if(!GetScrollInfo(handle, bar, &si))
    {
        return -1;
    }
    return si.nMax;
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
   Box *newbox = calloc(sizeof(Box), 1);
   HWND hwndframe;

   newbox->pad = pad;
   newbox->type = type;
   newbox->count = 0;
   newbox->cinfo.fore = newbox->cinfo.back = -1;

   hwndframe = CreateWindow(FRAMECLASSNAME,
                      NULL,
                      WS_VISIBLE | WS_CHILD,
                      0,0,0,0,
                      DW_HWND_OBJECT,
                      NULL,
                      _DWInstance,
                      NULL);

   newbox->grouphwnd = CreateWindow(BUTTONCLASSNAME,
                            UTF8toWide(title),
                            WS_CHILD | BS_GROUPBOX |
                            WS_VISIBLE | WS_CLIPCHILDREN,
                            0,0,0,0,
                            hwndframe,
                            NULL,
                            _DWInstance,
                            NULL);

   /* Disable visual styles by default */
   if(_SetWindowTheme)
      _SetWindowTheme(newbox->grouphwnd, L"", L"");

   SetWindowLongPtr(hwndframe, GWLP_USERDATA, (LONG_PTR)newbox);
   newbox->cinfo.pOldProc = SubclassWindow(hwndframe, _dw_colorwndproc);
   dw_window_set_font(hwndframe, DefaultFont);
   return hwndframe;
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long id)
{
   CLIENTCREATESTRUCT ccs;
   HWND hwndframe;

   ccs.hWindowMenu = NULL;
   ccs.idFirstChild = 0;

   hwndframe = CreateWindow(TEXT("MDICLIENT"),
                      NULL,
                      WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                      0,0,0,0,
                      DW_HWND_OBJECT,
                      (HMENU)(uintptr_t)id,
                      _DWInstance,
                      &ccs);
   return hwndframe;
}

/*
 * Create a new HTML browser frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_html_new(unsigned long id)
{
#ifdef BUILD_HTML
   return CreateWindow(BrowserClassName,
                  NULL,
                  WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                  0,0,0,0,
                  DW_HWND_OBJECT,
                  (HMENU)(uintptr_t)id,
                  _DWInstance,
                  NULL);
#else
   dw_debug("HTML widget not available; Support not enabled in this build.\n");
   return 0;
#endif
}

#ifdef BUILD_HTML
void _dw_html_action(HWND hwnd, int action);
int _dw_html_raw(HWND hwnd, const char *string);
int _dw_html_url(HWND hwnd, const char *url);
int _dw_html_javascript_run(HWND hwnd, const char *script, void *scriptdata);
#ifdef BUILD_EDGE
void _dw_edge_action(HWND hwnd, int action);
int _dw_edge_raw(HWND hwnd, const char *string);
int _dw_edge_url(HWND hwnd, const char *url);
int _dw_edge_javascript_run(HWND hwnd, const char *script, void *scriptdata);
int _dw_edge_javascript_add(HWND hwnd, const char *name);
#endif
#endif

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
#ifdef BUILD_HTML
#ifdef BUILD_EDGE
   if (_DW_EDGE_DETECTED)
      _dw_edge_action(handle, action);
   else
#endif
   _dw_html_action(handle, action);
#endif
}

/*
 * Render raw HTML code in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       string: String buffer containt HTML code to
 *               be rendered.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_raw(HWND handle, const char *string)
{
#ifdef BUILD_HTML
#ifdef BUILD_EDGE
   if (_DW_EDGE_DETECTED)
      return _dw_edge_raw(handle, string);
#endif
   return _dw_html_raw(handle, string);
#else
   return DW_ERROR_GENERAL;
#endif
}

/*
 * Render file or web page in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       url: Universal Resource Locator of the web or
 *               file object to be rendered.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_url(HWND handle, const char *url)
{
#ifdef BUILD_HTML
#ifdef BUILD_EDGE
   if (_DW_EDGE_DETECTED)
      return _dw_edge_url(handle, url);
#endif
   return _dw_html_url(handle, url);
#else
   return DW_ERROR_GENERAL;
#endif
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
int dw_html_javascript_run(HWND handle, const char *script, void *scriptdata)
{
#ifdef BUILD_HTML
#ifdef BUILD_EDGE
   if (_DW_EDGE_DETECTED)
      return _dw_edge_javascript_run(handle, script, scriptdata);
#endif
   return _dw_html_javascript_run(handle, script, scriptdata);
#else
   return DW_ERROR_UNKNOWN;
#endif
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
#ifdef BUILD_HTML
#ifdef BUILD_EDGE
   if (_DW_EDGE_DETECTED)
      return _dw_edge_javascript_add(handle, name);
#endif
#endif
   return DW_ERROR_UNKNOWN;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_bitmap_new(ULONG id)
{
   return CreateWindow(STATICCLASSNAME,
                  NULL,
                  SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE |
                  WS_CHILD | WS_CLIPCHILDREN,
                  0,0,0,0,
                  DW_HWND_OBJECT,
                  (HMENU)(uintptr_t)id,
                  _DWInstance,
                  NULL);
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG id, int top)
{
   ULONG flags = 0;
   HWND tmp;
   NotebookPage **array = calloc(256, sizeof(NotebookPage *));
   ColorInfo *cinfo;

   if(!top)
      flags = TCS_BOTTOM;

   tmp = CreateWindow(WC_TABCONTROL,
                  NULL,
                  WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | flags,
                  0,0,0,0,
                  DW_HWND_OBJECT,
                  (HMENU)(uintptr_t)id,
                  _DWInstance,
                  NULL);
   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_simplewndproc);
   dw_window_set_data(tmp, "_dw_array", (void *)array);
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
   return (HMENUI)CreatePopupMenu();
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
   HMENUI tmp;
   ColorInfo *cinfo = _dw_window_get_cinfo(location);

   if(!cinfo)
      return NULL;

#ifdef DARK_MODE_TITLEBAR_MENU
   if(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED)
      tmp = (HMENUI)CreatePopupMenu();
   else
#endif
   {
      MENUINFO mi;

      tmp = (HMENUI)CreateMenu();

      mi.cbSize = sizeof(MENUINFO);
      mi.fMask = MIM_MENUDATA;
      mi.dwMenuData = (ULONG_PTR)1;

      SetMenuInfo((HMENU)tmp, &mi);
   }

   cinfo->hmenu = (HMENU)tmp;

#ifdef DARK_MODE_TITLEBAR_MENU
   if(!(_DW_DARK_MODE_ALLOWED > DW_DARK_MODE_BASIC && _DW_DARK_MODE_SUPPORTED))
#endif      
   SetMenu(location, (HMENU)tmp);
   return location;
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
      HMENU mymenu = (HMENU)*menu;

      if(IsWindow((HWND)mymenu) && !IsMenu(mymenu))
      {
         ColorInfo *cinfo = _dw_window_get_cinfo((HWND)mymenu);

         if(cinfo)
            mymenu = cinfo->hmenu;
      }
      if(IsMenu(mymenu))
         DestroyMenu(mymenu);
      *menu = NULL;
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
 *       menu: The handle to the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       flags: Extended attributes to set on the menu.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG id, ULONG flags, int end, int check, HMENUI submenu)
{
   MENUITEMINFO mii;
   HMENU mymenu = (HMENU)menux;
   char buffer[31] = {0};
   int is_checked, is_disabled;
   char *menutitle = (char *)title;

   /*
    * Check if this is a menubar; if so get the menu object
    * for the menubar
    */
   if(IsWindow(menux) && !IsMenu(mymenu))
   {
      ColorInfo *cinfo = _dw_window_get_cinfo((HWND)menux);

      if(cinfo)
         mymenu = cinfo->hmenu;
      else
         return NULL;
   }

   memset( &mii, 0, sizeof(mii) );
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;

   /* Convert from OS/2 style accellerators to Win32 style */
   if (title)
   {
      char *tmp = menutitle = _alloca(strlen(title)+1);

      strcpy(tmp, title);

      while(*tmp)
      {
         if(*tmp == '~')
            *tmp = '&';
         tmp++;
      }
   }

   if (menutitle && *menutitle)
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
      mii.fType = MFT_STRING;
   }
   else
      mii.fType = MFT_SEPARATOR;

   /*
    * Handle flags
    */
   is_checked = (flags & DW_MIS_CHECKED) ? 1 : 0;
   if ( is_checked )
      mii.fState |= MFS_CHECKED;
   else
      mii.fState |= MFS_UNCHECKED;
   is_disabled = (flags & DW_MIS_DISABLED) ? 1 : 0;
   if ( is_disabled )
      mii.fState |= MFS_DISABLED;
   else
      mii.fState |= MFS_ENABLED;

   mii.wID = id;
   if (IsMenu((HMENU)submenu))
      mii.hSubMenu = (HMENU)submenu;
   else
      mii.hSubMenu = 0;
   mii.dwTypeData = UTF8toWide(menutitle);
   mii.cch = (UINT)_tcslen(mii.dwTypeData);

   InsertMenuItem(mymenu, 65535, TRUE, &mii);

   _snprintf(buffer, 30, "_dw_id%ld", id);
   dw_window_set_data( DW_HWND_OBJECT, buffer, DW_POINTER(mymenu) );
   _snprintf(buffer, 30, "_dw_checkable%ld", id);
   dw_window_set_data( DW_HWND_OBJECT, buffer, DW_INT_TO_POINTER(check) );
   _snprintf(buffer, 30, "_dw_ischecked%ld", id);
   dw_window_set_data( DW_HWND_OBJECT, buffer, DW_INT_TO_POINTER(is_checked) );
   _snprintf(buffer, 30, "_dw_isdisabled%ld", id);
   dw_window_set_data( DW_HWND_OBJECT, buffer, DW_INT_TO_POINTER(is_disabled) );

   if (submenu)
   {
      MENUINFO mi;

      mi.cbSize = sizeof(MENUINFO);
      mi.fMask = MIM_MENUDATA;
      mi.dwMenuData = (ULONG_PTR)mymenu;

      SetMenuInfo( (HMENU)submenu, &mi );
   }

   if (IsWindow(menux) && !IsMenu((HMENU)menux))
      DrawMenuBar(menux);
   return (HWND)(uintptr_t)id;
}

/*
 * Sets the state of a menu item check.
 * Deprecated: use dw_menu_item_set_state()
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 */
void API dw_menu_item_set_check(HMENUI menux, unsigned long id, int check)
{
   MENUITEMINFO mii;
   HMENU mymenu = (HMENU)menux;
   char buffer[30];

   if(IsWindow(menux) && !IsMenu(mymenu))
   {
      ColorInfo *cinfo = _dw_window_get_cinfo((HWND)menux);

      if(cinfo)
         mymenu = cinfo->hmenu;
      else
         return;
   }

   /*
    * Get the current state of the menu item in case it already has some other state set on it
    */
   memset( &mii, 0, sizeof(mii) );
   GetMenuItemInfo( mymenu, id, FALSE, &mii);

   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE | MIIM_CHECKMARKS;
   if (check)
      mii.fState |= MFS_CHECKED;
   else
      mii.fState |= MFS_UNCHECKED;
   SetMenuItemInfo( mymenu, id, FALSE, &mii );
   /*
    * Keep our internal state consistent
    */
   _snprintf( buffer, 30, "_dw_ischecked%ld", id );
   dw_window_set_data( DW_HWND_OBJECT, buffer, DW_INT_TO_POINTER(check) );
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       flags: DW_MIS_ENABLED/DW_MIS_DISABLED
 *              DW_MIS_CHECKED/DW_MIS_UNCHECKED
 */
void API dw_menu_item_set_state(HMENUI menux, unsigned long id, unsigned long state)
{
   MENUITEMINFO mii;
   HMENU mymenu = (HMENU)menux;
   char buffer1[31] = {0}, buffer2[31] = {0};
   int check;
   int disabled;

   if(IsWindow(menux) && !IsMenu(mymenu))
   {
      ColorInfo *cinfo = _dw_window_get_cinfo((HWND)menux);

      if(cinfo)
         mymenu = cinfo->hmenu;
      else
         return;
   }

   _snprintf( buffer1, 30, "_dw_ischecked%ld", id );
   check = DW_POINTER_TO_INT(dw_window_get_data( DW_HWND_OBJECT, buffer1 ));
   _snprintf( buffer2, 30, "_dw_isdisabled%ld", id );
   disabled = DW_POINTER_TO_INT(dw_window_get_data( DW_HWND_OBJECT, buffer2 ));

   memset(&mii, 0, sizeof(mii));

   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STATE | MIIM_CHECKMARKS;
   if((state & DW_MIS_CHECKED) || (state & DW_MIS_UNCHECKED))
   {
      /*
       * If we are changing state of "checked" base our setting on the passed flag...
       */
      if ( state & DW_MIS_CHECKED )
      {
         mii.fState |= MFS_CHECKED;
         check = 1;
      }
      else
      {
         mii.fState |= MFS_UNCHECKED;
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
         mii.fState |= MFS_CHECKED;
      }
      else
      {
         mii.fState |= MFS_UNCHECKED;
      }
   }
   if((state & DW_MIS_ENABLED) || (state & DW_MIS_DISABLED))
   {
      if ( state & DW_MIS_DISABLED )
      {
         mii.fState |= MFS_DISABLED;
         disabled = 1;
      }
      else
      {
         mii.fState |= MFS_ENABLED;
         disabled = 0;
      }
   }
   else
   {
      /*
       * ...otherwise base our setting on the current "checked" state.
       */
      if ( disabled )
      {
         mii.fState |= MFS_DISABLED;
      }
      else
      {
         mii.fState |= MFS_ENABLED;
      }
   }
   SetMenuItemInfo(mymenu, id, FALSE, &mii);
   /*
    * Keep our internal checked state consistent
    */
   dw_window_set_data(DW_HWND_OBJECT, buffer1, DW_INT_TO_POINTER(check));
   dw_window_set_data(DW_HWND_OBJECT, buffer2, DW_INT_TO_POINTER(disabled));
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
   HMENU mymenu = (HMENU)menux;

   if(IsWindow(menux) && !IsMenu(mymenu))
   {
      ColorInfo *cinfo = _dw_window_get_cinfo((HWND)menux);

      if(cinfo)
         mymenu = cinfo->hmenu;
      else
         return DW_ERROR_UNKNOWN;
   }

   if(mymenu == 0 || DeleteMenu(mymenu, id, MF_BYCOMMAND) == 0)
      return DW_ERROR_UNKNOWN;

   /* If the ID was autogenerated it is safe to remove it */
   if(id >= 30000)
      dw_signal_disconnect_by_window((HWND)(uintptr_t)id);

   /* Make sure the menu is redrawn if needed */
   if((HMENU)menux != mymenu)
      DrawMenuBar(menux);
   return DW_ERROR_NONE;
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
      HMENU mymenu = (HMENU)*menu;

      if(IsWindow(*menu) && !IsMenu(mymenu))
      {
         ColorInfo *cinfo = _dw_window_get_cinfo((HWND)*menu);

         if(cinfo)
            mymenu = cinfo->hmenu;
         else
            return;
      }

      _dw_popup = parent;
      TrackPopupMenu(mymenu, 0, x, y, 0, parent, NULL);
      PostMessage(DW_HWND_OBJECT, WM_USER+5, (LPARAM)mymenu, 0);
      *menu = NULL;
   }
}


/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_container_new(ULONG id, int multi)
{
   HWND tmp = CreateWindow(WC_LISTVIEW,
                     NULL,
                     WS_VISIBLE | WS_CHILD |
                     (multi ? 0 : LVS_SINGLESEL) |
                     LVS_REPORT | LVS_SHOWSELALWAYS |
                     LVS_SHAREIMAGELISTS | WS_BORDER |
                     WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   ContainerInfo *cinfo = (ContainerInfo *)calloc(1, sizeof(ContainerInfo));

   if(!cinfo)
   {
      DestroyWindow(tmp);
      return NULL;
   }

   cinfo->cinfo.pOldProc = SubclassWindow(tmp, _dw_containerwndproc);
   cinfo->cinfo.fore = cinfo->cinfo.back = -1;
   cinfo->odd = cinfo->even = DW_RGB_TRANSPARENT;

   SetWindowLongPtr(tmp, GWLP_USERDATA, (LONG_PTR)cinfo);
   dw_window_set_font(tmp, DefaultFont);
   /* If we are running XP or higher... */
   if(IS_XPPLUS)
   {
      /* Enable double buffering to prevent flicker */
      ListView_SetExtendedListViewStyleEx(tmp, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
   }
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
   HWND tmp = CreateWindow(WC_TREEVIEW,
                     NULL,
                     WS_VISIBLE | WS_CHILD |
                     TVS_HASLINES | TVS_SHOWSELALWAYS |
                     TVS_HASBUTTONS | TVS_LINESATROOT |
                     WS_BORDER | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   ContainerInfo *cinfo = (ContainerInfo *)calloc(1, sizeof(ContainerInfo));
   TreeView_SetItemHeight(tmp, 16);

   if(!cinfo)
   {
      DestroyWindow(tmp);
      return NULL;
   }

   cinfo->cinfo.pOldProc = SubclassWindow(tmp, _dw_treewndproc);
   cinfo->cinfo.fore = cinfo->cinfo.back = -1;
   cinfo->odd = cinfo->even = DW_RGB_TRANSPARENT;

   SetWindowLongPtr(tmp, GWLP_USERDATA, (LONG_PTR)cinfo);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
   POINT ptl;

   GetCursorPos(&ptl);
   if(x && y)
   {
      *x = ptl.x;
      *y = ptl.y;
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
   SetCursorPos(x, y);
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(const char *text, ULONG id)
{
   HWND tmp = CreateWindow(STATICCLASSNAME,
                     UTF8toWide(text),
                     SS_NOPREFIX | SS_NOTIFY | SS_LEFTNOWORDWRAP |
                     WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
#ifdef AEROGLASS
   ColorInfo *cinfo = _dw_window_new_cinfo(tmp, FALSE);

   if(cinfo)
      cinfo->pOldProc = SubclassWindow(tmp, _dw_staticwndproc);
#endif
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_DEFAULT, DW_RGB_TRANSPARENT);
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
   HWND tmp = CreateWindow(StatusbarClassName,
                     UTF8toWide(text),
                     WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG id)
{
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
#ifdef RICHEDIT
                       _DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED ? (hmsftedit ? MSFTEDIT_CLASS : (hrichedit ? RICHEDIT_CLASS : EDITCLASSNAME)) : EDITCLASSNAME,
#else
                       EDITCLASSNAME,
#endif
                       NULL,
                       WS_VISIBLE | WS_BORDER |
                       WS_VSCROLL | ES_MULTILINE |
                       ES_WANTRETURN | WS_CHILD |
                       WS_CLIPCHILDREN,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);
   ContainerInfo *cinfo = (ContainerInfo *)calloc(1, sizeof(ContainerInfo));

   if(!cinfo)
   {
      DestroyWindow(tmp);
      return NULL;
   }

#ifdef RICHEDIT
   if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
      cinfo->cinfo.pOldProc = SubclassWindow(tmp, _dw_richeditwndproc);
   else
#endif
   cinfo->cinfo.pOldProc = SubclassWindow(tmp, _dw_simplewndproc);
   cinfo->cinfo.fore = cinfo->cinfo.back = -1;
   cinfo->odd = cinfo->even = DW_RGB_TRANSPARENT;

   SetWindowLongPtr(tmp, GWLP_USERDATA, (LONG_PTR)cinfo);
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
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
                       EDITCLASSNAME,
                       UTF8toWide(text),
                       ES_WANTRETURN | WS_CHILD |
                       WS_BORDER | ES_AUTOHSCROLL |
                       WS_VISIBLE | WS_CLIPCHILDREN,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);

   _dw_window_new_cinfo(tmp, TRUE);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

/*
 * Create a new Entryfield passwird window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(const char *text, ULONG id)
{
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
                       EDITCLASSNAME,
                       UTF8toWide(text),
                       ES_WANTRETURN | WS_CHILD |
                       ES_PASSWORD | WS_BORDER | WS_VISIBLE |
                       ES_AUTOHSCROLL | WS_CLIPCHILDREN,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);

   _dw_window_new_cinfo(tmp, TRUE);
   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

BOOL CALLBACK _dw_subclass_child(HWND handle, LPARAM lp)
{
   ColorInfo *cinfo = (ColorInfo *)lp;

   if(cinfo)
   {
      cinfo->buddy = handle;
      cinfo->pOldProc = SubclassWindow(handle, _dw_colorwndproc);
      SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)cinfo);
   }
   return FALSE;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_combobox_new(const char *text, ULONG id)
{
   HWND tmp = CreateWindow(COMBOBOXCLASSNAME,
                     UTF8toWide(text),
                     WS_CHILD | CBS_DROPDOWN | WS_VSCROLL |
                     WS_CLIPCHILDREN | CBS_AUTOHSCROLL | WS_VISIBLE,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   ColorInfo *cinfo = (ColorInfo *)calloc(1, sizeof(ColorInfo));
   ColorInfo *cinfo2 = (ColorInfo *)calloc(1, sizeof(ColorInfo));

   if(!cinfo || !cinfo2)
   {
      if(cinfo)
         free(cinfo);
      if(cinfo2)
         free(cinfo2);
      DestroyWindow(tmp);
      return NULL;
   }

   cinfo2->fore = cinfo->fore = -1;
   cinfo2->back = cinfo->back = -1;
   cinfo2->combo = cinfo->combo = tmp;
   EnumChildWindows(tmp, _dw_subclass_child, (LPARAM)cinfo2);
   cinfo->buddy = cinfo2->buddy;

   SetWindowLongPtr(tmp, GWLP_USERDATA, (LONG_PTR)cinfo);
   dw_window_set_font(tmp, DefaultFont);
   SetWindowText(tmp, UTF8toWide(text));
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
   HWND tmp = CreateWindow(BUTTONCLASSNAME,
                     UTF8toWide(text),
                     WS_CHILD | BS_PUSHBUTTON |
                     WS_VISIBLE | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   ColorInfo *cinfo;

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_buttonwndproc);

   dw_window_set_font(tmp, DefaultFont);
   return tmp;
}

#ifdef TOOLBAR
/* Internal function to create a grayscale bitmap from a color one */
void _dw_to_grayscale(HBITMAP hbm, int width, int height)
{
   HDC hdc = CreateCompatibleDC(NULL);
   if (hdc)
   {
      HBITMAP hbmPrev = SelectBitmap(hdc, hbm);
      int x, y;

      for(y=0;y<height;y++)
      {
         for(x=0;x<width;x++)
         {
            COLORREF c = GetPixel(hdc, x, y);
            /* Use half-values then add 127 to make it look washed out */
            int luma = (int)(GetRValue(c)*0.15 + GetGValue(c)*0.3+ GetBValue(c)*0.06) + 127;

            SetPixel(hdc, x, y, RGB(luma,luma,luma));
         }
      }
      SelectBitmap(hdc, hbmPrev);
      DeleteDC(hdc);
   }
}

/* Internal function to create a toolbar based button */
HWND _dw_create_toolbar(const char *text, ULONG id, HICON icon, HBITMAP hbitmap)
{
   HWND tmp;
   HIMAGELIST imlist, dimlist;
   BITMAP bmi = { 0 };
   TBBUTTON tbButtons[] = {
   { MAKELONG(0, 0), id, TBSTATE_ENABLED, TBSTYLE_BUTTON}
   };

   /* Get the bitmap from either the icon or bitmap itself */
   if(hbitmap)
   {
      GetObject(hbitmap, sizeof(BITMAP), &bmi);
      imlist = ImageList_Create(bmi.bmWidth, bmi.bmHeight, ILC_COLOR32, 1, 0);
      ImageList_Add(imlist, hbitmap, NULL);
      dimlist = ImageList_Create(bmi.bmWidth, bmi.bmHeight, ILC_COLOR32, 1, 0);
      _dw_to_grayscale(hbitmap, bmi.bmWidth, bmi.bmHeight);
      ImageList_Add(dimlist, hbitmap, NULL);
      DeleteObject(hbitmap);
   }
   else if(icon)
   {
      ICONINFO iconinfo;

      GetIconInfo(icon, &iconinfo);
      GetObject(iconinfo.hbmColor, sizeof(BITMAP), &bmi);
      imlist = ImageList_Create(bmi.bmWidth, bmi.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
      ImageList_AddIcon(imlist, icon);
      dimlist = ImageList_Create(bmi.bmWidth, bmi.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
      _dw_to_grayscale(iconinfo.hbmColor, bmi.bmWidth, bmi.bmHeight);
      ImageList_Add(dimlist, iconinfo.hbmColor, iconinfo.hbmMask);
      DeleteObject(iconinfo.hbmColor);
      DeleteObject(iconinfo.hbmMask);
      DestroyIcon(icon);
   }
   else
      return 0;

   /* Create the toolbar */
   tmp = CreateWindowEx(0L, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | TBSTYLE_AUTOSIZE | CCS_NORESIZE |
                        CCS_NOPARENTALIGN | CCS_NODIVIDER, 0, 0, 100, 30, DW_HWND_OBJECT, (HMENU)(uintptr_t)id, _DWInstance, NULL);

   /* Disable visual styles by default */
   if(_SetWindowTheme)
      _SetWindowTheme(tmp, L"", L"");

   /* Insert the single bitmap and button into the toolbar */
   SendMessage(tmp, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
   SendMessage(tmp, TB_SETBUTTONSIZE, 0, MAKELPARAM(bmi.bmWidth, bmi.bmHeight));
   SendMessage(tmp, TB_SETPADDING, 0, 0);
   SendMessage(tmp, TB_SETIMAGELIST, 0, (LPARAM)imlist);
   SendMessage(tmp, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)dimlist);
   SendMessage(tmp, TB_ADDBUTTONS, 1, (LPARAM) &tbButtons);

   _dw_create_tooltip(tmp, text);
   return tmp;
}
#endif

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(const char *text, ULONG id)
{
   HICON icon = LoadImage(_DWInstance, MAKEINTRESOURCE(id), IMAGE_ICON, 0, 0, 0);
   HBITMAP hbitmap = icon ? 0 : LoadBitmap(_DWInstance, MAKEINTRESOURCE(id));
   ColorInfo *cinfo;
   HWND tmp;

#ifdef TOOLBAR
   if((tmp = _dw_create_toolbar(text, id, icon, hbitmap)))
   {
      if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
         cinfo->pOldProc = SubclassWindow(tmp, _dw_simplewndproc);
      return tmp;
   }
#endif

   tmp = CreateWindow(BUTTONCLASSNAME,
                  NULL,
                  WS_CHILD | BS_PUSHBUTTON |
                  WS_VISIBLE | WS_CLIPCHILDREN |
                  (icon ? BS_ICON : BS_BITMAP),
                  0,0,0,0,
                  DW_HWND_OBJECT,
                  (HMENU)(uintptr_t)id,
                  _DWInstance,
                  NULL);

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_buttonwndproc);

   _dw_create_tooltip(tmp, text);

   if(icon)
      SendMessage(tmp, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)icon);
   else if(hbitmap)
      SendMessage(tmp, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitmap);
   return tmp;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP or ICO on OS/2 or Windows, XPM on Unix)
 */
HWND API dw_bitmapbutton_new_from_file(const char *text, unsigned long id, const char *filename)
{
   HWND tmp;
   ColorInfo *cinfo;
   HBITMAP hbitmap = 0;
   HANDLE hicon = 0;
   int windowtype = 0;

#ifdef GDIPLUS
   if((hicon = _dw_load_icon(filename)))
      windowtype = BS_ICON;
   else
   {
      hbitmap = _dw_load_bitmap(filename, NULL);
      windowtype = BS_BITMAP;
   }
#else
   windowtype = _dw_get_image_handle(filename, &hicon, &hbitmap);
#endif

#ifdef TOOLBAR
   if((tmp = _dw_create_toolbar(text, id, hicon, hbitmap)))
   {
      if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
         cinfo->pOldProc = SubclassWindow(tmp, _dw_simplewndproc);
      return tmp;
   }
#endif
   tmp = CreateWindow( BUTTONCLASSNAME,
                       NULL,
                       windowtype | WS_CHILD | BS_PUSHBUTTON | WS_CLIPCHILDREN | WS_VISIBLE,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_buttonwndproc);

   _dw_create_tooltip(tmp, text);

   if (hicon)
      SendMessage(tmp, BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
   else if (hbitmap)
      SendMessage(tmp, BM_SETIMAGE,(WPARAM)IMAGE_BITMAP, (LPARAM)hbitmap);
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
   HWND tmp;
   ColorInfo *cinfo;
   HBITMAP hbitmap = 0;
   HANDLE hicon = 0;
   char *file;
   FILE *fp;
   int windowtype = BS_BITMAP;

   file = _tempnam( _dw_alternate_temp_dir, "dw" );
   if ( file != NULL )
   {
      fp = fopen( file, "wb" );
      if ( fp != NULL )
      {
         fwrite( data, 1, len, fp );
         fclose( fp );
#ifdef GDIPLUS
         if((hicon = _dw_load_icon(file)))
            windowtype = BS_ICON;
         else
            hbitmap = _dw_load_bitmap(file, NULL);
#else
         if ( len > 1 && data[0] == 'B' && data[1] == 'M' ) /* first 2 chars of data is BM, then its a BMP */
            hbitmap = (HBITMAP)LoadImage( NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
         else /* otherwise its assumed to be an ico */
         {
            hicon = LoadImage( NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE );
            windowtype = BS_ICON;
         }
#endif
      }
      else
      {
         _unlink( file );
         free( file );
         return 0;
      }
      _unlink( file );
      free( file );
   }

#ifdef TOOLBAR
   if((tmp = _dw_create_toolbar(text, id, hicon, hbitmap)))
   {
      if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
         cinfo->pOldProc = SubclassWindow(tmp, _dw_simplewndproc);
      return tmp;
   }
#endif
   tmp = CreateWindow( BUTTONCLASSNAME,
                       NULL,
                       WS_CHILD | BS_PUSHBUTTON |
                       windowtype | WS_CLIPCHILDREN |
                       WS_VISIBLE,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL );

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow( tmp, _dw_buttonwndproc );

   _dw_create_tooltip(tmp, text);

   if(hicon)
      SendMessage(tmp, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hicon);
   else if(hbitmap)
      SendMessage(tmp, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitmap);
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
   HWND buddy = CreateWindowEx(WS_EX_CLIENTEDGE,
                        EDITCLASSNAME,
                        UTF8toWide(text),
                        WS_CHILD | WS_BORDER | WS_VISIBLE |
                        ES_NUMBER | WS_CLIPCHILDREN,
                        0,0,0,0,
                        DW_HWND_OBJECT,
                        NULL,
                        _DWInstance,
                        NULL);
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
                       UPDOWN_CLASS,
                       NULL,
                       WS_CHILD | UDS_ALIGNRIGHT | WS_BORDER |
                       UDS_ARROWKEYS | UDS_SETBUDDYINT |
                       UDS_WRAP | UDS_NOTHOUSANDS | WS_VISIBLE,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);
   ColorInfo *cinfo;

   SendMessage(tmp, UDM_SETRANGE32, (WPARAM)-65536,(LPARAM)65536);
   SendMessage(tmp, UDM_SETBUDDY, (WPARAM)buddy, 0);
   
   if((cinfo = _dw_window_new_cinfo(buddy, TRUE)))
      cinfo->buddy = tmp;

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
   {
      cinfo->buddy = buddy;

      /* The horrible spinbutton workaround isn't necessary
       * any more on Vista or 7... but still seems necessary
       * for XP, so only enable it if on XP or lower.
       */
      if(!IS_VISTAPLUS)
         cinfo->pOldProc = SubclassWindow(tmp, _spinnerwndproc);
   }

   dw_window_set_font(buddy, DefaultFont);
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
   ColorInfo *cinfo;
   HWND tmp = CreateWindow(BUTTONCLASSNAME,
                     UTF8toWide(text),
                     WS_CHILD | BS_AUTORADIOBUTTON |
                     WS_CLIPCHILDREN | WS_VISIBLE,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);

   /* Disable visual styles by default */
   if(_SetWindowTheme)
      _SetWindowTheme(tmp, L"", L"");

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_buttonwndproc);
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_DEFAULT, DW_RGB_TRANSPARENT);
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
   HWND tmp = CreateWindow(TRACKBAR_CLASS,
                     NULL,
                     WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE |
                     (vertical ? TBS_VERT : TBS_HORZ),
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);

   _dw_window_new_cinfo(tmp, TRUE);
   SendMessage(tmp, TBM_SETRANGE, (WPARAM)FALSE, MAKELPARAM(0, increments-1));
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
   HWND tmp = CreateWindow(SCROLLBARCLASSNAME,
                     NULL,
                     WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE |
                     (vertical ? SBS_VERT : SBS_HORZ),
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);

   _dw_window_new_cinfo(tmp, TRUE);
   dw_window_set_data(tmp, "_dw_scrollbar", (void *)1);
   return tmp;
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG id)
{
   return CreateWindow(PROGRESS_CLASS,
                  NULL,
                  WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                  0,0,0,0,
                  DW_HWND_OBJECT,
                  (HMENU)(uintptr_t)id,
                  _DWInstance,
                  NULL);
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(const char *text, ULONG id)
{
   ColorInfo *cinfo;
   HWND tmp = CreateWindow(BUTTONCLASSNAME,
                     UTF8toWide(text),
                     WS_CHILD | BS_AUTOCHECKBOX |
                     BS_TEXT | WS_CLIPCHILDREN | WS_VISIBLE,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);

   /* Disable visual styles by default */
   if(_SetWindowTheme)
      _SetWindowTheme(tmp, L"", L"");

   if((cinfo = _dw_window_new_cinfo(tmp, FALSE)))
      cinfo->pOldProc = SubclassWindow(tmp, _dw_buttonwndproc);
   dw_window_set_data(tmp, "_dw_checkbox", DW_INT_TO_POINTER(1));
   dw_window_set_font(tmp, DefaultFont);
   dw_window_set_color(tmp, DW_CLR_DEFAULT, DW_RGB_TRANSPARENT);
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
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
                       LISTBOXCLASSNAME,
                       NULL,
                       WS_VISIBLE | LBS_NOINTEGRALHEIGHT |
                       WS_CHILD | LBS_HASSTRINGS |
                       LBS_NOTIFY | WS_BORDER  | WS_CLIPCHILDREN |
                       WS_VSCROLL | (multi ? LBS_MULTIPLESEL : 0) ,
                       0,0,0,0,
                       DW_HWND_OBJECT,
                       (HMENU)(uintptr_t)id,
                       _DWInstance,
                       NULL);
   ContainerInfo *cinfo = (ContainerInfo *)calloc(1, sizeof(ContainerInfo));

   if(!cinfo)
   {
      DestroyWindow(tmp);
      return NULL;
   }

   cinfo->cinfo.fore = cinfo->cinfo.back = -1;
   cinfo->odd = cinfo->even = DW_RGB_TRANSPARENT;
   cinfo->cinfo.pOldProc = SubclassWindow(tmp, _dw_containerwndproc);

   SetWindowLongPtr(tmp, GWLP_USERDATA, (LONG_PTR)cinfo);
   dw_window_set_font(tmp, DefaultFont);
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
   int iicon = DW_POINTER_TO_INT(icon);
   HICON hicon = iicon < 65536 ? LoadIcon(_DWInstance, MAKEINTRESOURCE(iicon)) : (HICON)icon;

   SendMessage(handle, WM_SETICON,
            (WPARAM) IMAGE_ICON,
            (LPARAM) hicon);
}

/* Internal function to set bitmap for the next two functions */
int _dw_window_set_bitmap(HWND handle, HICON icon, HBITMAP hbitmap)
{
   HBITMAP oldbitmap = 0;
   HANDLE oldicon = 0;
   TCHAR tmpbuf[100] = {0};

   if (!icon && !hbitmap)
      return DW_ERROR_GENERAL;

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1)==0)
   {
      oldbitmap = (HBITMAP)SendMessage(handle, BM_GETIMAGE, IMAGE_BITMAP, 0);
      oldicon = (HICON)SendMessage(handle, BM_GETIMAGE, IMAGE_ICON, 0);
      SendMessage(handle, BM_SETIMAGE,
               (icon ? (WPARAM)IMAGE_ICON : (WPARAM)IMAGE_BITMAP),
               (icon ? (LPARAM)icon : (LPARAM)hbitmap));
   }
#ifdef TOOLBAR
   /* Bitmap Buttons */
   else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
   {
      HIMAGELIST imlist = (HIMAGELIST)SendMessage(handle, TB_GETIMAGELIST, 0, 0);
      HIMAGELIST dimlist = (HIMAGELIST)SendMessage(handle, TB_GETDISABLEDIMAGELIST, 0, 0);
      BITMAP bmi = { 0 };

      if(hbitmap)
      {
         GetObject(hbitmap, sizeof(BITMAP), &bmi);
         ImageList_Replace(imlist, 0, hbitmap, NULL);
         _dw_to_grayscale(hbitmap, bmi.bmWidth, bmi.bmHeight);
         ImageList_Replace(dimlist, 0, hbitmap, NULL);
         DeleteObject(hbitmap);
      }
      else if(icon)
      {
         ICONINFO iconinfo;

         GetIconInfo(icon, &iconinfo);
         GetObject(iconinfo.hbmColor, sizeof(BITMAP), &bmi);
         ImageList_ReplaceIcon(imlist, 0, icon);
         _dw_to_grayscale(iconinfo.hbmColor, bmi.bmWidth, bmi.bmHeight);
         ImageList_Replace(dimlist, 0, iconinfo.hbmColor, iconinfo.hbmMask);
         DeleteObject(iconinfo.hbmColor);
         DeleteObject(iconinfo.hbmMask);
         DestroyIcon(icon);
      }
      InvalidateRect(handle, NULL, FALSE);
   }
#endif
   else
   {
      oldbitmap = (HBITMAP)SendMessage(handle, STM_GETIMAGE, IMAGE_BITMAP, 0);
      oldicon = (HICON)SendMessage(handle, STM_GETIMAGE, IMAGE_ICON, 0);
      SendMessage(handle, STM_SETIMAGE,
               (icon ? (WPARAM)IMAGE_ICON : (WPARAM)IMAGE_BITMAP),
               (icon ? (LPARAM)icon : (LPARAM)hbitmap));
   }

   if(oldbitmap)
      DeleteObject(oldbitmap);
   if(oldicon)
      DeleteObject(oldicon);

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
   HBITMAP hbitmap = 0;
   HANDLE icon = 0;

   if(id)
   {
      hbitmap = LoadImage(_DWInstance, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_SHARED);
      icon = LoadImage(_DWInstance, MAKEINTRESOURCE(id), IMAGE_ICON, 0, 0, LR_SHARED);
   }
   else if(filename)
   {
#ifdef GDIPLUS
      hbitmap = _dw_load_bitmap(filename, NULL);
#else
      _dw_get_image_handle(filename, &icon, &hbitmap);
#endif
   }
   else
      return DW_ERROR_UNKNOWN;

   return _dw_window_set_bitmap(handle, icon, hbitmap);
}

/*
 * Sets the bitmap used for a given static window from data.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the data param)
 *       data: the image from meory
 *                 Bitmap on Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 *       len: length of data
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long id, const char *data, int len)
{
   HBITMAP hbitmap=0;
   HICON icon=0;
   char *file;
   FILE *fp;

   if (data)
   {
      file = _tempnam( _dw_alternate_temp_dir, "dw" );
      if ( file != NULL )
      {
         fp = fopen( file, "wb" );
         if ( fp != NULL )
         {
            fwrite( data, 1, len, fp );
            fclose( fp );
#ifdef GDIPLUS
            hbitmap = _dw_load_bitmap(file, NULL);
#else
            if ( len > 1 && data[0] == 'B' && data[1] == 'M' ) /* first 2 chars of data is BM, then its a BMP */
               hbitmap = (HBITMAP)LoadImage( NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
            else /* otherwise its assumed to be an ico */
               icon = LoadImage( NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE );
#endif
         }
         else
         {
            _unlink( file );
            free( file );
            return DW_ERROR_GENERAL;
         }
         _unlink( file );
         free( file );
      }
      if (icon == 0 && hbitmap == 0)
         return DW_ERROR_GENERAL;
   }
   else if ( id )
   {
      hbitmap = LoadBitmap(_DWInstance, MAKEINTRESOURCE(id));
      icon = LoadImage(_DWInstance, MAKEINTRESOURCE(id), IMAGE_ICON, 0, 0, LR_SHARED);
   }
   else
      return DW_ERROR_UNKNOWN;

   return _dw_window_set_bitmap(handle, icon, hbitmap);
}


/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, const char *text)
{
   Box *thisbox;
   TCHAR tmpbuf[100] = {0}, *wtext = UTF8toWide(text);

   GetClassName(handle, tmpbuf, 99);

   SetWindowText(handle, wtext);

   /* Combobox */
   if(_tcsnicmp( tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1) == 0)
      SendMessage(handle, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
   else if(_tcsnicmp( tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1) == 0)
   {
      ColorInfo *cinfo = _dw_window_get_cinfo(handle);
      if(cinfo && cinfo->buddy)
         SetWindowText(cinfo->buddy, wtext);
   }
   else if(_tcsnicmp( tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1) == 0)
   {
      /* groupbox */
      thisbox = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);
      if(thisbox && thisbox->grouphwnd != (HWND)NULL)
         SetWindowText(thisbox->grouphwnd, wtext);
   }
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
    ColorInfo *cinfo = _dw_window_get_cinfo(handle);

    if(cinfo && cinfo->buddy)
        _dw_create_tooltip(cinfo->buddy, bubbletext);
    _dw_create_tooltip(handle, bubbletext);
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
   char *retbuf = NULL;
   TCHAR *tempbuf, tmpbuf[100] = { 0 };
   int len;

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp( tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1) == 0)
   {
      ColorInfo *cinfo = (ColorInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);

      if(cinfo && cinfo->buddy)
        handle = cinfo->buddy;
      else
        return NULL;
   }

   /* Figure out the wide length, allocate a temp buffer
    * and fill it with the current text.
    */
   len = GetWindowTextLength(handle) + 1;
   if((tempbuf = _alloca(len * sizeof(TCHAR))))
      GetWindowText(handle, tempbuf, len);

   /* Figure out the UTF8 length, allocate a return buffer
    * and fill it with the UTF8 text and return it.
    */
   if(tempbuf && (retbuf = WideToUTF8(tempbuf)))
      retbuf = _strdup(retbuf);
   return retbuf;
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
   if(handle < (HWND)65536)
   {
      char buffer[31] = {0};
      HMENU mymenu;
      ULONG id = (ULONG)(uintptr_t)handle;

      _snprintf(buffer, 30, "_dw_id%ld", id);
      mymenu = (HMENU)dw_window_get_data(DW_HWND_OBJECT, buffer);

      if(mymenu && IsMenu(mymenu))
         dw_menu_item_set_state((HMENUI)mymenu, id, DW_MIS_DISABLED);
   }
   else
      EnableWindow(handle, FALSE);
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
   if(handle < (HWND)65536)
   {
      char buffer[31] = {0};
      HMENU mymenu;
      ULONG id = (ULONG)(uintptr_t)handle;

      _snprintf(buffer, 30, "_dw_id%ld", id);
      mymenu = (HMENU)dw_window_get_data(DW_HWND_OBJECT, buffer);

      if(mymenu && IsMenu(mymenu))
         dw_menu_item_set_state((HMENUI)mymenu, id, DW_MIS_ENABLED);
   }
   else
   EnableWindow(handle, TRUE);
}

static HWND _dw_wfid_hwnd = NULL;

BOOL CALLBACK _dw_wfid(HWND handle, LPARAM lParam)
{
   if(GetWindowLong(handle, GWL_ID) == lParam)
   {
      _dw_wfid_hwnd = handle;
      return FALSE;
   }
   return TRUE;
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
   _dw_wfid_hwnd = NULL;
   EnumChildWindows(handle, _dw_wfid, (LPARAM)id);
    return _dw_wfid_hwnd;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
   Box *thisbox = NULL;
   TCHAR tmpbuf[100] = {0};

      /*
       * If you try and pack an item into itself VERY bad things can happen; like at least an
       * infinite loop on GTK! Lets be safe!
       */
   if(box == item)
   {
      dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
      return;
   }

   GetClassName(box, tmpbuf, 99);

   /* If we are in a scrolled box... extract the interal box */
   if(_tcsnicmp(tmpbuf, ScrollClassName, _tcslen(ScrollClassName)+1)==0)
   {
        ColorInfo *cinfo = _dw_window_get_cinfo(box);
        if(cinfo)
        {
            box = cinfo->buddy;
            thisbox = (Box *)GetWindowLongPtr(box, GWLP_USERDATA);
        }
   }
   else //if(_tcsnicmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1)==0)
       thisbox = (Box *)GetWindowLongPtr(box, GWLP_USERDATA);
   if(thisbox)
   {
      int z, x = 0;
      Item *tmpitem, *thisitem = thisbox->items;

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

      GetClassName(item, tmpbuf, 99);

      if(vsize && !height)
         height = 1;
      if(hsize && !width)
         width = 1;

      if(_tcsnicmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1)==0)
         tmpitem[index].type = _DW_TYPE_BOX;
      else
      {
         if(_tcsnicmp(tmpbuf, TEXT("SysMonthCal32"), 13)==0)
         {
            RECT rc;
            MonthCal_GetMinReqRect(item, &rc);
            width = 1 + rc.right - rc.left;
            height = 1 + rc.bottom - rc.top;
            tmpitem[index].type = _DW_TYPE_ITEM;
         }
         else
         {
            if ( width == 0 && hsize == FALSE )
               dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
            if ( height == 0 && vsize == FALSE )
               dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Height and expand Vertical both unset for box: %x item: %x",box,item);

            tmpitem[index].type = _DW_TYPE_ITEM;
         }
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

#ifdef AEROGLASS
      _dw_allow_dark_mode_for_window(item, _DW_DARK_MODE_ENABLED);
#endif
      SetParent(item, box);
      if(_tcsnicmp(tmpbuf, UPDOWN_CLASS, _tcslen(UPDOWN_CLASS)+1)==0)
      {
         ColorInfo *cinfo = _dw_window_get_cinfo(item);

         if(cinfo)
         {
            SetParent(cinfo->buddy, box);
            ShowWindow(cinfo->buddy, SW_SHOW);
            SendMessage(item, UDM_SETBUDDY, (WPARAM)cinfo->buddy, 0);
         }
      }
#ifdef TOOLBAR
      else if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
      {
#ifdef AEROGLASS	
         if(!(_dw_composition && (GetWindowLongPtr(_dw_toplevel_window(box), GWL_EXSTYLE) & WS_EX_LAYERED)))
#endif
         {
            /* Enable double buffering if our window isn't composited */
#ifdef TBSTYLE_EX_DOUBLEBUFFER
            SendMessage(item, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DOUBLEBUFFER);
#endif
         }
      }
#endif
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
   HWND parent = GetParent(handle);

   if(handle && parent != HWND_DESKTOP)
   {
      Box *thisbox = (Box *)GetWindowLongPtr(parent, GWLP_USERDATA);

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

         SetParent(handle, DW_HWND_OBJECT);
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
   Box *thisbox = (Box *)GetWindowLongPtr(box, GWLP_USERDATA);

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
         SetParent(handle, DW_HWND_OBJECT);
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
void _dw_get_window_for_size(HWND handle, unsigned long *width, unsigned long *height)
{
   Box *thisbox = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);

   if(thisbox)
   {
      int depth = 0;
      DWORD dwStyle = GetWindowLong(handle, GWL_STYLE);
      DWORD dwExStyle = GetWindowLong(handle, GWL_EXSTYLE);
      HMENU menu = GetMenu(handle) ;
      RECT rc = { 0 } ;

      /* Calculate space requirements */
      _dw_resize_box(thisbox, &depth, *width, *height, 0, 0, 1);

      rc.right = thisbox->minwidth;
      rc.bottom = thisbox->minheight;

      /* Take into account the window border and menu here */
      AdjustWindowRectEx(&rc, dwStyle, menu ? TRUE : FALSE, dwExStyle);

      if ( *width < 1 ) *width = rc.right - rc.left;
      if ( *height < 1 ) *height = rc.bottom - rc.top;
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
      _dw_get_window_for_size(handle, &width, &height);

   /* Finally set the size */
   SetWindowPos(handle, (HWND)NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
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
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, ClassName, _tcslen(ClassName)+1) == 0)
   {
      unsigned long thiswidth = 0, thisheight = 0;

      /* Get the size with the border */
      _dw_get_window_for_size(handle, &thiswidth, &thisheight);

      /* Return what was requested */
      if(width) *width = (int)thiswidth;
      if(height) *height = (int)thisheight;
   }
   else if(_tcsnicmp(tmpbuf, FRAMECLASSNAME, _tcslen(FRAMECLASSNAME)+1) == 0)
   {
      Box *thisbox = (Box *)GetWindowLongPtr(handle, GWLP_USERDATA);

      if(thisbox)
      {
         int depth = 0;

         /* Calculate space requirements */
         _dw_resize_box(thisbox, &depth, 0, 0, 0, 0, 1);

         /* Return what was requested */
         if(width) *width = thisbox->minwidth;
         if(height) *height = thisbox->minheight;
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
   return GetSystemMetrics(SM_CXSCREEN);
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
   return GetSystemMetrics(SM_CYSCREEN);
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
   int bpp;
   HDC hdc = GetDC(HWND_DESKTOP);

   bpp = GetDeviceCaps(hdc, BITSPIXEL);

   ReleaseDC(HWND_DESKTOP, hdc);

   return bpp;
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
   if(horz || vert)
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
      else if((vert & 0xf) == DW_GRAV_BOTTOM)
         newy = dw_screen_height() - height - *y;

      /* Save the new values */
      *x = newx;
      *y = newy;

       /* Adjust the values to avoid Taskbar if requested */
       if((horz | vert) & DW_GRAV_OBSTACLES)
       {
         POINT pt = { 0, 0 };
         HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
         MONITORINFO mi;

         mi.cbSize = sizeof(MONITORINFO);

         GetMonitorInfo(mon, &mi);

         if(horz & DW_GRAV_OBSTACLES)
         {
            if((horz & 0xf) == DW_GRAV_LEFT)
               *x += (mi.rcWork.left - mi.rcMonitor.left);
            else if((horz & 0xf) == DW_GRAV_RIGHT)
               *x -= (mi.rcMonitor.right - mi.rcWork.right);
         }
         if(vert & DW_GRAV_OBSTACLES)
         {
            if((vert & 0xf) == DW_GRAV_TOP)
               *y += (mi.rcWork.top - mi.rcMonitor.top);
            else if((vert & 0xf) == DW_GRAV_BOTTOM)
               *y -= (mi.rcMonitor.bottom - mi.rcWork.bottom);
         }
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
void API dw_window_set_pos(HWND handle, long x, long y)
{
   unsigned long width, height;
   RECT rect;

   GetClientRect(handle, &rect);

   /* Can't position an unsized window, so attempt to auto-size */
   if((rect.bottom - rect.top) == 0 || (rect.right - rect.left) == 0)
      dw_window_set_size(handle, 0, 0);

   dw_window_get_pos_size(handle, NULL, NULL, &width, &height);
   _dw_handle_gravity(handle, &x, &y, width, height);
   SetWindowPos(handle, (HWND)NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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
void API dw_window_set_pos_size(HWND handle, long x, long y, ULONG width, ULONG height)
{
   /* Attempt to auto-size */
   if ( width < 1 || height < 1 )
      _dw_get_window_for_size(handle, &width, &height);

   _dw_handle_gravity(handle, &x, &y, width, height);
   /* Finally set the size */
   SetWindowPos(handle, (HWND)NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
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
void API dw_window_get_pos_size(HWND handle, long *x, long *y, ULONG *width, ULONG *height)
{
   WINDOWPLACEMENT wp;

   wp.length = sizeof(WINDOWPLACEMENT);

   GetWindowPlacement(handle, &wp);
   if( wp.showCmd == SW_SHOWMAXIMIZED)
   {
      if(x)
         *x=0;
      if(y)
         *y=0;
      if(width)
         *width=dw_screen_width();
      if(height)
         *height=dw_screen_height();
   }
   else
   {
      if(x)
         *x = wp.rcNormalPosition.left;
      if(y)
         *y = wp.rcNormalPosition.top;
      if(width)
         *width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
      if(height)
         *height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
   }
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
   ULONG tmp, currentstyle;
   ColorInfo *cinfo;
   TCHAR tmpbuf[100] = {0};

   if(!handle || !mask)
      return;

   if(handle < (HWND)65536)
   {
      char buffer[31] = {0};
      HMENU mymenu;
      ULONG id = (ULONG)(uintptr_t)handle;

      _snprintf(buffer, 30, "_dw_id%ld", id);
      mymenu = (HMENU)dw_window_get_data(DW_HWND_OBJECT, buffer);

      if(mymenu && IsMenu(mymenu))
         dw_menu_item_set_state((HMENUI)mymenu, id, style & mask);
      return;
   }

   GetClassName(handle, tmpbuf, 99);

   currentstyle = GetWindowLong(handle, GWL_STYLE);
   cinfo = _dw_window_get_cinfo(handle);

#ifdef TOOLBAR
   /* Bitmap Buttons */
   if(_tcsnicmp(tmpbuf, TOOLBARCLASSNAME, _tcslen(TOOLBARCLASSNAME)+1) == 0)
   {
      ULONG thisstyle = (TBSTYLE_FLAT | TBSTYLE_TRANSPARENT);

      if(mask & DW_BS_NOBORDER)
      {
         SetWindowLong(handle, GWL_STYLE, (style & DW_BS_NOBORDER) ? (currentstyle | thisstyle) : (currentstyle & ~thisstyle));

         /* Enable or disable visual themese */
         if(_SetWindowTheme)
            _SetWindowTheme(handle, (style & DW_BS_NOBORDER) ? NULL : L"", (style & DW_BS_NOBORDER) ? NULL : L"");

         return;
      }
   }
#endif

   tmp = currentstyle | mask;
   tmp ^= mask;
   tmp |= style & mask;

   /* Drop out for status bar, it currently doesn't accept styles on Windows */
   if(_tcsnicmp(tmpbuf, StatusbarClassName, _tcslen(StatusbarClassName)+1)==0)
      return;
   else if(_tcsnicmp(tmpbuf, ClassName, _tcslen(ClassName)+1)==0)
   {
      tmp = tmp & 0xffff0000;
#ifdef AEROGLASS
      if(mask & DW_FCF_COMPOSITED && _DwmExtendFrameIntoClientArea && _dw_composition && !IS_WIN8PLUS)
      {
         if(style & DW_FCF_COMPOSITED)
         {
            MARGINS mar = {-1,-1,-1,-1};

            /* Attempt to enable Aero glass background on the entire window */
            SetLayeredWindowAttributes(handle, _dw_transparencykey, 0, LWA_COLORKEY);
            _DwmExtendFrameIntoClientArea(handle, &mar);
            _dw_show_margins(handle, mar, __LINE__);
         }
         else
         {
            MARGINS mar = {0};

#ifdef DARK_MODE_TITLEBAR_MENU
            if(cinfo)
               mar = _dw_rect_to_margins(cinfo->rect);
#endif

            /* Remove Aero Glass */
            SetLayeredWindowAttributes(handle, _dw_transparencykey, 255, LWA_ALPHA);
            _DwmExtendFrameIntoClientArea(handle, &mar);
            _dw_show_margins(handle, mar, __LINE__);
         }
      }
#endif
   }
   else if(_tcsnicmp(tmpbuf, STATICCLASSNAME, _tcslen(STATICCLASSNAME)+1)==0)
   {
      ULONG thismask = mask & ~(DW_DT_VCENTER | DW_DT_WORDBREAK);
      ULONG thisstyle = style & ~(DW_DT_VCENTER | DW_DT_WORDBREAK);
      ULONG type = style & mask & 0xFL;

      /* Need to filter out bits that shouldn't be set */
      tmp = currentstyle | thismask;
      tmp ^= thismask;
      tmp |= thisstyle & thismask;

      if(mask & DW_DT_VCENTER && style & DW_DT_VCENTER && !cinfo)
         cinfo = _dw_window_new_cinfo(handle, TRUE);

      /* Alignment style is 0 for word wrap */
      if((style & DW_DT_WORDBREAK) && (mask & DW_DT_WORDBREAK))
         tmp &= ~(0xFL);
      else if(type == SS_LEFTNOWORDWRAP)
         tmp = (tmp & ~(0xFL)) | SS_LEFTNOWORDWRAP;
      else if(type == SS_CENTER)
         tmp = (tmp & ~(0xFL)) | SS_CENTER;
      else if(type == SS_RIGHT)
         tmp = (tmp & ~(0xFL)) | SS_RIGHT;
   }

   /* If we have cinfo store the style there for later use */
   if(cinfo)
   {
      cinfo->style = cinfo->style | mask;
      cinfo->style ^= mask;
      cinfo->style |= style & mask;
   }

   SetWindowLong(handle, GWL_STYLE, tmp);
}

/* Finds the physical ID from the reference ID */
int _dw_findnotebookid(NotebookPage **array, int pageid)
{
   int z;

   for(z=0;z<256;z++)
   {
      if(array[z] && array[z]->realid == pageid)
         return z;
   }
   return -1;
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
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");

   if(array)
   {
      int z, refid = -1;

      for(z=0;z<256;z++)
      {
         if(_dw_findnotebookid(array, z) == -1)
         {
            refid = z;
            break;
         }
      }

      if(refid == -1)
         return -1;

      for(z=0;z<256;z++)
      {
         if(!array[z])
         {
            array[z] = calloc(1, sizeof(NotebookPage));
            array[z]->realid = refid;
            array[z]->item.mask = TCIF_TEXT;
            array[z]->item.iImage = -1;
            array[z]->item.pszText = TEXT("");
            TabCtrl_InsertItem(handle, z, &(array[z]->item));
            return refid;
         }
      }
   }
   return -1;
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_text(HWND handle, ULONG pageidx, const char *text)
{

   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
   int pageid;

   if(!array)
      return;

   pageid = _dw_findnotebookid(array, pageidx);

   if(pageid > -1 && array[pageid])
   {
      array[pageid]->item.mask = TCIF_TEXT;
      array[pageid]->item.pszText = UTF8toWide(text);
      TabCtrl_SetItem(handle, pageid, &(array[pageid]->item));
      _dw_resize_notebook_page(handle, pageid);
   }
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
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
void API dw_notebook_pack(HWND handle, ULONG pageidx, HWND page)
{
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
   int pageid;

   if(!array)
      return;

   pageid = _dw_findnotebookid(array, pageidx);

   if(pageid > -1 && array[pageid])
   {
      HWND tmpbox = dw_box_new(DW_VERT, 0);

      dw_box_pack_start(tmpbox, page, 0, 0, TRUE, TRUE, 0);
      if(array[pageid]->hwnd)
         dw_window_destroy(array[pageid]->hwnd);
      array[pageid]->hwnd = tmpbox;
      if(pageidx == dw_notebook_page_get(handle))
      {
         ShowWindow(tmpbox, SW_HIDE);
         SetParent(tmpbox, handle);
         _dw_resize_notebook_page(handle, pageid);
      }
   }
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned long pageidx)
{
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
   int newid = -1, z, pageid;
   HWND pagehwnd = 0;

   if(!array)
      return;

   pageid = _dw_findnotebookid(array, pageidx);

   if(pageid < 0)
      return;

   if(array[pageid])
   {
      SetParent(array[pageid]->hwnd, DW_HWND_OBJECT);
      pagehwnd = array[pageid]->hwnd;
      free(array[pageid]);
      array[pageid] = NULL;
   }

   TabCtrl_DeleteItem(handle, pageid);

   /* Shift the pages over 1 */
   for(z=pageid;z<255;z++)
      array[z] = array[z+1];
   array[255] = NULL;

   for(z=0;z<256;z++)
   {
      if(array[z])
      {
         newid = z;
         break;
      }
   }
   if(newid > -1)
   {
      SetParent(array[newid]->hwnd, handle);
      _dw_resize_notebook_page(handle, newid);
      dw_notebook_page_set(handle, array[newid]->realid);
   }
   if(pagehwnd)
      dw_window_destroy(pagehwnd);
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
   int physid = TabCtrl_GetCurSel(handle);

   if(physid > -1 && physid < 256 && array && array[physid])
      return array[physid]->realid;
   return -1;
}

/*
 * Sets the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned long pageidx)
{
   NotebookPage **array = (NotebookPage **)dw_window_get_data(handle, "_dw_array");
   int pageid;

   if(!array)
      return;

   pageid = _dw_findnotebookid(array, pageidx);

   if(pageid > -1 && pageid < 256)
   {
      int oldpage = TabCtrl_GetCurSel(handle);

      if(oldpage > -1 && array && array[oldpage])
         SetParent(array[oldpage]->hwnd, DW_HWND_OBJECT);

      TabCtrl_SetCurSel(handle, pageid);

      SetParent(array[pageid]->hwnd, handle);
      _dw_resize_notebook_page(handle, pageid);
   }
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, const char *text)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      SendMessage(handle,
               CB_ADDSTRING,
               0, (LPARAM)UTF8toWide(text));
   else
      SendMessage(handle,
               LB_ADDSTRING,
               0, (LPARAM)UTF8toWide(text));
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
   TCHAR tmpbuf[100] = {0};
   int listbox_type;
   int i;

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      listbox_type = CB_ADDSTRING;
   else
      listbox_type = LB_ADDSTRING;

   for(i=0;i<count;i++)
      SendMessage(handle,(WPARAM)listbox_type,0,(LPARAM)UTF8toWide(text[i]));
}

/*
 * Inserts the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 *          pos: 0 based position to insert text
 */
void API dw_listbox_insert(HWND handle, const char *text, int pos)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      SendMessage(handle,
               CB_INSERTSTRING,
               pos, (LPARAM)UTF8toWide(text));
   else
      SendMessage(handle,
               LB_INSERTSTRING,
               pos, (LPARAM)UTF8toWide(text));
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
   {
      char *buf = dw_window_get_text(handle);

      SendMessage(handle,
               CB_RESETCONTENT, 0L, 0L);

      if(buf)
      {
         dw_window_set_text(handle, buf);
         free(buf);
      }
   }
   else
      SendMessage(handle,
               LB_RESETCONTENT, 0L, 0L);
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
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
   {
      SendMessage(handle,  CB_DELETESTRING, (WPARAM)index, 0);
      SendMessage(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)buffer);
   }
   else
   {
      unsigned int sel = (unsigned int)SendMessage(handle, LB_GETCURSEL, 0, 0);
      SendMessage(handle,  LB_DELETESTRING, (WPARAM)index, 0);
      SendMessage(handle, LB_INSERTSTRING, (WPARAM)index, (LPARAM)buffer);
      SendMessage(handle, LB_SETCURSEL, (WPARAM)sel, 0);
      SendMessage(handle, LB_SETSEL, (WPARAM)TRUE, (LPARAM)sel);
   }
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
   TCHAR tmpbuf[100] = {0}, *wbuffer;
   unsigned int len;

   buffer[0] = 0;

   if(!buffer || !length)
      return;

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
   {
      len = (int)SendMessage(handle, CB_GETLBTEXTLEN, (WPARAM)index, 0);

      if(len != CB_ERR && (wbuffer = _alloca((len+1)*sizeof(TCHAR))))
      {
         SendMessage(handle, CB_GETLBTEXT, (WPARAM)index, (LPARAM)wbuffer);
         strncpy(buffer, WideToUTF8(wbuffer), length);
      }
   }
   else
   {
      len = (int)SendMessage(handle, LB_GETTEXTLEN, (WPARAM)index, 0);

      if(len != LB_ERR && (wbuffer = _alloca((len+1)*sizeof(TCHAR))))
      {
         SendMessage(handle, LB_GETTEXT, (WPARAM)index, (LPARAM)wbuffer);
         strncpy(buffer, WideToUTF8(wbuffer), length);
      }
   }
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
int API dw_listbox_selected(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      return (unsigned int)SendMessage(handle,
                               CB_GETCURSEL,
                               0, 0);

   return (unsigned int)SendMessage(handle,
                            LB_GETCURSEL,
                            0, 0);
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
   int *array, count, z;
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   /* This doesn't work on comboboxes */
   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      return -1;

   count = (int)SendMessage(handle, LB_GETSELCOUNT, 0, 0);
   if(count > 0)
   {
      array = malloc(sizeof(int)*count);
      SendMessage(handle, LB_GETSELITEMS, (WPARAM)count, (LPARAM)array);

      if(where == -1)
      {
         int ret = array[0];
         free(array);
         return ret;
      }
      for(z=0;z<count;z++)
      {
         if(array[z] == where && (z+1) < count)
         {
            int ret = array[z+1];
            free(array);
            return ret;
         }
      }
      free(array);
   }
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
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      SendMessage(handle, CB_SETCURSEL, (WPARAM)index, 0);
   else
   {
      SendMessage(handle, LB_SETCURSEL, (WPARAM)index, 0);
      SendMessage(handle, LB_SETSEL, (WPARAM)state, (LPARAM)index);
   }
   _dw_wndproc(handle, WM_COMMAND, (WPARAM)(LBN_SELCHANGE << 16), (LPARAM)handle);
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      SendMessage(handle, CB_DELETESTRING, (WPARAM)index, 0);
   else
      SendMessage(handle, LB_DELETESTRING, (WPARAM)index, 0);
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
int API dw_listbox_count(HWND handle)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      return (int)SendMessage(handle,
                        CB_GETCOUNT,0L, 0L);

   return (int)SendMessage(handle,
                     LB_GETCOUNT,0L, 0L);
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void API dw_listbox_set_top(HWND handle, int top)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   /* This doesn't work on comboboxes */
   if(_tcsnicmp(tmpbuf, COMBOBOXCLASSNAME, _tcslen(COMBOBOXCLASSNAME)+1)==0)
      return;

   SendMessage(handle, LB_SETTOPINDEX, (WPARAM)top, 0);
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
   int textlen, len = GetWindowTextLength(handle);
   TCHAR *tmpbuf, *srcbuf = UTF8toWide(buffer);

   if(startpoint < 0)
      startpoint = 0;

   if(!buffer || (textlen = (int)_tcslen(srcbuf)) < 1)
      return startpoint;

   tmpbuf = calloc(sizeof(TCHAR), len + textlen + startpoint + 2);

   if(len)
   {
      TCHAR *dest, *start;
      int copylen = len - startpoint;

      GetWindowText(handle, tmpbuf, len+1);

      dest = &tmpbuf[startpoint+textlen];
      start = &tmpbuf[startpoint];

      if(copylen > 0)
         memcpy(dest, start, copylen*sizeof(TCHAR));
   }
   memcpy(&tmpbuf[startpoint], srcbuf, textlen*sizeof(TCHAR));

   SetWindowText(handle, tmpbuf);

   free(tmpbuf);
   return (startpoint + textlen);
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported. MUST allow for trailing nul character.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
void API dw_mle_export(HWND handle, char *buffer, int startpoint, int length)
{
   int max, len = GetWindowTextLength(handle);
   TCHAR *tmpbuf = calloc(sizeof(TCHAR), len+2);

   if(len)
      GetWindowText(handle, tmpbuf, len+1);

   buffer[0] = 0;

   if(startpoint < len)
   {
      max = MIN(length, len - startpoint);

      memcpy(buffer, WideToUTF8(&tmpbuf[startpoint]), max);
      buffer[max] = '\0';
   }

   free(tmpbuf);
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
      *bytes = GetWindowTextLength(handle);
   if(lines)
      *lines = (unsigned long)SendMessage(handle, EM_GETLINECOUNT, 0, 0);
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
   int len = GetWindowTextLength(handle);
   TCHAR *tmpbuf = calloc(sizeof(TCHAR), len+2);

   GetWindowText(handle, tmpbuf, len+1);

   if(startpoint + length < len)
   {
      _tcscpy(&tmpbuf[startpoint], &tmpbuf[startpoint+length]);

      SetWindowText(handle, tmpbuf);
   }

   free(tmpbuf);
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
   SetWindowText(handle, TEXT(""));
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
   int point = (int)SendMessage(handle, EM_LINEINDEX, (WPARAM)line, 0);
   dw_mle_set_cursor(handle, point);
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
   SendMessage(handle, EM_SETREADONLY, (WPARAM)(state ? FALSE : TRUE), 0);
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
#ifdef RICHEDIT
   /* If it is a rich edit control use the rich edit message */
   if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
   {
      SendMessage(handle, EM_SHOWSCROLLBAR, (WPARAM)SB_HORZ, (LPARAM)(state ? FALSE : TRUE));
      SendMessage(handle, EM_SETTARGETDEVICE, 0, state ? 0 : 1);
   }
#endif   
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
   SendMessage(handle, EM_SETSEL, (WPARAM)point, (LPARAM)point);
#ifdef RICHEDIT
   if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
      SendMessage(handle, EM_HIDESELECTION, 0, 0);
#endif
   SendMessage(handle, EM_SCROLLCARET, 0, 0);
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
   int len = GetWindowTextLength(handle);
   TCHAR *tmpbuf = calloc(sizeof(TCHAR), len+2);
   TCHAR *searchtext = UTF8toWide(text);
   int z, textlen, retval = 0;

   GetWindowText(handle, tmpbuf, len+1);

   textlen = (int)strlen(text);

   if(flags & DW_MLE_CASESENSITIVE)
   {
      for(z=point;z<(len-textlen) && !retval;z++)
      {
         if(_tcsncmp(&tmpbuf[z], searchtext, textlen) == 0)
            retval = z + textlen;
      }
   }
   else
   {
      for(z=point;z<(len-textlen) && !retval;z++)
      {
         if(_tcsnicmp(&tmpbuf[z], searchtext, textlen) == 0)
            retval = z + textlen;
      }
   }

   if(retval)
   {
      SendMessage(handle, EM_SETSEL, (WPARAM)retval - textlen, (LPARAM)retval);
#ifdef RICHEDIT
      if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
         SendMessage(handle, EM_HIDESELECTION, 0, 0);
#endif
      SendMessage(handle, EM_SCROLLCARET, 0, 0);
   }

   free(tmpbuf);

   return retval;
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
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
   if(position == DW_PERCENT_INDETERMINATE)
   {
      /* If our common controls supports it... */
      if((_dwComctlVer >= PACKVERSION(6,0)))
      {
          /* Enable the style on the control */
          SetWindowLong(handle, GWL_STYLE, GetWindowLong(handle, GWL_STYLE) | PBS_MARQUEE);
          /* Start the bar going */
          SendMessage(handle, PBM_SETMARQUEE, 1, 100);
      }
      else
         SendMessage(handle, PBM_SETPOS, 0, 0);
   }
   else
   {
      if((_dwComctlVer >= PACKVERSION(6,0)))
      {
          unsigned long style = GetWindowLong(handle, GWL_STYLE);

          if(style & PBS_MARQUEE)
          {
             /* Stop the bar */
             SendMessage(handle, PBM_SETMARQUEE, 0, 0);
             /* Disable the style on the control */
             SetWindowLong(handle, GWL_STYLE, style & ~PBS_MARQUEE);
          }
      }
      /* Otherwise just set the position */
      SendMessage(handle, PBM_SETPOS, (WPARAM)position, 0);
   }
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
   int max = (int)SendMessage(handle, TBM_GETRANGEMAX, 0, 0);
   ULONG currentstyle = GetWindowLong(handle, GWL_STYLE);

   if(currentstyle & TBS_VERT)
      return max - (unsigned int)SendMessage(handle, TBM_GETPOS, 0, 0);
   return (unsigned int)SendMessage(handle, TBM_GETPOS, 0, 0);
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
   int max = (int)SendMessage(handle, TBM_GETRANGEMAX, 0, 0);
   ULONG currentstyle = GetWindowLong(handle, GWL_STYLE);

   if(currentstyle & TBS_VERT)
      SendMessage(handle, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)max - position);
   else
      SendMessage(handle, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)position);
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
   return (unsigned int)SendMessage(handle, SBM_GETPOS, 0, 0);
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
   dw_window_set_data(handle, "_dw_scrollbar_value", DW_UINT_TO_POINTER(position));
   SendMessage(handle, SBM_SETPOS, (WPARAM)position, (LPARAM)TRUE);
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
   SCROLLINFO si;

   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_RANGE | SIF_PAGE;
   si.nMin = 0;
   si.nMax = range - 1;
   si.nPage = visible;
   SendMessage(handle, SBM_SETSCROLLINFO, (WPARAM)TRUE, (LPARAM)&si);
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
   TCHAR tmpbuf[101] = {0};
   ColorInfo *cinfo = _dw_window_get_cinfo(handle);

   _sntprintf(tmpbuf, 100, TEXT("%ld"), position);

   if(cinfo && cinfo->buddy)
      SetWindowText(cinfo->buddy, tmpbuf);

   SendMessage(handle, UDM_SETPOS32, 0, (LPARAM)position);
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_limits(HWND handle, long upper, long lower)
{
   SendMessage(handle, UDM_SETRANGE32, (WPARAM)lower,(LPARAM)upper);
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
   SendMessage(handle, EM_SETLIMITTEXT, (WPARAM)limit, 0);
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
   return (long)SendMessage(handle, UDM_GETPOS32, 0, 0);
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int API dw_checkbox_get(HWND handle)
{
   if(SendMessage(handle, BM_GETCHECK, 0, 0) == BST_CHECKED)
      return (in_checkbox_handler ? FALSE : TRUE);
   return (in_checkbox_handler ? TRUE : FALSE);
}

/* This function unchecks all radiobuttons on a box */
BOOL CALLBACK _dw_uncheck_radios(HWND handle, LPARAM lParam)
{
   TCHAR tmpbuf[100] = {0};

   GetClassName(handle, tmpbuf, 99);

   if(_tcsnicmp(tmpbuf, BUTTONCLASSNAME, _tcslen(BUTTONCLASSNAME)+1)==0)
   {
      if(!dw_window_get_data(handle, "_dw_checkbox"))
         SendMessage(handle, BM_SETCHECK, 0, 0);
   }
   return TRUE;
}
/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
   if(!dw_window_get_data(handle, "_dw_checkbox"))
   {
      HWND parent = GetParent(handle);

      if(parent)
         EnumChildWindows(parent, _dw_uncheck_radios, 0);
   }
   SendMessage(handle, BM_SETCHECK, (WPARAM)value, 0);
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
   TVITEM tvi;
   TVINSERTSTRUCT tvins;
   HTREEITEM hti;

   tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
   tvi.pszText = UTF8toWide(title);
   tvi.lParam = (LPARAM)itemdata;
   tvi.cchTextMax = (int)_tcslen(tvi.pszText);
   tvi.iSelectedImage = tvi.iImage = _dw_lookup_icon(handle, (HICON)icon, 1);

   tvins.item = tvi;
   tvins.hParent = parent;
   tvins.hInsertAfter = item ? item : TVI_FIRST;

   hti = TreeView_InsertItem(handle, &tvins);

   return hti;
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
   TVITEM tvi;
   TVINSERTSTRUCT tvins;
   HTREEITEM hti;

   tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
   tvi.pszText = UTF8toWide(title);
   tvi.lParam = (LPARAM)itemdata;
   tvi.cchTextMax = (int)_tcslen(tvi.pszText);
   tvi.iSelectedImage = tvi.iImage = _dw_lookup_icon(handle, (HICON)icon, 1);

   tvins.item = tvi;
   tvins.hParent = parent;
   tvins.hInsertAfter = TVI_LAST;

   hti = TreeView_InsertItem(handle, &tvins);

   return hti;
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
   TVITEM tvi;

   tvi.mask = TVIF_HANDLE;
   tvi.hItem = item;

   if(TreeView_GetItem(handle, &tvi))
   {
      tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      tvi.pszText = UTF8toWide(title);
      tvi.cchTextMax = (int)_tcslen(tvi.pszText);
      tvi.iSelectedImage = tvi.iImage = _dw_lookup_icon(handle, (HICON)icon, 1);
      tvi.hItem = (HTREEITEM)item;

      TreeView_SetItem(handle, &tvi);
   }
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
   TVITEM tvi;

   tvi.mask = TVIF_HANDLE | TVIF_PARAM;
   tvi.hItem = item;
   tvi.lParam = (LPARAM)itemdata;

   TreeView_SetItem(handle, &tvi);
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
   TVITEM tvi;

   tvi.mask = TVIF_HANDLE | TVIF_PARAM;
   tvi.hItem = item;

   if(TreeView_GetItem(handle, &tvi))
   {
      return (void *)tvi.lParam;
   }
   return NULL;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
   TVITEM tvi;
   TCHAR textbuf[1025] = {0}, *textptr = textbuf;

   tvi.mask = TVIF_HANDLE | TVIF_TEXT;
   tvi.hItem = item;
   tvi.pszText = textptr;
   tvi.cchTextMax = 1024;

   if(TreeView_GetItem(handle, &tvi))
      return _strdup(WideToUTF8(textptr));
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
   return TreeView_GetParent(handle, item);
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
   TreeView_SelectItem(handle, item);
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
   dw_window_set_data(handle, "_dw_select_item", DW_INT_TO_POINTER(1));
   TreeView_DeleteAllItems(handle);
   dw_window_set_data(handle, "_dw_select_item", NULL);
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
   TreeView_Expand(handle, item, TVE_EXPAND);
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
   TreeView_Expand(handle, item, TVE_COLLAPSE);
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
   TreeView_DeleteItem(handle, item);
}

/*
 * Sets up the container columns.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 *          separator: The column number that contains the main separator.
 *                     (only used on OS/2 but must be >= 0 on all)
 */
int API dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator)
{
   ContainerInfo *cinfo = (ContainerInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);
   int z, l = 0;
   unsigned long *tempflags = calloc(sizeof(unsigned long), count + 2);
   LVCOLUMN lvc;

   if(separator == -1)
   {
      tempflags[0] = DW_CFA_RESERVED;
      l = 1;
   }

   memcpy(&tempflags[l], flags, sizeof(unsigned long) * count);
   tempflags[count + l] = 0;
   cinfo->flags = tempflags;
   cinfo->columns = count + l;


   for(z=0;z<count;z++)
   {
      if(titles[z])
      {
         lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
         lvc.pszText = UTF8toWide(titles[z]);
         lvc.cchTextMax = (int)_tcslen(lvc.pszText);
         if(flags[z] & DW_CFA_RIGHT)
            lvc.fmt = LVCFMT_RIGHT;
         else if(flags[z] & DW_CFA_CENTER)
            lvc.fmt = LVCFMT_CENTER;
         else
            lvc.fmt = LVCFMT_LEFT;
         lvc.cx = 75;
         lvc.iSubItem = count;
         SendMessage(handle, LVM_INSERTCOLUMN, (WPARAM)z + l, (LPARAM)&lvc);
      }
   }
   ListView_SetExtendedListViewStyle(handle, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
   return DW_ERROR_NONE;
}

/*
 * Configures the main filesystem column title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, const char *title)
{
    char *newtitle = _strdup(title ? title : "");

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
   char *coltitle = (char *)dw_window_get_data(handle, "_dw_coltitle");
   LV_COLUMN lvc;

   lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
   lvc.pszText = coltitle ? UTF8toWide(coltitle) : TEXT("Filename");
   lvc.cchTextMax = 8;
   lvc.fmt = 0;
   if(!count)
      lvc.cx = 300;
   else
      lvc.cx = 150;
   lvc.iSubItem = count;
   SendMessage(handle, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&lvc);
   dw_container_setup(handle, flags, titles, count, -1);
   if(coltitle)
   {
      dw_window_set_data(handle, "_dw_coltitle", NULL);
      free(coltitle);
   }
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
   return (HICN)LoadIcon(_DWInstance, MAKEINTRESOURCE(id));
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
#ifdef GDIPLUS
    return _dw_load_icon(filename);
#else
   char *file = malloc(strlen(filename) + 5);
   HANDLE icon;

   if(!file)
      return 0;

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if(access(file, 04) != 0)
   {
      /* Try with .ico extention */
      strcat(file, ".ico");
      if(access(file, 04) != 0)
      {
         free(file);
         return 0;
      }
   }
   icon = LoadImage(NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
   free(file);
   return (HICN)icon;
#endif
}

/*
 * Obtains an icon from data
 * Parameters:
 *       data: Source of icon data
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
   HANDLE icon = 0;
   char *file;
   FILE *fp;

   if ( !data )
      return 0;
   file = _tempnam( _dw_alternate_temp_dir, "dw" );
   if ( file != NULL )
   {
      fp = fopen( file, "wb" );
      if ( fp != NULL )
      {
         fwrite( data, 1, len, fp );
         fclose( fp );
#ifdef GDIPLUS
         icon = _dw_load_icon(file);
#else
         icon = LoadImage( NULL, UTF8toWide(file), IMAGE_ICON, 0, 0, LR_LOADFROMFILE );
#endif
      }
      else
      {
         _unlink( file );
         free( file );
         return 0;
      }
      _unlink( file );
      free( file );
   }
   return (HICN)icon;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
   DestroyIcon((HICON)handle);
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
   LV_ITEM lvi;
   int z, item;

   lvi.mask = LVIF_DI_SETITEM | LVIF_TEXT | LVIF_IMAGE;
   lvi.iSubItem = 0;
   /* Insert at the end */
   lvi.iItem = 1000000;
   lvi.pszText = TEXT("");
   lvi.cchTextMax = 1;
   lvi.iImage = -1;

   ShowWindow(handle, SW_HIDE);
   item = ListView_InsertItem(handle, &lvi);
   for(z=1;z<rowcount;z++)
      ListView_InsertItem(handle, &lvi);
   dw_window_set_data(handle, "_dw_insertitem", DW_INT_TO_POINTER(item));
   return (void *)handle;
}

/* Finds a icon in the table, otherwise it adds it to the table
 * and returns the index in the table.
 */
int _dw_lookup_icon(HWND handle, HICON hicon, int type)
{
   int z;
   static HWND lasthwnd = NULL;

   /* We can't add an invalid handle */
   if(!hicon)
      return -1;

   if(!_dw_image_list_small || !_dw_image_list_large)
   {
      _dw_image_list_small = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, ICON_INDEX_LIMIT, 0);
      _dw_image_list_large = ImageList_Create(32, 32, ILC_COLOR16 | ILC_MASK, ICON_INDEX_LIMIT, 0);
   }
   for(z=0;z<ICON_INDEX_LIMIT;z++)
   {
      if(!_dw_icon_lookup[z])
      {
         _dw_icon_lookup[z] = hicon;
         ImageList_AddIcon(_dw_image_list_small, hicon);
         ImageList_AddIcon(_dw_image_list_large, hicon);
         if(type)
         {
            TreeView_SetImageList(handle, _dw_image_list_small, TVSIL_NORMAL);
         }
         else
         {
            ListView_SetImageList(handle, _dw_image_list_small, LVSIL_SMALL);
            ListView_SetImageList(handle, _dw_image_list_large, LVSIL_NORMAL);
         }
         lasthwnd = handle;
         return z;
      }

      if(hicon == _dw_icon_lookup[z])
      {
         if(lasthwnd != handle)
         {
            if(type)
            {
               TreeView_SetImageList(handle, _dw_image_list_small, TVSIL_NORMAL);
            }
            else
            {
               ListView_SetImageList(handle, _dw_image_list_small, LVSIL_SMALL);
               ListView_SetImageList(handle, _dw_image_list_large, LVSIL_NORMAL);
            }
            lasthwnd = handle;
         }
         return z;
      }
   }
   return -1;
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
   LV_ITEM lvi;
   int item = 0;

   if(pointer)
   {
      item = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_insertitem"));
   }

   lvi.iItem = row + item;
   lvi.iSubItem = 0;
   lvi.mask = LVIF_DI_SETITEM | LVIF_IMAGE | LVIF_TEXT;
   lvi.pszText = UTF8toWide(filename);
   lvi.cchTextMax = (int)_tcslen(lvi.pszText);
   lvi.iImage = _dw_lookup_icon(handle, (HICON)icon, 0);

   ListView_SetItem(handle, &lvi);
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
   dw_container_set_item(handle, pointer, column + 1, row, data);
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
   ContainerInfo *cinfo = (ContainerInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);
   ULONG *flags;
   LV_ITEM lvi;
   TCHAR textbuffer[101] = {0};
   int item = 0;

   if(pointer)
   {
      item = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_insertitem"));
   }

   if(!cinfo || !cinfo->flags)
      return;

   flags = cinfo->flags;

   lvi.mask = LVIF_DI_SETITEM | LVIF_TEXT;
   lvi.iItem = row + item;
   lvi.iSubItem = column;
   lvi.pszText = textbuffer;
   lvi.cchTextMax = 0;

   if(flags[column] & DW_CFA_BITMAPORICON)
   {
      lvi.mask = LVIF_DI_SETITEM | LVIF_IMAGE;
      lvi.pszText = NULL;

      if(data)
      {
         HICON hicon = *((HICON *)data);

         lvi.iImage = _dw_lookup_icon(handle, hicon, 0);
      }
      else
         lvi.iImage = -1;
   }
   else if(flags[column] & DW_CFA_STRING && data)
   {
      char *tmp = *((char **)data);

      if(!tmp)
         tmp = "";

      lvi.pszText = UTF8toWide(tmp);
      lvi.cchTextMax = (int)_tcslen(lvi.pszText);
   }
   else if(flags[column] & DW_CFA_ULONG && data)
   {
      ULONG tmp = *((ULONG *)data);

      _sntprintf(textbuffer, 100, TEXT("%lu"), tmp);

      lvi.cchTextMax = (int)_tcslen(textbuffer);
   }
   else if(flags[column] & DW_CFA_DATE && data)
   {
      struct tm curtm;
      CDATE cdate = *((CDATE *)data);

      memset(&curtm, 0, sizeof(struct tm));

      /* Safety check... zero dates are crashing
       * Visual Studio 2008. -Brian
       */
      if(cdate.year > 1900 && cdate.year < 2100)
      {
          curtm.tm_mday = (cdate.day >= 0 && cdate.day < 32) ? cdate.day : 1;
          curtm.tm_mon = (cdate.month > 0 && cdate.month < 13) ? cdate.month - 1 : 0;
          curtm.tm_year = cdate.year - 1900;
          _tcsftime(textbuffer, 100, TEXT("%x"), &curtm);
      }

      lvi.cchTextMax = (int)_tcslen(textbuffer);
   }
   else if(flags[column] & DW_CFA_TIME && data)
   {
      struct tm curtm;
      CTIME ctime = *((CTIME *)data);

      curtm.tm_hour = (ctime.hours >= 0 && ctime.hours < 24) ? ctime.hours : 0;
      curtm.tm_min = (ctime.minutes >= 0 && ctime.minutes < 60) ? ctime.minutes : 0;
      curtm.tm_sec = (ctime.seconds >= 0 && ctime.seconds < 60) ? ctime.seconds : 0;

      _tcsftime(textbuffer, 100, TEXT("%X"), &curtm);

      lvi.cchTextMax = (int)_tcslen(textbuffer);
   }

   ListView_SetItem(handle, &lvi);
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
   dw_filesystem_set_item(handle, NULL, column, row, data);
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
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
   ContainerInfo *cinfo = (ContainerInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);
   ULONG *flags;
   int rc;

   if(!cinfo || !cinfo->flags)
      return 0;

   flags = cinfo->flags;

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
   return dw_container_get_column_type( handle, column + 1 );
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
    ContainerInfo *cinfo = (ContainerInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);

    /* Drop out on error */
    if(!cinfo)
        return;

    /* Create new brushes or remove if transparent */
    cinfo->odd = oddcolor;
    cinfo->even = evencolor;
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
   ListView_SetColumnWidth(handle, column, width);
}

/* Internal version that handles both types */
void _dw_container_set_row_data(HWND handle, void *pointer, int row, int type, void *data)
{
   LV_ITEM lvi;
   int item = 0;

   if(pointer)
   {
      item = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_insertitem"));
   }

   memset(&lvi, 0, sizeof(LV_ITEM));

   lvi.iItem = row + item;
   lvi.mask = LVIF_PARAM;

   if(ListView_GetItem(handle, &lvi))
   {
      void **params = (void **)lvi.lParam;
      void *newparam = data;

      /* Make sure we have our pointer array... */
      if(!params)
      {
         /* If not allocate it */
         params = (void **)calloc(2, sizeof(void *));
         lvi.lParam = (LPARAM)params;
         ListView_SetItem(handle, &lvi);
      }
      /* If type string, we need to duplicate the string...
       * freeing any existing string.
       */
      if(type == _DW_DATA_TYPE_STRING)
      {
         void *oldparam = params[type];

         params[type] = NULL;

         if(oldparam)
            free(oldparam);
         if(newparam)
            newparam = _strdup((char *)newparam);
      }
      /* Set the new data in the pointer array */
      params[type] = newparam;
   }
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
   _dw_container_set_row_data(pointer, pointer, row, _DW_DATA_TYPE_STRING, (void *)title);
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
   _dw_container_set_row_data(handle, NULL, row, _DW_DATA_TYPE_STRING, (void *)title);
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
   _dw_container_set_row_data(pointer, pointer, row, _DW_DATA_TYPE_POINTER, data);
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
   _dw_container_set_row_data(handle, NULL, row, _DW_DATA_TYPE_POINTER, data);
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
   ShowWindow(handle, SW_SHOW);
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
   ListView_DeleteAllItems(handle);
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
{
   int z, _index = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_index"));

   for(z=0;z<rowcount;z++)
   {
      ListView_DeleteItem(handle, 0);
   }
   if(rowcount > _index)
      dw_window_set_data(handle, "_dw_index", 0);
   else
      dw_window_set_data(handle, "_dw_index", DW_INT_TO_POINTER((_index - rowcount)));
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
   switch(direction)
   {
   case DW_SCROLL_TOP:
      ListView_Scroll(handle, 0, -10000000);
        break;
   case DW_SCROLL_BOTTOM:
      ListView_Scroll(handle, 0, 10000000);
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
   LV_ITEM lvi;
   int _index = ListView_GetNextItem(handle, -1, flags);
   void **params;
   int type = _DW_DATA_TYPE_STRING;
   char *retval = NULL;

   if(_index == -1)
      return retval;

   if(flags & DW_CR_RETDATA)
      type = _DW_DATA_TYPE_POINTER;

   memset(&lvi, 0, sizeof(LV_ITEM));

   lvi.iItem = _index;
   lvi.mask = LVIF_PARAM;

   ListView_GetItem(handle, &lvi);
   params = (void **)lvi.lParam;

   if(params)
   {
      if(type == _DW_DATA_TYPE_STRING && params[type])
         retval = _strdup((char *)params[type]);
      else
         retval = (char *)params[type];
   }

   dw_window_set_data(handle, "_dw_index", DW_INT_TO_POINTER(_index));
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
   LV_ITEM lvi;
   int _index = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_index"));
   void **params;
   int type = _DW_DATA_TYPE_STRING;
   char *retval = NULL;

   _index = ListView_GetNextItem(handle, _index, flags);

   if(_index == -1)
      return retval;

   if(flags & DW_CR_RETDATA)
      type = _DW_DATA_TYPE_POINTER;

   memset(&lvi, 0, sizeof(LV_ITEM));

   lvi.iItem = _index;
   lvi.mask = LVIF_PARAM;

   ListView_GetItem(handle, &lvi);
   params = (void **)lvi.lParam;

   if(params)
   {
      if(type == _DW_DATA_TYPE_STRING && params[type])
         retval = _strdup((char *)params[type]);
      else
         retval = (char *)params[type];
   }

   dw_window_set_data(handle, "_dw_index", DW_INT_TO_POINTER(_index));
   return retval;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor(HWND handle, const char *text)
{
   int index = ListView_GetNextItem(handle, -1, LVNI_ALL);

   while ( index != -1 )
   {
      LV_ITEM lvi;
      void **params;

      memset(&lvi, 0, sizeof(LV_ITEM));

      lvi.iItem = index;
      lvi.mask = LVIF_PARAM;

      ListView_GetItem( handle, &lvi );
      params = (void **)lvi.lParam;

      if ( params && params[_DW_DATA_TYPE_STRING] && strcmp( (char *)params[_DW_DATA_TYPE_STRING], text ) == 0 )
      {
         unsigned long width, height;

         ListView_SetItemState( handle, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
         dw_window_get_pos_size( handle, NULL, NULL, &width, &height);
         if(width < 10 || height < 10)
            dw_window_set_data( handle, "_dw_cursor", DW_INT_TO_POINTER(index) );
         ListView_EnsureVisible( handle, index, TRUE );
         return;
      }

      index = ListView_GetNextItem( handle, index, LVNI_ALL );
   }
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_cursor_by_data(HWND handle, void *data)
{
   int index = ListView_GetNextItem(handle, -1, LVNI_ALL);

   while ( index != -1 )
   {
      LV_ITEM lvi;
      void **params;

      memset(&lvi, 0, sizeof(LV_ITEM));

      lvi.iItem = index;
      lvi.mask = LVIF_PARAM;

      ListView_GetItem( handle, &lvi );
      params = (void **)lvi.lParam;

      if ( params && params[_DW_DATA_TYPE_POINTER] == data )
      {
         unsigned long width, height;

         ListView_SetItemState( handle, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
         dw_window_get_pos_size( handle, NULL, NULL, &width, &height);
         if(width < 10 || height < 10)
            dw_window_set_data( handle, "_dw_cursor", DW_INT_TO_POINTER(index) );
         ListView_EnsureVisible( handle, index, TRUE );
         return;
      }

      index = ListView_GetNextItem( handle, index, LVNI_ALL );
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
   int index = ListView_GetNextItem(handle, -1, LVNI_ALL);

   while(index != -1)
   {
      LV_ITEM lvi;
      void **params;

      memset(&lvi, 0, sizeof(LV_ITEM));

      lvi.iItem = index;
      lvi.mask = LVIF_PARAM;

      ListView_GetItem(handle, &lvi);
      params = (void **)lvi.lParam;

      if ( params && params[_DW_DATA_TYPE_STRING] && strcmp( (char *)params[_DW_DATA_TYPE_STRING], text ) == 0 )
      {
         int _index = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_index"));

         if(index < _index)
            dw_window_set_data(handle, "_dw_index", DW_INT_TO_POINTER((_index - 1)));

         ListView_DeleteItem(handle, index);
         return;
      }

      index = ListView_GetNextItem(handle, index, LVNI_ALL);
   }
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row_by_data(HWND handle, void *data)
{
   int index = ListView_GetNextItem(handle, -1, LVNI_ALL);

   while(index != -1)
   {
      LV_ITEM lvi;
      void **params;

      memset(&lvi, 0, sizeof(LV_ITEM));

      lvi.iItem = index;
      lvi.mask = LVIF_PARAM;

      ListView_GetItem(handle, &lvi);
      params = (void **)lvi.lParam;

      if ( params && params[_DW_DATA_TYPE_POINTER] == data )
      {
         int _index = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_index"));

         if(index < _index)
            dw_window_set_data(handle, "_dw_index", DW_INT_TO_POINTER((_index - 1)));

         ListView_DeleteItem(handle, index);
         return;
      }

      index = ListView_GetNextItem(handle, index, LVNI_ALL);
   }
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
   ContainerInfo *cinfo = (ContainerInfo *)GetWindowLongPtr(handle, GWLP_USERDATA);
   if(cinfo && cinfo->columns == 1)
   {
      ListView_SetColumnWidth(handle, 0, LVSCW_AUTOSIZE);
   }
   else if(cinfo && cinfo->columns > 1)
   {
      ULONG *flags = cinfo->flags, *columns = calloc(sizeof(ULONG), cinfo->columns);
      TCHAR *text = calloc(sizeof(TCHAR), 1024);
      unsigned int z;
      int index;

      /* Initialize with sizes of column labels */
      for(z=0;z<cinfo->columns;z++)
      {
         if(flags[z] & DW_CFA_BITMAPORICON)
            columns[z] = 5;
         else
         {
            LVCOLUMN lvc;

            lvc.mask = LVCF_TEXT;
            lvc.cchTextMax = 1023;
            lvc.pszText = text;

            if(ListView_GetColumn(handle, z, &lvc))
               columns[z] = ListView_GetStringWidth(handle, lvc.pszText);

            if(flags[z] & DW_CFA_RESERVED)
               columns[z] += 20;
         }
      }

      index = ListView_GetNextItem(handle, -1, LVNI_ALL);

      /* Query all the item texts */
      while(index != -1)
      {
         for(z=0;z<cinfo->columns;z++)
         {
            unsigned int width;

            ListView_GetItemText(handle, index, z, text, 1023);
            width = ListView_GetStringWidth(handle, text);

            /* Figure extra space for the icon for the first column */
            if(z == 0)
               width += 20;

            if(width > columns[z])
               columns[z] = width;
         }

         index = ListView_GetNextItem(handle, index, LVNI_ALL);
      }

      /* Set the new sizes... Microsoft says we need to add
       * padding to the calculated sized but does not give
       * a value.  Trial and error shows that 16 works for us.
       */
      for(z=0;z<cinfo->columns;z++)
         ListView_SetColumnWidth(handle, z, columns[z] + 16);

      free(columns);
      free(text);
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
   NOTIFYICONDATA tnid;

   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = handle;
   tnid.uID = (UINT)(uintptr_t)icon;
   tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
   tnid.uCallbackMessage = WM_USER+2;
   tnid.hIcon = (HICON)icon;
   if(bubbletext)
      _tcsncpy(tnid.szTip, UTF8toWide(bubbletext), sizeof(tnid.szTip) / sizeof(TCHAR));
   else
      tnid.szTip[0] = 0;

   Shell_NotifyIcon(NIM_ADD, &tnid);
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
   NOTIFYICONDATA tnid;

   tnid.cbSize = sizeof(NOTIFYICONDATA);
   tnid.hWnd = handle;
   tnid.uID = (UINT)(uintptr_t)icon;

   Shell_NotifyIcon(NIM_DELETE, &tnid);
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
   Box *newbox = calloc(sizeof(Box), 1);
   HWND tmp = CreateWindow(ObjectClassName,
                     NULL,
                     WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);
   newbox->pad = 0;
   newbox->type = 0;
   newbox->count = 0;
   newbox->grouphwnd = (HWND)NULL;
   newbox->cinfo.pOldProc = SubclassWindow(tmp, _dw_rendwndproc);
   newbox->cinfo.fore = newbox->cinfo.back = -1;
   SetWindowLongPtr( tmp, GWLP_USERDATA, (LONG_PTR)newbox );
   return tmp;
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
void API dw_render_redraw(HWND handle)
{
   InvalidateRect(handle, NULL, FALSE);
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
   HPEN hPen = TlsGetValue(_dw_hpen);
   HBRUSH hBrush = TlsGetValue(_dw_hbrush);
   COLORREF foreground;
#ifdef GDIPLUS
   GpBrush *brush = TlsGetValue(_dw_gpbrush);
   GpPen *pen = TlsGetValue(_dw_gppen);
   ARGB gpfore;
#endif

   value = _dw_internal_color(value);
   foreground = RGB(DW_RED_VALUE(value), DW_GREEN_VALUE(value), DW_BLUE_VALUE(value));
#ifdef GDIPLUS
   gpfore = MAKEARGB(255, DW_RED_VALUE(value), DW_GREEN_VALUE(value), DW_BLUE_VALUE(value));

   GdipDeletePen(pen);
   GdipCreatePen1(gpfore, 1.0, UnitPixel, &pen);
   TlsSetValue(_dw_gppen, (LPVOID)pen);
   GdipSetSolidFillColor(brush, gpfore);
#endif

   DeleteObject(hPen);
   DeleteObject(hBrush);
   TlsSetValue(_dw_foreground, (LPVOID)(uintptr_t)foreground);
   TlsSetValue(_dw_hpen, CreatePen(PS_SOLID, 1, foreground));
   TlsSetValue(_dw_hbrush, CreateSolidBrush(foreground));
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
   COLORREF background;

   value = _dw_internal_color(value);
   background = RGB(DW_RED_VALUE(value), DW_GREEN_VALUE(value), DW_BLUE_VALUE(value));

   if(value == DW_RGB_TRANSPARENT)
      TlsSetValue(_dw_background, (LPVOID)DW_RGB_TRANSPARENT);
   else
      TlsSetValue(_dw_background, (LPVOID)(uintptr_t)background);
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
   CHOOSECOLOR cc;
   unsigned long newcolor;
   COLORREF acrCustClr[16] = {0};

   value = _dw_internal_color(value);
   if(value == DW_RGB_TRANSPARENT)
      newcolor = DW_RGB_TRANSPARENT;
   else
      newcolor = RGB(DW_RED_VALUE(value), DW_GREEN_VALUE(value), DW_BLUE_VALUE(value));
   ZeroMemory(&cc, sizeof(CHOOSECOLOR));
   cc.lStructSize = sizeof(CHOOSECOLOR);
   cc.rgbResult = newcolor;
   cc.hwndOwner = HWND_DESKTOP;
   cc.lpCustColors = (LPDWORD)acrCustClr;
   cc.Flags = CC_FULLOPEN | CC_RGBINIT;
   if (ChooseColor(&cc) == TRUE)
      newcolor = DW_RGB(DW_RED_VALUE(cc.rgbResult), DW_GREEN_VALUE(cc.rgbResult), DW_BLUE_VALUE(cc.rgbResult));
   return newcolor;
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
#ifdef GDIPLUS
   /* There doesn't seem to be an equivalent to SetPixel in GDI+ ...
    * so instead we call dw_draw_rect() with 1 for width and height.
    */
   dw_draw_rect(handle, pixmap, DW_DRAW_FILL | DW_DRAW_NOAA, x, y, 1, 1);
#else
   HDC hdcPaint;

   if(handle && _dw_render_safe_check(handle))
      hdcPaint = GetDC(handle);
   else if(pixmap)
      hdcPaint = pixmap->hdc;
   else
      return;

   SetPixel(hdcPaint, x, y, (COLORREF)TlsGetValue(_dw_foreground));
   if(!pixmap)
      ReleaseDC(handle, hdcPaint);
#endif
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
#ifdef GDIPLUS
   GpGraphics *graphics = NULL;
   GpPen *pen = TlsGetValue(_dw_gppen);

   if(handle && _dw_render_safe_check(handle))
      GdipCreateFromHWND(handle, &graphics);
   else if(pixmap)
      GdipCreateFromHDC(pixmap->hdc, &graphics);
   else
      return;

   GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);
   GdipDrawLineI(graphics, pen, x1, y1, x2, y2);
   GdipDeleteGraphics(graphics);
#else
   HDC hdcPaint;
   HPEN oldPen;

   if(handle && _dw_render_safe_check(handle))
      hdcPaint = GetDC(handle);
   else if(pixmap)
      hdcPaint = pixmap->hdc;
   else
      return;

   oldPen = SelectObject(hdcPaint, TlsGetValue(_dw_hpen));
   MoveToEx(hdcPaint, x1, y1, NULL);
   LineTo(hdcPaint, x2, y2);
   SelectObject(hdcPaint, oldPen);
   /* For some reason Win98 (at least) fails
    * to draw the last pixel.  So I do it myself.
    */
   SetPixel(hdcPaint, x2, y2, (COLORREF)TlsGetValue(_dw_foreground));
   if(!pixmap)
      ReleaseDC(handle, hdcPaint);
#endif
}

/* Internal function to generate POINT arrays used by Windows APIs */
POINT *_makePoints(int *npoints, int *x, int *y)
{
   POINT *points;
   int i;

   /*
    * Allocate enough space for the number of points supplied plus 1.
    * Under windows, unless the first and last points are the same
    * the polygon won't be closed
    */
   points = (POINT *)malloc( ((*npoints)+1) * sizeof(POINT) );

   if(points)
   {
      for ( i = 0 ; i < *npoints ; i++ )
      {
         points[i].x = x[i];
         points[i].y = y[i];
      }
      if ( !( points[0].x == points[(*npoints)-1].x
      &&   points[0].y == points[(*npoints)-1].y ) )
      {
         /* set the last point to be the same as the first point... */
         points[*npoints].x = points[0].x;
         points[*npoints].y = points[0].y;
         /* ... and increment the number of points */
         (*npoints)++;
      }
   }
   return points;
}

/* Draw a closed polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       number of points
 *       x[]: X coordinates.
 *       y[]: Y coordinates.
 */
void API dw_draw_polygon(HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
{
   POINT *points = NULL;

   /* Sanity check */
   if(npoints < 1)
      return;

#ifdef GDIPLUS
   {
      GpGraphics *graphics = NULL;

      if(handle && _dw_render_safe_check(handle))
         GdipCreateFromHWND(handle, &graphics);
      else if(pixmap)
         GdipCreateFromHDC(pixmap->hdc, &graphics);
      else
         return;

      /* Enable antialiasing if the DW_DRAW_NOAA flag isn't set */
      if(!(flags & DW_DRAW_NOAA))
        GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);

      if((points = _makePoints(&npoints, x, y)))
      {
         if(flags & DW_DRAW_FILL)
         {
            GpBrush *brush = TlsGetValue(_dw_gpbrush);

            GdipFillPolygon2I(graphics, brush, points, npoints);
         }
         else
         {
            GpPen *pen = TlsGetValue(_dw_gppen);

            GdipDrawPolygonI(graphics, pen, points, npoints);
         }
      }
      GdipDeleteGraphics(graphics);
   }
#else
   {
      HDC hdcPaint;

      if(handle && _dw_render_safe_check(handle))
         hdcPaint = GetDC( handle );
      else if(pixmap)
         hdcPaint = pixmap->hdc;
      else
         return;

      if((points = _makePoints(&npoints, x, y)))
      {
         HBRUSH oldBrush = SelectObject( hdcPaint, TlsGetValue(_dw_hbrush) );
         HPEN oldPen = SelectObject( hdcPaint, TlsGetValue(_dw_hpen) );

         if ( flags & DW_DRAW_FILL )
            Polygon( hdcPaint, points, npoints );
         else
            Polyline( hdcPaint, points, npoints );

         SelectObject( hdcPaint, oldBrush );
         SelectObject( hdcPaint, oldPen );
      }

      if ( !pixmap )
         ReleaseDC( handle, hdcPaint );
   }
#endif
   if(points)
      free(points);
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
#ifdef GDIPLUS
   GpGraphics *graphics = NULL;

   if(handle && _dw_render_safe_check(handle))
      GdipCreateFromHWND(handle, &graphics);
   else if(pixmap)
      GdipCreateFromHDC(pixmap->hdc, &graphics);
   else
      return;

   /* Enable antialiasing if the DW_DRAW_NOAA flag isn't set */
   if(!(flags & DW_DRAW_NOAA))
     GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);

   if(flags & DW_DRAW_FILL)
   {
      GpBrush *brush = TlsGetValue(_dw_gpbrush);

      GdipFillRectangleI(graphics, brush, x, y, width, height);
   }
   else
   {
      GpPen *pen = TlsGetValue(_dw_gppen);

      GdipDrawRectangleI(graphics, pen, x, y, width, height);
   }
   GdipDeleteGraphics(graphics);
#else
   HDC hdcPaint;
   RECT Rect;

   if(handle && _dw_render_safe_check(handle))
      hdcPaint = GetDC(handle);
   else if(pixmap)
      hdcPaint = pixmap->hdc;
   else
      return;

   SetRect(&Rect, x, y, x + width , y + height );
   if(flags & DW_DRAW_FILL)
      FillRect(hdcPaint, &Rect, TlsGetValue(_dw_hbrush));
   else
      FrameRect(hdcPaint, &Rect, TlsGetValue(_dw_hbrush));
   if(!pixmap)
      ReleaseDC(handle, hdcPaint);
#endif
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
#ifdef GDIPLUS
   GpGraphics *graphics = NULL;
   GpPen *pen = TlsGetValue(_dw_gppen);

   if(handle && _dw_render_safe_check(handle))
      GdipCreateFromHWND(handle, &graphics);
   else if(pixmap)
      GdipCreateFromHDC(pixmap->hdc, &graphics);
   else
      return;

   /* Enable antialiasing if the DW_DRAW_NOAA flag isn't set */
   if(!(flags & DW_DRAW_NOAA))
     GdipSetSmoothingMode(graphics, SmoothingModeAntiAlias);

   if(flags & DW_DRAW_FULL)
   {
      if(flags & DW_DRAW_FILL)
      {
         GpBrush *brush = TlsGetValue(_dw_gpbrush);

         GdipFillEllipseI(graphics, brush, x1 < x2 ? x1 : x2, y1 < y2 ? y1 : y2, abs(x1-x2), abs(y1-y2));
      }
      else
         GdipDrawEllipseI(graphics, pen, x1 < x2 ? x1 : x2, y1 < y2 ? y1 : y2, abs(x1-x2), abs(y1-y2));
   }
   else
   {
      double a1 = atan2((y1-yorigin), (x1-xorigin));
      double a2 = atan2((y2-yorigin), (x2-xorigin));
      double dx = xorigin - x1;
      double dy = yorigin - y1;
      double r = sqrt(dx*dx + dy*dy);
      double sweep;
      int ri = (int)r;

      /* Convert to degrees */
      a1 *= (180.0 / M_PI);
      a2 *= (180.0 / M_PI);
      sweep = fabs(a1 - a2);

      GdipDrawArcI(graphics, pen, xorigin-ri, yorigin-ri, ri*2, ri*2, (REAL)a1, (REAL)sweep);
   }
   GdipDeleteGraphics(graphics);
#else
   HDC hdcPaint;
   HBRUSH oldBrush;
   HPEN oldPen;
   double dx = xorigin - x1;
   double dy = yorigin - y1;
   double r = sqrt(dx*dx + dy*dy);
   int ri = (int)r;

   if(handle && _dw_render_safe_check(handle))
      hdcPaint = GetDC(handle);
   else if(pixmap)
      hdcPaint = pixmap->hdc;
   else
      return;

   if(flags & DW_DRAW_FILL)
      oldBrush = SelectObject( hdcPaint, TlsGetValue(_dw_hbrush) );
   else
      oldBrush = SelectObject( hdcPaint, GetStockObject(HOLLOW_BRUSH) );
   oldPen = SelectObject( hdcPaint, TlsGetValue(_dw_hpen) );
   if(flags & DW_DRAW_FULL)
      Ellipse(hdcPaint, x1, y1, x2, y2);
   else
      Arc(hdcPaint, xorigin-ri, yorigin-ri, xorigin+ri, yorigin+ri, x2, y2, x1, y1);
   SelectObject( hdcPaint, oldBrush );
   SelectObject( hdcPaint, oldPen );

   if(!pixmap)
      ReleaseDC(handle, hdcPaint);
#endif
}

#ifdef GDIPLUS
/* Internal function to increase or decrease coordinates/sizes
 * by the difference of the screen DPI (96) and the context DPI.
 */
void _dw_convert_dpi(HDC hdc, int *x, int *y, int mult)
{
   int ratiox = (int)GetDeviceCaps(hdc, LOGPIXELSX)/96;
   int ratioy = (int)GetDeviceCaps(hdc, LOGPIXELSY)/96;
   if(ratiox > 1 && ratioy > 1)
   {
      if(mult)
      {
         if(x)
            *x *= ratiox;
         if(y)
            *y *= ratioy;
      }
      else
      {
         if(x)
            *x /= ratiox;
         if(y)
            *y /= ratioy;
      }
   }
}
#endif

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
   HDC hdc;
   int mustdelete = 0;
   HFONT hFont = 0, oldFont = 0;
   ColorInfo *cinfo = NULL;
   COLORREF background;
   TCHAR *wtext = UTF8toWide(text);

   if(handle && _dw_render_safe_check(handle))
      hdc = GetDC(handle);
   else if(pixmap)
      hdc = pixmap->hdc;
   else
      return;

   if(handle)
      cinfo = _dw_window_get_cinfo(handle);
   else if(pixmap->font)
      hFont = pixmap->font;
   else if(pixmap->handle)
      cinfo = _dw_window_get_cinfo(pixmap->handle);

   if(cinfo)
   {
      hFont = _dw_acquire_font(handle, cinfo->fontname);
      mustdelete = 1;
   }

   background = (COLORREF)(uintptr_t)TlsGetValue(_dw_background);
   if(hFont)
      oldFont = SelectObject(hdc, hFont);
   SetTextColor(hdc, (COLORREF)(uintptr_t)TlsGetValue(_dw_foreground));
   if(background == DW_RGB_TRANSPARENT)
      SetBkMode(hdc, TRANSPARENT);
   else
   {
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, background);
   }
#ifdef GDIPLUS
   _dw_convert_dpi(hdc, &x, &y, TRUE);
#endif
   TextOut(hdc, x, y, wtext, (int)_tcslen(wtext));
   if(oldFont)
      SelectObject(hdc, oldFont);
   if(mustdelete)
      DeleteObject(hFont);
   if(!pixmap)
      ReleaseDC(handle, hdc);
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
   HDC hdc;
   int mustdelete = 0;
   HFONT hFont = NULL, oldFont;
   SIZE sz;
   TCHAR *wtext = UTF8toWide(text);

   if(handle)
      hdc = GetDC(handle);
   else if(pixmap)
      hdc = pixmap->hdc;
   else
      return;

   if(pixmap && pixmap->font)
   {
       hFont = pixmap->font;
   }
   else
   {
      ColorInfo *cinfo = NULL;

      if(handle)
         cinfo = _dw_window_get_cinfo(handle);
      else if(pixmap && pixmap->handle)
         cinfo = _dw_window_get_cinfo(pixmap->handle);

      if(cinfo)
      {
         hFont = _dw_acquire_font(handle, cinfo->fontname);
         mustdelete = 1;
      }
   }
   oldFont = SelectObject(hdc, hFont);

   GetTextExtentPoint32(hdc, wtext, (int)_tcslen(wtext), &sz);

   if(width)
      *width = sz.cx;

   if(height)
      *height = sz.cy;

#ifdef GDIPLUS
   _dw_convert_dpi(hdc, width, height, FALSE);
#endif

   SelectObject(hdc, oldFont);
   if(mustdelete)
      DeleteObject(hFont);
   if(!pixmap)
      ReleaseDC(handle, hdc);
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
   HPIXMAP pixmap;
   HDC hdc;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   hdc = GetDC(handle);

   pixmap->width = width; pixmap->height = height;

   pixmap->handle = handle;
   pixmap->hbm = CreateCompatibleBitmap(hdc, width, height);
   pixmap->hdc = CreateCompatibleDC(hdc);
   pixmap->transcolor = DW_RGB_TRANSPARENT;

   SelectObject(pixmap->hdc, pixmap->hbm);

   ReleaseDC(handle, hdc);

   return pixmap;
}

#ifndef GDIPLUS
/* Read the file bitmap header ourselves...
 * apparently we can't check the depth once loaded...
 * since it seems to normalize it to our screen depth.
 */
unsigned long _dw_read_bitmap_header(char *file)
{
    BITMAPFILEHEADER header;
    BITMAPINFO *info;
    FILE *fp;
    int infosize;
    int depth = 0;

    /* Try opening the file; use "rb" mode to read this *binary* file. */
    if((fp = fopen(file, "rb")) == NULL)
        return 0;

    /* Read the file header and any following bitmap information... */
    if(fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
    {
        /* Couldn't read the file header */
        fclose(fp);
        return 0;
    }

    if(header.bfType != 'MB') /* Check for BM reversed... */
    {
        /* Not a bitmap file */
        fclose(fp);
        return 0;
    }

    infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);
    if((info = (BITMAPINFO *)calloc(infosize, 1)) == NULL)
    {
        /* Couldn't allocate memory for bitmap info */
        fclose(fp);
        return 0;
    }

    if(fread(info, 1, infosize, fp) == infosize)
    {
        /* Read the bitmap header */
        depth = info->bmiHeader.biBitCount;
    }
    free(info);
    fclose(fp);
    return depth;
}
#endif

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
   BITMAP bm;
   HDC hdc;
#ifndef GDIPLUS
   char *file;
#endif

   if (!filename || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

#ifdef GDIPLUS
   pixmap->hbm = _dw_load_bitmap(filename, &pixmap->depth);
#else
   file = _alloca(strlen(filename) + 5);
   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if(access(file, 04) != 0)
   {
      /* Try with .bmp extention */
      strcat(file, ".bmp");
      if(access(file, 04) != 0)
      {
         free(pixmap);
         free(file);
         return NULL;
      }
   }

   pixmap->hbm = (HBITMAP)LoadImage(NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
   pixmap->depth = _dw_read_bitmap_header(file);
#endif

   pixmap->handle = handle;

   if ( !pixmap->hbm )
   {
      free(pixmap);
      return NULL;
   }

   hdc = GetDC(handle);

   pixmap->hdc = CreateCompatibleDC( hdc );
   GetObject( pixmap->hbm, sizeof(bm), &bm );
   pixmap->width = bm.bmWidth; pixmap->height = bm.bmHeight;
   SelectObject( pixmap->hdc, pixmap->hbm );
   ReleaseDC( handle, hdc );
   pixmap->transcolor = DW_RGB_TRANSPARENT;

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
   BITMAP bm;
   HDC hdc;
   char *file;
   FILE *fp;

   if ( !(pixmap = calloc(1,sizeof(struct _hpixmap))) )
   {
      return NULL;
   }

   hdc = GetDC(handle);

   pixmap->handle = handle;

   file = _tempnam( _dw_alternate_temp_dir, "dw" );
   if ( file != NULL )
   {
      fp = fopen( file, "wb" );
      if ( fp != NULL )
      {
         fwrite( data, 1, len, fp );
         fclose( fp );
#ifdef GDIPLUS
         pixmap->hbm = _dw_load_bitmap(file, NULL);
#else
         pixmap->hbm = (HBITMAP)LoadImage( NULL, UTF8toWide(file), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
         pixmap->depth = _dw_read_bitmap_header(file);
#endif
      }
      else
      {
         _unlink( file );
         free( file );
         free( pixmap );
         return NULL;
      }
      _unlink( file );
      free( file );
   }

   if ( !pixmap->hbm )
   {
      free( pixmap );
      ReleaseDC( handle, hdc );
      return NULL;
   }

   pixmap->hdc = CreateCompatibleDC( hdc );

   GetObject( pixmap->hbm, sizeof(bm), &bm );

   pixmap->width = bm.bmWidth; pixmap->height = bm.bmHeight;

   SelectObject( pixmap->hdc, pixmap->hbm );

   ReleaseDC( handle, hdc );
   pixmap->transcolor = DW_RGB_TRANSPARENT;

   return pixmap;
}

/*
 * Creates a bitmap mask for rendering bitmaps with transparent backgrounds
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
   if ( pixmap && pixmap->depth < 32)
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
   HPIXMAP pixmap;
   BITMAP bm;
   HDC hdc;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   hdc = GetDC(handle);

   pixmap->hbm = LoadBitmap(_DWInstance, MAKEINTRESOURCE(id));
   pixmap->hdc = CreateCompatibleDC(hdc);

   GetObject(pixmap->hbm, sizeof(BITMAP), (void *)&bm);

   pixmap->width = bm.bmWidth; pixmap->height = bm.bmHeight;
   pixmap->depth = bm.bmBitsPixel;

   SelectObject(pixmap->hdc, pixmap->hbm);

   ReleaseDC(handle, hdc);
   pixmap->transcolor = DW_RGB_TRANSPARENT;

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
    if(pixmap)
    {
        HFONT hfont = _dw_acquire_font2(pixmap->hdc, fontname);

        if(hfont)
        {
            HFONT oldfont = pixmap->font;
            pixmap->font = hfont;
            if(oldfont)
                DeleteObject(oldfont);
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
      DeleteDC(pixmap->hdc);
      DeleteObject(pixmap->hbm);
      if(pixmap->font)
        DeleteObject(pixmap->font);
      free(pixmap);
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
   HDC hdcdest;
   HDC hdcsrc;
   static BLENDFUNCTION bf = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };
   int swidth = srcwidth, sheight = srcheight;

   /* Do some sanity checks */
   if(dest && _dw_render_safe_check(dest))
      hdcdest = GetDC(dest);
   else if(destp)
      hdcdest = destp->hdc;
   else
      return DW_ERROR_GENERAL;

   if(src)
      hdcsrc = GetDC(src);
   else if(srcp)
      hdcsrc = srcp->hdc;
   else
      return DW_ERROR_GENERAL;

   if((srcheight == -1 || srcwidth == -1) && srcheight != srcwidth)
      return DW_ERROR_GENERAL;

   if(srcheight == -1 && srcwidth == -1)
   {
       swidth = width;
       sheight = height;
   }

#ifdef GDIPLUS
   /* Do conversion on all the coordinates */
   _dw_convert_dpi(hdcdest, &xdest, &ydest, TRUE);
   _dw_convert_dpi(hdcdest, &width, &height, TRUE);
   _dw_convert_dpi(hdcsrc, &xsrc, &ysrc, TRUE);
   _dw_convert_dpi(hdcsrc, &swidth, &sheight, TRUE);
#endif

   /* If it is a 32bpp bitmap (with alpha) use AlphaBlend unless it fails */
   if(srcp && srcp->depth == 32 && AlphaBlend(hdcdest, xdest, ydest, width, height, hdcsrc, xsrc, ysrc, swidth, sheight, bf))
   {
        /* Don't do anything */
   }
   /* Otherwise perform special bitblt with manual transparency */
   else if(srcp && srcp->transcolor != DW_RGB_TRANSPARENT)
   {
      TransparentBlt( hdcdest, xdest, ydest, width, height, hdcsrc, xsrc, ysrc, swidth, sheight, RGB( DW_RED_VALUE(srcp->transcolor), DW_GREEN_VALUE(srcp->transcolor), DW_BLUE_VALUE(srcp->transcolor)) );
   }
   else
   {
      /* Finally fall back to the classic BitBlt */
      if(srcwidth == -1 && srcheight == -1)
         BitBlt(hdcdest, xdest, ydest, width, height, hdcsrc, xsrc, ysrc, SRCCOPY);
      else
         StretchBlt(hdcdest, xdest, ydest, width, height, hdcsrc, xsrc, ysrc, swidth, sheight, SRCCOPY);
   }
   if(!destp)
      ReleaseDC(dest, hdcdest);
   if(!srcp)
      ReleaseDC(src, hdcsrc);

   return DW_ERROR_NONE;
}

/* Run Beep() in a separate thread so it doesn't block */
void _dw_beep_thread(void *data)
{
   int *info = (int *)data;

   if(data)
   {
      Beep(info[0], info[1]);
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

      _beginthread(_dw_beep_thread, 100, (void *)info);
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
   if(!handle)
      return DW_ERROR_UNKNOWN;

   *handle = LoadLibrary(UTF8toWide(name));
   return (NULL == *handle);
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
      return DW_ERROR_UNKNOWN;

   if(0 == strlen(name))
      return DW_ERROR_UNKNOWN;

   *func = (void*)GetProcAddress(handle, name);
   return   (NULL == *func);
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int API dw_module_close(HMOD handle)
{
   return FreeLibrary(handle);
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
{
   return (HMTX)CreateMutex(NULL, FALSE, NULL);
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
   CloseHandle((HANDLE)mutex);
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_lock(HMTX mutex)
{
   if(_dwtid == dw_thread_id())
   {
      int rc = WaitForSingleObject((HANDLE)mutex, 0);

      while(rc == WAIT_TIMEOUT)
      {
         dw_main_sleep(1);
         rc = WaitForSingleObject((HANDLE)mutex, 0);
      }
   }
    else
      WaitForSingleObject((HANDLE)mutex, INFINITE);
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
    if(WaitForSingleObject((HANDLE)mutex, 0) == WAIT_OBJECT_0)
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
   ReleaseMutex((HANDLE)mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
{
    return CreateEvent(NULL, TRUE, FALSE, NULL);
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_reset(HEV eve)
{
   if(ResetEvent(eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_post(HEV eve)
{
   if(SetEvent(eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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

   rc = WaitForSingleObject(eve, timeout != -1 ? timeout : INFINITE);
   if(rc == WAIT_OBJECT_0)
      return DW_ERROR_NONE;
   if(rc == WAIT_TIMEOUT)
      return DW_ERROR_TIMEOUT;
   if(rc == WAIT_ABANDONED)
      return DW_ERROR_INTERRUPT;
   return DW_ERROR_GENERAL;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_close(HEV *eve)
{
   if(eve && CloseHandle(*eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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
   SECURITY_ATTRIBUTES sa;

   sa.nLength = sizeof( SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = &_dwsd;
   sa.bInheritHandle = FALSE;

   return CreateEvent(&sa, TRUE, FALSE, UTF8toWide(name));
}

/* Destroy this semaphore.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              a create call.
 */
HEV API dw_named_event_get(const char *name)
{
   return OpenEvent(EVENT_ALL_ACCESS, FALSE, UTF8toWide(name));
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_reset(HEV eve)
{
   if(ResetEvent(eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_post(HEV eve)
{
   if(SetEvent(eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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

   rc = WaitForSingleObject(eve, timeout != -1 ? timeout : INFINITE);
   if(rc == WAIT_OBJECT_0)
      return DW_ERROR_NONE;
   if(rc == WAIT_TIMEOUT)
      return DW_ERROR_TIMEOUT;
   if(rc == WAIT_ABANDONED)
      return DW_ERROR_INTERRUPT;
   return DW_ERROR_GENERAL;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_close(HEV eve)
{
   if(CloseHandle(eve))
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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
   SECURITY_ATTRIBUTES sa;
   HSHM handle;

   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = &_dwsd;
   sa.bInheritHandle = FALSE;

   handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, size, UTF8toWide(name));

   if(!handle)
      return 0;

   *dest = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

   if(!*dest)
   {
      CloseHandle(handle);
      return 0;
   }

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
   HSHM handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, UTF8toWide(name));

   if(!handle)
      return 0;

   *dest = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

   if(!*dest)
   {
      CloseHandle(handle);
      return 0;
   }

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
   UnmapViewOfFile(ptr);
   CloseHandle(handle);
   return 0;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
    COLORREF foreground = RGB(128,128,128);
    COLORREF background = DW_RGB_TRANSPARENT;
#ifdef GDIPLUS
    ARGB gpfore = MAKEARGB(255, 128, 128, 128);
    GpBrush *brush;
    GpPen *pen;

    GdipCreatePen1(gpfore, 1.0, UnitPixel, &pen);
    TlsSetValue(_dw_gppen, (LPVOID)pen);
    GdipCreateSolidFill(gpfore, &brush);
    TlsSetValue(_dw_gpbrush, brush);
#endif
    TlsSetValue(_dw_foreground, DW_UINT_TO_POINTER(foreground));
    TlsSetValue(_dw_background, DW_UINT_TO_POINTER(background));
    TlsSetValue(_dw_hpen, CreatePen(PS_SOLID, 1, foreground));
    TlsSetValue(_dw_hbrush, CreateSolidBrush(foreground));
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
   HPEN hPen;
   HBRUSH hBrush;
#ifdef GDIPLUS
   GpBrush *brush;
   GpPen *pen;
#endif

   if((hPen = TlsGetValue(_dw_hpen)))
       DeleteObject(hPen);
   if((hBrush = TlsGetValue(_dw_hbrush)))
       DeleteObject(hBrush);
#ifdef GDIPLUS
   if((brush = TlsGetValue(_dw_gpbrush)))
      GdipDeleteBrush(brush);
   if((pen = TlsGetValue(_dw_gppen)))
      GdipDeletePen(pen);
#endif
}

/*
 * Encapsulate thread creation on Win32.
 */
void _dwthreadstart(void *data)
{
   void (* threadfunc)(void *) = NULL;
   void **tmp = (void **)data;

   _dw_init_thread();

   threadfunc = (void (*)(void *))tmp[0];
   threadfunc(tmp[1]);

   free(tmp);
   _dw_deinit_thread();
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
#if defined(__CYGWIN__)
   return 0;
#else
   void **tmp = malloc(sizeof(void *) * 2);

   tmp[0] = func;
   tmp[1] = data;

   return (DWTID)_beginthread((void(*)(void *))_dwthreadstart, stack, tmp);
#endif
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
#if !defined(__CYGWIN__)
   _dw_deinit_thread();
   _endthread();
#endif
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
#if defined(__CYGWIN__)
   return 0;
#else
   return (DWTID)GetCurrentThreadId();
#endif
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
   OleUninitialize();
#ifdef AEROGLASS
   /* Free any in use libraries */
   FreeLibrary(hdwm);
#endif
   FreeLibrary(huxtheme);
   DestroyWindow(_dw_tooltip);
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
   HWND tmp = CreateWindow(SplitbarClassName,
                     NULL,
                     WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
                     0,0,0,0,
                     DW_HWND_OBJECT,
                     (HMENU)(uintptr_t)id,
                     _DWInstance,
                     NULL);

   if(tmp)
   {
      HWND tmpbox = dw_box_new(DW_VERT, 0);
      float *percent = (float *)malloc(sizeof(float));

      dw_box_pack_start(tmpbox, topleft, 1, 1, TRUE, TRUE, 0);
      SetParent(tmpbox, tmp);
      dw_window_set_data(tmp, "_dw_topleft", DW_POINTER(tmpbox));

      tmpbox = dw_box_new(DW_VERT, 0);
      dw_box_pack_start(tmpbox, bottomright, 1, 1, TRUE, TRUE, 0);
      SetParent(tmpbox, tmp);
      dw_window_set_data(tmp, "_dw_bottomright", DW_POINTER(tmpbox));
      *percent = 50.0;
      dw_window_set_data(tmp, "_dw_percent", DW_POINTER(percent));
      dw_window_set_data(tmp, "_dw_type", DW_INT_TO_POINTER(type));
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
   int type = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_type"));
   unsigned long width, height;

   if(mypercent)
      *mypercent = percent;

   dw_window_get_pos_size(handle, NULL, NULL, &width, &height);

   if(width > 0 && height > 0)
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

/*
 * Creates a calendar window (widget) with given parameters.
 * Parameters:
 *       type: Value can be DW_VERT or DW_HORZ.
 *       topleft: Handle to the window to be top or left.
 *       bottomright:  Handle to the window to be bottom or right.
 * Classname: SysMonthCal32
 * Returns:
 *       A handle to a calendar window or NULL on failure.
 */
HWND API dw_calendar_new(unsigned long id)
{
   RECT rc;
   MONTHDAYSTATE mds[3];
   HWND tmp = CreateWindowEx(WS_EX_CLIENTEDGE,
                           MONTHCAL_CLASS,
                           NULL,
                           WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | MCS_DAYSTATE,
                           0,0,0,0,
                           DW_HWND_OBJECT,
                           (HMENU)(uintptr_t)id,
                           _DWInstance,
                           NULL);
   if ( tmp )
   {
      // Get the size required to show an entire month.
      MonthCal_GetMinReqRect(tmp, &rc);
      // Resize the control now that the size values have been obtained.
      SetWindowPos(tmp, NULL, 0, 0,
               rc.right, rc.bottom,
               SWP_NOZORDER | SWP_NOMOVE);
      mds[0] = mds[1] = mds[2] = (MONTHDAYSTATE)0;
      MonthCal_SetDayState(tmp,3,mds);
      return tmp;
   }
   else
      return NULL;
}

/*
 * Sets the current date of a calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year:   The year to set the date to
 *       month:  The month to set the date to
 *       day:    The day to set the date to
 */
void API dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
   MONTHDAYSTATE mds[3];
   SYSTEMTIME date;
   date.wYear = year;
   date.wMonth = month;
   date.wDay = day;
   if ( MonthCal_SetCurSel( handle, &date ) )
   {
   }
   else
   {
   }
   mds[0] = mds[1] = mds[2] = (MONTHDAYSTATE)0;
   MonthCal_SetDayState(handle,3,mds);
}

/*
 * Gets the date from the calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year:   Pointer to the year to get the date to
 *       month:  Pointer to the month to get the date to
 *       day:    Pointer to the day to get the date to
 */
void API dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
   SYSTEMTIME date;
   if ( MonthCal_GetCurSel( handle, &date ) )
   {
      *year = date.wYear;
      *month = date.wMonth;
      *day = date.wDay;
   }
   else
   {
      *year = *month = *day = 0;
   }
}

/* Internal function to set the focus from the window thread */
void _dw_set_focus(HWND handle)
{
    SetFocus(handle);
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
    if(_dwtid == (DWTID)-1)
        SetFocus(handle);
    else
        dw_window_function(handle, (void *)_dw_set_focus, handle);
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
   Box *thisbox = (Box *)GetWindowLongPtr(window, GWLP_USERDATA);

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
   ColorInfo *cinfo = _dw_window_get_cinfo(window);

   if(cinfo)
      cinfo->clickdefault = next;
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
   HANDLE handle;
   char *ret = NULL;
   TCHAR *tmp;
#ifdef UNICODE
   int type = CF_UNICODETEXT;
#else
   int type = CF_TEXT;
#endif

   if ( !OpenClipboard( NULL ) )
      return ret;

   if ( ( handle = GetClipboardData(type) ) == NULL )
   {
      CloseClipboard();
      return ret;
   }

   if ( (tmp = GlobalLock(handle)) && _tcslen(tmp) )
   {
        ret = _strdup(WideToUTF8(tmp));
        GlobalUnlock(handle);
   }
   CloseClipboard();
   return ret;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
   HGLOBAL ptr1;
   LPTSTR ptr2;
   TCHAR *buf;
#ifdef UNICODE
   int type = CF_UNICODETEXT;
   char *src = calloc(len + 1, 1);

   memcpy(src, str, len);
   buf = UTF8toWide(src);
   free(src);
   len = (int)_tcslen(buf);
#else
   int type = CF_TEXT;

   buf = str;
#endif

   if ( !OpenClipboard( NULL ) )
      return;

   ptr1 = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, (len + 1) * sizeof(TCHAR) );

   if ( !ptr1 )
      return;

   ptr2 = GlobalLock( ptr1 );

   memcpy(ptr2, buf, (len + 1) * sizeof(TCHAR));
   GlobalUnlock( ptr1 );
   EmptyClipboard();

   SetClipboardData( type, ptr1 );

   CloseClipboard();
   GlobalFree( ptr1 );
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
#ifdef BUILD_TOAST
   char outbuf[1025] = {0};

   if(description)
   {
      va_list args;

      va_start(args, description);
      vsnprintf(outbuf, 1024, description, args);
      va_end(args);
   }
   return (HWND)_dw_notification_new(UTF8toWide(title), imagepath ? UTF8toWide(imagepath) : NULL, UTF8toWide(outbuf));
#else
   return NULL;
#endif
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
#ifdef BUILD_TOAST
   return _dw_notification_send((void *)notification);
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
   if(!env)
      return;

   /* Get the Windows version. */

   env->MajorVersion =  (DWORD)(LOBYTE(LOWORD(_dwVersion)));
   env->MinorVersion =  (DWORD)(HIBYTE(LOWORD(_dwVersion)));

   /* Get the build number for Windows NT/Windows 2000. */

   env->MinorBuild =  0;

   if (_dwVersion < 0x80000000)
   {
      env->MajorBuild = (DWORD)(HIWORD(_dwVersion));

      if(env->MajorVersion == 5 && env->MinorVersion == 0)
         strcpy(env->osName, "Windows 2000");
      else if(env->MajorVersion == 5 && env->MinorVersion > 0)
         strcpy(env->osName, "Windows XP");
      else if(env->MajorVersion == 6 && env->MinorVersion == 0)
         strcpy(env->osName, "Windows Vista");
      else if(env->MajorVersion == 6 && env->MinorVersion == 1)
         strcpy(env->osName, "Windows 7");
      else if(env->MajorVersion == 6 && env->MinorVersion > 1)
         strcpy(env->osName, "Windows 8");
      else if(env->MajorVersion == 10)
      {
         if(env->MajorBuild < 20000)
            strcpy(env->osName, "Windows 10");
         else 
            strcpy(env->osName, "Windows 11");
      }
      else
         strcpy(env->osName, "Windows NT");
   }
   else
   {
      strcpy(env->osName, "Windows 95/98/ME");
      env->MajorBuild =  0;
   }

   strcpy(env->buildDate, __DATE__);
   strcpy(env->buildTime, __TIME__);
#ifdef BUILD_HTML
#  ifdef BUILD_EDGE
   strcpy(env->htmlEngine, _DW_EDGE_DETECTED ? "EDGE" : "IE");
#  else
   strcpy(env->htmlEngine, "IE");
#  endif
#else
   strcpy(env->htmlEngine, "N/A");
#endif

   env->DWMajorVersion = DW_MAJOR_VERSION;
   env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
   env->DWSubVersion = VER_REV;
#else
   env->DWSubVersion = DW_SUB_VERSION;
#endif
}

/* Helper to make sure all /s are \s */
void _dw_to_dos(TCHAR *dst, TCHAR *src)
{
   int x = 0;

   while(src[x])
   {
      if(src[x] == TEXT('/'))
         dst[x] = TEXT('\\');
      else
         dst[x] = src[x];
      x++;
   }
   dst[x] = 0;
}

#define BROWSEBUFSIZE 1000

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extention.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE or DW_DIRECTORY_OPEN.
 * Returns:
 *       NULL on error. A malloced buffer containing
 *       the file path on success.
 *
 */
char * API dw_file_browse(const char *title, const char *defpath, const char *ext, int flags)
{
   OPENFILENAME of = {0};
   TCHAR filenamebuf[BROWSEBUFSIZE+1] = {0}, *fbuf = filenamebuf;
   TCHAR filterbuf[BROWSEBUFSIZE+1] = {0};
   TCHAR *exten = UTF8toWide(ext);
   TCHAR *dpath = UTF8toWide(defpath);
   int rc;

   if ( flags == DW_DIRECTORY_OPEN )
   {
   /* If we aren't building a DLL, use the more simple browser */
#ifndef BUILD_DLL
      BROWSEINFO bi = {0};
      TCHAR szDir[MAX_PATH];
      LPITEMIDLIST pidl;
      LPMALLOC pMalloc;

      if (SUCCEEDED(SHGetMalloc(&pMalloc)))
      {
         bi.hwndOwner = NULL;
         bi.pszDisplayName = 0;
         bi.pidlRoot = 0;
         bi.lpszTitle = UTF8toWide(title);
         bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
         bi.lpfn = NULL; /*BrowseCallbackProc*/

         pidl = SHBrowseForFolder(&bi);
         if (pidl)
         {
            if (SHGetPathFromIDList(pidl,szDir))
            {
               _tcsncpy(filenamebuf,szDir,BROWSEBUFSIZE);
            }

            /* In C++: pMalloc->Free(pidl); pMalloc->Release(); */
            pMalloc->lpVtbl->Free(pMalloc,pidl);
            pMalloc->lpVtbl->Release(pMalloc);
            return _strdup(WideToUTF8(filenamebuf));
         }
      }
#else
     if ( XBrowseForFolder( NULL,
                            (LPCTSTR)dpath,
                            -1,
                            (LPCTSTR)UTF8toWide(title),
                            (LPTSTR)filenamebuf,
                            BROWSEBUFSIZE,
                            FALSE ) )
     {
        return _strdup( WideToUTF8(fbuf) );
     }
#endif
   }
   else
   {
      DWORD att = defpath ? GetFileAttributes(UTF8toWide(defpath)) : INVALID_FILE_ATTRIBUTES;

      if (ext)
      {
         /*
          * The following mess is because sprintf() trunates at first \0
          * and format of filter is eg: "c files (*.c)\0*.c\0All Files\0*.*\0\0"
          */
         int len;
         TCHAR *ptr = filterbuf;
         TCHAR *start = filterbuf;

         len = _sntprintf( ptr, BROWSEBUFSIZE - (ptr - start), TEXT("%s Files (*.%s)"), exten, exten );
         ptr = ptr + len + 1; /* past first \0 */
         len = _sntprintf( ptr, BROWSEBUFSIZE - (ptr - start), TEXT("*.%s"), exten );
         ptr = ptr + len + 1; /* past next \0 */
         len = _sntprintf( ptr, BROWSEBUFSIZE - (ptr - start), TEXT("All Files") );
         ptr = ptr + len + 1; /* past next \0 */
         len = _sntprintf( ptr, BROWSEBUFSIZE - (ptr - start), TEXT("*.*") );
      }

      memset( &of, 0, sizeof(OPENFILENAME) );

      of.lStructSize = sizeof(OPENFILENAME);
      of.hwndOwner = HWND_DESKTOP;
      of.hInstance = _DWInstance;
      of.lpstrTitle = UTF8toWide(title);
      of.lpstrInitialDir = TEXT(".");
      if(att != INVALID_FILE_ATTRIBUTES && (att & FILE_ATTRIBUTE_DIRECTORY))
        of.lpstrInitialDir = dpath;
      else if(defpath)
        _dw_to_dos(filenamebuf, dpath);
      of.lpstrFile = filenamebuf;
      of.lpstrFilter = filterbuf;
      of.nFilterIndex = 1;
      of.nMaxFile = BROWSEBUFSIZE;
      /*of.lpstrDefExt = ext;*/
      of.Flags = OFN_NOCHANGEDIR;

      if (flags & DW_FILE_SAVE)
      {
         of.Flags |= OFN_OVERWRITEPROMPT;
         rc = GetSaveFileName(&of);
      }
      else
      {
         of.Flags |= OFN_FILEMUSTEXIST;
         rc = GetOpenFileName(&of);
      }

      if (rc)
         return _strdup(WideToUTF8(of.lpstrFile));
   }
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
int API dw_exec(const char *program, int type, char **params)
{
   char **newparams;
   int retcode, count = 0, z;

   while(params[count])
   {
      count++;
   }

   newparams = (char **)malloc(sizeof(char *) * (count+1));

   for(z=0;z<count;z++)
   {
      if(strchr(params[z], ' '))
      {		
         newparams[z] = malloc(strlen(params[z])+3);
         strcpy(newparams[z], "\"");
         strcat(newparams[z], params[z]);
         strcat(newparams[z], "\"");
      }
      else
         newparams[z] = _strdup(params[z]);
   }
   newparams[count] = NULL;

   /* Why does this return intptr_t ... can the PID exceed an integer value? */
   retcode = (int)_spawnvp(P_NOWAIT, program, (const char * const *)newparams);

   for(z=0;z<count;z++)
   {
      free(newparams[z]);
   }
   free(newparams);

   return retcode;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(const char *url)
{
   char *browseurl = _strdup(url);
   int retcode;

   if(strlen(browseurl) > 7 && strncmp(browseurl, "file://", 7) == 0)
   {
      int len, z;

      browseurl = &browseurl[7];
      len = (int)strlen(browseurl);

      for(z=0;z<len;z++)
      {
         if(browseurl[z] == '|')
            browseurl[z] = ':';
         if(browseurl[z] == '/')
            browseurl[z] = '\\';
      }
   }

   retcode = DW_POINTER_TO_INT(ShellExecute(NULL, TEXT("open"), UTF8toWide(browseurl), NULL, NULL, SW_SHOWNORMAL));
   free(browseurl);
   if(retcode<33 && retcode != 2)
      return DW_ERROR_UNKNOWN;
   return DW_ERROR_NONE;
}

typedef struct _dwprint
{
    PRINTDLG pd;
    DOCINFO di;
    int (DWSIGNAL *drawfunc)(HPRINT, HPIXMAP, int, void *);
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
HPRINT API dw_print_new(const char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
    DWPrint *print;

    if(!drawfunc || !(print = calloc(1, sizeof(DWPrint))))
        return NULL;

    print->drawfunc = drawfunc;
    print->drawdata = drawdata;
    print->pd.lStructSize = sizeof(PRINTDLG);
    print->pd.hwndOwner = HWND_DESKTOP;
    print->pd.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    print->pd.nCopies = 1;
    print->pd.nFromPage = 1;
    print->pd.nToPage = pages;
    print->pd.nMinPage = 1;
    print->pd.nMaxPage = pages;

    if(!PrintDlg(&(print->pd)))
    {
        free(print);
        return NULL;
    }

    print->di.cbSize = sizeof(DOCINFO);
    print->di.lpszDocName = jobname ? UTF8toWide(jobname) : TEXT("Dynamic Windows Print Job");
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
    HPIXMAP pixmap;
    int x, width, height, result = DW_ERROR_UNKNOWN;

    if(!p)
        return result;

    if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
        return result;

    width = GetDeviceCaps(p->pd.hDC, HORZRES);
    height = GetDeviceCaps(p->pd.hDC, VERTRES);

    pixmap->hbm = CreateCompatibleBitmap(p->pd.hDC, width, height);
    pixmap->hdc = p->pd.hDC;
    pixmap->transcolor = DW_RGB_TRANSPARENT;

#ifdef GDIPLUS
   /* Convert the size based on the DPI */
   _dw_convert_dpi(pixmap->hdc, &width, &height, FALSE);
#endif

    pixmap->width = width;
    pixmap->height = height;

    SelectObject(pixmap->hdc, pixmap->hbm);

    /* Start the job */
    StartDoc(p->pd.hDC, &(p->di));

    /* Cycle through each page */
    for(x=p->pd.nFromPage-1; x<p->pd.nToPage && p->drawfunc; x++)
    {
        StartPage(p->pd.hDC);
        p->drawfunc(print, pixmap, x, p->drawdata);
        EndPage(p->pd.hDC);
    }
    if(p->drawfunc)
    {
        result = DW_ERROR_NONE;
        EndDoc(p->pd.hDC);
    }
    else
        AbortDoc(p->pd.hDC);
    /* Free memory */
    dw_pixmap_destroy(pixmap);
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
    static char _user_dir[1024] = {0};

    if(!_user_dir[0])
    {
        HANDLE hToken = 0;

        /* Use the Windows API to get the user's profile directory */
        if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            DWORD BufSize = 1024;
            TCHAR TmpBuf[1024], *Buf = TmpBuf;

            GetUserProfileDirectory(hToken, Buf, &BufSize);
            CloseHandle(hToken);
            strncpy(_user_dir, WideToUTF8(Buf), 1023);
        }
        /* If it fails set it to the root directory */
        if(!_user_dir[0])
        {
            strcpy(_user_dir, "C:\\");
        }
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
int API dw_app_id_set(const char *appid, const char *appname)
{
    strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
    if(appname)
        strncpy(_dw_app_name, appname, _DW_APP_ID_SIZE);
    return DW_ERROR_NONE;
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
   SendMessage(_dw_toplevel_window(handle), WM_USER, (WPARAM)function, (LPARAM)data);
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
      if(_stricmp(tmp->varname, varname) == 0)
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
         new->varname = _strdup(varname);
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
      if(all || _stricmp(tmp->varname, varname) == 0)
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
   ColorInfo *cinfo = _dw_window_get_cinfo(window);

   if(!cinfo)
   {
      if(!dataname)
         return;

      cinfo = _dw_window_new_cinfo(window, FALSE);
   }

   if(cinfo)
   {
      if(data)
         _dw_new_userdata(&(cinfo->root), dataname, data);
      else
      {
         if(dataname)
            _dw_remove_userdata(&(cinfo->root), dataname, FALSE);
         else
            _dw_remove_userdata(&(cinfo->root), NULL, TRUE);
      }
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
   ColorInfo *cinfo = _dw_window_get_cinfo(window);

   if(cinfo && cinfo->root && dataname)
   {
      UserData *ud = _dw_find_userdata(&(cinfo->root), dataname);
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
      /* Warning: This seems to return UINT_PTR on some systems...
       * which may exceed the storage of int that our API uses.
       */
      int timerid = (int)SetTimer(NULL, 0, interval, _dw_timerproc);

      if(timerid)
      {
         _dw_new_signal(WM_TIMER, NULL, timerid, sigfunc, NULL, data);
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

   KillTimer(NULL, id);

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

   if (window && signame && sigfunc)
   {
      if (_stricmp(signame, DW_SIGNAL_SET_FOCUS) == 0)
         window = _dw_normalize_handle(window);

      if ((message = _dw_findsigmessage(signame)) != 0)
      {
         /* Handle special case of the menu item */
         if (message == WM_COMMAND && window < (HWND)65536)
         {
            char buffer[16];
            HWND owner;

            _snprintf(buffer, 15, "_dw_id%d", (int)(intptr_t)window);
            owner = (HWND)dw_window_get_data(DW_HWND_OBJECT, buffer);

            /* Make sure there are no dupes from popups */
            dw_signal_disconnect_by_window(window);

            if (owner)
            {
               id = (ULONG)(uintptr_t)window;
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
      if(((window < (HWND)65536 && (int)(intptr_t)window == tmp->id) || tmp->window == window) && tmp->message == message)
      {
         void (DWSIGNAL *discfunc)(HWND, void *) = (void (*)(HWND, void *))tmp->discfunction;

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
      if((window < (HWND)65536 && (int)(intptr_t)window == tmp->id) || tmp->window == window)
      {
         void (DWSIGNAL *discfunc)(HWND, void *) = (void (*)(HWND, void *))tmp->discfunction;

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
      if(((window < (HWND)65536 && (int)(intptr_t)window == tmp->id) || tmp->window == window) && tmp->data == data)
      {
         void (DWSIGNAL *discfunc)(HWND, void *) = (void (*)(HWND, void *))tmp->discfunction;

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
    return _dw_UTF8toWide(utf8string, malloc(MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0) * sizeof(WCHAR)));
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
    return _dw_WideToUTF8(wstring, malloc(WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL)));
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
#ifdef BUILD_HTML
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
#endif
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MDI:
        case DW_FEATURE_TASK_BAR:  
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_RENDER_SAFE:
            return _dw_render_safe_mode;
#if defined(BUILD_HTML) && defined(BUILD_EDGE)
        case DW_FEATURE_HTML_MESSAGE:
            return _DW_EDGE_DETECTED ? DW_FEATURE_ENABLED : DW_FEATURE_UNSUPPORTED;
#endif
#ifdef BUILD_TOAST
        case DW_FEATURE_NOTIFICATION:
        {
            if(_dw_toast_is_compatible())
                return DW_FEATURE_ENABLED;
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
#ifdef AEROGLASS
        case DW_FEATURE_WINDOW_TRANSPARENCY:
        {
            if(_dw_composition)
                return DW_FEATURE_ENABLED;
            return DW_FEATURE_DISABLED;
        }
        case DW_FEATURE_DARK_MODE:
        {
            if(_DW_DARK_MODE_SUPPORTED)
            {
                /* Special case for Full dark mode setting... return DW_DARK_MODE_BASIC
                 * with DW_DARK_MODE_FULL requested but the system is in light mode.
                 */
                if(_DW_DARK_MODE_ALLOWED == DW_DARK_MODE_FULL && !_DW_DARK_MODE_ENABLED)
                    return DW_DARK_MODE_BASIC;
                return _DW_DARK_MODE_ALLOWED;
            }
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
#ifdef RICHEDIT
        /* Word wrap support on Windows is tied to Rich Edit,
         * if Rich Edit MLE is unsupported or disabled...
         * Word Wrap will return Unsupported.
         */
        case DW_FEATURE_MLE_WORD_WRAP:
        {
            if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
                return DW_FEATURE_ENABLED;
            return DW_FEATURE_UNSUPPORTED;
        }
        case DW_FEATURE_MLE_RICH_EDIT:
        {
            if(hmsftedit || hrichedit)
                return _DW_MLE_RICH_EDIT;
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
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
#ifdef BUILD_HTML
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
#endif
#ifdef AEROGLASS
        case DW_FEATURE_WINDOW_TRANSPARENCY:
#endif
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MDI:
        case DW_FEATURE_TASK_BAR:  
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_ERROR_GENERAL;
#if defined(BUILD_HTML) && defined(BUILD_EDGE)
        case DW_FEATURE_HTML_MESSAGE:
            return _DW_EDGE_DETECTED ? DW_ERROR_GENERAL : DW_FEATURE_UNSUPPORTED;
#endif
#ifdef BUILD_TOAST
        case DW_FEATURE_NOTIFICATION:
        {
            if(_dw_toast_is_compatible())
                return DW_ERROR_GENERAL;
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
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
#ifdef AEROGLASS
        case DW_FEATURE_DARK_MODE:
        {
            if(state >= DW_DARK_MODE_DISABLED && state <= DW_DARK_MODE_FORCED)
            {
                _DW_DARK_MODE_ALLOWED = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
#endif
#ifdef RICHEDIT
        /* Word wrap support on Windows is tied to Rich Edit,
         * if Rich Edit MLE is unsupported or disabled...
         * Word Wrap will return Unsupported. If supported, 
         * it is not configurable, configure Rich Edit instead.
         */
        case DW_FEATURE_MLE_WORD_WRAP:
        {
            if(_DW_MLE_RICH_EDIT == DW_FEATURE_ENABLED && (hrichedit || hmsftedit))
                return DW_ERROR_GENERAL;
            return DW_FEATURE_UNSUPPORTED;
        }
        case DW_FEATURE_MLE_RICH_EDIT:
        {
            if(state >= DW_FEATURE_DISABLED && state <= DW_FEATURE_ENABLED)
            {
                _DW_MLE_RICH_EDIT = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
#endif
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}

