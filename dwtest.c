#define INCL_DOS
#define INCL_WIN

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dw.h"


unsigned long flStyle = DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
	DW_FCF_SHELLPOSITION | DW_FCF_TASKLIST | DW_FCF_DLGBORDER;

HWND mainwindow,
     entryfield,
     okbutton,
     cancelbutton,
     lbbox,
     notebookbox,
     notebookbox1,
     notebookbox2,
     notebook,
     scrollbar,
     status,
     stext,
     pagebox,
     textbox1, textbox2,
     buttonbox;

int DWSIGNAL exit_callback(HWND window, void *data)
{
	dw_window_destroy((HWND)data);
	exit(0);
	return -1;
}

int DWSIGNAL test_callback(HWND window, void *data)
{
	dw_window_destroy((HWND)data);
	exit(0);
	return -1;
}

int DWSIGNAL browse_callback(HWND window, void *data)
{
	dw_file_browse("test string", NULL, "c", DW_FILE_OPEN );
	return 0;
}

void archive_add(void)
{
	HWND browsebutton, browsebox;

	lbbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(notebookbox1, lbbox, 150, 70, TRUE, TRUE, 0);

	/* Archive Name */
	stext = dw_text_new("Archive Name", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 2);

	browsebox = dw_box_new(BOXHORZ, 0);

	dw_box_pack_start(lbbox, browsebox, 0, 0, TRUE, TRUE, 0);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(browsebox, entryfield, 100, 15, TRUE, TRUE, 4);

	browsebutton = dw_button_new("Browse", 1001L);

	dw_box_pack_start(browsebox, browsebutton, 30, 15, TRUE, TRUE, 0);

	dw_window_set_color(browsebox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
	dw_window_set_font(browsebutton, "9.WarpSans");
	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

	/* Archive Description */
	stext = dw_text_new("Archive Description", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 4);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(lbbox, entryfield, 130, 15, TRUE, TRUE, 4);

	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

	/* Comments */
	stext = dw_text_new("Comments", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 4);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(lbbox, entryfield, 130, 15, TRUE, TRUE, 4);

	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

	/* Buttons */
	buttonbox = dw_box_new(BOXHORZ, 10);

	dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, TRUE, 0);

	okbutton = dw_button_new("Ok", 1001L);

	dw_box_pack_start(buttonbox, okbutton, 130, 30, TRUE, TRUE, 2);

	cancelbutton = dw_button_new("Cancel", 1002L);

	dw_box_pack_start(buttonbox, cancelbutton, 130, 30, TRUE, TRUE, 2);

	/* Set some nice fonts and colors */
	dw_window_set_color(lbbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
	dw_window_set_color(buttonbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
	dw_window_set_color(okbutton, DW_CLR_PALEGRAY, DW_CLR_DARKCYAN);
	dw_window_set_font(okbutton, "9.WarpSans");
	dw_window_set_font(cancelbutton, "9.WarpSans");

	dw_signal_connect(browsebutton, "clicked", DW_SIGNAL_FUNC(browse_callback), (void *)notebookbox1);
	dw_signal_connect(okbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)notebookbox1);
	dw_signal_connect(cancelbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)notebookbox1);
}


int font_width = 9;
int font_height=6;
int rows=100,width1=6,width2=50;

/* This gets called when a part of the graph needs to be repainted. */
int DWSIGNAL text_expose(HWND hwnd, DWExpose *exp, void *data)
{
	HPIXMAP hpm = (HPIXMAP)data;

	dw_pixmap_bitblt(hwnd, NULL, 0, 0, font_width*width1, font_height*rows, NULL, hpm, 0, 0 );
	dw_flush();
	return TRUE;
}

/* Callback to handle user selection of the scrollbar position */
void DWSIGNAL scrollbar_valuechanged(HWND hwnd, int value, void *data)
{
	if(data)
	{
		HWND stext = (HWND)data;
		char tmpbuf[100];

		sprintf(tmpbuf, "%d", value);
		dw_window_set_text(stext, tmpbuf);
	}
}
void text_add(void)
{
	int i,y,depth = dw_color_depth();
	char buf[10];
	HPIXMAP text1pm,text2pm;

	pagebox = dw_box_new(BOXVERT, 5);
	dw_box_pack_start( notebookbox2, pagebox, 1, 1, TRUE, TRUE, 0);

	textbox1 = dw_render_new( 100 );
	dw_box_pack_start( pagebox, textbox1, font_width*width1, font_height*rows, TRUE, TRUE, 4);
	dw_window_set_font(textbox1, "9.WarpSans");

	textbox2 = dw_render_new( 101 );
	dw_box_pack_start( pagebox, textbox2, font_width*width2, font_height*rows, TRUE, TRUE, 4);
	dw_window_set_font(textbox2, "9.WarpSans");

	scrollbar = dw_scrollbar_new(FALSE, 100, 50);
	dw_box_pack_start( pagebox, scrollbar, 100, 20, TRUE, FALSE, 0);
	dw_scrollbar_set_range(scrollbar, 100, 50);
	dw_scrollbar_set_pos(scrollbar, 10);

	status = dw_status_text_new("", 0);
	dw_box_pack_start( pagebox, status, 100, 20, TRUE, FALSE, 1);

	text1pm = dw_pixmap_new( textbox1, font_width*width1, font_height*rows, depth );
	text2pm = dw_pixmap_new( textbox2, font_width*width2, font_height*rows, depth );

	dw_color_foreground_set(DW_CLR_WHITE);
	dw_draw_rect(0, text1pm, TRUE, 0, 0, font_width*width1, font_height*rows);
	dw_draw_rect(0, text2pm, TRUE, 0, 0, font_width*width2, font_height*rows);

	dw_font_text_extents( NULL, text1pm, "O", &font_width, &font_height );
	dw_messagebox("DWTest", "Width: %d Height: %d\n", font_width, font_height);

	dw_color_background_set( DW_CLR_WHITE );
	for ( i = 0;i < 100; i++)
	{
		y = i*font_height;

		dw_color_foreground_set( DW_CLR_BLACK );
		sprintf( buf, "%6.6d", i );
		dw_draw_text( NULL, text1pm, 0, y, buf);
		dw_draw_text( NULL, text2pm, 0, y, buf);
	}
	dw_signal_connect(textbox1, "expose_event", DW_SIGNAL_FUNC(text_expose), text1pm);
	dw_signal_connect(textbox2, "expose_event", DW_SIGNAL_FUNC(text_expose), text2pm);
	dw_signal_connect(scrollbar, "value_changed", DW_SIGNAL_FUNC(scrollbar_valuechanged), (void *)status);
}

/* Beep every second */
int DWSIGNAL timer_callback(void *data)
{
	dw_beep(200, 200);

	/* Return TRUE so we get called again */
	return TRUE;
}

/*
 * Let's demonstrate the functionality of this library. :)
 */
int main(int argc, char *argv[])
{
	ULONG notebookpage1;
	ULONG notebookpage2;
	int timerid;

	dw_init(TRUE, argc, argv);

	mainwindow = dw_window_new( HWND_DESKTOP, "dwindows test", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX );

	notebookbox = dw_box_new( BOXVERT, 5 );
	dw_box_pack_start( mainwindow, notebookbox, 1, 1, TRUE, TRUE, 0);

	notebook = dw_notebook_new( 1, TRUE );
	dw_box_pack_start( notebookbox, notebook, 100, 100, TRUE, TRUE, 0);
	dw_window_set_font( notebook, "8.WarpSans");

	notebookbox1 = dw_box_new( BOXVERT, 5 );
	notebookpage1 = dw_notebook_page_new( notebook, 0, TRUE );
	dw_notebook_pack( notebook, notebookpage1, notebookbox1 );
	dw_notebook_page_set_text( notebook, notebookpage1, "first page");
	archive_add();

	notebookbox2 = dw_box_new( BOXVERT, 5 );
	notebookpage2 = dw_notebook_page_new( notebook, 1, FALSE );
	dw_notebook_pack( notebook, notebookpage2, notebookbox2 );
	dw_notebook_page_set_text( notebook, notebookpage2, "second page");
	text_add();

	dw_signal_connect(mainwindow, "delete_event", DW_SIGNAL_FUNC(exit_callback), (void *)mainwindow);
	timerid = dw_timer_connect(1000, DW_SIGNAL_FUNC(timer_callback), 0);
	dw_window_set_usize(mainwindow, 640, 480);
	dw_window_show(mainwindow);

	dw_main();

	return 0;
}
