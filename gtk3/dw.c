/*
 * Dynamic Windows:
 *          A GTK like cross-platform GUI
 *          GTK3 forwarder module for portabilty.
 *
 * (C) 2000-2012 Brian Smith <brian@dbsoft.org>
 * (C) 2003-2011 Mark Hessling <mark@rexx.org>
 * (C) 2002 Nickolay V. Shmyrev <shmyrev@yandex.ru>
 */
#include "config.h"
#include "dw.h"
#include <string.h>
#include <stdlib.h>
#if !defined(GDK_WINDOWING_WIN32)
# include <sys/utsname.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <sys/mman.h>
#endif
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

#ifdef USE_WEBKIT
#include <webkit/webkit.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#include "gtk/messagebox_error.xpm"
#include "gtk/messagebox_warning.xpm"
#include "gtk/messagebox_information.xpm"
#include "gtk/messagebox_question.xpm"

/* These are used for resource management */
#if defined(DW_RESOURCES) && !defined(BUILD_DLL)
extern DWResources _resources;
#endif

/* ff = 255 = 1.0000
 * ee = 238 = 0.9333
 * cc = 204 = 0.8000
 * bb = 187 = 0.7333
 * aa = 170 = 0.6667
 * 77 = 119 = 0.4667
 * 00 = 0   = 0.0000
 */
GdkRGBA _colors[] =
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
char *image_exts[NUM_EXTS] =
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

pthread_key_t _dw_fg_color_key;
pthread_key_t _dw_bg_color_key;
pthread_key_t _dw_mutex_key;

GtkWidget *last_window = NULL, *popup = NULL;

static int _dw_ignore_click = 0, _dw_ignore_expand = 0, _dw_color_active = 0;
static pthread_t _dw_thread = (pthread_t)-1;

#define  DW_MUTEX_LOCK { if(pthread_self() != _dw_thread && !pthread_getspecific(_dw_mutex_key)) { gdk_threads_enter(); pthread_setspecific(_dw_mutex_key, (void *)1); _locked_by_me = TRUE; } }
#define  DW_MUTEX_UNLOCK { if(pthread_self() != _dw_thread && _locked_by_me == TRUE) { gdk_threads_leave(); pthread_setspecific(_dw_mutex_key, NULL); _locked_by_me = FALSE; } }

#define DEFAULT_SIZE_WIDTH 12
#define DEFAULT_SIZE_HEIGHT 6
#define DEFAULT_TITLEBAR_HEIGHT 22

#define _DW_TREE_TYPE_CONTAINER  1
#define _DW_TREE_TYPE_TREE       2
#define _DW_TREE_TYPE_LISTBOX    3
#define _DW_TREE_TYPE_COMBOBOX   4

/* Signal forwarder prototypes */
static gint _button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gint _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint _key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gint _generic_event(GtkWidget *widget, gpointer data);
static gint _configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gint _activate_event(GtkWidget *widget, gpointer data);
static gint _container_enter_event(GtkWidget *widget, GdkEventAny *event, gpointer data);
static gint _combobox_select_event(GtkWidget *widget, gpointer data);
static gint _expose_event(GtkWidget *widget, cairo_t *cr, gpointer data);
static gint _set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data);
static gint _tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _value_changed_event(GtkAdjustment *adjustment, gpointer user_data);
static gint _tree_select_event(GtkTreeSelection *sel, gpointer data);
static gint _tree_expand_event(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer data);
static gint _switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
static gint _column_click_event(GtkWidget *widget, gpointer data);

#ifdef USE_WEBKIT
/*
 * we need to add these equivalents from webkitwebview.h so we can refer to
 * our own pointers to functions (we don't link with the webkit libraries
 */
# define DW_WEBKIT_TYPE_WEB_VIEW            (_webkit_web_view_get_type())
# define DW_WEBKIT_WEB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), DW_WEBKIT_TYPE_WEB_VIEW, WebKitWebView))
WEBKIT_API GType (*_webkit_web_view_get_type)(void) = NULL;
WEBKIT_API void (*_webkit_web_view_load_html_string)(WebKitWebView *, const gchar *, const gchar *) = NULL;
WEBKIT_API void (*_webkit_web_view_open)(WebKitWebView *, const gchar *) = NULL;
WEBKIT_API GtkWidget *(*_webkit_web_view_new)(void) = NULL;
WEBKIT_API void (*_webkit_web_view_go_back)(WebKitWebView *) = NULL;
WEBKIT_API void (*_webkit_web_view_go_forward)(WebKitWebView *) = NULL;
WEBKIT_API void (*_webkit_web_view_reload)(WebKitWebView *) = NULL;
WEBKIT_API void (*_webkit_web_view_stop_loading)(WebKitWebView *) = NULL;
WEBKIT_API void (*_webkit_web_frame_print)(WebKitWebFrame *) = NULL;
WEBKIT_API WebKitWebFrame *(*_webkit_web_view_get_focused_frame)(WebKitWebView *) = NULL;
#endif

GObject *_DWObject = NULL;
char *_DWDefaultFont = NULL;
static char _dw_share_path[PATH_MAX+1] = { 0 };

typedef struct
{
   void *func;
   char name[30];

} SignalList;

typedef struct
{
   HWND window;
   void *func;
   gpointer data;
   gint cid;
   void *intfunc;

} SignalHandler;

#define SIGNALMAX 18

/* A list of signal forwarders, to account for paramater differences. */
static SignalList SignalTranslate[SIGNALMAX] = {
   { _configure_event,         DW_SIGNAL_CONFIGURE },
   { _key_press_event,         DW_SIGNAL_KEY_PRESS },
   { _button_press_event,      DW_SIGNAL_BUTTON_PRESS },
   { _button_release_event,    DW_SIGNAL_BUTTON_RELEASE },
   { _motion_notify_event,     DW_SIGNAL_MOTION_NOTIFY },
   { _delete_event,            DW_SIGNAL_DELETE },
   { _expose_event,            DW_SIGNAL_EXPOSE },
   { _activate_event,          "activate" },
   { _generic_event,           DW_SIGNAL_CLICKED },
   { _container_enter_event,   DW_SIGNAL_ITEM_ENTER },
   { _tree_context_event,      DW_SIGNAL_ITEM_CONTEXT },
   { _combobox_select_event,   DW_SIGNAL_LIST_SELECT },
   { _tree_select_event,       DW_SIGNAL_ITEM_SELECT },
   { _set_focus_event,         DW_SIGNAL_SET_FOCUS },
   { _value_changed_event,     DW_SIGNAL_VALUE_CHANGED },
   { _switch_page_event,       DW_SIGNAL_SWITCH_PAGE },
   { _column_click_event,      DW_SIGNAL_COLUMN_CLICK },
   { _tree_expand_event,       DW_SIGNAL_TREE_EXPAND }
};

/* Alignment flags */
#define DW_CENTER 0.5f
#define DW_LEFT 0.0f
#define DW_RIGHT 1.0f
#define DW_TOP 0.0f
#define DW_BOTTOM 1.0f

/* MDI Support Code */
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
static gboolean move_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean resize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean iconify_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean maximize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean kill_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data);

static void gtk_mdi_add(GtkContainer *container, GtkWidget *widget);
static void gtk_mdi_remove_true(GtkContainer *container, GtkWidget *widget);
static void gtk_mdi_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

static GtkMdiChild *get_child(GtkMdi *mdi, GtkWidget * widget);

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
   gtk_widget_override_background_color (mdi, GTK_STATE_NORMAL, &background);

   return mdi;
}

static void gtk_mdi_put(GtkMdi *mdi, GtkWidget *child_widget, gint x, gint y, GtkWidget *label)
{
   GtkMdiChild *child;

   GtkWidget *table;
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
   top_event_box = gtk_event_box_new ();
   bottom_event_box = gtk_event_box_new ();
   table = gtk_table_new (4, 7, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 1);
   gtk_table_set_col_spacings (GTK_TABLE (table), 1);
   gtk_table_set_row_spacing (GTK_TABLE (table), 3, 0);
   gtk_table_set_col_spacing (GTK_TABLE (table), 6, 0);
   gtk_table_set_row_spacing (GTK_TABLE (table), 2, 0);
   gtk_table_set_col_spacing (GTK_TABLE (table), 5, 0);

   for (i = 0; i < 3; i++)
   {
      button[i] = gtk_event_box_new ();
      gtk_widget_set_events (button[0], GDK_BUTTON_PRESS_MASK);
   }

   gdk_rgba_parse (&color, GTK_MDI_LABEL_BACKGROUND);

   gtk_widget_override_background_color (top_event_box, GTK_STATE_NORMAL, &color);
   gtk_widget_override_background_color (bottom_event_box, GTK_STATE_NORMAL, &color);
   gtk_widget_override_background_color (child_box, GTK_STATE_NORMAL, &color);
   for (i = GTK_STATE_NORMAL; i < GTK_STATE_ACTIVE; i++)
   {
      for (j = 0; j < 3; j++)
      {
         gtk_widget_override_background_color (button[j], i, &color);
      }
   }
   gdk_rgba_parse (&color, GTK_MDI_LABEL_FOREGROUND);
   gtk_widget_override_color (label, GTK_STATE_NORMAL, &color);
   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

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

   gtk_table_attach (GTK_TABLE (table), child_widget_box, 1, 6, 2, 3,
                 GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                 GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                 0, 0);
   gtk_table_attach (GTK_TABLE (table), top_event_box, 1, 2, 1, 2,
                 GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                 0,
                 0, 0);
   gtk_table_attach (GTK_TABLE (table), bottom_event_box, 6, 7, 3, 4,
                 0,
                 0,
                 0, 0);
   gtk_table_attach (GTK_TABLE (table), button[0], 2, 3, 1, 2,
                 0,
                 0,
                 0, 0);
   gtk_table_attach (GTK_TABLE (table), button[1], 3, 4, 1, 2,
                 0,
                 0,
                 0, 0);
   gtk_table_attach (GTK_TABLE (table), button[2], 4, 5, 1, 2,
                 0,
                 0,
                 0, 0);

   gtk_container_add (GTK_CONTAINER (child_box), table);

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
   gtk_widget_show (table);
   gtk_widget_show (top_event_box);
   gtk_widget_show (bottom_event_box);
   gtk_widget_show (child_widget_box);
   for (i = 0; i < 3; i++)
   {
      gtk_widget_show (button[i]);
   }

   cursor = gdk_cursor_new (GDK_HAND1);
   gtk_widget_realize (top_event_box);
   gdk_window_set_cursor (gtk_widget_get_window(top_event_box), cursor);
   cursor = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);
   gtk_widget_realize (bottom_event_box);
   gdk_window_set_cursor (gtk_widget_get_window(bottom_event_box), cursor);

   g_signal_connect (G_OBJECT (top_event_box), "event",
                 G_CALLBACK (move_child_callback),
                 child);
   g_signal_connect (G_OBJECT (bottom_event_box), "event",
                 G_CALLBACK (resize_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[0]), "button_press_event",
                 G_CALLBACK (iconify_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[1]), "button_press_event",
                 G_CALLBACK (maximize_child_callback),
                 child);
   g_signal_connect (G_OBJECT (button[2]), "button_press_event",
                 G_CALLBACK (kill_child_callback),
                 child);
}

static void gtk_mdi_move(GtkMdi *mdi, GtkWidget *widget, gint x, gint y)
{
   GtkMdiChild *child;

   g_return_if_fail(GTK_IS_MDI(mdi));
   g_return_if_fail(GTK_IS_WIDGET(widget));

   child = get_child(mdi, widget);
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

   child = get_child(mdi, widget);
   g_return_if_fail(child);

   *x = child->x;
   *y = child->y;
}

static void gtk_mdi_set_state(GtkMdi *mdi, GtkWidget *widget, GtkMdiChildState state)
{
   GtkMdiChild *child;

   g_return_if_fail (GTK_IS_MDI (mdi));
   g_return_if_fail (GTK_IS_WIDGET (widget));

   child = get_child (mdi, widget);
   g_return_if_fail (child);

   child->state = state;
   if (gtk_widget_get_visible(child->widget) && gtk_widget_get_visible(GTK_WIDGET(mdi)))
      gtk_widget_queue_resize(GTK_WIDGET(child->widget));
}

static void gtk_mdi_remove(GtkMdi *mdi, GtkWidget *widget)
{
   GtkMdiChild *child;

   g_return_if_fail (GTK_IS_MDI (mdi));
   child = get_child (mdi, widget);
   g_return_if_fail (child);
   gtk_mdi_remove_true (GTK_CONTAINER (mdi), child->widget);
}

static void gtk_mdi_realize(GtkWidget *widget)
{
   GtkMdi *mdi;
   GdkWindowAttr attributes;
   gint attributes_mask;
   GdkWindow *thiswindow;
   GtkStyleContext *context;

   mdi = GTK_MDI (widget);

   g_return_if_fail (widget != NULL);
   g_return_if_fail (GTK_IS_MDI (mdi));

   gtk_widget_set_realized(widget, TRUE);

   GtkAllocation allocation;
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

   if((context = gtk_widget_get_style_context(widget)))
      gtk_style_context_set_background (context, gtk_widget_get_window(widget));
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

   g_return_val_if_fail (widget != NULL, FALSE);
   g_return_val_if_fail (GTK_IS_MDI (widget), FALSE);
   g_return_val_if_fail (cr != NULL, FALSE);

   mdi = GTK_MDI (widget);
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

static gboolean move_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
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
         if (gdk_device_grab (gdk_event_get_device(event),
                          event->button.window,
                          GDK_OWNERSHIP_WINDOW,
                          FALSE,
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                          GDK_BUTTON_RELEASE_MASK,
                          NULL,
                          event->button.time) != GDK_GRAB_SUCCESS)
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

         gdk_device_ungrab (gdk_event_get_device(event), event->button.time);
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

static gboolean resize_child_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
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
         if (gdk_device_grab (gdk_event_get_device(event),
                          event->button.window,
                          GDK_OWNERSHIP_WINDOW,
                          FALSE,
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                          GDK_BUTTON_RELEASE_MASK,
                          NULL,
                          event->button.time) != GDK_GRAB_SUCCESS)
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

         gdk_device_ungrab (gdk_event_get_device(event), event->button.time);
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

static gboolean iconify_child_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
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

static gboolean maximize_child_callback (GtkWidget *widget, GdkEvent * event, gpointer data)
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

static gboolean kill_child_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GtkMdiChild *child;
   GtkMdi *mdi;

   child = (GtkMdiChild *) data;
   mdi = child->mdi;

   g_return_val_if_fail (GTK_IS_MDI (mdi), FALSE);

   gtk_mdi_remove_true (GTK_CONTAINER (mdi), child->widget);
   return FALSE;
}

static GtkMdiChild *get_child (GtkMdi *mdi, GtkWidget *widget)
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
static void *_findsigfunc(char *signame)
{
   int z;

   for(z=0;z<SIGNALMAX;z++)
   {
      if(strcasecmp(signame, SignalTranslate[z].name) == 0)
         return SignalTranslate[z].func;
   }
   return NULL;
}

static SignalHandler _get_signal_handler(GtkWidget *widget, gpointer data)
{
   int counter = GPOINTER_TO_INT(data);
   SignalHandler sh;
   char text[100];

   sprintf(text, "_dw_sigwindow%d", counter);
   sh.window = (HWND)g_object_get_data(G_OBJECT(widget), text);
   sprintf(text, "_dw_sigfunc%d", counter);
   sh.func = (void *)g_object_get_data(G_OBJECT(widget), text);
   sprintf(text, "_dw_intfunc%d", counter);
   sh.intfunc = (void *)g_object_get_data(G_OBJECT(widget), text);
   sprintf(text, "_dw_sigdata%d", counter);
   sh.data = g_object_get_data(G_OBJECT(widget), text);
   sprintf(text, "_dw_sigcid%d", counter);
   sh.cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));

   return sh;
}

static void _remove_signal_handler(GtkWidget *widget, int counter)
{
   char text[100];
   gint cid;

   sprintf(text, "_dw_sigcid%d", counter);
   cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));
   g_signal_handler_disconnect(G_OBJECT(widget), cid);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   sprintf(text, "_dw_sigwindow%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   sprintf(text, "_dw_sigfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   sprintf(text, "_dw_intfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   sprintf(text, "_dw_sigdata%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
}

static int _set_signal_handler(GtkWidget *widget, HWND window, void *func, gpointer data, void *intfunc)
{
   int counter = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_sigcounter"));
   char text[100];

   sprintf(text, "_dw_sigwindow%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)window);
   sprintf(text, "_dw_sigfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)func);
   sprintf(text, "_dw_intfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)intfunc);
   sprintf(text, "_dw_sigdata%d", counter);
   g_object_set_data(G_OBJECT(widget), text, (gpointer)data);

   counter++;
   g_object_set_data(G_OBJECT(widget), "_dw_sigcounter", GINT_TO_POINTER(counter));

   return counter - 1;
}

static void _set_signal_handler_id(GtkWidget *widget, int counter, gint cid)
{
   char text[100];

   sprintf(text, "_dw_sigcid%d", counter);
   g_object_set_data(G_OBJECT(widget), text, GINT_TO_POINTER(cid));
}

static gint _set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data)
{
   SignalHandler work = _get_signal_handler((GtkWidget *)window, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*setfocusfunc)(HWND, void *) = work.func;

      retval = setfocusfunc(work.window, work.data);
   }
   return retval;
}

static gint _button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = event->button;

      if(event->button == 3)
         mybutton = 2;
      else if(event->button == 2)
         mybutton = 3;

      retval = buttonfunc(work.window, event->x, event->y, mybutton, work.data);
   }
   return retval;
}

static gint _button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = event->button;

      if(event->button == 3)
         mybutton = 2;
      else if(event->button == 2)
         mybutton = 3;

      retval = buttonfunc(work.window, event->x, event->y, mybutton, work.data);
   }
   return retval;
}

static gint _motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
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

static gint _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*closefunc)(HWND, void *) = work.func;

      retval = closefunc(work.window, work.data);
   }
   return retval;
}

static gint _key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*keypressfunc)(HWND, char, int, int, void *) = work.func;

      retval = keypressfunc(work.window, *event->string, event->keyval,
                       event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK), work.data);
   }
   return retval;
}

static gint _generic_event(GtkWidget *widget, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*genericfunc)(HWND, void *) = work.func;

      retval = genericfunc(work.window, work.data);
   }
   return retval;
}

static gint _activate_event(GtkWidget *widget, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window && !_dw_ignore_click)
   {
      int (*activatefunc)(HWND, void *) = work.func;

      retval = activatefunc(popup ? popup : work.window, work.data);
      popup = NULL;
   }
   return retval;
}

static gint _configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*sizefunc)(HWND, int, int, void *) = work.func;

      retval = sizefunc(work.window, event->width, event->height, work.data);
   }
   return retval;
}

static gint _expose_event(GtkWidget *widget, cairo_t *cr, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   int retval = FALSE;

   if(work.window)
   {
      DWExpose exp;
      int (*exposefunc)(HWND, DWExpose *, void *) = work.func;

      exp.x = exp.y = 0;
      exp.width = gtk_widget_get_allocated_width(widget);
      exp.height = gtk_widget_get_allocated_height(widget);
      retval = exposefunc(work.window, &exp, work.data);
   }
   return retval;
}

static gint _combobox_select_event(GtkWidget *widget, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   static int _dw_recursing = 0;
   int retval = FALSE;

   if(_dw_recursing)
      return FALSE;

   if(work.window && GTK_IS_COMBO_BOX(widget))
   {
      GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

      if(store)
      {
         GtkTreeIter iter;
         GtkTreePath *path;

         _dw_recursing = 1;

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

         _dw_recursing = 0;
      }
   }
   return retval;
}

static gint _tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
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
                  gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, -1);
               }
               else
               {
                  gtk_tree_model_get(store, &iter, 0, &text, -1);
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
                        gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, -1);
                     }
                     else
                     {
                        gtk_tree_model_get(store, &iter, 0, &text, -1);
                     }
                  }
                  gtk_tree_path_free(path);
               }
            }
         }

         retval = contextfunc(work.window, text, event->x, event->y, work.data, itemdata);
      }
   }
   return retval;
}

static gint _tree_select_event(GtkTreeSelection *sel, gpointer data)
{
   GtkWidget *item = NULL, *widget = (GtkWidget *)gtk_tree_selection_get_tree_view(sel);
   int retval = FALSE;

   if(widget)
   {
      SignalHandler work = _get_signal_handler(widget, data);

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
               gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, 3, &item, -1);
               retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
            }
            else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
            {
               gtk_tree_model_get(store, &iter, 0, &text, -1);
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
                     gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, 3, &item, -1);
                     retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
                  }
                  else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
                  {
                     gtk_tree_model_get(store, &iter, 0, &text, -1);
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
      }
   }
   return retval;
}

static gint _tree_expand_event(GtkTreeView *widget, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
   SignalHandler work = _get_signal_handler((GtkWidget *)widget, data);
   int retval = FALSE;

   if(!_dw_ignore_expand && work.window)
   {
      int (*treeexpandfunc)(HWND, HTREEITEM, void *) = work.func;
      retval = treeexpandfunc(work.window, (HTREEITEM)iter, work.data);
   }
   return retval;
}

static gint _container_enter_event(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
   SignalHandler work = _get_signal_handler(widget, data);
   GdkEventKey *keyevent = (GdkEventKey *)event;
   GdkEventButton *buttonevent = (GdkEventButton *)event;
   int retval = FALSE;

   if(work.window)
   {
      /* Handle both key and button events together */
      if((event->type == GDK_2BUTTON_PRESS && buttonevent->button == 1) ||
         (event->type == GDK_KEY_PRESS && keyevent->keyval == VK_RETURN))
      {
         int (*contextfunc)(HWND, char *, void *) = work.func;
         char *text = NULL;

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
                     gtk_tree_model_get(store, &iter, 0, &text, -1);
                     retval = contextfunc(work.window, text, work.data);
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
int _get_logical_page(HWND handle, unsigned long pageid)
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


static gint _switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data)
{
   SignalHandler work = _get_signal_handler((GtkWidget *)notebook, data);
   int retval = FALSE;

   if(work.window)
   {
      int (*switchpagefunc)(HWND, unsigned long, void *) = work.func;
      retval = switchpagefunc(work.window, _get_logical_page(GTK_WIDGET(notebook), page_num), work.data);
   }
   return retval;
}

static gint _column_click_event(GtkWidget *widget, gpointer data)
{
   GtkWidget *tree = data;
   gint handlerdata = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tree), "_dw_column_click_id"));
   SignalHandler work;
   int retval = FALSE;

   if(handlerdata)
   {
      work = _get_signal_handler(tree, GINT_TO_POINTER(handlerdata-1));

      if(work.window)
      {
         int column_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_column"));
         int (*clickcolumnfunc)(HWND, int, void *) = work.func;
         retval = clickcolumnfunc(work.window, column_num, work.data);
      }
   }
   return retval;
}

static int _round_value(gfloat val)
{
   int newval = (int)val;

   if(val >= 0.5 + (gfloat)newval)
      newval++;

   return newval;
}

static gint _value_changed_event(GtkAdjustment *adjustment, gpointer data)
{
   int max = _round_value(gtk_adjustment_get_upper(adjustment));
   int val = _round_value(gtk_adjustment_get_value(adjustment));
   GtkWidget *slider = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_slider");
   GtkWidget *spinbutton = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_spinbutton");
   GtkWidget *scrollbar = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_scrollbar");

   if(g_object_get_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event"))
      return FALSE;

   if (slider || spinbutton || scrollbar)
   {
      SignalHandler work = _get_signal_handler((GtkWidget *)adjustment, data);

      if (work.window)
      {
         int (*valuechangedfunc)(HWND, int, void *) = work.func;

         if(slider && GTK_IS_VSCALE(slider))
            valuechangedfunc(work.window, (max - val) - 1,  work.data);
         else
            valuechangedfunc(work.window, val,  work.data);
      }
   }
   return FALSE;
}

static gint _default_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
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

static GdkPixbuf *_find_pixbuf(HICN icon, unsigned long *userwidth, unsigned long *userheight)
{
   char *data = NULL;
   int z, id = GPOINTER_TO_INT(icon);

   /* Quick dropout for non-handle */
   if(!icon)
      return NULL;

   if(id > 65535)
   {
      GdkPixbuf *icon_pixbuf = icon;

      if(userwidth)
         *userwidth = gdk_pixbuf_get_width(icon_pixbuf);
      if(userheight)
         *userheight = gdk_pixbuf_get_height(icon_pixbuf);

      return icon;
   }

   for(z=0;z<_resources.resource_max;z++)
   {
      if(_resources.resource_id[z] == id)
      {
         data = _resources.resource_data[z];
         break;
      }
   }

   if(data)
   {
      GdkPixbuf *icon_pixbuf;
      
      if(data[0] == 'G' && data[1] == 'd' && data[2] == 'k' && data[3] == 'P')
         icon_pixbuf = gdk_pixbuf_new_from_inline(-1, (const guint8 *)data, FALSE, NULL);
      else
         icon_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)data);

      if(userwidth)
         *userwidth = gdk_pixbuf_get_width(icon_pixbuf);
      if(userheight)
         *userheight = gdk_pixbuf_get_height(icon_pixbuf);

      return icon_pixbuf;
   }
   return NULL;
}

void _init_thread(void)
{
   GdkRGBA *foreground = malloc(sizeof(GdkRGBA));

   foreground->alpha = foreground->red = foreground->green = foreground->blue = 0.0;
   pthread_setspecific(_dw_fg_color_key, foreground);
   pthread_setspecific(_dw_bg_color_key, NULL);
}

/* Try to load the WebKitGtk shared libary */
#ifdef USE_WEBKIT
void init_webkit(void)
{
   char libname[100];
   void *handle = NULL;

   sprintf( libname, "lib%s.so", WEBKIT_LIB);
   handle = dlopen( libname, RTLD_LAZY );

   /* If we loaded it, grab the symbols we want */
   if ( handle )
   {
      _webkit_web_view_get_type = dlsym( handle, "webkit_web_view_get_type" );
      _webkit_web_view_load_html_string = dlsym( handle, "webkit_web_view_load_html_string" );
      _webkit_web_view_open = dlsym( handle, "webkit_web_view_open" );
      _webkit_web_view_new = dlsym( handle, "webkit_web_view_new" );
      _webkit_web_view_go_back = dlsym( handle, "webkit_web_view_go_back" );
      _webkit_web_view_go_forward = dlsym( handle, "webkit_web_view_go_forward" );
      _webkit_web_view_reload = dlsym( handle, "webkit_web_view_reload" );
      _webkit_web_view_stop_loading = dlsym( handle, "webkit_web_view_stop_loading" );
      _webkit_web_frame_print = dlsym( handle, "webkit_web_frame_print" );
      _webkit_web_view_get_focused_frame = dlsym( handle, "webkit_web_view_get_focused_frame" );
   }
}
#endif

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int dw_int_init(DWResources *res, int newthread, int *argc, char **argv[])
{
   if(res)
   {
      _resources.resource_max = res->resource_max;
      _resources.resource_id = res->resource_id;
      _resources.resource_data = res->resource_data;
   }
   
   /* Setup the private data directory */
   if(argc && argv && *argc > 0 && (*argv)[0])
   {
      char *pathcopy = strdup((*argv)[0]);
      char *pos = strrchr(pathcopy, '/');
      char *binname = pathcopy;
      
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
         char *binpos = strstr(pathcopy, "/bin");
         
         if(binpos)
            strncpy(_dw_share_path, pathcopy, (size_t)(binpos - pathcopy));
         else
            strcpy(_dw_share_path, "/usr/local");
         strcat(_dw_share_path, "/share/");
         strcat(_dw_share_path, binname);
      }
      if(pathcopy)
         free(pathcopy);
   }
   /* If that failed... just get the current directory */
   if(!_dw_share_path[0] && !getcwd(_dw_share_path, PATH_MAX))
      _dw_share_path[0] = '/';
   
#if !GLIB_CHECK_VERSION(2,31,0)
   g_thread_init(NULL);
#endif
   gdk_threads_init();

   gtk_init(argc, argv);

   pthread_key_create(&_dw_fg_color_key, NULL);
   pthread_key_create(&_dw_bg_color_key, NULL);
   pthread_key_create(&_dw_mutex_key, NULL);

   _init_thread();

   /* Create a global object for glib activities */
   _DWObject = g_object_new(G_TYPE_OBJECT, NULL);

#ifdef USE_WEBKIT
   init_webkit();
#endif

   return TRUE;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
   gdk_threads_enter();
   _dw_thread = pthread_self();
   gtk_main();
   _dw_thread = (pthread_t)-1;
   gdk_threads_leave();
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
         int _locked_by_me = FALSE;
         
         if(orig == (pthread_t)-1)
         {
            if(!pthread_getspecific(_dw_mutex_key))
            {
               gdk_threads_enter();
               pthread_setspecific(_dw_mutex_key, (void *)1);
               _locked_by_me = TRUE;
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
            if(_locked_by_me)
            {
               pthread_setspecific(_dw_mutex_key, NULL);
               gdk_threads_leave();
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
   int _locked_by_me = FALSE;
   
   if(_dw_thread == (pthread_t)-1)
   {
      if(!pthread_getspecific(_dw_mutex_key))
      {
         gdk_threads_enter();
         pthread_setspecific(_dw_mutex_key, (void *)1);
         _locked_by_me = TRUE;
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
      if(_locked_by_me)
      {
         pthread_setspecific(_dw_mutex_key, NULL);
         gdk_threads_leave();
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

   /* _dw_thread will be -1 if dw_main hasn't been run yet. */
   if(_dw_thread == (pthread_t)-1)
   {
      _dw_thread = pthread_self();
      newprocess = 1;
      gdk_threads_enter();
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
      gdk_threads_leave();
   }

   dw_event_close(&dialog->eve);
   tmp = dialog->result;
   free(dialog);
   return tmp;
}

static int _dw_ok_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;

   if(!dwwait)
      return FALSE;

   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, (void *)DW_MB_RETURN_OK);
   return FALSE;
}

int _dw_yes_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;

   if(!dwwait)
      return FALSE;

   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, (void *)DW_MB_RETURN_YES);
   return FALSE;
}

int _dw_no_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;

   if(!dwwait)
      return FALSE;

   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, (void *)DW_MB_RETURN_NO);
   return FALSE;
}

int _dw_cancel_func(HWND window, void *data)
{
   DWDialog *dwwait = (DWDialog *)data;

   if(!dwwait)
      return FALSE;

   dw_window_destroy((HWND)dwwait->data);
   dw_dialog_dismiss((DWDialog *)data, (void *)DW_MB_RETURN_CANCEL);
   return FALSE;
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
   
   fprintf(stderr, "%s", outbuf);
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: Defines buttons and icons to display
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int dw_messagebox(char *title, int flags, char *format, ...)
{
   HWND entrywindow, texttargetbox, imagetextbox, mainbox, okbutton, nobutton, yesbutton, cancelbutton, buttonbox, stext;
   ULONG flStyle = DW_FCF_TITLEBAR | DW_FCF_SHELLPOSITION | DW_FCF_SIZEBORDER;
   DWDialog *dwwait;
   va_list args;
   char outbuf[1025] = {0};
   char **xpm_data = NULL;
   int x, y, extra_width=0,text_width,text_height;
   int width,height;

   va_start(args, format);
   vsnprintf(outbuf, 1024, format, args);
   va_end(args);

   entrywindow = dw_window_new(HWND_DESKTOP, title, flStyle);
   mainbox = dw_box_new(DW_VERT, 10);
   dw_box_pack_start(entrywindow, mainbox, 0, 0, TRUE, TRUE, 0);

   /* determine if an icon is to be used - if so we need another HORZ box */
   if((flags & DW_MB_ERROR) | (flags & DW_MB_WARNING) | (flags & DW_MB_INFORMATION) | (flags & DW_MB_QUESTION))
   {
      imagetextbox = dw_box_new(DW_HORZ, 0);
      dw_box_pack_start(mainbox, imagetextbox, 0, 0, TRUE, TRUE, 2);
      texttargetbox = imagetextbox;
   }
   else
   {
      imagetextbox = NULL;
      texttargetbox = mainbox;
   }

   if(flags & DW_MB_ERROR)
      xpm_data = (char **)_dw_messagebox_error;
   else if(flags & DW_MB_WARNING)
      xpm_data = (char **)_dw_messagebox_warning;
   else if(flags & DW_MB_INFORMATION)
      xpm_data = (char **)_dw_messagebox_information;
   else if(flags & DW_MB_QUESTION)
      xpm_data = (char **)_dw_messagebox_question;

   if(xpm_data)
      extra_width = 32;

   if(texttargetbox == imagetextbox)
   {
      HWND handle = dw_bitmap_new( 100 );
      GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)xpm_data);

      gtk_image_set_from_pixbuf(GTK_IMAGE(handle), icon_pixbuf);

      dw_box_pack_start( texttargetbox, handle, 32, 32, FALSE, FALSE, 2);
   }

   /* Create text */
   text_width = 240;
   text_height = 0;
   stext = dw_text_new(outbuf, 0);
   dw_window_set_style(stext, DW_DT_WORDBREAK, DW_DT_WORDBREAK);
   dw_font_text_extents_get(stext, NULL, outbuf, &width, &height);

   text_width = min( width, dw_screen_width() - extra_width - 100 );
   text_height = min( height, dw_screen_height() );

   dw_box_pack_start(texttargetbox, stext, text_width, text_height, TRUE, TRUE, 2);

   /* Buttons */
   buttonbox = dw_box_new(DW_HORZ, 10);

   dw_box_pack_start(mainbox, buttonbox, 0, 0, TRUE, FALSE, 0);

   dwwait = dw_dialog_new((void *)entrywindow);

   /* which buttons ? */
   if(flags & DW_MB_OK)
   {
      okbutton = dw_button_new("Ok", 1001L);
      dw_box_pack_start(buttonbox, okbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(okbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_ok_func), (void *)dwwait);
   }
   else if(flags & DW_MB_OKCANCEL)
   {
      okbutton = dw_button_new("Ok", 1001L);
      dw_box_pack_start(buttonbox, okbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(okbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_ok_func), (void *)dwwait);
      cancelbutton = dw_button_new("Cancel", 1002L);
      dw_box_pack_start(buttonbox, cancelbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(cancelbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_cancel_func), (void *)dwwait);
   }
   else if(flags & DW_MB_YESNO)
   {
      yesbutton = dw_button_new("Yes", 1001L);
      dw_box_pack_start(buttonbox, yesbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(yesbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_yes_func), (void *)dwwait);
      nobutton = dw_button_new("No", 1002L);
      dw_box_pack_start(buttonbox, nobutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(nobutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_no_func), (void *)dwwait);
   }
   else if(flags & DW_MB_YESNOCANCEL)
   {
      yesbutton = dw_button_new("Yes", 1001L);
      dw_box_pack_start(buttonbox, yesbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(yesbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_yes_func), (void *)dwwait);
      nobutton = dw_button_new("No", 1002L);
      dw_box_pack_start(buttonbox, nobutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(nobutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_no_func), (void *)dwwait);
      cancelbutton = dw_button_new("Cancel", 1003L);
      dw_box_pack_start(buttonbox, cancelbutton, 50, 30, TRUE, FALSE, 2);
      dw_signal_connect(cancelbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_dw_cancel_func), (void *)dwwait);
   }

   height = max(50,text_height)+100;
   x = (dw_screen_width() - (text_width+60+extra_width))/2;
   y = (dw_screen_height() - height)/2;

   dw_window_set_pos_size(entrywindow, x, y, (text_width+60+extra_width), height);

   dw_window_show(entrywindow);

   return GPOINTER_TO_INT(dw_dialog_wait(dwwait));
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int dw_window_minimize(HWND handle)
{
   int _locked_by_me = FALSE;
   GtkWidget *mdi = NULL;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_ICONIFIED);
   }
   else
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
   GtkWidget *defaultitem;
   GtkWidget *mdi;

   if (!handle)
      return 0;

   DW_MUTEX_LOCK;
   gtk_widget_show(handle);
   if ((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_NORMAL);
   }
   else
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
         gdk_flush();
         gdk_window_show(window);
         gdk_flush();
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
   int _locked_by_me = FALSE;
   GtkWidget *mdi = NULL;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_set_state(GTK_MDI(mdi), handle, CHILD_ICONIFIED);
   }
   else
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
   int _locked_by_me = FALSE;
   GtkWidget *mdi = NULL;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_remove(GTK_MDI(mdi), handle);
   }
   if(GTK_IS_WIDGET(handle))
   {
      GtkWidget *eventbox = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_eventbox");

      if(eventbox && GTK_IS_WIDGET(eventbox))
         gtk_widget_destroy(eventbox);
      else
         gtk_widget_destroy(handle);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gdk_window_reparent(gtk_widget_get_window(GTK_WIDGET(handle)), newparent ? gtk_widget_get_window(GTK_WIDGET(newparent)) : gdk_get_default_root_window(), 0, 0);
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(char *fontname)
{
   char *oldfont = _DWDefaultFont;
   
   _DWDefaultFont = strdup(fontname);
   
   if(oldfont)
      free(oldfont);
}

/* Convert DW style font to pango style */
void _convert_font(char *font)
{
   char *name = strchr(font, '.');

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
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int dw_window_set_font(HWND handle, char *fontname)
{
   PangoFontDescription *pfont;
   GtkWidget *handle2 = handle;
   char *font = strdup(fontname);
   int _locked_by_me = FALSE;
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

   _convert_font(font);

   /* Free old font name if one is allocated */
   data = g_object_get_data(G_OBJECT(handle2), "_dw_fontname");
   g_object_set_data(G_OBJECT(handle2), "_dw_fontname", (gpointer)font);
   if(data)
      free(data);

   pfont = pango_font_description_from_string(fontname);

   if(pfont)
   {
      gtk_widget_override_font(handle2, pfont);
      pango_font_description_free(pfont);
   }
   DW_MUTEX_UNLOCK;
   return TRUE;
}

static int _dw_font_active = 0;

/* Internal function to handle the font OK press */
static gint _gtk_font_ok(GtkWidget *widget, DWDialog *dwwait)
{
   GtkFontSelectionDialog *fd;
   char *retfont = NULL;
   gchar *fontname;
   int len, x;

   if(!dwwait)
      return FALSE;

   fd = dwwait->data;
   fontname = gtk_font_selection_dialog_get_font_name(fd);
   if(fontname && (retfont = strdup(fontname)))
   {
      len = strlen(fontname);
      /* Convert to Dynamic Windows format if we can... */
      if(len > 0 && isdigit(fontname[len-1]))
      {
         int size;

         x=len-1;
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
      dw_free(fontname);
   }
   gtk_widget_destroy(GTK_WIDGET(fd));
   _dw_font_active = 0;
   dw_dialog_dismiss(dwwait, (void *)retfont);
   return FALSE;
}

/* Internal function to handle the font Cancel press */
static gint _gtk_font_cancel(GtkWidget *widget, DWDialog *dwwait)
{
   if(!dwwait)
      return FALSE;

   gtk_widget_destroy(GTK_WIDGET(dwwait->data));
   _dw_font_active = 0;
   dw_dialog_dismiss(dwwait, NULL);
   return FALSE;
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(char *currfont)
{
   GtkFontSelectionDialog *fd;
   GtkWidget *ok_button, *cancel_button;
   char *font = currfont ? strdup(currfont) : NULL;
   char *name = font ? strchr(font, '.') : NULL;
   int _locked_by_me = FALSE;
   char *retfont = NULL;
   DWDialog *dwwait;

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
   /* The DW mutex should be sufficient for
    * insuring no thread changes this unknowingly.
    */
   if(_dw_font_active)
   {
      DW_MUTEX_UNLOCK;
      if(font)
         free(font);
      return NULL;
   }
   fd = (GtkFontSelectionDialog *)gtk_font_selection_dialog_new("Choose font");
   if(font)
   {
      gtk_font_selection_dialog_set_font_name(fd, font);
      free(font);
   }

   _dw_font_active = 1;

   dwwait = dw_dialog_new((void *)fd);

   ok_button = gtk_font_selection_dialog_get_ok_button(fd);
   cancel_button = gtk_font_selection_dialog_get_cancel_button(fd);
   g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(_gtk_font_ok), dwwait);
   g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(_gtk_font_cancel), dwwait);

   gtk_widget_show(GTK_WIDGET(fd));

   retfont = (char *)dw_dialog_wait(dwwait);
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
   int _locked_by_me = FALSE;

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

void _free_gdk_colors(HWND handle)
{
   GdkRGBA *old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_foregdk");

   if(old)
      free(old);

   old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_backgdk");

   if(old)
      free(old);
}

/* Free old color pointers and allocate new ones */
static void _save_gdk_colors(HWND handle, GdkRGBA fore, GdkRGBA back)
{
   GdkRGBA *foregdk = malloc(sizeof(GdkRGBA));
   GdkRGBA *backgdk = malloc(sizeof(GdkRGBA));

   _free_gdk_colors(handle);

   *foregdk = fore;
   *backgdk = back;

   g_object_set_data(G_OBJECT(handle), "_dw_foregdk", (gpointer)foregdk);
   g_object_set_data(G_OBJECT(handle), "_dw_backgdk", (gpointer)backgdk);
}

static int _set_color(HWND handle, unsigned long fore, unsigned long back)
{
   /* Remember that each color component in X11 use 16 bit no matter
    * what the destination display supports. (and thus GDK)
    */
   GdkRGBA forecolor, backcolor;

   if(fore & DW_RGB_COLOR)
   {
      forecolor.alpha = 1.0;
      forecolor.red = (gdouble)DW_RED_VALUE(fore) / 255.0;
      forecolor.green = (gdouble)DW_GREEN_VALUE(fore) / 255.0;
      forecolor.blue = (gdouble)DW_BLUE_VALUE(fore) / 255.0;

      gtk_widget_override_color(handle, GTK_STATE_NORMAL, &forecolor);
      gtk_widget_override_color(handle, GTK_STATE_ACTIVE, &forecolor);
   }
   else if(fore != DW_CLR_DEFAULT)
   {
      forecolor = _colors[fore];

      gtk_widget_override_color(handle, GTK_STATE_NORMAL, &forecolor);
      gtk_widget_override_color(handle, GTK_STATE_ACTIVE, &forecolor);
   }
   if(back & DW_RGB_COLOR)
   {
      backcolor.alpha = 1.0;
      backcolor.red = (gdouble)DW_RED_VALUE(back) / 255.0;
      backcolor.green = (gdouble)DW_GREEN_VALUE(back) / 255.0;
      backcolor.blue = (gdouble)DW_BLUE_VALUE(back) / 255.0;

      gtk_widget_override_background_color(handle, GTK_STATE_NORMAL, &backcolor);
      gtk_widget_override_background_color(handle, GTK_STATE_ACTIVE, &backcolor);
   }
   else if(back != DW_CLR_DEFAULT)
   {
      backcolor = _colors[back];

      gtk_widget_override_background_color(handle, GTK_STATE_NORMAL, &backcolor);
      gtk_widget_override_background_color(handle, GTK_STATE_ACTIVE, &backcolor);
   }

   _save_gdk_colors(handle, forecolor, backcolor);

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
   int _locked_by_me = FALSE;

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
         handle2 = tmp;
   }

   _set_color(handle2, fore, back);

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

static GdkDeviceManager *manager = NULL;

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void dw_window_capture(HWND handle)
{
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   manager = gdk_display_get_device_manager(gtk_widget_get_display(handle));
   gdk_device_grab(gdk_device_manager_get_client_pointer(manager),
                   gtk_widget_get_window(handle),
                   GDK_OWNERSHIP_WINDOW,
                   FALSE, 
                   GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK, 
                   NULL, 
                   GDK_CURRENT_TIME);
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
   int _locked_by_me = FALSE;
   GdkCursor *cursor;

   DW_MUTEX_LOCK;
   if(pointertype > 65535)
   {
      GdkPixbuf  *pixbuf = _find_pixbuf(GINT_TO_POINTER(pointertype), NULL, NULL);
      cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), pixbuf, 8, 8);
   }
   else if(!pointertype)
      cursor = NULL;
   else
      cursor = gdk_cursor_new(pointertype);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gdk_device_ungrab(gdk_device_manager_get_client_pointer(manager), GDK_CURRENT_TIME);
   manager = NULL;
   DW_MUTEX_UNLOCK;
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
HWND dw_window_new(HWND hwndOwner, char *title, unsigned long flStyle)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;
   int flags = 0;

   DW_MUTEX_LOCK;
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
   {
      GtkWidget *box = dw_box_new(DW_VERT, 0);
      GtkWidget *grid = gtk_grid_new();

      gtk_widget_show_all(grid);
            
      last_window = tmp = gtk_window_new(GTK_WINDOW_TOPLEVEL);

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

      if(flStyle & DW_FCF_MAXIMIZE)
         gtk_window_maximize(GTK_WINDOW(tmp));
         
      if(flStyle & DW_FCF_MINIMIZE)
         gtk_window_iconify(GTK_WINDOW(tmp));
         
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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

   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp),box);
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
   int val = -1, _locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if (!handle)
      return -1;

   DW_MUTEX_LOCK;
   if ( orient == DW_HORZ )
      adjustment = gtk_scrolled_window_get_hadjustment( GTK_SCROLLED_WINDOW(handle) );
   else
      adjustment = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(handle) );
   if (adjustment)
      val = _round_value(gtk_adjustment_get_value(adjustment));
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
   int range = -1, _locked_by_me = FALSE;
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
      range = _round_value(gtk_adjustment_get_upper(adjustment));
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
HWND dw_groupbox_new(int type, int pad, char *title)
{
   GtkWidget *tmp, *frame;
   int _locked_by_me = FALSE;

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

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND dw_mdi_new(unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_mdi_new();
   gtk_widget_show(tmp);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_bitmap_new(unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;
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
      int _locked_by_me = FALSE;
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
 *       menu: The handle to the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND dw_menu_append_item(HMENUI menu, char *title, unsigned long id, unsigned long flags, int end, int check, HMENUI submenu)
{
   GtkWidget *tmphandle;
   char accel, *tempbuf = malloc(strlen(title)+1);
   int _locked_by_me = FALSE, submenucount;
   GtkAccelGroup *accel_group;

   if (!menu)
   {
      free(tempbuf);
      return NULL;
   }

   DW_MUTEX_LOCK;
   accel = _removetilde(tempbuf, title);

   accel_group = (GtkAccelGroup *)g_object_get_data(G_OBJECT(menu), "_dw_accel");
   submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "_dw_submenucount"));

   if (strlen(tempbuf) == 0)
      tmphandle=gtk_menu_item_new();
   else
   {
      char numbuf[11] = {0};

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
         snprintf(numbuf, 10, "%lu", id);
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
         snprintf(numbuf, 10, "%lu", id);
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

GtkWidget *_find_submenu_id(GtkWidget *start, char *name)
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
         if((menuitem = _find_submenu_id(submenu, name)))
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
   char numbuf[11];
   GtkWidget *tmphandle;
   int _locked_by_me = FALSE;

   if(!menu)
      return;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 10, "%lu", id);
   tmphandle = _find_submenu_id(menu, numbuf);

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
   char numbuf[11] = {0};
   GtkWidget *tmphandle;
   int check;
   int _locked_by_me = FALSE;

   if(!menu)
      return;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 10, "%lu", id);
   tmphandle = _find_submenu_id(menu, numbuf);

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
   char numbuf[11];
   GtkWidget *tmphandle;
   int _locked_by_me = FALSE;
   int ret = DW_ERROR_UNKNOWN;

   if(!menu)
      return ret;

   DW_MUTEX_LOCK;
   snprintf(numbuf, 10, "%lu", id);
   tmphandle = _find_submenu_id(menu, numbuf);

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
   int _locked_by_me = FALSE;

   if(!menu || !*menu)
      return;

   popup = parent;

   DW_MUTEX_LOCK;
   gtk_menu_popup(GTK_MENU(*menu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
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
   GdkModifierType state = 0;
   int gx, gy;
   int _locked_by_me = FALSE;
   GdkDisplay *display;
   GdkDeviceManager *manager;

   DW_MUTEX_LOCK;
#ifdef GDK_WINDOWING_X11
   display = gdk_display_get_default();
   manager = gdk_display_get_device_manager(display);
   gdk_window_get_device_position (gdk_x11_window_lookup_for_display(display, GDK_ROOT_WINDOW()), 
                                   gdk_device_manager_get_client_pointer(manager), &gx, &gy, &state);
#endif
   if(x)
      *x = gx;
   if(y)
      *y = gy;
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void dw_pointer_set_pos(long x, long y)
{
   int _locked_by_me = FALSE;
   GdkDisplay *display;
   GdkDeviceManager *manager;

   DW_MUTEX_LOCK;
#ifdef GDK_WINDOWING_X11
   display = gdk_display_get_default();
   manager = gdk_display_get_device_manager(display);
   gdk_device_warp( gdk_device_manager_get_client_pointer(manager), gdk_screen_get_default(), x, y );
#endif
   DW_MUTEX_UNLOCK;
}

#define _DW_TREE_CONTAINER 1
#define _DW_TREE_TREE      2
#define _DW_TREE_LISTBOX   3

GtkWidget *_tree_create(unsigned long id)
{
   GtkWidget *tmp;

   tmp = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp),
               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   gtk_widget_show(tmp);
   return tmp;
}

GtkWidget *_tree_setup(GtkWidget *tmp, GtkTreeModel *store)
{
   GtkWidget *tree = gtk_tree_view_new_with_model(store);
   gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp), tree);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _tree_create(id)))
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _tree_create(id)))
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   store = gtk_tree_store_new(4, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER, G_TYPE_POINTER);
   tree = _tree_setup(tmp, GTK_TREE_MODEL(store));
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
HWND dw_text_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_label_new(text);

   /* Left and centered */
   gtk_misc_set_alignment(GTK_MISC(tmp), 0.0f, 0.5f);
   gtk_widget_show(tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   gtk_misc_set_alignment(GTK_MISC(tmp), DW_LEFT, DW_LEFT);
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
HWND dw_status_text_new(char *text, ULONG id)
{
   GtkWidget *tmp, *frame;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   tmp = gtk_label_new(text);
   gtk_container_add(GTK_CONTAINER(frame), tmp);
   gtk_widget_show(tmp);
   gtk_widget_show(frame);

   /* Left and centered */
   gtk_misc_set_alignment(GTK_MISC(tmp), 0.0f, 0.5f);
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
   int _locked_by_me = FALSE;

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
HWND dw_entryfield_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
HWND dw_entryfield_password_new(char *text, ULONG id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
HWND dw_combobox_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   GtkListStore *store;
   int _locked_by_me = FALSE;

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
HWND dw_button_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
HWND dw_bitmapbutton_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   GtkWidget *bitmap;
   int _locked_by_me = FALSE;

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
HWND dw_bitmapbutton_new_from_file(char *text, unsigned long id, char *filename)
{
   GtkWidget *bitmap;
   GtkWidget *box;
   GtkWidget *label;
   GtkWidget *button;
   char *label_text=NULL;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;

   /* Create box for image and label */
   box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_container_set_border_width (GTK_CONTAINER (box), 2);

   /* Create a new button */
   button = gtk_button_new();
   /* Now on to the image stuff */
   bitmap = dw_bitmap_new(id);
   if(bitmap)
   {
      dw_window_set_bitmap( bitmap, 0, filename );
      /* Pack the image into the box */
      gtk_box_pack_start( GTK_BOX(box), bitmap, TRUE, FALSE, 3 );
      gtk_widget_show( bitmap );
      g_object_set_data(G_OBJECT(button), "_dw_bitmap", bitmap);
   }
   if(label_text)
   {
      /* Create a label for the button */
      label = gtk_label_new( label_text );
      /* Pack the label into the box */
      gtk_box_pack_start( GTK_BOX(box), label, TRUE, FALSE, 3 );
      gtk_widget_show( label );
   }

   /* Pack and show all our widgets */
   gtk_widget_show( box );
   gtk_container_add( GTK_CONTAINER(button), box );
   gtk_widget_show( button );
   if(text)
   {
      gtk_widget_set_tooltip_text(button, text);
   }
   g_object_set_data(G_OBJECT(button), "_dw_id", GINT_TO_POINTER(id));
   DW_MUTEX_UNLOCK;
   return button;
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
HWND dw_bitmapbutton_new_from_data(char *text, unsigned long id, char *data, int len)
{
   GtkWidget *tmp;
   GtkWidget *bitmap;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   tmp = gtk_button_new();
   bitmap = dw_bitmap_new(id);

   if ( bitmap )
   {
      dw_window_set_bitmap_from_data(bitmap, 0, data, len);
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
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_spinbutton_new(char *text, unsigned long id)
{
   GtkAdjustment *adj;
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
HWND dw_radiobutton_new(char *text, ULONG id)
{
   /* This will have to be fixed in the future. */
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, 0, 1, 1, 1);
   tmp = gtk_scrollbar_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, adjustment);
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
   int _locked_by_me = FALSE;

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
HWND dw_checkbox_new(char *text, unsigned long id)
{
   GtkWidget *tmp;
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(!(tmp = _tree_create(id)))
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   store = gtk_list_store_new(1, G_TYPE_STRING);
   tree = _tree_setup(tmp, GTK_TREE_MODEL(store));
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   icon_pixbuf = _find_pixbuf(icon, NULL, NULL);

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
 */
void dw_window_set_bitmap(HWND handle, unsigned long id, char *filename)
{
   GdkPixbuf *tmp = NULL;
   int found_ext = 0;
   int i;
   int _locked_by_me = FALSE;

   if(!id && !filename)
      return;

   DW_MUTEX_LOCK;
   if(id)
      tmp = _find_pixbuf((HICN)id, NULL, NULL);
   else
   {
      char *file = alloca(strlen(filename) + 6);

      if (!file)
      {
         DW_MUTEX_UNLOCK;
         return;
      }

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if ( access(file, 04 ) != 0 )
      {
         /* Try with various extentions */
         for ( i = 0; i < NUM_EXTS; i++ )
         {
            strcpy( file, filename );
            strcat( file, image_exts[i] );
            if ( access( file, 04 ) == 0 )
            {
               found_ext = 1;
               break;
            }
         }
         if ( found_ext == 0 )
         {
            DW_MUTEX_UNLOCK;
            return;
         }
      }
      tmp = gdk_pixbuf_new_from_file(file, NULL );
   }

   if (tmp)
   {
      if ( GTK_IS_BUTTON(handle) )
      {
         GtkWidget *image = gtk_button_get_image( GTK_BUTTON(handle) );
         gtk_image_set_from_pixbuf(GTK_IMAGE(image), tmp);
      }
      else
      {
         gtk_image_set_from_pixbuf(GTK_IMAGE(handle), tmp);
      }
   }
   DW_MUTEX_UNLOCK;
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
 */
void dw_window_set_bitmap_from_data(HWND handle, unsigned long id, char *data, int len)
{
   GdkPixbuf *tmp = NULL;
   int _locked_by_me = FALSE;
   char *file;
   FILE *fp;

   if (!id && !data)
      return;

   DW_MUTEX_LOCK;
   if (id)
      tmp = _find_pixbuf((HICN)id, NULL, NULL);
   else
   {
      if (!data)
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      /*
       * A real hack; create a temporary file and write the contents
       * of the data to the file
       */
      file = tmpnam( NULL );
      fp = fopen( file, "wb" );
      if ( fp )
      {
         fwrite( data, len, 1, fp );
         fclose( fp );
      }
      else
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      tmp = gdk_pixbuf_new_from_file(file, NULL );
      /* remove our temporary file */
      unlink (file );
   }

   if(tmp)
   {
      gtk_image_set_from_pixbuf(GTK_IMAGE(handle), tmp);
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associated with a given window.
 */
void dw_window_set_text(HWND handle, char *text)
{
   int _locked_by_me = FALSE;
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
void API dw_window_set_tooltip(HWND handle, char *bubbletext)
{
   int _locked_by_me = FALSE;

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
   const char *possible = "";
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_ENTRY(handle))
      possible = gtk_entry_get_text(GTK_ENTRY(handle));
   else if(GTK_IS_COMBO_BOX(handle))
      possible = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(handle))));

   DW_MUTEX_UNLOCK;
   return strdup(possible);
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void dw_window_disable(HWND handle)
{
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
unsigned int dw_mle_import(HWND handle, char *buffer, int startpoint)
{
   unsigned int tmppoint = startpoint;
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int length, _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void dw_mle_set_cursor(HWND handle, int point)
{
   int _locked_by_me = FALSE;

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
int dw_mle_search(HWND handle, char *text, int point, unsigned long flags)
{
   int _locked_by_me = FALSE, retval = 0;

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
   int _locked_by_me = FALSE;

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
   int val = 0, _locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      int max = _round_value(gtk_adjustment_get_upper(adjustment)) - 1;
      int thisval = _round_value(gtk_adjustment_get_value(adjustment));

      if(GTK_IS_VSCALE(handle))
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
   int _locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      int max = _round_value(gtk_adjustment_get_upper(adjustment)) - 1;

      if(GTK_IS_VSCALE(handle))
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
   int val = 0, _locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return 0;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
      val = _round_value(gtk_adjustment_get_value(adjustment));
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
   int _locked_by_me = FALSE;
   GtkAdjustment *adjustment;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
   if(adjustment)
   {
      g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event", GINT_TO_POINTER(1));
      gtk_adjustment_set_value(adjustment, (gfloat)position);
      g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event", GINT_TO_POINTER(0));
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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
HTREEITEM dw_tree_insert_after(HWND handle, HTREEITEM item, char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;
   int _locked_by_me = FALSE;

   if(!handle)
      return NULL;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

      pixbuf = _find_pixbuf(icon, NULL, NULL);

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
HTREEITEM dw_tree_insert(HWND handle, char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;
   int _locked_by_me = FALSE;

   if(!handle)
      return NULL;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

      pixbuf = _find_pixbuf(icon, NULL, NULL);

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
void dw_tree_item_change(HWND handle, HTREEITEM item, char *title, HICN icon)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   int _locked_by_me = FALSE;

   if(!handle)
      return;

   DW_MUTEX_LOCK;
   if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
      && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
   {
      pixbuf = _find_pixbuf(icon, NULL, NULL);

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
   char *text = NULL;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(!handle || !item)
      return text;

   DW_MUTEX_LOCK;
   tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   if(tree && GTK_IS_TREE_VIEW(tree) &&
      (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      gtk_tree_model_get(store, (GtkTreeIter *)item, 0, &text, -1);
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
   int _locked_by_me = FALSE;
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
      GtkTreeIter *p = malloc(sizeof(GtkTreeIter));

      if(gtk_tree_model_iter_parent(store, p, (GtkTreeIter *)item))
         parent = p;
      else
         free(p);
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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

static int _dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator, int extra)
{
   int z;
   char numbuf[21];
   GtkWidget *tree;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;
   int _locked_by_me = FALSE;
   GType *array = calloc(count + 2, sizeof(GType));

   DW_MUTEX_LOCK;
   /* Save some of the info so it is easily accessible */
   g_object_set_data(G_OBJECT(handle), "_dw_cont_columns", GINT_TO_POINTER(count));
   g_object_set_data(G_OBJECT(handle), "_dw_cont_extra", GINT_TO_POINTER(extra));

   /* First param is row title/data */
   array[0] = G_TYPE_POINTER;
   array[1] = G_TYPE_POINTER;
   /* First loop... create array to create the store */
   for(z=0;z<count;z++)
   {
      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         array[1] = GDK_TYPE_PIXBUF;
         array[2] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         array[z+2] = GDK_TYPE_PIXBUF;
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         array[z+2] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         array[z+2] = G_TYPE_ULONG;
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         array[z+2] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         array[z+2] = G_TYPE_STRING;
      }
   }
   /* Create the store and then the tree */
   store = gtk_list_store_newv(count+2, array);
   tree = _tree_setup(handle, GTK_TREE_MODEL(store));
   g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER));
   /* Second loop... create the columns */
   for(z=0;z<count;z++)
   {
      snprintf(numbuf, 20, "_dw_cont_col%d", z);
      g_object_set_data(G_OBJECT(tree), numbuf, GINT_TO_POINTER(flags[z]));
      col = gtk_tree_view_column_new();
      rend = NULL;

      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", 1);
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", 2);
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", z+2);
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+2);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+2);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+2);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+2);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      g_object_set_data(G_OBJECT(col), "_dw_column", GINT_TO_POINTER(z));
      g_signal_connect(G_OBJECT(col), "clicked", G_CALLBACK(_column_click_event), (gpointer)tree);
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

   newtitles[0] = "Filename";
   newflags[0] = DW_CFA_STRINGANDICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

   memcpy(&newtitles[1], titles, sizeof(char *) * count);
   memcpy(&newflags[1], flags, sizeof(unsigned long) * count);

   _dw_container_setup(handle, newflags, newtitles, count + 1, 1, 1);

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

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_file(char *filename)
{
   int _locked_by_me = FALSE;
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
         strcat( file, image_exts[i] );
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
   ret = gdk_pixbuf_new_from_file(file, NULL);
   DW_MUTEX_UNLOCK;
   return ret;
}

/*
 * Obtains an icon from data.
 * Parameters:
 *       data: Source of data for image.
 *       len:  length of data
 */
HICN API dw_icon_load_from_data(char *data, int len)
{
   int _locked_by_me = FALSE;
   char *file;
   FILE *fp;
   HICN ret = 0;

   /*
    * A real hack; create a temporary file and write the contents
    * of the data to the file
    */
   file = tmpnam( NULL );
   fp = fopen( file, "wb" );
   if ( fp )
   {
      fwrite( data, len, 1, fp );
      fclose( fp );
   }
   else
   {
      return 0;
   }
   DW_MUTEX_LOCK;
   ret = gdk_pixbuf_new_from_file(file, NULL);
   DW_MUTEX_UNLOCK;
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
   int _locked_by_me = FALSE;

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
   char numbuf[21], textbuffer[101] = {0};
   int flag = 0;
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      snprintf(numbuf, 20, "_dw_cont_col%d", column);
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
            GdkPixbuf *pixbuf = hicon ? _find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, 1, pixbuf, -1);
            gtk_list_store_set(store, &iter, 2, tmp, -1);
         }
         else if(flag & DW_CFA_BITMAPORICON)
         {
            HICN hicon = data ? *((HICN *)data) : 0;
            GdkPixbuf *pixbuf = hicon ? _find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, column + 2, pixbuf, -1);
         }
         else if(flag & DW_CFA_STRING)
         {
            char *tmp = data ? *((char **)data) : NULL;
            gtk_list_store_set(store, &iter, column + 2, tmp, -1);
         }
         else if(flag & DW_CFA_ULONG)
         {
            ULONG tmp = data ? *((ULONG *)data): 0;

            gtk_list_store_set(store, &iter, column + 2, tmp, -1);
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
            gtk_list_store_set(store, &iter, column + 2, textbuffer, -1);
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
            gtk_list_store_set(store, &iter, column + 2, textbuffer, -1);
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
void dw_filesystem_set_file(HWND handle, void *pointer, int row, char *filename, HICN icon)
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
   char numbuf[20];
   int flag, rc = 0;
   GtkWidget *cont = handle;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
   if(!cont)
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }

   snprintf(numbuf, 20, "_dw_cont_col%d", column);
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
   GtkWidget *cont;
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
void _dw_container_set_row_title(HWND handle, void *pointer, int row, char *title)
{
   GtkWidget *cont = handle;
   GtkListStore *store = NULL;
   int _locked_by_me = FALSE;

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
         gtk_list_store_set(store, &iter, 0, (gpointer)title, -1);
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
void dw_container_set_row_title(void *pointer, int row, char *title)
{
   _dw_container_set_row_title(pointer, pointer, row, title);
}

/*
 * Changes the title of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void dw_container_change_row_title(HWND handle, int row, char *title)
{
   _dw_container_set_row_title(handle, NULL, row, title);
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &retval, -1);
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
               gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &retval, -1);
            }
            gtk_tree_path_free(path);
         }
      }
      else
      {
         GtkTreeIter iter;

         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0))
         {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(1));
         }
      }
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
   int _locked_by_me = FALSE;

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
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &retval, -1);
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
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(pos+1));
         }
      }
   }
   DW_MUTEX_UNLOCK;
   return retval;
}

int _find_iter(GtkListStore *store, GtkTreeIter *iter, char *text, int textcomp)
{
   int z, rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
   char *thistext;

   for(z=0;z<rows;z++)
   {
      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, NULL, z))
      {
         gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &thistext, -1);
         if((textcomp && thistext && strcmp(thistext, text) == 0) || (!textcomp && thistext == text))
         {
            return TRUE;
         }
      }
   }
   return FALSE;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_cursor(HWND handle, char *text)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int textcomp, _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
   textcomp = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_textcomp"));

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      if(_find_iter(store, &iter, text, textcomp))
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
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void dw_container_delete_row(HWND handle, char *text)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   int textcomp, _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
   textcomp = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), "_dw_textcomp"));

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;
      int rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      if(_find_iter(store, &iter, text, textcomp))
      {
         gtk_list_store_remove(store, &iter);
         rows--;
      }

      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rows));
   }
   DW_MUTEX_UNLOCK;
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void dw_container_optimize(HWND handle)
{
   GtkWidget *cont;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         gtk_tree_view_columns_autosize(GTK_TREE_VIEW(cont));
   DW_MUTEX_UNLOCK;
}

/* Translate the status message into a message on our buddy window */
static void _status_translate(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
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
void dw_taskbar_insert(HWND handle, HICN icon, char *bubbletext)
{
   GtkStatusIcon *status;
   GdkPixbuf *pixbuf;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   pixbuf = _find_pixbuf(icon, NULL, NULL);
   status = gtk_status_icon_new_from_pixbuf(pixbuf);
   if(bubbletext)
      gtk_status_icon_set_tooltip_text(status, bubbletext);
   g_object_set_data(G_OBJECT(handle), "_dw_taskbar", status);
   g_signal_connect(G_OBJECT (status), "popup-menu", G_CALLBACK (_status_translate), handle);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   status = g_object_get_data(G_OBJECT(handle), "_dw_taskbar");
   g_object_unref(G_OBJECT(status));
   DW_MUTEX_UNLOCK;
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
   int _locked_by_me = FALSE;
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
   gtk_widget_set_can_focus(tmp, TRUE);
   gtk_widget_show(tmp);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_MUTEX_UNLOCK;
   return tmp;
}

/* Returns a GdkRGBA from a DW color */
static GdkRGBA _internal_color(unsigned long value)
{
   if(DW_RGB_COLOR & value)
   {
      GdkRGBA color = { (gfloat)DW_RED_VALUE(value) / 255.0, (gfloat)DW_GREEN_VALUE(value) / 255.0, (gfloat)DW_BLUE_VALUE(value) / 255.0, 1.0 };
      return color;
   }
   if (value < 16)
      return _colors[value];
   return _colors[0];
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void dw_color_foreground_set(unsigned long value)
{
   GdkRGBA color = _internal_color(value);
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
      GdkRGBA color = _internal_color(value);

      if(!background)
      {
         background = malloc(sizeof(GdkRGBA));
         pthread_setspecific(_dw_bg_color_key, background);
      }
      *background = color;
   }
}

/* Internal function to handle the color OK press */
static gint _gtk_color_ok(GtkWidget *widget, DWDialog *dwwait)
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
static gint _gtk_color_cancel(GtkWidget *widget, DWDialog *dwwait)
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
   int _locked_by_me = FALSE;
   DWDialog *dwwait;
   GtkColorSelection *colorsel;
   GdkRGBA color = _internal_color(value);
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
   g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(_gtk_color_ok), dwwait);
   g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(_gtk_color_cancel), dwwait);

   gtk_color_selection_set_previous_rgba(colorsel,&color);
   gtk_color_selection_set_current_rgba(colorsel,&color);
   gtk_color_selection_set_has_palette(colorsel,TRUE);

   gtk_widget_show(colorw);

   dw_color = (unsigned long)dw_dialog_wait(dwwait);
   if ((unsigned long)dw_color == -1)
      dw_color = value;
   DW_MUTEX_UNLOCK;
   return (unsigned long)dw_color;
/*
   dw_messagebox("Not implemented", DW_MB_OK|DW_MB_INFORMATION, "This feature not yet supported.");
   return value;
*/
}

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void dw_draw_point(HWND handle, HPIXMAP pixmap, int x, int y)
{
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
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
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
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
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int z;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
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
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
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
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
      GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);
      double dx = xorigin - x1;
      double dy = yorigin - y1;
      double r = sqrt(dx*dx + dy*dy);
      double a1 = atan2((y1-yorigin), (x1-xorigin));
      double a2 = atan2((y2-yorigin), (x2-xorigin));
      int width = x2-x1;
      float scale = (float)(y2-y1)/(float)width;
      
      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
         
      gdk_cairo_set_source_rgba(cr, foreground);
      cairo_set_line_width(cr, 1);
      if(scale != 1.0)
         cairo_scale(cr, 1.0, scale);
      if(flags & DW_DRAW_FULL)
         cairo_arc(cr, xorigin, yorigin / scale, (x2-x1)/2, 0, M_PI*2);
      else
         cairo_arc(cr, xorigin, yorigin, r, a1, a2);
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
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
void dw_draw_text(HWND handle, HPIXMAP pixmap, int x, int y, char *text)
{
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;
   PangoFontDescription *font;
   char *tmpname, *fontname = "monospace 10";

   if(!text)
      return;

   DW_MUTEX_LOCK;
   if(handle)
   {
      GdkWindow *window = gtk_widget_get_window(handle);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return;
      }
      cr = gdk_cairo_create(window);
      if((tmpname = (char *)g_object_get_data(G_OBJECT(handle), "_dw_fontname")))
         fontname = tmpname;
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
void dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, char *text, int *width, int *height)
{
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;
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
HPIXMAP dw_pixmap_new_from_file(HWND handle, char *filename)
{
   int _locked_by_me = FALSE;
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
         strcat( file, image_exts[i] );
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
HPIXMAP dw_pixmap_new_from_data(HWND handle, char *data, int len)
{
   int _locked_by_me = FALSE;
   char *file;
   FILE *fp;
   HPIXMAP pixmap;

   if (!data || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;

   DW_MUTEX_LOCK;
   /*
    * A real hack; create a temporary file and write the contents
    * of the data to the file
    */
   file = tmpnam( NULL );
   fp = fopen( file, "wb" );
   if ( fp )
   {
      fwrite( data, len, 1, fp );
      fclose( fp );
   }
   else
   {
      DW_MUTEX_UNLOCK;
      return 0;
   }
   pixmap->pixbuf = gdk_pixbuf_new_from_file(file, NULL);
   pixmap->image = cairo_image_surface_create_from_png(file);
   pixmap->width = gdk_pixbuf_get_width(pixmap->pixbuf);
   pixmap->height = gdk_pixbuf_get_height(pixmap->pixbuf);
   /* remove our temporary file */
   unlink (file );
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
   pixmap = pixmap;
   color = color;
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
   int _locked_by_me = FALSE;

   if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
      return NULL;


   DW_MUTEX_LOCK;
   pixmap->pixbuf = gdk_pixbuf_copy(_find_pixbuf((HICN)id, &pixmap->width, &pixmap->height));
   DW_MUTEX_UNLOCK;
   return pixmap;
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void dw_flush(void)
{
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
    if(pixmap && fontname && *fontname)
    {
         char *oldfont = pixmap->font;
         pixmap->font = strdup(fontname);
         _convert_font(pixmap->font);
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
void dw_pixmap_destroy(HPIXMAP pixmap)
{
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   g_object_unref(G_OBJECT(pixmap->pixbuf));
   cairo_surface_destroy(pixmap->image);
   if(pixmap->font)
      free(pixmap->font);
   free(pixmap);
   DW_MUTEX_UNLOCK;
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
   int _locked_by_me = FALSE;
   cairo_t *cr = NULL;
   int retval = DW_ERROR_GENERAL;

   if((!dest && (!destp || !destp->image)) || (!src && (!srcp || !srcp->image)))
      return retval;

   DW_MUTEX_LOCK;
   if(dest)
   {
      GdkWindow *window = gtk_widget_get_window(dest);
      /* Safety check for non-existant windows */
      if(!window || !GDK_IS_WINDOW(window))
      {
         DW_MUTEX_UNLOCK;
         return retval;
      }
      cr = gdk_cairo_create(window);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   gdk_beep();
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
int dw_module_load(char *name, HMOD *handle)
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
   /* If we are being called from an event handler we must release
    * the GTK mutex so we don't deadlock.
    */
   if(pthread_self() == _dw_thread)
      gdk_threads_leave();

   pthread_mutex_lock(mutex);

   /* And of course relock it when we have acquired the mutext */
   if(pthread_self() == _dw_thread)
      gdk_threads_enter();
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
      int result;

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
HEV dw_named_event_new(char *name)
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
   pthread_create(&dwthread, NULL, (void *)_handle_sem, (void *)tmpsock);
   return GINT_TO_POINTER(ev);
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
    * cleanup will continue in _handle_sem.
    */
   close(GPOINTER_TO_INT(eve));
   return DW_ERROR_NONE;
}

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
   void (*threadfunc)(void *) = NULL;
   void **tmp = (void **)data;
   GdkRGBA *foreground, *background;

   threadfunc = (void (*)(void *))tmp[0];

   /* Initialize colors */
   _init_thread();

   threadfunc(tmp[1]);
   free(tmp);

   /* Free colors */
   if((foreground = pthread_getspecific(_dw_fg_color_key)))
      free(foreground);
   if((background = pthread_getspecific(_dw_bg_color_key)))
      free(background);
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
   DWTID gtkthread;
   void **tmp = malloc(sizeof(void *) * 2);
   int rc;

   tmp[0] = func;
   tmp[1] = data;

   rc = pthread_create(&gtkthread, NULL, (void *)_dwthreadstart, (void *)tmp);
   if ( rc == 0 )
      return gtkthread;
   else
      return rc;
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
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void dw_exit(int exitcode)
{
   exit(exitcode);
}

/* Internal function to get the recommended size of scrolled items */
void _get_scrolled_size(GtkWidget *item, gint *thiswidth, gint *thisheight)
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
   int warn = FALSE, _locked_by_me = FALSE;
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
#if GTK_CHECK_VERSION(3,1,0)
         gtk_grid_insert_row(GTK_GRID(box), index);
#else
         #warning Dynamic Windows GTK3 support requires 3.1 or higher for full support.
#endif
         gtk_grid_attach(GTK_GRID(box), item, 0, index, 1, 1);
      }
      else
      {
#if GTK_CHECK_VERSION(3,1,0)
         gtk_grid_insert_column(GTK_GRID(box), index);
#endif         
         gtk_grid_attach(GTK_GRID(box), item, index, 0, 1, 1);
      }
      g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount + 1));
      /* Special case for scrolled windows */
      if(GTK_IS_SCROLLED_WINDOW(item))
      {
         gint scrolledwidth = 0, scrolledheight = 0;
         
         /* Pre-run the calculation code for MLE/Container/Tree if needed */
         if((width < 1 && !hsize) || (height < 1 && !vsize))
            _get_scrolled_size(item, &scrolledwidth, &scrolledheight);
         
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
 * Pack windows (widgets) into a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to be back.
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
 *       item: Window handle of the item to be back.
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
 *       item: Window handle of the item to be back.
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


union extents_union { guchar **gu_extents; unsigned long **extents; };
static GdkAtom extents_atom = 0;
static time_t extents_time = 0;

static gboolean _dw_property_notify(GtkWidget *window, GdkEventProperty* event, GdkWindow *gdkwindow)
{
   /* Check to see if we got a property change */
   if(event->state == GDK_PROPERTY_NEW_VALUE && event->atom == extents_atom && event->window == gdkwindow)
      extents_time = 0;
   return FALSE;
}

/* Internal function to figure out the frame extents of an unmapped window */
void _dw_get_frame_extents(GtkWidget *window, int *vert, int *horz)
{
   if(gtk_window_get_decorated(GTK_WINDOW(window)))
   {
      const char *request = "_NET_REQUEST_FRAME_EXTENTS";
      unsigned long *extents = NULL;
      union extents_union eu;
      GdkAtom request_extents = gdk_atom_intern(request, FALSE);
      GdkWindow *gdkwindow = gtk_widget_get_window(window);
      GdkDisplay *display = gtk_widget_get_display(window);

      if(!extents_atom)
         extents_atom = gdk_atom_intern("_NET_FRAME_EXTENTS", FALSE);

      /* Set some rational defaults.. just in case */
      *vert = 28;
      *horz = 12;

      /* See if the current window manager supports _NET_REQUEST_FRAME_EXTENTS */
      if(gdk_x11_screen_supports_net_wm_hint(gdk_screen_get_default(), request_extents))
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
         time(&extents_time);

         /* Look for the property notify event */
         do
         {
            dw_main_iteration();
            time(&currtime);
         }
         while(currtime - extents_time < 2);
         
         /* Remove the signal handler now that we are done */
         g_signal_handler_disconnect(G_OBJECT(window), connid);
      }
      
      /* Attempt to retrieve window's frame extents. */
      eu.extents = &extents;
      if(gdk_property_get(gdkwindow,
                          extents_atom,
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      gint scrolledwidth, scrolledheight;
      
      _get_scrolled_size(handle, &scrolledwidth, &scrolledheight);
      
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

/*
 * Returns the width of the screen.
 */
int dw_screen_width(void)
{
   int retval;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   retval = gdk_screen_width();
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Returns the height of the screen.
 */
int dw_screen_height(void)
{
   int retval;
   int _locked_by_me = FALSE;

   DW_MUTEX_UNLOCK;
   retval = gdk_screen_height();
   DW_MUTEX_UNLOCK;
   return retval;
}

/* This should return the current color depth */
unsigned long dw_color_depth_get(void)
{
   int retval;
   GdkVisual *vis;
   int _locked_by_me = FALSE;

   DW_MUTEX_UNLOCK;
   vis = gdk_visual_get_system();
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
   int _locked_by_me = FALSE;
   GtkWidget *mdi;

   if(!handle)
      return;
      
   DW_MUTEX_LOCK;
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_move(GTK_MDI(mdi), handle, x, y);
   }
   else
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
               newx += ((gdk_screen_width() / 2) - (width / 2));
            /* Handle right gravity */
            else if((horz & 0xf) == DW_GRAV_RIGHT)
               newx = gdk_screen_width() - width - x;
            /* Handle vertical center gravity */
            if((vert & 0xf) == DW_GRAV_CENTER)
               newy += ((gdk_screen_height() / 2) - (height / 2));
            else if((vert & 0xf) == DW_GRAV_BOTTOM)
               newy = gdk_screen_height() - height - y;

#if GTK_CHECK_VERSION(3,3,8)               
            /* Adjust the values to avoid Gnome bar if requested */
            if((horz | vert) & DW_GRAV_OBSTACLES)
            {
               GdkRectangle rect;
               GdkScreen *screen = gdk_screen_get_default();
               
               gdk_screen_get_monitor_workarea(screen, 0, &rect);
               
               if(horz & DW_GRAV_OBSTACLES)
               {
                  if((horz & 0xf) == DW_GRAV_LEFT)
                     newx += rect.x;
                  else if((horz & 0xf) == DW_GRAV_RIGHT)
                     newx -= dw_screen_width() - (rect.x + rect.width);
               }
               if(vert & DW_GRAV_OBSTACLES)
               {
                  if((vert & 0xf) == DW_GRAV_TOP)
                     newy += rect.y;
                  else if((vert & 0xf) == DW_GRAV_BOTTOM)
                     newy -= dw_screen_height() - (rect.y + rect.height);
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
   int _locked_by_me = FALSE;
   gint gx = 0, gy = 0, gwidth = 0, gheight = 0;
   GtkWidget *mdi;

   DW_MUTEX_LOCK;
   /* If it is an MDI window query what we can */
   if((mdi = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_mdi")) && GTK_IS_MDI(mdi))
   {
      gtk_mdi_get_pos(GTK_MDI(mdi), handle, &gx, &gy);
   }
   else
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
   int _locked_by_me = FALSE;

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
      gtk_misc_set_alignment( GTK_MISC(handle2), x, y );
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
   int _locked_by_me = FALSE;
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
int _get_physical_page(HWND handle, unsigned long pageid)
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
void dw_notebook_page_destroy(HWND handle, unsigned int pageid)
{
   int realpage, _locked_by_me = FALSE;
   GtkWidget **pagearray;

   DW_MUTEX_LOCK;
   realpage = _get_physical_page(handle, pageid);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   phys = gtk_notebook_get_current_page(GTK_NOTEBOOK(handle));
   retval = _get_logical_page(handle, phys);
   DW_MUTEX_UNLOCK;
   return retval;
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void dw_notebook_page_set(HWND handle, unsigned int pageid)
{
   int realpage, _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   realpage = _get_physical_page(handle, pageid);
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
void dw_notebook_page_set_text(HWND handle, unsigned long pageid, char *text)
{
   GtkWidget *child;
   int realpage, _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   realpage = _get_physical_page(handle, pageid);
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
void dw_notebook_page_set_status_text(HWND handle, unsigned long pageid, char *text)
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
   int num, z, realpage = -1, pad, _locked_by_me = FALSE;
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
void dw_listbox_append(HWND handle, char *text)
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
void dw_listbox_insert(HWND handle, char *text, int pos)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &text, -1);
            strncpy(buffer, text, length);
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
void dw_listbox_set_text(HWND handle, unsigned int index, char *buffer)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int _locked_by_me = FALSE;

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
            gtk_list_store_set(store, &iter, buffer);
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
gboolean _splitbar_set_percent(gpointer data)
{
   GtkWidget *widget = data;
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   if(percent)
   {
      GtkAllocation alloc;
      
      gtk_widget_get_allocation(widget, &alloc);
      
      if(GTK_IS_HPANED(widget))
         gtk_paned_set_position(GTK_PANED(widget), (int)(alloc.width * (*percent / 100.0)));
      if(GTK_IS_VPANED(widget))
         gtk_paned_set_position(GTK_PANED(widget), (int)(alloc.height * (*percent / 100.0)));
      g_object_set_data(G_OBJECT(widget), "_dw_percent", NULL);
      free(percent);
   }
   return FALSE;
}

/* Reposition the bar according to the percentage */
static gint _splitbar_size_allocate(GtkWidget *widget, GtkAllocation *event, gpointer data)
{
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   /* Prevent infinite recursion ;) */
   if(!percent || event->width < 20 || event->height < 20)
      return FALSE;

   g_idle_add(_splitbar_set_percent, widget);
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
   int _locked_by_me = FALSE;
   float *percent = malloc(sizeof(float));

   DW_MUTEX_LOCK;
   tmp = gtk_paned_new(type == DW_HORZ ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
   gtk_paned_pack1(GTK_PANED(tmp), topleft, TRUE, FALSE);
   gtk_paned_pack2(GTK_PANED(tmp), bottomright, TRUE, FALSE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   *percent = 50.0;
   g_object_set_data(G_OBJECT(tmp), "_dw_percent", (gpointer)percent);
   g_signal_connect(G_OBJECT(tmp), "size-allocate", G_CALLBACK(_splitbar_size_allocate), NULL);
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

   if(GTK_IS_VPANED(handle))
      size = gtk_widget_get_allocated_height(handle);
   else if(GTK_IS_HPANED(handle))
      size = gtk_widget_get_allocated_width(handle);

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
   int _locked_by_me = FALSE;
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
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

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
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 */
void dw_window_default(HWND window, HWND defaultitem)
{
   int _locked_by_me = FALSE;

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
   int _locked_by_me = FALSE;

   if(!window)
      return;

   DW_MUTEX_LOCK;
   g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(_default_key_press_event), next);
   DW_MUTEX_UNLOCK;
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
#ifdef VER_REV
   env->DWSubVersion = VER_REV;
#else
   env->DWSubVersion = DW_SUB_VERSION;
#endif

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
char *dw_file_browse(char *title, char *defpath, char *ext, int flags)
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
         button = GTK_STOCK_OPEN;
         break;
      case DW_FILE_OPEN:
         action = GTK_FILE_CHOOSER_ACTION_OPEN;
         button = GTK_STOCK_OPEN;
         break;
      case DW_FILE_SAVE:
         action = GTK_FILE_CHOOSER_ACTION_SAVE;
         button = GTK_STOCK_SAVE;
         break;
      default:
         dw_messagebox( "Coding error", DW_MB_OK|DW_MB_ERROR, "dw_file_browse() flags argument invalid.");
         return NULL;
         break;
   }

   filew = gtk_file_chooser_dialog_new ( title,
                                         NULL,
                                         action,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
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

   if((ret = fork()) == 0)
   {
      int i;

      for (i = 3; i < 256; i++)
         close(i);
      setsid();
      if(type == DW_EXEC_GUI)
      {
         execvp(program, params);
      }
      else if(type == DW_EXEC_CON)
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
   /* If possible load the URL/URI using gvfs... */
   if(gtk_show_uri(gdk_screen_get_default(), url, GDK_CURRENT_TIME, NULL))
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
      execargs[1] = url;
      execargs[2] = NULL;

      return dw_exec(browser, DW_EXEC_GUI, execargs);
   }
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void dw_html_action(HWND handle, int action)
{
#ifdef USE_WEBKIT
   int _locked_by_me = FALSE;
   WebKitWebView *web_view;
   WebKitWebFrame *frame;

   if (!_webkit_web_view_open)
      return;

   DW_MUTEX_LOCK;
   web_view = (WebKitWebView *)g_object_get_data(G_OBJECT(handle), "_dw_web_view");
   if ( web_view )
   {
      switch( action )
      {
         case DW_HTML_GOBACK:
            _webkit_web_view_go_back( web_view );
            break;
         case DW_HTML_GOFORWARD:
            _webkit_web_view_go_forward( web_view );
            break;
         case DW_HTML_GOHOME:
            _webkit_web_view_open( web_view, "http://dwindows.netlabs.org" );
            break;
         case DW_HTML_RELOAD:
            _webkit_web_view_reload( web_view );
            break;
         case DW_HTML_STOP:
            _webkit_web_view_stop_loading( web_view );
            break;
         case DW_HTML_PRINT:
            frame = _webkit_web_view_get_focused_frame( web_view );
            _webkit_web_frame_print( frame );
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
int dw_html_raw(HWND handle, char *string)
{
#ifdef USE_WEBKIT
   int _locked_by_me = FALSE;
   WebKitWebView *web_view;

   if (!_webkit_web_view_open)
      return -1;

   DW_MUTEX_LOCK;
   web_view = (WebKitWebView *)g_object_get_data(G_OBJECT(handle), "_dw_web_view");
   if ( web_view )
   {
      _webkit_web_view_load_html_string( web_view, string, "file:///");
      gtk_widget_show( GTK_WIDGET(handle) );
   }
   DW_MUTEX_UNLOCK;
   return 0;
#else
   return -1;
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
int dw_html_url(HWND handle, char *url)
{
#ifdef USE_WEBKIT
   int _locked_by_me = FALSE;
   WebKitWebView *web_view;

   if (!_webkit_web_view_open)
      return -1;

   DW_MUTEX_LOCK;
   web_view = (WebKitWebView *)g_object_get_data(G_OBJECT(handle), "_dw_web_view");
   if ( web_view )
   {
      _webkit_web_view_open( web_view, url );
      gtk_widget_show(GTK_WIDGET(handle));
   }
   DW_MUTEX_UNLOCK;
   return 0;
#else
   return -1;
#endif
}

#ifdef USE_WEBKIT
static void _dw_html_print_cb( GtkWidget *widget, gpointer *data )
{
   WebKitWebView *web_view = DW_WEBKIT_WEB_VIEW(data);
   WebKitWebFrame *frame;

   frame = _webkit_web_view_get_focused_frame( web_view );
   _webkit_web_frame_print( frame );
}
/*
 * Fired when the user right-clicks to get the popup-menu on the HTML widget
 * We add a "Print" menu item to enable printing
 * user_data is not used
 */
static void _dw_html_populate_popup_cb( WebKitWebView *web_view, GtkMenu *menu, gpointer user_data )
{
   GtkWidget *image = gtk_image_new_from_stock( GTK_STOCK_PRINT, GTK_ICON_SIZE_MENU );
   GtkWidget *item = gtk_image_menu_item_new_with_label( "Print" );

   gtk_image_menu_item_set_image( GTK_IMAGE_MENU_ITEM(item), image );
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
   g_signal_connect( item, "activate", G_CALLBACK(_dw_html_print_cb), web_view );
   gtk_widget_show(item);
}
#endif

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND dw_html_new(unsigned long id)
{
   GtkWidget *widget = NULL;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
#ifdef USE_WEBKIT
   if (!_webkit_web_view_open)
   {
      dw_debug( "HTML widget not available; you do not have access to webkit.\n" );
   }
   else
   {
      WebKitWebView *web_view;
      widget = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
      web_view = (WebKitWebView *)_webkit_web_view_new();
      /* web_view = WEBKIT_WEB_VIEW(_webkit_web_view_new() ); */
      gtk_container_add( GTK_CONTAINER (widget), GTK_WIDGET(web_view) );
      gtk_widget_show( GTK_WIDGET(web_view) );
      g_object_set_data(G_OBJECT(widget), "_dw_web_view", (gpointer)web_view);
      g_signal_connect( web_view, "populate-popup", G_CALLBACK(_dw_html_populate_popup_cb), NULL );
   }
   gtk_widget_show(widget);
#else
   dw_debug( "HTML widget not available; you do not have access to webkit.\n" );
#endif
   DW_MUTEX_UNLOCK;
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
   int _locked_by_me = FALSE;
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
void  dw_clipboard_set_text( char *str, int len )
{
   int _locked_by_me = FALSE;
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
HPRINT API dw_print_new(char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
   GtkPrintOperation *op;
   int _locked_by_me = FALSE;
   
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
   int _locked_by_me = FALSE;
   
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
   int _locked_by_me = FALSE;
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
void dw_window_set_data(HWND window, char *dataname, void *data)
{
   HWND thiswindow = window;
   int _locked_by_me = FALSE;

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
void *dw_window_get_data(HWND window, char *dataname)
{
   HWND thiswindow = window;
   int _locked_by_me = FALSE;
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
int API dw_timer_connect(int interval, void *sigfunc, void *data)
{
   int *tag, _locked_by_me = FALSE;
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
void API dw_timer_disconnect(int id)
{
   int _locked_by_me = FALSE;
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
static HWND _find_signal_window(HWND window, char *signame)
{
   HWND thiswindow = window;

   if(GTK_IS_SCROLLED_WINDOW(thiswindow))
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   else if(GTK_IS_VSCALE(thiswindow) || GTK_IS_HSCALE(thiswindow) ||
         GTK_IS_VSCROLLBAR(thiswindow) || GTK_IS_HSCROLLBAR(thiswindow) ||
         GTK_IS_SPIN_BUTTON(thiswindow))
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
void dw_signal_connect(HWND window, char *signame, void *sigfunc, void *data)
{
   void *thisfunc  = _findsigfunc(signame);
   char *thisname = signame;
   HWND thiswindow = window;
   int sigid, _locked_by_me = FALSE;
   gint cid;

   DW_MUTEX_LOCK;
   /*
    * If the window we are setting the signal on is a scrolled window we need to get
    * the "real" widget type. thiswindow is the "real" widget type
    */
   if (GTK_IS_SCROLLED_WINDOW(thiswindow))
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }

   if (strcmp(signame, DW_SIGNAL_EXPOSE) == 0)
   {
      thisname = "draw";
   }
   else if (GTK_IS_MENU_ITEM(thiswindow) && strcmp(signame, DW_SIGNAL_CLICKED) == 0)
   {
      thisname = "activate";
      thisfunc = _findsigfunc(thisname);
   }
   else if (GTK_IS_TREE_VIEW(thiswindow)  && strcmp(signame, DW_SIGNAL_ITEM_CONTEXT) == 0)
   {
      sigid = _set_signal_handler(thiswindow, window, sigfunc, data, thisfunc);
      cid = g_signal_connect(G_OBJECT(thiswindow), "button_press_event", G_CALLBACK(thisfunc), GINT_TO_POINTER(sigid));
      _set_signal_handler_id(thiswindow, sigid, cid);

      DW_MUTEX_UNLOCK;
      return;
   }
   else if ((GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_SELECT) == 0) ||
            (GTK_IS_COMBO_BOX(thiswindow) && strcmp(signame, DW_SIGNAL_LIST_SELECT) == 0))
   {
      GtkWidget *widget = thiswindow;

      thisname = "changed";

      sigid = _set_signal_handler(widget, window, sigfunc, data, thisfunc);
      if(GTK_IS_TREE_VIEW(thiswindow))
      {
         thiswindow = (GtkWidget *)gtk_tree_view_get_selection(GTK_TREE_VIEW(thiswindow));
         cid = g_signal_connect(G_OBJECT(thiswindow), thisname, G_CALLBACK(thisfunc), GINT_TO_POINTER(sigid));
      }
      else
      {
         cid = g_signal_connect(G_OBJECT(thiswindow), thisname, G_CALLBACK(_combobox_select_event), GINT_TO_POINTER(sigid));
      }
      _set_signal_handler_id(widget, sigid, cid);

      DW_MUTEX_UNLOCK;
      return;
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_TREE_EXPAND) == 0)
   {
      thisname = "row-expanded";
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_ENTER) == 0)
   {
      sigid = _set_signal_handler(thiswindow, window, sigfunc, data, _container_enter_event);
      cid = g_signal_connect(G_OBJECT(thiswindow), "key_press_event", G_CALLBACK(_container_enter_event), GINT_TO_POINTER(sigid));
      _set_signal_handler_id(thiswindow, sigid, cid);

      thisname = "button_press_event";
      thisfunc = _findsigfunc(DW_SIGNAL_ITEM_ENTER);
   }
   else if (GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_COLUMN_CLICK) == 0)
   {
      /* We don't actually need a signal handler here... just need to assign the handler ID
       * Since the handlers for the columns were already created in _dw_container_setup()
       */
      sigid = _set_signal_handler(thiswindow, window, sigfunc, data, _column_click_event);
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
#if 0
   else if (strcmp(signame, DW_SIGNAL_LOSE_FOCUS) == 0)
   {
      thisname = "focus-out-event";
      if(GTK_IS_COMBO_BOX(thiswindow))
         thiswindow = GTK_COMBO_BOX(thiswindow)->entry;
   }
#endif
   else if (GTK_IS_VSCALE(thiswindow) || GTK_IS_HSCALE(thiswindow) ||
         GTK_IS_VSCROLLBAR(thiswindow) || GTK_IS_HSCROLLBAR(thiswindow) ||
         GTK_IS_SPIN_BUTTON(thiswindow) )
   {
      thiswindow = (GtkWidget *)g_object_get_data(G_OBJECT(thiswindow), "_dw_adjustment");
   }
   else if (GTK_IS_NOTEBOOK(thiswindow) && strcmp(signame, DW_SIGNAL_SWITCH_PAGE) == 0)
   {
      thisname = "switch-page";
   }

   if (!thisfunc || !thiswindow)
   {
      DW_MUTEX_UNLOCK;
      return;
   }

   sigid = _set_signal_handler(thiswindow, window, sigfunc, data, thisfunc);
   cid = g_signal_connect(G_OBJECT(thiswindow), thisname, G_CALLBACK(thisfunc),GINT_TO_POINTER(sigid));
   _set_signal_handler_id(thiswindow, sigid, cid);
   DW_MUTEX_UNLOCK;
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void dw_signal_disconnect_by_name(HWND window, char *signame)
{
   HWND thiswindow;
   int z, count;
   void *thisfunc;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   thiswindow = _find_signal_window(window, signame);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(thiswindow), "_dw_sigcounter"));
   thisfunc  = _findsigfunc(signame);

   for(z=0;z<count;z++)
   {
      SignalHandler sh = _get_signal_handler(thiswindow, GINT_TO_POINTER(z));

      if(sh.intfunc == thisfunc)
         _remove_signal_handler(thiswindow, z);
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
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   thiswindow = _find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(thiswindow), "_dw_sigcounter"));

   for(z=0;z<count;z++)
      _remove_signal_handler(thiswindow, z);
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
   HWND thiswindow;
   int z, count;
   int _locked_by_me = FALSE;

   DW_MUTEX_LOCK;
   thiswindow = _find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(thiswindow), "_dw_sigcounter"));

   for(z=0;z<count;z++)
   {
      SignalHandler sh = _get_signal_handler(thiswindow, GINT_TO_POINTER(z));

      if(sh.data == data)
         _remove_signal_handler(thiswindow, z);
   }
   DW_MUTEX_UNLOCK;
}

