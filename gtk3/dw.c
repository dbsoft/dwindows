/*
 * Dynamic Windows:
 *          A GTK like cross-platform GUI
 *          GTK3 forwarder module for portabilty.
 *
 * (C) 2000-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2003-2022 Mark Hessling <mark@rexx.org>
 * (C) 2002 Nickolay V. Shmyrev <shmyrev@yandex.ru>
 */
#include "dwconfig.h"
#include "dw.h"
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>


#ifdef USE_WEBKIT2
#include <webkit2/webkit2.h>
#if !WEBKIT_CHECK_VERSION(2, 22, 0)
#include <JavaScriptCore/JavaScript.h>
#endif
#elif defined(USE_WEBKIT)
#include <webkit/webkit.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if !GTK_CHECK_VERSION(3,1,0)
#error GTK 3.0 is no longer supported, please use 3.2 or later.
#endif

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

/* These are used for legacy resource management 
 * Starting with Dynamic Windows 3.1 the default 
 * compiled in resource management is GResource.
 */
#ifdef DW_INCLUDE_DEPRECATED_RESOURCES
#if defined(DW_RESOURCES) && !defined(BUILD_DLL)
extern DWResources _resources;
#endif
#endif

/* ff = 255 = 1.0000
 * ee = 238 = 0.9333
 * cc = 204 = 0.8000
 * bb = 187 = 0.7333
 * aa = 170 = 0.6667
 * 77 = 119 = 0.4667
 * 00 = 0   = 0.0000
 */
GdkRGBA _dw_colors[] =
{
   { 0.0000, 0.0000, 0.0000, 1.0 },   /* 0  black */
   { 0.7333, 0.0000, 0.0000, 1.0 },   /* 1  red */
   { 0.0000, 0.7333, 0.0000, 1.0 },   /* 2  green */
   { 0.6667, 0.6667, 0.0000, 1.0 },   /* 3  yellow */
   { 0.0000, 0.0000, 0.8000, 1.0 },   /* 4  blue */
   { 0.7333, 0.0000, 0.7333, 1.0 },   /* 5  magenta */
   { 0.0000, 0.7333, 0.7333, 1.0 },   /* 6  cyan */
   { 0.7333, 0.7333, 0.7333, 1.0 },   /* 7  white */
   { 0.4667, 0.4667, 0.4667, 1.0 },   /* 8  grey */
   { 1.0000, 0.0000, 0.0000, 1.0 },   /* 9  bright red */
   { 0.0000, 1.0000, 0.0000, 1.0 },   /* 10 bright green */
   { 0.9333, 0.9333, 0.0000, 1.0 },   /* 11 bright yellow */
   { 0.0000, 0.0000, 1.0000, 1.0 },   /* 12 bright blue */
   { 1.0000, 0.0000, 1.0000, 1.0 },   /* 13 bright magenta */
   { 0.0000, 0.9333, 0.9333, 1.0 },   /* 14 bright cyan */
   { 1.0000, 1.0000, 1.0000, 1.0 },   /* 15 bright white */
};

/*
 * List those icons that have transparency first
 */
#define NUM_EXTS 9
char *_dw_image_exts[NUM_EXTS] =
{
   ".xpm",
   ".png",
   ".ico",
   ".icns",
   ".gif",
   ".jpg",
   ".jpeg",
   ".tiff",
   ".bmp"
};

#ifndef max
# define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
# define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif

static pthread_key_t _dw_fg_color_key;
static pthread_key_t _dw_bg_color_key;
static pthread_key_t _dw_mutex_key;

static GList *_dw_dirty_list = NULL;

static GtkWidget *_dw_popup = NULL;

static int _dw_ignore_click = 0, _dw_ignore_expand = 0;
static pthread_t _dw_thread = (pthread_t)-1;

void (*_dw_gdk_threads_init)(void) = NULL;
void (*_dw_gdk_threads_enter)(void) = NULL;
void (*_dw_gdk_threads_leave)(void) = NULL;

#define  DW_MUTEX_LOCK { if(pthread_self() != _dw_thread && !pthread_getspecific(_dw_mutex_key)) { _dw_gdk_threads_enter(); pthread_setspecific(_dw_mutex_key, (void *)&_dw_locked_by_me); _dw_locked_by_me = TRUE; } }
#define  DW_MUTEX_UNLOCK { if(pthread_self() != _dw_thread && _dw_locked_by_me == TRUE) { _dw_gdk_threads_leave(); pthread_setspecific(_dw_mutex_key, NULL); _dw_locked_by_me = FALSE; } }

#define DEFAULT_SIZE_WIDTH 12
#define DEFAULT_SIZE_HEIGHT 6
#define DEFAULT_TITLEBAR_HEIGHT 22

#define _DW_TREE_TYPE_CONTAINER  1
#define _DW_TREE_TYPE_TREE       2
#define _DW_TREE_TYPE_LISTBOX    3
#define _DW_TREE_TYPE_COMBOBOX   4

/* Signal forwarder prototypes */
static gint _dw_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _dw_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _dw_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gint _dw_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint _dw_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gint _dw_generic_event(GtkWidget *widget, gpointer data);
static gint _dw_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gint _dw_activate_event(GtkWidget *widget, gpointer data);
static gint _dw_container_enter_event(GtkWidget *widget, GdkEventAny *event, gpointer data);
static gint _dw_combobox_select_event(GtkWidget *widget, gpointer data);
static gint _dw_expose_event(GtkWidget *widget, cairo_t *cr, gpointer data);
static gint _dw_set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data);
static gint _dw_tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _dw_value_changed_event(GtkWidget *widget, gpointer user_data);
static gint _dw_tree_select_event(GtkTreeSelection *sel, gpointer data);
static gint _dw_tree_expand_event(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer data);
static gint _dw_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
static gint _dw_column_click_event(GtkWidget *widget, gpointer data);
static void _dw_html_result_event(GObject *object, GAsyncResult *result, gpointer script_data);
#ifdef USE_WEBKIT
#ifdef USE_WEBKIT2
static void _dw_html_message_event(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer *data);
static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitLoadEvent load_event, gpointer data);
#else
static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitWebFrame *frame, gpointer user_data);
#endif
#endif
static void _dw_signal_disconnect(gpointer data, GClosure *closure);

GObject *_DWObject = NULL;
#if GLIB_CHECK_VERSION(2,28,0)
GApplication *_DWApp = NULL;
static char _dw_app_id[_DW_APP_ID_SIZE+1] = { 0 };
#endif
char *_DWDefaultFont = NULL;
static char _dw_share_path[PATH_MAX+1] = { 0 };
static int _dw_render_safe_mode = DW_FEATURE_UNSUPPORTED;

/* Return TRUE if it is safe to draw on the window handle.
 * Either we are in unsafe mode, or we are in an EXPOSE
 * event for the requested render window handle.
 */
int _dw_render_safe_check(GtkWidget *handle)
{
    if(_dw_render_safe_mode == DW_FEATURE_DISABLED || 
       (handle && g_object_get_data(G_OBJECT(handle), "_dw_expose")))
           return TRUE;
    return FALSE;
}

int _dw_is_render(GtkWidget *handle)
{
   if(GTK_IS_DRAWING_AREA(handle))
       return TRUE;
   return FALSE;
}

typedef struct
{
   void *func;
   char name[30];

} DWSignalList;

typedef struct
{
   HWND window;
   void *func;
   gpointer data;
   gint cid;
   void *intfunc;

} DWSignalHandler;

/* A list of signal forwarders, to account for parameter differences. */
static DWSignalList DWSignalTranslate[] = {
   { _dw_configure_event,         DW_SIGNAL_CONFIGURE },
   { _dw_key_press_event,         DW_SIGNAL_KEY_PRESS },
   { _dw_button_press_event,      DW_SIGNAL_BUTTON_PRESS },
   { _dw_button_release_event,    DW_SIGNAL_BUTTON_RELEASE },
   { _dw_motion_notify_event,     DW_SIGNAL_MOTION_NOTIFY },
   { _dw_delete_event,            DW_SIGNAL_DELETE },
   { _dw_expose_event,            DW_SIGNAL_EXPOSE },
   { _dw_activate_event,          "activate" },
   { _dw_generic_event,           DW_SIGNAL_CLICKED },
   { _dw_container_enter_event,   DW_SIGNAL_ITEM_ENTER },
   { _dw_tree_context_event,      DW_SIGNAL_ITEM_CONTEXT },
   { _dw_combobox_select_event,   DW_SIGNAL_LIST_SELECT },
   { _dw_tree_select_event,       DW_SIGNAL_ITEM_SELECT },
   { _dw_set_focus_event,         DW_SIGNAL_SET_FOCUS },
   { _dw_value_changed_event,     DW_SIGNAL_VALUE_CHANGED },
   { _dw_switch_page_event,       DW_SIGNAL_SWITCH_PAGE },
   { _dw_column_click_event,      DW_SIGNAL_COLUMN_CLICK },
   { _dw_tree_expand_event,       DW_SIGNAL_TREE_EXPAND },
#ifdef USE_WEBKIT
   { _dw_html_changed_event,      DW_SIGNAL_HTML_CHANGED },
#else
   { _dw_generic_event,           DW_SIGNAL_HTML_CHANGED },
#endif
   { _dw_html_result_event,       DW_SIGNAL_HTML_RESULT },
#ifdef USE_WEBKIT2
   { _dw_html_message_event,      DW_SIGNAL_HTML_MESSAGE },
#endif
   { NULL,                        "" }
};

/* Alignment flags */
#define DW_CENTER 0.5f
#define DW_LEFT 0.0f
#define DW_RIGHT 1.0f
#define DW_TOP 0.0f
#define DW_BOTTOM 1.0f

#ifdef DW_INCLUDE_DEPRECATED
/* MDI Support Code */
static void _dw_override_color(GtkWidget *widget, const char *element, GdkRGBA *color);
static void _dw_override_font(GtkWidget *widget, const char *font);
#define GTK_MDI(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_mdi_get_type (), GtkMdi)
#define GTK_MDI_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_mdi_get_type (), GtkMdiClass)
#define GTK_IS_MDI(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_mdi_get_type ())

typedef struct _GtkMdi GtkMdi;
typedef struct _GtkMdiClass GtkMdiClass;
typedef struct _GtkMdiDragInfo GtkMdiDragInfo;
typedef enum _GtkMdiChildState GtkMdiChildState;

enum _GtkMdiChildState
{
   CHILD_NORMAL,
   CHILD_MAXIMIZED,
   CHILD_ICONIFIED
};

struct _GtkMdi
{
   GtkContainer container;
   GList *children;

   GdkPoint drag_start;
   gint drag_button;
};

struct _GtkMdiClass
{
   GtkContainerClass parent_class;

   void (*mdi) (GtkMdi * mdi);
};

#include "gtk/maximize.xpm"
#include "gtk/minimize.xpm"
#include "gtk/kill.xpm"

#define GTK_MDI_BACKGROUND "Grey70"
#define GTK_MDI_LABEL_BACKGROUND    "RoyalBlue4"
#define GTK_MDI_LABEL_FOREGROUND    "white"
#define GTK_MDI_DEFAULT_WIDTH 0
#define GTK_MDI_DEFAULT_HEIGHT 0
#define GTK_MDI_MIN_HEIGHT 22
#define GTK_MDI_MIN_WIDTH  55

typedef struct _GtkMdiChild GtkMdiChild;

struct _GtkMdiChild
{
   GtkWidget *widget;

   GtkWidget *child;
   GtkMdi *mdi;

   gint x;
   gint y;
   gint width;
   gint height;

   GtkMdiChildState state;
};

static void gtk_mdi_class_init(GtkMdiClass *klass);
static void gtk_mdi_init(GtkMdi *mdi);

static void gtk_mdi_realize(GtkWidget *widget);
static void gtk_mdi_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static gboolean gtk_mdi_draw(GtkWidget *widget, cairo_t *cr);
static void gtk_mdi_get_preferred_width (GtkWidget *widget, gint *minimum_width, gint *natural_width);
static void gtk_mdi_get_preferred_height (GtkWidget *widget, gint *minimum_height, gint *natural_height);

/* Callbacks */
static gboolean _dw_mdi_move_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean _dw_mdi_resize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean _dw_mdi_iconify_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean _dw_mdi_maximize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean _dw_mdi_kill_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);

static void gtk_mdi_add(GtkContainer *container, GtkWidget *widget);
static void gtk_mdi_remove_true(GtkContainer *container, GtkWidget *widget);
static void gtk_mdi_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

static GtkMdiChild *_dw_mdi_get_child(GtkMdi *mdi, GtkWidget * widget);

static GType gtk_mdi_get_type(void)
{
   static GType mdi_type = 0;

   if (!mdi_type)
   {

      static const GTypeInfo mdi_info =
      {
         sizeof (GtkMdiClass),
         NULL,
         NULL,
         (GClassInitFunc) gtk_mdi_class_init,
         NULL,
         NULL,
         sizeof (GtkMdi),
         0,
         (GInstanceInitFunc) gtk_mdi_init,
      };

      mdi_type = g_type_register_static (GTK_TYPE_CONTAINER, "GtkMdi", &mdi_info, 0);
   }
   return mdi_type;
}

/* Local data */
static GtkWidgetClass *parent_class = NULL;

static void gtk_mdi_class_init(GtkMdiClass *class)
{
   GtkWidgetClass *widget_class;
   GtkContainerClass *container_class;

   widget_class = (GtkWidgetClass *) class;
   container_class = (GtkContainerClass *) class;

   parent_class = g_type_class_ref (GTK_TYPE_CONTAINER);

   widget_class->realize = gtk_mdi_realize;
   widget_class->draw = gtk_mdi_draw;
   widget_class->get_preferred_height = gtk_mdi_get_preferred_height;
   widget_class->get_preferred_width = gtk_mdi_get_preferred_width;
   widget_class->size_allocate = gtk_mdi_size_allocate;

   container_class->add = gtk_mdi_add;
   container_class->remove = gtk_mdi_remove_true;
   container_class->forall = gtk_mdi_forall;
   class->mdi = NULL;
}

static void gtk_mdi_init(GtkMdi *mdi)
{
   mdi->drag_button = -1;
   mdi->children = NULL;
}

static GtkWidget *gtk_mdi_new(void)
{
   GtkWidget *mdi;
   GdkRGBA background;

   mdi = GTK_WIDGET (g_object_new (gtk_mdi_get_type (), NULL));
   gdk_rgba_parse (&background, GTK_MDI_BACKGROUND);
   _dw_override_color (mdi, "background-color", &background);

   return mdi;
}

static void gtk_mdi_put(GtkMdi *mdi, GtkWidget *child_widget, gint x, gint y, GtkWidget *label)
{
   GtkMdiChild *child;

   GtkWidget *grid;
   GtkWidget *button[3];

   GtkWidget *child_box;
   GtkWidget *top_event_box;
   GtkWidget *bottom_event_box;
   GtkWidget *child_widget_box;
   GtkWidget *image;

   GdkRGBA color;
   gint i, j;
   GdkCursor *cursor;
   GdkPixbuf *pixbuf;

   child_box = gtk_event_box_new ();
   child_widget_box = gtk_event_box_new ();
#if GTK_CHECK_VERSION(3,12,0)
   gtk_widget_set_margin_start(child_widget_box, 2);
#else
   gtk_widget_set_margin_left(child_widget_box, 2);
#endif
   top_event_box = gtk_event_box_new ();
   bottom_event_box = gtk_event_box_new ();
   grid = gtk_grid_new ();
   gtk_grid_set_row_spacing(GTK_GRID(grid), 1);
   gtk_grid_set_column_spacing(GTK_GRID(grid), 1);

   for (i = 0; i < 3; i++)
   {
      button[i] = gtk_event_box_new ();
      gtk_widget_set_events (button[i], GDK_BUTTON_PRESS_MASK);
   }

   gdk_rgba_parse (&color, GTK_MDI_LABEL_BACKGROUND);

   _dw_override_color (top_event_box, "background-color", &color);
   _dw_override_color (bottom_event_box, "background-color", &color);
   _dw_override_color (child_box, "background-color", &color);
   for (j = 0; j < 3; j++)
   {
      _dw_override_color (button[j], "background-color", &color);
   }
   gdk_rgba_parse (&color, GTK_MDI_LABEL_FOREGROUND);
   _dw_override_color (label, "color", &color);
#if GTK_CHECK_VERSION(3,16,0)
   gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
   gtk_label_set_yalign(GTK_LABEL(label), 0.5f);
#else
   gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);	
#endif	

   gtk_container_add (GTK_CONTAINER (top_event_box), label);
   gtk_container_add (GTK_CONTAINER (child_widget_box), child_widget);
   gtk_widget_set_size_request (bottom_event_box, 2, 2);


   pixbuf =  gdk_pixbuf_new_from_xpm_data((const gchar **)minimize_xpm);
   image = gtk_image_new_from_pixbuf(pixbuf);
   gtk_widget_show(image);
   gtk_container_add (GTK_CONTAINER (button[0]), image);
   pixbuf =  gdk_pixbuf_new_from_xpm_data((const gchar **) maximize_xpm);
   image = gtk_image_new_from_pixbuf(pixbuf);
   gtk_widget_show(image);
   gtk_container_add (GTK_CONTAINER (button[1]), image);
   pixbuf =  gdk_pixbuf_new_from_xpm_data((const gchar **) kill_xpm);
   image = gtk_image_new_from_pixbuf(pixbuf);
   gtk_widget_show(image);
   gtk_container_add (GTK_CONTAINER (button[2]), image);


   /* Titlebar and buttons */
   gtk_widget_set_hexpand(top_event_box, TRUE);
   gtk_grid_attach (GTK_GRID (grid), top_event_box, 0, 0, 1, 1);
   gtk_grid_attach (GTK_GRID (grid), button[0], 1, 0, 1, 1);
   gtk_grid_attach (GTK_GRID (grid), button[1], 2, 0, 1, 1);
   gtk_grid_attach (GTK_GRID (grid), button[2], 3, 0, 1, 1);

   /* Window contents */
   gtk_widget_set_hexpand(child_widget_box, TRUE);
   gtk_widget_set_vexpand(child_widget_box, TRUE);
   gtk_grid_attach (GTK_GRID (grid), child_widget_box, 0, 1, 4, 1);

   /* Bottom border */
   gtk_grid_attach (GTK_GRID (grid), bottom_event_box, 4, 2, 1, 1);

   gtk_container_add (GTK_CONTAINER (child_box), grid);

   child = g_new (GtkMdiChild, 1);
   child->widget = child_box;
   child->x = x;
   child->y = y;
   child->width = -1;
   child->height = -1;
   child->child = child_widget;
   child->mdi = mdi;
   child->state = CHILD_NORMAL;

   gtk_widget_set_parent (child_box, GTK_WIDGET (mdi));
   mdi->children = g_list_append (mdi->children, child);

   gtk_widget_show (child_box);
   gtk_widget_show (grid);
   gtk_widget_show (top_event_box);
   gtk_widget_show (bottom_event_box);
   gtk_widget_show (child_widget_box);
   for (i = 0; i < 3; i++)
   {
      gtk_widget_show (button[i]);
   }

   cursor = gdk_cursor_new_for_display (gdk_display_get_default(), GDK_HAND1);
   gtk_widget_realize (top_event_box);
   gdk_window_set_cursor (gtk_widget_get_window(top_event_box), cursor);
   cursor = gdk_cursor_new_for_display (gdk_display_get_default(), GDK_BOTTOM_RIGHT_CORNER);
   gtk_widget_realize (bottom_event_box);
   gdk_window_set_cursor (gtk_widget_get_window(bottom_event_box), cursor);

   g_signal_connect (G_OBJECT (top_event_box), "event",
                 G_CALLBACK (_dw_mdi_move_child_callback),
                 child);
   g_signal_connect (G_OBJECT (bottom_event_box), "event",
                 G_CALLBACK (_dw_mdi_resize_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[0]), "button_press_event",
                 G_CALLBACK (_dw_mdi_iconify_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[1]), "button_press_event",
                 G_CALLBACK (_dw_mdi_maximize_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[2]), "button_press_event",
                 G_CALLBACK (_dw_mdi_kill_child_callback),
                 child);
}

static void gtk_mdi_move(GtkMdi *mdi, GtkWidget *widget, gint x, gint y)
{
   GtkMdiChild *child;

   g_return_if_fail(GTK_IS_MDI(mdi));
   g_return_if_fail(GTK_IS_WIDGET(widget));

   child = _dw_mdi_get_child(mdi, widget);
   g_return_if_fail(child);

   child->x = x;
   child->y = y;
   if (gtk_widget_get_visible(widget) && gtk_widget_get_visible(GTK_WIDGET(mdi)))
      gtk_widget_queue_resize(GTK_WIDGET(widget));
}

static void gtk_mdi_get_pos(GtkMdi *mdi, GtkWidget *widget, gint *x, gint *y)
{
   GtkMdiChild *child;

   g_return_if_fail(GTK_IS_MDI (mdi));
   g_return_if_fail(GTK_IS_WIDGET (widget));

   child = _dw_mdi_get_child(mdi, widget);
   g_return_if_fail(child);

   *x = child->x;
   *y = child->y;
}

static void gtk_mdi_set_state(GtkMdi *mdi, GtkWidget *widget, GtkMdiChildState state)
{
   GtkMdiChild *child;

   g_return_if_fail (GTK_IS_MDI (mdi));
   g_return_if_fail (GTK_IS_WIDGET (widget));

   child = _dw_mdi_get_child (mdi, widget);
   g_return_if_fail (child);

   child->state = state;
   if (gtk_widget_get_visible(child->widget) && gtk_widget_get_visible(GTK_WIDGET(mdi)))
      gtk_widget_queue_resize(GTK_WIDGET(child->widget));
}

static void gtk_mdi_remove(GtkMdi *mdi, GtkWidget *widget)
{
   GtkMdiChild *child;

   g_return_if_fail (GTK_IS_MDI (mdi));
   child = _dw_mdi_get_child (mdi, widget);
   g_return_if_fail (child);
   gtk_mdi_remove_true (GTK_CONTAINER (mdi), child->widget);
}

static void gtk_mdi_realize(GtkWidget *widget)
{
   GtkMdi *mdi;
   GdkWindowAttr attributes;
   gint attributes_mask;
   GdkWindow *thiswindow;
   GtkAllocation allocation;

   mdi = GTK_MDI (widget);

   g_return_if_fail (widget != NULL);
   g_return_if_fail (GTK_IS_MDI (mdi));

   gtk_widget_set_realized(widget, TRUE);

   gtk_widget_get_allocation(widget, &allocation);
   attributes.x = allocation.x;
   attributes.y = allocation.y;
   attributes.width = allocation.width;
   attributes.height = allocation.height;
   attributes.wclass = GDK_INPUT_OUTPUT;
   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.event_mask = gtk_widget_get_events (widget) |
      GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
      GDK_POINTER_MOTION_HINT_MASK;
   attributes.visual = gtk_widget_get_visual (widget);

   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
   thiswindow = gdk_window_new (gtk_widget_get_parent_window(widget), &attributes, attributes_mask);
   gtk_widget_set_window(widget, thiswindow);

   gdk_window_set_user_data (gtk_widget_get_window(widget), widget);
}

static void gtk_mdi_get_preferred_width (GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
   GtkMdi *mdi;
   GtkMdiChild *child;
   GList *children;
   gint child_minimum_width, child_natural_width;

   mdi = GTK_MDI (widget);
   *natural_width = *minimum_width = GTK_MDI_DEFAULT_WIDTH;

   children = mdi->children;
   while(children)
   {
      child = children->data;
      children = children->next;

      if(gtk_widget_get_visible(child->widget))
      {
         gtk_widget_get_preferred_width(child->widget, &child_minimum_width, &child_natural_width);
      }
   }
}

static void gtk_mdi_get_preferred_height (GtkWidget *widget, gint *minimum_height, gint *natural_height)
{
   GtkMdi *mdi;
   GtkMdiChild *child;
   GList *children;
   gint child_minimum_height, child_natural_height;

   mdi = GTK_MDI (widget);
   *natural_height = *minimum_height = GTK_MDI_DEFAULT_HEIGHT;

   children = mdi->children;
   while(children)
   {
      child = children->data;
      children = children->next;

      if(gtk_widget_get_visible(child->widget))
      {
         gtk_widget_get_preferred_height(child->widget, &child_minimum_height, &child_natural_height);
      }
   }
}

static void gtk_mdi_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
   GtkMdi *mdi;
   GtkMdiChild *child;
   GtkAllocation child_allocation;
   GtkRequisition child_requisition;
   GList *children;

   mdi = GTK_MDI (widget);

   gtk_widget_set_allocation(widget, allocation);

   if(gtk_widget_get_realized(widget))
      gdk_window_move_resize (gtk_widget_get_window(widget),
                        allocation->x,
                        allocation->y,
                        allocation->width,
                        allocation->height);


   children = mdi->children;
   while(children)
   {
      child = children->data;
      children = children->next;

      if(gtk_widget_get_visible(child->widget))
      {
         gtk_widget_get_preferred_size (child->widget, NULL, &child_requisition);
         child_allocation.x = 0;
         child_allocation.y = 0;
         switch (child->state)
         {
         case CHILD_NORMAL:
            {
               if ((child->width < 0) && (child->height < 0))
               {
                  child_allocation.width = child_requisition.width;
                  child_allocation.height = child_requisition.height;
               }
               else
               {
                  child_allocation.width = child->width;
                  child_allocation.height = child->height;
                  child->width = -1;
                  child->height = -1;
               }
               child_allocation.x += child->x;
               child_allocation.y += child->y;
               break;
            }
         case CHILD_MAXIMIZED:
            {
               if ((child->width < 0) && (child->height < 0))
               {
                  child->width = child_requisition.width;
                  child->height = child_requisition.height;
               }
               child_allocation.width = allocation->width;
               child_allocation.height = allocation->height;
            }
            break;
         case CHILD_ICONIFIED:
            {
               if ((child->width < 0) && (child->height < 0))
               {
                  child->width = child_requisition.width;
                  child->height = child_requisition.height;
               }
               child_allocation.x += child->x;
               child_allocation.y += child->y;
               child_allocation.width = child_requisition.width;
               child_allocation.height = GTK_MDI_MIN_HEIGHT;
               break;
            }
         }
         gtk_widget_size_allocate (child->widget, &child_allocation);
      }
   }
}

static gboolean gtk_mdi_draw(GtkWidget *widget, cairo_t *cr)
{
   GtkMdiChild *child;
   GList *children;
   GtkMdi *mdi;
   GtkStyleContext *context;

   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GTK_IS_MDI (widget), FALSE);
   g_return_val_if_fail (cr != NULL, FALSE);

   mdi = GTK_MDI (widget);

   if((context = gtk_widget_get_style_context(widget)))
   {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);
      gtk_render_background (context, cr, 0, 0, allocation.x, allocation.y);
   }

   for (children = mdi->children; children; children = children->next)
   {
      child = (GtkMdiChild *) children->data;
      gtk_container_propagate_draw (GTK_CONTAINER (mdi),
                              child->widget,
                              cr);
   }
   return FALSE;
}

static void gtk_mdi_add(GtkContainer *container, GtkWidget *widget)
{
   GtkWidget *label;
   label = gtk_label_new ("");
   gtk_mdi_put (GTK_MDI (container), widget, 0, 0, label);
}

static void gtk_mdi_remove_true(GtkContainer *container, GtkWidget *widget)
{
   GtkMdi *mdi;
   GtkMdiChild *child = NULL;
   GList *children;

   mdi = GTK_MDI (container);

   children = mdi->children;
   while (children)
   {
      child = children->data;
      if (child->widget == widget)
         break;

      children = children->next;
   }

   if(child)
   {
      gtk_widget_unparent (child->widget);
      g_free (child);
   }
   mdi->children = g_list_remove_link (mdi->children, children);
   g_list_free (children);
}

static void gtk_mdi_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
   GtkMdi *mdi;
   GtkMdiChild *child;
   GList *children;

   g_return_if_fail (callback != NULL);

   mdi = GTK_MDI (container);

   children = mdi->children;
   while (children)
   {
      child = children->data;
      children = children->next;

      (*callback) (child->widget, callback_data);
   }
}

static gboolean _dw_mdi_move_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GtkMdi *mdi;
   GtkMdiChild *child;

   child = (GtkMdiChild *) data;
   mdi = child->mdi;

   g_return_val_if_fail (GTK_IS_MDI (mdi), FALSE);
   g_return_val_if_fail (GTK_IS_EVENT_BOX (widget), FALSE);


   switch (event->type)
   {
   case GDK_2BUTTON_PRESS:
      {
         gdk_window_raise (gtk_widget_get_window(child->widget));
      }
   case GDK_BUTTON_PRESS:
      if (child->state == CHILD_MAXIMIZED)
         return FALSE;
      if (mdi->drag_button < 0)
      {
#if GTK_CHECK_VERSION(3,20,0)
         if (gdk_seat_grab (gdk_event_get_seat(event),
                          event->button.window,
                          GDK_SEAT_CAPABILITY_ALL_POINTING,
                          FALSE, NULL, event, NULL, NULL) != GDK_GRAB_SUCCESS)
#else
         if (gdk_device_grab (gdk_event_get_device(event),
                          event->button.window,
                          GDK_OWNERSHIP_WINDOW,
                          FALSE,
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                          GDK_BUTTON_RELEASE_MASK,
                          NULL,
                          event->button.time) != GDK_GRAB_SUCCESS)
#endif
            return FALSE;

         mdi->drag_button = event->button.button;

         mdi->drag_start.x = event->button.x;
         mdi->drag_start.y = event->button.y;
      }
      break;

   case GDK_BUTTON_RELEASE:
      if (mdi->drag_button < 0)
         return FALSE;

      if (mdi->drag_button == event->button.button)
      {
         int x, y;

#if GTK_CHECK_VERSION(3,20,0)
         gdk_seat_ungrab (gdk_event_get_seat(event));
#else
         gdk_device_ungrab (gdk_event_get_device(event), event->button.time);
#endif
         mdi->drag_button = -1;

         x = event->button.x + child->x - mdi->drag_start.x;
         y = event->button.y + child->y - mdi->drag_start.y;

         gtk_mdi_move (mdi, child->child, x, y);
      }
      break;

   case GDK_MOTION_NOTIFY:
      {
         int x, y;

         if (mdi->drag_button < 0)
            return FALSE;

         gdk_window_get_device_position (gtk_widget_get_window(widget), gdk_event_get_device(event), &x, &y, NULL);


         x = x - mdi->drag_start.x + child->x;
         y = y - mdi->drag_start.y + child->y;


         gtk_mdi_move (mdi, child->child, x, y);
      }
      break;

   default:
      break;
   }

   return FALSE;
}

static gboolean _dw_mdi_resize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GtkMdi *mdi;
   GtkMdiChild *child;

   child = (GtkMdiChild *) data;
   mdi = child->mdi;

   g_return_val_if_fail (GTK_IS_MDI (mdi), FALSE);
   g_return_val_if_fail (GTK_IS_EVENT_BOX (widget), FALSE);

   switch (event->type)
   {
   case GDK_BUTTON_PRESS:
      if (mdi->drag_button < 0)
      {
#if GTK_CHECK_VERSION(3,20,0)
         if (gdk_seat_grab (gdk_event_get_seat(event),
                          event->button.window,
                          GDK_SEAT_CAPABILITY_ALL_POINTING,
                          FALSE, NULL, event, NULL, NULL) != GDK_GRAB_SUCCESS)
#else
         if (gdk_device_grab (gdk_event_get_device(event),
                          event->button.window,
                          GDK_OWNERSHIP_WINDOW,
                          FALSE,
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                          GDK_BUTTON_RELEASE_MASK,
                          NULL,
                          event->button.time) != GDK_GRAB_SUCCESS)
#endif
            return FALSE;

         mdi->drag_button = event->button.button;
         if ((child->state == CHILD_MAXIMIZED) || (child->state == CHILD_ICONIFIED))
         {
            GtkAllocation allocation;

            child->state = CHILD_NORMAL;
            gtk_widget_get_allocation(child->widget, &allocation);
            child->x = allocation.x;
            child->y = allocation.y;
            child->width = allocation.width;
            child->height = allocation.height;
         }

      }
      break;

   case GDK_BUTTON_RELEASE:
      if (mdi->drag_button < 0)
         return FALSE;

      if (mdi->drag_button == event->button.button)
      {
         int width, height;
         GtkAllocation allocation;

#if GTK_CHECK_VERSION(3,20,0)
         gdk_seat_ungrab (gdk_event_get_seat(event));
#else
         gdk_device_ungrab (gdk_event_get_device(event), event->button.time);
#endif
         mdi->drag_button = -1;

         gtk_widget_get_allocation(widget, &allocation);
         width = event->button.x + allocation.x;
         height = event->button.y + allocation.y;

         width = MAX (width, GTK_MDI_MIN_WIDTH);
         height = MAX (height, GTK_MDI_MIN_HEIGHT);

         gtk_widget_set_size_request (child->widget, width, height);
         gtk_widget_queue_resize (child->widget);
      }
      break;

   case GDK_MOTION_NOTIFY:
      {
         int x, y;
         int width, height;
         GtkAllocation allocation;

         if (mdi->drag_button < 0)
            return FALSE;

         gdk_window_get_device_position (gtk_widget_get_window(widget), gdk_event_get_device(event), &x, &y, NULL);

         gtk_widget_get_allocation(widget, &allocation);
         width = x + allocation.x;
         height = y + allocation.y;

         width = MAX (width, GTK_MDI_MIN_WIDTH);
         height = MAX (height, GTK_MDI_MIN_HEIGHT);

         gtk_widget_set_size_request (child->widget, width, height);
         gtk_widget_queue_resize (child->widget);
      }
      break;

   default:
      break;
   }

   return FALSE;
}

static gboolean _dw_mdi_iconify_child_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GtkMdiChild *child;
   child = (GtkMdiChild *) data;
   if(child->state == CHILD_ICONIFIED)
   {
      child->state = CHILD_NORMAL;
   }
   else
   {
      child->state = CHILD_ICONIFIED;
   }
   if(gtk_widget_get_visible(child->widget))
      gtk_widget_queue_resize(GTK_WIDGET (child->widget));
   return FALSE;
}

static gboolean _dw_mdi_maximize_child_callback (GtkWidget *widget, GdkEvent * event, gpointer data)
{
   GtkMdiChild *child;
   child = (GtkMdiChild *) data;
   if (child->state == CHILD_MAXIMIZED)
   {
      child->state = CHILD_NORMAL;
   }
   else
   {
      child->state = CHILD_MAXIMIZED;
   }
   if(gtk_widget_get_visible(child->widget))
      gtk_widget_queue_resize(GTK_WIDGET (child->widget));
   return FALSE;
}

static gboolean _dw_mdi_kill_child_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GtkMdiChild *child;
   GtkMdi *mdi;

   child = (GtkMdiChild *) data;
   mdi = child->mdi;

   g_return_val_if_fail (GTK_IS_MDI (mdi), FALSE);

   gtk_mdi_remove_true (GTK_CONTAINER (mdi), child->widget);
   return FALSE;
}

static GtkMdiChild *_dw_mdi_get_child (GtkMdi *mdi, GtkWidget *widget)
{
   GList *children;

   children = mdi->children;
   while (children)
   {
      GtkMdiChild *child;

      child = children->data;
      children = children->next;

      if (child->child == widget)
         return child;
   }

   return NULL;
}
#endif

static void _dw_msleep(long period)
{
#ifdef __sun__
   /* usleep() isn't threadsafe on Solaris */
   struct timespec req;

   req.tv_sec = 0;
   if(period >= 1000)
   {
      req.tv_sec = (int)(period / 1000);
      period -= (req.tv_sec * 1000);
   }
   req.tv_nsec = period * 10000000;
	
   nanosleep(&req, NULL);
#else
   usleep(period * 1000);
#endif
}

/* Finds the translation function for a given signal name */
static DWSignalList _dw_findsignal(const char *signame)
{
   int z=0;
   static DWSignalList empty = {0};

   while(DWSignalTranslate[z].func)
   {
      if(strcasecmp(signame, DWSignalTranslate[z].name) == 0)
         return DWSignalTranslate[z];
      z++;
   }
   return empty;
}

static DWSignalHandler _dw_get_signal_handler(gpointer data)
{
   DWSignalHandler sh = {0};

   if(data)
   {
      void **params = (void **)data;
      int counter = GPOINTER_TO_INT(params[0]);
      GtkWidget *widget = (GtkWidget *)params[2];
      char text[101] = {0};

      snprintf(text, 100, "_dw_sigwindow%d", counter);
      sh.window = (HWND)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigfunc%d", counter);
      sh.func = (void *)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_intfunc%d", counter);
      sh.intfunc = (void *)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigdata%d", counter);
      sh.data = g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigcid%d", counter);
      sh.cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));
   }
   return sh;
}

static void _dw_remove_signal_handler(GtkWidget *widget, int counter)
{
   char text[101] = {0};
   gint cid;

   snprintf(text, 100, "_dw_sigcid%d", counter);
   cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));
   if(cid > 0)
      g_signal_handler_disconnect(G_OBJECT(widget), cid);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigwindow%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_intfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigdata%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
}

static int _dw_set_signal_handler(GtkWidget *widget, HWND window, void *func, gpointer data, void *intfunc, void *discfunc)
{
   int counter = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_sigcounter"));
   char text[101] = {0};

   snprintf(text, 100, "_dw_sigwindow%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)window);
   snprintf(text, 100, "_dw_sigfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)func);
   snprintf(text, 100, "_dw_intfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)intfunc);
   snprintf(text, 100, "_dw_discfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)discfunc);
   snprintf(text, 100, "_dw_sigdata%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)data);

   counter++;
   g_object_set_data(G_OBJECT(widget), "_dw_sigcounter", GINT_TO_POINTER(counter));

   return counter - 1;
}

static void _dw_set_signal_handler_id(GtkWidget *widget, int counter, gint cid)
{
   if(cid > 0)
   {
      char text[101] = {0};

      snprintf(text, 100, "_dw_sigcid%d", counter);
      g_object_set_data(G_OBJECT(widget), text, GINT_TO_POINTER(cid));
   }
   else
      dw_debug("WARNING: Dynamic Windows failed to connect signal.\n");
}

static void _dw_html_result_event(GObject *object, GAsyncResult *result, gpointer script_data)
{
#if USE_WEBKIT2
    pthread_t saved_thread = _dw_thread;
#if !WEBKIT_CHECK_VERSION(2, 40, 0)
    WebKitJavascriptResult *js_result;
#endif
#if WEBKIT_CHECK_VERSION(2, 22, 0)
    JSCValue *value;
#else
    JSValueRef value;
    JSGlobalContextRef context;
#endif
    GError *error = NULL;
    int (*htmlresultfunc)(HWND, int, char *, void *, void *) = NULL;
    gint handlerdata = GPOINTER_TO_INT(g_object_get_data(object, "_dw_html_result_id"));
    void *user_data = NULL;

    _dw_thread = (pthread_t)-1;
    if(handlerdata)
    {
        DWSignalHandler work;
        void *params[3] = { GINT_TO_POINTER(handlerdata-1), 0, object };

        work = _dw_get_signal_handler(params);

        if(work.window)
        {
            htmlresultfunc = work.func;
            user_data = work.data;
        }
    }

#if WEBKIT_CHECK_VERSION(2, 40, 0)
    if(!(value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error)))
#else
    if(!(js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error)))
#endif
    {
        if(htmlresultfunc)
           htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, error->message, script_data, user_data);
        g_error_free(error);
        _dw_thread = saved_thread;
        return;
    }

#if WEBKIT_CHECK_VERSION(2, 22, 0)
#if !WEBKIT_CHECK_VERSION(2, 40, 0)
    value = webkit_javascript_result_get_js_value(js_result);
#endif
    if(jsc_value_is_string(value))
    {
        gchar *str_value = jsc_value_to_string(value);
        JSCException *exception = jsc_context_get_exception(jsc_value_get_context(value));
#else
    context = webkit_javascript_result_get_global_context(js_result);
    value = webkit_javascript_result_get_value(js_result);
    if (JSValueIsString(context, value))
    {
        JSStringRef js_str_value;
        gchar *str_value;
        gsize str_length;

        js_str_value = JSValueToStringCopy(context, value, NULL);
        str_length = JSStringGetMaximumUTF8CStringSize(js_str_value);
        str_value = (gchar *)g_malloc (str_length);
        JSStringGetUTF8CString(js_str_value, str_value, str_length);
        JSStringRelease(js_str_value);
#endif

        if(htmlresultfunc)
        {
#if WEBKIT_CHECK_VERSION(2, 22, 0)
           if(exception)
               htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, (char *)jsc_exception_get_message(exception), script_data, user_data);
           else
#endif
               htmlresultfunc((HWND)object, DW_ERROR_NONE, str_value, script_data, user_data);
        }
        g_free (str_value);
    }
    else if(htmlresultfunc)
        htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, NULL, script_data, user_data);
#if !WEBKIT_CHECK_VERSION(2, 40, 0)
    webkit_javascript_result_unref (js_result);
#endif
   _dw_thread = saved_thread;
#endif
}

#ifdef USE_WEBKIT2
static void _dw_html_message_event(WebKitUserContentManager *manager, WebKitJavascriptResult *js_result, gpointer *data)
{
    pthread_t saved_thread = _dw_thread;
    HWND window = (HWND)data[0];
    int (*htmlmessagefunc)(HWND, char *, char *, void *) = NULL;
    void *user_data = NULL;
    gchar *name = (gchar *)data[1];
    gint handlerdata;
#if WEBKIT_CHECK_VERSION(2, 22, 0)
    JSCValue *value;
#else
    JSValueRef value;
    JSGlobalContextRef context;
#endif

    _dw_thread = (pthread_t)-1;
    if(window && (handlerdata = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "_dw_html_message_id"))))
    {
        void *params[3] = { GINT_TO_POINTER(handlerdata-1), 0, window };
        DWSignalHandler work = _dw_get_signal_handler(params);

        if(work.window)
        {
            htmlmessagefunc = work.func;
            user_data = work.data;
        }
    }

#if WEBKIT_CHECK_VERSION(2, 22, 0)
    value = webkit_javascript_result_get_js_value(js_result);
    if(jsc_value_is_string(value))
    {
        gchar *str_value = jsc_value_to_string(value);
        JSCException *exception = jsc_context_get_exception(jsc_value_get_context(value));
#else
    context = webkit_javascript_result_get_global_context(js_result);
    value = webkit_javascript_result_get_value(js_result);
    if(JSValueIsString(context, value))
    {
        JSStringRef js_str_value;
        gchar *str_value;
        gsize str_length;
        void *exception = NULL;

        js_str_value = JSValueToStringCopy(context, value, NULL);
        str_length = JSStringGetMaximumUTF8CStringSize(js_str_value);
        str_value = (gchar *)g_malloc (str_length);
        JSStringGetUTF8CString(js_str_value, str_value, str_length);
        JSStringRelease(js_str_value);
#endif

        if(htmlmessagefunc && !exception)
            htmlmessagefunc(window, name, str_value, user_data);

        g_free(str_value);

        if(!exception)
        {
            _dw_thread = saved_thread;
            return;
        }
    }

    if(htmlmessagefunc)
        htmlmessagefunc(window, name, NULL, user_data);
   _dw_thread = saved_thread;
}
#endif

#ifdef USE_WEBKIT
#ifdef USE_WEBKIT2
static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitLoadEvent load_event, gpointer data)
{
    DWSignalHandler work = _dw_get_signal_handler(data);
    char *location = (char *)webkit_web_view_get_uri(web_view);
    int status = 0;

    switch (load_event) {
    case WEBKIT_LOAD_STARTED:
        status = DW_HTML_CHANGE_STARTED;
        break;
    case WEBKIT_LOAD_REDIRECTED:
        status = DW_HTML_CHANGE_REDIRECT;
        break;
    case WEBKIT_LOAD_COMMITTED:
        status = DW_HTML_CHANGE_LOADING;
        break;
    case WEBKIT_LOAD_FINISHED:
        status = DW_HTML_CHANGE_COMPLETE;
        break;
    }
    if(status && location && work.window && work.func)
    {
        int (*htmlchangedfunc)(HWND, int, char *, void *) = work.func;

        htmlchangedfunc(work.window, status, location, work.data);
    }
}
#else
static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitWebFrame *frame, gpointer data)
{
    DWSignalHandler work = _dw_get_signal_handler(data);
    char *location = (char *)webkit_web_view_get_uri(web_view);
    int status = 0;
    void **params = data;

    if(params)
      status = DW_POINTER_TO_INT(params[3]);

    if(status && location && work.window && work.func)
    {
        int (*htmlchangedfunc)(HWND, int, char *, void *) = work.func;

        htmlchangedfunc(work.window, status, location, work.data);
    }
}
#endif
#endif

static gint _dw_set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*setfocusfunc)(HWND, void *) = work.func;

      retval = setfocusfunc(work.window, work.data);
   }
   return retval;
}

static gint _dw_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = DW_BUTTON1_MASK;

      if(event->button == 3)
         mybutton = DW_BUTTON2_MASK;
      else if(event->button == 2)
         mybutton = DW_BUTTON3_MASK;

      retval = buttonfunc(work.window, event->x, event->y, mybutton, work.data);
   }
   return retval;
}

static gint _dw_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = DW_BUTTON3_MASK;

      if(event->button == 3)
         mybutton = DW_BUTTON2_MASK;
      else if(event->button == 2)
         mybutton = DW_BUTTON3_MASK;

      retval = buttonfunc(work.window, event->x, event->y, mybutton, work.data);
   }
   return retval;
}

static gint _dw_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*motionfunc)(HWND, int, int, int, void *) = work.func;
      int keys = 0, x, y;
      GdkModifierType state = 0;

      if (event->is_hint)
         gdk_window_get_device_position (event->window, gdk_event_get_device((GdkEvent *)event), &x, &y, &state);
      else
      {
         x = event->x;
         y = event->y;
         state = event->state;
      }

      if (state & GDK_BUTTON1_MASK)
         keys = DW_BUTTON1_MASK;
      if (state & GDK_BUTTON3_MASK)
         keys |= DW_BUTTON2_MASK;
      if (state & GDK_BUTTON2_MASK)
         keys |= DW_BUTTON3_MASK;

      retval = motionfunc(work.window, x, y, keys, work.data);
   }
   return retval;
}

static gint _dw_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*closefunc)(HWND, void *) = work.func;

      retval = closefunc(work.window, work.data);
   }
   return retval;
}

static gint _dw_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*keypressfunc)(HWND, char, int, int, void *, char *) = work.func;
      guint32 unichar = gdk_keyval_to_unicode(event->keyval);
      char utf8[7] = { 0 };

      g_unichar_to_utf8(unichar, utf8);

      retval = keypressfunc(work.window, *event->string, event->keyval,
                       event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK), work.data, utf8);
   }
   return retval;
}

static gint _dw_generic_event(GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*genericfunc)(HWND, void *) = work.func;

      retval = genericfunc(work.window, work.data);
   }
   return retval;
}

static gint _dw_activate_event(GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window && !_dw_ignore_click)
   {
      int (*activatefunc)(HWND, void *) = work.func;

      retval = activatefunc(_dw_popup ? _dw_popup : work.window, work.data);
      _dw_popup = NULL;
   }
   return retval;
}

static gint _dw_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*sizefunc)(HWND, int, int, void *) = work.func;

      retval = sizefunc(work.window, event->width, event->height, work.data);
   }
   return retval;
}

static gint _dw_expose_event(GtkWidget *widget, cairo_t *cr, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      DWExpose exp;
      int (*exposefunc)(HWND, DWExpose *, void *) = work.func;
      gpointer oldrender = NULL;

      /* Remove the currently drawn widget from the dirty list */
      _dw_dirty_list = g_list_remove(_dw_dirty_list, widget);

      exp.x = exp.y = 0;
      exp.width = gtk_widget_get_allocated_width(widget);
      exp.height = gtk_widget_get_allocated_height(widget);
      g_object_set_data(G_OBJECT(work.window), "_dw_cr", (gpointer)cr);
      if(_dw_render_safe_mode == DW_FEATURE_ENABLED && _dw_is_render(work.window))
      {
      	oldrender = g_object_get_data(G_OBJECT(work.window), "_dw_expose");
      	g_object_set_data(G_OBJECT(work.window), "_dw_expose", (gpointer)1);
      }
      retval = exposefunc(work.window, &exp, work.data);
      if(_dw_render_safe_mode == DW_FEATURE_ENABLED)
      	g_object_set_data(G_OBJECT(work.window), "_dw_expose", oldrender);
      g_object_set_data(G_OBJECT(work.window), "_dw_cr", NULL);
   }
   return retval;
}

static gint _dw_combobox_select_event(GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(g_object_get_data(G_OBJECT(widget), "_dw_recursing"))
      return FALSE;

   if(work.window && GTK_IS_COMBO_BOX(widget))
   {
      GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

      if(store)
      {
         GtkTreeIter iter;
         GtkTreePath *path;

         g_object_set_data(G_OBJECT(widget), "_dw_recursing", GINT_TO_POINTER(1));

         if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
         {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  int (*selectfunc)(HWND, int, void *) = work.func;

                  retval = selectfunc(work.window, indices[0], work.data);
               }
               gtk_tree_path_free(path);
            }
         }

         g_object_set_data(G_OBJECT(widget), "_dw_recursing", NULL);
      }
   }
   return retval;
}

#define _DW_DATA_TYPE_STRING  0
#define _DW_DATA_TYPE_POINTER 1

static gint _dw_tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      if(event->button == 3)
      {
         int (*contextfunc)(HWND, char *, int, int, void *, void *) = work.func;
         char *text = NULL;
         void *itemdata = NULL;

         if(widget && GTK_IS_TREE_VIEW(widget))
         {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
            GtkTreeIter iter;

            if(sel && gtk_tree_selection_get_mode(sel) != GTK_SELECTION_MULTIPLE &&
               gtk_tree_selection_get_selected(sel, NULL, &iter))
            {
               if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
               {
                  gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, -1);
               }
               else
               {
                  gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
               }
            }
            else
            {
               GtkTreePath *path;

               gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
               if(path)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
                  {
                     if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
                     {
                        gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, -1);
                     }
                     else
                     {
                        gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
                     }
                  }
                  gtk_tree_path_free(path);
               }
            }
         }

         retval = contextfunc(work.window, text, event->x, event->y, work.data, itemdata);
         if(text)
            g_free(text);
      }
   }
   return retval;
}

static gint _dw_tree_select_event(GtkTreeSelection *sel, gpointer data)
{
   GtkWidget *item = NULL, *widget = (GtkWidget *)gtk_tree_selection_get_tree_view(sel);
   int retval = FALSE;

   if(widget)
   {
      DWSignalHandler work = _dw_get_signal_handler(data);

      if(work.window)
      {
         int (*treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = work.func;
         GtkTreeIter iter;
         char *text = NULL;
         void *itemdata = NULL;
         GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

         if(g_object_get_data(G_OBJECT(widget), "_dw_double_click"))
         {
            g_object_set_data(G_OBJECT(widget), "_dw_double_click", GINT_TO_POINTER(0));
            return TRUE;
         }

         if(gtk_tree_selection_get_mode(sel) != GTK_SELECTION_MULTIPLE &&
            gtk_tree_selection_get_selected(sel, NULL, &iter))
         {
            if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
            {
               gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, 3, &item, -1);
               retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
            }
            else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
            {
               gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
               retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
            }
            else
            {
               GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

               if(path)
               {
                  gint *indices = gtk_tree_path_get_indices(path);

                  if(indices)
                  {
                     int (*selectfunc)(HWND, int, void *) = work.func;

                     retval = selectfunc(work.window, indices[0], work.data);
                  }
                  gtk_tree_path_free(path);
               }
            }
         }
         else
         {
            GtkTreePath *path;

            gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
            if(path)
            {
               GtkTreeIter iter;

               if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
               {
                  if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, 3, &item, -1);
                     retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
                  }
                  else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
                     retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
                  }
                  else
                  {
                     gint *indices = gtk_tree_path_get_indices(path);

                     if(indices)
                     {
                        int (*selectfunc)(HWND, int, void *) = work.func;

                        retval = selectfunc(work.window, indices[0], work.data);
                     }
                  }
               }
               gtk_tree_path_free(path);
            }
         }
         if(text)
            g_free(text);
      }
   }
   return retval;
}

static gint _dw_tree_expand_event(GtkTreeView *widget, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(!_dw_ignore_expand && work.window)
   {
      int (*treeexpandfunc)(HWND, HTREEITEM, void *) = work.func;
      retval = treeexpandfunc(work.window, (HTREEITEM)iter, work.data);
   }
   return retval;
}

static gint _dw_container_enter_event(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   GdkEventKey *keyevent = (GdkEventKey *)event;
   GdkEventButton *buttonevent = (GdkEventButton *)event;
   int retval = FALSE;

   if(work.window)
   {
      /* Handle both key and button events together */
      if((event->type == GDK_2BUTTON_PRESS && buttonevent->button == 1) ||
         (event->type == GDK_KEY_PRESS && keyevent->keyval == VK_RETURN))
      {
         int (*contextfunc)(HWND, char *, void *, void *) = work.func;
         char *text = NULL;
         void *data = NULL;

         /* Prevent some double events from happening */
         if(event->type == GDK_2BUTTON_PRESS)
         {
            g_object_set_data(G_OBJECT(widget), "_dw_double_click", GINT_TO_POINTER(1));
         }

         /* Sanity check */
         if(GTK_IS_TREE_VIEW(widget))
         {
            GtkTreePath *path;
            GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

            gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
            if(path)
            {
               GtkTreeIter iter;

               if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
               {
                  if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &data, -1);
                     retval = contextfunc(work.window, text, work.data, data);
                     if(text)
                        g_free(text);
                  }
               }
               gtk_tree_path_free(path);
            }
         }
      }
   }
   return retval;
}

/* Return the logical page id from the physical page id */
int _dw_get_logical_page(HWND handle, unsigned long pageid)
{
   int z;
   GtkWidget **pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray");
   GtkWidget *thispage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), pageid);

   if(pagearray && thispage)
   {
      for(z=0;z<256;z++)
      {
         if(thispage == pagearray[z])
            return z;
      }
   }
   return 256;
}


static gint _dw_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*switchpagefunc)(HWND, unsigned long, void *) = work.func;
      retval = switchpagefunc(work.window, _dw_get_logical_page(GTK_WIDGET(notebook), page_num), work.data);
   }
   return retval;
}

static gint _dw_column_click_event(GtkWidget *widget, gpointer data)
{
   void **params = data;
   int retval = FALSE;

   if(params && params[2])
   {
      GtkWidget *tree = (GtkWidget *)params[2];
      gint handlerdata = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tree), "_dw_column_click_id"));

      if(handlerdata)
      {
         DWSignalHandler work;

         params[0] = GINT_TO_POINTER(handlerdata-1);
         work = _dw_get_signal_handler(params);

         if(work.window)
         {
            int column_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_column"));
            int (*clickcolumnfunc)(HWND, int, void *) = work.func;
            retval = clickcolumnfunc(work.window, column_num, work.data);
         }
      }
   }
   return retval;
}

static int _dw_round_value(gfloat val)
{
   int newval = (int)val;

   if(val >= 0.5 + (gfloat)newval)
      newval++;

   return newval;
}

static gint _dw_value_changed_event(GtkWidget *widget, gpointer data)
{
   GtkWidget *slider, *spinbutton, *scrollbar;
   GtkAdjustment *adjustment = (GtkAdjustment *)widget;
   int max, val;

   if(!GTK_IS_ADJUSTMENT(adjustment))
      adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(widget), "_dw_adjustment");

   slider = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_slider");
   spinbutton = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_spinbutton");
   scrollbar = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_scrollbar");

   max = _dw_round_value(gtk_adjustment_get_upper(adjustment));
   val = _dw_round_value(gtk_adjustment_get_value(adjustment));

   if(g_object_get_data(G_OBJECT(adjustment), "_dw_suppress_dw_value_changed_event"))
      return FALSE;

   if (slider || spinbutton || scrollbar)
   {
      DWSignalHandler work = _dw_get_signal_handler(data);

      if (work.window)
      {
         int (*valuechangedfunc)(HWND, int, void *) = work.func;

         if(slider && gtk_orientable_get_orientation(GTK_ORIENTABLE(slider)) == GTK_ORIENTATION_VERTICAL)
            valuechangedfunc(work.window, (max - val) - 1,  work.data);
         else
            valuechangedfunc(work.window, val,  work.data);
      }
   }
   return FALSE;
}

static gint _dw_default_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
   GtkWidget *next = (GtkWidget *)data;

   if(next)
   {
      if(event->keyval == GDK_KEY_Return)
      {
         if(GTK_IS_BUTTON(next))
            g_signal_emit_by_name(G_OBJECT(next), "clicked");
         else
            gtk_widget_grab_focus(next);
      }
   }
   return FALSE;
}

#ifdef DW_INCLUDE_DEPRECATED_RESOURCES
static GdkPixbuf *_dw_pixbuf_from_data(char *data)
{
   if(data[0] == 'G' && data[1] == 'd' && data[2] == 'k' && data[3] == 'P')
      return gdk_pixbuf_new_from_inline(-1, (const guint8 *)data, FALSE, NULL);
   return gdk_pixbuf_new_from_xpm_data((const char **)data);
}
#endif

static GdkPixbuf *_dw_pixbuf_from_resource(unsigned int rid)
{
#if GLIB_CHECK_VERSION(2,32,0)
   char resource_path[201] = {0};
   snprintf(resource_path, 200, "/org/dbsoft/dwindows/resources/%u.png", rid);
   return gdk_pixbuf_new_from_resource(resource_path, NULL);
#else
   return NULL;
#endif   
}

static GdkPixbuf *_dw_find_pixbuf(HICN icon, unsigned long *userwidth, unsigned long *userheight)
{
   unsigned int id = GPOINTER_TO_INT(icon);
   GdkPixbuf *icon_pixbuf = NULL;

   /* Quick dropout for non-handle */
   if(!icon)
      return NULL;

   if(id > 65535)
      icon_pixbuf = icon;
   else
      icon_pixbuf = _dw_pixbuf_from_resource(id);
   
#ifdef DW_INCLUDE_DEPRECATED_RESOURCES
   if(!icon_pixbuf)
   {
      char *data = NULL;
      int z;

      for(z=0;z<_resources.resource_max;z++)
      {
         if(_resources.resource_id[z] == id)
         {
            data = _resources.resource_data[z];
            break;
         }
      }

      if(data)
         icon_pixbuf = _dw_pixbuf_from_data(data);
   }
#endif
   if(userwidth)
      *userwidth = icon_pixbuf ? gdk_pixbuf_get_width(icon_pixbuf) : 0;
   if(userheight)
      *userheight = icon_pixbuf ? gdk_pixbuf_get_height(icon_pixbuf) : 0;

   return icon_pixbuf;
}

#if GLIB_CHECK_VERSION(2,40,0)
/* Handle system notification click callbacks */
static void _dw_notification_handler(GSimpleAction *action, GVariant *param, gpointer user_data)
{
   char textbuf[101] = {0};
   int (*func)(HWND, void *);
   void *data;
   
   snprintf(textbuf, 100, "dw-notification-%llu-func", DW_POINTER_TO_ULONGLONG(g_variant_get_uint64(param)));
   func = g_object_get_data(G_OBJECT(_DWApp), textbuf);
   g_object_set_data(G_OBJECT(_DWApp), textbuf, NULL);
   snprintf(textbuf, 100, "dw-notification-%llu-data", DW_POINTER_TO_ULONGLONG(g_variant_get_uint64(param)));
   data = g_object_get_data(G_OBJECT(_DWApp), textbuf);
   g_object_set_data(G_OBJECT(_DWApp), textbuf, NULL);  
   
   if(func)
      func((HWND)DW_ULONGLONG_TO_POINTER(g_variant_get_uint64(param)), data);  
}
#endif

#if GLIB_CHECK_VERSION(2,28,0)
static void _dw_app_activate(GApplication *app, gpointer user_data)
{
   /* Not sure why this signal is required, but GLib gives warnings
    * when this signal is not connected, so putting this here to
    * quell the warning and can be used at a later point if needed.
    */
}
#endif

#ifdef DW_INCLUDE_DEPRECATED_RESOURCES
#undef dw_init
int dw_int_init(DWResources *res, int newthread, int *pargc, char **pargv[])
{
   int argc = pargc ? *pargc : 0;
   char **argv = pargv ? *pargv : NULL;
   
   if(res)
   {
      _resources.resource_max = res->resource_max;
      _resources.resource_id = res->resource_id;
      _resources.resource_data = res->resource_data;
   }
   return dw_init(newthread, argc, argv);
}
#endif

void _dw_init_path(char *arg)
{
   char path[PATH_MAX+1] = {0};

#ifdef __linux__
   if(readlink("/proc/self/exe", path, PATH_MAX) == -1)
#elif defined(__FreeBSD__)
   int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
   size_t length = PATH_MAX;

   if(sysctl(name, 4, path, &length, NULL, 0) == -1 || length <= 1)
#elif defined(__sun__)
   char procpath[101] = {0};

   snprintf(procpath, 100, "/proc/%ld/path/a.out", (long)getpid());

   if(readlink(procpath, path, PATH_MAX) == -1)
#endif
      strncpy(path, arg ? arg : "", PATH_MAX);

   if(path[0])
   {
      char *pos = strrchr(path, '/');
      char *binname = path;

      /* If we have a / then...
       * the binary name should be at the end.
       */
      if(pos)
      {
         binname = pos + 1;
         *pos = 0;
      }

      if(*binname)
      {
         char *binpos = strstr(path, "/bin");

         if(binpos)
            strncpy(_dw_share_path, path, (size_t)(binpos - path));
         else
            strcpy(_dw_share_path, "/usr/local");
         strcat(_dw_share_path, "/share/");
         strcat(_dw_share_path, binname);
#if GLIB_CHECK_VERSION(2,28,0)
         if(!_dw_app_id[0])
         {
            /* If we have a binary name, use that for the Application ID instead. */
            snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.%s", DW_APP_DOMAIN_DEFAULT, binname);
         }
#endif
      }
   }
   /* If that failed... just get the current directory */
   if(!_dw_share_path[0] && !getcwd(_dw_share_path, PATH_MAX))
      _dw_share_path[0] = '/';
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int dw_init(int newthread, int argc, char *argv[])
{
   /* Setup the private data directory */
   _dw_init_path(argc > 0 ? argv[0] : NULL);

#if !GLIB_CHECK_VERSION(2,31,0)
   g_thread_init(NULL);
#endif

   /* Load these functions via dlsym to avoid deprecation warnings */
   _dw_gdk_threads_init = dlsym(RTLD_DEFAULT, "gdk_threads_init");
   _dw_gdk_threads_enter = dlsym(RTLD_DEFAULT, "gdk_threads_enter");
   _dw_gdk_threads_leave = dlsym(RTLD_DEFAULT, "gdk_threads_leave");

   _dw_gdk_threads_init();

   gtk_init(&argc, &argv);

   pthread_key_create(&_dw_fg_color_key, NULL);
   pthread_key_create(&_dw_bg_color_key, NULL);
   pthread_key_create(&_dw_mutex_key, NULL);

   _dw_init_thread();

   /* Create a global object for glib activities */
   _DWObject = g_object_new(G_TYPE_OBJECT, NULL);

#if GLIB_CHECK_VERSION(2,28,0)
   if(!_dw_app_id[0])
   {
      /* Generate an Application ID based on the PID if all else fails. */
      snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, (int)getpid());
   }

   /* Initialize the application subsystem on supported versions...
    * we generate an application ID based on the binary name or PID
    * instead of passing NULL to enable full application support.
    */
#if GLIB_CHECK_VERSION(2,74,0)
   _DWApp = g_application_new(_dw_app_id, G_APPLICATION_DEFAULT_FLAGS);
#else
   _DWApp = g_application_new(_dw_app_id, G_APPLICATION_FLAGS_NONE);
#endif
   if(_DWApp && g_application_register(_DWApp, NULL, NULL))
   {
#if GLIB_CHECK_VERSION(2,40,0)
      /* Creat our notification handler for any notifications */
      GSimpleAction *action = g_simple_action_new("notification", G_VARIANT_TYPE_UINT64);
      
      g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(_dw_notification_handler), NULL);
      g_action_map_add_action(G_ACTION_MAP(_DWApp), G_ACTION(action));
#endif
      g_set_prgname(_dw_app_id);
      g_signal_connect(_DWApp, "activate", G_CALLBACK(_dw_app_activate), NULL);
      g_application_activate(_DWApp);
   }
#endif
   if(_dw_render_safe_mode == DW_FEATURE_UNSUPPORTED)
   {
#ifdef GDK_WINDOWING_X11
   	GdkDisplay *display = gdk_display_get_default();
   
	   /* If we are in X11 mode safe mode should be disabled by default */
	   if(display && GDK_IS_X11_DISPLAY(display))
	   	_dw_render_safe_mode = DW_FEATURE_DISABLED;
	   else
#endif
			_dw_render_safe_mode = DW_FEATURE_ENABLED;
	}
	return DW_ERROR_NONE;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
   _dw_gdk_threads_enter();
   _dw_thread = pthread_self();
   gtk_main();
   _dw_thread = (pthread_t)-1;
   _dw_gdk_threads_leave();
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
   gtk_main_quit();
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
   struct timeval tv, start;
   pthread_t curr = pthread_self();

   gettimeofday(&start, NULL);

   if(_dw_thread == (pthread_t)-1 || _dw_thread == curr)
   {
      pthread_t orig = _dw_thread;

      gettimeofday(&tv, NULL);

      while(((tv.tv_sec - start.tv_sec)*1000) + ((tv.tv_usec - start.tv_usec)/1000) <= milliseconds)
      {
         int _dw_locked_by_me = FALSE;

         if(orig == (pthread_t)-1)
         {
            if(!pthread_getspecific(_dw_mutex_key))
            {
               _dw_gdk_threads_enter();
               pthread_setspecific(_dw_mutex_key, (void *)&_dw_locked_by_me);
               _dw_locked_by_me = TRUE;
            }
            _dw_thread = curr;
         }
         if(curr == _dw_thread && gtk_events_pending())
            gtk_main_iteration();
         else
            _dw_msleep(1);
         if(orig == (pthread_t)-1)
         {
            _dw_thread = orig;
            if(_dw_locked_by_me)
            {
               pthread_setspecific(_dw_mutex_key, NULL);
               _dw_gdk_threads_leave();
            }
         }
         gettimeofday(&tv, NULL);
      }
   }
   else
      _dw_msleep(milliseconds);
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
   pthread_t orig = _dw_thread;
   pthread_t curr = pthread_self();
   int _dw_locked_by_me = FALSE;

   if(_dw_thread == (pthread_t)-1)
   {
      if(!pthread_getspecific(_dw_mutex_key))
      {
         _dw_gdk_threads_enter();
         pthread_setspecific(_dw_mutex_key, (void *)&_dw_locked_by_me);
         _dw_locked_by_me = TRUE;
      }
      _dw_thread = curr;
   }
   if(curr == _dw_thread && gtk_events_pending())
      gtk_main_iteration();
   else
      sched_yield();
   if(orig == (pthread_t)-1)
   {
      _dw_thread = orig;
      if(_dw_locked_by_me)
      {
         pthread_setspecific(_dw_mutex_key, NULL);
         _dw_gdk_threads_leave();
      }
   }
}

/*
 * Free's memory allocated by dynamic windows.
 * Parameters:
 *           ptr: Pointer to dynamic windows allocated
 *                memory to be free()'d.
 */
void dw_free(void *ptr)
{
   free(ptr);
}

/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog *dw_dialog_new(void *data)
{
   DWDialog *tmp = malloc(sizeof(DWDialog));

   if ( tmp )
   {
      tmp->eve = dw_event_new();
      dw_event_reset(tmp->eve);
      tmp->data = data;
      tmp->done = FALSE;
      tmp->method = FALSE;
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
int dw_dialog_dismiss(DWDialog *dialog, void *result)
{
   dialog->result = result;
   if(dialog->method)
      gtk_main_quit();
   else
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
void *dw_dialog_wait(DWDialog *dialog)
{
   void *tmp;
   int newprocess = 0;

   if(!dialog)
      return NULL;

   /* _dw_thread will be -1 if dw_main hasn't been run yet. */
   if(_dw_thread == (pthread_t)-1)
   {
      _dw_thread = pthread_self();
      newprocess = 1;
      _dw_gdk_threads_enter();
   }

   if(pthread_self() == _dw_thread)
   {
      dialog->method = TRUE;
      gtk_main();
   }
   else
   {
      dialog->method = FALSE;
      dw_event_wait(dialog->eve, -1);
   }

   if(newprocess)
   {
      _dw_thread = (pthread_t)-1;
      _dw_gdk_threads_leave();
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
   vfprintf(stderr, format, args);
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: Defines buttons and icons to display
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

int dw_vmessagebox(const char *title, int flags, const char *format, va_list args)
{
   GtkMessageType gtkicon = GTK_MESSAGE_OTHER;
   GtkButtonsType gtkbuttons = GTK_BUTTONS_OK;
   GtkWidget *dialog;
   int response, _dw_locked_by_me = FALSE;
   char outbuf[1025] = {0};

   vsnprintf(outbuf, 1024, format, args);

   if(flags & DW_MB_ERROR)
      gtkicon = GTK_MESSAGE_ERROR;
   else if(flags & DW_MB_WARNING)
      gtkicon = GTK_MESSAGE_WARNING;
   else if(flags & DW_MB_INFORMATION)
      gtkicon = GTK_MESSAGE_INFO;
   else if(flags & DW_MB_QUESTION)
      gtkicon = GTK_MESSAGE_QUESTION;

   if(flags & DW_MB_OKCANCEL)
      gtkbuttons = GTK_BUTTONS_OK_CANCEL;
   else if(flags & (DW_MB_YESNO | DW_MB_YESNOCANCEL))
      gtkbuttons = GTK_BUTTONS_YES_NO;

   DW_MUTEX_LOCK;
   dialog = gtk_message_dialog_new(NULL,
#if GTK_CHECK_VERSION(3,12,0)
                                   GTK_DIALOG_USE_HEADER_BAR |
#endif
                                   GTK_DIALOG_MODAL, gtkicon, gtkbuttons, "%s", title);
   gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", outbuf);
   if(flags & DW_MB_YESNOCANCEL)
      gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_CANCEL);
   response = gtk_dialog_run(GTK_DIALOG(dialog));
   gtk_widget_destroy(dialog);
   DW_MUTEX_UNLOCK;
   switch(response)
   {
      case GTK_RESPONSE_OK:
         return DW_MB_RETURN_OK;
      case GTK_RESPONSE_CANCEL:
         return DW_MB_RETURN_CANCEL;
      case GTK_RESPONSE_YES:
         return DW_MB_RETURN_YES;
      case GTK_RESPONSE_NO:
         return DW_MB_RETURN_NO;
      default:
      {
         /* Handle the destruction of the dialog result */
         if(flags & (DW_MB_OKCANCEL | DW_MB_YESNOCANCEL))
            return DW_MB_RETURN_CANCEL;
         else if(flags & DW_MB_YESNO)
            return DW_MB_RETURN_NO;
      }
   }
   return DW_MB_RETURN_OK;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int dw_window_minimize(HWND handle)
{
   int _dw_locked_by_me = FALSE;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi = NULL;
#endif

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_ICONIFIED);
   }
   else
#endif
   {
      gtk_window_iconify( GTK_WINDOW(handle) );
   }
   DW_MUTEX_UNLOCK;
   return 0;
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int dw_window_raise(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   gdk_window_raise(gtk_widget_get_window(GTK_WIDGET(handle)));
   DW_MUTEX_UNLOCK;
   return 0;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int dw_window_lower(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   gdk_window_lower(gtk_widget_get_window(GTK_WIDGET(handle)));
   DW_MUTEX_UNLOCK;
   return 0;
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int dw_window_show(HWND handle)
{
   int _dw_locked_by_me = FALSE;
   GtkWidget *defaultitem;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi;
#endif

   if (!handle)
      return 0;

   DW_MUTEX_LOCK;
   gtk_widget_show(handle);
#ifdef DW_INCLUDE_DEPRECATED
   if ((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_NORMAL);
   }
   else
#endif
   {
      GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(handle));
      if (window && gtk_widget_get_mapped(handle))
      {
         int width = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_width"));
         int height = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_height"));

         /* If we had a size request before shown */
         if (width || height)
         {
            /* Call the size function again now that we are realized */
            dw_window_set_size(handle, width, height);
            /* Clear out the data so we don't do it again */
            g_object_set_data(G_OBJECT(handle), "_dw_width", NULL);
            g_object_set_data(G_OBJECT(handle), "_dw_height", NULL);
         }

         /* If we had a position request before shown */
         if (g_object_get_data(G_OBJECT(handle), "_dw_pos"))
         {
            int x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_x"));
            int y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_y"));

            /* Call the position function again now that we are realized */
            dw_window_set_pos(handle, x, y);
            /* Clear out the data so we don't do it again */
            g_object_set_data(G_OBJECT(handle), "_dw_pos", NULL);
         }

         gdk_window_raise(window);
         gdk_display_flush(gdk_display_get_default());
         gdk_window_show(window);
         gdk_display_flush(gdk_display_get_default());
      }
      defaultitem = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_defaultitem");
      if (defaultitem)
         gtk_widget_grab_focus(defaultitem);
   }
   DW_MUTEX_UNLOCK;
   return 0;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int dw_window_hide(HWND handle)
{
   int _dw_locked_by_me = FALSE;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi = NULL;
#endif

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_ICONIFIED);
   }
   else
#endif
      gtk_widget_hide(handle);
   DW_MUTEX_UNLOCK;
   return 0;
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
int dw_window_destroy(HWND handle)
{
   int _dw_locked_by_me = FALSE;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi = NULL;
#endif

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_remove(GTK_MDI(mdi), handle);
   }
#endif
   if(GTK_IS_WIDGET(handle))
   {
      GtkWidget *box, *handle2 = handle;
      GtkWidget *eventbox = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_eventbox");

      /* Handle the invisible event box if it exists */
      if(eventbox && GTK_IS_WIDGET(eventbox))
         handle2 = eventbox;

      /* Check if we are removing a widget from a box */	
      if((box = gtk_widget_get_parent(handle2)) && GTK_IS_GRID(box))
      {
         /* Get the number of items in the box... */
         int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
#if GTK_CHECK_VERSION(3,10,0)
         int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
         int z;

         /* Figure out where in the grid this widget is and remove that row/column */
         if(boxtype == DW_VERT)
         {
            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), 0, z) == handle2)
               {
                  gtk_grid_remove_row(GTK_GRID(box), z);
                  handle2 = NULL;
                  break;
               }
            }
         }
         else
         {
            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), z, 0) == handle2)
               {
                  gtk_grid_remove_column(GTK_GRID(box), z);
                  handle2 = NULL;
                  break;
               }
            }
         }
#endif
			
         if(boxcount > 0)
         {
            /* Decrease the count by 1 */
            boxcount--;
            g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
         }
      }
      /* Finally destroy the widget, make sure it is still
       * a valid widget if it got removed from the grid.
       */
      if(handle2 && GTK_IS_WIDGET(handle2))
         gtk_widget_destroy(handle2);
   }
   DW_MUTEX_UNLOCK;
   return 0;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void dw_window_redraw(HWND handle)
{
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void dw_window_reparent(HWND handle, HWND newparent)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gdk_window_reparent(gtk_widget_get_window(GTK_WIDGET(handle)), newparent ? gtk_widget_get_window(GTK_WIDGET(newparent)) : gdk_get_default_root_window(), 0, 0);
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
   char *oldfont = _DWDefaultFont;

   _DWDefaultFont = strdup(fontname);

   if(oldfont)
      free(oldfont);
}

/* Convert DW style font to CSS syntax (or Pango for older versions):
 * font: font-style font-variant font-weight font-size/line-height font-family
 */
char *_dw_convert_font(const char *font)
{
   char *newfont = NULL;
   
   if(font)
   {
      char *name = strchr(font, '.');
#if GTK_CHECK_VERSION(3,20,0)
      char *Italic = strstr(font, " Italic");
      char *Bold = strstr(font, " Bold");
#endif
      
      /* Detect Dynamic Windows style font name...
       * Format: ##.Fontname
       * and convert to CSS or Pango syntax
       */
      if(name && (name++) && isdigit(*font))
      {
          int size = atoi(font);
#if GTK_CHECK_VERSION(3,20,0)
          int len = (Italic ? (Bold ? (Italic > Bold ? (Bold - name) : (Italic - name)) : (Italic - name)) : (Bold ? (Bold - name) : strlen(name)));
          char *newname = alloca(len+1);
          
          memset(newname, 0, len+1);
          strncpy(newname, name, len);
          
          newfont = g_strdup_printf("%s normal %s %dpx \"%s\"", Italic ? "italic" : "normal",
                                    Bold ? "bold" : "normal", size, newname);
#else
          newfont = g_strdup_printf("%s %d", name, size);
#endif
      }
   }
   return newfont;
}

/* Internal functions to convert to GTK3 style CSS */
static void _dw_override_color(GtkWidget *widget, const char *element, GdkRGBA *color)
{
   gchar *dataname = g_strdup_printf ("_dw_color_%s", element);
   GtkCssProvider *provider = g_object_get_data(G_OBJECT(widget), dataname);
   GtkStyleContext *scontext = gtk_widget_get_style_context(widget);
   
   /* If we have an old context from a previous override remove it */
   if(provider)
   {
      gtk_style_context_remove_provider(scontext, GTK_STYLE_PROVIDER(provider));
      g_object_unref(provider);
      provider = NULL;
   }
   
   /* If we have a new color, create a new provider and add it */
   if(color)
   {
      gchar *scolor = gdk_rgba_to_string(color);
      gchar *css = g_strdup_printf ("* { %s: %s; }", element, scolor);
      
      provider = gtk_css_provider_new();
      g_free(scolor);
      gtk_css_provider_load_from_data(provider, css, -1, NULL);
      g_free(css);
      gtk_style_context_add_provider(scontext, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   g_object_set_data(G_OBJECT(widget), dataname, (gpointer)provider);
   g_free(dataname);
}

static void _dw_override_font(GtkWidget *widget, const char *font)
{
   GtkCssProvider *provider = g_object_get_data(G_OBJECT(widget), "_dw_font");
   GtkStyleContext *scontext = gtk_widget_get_style_context(widget);
   
   /* If we have an old context from a previous override remove it */
   if(provider)
   {
      gtk_style_context_remove_provider(scontext, GTK_STYLE_PROVIDER(provider));
      g_object_unref(provider);
      provider = NULL;
   }
   
   /* If we have a new font, create a new provider and add it */
   if(font)
   {
      gchar *css = g_strdup_printf ("* { font: %s; }", font);
      
      provider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(provider, css, -1, NULL);
      g_free(css);
      gtk_style_context_add_provider(scontext, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   g_object_set_data(G_OBJECT(widget), "_dw_font", (gpointer)provider);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int dw_window_set_font(HWND handle, const char *fontname)
{
   GtkWidget *handle2 = handle;
   char *font = _dw_convert_font(fontname);
   int _dw_locked_by_me = FALSE;
   gpointer data;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* If it is a groupox we want to operate on the frame label */
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = gtk_frame_get_label_widget(GTK_FRAME(handle));
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_COMBO_BOX(handle) || GTK_IS_BUTTON(handle))
   {
      GtkWidget *tmp = gtk_bin_get_child(GTK_BIN(handle));
      if(tmp)
         handle2 = tmp;
   }

   /* Free old font name if one is allocated */
   data = g_object_get_data(G_OBJECT(handle2), "_dw_fontname");
   g_object_set_data(G_OBJECT(handle2), "_dw_fontname", (gpointer)font);
   if(data)
      free(data);

   _dw_override_font(handle2, font);

   DW_MUTEX_UNLOCK;
   return TRUE;
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
   GtkFontChooser *fd;
   char *font = currfont ? strdup(currfont) : NULL;
   char *name = font ? strchr(font, '.') : NULL;
   int _dw_locked_by_me = FALSE;
   char *retfont = NULL;

   /* Detect Dynamic Windows style font name...
    * Format: ##.Fontname
    * and convert to a Pango name
    */
   if(name && isdigit(*font))
   {
       int size = atoi(font);
       *name = 0;
       name++;
       sprintf(font, "%s %d", name, size);
   }

   DW_MUTEX_LOCK;
   fd = (GtkFontChooser *)gtk_font_chooser_dialog_new("Choose font", NULL);
   if(font)
   {
      gtk_font_chooser_set_font(fd, font);
      free(font);
   }

   gtk_widget_show(GTK_WIDGET(fd));

   if(gtk_dialog_run(GTK_DIALOG(fd)) == GTK_RESPONSE_OK)
   {
      char *fontname = gtk_font_chooser_get_font(fd);
      if(fontname && (retfont = strdup(fontname)))
      {
         int len = strlen(fontname);
         /* Convert to Dynamic Windows format if we can... */
         if(len > 0 && isdigit(fontname[len-1]))
         {
            int size, x=len-1;

            while(x > 0 && fontname[x] != ' ')
            {
               x--;
            }
            size = atoi(&fontname[x]);
            /* If we were able to find a valid size... */
            if(size > 0)
            {
               /* Null terminate after the name...
                * and create the Dynamic Windows style font.
                */
               fontname[x] = 0;
               snprintf(retfont, len+1, "%d.%s", size, fontname);
            }
         }
         g_free(fontname);
      }
   }
   gtk_widget_destroy(GTK_WIDGET(fd));
   DW_MUTEX_UNLOCK;
   return retfont;
}

/*
 * Gets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 */
char *dw_window_get_font(HWND handle)
{
   PangoFontDescription *pfont;
   PangoContext *pcontext;
   GtkWidget *handle2 = handle;
   char *font;
   char *retfont=NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* If it is a groupox we want to operate on the frame label */
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = gtk_frame_get_label_widget(GTK_FRAME(handle));
      if(tmp)
         handle2 = tmp;
   }

   pcontext = gtk_widget_get_pango_context( handle2 );
   if ( pcontext )
   {
      pfont = pango_context_get_font_description( pcontext );
      if ( pfont )
      {
         int len, x;

         font = pango_font_description_to_string( pfont );
         retfont = strdup(font);
         len = strlen(font);
         /* Convert to Dynamic Windows format if we can... */
         if(len > 0 && isdigit(font[len-1]))
         {
            int size;

            x=len-1;
            while(x > 0 && font[x] != ' ')
            {
               x--;
            }
            size = atoi(&font[x]);
            /* If we were able to find a valid size... */
            if(size > 0)
            {
               /* Null terminate after the name...
                * and create the Dynamic Windows style font.
                */
               font[x] = 0;
               snprintf(retfont, len+1, "%d.%s", size, font);
            }
         }
         g_free( font );
      }
   }
   DW_MUTEX_UNLOCK;
   return retfont;
}

void _dw_free_gdk_colors(HWND handle)
{
   GdkRGBA *old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_foregdk");

   if(old)
      free(old);

   old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_backgdk");

   if(old)
      free(old);
}

/* Free old color pointers and allocate new ones */
static void _dw_save_gdk_colors(HWND handle, GdkRGBA fore, GdkRGBA back)
{
   GdkRGBA *foregdk = malloc(sizeof(GdkRGBA));
   GdkRGBA *backgdk = malloc(sizeof(GdkRGBA));

   _dw_free_gdk_colors(handle);

   *foregdk = fore;
   *backgdk = back;

   g_object_set_data(G_OBJECT(handle), "_dw_foregdk", (gpointer)foregdk);
   g_object_set_data(G_OBJECT(handle), "_dw_backgdk", (gpointer)backgdk);
}

static int _dw_set_color(HWND handle, unsigned long fore, unsigned long back)
{
   /* Remember that each color component in X11 use 16 bit no matter
    * what the destination display supports. (and thus GDK)
    */
   GdkRGBA forecolor = {0}, backcolor = {0};

   if(fore & DW_RGB_COLOR)
   {
      forecolor.alpha = 1.0;
      forecolor.red = (gdouble)DW_RED_VALUE(fore) / 255.0;
      forecolor.green = (gdouble)DW_GREEN_VALUE(fore) / 255.0;
      forecolor.blue = (gdouble)DW_BLUE_VALUE(fore) / 255.0;
   }
   else if(fore != DW_CLR_DEFAULT)
      forecolor = _dw_colors[fore];

   _dw_override_color(handle, "color", fore != DW_CLR_DEFAULT ? &forecolor : NULL);

   if(back & DW_RGB_COLOR)
   {
      backcolor.alpha = 1.0;
      backcolor.red = (gdouble)DW_RED_VALUE(back) / 255.0;
      backcolor.green = (gdouble)DW_GREEN_VALUE(back) / 255.0;
      backcolor.blue = (gdouble)DW_BLUE_VALUE(back) / 255.0;
   }
   else if(back != DW_CLR_DEFAULT)
      backcolor = _dw_colors[back];

   _dw_override_color(handle, "background-color", back != DW_CLR_DEFAULT ? &backcolor : NULL);

   _dw_save_gdk_colors(handle, forecolor, backcolor);

   return TRUE;
}
/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in RGB format.
 *          back: Background color in RGB format.
 */
int dw_window_set_color(HWND handle, unsigned long fore, unsigned long back)
{
   GtkWidget *handle2 = handle;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;

   if(GTK_IS_SCROLLED_WINDOW(handle) || GTK_IS_BOX(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_GRID(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_eventbox");
      if(tmp)
      {
         handle2 = tmp;
         fore = DW_CLR_DEFAULT;
      }
   }

   _dw_set_color(handle2, fore, back);

   DW_MUTEX_UNLOCK;
   return TRUE;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int dw_window_set_border(HWND handle, int border)
{
   /* TODO */
   return 0;
}

#if GTK_CHECK_VERSION(3,20,0)
static GdkSeat *_dw_grab_seat = NULL;
#else
static GdkDeviceManager *_dw_grab_manager = NULL;
#endif

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void dw_window_capture(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
#if GTK_CHECK_VERSION(3,20,0)
   _dw_grab_seat = gdk_display_get_default_seat(gtk_widget_get_display(handle));
   gdk_seat_grab (_dw_grab_seat,
                  gtk_widget_get_window(handle),
                  GDK_SEAT_CAPABILITY_ALL_POINTING,
                  FALSE, NULL, NULL, NULL, NULL);
#else
   _dw_grab_manager = gdk_display_get_device_manager(gtk_widget_get_display(handle));
   gdk_device_grab(gdk_device_manager_get_client_pointer(_dw_grab_manager),
                   gtk_widget_get_window(handle),
                   GDK_OWNERSHIP_WINDOW,
                   FALSE,
                   GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
                   NULL,
                   GDK_CURRENT_TIME);
#endif
   DW_MUTEX_UNLOCK;
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void dw_window_set_pointer(HWND handle, int pointertype)
{
   int _dw_locked_by_me = FALSE;
   GdkCursor *cursor;

   DW_MUTEX_LOCK;
   if(pointertype > 65535)
   {
      GdkPixbuf  *pixbuf = _dw_find_pixbuf(GINT_TO_POINTER(pointertype), NULL, NULL);
      cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), pixbuf, 8, 8);
   }
   else if(!pointertype)
      cursor = NULL;
   else
      cursor = gdk_cursor_new_for_display(gdk_display_get_default(), pointertype);
   if(handle && gtk_widget_get_window(handle))
      gdk_window_set_cursor(gtk_widget_get_window(handle), cursor);
   if(cursor)
      g_object_unref(cursor);
   DW_MUTEX_UNLOCK;
}

/*
 * Releases previous mouse capture.
 */
void dw_window_release(void)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
#if GTK_CHECK_VERSION(3,20,0)
   if(_dw_grab_seat)
      gdk_seat_ungrab(_dw_grab_seat);
   _dw_grab_seat = NULL;
#else
   gdk_device_ungrab(gdk_device_manager_get_client_pointer(_dw_grab_manager), GDK_CURRENT_TIME);
   _dw_grab_manager = NULL;
#endif
   DW_MUTEX_UNLOCK;
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
HWND dw_window_new(HWND hwndOwner, const char *title, unsigned long flStyle)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;
   int flags = 0;

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   if(hwndOwner && GTK_IS_MDI(hwndOwner))
   {
      GtkWidget *label;

      tmp = dw_box_new(DW_VERT, 0);

      label = gtk_label_new(title);
      gtk_widget_show(label);
      g_object_set_data(G_OBJECT(tmp), "_dw_mdi_child", GINT_TO_POINTER(1));
      g_object_set_data(G_OBJECT(tmp), "_dw_mdi_title", (gpointer)label);
      g_object_set_data(G_OBJECT(tmp), "_dw_mdi", (gpointer)hwndOwner);

      gtk_mdi_put(GTK_MDI(hwndOwner), tmp, 100, 75, label);
   }
   else
#endif
   {
      GtkWidget *box = dw_box_new(DW_VERT, 0);
      GtkWidget *grid = gtk_grid_new();

      gtk_widget_show_all(grid);

      tmp = gtk_window_new(GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title(GTK_WINDOW(tmp), title);
      gtk_window_set_resizable(GTK_WINDOW(tmp), (flStyle & DW_FCF_SIZEBORDER) ? TRUE : FALSE);

      gtk_widget_realize(tmp);

      if(flStyle & DW_FCF_TITLEBAR)
         flags |= GDK_DECOR_TITLE;

      if(flStyle & DW_FCF_MINMAX)
         flags |= GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE;

      if(flStyle & DW_FCF_SIZEBORDER)
         flags |= GDK_DECOR_RESIZEH | GDK_DECOR_BORDER;

      if(flStyle & (DW_FCF_BORDER | DW_FCF_DLGBORDER))
         flags |= GDK_DECOR_BORDER;

      if(flStyle & DW_FCF_FULLSCREEN)
         gtk_window_fullscreen(GTK_WINDOW(tmp));
      else
      {
         if(flStyle & DW_FCF_MAXIMIZE)
            gtk_window_maximize(GTK_WINDOW(tmp));

         if(flStyle & DW_FCF_MINIMIZE)
            gtk_window_iconify(GTK_WINDOW(tmp));
      }

      /* Either the CLOSEBUTTON or SYSMENU flags should make it deletable */
      gtk_window_set_deletable(GTK_WINDOW(tmp), (flStyle & (DW_FCF_CLOSEBUTTON | DW_FCF_SYSMENU)) ? TRUE : FALSE);

      gdk_window_set_decorations(gtk_widget_get_window(tmp), flags);
      if(!flags)
         gtk_window_set_decorated(GTK_WINDOW(tmp), FALSE);

      if(hwndOwner)
         gdk_window_reparent(gtk_widget_get_window(GTK_WIDGET(tmp)), gtk_widget_get_window(GTK_WIDGET(hwndOwner)), 0, 0);

      if(flStyle & DW_FCF_SIZEBORDER)
         g_object_set_data(G_OBJECT(tmp), "_dw_size", GINT_TO_POINTER(1));

      gtk_grid_attach(GTK_GRID(grid), box, 0, 1, 1, 1);
      gtk_container_add(GTK_CONTAINER(tmp), grid);
      g_object_set_data(G_OBJECT(tmp), "_dw_boxhandle", (gpointer)box);
      g_object_set_data(G_OBJECT(tmp), "_dw_grid", (gpointer)grid);
      gtk_widget_add_events(GTK_WIDGET(tmp), GDK_PROPERTY_CHANGE_MASK);
   }
   g_object_set_data(G_OBJECT(tmp), "_dw_style", GINT_TO_POINTER(flStyle));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND dw_box_new(int type, int pad)
{
   GtkWidget *tmp, *eventbox;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_grid_new();
   eventbox = gtk_event_box_new();

   gtk_widget_show(eventbox);
   g_object_set_data(G_OBJECT(tmp), "_dw_eventbox", (gpointer)eventbox);
   g_object_set_data(G_OBJECT(tmp), "_dw_boxtype", GINT_TO_POINTER(type));
   g_object_set_data(G_OBJECT(tmp), "_dw_boxpad", GINT_TO_POINTER(pad));
   gtk_widget_show(tmp);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND dw_scrollbox_new( int type, int pad )
{
   GtkWidget *tmp, *box, *eventbox;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

   box = gtk_grid_new();
   eventbox = gtk_event_box_new();

   gtk_widget_show(eventbox);
   g_object_set_data(G_OBJECT(box), "_dw_eventbox", (gpointer)eventbox);
   g_object_set_data(G_OBJECT(box), "_dw_boxtype", GINT_TO_POINTER(type));
   g_object_set_data(G_OBJECT(box), "_dw_boxpad", GINT_TO_POINTER(pad));
   g_object_set_data(G_OBJECT(tmp), "_dw_boxhandle", (gpointer)box);

#if GTK_CHECK_VERSION(3,8,0)
   gtk_container_add(GTK_CONTAINER(tmp), box);
#else
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp),box);
#endif
   g_object_set_data(G_OBJECT(tmp), "_dw_user", box);
   gtk_widget_show(box);
   gtk_widget_show(tmp);

   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int dw_scrollbox_get_pos(HWND handle, int orient)
{
   int val = -1, _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if (!handle)
      return -1;

   DW_MUTEX_LOCK;
   if ( orient == DW_HORZ )
      adjustment = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW(handle) );
   else
      adjustment = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(handle) );
   if (adjustment)
      val = _dw_round_value(gtk_adjustment_get_value(adjustment));
   DW_MUTEX_UNLOCK;
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
   int range = -1, _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if (!handle)
      return -1;

   DW_MUTEX_LOCK;
   if ( orient == DW_HORZ )
      adjustment = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW(handle) );
   else
      adjustment = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(handle) );
   if (adjustment)
   {
      range = _dw_round_value(gtk_adjustment_get_upper(adjustment));
   }
   DW_MUTEX_UNLOCK;
   return range;
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 */
HWND dw_groupbox_new(int type, int pad, const char *title)
{
   GtkWidget *tmp, *frame;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_frame_set_label(GTK_FRAME(frame), title && *title ? title : NULL);

   tmp = gtk_grid_new();
   gtk_container_set_border_width(GTK_CONTAINER(tmp), pad);
   g_object_set_data(G_OBJECT(tmp), "_dw_boxtype", GINT_TO_POINTER(type));
   g_object_set_data(G_OBJECT(tmp), "_dw_boxpad", GINT_TO_POINTER(pad));
   g_object_set_data(G_OBJECT(frame), "_dw_boxhandle", (gpointer)tmp);
   gtk_container_add(GTK_CONTAINER(frame), tmp);
   gtk_widget_show(tmp);
   gtk_widget_show(frame);
   if(_DWDefaultFont)
      dw_window_set_font(frame, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return frame;
}

#ifdef DW_INCLUDE_DEPRECATED
/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND dw_mdi_new(unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_mdi_new();
   gtk_widget_show(tmp);
   DW_MUTEX_UNLOCK;
   return tmp;
}
#endif

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_bitmap_new(unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_image_new();
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND dw_notebook_new(unsigned long id, int top)
{
   GtkWidget *tmp, **pagearray = calloc(sizeof(GtkWidget *), 256);
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_notebook_new();
   if(top)
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tmp), GTK_POS_TOP);
   else
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tmp), GTK_POS_BOTTOM);
   gtk_notebook_set_scrollable(GTK_NOTEBOOK(tmp), TRUE);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   g_object_set_data(G_OBJECT(tmp), "_dw_pagearray", (gpointer)pagearray);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI dw_menu_new(unsigned long id)
{
   int _dw_locked_by_me = FALSE;
   GtkAccelGroup *accel_group;
   HMENUI tmp;

   DW_MUTEX_LOCK;
   tmp = gtk_menu_new();
   gtk_widget_show(tmp);
   accel_group = gtk_accel_group_new();
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   g_object_set_data(G_OBJECT(tmp), "_dw_accel", (gpointer)accel_group);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 * If there is no box already packed into the "location", the menu will not appear
 * so tell the user.
 */
HMENUI dw_menubar_new(HWND location)
{
   int _dw_locked_by_me = FALSE;
   GtkAccelGroup *accel_group;
   GtkWidget *box;
   HMENUI tmp = 0;

   DW_MUTEX_LOCK;
   if(GTK_IS_WINDOW(location) &&
      (box = (GtkWidget *)g_object_get_data(G_OBJECT(location), "_dw_grid")))
   {
      /* If there is an existing menu bar, remove it */
      GtkWidget *oldmenu = (GtkWidget *)g_object_get_data(G_OBJECT(location), "_dw_menubar");
      if(oldmenu)
         gtk_widget_destroy(oldmenu);
      /* Create a new menu bar */
      tmp = gtk_menu_bar_new();
      gtk_widget_show(tmp);
      accel_group = gtk_accel_group_new();
      g_object_set_data(G_OBJECT(tmp), "_dw_accel", (gpointer)accel_group);
      /* Save pointers to each other */
      g_object_set_data(G_OBJECT(location), "_dw_menubar", (gpointer)tmp);
      g_object_set_data(G_OBJECT(tmp), "_dw_window", (gpointer)location);
      gtk_grid_attach(GTK_GRID(box), tmp, 0, 0, 1, 1);
   }
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void dw_menu_destroy(HMENUI *menu)
{
   if(menu && *menu)
   {
      int _dw_locked_by_me = FALSE;
      GtkWidget *window;

      DW_MUTEX_LOCK;
      /* If it is a menu bar, try to delete the reference to it */
      if(GTK_IS_MENU_BAR(*menu) &&
         (window = (GtkWidget *)g_object_get_data(G_OBJECT(*menu), "_dw_window")))
            g_object_set_data(G_OBJECT(window), "_dw_menubar", NULL);
      /* Actually destroy the menu */
      gtk_widget_destroy(*menu);
      *menu = NULL;
      DW_MUTEX_UNLOCK;
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
 *       menu: The handle to the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND dw_menu_append_item(HMENUI menu, const char *title, unsigned long id, unsigned long flags, int end, int check, HMENUI submenu)
{
   GtkWidget *tmphandle;
   char accel, *tempbuf = malloc(strlen(title)+1);
   int _dw_locked_by_me = FALSE, submenucount;
   GtkAccelGroup *accel_group;

   if (!menu)
   {
      free(tempbuf);
      return NULL;
   }

   DW_MUTEX_LOCK;
   accel = _dw_removetilde(tempbuf, title);

   accel_group = (GtkAccelGroup *)g_object_get_data(G_OBJECT(menu), "_dw_accel");
   submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "_dw_submenucount"));

   if (strlen(tempbuf) == 0)
      tmphandle=gtk_menu_item_new();
   else
   {
      char numbuf[25] = {0};

      if (check)
      {
         tmphandle = gtk_check_menu_item_new_with_label(tempbuf);
         if (accel && accel_group)
         {
            gtk_label_set_use_underline(GTK_LABEL(gtk_bin_get_child(GTK_BIN(tmphandle))), TRUE);
#if 0 /* TODO: This isn't working right */
            gtk_widget_add_accelerator(tmphandle, "activate", accel_group, tmp_key, GDK_MOD1_MASK, 0);
#endif
         }
         snprintf(numbuf, 24, "%lu", id);
         g_object_set_data(G_OBJECT(menu), numbuf, (gpointer)tmphandle);
      }
      else
      {
         tmphandle=gtk_menu_item_new_with_label(tempbuf);
         if (accel && accel_group)
         {
            gtk_label_set_use_underline(GTK_LABEL(gtk_bin_get_child(GTK_BIN(tmphandle))), TRUE);
#if 0 /* TODO: This isn't working right */
            gtk_widget_add_accelerator(tmphandle, "activate", accel_group, tmp_key, GDK_MOD1_MASK, 0);
#endif
         }
         snprintf(numbuf, 24, "%lu", id);
         g_object_set_data(G_OBJECT(menu), numbuf, (gpointer)tmphandle);
      }
   }

   gtk_widget_show(tmphandle);

   if (submenu)
   {
      char tempbuf[101] = {0};

      snprintf(tempbuf, 100, "_dw_submenu%d", submenucount);
      submenucount++;
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(tmphandle), submenu);
      g_object_set_data(G_OBJECT(menu), tempbuf, (gpointer)submenu);
      g_object_set_data(G_OBJECT(menu), "_dw_submenucount", GINT_TO_POINTER(submenucount));
   }

   if (GTK_IS_MENU_BAR(menu))
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmphandle);
   else
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmphandle);

   g_object_set_data(G_OBJECT(tmphandle), "_dw_id", GINT_TO_POINTER(id));
   free(tempbuf);
   /*
    * Set flags
    */
   if ( check && (flags & DW_MIS_CHECKED) )
   {
      _dw_ignore_click = 1;
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmphandle), 1);
      _dw_ignore_click = 0;
   }

   if ( flags & DW_MIS_DISABLED )
      gtk_widget_set_sensitive( tmphandle, FALSE );

   DW_MUTEX_UNLOCK;
   return tmphandle;
}

GtkWidget *_dw_find_submenu_id(GtkWidget *start, const char *name)
{
   GtkWidget *tmp;
   int z, submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(start), "_dw_submenucount"));

   if((tmp = g_object_get_data(G_OBJECT(start), name)))
      return tmp;

   for(z=0;z<submenucount;z++)
   {
      char tempbuf[101] = {0};
      GtkWidget *submenu, *menuitem;

      snprintf(tempbuf, 100, "_dw_submenu%d", z);

      if((submenu = g_object_get_data(G_OBJECT(start), tempbuf)))
      {
         if((menuitem = _dw_find_submenu_id(submenu, name)))
            return menuitem;
      }
   }
   return NULL;
}

/*
 * Sets the state of a menu item check.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 * deprecated: use dw_menu_item_set_state()
 */
void dw_menu_item_set_check(HMENUI menu, unsigned long id, int check)
{
   char numbuf[25] = {0};
   GtkWidget *tmphandle;
   int _dw_locked_by_me = FALSE;

   if(!menu)
      return;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 24, "%lu", id);
   tmphandle = _dw_find_submenu_id(menu, numbuf);

   if(tmphandle)
   {
      _dw_ignore_click = 1;
      if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(tmphandle)) != check)
         gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmphandle), check);
      _dw_ignore_click = 0;
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       state: TRUE for checked FALSE for not checked.
 */
void dw_menu_item_set_state(HMENUI menu, unsigned long id, unsigned long state)
{
   char numbuf[25] = {0};
   GtkWidget *tmphandle;
   int check;
   int _dw_locked_by_me = FALSE;

   if(!menu)
      return;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 24, "%lu", id);
   tmphandle = _dw_find_submenu_id(menu, numbuf);

   if ( (state & DW_MIS_CHECKED) || (state & DW_MIS_UNCHECKED) )
   {
      if ( state & DW_MIS_CHECKED )
         check = 1;
      else
         check = 0;

      if (tmphandle)
      {
         _dw_ignore_click = 1;
         if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(tmphandle)) != check)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmphandle), check);
         _dw_ignore_click = 0;
      }
   }
   if ( (state & DW_MIS_ENABLED) || (state & DW_MIS_DISABLED) )
   {
      if (tmphandle )
      {
         _dw_ignore_click = 1;
         if ( state & DW_MIS_ENABLED )
            gtk_widget_set_sensitive( tmphandle, TRUE );
         else
            gtk_widget_set_sensitive( tmphandle, FALSE );
         _dw_ignore_click = 0;
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menu, unsigned long id)
{
   char numbuf[25] = {0};
   GtkWidget *tmphandle;
   int _dw_locked_by_me = FALSE;
   int ret = DW_ERROR_UNKNOWN;

   if(!menu)
      return ret;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 24, "%lu", id);
   tmphandle = _dw_find_submenu_id(menu, numbuf);

   if(tmphandle)
   {
      gtk_widget_destroy(tmphandle);
      g_object_set_data(G_OBJECT(menu), numbuf, NULL);
      ret = DW_ERROR_NONE;
   }
   DW_MUTEX_UNLOCK;
   return ret;
}

/*
 * Pops up a context menu at given x and y coordinates.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       parent: Handle to the window initiating the popup.
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void dw_menu_popup(HMENUI *menu, HWND parent, int x, int y)
{
   int _dw_locked_by_me = FALSE;

   if(!menu || !*menu)
      return;

   _dw_popup = parent;

   DW_MUTEX_LOCK;
#if GTK_CHECK_VERSION(3,22,0)
   gtk_menu_popup_at_pointer(GTK_MENU(*menu), NULL);
#else
   gtk_menu_popup(GTK_MENU(*menu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
#endif
   *menu = NULL;
   DW_MUTEX_UNLOCK;
}


/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void dw_pointer_query_pos(long *x, long *y)
{
   int gx = 0, gy = 0;
#ifdef GDK_WINDOWING_X11
   GdkModifierType state = 0;
   int _dw_locked_by_me = FALSE;
   GdkDisplay *display;

   DW_MUTEX_LOCK;
   display = gdk_display_get_default();
   
   if(display && GDK_IS_X11_DISPLAY(display))
   {
#if GTK_CHECK_VERSION(3,20,0)
      GdkSeat *seat = gdk_display_get_default_seat(display);
      GdkDevice *device = gdk_seat_get_pointer(seat);
#else
      GdkDeviceManager *manager = gdk_display_get_device_manager(display);
      GdkDevice *device = gdk_device_manager_get_client_pointer(manager);
#endif
      gdk_window_get_device_position(gdk_x11_window_lookup_for_display(display, GDK_ROOT_WINDOW()),
                                     device, &gx, &gy, &state);
   }
   DW_MUTEX_UNLOCK;
#endif
   if(x)
      *x = gx;
   if(y)
      *y = gy;
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void dw_pointer_set_pos(long x, long y)
{
#ifdef GDK_WINDOWING_X11
   int _dw_locked_by_me = FALSE;
   GdkDisplay *display;

   DW_MUTEX_LOCK;
   display = gdk_display_get_default();
   
   if(display && GDK_IS_X11_DISPLAY(display))
   {
#if GTK_CHECK_VERSION(3,20,0)
      GdkSeat *seat = gdk_display_get_default_seat(display);
      GdkDevice *device = gdk_seat_get_pointer(seat);
#else
      GdkDeviceManager *manager = gdk_display_get_device_manager(display);
      GdkDevice *device = gdk_device_manager_get_client_pointer(manager);
#endif
      gdk_device_warp(device, gdk_screen_get_default(), x, y);
   }
   DW_MUTEX_UNLOCK;
#endif
}

#define _DW_TREE_CONTAINER 1
#define _DW_TREE_TREE      2
#define _DW_TREE_LISTBOX   3

GtkWidget *_dw_tree_create(unsigned long id)
{
   GtkWidget *tmp;

   tmp = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp),
               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   gtk_widget_show(tmp);
   return tmp;
}

GtkWidget *_dw_tree_setup(GtkWidget *tmp, GtkTreeModel *store)
{
   GtkWidget *tree = gtk_tree_view_new_with_model(store);
   gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
#if GTK_CHECK_VERSION(3,8,0)
   gtk_container_add(GTK_CONTAINER(tmp), tree);
#else
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp), tree);
#endif
   g_object_set_data(G_OBJECT(tmp), "_dw_user", (gpointer)tree);
   return tree;
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND dw_container_new(unsigned long id, int multi)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _dw_tree_create(id)))
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER));
   g_object_set_data(G_OBJECT(tmp), "_dw_multi_sel", GINT_TO_POINTER(multi));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND dw_tree_new(ULONG id)
{
   GtkWidget *tmp, *tree;
   GtkTreeStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _dw_tree_create(id)))
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   store = gtk_tree_store_new(4, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER, G_TYPE_POINTER);
   tree = _dw_tree_setup(tmp, GTK_TREE_MODEL(store));
   g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_TREE));
   g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_TREE));
   col = gtk_tree_view_column_new();

   rend = gtk_cell_renderer_pixbuf_new();
   gtk_tree_view_column_pack_start(col, rend, FALSE);
   gtk_tree_view_column_add_attribute(col, rend, "pixbuf", 1);
   rend = gtk_cell_renderer_text_new();
   gtk_tree_view_column_pack_start(col, rend, TRUE);
   gtk_tree_view_column_add_attribute(col, rend, "text", 0);

   gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
   gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree), col);
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

   sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
   gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
   gtk_widget_show(tree);

   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}


/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_text_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_label_new(text);

   /* Left and centered */
#if GTK_CHECK_VERSION(3,16,0)
   gtk_label_set_xalign(GTK_LABEL(tmp), 0.0f);
   gtk_label_set_yalign(GTK_LABEL(tmp), 0.5f);
#else
   gtk_misc_set_alignment(GTK_MISC(tmp), 0.0f, 0.5f);	
#endif	
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_status_text_new(const char *text, ULONG id)
{
   GtkWidget *tmp, *frame;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   tmp = gtk_label_new(text);
   gtk_container_add(GTK_CONTAINER(frame), tmp);
   gtk_widget_show(tmp);
   gtk_widget_show(frame);

   /* Left and centered */
#if GTK_CHECK_VERSION(3,16,0)
   gtk_label_set_xalign(GTK_LABEL(tmp), 0.0f);
   gtk_label_set_yalign(GTK_LABEL(tmp), 0.5f);
#else
   gtk_misc_set_alignment(GTK_MISC(tmp), 0.0f, 0.5f);
#endif
   g_object_set_data(G_OBJECT(frame), "_dw_id", GINT_TO_POINTER(id));
   g_object_set_data(G_OBJECT(frame), "_dw_label", (gpointer)tmp);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return frame;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_mle_new(unsigned long id)
{
   GtkWidget *tmp, *tmpbox;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmpbox = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(tmpbox),
               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tmpbox), GTK_SHADOW_ETCHED_IN);
   tmp = gtk_text_view_new();
   gtk_container_add (GTK_CONTAINER(tmpbox), tmp);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), GTK_WRAP_WORD);

   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   g_object_set_data(G_OBJECT(tmpbox), "_dw_user", (gpointer)tmp);
   gtk_widget_show(tmp);
   gtk_widget_show(tmpbox);
   if(_DWDefaultFont)
      dw_window_set_font(tmpbox, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmpbox;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_entryfield_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_entry_new();

   gtk_entry_set_text(GTK_ENTRY(tmp), text);
   gtk_entry_set_width_chars(GTK_ENTRY(tmp), 0);

   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));

    if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_entryfield_password_new(const char *text, ULONG id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_entry_new();

   gtk_entry_set_visibility(GTK_ENTRY(tmp), FALSE);
   gtk_entry_set_text(GTK_ENTRY(tmp), text);
   gtk_entry_set_width_chars(GTK_ENTRY(tmp), 0);

   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));

   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_combobox_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   GtkListStore *store;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   store = gtk_list_store_new(1, G_TYPE_STRING);
   tmp = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
   gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(tmp), 0);
   gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(tmp))), text);
   gtk_entry_set_width_chars(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(tmp))), 0);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_COMBOBOX));
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_button_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_button_new_with_label(text);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND dw_bitmapbutton_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   GtkWidget *bitmap;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_button_new();
   bitmap = dw_bitmap_new(id);

   if(bitmap)
   {
      dw_window_set_bitmap(bitmap, id, NULL);
      gtk_container_add (GTK_CONTAINER(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_show(tmp);
   if(text)
   {
      gtk_widget_set_tooltip_text(tmp, text);
   }
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       label_text: Text to display on button. TBD when Windows works
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 */
HWND dw_bitmapbutton_new_from_file(const char *text, unsigned long id, const char *filename)
{
   GtkWidget *tmp;
   GtkWidget *bitmap;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Create a new button */
   tmp = gtk_button_new();
   /* Now on to the image stuff */
   bitmap = dw_bitmap_new(id);
   if(bitmap)
   {
      dw_window_set_bitmap(bitmap, 0, filename);
      gtk_container_add(GTK_CONTAINER(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_show(tmp);
   if(text)
   {
      gtk_widget_set_tooltip_text(tmp, text);
   }
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: Raw data of image.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       len:  Length of raw data
 */
HWND dw_bitmapbutton_new_from_data(const char *text, unsigned long id, const char *data, int len)
{
   GtkWidget *tmp;
   GtkWidget *bitmap;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_button_new();
   bitmap = dw_bitmap_new(id);

   if(bitmap)
   {
      dw_window_set_bitmap_from_data(bitmap, 0, data, len);
      gtk_container_add(GTK_CONTAINER(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_show(tmp);
   if(text)
   {
      gtk_widget_set_tooltip_text(tmp, text);
   }
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_spinbutton_new(const char *text, unsigned long id)
{
   GtkAdjustment *adj;
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   adj = (GtkAdjustment *)gtk_adjustment_new ((float)atoi(text), -65536.0, 65536.0, 1.0, 5.0, 0.0);
   tmp = gtk_spin_button_new (adj, 0, 0);
   gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
   gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(tmp), TRUE);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adj);
   g_object_set_data(G_OBJECT(adj), "_dw_spinbutton", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_radiobutton_new(const char *text, ULONG id)
{
   /* This will have to be fixed in the future. */
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_radio_button_new_with_label(NULL, text);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   gtk_widget_show(tmp);

   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_slider_new(int vertical, int increments, ULONG id)
{
   GtkWidget *tmp;
   GtkAdjustment *adjustment;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, (gfloat)increments, 1, 1, 1);
   tmp = gtk_scale_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, adjustment);
   gtk_widget_show(tmp);
   gtk_scale_set_draw_value(GTK_SCALE(tmp), 0);
   gtk_scale_set_digits(GTK_SCALE(tmp), 0);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adjustment);
   g_object_set_data(G_OBJECT(adjustment), "_dw_slider", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

#ifdef HAVE_OVERLAY_SCROLLBARS
gboolean ubuntu_overlay_scrollbar_get_enabled(void);
void ubuntu_overlay_scrollbar_set_enabled(gboolean enabled);
#endif

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_scrollbar_new(int vertical, ULONG id)
{
   GtkWidget *tmp;
   GtkAdjustment *adjustment;
   int _dw_locked_by_me = FALSE;
#ifdef HAVE_OVERLAY_SCROLLBARS
   gboolean overlaysb;
#endif

   DW_MUTEX_LOCK;
#ifdef HAVE_OVERLAY_SCROLLBARS
   overlaysb = ubuntu_overlay_scrollbar_get_enabled();
   ubuntu_overlay_scrollbar_set_enabled(FALSE);
#endif
   adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, 0, 1, 1, 1);
   tmp = gtk_scrollbar_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, adjustment);
#ifdef HAVE_OVERLAY_SCROLLBARS
   ubuntu_overlay_scrollbar_set_enabled(overlaysb);
#endif
   gtk_widget_set_can_focus(tmp, FALSE);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adjustment);
   g_object_set_data(G_OBJECT(adjustment), "_dw_scrollbar", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_percent_new(unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_progress_bar_new();
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_checkbox_new(const char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_check_button_new_with_label(text);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
HWND dw_listbox_new(unsigned long id, int multi)
{
   GtkWidget *tmp, *tree;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _dw_tree_create(id)))
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   store = gtk_list_store_new(1, G_TYPE_STRING);
   tree = _dw_tree_setup(tmp, GTK_TREE_MODEL(store));
   g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX));
   g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX));

   col = gtk_tree_view_column_new();
   rend = gtk_cell_renderer_text_new();
   gtk_tree_view_column_pack_start(col, rend, TRUE);
   gtk_tree_view_column_add_attribute(col, rend, "text", 0);

   gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
   gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree), col);
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

   sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
   if(multi)
   {
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
   }
   else
   {
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
   }
   gtk_widget_show(tree);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void dw_window_set_icon(HWND handle, HICN icon)
{
   GdkPixbuf *icon_pixbuf;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   icon_pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

   if(gtk_widget_get_window(handle) && icon_pixbuf)
   {
      GList *list = g_list_append(NULL, icon_pixbuf);
      gdk_window_set_icon_list(gtk_widget_get_window(handle), list);
      g_list_free(list);
   }
   DW_MUTEX_UNLOCK;
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
int dw_window_set_bitmap(HWND handle, unsigned long id, const char *filename)
{
   GdkPixbuf *tmp = NULL;
   int i, found_ext = 0;
   int _dw_locked_by_me = FALSE;
   int retval = DW_ERROR_UNKNOWN;

   if(!id && !filename)
      return retval;

   DW_MUTEX_LOCK;
   if(id)
      tmp = _dw_find_pixbuf((HICN)id, NULL, NULL);
   else
   {
      char *file = alloca(strlen(filename) + 6);

      if(!file)
      {
         DW_MUTEX_UNLOCK;
         return DW_ERROR_GENERAL;
      }

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) != 0)
      {
         /* Try with various extentions */
         for(i = 0; i < NUM_EXTS; i++)
         {
            strcpy(file, filename);
            strcat(file, _dw_image_exts[i]);
            if(access( file, 04 ) == 0)
            {
               found_ext = 1;
               break;
            }
         }
         if(found_ext == 0)
         {
            DW_MUTEX_UNLOCK;
            return DW_ERROR_GENERAL;
         }
      }
      tmp = gdk_pixbuf_new_from_file(file, NULL);
   }

   if(tmp)
   {
      if(GTK_IS_BUTTON(handle))
      {
         GtkWidget *pixmap = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_bitmap");
         if(pixmap)
         {
            gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), tmp);
	         retval = DW_ERROR_NONE;
         }
      }
      else if(GTK_IS_IMAGE(handle))
      {
         gtk_image_set_from_pixbuf(GTK_IMAGE(handle), tmp);
         retval = DW_ERROR_NONE;
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       data: the image data
 *                 Bitmap on Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 *       len: length of data
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int dw_window_set_bitmap_from_data(HWND handle, unsigned long id, const char *data, int len)
{
   GdkPixbuf *tmp = NULL;
   int _dw_locked_by_me = FALSE;
   int retval = DW_ERROR_UNKNOWN;

   if(!id && !data)
      return retval;

   DW_MUTEX_LOCK;
   if(data)
   {
      /*
       * A real hack; create a temporary file and write the contents
       * of the data to the file
       */
      char template[] = "/tmp/dwpixmapXXXXXX";
      int written = -1, fd = mkstemp(template);

      if(fd != -1)
      {
         written = write(fd, data, len);
         close(fd);
      }
      /* Bail if we couldn't write full file */
      if(fd == -1 || written != len)
      {
         DW_MUTEX_UNLOCK;
         return DW_ERROR_GENERAL;
      }

      tmp = gdk_pixbuf_new_from_file(template, NULL);
      /* remove our temporary file */
      unlink(template);
   }
   else if (id)
      tmp = _dw_find_pixbuf((HICN)id, NULL, NULL);

   if(tmp)
   {
      if(GTK_IS_BUTTON(handle))
      {
         GtkWidget *pixmap = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_bitmap");

         if(pixmap)
         {
            gtk_image_set_from_pixbuf(GTK_IMAGE(pixmap), tmp);
            retval = DW_ERROR_NONE;
         }
      }
      else if(GTK_IS_IMAGE(handle))
      {
         gtk_image_set_from_pixbuf(GTK_IMAGE(handle), tmp);
         retval = DW_ERROR_NONE;
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associated with a given window.
 */
void dw_window_set_text(HWND handle, const char *text)
{
   int _dw_locked_by_me = FALSE;
   GtkWidget *tmp;

   DW_MUTEX_LOCK;
   if((tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi_title")))
      handle = tmp;
   if(GTK_IS_ENTRY(handle))
      gtk_entry_set_text(GTK_ENTRY(handle), text);
   else if(GTK_IS_COMBO_BOX(handle))
      gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(handle))), text);
   else if(GTK_IS_LABEL(handle))
      gtk_label_set_text(GTK_LABEL(handle), text);
   else if(GTK_IS_BUTTON(handle))
   {
      gtk_button_set_label(GTK_BUTTON(handle), text);
   }
   else if(gtk_widget_is_toplevel(handle))
      gtk_window_set_title(GTK_WINDOW(handle), text);
   else if (GTK_IS_FRAME(handle))
   {
      /*
       * This is a groupbox or status_text
       */
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_label");
      if ( tmp && GTK_IS_LABEL(tmp) )
         gtk_label_set_text(GTK_LABEL(tmp), text);
      else /* assume groupbox */
         gtk_frame_set_label(GTK_FRAME(handle), text && *text ? text : NULL);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, const char *bubbletext)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(bubbletext && *bubbletext)
      gtk_widget_set_tooltip_text(handle, bubbletext);
   else
      gtk_widget_set_has_tooltip(handle, FALSE);
   DW_MUTEX_UNLOCK;
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       text: The text associsated with a given window.
 */
char *dw_window_get_text(HWND handle)
{
   const char *possible = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_ENTRY(handle))
      possible = gtk_entry_get_text(GTK_ENTRY(handle));
   else if(GTK_IS_COMBO_BOX(handle))
      possible = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(handle))));
   else if(GTK_IS_LABEL(handle))
      possible = gtk_label_get_text(GTK_LABEL(handle));

   DW_MUTEX_UNLOCK;
   return strdup(possible ? possible : "");
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void dw_window_disable(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gtk_widget_set_sensitive(handle, FALSE);
   DW_MUTEX_UNLOCK;
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void dw_window_enable(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gtk_widget_set_sensitive(handle, TRUE);
   DW_MUTEX_UNLOCK;
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
   GList *orig = NULL, *list = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(handle && GTK_IS_CONTAINER(handle))
   {
      orig = list = gtk_container_get_children(GTK_CONTAINER(handle));
   }
   while(list)
   {
      if(GTK_IS_WIDGET(list->data))
      {
         if(id == GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list->data), "_dw_id")))
         {
            HWND ret = (HWND)list->data;
            g_list_free(orig);
            DW_MUTEX_UNLOCK;
            return ret;
         }
      }
      list = list->next;
   }
   if(orig)
      g_list_free(orig);
   DW_MUTEX_UNLOCK;
    return 0L;
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 */
unsigned int dw_mle_import(HWND handle, const char *buffer, int startpoint)
{
   unsigned int tmppoint = startpoint;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, startpoint);
         gtk_text_buffer_place_cursor(tbuffer, &iter);
         gtk_text_buffer_insert_at_cursor(tbuffer, buffer, -1);
         tmppoint = (startpoint > -1 ? startpoint : 0) + strlen(buffer);
      }
   }
   DW_MUTEX_UNLOCK;
   return tmppoint;
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported. MUST allow for trailing nul character.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
void dw_mle_export(HWND handle, char *buffer, int startpoint, int length)
{
   int _dw_locked_by_me = FALSE;
   gchar *text;

   DW_MUTEX_LOCK;
   /* force the return value to nul in case the following tests fail */
   if(buffer)
      buffer[0] = '\0';
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter start, end;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
         gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
         text = gtk_text_iter_get_text(&start, &end);
         if(text) /* Should this get freed? */
         {
            if(buffer)
               strcpy(buffer, text);
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Obtains information about an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          bytes: A pointer to a variable to return the total bytes.
 *          lines: A pointer to a variable to return the number of lines.
 */
void dw_mle_get_size(HWND handle, unsigned long *bytes, unsigned long *lines)
{
   int _dw_locked_by_me = FALSE;

   if(bytes)
      *bytes = 0;
   if(lines)
      *lines = 0;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));

         if(bytes)
            *bytes = gtk_text_buffer_get_char_count(buffer);
         if(lines)
            *lines = gtk_text_buffer_get_line_count(buffer);
      }
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
void dw_mle_delete(HWND handle, int startpoint, int length)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter start, end;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
         gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
         gtk_text_buffer_delete(tbuffer, &start, &end);
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void dw_mle_clear(HWND handle)
{
   int length, _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));

         length = -1;
         gtk_text_buffer_set_text(buffer, "", length);
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          line: Line to be visible.
 */
void dw_mle_set_visible(HWND handle, int line)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;
         GtkTextMark *mark = (GtkTextMark *)g_object_get_data(G_OBJECT(handle), "_dw_mark");

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, 0);
         gtk_text_iter_set_line(&iter, line);
         if(!mark)
         {
            mark = gtk_text_buffer_create_mark(tbuffer, NULL, &iter, FALSE);
            g_object_set_data(G_OBJECT(handle), "_dw_mark", (gpointer)mark);
         }
         else
            gtk_text_buffer_move_mark(tbuffer, mark, &iter);
         gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tmp), mark,
                               0, FALSE, 0, 0);
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void dw_mle_set_editable(HWND handle, int state)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
         gtk_text_view_set_editable(GTK_TEXT_VIEW(tmp), state);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void dw_mle_set_word_wrap(HWND handle, int state)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
         gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), state ? GTK_WRAP_WORD : GTK_WRAP_NONE);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
void dw_mle_set_auto_complete(HWND handle, int state)
{
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void dw_mle_set_cursor(HWND handle, int point)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;
         GtkTextMark *mark = (GtkTextMark *)g_object_get_data(G_OBJECT(handle), "_dw_mark");

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
         if(!mark)
         {
            mark = gtk_text_buffer_create_mark(tbuffer, NULL, &iter, FALSE);
            g_object_set_data(G_OBJECT(handle), "_dw_mark", (gpointer)mark);
         }
         else
            gtk_text_buffer_move_mark(tbuffer, mark, &iter);
         gtk_text_buffer_place_cursor(tbuffer, &iter);
         gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tmp), mark,
                               0, FALSE, 0, 0);
      }
   }
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
int dw_mle_search(HWND handle, const char *text, int point, unsigned long flags)
{
   int _dw_locked_by_me = FALSE, retval = 0;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter, found;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
         gtk_text_iter_forward_search(&iter, text, GTK_TEXT_SEARCH_TEXT_ONLY, &found, NULL, NULL);
         retval = gtk_text_iter_get_offset(&found);
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void dw_mle_freeze(HWND handle)
{
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void dw_mle_thaw(HWND handle)
{
}

/* Internal function to update the progress bar
 * while in an indeterminate state.
 */
gboolean _dw_update_progress_bar(gpointer data)
{
   if(g_object_get_data(G_OBJECT(data), "_dw_alive"))
   {
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));
      return TRUE;
   }
   return FALSE;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void dw_percent_set_pos(HWND handle, unsigned int position)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(position == DW_PERCENT_INDETERMINATE)
   {
      /* Check if we are indeterminate already */
      if(!g_object_get_data(G_OBJECT(handle), "_dw_alive"))
      {
         /* If not become indeterminate... and start a timer to continue */
         gtk_progress_bar_pulse(GTK_PROGRESS_BAR(handle));
         g_object_set_data(G_OBJECT(handle), "_dw_alive", GINT_TO_POINTER(1));
         g_timeout_add(100, (GSourceFunc)_dw_update_progress_bar, (gpointer)handle);
      }
   }
   else
   {
      /* Cancel the existing timer if one is there */
      g_object_set_data(G_OBJECT(handle), "_dw_alive", NULL);
      /* Set the position like normal */
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(handle), (gfloat)position/100);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int dw_slider_get_pos(HWND handle)
{
   int val = 0, _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      int max = _dw_round_value(gtk_adjustment_get_upper(adjustment)) - 1;
      int thisval = _dw_round_value(gtk_adjustment_get_value(adjustment));

      if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_VERTICAL)
         val = max - thisval;
        else
         val = thisval;
   }
   DW_MUTEX_UNLOCK;
   return val;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void dw_slider_set_pos(HWND handle, unsigned int position)
{
   int _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      int max = _dw_round_value(gtk_adjustment_get_upper(adjustment)) - 1;

      if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_VERTICAL)
         gtk_adjustment_set_value(adjustment, (gfloat)(max - position));
        else
         gtk_adjustment_set_value(adjustment, (gfloat)position);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int dw_scrollbar_get_pos(HWND handle)
{
   int val = 0, _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
      val = _dw_round_value(gtk_adjustment_get_value(adjustment));
   DW_MUTEX_UNLOCK;
   return val;
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
   int _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_dw_value_changed_event", GINT_TO_POINTER(1));
      gtk_adjustment_set_value(adjustment, (gfloat)position);
      g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_dw_value_changed_event", GINT_TO_POINTER(0));
   }
   DW_MUTEX_UNLOCK;
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
   int _dw_locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      gtk_adjustment_set_upper(adjustment, (gdouble)range);
      gtk_adjustment_set_page_increment(adjustment,(gdouble)visible);
      gtk_adjustment_set_page_size(adjustment, (gdouble)visible);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void dw_spinbutton_set_pos(HWND handle, long position)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(handle), (gfloat)position);
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 *          position: Current value of the spinbutton.
 */
void dw_spinbutton_set_limits(HWND handle, long upper, long lower)
{
   long curval;
   GtkAdjustment *adj;
   int _dw_locked_by_me = FALSE;

   curval = dw_spinbutton_get_pos(handle);
   DW_MUTEX_LOCK;
   adj = (GtkAdjustment *)gtk_adjustment_new((gfloat)curval, (gfloat)lower, (gfloat)upper, 1.0, 5.0, 0.0);
   gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(handle), adj);
   /*
    * Set our internal relationships between the adjustment and the spinbutton
    */
   g_object_set_data(G_OBJECT(handle), "_dw_adjustment", (gpointer)adj);
   g_object_set_data(G_OBJECT(adj), "_dw_spinbutton", (gpointer)handle);
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void dw_entryfield_set_limit(HWND handle, ULONG limit)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gtk_entry_set_max_length(GTK_ENTRY(handle), limit);
   DW_MUTEX_UNLOCK;
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long dw_spinbutton_get_pos(HWND handle)
{
   long retval;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   retval = (long)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(handle));
   DW_MUTEX_UNLOCK;

   return retval;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int dw_checkbox_get(HWND handle)
{
   int retval;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   retval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(handle));
   DW_MUTEX_UNLOCK;

   return retval;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void dw_checkbox_set(HWND handle, int value)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(handle), value);
   DW_MUTEX_UNLOCK;
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
HTREEITEM dw_tree_insert_after(HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return NULL;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

      pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

      gtk_tree_store_insert_after(store, iter, (GtkTreeIter *)parent, (GtkTreeIter *)item);
      gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
      retval = (HTREEITEM)iter;
   }
   DW_MUTEX_UNLOCK;

   return retval;
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
HTREEITEM dw_tree_insert(HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return NULL;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

      pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

      gtk_tree_store_append (store, iter, (GtkTreeIter *)parent);
      gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
      retval = (HTREEITEM)iter;
   }
   DW_MUTEX_UNLOCK;

   return retval;
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void dw_tree_item_change(HWND handle, HTREEITEM item, const char *title, HICN icon)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

      gtk_tree_store_set(store, (GtkTreeIter *)item, 0, title, 1, pixbuf, -1);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
void dw_tree_item_set_data(HWND handle, HTREEITEM item, void *itemdata)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle || !item)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
         gtk_tree_store_set(store, (GtkTreeIter *)item, 2, itemdata, -1);
   DW_MUTEX_UNLOCK;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
   int _dw_locked_by_me = FALSE;
   char *text = NULL;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(!handle || !item)
      return text;

   DW_MUTEX_LOCK;
   tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   if(tree && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      gtk_tree_model_get(store, (GtkTreeIter *)item, _DW_DATA_TYPE_STRING, &text, -1);
   if(text)
   {
      char *temp = text;
      text = strdup(temp);
      g_free(temp);
   }
   DW_MUTEX_UNLOCK;
   return text;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
HTREEITEM API dw_tree_get_parent(HWND handle, HTREEITEM item)
{
   int _dw_locked_by_me = FALSE;
   HTREEITEM parent = (HTREEITEM)0;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(!handle || !item)
      return parent;

   DW_MUTEX_LOCK;
   tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   if(tree && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      GtkTreeIter iter;

      if(gtk_tree_model_iter_parent(store, &iter, (GtkTreeIter *)item))
         gtk_tree_model_get(store, &iter, 3, &parent, -1);
   }
   DW_MUTEX_UNLOCK;
   return parent;
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
void *dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
   void *ret = NULL;
   GtkWidget *tree;
   GtkTreeModel *store;
   int _dw_locked_by_me = FALSE;

   if(!handle || !item)
      return NULL;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
         gtk_tree_model_get(store, (GtkTreeIter *)item, 2, &ret, -1);
   DW_MUTEX_UNLOCK;
   return ret;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void dw_tree_item_select(HWND handle, HTREEITEM item)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle || !item)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
      GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
      gtk_tree_selection_select_iter(sel, (GtkTreeIter *)item);
      gtk_tree_path_free(path);
   }
   DW_MUTEX_UNLOCK;
}

static void _dw_recursive_free(GtkTreeModel *store, GtkTreeIter parent)
{
   void *data;
   GtkTreeIter iter;

   gtk_tree_model_get(store, &parent, 3, &data, -1);
   if(data)
      free(data);
   gtk_tree_store_set(GTK_TREE_STORE(store), &parent, 3, NULL, -1);

   if(gtk_tree_model_iter_children(store, &iter, &parent))
   {
      do {
         _dw_recursive_free(GTK_TREE_MODEL(store), iter);
      } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
   }
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void dw_tree_clear(HWND handle)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         GtkTreeIter iter;

         if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
         {
            do {
               _dw_recursive_free(GTK_TREE_MODEL(store), iter);
            } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
         }
         gtk_tree_store_clear(store);
      }
   DW_MUTEX_UNLOCK;
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void dw_tree_item_expand(HWND handle, HTREEITEM item)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
      gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE);
      gtk_tree_path_free(path);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
      gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), path);
      gtk_tree_path_free(path);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void dw_tree_item_delete(HWND handle, HTREEITEM item)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      gtk_tree_store_remove(store, (GtkTreeIter *)item);
      free(item);
   }
   DW_MUTEX_UNLOCK;
}

#define _DW_CONTAINER_STORE_EXTRA 2

static int _dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator, int extra)
{
   int z;
   char numbuf[25] = {0};
   GtkWidget *tree;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;
   int _dw_locked_by_me = FALSE;
   GType *array = calloc(count + _DW_CONTAINER_STORE_EXTRA + 1, sizeof(GType));

   DW_MUTEX_LOCK;
   /* Save some of the info so it is easily accessible */
   g_object_set_data(G_OBJECT(handle), "_dw_cont_columns", GINT_TO_POINTER(count));
   g_object_set_data(G_OBJECT(handle), "_dw_cont_extra", GINT_TO_POINTER(extra));

   /* First param is row title */
   array[0] = G_TYPE_STRING;
   /* Second param is row data */
   array[1] = G_TYPE_POINTER;
   array[2] = G_TYPE_POINTER;
   /* First loop... create array to create the store */
   for(z=0;z<count;z++)
   {
      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         array[_DW_CONTAINER_STORE_EXTRA] = GDK_TYPE_PIXBUF;
         array[_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = GDK_TYPE_PIXBUF;
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_ULONG;
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
   }
   /* Create the store and then the tree */
   store = gtk_list_store_newv(count + _DW_CONTAINER_STORE_EXTRA + 1, array);
   tree = _dw_tree_setup(handle, GTK_TREE_MODEL(store));
   g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER));
   /* Second loop... create the columns */
   for(z=0;z<count;z++)
   {
      snprintf(numbuf, 24, "_dw_cont_col%d", z);
      g_object_set_data(G_OBJECT(tree), numbuf, GINT_TO_POINTER(flags[z]));
      col = gtk_tree_view_column_new();
      rend = NULL;
      void **params = calloc(sizeof(void *), 3);

      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", _DW_CONTAINER_STORE_EXTRA);
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", _DW_CONTAINER_STORE_EXTRA+1);
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", z+_DW_CONTAINER_STORE_EXTRA+1);
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      g_object_set_data(G_OBJECT(col), "_dw_column", GINT_TO_POINTER(z));
      params[2] = tree;
      g_signal_connect_data(G_OBJECT(col), "clicked", G_CALLBACK(_dw_column_click_event), (gpointer)params, _dw_signal_disconnect, 0);
      gtk_tree_view_column_set_title(col, titles[z]);
      if(flags[z] & DW_CFA_RIGHT)
      {
         gtk_tree_view_column_set_alignment(col, 1.0);
         if(rend)
            gtk_cell_renderer_set_alignment(rend, 1.0, 0.5);
      }
      else if(flags[z] & DW_CFA_CENTER)
      {
         gtk_tree_view_column_set_alignment(col, 0.5);
         if(rend)
            gtk_cell_renderer_set_alignment(rend, 0.5, 0.5);
      }
      gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
   }
   /* Finish up */
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
   gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(tree), TRUE);
   gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tree), TRUE);
   sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
   if(g_object_get_data(G_OBJECT(handle), "_dw_multi_sel"))
   {
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
   }
   else
   {
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
   }
   gtk_widget_show(tree);
   free(array);
   if(_DWDefaultFont)
      dw_window_set_font(handle, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return DW_ERROR_NONE;
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
int dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator)
{
   return _dw_container_setup(handle, flags, titles, count, separator, 0);
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
	
	g_object_set_data(G_OBJECT(handle), "_dw_coltitle", newtitle);
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 */
int dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count)
{
   char **newtitles = malloc(sizeof(char *) * (count + 1));
   unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 1));
   char *coltitle = (char *)g_object_get_data(G_OBJECT(handle), "_dw_coltitle");

   newtitles[0] = coltitle ? coltitle : "Filename";
   newflags[0] = DW_CFA_STRINGANDICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

   memcpy(&newtitles[1], titles, sizeof(char *) * count);
   memcpy(&newflags[1], flags, sizeof(unsigned long) * count);

   _dw_container_setup(handle, newflags, newtitles, count + 1, 1, 1);

   if(coltitle)
   {
	  g_object_set_data(G_OBJECT(handle), "_dw_coltitle", NULL);
	  free(coltitle);
   }
   if ( newtitles) free(newtitles);
   if ( newflags ) free(newflags);
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
HICN dw_icon_load(unsigned long module, unsigned long id)
{
   return (HICN)id;
}

/* Internal function to keep HICNs from getting too big */
GdkPixbuf *_icon_resize(GdkPixbuf *ret)
{
   if(ret)
   {
      int pwidth = gdk_pixbuf_get_width(ret);
      int pheight = gdk_pixbuf_get_height(ret);

      if(pwidth > 24 || pheight > 24)
      {
         GdkPixbuf *orig = ret;
         ret = gdk_pixbuf_scale_simple(ret, pwidth > 24 ? 24 : pwidth, pheight > 24 ? 24 : pheight, GDK_INTERP_BILINEAR);
         g_object_unref(G_OBJECT(orig));
      }
   }
   return ret;
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
   int _dw_locked_by_me = FALSE;
   char *file = alloca(strlen(filename) + 6);
   int i, found_ext = 0;
   HICN ret = 0;

   if (!file)
      return 0;

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if (access(file, 04) != 0)
   {
      /* Try with various extentions */
      for ( i = 0; i < NUM_EXTS; i++ )
      {
         strcpy( file, filename );
         strcat( file, _dw_image_exts[i] );
         if ( access( file, 04 ) == 0 )
         {
            found_ext = 1;
            break;
         }
      }
      if ( found_ext == 0 )
      {
         return 0;
      }
   }

   DW_MUTEX_LOCK;
   ret = _icon_resize(gdk_pixbuf_new_from_file(file, NULL));
   DW_MUTEX_UNLOCK;
   return ret;
}

/*
 * Obtains an icon from data.
 * Parameters:
 *       data: Source of data for image.
 *       len:  length of data
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
   int fd, written = -1, _dw_locked_by_me = FALSE;
   char template[] = "/tmp/dwiconXXXXXX";
   HICN ret = 0;

   /*
    * A real hack; create a temporary file and write the contents
    * of the data to the file
    */
   if((fd = mkstemp(template)) != -1)
   {
      written = write(fd, data, len);
      close(fd);
   }
   /* Bail if we couldn't write full file */
   if(fd == -1 || written != len)
      return 0;
   DW_MUTEX_LOCK;
   ret = _icon_resize(gdk_pixbuf_new_from_file(template, NULL));
   DW_MUTEX_UNLOCK;
   unlink(template);
   return ret;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void dw_icon_free(HICN handle)
{
   int iicon = GPOINTER_TO_INT(handle);

   if(iicon > 65535)
   {
      g_object_unref(handle);
   }
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void *dw_container_alloc(HWND handle, int rowcount)
{
   int z, prevrowcount = 0;
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      prevrowcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      for(z=0;z<rowcount;z++)
      {
         gtk_list_store_append(store, &iter);
      }
      g_object_set_data(G_OBJECT(cont), "_dw_insertpos", GINT_TO_POINTER(prevrowcount));
      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rowcount + prevrowcount));
   }
   DW_MUTEX_UNLOCK;
   return (void *)cont;
}

/*
 * Internal representation of dw_container_set_item() extracted so we can pass
 * two data pointers; icon and text for dw_filesystem_set_item().
 */
void _dw_container_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
   char numbuf[25] = {0}, textbuffer[101] = {0};
   int flag = 0;
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      snprintf(numbuf, 24, "_dw_cont_col%d", column);
      flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), numbuf));
      if(pointer)
      {
         row += GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_insertpos"));
      }

      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, row))
      {
         if(flag & DW_CFA_STRINGANDICON)
         {
            void **thisdata = (void **)data;
            HICN hicon = data ? *((HICN *)thisdata[0]) : 0;
            char *tmp = data ? (char *)thisdata[1] : NULL;
            GdkPixbuf *pixbuf = hicon ? _dw_find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, _DW_CONTAINER_STORE_EXTRA, pixbuf, -1);
            gtk_list_store_set(store, &iter, _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_BITMAPORICON)
         {
            HICN hicon = data ? *((HICN *)data) : 0;
            GdkPixbuf *pixbuf = hicon ? _dw_find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, pixbuf, -1);
         }
         else if(flag & DW_CFA_STRING)
         {
            char *tmp = data ? *((char **)data) : NULL;
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_ULONG)
         {
            ULONG tmp = data ? *((ULONG *)data): 0;

            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_DATE)
         {
            if(data)
            {
               struct tm curtm;
               CDATE cdate = *((CDATE *)data);

               memset( &curtm, 0, sizeof(curtm) );
               curtm.tm_mday = cdate.day;
               curtm.tm_mon = cdate.month - 1;
               curtm.tm_year = cdate.year - 1900;

               strftime(textbuffer, 100, "%x", &curtm);
            }
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, textbuffer, -1);
         }
         else if(flag & DW_CFA_TIME)
         {
            if(data)
            {
               struct tm curtm;
               CTIME ctime = *((CTIME *)data);

               memset( &curtm, 0, sizeof(curtm) );
               curtm.tm_hour = ctime.hours;
               curtm.tm_min = ctime.minutes;
               curtm.tm_sec = ctime.seconds;

               strftime(textbuffer, 100, "%X", &curtm);
            }
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, textbuffer, -1);
         }
      }
   }
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
void dw_container_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
   _dw_container_set_item(handle, pointer, column, row, data);
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void dw_container_change_item(HWND handle, int column, int row, void *data)
{
   _dw_container_set_item(handle, NULL, column, row, data);
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
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void dw_filesystem_set_file(HWND handle, void *pointer, int row, const char *filename, HICN icon)
{
   void *data[2] = { (void *)&icon, (void *)filename };

   _dw_container_set_item(handle, pointer, 0, row, (void *)data);
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
void dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
   _dw_container_set_item(handle, pointer, column + 1, row, data);
}

/*
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int dw_container_get_column_type(HWND handle, int column)
{
   char numbuf[25] = {0};
   int flag, rc = 0;
   GtkWidget *cont = handle;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
   if(!cont)
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }

   snprintf(numbuf, 24, "_dw_cont_col%d", column);
   flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), numbuf));

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
#if !GTK_CHECK_VERSION(3,14,0)
   GtkWidget *cont;
   int _dw_locked_by_me = FALSE;
   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
   {
      if(oddcolor == DW_RGB_TRANSPARENT && evencolor == DW_RGB_TRANSPARENT)
         gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(cont), FALSE);
      else
         gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(cont), TRUE);
   }
   DW_MUTEX_UNLOCK;
#endif
}

/*
 * Sets the width of a column in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          column: Zero based column of width being set.
 *          width: Width of column in pixels.
 */
void dw_container_set_column_width(HWND handle, int column, int width)
{
   GtkWidget *cont;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
   {
      GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(cont), column);

      if(col && GTK_IS_TREE_VIEW_COLUMN(col))
      {
         gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(col), width);
      }
   }
   DW_MUTEX_UNLOCK;
}

/* Internal version for both */
void _dw_container_set_row_data(HWND handle, void *pointer, int row, int type, void *data)
{
   GtkWidget *cont = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      if(pointer)
      {
         row += GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_insertpos"));
      }

      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, row))
      {
         gtk_list_store_set(store, &iter, type, (gpointer)data, -1);
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void dw_container_set_row_title(void *pointer, int row, const char *title)
{
   _dw_container_set_row_data(pointer, pointer, row, _DW_DATA_TYPE_STRING, (void *)title);
}

/*
 * Changes the title of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void dw_container_change_row_title(HWND handle, int row, const char *title)
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
void dw_container_set_row_data(void *pointer, int row, void *data)
{
   _dw_container_set_row_data(pointer, pointer, row, _DW_DATA_TYPE_POINTER, data);
}

/*
 * Changes the data of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
void dw_container_change_row_data(HWND handle, int row, void *data)
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
void dw_container_insert(HWND handle, void *pointer, int rowcount)
{
   /* Don't need to do anything here */
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void dw_container_delete(HWND handle, int rowcount)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;
      int rows, z;

      rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      for(z=0;z<rowcount;z++)
      {
         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0))
            gtk_list_store_remove(store, &iter);
      }

      if(rows - rowcount < 0)
         rows = 0;
      else
         rows -= rowcount;

      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rows));
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void dw_container_clear(HWND handle, int redraw)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(0));
      g_object_set_data(G_OBJECT(cont), "_dw_insertpos", GINT_TO_POINTER(0));

      gtk_list_store_clear(store);
   }
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
void dw_container_scroll(HWND handle, int direction, long rows)
{
   GtkWidget *cont;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
   {
      GtkAdjustment *adjust = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(cont));

      if(adjust)
      {
         gint rowcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));
         gdouble currpos = gtk_adjustment_get_value(adjust);
         gdouble upper = gtk_adjustment_get_upper(adjust);
         gdouble lower = gtk_adjustment_get_lower(adjust);
         gdouble change;

         /* Safety check */
         if(rowcount < 1)
         {
            DW_MUTEX_UNLOCK;
            return;
         }

         change = ((gdouble)rows/(gdouble)rowcount) * (upper - lower);

         switch(direction)
         {
           case DW_SCROLL_TOP:
           {
               gtk_adjustment_set_value(adjust, lower);
               break;
           }
           case DW_SCROLL_BOTTOM:
           {
               gtk_adjustment_set_value(adjust, upper);
               break;
           }
           case DW_SCROLL_UP:
           {
               gdouble newpos = currpos - change;
               if(newpos < lower)
               {
                   newpos = lower;
               }
               gtk_adjustment_set_value(adjust, newpos);
               break;
           }
           case DW_SCROLL_DOWN:
           {
               gdouble newpos = currpos + change;
               if(newpos > upper)
               {
                   newpos = upper;
               }
               gtk_adjustment_set_value(adjust, newpos);
               break;
           }
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
char *dw_container_query_start(HWND handle, unsigned long flags)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   char *retval = NULL;
   int _dw_locked_by_me = FALSE;
   int type = flags & DW_CR_RETDATA ? _DW_DATA_TYPE_POINTER : _DW_DATA_TYPE_STRING;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      /* These should be separate but right now this will work */
      if(flags & DW_CRA_SELECTED)
      {
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cont));
         GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);
         if(list)
         {
            GtkTreePath *path = g_list_nth_data(list, 0);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, indices[0]))
                  {
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
                     g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(1));
                  }
               }
            }
            g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free(list);
         }
      }
      else if(flags & DW_CRA_CURSORED)
      {
         GtkTreePath *path;

         gtk_tree_view_get_cursor(GTK_TREE_VIEW(cont), &path, NULL);
         if(path)
         {
            GtkTreeIter iter;

            if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
            {
               gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            }
            gtk_tree_path_free(path);
         }
      }
      else
      {
         GtkTreeIter iter;

         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0))
         {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(1));
         }
      }
   }
   /* If there is a title, duplicate it and free the temporary copy */
   if(retval && type == _DW_DATA_TYPE_STRING)
   {
      char *temp = retval;
      retval = strdup(temp);
      g_free(temp);
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
char *dw_container_query_next(HWND handle, unsigned long flags)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   char *retval = NULL;
   int _dw_locked_by_me = FALSE;
   int type = flags & DW_CR_RETDATA ? _DW_DATA_TYPE_POINTER : _DW_DATA_TYPE_STRING;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      int pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_querypos"));
      int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);

      /* These should be separate but right now this will work */
      if(flags & DW_CRA_SELECTED)
      {
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cont));
         GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);

         if(list)
         {
            GtkTreePath *path = g_list_nth_data(list, pos);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, indices[0]))
                  {
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
                     g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(pos+1));
                  }
               }
            }
            g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free(list);
         }
      }
      else if(flags & DW_CRA_CURSORED)
      {
         /* There will only be one item cursored,
          * retrieve it with dw_container_query_start()
          */
         retval = NULL;
      }
      else
      {
         GtkTreeIter iter;

         if(pos < count && gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, pos))
         {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(pos+1));
         }
      }
   }
   /* If there is a title, duplicate it and free the temporary copy */
   if(retval && type == _DW_DATA_TYPE_STRING)
   {
      char *temp = retval;
      retval = strdup(temp);
      g_free(temp);
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

int _dw_find_iter(GtkListStore *store, GtkTreeIter *iter, void *data, int textcomp)
{
   int z, rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
   void *thisdata;
   int retval = FALSE;

   for(z=0;z<rows;z++)
   {
      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, NULL, z))
      {
         /* Get either string from position 0 or pointer from position 1 */
         gtk_tree_model_get(GTK_TREE_MODEL(store), iter, textcomp ? _DW_DATA_TYPE_STRING : _DW_DATA_TYPE_POINTER, &thisdata, -1);
         if((textcomp && thisdata && strcmp((char *)thisdata, (char *)data) == 0) || (!textcomp && thisdata == data))
         {
            retval = TRUE;
            z = rows;
         }
         if(textcomp && thisdata)
            g_free(thisdata);
      }
   }
   return retval;
}

void _dw_container_cursor(HWND handle, void *data, int textcomp)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      if(_dw_find_iter(store, &iter, data, textcomp))
      {
         GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

         if(path)
         {
            gtk_tree_view_row_activated(GTK_TREE_VIEW(cont), path, NULL);
            gtk_tree_path_free(path);
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_cursor(HWND handle, const char *text)
{
   _dw_container_cursor(handle, (void *)text, TRUE);
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_cursor_by_data(HWND handle, void *data)
{
   _dw_container_cursor(handle, data, FALSE);
}

void _dw_container_delete_row(HWND handle, void *data, int textcomp)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;
      int rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      if(_dw_find_iter(store, &iter, data, textcomp))
      {
         gtk_list_store_remove(store, &iter);
         rows--;
      }

      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rows));
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_delete_row(HWND handle, const char *text)
{
   _dw_container_delete_row(handle, (void *)text, TRUE);
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_delete_row_by_data(HWND handle, void *data)
{
   _dw_container_delete_row(handle, data, FALSE);
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void dw_container_optimize(HWND handle)
{
   GtkWidget *cont;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         gtk_tree_view_columns_autosize(GTK_TREE_VIEW(cont));
   DW_MUTEX_UNLOCK;
}

/* Translate the status message into a message on our buddy window */
static void _dw_status_translate(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
   GdkEventButton event = { 0 };
   long x, y;
   gboolean retval;

   dw_pointer_query_pos(&x, &y);

   event.button = button;
   event.x = x;
   event.y = y;

   g_signal_emit_by_name(G_OBJECT(user_data), "button_press_event", &event, &retval);
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void dw_taskbar_insert(HWND handle, HICN icon, const char *bubbletext)
{
   GtkStatusIcon *status;
   GdkPixbuf *pixbuf;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   pixbuf = _dw_find_pixbuf(icon, NULL, NULL);
   status = gtk_status_icon_new_from_pixbuf(pixbuf);
   if(bubbletext)
      gtk_status_icon_set_tooltip_text(status, bubbletext);
   g_object_set_data(G_OBJECT(handle), "_dw_taskbar", status);
   g_signal_connect(G_OBJECT (status), "popup-menu", G_CALLBACK (_dw_status_translate), handle);
   gtk_status_icon_set_visible(status, TRUE);
   DW_MUTEX_UNLOCK;
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void dw_taskbar_delete(HWND handle, HICN icon)
{
   GtkStatusIcon *status;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   status = g_object_get_data(G_OBJECT(handle), "_dw_taskbar");
   g_object_unref(G_OBJECT(status));
   DW_MUTEX_UNLOCK;
}

/* Make sure the widget is out of the dirty list if it is destroyed */
static void _dw_render_destroy(GtkWidget *widget, gpointer data)
{
   _dw_dirty_list = g_list_remove(_dw_dirty_list, widget);
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
HWND dw_render_new(unsigned long id)
{
   int _dw_locked_by_me = FALSE;
   GtkWidget *tmp;

   DW_MUTEX_LOCK;
   tmp = gtk_drawing_area_new();
   gtk_widget_set_events(tmp, GDK_EXPOSURE_MASK
                    | GDK_LEAVE_NOTIFY_MASK
                    | GDK_BUTTON_PRESS_MASK
                    | GDK_BUTTON_RELEASE_MASK
                    | GDK_KEY_PRESS_MASK
                    | GDK_POINTER_MOTION_MASK
                    | GDK_POINTER_MOTION_HINT_MASK);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   g_signal_connect(G_OBJECT(tmp), "destroy", G_CALLBACK(_dw_render_destroy), NULL);
   gtk_widget_set_can_focus(tmp, TRUE);
   gtk_widget_show(tmp);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
void API dw_render_redraw(HWND handle)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(handle && GTK_IS_WIDGET(handle))
      gtk_widget_queue_draw(handle);
   DW_MUTEX_UNLOCK;
}

/* Returns a GdkRGBA from a DW color */
static GdkRGBA _dw_internal_color(unsigned long value)
{
   if(DW_RGB_COLOR & value)
   {
      GdkRGBA color = { (gfloat)DW_RED_VALUE(value) / 255.0, (gfloat)DW_GREEN_VALUE(value) / 255.0, (gfloat)DW_BLUE_VALUE(value) / 255.0, 1.0 };
      return color;
   }
   if (value < 16)
      return _dw_colors[value];
   return _dw_colors[0];
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void dw_color_foreground_set(unsigned long value)
{
   GdkRGBA color = _dw_internal_color(value);
   GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

   *foreground = color;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void dw_color_background_set(unsigned long value)
{
   GdkRGBA *background = pthread_getspecific(_dw_bg_color_key);

   if(value == DW_CLR_DEFAULT)
   {
      if(background)
      {
         pthread_setspecific(_dw_bg_color_key, NULL);
         free(background);
      }
   }
   else
   {
      GdkRGBA color = _dw_internal_color(value);

      if(!background)
      {
         background = malloc(sizeof(GdkRGBA));
         pthread_setspecific(_dw_bg_color_key, background);
      }
      *background = color;
   }
}

#if GTK_CHECK_VERSION(3,3,11)
/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
   GtkColorChooser *cd;
   GdkRGBA color = _dw_internal_color(value);
   int _dw_locked_by_me = FALSE;
   unsigned long retcolor = value;

   DW_MUTEX_LOCK;
   cd = (GtkColorChooser *)gtk_color_chooser_dialog_new("Choose color", NULL);
   gtk_color_chooser_set_use_alpha(cd, FALSE);
   gtk_color_chooser_set_rgba(cd, &color);

   gtk_widget_show(GTK_WIDGET(cd));

   if(gtk_dialog_run(GTK_DIALOG(cd)) == GTK_RESPONSE_OK)
   {
      gtk_color_chooser_get_rgba(cd, &color);
      retcolor = DW_RGB((int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));
   }
   gtk_widget_destroy(GTK_WIDGET(cd));
   DW_MUTEX_UNLOCK;
   return retcolor;
}
#else
static int _dw_color_active = 0;

/* Internal function to handle the color OK press */
static gint _dw_color_ok(GtkWidget *widget, DWDialog *dwwait)
{
   GdkRGBA color;
   unsigned long dw_color;
   GtkColorSelection *colorsel;

   if(!dwwait)
      return FALSE;

   colorsel =  (GtkColorSelection *)gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dwwait->data));
   gtk_color_selection_get_current_rgba(colorsel, &color);
   gtk_widget_destroy(GTK_WIDGET(dwwait->data));
   _dw_color_active = 0;
   dw_color = DW_RGB((int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));
   dw_dialog_dismiss(dwwait, (void *)dw_color);
   return FALSE;
}

/* Internal function to handle the color Cancel press */
static gint _dw_color_cancel(GtkWidget *widget, DWDialog *dwwait)
{
   if(!dwwait)
      return FALSE;

   gtk_widget_destroy(GTK_WIDGET(dwwait->data));
   _dw_color_active = 0;
   dw_dialog_dismiss(dwwait, (void *)-1);
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
   GtkWidget *colorw, *ok_button, *cancel_button;
   int _dw_locked_by_me = FALSE;
   DWDialog *dwwait;
   GtkColorSelection *colorsel;
   GdkRGBA color = _dw_internal_color(value);
   unsigned long dw_color;

   DW_MUTEX_LOCK;

   /* The DW mutex should be sufficient for
    * insuring no thread changes this unknowingly.
    */
   if(_dw_color_active)
   {
      DW_MUTEX_UNLOCK;
      return value;
   }

   _dw_color_active = 1;

   colorw = gtk_color_selection_dialog_new("Select Color");

   dwwait = dw_dialog_new((void *)colorw);

   colorsel = (GtkColorSelection *)gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(colorw));
   g_object_get(G_OBJECT(colorw), "ok-button", &ok_button, "cancel-button", &cancel_button, NULL);
   g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(_dw_color_ok), dwwait);
   g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(_dw_color_cancel), dwwait);

   gtk_color_selection_set_previous_rgba(colorsel,&color);
   gtk_color_selection_set_current_rgba(colorsel,&color);
   gtk_color_selection_set_has_palette(colorsel,TRUE);

   gtk_widget_show(colorw);

   dw_color = (unsigned long)dw_dialog_wait(dwwait);
   if ((unsigned long)dw_color == -1)
      dw_color = value;
   DW_MUTEX_UNLOCK;
   return (unsigned long)dw_color;
}
#endif

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void dw_draw_point(HWND handle, HPIXMAP pixmap, int x, int y)
{
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();

      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x, y);
      cairo_stroke(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
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
void dw_draw_line(HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2)
{
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x1, y1);
      cairo_line_to(cr, x2, y2);
      cairo_stroke(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
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
void dw_draw_polygon(HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
{
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
   int z;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x[0], y[0]);
      for(z=1;z<npoints;z++)
      {
         cairo_line_to(cr, x[z], y[z]);
      }
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
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
void dw_draw_rect(HWND handle, HPIXMAP pixmap, int flags, int x, int y, int width, int height)
{
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x, y);
      cairo_line_to(cr, x, y + height);
      cairo_line_to(cr, x + width, y + height);
      cairo_line_to(cr, x + width, y);
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
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
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);
      int width = abs(x2-x1);
      float scale = fabs((float)(y2-y1))/(float)width;

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      if(scale != 1.0)
         cairo_scale(cr, 1.0, scale);
      if(flags & DW_DRAW_FULL)
         cairo_arc(cr, xorigin, yorigin / scale, width/2, 0, M_PI*2);
      else
      {
         double dx = xorigin - x1;
         double dy = yorigin - y1;
         double r = sqrt(dx*dx + dy*dy);
         double a1 = atan2((y1-yorigin), (x1-xorigin));
         double a2 = atan2((y2-yorigin), (x2-xorigin));

         cairo_arc(cr, xorigin, yorigin, r, a1, a2);
      }
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
*       text: Text to be displayed.
  */
void dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, const char *text)
{
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
   PangoFontDescription *font;
   char *tmpname, *fontname = "monospace 10";
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   if(!text)
      return;

   DW_MUTEX_LOCK;
   if(handle && _dw_render_safe_check(handle))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(handle), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
         if((tmpname = (char *)g_object_get_data(G_OBJECT(handle), "_dw_fontname")))
            fontname = tmpname;
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, handle) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, handle);
      }
   }
   else if(pixmap)
   {
      if(pixmap->font)
         fontname = pixmap->font;
      else if(pixmap->handle && (tmpname = (char *)g_object_get_data(G_OBJECT(pixmap->handle), "_dw_fontname")))
         fontname = tmpname;
      cr = cairo_create(pixmap->image);
   }
   if(cr)
   {
      font = pango_font_description_from_string(fontname);
      if(font)
      {
         PangoContext *context = pango_cairo_create_context(cr);

         if(context)
         {
            PangoLayout *layout = pango_layout_new(context);

            if(layout)
            {
               GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);
               GdkRGBA *background = pthread_getspecific(_dw_bg_color_key);

               pango_layout_set_font_description(layout, font);
               pango_layout_set_text(layout, text, strlen(text));

               gdk_cairo_set_source_rgba(cr, foreground);
               /* Create a background color attribute if required */
               if(background)
               {
                  PangoAttrList *list = pango_layout_get_attributes(layout);
                  PangoAttribute *attr = pango_attr_background_new((guint16)(background->red * 65535),
                                                                   (guint16)(background->green * 65535),
                                                                   (guint16)(background->blue* 65535));
                  if(!list)
                  {
                     list = pango_attr_list_new();
                  }
                  pango_attr_list_change(list, attr);
                  pango_layout_set_attributes(layout, list);
               }
               /* Do the drawing */
               cairo_move_to(cr, x, y);
               pango_cairo_show_layout (cr, layout);

               g_object_unref(layout);
            }
            g_object_unref(context);
         }
         pango_font_description_free(font);
      }
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(handle), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
   }
   DW_MUTEX_UNLOCK;
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height Pointer to a variable to be filled in with the height.
 */
void dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
{
   int _dw_locked_by_me = FALSE;
   PangoFontDescription *font;
   char *fontname = NULL;
   int free_fontname = 0;

   if(!text)
      return;

   DW_MUTEX_LOCK;
   if(handle)
   {
      fontname = (char *)g_object_get_data(G_OBJECT(handle), "_dw_fontname");
      if ( fontname == NULL )
      {
         fontname = dw_window_get_font(handle);
         free_fontname = 1;
      }
   }
   else if(pixmap)
   {
      if(pixmap->font)
         fontname = pixmap->font;
      else if(pixmap->handle)
         fontname = (char *)g_object_get_data(G_OBJECT(pixmap->handle), "_dw_fontname");
   }

   font = pango_font_description_from_string(fontname ? fontname : "monospace 10");
   if(font)
   {
      PangoContext *context = gdk_pango_context_get();

      if(context)
      {
         PangoLayout *layout = pango_layout_new(context);

         if(layout)
         {
            PangoRectangle rect;

            pango_layout_set_font_description(layout, font);
            pango_layout_set_text(layout, text, -1);
            pango_layout_get_pixel_extents(layout, NULL, &rect);

            if(width)
               *width = rect.width;
            if(height)
               *height = rect.height;

            g_object_unref(layout);
         }
         g_object_unref(context);
      }
      pango_font_description_free(font);
   }
   if ( free_fontname )
      free( fontname );
   DW_MUTEX_UNLOCK;
}

/*
 * Creates a pixmap with given parameters.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *               or zero to enable this pixmap to be written
 *               off the screen to reduce flicker
 *       width: Width of the pixmap in pixels.
 *       height: Height of the pixmap in pixels.
 *       depth: Color depth of the pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP dw_pixmap_new(HWND handle, unsigned long width, unsigned long height, int depth)
{
   int _dw_locked_by_me = FALSE;
   HPIXMAP pixmap;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   if (!depth)
      depth = -1;

   pixmap->width = width; pixmap->height = height;


   DW_MUTEX_LOCK;
   pixmap->handle = handle;
   /* Depth needs to be divided by 3... but for the RGB colorspace...
    * only 8 bits per sample is allowed, so to avoid issues just pass 8 for now.
    */
   pixmap->pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, width, height );
   pixmap->image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
   DW_MUTEX_UNLOCK;
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
HPIXMAP dw_pixmap_new_from_file(HWND handle, const char *filename)
{
   int _dw_locked_by_me = FALSE;
   HPIXMAP pixmap;
   char *file = alloca(strlen(filename) + 6);
   int found_ext = 0;
   int i;

   if (!file || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   strcpy(file, filename);

   /* check if we can read from this file (it exists and read permission) */
   if(access(file, 04) != 0)
   {
      /* Try with various extentions */
      for ( i = 0; i < NUM_EXTS; i++ )
      {
         strcpy( file, filename );
         strcat( file, _dw_image_exts[i] );
         if ( access( file, 04 ) == 0 )
         {
            found_ext = 1;
            break;
         }
      }
      if ( found_ext == 0 )
      {
         free(pixmap);
         return NULL;
      }
   }

   DW_MUTEX_LOCK;
   pixmap->pixbuf = gdk_pixbuf_new_from_file(file, NULL);
   pixmap->image = cairo_image_surface_create_from_png(file);
   pixmap->width = gdk_pixbuf_get_width(pixmap->pixbuf);
   pixmap->height = gdk_pixbuf_get_height(pixmap->pixbuf);
   pixmap->handle = handle;
   DW_MUTEX_UNLOCK;
   return pixmap;
}

/*
 * Creates a pixmap from data
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of image data
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP dw_pixmap_new_from_data(HWND handle, const char *data, int len)
{
   int fd, written = -1, _dw_locked_by_me = FALSE;
   HPIXMAP pixmap;
   char template[] = "/tmp/dwpixmapXXXXXX";

   if(!data || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   DW_MUTEX_LOCK;
   /*
    * A real hack; create a temporary file and write the contents
    * of the data to the file
    */
   if((fd = mkstemp(template)) != -1)
   {
      written = write(fd, data, len);
      close(fd);
   }
   /* Bail if we couldn't write full file */
   if(fd == -1 || written != len)
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   pixmap->pixbuf = gdk_pixbuf_new_from_file(template, NULL);
   pixmap->image = cairo_image_surface_create_from_png(template);
   pixmap->width = gdk_pixbuf_get_width(pixmap->pixbuf);
   pixmap->height = gdk_pixbuf_get_height(pixmap->pixbuf);
   /* remove our temporary file */
   unlink(template);
   pixmap->handle = handle;
   DW_MUTEX_UNLOCK;
   return pixmap;
}

/*
 * Sets the transparent color for a pixmap
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  transparent color
 * Note: This does nothing on GTK+ as transparency
 *       is handled automatically
 */
void dw_pixmap_set_transparent_color(HPIXMAP pixmap, unsigned long color)
{
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP dw_pixmap_grab(HWND handle, ULONG id)
{
   HPIXMAP pixmap;
   int _dw_locked_by_me = FALSE;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;


   DW_MUTEX_LOCK;
   pixmap->pixbuf = gdk_pixbuf_copy(_dw_find_pixbuf((HICN)id, &pixmap->width, &pixmap->height));
   DW_MUTEX_UNLOCK;
   return pixmap;
}

static void _dw_flush_dirty(gpointer widget, gpointer data)
{
   if(widget && GTK_IS_WIDGET(widget))
      gtk_widget_queue_draw(GTK_WIDGET(widget));
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void dw_flush(void)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   g_list_foreach(_dw_dirty_list, _dw_flush_dirty, NULL);
   g_list_free(_dw_dirty_list);
   _dw_dirty_list = NULL;
   DW_MUTEX_UNLOCK;
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
         char *oldfont = pixmap->font;

         pixmap->font = _dw_convert_font(fontname);

         if(oldfont)
             free(oldfont);
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
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   g_object_unref(G_OBJECT(pixmap->pixbuf));
   cairo_surface_destroy(pixmap->image);
   if(pixmap->font)
      free(pixmap->font);
   free(pixmap);
   DW_MUTEX_UNLOCK;
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
   int _dw_locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int cached = FALSE;
   int retval = DW_ERROR_GENERAL;
#if GTK_CHECK_VERSION(3,22,0)
   GdkDrawingContext *dc = NULL;
   cairo_region_t *clip = NULL;
#endif

   if((!dest && (!destp || !destp->image)) || (!src && (!srcp || !srcp->image)))
      return retval;

   DW_MUTEX_LOCK;
   if(dest && _dw_render_safe_check(dest))
   {
      GdkDisplay *display = gdk_display_get_default();
      
      if((cr = g_object_get_data(G_OBJECT(dest), "_dw_cr")))
         cached = TRUE;
      else if((display && GDK_IS_X11_DISPLAY(display)))
      {
         GdkWindow *window = gtk_widget_get_window(dest);
         /* Safety check for non-existant windows */
         if(!window || !GDK_IS_WINDOW(window))
         {
            DW_MUTEX_UNLOCK;
            return retval;
         }
#if GTK_CHECK_VERSION(3,22,0)
         clip = gdk_window_get_clip_region(window);
         dc = gdk_window_begin_draw_frame(window, clip);
         cr = gdk_drawing_context_get_cairo_context(dc);
#else
         cr = gdk_cairo_create(window);
#endif
      }
      else
      {
         /* If not X11, just mark the widget dirty.. and trigger draw in dw_flush() */
         if(g_list_find(_dw_dirty_list, dest) == NULL)
            _dw_dirty_list = g_list_append(_dw_dirty_list, dest);
      }
   }
   else if(destp)
      cr = cairo_create(destp->image);

   if(cr)
   {
      double xscale = 1, yscale = 1;

      if(srcwidth != -1 && srcheight != -1)
      {
         xscale = (double)width / (double)srcwidth;
         yscale = (double)height / (double)srcheight;
         cairo_scale(cr, xscale, yscale);
      }

      if(src)
         gdk_cairo_set_source_window (cr, gtk_widget_get_window(src), (xdest + xsrc) / xscale, (ydest + ysrc) / yscale);
      else if(srcp)
         cairo_set_source_surface (cr, srcp->image, (xdest + xsrc) / xscale, (ydest + ysrc) / yscale);

      cairo_rectangle(cr, xdest / xscale, ydest / yscale, width, height);
      cairo_fill(cr);
#if GTK_CHECK_VERSION(3,22,0)
      if(clip)
         cairo_region_destroy(clip);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(dc)
         gdk_window_end_draw_frame(gtk_widget_get_window(dest), dc);
      else
#endif
      if(!cached)
         cairo_destroy(cr);
      retval = DW_ERROR_NONE;
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void dw_beep(int freq, int dur)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gdk_display_beep(gdk_display_get_default());
   DW_MUTEX_UNLOCK;
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
int dw_module_load(const char *name, HMOD *handle)
{
   int len;
   char *newname;
   char errorbuf[1025] = {0};


   if(!handle)
      return -1;

   if((len = strlen(name)) == 0)
      return   -1;

   /* Lenth + "lib" + ".so" + NULL */
   newname = malloc(len + 7);

   if(!newname)
      return -1;

   sprintf(newname, "lib%s.so", name);
   _my_strlwr(newname);

   *handle = dlopen(newname, RTLD_NOW);
   if(*handle == NULL)
   {
      strncpy(errorbuf, dlerror(), 1024);
      printf("%s\n", errorbuf);
      sprintf(newname, "lib%s.so", name);
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
int dw_module_symbol(HMOD handle, const char *name, void**func)
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
   /* If we are being called from an event handler we must release
    * the GTK mutex so we don't deadlock.
    */
   if(pthread_self() == _dw_thread)
      _dw_gdk_threads_leave();

   pthread_mutex_lock(mutex);

   /* And of course relock it when we have acquired the mutext */
   if(pthread_self() == _dw_thread)
      _dw_gdk_threads_enter();
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
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   eve->posted = 1;
   pthread_mutex_unlock (&(eve->mutex));
   return DW_ERROR_NONE;
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
      int DW_UNUSED(result);

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
            if((bytesread = read(array[z].fd, &command, 1)) < 1)
            {
               struct _dw_seminfo *newarray;

               /* Remove this connection from the set */
               newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount-1));
               if(!z)
                  memcpy(newarray, &array[1], sizeof(struct _dw_seminfo)*(connectcount-1));
               else
               {
                  memcpy(newarray, array, sizeof(struct _dw_seminfo)*z);
                  if(z!=(connectcount-1))
                     memcpy(&newarray[z], &array[z+1], sizeof(struct _dw_seminfo)*(connectcount-(z+1)));
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
                           result = write(array[s].fd, &tmp, 1);
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
                        result = write(array[z].fd, &tmp, 1);
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
HEV dw_named_event_new(const char *name)
{
   struct sockaddr_un un;
   int ev, *tmpsock = (int *)malloc(sizeof(int)*2);
   DWTID dwthread;

   if(!tmpsock)
      return NULL;

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
      return NULL;
   }

   /* Create a thread to handle this event semaphore */
   pthread_create(&dwthread, NULL, (void *)_dw_handle_sem, (void *)tmpsock);
   return GINT_TO_POINTER(ev);
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV dw_named_event_get(const char *name)
{
   struct sockaddr_un un;
   int ev = socket(AF_UNIX, SOCK_STREAM, 0);
   if(ev < 0)
      return NULL;

   un.sun_family=AF_UNIX;
   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   strcpy(un.sun_path, "/tmp/.dw/");
   strcat(un.sun_path, name);
   connect(ev, (struct sockaddr *)&un, sizeof(un));
   return GINT_TO_POINTER(ev);
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

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NONE;

   if(write(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NONE;

   if(write(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
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
int dw_named_event_wait(HEV eve, unsigned long timeout)
{
   fd_set rd;
   struct timeval tv, *useme = NULL;
   int retval = 0;
   char tmp;

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NON_INIT;

   /* Set the timout or infinite */
   if(timeout != -1)
   {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;

      useme = &tv;
   }

   FD_ZERO(&rd);
   FD_SET(GPOINTER_TO_INT(eve), &rd);

   /* Signal wait */
   tmp = (char)2;
   retval = write(GPOINTER_TO_INT(eve), &tmp, 1);

   if(retval == 1)
      retval = select(GPOINTER_TO_INT(eve)+1, &rd, NULL, NULL, useme);

   /* Signal done waiting. */
   tmp = (char)3;
   if(retval == 1)
      retval = write(GPOINTER_TO_INT(eve), &tmp, 1);

   if(retval == 0)
      return DW_ERROR_TIMEOUT;
   else if(retval == -1)
      return DW_ERROR_INTERRUPT;

   /* Clear the entry from the pipe so
    * we don't loop endlessly. :)
    */
   if(read(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
	return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
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
    * cleanup will continue in _dw_handle_sem.
    */
   close(GPOINTER_TO_INT(eve));
   return DW_ERROR_NONE;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
   GdkRGBA *foreground = malloc(sizeof(GdkRGBA));

   foreground->alpha = foreground->red = foreground->green = foreground->blue = 0.0;
   pthread_setspecific(_dw_fg_color_key, foreground);
   pthread_setspecific(_dw_bg_color_key, NULL);
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
   GdkRGBA *foreground, *background;

   if((foreground = pthread_getspecific(_dw_fg_color_key)))
      free(foreground);
   if((background = pthread_getspecific(_dw_bg_color_key)))
      free(background);
}

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
   void (*threadfunc)(void *) = NULL;
   void **tmp = (void **)data;

   threadfunc = (void (*)(void *))tmp[0];

   /* Initialize colors */
   _dw_init_thread();

   threadfunc(tmp[1]);
   free(tmp);

   /* Free colors */
   _dw_deinit_thread();
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         handle: A pointer to receive a SHM identifier.
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 */
HSHM dw_named_memory_new(void **dest, int size, const char *name)
{
   char namebuf[1025];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
   {
      free(handle);
      return NULL;
   }

   if(ftruncate(handle->fd, size))
   {
       close(handle->fd);
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
HSHM dw_named_memory_get(void **dest, int size, const char *name)
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
   DWTID gtkthread;
   void **tmp = malloc(sizeof(void *) * 2);
   int rc;

   tmp[0] = func;
   tmp[1] = data;

   rc = pthread_create(&gtkthread, NULL, (void *)_dwthreadstart, (void *)tmp);
   if ( rc == 0 )
      return gtkthread;
   return (DWTID)DW_ERROR_UNKNOWN;
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
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void dw_shutdown(void)
{
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void dw_exit(int exitcode)
{
   exit(exitcode);
}

/* Internal function to get the recommended size of scrolled items */
void _dw_get_scrolled_size(GtkWidget *item, gint *thiswidth, gint *thisheight)
{
   GtkWidget *widget = g_object_get_data(G_OBJECT(item), "_dw_user");

   *thisheight = *thiswidth = 0;

   if(widget)
   {
      if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
      {
         /* Set to half for tree */
         *thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
         *thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
      }
      else if(GTK_IS_TEXT_VIEW(widget))
      {
         unsigned long bytes;
         int height, width;
         char *buf, *ptr;
         int wrap = (gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(widget)) == GTK_WRAP_WORD);
         static char testtext[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
         int hscrolled = FALSE;

         *thiswidth = *thisheight = 0;

         dw_mle_get_size(item, &bytes, NULL);

         ptr = buf = alloca(bytes + 2);
         dw_mle_export(item, buf, 0, (int)bytes);
         buf[bytes] = 0;
         strcat(buf, "\r");

         /* MLE */
         while((ptr = strstr(buf, "\r")))
         {
            ptr[0] = 0;
            width = 0;
            if(strlen(buf))
               dw_font_text_extents_get(item, NULL, buf, &width, &height);
            else
               dw_font_text_extents_get(item, NULL, testtext, NULL, &height);

            if(wrap && width > _DW_SCROLLED_MAX_WIDTH)
            {
               *thiswidth = _DW_SCROLLED_MAX_WIDTH;
               *thisheight += height * (width / _DW_SCROLLED_MAX_WIDTH);
            }
            else
            {
               if(width > *thiswidth)
               {
                  if(width > _DW_SCROLLED_MAX_WIDTH)
                  {
                     *thiswidth = _DW_SCROLLED_MAX_WIDTH;
                     hscrolled = TRUE;
                  }
                  else
                     *thiswidth = width;
               }
            }
            *thisheight += height;
            if(ptr[1] == '\n')
               buf = &ptr[2];
            else
               buf = &ptr[1];
         }
         if(hscrolled)
            *thisheight += 10;
      }
      else
      {
         gtk_widget_get_preferred_height(GTK_WIDGET(widget), NULL, thisheight);
         gtk_widget_get_preferred_width(GTK_WIDGET(widget), NULL, thiswidth);

         *thisheight += 20;
         *thiswidth += 20;
      }
   }
   if(*thiswidth < _DW_SCROLLED_MIN_WIDTH)
      *thiswidth = _DW_SCROLLED_MIN_WIDTH;
   if(*thiswidth > _DW_SCROLLED_MAX_WIDTH)
      *thiswidth = _DW_SCROLLED_MAX_WIDTH;
   if(*thisheight < _DW_SCROLLED_MIN_HEIGHT)
      *thisheight = _DW_SCROLLED_MIN_HEIGHT;
   if(*thisheight > _DW_SCROLLED_MAX_HEIGHT)
      *thisheight = _DW_SCROLLED_MAX_HEIGHT;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
   int warn = FALSE, _dw_locked_by_me = FALSE;
   GtkWidget *tmp, *tmpitem, *image = NULL;

   if(!box)
      return;

      /*
       * If you try and pack an item into itself VERY bad things can happen; like at least an
       * infinite loop on GTK! Lets be safe!
       */
   if(box == item)
   {
      dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
      return;
   }

   DW_MUTEX_LOCK;

   /* If this is a special box, like: Window, Groupbox, Scrollbox...
    * get the internal box handle.
    */
   if((tmp  = g_object_get_data(G_OBJECT(box), "_dw_boxhandle")))
      box = tmp;

   /* Can't pack nothing with GTK, so create an empty label */
   if(!item)
   {
      item = gtk_label_new("");
      g_object_set_data(G_OBJECT(item), "_dw_padding", GINT_TO_POINTER(1));
      gtk_widget_show_all(item);
   }
   /* Due to GTK3 minimum size limitations, if we are packing a widget
    * with an image, we need to scale the image down to fit the packed size.
    */
   else if((image = g_object_get_data(G_OBJECT(item), "_dw_bitmap")))
   {
      GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));

      if(pixbuf)
      {
         int pwidth = gdk_pixbuf_get_width(pixbuf);
         int pheight = gdk_pixbuf_get_height(pixbuf);

         if(width == -1)
            width = pwidth;
         if(height == -1)
            height = pheight;

         if(pwidth > width || pheight > height)
            pixbuf = gdk_pixbuf_scale_simple(pixbuf, pwidth > width ? width : pwidth, pheight > height ? height : pheight, GDK_INTERP_BILINEAR);
         gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
      }
   }

   /* Check if the item to be packed is a special box */
   tmpitem = (GtkWidget *)g_object_get_data(G_OBJECT(item), "_dw_boxhandle");

   /* Make sure our target box is valid */
   if(GTK_IS_GRID(box))
   {
      int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
      int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));

      /* If the item being packed is a box, then we use it's padding
       * instead of the padding specified on the pack line, this is
       * due to a bug in the OS/2 and Win32 renderer and a limitation
       * of the GtkTable class.
       */
      if(GTK_IS_GRID(item) || (tmpitem && GTK_IS_GRID(tmpitem)))
      {
         GtkWidget *eventbox = (GtkWidget *)g_object_get_data(G_OBJECT(item), "_dw_eventbox");

         /* NOTE: I left in the ability to pack boxes with a size,
          *       this eliminates that by forcing the size to 0.
          */
         height = width = 0;

         if(eventbox)
         {
            int boxpad = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "_dw_boxpad"));
            gtk_container_add(GTK_CONTAINER(eventbox), item);
            gtk_container_set_border_width(GTK_CONTAINER(eventbox), boxpad);
            item = eventbox;
         }
      }
      else
      {
         /* Only show warning if item is not a box */
         warn = TRUE;
      }

      /* Do some sanity bounds checking */
      if(index < 0)
         index = 0;
      if(index > boxcount)
         index = boxcount;

#if !GTK_CHECK_VERSION(3,10,0)
      /* Fix the index by taking into account empty cells */
      if(boxtype == DW_VERT)
      {
         int z;
      	
         for(z=0;z<index;z++)
         {
            if(!gtk_grid_get_child_at(GTK_GRID(box), 0, z))
               index++;
         }
      }
      else
      {
         int z;
      	
         for(z=0;z<index;z++)
         {
            if(!gtk_grid_get_child_at(GTK_GRID(box), z, 0))
               index++;
         }
      }
#endif

      g_object_set_data(G_OBJECT(item), "_dw_table", box);
      /* Set the expand attribute on the widgets now instead of the container */
      gtk_widget_set_vexpand(item, vsize);
      gtk_widget_set_valign(item, vsize ? GTK_ALIGN_FILL : GTK_ALIGN_START);
      gtk_widget_set_hexpand(item, hsize);
      gtk_widget_set_halign(item, hsize ? GTK_ALIGN_FILL : GTK_ALIGN_START);
      /* Use the margin property as padding */
      g_object_set(G_OBJECT(item), "margin", pad, NULL);
      /* Add to the grid using insert...
       * rows for vertical boxes and columns for horizontal.
       */
      if(boxtype == DW_VERT)
      {
         gtk_grid_insert_row(GTK_GRID(box), index);
         gtk_grid_attach(GTK_GRID(box), item, 0, index, 1, 1);
      }
      else
      {
         gtk_grid_insert_column(GTK_GRID(box), index);
         gtk_grid_attach(GTK_GRID(box), item, index, 0, 1, 1);
      }
      g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount + 1));
      /* Special case for scrolled windows */
      if(GTK_IS_SCROLLED_WINDOW(item))
      {
         gint scrolledwidth = 0, scrolledheight = 0;

         /* Pre-run the calculation code for MLE/Container/Tree if needed */
         if((width < 1 && !hsize) || (height < 1 && !vsize))
            _dw_get_scrolled_size(item, &scrolledwidth, &scrolledheight);

         if(width > 0)
            gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(item), width);
         else if(!hsize)
            gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(item), scrolledwidth);
         if(height > 0)
            gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(item), height);
         else if(!vsize)
            gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(item), scrolledheight);
      }
      else
      {
         /* Set the requested size of the widget */
         if(width == -1 && (GTK_IS_COMBO_BOX(item) || GTK_IS_ENTRY(item)))
            gtk_widget_set_size_request(item, 150, height);
         else if(width == -1 && GTK_IS_SPIN_BUTTON(item))
            gtk_widget_set_size_request(item, 50, height);
         else      	
            gtk_widget_set_size_request(item, width, height);
      }
      if(GTK_IS_RADIO_BUTTON(item))
      {
         GSList *group;
         GtkWidget *groupstart = (GtkWidget *)g_object_get_data(G_OBJECT(box), "_dw_group");

         if(groupstart)
         {
            group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(groupstart));
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(item), group);
         }
         else
            g_object_set_data(G_OBJECT(box), "_dw_group", (gpointer)item);
      }
      /* If we previously incremented the reference count... drop it now that it is packed */
      if(g_object_get_data(G_OBJECT(item), "_dw_refed"))
      {
         g_object_unref(G_OBJECT(item));
         g_object_set_data(G_OBJECT(item), "_dw_refed", NULL);
      }
   }
   DW_MUTEX_UNLOCK;

   if(warn)
   {
      if ( width == 0 && hsize == FALSE )
         dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
      if ( height == 0 && vsize == FALSE )
         dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Height and expand Vertical both unset for box: %x item: %x",box,item);
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
   int _dw_locked_by_me = FALSE, retcode = DW_ERROR_GENERAL;

   DW_MUTEX_LOCK;
   if(GTK_IS_WIDGET(handle))
   {
      GtkWidget *box, *handle2 = handle;
      GtkWidget *eventbox = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_eventbox");

      /* Handle the invisible event box if it exists */
      if(eventbox && GTK_IS_WIDGET(eventbox))
         handle2 = eventbox;

      /* Check if we are removing a widget from a box */	
      if((box = gtk_widget_get_parent(handle2)) && GTK_IS_GRID(box))
      {
         /* Get the number of items in the box... */
         int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
#if GTK_CHECK_VERSION(3,10,0)
         int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
#endif
			
         if(boxcount > 0)
         {
            /* Decrease the count by 1 */
            boxcount--;
            g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
         }
         /* If we haven't incremented the reference count... raise it before removal */
         if(!g_object_get_data(G_OBJECT(handle2), "_dw_refed"))
         {
            g_object_ref(G_OBJECT(handle2));
            g_object_set_data(G_OBJECT(handle2), "_dw_refed", GINT_TO_POINTER(1));
         }
         /* Remove the widget from the box */
#if GTK_CHECK_VERSION(3,10,0)
         /* Figure out where in the grid this widget is and remove that row/column */
         if(boxtype == DW_VERT)
         {
            int z;

            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), 0, z) == handle2)
               {
                  gtk_grid_remove_row(GTK_GRID(box), z);
                  break;
               }
            }
         }
         else
         {
            int z;

            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), z, 0) == handle2)
               {
                  gtk_grid_remove_column(GTK_GRID(box), z);
                  break;
               }
            }
         }
#else
         gtk_container_remove(GTK_CONTAINER(box), handle2);
#endif
         retcode = DW_ERROR_NONE;
      }
   }
   DW_MUTEX_UNLOCK;
   return retcode;
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
   int _dw_locked_by_me = FALSE;
   HWND retval = 0;

   DW_MUTEX_LOCK;
   /* Check if we are removing a widget from a box */	
   if(GTK_IS_GRID(box))
   {
      /* Get the number of items in the box... */
      int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
      int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
#if GTK_CHECK_VERSION(3,10,0)
      GtkWidget *item = (boxtype == DW_VERT) ? gtk_grid_get_child_at(GTK_GRID(box), 0, index) : gtk_grid_get_child_at(GTK_GRID(box), index, 0);
#else
      GtkWidget *item;
		
      /* Fix the index by taking into account empty cells */
      if(boxtype == DW_VERT)
      {
         int z;
      	
         for(z=0;z<index;z++)
         {
            if(!gtk_grid_get_child_at(GTK_GRID(box), 0, z))
               index++;
         }
         item = gtk_grid_get_child_at(GTK_GRID(box), 0, index);
      }
      else
      {
         int z;
      	
         for(z=0;z<index;z++)
         {
            if(!gtk_grid_get_child_at(GTK_GRID(box), z, 0))
               index++;
         }
         item = gtk_grid_get_child_at(GTK_GRID(box), index, 0);
      }
#endif

      if(boxcount > 0)
      {
         /* Decrease the count by 1 */
         boxcount--;
         g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
      }
      /* If we haven't incremented the reference count... raise it before removal */
      if(item && g_object_get_data(G_OBJECT(item), "_dw_padding"))
         gtk_widget_destroy(item);
      else if(item)
      {
         if(!g_object_get_data(G_OBJECT(item), "_dw_refed"))
         {
            g_object_ref(G_OBJECT(item));
            g_object_set_data(G_OBJECT(item), "_dw_refed", GINT_TO_POINTER(1));
         }
         /* Remove the widget from the box */
         gtk_container_remove(GTK_CONTAINER(box), item);
#if GTK_CHECK_VERSION(3,10,0)
         if(boxtype == DW_VERT)
            gtk_grid_remove_row(GTK_GRID(box), index);
         else
            gtk_grid_remove_column(GTK_GRID(box), index);
#endif
         retval = item;
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
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


union _dw_extents_union { guchar **gu_extents; unsigned long **extents; };
static GdkAtom _dw_extents_atom = 0;
static time_t _dw_extents_time = 0;

static gboolean _dw_property_notify(GtkWidget *window, GdkEventProperty* event, GdkWindow *gdkwindow)
{
   /* Check to see if we got a property change */
   if(event->state == GDK_PROPERTY_NEW_VALUE && event->atom == _dw_extents_atom && event->window == gdkwindow)
      _dw_extents_time = 0;
   return FALSE;
}

/* Internal function to figure out the frame extents of an unmapped window */
void _dw_get_frame_extents(GtkWidget *window, int *vert, int *horz)
{
   if(gtk_window_get_decorated(GTK_WINDOW(window)))
   {
      const char *request = "_NET_REQUEST_FRAME_EXTENTS";
      unsigned long *extents = NULL;
      union _dw_extents_union eu;
      GdkAtom request_extents = gdk_atom_intern(request, FALSE);
      GdkWindow *gdkwindow = gtk_widget_get_window(window);
      GdkDisplay *display = gtk_widget_get_display(window);

      if(!_dw_extents_atom)
         _dw_extents_atom = gdk_atom_intern("_NET_FRAME_EXTENTS", FALSE);

      /* Set some rational defaults.. just in case */
      *vert = 28;
      *horz = 12;

      /* See if the current window manager supports _NET_REQUEST_FRAME_EXTENTS */
      if(display && GDK_IS_X11_DISPLAY(display) && gdk_x11_screen_supports_net_wm_hint(gdk_screen_get_default(), request_extents))
      {
         Display *xdisplay = GDK_DISPLAY_XDISPLAY(display);
         GdkWindow *root_window = gdk_get_default_root_window();
         Window xroot_window = GDK_WINDOW_XID(root_window);
         Atom extents_request_atom = gdk_x11_get_xatom_by_name_for_display(display, request);
         unsigned long window_id = GDK_WINDOW_XID(gdkwindow);
         XEvent xevent = {0};
         time_t currtime;
         gulong connid;

         xevent.xclient.type = ClientMessage;
         xevent.xclient.message_type = extents_request_atom;
         xevent.xclient.display = xdisplay;
         xevent.xclient.window = window_id;
         xevent.xclient.format = 32;

         /* Send the property request */
         XSendEvent(xdisplay, xroot_window, False,
                   (SubstructureRedirectMask | SubstructureNotifyMask),
                   &xevent);

         /* Connect a signal to look for the property change */
         connid = g_signal_connect(G_OBJECT(window), "property_notify_event", G_CALLBACK(_dw_property_notify), gdkwindow);

         /* Record the request time */
         time(&_dw_extents_time);

         /* Look for the property notify event */
         do
         {
            dw_main_iteration();
            time(&currtime);
         }
         while(currtime - _dw_extents_time < 2);

         /* Remove the signal handler now that we are done */
         g_signal_handler_disconnect(G_OBJECT(window), connid);
      }

      /* Attempt to retrieve window's frame extents. */
      eu.extents = &extents;
      if(gdk_property_get(gdkwindow,
                          _dw_extents_atom,
                          gdk_atom_intern("CARDINAL", FALSE),
                          0, sizeof(unsigned long)*4, FALSE,
                          NULL, NULL, NULL, eu.gu_extents))
      {
         *horz = extents[0] + extents[1];
         *vert = extents[2] + extents[3];
      }
   }
   else
      *horz = *vert = 0;
}

/*
 * Sets the size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void dw_window_set_size(HWND handle, unsigned long width, unsigned long height)
{
   int _dw_locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if(GTK_IS_WINDOW(handle))
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      int cx = 0, cy = 0;

      /* Window is mapped query the frame size directly */
      if(window && gtk_widget_get_mapped(handle))
      {
         GdkRectangle frame;
         gint gwidth, gheight;

         /* Calculate the border size */
         gdk_window_get_frame_extents(window, &frame);
         gdk_window_get_geometry(window, NULL, NULL, &gwidth, &gheight);

         cx = frame.width - gwidth;
         if(cx < 0)
            cx = 0;
         cy = frame.height - gheight;
         if(cy < 0)
            cy = 0;
      }
      else
      {
         /* Check if we have cached frame size values */
         if(!((cx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_frame_width"))) |
              (cy = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_frame_height")))))
         {
            /* If not try to ask the window manager for the estimated size...
             * and finally if all else fails, guess.
             */
            _dw_get_frame_extents(handle, &cy, &cx);
            /* Cache values for later use */
            g_object_set_data(G_OBJECT(handle), "_dw_frame_width", GINT_TO_POINTER(cx));
            g_object_set_data(G_OBJECT(handle), "_dw_frame_height", GINT_TO_POINTER(cy));
         }
         /* Save the size for when it is shown */
         g_object_set_data(G_OBJECT(handle), "_dw_width", GINT_TO_POINTER(width));
         g_object_set_data(G_OBJECT(handle), "_dw_height", GINT_TO_POINTER(height));
      }
      /* Resize minus the border size */
      if(width > cx && height > cy)
      {
         gtk_window_resize(GTK_WINDOW(handle), width - cx , height - cy );
         gtk_window_set_default_size(GTK_WINDOW(handle), width - cx, height - cy);
      }
   }
   else
      gtk_widget_set_size_request(handle, width, height);
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
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      gint scrolledwidth, scrolledheight;

      _dw_get_scrolled_size(handle, &scrolledwidth, &scrolledheight);

      if(width)
         *width = scrolledwidth;
      if(height)
         *height = scrolledheight;
   }
   else
   {
      if(width)
         gtk_widget_get_preferred_width(handle, NULL, width);
      if(height)
         gtk_widget_get_preferred_height(handle, NULL, height);
   }
   DW_MUTEX_UNLOCK;
}

/* Internal version to simplify the code with multiple versions of GTK */
int _dw_screen_width(void)
{
#if GTK_CHECK_VERSION(3,22,0)
   {
      GdkDisplay *display = gdk_display_get_default();

      if(display)
      {
         GdkMonitor *monitor = gdk_display_get_primary_monitor(display);

         if(monitor)
         {
            GdkRectangle rc = { 0, 0, 0 ,0 };
            gdk_monitor_get_geometry(monitor, &rc);
            return rc.width;
         }
      }
   }
   return 0;
#else
   return gdk_screen_width();
#endif
}

/*
 * Returns the width of the screen.
 */
int dw_screen_width(void)
{
   int retval = 0;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   retval = _dw_screen_width();
   DW_MUTEX_UNLOCK;
   return retval;
}

/* Internal version to simplify the code with multiple versions of GTK */
int _dw_screen_height(void)
{
#if GTK_CHECK_VERSION(3,22,0)
   {
      GdkDisplay *display = gdk_display_get_default();

      if(display)
      {
         GdkMonitor *monitor = gdk_display_get_primary_monitor(display);

         if(monitor)
         {
            GdkRectangle rc = { 0, 0, 0 ,0 };
            gdk_monitor_get_geometry(monitor, &rc);
            return rc.height;
         }
      }
   }
   return 0;
#else
   return gdk_screen_height();
#endif
}

/*
 * Returns the height of the screen.
 */
int dw_screen_height(void)
{
   int retval;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   retval = _dw_screen_height();
   DW_MUTEX_UNLOCK;
   return retval;
}

/* This should return the current color depth */
unsigned long dw_color_depth_get(void)
{
   int retval;
   GdkVisual *vis;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
#if GTK_CHECK_VERSION(3,22,0)
   vis = gdk_screen_get_system_visual(gdk_screen_get_default());
#else
   vis = gdk_visual_get_system();
#endif
   retval = gdk_visual_get_depth(vis);
   DW_MUTEX_UNLOCK;
   return retval;
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
void dw_window_set_pos(HWND handle, long x, long y)
{
   int _dw_locked_by_me = FALSE;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi;
#endif

   if(!handle)
      return;

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_move(GTK_MDI(mdi), handle, x, y);
   }
   else
#endif
   {
      GdkWindow *window = NULL;

      if(GTK_IS_WINDOW(handle))
      {
         GdkWindow *window = gtk_widget_get_window(handle);
         int horz = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_grav_horz"));
         int vert = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_grav_vert"));
         int cx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_frame_width"));
         int cy = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_frame_height"));
         int newx = x, newy = y, width = 0, height = 0;

         /* If the window is mapped */
         if(window && gtk_widget_get_mapped(handle))
         {
            /* If we need the width or height... */
            if(horz || vert)
            {
               GdkRectangle frame;
               int count = 0;

               /* Get the frame size */
               gdk_window_get_frame_extents(window, &frame);

               /* FIXME: Sometimes we get returned an invalid 200x200
                * result... so if we get this... try the call a second
                * time and hope for a better result.
                */
               while((frame.width == 200 || frame.width == (200 + cx)) &&
                     (frame.height == 200 || frame.height == (200 + cy)) && count < 10)
               {
                  _dw_msleep(1);
                  count++;
                  gdk_window_get_frame_extents(window, &frame);
               }
               width = frame.width;
               height = frame.height;
            }
         }
         else
         {
            /* Check if we have cached frame size values */
            if(!(cx | cy))
            {
               /* If not try to ask the window manager for the estimated size...
                * and finally if all else fails, guess.
                */
               _dw_get_frame_extents(handle, &cy, &cx);
               /* Cache values for later use */
               g_object_set_data(G_OBJECT(handle), "_dw_frame_width", GINT_TO_POINTER(cx));
               g_object_set_data(G_OBJECT(handle), "_dw_frame_height", GINT_TO_POINTER(cy));
            }
            /* Save the positions for when it is shown */
            g_object_set_data(G_OBJECT(handle), "_dw_x", GINT_TO_POINTER(x));
            g_object_set_data(G_OBJECT(handle), "_dw_y", GINT_TO_POINTER(y));
            g_object_set_data(G_OBJECT(handle), "_dw_pos", GINT_TO_POINTER(1));
            /* Check to see if there is a pending size request too */
            width = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_width"));
            height = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_height"));
            if(!width || !height)
            {
               /* Ask what GTK is planning on suggesting for the window size */
               gtk_window_get_size(GTK_WINDOW(handle), !width ? &width : NULL, !height ? &height : NULL);
            }
            /* Add the frame size to it */
            width += cx;
            height += cy;
         }
         /* Do any gravity calculations */
         if(horz || vert)
         {
            /* Handle horizontal center gravity */
            if((horz & 0xf) == DW_GRAV_CENTER)
               newx += ((_dw_screen_width() / 2) - (width / 2));
            /* Handle right gravity */
            else if((horz & 0xf) == DW_GRAV_RIGHT)
               newx = _dw_screen_width() - width - x;
            /* Handle vertical center gravity */
            if((vert & 0xf) == DW_GRAV_CENTER)
               newy += ((_dw_screen_height() / 2) - (height / 2));
            else if((vert & 0xf) == DW_GRAV_BOTTOM)
               newy = _dw_screen_height() - height - y;

#if GTK_CHECK_VERSION(3,3,8)
            /* Adjust the values to avoid Gnome bar if requested */
            if((horz | vert) & DW_GRAV_OBSTACLES)
            {
               GdkRectangle rect = { 0, 0, 0, 0 };
#if GTK_CHECK_VERSION(3,22,0)
               GdkDisplay *display = gdk_display_get_default();

               if(display)
               {
                  GdkMonitor *monitor = gdk_display_get_primary_monitor(display);

                  if(monitor)
                     gdk_monitor_get_workarea(monitor, &rect);
               }
#else
               GdkScreen *screen = gdk_screen_get_default();

               gdk_screen_get_monitor_workarea(screen, 0, &rect);
#endif
               if(horz & DW_GRAV_OBSTACLES)
               {
                  if((horz & 0xf) == DW_GRAV_LEFT)
                     newx += rect.x;
                  else if((horz & 0xf) == DW_GRAV_RIGHT)
                     newx -= _dw_screen_width() - (rect.x + rect.width);
               }
               if(vert & DW_GRAV_OBSTACLES)
               {
                  if((vert & 0xf) == DW_GRAV_TOP)
                     newy += rect.y;
                  else if((vert & 0xf) == DW_GRAV_BOTTOM)
                     newy -= _dw_screen_height() - (rect.y + rect.height);
               }
            }
#endif
         }
         /* Finally move the window into place */
         gtk_window_move(GTK_WINDOW(handle), newx, newy);
      }
      else if((window = gtk_widget_get_window(handle)))
         gdk_window_move(window, x, y);
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
void dw_window_set_pos_size(HWND handle, long x, long y, unsigned long width, unsigned long height)
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
void dw_window_get_pos_size(HWND handle, long *x, long *y, ULONG *width, ULONG *height)
{
   int _dw_locked_by_me = FALSE;
   gint gx = 0, gy = 0, gwidth = 0, gheight = 0;
#ifdef DW_INCLUDE_DEPRECATED
   GtkWidget *mdi;
#endif

   DW_MUTEX_LOCK;
#ifdef DW_INCLUDE_DEPRECATED
   /* If it is an MDI window query what we can */
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_get_pos(GTK_MDI(mdi), handle, &gx, &gy);
   }
   else
#endif
   {
      GdkWindow *window;

      /* Can only query if the window is realized */
      if(handle && (window = gtk_widget_get_window(handle)))
      {
         /* If it is a toplevel window */
         if(GTK_IS_WINDOW(handle))
         {
            if(gtk_widget_get_mapped(handle))
            {
               GdkRectangle frame;

               /* Calculate the border rectangle */
               gdk_window_get_frame_extents(window, &frame);
               gx = frame.x;
               gy = frame.y;
               gwidth = frame.width;
               gheight = frame.height;
            }
         }
         else
         {
            /* Get an individual widget coordinates */
            gdk_window_get_geometry(window, &gx, &gy, &gwidth, &gheight);
            gdk_window_get_root_origin(window, &gx, &gy);
         }
      }
   }
   /* Fill in the requested fields */
   if(x)
      *x = gx;
   if(y)
      *y = gy;
   if(width)
      *width = gwidth;
   if(height)
      *height = gheight;
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void dw_window_set_style(HWND handle, unsigned long style, unsigned long mask)
{
   GtkWidget *handle2 = handle;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_label");
      if(tmp && GTK_IS_LABEL(tmp))
         handle2 = tmp;
   }
   else if(GTK_IS_BUTTON(handle))
   {
        if(mask & DW_BS_NOBORDER)
        {
            if(style & DW_BS_NOBORDER)
            {
                gtk_button_set_relief((GtkButton *)handle, GTK_RELIEF_NONE);
            }
            else
            {
                gtk_button_set_relief((GtkButton *)handle, GTK_RELIEF_NORMAL);
            }
        }
   }
   if ( GTK_IS_LABEL(handle2) )
   {
      gfloat x=DW_LEFT, y=DW_CENTER;
      /* horizontal... */
      if ( style & DW_DT_CENTER )
         x = DW_CENTER;
      if ( style & DW_DT_RIGHT )
         x = DW_RIGHT;
      if ( style & DW_DT_LEFT )
         x = DW_LEFT;
      /* vertical... */
      if ( style & DW_DT_VCENTER )
         y = DW_CENTER;
      if ( style & DW_DT_TOP )
         y = DW_TOP;
      if ( style & DW_DT_BOTTOM )
         y = DW_BOTTOM;
#if GTK_CHECK_VERSION(3,16,0)
      gtk_label_set_xalign(GTK_LABEL(handle2), x);
      gtk_label_set_yalign(GTK_LABEL(handle2), y);
#else
      gtk_misc_set_alignment(GTK_MISC(handle2), x, y);
#endif	
      if ( style & DW_DT_WORDBREAK )
         gtk_label_set_line_wrap( GTK_LABEL(handle), TRUE );
   }
   if ( GTK_IS_CHECK_MENU_ITEM(handle2) && (mask & (DW_MIS_CHECKED | DW_MIS_UNCHECKED)) )
   {
      int check = 0;

      if ( style & DW_MIS_CHECKED )
         check = 1;

      _dw_ignore_click = 1;
      if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(handle2)) != check)
         gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(handle2), check);
      _dw_ignore_click = 0;
   }
   if ( (GTK_IS_CHECK_MENU_ITEM(handle2) || GTK_IS_MENU_ITEM(handle2)) && (mask & (DW_MIS_ENABLED | DW_MIS_DISABLED) ))
   {
      _dw_ignore_click = 1;
      if ( style & DW_MIS_ENABLED )
         gtk_widget_set_sensitive( handle2, TRUE );
      else
         gtk_widget_set_sensitive( handle2, FALSE );
      _dw_ignore_click = 0;
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 */
unsigned long dw_notebook_page_new(HWND handle, unsigned long flags, int front)
{
   int z;
   int _dw_locked_by_me = FALSE;
   GtkWidget **pagearray;

   DW_MUTEX_LOCK;
   pagearray = (GtkWidget **)g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(pagearray)
   {
      for(z=0;z<256;z++)
      {
         if(!pagearray[z])
         {
            char text[101] = {0};
            int num = z;

            if(front)
               num |= 1 << 16;

            snprintf(text, 100, "_dw_page%d", z);
            /* Save the real id and the creation flags */
            g_object_set_data(G_OBJECT(handle), text, GINT_TO_POINTER(num));
            DW_MUTEX_UNLOCK;
            return z;
         }
      }
   }
   DW_MUTEX_UNLOCK;

   /* Hopefully this won't happen. */
   return 256;
}

/* Return the physical page id from the logical page id */
int _dw_get_physical_page(HWND handle, unsigned long pageid)
{
   int z;
   GtkWidget *thispage, **pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(pagearray)
   {
      for(z=0;z<256;z++)
      {
         if((thispage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), z)))
         {
            if(thispage == pagearray[pageid])
               return z;
         }
      }
   }
   return 256;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void dw_notebook_page_destroy(HWND handle, unsigned long pageid)
{
   int realpage, _dw_locked_by_me = FALSE;
   GtkWidget **pagearray;

   DW_MUTEX_LOCK;
   realpage = _dw_get_physical_page(handle, pageid);
   if(realpage > -1 && realpage < 256)
   {
      gtk_notebook_remove_page(GTK_NOTEBOOK(handle), realpage);
      if((pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray")))
         pagearray[pageid] = NULL;
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long dw_notebook_page_get(HWND handle)
{
   int retval, phys;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   phys = gtk_notebook_get_current_page(GTK_NOTEBOOK(handle));
   retval = _dw_get_logical_page(handle, phys);
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void dw_notebook_page_set(HWND handle, unsigned long pageid)
{
   int realpage, _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   realpage = _dw_get_physical_page(handle, pageid);
   if(realpage > -1 && realpage < 256)
      gtk_notebook_set_current_page(GTK_NOTEBOOK(handle), pageid);
   DW_MUTEX_UNLOCK;
}


/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void dw_notebook_page_set_text(HWND handle, unsigned long pageid, const char *text)
{
   GtkWidget *child;
   int realpage, _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   realpage = _dw_get_physical_page(handle, pageid);
   if(realpage < 0 || realpage > 255)
   {
      char ptext[101] = {0};
      int num;

      snprintf(ptext, 100, "_dw_page%d", (int)pageid);
      num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), ptext));
      realpage = 0xFF & num;
   }

   if(realpage > -1 && realpage < 256)
   {
      child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), realpage);
      if(child)
         gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(handle), child, text);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void dw_notebook_page_set_status_text(HWND handle, unsigned long pageid, const char *text)
{
   /* TODO (if possible) */
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
void dw_notebook_pack(HWND handle, unsigned long pageid, HWND page)
{
   GtkWidget *label, *child, *oldlabel, **pagearray;
   const gchar *text = NULL;
   int num, z, realpage = -1, pad, _dw_locked_by_me = FALSE;
   char ptext[101] = {0};

   DW_MUTEX_LOCK;
   snprintf(ptext, 100, "_dw_page%d", (int)pageid);
   num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), ptext));
   g_object_set_data(G_OBJECT(handle), ptext, NULL);
   pagearray = (GtkWidget **)g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(!pagearray)
   {
      DW_MUTEX_UNLOCK;
      return;
   }

   /* The page already exists... so get it's current page */
   if(pagearray[pageid])
   {
      for(z=0;z<256;z++)
      {
         child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), z);
         if(child == pagearray[pageid])
         {
            oldlabel = gtk_notebook_get_tab_label(GTK_NOTEBOOK(handle), child);
            if(oldlabel)
               text = gtk_label_get_text(GTK_LABEL(oldlabel));
            gtk_notebook_remove_page(GTK_NOTEBOOK(handle), z);
            realpage = z;
            break;
         }
      }
   }

   pagearray[pageid] = page;

   label = gtk_label_new(text ? text : "");

   if(GTK_IS_GRID(page))
   {
      pad = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "_dw_boxpad"));
      gtk_container_set_border_width(GTK_CONTAINER(page), pad);
   }

   if(realpage != -1)
      gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, realpage);
   else if(num & ~(0xFF))
      gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, 0);
   else
      gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, 256);
   DW_MUTEX_UNLOCK;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void dw_listbox_append(HWND handle, const char *text)
{
   dw_listbox_insert(handle, text, -1);
}

/*
 * Inserts the specified text int the listbox's (or combobox) entry list at the
 * position indicated.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to insert into listbox.
 *          pos: 0-based index into listbox. -1 will append
 */
void dw_listbox_insert(HWND handle, const char *text, int pos)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      GtkTreeIter iter;

      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(!store)
      {
         DW_MUTEX_UNLOCK;
         return;
      }

      if(pos < 0)
      {
         /* Insert an entry at the end */
         gtk_list_store_append(store, &iter);
      }
      else
      {
         /* Insert at position */
         gtk_list_store_insert(store, &iter, pos);
      }
      gtk_list_store_set (store, &iter, 0, text, -1);
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
void dw_listbox_list_append(HWND handle, char **text, int count)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      int z;
      GtkTreeIter iter;

      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(!store)
      {
         DW_MUTEX_UNLOCK;
         return;
      }

      /* Insert entries at the end */
      for(z=0;z<count;z++)
      {
         gtk_list_store_append(store, &iter);
         gtk_list_store_set (store, &iter, 0, text[z], -1);
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void dw_listbox_clear(HWND handle)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(!store)
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      /* Clear the list */
      gtk_list_store_clear(store);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be counted
 */
int dw_listbox_count(HWND handle)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;
   int retval = 0;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         /* Get the number of children at the top level */
         retval = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void dw_listbox_set_top(HWND handle, int top)
{
   GtkWidget *handle2 = handle;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* Make sure it is the correct tree type */
   if(handle2 && GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
   {
      GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(handle));
      GtkListStore *store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));

      if(store && adjust)
      {
         /* Get the number of children at the top level */
         gint rowcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
         gdouble pagesize = gtk_adjustment_get_page_size(adjust);
         gdouble upper = gtk_adjustment_get_upper(adjust) - pagesize;
         gdouble lower = gtk_adjustment_get_lower(adjust);
         gdouble change;

         /* Safety check */
         if(rowcount < 2)
         {
            DW_MUTEX_UNLOCK;
            return;
         }

         /* Verify the range */
         rowcount--;
         if(top > rowcount)
            top = rowcount;

         change = ((gdouble)top/(gdouble)rowcount) * (upper - lower);

         gtk_adjustment_set_value(adjust, change + lower);
      }
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
void dw_listbox_get_text(HWND handle, unsigned int index, char *buffer, unsigned int length)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            /* Get the text */
            gchar *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, _DW_DATA_TYPE_STRING, &text, -1);
            if(text)
            {
               strncpy(buffer, text, length);
               g_free(text);
            }
            DW_MUTEX_UNLOCK;
            return;
         }
      }
   }
   buffer[0] = '\0';
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
void dw_listbox_set_text(HWND handle, unsigned int index, const char *buffer)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            /* Update the text */
            gtk_list_store_set(store, &iter, 0, buffer, -1);
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int dw_listbox_selected_multi(HWND handle, int where)
{
   GtkWidget *handle2;
   GtkListStore *store = NULL;
   int retval = DW_LIT_NONE;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   handle2 = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(handle2 && GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));

   if(store)
   {
      GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
      GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);

      if(list)
      {
         int counter = 0;
         GtkTreePath *path = g_list_nth_data(list, 0);

         while(path)
         {
            gint *indices = gtk_tree_path_get_indices(path);

            if(indices && indices[0] > where)
            {
               retval = indices[0];
               break;
            }

            counter++;
            path = g_list_nth_data(list, counter);
         }

         g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
         g_list_free(list);
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
int dw_listbox_selected(HWND handle)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;
   unsigned int retval = 0;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         if(GTK_IS_TREE_VIEW(handle2))
         {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
            GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);
            if(list)
            {
               GtkTreePath *path = g_list_nth_data(list, 0);
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  retval = indices[0];
               }

               g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
               g_list_free(list);
            }
         }
         else
         {
            GtkTreeIter iter;
            GtkTreePath *path;

            if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(handle2), &iter))
            {
               path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
               if(path)
               {
                  gint *indices = gtk_tree_path_get_indices(path);

                  if(indices)
                  {
                     retval = indices[0];
                  }
                  gtk_tree_path_free(path);
               }
            }
         }
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
void dw_listbox_select(HWND handle, int index, int state)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            if(GTK_IS_COMBO_BOX(handle2))
            {
               gtk_combo_box_set_active_iter(GTK_COMBO_BOX(handle2), &iter);
            }
            else
            {
               GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
               if(state)
               {
                  /* Select the item */
                  gtk_tree_selection_select_iter(sel, &iter);
               }
               else
               {
                  /* Deselect the item */
                  gtk_tree_selection_unselect_iter(sel, &iter);
               }
            }
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void dw_listbox_delete(HWND handle, int index)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            gtk_list_store_remove(store, &iter);
         }
      }
   }
   DW_MUTEX_UNLOCK;
}

/* Function to do delayed positioning */
gboolean _dw_splitbar_set_percent(gpointer data)
{
   GtkWidget *widget = data;
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   if(percent)
   {
      GtkAllocation alloc;

      gtk_widget_get_allocation(widget, &alloc);

      if(gtk_orientable_get_orientation(GTK_ORIENTABLE(widget)) == GTK_ORIENTATION_HORIZONTAL)
         gtk_paned_set_position(GTK_PANED(widget), (int)(alloc.width * (*percent / 100.0)));
      else
         gtk_paned_set_position(GTK_PANED(widget), (int)(alloc.height * (*percent / 100.0)));
      g_object_set_data(G_OBJECT(widget), "_dw_percent", NULL);
      free(percent);
   }
   return FALSE;
}

/* Reposition the bar according to the percentage */
static gint _dw_splitbar_size_allocate(GtkWidget *widget, GtkAllocation *event, gpointer data)
{
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   /* Prevent infinite recursion ;) */
   if(!percent || event->width < 20 || event->height < 20)
      return FALSE;

   g_idle_add(_dw_splitbar_set_percent, widget);
   return FALSE;
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
HWND dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long id)
{
   GtkWidget *tmp = NULL;
   int _dw_locked_by_me = FALSE;
   float *percent = malloc(sizeof(float));

   DW_MUTEX_LOCK;
   tmp = gtk_paned_new(type == DW_HORZ ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
   gtk_paned_pack1(GTK_PANED(tmp), topleft, TRUE, FALSE);
   gtk_paned_pack2(GTK_PANED(tmp), bottomright, TRUE, FALSE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   *percent = 50.0;
   g_object_set_data(G_OBJECT(tmp), "_dw_percent", (gpointer)percent);
   g_signal_connect(G_OBJECT(tmp), "size-allocate", G_CALLBACK(_dw_splitbar_size_allocate), NULL);
   gtk_widget_show(tmp);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void dw_splitbar_set(HWND handle, float percent)
{
   float *mypercent = (float *)dw_window_get_data(handle, "_dw_percent");
   int size = 0, position;

   if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_HORIZONTAL)
      size = gtk_widget_get_allocated_width(handle);
   else
      size = gtk_widget_get_allocated_height(handle);

   if(mypercent)
      *mypercent = percent;

   if(size > 10)
   {
      position = (int)((float)size * (percent / 100.0));

      gtk_paned_set_position(GTK_PANED(handle), position);
   }
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float dw_splitbar_get(HWND handle)
{
   float *percent = (float *)dw_window_get_data(handle, "_dw_percent");

   if(percent)
      return *percent;
   return 0.0;
}

/*
 * Creates a calendar window (widget) with given parameters.
 * Parameters:
 *       id: Unique identifier for calendar widget
 * Returns:
 *       A handle to a calendar window or NULL on failure.
 */
HWND dw_calendar_new(unsigned long id)
{
   GtkWidget *tmp;
   int _dw_locked_by_me = FALSE;
   GtkCalendarDisplayOptions flags;
   time_t now;
   struct tm *tmdata;

   DW_MUTEX_LOCK;
   tmp = gtk_calendar_new();
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   /* select today */
   flags = GTK_CALENDAR_SHOW_HEADING|GTK_CALENDAR_SHOW_DAY_NAMES;
   gtk_calendar_set_display_options( GTK_CALENDAR(tmp), flags );
   now = time( NULL );
   tmdata = localtime( & now );
   gtk_calendar_select_month( GTK_CALENDAR(tmp), tmdata->tm_mon, 1900+tmdata->tm_year );
   gtk_calendar_select_day( GTK_CALENDAR(tmp), tmdata->tm_mday );

   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Sets the current date of a calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
void dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_CALENDAR(handle))
   {
      gtk_calendar_select_month(GTK_CALENDAR(handle),month-1,year);
      gtk_calendar_select_day(GTK_CALENDAR(handle), day);
   }
   DW_MUTEX_UNLOCK;
   return;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
void dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_CALENDAR(handle))
   {
      gtk_calendar_get_date(GTK_CALENDAR(handle),year,month,day);
      *month = *month + 1;
   }
   DW_MUTEX_UNLOCK;
   return;
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
    int _dw_locked_by_me = FALSE;

    if(!handle)
       return;

    DW_MUTEX_LOCK;
    gtk_widget_grab_focus(handle);
    DW_MUTEX_UNLOCK;
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void dw_window_default(HWND window, HWND defaultitem)
{
   int _dw_locked_by_me = FALSE;

   if(!window)
      return;

   DW_MUTEX_LOCK;
   g_object_set_data(G_OBJECT(window),  "_dw_defaultitem", (gpointer)defaultitem);
   DW_MUTEX_UNLOCK;
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void dw_window_click_default(HWND window, HWND next)
{
   int _dw_locked_by_me = FALSE;

   if(!window)
      return;

   DW_MUTEX_LOCK;
   g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(_dw_default_key_press_event), next);
   DW_MUTEX_UNLOCK;
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
HWND dw_notification_new(const char *title, const char *imagepath, const char *description, ...)
{
#if GLIB_CHECK_VERSION(2,40,0)
   GNotification *notification = g_notification_new(title);

   if(notification)
   {
      if(description)
      {
         va_list args;
         char outbuf[1025] = {0};

         va_start(args, description);
         vsnprintf(outbuf, 1024, description, args);
         va_end(args);

         g_notification_set_body(notification, outbuf);
      }
#if GLIB_CHECK_VERSION(2,52,0)
      if(imagepath)
      {
         GFile *file = g_file_new_for_path(imagepath);
         GBytes *bytes = g_file_load_bytes(file, NULL, NULL, NULL);
         
         if(bytes)
         {
            GIcon *icon = g_bytes_icon_new(bytes);
            
            if(icon)
               g_notification_set_icon(notification, G_ICON(icon));
         }
      }
#endif
      g_notification_set_default_action_and_target(notification, "app.notification", "t", 
                                                  (guint64)DW_POINTER_TO_ULONGLONG(notification)); 
   }
   return (HWND)notification;
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
int dw_notification_send(HWND notification)
{
#if GLIB_CHECK_VERSION(2,40,0)
   if(notification)
   {
      char id[101] = {0};

      /* Generate a unique ID based on the notification handle,
       * so we can use it to remove the notification in dw_window_destroy().
       */
      snprintf(id, 100, "dw-notification-%llu", DW_POINTER_TO_ULONGLONG(notification));
      g_application_send_notification(_DWApp, id, (GNotification *)notification);
      return DW_ERROR_NONE;
   }
#endif
   return DW_ERROR_UNKNOWN;
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void dw_environment_query(DWEnv *env)
{
   struct utsname name;
   char tempbuf[_DW_ENV_STRING_SIZE] = { 0 }, *dot;

   uname(&name);
   memset(env, '\0', sizeof(DWEnv));
   strncpy(env->osName, name.sysname, sizeof(env->osName)-1);
   strncpy(tempbuf, name.release, sizeof(tempbuf)-1);

   strncpy(env->buildDate, __DATE__, sizeof(env->buildDate)-1);
   strncpy(env->buildTime, __TIME__, sizeof(env->buildTime)-1);
#ifdef USE_WEBKIT
#  ifdef USE_WEBKIT2
   strncpy(env->htmlEngine, "WEBKIT2", sizeof(env->htmlEngine)-1);
#  else
   strncpy(env->htmlEngine, "WEBKIT", sizeof(env->htmlEngine)-1);
#  endif
#else
   strncpy(env->htmlEngine, "N/A", sizeof(env->htmlEngine)-1);
#endif
   env->DWMajorVersion = DW_MAJOR_VERSION;
   env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
   env->DWSubVersion = VER_REV;
#else
   env->DWSubVersion = DW_SUB_VERSION;
#endif

   if((dot = strchr(tempbuf, '.')) != NULL)
   {
      *dot = '\0';
      env->MajorVersion = atoi(tempbuf);
      env->MinorVersion = atoi(&dot[1]);
      return;
   }
   env->MajorVersion = atoi(tempbuf);
}

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extention.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE or DW_DIRECTORY_OPEN
 * Returns:
 *       NULL on error. A malloced buffer containing
 *       the file path on success.
 *
 */
char *dw_file_browse(const char *title, const char *defpath, const char *ext, int flags)
{
   GtkWidget *filew;

   GtkFileChooserAction action;
   GtkFileFilter *filter1 = NULL;
   GtkFileFilter *filter2 = NULL;
   gchar *button;
   char *filename = NULL;
   char buf[1000];
   char mypath[PATH_MAX+1];

   switch (flags )
   {
      case DW_DIRECTORY_OPEN:
         action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
         button = _("_Open");
         break;
      case DW_FILE_OPEN:
         action = GTK_FILE_CHOOSER_ACTION_OPEN;
         button = _("_Open");
         break;
      case DW_FILE_SAVE:
         action = GTK_FILE_CHOOSER_ACTION_SAVE;
         button = _("_Save");
         break;
      default:
         dw_messagebox( "Coding error", DW_MB_OK|DW_MB_ERROR, "dw_file_browse() flags argument invalid.");
         return NULL;
         break;
   }

   filew = gtk_file_chooser_dialog_new ( title,
                                         NULL,
                                         action,
                                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                                         button, GTK_RESPONSE_ACCEPT,
                                         NULL);

   if ( flags == DW_FILE_SAVE )
      gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER( filew ), TRUE );

   if ( ext )
   {
      filter1 = gtk_file_filter_new();
      sprintf( buf, "*.%s", ext );
      gtk_file_filter_add_pattern( filter1, (gchar *)buf );
      sprintf( buf, "\"%s\" files", ext );
      gtk_file_filter_set_name( filter1, (gchar *)buf );
      filter2 = gtk_file_filter_new();
      gtk_file_filter_add_pattern( filter2, (gchar *)"*" );
      gtk_file_filter_set_name( filter2, (gchar *)"All Files" );
      gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( filew ), filter1 );
      gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( filew ), filter2 );
   }

   if ( defpath )
   {
      struct stat buf;

      if ( g_path_is_absolute( defpath ) || !realpath(defpath, mypath))
      {
         strcpy( mypath, defpath );
      }

      /* See if the path exists */
      if(stat(mypath, &buf) == 0)
      {
         /* If the path is a directory... set the current folder */
         if(buf.st_mode & S_IFDIR)
            gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( filew ), mypath );
         else if(flags == DW_FILE_SAVE) /* Otherwise set the filename */
            gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( filew ), mypath );
         else if(flags == DW_FILE_OPEN)
            gtk_file_chooser_select_filename( GTK_FILE_CHOOSER( filew), mypath );
      }
      else if(flags == DW_FILE_SAVE)
      {
         if(strchr(mypath, '/'))
         {
            unsigned long x = strlen(mypath) - 1;

            /* Trim off the filename */
            while(x > 0 && mypath[x] != '/')
            {
                x--;
            }
            if(mypath[x] == '/')
            {
                char *file = NULL;
                char temp[PATH_MAX+1];

                /* Save the original path in temp */
                strcpy(temp, mypath);
                mypath[x] = 0;

                /* Check to make sure the trimmed piece is a directory */
                if(realpath(mypath, temp) && stat(temp, &buf) == 0)
                {
                   if(buf.st_mode & S_IFDIR)
                   {
                      /* We now have it split */
                      file = &mypath[x+1];
                   }
                }

                /* Select folder... */
                gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(filew), temp );
                /* ... and file separately */
                if(file)
                   gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(filew), file );
            }
         }
      }
   }

   if ( gtk_dialog_run( GTK_DIALOG( filew ) ) == GTK_RESPONSE_ACCEPT )
   {
      filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( filew ) );
      /*g_free (filename);*/
   }

   gtk_widget_destroy( filew );
   return filename;
}

#if GLIB_CHECK_VERSION(2,36,0)
static void _dw_exec_launched(GAppLaunchContext *context, GAppInfo *info, GVariant *platform_data, gpointer data)
{
   g_variant_lookup(platform_data, "pid", "i", data);
}
#endif

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       DW_ERROR_UNKNOWN on error.
 */
int API dw_exec(const char *program, int type, char **params)
{
   GAppInfo *appinfo = NULL;
   char *commandline;
   int retval = DW_ERROR_UNKNOWN;
   
   /* Generate a command line from the parameters */
   if(params && *params)
   {
      int z = 0, len = 0;
      
      while(params[z])
      {
         len+=strlen(params[z]) + 1;
         z++;
      }
      z=1;
      commandline = calloc(1, len);
      strcpy(commandline, params[0]);
      while(params[z])
      {
         strcat(commandline, " ");
         strcat(commandline, params[z]);
         z++;
      }
   }
   else
      commandline = strdup(program);

   /* Attempt to use app preferences to launch the application, using the selected Terminal if necessary */
   if((appinfo = g_app_info_create_from_commandline(commandline, NULL, 
      type == DW_EXEC_CON ? G_APP_INFO_CREATE_NEEDS_TERMINAL : G_APP_INFO_CREATE_NONE, NULL)))
   {
      GAppLaunchContext *context = g_app_launch_context_new();

#if GLIB_CHECK_VERSION(2,36,0)
      g_signal_connect(G_OBJECT(context), "launched", G_CALLBACK(_dw_exec_launched), (gpointer)&retval);
#endif

      if(g_app_info_launch(appinfo, NULL, context, NULL) && retval == DW_ERROR_UNKNOWN)
         retval = DW_ERROR_NONE;

      g_object_unref(appinfo);
      g_object_unref(context);
   }
   free(commandline);
   return retval;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int dw_browse(const char *url)
{
   /* If possible load the URL/URI using gvfs... */
#if GTK_CHECK_VERSION(3,22,0)
   if(gtk_show_uri_on_window(NULL, url, GDK_CURRENT_TIME, NULL))
#else
   if(gtk_show_uri(gdk_screen_get_default(), url, GDK_CURRENT_TIME, NULL))
#endif
   {
      return DW_ERROR_NONE;
   }
   else
   {
      /* Otherwise just fall back to executing firefox...
       * or the browser defined by the DW_BROWSER variable.
       */
      char *execargs[3], *browser = "firefox", *tmp;

      tmp = getenv( "DW_BROWSER" );
      if(tmp) browser = tmp;
      execargs[0] = browser;
      execargs[1] = (char *)url;
      execargs[2] = NULL;

      return dw_exec(browser, DW_EXEC_GUI, execargs);
   }
}

#ifdef USE_WEBKIT
/* Helper function to get the web view handle */
WebKitWebView *_dw_html_web_view(GtkWidget *widget)
{
   if(widget)
   {
      WebKitWebView *web_view = (WebKitWebView *)widget;
      if(WEBKIT_IS_WEB_VIEW(web_view))
         return web_view;
#ifndef USE_WEBKIT2
      web_view = (WebKitWebView *)g_object_get_data(G_OBJECT(widget), "_dw_web_view");
      if(WEBKIT_IS_WEB_VIEW(web_view))
         return web_view;
#endif
   }
   return NULL;
}
#endif
/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void dw_html_action(HWND handle, int action)
{
#ifdef USE_WEBKIT
   int _dw_locked_by_me = FALSE;
   WebKitWebView *web_view;

   DW_MUTEX_LOCK;

   if((web_view = _dw_html_web_view(handle)))
   {
      switch(action)
      {
         case DW_HTML_GOBACK:
            webkit_web_view_go_back(web_view);
            break;
         case DW_HTML_GOFORWARD:
            webkit_web_view_go_forward(web_view);
            break;
         case DW_HTML_GOHOME:
#ifdef USE_WEBKIT2
            webkit_web_view_load_uri(web_view, DW_HOME_URL);
#else
            webkit_web_view_open(web_view, DW_HOME_URL);
#endif
            break;
         case DW_HTML_RELOAD:
            webkit_web_view_reload(web_view);
            break;
         case DW_HTML_STOP:
            webkit_web_view_stop_loading(web_view);
            break;
         case DW_HTML_PRINT:
            {
#ifdef USE_WEBKIT2
               WebKitPrintOperation *operation = webkit_print_operation_new(web_view);
               webkit_print_operation_run_dialog(operation, NULL);
               g_object_unref(operation);
#else
               WebKitWebFrame *frame = webkit_web_view_get_focused_frame(web_view);
               webkit_web_frame_print(frame);
#endif
            }
            break;
      }
   }
   DW_MUTEX_UNLOCK;
#endif
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
int dw_html_raw(HWND handle, const char *string)
{
#ifdef USE_WEBKIT
   int _dw_locked_by_me = FALSE;
   WebKitWebView *web_view;

   DW_MUTEX_LOCK;
   if((web_view = _dw_html_web_view(handle)))
   {
#ifdef USE_WEBKIT2
      webkit_web_view_load_html(web_view, string, "file:///");
#else
      webkit_web_view_load_html_string(web_view, string, "file:///");
#endif
      gtk_widget_show(GTK_WIDGET(handle));
   }
   DW_MUTEX_UNLOCK;
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
#endif
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
int dw_html_url(HWND handle, const char *url)
{
#ifdef USE_WEBKIT
   int _dw_locked_by_me = FALSE;
   WebKitWebView *web_view;

   DW_MUTEX_LOCK;
   if((web_view = _dw_html_web_view(handle)))
   {
#ifdef USE_WEBKIT2
      webkit_web_view_load_uri(web_view, url);
#else
      webkit_web_view_open(web_view, url);
#endif
      gtk_widget_show(GTK_WIDGET(handle));
   }
   DW_MUTEX_UNLOCK;
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
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
#ifdef USE_WEBKIT
   int _dw_locked_by_me = FALSE;
   WebKitWebView *web_view;

   DW_MUTEX_LOCK;
   if(script && (web_view = _dw_html_web_view(handle)))
#ifdef USE_WEBKIT2
#if WEBKIT_CHECK_VERSION(2, 40, 0)
      webkit_web_view_evaluate_javascript(web_view, script, strlen(script), NULL, NULL, NULL, _dw_html_result_event, scriptdata);
#else
      webkit_web_view_run_javascript(web_view, script, NULL, _dw_html_result_event, scriptdata);
#endif
#else
      webkit_web_view_execute_script(web_view, script);
#endif
   DW_MUTEX_UNLOCK;
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/* Free the name when the signal disconnects */
void _dw_html_message_disconnect(gpointer gdata, GClosure *closure)
{
    gpointer *data = (gpointer *)gdata;

    if(data)
    {
        gchar *name = (gchar *)data[1];

        if(name)
            g_free(name);
        free(data);
    }
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
#ifdef USE_WEBKIT2
   WebKitWebView *web_view= _dw_html_web_view(handle);
   WebKitUserContentManager *manager;

    if(web_view && (manager = webkit_web_view_get_user_content_manager(web_view)) && name) 
    {
        /* Script to inject that will call the handler we are adding */
        gchar *script = g_strdup_printf("function %s(body) {window.webkit.messageHandlers.%s.postMessage(body);}", 
                            name, name);
        gchar *signal = g_strdup_printf("script-message-received::%s", name);
        WebKitUserScript *userscript = webkit_user_script_new(script, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                              WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
        gpointer *data = calloc(sizeof(gpointer), 2);

        data[0] = handle;
        data[1] = g_strdup(name);
        g_signal_connect_data(manager, signal, G_CALLBACK(_dw_html_message_event), data, _dw_html_message_disconnect, 0);
        webkit_user_content_manager_register_script_message_handler(manager, name);
        webkit_user_content_manager_add_script(manager, userscript);

        g_free(script);
        g_free(signal);
        return DW_ERROR_NONE;
    }
#endif
    return DW_ERROR_UNKNOWN;
}

#if defined(USE_WEBKIT) && !defined(USE_WEBKIT2)
static void _dw_html_print_cb(GtkWidget *widget, gpointer *data)
{
   WebKitWebView *web_view = WEBKIT_WEB_VIEW(data);
   WebKitWebFrame *frame;

   frame = webkit_web_view_get_focused_frame(web_view);
   webkit_web_frame_print(frame);
}

/*
 * Fired when the user right-clicks to get the popup-menu on the HTML widget
 * We add a "Print" menu item to enable printing
 * user_data is not used
 */
static void _dw_html_populate_popup_cb(WebKitWebView *web_view, GtkMenu *menu, gpointer user_data)
{
   GtkWidget *item = gtk_menu_item_new_with_mnemonic( _("_Print") );

   gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
   g_signal_connect(item, "activate", G_CALLBACK(_dw_html_print_cb), web_view);
   gtk_widget_show(item);
}
#endif

/*
 * Create a new HTML window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_html_new(unsigned long id)
{
   GtkWidget *widget = NULL;
#ifdef USE_WEBKIT
   int _dw_locked_by_me = FALSE;
   WebKitWebView *web_view;
#ifdef USE_WEBKIT2
   WebKitSettings *settings;
   WebKitUserContentManager *manager;
#else
   WebKitWebSettings *settings;
#endif

   DW_MUTEX_LOCK;
#ifdef USE_WEBKIT2
   manager = webkit_user_content_manager_new();
   web_view = (WebKitWebView *)webkit_web_view_new_with_user_content_manager(manager);
#else
   web_view = (WebKitWebView *)webkit_web_view_new();
#endif
   /* WebKit2 no longer requires a scrolled window...
    * So only create a scrolled window and pack it in older versions.
    */
#ifndef USE_WEBKIT2
   widget = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER (widget), GTK_WIDGET(web_view));
   gtk_widget_show(GTK_WIDGET(web_view));
   g_object_set_data(G_OBJECT(widget), "_dw_web_view", (gpointer)web_view);
   g_signal_connect(web_view, "populate-popup", G_CALLBACK(_dw_html_populate_popup_cb), NULL);
   /* Create a new websettings and enable java script */
   settings = webkit_web_settings_new();
   g_object_set(G_OBJECT(settings), "enable-scripts", TRUE, NULL);
   webkit_web_view_set_settings(WEBKIT_WEB_VIEW(web_view), settings);
#else
   settings = webkit_web_view_get_settings(web_view);
   /* Make sure java script is enabled */
   webkit_settings_set_enable_javascript(settings, TRUE);
   webkit_web_view_set_settings(web_view, settings);
   widget = (GtkWidget *)web_view;
#endif
   gtk_widget_show(widget);
   DW_MUTEX_UNLOCK;
#else
   dw_debug( "HTML widget not available; you do not have access to webkit.\n" );
#endif
   return widget;
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
   int _dw_locked_by_me = FALSE;
   GtkClipboard *clipboard_object;
   char *ret = NULL;

   DW_MUTEX_LOCK;
   if((clipboard_object = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD )))
   {
      gchar *clipboard_contents;

      if((clipboard_contents = gtk_clipboard_wait_for_text( clipboard_object )))
      {
         ret = strdup((char *)clipboard_contents);
         g_free(clipboard_contents);
      }
   }
   DW_MUTEX_UNLOCK;
   return ret;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void  dw_clipboard_set_text(const char *str, int len)
{
   int _dw_locked_by_me = FALSE;
   GtkClipboard *clipboard_object;

   DW_MUTEX_LOCK;
   if((clipboard_object = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD )))
   {
      gtk_clipboard_set_text( clipboard_object, str, len );
   }
   DW_MUTEX_UNLOCK;
}

/* Internal function to create the drawable pixmap and call the function */
static void _dw_draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr)
{
   cairo_t *cr = gtk_print_context_get_cairo_context(context);
   void *drawdata = g_object_get_data(G_OBJECT(operation), "_dw_drawdata");
   int (*drawfunc)(HPRINT, HPIXMAP, int, void *) = g_object_get_data(G_OBJECT(operation), "_dw_drawfunc");
   int result = 0;
   HPIXMAP pixmap;

   if(cr && drawfunc && (pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      pixmap->image = cairo_get_group_target(cr);
      pixmap->handle = (HWND)operation;
      pixmap->width = gtk_print_context_get_width(context);
      pixmap->height = gtk_print_context_get_height(context);
      result = drawfunc((HPRINT)operation, pixmap, page_nr, drawdata);
      if(result)
         gtk_print_operation_draw_page_finish(operation);
      free(pixmap);
   }
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
   GtkPrintOperation *op;
   int _dw_locked_by_me = FALSE;

   if(!drawfunc)
      return NULL;

   DW_MUTEX_LOCK;
   if((op = gtk_print_operation_new()))
   {
      gtk_print_operation_set_n_pages(op, pages);
      gtk_print_operation_set_job_name(op, jobname ? jobname : "Dynamic Windows Print Job");
      g_object_set_data(G_OBJECT(op), "_dw_drawfunc", drawfunc);
      g_object_set_data(G_OBJECT(op), "_dw_drawdata", drawdata);
      g_signal_connect(op, "draw_page", G_CALLBACK(_dw_draw_page), NULL);
   }
   DW_MUTEX_UNLOCK;
   return (HPRINT)op;
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
   GtkPrintOperationResult res;
   GtkPrintOperation *op = (GtkPrintOperation *)print;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   res = gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
   DW_MUTEX_UNLOCK;
   return (res == GTK_PRINT_OPERATION_RESULT_ERROR ? DW_ERROR_UNKNOWN : DW_ERROR_NONE);
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
   int _dw_locked_by_me = FALSE;
   GtkPrintOperation *op = (GtkPrintOperation *)print;

   DW_MUTEX_LOCK;
   gtk_print_operation_cancel(op);
   DW_MUTEX_UNLOCK;
}

/*
 * Returns a pointer to a static buffer which contains the
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
 * Returns a pointer to a static buffer which containes the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    return _dw_share_path;
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
#if GLIB_CHECK_VERSION(2,28,0)
   if(g_application_id_is_valid(appid))
   {
      strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
      if(appname)
      	g_set_application_name(appname);
      return DW_ERROR_NONE;
   }
   return DW_ERROR_GENERAL;
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/*
 * Call a function from the window (widget)'s context.
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
void dw_window_function(HWND handle, void *function, void *data)
{
   void (* windowfunc)(void *);

   windowfunc = function;

   if(windowfunc)
      windowfunc(data);
}

/*
 * Add a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void dw_window_set_data(HWND window, const char *dataname, void *data)
{
   HWND thiswindow = window;
   int _dw_locked_by_me = FALSE;

   if(!window)
      return;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(window))
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }
   if(thiswindow && G_IS_OBJECT(thiswindow))
      g_object_set_data(G_OBJECT(thiswindow), dataname, (gpointer)data);
   DW_MUTEX_UNLOCK;
}

/*
 * Gets a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void *dw_window_get_data(HWND window, const char *dataname)
{
   HWND thiswindow = window;
   int _dw_locked_by_me = FALSE;
   void *ret = NULL;

   if(!window)
      return NULL;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(window))
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }
   if(G_IS_OBJECT(thiswindow))
      ret = (void *)g_object_get_data(G_OBJECT(thiswindow), dataname);
   DW_MUTEX_UNLOCK;
   return ret;
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

/* Internal function to get the state of the timer before firing */
gboolean _dw_timer_func(gpointer data)
{
   void (*sigfunc)(void *data) = NULL;
   void *sdata;
   char tmpbuf[31] = {0};
   int *tag = data;

   if(tag)
   {
      snprintf(tmpbuf, 30, "_dw_timer%d", *tag);
      sigfunc = g_object_get_data(G_OBJECT(_DWObject), tmpbuf);
      snprintf(tmpbuf, 30, "_dw_timerdata%d", *tag);
      sdata = g_object_get_data(G_OBJECT(_DWObject), tmpbuf);
   }
   if(!sigfunc)
   {
      if(tag)
         free(tag);
      return FALSE;
   }
   sigfunc(sdata);
   return TRUE;
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
   int *tag, _dw_locked_by_me = FALSE;
   char tmpbuf[31] = {0};

   tag = calloc(1, sizeof(int));

   DW_MUTEX_LOCK;
   *tag = g_timeout_add(interval, (GSourceFunc)_dw_timer_func, (gpointer)tag);
   snprintf(tmpbuf, 30, "_dw_timer%d", *tag);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, sigfunc);
   snprintf(tmpbuf, 30, "_dw_timerdata%d", *tag);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, data);
   DW_MUTEX_UNLOCK;
   return *tag;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(HTIMER id)
{
   int _dw_locked_by_me = FALSE;
   char tmpbuf[31] = {0};

   snprintf(tmpbuf, 30, "_dw_timer%d", id);
   DW_MUTEX_LOCK;
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, NULL);
   snprintf(tmpbuf, 30, "_dw_timerdata%d", id);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, NULL);
   DW_MUTEX_UNLOCK;
}

/* Get the actual signal window handle not the user window handle
 * Should mimic the code in dw_signal_connect() below.
 */
static HWND _dw_find_signal_window(HWND window, const char *signame)
{
   HWND thiswindow = window;

   if(GTK_IS_SCROLLED_WINDOW(thiswindow))
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   else if(GTK_IS_SCALE(thiswindow) || GTK_IS_SCROLLBAR(thiswindow) || GTK_IS_SPIN_BUTTON(thiswindow))
      thiswindow = (GtkWidget *)g_object_get_data(G_OBJECT(thiswindow), "_dw_adjustment");
   else if(GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_SELECT) == 0)
      thiswindow = (GtkWidget *)gtk_tree_view_get_selection(GTK_TREE_VIEW(thiswindow));
   return thiswindow;
}

/*
 * Add a callback to a window event.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 */
void dw_signal_connect(HWND window, const char *signame, void *sigfunc, void *data)
{
   dw_signal_connect_data(window, signame, sigfunc, NULL, data);
}

/* Internal function to free any allocated signal data..
 * and call any required function to free additional memory.
 */
static void _dw_signal_disconnect(gpointer data, GClosure *closure)
{
   if(data)
   {
      void **params = (void **)data;
      void (*discfunc)(HWND, void *) = params[1];

      if(discfunc)
      {
         DWSignalHandler work = _dw_get_signal_handler(data);

         if(work.window)
         {
            discfunc(work.window, work.data);
         }
      }
      free(data);
   }
}

#define _DW_INTERNAL_CALLBACK_PARAMS 4

/*
 * Add a callback to a window event with a closure callback.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       discfunc: The pointer to the function called when this handler is removed.
 *       data: User data to be passed to the handler function.
 */
void dw_signal_connect_data(HWND window, const char *signame, void *sigfunc, void *discfunc, void *data)
{
   DWSignalList signal = _dw_findsignal(signame);
   void *thisfunc  = signal.func;
   char *thisname = (char *)signame;
   HWND thiswindow = window;
   int sigid, _dw_locked_by_me = FALSE;
   void **params = calloc(_DW_INTERNAL_CALLBACK_PARAMS, sizeof(void *));
   gint cid;

   /* Save the disconnect function pointer */
   params[1] = discfunc;

   DW_MUTEX_LOCK;
#if GLIB_CHECK_VERSION(2,40,0)
   /* Special case for handling notification signals, which aren't really signals */
   if (G_IS_NOTIFICATION(thiswindow) && strcmp(signame, DW_SIGNAL_CLICKED) == 0)
   {
      char textbuf[101] = {0};
      snprintf(textbuf, 100, "dw-notification-%llu-func", DW_POINTER_TO_ULONGLONG(thiswindow));
      g_object_set_data(G_OBJECT(_DWApp), textbuf, DW_POINTER(sigfunc));
      snprintf(textbuf, 100, "dw-notification-%llu-data", DW_POINTER_TO_ULONGLONG(thiswindow));
      g_object_set_data(G_OBJECT(_DWApp), textbuf, DW_POINTER(data)); 
      DW_MUTEX_UNLOCK;     
      return;
   }   
#endif
   /*
    * If the window we are setting the signal on is a scrolled window we need to get
    * the "real" widget type. thiswindow is the "real" widget type
    */
   if (GTK_IS_SCROLLED_WINDOW(thiswindow)
#ifdef USE_WEBKIT
       && !(thiswindow = (HWND)_dw_html_web_view(thiswindow))
#endif
   )
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }

   if (strcmp(signame, DW_SIGNAL_EXPOSE) == 0)
   {
      thisname = "draw";
   }
   else if (GTK_IS_MENU_ITEM(thiswindow) && strcmp(signame, DW_SIGNAL_CLICKED) == 0)
   {
      DWSignalList signal = _dw_findsignal(signame);
      thisname = "activate";
      thisfunc = signal.func;
   }
   else if (GTK_IS_TREE_VIEW(thiswindow)  && strcmp(signame, DW_SIGNAL_ITEM_CONTEXT) == 0)
   {
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, thisfunc, discfunc);
      params[0] = GINT_TO_POINTER(sigid);
      params[2] = (void *)thiswindow;
      cid = g_signal_connect_data(G_OBJECT(thiswindow), "button_press_event", G_CALLBACK(thisfunc), params, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(thiswindow, sigid, cid);

      DW_MUTEX_UNLOCK;
      return;
   }
   else if ((GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_SELECT) == 0) ||
            (GTK_IS_COMBO_BOX(thiswindow) && strcmp(signame, DW_SIGNAL_LIST_SELECT) == 0))
   {
      GtkWidget *widget = thiswindow;

      thisname = "changed";

      sigid = _dw_set_signal_handler(widget, window, sigfunc, data, thisfunc, discfunc);
      params[0] = GINT_TO_POINTER(sigid);
      params[2] = (void *)thiswindow;
      if(GTK_IS_TREE_VIEW(thiswindow))
      {
         thiswindow = (GtkWidget *)gtk_tree_view_get_selection(GTK_TREE_VIEW(thiswindow));
         cid = g_signal_connect_data(G_OBJECT(thiswindow), thisname, G_CALLBACK(thisfunc), params, _dw_signal_disconnect, 0);
      }
      else
      {
         cid = g_signal_connect_data(G_OBJECT(thiswindow), thisname, G_CALLBACK(_dw_combobox_select_event), params, _dw_signal_disconnect, 0);
      }
      _dw_set_signal_handler_id(widget, sigid, cid);

      DW_MUTEX_UNLOCK;
      return;
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_TREE_EXPAND) == 0)
   {
      thisname = "row-expanded";
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_ENTER) == 0)
   {
      DWSignalList signal = _dw_findsignal(DW_SIGNAL_ITEM_ENTER);
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, _dw_container_enter_event, discfunc);
      params[0] = GINT_TO_POINTER(sigid);
      params[2] = (void *)thiswindow;
      cid = g_signal_connect_data(G_OBJECT(thiswindow), "key_press_event", G_CALLBACK(_dw_container_enter_event), params, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(thiswindow, sigid, cid);

      params = calloc(sizeof(void *), _DW_INTERNAL_CALLBACK_PARAMS);

      thisname = "button_press_event";
      thisfunc = signal.func;
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_COLUMN_CLICK) == 0)
   {
      /* We don't actually need a signal handler here... just need to assign the handler ID
       * Since the handlers for the columns were already created in _dw_container_setup()
       */
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, _dw_column_click_event, discfunc);
      g_object_set_data(G_OBJECT(thiswindow), "_dw_column_click_id", GINT_TO_POINTER(sigid+1));
      DW_MUTEX_UNLOCK;
      return;
   }
   else if (strcmp(signame, DW_SIGNAL_SET_FOCUS) == 0)
   {
      thisname = "focus-in-event";
      if (GTK_IS_COMBO_BOX(thiswindow))
         thiswindow = gtk_bin_get_child(GTK_BIN(thiswindow));
   }
#ifdef USE_WEBKIT
   else if (WEBKIT_IS_WEB_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_HTML_CHANGED) == 0)
   {
#ifdef USE_WEBKIT2
      thisname = "load-changed";
#else
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, thisfunc, discfunc);
      params[0] = GINT_TO_POINTER(sigid);
      params[2] = (void *)thiswindow;
      params[3] = GINT_TO_POINTER(DW_HTML_CHANGE_STARTED);
      cid = g_signal_connect_data(G_OBJECT(thiswindow), "load-started", G_CALLBACK(thisfunc), params, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(thiswindow, sigid, cid);

      params = calloc(sizeof(void *), _DW_INTERNAL_CALLBACK_PARAMS);

      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, thisfunc, discfunc);
      params[0] = GINT_TO_POINTER(sigid);
      params[2] = (void *)thiswindow;
      params[3] = GINT_TO_POINTER(DW_HTML_CHANGE_LOADING);
      cid = g_signal_connect_data(G_OBJECT(thiswindow), "load-committed", G_CALLBACK(thisfunc), params, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(thiswindow, sigid, cid);

      params = calloc(sizeof(void *), _DW_INTERNAL_CALLBACK_PARAMS);
      params[3] = GINT_TO_POINTER(DW_HTML_CHANGE_COMPLETE);

      thisname = "load-finished";
#endif
   }
   else if (WEBKIT_IS_WEB_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_HTML_RESULT) == 0)
   {
      /* We don't actually need a signal handler here... just need to assign the handler ID
       * Since the handler is created in dw_html_javasript_run()
       */
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, _dw_html_result_event, discfunc);
      g_object_set_data(G_OBJECT(thiswindow), "_dw_html_result_id", GINT_TO_POINTER(sigid+1));
#ifndef USE_WEBKIT2
      dw_debug("WARNING: DW_SIGNAL_HTML_RESULT will not be generated unless using webkit2gtk!\n");
#endif
      DW_MUTEX_UNLOCK;
      return;
   }
#ifdef USE_WEBKIT2
   else if (WEBKIT_IS_WEB_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_HTML_MESSAGE) == 0)
   {
      /* We don't actually need a signal handler here... just need to assign the handler ID
       * Since the handler is created in dw_html_javasript_add()
       */
      sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, _dw_html_message_event, discfunc);
      g_object_set_data(G_OBJECT(thiswindow), "_dw_html_message_id", GINT_TO_POINTER(sigid+1));
      DW_MUTEX_UNLOCK;
      return;
   }
#endif
#endif
#if 0
   else if (strcmp(signame, DW_SIGNAL_LOSE_FOCUS) == 0)
   {
      thisname = "focus-out-event";
      if(GTK_IS_COMBO_BOX(thiswindow))
         thiswindow = GTK_COMBO_BOX(thiswindow)->entry;
   }
#endif
   else if (GTK_IS_SCALE(thiswindow) || GTK_IS_SCROLLBAR(thiswindow) || GTK_IS_SPIN_BUTTON(thiswindow) )
   {
      thiswindow = (GtkWidget *)g_object_get_data(G_OBJECT(thiswindow), "_dw_adjustment");
   }
   else if (GTK_IS_NOTEBOOK(thiswindow) && strcmp(signame, DW_SIGNAL_SWITCH_PAGE) == 0)
   {
      thisname = "switch-page";
   }

   if (!thisfunc || !thiswindow)
   {
      free(params);
      DW_MUTEX_UNLOCK;
      return;
   }

   sigid = _dw_set_signal_handler(thiswindow, window, sigfunc, data, thisfunc, discfunc);
   params[0] = GINT_TO_POINTER(sigid);
   params[2] = (void *)thiswindow;
   cid = g_signal_connect_data(G_OBJECT(thiswindow), thisname, G_CALLBACK(thisfunc), params, _dw_signal_disconnect, 0);
   _dw_set_signal_handler_id(thiswindow, sigid, cid);
   DW_MUTEX_UNLOCK;
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void dw_signal_disconnect_by_name(HWND window, const char *signame)
{
   DWSignalList signal = _dw_findsignal(signame);
   void *thisfunc = signal.func;
   int z, count;
   int _dw_locked_by_me = FALSE;
   void **params = alloca(sizeof(void *) * 3);

   DW_MUTEX_LOCK;
   params[2] = _dw_find_signal_window(window, signame);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(params[2]), "_dw_sigcounter"));

   for(z=0;z<count;z++)
   {
      DWSignalHandler sh;

      params[0] = GINT_TO_POINTER(z);
      sh = _dw_get_signal_handler(params);

      if(sh.intfunc == thisfunc)
         _dw_remove_signal_handler((HWND)params[2], z);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void dw_signal_disconnect_by_window(HWND window)
{
   HWND thiswindow;
   int z, count;
   int _dw_locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   thiswindow = _dw_find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(thiswindow), "_dw_sigcounter"));

   for(z=0;z<count;z++)
      _dw_remove_signal_handler(thiswindow, z);
   g_object_set_data(G_OBJECT(thiswindow), "_dw_sigcounter", NULL);
   DW_MUTEX_UNLOCK;
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void dw_signal_disconnect_by_data(HWND window, void *data)
{
   int z, count;
   int _dw_locked_by_me = FALSE;
   void **params = alloca(sizeof(void *) * 3);

   DW_MUTEX_LOCK;
   params[2] = _dw_find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(params[2]), "_dw_sigcounter"));

   for(z=0;z<count;z++)
   {
      DWSignalHandler sh;

      params[0] = GINT_TO_POINTER(z);
      sh = _dw_get_signal_handler(params);

      if(sh.data == data)
         _dw_remove_signal_handler((HWND)params[2], z);
   }
   DW_MUTEX_UNLOCK;
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
   wchar_t *retval = NULL, *freeme;

   if(sizeof(wchar_t) == sizeof(gunichar))
      freeme = retval = (wchar_t *)g_utf8_to_ucs4(utf8string, -1, NULL, NULL, NULL);
   else if(sizeof(wchar_t) == sizeof(gunichar2))
      freeme = retval = (wchar_t *)g_utf8_to_utf16(utf8string, -1, NULL, NULL, NULL);
   if(retval)
   {
      retval = wcsdup(retval);
      g_free(freeme);
   }
   return retval;
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
   char *retval = NULL, *freeme;

   if(sizeof(wchar_t) == sizeof(gunichar))
      freeme = retval = g_ucs4_to_utf8((gunichar *)wstring, -1, NULL, NULL, NULL);
   else if(sizeof(wchar_t) == sizeof(gunichar2))
      freeme = retval = g_utf16_to_utf8((gunichar2 *)wstring, -1, NULL, NULL, NULL);
   if(retval)
   {
      retval = strdup(retval);
      g_free(freeme);
   }
   return retval;
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
#ifdef USE_WEBKIT
        case DW_FEATURE_HTML:
#endif
#ifdef USE_WEBKIT2
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
#endif
#if GLIB_CHECK_VERSION(2,40,0)
        case DW_FEATURE_NOTIFICATION:
#endif
#ifdef DW_INCLUDE_DEPRECATED
        case DW_FEATURE_MDI:
#endif
#if !GTK_CHECK_VERSION(3,14,0)
        case DW_FEATURE_CONTAINER_STRIPE:
#endif
        case DW_FEATURE_TASK_BAR:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_TREE:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_RENDER_SAFE:
            return _dw_render_safe_mode;
#ifdef GDK_WINDOWING_X11
        case DW_FEATURE_WINDOW_PLACEMENT:
        {
            int _dw_locked_by_me = FALSE, retval = DW_FEATURE_UNSUPPORTED;
            GdkDisplay *display;

            DW_MUTEX_LOCK;
            display = gdk_display_get_default();
           
            if(display && GDK_IS_X11_DISPLAY(display))
                retval = DW_FEATURE_ENABLED;
            DW_MUTEX_UNLOCK;
            return retval;
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
#ifdef USE_WEBKIT
        case DW_FEATURE_HTML:
#endif
#ifdef USE_WEBKIT2
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
#endif
#if GLIB_CHECK_VERSION(2,40,0)
        case DW_FEATURE_NOTIFICATION:
#endif
#ifdef DW_INCLUDE_DEPRECATED
        case DW_FEATURE_MDI:
#endif
#if !GTK_CHECK_VERSION(3,14,0)
        case DW_FEATURE_CONTAINER_STRIPE:
#endif
        case DW_FEATURE_TASK_BAR:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_TREE:
            return DW_ERROR_GENERAL;
#ifdef GDK_WINDOWING_X11
        case DW_FEATURE_WINDOW_PLACEMENT:
        {
            int _dw_locked_by_me = FALSE, retval = DW_FEATURE_UNSUPPORTED;
            GdkDisplay *display;

            DW_MUTEX_LOCK;
            display = gdk_display_get_default();
           
            if(display && GDK_IS_X11_DISPLAY(display))
                retval = DW_ERROR_GENERAL;
            DW_MUTEX_UNLOCK;
            return retval;
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
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}
