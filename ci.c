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
	 stext,
	 buttonbox;

int test_callback(HWND window, void *data)
{
	dw_window_destroy((HWND)data);
	exit(0);
	return -1;
}

void archive_add(void)
{
	HWND browsebutton, browsebox;

	mainwindow = dw_window_new(HWND_DESKTOP, "Add new archive", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);

	lbbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(mainwindow, lbbox, 150, 70, TRUE, TRUE, 0);

    /* Archive Name */
	stext = dw_text_new("Archive Name", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 2);

	browsebox = dw_box_new(BOXHORZ, 0);

	dw_box_pack_start(lbbox, browsebox, 130, 15, TRUE, TRUE, 0);

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

	dw_box_pack_start(lbbox, buttonbox, 140, 210, TRUE, TRUE, 0);

	okbutton = dw_button_new("Ok", 1001L);

	dw_box_pack_start(buttonbox, okbutton, 130, 30, TRUE, TRUE, 2);

	cancelbutton = dw_button_new("Cancel", 1002L);

	dw_box_pack_start(buttonbox, cancelbutton, 130, 30, TRUE, TRUE, 2);

	/* Set some nice fonts and colors */
	dw_window_set_color(lbbox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
	dw_window_set_color(buttonbox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
	dw_window_set_font(okbutton, "9.WarpSans");
	dw_window_set_font(cancelbutton, "9.WarpSans");

	dw_signal_connect(browsebutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(okbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(cancelbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(mainwindow, "delete_event", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);

	dw_window_set_usize(mainwindow, 340, 250);

	dw_window_show(mainwindow);
}

void object_add(void)
{
	mainwindow = dw_window_new(HWND_DESKTOP, "Add new object", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);

	lbbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(mainwindow, lbbox, 150, 70, TRUE, TRUE, 0);

    /* Object Name */
	stext = dw_text_new("Object Name", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 0);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(lbbox, entryfield, 130, 15, TRUE, TRUE, 0);

	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

    /* Object ID */
	stext = dw_text_new("Object ID", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 0);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(lbbox, entryfield, 130, 15, TRUE, TRUE, 0);

	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

    /* Object Type */
	stext = dw_text_new("Object Type", 0);

	dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

	dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 0);

	entryfield = dw_entryfield_new("", 100L);

	dw_box_pack_start(lbbox, entryfield, 130, 15, TRUE, TRUE, 0);

	dw_window_set_font(stext, "9.WarpSans");
	dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);
	dw_window_set_font(entryfield, "9.WarpSans");

    /* Buttons */
	buttonbox = dw_box_new(BOXHORZ, 10);

	dw_box_pack_start(lbbox, buttonbox, 140, 210, TRUE, TRUE, 0);

	okbutton = dw_button_new("Ok", 1001L);

	dw_box_pack_start(buttonbox, okbutton, 50, 30, TRUE, TRUE, 0);

	cancelbutton = dw_button_new("Cancel", 1002L);

	dw_box_pack_start(buttonbox, cancelbutton, 50, 30, TRUE, TRUE, 0);

	/* Set some nice fonts and colors */
	dw_window_set_color(lbbox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
	dw_window_set_color(buttonbox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
	dw_window_set_font(okbutton, "9.WarpSans");
	dw_window_set_font(cancelbutton, "9.WarpSans");

	dw_signal_connect(okbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(cancelbutton, "clicked", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);
	dw_signal_connect(mainwindow, "delete_event", DW_SIGNAL_FUNC(test_callback), (void *)mainwindow);

	dw_window_set_usize(mainwindow, 340, 250);

	dw_window_show(mainwindow);
}

/*
 * Let's demonstrate the functionality of this library. :)
 */
int main(void)
{
	dw_init(TRUE);

	archive_add();
	dw_main(0L, NULL);

	object_add();
	dw_main(0L, NULL);

	return 0;
}
