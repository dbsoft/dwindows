/* $Id$ */

#ifndef _H_DW
#define _H_DW

/* Dynamic Windows version numbers */
#define DW_MAJOR_VERSION 1
#define DW_MINOR_VERSION 0
#define DW_SUB_VERSION 0

#if defined(__OS2__) || defined(__WIN32__) || defined(WINNT) || defined(__EMX__)
/* OS/2 or Windows */

/* Used internally */
#define TYPEBOX  0
#define TYPEITEM 1

#define SIZESTATIC 0
#define SIZEEXPAND 1

#define SPLITBAR_WIDTH 3
#define BUBBLE_HELP_MAX 256

typedef struct _user_data
{
	struct _user_data *next;
	void              *data;
	char              *varname;
} UserData;

#if defined(__OS2__) || defined(__EMX__)
#define INCL_DOS
#define INCL_WIN
#define INCL_GPI

#include <os2.h>

/* Lets make some platform independent defines :) */
#define DW_DT_LEFT               DT_LEFT
#define DW_DT_QUERYEXTENT        DT_QUERYEXTENT
#define DW_DT_UNDERSCORE         DT_UNDERSCORE
#define DW_DT_STRIKEOUT          DT_STRIKEOUT
#define DW_DT_TEXTATTRS          DT_TEXTATTRS
#define DW_DT_EXTERNALLEADING    DT_EXTERNALLEADING
#define DW_DT_CENTER             DT_CENTER
#define DW_DT_RIGHT              DT_RIGHT
#define DW_DT_TOP                DT_TOP
#define DW_DT_VCENTER            DT_VCENTER
#define DW_DT_BOTTOM             DT_BOTTOM
#define DW_DT_HALFTONE           DT_HALFTONE
#define DW_DT_MNEMONIC           DT_MNEMONIC
#define DW_DT_WORDBREAK          DT_WORDBREAK
#define DW_DT_ERASERECT          DT_ERASERECT

#define DW_CLR_WHITE             16
#define DW_CLR_BLACK             17
#define DW_CLR_BLUE              CLR_BLUE
#define DW_CLR_RED               CLR_RED
#define DW_CLR_PINK              CLR_PINK
#define DW_CLR_GREEN             CLR_GREEN
#define DW_CLR_CYAN              CLR_CYAN
#define DW_CLR_YELLOW            CLR_YELLOW
#define DW_CLR_DARKGRAY          CLR_DARKGRAY
#define DW_CLR_DARKBLUE          CLR_DARKBLUE
#define DW_CLR_DARKRED           CLR_DARKRED
#define DW_CLR_DARKPINK          CLR_DARKPINK
#define DW_CLR_DARKGREEN         CLR_DARKGREEN
#define DW_CLR_DARKCYAN          CLR_DARKCYAN
#define DW_CLR_BROWN             CLR_BROWN
#define DW_CLR_PALEGRAY          CLR_PALEGRAY

#define DW_FCF_TITLEBAR          FCF_TITLEBAR
#define DW_FCF_SYSMENU           (FCF_SYSMENU | FCF_CLOSEBUTTON)
#define DW_FCF_MENU              FCF_MENU
#define DW_FCF_SIZEBORDER        FCF_SIZEBORDER
#define DW_FCF_MINBUTTON         FCF_MINBUTTON
#define DW_FCF_MAXBUTTON         FCF_MAXBUTTON
#define DW_FCF_MINMAX            FCF_MINMAX
#define DW_FCF_VERTSCROLL        FCF_VERTSCROLL
#define DW_FCF_HORZSCROLL        FCF_HORZSCROLL
#define DW_FCF_DLGBORDER         FCF_DLGBORDER
#define DW_FCF_BORDER            FCF_BORDER
#define DW_FCF_SHELLPOSITION     FCF_SHELLPOSITION
#define DW_FCF_TASKLIST          FCF_TASKLIST
#define DW_FCF_NOBYTEALIGN       FCF_NOBYTEALIGN
#define DW_FCF_NOMOVEWITHOWNER   FCF_NOMOVEWITHOWNER
#define DW_FCF_SYSMODAL          FCF_SYSMODAL
#define DW_FCF_HIDEBUTTON        FCF_HIDEBUTTON
#define DW_FCF_HIDEMAX           FCF_HIDEMAX
#define DW_FCF_AUTOICON          FCF_AUTOICON

#define DW_CFA_BITMAPORICON      CFA_BITMAPORICON
#define DW_CFA_STRING            CFA_STRING
#define DW_CFA_ULONG             CFA_ULONG
#define DW_CFA_TIME              CFA_TIME
#define DW_CFA_DATE              CFA_DATE
#define DW_CFA_CENTER            CFA_CENTER
#define DW_CFA_LEFT              CFA_LEFT
#define DW_CFA_RIGHT             CFA_RIGHT
#define DW_CFA_HORZSEPARATOR     CFA_HORZSEPARATOR
#define DW_CFA_SEPARATOR         CFA_SEPARATOR

#define DW_CA_DETAILSVIEWTITLES  CA_DETAILSVIEWTITLES
#define DW_CV_MINI               CV_MINI
#define DW_CV_DETAIL             CV_DETAIL

#define DW_CRA_SELECTED          CRA_SELECTED
#define DW_CRA_CURSORED          CRA_CURSORED

#define DW_SLS_READONLY          SLS_READONLY
#define DW_SLS_RIBBONSTRIP       SLS_RIBBONSTRIP

#define DW_CCS_SINGLESEL         CCS_SINGLESEL
#define DW_CCS_EXTENDSEL         CCS_EXTENDSEL

#define DW_LS_MULTIPLESEL        LS_MULTIPLESEL

#define DW_LIT_NONE              -1

#define DW_MLE_CASESENSITIVE     MLFSEARCH_CASESENSITIVE

#define DW_POINTER_ARROW         SPTR_ARROW
#define DW_POINTER_CLOCK         SPTR_WAIT

#define DW_OS2_NEW_WINDOW        1

typedef struct _window_data {
	PFNWP oldproc;
	UserData *root;
	HWND clickdefault;
	ULONG flags;
	void *data;
} WindowData;

typedef struct _hpixmap {
	unsigned long width, height;
	HDC hdc;
	HPS hps;
	HBITMAP hbm;
	HWND handle;
} *HPIXMAP;

typedef struct _hmenui {
	HWND menu;
} *HMENUI;

extern HAB dwhab;
extern HMQ dwhmq;
#endif

#if defined(__WIN32__) || defined(WINNT)
#include <windows.h>
#include <commctrl.h>

/* Cygwin doesn't seem to have these... */
#if defined(__CYGWIN32__)
#define LVS_EX_GRIDLINES        0x00000001
#define LVS_EX_FULLROWSELECT    0x00000020
#define LVM_SETEXTENDEDLISTVIEWSTYLE (0x1000 + 54)
#define ListView_SetExtendedListViewStyle(hwndLV, dw) (DWORD)SendMessage((hwndLV), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dw)
#endif
#ifndef UDM_SETPOS32
#define UDM_SETPOS32            (WM_USER+113)
#endif
#ifndef UDM_GETPOS32
#define UDM_GETPOS32            (WM_USER+114)
#endif

/* Lets make some platform independent defines :) */
#define DW_DT_LEFT               SS_LEFT
#define DW_DT_QUERYEXTENT        0
#define DW_DT_UNDERSCORE         0
#define DW_DT_STRIKEOUT          0
#define DW_DT_TEXTATTRS          0
#define DW_DT_EXTERNALLEADING    0
#define DW_DT_CENTER             SS_CENTER
#define DW_DT_RIGHT              SS_RIGHT
#define DW_DT_TOP                0
#define DW_DT_VCENTER            SS_NOPREFIX
#define DW_DT_BOTTOM             0
#define DW_DT_HALFTONE           0
#define DW_DT_MNEMONIC           0
#define DW_DT_WORDBREAK          0
#define DW_DT_ERASERECT          0

/* These corespond to the entries in the color
 * arrays in the Win32 dw.c, they are also the
 * same as DOS ANSI colors.
 */
#define DW_CLR_BLACK             0
#define DW_CLR_DARKRED           1
#define DW_CLR_DARKGREEN         2
#define DW_CLR_BROWN             3
#define DW_CLR_DARKBLUE          4
#define DW_CLR_DARKPINK          5
#define DW_CLR_DARKCYAN          6
#define DW_CLR_PALEGRAY          7
#define DW_CLR_DARKGRAY          8
#define DW_CLR_RED               9
#define DW_CLR_GREEN             10
#define DW_CLR_YELLOW            11
#define DW_CLR_BLUE              12
#define DW_CLR_PINK              13
#define DW_CLR_CYAN              14
#define DW_CLR_WHITE             15

#define DW_FCF_TITLEBAR          WS_CAPTION
#define DW_FCF_SYSMENU           WS_SYSMENU
#define DW_FCF_MENU              0
#define DW_FCF_SIZEBORDER        WS_THICKFRAME
#define DW_FCF_MINBUTTON         WS_MINIMIZEBOX
#define DW_FCF_MAXBUTTON         WS_MAXIMIZEBOX
#define DW_FCF_MINMAX            (WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define DW_FCF_VERTSCROLL        WS_VSCROLL
#define DW_FCF_HORZSCROLL        WS_HSCROLL
#define DW_FCF_DLGBORDER         WS_DLGFRAME
#define DW_FCF_BORDER            WS_BORDER
#define DW_FCF_SHELLPOSITION     0
#define DW_FCF_TASKLIST          WS_VSCROLL
#define DW_FCF_NOBYTEALIGN       0
#define DW_FCF_NOMOVEWITHOWNER   0
#define DW_FCF_SYSMODAL          0
#define DW_FCF_HIDEBUTTON        WS_MINIMIZEBOX
#define DW_FCF_HIDEMAX           0
#define DW_FCF_AUTOICON          0

#define DW_CFA_BITMAPORICON      1
#define DW_CFA_STRING            (1 << 1)
#define DW_CFA_ULONG             (1 << 2)
#define DW_CFA_TIME              (1 << 3)
#define DW_CFA_DATE              (1 << 4)
#define DW_CFA_CENTER            (1 << 5)
#define DW_CFA_LEFT              (1 << 6)
#define DW_CFA_RIGHT             (1 << 7)
#define DW_CFA_HORZSEPARATOR     0
#define DW_CFA_SEPARATOR         0

#define DW_CA_DETAILSVIEWTITLES  0
#define DW_CV_MINI               0
#define DW_CV_DETAIL             0

#define DW_CRA_SELECTED          LVNI_SELECTED
#define DW_CRA_CURSORED          LVNI_FOCUSED

#define DW_SLS_READONLY          0
#define DW_SLS_RIBBONSTRIP       0

#define DW_CCS_SINGLESEL         0
#define DW_CCS_EXTENDSEL         0

#define DW_LS_MULTIPLESEL        LBS_MULTIPLESEL

#define DW_LIT_NONE              -1

#define DW_MLE_CASESENSITIVE     1

#define DW_POINTER_ARROW         32512
#define DW_POINTER_CLOCK         32514

#define STATICCLASSNAME "STATIC"
#define COMBOBOXCLASSNAME "COMBOBOX"
#define LISTBOXCLASSNAME "LISTBOX"
#define BUTTONCLASSNAME "BUTTON"
#define POPUPMENUCLASSNAME "POPUPMENU"
#define EDITCLASSNAME "EDIT"
#define FRAMECLASSNAME "FRAME"

#define ClassName "dynamicwindows"
#define SplitbarClassName "dwsplitbar"
#define ObjectClassName "dwobjectclass"
#define DefaultFont NULL

typedef struct _color {
	int fore;
	int back;
	HWND combo, buddy;
	int user;
	int vcenter;
	HWND clickdefault;
	HBRUSH hbrush;
	char fontname[128];
	WNDPROC pOldProc;
	UserData *root;
} ColorInfo;

typedef struct _notebookpage {
	ColorInfo cinfo;
	TC_ITEM item;
	HWND hwnd;
	int realid;
} NotebookPage;

typedef HANDLE HMTX;
typedef HANDLE HEV;

typedef struct _container {
	ColorInfo cinfo;
	ULONG *flags;
	WNDPROC pOldProc;
	ULONG columns;
} ContainerInfo;

typedef struct _hpixmap {
	unsigned long width, height;
	HBITMAP hbm;
	HDC hdc;
	HWND handle;
	void *bits;
} *HPIXMAP;

typedef struct _hmenui {
	HMENU menu;
	HWND hwnd;
} *HMENUI;

#endif

typedef struct _item {
    /* Item type - Box or Item */
	int type;
    /* Handle to Frame or Window */
	HWND hwnd;
    /* Width and Height of static size */
	int width, height, origwidth, origheight;
    /* Size Type - Static or Expand */
	int hsize, vsize;
	/* Padding */
	int pad;
	/* Ratio of current item */
    float xratio, yratio;
} Item;

typedef struct _box {
#if defined(__WIN32__) || defined(WINNT)
	ColorInfo cinfo;
#elif defined(__OS2__) || defined(__EMX__)
	PFNWP oldproc;
	UserData *root;
	HWND hwndtitle;
	int titlebar;
#endif
    /* Number of items in the box */
	int count;
	/* Box type - horizontal or vertical */
	int type;
	/* Padding */
	int pad, parentpad;
	/* Groupbox */
	HWND grouphwnd;
	/* Default item */
	HWND defaultitem;
	/* Used as temporary storage in the calculation stage */
    int upx, upy, minheight, minwidth;
	/* Ratio in this box */
	float xratio, yratio, parentxratio, parentyratio;
	/* Used for calculating individual item ratios */
	int width, height;
	/* Any combinations of flags describing the box */
	unsigned long flags;
	/* Array of item structures */
	struct _item *items;
} Box;

typedef struct _bubblebutton {
#if defined(__WIN32__) || defined(WINNT)
	ColorInfo cinfo;
	int checkbox;
	WNDPROC pOldProc;
#endif
#if defined(__OS2__) || defined(__EMX__)
	PFNWP pOldProc;
	UserData *root;
#endif
	unsigned long id;
	char bubbletext[BUBBLE_HELP_MAX];
} BubbleButton;

void dw_box_pack_start_stub(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad);
void dw_box_pack_end_stub(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad);
#else
/* GTK */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <pthread.h>

/* Lets make some platform independent defines :) */
#define DW_DT_LEFT               1
#define DW_DT_UNDERSCORE         (1 << 1)
#define DW_DT_STRIKEOUT          (1 << 2)
#define DW_DT_CENTER             (1 << 3)
#define DW_DT_RIGHT              (1 << 4)
#define DW_DT_TOP                (1 << 5)
#define DW_DT_VCENTER            (1 << 6)
#define DW_DT_BOTTOM             (1 << 7)
#define DW_DT_HALFTONE           (1 << 8)
#define DW_DT_MNEMONIC           (1 << 9)
#define DW_DT_WORDBREAK          (1 << 10)
#define DW_DT_ERASERECT          (1 << 11)

#define DW_CLR_BLACK             0
#define DW_CLR_DARKRED           1
#define DW_CLR_DARKGREEN         2
#define DW_CLR_BROWN             3
#define DW_CLR_DARKBLUE          4
#define DW_CLR_DARKPINK          5
#define DW_CLR_DARKCYAN          6
#define DW_CLR_PALEGRAY          7
#define DW_CLR_DARKGRAY          8
#define DW_CLR_RED               9
#define DW_CLR_GREEN             10
#define DW_CLR_YELLOW            11
#define DW_CLR_BLUE              12
#define DW_CLR_PINK              13
#define DW_CLR_CYAN              14
#define DW_CLR_WHITE             15

#define DW_FCF_TITLEBAR          1
#define DW_FCF_SYSMENU           (1 << 1)
#define DW_FCF_MENU              (1 << 2)
#define DW_FCF_SIZEBORDER        (1 << 3)
#define DW_FCF_MINBUTTON         (1 << 4)
#define DW_FCF_MAXBUTTON         (1 << 5)
#define DW_FCF_MINMAX            (1 << 6)
#define DW_FCF_VERTSCROLL        (1 << 7)
#define DW_FCF_HORZSCROLL        (1 << 8)
#define DW_FCF_DLGBORDER         (1 << 9)
#define DW_FCF_BORDER            (1 << 10)
#define DW_FCF_SHELLPOSITION     (1 << 11)
#define DW_FCF_TASKLIST          (1 << 12)
#define DW_FCF_NOBYTEALIGN       (1 << 13)
#define DW_FCF_NOMOVEWITHOWNER   (1 << 14)
#define DW_FCF_SYSMODAL          (1 << 15)
#define DW_FCF_HIDEBUTTON        (1 << 16)
#define DW_FCF_HIDEMAX           (1 << 17)
#define DW_FCF_AUTOICON          (1 << 18)

#define DW_CFA_BITMAPORICON      1
#define DW_CFA_STRING            (1 << 1)
#define DW_CFA_ULONG             (1 << 2)
#define DW_CFA_TIME              (1 << 3)
#define DW_CFA_DATE              (1 << 4)
#define DW_CFA_CENTER            (1 << 5)
#define DW_CFA_LEFT              (1 << 6)
#define DW_CFA_RIGHT             (1 << 7)
#define DW_CFA_HORZSEPARATOR     (1 << 8)
#define DW_CFA_SEPARATOR         (1 << 9)

#define DW_CA_DETAILSVIEWTITLES  1
#define DW_CV_MINI               (1 << 1)
#define DW_CV_DETAIL             (1 << 2)

#define DW_SLS_READONLY          1
#define DW_SLS_RIBBONSTRIP       (1 << 1)

#define DW_CCS_SINGLESEL         1
#define DW_CCS_EXTENDSEL         (1 << 1)

#define DW_CRA_SELECTED          1
#define DW_CRA_CURSORED          (1 << 1)

#define DW_LS_MULTIPLESEL        1

#define DW_LIT_NONE              -1

#define DW_MLE_CASESENSITIVE     1

#define DW_POINTER_ARROW         GDK_ARROW
#define DW_POINTER_CLOCK         GDK_CLOCK

#define DW_DESKTOP               ((HWND)0)
#define HWND_DESKTOP             ((HWND)0)

typedef GtkWidget *HWND;
#ifndef _ENVRNMNT_H
typedef unsigned long ULONG;
#endif
typedef unsigned char UCHAR;
typedef long LONG;
typedef unsigned short USHORT;
typedef short SHORT;
typedef pthread_mutex_t HMTX;
typedef struct _dw_unix_event {
	pthread_mutex_t mutex;
	pthread_cond_t event;
	pthread_t thread;
	int alive;
    int posted;
} *HEV;
typedef pthread_t DWTID;

typedef struct _hpixmap {
	unsigned long width, height;
	GdkPixmap *pixmap;
    HWND handle;
} *HPIXMAP;

typedef struct _hmenui {
	GtkWidget *menu;
} *HMENUI;

typedef struct _resource_struct {
	long resource_max, *resource_id;
	char **resource_data;
} DWResources;

#if !defined(DW_RESOURCES) || defined(BUILD_DLL)
static DWResources _resources = { 0, 0, 0 };
#else
extern DWResources _resources;
#endif

#endif

#if !defined(__OS2__) && !defined(__EMX__)
typedef struct _CDATE
{
  UCHAR	 day;
  UCHAR	 month;
  USHORT year;
} CDATE;
typedef CDATE *PCDATE;

typedef struct _CTIME
{
  UCHAR hours;
  UCHAR minutes;
  UCHAR seconds;
  UCHAR ucReserved;
} CTIME;
typedef CTIME *PCTIME;
#endif

#if defined(__OS2__) || defined(__WIN32__) || defined(WINNT) || defined(__EMX__)
typedef unsigned long DWTID;
#endif

typedef struct _dwenv {
	/* Operating System Name and DW Build Date/Time */
	char osName[30], buildDate[30], buildTime[30];
	/* Versions and builds */
	short MajorVersion, MinorVersion, MajorBuild, MinorBuild;
	/* Dynamic Window version */
	short DWMajorVersion, DWMinorVersion, DWSubVersion;
} DWEnv;


typedef struct _dwexpose {
	int x, y;
	int width, height;
} DWExpose;

typedef struct _dwdialog {
	HEV eve;
	int done;
	void *data, *result;
} DWDialog;

#define DW_SIGNAL_FUNC(a) ((void *)a)

#define DW_MINIMIZED 1

#define DW_BUTTON1_MASK 1
#define DW_BUTTON2_MASK (1 << 1)
#define DW_BUTTON3_MASK (1 << 2)

#define DW_EXEC_CON 0
#define DW_EXEC_GUI 1

#define DW_FILE_OPEN 0
#define DW_FILE_SAVE 1

#define BOXHORZ 0
#define BOXVERT 1

#define DW_SCROLL_UP 0
#define DW_SCROLL_DOWN 1
#define DW_SCROLL_TOP 2
#define DW_SCROLL_BOTTOM 3

#define DW_PIXMAP_WIDTH(x) (x ? x->width : 0)
#define DW_PIXMAP_HEIGHT(x) (x ? x->height : 0)

#define DW_RGB_COLOR (0xF0000000)
#define DW_RGB_MASK (0x00FFFFFF)
#define DW_RED_MASK (0x000000FF)
#define DW_GREEN_MASK (0x0000FF00)
#define DW_BLUE_MASK (0x00FF0000)
#define DW_RED_VALUE(a) (a & DW_RED_MASK)
#define DW_GREEN_VALUE(a) ((a & DW_GREEN_MASK) >> 8)
#define DW_BLUE_VALUE(a) ((a & DW_BLUE_MASK) >> 16)
#define DW_RGB(a, b, c) (0xF0000000 | a | b << 8 | c << 16)

#if defined(__OS2__) || defined(__EMX__)
#define DW_OS2_RGB(a) ((DW_RED_VALUE(a) << 16) | (DW_GREEN_VALUE(a) << 8) | DW_BLUE_VALUE(a))
#endif

/* Public function prototypes */
void dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad);
void dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad);
#if !defined(__OS2__) && !defined(__WIN32__) && !defined(__EMX__)
int dw_int_init(DWResources *res, int newthread, int *argc, char **argv[]);
#define dw_init(a, b, c) dw_int_init(&_resources, a, &b, &c)
#else
int dw_init(int newthread, int argc, char *argv[]);
#endif
void dw_main(void);
void dw_main_sleep(int seconds);
void dw_free(void *ptr);
int dw_window_show(HWND handle);
int dw_window_hide(HWND handle);
int dw_window_minimize(HWND handle);
int dw_window_raise(HWND handle);
int dw_window_lower(HWND handle);
int dw_window_destroy(HWND handle);
void dw_window_redraw(HWND handle);
int dw_window_set_font(HWND handle, char *fontname);
int dw_window_set_color(HWND handle, unsigned long fore, unsigned long back);
HWND dw_window_new(HWND hwndOwner, char *title, unsigned long flStyle);
HWND dw_box_new(int type, int pad);
HWND dw_groupbox_new(int type, int pad, char *title);
HWND dw_mdi_new(unsigned long id);
HWND dw_bitmap_new(unsigned long id);
HWND dw_bitmapbutton_new(char *text, unsigned long id);
HWND dw_container_new(unsigned long id);
HWND dw_tree_new(unsigned long id);
HWND dw_text_new(char *text, unsigned long id);
HWND dw_status_text_new(char *text, unsigned long id);
HWND dw_mle_new(unsigned long id);
HWND dw_entryfield_new(char *text, unsigned long id);
HWND dw_entryfield_password_new(char *text, ULONG id);
HWND dw_combobox_new(char *text, unsigned long id);
HWND dw_button_new(char *text, unsigned long id);
HWND dw_spinbutton_new(char *text, unsigned long id);
HWND dw_radiobutton_new(char *text, ULONG id);
HWND dw_percent_new(unsigned long id);
HWND dw_slider_new(int vertical, int increments, ULONG id);
HWND dw_checkbox_new(char *text, unsigned long id);
HWND dw_listbox_new(unsigned long id, int multi);
void dw_listbox_append(HWND handle, char *text);
void dw_listbox_clear(HWND handle);
int dw_listbox_count(HWND handle);
void dw_listbox_set_top(HWND handle, int top);
void dw_listbox_select(HWND handle, int index, int state);
void dw_listbox_delete(HWND handle, int index);
void dw_listbox_query_text(HWND handle, unsigned int index, char *buffer, unsigned int length);
void dw_listbox_set_text(HWND handle, unsigned int index, char *buffer);
unsigned int dw_listbox_selected(HWND handle);
int dw_listbox_selected_multi(HWND handle, int where);
unsigned int dw_percent_query_range(HWND handle);
void dw_percent_set_pos(HWND handle, unsigned int position);
unsigned int dw_slider_query_pos(HWND handle);
void dw_slider_set_pos(HWND handle, unsigned int position);
void dw_window_set_pos(HWND handle, unsigned long x, unsigned long y);
void dw_window_set_usize(HWND handle, unsigned long width, unsigned long height);
void dw_window_set_pos_size(HWND handle, unsigned long x, unsigned long y, unsigned long width, unsigned long height);
void dw_window_get_pos_size(HWND handle, unsigned long *x, unsigned long *y, unsigned long *width, unsigned long *height);
void dw_window_set_style(HWND handle, unsigned long style, unsigned long mask);
void dw_window_set_icon(HWND handle, unsigned long id);
void dw_window_set_bitmap(HWND handle, unsigned long id);
char *dw_window_get_text(HWND handle);
void dw_window_set_text(HWND handle, char *text);
int dw_window_set_border(HWND handle, int border);
void dw_window_disable(HWND handle);
void dw_window_enable(HWND handle);
void dw_window_capture(HWND handle);
void dw_window_release(void);
void dw_window_reparent(HWND handle, HWND newparent);
void dw_window_pointer(HWND handle, int pointertype);
void dw_window_default(HWND window, HWND defaultitem);
void dw_window_click_default(HWND window, HWND next);
unsigned int dw_mle_import(HWND handle, char *buffer, int startpoint);
void dw_mle_export(HWND handle, char *buffer, int startpoint, int length);
void dw_mle_query(HWND handle, unsigned long *bytes, unsigned long *lines);
void dw_mle_delete(HWND handle, int startpoint, int length);
void dw_mle_clear(HWND handle);
void dw_mle_freeze(HWND handle);
void dw_mle_thaw(HWND handle);
void dw_mle_set(HWND handle, int point);
void dw_mle_set_visible(HWND handle, int line);
void dw_mle_set_editable(HWND handle, int state);
void dw_mle_set_word_wrap(HWND handle, int state);
int dw_mle_search(HWND handle, char *text, int point, unsigned long flags);
void dw_spinbutton_set_pos(HWND handle, long position);
void dw_spinbutton_set_limits(HWND handle, long upper, long lower);
void dw_entryfield_set_limit(HWND handle, ULONG limit);
long dw_spinbutton_query(HWND handle);
int dw_checkbox_query(HWND handle);
void dw_checkbox_set(HWND handle, int value);
HWND dw_tree_insert(HWND handle, char *title, unsigned long icon, HWND parent, void *itemdata);
HWND dw_tree_insert_after(HWND handle, HWND item, char *title, unsigned long icon, HWND parent, void *itemdata);
void dw_tree_clear(HWND handle);
void dw_tree_delete(HWND handle, HWND item);
void dw_tree_set(HWND handle, HWND item, char *title, unsigned long icon);
void dw_tree_expand(HWND handle, HWND item);
void dw_tree_collapse(HWND handle, HWND item);
void dw_tree_item_select(HWND handle, HWND item);
void dw_tree_set_data(HWND handle, HWND item, void *itemdata);
int dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator);
unsigned long dw_icon_load(unsigned long module, unsigned long id);
void dw_icon_free(unsigned long handle);
void *dw_container_alloc(HWND handle, int rowcount);
void dw_container_set_item(HWND handle, void *pointer, int column, int row, void *data);
void dw_container_set_column_width(HWND handle, int column, int width);
void dw_container_set_row_title(void *pointer, int row, char *title);
void dw_container_insert(HWND handle, void *pointer, int rowcount);
void dw_container_clear(HWND handle, int redraw);
void dw_container_delete(HWND handle, int rowcount);
void dw_container_set_view(HWND handle, unsigned long flags, int iconwidth, int iconheight);
char *dw_container_query_start(HWND handle, unsigned long flags);
char *dw_container_query_next(HWND handle, unsigned long flags);
void dw_container_scroll(HWND handle, int direction, long rows);
void dw_container_cursor(HWND handle, char *text);
void dw_container_optimize(HWND handle);
int dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count);
void dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data);
void dw_filesystem_set_file(HWND handle, void *pointer, int row, char *filename, unsigned long icon);
int dw_screen_width(void);
int dw_screen_height(void);
unsigned long dw_color_depth(void);
HWND dw_notebook_new(unsigned long id, int top);
unsigned long dw_notebook_page_new(HWND handle, unsigned long flags, int front);
void dw_notebook_page_destroy(HWND handle, unsigned int pageid);
void dw_notebook_page_set_text(HWND handle, unsigned long pageid, char *text);
void dw_notebook_page_set_status_text(HWND handle, unsigned long pageid, char *text);
void dw_notebook_page_set(HWND handle, unsigned int pageid);
unsigned int dw_notebook_page_query(HWND handle);
void dw_notebook_pack(HWND handle, unsigned long pageid, HWND page);
HWND dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long id);
void dw_splitbar_set(HWND handle, float percent);
float dw_splitbar_get(HWND handle);
HMENUI dw_menu_new(unsigned long id);
HMENUI dw_menubar_new(HWND location);
HWND dw_menu_append_item(HMENUI menu, char *title, unsigned long id, unsigned long flags, int end, int check, HMENUI submenu);
void dw_menu_item_set_check(HMENUI menu, unsigned long id, int check);
void dw_menu_popup(HMENUI *menu, HWND parent, int x, int y);
void dw_menu_destroy(HMENUI *menu);
void dw_pointer_query_pos(long *x, long *y);
void dw_pointer_set_pos(long x, long y);
void dw_window_function(HWND handle, void *function, void *data);
HWND dw_window_from_id(HWND handle, int id);
HMTX dw_mutex_new(void);
void dw_mutex_close(HMTX mutex);
void dw_mutex_lock(HMTX mutex);
void dw_mutex_unlock(HMTX mutex);
HEV dw_event_new(void);
int dw_event_reset(HEV eve);
int dw_event_post(HEV eve);
int dw_event_wait(HEV eve, unsigned long timeout);
int dw_event_close (HEV *eve);
DWTID dw_thread_new(void *func, void *data, int stack);
void dw_thread_end(void);
DWTID dw_thread_id(void);
void dw_exit(int exitcode);
HWND dw_render_new(unsigned long id);
void dw_color_foreground_set(unsigned long value);
void dw_color_background_set(unsigned long value);
void dw_draw_point(HWND handle, HPIXMAP pixmap, int x, int y);
void dw_draw_line(HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2);
void dw_draw_rect(HWND handle, HPIXMAP pixmap, int fill, int x, int y, int width, int height);
void dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, char *text);
void dw_font_text_extents(HWND handle, HPIXMAP pixmap, char *text, int *width, int *height);
void dw_flush(void);
void dw_pixmap_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc);
HPIXMAP dw_pixmap_new(HWND handle, unsigned long width, unsigned long height, int depth);
HPIXMAP dw_pixmap_grab(HWND handle, ULONG id);
void dw_pixmap_destroy(HPIXMAP pixmap);
void dw_beep(int freq, int dur);
int dw_messagebox(char *title, char *format, ...);
int dw_yesno(char *title, char *text);
void dw_environment_query(DWEnv *env);
int dw_exec(char *program, int type, char **params);
int dw_browse(char *url);
char *dw_file_browse(char *title, char *defpath, char *ext, int flags);
char *dw_user_dir(void);
DWDialog *dw_dialog_new(void *data);
int dw_dialog_dismiss(DWDialog *dialog, void *result);
void *dw_dialog_wait(DWDialog *dialog);
void dw_window_set_data(HWND window, char *dataname, void *data);
void *dw_window_get_data(HWND window, char *dataname);
#ifndef NO_SIGNALS
void dw_signal_connect(HWND window, char *signame, void *sigfunc, void *data);
void dw_signal_disconnect_by_window(HWND window);
void dw_signal_disconnect_by_data(HWND window, void *data);
void dw_signal_disconnect_by_name(HWND window, char *signame);
#endif

#endif
