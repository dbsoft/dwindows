#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dw.h"

/* Select a fixed width font for our platform */
#ifdef __OS2__
#define FIXEDFONT "5.System VIO"
#define FOLDER_ICON_NAME "os2\\folder"
#define FILE_ICON_NAME "os2\\file"
#elif defined(__WIN32__)
#define FIXEDFONT "10.Terminal"
#define FOLDER_ICON_NAME "win\\folder"
#define FILE_ICON_NAME "win\\file"
#elif defined(__MAC__)
#define FIXEDFONT "8.Monaco"
#define FOLDER_ICON_NAME "mac/folder"
#define FILE_ICON_NAME "mac/file"
#elif GTK_MAJOR_VERSION > 1
#define FIXEDFONT "monospace 10"
#define FOLDER_ICON_NAME "gtk/folder"
#define FILE_ICON_NAME "gtk/file"
#else
#define FIXEDFONT "fixed"
#define FOLDER_ICON_NAME "gtk/folder"
#define FILE_ICON_NAME "gtk/file"
#endif

#define SCROLLBARWIDTH 14

unsigned long flStyle = DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
   DW_FCF_SHELLPOSITION | DW_FCF_TASKLIST | DW_FCF_DLGBORDER;

unsigned long current_color = DW_RGB(100,100,100);

int iteration = 0;
   void create_button( int);

HWND mainwindow,
   entryfield,
   checkable_menuitem,
   noncheckable_menuitem,
   cursortogglebutton,
   colorchoosebutton,
   okbutton,
   cancelbutton,
   lbbox,
   combox,
   combobox1,
   combobox2,
   spinbutton,
   slider,
   notebookbox,
   notebookbox1,
   notebookbox2,
   notebookbox3,
   notebookbox4,
   notebookbox5,
   notebookbox6,
   notebookbox7,
   html,
   rawhtml,
   notebook,
   vscrollbar,
   hscrollbar,
   status, status1,
   container_status,
   tree_status,
   stext,
   tree,
   container,
   container_mle,
   pagebox,
   treebox,
   containerbox,
   textbox1, textbox2, textboxA,
   gap_box,
   buttonbox,
   buttonsbox,
   buttonboxperm,
   cal,
   filetoolbarbox;

HMENUI mainmenubar,changeable_menu;
#define CHECKABLE_MENUITEMID 2001
#define NONCHECKABLE_MENUITEMID 2002

void *containerinfo;


int menu_enabled = 1;

HPIXMAP text1pm,text2pm;
HICN fileicon,foldericon;
int mle_point=-1;

int font_width = 8;
int font_height=12;
int rows=100,width1=6,cols=80;
char *current_file = NULL;
int timerid;
int num_lines=0;
int max_linewidth=0;
int current_row=0,current_col=0;
int cursor_arrow = 1;

FILE *fp=NULL;
char **lp;

char *resolve_keyname( int vk )
{
   char *keyname;
   switch(vk)
      {
         case  VK_LBUTTON : keyname =  "VK_LBUTTON"; break;
         case  VK_RBUTTON : keyname =  "VK_RBUTTON"; break;
         case  VK_CANCEL  : keyname =  "VK_CANCEL"; break;
         case  VK_MBUTTON : keyname =  "VK_MBUTTON"; break;
/*       case  VK_BACK    : keyname =  "VK_BACK"; break;*/
         case  VK_TAB     : keyname =  "VK_TAB"; break;
         case  VK_CLEAR   : keyname =  "VK_CLEAR"; break;
         case  VK_RETURN  : keyname =  "VK_RETURN"; break;
/*       case  VK_MENU    : keyname =  "VK_MENU"; break;*/
         case  VK_PAUSE   : keyname =  "VK_PAUSE"; break;
         case  VK_CAPITAL : keyname =  "VK_CAPITAL"; break;
         case  VK_ESCAPE  : keyname =  "VK_ESCAPE"; break;
         case  VK_SPACE   : keyname =  "VK_SPACE"; break;
         case  VK_PRIOR   : keyname =  "VK_PRIOR"; break;
         case  VK_NEXT    : keyname =  "VK_NEXT"; break;
         case  VK_END     : keyname =  "VK_END"; break;
         case  VK_HOME    : keyname =  "VK_HOME"; break;
         case  VK_LEFT    : keyname =  "VK_LEFT"; break;
         case  VK_UP      : keyname =  "VK_UP"; break;
         case  VK_RIGHT   : keyname =  "VK_RIGHT"; break;
         case  VK_DOWN    : keyname =  "VK_DOWN"; break;
         case  VK_SELECT  : keyname =  "VK_SELECT"; break;
         case  VK_PRINT   : keyname =  "VK_PRINT"; break;
         case  VK_EXECUTE : keyname =  "VK_EXECUTE"; break;
         case  VK_SNAPSHOT: keyname =  "VK_SNAPSHOT"; break;
         case  VK_INSERT  : keyname =  "VK_INSERT"; break;
         case  VK_DELETE  : keyname =  "VK_DELETE"; break;
         case  VK_HELP    : keyname =  "VK_HELP"; break;
         case  VK_LWIN    : keyname =  "VK_LWIN"; break;
         case  VK_RWIN    : keyname =  "VK_RWIN"; break;
         case  VK_NUMPAD0 : keyname =  "VK_NUMPAD0"; break;
         case  VK_NUMPAD1 : keyname =  "VK_NUMPAD1"; break;
         case  VK_NUMPAD2 : keyname =  "VK_NUMPAD2"; break;
         case  VK_NUMPAD3 : keyname =  "VK_NUMPAD3"; break;
         case  VK_NUMPAD4 : keyname =  "VK_NUMPAD4"; break;
         case  VK_NUMPAD5 : keyname =  "VK_NUMPAD5"; break;
         case  VK_NUMPAD6 : keyname =  "VK_NUMPAD6"; break;
         case  VK_NUMPAD7 : keyname =  "VK_NUMPAD7"; break;
         case  VK_NUMPAD8 : keyname =  "VK_NUMPAD8"; break;
         case  VK_NUMPAD9 : keyname =  "VK_NUMPAD9"; break;
         case  VK_MULTIPLY: keyname =  "VK_MULTIPLY"; break;
         case  VK_ADD     : keyname =  "VK_ADD"; break;
         case  VK_SEPARATOR: keyname = "VK_SEPARATOR"; break;
         case  VK_SUBTRACT: keyname =  "VK_SUBTRACT"; break;
         case  VK_DECIMAL : keyname =  "VK_DECIMAL"; break;
         case  VK_DIVIDE  : keyname =  "VK_DIVIDE"; break;
         case  VK_F1      : keyname =  "VK_F1"; break;
         case  VK_F2      : keyname =  "VK_F2"; break;
         case  VK_F3      : keyname =  "VK_F3"; break;
         case  VK_F4      : keyname =  "VK_F4"; break;
         case  VK_F5      : keyname =  "VK_F5"; break;
         case  VK_F6      : keyname =  "VK_F6"; break;
         case  VK_F7      : keyname =  "VK_F7"; break;
         case  VK_F8      : keyname =  "VK_F8"; break;
         case  VK_F9      : keyname =  "VK_F9"; break;
         case  VK_F10     : keyname =  "VK_F10"; break;
         case  VK_F11     : keyname =  "VK_F11"; break;
         case  VK_F12     : keyname =  "VK_F12"; break;
         case  VK_F13     : keyname =  "VK_F13"; break;
         case  VK_F14     : keyname =  "VK_F14"; break;
         case  VK_F15     : keyname =  "VK_F15"; break;
         case  VK_F16     : keyname =  "VK_F16"; break;
         case  VK_F17     : keyname =  "VK_F17"; break;
         case  VK_F18     : keyname =  "VK_F18"; break;
         case  VK_F19     : keyname =  "VK_F19"; break;
         case  VK_F20     : keyname =  "VK_F20"; break;
         case  VK_F21     : keyname =  "VK_F21"; break;
         case  VK_F22     : keyname =  "VK_F22"; break;
         case  VK_F23     : keyname =  "VK_F23"; break;
         case  VK_F24     : keyname =  "VK_F24"; break;
         case  VK_NUMLOCK : keyname =  "VK_NUMLOCK"; break;
         case  VK_SCROLL  : keyname =  "VK_SCROLL"; break;
         case  VK_LSHIFT  : keyname =  "VK_LSHIFT"; break;
         case  VK_RSHIFT  : keyname =  "VK_RSHIFT"; break;
         case  VK_LCONTROL: keyname =  "VK_LCONTROL"; break;
         case  VK_RCONTROL: keyname =  "VK_RCONTROL"; break;
/*       case  VK_LMENU   : keyname =  "VK_LMENU"; break; */
/*       case  VK_RMENU   : keyname =  "VK_RMENU"; break;*/
         default: keyname = "<unknown>"; break;
      }
   return keyname;
}

char *resolve_keymodifiers( int mask )
{
   if ( (mask & KC_CTRL) && (mask & KC_SHIFT) && (mask & KC_ALT) )
      return "KC_CTRL KC_SHIFT KC_ALT";
   else if ( (mask & KC_CTRL) && (mask & KC_SHIFT) )
      return "KC_CTRL KC_SHIFT";
   else if ( (mask & KC_CTRL) && (mask & KC_ALT) )
      return "KC_CTRL KC_ALT";
   else if ( (mask & KC_SHIFT) && (mask & KC_ALT) )
      return "KC_SHIFT KC_ALT";
   else if ( (mask & KC_SHIFT) )
      return "KC_SHIFT";
   else if ( (mask & KC_CTRL) )
      return "KC_CTRL";
   else if ( (mask & KC_ALT) )
      return "KC_ALT";
   else return "none";
}

/* This gets called when a part of the graph needs to be repainted. */
int DWSIGNAL text_expose(HWND hwnd, DWExpose *exp, void *data)
{
   HPIXMAP hpm;
   unsigned long width,height;

   if ( hwnd == textbox1 )
      hpm = text1pm;
   else if ( hwnd == textbox2 )
      hpm = text2pm;
   else
      return TRUE;

   width = DW_PIXMAP_WIDTH(hpm);
   height = DW_PIXMAP_HEIGHT(hpm);

   dw_pixmap_bitblt(hwnd, NULL, 0, 0, (int)width, (int)height, 0, hpm, 0, 0 );
   dw_flush();
   return TRUE;
}

void read_file( void )
{
   int i,len;
   fp = fopen( current_file, "r" );
   lp = (char **)calloc( 1000,sizeof(char *));
   /* should test for out of memory */
   max_linewidth=0;
   for ( i = 0; i < 1000; i++ )
   {
      lp[i] = (char *)calloc(1, 1025);
      if ( fgets( lp[i], 1024, fp ) == NULL )
         break;
      len = strlen( lp[i] );
      if ( len > max_linewidth )
         max_linewidth = len;
      if ( lp[i][len - 1] == '\n' )
         lp[i][len - 1] = '\0';
   }
   num_lines = i;
   fclose( fp );
   dw_scrollbar_set_range(hscrollbar, max_linewidth, cols);
   dw_scrollbar_set_pos(hscrollbar, 0);
   dw_scrollbar_set_range(vscrollbar, num_lines, rows);
   dw_scrollbar_set_pos(vscrollbar, 0);
}

void draw_file( int row, int col )
{
   char buf[10];
   int i,y,fileline;
   char *pLine;

   if ( current_file )
   {
      dw_color_foreground_set(DW_CLR_WHITE);
      dw_draw_rect(0, text1pm, TRUE, 0, 0, (int)DW_PIXMAP_WIDTH(text1pm), (int)DW_PIXMAP_HEIGHT(text1pm));
      dw_draw_rect(0, text2pm, TRUE, 0, 0, (int)DW_PIXMAP_WIDTH(text2pm), (int)DW_PIXMAP_HEIGHT(text2pm));

      for ( i = 0;(i < rows) && (i+row < num_lines); i++)
      {
         fileline = i + row - 1;
         y = i*font_height;
         dw_color_background_set( 1 + (fileline % 15) );
         dw_color_foreground_set( fileline % 16 );
         sprintf( buf, "%6.6d", i+row );
         dw_draw_text( 0, text1pm, 0, y, buf);
         pLine = lp[i+row];
         dw_draw_text( 0, text2pm, 0, y, pLine+col );
      }
      text_expose( textbox1, NULL, NULL);
      text_expose( textbox2, NULL, NULL);
   }
}


int DWSIGNAL colorchoose_callback(HWND window, void *data)
{
   current_color = dw_color_choose(current_color);
   return FALSE;
}

int DWSIGNAL cursortoggle_callback(HWND window, void *data)
{
   if(cursor_arrow)
   {
      dw_window_set_text((HWND)cursortogglebutton,"Set Cursor pointer - ARROW");
      dw_window_set_pointer((HWND)data,DW_POINTER_CLOCK);
      cursor_arrow = 0;
   }
   else
   {
      dw_window_set_text((HWND)cursortogglebutton,"Set Cursor pointer - CLOCK");
      dw_window_set_pointer((HWND)data,DW_POINTER_DEFAULT);
      cursor_arrow = 1;
   }
   return FALSE;
}

int DWSIGNAL beep_callback(HWND window, void *data)
{
   dw_timer_disconnect( timerid );
   return TRUE;
}

int DWSIGNAL keypress_callback(HWND window, char ch, int vk, int state, void *data)
{
   char tmpbuf[100];
   if ( ch )
      sprintf( tmpbuf, "Key: %c(%d) Modifiers: %s(%d)", ch, ch, resolve_keymodifiers(state), state );
   else
      sprintf( tmpbuf, "Key: %s(%d) Modifiers: %s(%d)", resolve_keyname(vk), vk, resolve_keymodifiers(state), state );
   dw_window_set_text( status1, tmpbuf);
   return 0;
}

int DWSIGNAL menu_callback(HWND window, void *data)
{
   char buf[100];

   sprintf( buf, "%s menu item selected", (char *)data );
   dw_messagebox( "Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, buf );
   return 0;
}

int DWSIGNAL menutoggle_callback(HWND window, void *data)
{
   if ( menu_enabled )
   {
      dw_menu_item_set_state( changeable_menu, CHECKABLE_MENUITEMID, DW_MIS_DISABLED );
      dw_menu_item_set_state( changeable_menu, NONCHECKABLE_MENUITEMID, DW_MIS_DISABLED );
      menu_enabled = 0;
   }
   else
   {
      dw_menu_item_set_state( changeable_menu, CHECKABLE_MENUITEMID, DW_MIS_ENABLED );
      dw_menu_item_set_state( changeable_menu, NONCHECKABLE_MENUITEMID, DW_MIS_ENABLED );
      menu_enabled = 1;
   }
   return 0;
}

int DWSIGNAL helpabout_callback(HWND window, void *data)
{
   char buf[100];
   DWEnv env;

   dw_environment_query(&env);
   sprintf( buf, "dwindows test\n\nOS: %s %s %s Version: %d.%d.%d.%d\n\ndwindows Version: %d.%d.%d",
            env.osName, env.buildDate, env.buildTime,
            env.MajorVersion, env.MinorVersion, env.MajorBuild, env.MinorBuild,
            env.DWMajorVersion, env.DWMinorVersion, env.DWSubVersion );
   dw_messagebox( "About dwindows", DW_MB_OK | DW_MB_INFORMATION, buf );
   return 0;
}

int DWSIGNAL exit_callback(HWND window, void *data)
{
   dw_window_destroy((HWND)data);
   exit(0);
   return -1;
}

int DWSIGNAL test_callback(HWND window, void *data)
{
   dw_window_destroy((HWND)data);
   if ( current_file )
      dw_free( current_file );
   exit(0);
   return -1;
}

int DWSIGNAL browse_callback(HWND window, void *data)
{
   char *tmp;
   tmp = dw_file_browse("test string", ".", "c", DW_FILE_OPEN );
   if ( tmp )
   {
      if ( current_file )
      {
         dw_free( current_file );
      }
      current_file = tmp;
      dw_window_set_text( entryfield, current_file );
      read_file();
      draw_file(0,0);
   }
   return 0;
}

int DWSIGNAL button_callback(HWND window, void *data)
{
   unsigned int y,m,d;
   unsigned int idx;
   int len;
   long spvalue;
   char buf1[100];
   char buf2[100];
   char buf3[500];

   idx = dw_listbox_selected( combobox1 );
   dw_listbox_get_text( combobox1, idx, buf1, 99 );
   idx = dw_listbox_selected( combobox2 );
   dw_listbox_get_text( combobox2, idx, buf2, 99 );
   dw_calendar_get_date( cal, &y, &m, &d );
   spvalue = dw_spinbutton_get_pos( spinbutton );
   len = sprintf( buf3, "spinbutton: %ld\ncombobox1: \"%s\"\ncombobox2: \"%s\"\ncalendar: %d-%d-%d",
                  spvalue,
                  buf1, buf2,
                  y, m, d );
   dw_messagebox( "Values", DW_MB_OK | DW_MB_INFORMATION, buf3 );
   dw_clipboard_set_text( buf3, len );
   return 0;
}

int DWSIGNAL redraw_button_box_callback(HWND window, void *data)
{
#if 0

   long x, y, width, height;
   dw_window_get_pos_size(filetoolbarbox , &x, &y, &width, &height);
   dw_window_destroy( filetoolbarbox );
   create_button(1);
   dw_window_set_pos_size(filetoolbarbox, x, y, width, height);
#else
   dw_window_enable( window);
   dw_window_destroy( noncheckable_menuitem );
#endif
   return 0;
}

int DWSIGNAL change_color_red_callback(HWND window, void *data)
{
   dw_window_set_color(buttonsbox, DW_CLR_RED, DW_CLR_RED);
   return 0;
}

int DWSIGNAL change_color_yellow_callback(HWND window, void *data)
{
   dw_window_set_color(buttonsbox, DW_CLR_YELLOW, DW_CLR_YELLOW);
   return 0;
}


/* Callback to handle user selection of the scrollbar position */
void DWSIGNAL scrollbar_valuechanged_callback(HWND hwnd, int value, void *data)
{
   if(data)
   {
      HWND stext = (HWND)data;
      char tmpbuf[100];
      if ( hwnd == vscrollbar )
      {
         current_row = value;
      }
      else
      {
         current_col = value;
      }
      sprintf(tmpbuf, "Row:%d Col:%d Lines:%d Cols:%d", current_row,current_col,num_lines,max_linewidth);
      dw_window_set_text(stext, tmpbuf);
      draw_file( current_row, current_col);
   }
}

/* Callback to handle user selection of the spinbutton position */
void DWSIGNAL spinbutton_valuechanged_callback(HWND hwnd, int value, void *data)
{
   dw_messagebox("DWTest", DW_MB_OK, "New value from spinbutton: %d\n", value);
}

/* Callback to handle user selection of the slider position */
void DWSIGNAL slider_valuechanged_callback(HWND hwnd, int value, void *data)
{
   dw_messagebox("DWTest", DW_MB_OK, "New value from slider: %d\n", value);
}

/* Handle size change of the main render window */
int DWSIGNAL configure_event(HWND hwnd, int width, int height, void *data)
{
   HPIXMAP old1 = text1pm, old2 = text2pm;
   unsigned long depth = dw_color_depth_get();

   rows = height / font_height;
   cols = width / font_width;

   /* Create new pixmaps with the current sizes */
   text1pm = dw_pixmap_new(textbox1, (unsigned long)(font_width*(width1)), (unsigned long)height, (int)depth);
   text2pm = dw_pixmap_new(textbox2, (unsigned long)width, (unsigned long)height, (int)depth);

   /* Destroy the old pixmaps */
   dw_pixmap_destroy(old1);
   dw_pixmap_destroy(old2);

   /* Update scrollbar ranges with new values */
   dw_scrollbar_set_range(hscrollbar, max_linewidth, cols);
   dw_scrollbar_set_range(vscrollbar, num_lines, rows);

   /* Redraw the window */
   draw_file( current_row, current_col);
   return TRUE;
}

int DWSIGNAL item_enter_cb( HWND window, char *text, void *data )
{
   char buf[200];
   HWND statline = (HWND)data;

   sprintf(buf,"DW_SIGNAL_ITEM_ENTER: Window: %x Text: %s", (unsigned int)window, text );
   dw_window_set_text( statline, buf);
   return 0;
}

int DWSIGNAL item_context_cb( HWND window, char *text, int x, int y, void *data, void *itemdata )
{
   char buf[200];
   HWND statline = (HWND)data;

   sprintf(buf,"DW_SIGNAL_ITEM_CONTEXT: Window: %x Text: %s x: %d y: %d Itemdata: %x", (unsigned int)window, text, x, y, (unsigned int)itemdata );
   dw_window_set_text( statline, buf);
   return 0;
}

int DWSIGNAL list_select_cb( HWND window, int item, void *data )
{
   char buf[200];
   HWND statline = (HWND)data;

   sprintf(buf,"DW_SIGNAL_LIST_SELECT: Window: %d Item: %d", (unsigned int)window, item );
   dw_window_set_text( statline, buf);
   return 0;
}

int DWSIGNAL item_select_cb( HWND window, HTREEITEM item, char *text, void *data, void *itemdata )
{
   char buf[200];
   HWND statline = (HWND)data;

   sprintf(buf,"DW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x", (unsigned int)window, (unsigned int)item, text, (unsigned int)itemdata );
   dw_window_set_text( statline, buf);
   return 0;
}

int DWSIGNAL container_select_cb( HWND window, HTREEITEM item, char *text, void *data, void *itemdata )
{
   char buf[200];
   char *str;
   HWND statline = (HWND)data;
   unsigned long size;

   sprintf(buf,"DW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x", (unsigned int)window, (unsigned int)item, text, (unsigned int)itemdata );
   dw_window_set_text( statline, buf);
   sprintf(buf,"\r\nDW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x\r\n", (unsigned int)window, (unsigned int)item, text, (unsigned int)itemdata );
   mle_point = dw_mle_import( container_mle, buf, mle_point);
   str = dw_container_query_start(container, DW_CRA_SELECTED);
   while(str)
   {
      sprintf(buf,"Selected: %s\r\n", str);
      mle_point = dw_mle_import( container_mle, buf, mle_point);
      str = dw_container_query_next(container, DW_CRA_SELECTED);
   }
   /* Make the last inserted point the cursor location */
   dw_mle_set_cursor(container_mle, mle_point);
/* set the details of item 0 to new data */
fprintf(stderr,"In cb: container: %x containerinfo: %x icon: %x\n", (int)container, (int)containerinfo, (int)fileicon);
   dw_filesystem_change_file(container, 0, "new data", fileicon);
   size = 999;
fprintf(stderr,"In cb: container: %x containerinfo: %x icon: %x\n", (int)container, (int)containerinfo, (int)fileicon);
   dw_filesystem_change_item(container, 0, 0, &size);
   return 0;
}

int DWSIGNAL switch_page_cb( HWND window, unsigned long page_num, void *itemdata )
{
   FILE *fp=fopen("log","a");
   fprintf(fp,"DW_SIGNAL_SWITCH_PAGE: Window: %x PageNum: %u Itemdata: %x\n", (unsigned int)window, (unsigned int)page_num, (unsigned int)itemdata );
   fclose(fp);
   return 0;
}

int DWSIGNAL column_click_cb( HWND window, int column_num, void *data )
{
   char buf[200], buf1[100];
   HWND statline = (HWND)data;
   int column_type;

   if(column_num == 0)
      strcpy(buf1,"Filename");
   else
   {
   column_type = dw_filesystem_get_column_type( window, column_num-1 );
   if( column_type == DW_CFA_STRING)
      strcpy(buf1,"String");
   else if( column_type == DW_CFA_ULONG)
      strcpy(buf1,"ULong");
   else if( column_type == DW_CFA_DATE)
      strcpy(buf1,"Date");
   else if( column_type == DW_CFA_TIME)
      strcpy(buf1,"Time");
   else if( column_type == DW_CFA_BITMAPORICON)
      strcpy(buf1,"BitmapOrIcon");
   else
      strcpy(buf1,"Unknown");
   }
   sprintf(buf,"DW_SIGNAL_COLUMN_CLICK: Window: %x Column: %d Type: %s Itemdata: %x", (unsigned int)window, column_num, buf1, (unsigned int)data );
   dw_window_set_text( statline, buf);
   return 0;
}

int DWSIGNAL combobox_select_event_callback(HWND window, int index)
{
   fprintf(stderr,"got combobox_select_event for index: %d, iteration: %d\n", index, iteration++);
   return FALSE;
}

void archive_add(void)
{
   HWND browsebutton, browsebox;

   lbbox = dw_box_new(BOXVERT, 10);

   dw_box_pack_start(notebookbox1, lbbox, 150, 70, TRUE, TRUE, 0);

   /* Archive Name */
   stext = dw_text_new("File to browse", 0);

   dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

   dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 2);

   browsebox = dw_box_new(BOXHORZ, 0);

   dw_box_pack_start(lbbox, browsebox, 0, 0, TRUE, TRUE, 0);

   entryfield = dw_entryfield_new("", 100L);

   dw_entryfield_set_limit(entryfield, 260);

   dw_box_pack_start(browsebox, entryfield, 100, 15, TRUE, TRUE, 4);

   browsebutton = dw_button_new("Browse", 1001L);

   dw_box_pack_start(browsebox, browsebutton, 30, 15, TRUE, TRUE, 0);

   dw_window_set_color(browsebox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
   dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);

   /* Buttons */
   buttonbox = dw_box_new(BOXHORZ, 10);

   dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, TRUE, 0);

   cursortogglebutton = dw_button_new("Set Cursor pointer - CLOCK", 1003L);
   dw_box_pack_start(buttonbox, cursortogglebutton, 130, 30, TRUE, TRUE, 2);

   colorchoosebutton = dw_button_new("Color Chooser Dialog", 1004L);
   dw_box_pack_start(buttonbox, colorchoosebutton, 130, 30, TRUE, TRUE, 2);

   okbutton = dw_button_new("Turn Off Annoying Beep!", 1001L);
   dw_box_pack_start(buttonbox, okbutton, 130, 30, TRUE, TRUE, 2);

   cancelbutton = dw_button_new("Exit", 1002L);
   dw_box_pack_start(buttonbox, cancelbutton, 130, 30, TRUE, TRUE, 2);
   dw_window_click_default( mainwindow, cancelbutton );

   /* Set some nice fonts and colors */
   dw_window_set_color(lbbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
   dw_window_set_color(buttonbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
   dw_window_set_color(okbutton, DW_CLR_PALEGRAY, DW_CLR_DARKCYAN);

   dw_signal_connect(browsebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(browse_callback), (void *)notebookbox1);
   dw_signal_connect(okbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(beep_callback), (void *)notebookbox1);
   dw_signal_connect(cancelbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(exit_callback), (void *)mainwindow);
   dw_signal_connect(cursortogglebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(cursortoggle_callback), (void *)mainwindow);
   dw_signal_connect(colorchoosebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(colorchoose_callback), (void *)mainwindow);
}


void text_add(void)
{
   unsigned long depth = dw_color_depth_get();
   HWND vscrollbox;

   /* create a box to pack into the notebook page */
   pagebox = dw_box_new(BOXHORZ, 2);
   dw_box_pack_start( notebookbox2, pagebox, 0, 0, TRUE, TRUE, 0);
   /* now a status area under this box */
   status = dw_status_text_new("", 0);
   dw_box_pack_start( notebookbox2, status, 100, 20, TRUE, FALSE, 1);
   /* and another one */
   status1 = dw_status_text_new("", 0);
   dw_box_pack_start( notebookbox2, status1, 100, 20, TRUE, FALSE, 1);

   /* create render box for number pixmap */
   textbox1 = dw_render_new( 100 );
   dw_window_set_font(textbox1, FIXEDFONT);
   dw_font_text_extents_get(textbox1, NULL, "(g", &font_width, &font_height);
   font_width = font_width / 2;
   vscrollbox = dw_box_new(BOXVERT, 0);
   dw_box_pack_start(vscrollbox, textbox1, font_width*width1, font_height*rows, FALSE, TRUE, 0);
   dw_box_pack_start(vscrollbox, 0, (font_width*(width1+1)), SCROLLBARWIDTH, FALSE, FALSE, 0);
   dw_box_pack_start(pagebox, vscrollbox, 0, 0, FALSE, TRUE, 0);

   /* pack empty space 1 character wide */
   dw_box_pack_start(pagebox, 0, font_width, 0, FALSE, TRUE, 0);

   /* create box for filecontents and horz scrollbar */
   textboxA = dw_box_new( BOXVERT,0 );
   dw_box_pack_start( pagebox, textboxA, 0, 0, TRUE, TRUE, 0);

   /* create render box for filecontents pixmap */
   textbox2 = dw_render_new( 101 );
   dw_box_pack_start( textboxA, textbox2, 10, 10, TRUE, TRUE, 0);
   dw_window_set_font(textbox2, FIXEDFONT);
   /* create horizonal scrollbar */
   hscrollbar = dw_scrollbar_new(FALSE, 50);
   dw_box_pack_start( textboxA, hscrollbar, 100, SCROLLBARWIDTH, TRUE, FALSE, 0);

   /* create vertical scrollbar */
   vscrollbox = dw_box_new(BOXVERT, 0);
   vscrollbar = dw_scrollbar_new(TRUE, 50);
   dw_box_pack_start(vscrollbox, vscrollbar, SCROLLBARWIDTH, 100, FALSE, TRUE, 0);
   /* Pack an area of empty space 14x14 pixels */
   dw_box_pack_start(vscrollbox, 0, SCROLLBARWIDTH, SCROLLBARWIDTH, FALSE, FALSE, 0);
   dw_box_pack_start(pagebox, vscrollbox, 0, 0, FALSE, TRUE, 0);

   text1pm = dw_pixmap_new( textbox1, font_width*width1, font_height*rows, depth );
   text2pm = dw_pixmap_new( textbox2, font_width*cols, font_height*rows, depth );

   dw_messagebox("DWTest", DW_MB_OK|DW_MB_INFORMATION, "Width: %d Height: %d\n", font_width, font_height);
   dw_draw_rect(0, text1pm, TRUE, 0, 0, font_width*width1, font_height*rows);
   dw_draw_rect(0, text2pm, TRUE, 0, 0, font_width*cols, font_height*rows);
   dw_signal_connect(textbox1, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(text_expose), NULL);
   dw_signal_connect(textbox2, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(text_expose), NULL);
   dw_signal_connect(textbox2, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(configure_event), text2pm);
   dw_signal_connect(hscrollbar, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(scrollbar_valuechanged_callback), (void *)status);
   dw_signal_connect(vscrollbar, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(scrollbar_valuechanged_callback), (void *)status);

   dw_signal_connect(mainwindow, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(keypress_callback), NULL);
}

void tree_add(void)
{
   HTREEITEM t1,t2,t3,t4,t5,t6;

   /* create a box to pack into the notebook page */
   treebox = dw_box_new(BOXHORZ, 2);
   dw_box_pack_start( notebookbox3, treebox, 500, 200, TRUE, TRUE, 0);

   /* now a tree area under this box */
   tree = dw_tree_new(101);
   dw_box_pack_start( notebookbox3, tree, 500, 200, TRUE, FALSE, 1);

   /* and a status area to see whats going on */
   tree_status = dw_status_text_new("", 0);
   dw_box_pack_start( notebookbox3, tree_status, 100, 20, TRUE, FALSE, 1);

   foldericon = dw_icon_load_from_file( FOLDER_ICON_NAME );
   fileicon = dw_icon_load_from_file( FILE_ICON_NAME  );

   /* set up our signal trappers... */
   dw_signal_connect(tree, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(item_context_cb), (void *)tree_status);
   dw_signal_connect(tree, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(item_select_cb), (void *)tree_status);

   t1 = dw_tree_insert(tree, "tree folder 1", foldericon, NULL, (void *)1 );
   t2 = dw_tree_insert(tree, "tree folder 2", foldericon, NULL, (void *)2 );
   t3 = dw_tree_insert(tree, "tree file 1", fileicon, t1, (void *)3 );
   t4 = dw_tree_insert(tree, "tree file 2", fileicon, t1, (void *)4 );
   t5 = dw_tree_insert(tree, "tree file 3", fileicon, t2, (void *)5 );
   t6 = dw_tree_insert(tree, "tree file 4", fileicon, t2, (void *)6 );
   /* set the folder name and icon again to show error with dw_tree_item_change under GTK 2.0 */
   dw_tree_item_change(tree, t1, "tree folder 1", foldericon );
   dw_tree_item_change(tree, t2, "tree folder 2", foldericon );
}

void container_add(void)
{
   char *titles[3];
   char *names[3];
   char buffer[100];
   unsigned long flags[3] = {  DW_CFA_ULONG | DW_CFA_RIGHT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
   DW_CFA_TIME | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
   DW_CFA_DATE | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR };
   int z;
   CTIME time;
   CDATE date;
   unsigned long size;
   HICN thisicon;

   /* create a box to pack into the notebook page */
   containerbox = dw_box_new(BOXHORZ, 2);
   dw_box_pack_start( notebookbox4, containerbox, 500, 200, TRUE, TRUE, 0);

   /* now a container area under this box */
   container = dw_container_new(100, TRUE);
   dw_box_pack_start( notebookbox4, container, 500, 200, TRUE, FALSE, 1);

   /* and a status area to see whats going on */
   container_status = dw_status_text_new("", 0);
   dw_box_pack_start( notebookbox4, container_status, 100, 20, TRUE, FALSE, 1);

   titles[0] = "Size";
   titles[1] = "Time";
   titles[2] = "Date";

   dw_filesystem_setup(container, flags, titles, 3);
   containerinfo = dw_container_alloc(container, 3);

   for(z=0;z<3;z++)
   {
      names[z] = (char *)malloc( 100 );
      /* yes, there is a memory leak here */
      sprintf(names[z],"Don't allocate from stack: Item: %d",z);
      size = z*100;
      sprintf(buffer, "Filename %d",z+1);
      if (z == 0 ) thisicon = foldericon;
      else thisicon = fileicon;
fprintf(stderr,"Initial: container: %x containerinfo: %x icon: %x\n", (int)container, (int)containerinfo, (int)thisicon);
      dw_filesystem_set_file(container, containerinfo, z, buffer, thisicon);
      dw_filesystem_set_item(container, containerinfo, 0, z, &size);

      time.seconds = z+10;
      time.minutes = z+10;
      time.hours = z+10;
      dw_filesystem_set_item(container, containerinfo, 1, z, &time);

      date.day = z+10;
      date.month = z+10;
      date.year = z+2000;
      dw_filesystem_set_item(container, containerinfo, 2, z, &date);

      dw_container_set_row_title(containerinfo, z, names[z]);
   }

   dw_container_insert(container, containerinfo, 3);
   dw_container_optimize(container);

   container_mle = dw_mle_new( 111 );
   dw_box_pack_start( containerbox, container_mle, 500, 200, TRUE, TRUE, 0);

   /* connect our event trappers... */
   dw_signal_connect(container, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(item_enter_cb), (void *)container_status);
   dw_signal_connect(container, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(item_context_cb), (void *)container_status);
   dw_signal_connect(container, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(container_select_cb), (void *)container_status);
   dw_signal_connect(container, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(column_click_cb), (void *)container_status);
}

/* Beep every second */
int DWSIGNAL timer_callback(void *data)
{
   dw_beep(200, 200);

   /* Return TRUE so we get called again */
   return TRUE;
}


void buttons_add(void)
{
   HWND abutton1,abutton2,calbox;
   int i;
   char **text;

   /* create a box to pack into the notebook page */
   buttonsbox = dw_box_new(BOXVERT, 2);
   dw_box_pack_start( notebookbox5, buttonsbox, 25, 200, TRUE, TRUE, 0);
   dw_window_set_color(buttonsbox, DW_CLR_RED, DW_CLR_RED);

   calbox = dw_box_new(DW_HORZ, 0);
   dw_box_pack_start(notebookbox5, calbox, 500, 200, TRUE, TRUE, 1);
   cal = dw_calendar_new(100);
   dw_box_pack_start(calbox, cal, 180, 160, TRUE, TRUE, 0);
/*
   dw_calendar_set_date(cal, 2001, 1, 1);
*/
/*
 * Create our file toolbar boxes...
 */
   buttonboxperm = dw_box_new( BOXVERT, 0 );
   dw_box_pack_start( buttonsbox, buttonboxperm, 25, 0, FALSE, TRUE, 2 );
   dw_window_set_color(buttonboxperm, DW_CLR_WHITE, DW_CLR_WHITE);
//   abutton1 = dw_bitmapbutton_new_from_file( "Top Button", 0, FILE_ICON_NAME );
   abutton1 = dw_bitmapbutton_new_from_file( "Top Button", 0, "z:\\projects\\RexxGd\\regina\\tile_up.png" );
   dw_box_pack_start( buttonboxperm, abutton1, 100, 30, FALSE, FALSE, 0 );
   dw_signal_connect( abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(button_callback), NULL );
   dw_box_pack_start( buttonboxperm, 0, 25, 5, FALSE, FALSE, 0 );
   abutton2 = dw_bitmapbutton_new_from_file( "Bottom", 0, FOLDER_ICON_NAME );
   dw_box_pack_start( buttonsbox, abutton2, 25, 25, FALSE, FALSE, 0 );
   dw_signal_connect( abutton2, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(button_callback), NULL );

   create_button(0);
/* make a combobox */
   combox = dw_box_new(BOXVERT, 2);
   dw_box_pack_start( notebookbox5, combox, 25, 200, TRUE, TRUE, 0);
   combobox1 = dw_combobox_new( "fred", 0 ); /* no point in specifying an initial value */
   dw_listbox_append( combobox1, "fred" );
   dw_box_pack_start( combox, combobox1, 200, 20, TRUE, FALSE, 0);
/*
   dw_window_set_text( combobox, "initial value");
*/
   dw_signal_connect( combobox1, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(combobox_select_event_callback), NULL );
#if 0
   /* add LOTS of items */
   fprintf(stderr,"before appending 100 items to combobox using dw_listbox_append()\n");
   for( i = 0; i < 100; i++ )
   {
      sprintf( buf, "item %d", i);
      dw_listbox_append( combobox1, buf );
   }
   fprintf(stderr,"after appending 100 items to combobox\n");
#endif

   combobox2 = dw_combobox_new( "joe", 0 ); /* no point in specifying an initial value */
   dw_box_pack_start( combox, combobox2, 200, 20, TRUE, FALSE, 0);
/*
   dw_window_set_text( combobox, "initial value");
*/
   dw_signal_connect( combobox2, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(combobox_select_event_callback), NULL );
   /* add LOTS of items */
   fprintf(stderr,"before appending 500 items to combobox using dw_listbox_list_append()\n");
   text = (char **)malloc(500*sizeof(char *));
   for( i = 0; i < 500; i++ )
   {
      text[i] = (char *)malloc( 50 );
      sprintf( text[i], "item %d", i);
   }
   dw_listbox_list_append( combobox2, text, 500 );
   fprintf(stderr,"after appending 500 items to combobox\n");
   for( i = 0; i < 500; i++ )
   {
      free(text[i]);
   }
   free(text);
   /* now insert a couple of items */
   dw_listbox_insert( combobox2, "inserted item 2", 2 );
   dw_listbox_insert( combobox2, "inserted item 5", 5 );
   /* make a spinbutton */
   spinbutton = dw_spinbutton_new( "", 0 ); /* no point in specifying text */
   dw_box_pack_start( combox, spinbutton, 200, 20, TRUE, FALSE, 0);
   dw_spinbutton_set_limits( spinbutton, 100, 1 );
   dw_spinbutton_set_pos( spinbutton, 30 );
   dw_signal_connect( spinbutton, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(spinbutton_valuechanged_callback), NULL );
   /* make a slider */
   slider = dw_slider_new( FALSE, 10, 0 ); /* no point in specifying text */
   dw_box_pack_start( combox, slider, 200, 20, TRUE, FALSE, 0);
   dw_signal_connect( slider, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(slider_valuechanged_callback), NULL );
}

void create_button( int redraw)
{
   HWND abutton1;
   filetoolbarbox = dw_box_new( BOXVERT, 0 );
   dw_box_pack_start( buttonboxperm, filetoolbarbox, 0, 0, TRUE, TRUE, 0 );

   abutton1 = dw_bitmapbutton_new_from_file( "Should be under Top button", 0, "junk" );
   dw_box_pack_start( filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
   dw_signal_connect( abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(change_color_red_callback), NULL );
   dw_box_pack_start( filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0 );

   abutton1 = dw_bitmapbutton_new_from_file( "Should be under Top button", 0, "junk" );
   dw_box_pack_start( filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
   dw_signal_connect( abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(change_color_yellow_callback), NULL );
   dw_box_pack_start( filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0 );

   abutton1 = dw_bitmapbutton_new_from_file( "Should be under Top button", 0, "junk" );
   dw_box_pack_start( filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
   dw_signal_connect( abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(redraw_button_box_callback), NULL );
   dw_box_pack_start( filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0 );
   if ( redraw )
   {
      dw_window_redraw( filetoolbarbox );
      dw_window_redraw( mainwindow );
   }
}

void mdi_add(void)
{
   HWND mdibox, mdi, mdi1w, mdi1box, ef, mdi2w, mdi2box, bb;

   /* create a box to pack into the notebook page */
   mdibox = dw_box_new(DW_HORZ, 0);

   dw_box_pack_start(notebookbox6, mdibox, 500, 200, TRUE, TRUE, 1);

   /* now a mdi under this box */
   mdi = dw_mdi_new(333);
   dw_box_pack_start(mdibox, mdi, 500, 200, TRUE, TRUE, 2);

   mdi1w = dw_window_new(mdi, "MDI1", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
   mdi1box = dw_box_new(DW_HORZ, 0);
   dw_box_pack_start(mdi1w, mdi1box, 0, 0, TRUE, TRUE, 0);
   ef = dw_entryfield_new("", 0);
   dw_box_pack_start(mdi1box, ef, 100, 20, FALSE, FALSE, 4);
   dw_window_set_size(mdi1w, 200, 100);
   dw_window_show(mdi1w);

   mdi2w = dw_window_new(mdi, "MDI2", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
   mdi2box = dw_box_new(DW_HORZ, 0);
   dw_box_pack_start(mdi2w, mdi2box, 0, 0, TRUE, TRUE, 0);
   ef = dw_entryfield_new( "", 0);
   dw_box_pack_start(mdi2box, ef, 150, 30, FALSE, FALSE, 4);
   bb = dw_button_new("Browse", 0);
   dw_box_pack_start(mdi2box, bb, 60, 30, FALSE, FALSE, 4);
   dw_window_set_size(mdi2w, 200, 200);
   dw_window_show(mdi2w);
}

void menu_add(void)
{
   HMENUI menuitem,menu;

   mainmenubar = dw_menubar_new( mainwindow );
   /* add menus to the menubar */
   menu = dw_menu_new( 0 );
   menuitem = dw_menu_append_item( menu, "~Quit", 1019, 0, TRUE, FALSE, 0 );
   dw_signal_connect( menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(exit_callback), (void *)mainwindow);
   /*
    * Add the "File" menu to the menubar...
    */
   dw_menu_append_item( mainmenubar, "~File", 1010, 0, TRUE, FALSE, menu );

   changeable_menu = dw_menu_new( 0 );
   checkable_menuitem = dw_menu_append_item( changeable_menu, "~Checkable Menu Item", CHECKABLE_MENUITEMID, 0, TRUE, TRUE, 0 );
   dw_signal_connect( checkable_menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_callback), (void *)"checkable");
   noncheckable_menuitem = dw_menu_append_item( changeable_menu, "~Non-checkable Menu Item", NONCHECKABLE_MENUITEMID, 0, TRUE, FALSE, 0 );
   dw_signal_connect( noncheckable_menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_callback), (void *)"non-checkable");
   menuitem = dw_menu_append_item( changeable_menu, "~Disabled menu Item", 2003, DW_MIS_DISABLED|DW_MIS_CHECKED, TRUE, TRUE, 0 );
   /* seperator */
   dw_menu_append_item( changeable_menu, DW_MENU_SEPARATOR, 3999, 0, TRUE, FALSE, 0 );
   menuitem = dw_menu_append_item( changeable_menu, "~Menu Items Disabled", 2009, 0, TRUE, TRUE, 0 );
   dw_signal_connect( menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menutoggle_callback), (void *)NULL);
   /*
    * Add the "Menu" menu to the menubar...
    */
   dw_menu_append_item( mainmenubar, "~Menu", 1020, 0, TRUE, FALSE, changeable_menu );

   menu = dw_menu_new( 0 );
   menuitem = dw_menu_append_item( menu, "~About", 1091, 0, TRUE, FALSE, 0 );
   dw_signal_connect( menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(helpabout_callback), (void *)mainwindow);
   /*
    * Add the "Help" menu to the menubar...
    */
   dw_menu_append_item( mainmenubar, "~Help", 1090, 0, TRUE, FALSE, menu );
}

/*
 * Let's demonstrate the functionality of this library. :)
 */
int main(int argc, char *argv[])
{
   ULONG notebookpage1;
   ULONG notebookpage2;
   ULONG notebookpage3;
   ULONG notebookpage4;
   ULONG notebookpage5;
   ULONG notebookpage6;
   ULONG notebookpage7;

   dw_init(TRUE, argc, argv);

   mainwindow = dw_window_new( HWND_DESKTOP, "dwindows test", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
   dw_window_set_icon(mainwindow, fileicon);

   notebookbox = dw_box_new( BOXVERT, 5 );
   dw_box_pack_start( mainwindow, notebookbox, 0, 0, TRUE, TRUE, 0);
   /* MUST pack a box into the mainwindow BEFORE adding menus */
   menu_add();

   notebook = dw_notebook_new( 1, TRUE );
   dw_box_pack_start( notebookbox, notebook, 100, 100, TRUE, TRUE, 0);
   dw_signal_connect(notebook, DW_SIGNAL_SWITCH_PAGE, DW_SIGNAL_FUNC(switch_page_cb), NULL);

   notebookbox1 = dw_box_new( BOXVERT, 5 );
   notebookpage1 = dw_notebook_page_new( notebook, 0, TRUE );
   dw_notebook_pack( notebook, notebookpage1, notebookbox1 );
   dw_notebook_page_set_text( notebook, notebookpage1, "buttons and entry");
   archive_add();

   notebookbox2 = dw_box_new( BOXVERT, 5 );
   notebookpage2 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage2, notebookbox2 );
   dw_notebook_page_set_text( notebook, notebookpage2, "render");
   text_add();

   notebookbox3 = dw_box_new( BOXVERT, 5 );
   notebookpage3 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage3, notebookbox3 );
   dw_notebook_page_set_text( notebook, notebookpage3, "tree");
   tree_add();

   notebookbox4 = dw_box_new( BOXVERT, 5 );
   notebookpage4 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage4, notebookbox4 );
   dw_notebook_page_set_text( notebook, notebookpage4, "container");
   container_add();

   notebookbox5 = dw_box_new( BOXVERT, 5 );
   notebookpage5 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage5, notebookbox5 );
   dw_notebook_page_set_text( notebook, notebookpage5, "buttons");
   buttons_add();

   notebookbox6 = dw_box_new( BOXVERT, 5 );
   notebookpage6 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage6, notebookbox6 );
   dw_notebook_page_set_text( notebook, notebookpage6, "mdi");
   mdi_add();

   notebookbox7 = dw_box_new( BOXVERT, 6 );
   notebookpage7 = dw_notebook_page_new( notebook, 1, FALSE );
   dw_notebook_pack( notebook, notebookpage7, notebookbox7 );
   dw_notebook_page_set_text( notebook, notebookpage7, "html");

#if 1
   rawhtml = dw_html_new(1001);
   dw_box_pack_start( notebookbox7, rawhtml, 0, 100, TRUE, FALSE, 0);
   dw_html_raw(rawhtml, "<html><body><center><h1>dwtest</h1></center></body></html>");
   html = dw_html_new(1002);
   dw_box_pack_start( notebookbox7, html, 0, 100, TRUE, TRUE, 0);
   /*
    * This originally had:
      dw_html_url(html, "http://dwindows.netlabs.org");
    * but as the base file is XML, the Windows browser instance can't display it.
    * Instead a pure HTML based web page is displayed. MHES 5/4/2008
    */
   dw_html_url(html, "http://www.rexx.org");
#endif
   dw_signal_connect(mainwindow, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(exit_callback), (void *)mainwindow);
   timerid = dw_timer_connect(2000, DW_SIGNAL_FUNC(timer_callback), 0);
   dw_window_set_size(mainwindow, 640, 520);
   dw_window_show(mainwindow);

   dw_main();

   return 0;
}
