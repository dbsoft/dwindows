/*
 * Dynamic Windows:
 *          A GTK like implementation of the PM GUI
 *          GTK forwarder module for portabilty.
 *
 * (C) 2000-2002 Brian Smith <dbsoft@technologist.com>
 *
 */
#include "dw.h"
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "config.h"
#include <gdk/gdkkeysyms.h>
#ifdef USE_IMLIB
#include <gdk_imlib.h>
#endif
#if GTK_MAJOR_VERSION > 1
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

/* These are used for resource management */
#if defined(DW_RESOURCES) && !defined(BUILD_DLL)
extern DWResources _resources;
#endif

GdkColor _colors[] =
{
	{ 0, 0x0000, 0x0000, 0x0000 },	/* 0  black */
	{ 0, 0xbbbb, 0x0000, 0x0000 },	/* 1  red */
	{ 0, 0x0000, 0xbbbb, 0x0000 },	/* 2  green */
	{ 0, 0xaaaa, 0xaaaa, 0x0000 },	/* 3  yellow */
	{ 0, 0x0000, 0x0000, 0xcccc },	/* 4  blue */
	{ 0, 0xbbbb, 0x0000, 0xbbbb },	/* 5  magenta */
	{ 0, 0x0000, 0xbbbb, 0xbbbb },	/* 6  cyan */
	{ 0, 0xbbbb, 0xbbbb, 0xbbbb },	/* 7  white */
	{ 0, 0x7777, 0x7777, 0x7777 },	/* 8  grey */
	{ 0, 0xffff, 0x0000, 0x0000 },	/* 9  bright red */
	{ 0, 0x0000, 0xffff, 0x0000 },	/* 10 bright green */
	{ 0, 0xeeee, 0xeeee, 0x0000 },	/* 11 bright yellow */
	{ 0, 0x0000, 0x0000, 0xffff },	/* 12 bright blue */
	{ 0, 0xffff, 0x0000, 0xffff },	/* 13 bright magenta */
	{ 0, 0x0000, 0xeeee, 0xeeee },	/* 14 bright cyan */
	{ 0, 0xffff, 0xffff, 0xffff },	/* 15 bright white */
};

#define DW_THREAD_LIMIT 50

DWTID _dw_thread_list[DW_THREAD_LIMIT];
GdkColor _foreground[DW_THREAD_LIMIT];
GdkColor _background[DW_THREAD_LIMIT];

GtkWidget *last_window = NULL, *popup = NULL;

static int _dw_file_active = 0, _dw_ignore_click = 0, _dw_unselecting = 0;
static pthread_t _dw_thread = (pthread_t)-1;
static int _dw_mutex_locked[DW_THREAD_LIMIT];
/* Use default border size for the default enlightenment theme */
static int _dw_border_width = 12, _dw_border_height = 28;

#define  DW_MUTEX_LOCK { int index = _find_thread_index(dw_thread_id()); if(pthread_self() != _dw_thread && _dw_mutex_locked[index] == FALSE) { gdk_threads_enter(); _dw_mutex_locked[index] = TRUE; _locked_by_me = TRUE;  } }
#define  DW_MUTEX_UNLOCK { if(pthread_self() != _dw_thread && _locked_by_me == TRUE) { gdk_threads_leave(); _dw_mutex_locked[_find_thread_index(dw_thread_id())] = FALSE; _locked_by_me = FALSE; } }

#define DEFAULT_SIZE_WIDTH 12
#define DEFAULT_SIZE_HEIGHT 6
#define DEFAULT_TITLEBAR_HEIGHT 22

static GdkColormap *_dw_cmap = NULL;

/* Signal forwarder prototypes */
static gint _button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gint _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint _key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gint _generic_event(GtkWidget *widget, gpointer data);
static gint _configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gint _activate_event(GtkWidget *widget, gpointer data);
static gint _container_select_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _container_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _item_select_event(GtkWidget *widget, GtkWidget *child, gpointer data);
static gint _expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gint _set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data);
static gint _tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint _value_changed_event(GtkAdjustment *adjustment, gpointer user_data);
#if GTK_MAJOR_VERSION > 1
static gint _tree_select_event(GtkTreeSelection *sel, gpointer data);
#else
static gint _tree_select_event(GtkTree *tree, GtkWidget *child, gpointer data);
#endif

typedef struct
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	int used;
	unsigned long width, height;
} DWPrivatePixmap;

static DWPrivatePixmap *_PixmapArray = NULL;
static int _PixmapCount = 0;

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

} SignalHandler;

#define SIGNALMAX 16

/* A list of signal forwarders, to account for paramater differences. */
static SignalList SignalTranslate[SIGNALMAX] = {
	{ _configure_event, "configure_event" },
	{ _key_press_event, "key_press_event" },
	{ _button_press_event, "button_press_event" },
	{ _button_release_event, "button_release_event" },
	{ _motion_notify_event, "motion_notify_event" },
	{ _delete_event, "delete_event" },
	{ _expose_event, "expose_event" },
	{ _activate_event, "activate" },
	{ _generic_event, "clicked" },
	{ _container_select_event, "container-select" },
	{ _container_context_event, "container-context" },
	{ _tree_context_event, "tree-context" },
	{ _item_select_event, "item-select" },
	{ _tree_select_event, "tree-select" },
	{ _set_focus_event, "set-focus" },
	{ _value_changed_event, "value_changed" }
};

/* Alignment flags */
#define DW_CENTER 0.5f
#define DW_LEFT 0.0f
#define DW_RIGHT 1.0f

static void _dw_msleep(long period)
{
#ifdef __sun__
	/* usleep() isn't threadsafe on Solaris */
	struct timespec req;

	req.tv_sec = 0;
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

static gint _set_focus_event(GtkWindow *window, GtkWidget *widget, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*setfocusfunc)(HWND, void *) = work->func;

		retval = setfocusfunc((HWND)window, work->data);
	}
	return retval;
}

static gint _button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*buttonfunc)(HWND, int, int, int, void *) = work->func;
		int mybutton = event->button;

		if(event->button == 3)
			mybutton = 2;
		else if(event->button == 2)
			mybutton = 3;

		retval = buttonfunc(widget, event->x, event->y, mybutton, work->data);
	}
	return retval;
}

static gint _button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*buttonfunc)(HWND, int, int, int, void *) = work->func;
		int mybutton = event->button;

		if(event->button == 3)
			mybutton = 2;
		else if(event->button == 2)
			mybutton = 3;

		retval = buttonfunc(widget, event->x, event->y, mybutton, work->data);
	}
	return retval;
}

static gint _motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*motionfunc)(HWND, int, int, int, void *) = work->func;
		int keys = 0, x, y;
		GdkModifierType state;

		if (event->is_hint)
			gdk_window_get_pointer (event->window, &x, &y, &state);
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

		retval = motionfunc(widget, x, y, keys, work->data);
	}
	return retval;
}

static gint _delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*closefunc)(HWND, void *) = work->func;

		retval = closefunc(widget, work->data);
	}
	return retval;
}

static gint _key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*keypressfunc)(HWND, char, int, int, void *) = work->func;

		retval = keypressfunc(widget, *event->string, event->keyval,
							  event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK), work->data);
	}
	return retval;
}

static gint _generic_event(GtkWidget *widget, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*genericfunc)(HWND, void *) = work->func;

		retval = genericfunc(widget, work->data);
	}
	return retval;
}

static gint _activate_event(GtkWidget *widget, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work && !_dw_ignore_click)
	{
		int (*activatefunc)(HWND, void *) = work->func;

		retval = activatefunc(popup ? popup : work->window, work->data);
	}
	return retval;
}

static gint _configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*sizefunc)(HWND, int, int, void *) = work->func;

		retval = sizefunc(widget, event->width, event->height, work->data);
	}
	return retval;
}

static gint _expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		DWExpose exp;
		int (*exposefunc)(HWND, DWExpose *, void *) = work->func;

		exp.x = event->area.x;
		exp.y = event->area.y;
		exp.width = event->area.width;
		exp.height = event->area.height;
		retval = exposefunc(widget, &exp, work->data);
	}
	return retval;
}

static gint _item_select_event(GtkWidget *widget, GtkWidget *child, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	static int _dw_recursing = 0;
	int retval = FALSE;

	if(_dw_recursing)
		return FALSE;

	if(work)
	{
		int (*selectfunc)(HWND, int, void *) = work->func;
		GList *list;
		int item = 0;

		_dw_recursing = 1;

		if(GTK_IS_COMBO(work->window))
			list = GTK_LIST(GTK_COMBO(work->window)->list)->children;
		else if(GTK_IS_LIST(widget))
			list = GTK_LIST(widget)->children;
		else
			return FALSE;

		while(list)
		{
			if(list->data == (gpointer)child)
			{
				if(!gtk_object_get_data(GTK_OBJECT(work->window), "appending"))
				{
					gtk_object_set_data(GTK_OBJECT(work->window), "item", (gpointer)item);
					if(selectfunc)
						retval = selectfunc(work->window, item, work->data);
				}
				break;
			}
			item++;
			list = list->next;
		}
		_dw_recursing = 0;
	}
	return retval;
}

static gint _container_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		if(event->button == 3)
		{
			int (*contextfunc)(HWND, char *, int, int, void *) = work->func;
			char *text;
			int row, col;

			gtk_clist_get_selection_info(GTK_CLIST(widget), event->x, event->y, &row, &col);

			text = (char *)gtk_clist_get_row_data(GTK_CLIST(widget), row);
			retval = contextfunc(work->window, text, event->x, event->y, work->data);
		}
	}
	return retval;
}

static gint _tree_context_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		if(event->button == 3)
		{
#if GTK_MAJOR_VERSION > 1
			int (*contextfunc)(HWND, char *, int, int, void *, void *) = work->func;
			char *text = NULL;
			void *itemdata = NULL;

			if(widget && GTK_IS_TREE_VIEW(widget))
			{
				GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
				GtkTreeIter iter;
        
				if(sel && gtk_tree_selection_get_selected(sel, NULL, &iter))
				{
					GtkTreeModel *store = (GtkTreeModel *)gtk_object_get_data(GTK_OBJECT(widget), "_dw_tree_store");
					gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, -1);
				}
			}

			retval = contextfunc(work->window, text, event->x, event->y, work->data, itemdata);
#else
			int (*contextfunc)(HWND, char *, int, int, void *, void *) = work->func;
			char *text = (char *)gtk_object_get_data(GTK_OBJECT(widget), "text");
			void *itemdata = (void *)gtk_object_get_data(GTK_OBJECT(widget), "itemdata");

			if(widget != work->window)
			{
				GtkWidget *tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(work->window));

				if(tree && GTK_IS_TREE(tree))
				{
					GtkWidget *lastselect = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(tree), "lastselect");

					if(lastselect && GTK_IS_TREE_ITEM(lastselect))
					{
						text = (char *)gtk_object_get_data(GTK_OBJECT(lastselect), "text");
						itemdata = (void *)gtk_object_get_data(GTK_OBJECT(lastselect), "itemdata");
					}
				}
			}

			retval = contextfunc(work->window, text, event->x, event->y, work->data, itemdata);
#endif
		}
	}
	return retval;
}

#if GTK_MAJOR_VERSION > 1
static gint _tree_select_event(GtkTreeSelection *sel, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		int (*treeselectfunc)(HWND, HWND, char *, void *, void *) = work->func;
		GtkTreeIter iter;
		char *text = NULL;
		void *itemdata = NULL;
		GtkWidget *item, *widget = (GtkWidget *)gtk_tree_selection_get_tree_view(sel);
          
		if(widget && gtk_tree_selection_get_selected(sel, NULL, &iter))
		{
			GtkTreeModel *store = (GtkTreeModel *)gtk_object_get_data(GTK_OBJECT(widget), "_dw_tree_store");
			gtk_tree_model_get(store, &iter, 0, &text, 2, &itemdata, 3, &item, -1);
			retval = treeselectfunc(work->window, item, text, itemdata, work->data);
		}
	}
	return retval;
}
#else
static gint _tree_select_event(GtkTree *tree, GtkWidget *child, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	GtkWidget *treeroot = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(child), "tree");
	int retval = FALSE;

	if(treeroot && GTK_IS_TREE(treeroot))
	{
		GtkWidget *lastselect = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(treeroot), "lastselect");
		if(lastselect && GTK_IS_TREE_ITEM(lastselect))
			gtk_tree_item_deselect(GTK_TREE_ITEM(lastselect));
		gtk_object_set_data(GTK_OBJECT(treeroot), "lastselect", (gpointer)child);
	}

	if(work)
	{
		int (*treeselectfunc)(HWND, HWND, char *, void *, void *) = work->func;
		char *text = (char *)gtk_object_get_data(GTK_OBJECT(child), "text");
		void *itemdata = (char *)gtk_object_get_data(GTK_OBJECT(child), "itemdata");
		retval = treeselectfunc(work->window, child, text, itemdata, work->data);
	}
	return retval;
}
#endif

static gint _container_select_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	int retval = FALSE;

	if(work)
	{
		if(event->button == 1 && event->type == GDK_2BUTTON_PRESS)
		{
			int (*contextfunc)(HWND, char *, void *) = work->func;
			char *text;
			int row, col;

			gtk_clist_get_selection_info(GTK_CLIST(widget), event->x, event->y, &row, &col);

			text = (char *)gtk_clist_get_row_data(GTK_CLIST(widget), row);
			retval = contextfunc(work->window, text, work->data);
		}
	}
	return retval;
}

static gint _select_row(GtkWidget *widget, gint row, gint column, GdkEventButton *event, gpointer data)
{
	GList *tmp = (GList *)gtk_object_get_data(GTK_OBJECT(widget), "selectlist");
	char *rowdata = gtk_clist_get_row_data(GTK_CLIST(widget), row);
	int multi = (int)gtk_object_get_data(GTK_OBJECT(widget), "multi");

	if(rowdata)
	{
		if(!multi)
		{
			g_list_free(tmp);
			tmp = NULL;
		}

		tmp = g_list_append(tmp, rowdata);
		gtk_object_set_data(GTK_OBJECT(widget), "selectlist", tmp);
	}
	return FALSE;
}

static gint _container_select_row(GtkWidget *widget, gint row, gint column, GdkEventButton *event, gpointer data)
{
	SignalHandler *work = (SignalHandler *)data;
	char *rowdata = gtk_clist_get_row_data(GTK_CLIST(widget), row);
	int (*contextfunc)(HWND, char *, void *) = work->func;

	return contextfunc(work->window, rowdata, work->data);;
}

static gint _unselect_row(GtkWidget *widget, gint row, gint column, GdkEventButton *event, gpointer data)
{
	GList *tmp;
	char *rowdata;

	if(_dw_unselecting)
		return FALSE;

	tmp = (GList *)gtk_object_get_data(GTK_OBJECT(widget), "selectlist");
	rowdata = gtk_clist_get_row_data(GTK_CLIST(widget), row);

	if(rowdata && tmp)
	{
		tmp = g_list_remove(tmp, rowdata);
		gtk_object_set_data(GTK_OBJECT(widget), "selectlist", tmp);
	}
	return FALSE;
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
	SignalHandler *work = (SignalHandler *)data;

	if(work)
	{
		int (*valuechangedfunc)(HWND, int, void *) = work->func;
		int max = _round_value(adjustment->upper);
		int val = _round_value(adjustment->value);
		GtkWidget *slider = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(adjustment), "slider");
		GtkWidget *scrollbar = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(adjustment), "scrollbar");

		if(slider)
		{
			if(GTK_IS_VSCALE(slider))
				valuechangedfunc(work->window, (max - val) - 1,  work->data);
			else
				valuechangedfunc(work->window, val,  work->data);
		}
		else if(scrollbar)
		{
			valuechangedfunc(work->window, val,  work->data);
		}
	}
	return FALSE;
}

static gint _default_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GtkWidget *next = (GtkWidget *)data;

	if(next)
	{
		if(event->keyval == GDK_Return)
		{
			if(GTK_IS_BUTTON(next))
				gtk_signal_emit_by_name(GTK_OBJECT(next), "clicked");
			else
				gtk_widget_grab_focus(next);
		}
	}
	return FALSE;
}

static GdkPixmap *_find_private_pixmap(GdkBitmap **bitmap, long id, unsigned long *userwidth, unsigned long *userheight)
{
	if(id < _PixmapCount && _PixmapArray[id].used)
	{
		*bitmap = _PixmapArray[id].mask;
		if(userwidth)
			*userwidth = _PixmapArray[id].width;
		if(userheight)
			*userheight = _PixmapArray[id].height;
		return _PixmapArray[id].pixmap;
	}
	return NULL;
}

static GdkPixmap *_find_pixmap(GdkBitmap **bitmap, long id, HWND handle, unsigned long *userwidth, unsigned long *userheight)
{
	char *data = NULL;
	int z;

	if(id & (1 << 31))
		return _find_private_pixmap(bitmap, (id & 0xFFFFFF), userwidth, userheight);

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
		GdkPixmap *icon_pixmap = NULL;
#if GTK_MAJOR_VERSION > 1
		GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)data);

		if(userwidth)
			*userwidth = gdk_pixbuf_get_width(icon_pixbuf);
		if(userheight)
			*userheight = gdk_pixbuf_get_height(icon_pixbuf);
      
		gdk_pixbuf_render_pixmap_and_mask(icon_pixbuf, &icon_pixmap, bitmap, 1);
		g_object_unref(icon_pixbuf);
#elif defined(USE_IMLIB)
		gdk_imlib_data_to_pixmap((char **)data, &icon_pixmap, bitmap);
#else
		icon_pixmap = gdk_pixmap_create_from_xpm_d(handle->window, bitmap, &_colors[DW_CLR_PALEGRAY], (char **)data);
#endif
		return icon_pixmap;
	}
	return NULL;
}

#if GTK_MAJOR_VERSION > 1
static GdkPixbuf *_find_pixbuf(long id)
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
		return gdk_pixbuf_new_from_xpm_data((const char **)data);
	return NULL;
}
#endif

static void _size_allocate(GtkWindow *window)
{
  XSizeHints sizehints;

  sizehints.base_width = 1;
  sizehints.base_height = 1;
  sizehints.width_inc = 1;
  sizehints.height_inc = 1;
  sizehints.min_width = 8;
  sizehints.min_height = 8;
  
  sizehints.flags = (PBaseSize|PMinSize|PResizeInc);
  
  XSetWMNormalHints (GDK_DISPLAY(),
		     GDK_WINDOW_XWINDOW (GTK_WIDGET (window)->window),
		     &sizehints);
  gdk_flush ();
}

/* Find the index of a given thread */
static int _find_thread_index(DWTID tid)
{
	int z;

	for(z=0;z<DW_THREAD_LIMIT;z++)
	{
		if(_dw_thread_list[z] == tid)
			return z;
	}
	return 0;
}

/* Add a thread id to the thread list */
static void _dw_thread_add(DWTID tid)
{
	int z;

	for(z=0;z<DW_THREAD_LIMIT;z++)
	{
		if(_dw_thread_list[z] == tid)
			return;

		if(_dw_thread_list[z] == (DWTID)-1)
		{
			_dw_thread_list[z] = tid;
			_foreground[z].pixel = _foreground[z].red =_foreground[z].green = _foreground[z].blue = 0;
			_background[z].pixel = 0;
			_background[z].red = 0xaaaa;
			_background[z].green = 0xaaaa;
			_background[z].blue = 0xaaaa;
			return;
		}
	}
}

/* Remove a thread id to the thread list */
static void _dw_thread_remove(DWTID tid)
{
	int z;

	for(z=0;z<DW_THREAD_LIMIT;z++)
	{
		if(_dw_thread_list[z] == (DWTID)tid)
			_dw_thread_list[z] = (DWTID)-1;
	}
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int dw_int_init(DWResources *res, int newthread, int *argc, char **argv[])
{
	int z;
	char *tmp;

	if(res)
	{
		_resources.resource_max = res->resource_max;
		_resources.resource_id = res->resource_id;
		_resources.resource_data = res->resource_data;
	}
	gtk_set_locale();
	g_thread_init(NULL);
#if GTK_MAJOR_VERSION > 1
	gdk_threads_init();
#endif

	gtk_init(argc, argv);
#ifdef USE_IMLIB
	gdk_imlib_init();
#endif
	/* Add colors to the system colormap */
	_dw_cmap = gdk_colormap_get_system();
	for(z=0;z<16;z++)
		gdk_color_alloc(_dw_cmap, &_colors[z]);

	tmp = getenv("DW_BORDER_WIDTH");
	if(tmp)
		_dw_border_width = atoi(tmp);
	tmp = getenv("DW_BORDER_HEIGHT");
	if(tmp)
		_dw_border_height = atoi(tmp);

	for(z=0;z<DW_THREAD_LIMIT;z++)
		_dw_thread_list[z] = (DWTID)-1;

	gtk_rc_parse_string("style \"gtk-tooltips-style\" { bg[NORMAL] = \"#eeee00\" } widget \"gtk-tooltips\" style \"gtk-tooltips-style\"");

	return TRUE;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void dw_main(void)
{
	gdk_threads_enter();
	_dw_thread = pthread_self();
	_dw_thread_add(_dw_thread);
	gtk_main();
	_dw_thread = (pthread_t)-1;
	gdk_threads_leave();
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void dw_main_sleep(int milliseconds)
{
	struct timeval tv, start;

	gettimeofday(&start, NULL);

	if(_dw_thread == (pthread_t)-1 || _dw_thread == pthread_self())
	{
		gettimeofday(&tv, NULL);

		while(((tv.tv_sec - start.tv_sec)*1000) + ((tv.tv_usec - start.tv_usec)/1000) <= milliseconds)
		{
			gdk_threads_enter();
			if(gtk_events_pending())
				gtk_main_iteration();
			else
				_dw_msleep(1);
			gdk_threads_leave();
			gettimeofday(&tv, NULL);
		}
	}
	else
		_dw_msleep(milliseconds);
}

/*
 * Processes a single message iteration and returns.
 */
void dw_main_iteration(void)
{
	gdk_threads_enter();
	_dw_thread = pthread_self();
	_dw_thread_add(_dw_thread);
	gtk_main_iteration();
	_dw_thread = (pthread_t)-1;
	gdk_threads_leave();
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
int dw_dialog_dismiss(DWDialog *dialog, void *result)
{
	dialog->result = result;
	if(pthread_self() == _dw_thread || _dw_thread == (pthread_t)-1)
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
		gtk_main();
	else
		dw_event_wait(dialog->eve, -1);

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
	dw_dialog_dismiss((DWDialog *)data, (void *)0);
	return FALSE;
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int dw_messagebox(char *title, char *format, ...)
{
	HWND entrywindow, mainbox, okbutton, buttonbox, stext;
	ULONG flStyle = DW_FCF_TITLEBAR | DW_FCF_SHELLPOSITION | DW_FCF_DLGBORDER;
	DWDialog *dwwait;
	va_list args;
	char outbuf[256];
	int x, y;

	va_start(args, format);
	vsprintf(outbuf, format, args);
	va_end(args);

	entrywindow = dw_window_new(HWND_DESKTOP, title, flStyle);
	mainbox = dw_box_new(BOXVERT, 10);
	dw_box_pack_start(entrywindow, mainbox, 0, 0, TRUE, TRUE, 0);

	/* Archive Name */
	stext = dw_text_new(outbuf, 0);
	dw_window_set_style(stext, DW_DT_WORDBREAK, DW_DT_WORDBREAK);

	dw_box_pack_start(mainbox, stext, 205, 50, TRUE, TRUE, 2);

	/* Buttons */
	buttonbox = dw_box_new(BOXHORZ, 10);

	dw_box_pack_start(mainbox, buttonbox, 0, 0, TRUE, FALSE, 0);

	okbutton = dw_button_new("Ok", 1001L);

	dw_box_pack_start(buttonbox, okbutton, 50, 30, TRUE, FALSE, 2);

	dwwait = dw_dialog_new((void *)entrywindow);

	dw_signal_connect(okbutton, "clicked", DW_SIGNAL_FUNC(_dw_ok_func), (void *)dwwait);

	x = (dw_screen_width() - 220)/2;
	y = (dw_screen_height() - 110)/2;

	dw_window_set_pos_size(entrywindow, x, y, 220, 110);

	dw_window_show(entrywindow);

	dw_dialog_wait(dwwait);

	return strlen(outbuf);
}

int _dw_yes_func(HWND window, void *data)
{
	DWDialog *dwwait = (DWDialog *)data;

	if(!dwwait)
		return FALSE;

	dw_window_destroy((HWND)dwwait->data);
	dw_dialog_dismiss((DWDialog *)data, (void *)1);
	return FALSE;
}

int _dw_no_func(HWND window, void *data)
{
	DWDialog *dwwait = (DWDialog *)data;

	if(!dwwait)
		return FALSE;

	dw_window_destroy((HWND)dwwait->data);
	dw_dialog_dismiss((DWDialog *)data, (void *)0);
	return FALSE;
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           text: The text to display in the box.
 * Returns:
 *           True if YES False of NO.
 */
int dw_yesno(char *title, char *text)
{
	HWND entrywindow, mainbox, nobutton, yesbutton, buttonbox, stext;
	ULONG flStyle = DW_FCF_TITLEBAR | DW_FCF_SHELLPOSITION | DW_FCF_DLGBORDER;
	DWDialog *dwwait;
	int x, y;

	entrywindow = dw_window_new(HWND_DESKTOP, title, flStyle);

	mainbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(entrywindow, mainbox, 0, 0, TRUE, TRUE, 0);

	/* Archive Name */
	stext = dw_text_new(text, 0);
	dw_window_set_style(stext, DW_DT_WORDBREAK, DW_DT_WORDBREAK);

	dw_box_pack_start(mainbox, stext, 205, 50, TRUE, TRUE, 2);

	/* Buttons */
	buttonbox = dw_box_new(BOXHORZ, 10);

	dw_box_pack_start(mainbox, buttonbox, 0, 0, TRUE, FALSE, 0);

	yesbutton = dw_button_new("Yes", 1001L);

	dw_box_pack_start(buttonbox, yesbutton, 50, 30, TRUE, FALSE, 2);

	nobutton = dw_button_new("No", 1002L);

	dw_box_pack_start(buttonbox, nobutton, 50, 30, TRUE, FALSE, 2);

	dwwait = dw_dialog_new((void *)entrywindow);

	dw_signal_connect(yesbutton, "clicked", DW_SIGNAL_FUNC(_dw_yes_func), (void *)dwwait);
	dw_signal_connect(nobutton, "clicked", DW_SIGNAL_FUNC(_dw_no_func), (void *)dwwait);

	x = (dw_screen_width() - 220)/2;
	y = (dw_screen_height() - 110)/2;

	dw_window_set_pos_size(entrywindow, x, y, 220, 110);

	dw_window_show(entrywindow);

	return (int)dw_dialog_wait(dwwait);;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int dw_window_minimize(HWND handle)
{
	int _locked_by_me = FALSE;

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
	XIconifyWindow(GDK_WINDOW_XDISPLAY(GTK_WIDGET(handle)->window),
				   GDK_WINDOW_XWINDOW(GTK_WIDGET(handle)->window),
				   DefaultScreen (GDK_DISPLAY ()));
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
	gdk_window_raise(GTK_WIDGET(handle)->window);
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
	gdk_window_lower(GTK_WIDGET(handle)->window);
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

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
	gtk_widget_show(handle);
	if(GTK_WIDGET(handle)->window)
	{
		int width = (int)gtk_object_get_data(GTK_OBJECT(handle), "_dw_width");
		int height = (int)gtk_object_get_data(GTK_OBJECT(handle), "_dw_height");

		if(width && height)
		{
			gtk_widget_set_usize(handle, width, height);
			gtk_object_set_data(GTK_OBJECT(handle), "_dw_width", 0);
			gtk_object_set_data(GTK_OBJECT(handle), "_dw_height", 0);
		}

		gdk_window_raise(GTK_WIDGET(handle)->window);
		gdk_flush();
		gdk_window_show(GTK_WIDGET(handle)->window);
		gdk_flush();
	}
	defaultitem = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(handle), "_dw_defaultitem");
	if(defaultitem)
		gtk_widget_grab_focus(defaultitem);
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

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
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

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
	if(GTK_IS_WIDGET(handle))
		gtk_widget_destroy(handle);
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
	gdk_window_reparent(GTK_WIDGET(handle)->window, newparent ? GTK_WIDGET(newparent)->window : GDK_ROOT_PARENT(), 0, 0);
	DW_MUTEX_UNLOCK;
}

static int _set_font(HWND handle, char *fontname)
{
	int retval = FALSE;
#if GTK_MAJOR_VERSION < 2
	GtkStyle *style;
	GdkFont *font = NULL;

	font = gdk_font_load(fontname);

	if(font)
	{
		style = gtk_widget_get_style(handle);
		style->font = font;
		gtk_widget_set_style(handle, style);
		retval = TRUE;
	}
#else
	PangoFontDescription *font = pango_font_description_from_string(fontname);
  
	if(font)
	{
		gtk_widget_modify_font(handle, font);
		pango_font_description_free(font);
	}
#endif
	return retval;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int dw_window_set_font(HWND handle, char *fontname)
{
#if GTK_MAJOR_VERSION > 1
	PangoFontDescription *pfont;
#else
	GdkFont *gdkfont;
#endif
	GtkWidget *handle2 = handle;
	char *font;
	int _locked_by_me = FALSE;
	gpointer data;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	font = strdup(fontname);

#if GTK_MAJOR_VERSION < 2
	/* Free old font if it exists */
	gdkfont = (GdkFont *)gtk_object_get_data(GTK_OBJECT(handle2), "gdkfont");
	if(gdkfont)
		gdk_font_unref(gdkfont);
	gdkfont = gdk_font_load(fontname);
	if(!gdkfont)
		gdkfont = gdk_font_load("fixed");
	gtk_object_set_data(GTK_OBJECT(handle2), "gdkfont", (gpointer)gdkfont);
#endif

	/* Free old font name if one is allocated */
	data = gtk_object_get_data(GTK_OBJECT(handle2), "fontname");
	if(data)
		free(data);

	gtk_object_set_data(GTK_OBJECT(handle2), "fontname", (gpointer)font);
#if GTK_MAJOR_VERSION > 1
	pfont = pango_font_description_from_string(fontname);

	if(pfont)
	{
		gtk_widget_modify_font(handle2, pfont);
		pango_font_description_free(pfont);
	}
#endif
	DW_MUTEX_UNLOCK;
	return TRUE;
}

void _free_gdk_colors(HWND handle)
{
	GdkColor *old = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle), "foregdk");

	if(old)
		free(old);

	old = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle), "backgdk");

	if(old)
		free(old);
}

/* Free old color pointers and allocate new ones */
static void _save_gdk_colors(HWND handle, GdkColor fore, GdkColor back)
{
	GdkColor *foregdk = malloc(sizeof(GdkColor));
	GdkColor *backgdk = malloc(sizeof(GdkColor));

	_free_gdk_colors(handle);

	*foregdk = fore;
	*backgdk = back;

	gtk_object_set_data(GTK_OBJECT(handle), "foregdk", (gpointer)foregdk);
	gtk_object_set_data(GTK_OBJECT(handle), "backgdk", (gpointer)backgdk);
}

static int _set_color(HWND handle, unsigned long fore, unsigned long back)
{
	/* Remember that each color component in X11 use 16 bit no matter
	 * what the destination display supports. (and thus GDK)
	 */
	GdkColor forecolor, backcolor;
#if GTK_MAJOR_VERSION < 2
	GtkStyle *style = gtk_style_copy(gtk_widget_get_style(handle));
#endif

	if(fore & DW_RGB_COLOR)
	{
		forecolor.pixel = 0;
		forecolor.red = DW_RED_VALUE(fore) << 8;
		forecolor.green = DW_GREEN_VALUE(fore) << 8;
		forecolor.blue = DW_BLUE_VALUE(fore) << 8;

		gdk_color_alloc(_dw_cmap, &forecolor);

#if GTK_MAJOR_VERSION > 1
		gtk_widget_modify_text(handle, 0, &forecolor);
		gtk_widget_modify_text(handle, 1, &forecolor);
		gtk_widget_modify_fg(handle, 0, &forecolor);
		gtk_widget_modify_fg(handle, 1, &forecolor);
#else
		if(style)
			style->text[0] = style->text[1] = style->fg[0] = style->fg[1] = forecolor;
#endif
	}
	else if(fore != DW_CLR_DEFAULT)
	{
		forecolor = _colors[fore];

#if GTK_MAJOR_VERSION > 1
		gtk_widget_modify_text(handle, 0, &_colors[fore]);
		gtk_widget_modify_text(handle, 1, &_colors[fore]);
		gtk_widget_modify_fg(handle, 0, &_colors[fore]);
		gtk_widget_modify_fg(handle, 1, &_colors[fore]);
#else
		if(style)
			style->text[0] = style->text[1] = style->fg[0] = style->fg[1] = _colors[fore];
#endif
	}
	if(back & DW_RGB_COLOR)
	{
		backcolor.pixel = 0;
		backcolor.red = DW_RED_VALUE(back) << 8;
		backcolor.green = DW_GREEN_VALUE(back) << 8;
		backcolor.blue = DW_BLUE_VALUE(back) << 8;

		gdk_color_alloc(_dw_cmap, &backcolor);

#if GTK_MAJOR_VERSION > 1
		gtk_widget_modify_base(handle, 0, &backcolor);
		gtk_widget_modify_base(handle, 1, &backcolor);
		gtk_widget_modify_bg(handle, 0, &backcolor);
		gtk_widget_modify_bg(handle, 1, &backcolor);
#else
		if(style)
			style->base[0] = style->base[1] = style->bg[0] = style->bg[1] = backcolor;
#endif
	}
	else if(back != DW_CLR_DEFAULT)
	{
		backcolor = _colors[back];

#if GTK_MAJOR_VERSION > 1
		gtk_widget_modify_base(handle, 0, &_colors[back]);
		gtk_widget_modify_base(handle, 1, &_colors[back]);
		gtk_widget_modify_bg(handle, 0, &_colors[back]);
		gtk_widget_modify_bg(handle, 1, &_colors[back]);
#else
		if(style)
			style->base[0] = style->base[1] = style->bg[0] = style->bg[1] = _colors[back];
#endif
	}

	_save_gdk_colors(handle, forecolor, backcolor);

	if(GTK_IS_CLIST(handle))
	{
		int z, rowcount = (int)gtk_object_get_data(GTK_OBJECT(handle), "rowcount");

		for(z=0;z<rowcount;z++)
		{
			gtk_clist_set_foreground(GTK_CLIST(handle), z, &forecolor);
			gtk_clist_set_background(GTK_CLIST(handle), z, &backcolor);
		}
	}

#if GTK_MAJOR_VERSION < 2
	if(style)
	{
		gtk_widget_set_style(handle, style);
		gtk_style_unref(style);
	}
#endif
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
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_TABLE(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(handle), "eventbox");
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

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void dw_window_capture(HWND handle)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	gdk_pointer_grab(handle->window, TRUE, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK, NULL, NULL, GDK_CURRENT_TIME);
	DW_MUTEX_UNLOCK;
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void dw_window_pointer(HWND handle, int pointertype)
{
	int _locked_by_me = FALSE;
	GdkCursor *cursor;

	DW_MUTEX_LOCK;
	cursor = gdk_cursor_new(pointertype);
	if(handle && handle->window)
		gdk_window_set_cursor(handle->window, cursor);
	gdk_cursor_destroy(cursor);
	DW_MUTEX_UNLOCK;
}

/*
 * Releases previous mouse capture.
 */
void dw_window_release(void)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
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
	last_window = tmp = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(tmp), title);
	if(!(flStyle & DW_FCF_SIZEBORDER))
		gtk_window_set_policy(GTK_WINDOW(tmp), FALSE, FALSE, TRUE);

	gtk_widget_realize(tmp);

	if(flStyle & DW_FCF_TITLEBAR)
		flags |= GDK_DECOR_TITLE;

	if(flStyle & DW_FCF_MINMAX)
		flags |= GDK_DECOR_MINIMIZE | GDK_DECOR_MAXIMIZE;

	if(flStyle & DW_FCF_SIZEBORDER)
		flags |= GDK_DECOR_RESIZEH | GDK_DECOR_BORDER;

	if(flStyle & DW_FCF_BORDER || flStyle & DW_FCF_DLGBORDER)
		flags |= GDK_DECOR_BORDER;

	gdk_window_set_decorations(tmp->window, flags);

	if(hwndOwner)
		gdk_window_reparent(GTK_WIDGET(tmp)->window, GTK_WIDGET(hwndOwner)->window, 0, 0);

	if(flStyle & DW_FCF_SIZEBORDER)
		gtk_object_set_data(GTK_OBJECT(tmp), "_dw_size", (gpointer)1);

	DW_MUTEX_UNLOCK;
	return tmp;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either BOXVERT (vertical) or BOXHORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND dw_box_new(int type, int pad)
{
	GtkWidget *tmp, *eventbox;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_table_new(1, 1, FALSE);
	eventbox = gtk_event_box_new();

	gtk_widget_show(eventbox);
	gtk_object_set_data(GTK_OBJECT(tmp), "eventbox", (gpointer)eventbox);
	gtk_object_set_data(GTK_OBJECT(tmp), "boxtype", (gpointer)type);
	gtk_object_set_data(GTK_OBJECT(tmp), "boxpad", (gpointer)pad);
	gtk_widget_show(tmp);
	DW_MUTEX_UNLOCK;
	return tmp;
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either BOXVERT (vertical) or BOXHORZ (horizontal).
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
	tmp = gtk_table_new(1, 1, FALSE);
	gtk_container_border_width(GTK_CONTAINER(tmp), pad);
	gtk_object_set_data(GTK_OBJECT(tmp), "boxtype", (gpointer)type);
	gtk_object_set_data(GTK_OBJECT(tmp), "boxpad", (gpointer)pad);
	gtk_object_set_data(GTK_OBJECT(frame), "boxhandle", (gpointer)tmp);
	gtk_container_add(GTK_CONTAINER(frame), tmp);
	gtk_widget_show(tmp);
	gtk_widget_show(frame);
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
	tmp = gtk_vbox_new(FALSE, 0);
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
#if GTK_MAJOR_VERSION < 2
	GdkPixmap *pixmap = NULL;
	GdkBitmap *bitmap = NULL;
	static char * test_xpm[] = {
		"1 1 2 1",
		"	c None",
		".	c #FFFFFF",
		"."};
#endif
	GtkWidget *tmp;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	tmp = gtk_image_new();
#elif defined(USE_IMLIB)
	gdk_imlib_data_to_pixmap(test_xpm, &pixmap, &bitmap);
#else
	gtk_widget_realize(last_window);

	if(last_window)
		pixmap = gdk_pixmap_create_from_xpm_d(last_window->window, &bitmap, &_colors[DW_CLR_PALEGRAY], test_xpm);
#endif
#if GTK_MAJOR_VERSION < 2
	tmp = gtk_pixmap_new(pixmap, bitmap);
#endif
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
#if 0
	gtk_notebook_popup_enable(GTK_NOTEBOOK(tmp));
#endif
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_object_set_data(GTK_OBJECT(tmp), "pagearray", (gpointer)pagearray);
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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_object_set_data(GTK_OBJECT(tmp), "accel", (gpointer)accel_group);
	DW_MUTEX_UNLOCK;
	return tmp;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI dw_menubar_new(HWND location)
{
	GtkWidget *box;
	int _locked_by_me = FALSE;
	GtkAccelGroup *accel_group;
	HMENUI tmp;

	DW_MUTEX_LOCK;
	tmp = gtk_menu_bar_new();
	box = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(location));
	gtk_widget_show(tmp);
	accel_group = gtk_accel_group_new();
	gtk_object_set_data(GTK_OBJECT(tmp), "accel", (gpointer)accel_group);

	if(box)
		gtk_box_pack_end(GTK_BOX(box), tmp, FALSE, FALSE, 0);

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

		DW_MUTEX_LOCK;
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
 *       menu: The handle the the existing menu.
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
	guint tmp_key;
	GtkAccelGroup *accel_group;

	if(!menu)
	{
		free(tempbuf);
		return NULL;
	}

	DW_MUTEX_LOCK;
	accel = _removetilde(tempbuf, title);

	accel_group = (GtkAccelGroup *)gtk_object_get_data(GTK_OBJECT(menu), "accel");
	submenucount = (int)gtk_object_get_data(GTK_OBJECT(menu), "submenucount");

	if(strlen(tempbuf) == 0)
		tmphandle=gtk_menu_item_new();
	else
	{
		if(check)
		{
			char numbuf[10];
			if(accel && accel_group)
			{
				tmphandle=gtk_check_menu_item_new_with_label("");
				tmp_key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(tmphandle)->child), tempbuf);
#if 0 /* This isn't working right */
				gtk_widget_add_accelerator(tmphandle, "activate", accel_group, tmp_key, GDK_MOD1_MASK, 0);
#endif
			}
			else
				tmphandle=gtk_check_menu_item_new_with_label(tempbuf);
			gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(tmphandle), TRUE);
			sprintf(numbuf, "%lu", id);
			gtk_object_set_data(GTK_OBJECT(menu), numbuf, (gpointer)tmphandle);
		}
		else
		{
			if(accel && accel_group)
			{
				tmphandle=gtk_menu_item_new_with_label("");
				tmp_key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(tmphandle)->child), tempbuf);
#if 0 /* This isn't working right */
				gtk_widget_add_accelerator(tmphandle, "activate", accel_group, tmp_key, GDK_MOD1_MASK, 0);
#endif
			}
			else
				tmphandle=gtk_menu_item_new_with_label(tempbuf);
		}
	}

	gtk_widget_show(tmphandle);

	if(submenu)
	{
		char tempbuf[100];

		sprintf(tempbuf, "submenu%d", submenucount);
		submenucount++;
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(tmphandle), submenu);
		gtk_object_set_data(GTK_OBJECT(menu), tempbuf, (gpointer)submenu);
		gtk_object_set_data(GTK_OBJECT(menu), "submenucount", (gpointer)submenucount);
	}

	if(GTK_IS_MENU_BAR(menu))
		gtk_menu_bar_append(GTK_MENU_BAR(menu), tmphandle);
	else
		gtk_menu_append(GTK_MENU(menu), tmphandle);

	gtk_object_set_data(GTK_OBJECT(tmphandle), "id", (gpointer)id);
	free(tempbuf);
	DW_MUTEX_UNLOCK;
	return tmphandle;
}

GtkWidget *_find_submenu_id(GtkWidget *start, char *name)
{
	GtkWidget *tmp;
	int z, submenucount = (int)gtk_object_get_data(GTK_OBJECT(start), "submenucount");

	if((tmp = gtk_object_get_data(GTK_OBJECT(start), name)))
		return tmp;

	for(z=0;z<submenucount;z++)
	{
		char tempbuf[100];
		GtkWidget *submenu, *menuitem;

		sprintf(tempbuf, "submenu%d", z);

		if((submenu = gtk_object_get_data(GTK_OBJECT(start), tempbuf)))
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
 */
void dw_menu_item_set_check(HMENUI menu, unsigned long id, int check)
{
	char numbuf[10];
	GtkWidget *tmphandle;
	int _locked_by_me = FALSE;

	if(!menu)
		return;

	DW_MUTEX_LOCK;
	sprintf(numbuf, "%lu", id);
	tmphandle = _find_submenu_id(menu, numbuf);

	if(tmphandle)
	{
		_dw_ignore_click = 1;
		if(GTK_CHECK_MENU_ITEM(tmphandle)->active != check)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(tmphandle), check);
		_dw_ignore_click = 0;
	}
	DW_MUTEX_UNLOCK;
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
	GdkModifierType state;
	int gx, gy;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	gdk_window_get_pointer (GDK_ROOT_PARENT(), &gx, &gy, &state);
	*x = gx;
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

	DW_MUTEX_LOCK;
	XWarpPointer(GDK_DISPLAY(), None, GDK_ROOT_WINDOW(), 0,0,0,0, x, y);
	DW_MUTEX_UNLOCK;
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND dw_container_new(unsigned long id)
{
	GtkWidget *tmp;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tmp),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_widget_show(tmp);

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
#if GTK_MAJOR_VERSION > 1
	GtkTreeStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *rend;
	GtkTreeSelection *sel;
#endif
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_widget_show(tmp);
#if GTK_MAJOR_VERSION > 1
	store = gtk_tree_store_new(4, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER, G_TYPE_POINTER);
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_object_set_data(GTK_OBJECT(tree), "_dw_tree_store", (gpointer)store);
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
#else
	tree = gtk_tree_new();
#endif
	if(!tree)
	{
		gtk_widget_destroy(tmp);
		DW_MUTEX_UNLOCK;
		return FALSE;
	}
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp), tree);
#if GTK_MAJOR_VERSION < 2
	/* Set the selection mode */
	gtk_tree_set_selection_mode (GTK_TREE(tree), GTK_SELECTION_SINGLE);
	gtk_tree_set_view_mode(GTK_TREE(tree), GTK_TREE_VIEW_ITEM);
#endif

	gtk_object_set_user_data(GTK_OBJECT(tmp), (gpointer)tree);
	gtk_widget_show(tree);

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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_misc_set_alignment(GTK_MISC(tmp), DW_LEFT, DW_LEFT);
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
	gtk_object_set_data(GTK_OBJECT(frame), "id", (gpointer)id);
	gtk_object_set_data(GTK_OBJECT(frame), "label", (gpointer)tmp);
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
	GtkWidget *tmp, *tmpbox, *scroller;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	tmpbox = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(tmpbox),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tmpbox), GTK_SHADOW_ETCHED_IN);
	tmp = gtk_text_view_new();
	gtk_container_add (GTK_CONTAINER(tmpbox), tmp);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), GTK_WRAP_NONE);
	scroller = NULL;  
#else
	tmpbox = gtk_hbox_new(FALSE, 0);
	tmp = gtk_text_new(NULL, NULL);
	gtk_text_set_word_wrap(GTK_TEXT(tmp), FALSE);
	gtk_text_set_line_wrap(GTK_TEXT(tmp), FALSE);
	scroller = gtk_vscrollbar_new(GTK_TEXT(tmp)->vadj);
	GTK_WIDGET_UNSET_FLAGS(scroller, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(tmpbox), tmp, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tmpbox), scroller, FALSE, TRUE, 0);
	gtk_widget_show(scroller);
#endif
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_object_set_user_data(GTK_OBJECT(tmpbox), (gpointer)tmp);
	gtk_widget_show(tmp);
	gtk_widget_show(tmpbox);
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

	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);

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

	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);

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
	SignalHandler *work = malloc(sizeof(SignalHandler));
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_combo_new();
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(tmp)->entry), text);
	gtk_combo_set_use_arrows(GTK_COMBO(tmp), TRUE);
	gtk_object_set_user_data(GTK_OBJECT(tmp), NULL);
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);

	work->window = tmp;
	work->func = NULL;
	work->data = NULL;

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(tmp)->list), "select_child", GTK_SIGNAL_FUNC(_item_select_event), work);
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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
    GtkTooltips *tooltips;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_button_new();
	bitmap = dw_bitmap_new(id);

	if(bitmap)
	{
		dw_window_set_bitmap(bitmap, id, NULL);
		gtk_container_add (GTK_CONTAINER(tmp), bitmap);
	}
	gtk_widget_show(tmp);
	if(text)
	{
		tooltips = gtk_tooltips_new();
		gtk_tooltips_set_tip(tooltips, tmp, text, NULL);
		gtk_object_set_data(GTK_OBJECT(tmp), "tooltip", (gpointer)tooltips);
	}
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	DW_MUTEX_UNLOCK;
	return tmp;
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
HWND dw_bitmapbutton_new_from_file(char *text, unsigned long id, char filename)
{
	GtkWidget *tmp;
	GtkWidget *bitmap;
    GtkTooltips *tooltips;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_button_new();
	bitmap = dw_bitmap_new(id);

	if(bitmap)
	{
		dw_window_set_bitmap(bitmap, 0, filename);
		gtk_container_add (GTK_CONTAINER(tmp), bitmap);
	}
	gtk_widget_show(tmp);
	if(text)
	{
		tooltips = gtk_tooltips_new();
		gtk_tooltips_set_tip(tooltips, tmp, text, NULL);
		gtk_object_set_data(GTK_OBJECT(tmp), "tooltip", (gpointer)tooltips);
	}
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
	adj = (GtkAdjustment *)gtk_adjustment_new (1.0, 0.0, 100.0, 1.0, 5.0, 0.0);
	tmp = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(tmp), TRUE);
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	gtk_widget_show(tmp);

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
	if(vertical)
		tmp = gtk_vscale_new(adjustment);
	else
		tmp = gtk_hscale_new(adjustment);
	gtk_widget_show(tmp);
	gtk_scale_set_draw_value(GTK_SCALE(tmp), 0);
	gtk_scale_set_digits(GTK_SCALE(tmp), 0);
	gtk_object_set_data(GTK_OBJECT(tmp), "adjustment", (gpointer)adjustment);
	gtk_object_set_data(GTK_OBJECT(adjustment), "slider", (gpointer)tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
HWND dw_scrollbar_new(int vertical, int increments, ULONG id)
{
	GtkWidget *tmp;
	GtkAdjustment *adjustment;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, (gfloat)increments, 1, 1, 1);
	if(vertical)
		tmp = gtk_vscrollbar_new(adjustment);
	else
		tmp = gtk_hscrollbar_new(adjustment);
	GTK_WIDGET_UNSET_FLAGS(tmp, GTK_CAN_FOCUS);
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "adjustment", (gpointer)adjustment);
	gtk_object_set_data(GTK_OBJECT(adjustment), "scrollbar", (gpointer)tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
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
	GtkWidget *tmp, *list;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tmp = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	list = gtk_list_new();
	gtk_list_set_selection_mode(GTK_LIST(list), multi ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(tmp),
					      list);
	gtk_object_set_user_data(GTK_OBJECT(tmp), list);
	gtk_widget_show(list);
	gtk_widget_show(tmp);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);

	DW_MUTEX_UNLOCK;
	return tmp;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void dw_window_set_icon(HWND handle, unsigned long id)
{
	GdkBitmap *bitmap = NULL;
	GdkPixmap *icon_pixmap;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	icon_pixmap = _find_pixmap(&bitmap, id, handle, NULL, NULL);

	if(handle->window && icon_pixmap)
		gdk_window_set_icon(handle->window, NULL, icon_pixmap, bitmap);

	DW_MUTEX_UNLOCK;
}

HPIXMAP dw_pixmap_new_from_file(HWND handle, char *filename)
{
	int _locked_by_me = FALSE;
	HPIXMAP pixmap;
#ifndef USE_IMLIB
	GdkBitmap *bitmap = NULL;
#endif
#if GTK_MAJOR_VERSION > 1
	GdkPixbuf *pixbuf;
#elif defined(USE_IMLIB)
	GdkImlibImage *image;
#endif
	char *file = alloca(strlen(filename) + 5);

	if (!file || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
		return NULL;

	strcpy(file, filename);

	/* check if we can read from this file (it exists and read permission) */
	if(access(file, 04) != 0)
	{
		/* Try with .xpm extention */
		strcat(file, ".xpm");
		if(access(file, 04) != 0)
		{
			free(pixmap);
			return NULL;
		}
	}

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	pixbuf = gdk_pixbuf_new_from_file(file, NULL);

	pixmap->width = gdk_pixbuf_get_width(pixbuf);
	pixmap->height = gdk_pixbuf_get_height(pixbuf);

	gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap->pixmap, &bitmap, 1);
	g_object_unref(pixbuf);
#elif defined(USE_IMLIB)
	image = gdk_imlib_load_image(file);

	pixmap->width = image->rgb_width;
	pixmap->height = image->rgb_height;

	gdk_imlib_render(image, pixmap->width, pixmap->height);
	pixmap->pixmap = gdk_imlib_copy_image(image);
	gdk_imlib_destroy_image(image);
#else
	pixmap->pixmap = gdk_pixmap_create_from_xpm(handle->window, &bitmap, &_colors[DW_CLR_PALEGRAY], file);
#endif
	pixmap->handle = handle;
	DW_MUTEX_UNLOCK;
	return pixmap;
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
	GdkBitmap *bitmap = NULL;
	GdkPixmap *tmp;
	int _locked_by_me = FALSE;

	if(!id && !filename)
		return;

	DW_MUTEX_LOCK;
	if(id)
		tmp = _find_pixmap(&bitmap, id, handle, NULL, NULL);
	else
	{
		char *file = alloca(strlen(filename) + 5);
#if GTK_MAJOR_VERSION > 1
		GdkPixbuf *pixbuf;
#elif defined(USE_IMLIB)
		GdkImlibImage *image;
#endif

		if (!file)
		{
			DW_MUTEX_UNLOCK;
			return;
		}

		strcpy(file, filename);

		/* check if we can read from this file (it exists and read permission) */
		if(access(file, 04) != 0)
		{
			/* Try with .xpm extention */
			strcat(file, ".xpm");
			if(access(file, 04) != 0)
			{
				DW_MUTEX_UNLOCK;
				return NULL;
			}
		}
#if GTK_MAJOR_VERSION > 1
		pixbuf = gdk_pixbuf_new_from_file(file, NULL);

		gdk_pixbuf_render_pixmap_and_mask(pixbuf, &tmp, &bitmap, 1);
		g_object_unref(pixbuf);
#elif defined(USE_IMLIB)
		image = gdk_imlib_load_image(file);

		gdk_imlib_render(image, image->rgb_width, image->rgb_height);
		tmp = gdk_imlib_copy_image(image);
		gdk_imlib_destroy_image(image);
#else
		tmp = gdk_pixmap_create_from_xpm(handle->window, &bitmap, &_colors[DW_CLR_PALEGRAY], file);
#endif
	}

	if(tmp)
#if GTK_MAJOR_VERSION > 1
		gtk_image_set_from_pixmap(GTK_IMAGE(handle), tmp, bitmap);
#else
		gtk_pixmap_set(GTK_PIXMAP(handle), tmp, bitmap);
#endif
	DW_MUTEX_UNLOCK;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void dw_window_set_text(HWND handle, char *text)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_ENTRY(handle))
		gtk_entry_set_text(GTK_ENTRY(handle), text);
	else if(GTK_IS_COMBO(handle))
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(handle)->entry), text);
	else if(GTK_IS_LABEL(handle))
		gtk_label_set_text(GTK_LABEL(handle), text);
	else if(GTK_IS_FRAME(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(handle), "label");
		if(tmp && GTK_IS_LABEL(tmp))
			gtk_label_set_text(GTK_LABEL(tmp), text);
	}
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
#if GTK_MAJOR_VERSION > 1
	const char *possible = "";
#else
	char *possible = "";
#endif
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_ENTRY(handle))
		possible = gtk_entry_get_text(GTK_ENTRY(handle));
	else if(GTK_IS_COMBO(handle))
		possible = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(handle)->entry));

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
#if GTK_MAJOR_VERSION > 1
		orig = list = gtk_container_get_children(GTK_CONTAINER(handle));
#else
		orig = list = gtk_container_children(GTK_CONTAINER(handle));
#endif
	}
	while(list)
	{
		if(GTK_IS_WIDGET(list->data))
		{
			if(id == (int)gtk_object_get_data(GTK_OBJECT(list->data), "id"))
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

void _strip_cr(char *dest, char *src)
{
	int z, x = 0;

	for(z=0;z<strlen(src);z++)
	{
		if(src[z] != '\r')
		{
			dest[x] = src[z];
			x++;
		}
	}
	dest[x] = 0;
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
#else
	if(GTK_IS_BOX(handle))
#endif
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

#if GTK_MAJOR_VERSION > 1
		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			char *impbuf = malloc(strlen(buffer)+1);
			GtkTextBuffer *tbuffer;
			GtkTextIter iter;
      
			_strip_cr(impbuf, buffer);

			tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
			gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, startpoint);
			gtk_text_buffer_place_cursor(tbuffer, &iter);
			gtk_text_buffer_insert_at_cursor(tbuffer, impbuf, -1);
			tmppoint = (startpoint > -1 ? startpoint : 0) + strlen(impbuf);
			free(impbuf);
		}
#else
		GdkFont *font = (GdkFont *)gtk_object_get_data(GTK_OBJECT(handle), "gdkfont");
    
		if(tmp && GTK_IS_TEXT(tmp))
		{
			GdkColor *fore = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle), "foregdk");
			GdkColor *back = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle), "backgdk");
			char *impbuf = malloc(strlen(buffer)+1);

			_strip_cr(impbuf, buffer);

			gtk_text_set_point(GTK_TEXT(tmp), startpoint < 0 ? 0 : startpoint);
			gtk_text_insert(GTK_TEXT(tmp), font, fore, back, impbuf, -1);
			tmppoint = gtk_text_get_point(GTK_TEXT(tmp));
			free(impbuf);
		}
#endif
	}
	DW_MUTEX_UNLOCK;
	return tmppoint;
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
void dw_mle_export(HWND handle, char *buffer, int startpoint, int length)
{
	int _locked_by_me = FALSE;
	gchar *text;

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
#else
	if(GTK_IS_BOX(handle))
#endif
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

#if GTK_MAJOR_VERSION > 1
		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *tbuffer;
			GtkTextIter start, end;

			tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
			gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
			gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
			text = gtk_text_iter_get_text(&start, &end);
			if(text) /* Should this get freed? */
				strcpy(buffer, text);
		}
#else
		if(tmp && GTK_IS_TEXT(tmp))
		{
			text = gtk_editable_get_chars(GTK_EDITABLE(&(GTK_TEXT(tmp)->editable)), startpoint, startpoint + length);
			if(text)
			{
				strcpy(buffer, text);
				g_free(text);
			}
		}
#endif
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
void dw_mle_query(HWND handle, unsigned long *bytes, unsigned long *lines)
{
	int _locked_by_me = FALSE;

	if(bytes)
		*bytes = 0;
	if(lines)
		*lines = 0;

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));
      
			if(bytes)
				*bytes = gtk_text_buffer_get_char_count(buffer) + 1;
			if(lines)
				*lines = gtk_text_buffer_get_line_count(buffer) + 1;
		}
	}
#else
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			if(bytes)
				*bytes = gtk_text_get_length(GTK_TEXT(tmp)) + 1;
			if(lines)
			{
				gchar *text;

				*lines = 0;
				text = gtk_editable_get_chars(GTK_EDITABLE(&(GTK_TEXT(tmp)->editable)), 0, bytes ? *bytes : gtk_text_get_length(GTK_TEXT(tmp)));

				if(text)
				{
					int z, len = strlen(text);

					for(z=0;z<len;z++)
					{
						if(text[z] == '\n')
							(*lines)++;
					}
					g_free(text);
				}
			}
		}
	}
#endif
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
#else
	if(GTK_IS_BOX(handle))
#endif
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

#if GTK_MAJOR_VERSION > 1
		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *tbuffer;
			GtkTextIter start, end;

			tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
			gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
			gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
			gtk_text_buffer_delete(tbuffer, &start, &end);
		}
#else
		if(tmp && GTK_IS_TEXT(tmp))
		{
			gtk_text_set_point(GTK_TEXT(tmp), startpoint);
			gtk_text_forward_delete(GTK_TEXT(tmp), length);
		}
#endif
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));
      
			length = -1;
			gtk_text_buffer_set_text(buffer, "", length);
		}
	}
#else
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			length = gtk_text_get_length(GTK_TEXT(tmp));
			gtk_text_set_point(GTK_TEXT(tmp), 0);
			gtk_text_forward_delete(GTK_TEXT(tmp), length);
		}
	}
#endif
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
#if GTK_MAJOR_VERSION > 1
#else
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			unsigned long lines;
			float pos, ratio;

			dw_mle_query(handle, NULL, &lines);

			if(lines)
			{
				ratio = (float)line/(float)lines;

				pos = (ratio * (float)(GTK_TEXT(tmp)->vadj->upper - GTK_TEXT(tmp)->vadj->lower)) + GTK_TEXT(tmp)->vadj->lower;

				gtk_adjustment_set_value(GTK_TEXT(tmp)->vadj, pos);
			}
		}
	}
#endif
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT_VIEW(tmp))
			gtk_text_view_set_editable(GTK_TEXT_VIEW(tmp), state);
	}
#else
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
			gtk_text_set_editable(GTK_TEXT(tmp), state);
	}
#endif
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT_VIEW(tmp))
			gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), GTK_WRAP_WORD);
	}
#else
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			gtk_text_set_word_wrap(GTK_TEXT(tmp), state);
			gtk_text_set_line_wrap(GTK_TEXT(tmp), state);
		}
	}
#endif
	DW_MUTEX_UNLOCK;
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
void dw_mle_set(HWND handle, int point)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
#else
	if(GTK_IS_BOX(handle))
#endif
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

#if GTK_MAJOR_VERSION > 1
		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *tbuffer;
			GtkTextIter iter;

			tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
			gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
			gtk_text_buffer_place_cursor(tbuffer, &iter);
		}
#else
		if(tmp && GTK_IS_TEXT(tmp))
		{
			unsigned long chars;
			float pos, ratio;

			dw_mle_query(handle, &chars, NULL);

			if(chars)
			{
				ratio = (float)point/(float)chars;

				pos = (ratio * (float)(GTK_TEXT(tmp)->vadj->upper - GTK_TEXT(tmp)->vadj->lower)) + GTK_TEXT(tmp)->vadj->lower;

				gtk_adjustment_set_value(GTK_TEXT(tmp)->vadj, pos);
			}
			gtk_text_set_point(GTK_TEXT(tmp), point);
		}
#endif
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
#if GTK_MAJOR_VERSION > 1
	if(GTK_IS_SCROLLED_WINDOW(handle))
#else
	if(GTK_IS_BOX(handle))
#endif
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

#if GTK_MAJOR_VERSION > 1
		if(tmp && GTK_IS_TEXT_VIEW(tmp))
		{
			GtkTextBuffer *tbuffer;
			GtkTextIter iter, found;

			tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
			gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
			gtk_text_iter_forward_search(&iter, text, GTK_TEXT_SEARCH_TEXT_ONLY, &found, NULL, NULL);
			retval = gtk_text_iter_get_offset(&found);  
		}
#else
		if(tmp && GTK_IS_TEXT(tmp))
		{
			int len = gtk_text_get_length(GTK_TEXT(tmp));
			gchar *tmpbuf;

			tmpbuf = gtk_editable_get_chars(GTK_EDITABLE(&(GTK_TEXT(tmp)->editable)), 0, len);
			if(tmpbuf)
			{
				int z, textlen;

				textlen = strlen(text);

				if(flags & DW_MLE_CASESENSITIVE)
				{
					for(z=point;z<(len-textlen) && !retval;z++)
					{
						if(strncmp(&tmpbuf[z], text, textlen) == 0)
							retval = z + textlen;
					}
				}
				else
				{
					for(z=point;z<(len-textlen) && !retval;z++)
					{
						if(strncasecmp(&tmpbuf[z], text, textlen) == 0)
							retval = z + textlen;
					}
				}

				if(retval)
				{
					gtk_text_set_point(GTK_TEXT(tmp), retval - textlen);
					gtk_editable_select_region(&(GTK_TEXT(tmp)->editable), retval - textlen, retval);
				}

				g_free(tmpbuf);
			}
		}
#endif
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
#if GTK_MAJOR_VERSION < 2
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			gtk_text_freeze(GTK_TEXT(tmp));
		}
	}
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void dw_mle_thaw(HWND handle)
{
#if GTK_MAJOR_VERSION < 2
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_BOX(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));

		if(tmp && GTK_IS_TEXT(tmp))
		{
			gtk_text_thaw(GTK_TEXT(tmp));
		}
	}
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Returns the range of the percent bar.
 * Parameters:
 *          handle: Handle to the percent bar to be queried.
 */
unsigned int dw_percent_query_range(HWND handle)
{
	return 100;
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
	gtk_progress_bar_update(GTK_PROGRESS_BAR(handle), (gfloat)position/100);
	DW_MUTEX_UNLOCK;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int dw_slider_query_pos(HWND handle)
{
	int val = 0, _locked_by_me = FALSE;
	GtkAdjustment *adjustment;

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
	adjustment = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(handle), "adjustment");
	if(adjustment)
	{
		int max = _round_value(adjustment->upper) - 1;
		int thisval = _round_value(adjustment->value);

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
	adjustment = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(handle), "adjustment");
	if(adjustment)
	{
		int max = _round_value(adjustment->upper) - 1;

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
unsigned int dw_scrollbar_query_pos(HWND handle)
{
	int val = 0, _locked_by_me = FALSE;
	GtkAdjustment *adjustment;

	if(!handle)
		return 0;

	DW_MUTEX_LOCK;
	adjustment = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(handle), "adjustment");
	if(adjustment)
	{
		int max = _round_value(adjustment->upper) - 1;
		int thisval = _round_value(adjustment->value);

		if(GTK_IS_VSCROLLBAR(handle))
			val = max - thisval;
        else
			val = thisval;
	}
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
	adjustment = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(handle), "adjustment");
	if(adjustment)
		gtk_adjustment_set_value(adjustment, (gfloat)position);
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
	adjustment = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(handle), "adjustment");
	if(adjustment)
	{
		adjustment->upper = (gdouble)range;
		adjustment->page_increment = adjustment->page_size = (gdouble)visible;
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

	curval = dw_spinbutton_query(handle);
	DW_MUTEX_LOCK;
	adj = (GtkAdjustment *)gtk_adjustment_new((gfloat)curval, (gfloat)lower, (gfloat)upper, 1.0, 5.0, 0.0);
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
long dw_spinbutton_query(HWND handle)
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
int dw_checkbox_query(HWND handle)
{
	int retval;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	retval = GTK_TOGGLE_BUTTON(handle)->active;
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
HWND dw_tree_insert_after(HWND handle, HWND item, char *title, unsigned long icon, HWND parent, void *itemdata)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeIter *iter;
	GtkTreeStore *store;
	GdkPixbuf *pixbuf;
	HWND retval = 0;
	int _locked_by_me = FALSE;

	if(!handle)
		return NULL;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));
        
		pixbuf = _find_pixbuf(icon);

		gtk_tree_store_insert_after(store, iter, (GtkTreeIter *)parent, (GtkTreeIter *)item);
		gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
		if(pixbuf)
			g_object_unref(pixbuf);
		retval = (HWND)iter;    
	}
	DW_MUTEX_UNLOCK;
  
	return retval;
#else
	GtkWidget *newitem, *tree, *subtree, *label, *hbox, *pixmap;
	GdkPixmap *gdkpix;
	GdkBitmap *gdkbmp = NULL;
	int position = -1;
	int _locked_by_me = FALSE;

	if(!handle)
		return NULL;

	DW_MUTEX_LOCK;
	tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!tree || !GTK_IS_TREE(tree))
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	if(item && GTK_IS_TREE_ITEM(item))
		position = gtk_tree_child_position(GTK_TREE(tree), item);

	position++;

	newitem = gtk_tree_item_new();
	label = gtk_label_new(title);
	gtk_object_set_data(GTK_OBJECT(newitem), "text", (gpointer)strdup(title));
	gtk_object_set_data(GTK_OBJECT(newitem), "itemdata", (gpointer)itemdata);
	gtk_object_set_data(GTK_OBJECT(newitem), "tree", (gpointer)tree);
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_object_set_data(GTK_OBJECT(newitem), "hbox", (gpointer)hbox);
	gdkpix = _find_pixmap(&gdkbmp, icon, hbox, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(newitem), hbox);
	if(gdkpix)
	{
		pixmap = gtk_pixmap_new(gdkpix, gdkbmp);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, TRUE, 0);
		gtk_widget_show(pixmap);
	}
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);

	if(parent)
	{
		subtree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(parent));
		if(!subtree)
		{
			void *thisfunc = (void *)gtk_object_get_data(GTK_OBJECT(tree), "select-child-func");
			void *work = (void *)gtk_object_get_data(GTK_OBJECT(tree), "select-child-data");

			subtree = gtk_tree_new();

			if(thisfunc && work)
				gtk_signal_connect(GTK_OBJECT(subtree), "select-child", GTK_SIGNAL_FUNC(thisfunc), work);

			thisfunc = (void *)gtk_object_get_data(GTK_OBJECT(tree), "container-context-func");
			work = (void *)gtk_object_get_data(GTK_OBJECT(tree), "container-context-data");

			if(thisfunc && work)
				gtk_signal_connect(GTK_OBJECT(subtree), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);

			gtk_object_set_user_data(GTK_OBJECT(parent), subtree);
			gtk_tree_set_selection_mode(GTK_TREE(subtree), GTK_SELECTION_SINGLE);
			gtk_tree_set_view_mode(GTK_TREE(subtree), GTK_TREE_VIEW_ITEM);
			gtk_tree_item_set_subtree(GTK_TREE_ITEM(parent), subtree);
			gtk_tree_item_collapse(GTK_TREE_ITEM(parent));
			gtk_widget_show(subtree);
			gtk_tree_item_expand(GTK_TREE_ITEM(parent));
			gtk_tree_item_collapse(GTK_TREE_ITEM(parent));
		}
		gtk_object_set_data(GTK_OBJECT(newitem), "parenttree", (gpointer)subtree);
		gtk_tree_insert(GTK_TREE(subtree), newitem, position);
	}
	else
	{
		gtk_object_set_data(GTK_OBJECT(newitem), "parenttree", (gpointer)tree);
		gtk_tree_insert(GTK_TREE(tree), newitem, position);
	}
	gtk_tree_item_expand(GTK_TREE_ITEM(newitem));
	gtk_tree_item_collapse(GTK_TREE_ITEM(newitem));
	gtk_widget_show(newitem);
	DW_MUTEX_UNLOCK;
	return newitem;
#endif
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
HWND dw_tree_insert(HWND handle, char *title, unsigned long icon, HWND parent, void *itemdata)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeIter *iter;
	GtkTreeStore *store;
	GdkPixbuf *pixbuf;
	HWND retval = 0;
	int _locked_by_me = FALSE;

	if(!handle)
		return NULL;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));
    
		pixbuf = _find_pixbuf(icon);

		gtk_tree_store_append (store, iter, (GtkTreeIter *)parent);
		gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
		if(pixbuf)
			g_object_unref(pixbuf);
		retval = (HWND)iter;
	}
	DW_MUTEX_UNLOCK;

	return retval;
#else
	GtkWidget *item, *tree, *subtree, *label, *hbox, *pixmap;
	GdkPixmap *gdkpix;
	GdkBitmap *gdkbmp = NULL;
	int _locked_by_me = FALSE;

	if(!handle)
		return NULL;

	DW_MUTEX_LOCK;
	tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!tree || !GTK_IS_TREE(tree))
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}
	item = gtk_tree_item_new();
	label = gtk_label_new(title);
	gtk_object_set_data(GTK_OBJECT(item), "text", (gpointer)strdup(title));
	gtk_object_set_data(GTK_OBJECT(item), "itemdata", (gpointer)itemdata);
	gtk_object_set_data(GTK_OBJECT(item), "tree", (gpointer)tree);
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_object_set_data(GTK_OBJECT(item), "hbox", (gpointer)hbox);
	gdkpix = _find_pixmap(&gdkbmp, icon, hbox, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(item), hbox);
	if(gdkpix)
	{
		pixmap = gtk_pixmap_new(gdkpix, gdkbmp);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, TRUE, 0);
		gtk_widget_show(pixmap);
	}
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);

	if(parent)
	{
		subtree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(parent));
		if(!subtree)
		{
			void *thisfunc = (void *)gtk_object_get_data(GTK_OBJECT(tree), "select-child-func");
			void *work = (void *)gtk_object_get_data(GTK_OBJECT(tree), "select-child-data");

			subtree = gtk_tree_new();

			if(thisfunc && work)
				gtk_signal_connect(GTK_OBJECT(subtree), "select-child", GTK_SIGNAL_FUNC(thisfunc), work);

			thisfunc = (void *)gtk_object_get_data(GTK_OBJECT(tree), "container-context-func");
			work = (void *)gtk_object_get_data(GTK_OBJECT(tree), "container-context-data");

			if(thisfunc && work)
				gtk_signal_connect(GTK_OBJECT(subtree), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);

			gtk_object_set_user_data(GTK_OBJECT(parent), subtree);
			gtk_tree_set_selection_mode(GTK_TREE(subtree), GTK_SELECTION_SINGLE);
			gtk_tree_set_view_mode(GTK_TREE(subtree), GTK_TREE_VIEW_ITEM);
			gtk_tree_item_set_subtree(GTK_TREE_ITEM(parent), subtree);
			gtk_tree_item_collapse(GTK_TREE_ITEM(parent));
			gtk_widget_show(subtree);
			gtk_tree_item_expand(GTK_TREE_ITEM(parent));
			gtk_tree_item_collapse(GTK_TREE_ITEM(parent));
		}
		gtk_object_set_data(GTK_OBJECT(item), "parenttree", (gpointer)subtree);
		gtk_tree_append(GTK_TREE(subtree), item);
	}
	else
	{
		gtk_object_set_data(GTK_OBJECT(item), "parenttree", (gpointer)tree);
		gtk_tree_append(GTK_TREE(tree), item);
	}
	gtk_tree_item_expand(GTK_TREE_ITEM(item));
	gtk_tree_item_collapse(GTK_TREE_ITEM(item));
	gtk_widget_show(item);
	DW_MUTEX_UNLOCK;
	return item;
#endif
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
void dw_tree_set(HWND handle, HWND item, char *title, unsigned long icon)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	GdkPixbuf *pixbuf;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		pixbuf = _find_pixbuf(icon);
    
		gtk_tree_store_set(store, (GtkTreeIter *)item, 0, title, 1, pixbuf, -1);
		if(pixbuf)
			g_object_unref(pixbuf);
	}
	DW_MUTEX_UNLOCK;
#else
	GtkWidget *label, *hbox, *pixmap;
	GdkPixmap *gdkpix;
	GdkBitmap *gdkbmp = NULL;
	char *oldtext;
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	oldtext = (char *)gtk_object_get_data(GTK_OBJECT(item), "text");
	if(oldtext)
		free(oldtext);
	label = gtk_label_new(title);
	gtk_object_set_data(GTK_OBJECT(item), "text", (gpointer)strdup(title));
	hbox = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "hbox");
	gtk_widget_destroy(hbox);
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_object_set_data(GTK_OBJECT(item), "hbox", (gpointer)hbox);
	gdkpix = _find_pixmap(&gdkbmp, icon, hbox, NULL, NULL);
	gtk_container_add(GTK_CONTAINER(item), hbox);
	if(gdkpix)
	{
		pixmap = gtk_pixmap_new(gdkpix, gdkbmp);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, TRUE, 0);
		gtk_widget_show(pixmap);
	}
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	gtk_widget_show(hbox);
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
void dw_tree_set_data(HWND handle, HWND item, void *itemdata)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
			gtk_tree_store_set(store, (GtkTreeIter *)item, 2, itemdata, -1);
	DW_MUTEX_UNLOCK;
#else
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	gtk_object_set_data(GTK_OBJECT(item), "itemdata", (gpointer)itemdata);
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void dw_tree_item_select(HWND handle, HWND item)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{    
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
		gtk_tree_selection_select_iter(sel, (GtkTreeIter *)item);
		gtk_tree_path_free(path);
	}
	DW_MUTEX_UNLOCK;
#else
	GtkWidget *lastselect, *tree;
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	lastselect = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(tree), "lastselect");
	if(lastselect && GTK_IS_TREE_ITEM(lastselect))
		gtk_tree_item_deselect(GTK_TREE_ITEM(lastselect));
	gtk_tree_item_select(GTK_TREE_ITEM(item));
	gtk_object_set_data(GTK_OBJECT(tree), "lastselect", (gpointer)item);
	DW_MUTEX_UNLOCK;
#endif
}

#if GTK_MAJOR_VERSION > 1
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
#endif

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void dw_tree_clear(HWND handle)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;               
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
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
#else
	GtkWidget *tree;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!tree || !GTK_IS_TREE(tree))
	{
		DW_MUTEX_UNLOCK;
		return;
	}
	gtk_object_set_data(GTK_OBJECT(tree), "lastselect", NULL);
	gtk_tree_clear_items(GTK_TREE(tree), 0, 1000000);
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void dw_tree_expand(HWND handle, HWND item)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE);
		gtk_tree_path_free(path);
	}
	DW_MUTEX_UNLOCK;
#else
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	if(GTK_IS_TREE_ITEM(item))
		gtk_tree_item_expand(GTK_TREE_ITEM(item));
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void dw_tree_collapse(HWND handle, HWND item)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), path);
		gtk_tree_path_free(path);
	}
	DW_MUTEX_UNLOCK;
#else
	int _locked_by_me = FALSE;

	if(!handle || !item)
		return;

	DW_MUTEX_LOCK;
	if(GTK_IS_TREE_ITEM(item))
		gtk_tree_item_collapse(GTK_TREE_ITEM(item));
	DW_MUTEX_UNLOCK;
#endif
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void dw_tree_delete(HWND handle, HWND item)
{
#if GTK_MAJOR_VERSION > 1
	GtkWidget *tree;
	GtkTreeStore *store;
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if((tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle)))
		&& GTK_IS_TREE_VIEW(tree) &&
		(store = (GtkTreeStore *)gtk_object_get_data(GTK_OBJECT(tree), "_dw_tree_store")))
	{
		gtk_tree_store_remove(store, (GtkTreeIter *)item);
		free(item);
	}
	DW_MUTEX_UNLOCK;
#else
	GtkWidget *tree, *lastselect, *parenttree;
	int _locked_by_me = FALSE;

	if(!handle || !item || !GTK_IS_WIDGET(handle) || !GTK_IS_WIDGET(item))
		return;

	DW_MUTEX_LOCK;
	tree = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!tree || !GTK_IS_TREE(tree))
	{
		DW_MUTEX_UNLOCK;
		return;
	}

	lastselect = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(tree), "lastselect");

	parenttree = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "parenttree");

	if(lastselect == item)
	{
		gtk_tree_item_deselect(GTK_TREE_ITEM(lastselect));
		gtk_object_set_data(GTK_OBJECT(tree), "lastselect", NULL);
	}

	if(parenttree && GTK_IS_WIDGET(parenttree))
		gtk_container_remove(GTK_CONTAINER(parenttree), item);
	DW_MUTEX_UNLOCK;
#endif
}

static int _dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator, int extra)
{
	GtkWidget *clist;
	char numbuf[10];
	int z;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
    clist = gtk_clist_new_with_titles(count, (gchar **)titles);
	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return FALSE;
	}

	gtk_signal_connect(GTK_OBJECT(clist), "select_row", GTK_SIGNAL_FUNC(_select_row),  NULL);
	gtk_signal_connect(GTK_OBJECT(clist), "unselect_row", GTK_SIGNAL_FUNC(_unselect_row),  NULL);

	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
	gtk_container_add(GTK_CONTAINER(handle), clist);
	gtk_object_set_data(GTK_OBJECT(clist), "multi", (gpointer)0);
	gtk_object_set_user_data(GTK_OBJECT(handle), (gpointer)clist);
	gtk_widget_show(clist);
	gtk_object_set_data(GTK_OBJECT(clist), "colcount", (gpointer)count);

    if(extra)
		gtk_clist_set_column_width(GTK_CLIST(clist), 1, 120);

	for(z=0;z<count;z++)
	{
		if(!extra || z > 1)
			gtk_clist_set_column_width(GTK_CLIST(clist), z, 50);
		sprintf(numbuf, "%d", z);
		gtk_object_set_data(GTK_OBJECT(clist), numbuf, (gpointer)flags[z]);
	}

	DW_MUTEX_UNLOCK;
	return TRUE;
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
	char **newtitles = malloc(sizeof(char *) * (count + 2));
	unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 2));

	newtitles[0] = "Icon";
	newtitles[1] = "Filename";

	newflags[0] = DW_CFA_BITMAPORICON | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR;
	newflags[1] = DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

	memcpy(&newtitles[2], titles, sizeof(char *) * count);
	memcpy(&newflags[2], flags, sizeof(unsigned long) * count);

	_dw_container_setup(handle, newflags, newtitles, count + 2, 2, 1);

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
unsigned long dw_icon_load(unsigned long module, unsigned long id)
{
	return id;
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
	int found = -1, _locked_by_me = FALSE;
#if GTK_MAJOR_VERSION > 1
	GdkPixbuf *pixbuf;
#elif defined(USE_IMLIB)
	GdkImlibImage *image;
#endif
	char *file = alloca(strlen(filename) + 5);
	unsigned long z, ret = 0;

	if (!file)
		return 0;

	strcpy(file, filename);

	/* check if we can read from this file (it exists and read permission) */
	if(access(file, 04) != 0)
	{
		/* Try with .xpm extention */
		strcat(file, ".xpm");
		if(access(file, 04) != 0)
			return 0;
	}

	DW_MUTEX_LOCK;
	/* Find a free entry in the array */
	for(z=0;z<_PixmapCount;z++)
	{
		if(!_PixmapArray[z].used)
		{
			ret = found = z;
			break;
		}
	}

	/* If there are no free entries, expand the
	 * array.
	 */
	if(found == -1)
	{
		DWPrivatePixmap *old = _PixmapArray;

		ret = found = _PixmapCount;
		_PixmapCount++;

		_PixmapArray = malloc(sizeof(DWPrivatePixmap) * _PixmapCount);

		if(found)
			memcpy(_PixmapArray, old, sizeof(DWPrivatePixmap) * found);
		if(old)
			free(old);
		_PixmapArray[found].used = 1;
		_PixmapArray[found].pixmap = _PixmapArray[found].mask = NULL;
	}

#if GTK_MAJOR_VERSION > 1
	pixbuf = gdk_pixbuf_new_from_file(file, NULL);

	if(pixbuf)
	{
		_PixmapArray[found].width = gdk_pixbuf_get_width(pixbuf);
		_PixmapArray[found].height = gdk_pixbuf_get_height(pixbuf);

		gdk_pixbuf_render_pixmap_and_mask(pixbuf, &_PixmapArray[found].pixmap, &_PixmapArray[found].mask, 1);
		g_object_unref(pixbuf);
	}
#elif defined(USE_IMLIB)
	image = gdk_imlib_load_image(file);

	if(image)
	{
		_PixmapArray[found].width = image->rgb_width;
		_PixmapArray[found].height = image->rgb_height;

		gdk_imlib_render(image, image->rgb_width, image->rgb_height);
		_PixmapArray[found].pixmap = gdk_imlib_copy_image(image);
		_PixmapArray[found].mask = gdk_imlib_copy_mask(image);
		gdk_imlib_destroy_image(image);
	}
#else
	_PixmapArray[found].pixmap = gdk_pixmap_create_from_xpm(handle->window, &_PixmapArray[found].mask, &_colors[DW_CLR_PALEGRAY], file);
#endif
	DW_MUTEX_UNLOCK;
	if(!_PixmapArray[found].pixmap || !_PixmapArray[found].mask)
	{
		_PixmapArray[found].used = 0;
		_PixmapArray[found].pixmap = _PixmapArray[found].mask = NULL;
		return 0;
	}
	return ret | (1 << 31);
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void dw_icon_free(unsigned long handle)
{
	/* If it is a private icon, find the item
	 * free the associated structures and set
	 * the entry to unused.
	 */
	if(handle & (1 << 31))
	{
		unsigned long id = handle & 0xFFFFFF;

		if(id < _PixmapCount && _PixmapArray[id].used)
		{
			if(_PixmapArray[id].mask)
			{
				gdk_bitmap_unref(_PixmapArray[id].mask);
				_PixmapArray[id].mask = NULL;
			}
			if(_PixmapArray[id].pixmap)
			{
				gdk_pixmap_unref(_PixmapArray[id].pixmap);
				_PixmapArray[id].pixmap = NULL;
			}
			_PixmapArray[id].used = 0;
		}
	}
}

/* Clears a CList selection and associated selection list */
void _dw_unselect(GtkWidget *clist)
{
	GList *list = (GList *)gtk_object_get_data(GTK_OBJECT(clist), "selectlist");

	if(list)
		g_list_free(list);

	gtk_object_set_data(GTK_OBJECT(clist), "selectlist", NULL);

	_dw_unselecting = 1;
	gtk_clist_unselect_all(GTK_CLIST(clist));
	_dw_unselecting = 0;
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
void *dw_container_alloc(HWND handle, int rowcount)
{
	int z, count = 0;
	GtkWidget *clist;
	GdkColor *fore, *back;
	char **blah;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	count = (int)gtk_object_get_data(GTK_OBJECT(clist), "colcount");

	if(!count)
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	blah = malloc(sizeof(char *) * count);
	memset(blah, 0, sizeof(char *) * count);

	fore = (GdkColor *)gtk_object_get_data(GTK_OBJECT(clist), "foregdk");
	back = (GdkColor *)gtk_object_get_data(GTK_OBJECT(clist), "backgdk");
	gtk_clist_freeze(GTK_CLIST(clist));
	for(z=0;z<rowcount;z++)
	{
		gtk_clist_append(GTK_CLIST(clist), blah);
		if(fore)
			gtk_clist_set_foreground(GTK_CLIST(clist), z, fore);
		if(back)
			gtk_clist_set_background(GTK_CLIST(clist), z, back);
	}
	gtk_object_set_data(GTK_OBJECT(clist), "rowcount", (gpointer)rowcount);
	free(blah);
	DW_MUTEX_UNLOCK;
	return (void *)handle;
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
	char numbuf[10], textbuffer[100];
	int flag = 0;
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return;
	}

	sprintf(numbuf, "%d", column);
	flag = (int)gtk_object_get_data(GTK_OBJECT(clist), numbuf);

	if(flag & DW_CFA_BITMAPORICON)
	{
		long hicon = *((long *)data);
		GdkBitmap *bitmap = NULL;
		GdkPixmap *pixmap = _find_pixmap(&bitmap, hicon, clist, NULL, NULL);

		if(pixmap)
			gtk_clist_set_pixmap(GTK_CLIST(clist), row, column, pixmap, bitmap);
	}
	else if(flag & DW_CFA_STRING)
	{
		char *tmp = *((char **)data);
		gtk_clist_set_text(GTK_CLIST(clist), row, column, tmp);
	}
	else if(flag & DW_CFA_ULONG)
	{
		ULONG tmp = *((ULONG *)data);

		sprintf(textbuffer, "%lu", tmp);

		gtk_clist_set_text(GTK_CLIST(clist), row, column, textbuffer);
	}
	else if(flag & DW_CFA_DATE)
	{
		struct tm curtm;
		CDATE cdate = *((CDATE *)data);

		curtm.tm_mday = cdate.day;
		curtm.tm_mon = cdate.month - 1;
		curtm.tm_year = cdate.year - 1900;

		strftime(textbuffer, 100, "%x", &curtm);

		gtk_clist_set_text(GTK_CLIST(clist), row, column, textbuffer);
	}
	else if(flag & DW_CFA_TIME)
	{
		struct tm curtm;
		CTIME ctime = *((CTIME *)data);

		curtm.tm_hour = ctime.hours;
		curtm.tm_min = ctime.minutes;
		curtm.tm_sec = ctime.seconds;

		strftime(textbuffer, 100, "%X", &curtm);

		gtk_clist_set_text(GTK_CLIST(clist), row, column, textbuffer);
	}
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
void dw_container_change_item(HWND handle, int column, int row, void *data)
{
	dw_container_set_item(handle, NULL, column, row, data);
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
void dw_filesystem_set_file(HWND handle, void *pointer, int row, char *filename, unsigned long icon)
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
void dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
	dw_container_set_item(handle, pointer, column + 2, row, data);
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
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = gtk_object_get_user_data(GTK_OBJECT(handle));

	if(clist && GTK_IS_CLIST(clist))
		gtk_clist_set_column_width(GTK_CLIST(clist), column, width);
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
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(pointer));

	if(clist)
		gtk_clist_set_row_data(GTK_CLIST(clist), row, (gpointer)title);
	DW_MUTEX_UNLOCK;
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
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = gtk_object_get_user_data(GTK_OBJECT(handle));

	if(clist && GTK_IS_CLIST(clist))
		gtk_clist_thaw(GTK_CLIST(clist));
	DW_MUTEX_UNLOCK;
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void dw_container_delete(HWND handle, int rowcount)
{
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(clist && GTK_IS_CLIST(clist))
	{
		int rows, z;

		rows = (int)gtk_object_get_data(GTK_OBJECT(clist), "rowcount");

		_dw_unselect(clist);

		for(z=0;z<rowcount;z++)
			gtk_clist_remove(GTK_CLIST(clist), 0);

		if(rows - rowcount < 0)
			rows = 0;
		else
			rows -= rowcount;

		gtk_object_set_data(GTK_OBJECT(clist), "rowcount", (gpointer)rows);
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
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(clist && GTK_IS_CLIST(clist))
	{
		_dw_unselect(clist);
		gtk_clist_clear(GTK_CLIST(clist));
		gtk_object_set_data(GTK_OBJECT(clist), "rowcount", (gpointer)0);
	}
	DW_MUTEX_UNLOCK;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void dw_container_set_view(HWND handle, unsigned long flags, int iconwidth, int iconheight)
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
void dw_container_scroll(HWND handle, int direction, long rows)
{
	GtkAdjustment *adj;
	GtkWidget *clist;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));
	if(clist && GTK_IS_CLIST(clist))
	{
		adj = gtk_clist_get_vadjustment(GTK_CLIST(clist));
		if(adj)
		{
			switch(direction)
			{
			case DW_SCROLL_TOP:
				adj->value = adj->lower;
				break;
			case DW_SCROLL_BOTTOM:
				adj->value = adj->upper;
				break;
			}
			gtk_clist_set_vadjustment(GTK_CLIST(clist), adj);
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
	GtkWidget *clist;
	GList *list;
	char *retval = NULL;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));

	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	/* These should be separate but right now this will work */
	if(flags & DW_CRA_SELECTED || flags & DW_CRA_CURSORED)
	{
		/* If there is an old query list, free it */
		list = (GList *)gtk_object_get_data(GTK_OBJECT(clist), "querylist");
		if(list)
			g_list_free(list);

		/* Move the current selection list to the query list, and remove the
		 * current selection list.
		 */
		list = (GList *)gtk_object_get_data(GTK_OBJECT(clist), "selectlist");
		gtk_object_set_data(GTK_OBJECT(clist), "selectlist", NULL);
		gtk_object_set_data(GTK_OBJECT(clist), "querylist", (gpointer)list);
		_dw_unselect(clist);

		if(list)
		{
			gtk_object_set_data(GTK_OBJECT(clist), "querypos", (gpointer)1);
			if(list->data)
				retval =  list->data;
			else
				retval = "";
		}
	}
	else
	{
		retval = (char *)gtk_clist_get_row_data(GTK_CLIST(clist), 0);
		gtk_object_set_data(GTK_OBJECT(clist), "querypos", (gpointer)1);
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
	GtkWidget *clist;
	GList *list;
	char *retval = NULL;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));

	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	/* These should be separate but right now this will work */
	if(flags & DW_CRA_SELECTED || flags & DW_CRA_CURSORED)
	{
		list = (GList *)gtk_object_get_data(GTK_OBJECT(clist), "querylist");

		if(list)
		{
			int counter = 0, pos = (int)gtk_object_get_data(GTK_OBJECT(clist), "querypos");
			gtk_object_set_data(GTK_OBJECT(clist), "querypos", (gpointer)pos+1);

			while(list && counter < pos)
			{
				list = list->next;
				counter++;
			}

			if(list && list->data)
				retval = list->data;
			else if(list && !list->data)
				retval = "";
		}
	}
	else
	{
		int pos = (int)gtk_object_get_data(GTK_OBJECT(clist), "querypos");

		retval = (char *)gtk_clist_get_row_data(GTK_CLIST(clist), pos);
		gtk_object_set_data(GTK_OBJECT(clist), "querypos", (gpointer)pos+1);
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
void dw_container_cursor(HWND handle, char *text)
{
	int _locked_by_me = FALSE;
	GtkWidget *clist;
	int rowcount, z;
	char *rowdata;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));

	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return;
	}
	rowcount = (int)gtk_object_get_data(GTK_OBJECT(clist), "rowcount");

	for(z=0;z<rowcount;z++)
	{
		rowdata = gtk_clist_get_row_data(GTK_CLIST(clist), z);
		if(rowdata == text)
		{
			gfloat pos;
			GtkAdjustment *adj = gtk_clist_get_vadjustment(GTK_CLIST(clist));

			_dw_unselect(clist);

			gtk_clist_select_row(GTK_CLIST(clist), z, 0);

			pos = ((adj->upper - adj->lower) * ((gfloat)z/(gfloat)rowcount)) + adj->lower;
			gtk_adjustment_set_value(adj, pos);
			DW_MUTEX_UNLOCK;
			return;
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
	int _locked_by_me = FALSE;
	GtkWidget *clist;
	int rowcount, z;
	char *rowdata;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));

	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return;
	}
	rowcount = (int)gtk_object_get_data(GTK_OBJECT(clist), "rowcount");

	for(z=0;z<rowcount;z++)
	{
		rowdata = gtk_clist_get_row_data(GTK_CLIST(clist), z);
		if(rowdata == text)
		{
			_dw_unselect(clist);

			gtk_clist_remove(GTK_CLIST(clist), z);

			rowcount--;

			gtk_object_set_data(GTK_OBJECT(clist), "rowcount", (gpointer)rowcount);
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
void dw_container_optimize(HWND handle)
{
	int _locked_by_me = FALSE;
	GtkWidget *clist;
	int colcount, z;

	DW_MUTEX_LOCK;
	clist = (GtkWidget*)gtk_object_get_user_data(GTK_OBJECT(handle));

	if(!clist)
	{
		DW_MUTEX_UNLOCK;
		return;
	}
	colcount = (int)gtk_object_get_data(GTK_OBJECT(clist), "colcount");
	for(z=0;z<colcount;z++)
	{
		int width = gtk_clist_optimal_column_width(GTK_CLIST(clist), z);
		gtk_clist_set_column_width(GTK_CLIST(clist), z, width);
	}
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
						  | GDK_KEY_PRESS_MASK
						  | GDK_POINTER_MOTION_MASK
						  | GDK_POINTER_MOTION_HINT_MASK);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	GTK_WIDGET_SET_FLAGS(tmp, GTK_CAN_FOCUS);
	gtk_widget_show(tmp);
	DW_MUTEX_UNLOCK;
	return tmp;
}

/* Returns a GdkColor from a DW color */
static GdkColor _internal_color(unsigned long value)
{
	if(DW_RGB_COLOR & value)
	{
		GdkColor color = { 0, DW_RED_VALUE(value) << 8, DW_GREEN_VALUE(value) << 8, DW_BLUE_VALUE(value) << 8 };
		return color;
	}
	else if (value < 16)
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
	int _locked_by_me = FALSE, index = _find_thread_index(dw_thread_id());
	GdkColor color = _internal_color(value);

	DW_MUTEX_LOCK;
	gdk_color_alloc(_dw_cmap, &color);
	_foreground[index] = color;
	DW_MUTEX_UNLOCK;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void dw_color_background_set(unsigned long value)
{
	int _locked_by_me = FALSE, index = _find_thread_index(dw_thread_id());
	GdkColor color = _internal_color(value);

	DW_MUTEX_LOCK;
	gdk_color_alloc(_dw_cmap, &color);
	_background[index] = color;
	DW_MUTEX_UNLOCK;
}

GdkGC *_set_colors(GdkWindow *window)
{
	GdkGC *gc = NULL;
	int index = _find_thread_index(dw_thread_id());

	if(!window)
		return NULL;
	gc = gdk_gc_new(window);
	if(gc)
	{
		gdk_gc_set_foreground(gc, &_foreground[index]);
		gdk_gc_set_background(gc, &_background[index]);
	}
	return gc;
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
	GdkGC *gc = NULL;

	DW_MUTEX_LOCK;
	if(handle)
		gc = _set_colors(handle->window);
	else if(pixmap)
		gc = _set_colors(pixmap->pixmap);
	if(gc)
	{
		gdk_draw_point(handle ? handle->window : pixmap->pixmap, gc, x, y);
		gdk_gc_unref(gc);
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
	GdkGC *gc = NULL;

	DW_MUTEX_LOCK;
	if(handle)
		gc = _set_colors(handle->window);
	else if(pixmap)
		gc = _set_colors(pixmap->pixmap);
	if(gc)
	{
		gdk_draw_line(handle ? handle->window : pixmap->pixmap, gc, x1, y1, x2, y2);
		gdk_gc_unref(gc);
	}
	DW_MUTEX_UNLOCK;
}

/* Draw a rectangle on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
void dw_draw_rect(HWND handle, HPIXMAP pixmap, int fill, int x, int y, int width, int height)
{
	int _locked_by_me = FALSE;
	GdkGC *gc = NULL;

	DW_MUTEX_LOCK;
	if(handle)
		gc = _set_colors(handle->window);
	else if(pixmap)
		gc = _set_colors(pixmap->pixmap);
	if(gc)
	{
		gdk_draw_rectangle(handle ? handle->window : pixmap->pixmap, gc, fill, x, y, width, height);
		gdk_gc_unref(gc);
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
	GdkGC *gc = NULL;
#if GTK_MAJOR_VERSION > 1
	PangoFontDescription *font;
#else
	GdkFont *font;
#endif
	char *fontname = "fixed";

	if(!text)
		return;

	DW_MUTEX_LOCK;
	if(handle)
	{
		fontname = (char *)gtk_object_get_data(GTK_OBJECT(handle), "fontname");
		gc = _set_colors(handle->window);
	}
	else if(pixmap)
	{
		fontname = (char *)gtk_object_get_data(GTK_OBJECT(pixmap->handle), "fontname");
		gc = _set_colors(pixmap->pixmap);
	}
	if(gc)
	{
#if GTK_MAJOR_VERSION > 1
		font = pango_font_description_from_string(fontname);
		if(font)
		{
			PangoContext *context = gdk_pango_context_get();

			if(context)
			{
				PangoLayout *layout = pango_layout_new(context);

				if(layout)
				{
					pango_layout_set_font_description(layout, font);
					pango_layout_set_text(layout, text, strlen(text));

					gdk_draw_layout(handle ? handle->window : pixmap->pixmap, gc, x, y + 2, layout);

					g_object_unref(layout);
				}
				g_object_unref(context);
			}
			pango_font_description_free(font);
		}
#else
		font = gdk_font_load(fontname);
		if(!font)
			font = gdk_font_load("fixed");
		if(font)
		{
			gint ascent;

			gdk_text_extents(font, text, strlen(text), NULL, NULL, NULL, &ascent, NULL);
			gdk_draw_text(handle ? handle->window : pixmap->pixmap, font, gc, x, y + ascent + 2, text, strlen(text));
			gdk_font_unref(font);
		}
#endif
		gdk_gc_unref(gc);
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
void dw_font_text_extents(HWND handle, HPIXMAP pixmap, char *text, int *width, int *height)
{
	int _locked_by_me = FALSE;
#if GTK_MAJOR_VERSION > 1
	PangoFontDescription *font;
#else
	GdkFont *font;
#endif
	char *fontname = NULL;

	if(!text)
		return;

	DW_MUTEX_LOCK;
	if(handle)
		fontname = (char *)gtk_object_get_data(GTK_OBJECT(handle), "fontname");
	else if(pixmap)
		fontname = (char *)gtk_object_get_data(GTK_OBJECT(pixmap->handle), "fontname");

#if GTK_MAJOR_VERSION > 1
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
#else

	font = gdk_font_load(fontname ? fontname : "fixed");
	if(!font)
		font = gdk_font_load("fixed");
	if(font)
	{
		if(width)
			*width = gdk_string_width(font, text);
		if(height)
			*height = gdk_string_height(font, text);
		gdk_font_unref(font);
	}
#endif
	DW_MUTEX_UNLOCK;
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
	pixmap->pixmap = gdk_pixmap_new(handle->window, width, height, depth);
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
#ifndef USE_IMLIB
	GdkBitmap *bitmap = NULL;
#endif
#if GTK_MAJOR_VERSION > 1
	GdkPixbuf *pixbuf;
#elif defined(USE_IMLIB)
	GdkImlibImage *image;
#endif
	char *file = alloca(strlen(filename) + 5);

	if (!file || !(pixmap = calloc(1,sizeof(struct _hpixmap))))
		return NULL;

	strcpy(file, filename);

	/* check if we can read from this file (it exists and read permission) */
	if(access(file, 04) != 0)
	{
		/* Try with .xpm extention */
		strcat(file, ".xpm");
		if(access(file, 04) != 0)
		{
			free(pixmap);
			return NULL;
		}
	}

	DW_MUTEX_LOCK;
#if GTK_MAJOR_VERSION > 1
	pixbuf = gdk_pixbuf_new_from_file(file, NULL);

	pixmap->width = gdk_pixbuf_get_width(pixbuf);
	pixmap->height = gdk_pixbuf_get_height(pixbuf);

	gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap->pixmap, &bitmap, 1);
	g_object_unref(pixbuf);
#elif defined(USE_IMLIB)
	image = gdk_imlib_load_image(file);

	pixmap->width = image->rgb_width;
	pixmap->height = image->rgb_height;

	gdk_imlib_render(image, pixmap->width, pixmap->height);
	pixmap->pixmap = gdk_imlib_copy_image(image);
	gdk_imlib_destroy_image(image);
#else
	pixmap->pixmap = gdk_pixmap_create_from_xpm(handle->window, &bitmap, &_colors[DW_CLR_PALEGRAY], file);
#endif
	pixmap->handle = handle;
	DW_MUTEX_UNLOCK;
	return pixmap;
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
	GdkBitmap *bitmap = NULL;
	HPIXMAP pixmap;
	int _locked_by_me = FALSE;

	if (!(pixmap = calloc(1,sizeof(struct _hpixmap))))
		return NULL;


	DW_MUTEX_LOCK;
	pixmap->pixmap = _find_pixmap(&bitmap, id, handle, &pixmap->width, &pixmap->height);
	if(pixmap->pixmap)
	{
#if GTK_MAJOR_VERSION < 2
		GdkPixmapPrivate *pvt = (GdkPixmapPrivate *)pixmap->pixmap;
		pixmap->width = pvt->width; pixmap->height = pvt->height;
#endif
	}
	DW_MUTEX_UNLOCK;
	return pixmap;
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void dw_flush(void)
{
#if GTK_MAJOR_VERSION < 2
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	gdk_flush();
	DW_MUTEX_UNLOCK;
#endif
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
	gdk_pixmap_unref(pixmap->pixmap);
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
void dw_pixmap_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc)
{
	/* Ok, these #ifdefs are going to get a bit confusing because
	 * when using gdk-pixbuf, pixmaps are really pixbufs, so we
	 * have to use the pixbuf functions on them, and thus convoluting
	 * the code here a bit. -Brian
	 */
	int _locked_by_me = FALSE;
	GdkGC *gc = NULL;

	if((!dest && (!destp || !destp->pixmap)) || (!src && (!srcp || !srcp->pixmap)))
		return;

	DW_MUTEX_LOCK;
	if(dest)
		gc = _set_colors(dest->window);
	else if(src)
		gc = _set_colors(src->window);
	else if(destp)
		gc = gdk_gc_new(destp->pixmap);
	else if(srcp)
		gc = gdk_gc_new(srcp->pixmap);

	if(gc)
	{
			gdk_draw_pixmap(dest ? dest->window : destp->pixmap, gc, src ? src->window : srcp->pixmap, xsrc, ysrc, xdest, ydest, width, height);
			gdk_gc_unref(gc);
	}
	DW_MUTEX_UNLOCK;
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
	char errorbuf[1024];


	if(!handle)
		return -1;

	if((len = strlen(name)) == 0)
		return	-1;

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
		return	-1;

	if(strlen(name) == 0)
		return	-1;

	*func = (void*)dlsym(handle, name);
	return	(NULL == *func);
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

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
	void (*threadfunc)(void *) = NULL;
	void **tmp = (void **)data;

	threadfunc = (void (*)(void *))tmp[0];

	_dw_thread_add(dw_thread_id());
	threadfunc(tmp[1]);
	_dw_thread_remove(dw_thread_id());
	free(tmp);
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

	tmp[0] = func;
	tmp[1] = data;

	pthread_create(&gtkthread, NULL, (void *)_dwthreadstart, (void *)tmp);
	return gtkthread;
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

#define DW_EXPAND (GTK_EXPAND | GTK_SHRINK | GTK_FILL)

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
void dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
	int _locked_by_me = FALSE;
	GtkWidget *tmp;

	if(!box)
		return;

	DW_MUTEX_LOCK;

	if((tmp  = gtk_object_get_data(GTK_OBJECT(box), "boxhandle")))
		box = tmp;

	if(!item)
	{
		item = gtk_label_new("");
		gtk_widget_show(item);
	}

	if(GTK_IS_TABLE(box))
	{
		int boxcount = (int)gtk_object_get_data(GTK_OBJECT(box), "boxcount");
		int boxtype = (int)gtk_object_get_data(GTK_OBJECT(box), "boxtype");

		/* If the item being packed is a box, then we use it's padding
		 * instead of the padding specified on the pack line, this is
		 * due to a bug in the OS/2 and Win32 renderer and a limitation
		 * of the GtkTable class.
		 */
		if(GTK_IS_TABLE(item))
		{
			GtkWidget *eventbox = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "eventbox");

			if(eventbox)
			{
				gtk_container_add(GTK_CONTAINER(eventbox), item);
				pad = (int)gtk_object_get_data(GTK_OBJECT(item), "boxpad");
				item = eventbox;
			}
		}

		if(boxtype == BOXVERT)
			gtk_table_resize(GTK_TABLE(box), boxcount + 1, 1);
		else
			gtk_table_resize(GTK_TABLE(box), 1, boxcount + 1);

		gtk_table_attach(GTK_TABLE(box), item, 0, 1, 0, 1, hsize ? DW_EXPAND : 0, vsize ? DW_EXPAND : 0, pad, pad);
		gtk_object_set_data(GTK_OBJECT(box), "boxcount", (gpointer)boxcount + 1);
		gtk_widget_set_usize(item, width, height);
		if(GTK_IS_RADIO_BUTTON(item))
		{
			GSList *group;
			GtkWidget *groupstart = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(box), "group");

			if(groupstart)
			{
				group = gtk_radio_button_group(GTK_RADIO_BUTTON(groupstart));
				gtk_radio_button_set_group(GTK_RADIO_BUTTON(item), group);
			}
			else
				gtk_object_set_data(GTK_OBJECT(box), "group", (gpointer)item);
		}
	}
	else
	{
		GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

		if(GTK_IS_TABLE(item))
		{
			GtkWidget *eventbox = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "eventbox");

			if(eventbox)
			{
				gtk_container_add(GTK_CONTAINER(eventbox), item);
				pad = (int)gtk_object_get_data(GTK_OBJECT(item), "boxpad");
				item = eventbox;
			}
		}

		gtk_container_border_width(GTK_CONTAINER(box), pad);
		gtk_container_add(GTK_CONTAINER(box), vbox);
		gtk_box_pack_end(GTK_BOX(vbox), item, TRUE, TRUE, 0);
		gtk_widget_show(vbox);

		gtk_widget_set_usize(item, width, height);
		gtk_object_set_user_data(GTK_OBJECT(box), vbox);
	}
	DW_MUTEX_UNLOCK;
}

/*
 * Sets the size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void dw_window_set_usize(HWND handle, unsigned long width, unsigned long height)
{
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if(GTK_IS_WINDOW(handle))
	{
		_size_allocate(GTK_WINDOW(handle));
		if(handle->window)
			gdk_window_resize(handle->window, width - _dw_border_width, height - _dw_border_height);
		gtk_window_set_default_size(GTK_WINDOW(handle), width - _dw_border_width, height - _dw_border_height);
		if(!gtk_object_get_data(GTK_OBJECT(handle), "_dw_size"))
		{
			gtk_object_set_data(GTK_OBJECT(handle), "_dw_width", (gpointer)width - _dw_border_width);
			gtk_object_set_data(GTK_OBJECT(handle), "_dw_height", (gpointer)height - _dw_border_height);
		}
	}
	else
		gtk_widget_set_usize(handle, width, height);
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
unsigned long dw_color_depth(void)
{
	int retval;
	int _locked_by_me = FALSE;

	DW_MUTEX_UNLOCK;
	retval = gdk_visual_get_best_depth();
	DW_MUTEX_UNLOCK;
	return retval;
}

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
void dw_window_set_pos(HWND handle, unsigned long x, unsigned long y)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(handle && handle->window)
		gdk_window_move(handle->window, x, y);
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
void dw_window_set_pos_size(HWND handle, unsigned long x, unsigned long y, unsigned long width, unsigned long height)
{
	int _locked_by_me = FALSE;

	if(!handle)
		return;

	DW_MUTEX_LOCK;
	if(GTK_IS_WINDOW(handle))
	{
		_size_allocate(GTK_WINDOW(handle));

		gtk_widget_set_uposition(handle, x, y);
		if(handle->window)
			gdk_window_resize(handle->window, width - _dw_border_width, height - _dw_border_height);
		gtk_window_set_default_size(GTK_WINDOW(handle), width - _dw_border_width, height - _dw_border_height);
	}
	else if(handle->window)
	{
		gdk_window_resize(handle->window, width, height);
		gdk_window_move(handle->window, x, y);
	}
	DW_MUTEX_UNLOCK;
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
void dw_window_get_pos_size(HWND handle, ULONG *x, ULONG *y, ULONG *width, ULONG *height)
{
	int _locked_by_me = FALSE;
	gint gx, gy, gwidth, gheight, gdepth;

	if(handle && handle->window)
	{
		DW_MUTEX_LOCK;

		gdk_window_get_geometry(handle->window, &gx, &gy, &gwidth, &gheight, &gdepth);
		gdk_window_get_root_origin(handle->window, &gx, &gy);
		if(x)
			*x = gx;
		if(y)
			*y = gy;
		if(GTK_IS_WINDOW(handle))
		{
			if(width)
				*width = gwidth + _dw_border_width;
			if(height)
				*height = gheight + _dw_border_height;
		}
		else
		{
			if(width)
				*width = gwidth;
			if(height)
				*height = gheight;
		}
		DW_MUTEX_UNLOCK;
	}
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
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	if(GTK_IS_CLIST(handle2))
	{
		if(style & DW_CCS_EXTENDSEL)
		{
			gtk_clist_set_selection_mode(GTK_CLIST(handle2), GTK_SELECTION_EXTENDED);
			gtk_object_set_data(GTK_OBJECT(handle2), "multi", (gpointer)1);
		}
		if(style & DW_CCS_SINGLESEL)
		{
			gtk_clist_set_selection_mode(GTK_CLIST(handle2), GTK_SELECTION_SINGLE);
			gtk_object_set_data(GTK_OBJECT(handle2), "multi", (gpointer)0);
		}
	}
	if(GTK_IS_LABEL(handle2))
	{
		if(style & DW_DT_CENTER || style & DW_DT_VCENTER)
		{
			gfloat x, y;

			x = y = DW_LEFT;

			if(style & DW_DT_CENTER)
				x = DW_CENTER;

			if(style & DW_DT_VCENTER)
				y = DW_CENTER;

			gtk_misc_set_alignment(GTK_MISC(handle2), x, y);
		}
		if(style & DW_DT_WORDBREAK)
			gtk_label_set_line_wrap(GTK_LABEL(handle), TRUE);
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
	pagearray = (GtkWidget **)gtk_object_get_data(GTK_OBJECT(handle), "pagearray");

	if(pagearray)
	{    
		for(z=0;z<256;z++)
		{
			if(!pagearray[z])
			{
				char text[100];
				int num = z;

				if(front)
					num |= 1 << 16;

				sprintf(text, "page%d", z);
				/* Save the real id and the creation flags */
				gtk_object_set_data(GTK_OBJECT(handle), text, (gpointer)num);
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
	GtkWidget *thispage, **pagearray = gtk_object_get_data(GTK_OBJECT(handle), "pagearray");

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

/* Return the logical page id from the physical page id */
int _get_logical_page(HWND handle, unsigned long pageid)
{
	int z;
	GtkWidget **pagearray = gtk_object_get_data(GTK_OBJECT(handle), "pagearray");
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
		if((pagearray = gtk_object_get_data(GTK_OBJECT(handle), "pagearray")))
			pagearray[pageid] = NULL;
	}
	DW_MUTEX_UNLOCK;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned int dw_notebook_page_query(HWND handle)
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
		gtk_notebook_set_page(GTK_NOTEBOOK(handle), pageid);
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
		char ptext[100];
		int num;
    
		sprintf(ptext, "page%d", (int)pageid);
		num = (int)gtk_object_get_data(GTK_OBJECT(handle), ptext);
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
	gchar *text = NULL;
	int num, z, realpage = -1, pad, _locked_by_me = FALSE;
	char ptext[100];

	DW_MUTEX_LOCK;
	sprintf(ptext, "page%d", (int)pageid);
	num = (int)gtk_object_get_data(GTK_OBJECT(handle), ptext);
	gtk_object_set_data(GTK_OBJECT(handle), ptext, NULL);
	pagearray = (GtkWidget **)gtk_object_get_data(GTK_OBJECT(handle), "pagearray");

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
					gtk_label_get(GTK_LABEL(oldlabel), &text);
				gtk_notebook_remove_page(GTK_NOTEBOOK(handle), z);
				realpage = z;
				break;
			}
		}
	}
  
	pagearray[pageid] = page;

	label = gtk_label_new(text ? text : "");

	if(GTK_IS_TABLE(page))
	{
		pad = (int)gtk_object_get_data(GTK_OBJECT(page), "boxpad");
		gtk_container_border_width(GTK_CONTAINER(page), pad);
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
	GtkWidget *handle2 = handle;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	gtk_object_set_data(GTK_OBJECT(handle), "appending", (gpointer)1);
	if(GTK_IS_LIST(handle2))
	{
		GtkWidget *list_item;
		GList *tmp;
		char *font = (char *)gtk_object_get_data(GTK_OBJECT(handle), "font");
		GdkColor *fore = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle2), "foregdk");
		GdkColor *back = (GdkColor *)gtk_object_get_data(GTK_OBJECT(handle2), "backgdk");

		list_item=gtk_list_item_new_with_label(text);

		if(font)
			_set_font(GTK_LIST_ITEM(list_item)->item.bin.child, font);
		if(fore && back)
			_set_color(GTK_LIST_ITEM(list_item)->item.bin.child,
					   DW_RGB(fore->red, fore->green, fore->blue),
					   DW_RGB(back->red, back->green, back->blue));

		tmp  = g_list_append(NULL, list_item);
		gtk_widget_show(list_item);
		gtk_list_append_items(GTK_LIST(handle2),tmp);
	}
	else if(GTK_IS_COMBO(handle2))
	{
		GList *tmp = (GList *)gtk_object_get_user_data(GTK_OBJECT(handle2));
		char *addtext = strdup(text);

		if(addtext)
		{
			tmp = g_list_append(tmp, addtext);
			gtk_object_set_user_data(GTK_OBJECT(handle2), tmp);
			gtk_combo_set_popdown_strings(GTK_COMBO(handle2), tmp);
		}
	}
	gtk_object_set_data(GTK_OBJECT(handle), "appending", NULL);
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
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	if(GTK_IS_COMBO(handle2))
	{
		GList *list, *tmp = (GList *)gtk_object_get_user_data(GTK_OBJECT(handle2));

		if(tmp)
		{
			list = tmp;
			while(list)
			{
				if(list->data)
					free(list->data);
				list=list->next;
			}
			g_list_free(tmp);
		}
		gtk_object_set_user_data(GTK_OBJECT(handle2), NULL);
	}
	else if(GTK_IS_LIST(handle2))
	{
		int count = dw_listbox_count(handle);

		gtk_list_clear_items(GTK_LIST(handle2), 0, count - 1);
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
    int retval = 0;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		handle2 = GTK_COMBO(handle)->list;
	}
	if(GTK_IS_LIST(handle2))
	{
		GList *list = GTK_LIST(handle2)->children;
		while(list)
		{
			list = list->next;
			retval++;
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
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	if(GTK_IS_LIST(handle2))
	{
		int count = dw_listbox_count(handle);
		GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(handle));
		float pos, ratio;

		if(count)
		{
			ratio = (float)top/(float)count;

			pos = (ratio * (float)(adj->upper - adj->lower)) + adj->lower;

			gtk_adjustment_set_value(adj, pos);
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
void dw_listbox_query_text(HWND handle, unsigned int index, char *buffer, unsigned int length)
{
	GtkWidget *handle2 = handle;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		handle2 = GTK_COMBO(handle)->list;
	}
	if(GTK_IS_LIST(handle2))
	{
		int counter = 0;
		GList *list = GTK_LIST(handle2)->children;

		while(list)
		{
			if(counter == index)
			{
				gchar *text = "";

				if(GTK_IS_LIST_ITEM(list->data))
				{
					GtkListItem *li = GTK_LIST_ITEM(list->data);

					if(GTK_IS_ITEM(&(li->item)))
					{
						GtkItem *i = &(li->item);

						if(GTK_IS_BIN(&(i->bin)))
						{
							GtkBin *b = &(i->bin);

							if(GTK_IS_LABEL(b->child))
								gtk_label_get(GTK_LABEL(b->child), &text);
						}
					}
				}
				else if(GTK_IS_COMBO(handle) && list->data)
					text = (gchar *)list->data;

				strncpy(buffer, (char *)text, length);
				break;
			}
			list = list->next;
			counter++;
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
void dw_listbox_set_text(HWND handle, unsigned int index, char *buffer)
{
	GtkWidget *handle2 = handle;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		handle2 = GTK_COMBO(handle)->list;
	}
	if(GTK_IS_LIST(handle2))
	{
		int counter = 0;
		GList *list = GTK_LIST(handle2)->children;

		while(list)
		{
			if(counter == index)
			{

				if(GTK_IS_LIST_ITEM(list->data))
				{
					GtkListItem *li = GTK_LIST_ITEM(list->data);

					if(GTK_IS_ITEM(&(li->item)))
					{
						GtkItem *i = &(li->item);

						if(GTK_IS_BIN(&(i->bin)))
						{
							GtkBin *b = &(i->bin);

							if(GTK_IS_LABEL(b->child))
								gtk_label_set_text(GTK_LABEL(b->child), buffer);
						}
					}
				}
				else if(GTK_IS_COMBO(handle))
				{
					if(list->data)
						g_free(list->data);
					list->data = g_strdup(buffer);
				}
				break;
			}
			list = list->next;
			counter++;
		}
	}
	DW_MUTEX_UNLOCK;
}

#if GTK_MAJOR_VERSION < 2
/* Check if a GList item is in another GList */
static int _dw_in_list(GList *item, GList *list)
{
	while(list)
	{
		if(list->data == item->data)
			return TRUE;

		list = list->next;
	}
	return FALSE;
}
#endif

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
int dw_listbox_selected_multi(HWND handle, int where)
{
	GtkWidget *handle2 = handle;
	int retval = DW_LIT_NONE;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	if(GTK_IS_LIST(handle2))
	{
#if GTK_MAJOR_VERSION > 1
		int counter = 0;
		GList *list = GTK_LIST(handle2)->children;

		while(list)
		{
			GtkItem *item = (GtkItem *)list->data;

			if(item &&
			   item->bin.container.widget.state == GTK_STATE_SELECTED
			   && counter > where)
			{
				retval = counter;
				break;
			}


			list = list->next;
			counter++;
		}
#else
		int counter = 0;
		GList *list = GTK_LIST(handle2)->children;

		while(list)
		{
			if(counter > where && _dw_in_list(list, GTK_LIST(handle2)->selection))
			{
				retval = counter;
				break;
			}

			list = list->next;
			counter++;
		}
#endif
	}
	DW_MUTEX_UNLOCK;
	return retval;
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
unsigned int dw_listbox_selected(HWND handle)
{
	GtkWidget *handle2 = handle;
	int retval = DW_LIT_NONE;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		retval = (unsigned int)gtk_object_get_data(GTK_OBJECT(handle), "item");
		DW_MUTEX_UNLOCK;
		return retval;
	}
	if(GTK_IS_LIST(handle2))
	{
		int counter = 0;
		GList *list = GTK_LIST(handle2)->children;
#if GTK_MAJOR_VERSION > 1
		GList *selection = GTK_LIST(handle2)->undo_unselection;
#else
		GList *selection = GTK_LIST(handle2)->selection;
#endif
		if(selection)
		{
			while(list)
			{
				if(list->data == selection->data)
				{
					retval = counter;
					break;
				}

				list = list->next;
				counter++;
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
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		handle2 = GTK_COMBO(handle)->list;
	}
	if(GTK_IS_LIST(handle2))
	{
		if(state)
			gtk_list_select_item(GTK_LIST(handle2), index);
		else
			gtk_list_unselect_item(GTK_LIST(handle2), index);
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
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(handle))
	{
		GtkWidget *tmp = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(handle));
		if(tmp)
			handle2 = tmp;
	}
	else if(GTK_IS_COMBO(handle))
	{
		handle2 = GTK_COMBO(handle)->list;
	}
	if(GTK_IS_LIST(handle2))
		gtk_list_clear_items(GTK_LIST(handle2), index, index);
	DW_MUTEX_UNLOCK;
}

/* Reposition the bar according to the percentage */
static gint _splitbar_size_allocate(GtkWidget *widget, GtkAllocation *event, gpointer data)
{
	float *percent = (float *)gtk_object_get_data(GTK_OBJECT(widget), "_dw_percent");
	int lastwidth = (int)gtk_object_get_data(GTK_OBJECT(widget), "_dw_lastwidth");
	int lastheight = (int)gtk_object_get_data(GTK_OBJECT(widget), "_dw_lastheight");

	/* Prevent infinite recursion ;) */  
	if(!percent || (lastwidth == event->width && lastheight == event->height))
		return FALSE;

	lastwidth = event->width; lastheight = event->height;
  
	gtk_object_set_data(GTK_OBJECT(widget), "_dw_lastwidth", (gpointer)lastwidth);
	gtk_object_set_data(GTK_OBJECT(widget), "_dw_lastheight", (gpointer)lastheight);

	if(GTK_IS_HPANED(widget))
		gtk_paned_set_position(GTK_PANED(widget), (int)(event->width * (*percent / 100.0)));
	if(GTK_IS_VPANED(widget))
		gtk_paned_set_position(GTK_PANED(widget), (int)(event->height * (*percent / 100.0)));
	gtk_object_set_data(GTK_OBJECT(widget), "_dw_waiting", NULL);
	return FALSE;
}

#if GTK_MAJOR_VERSION > 1
/* Figure out the new percentage */
static void _splitbar_accept_position(GObject *object, GParamSpec *pspec, gpointer data)
{
	GtkWidget *widget = (GtkWidget *)data;
	float *percent = (float *)gtk_object_get_data(GTK_OBJECT(widget), "_dw_percent");
	int size = 0, position = gtk_paned_get_position(GTK_PANED(widget));

	if(!percent || gtk_object_get_data(GTK_OBJECT(widget), "_dw_waiting"))
		return;

	if(GTK_IS_VPANED(widget))
		size = widget->allocation.height;
	else if(GTK_IS_HPANED(widget))
		size = widget->allocation.width;

	if(size > 0)
		*percent = ((float)(position * 100) / (float)size);
}
#endif

/*
 * Creates a splitbar window (widget) with given parameters.
 * Parameters:
 *       type: Value can be BOXVERT or BOXHORZ.
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
	if(type == BOXHORZ)
		tmp = gtk_hpaned_new();
	else
		tmp = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(tmp), topleft);
	gtk_paned_add2(GTK_PANED(tmp), bottomright);
	gtk_object_set_data(GTK_OBJECT(tmp), "id", (gpointer)id);
	*percent = 50.0;
	gtk_object_set_data(GTK_OBJECT(tmp), "_dw_percent", (gpointer)percent);
	gtk_object_set_data(GTK_OBJECT(tmp), "_dw_waiting", (gpointer)1);
	gtk_signal_connect(GTK_OBJECT(tmp), "size-allocate", GTK_SIGNAL_FUNC(_splitbar_size_allocate), NULL);
#if GTK_MAJOR_VERSION > 1
	g_signal_connect(G_OBJECT(tmp), "notify::position", (GCallback)_splitbar_accept_position, (gpointer)tmp);
#else
	gtk_paned_set_handle_size(GTK_PANED(tmp), 3);
#endif
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
		size = handle->allocation.height;
	else if(GTK_IS_HPANED(handle))
		size = handle->allocation.width;

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
void dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
	int _locked_by_me = FALSE;
	GtkWidget *tmp;

	if(!box)
		return;

	DW_MUTEX_LOCK;

	if((tmp  = gtk_object_get_data(GTK_OBJECT(box), "boxhandle")))
		box = tmp;

	if(!item)
	{
		item = gtk_label_new("");
		gtk_widget_show(item);
	}

	if(GTK_IS_TABLE(box))
	{
		int boxcount = (int)gtk_object_get_data(GTK_OBJECT(box), "boxcount");
		int boxtype = (int)gtk_object_get_data(GTK_OBJECT(box), "boxtype");
		int x, y;

		/* If the item being packed is a box, then we use it's padding
		 * instead of the padding specified on the pack line, this is
		 * due to a bug in the OS/2 and Win32 renderer and a limitation
		 * of the GtkTable class.
		 */
		if(GTK_IS_TABLE(item))
		{
			GtkWidget *eventbox = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "eventbox");

			if(eventbox)
			{
				gtk_container_add(GTK_CONTAINER(eventbox), item);
				pad = (int)gtk_object_get_data(GTK_OBJECT(item), "boxpad");
				item = eventbox;
			}
		}

		if(boxtype == BOXVERT)
		{
			x = 0;
			y = boxcount;
			gtk_table_resize(GTK_TABLE(box), boxcount + 1, 1);
		}
		else
		{
			x = boxcount;
			y = 0;
			gtk_table_resize(GTK_TABLE(box), 1, boxcount + 1);
		}

		gtk_table_attach(GTK_TABLE(box), item, x, x + 1, y, y + 1, hsize ? DW_EXPAND : 0, vsize ? DW_EXPAND : 0, pad, pad);
		gtk_object_set_data(GTK_OBJECT(box), "boxcount", (gpointer)boxcount + 1);
		gtk_widget_set_usize(item, width, height);
		if(GTK_IS_RADIO_BUTTON(item))
		{
			GSList *group;
			GtkWidget *groupstart = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(box), "group");

			if(groupstart)
			{
				group = gtk_radio_button_group(GTK_RADIO_BUTTON(groupstart));
				gtk_radio_button_set_group(GTK_RADIO_BUTTON(item), group);
			}
			else
				gtk_object_set_data(GTK_OBJECT(box), "group", (gpointer)item);
		}
	}
	else
	{
		GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

		if(GTK_IS_TABLE(item))
		{
			GtkWidget *eventbox = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(item), "eventbox");

			if(eventbox)
			{
				gtk_container_add(GTK_CONTAINER(eventbox), item);
				pad = (int)gtk_object_get_data(GTK_OBJECT(item), "boxpad");
				item = eventbox;
			}
		}

		gtk_container_border_width(GTK_CONTAINER(box), pad);
		gtk_container_add(GTK_CONTAINER(box), vbox);
		gtk_box_pack_end(GTK_BOX(vbox), item, TRUE, TRUE, 0);
		gtk_widget_show(vbox);

		gtk_widget_set_usize(item, width, height);
		gtk_object_set_user_data(GTK_OBJECT(box), vbox);
	}
	DW_MUTEX_UNLOCK;
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
	gtk_object_set_data(GTK_OBJECT(window),  "_dw_defaultitem", (gpointer)defaultitem);
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
	gtk_signal_connect(GTK_OBJECT(window), "key_press_event", GTK_SIGNAL_FUNC(_default_key_press_event), next);
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

/* Internal function to handle the file OK press */
static gint _gtk_file_ok(GtkWidget *widget, DWDialog *dwwait)
{
#if GTK_MAJOR_VERSION > 1
	const char *tmp;
#else
	char *tmp;
#endif
	char *tmpdup=NULL;

	if(!dwwait)
		return FALSE;

	if((tmp = gtk_file_selection_get_filename(GTK_FILE_SELECTION(dwwait->data))))
		tmpdup = strdup(tmp);
	gtk_widget_destroy(GTK_WIDGET(dwwait->data));
	_dw_file_active = 0;
	dw_dialog_dismiss(dwwait, (void *)tmpdup);
	return FALSE;
}

/* Internal function to handle the file Cancel press */
static gint _gtk_file_cancel(GtkWidget *widget, DWDialog *dwwait)
{
	if(!dwwait)
		return FALSE;

	gtk_widget_destroy(GTK_WIDGET(dwwait->data));
	_dw_file_active = 0;
	dw_dialog_dismiss(dwwait, NULL);
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
char *dw_file_browse(char *title, char *defpath, char *ext, int flags)
{
	GtkWidget *filew;
	int _locked_by_me = FALSE;
	DWDialog *dwwait;

	DW_MUTEX_LOCK;

	/* The DW mutex should be sufficient for
	 * insuring no thread changes this unknowingly.
	 */
	if(_dw_file_active)
	{
		DW_MUTEX_UNLOCK;
		return NULL;
	}

	_dw_file_active = 1;

	filew = gtk_file_selection_new(title);

	dwwait = dw_dialog_new((void *)filew);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->ok_button), "clicked", (GtkSignalFunc) _gtk_file_ok, dwwait);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filew)->cancel_button), "clicked", (GtkSignalFunc) _gtk_file_cancel, dwwait);

	if(defpath)
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew), defpath);

	gtk_widget_show(filew);

	DW_MUTEX_UNLOCK;

	return (char *)dw_dialog_wait(dwwait);
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
	/* Is there a way to find the webbrowser in Unix? */
	char *execargs[3], *browser = "netscape";

	execargs[0] = browser;
	execargs[1] = url;
	execargs[2] = NULL;

	return dw_exec(browser, DW_EXEC_GUI, execargs);
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
	int _locked_by_me = FALSE;

	if(!window)
		return;

	DW_MUTEX_LOCK;
	if(GTK_IS_OBJECT(window))
	{
		if(GTK_IS_SCROLLED_WINDOW(window))
		{
			HWND thiswindow = (HWND)gtk_object_get_user_data(GTK_OBJECT(window));

			if(thiswindow && GTK_IS_OBJECT(thiswindow))
				gtk_object_set_data(GTK_OBJECT(thiswindow), dataname, (gpointer)data);
		}
		if(GTK_IS_COMBO(window))
			gtk_object_set_data(GTK_OBJECT(GTK_COMBO(window)->entry), dataname, (gpointer)data);
		gtk_object_set_data(GTK_OBJECT(window), dataname, (gpointer)data);
	}
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
	int _locked_by_me = FALSE;
	void *ret = NULL;

	if(!window)
		return NULL;

	DW_MUTEX_LOCK;
    if(GTK_IS_OBJECT(window))
		ret = (void *)gtk_object_get_data(GTK_OBJECT(window), dataname);
	DW_MUTEX_UNLOCK;
	return ret;
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
	int tag, _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	tag = gtk_timeout_add(interval, (GtkFunction)sigfunc, data);
	DW_MUTEX_UNLOCK;
	return tag;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(int id)
{
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	gtk_timeout_remove(id);
	DW_MUTEX_UNLOCK;
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
	SignalHandler *work = malloc(sizeof(SignalHandler));
	void *thisfunc  = _findsigfunc(signame);
	char *thisname = signame;
	HWND thiswindow = window;
	int _locked_by_me = FALSE;

	DW_MUTEX_LOCK;
	if(GTK_IS_SCROLLED_WINDOW(thiswindow))
	{
		thiswindow = (HWND)gtk_object_get_user_data(GTK_OBJECT(window));
	}

	if(GTK_IS_MENU_ITEM(thiswindow) && strcmp(signame, "clicked") == 0)
	{
		thisname = "activate";
		thisfunc = _findsigfunc(thisname);
	}
	else if(GTK_IS_CLIST(thiswindow) && strcmp(signame, "container-context") == 0)
	{
		thisname = "button_press_event";
		thisfunc = _findsigfunc("container-context");
	}
#if GTK_MAJOR_VERSION > 1
	else if(GTK_IS_TREE_VIEW(thiswindow)  && strcmp(signame, "container-context") == 0)
	{
		thisfunc = _findsigfunc("tree-context");

		work->window = window;
		work->data = data;
		work->func = sigfunc;

		gtk_signal_connect(GTK_OBJECT(thiswindow), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);
		gtk_signal_connect(GTK_OBJECT(window), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);
		DW_MUTEX_UNLOCK;
		return;
	}
	else if(GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, "tree-select") == 0)
	{
		work->window = window;
		work->data = data;
		work->func = sigfunc;
    
		thiswindow = (GtkWidget *)gtk_tree_view_get_selection(GTK_TREE_VIEW(thiswindow));
		thisname = "changed";
    
		g_signal_connect(G_OBJECT(thiswindow), thisname, (GCallback)thisfunc, work);
		DW_MUTEX_UNLOCK;
		return;
	}
#else
	else if(GTK_IS_TREE(thiswindow)  && strcmp(signame, "container-context") == 0)
	{
		thisfunc = _findsigfunc("tree-context");

		work->window = window;
		work->data = data;
		work->func = sigfunc;

		gtk_object_set_data(GTK_OBJECT(thiswindow), "container-context-func", (gpointer)thisfunc);
		gtk_object_set_data(GTK_OBJECT(thiswindow), "container-context-data", (gpointer)work);
		gtk_signal_connect(GTK_OBJECT(thiswindow), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);
		gtk_signal_connect(GTK_OBJECT(window), "button_press_event", GTK_SIGNAL_FUNC(thisfunc), work);
		DW_MUTEX_UNLOCK;
		return;
	}
	else if(GTK_IS_TREE(thiswindow) && strcmp(signame, "tree-select") == 0)
	{
		if(thisfunc)
		{
			gtk_object_set_data(GTK_OBJECT(thiswindow), "select-child-func", (gpointer)thisfunc);
			gtk_object_set_data(GTK_OBJECT(thiswindow), "select-child-data", (gpointer)work);
		}
		thisname = "select-child";
	}
#endif
	else if(GTK_IS_CLIST(thiswindow) && strcmp(signame, "container-select") == 0)
	{
		thisname = "button_press_event";
		thisfunc = _findsigfunc("container-select");
	}
	else if(GTK_IS_CLIST(thiswindow) && strcmp(signame, "tree-select") == 0)
	{
		thisname = "select_row";
		thisfunc = (void *)_container_select_row;
	}
	else if(GTK_IS_COMBO(thiswindow) && strcmp(signame, "item-select") == 0)
	{
		thisname = "select_child";
		thiswindow = GTK_COMBO(thiswindow)->list;
	}
	else if(GTK_IS_LIST(thiswindow) && strcmp(signame, "item-select") == 0)
	{
		thisname = "select_child";
	}
	else if(strcmp(signame, "set-focus") == 0)
	{
		thisname = "focus-in-event";
		if(GTK_IS_COMBO(thiswindow))
			thiswindow = GTK_COMBO(thiswindow)->entry;
	}
	else if(strcmp(signame, "lose-focus") == 0)
	{
		thisname = "focus-out-event";
		if(GTK_IS_COMBO(thiswindow))
			thiswindow = GTK_COMBO(thiswindow)->entry;
	}
	else if(GTK_IS_VSCALE(thiswindow) || GTK_IS_HSCALE(thiswindow) ||
			GTK_IS_VSCROLLBAR(thiswindow) || GTK_IS_HSCROLLBAR(thiswindow))
	{
		thiswindow = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(thiswindow), "adjustment");
	}

	if(!thisfunc || !thiswindow)
	{
		free(work);
		DW_MUTEX_UNLOCK;
		return;
	}

	work->window = window;
	work->data = data;
	work->func = sigfunc;

	gtk_signal_connect(GTK_OBJECT(thiswindow), thisname, GTK_SIGNAL_FUNC(thisfunc), work);
	DW_MUTEX_UNLOCK;
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void dw_signal_disconnect_by_name(HWND window, char *signame)
{
#if 0
	gtk_signal_disconnect_by_name(window, signame);
#endif
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void dw_signal_disconnect_by_window(HWND window)
{
#if 0
	gtk_signal_disconnect_by_window(window);
#endif
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void dw_signal_disconnect_by_data(HWND window, void *data)
{
	dw_signal_disconnect_by_data(window, data);
}

