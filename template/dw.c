/*
 * Dynamic Windows:
 *          A GTK like GUI implementation template.
 *
 * (C) 2011-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2011-2022 Mark Hessling <mark@rexx.org>
 *
 * Compile with $CC -I. -D__TEMPLATE__ template/dw.c 
 *
 */

#include "dw.h"
#include <stdlib.h>
#include <string.h>

/* Implement these to get and set the Box* pointer on the widget handle */
void *_dw_window_pointer_get(HWND handle)
{
    return NULL;
}

void _dw_window_pointer_set(HWND handle, Box *box)
{
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

#if 0
   /* If there are containers which have built-in padding like
    * groupboxes.. calculate the padding size and add it to the layout.
    */
   if(thisbox->grouphwnd)
   {
      char *text = dw_window_get_text(thisbox->grouphwnd);

      thisbox->grouppady = 0;

      if(text)
      {
         dw_font_text_extents_get(thisbox->grouphwnd, 0, text, NULL, &thisbox->grouppady);
         dw_free(text);
      }

      if(thisbox->grouppady)
         thisbox->grouppady += 3;
      else
         thisbox->grouppady = 6;

      thisbox->grouppadx = 6;

      thisbox->minwidth += thisbox->grouppadx;
      thisbox->usedpadx += thisbox->grouppadx;
      thisbox->minheight += thisbox->grouppady;
      thisbox->usedpady += thisbox->grouppady;
   }
#endif

   /* Count up all the space for all items in the box */
   for(z=0;z<thisbox->count;z++)
   {
      int itempad, itemwidth, itemheight;
        
      if(thisbox->items[z].type == _DW_TYPE_BOX)
      {
         Box *tmp = (Box *)_dw_window_pointer_get(thisbox->items[z].hwnd);

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
#if 0
            HWND handle = thisbox->items[z].hwnd;

            /* Here you put your platform specific placement widget placement code */
            PlaceWidget(handle, currentx + pad, currenty + pad, width, height);

            /* If any special handling needs to be done... like diving into
             * controls that have sub-layouts... like notebooks or splitbars...
             * do that here. Figure out the sub-layout size and call _dw_do_resize().
             */
#endif

            /* Advance the current position in the box */
            if(thisbox->type == DW_HORZ)
               currentx += width + (pad * 2);
            if(thisbox->type == DW_VERT)
               currenty += height + (pad * 2);
         }
      }
   }
}

/* This is a convenience function used in the window's resize event
 * to relayout the controls in the window.
 */
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

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
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
 * Returns a pointer to a static buffer which contains the
 * current user directory.  Or the root directory if it could
 * not be determined.
 */
char * API dw_user_dir(void)
{
   static char _dw_user_dir[MAX_PATH+1] = {0};

   if(!_dw_user_dir[0])
   {
      char *home = getenv("HOME");

      if(home)
         strcpy(_dw_user_dir, home);
      else
         strcpy(_dw_user_dir, "/");
   }
   return _dw_user_dir;
}

/*
 * Returns a pointer to a static buffer which containes the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    static char _dw_exec_dir[MAX_PATH+1] = {0};
    /* Code to determine the execution directory here,
     * some implementations make this variable global
     * and determin the location in dw_init().
     */
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
    return DW_ERROR_UNKNOWN;
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
   /* Output to stderr, if there is another way to send it
    * on the implementation platform, change this.
    */
   vfprintf(stderr, format, args);
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: flags to indicate buttons and icon
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 * Returns:
 *       DW_MB_RETURN_YES, DW_MB_RETURN_NO, DW_MB_RETURN_OK,
 *       or DW_MB_RETURN_CANCEL based on flags and user response.
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
    return NULL;
}

/*
 * Gets the contents of the default clipboard as text.
 * Parameters:
 *       None.
 * Returns:
 *       Pointer to an allocated string of text or NULL if clipboard empty or contents could not
 *       be converted to text.
 */
char * API dw_clipboard_get_text()
{
    return NULL;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       str: Text to put on the clipboard.
 *       len: Length of the text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
}


/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 * Returns:
 *       A handle to a dialog or NULL on failure.
 */
DWDialog * API dw_dialog_new(void *data)
{
#if 0
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
#endif
    return NULL;
}

/*
 * Accepts a dialog struct and returns the given data to the
 * initial called of dw_dialog_wait().
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 *           result: Data to be returned by dw_dialog_wait().
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_dialog_dismiss(DWDialog *dialog, void *result)
{
#if 0
    dialog->result = result;
    dw_event_post(dialog->eve);
    dialog->done = TRUE;
#endif
    return DW_ERROR_GENERAL;
}

/*
 * Accepts a dialog struct waits for dw_dialog_dismiss() to be
 * called by a signal handler with the given dialog struct.
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 * Returns:
 *       The data passed to dw_dialog_dismiss().
 */
void * API dw_dialog_wait(DWDialog *dialog)
{
    void *tmp = NULL;

#if 0
    while(!dialog->done)
    {
        dw_main_iteration();
    }
    dw_event_close(&dialog->eve);
    tmp = dialog->result;
    free(dialog);
#endif
    return tmp;
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 * Returns:
 *       A handle to a box or NULL on failure.
 */
HWND API dw_box_new(int type, int pad)
{
    return 0;
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 * Returns:
 *       A handle to a groupbox window or NULL on failure.
 */
HWND API dw_groupbox_new(int type, int pad, const char *title)
{
    return 0;
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 * Returns:
 *       A handle to a scrollbox or NULL on failure.
 */
HWND API dw_scrollbox_new( int type, int pad )
{
    return 0;
}

/*
 * Returns the position of the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 * Returns:
 *       The vertical or horizontal position in the scrollbox.
 */
int API dw_scrollbox_get_pos(HWND handle, int orient)
{
    return 0;
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 * Returns:
 *       The vertical or horizontal range of the scrollbox.
 */
int API dw_scrollbox_get_range(HWND handle, int orient)
{
    return 0;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
    Box *thisbox;
    int z, x = 0;
    Item *tmpitem, *thisitem;

    /* Sanity checks */
    if(!box || box == item)
        return;

    thisbox = _dw_window_pointer_get(box);
    thisitem = thisbox->items;

    /* Do some sanity bounds checking */
    if(index < 0)
       index = 0;
    if(index > thisbox->count)
       index = thisbox->count;
        
    /* Duplicate the existing data */
    tmpitem = malloc(sizeof(Item)*(thisbox->count+1));

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
    if(0 /* Test to see if "item" is a box */)
       tmpitem[index].type = _DW_TYPE_BOX;
    else
       tmpitem[index].type = _DW_TYPE_ITEM;

    tmpitem[index].hwnd = item;
    tmpitem[index].origwidth = tmpitem[index].width = width;
    tmpitem[index].origheight = tmpitem[index].height = height;
    tmpitem[index].pad = pad;
    if(hsize)
       tmpitem[index].hsize = _DW_SIZE_EXPAND;
    else
       tmpitem[index].hsize = _DW_SIZE_STATIC;

    if(vsize)
       tmpitem[index].vsize = _DW_SIZE_EXPAND;
    else
       tmpitem[index].vsize = _DW_SIZE_STATIC;

    thisbox->items = tmpitem;

    /* Update the item count */
    thisbox->count++;

    /* Add the item to the box */
#if 0
    /* Platform specific code to add item to box */
    BoxAdd(box, item);
#endif

    /* Free the old data */
    if(thisbox->count)
       free(thisitem);
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
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a button window or NULL on failure.
 */
HWND API dw_button_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to an entryfield window or NULL on failure.
 */
HWND API dw_entryfield_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to an entryfield password window or NULL on failure.
 */
HWND API dw_entryfield_password_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 * Returns:
 *       A handle to a bitmap button window or NULL on failure.
 */
HWND API dw_bitmapbutton_new(const char *text, ULONG resid)
{
    return 0;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a bitmap button window or NULL on failure.
 */
HWND API dw_bitmapbutton_new_from_file(const char *text, unsigned long cid, const char *filename)
{
    return 0;
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: The contents of the image
 *            (BMP or ICO on OS/2 or Windows, XPM on Unix)
 *       len: length of str
 * Returns:
 *       A handle to a bitmap button window or NULL on failure.
 */
HWND API dw_bitmapbutton_new_from_data(const char *text, unsigned long cid, const char *data, int len)
{
    return 0;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a spinbutton window or NULL on failure.
 */
HWND API dw_spinbutton_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
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
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 * Returns:
 *       Number value displayed in the spinbutton.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
    return 0;
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a radio button window or NULL on failure.
 */
HWND API dw_radiobutton_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a slider window or NULL on failure.
 */
HWND API dw_slider_new(int vertical, int increments, ULONG cid)
{
    return 0;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 * Returns:
 *       Position of the slider in the set range.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
    return 0;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a scrollbar window or NULL on failure.
 */
HWND API dw_scrollbar_new(int vertical, ULONG cid)
{
    return 0;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 * Returns:
 *       Position of the scrollbar in the set range.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
    return 0;
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
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
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a percent bar window or NULL on failure.
 */
HWND API dw_percent_new(ULONG cid)
{
    return 0;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
void API dw_percent_set_pos(HWND handle, unsigned int position)
{
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a checkbox window or NULL on failure.
 */
HWND API dw_checkbox_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 * Returns:
 *       State of checkbox (TRUE or FALSE).
 */
int API dw_checkbox_get(HWND handle)
{
    return FALSE;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 * Returns:
 *       A handle to a listbox window or NULL on failure.
 */
HWND API dw_listbox_new(ULONG cid, int multi)
{
    return 0;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, const char *text)
{
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
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
void API dw_listbox_clear(HWND handle)
{
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be counted.
 * Returns:
 *       The number of items in the listbox.
 */
int API dw_listbox_count(HWND handle)
{
    return 0;
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
void API dw_listbox_set_top(HWND handle, int top)
{
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
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 * Returns:
 *       The selected item index or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_listbox_selected(HWND handle)
{
    return DW_ERROR_UNKNOWN;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 * Returns:
 *       The next selected item or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_listbox_selected_multi(HWND handle, int where)
{
    return DW_ERROR_UNKNOWN;
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
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
void API dw_listbox_delete(HWND handle, int index)
{
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a combobox window or NULL on failure.
 */
HWND API dw_combobox_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a MLE window or NULL on failure.
 */
HWND API dw_mle_new(ULONG cid)
{
   return 0;
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 * Returns:
 *       Current position in the buffer.
 */
unsigned int API dw_mle_import(HWND handle, const char *buffer, int startpoint)
{
    return 0;
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
        *bytes = 0;
    if(lines)
        *lines = 0;
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
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
void API dw_mle_clear(HWND handle)
{
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
void API dw_mle_set_visible(HWND handle, int line)
{
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
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
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 * Returns:
 *       Position in buffer or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_mle_search(HWND handle, const char *text, int point, unsigned long flags)
{
    return DW_ERROR_UNKNOWN;
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
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a status text window or NULL on failure.
 */
HWND API dw_status_text_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       A handle to a text window or NULL on failure.
 */
HWND API dw_text_new(const char *text, ULONG cid)
{
    return 0;
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
HWND API dw_render_new(unsigned long cid)
{
    return 0;
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
void API dw_render_redraw(HWND handle)
{
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
    return value;
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
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height: Pointer to a variable to be filled in with the height.
 */
void API dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
{
}

/* Draw a polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       npoints: Number of points passed in.
 *       x: Pointer to array of X coordinates.
 *       y: Pointer to array of Y coordinates.
 */
void API dw_draw_polygon( HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y )
{
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
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       A handle to a tree window or NULL on failure.
 */
HWND API dw_tree_new(ULONG cid)
{
    return 0;
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
HTREEITEM API dw_tree_insert_after(HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    return 0;
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
    return 0;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A malloc()ed buffer of item text to be dw_free()ed or NULL on error.
 */
char * API dw_tree_get_title(HWND handle, HTREEITEM item)
{
    return NULL;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A handle to a tree item or NULL on failure.
 */
HTREEITEM API dw_tree_get_parent(HWND handle, HTREEITEM item)
{
    return 0;
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
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 * Returns:
 *       A pointer to tree item data or NULL on failure.
 */
void * API dw_tree_item_get_data(HWND handle, HTREEITEM item)
{
   return NULL;
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
void API dw_tree_item_select(HWND handle, HTREEITEM item)
{
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
void API dw_tree_clear(HWND handle)
{
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
void API dw_tree_item_expand(HWND handle, HTREEITEM item)
{
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
void API dw_tree_item_collapse(HWND handle, HTREEITEM item)
{
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
void API dw_tree_item_delete(HWND handle, HTREEITEM item)
{
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       A handle to a container window or NULL on failure.
 */
HWND API dw_container_new(ULONG cid, int multi)
{
    return 0;
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
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_container_setup(HWND handle, unsigned long *flags, char **titles, int count, int separator)
{
    return DW_ERROR_GENERAL;
}

/*
 * Configures the main filesystem column title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, const char *title)
{
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count)
{
    return DW_ERROR_GENERAL;
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 * Returns:
 *       Handle to container items allocated or NULL on error.
 */
void * API dw_container_alloc(HWND handle, int rowcount)
{
    return NULL;
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
}

/*
 * Gets column type for a container column.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 * Returns:
 *       Constant identifying the the column type.
 */
int API dw_container_get_column_type(HWND handle, int column)
{
    return 0;
}

/*
 * Gets column type for a filesystem container column.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 * Returns:
 *       Constant identifying the the column type.
 */
int API dw_filesystem_get_column_type(HWND handle, int column)
{
    return 0;
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
void API dw_container_set_row_title(void *pointer, int row, const char *title)
{
}


/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
void API dw_container_change_row_title(HWND handle, int row, const char *title)
{
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
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
void API dw_container_clear(HWND handle, int redraw)
{
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
void API dw_container_delete(HWND handle, int rowcount)
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
void API dw_container_scroll(HWND handle, int direction, long rows)
{
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 * Returns:
 *       Pointer to data associated with first entry or NULL on error.
 */
char * API dw_container_query_start(HWND handle, unsigned long flags)
{
    return NULL;
}

/*
 * Continues an existing query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 * Returns:
 *       Pointer to data associated with next entry or NULL on error or completion.
 */
char * API dw_container_query_next(HWND handle, unsigned long flags)
{
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
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_cursor_by_data(HWND handle, void *data)
{
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
void API dw_container_delete_row(HWND handle, const char *text)
{
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data:  Data usually returned by dw_container_query().
 */
void API dw_container_delete_row_by_data(HWND handle, void *data)
{
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
void API dw_container_optimize(HWND handle)
{
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
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 * Returns:
 *       Handle to the created icon or NULL on error.
 */
HICN API dw_icon_load(unsigned long module, unsigned long resid)
{
    return 0;
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       Handle to the created icon or NULL on error.
 */
HICN API dw_icon_load_from_file(const char *filename)
{
    return 0;
}

/*
 * Obtains an icon from data.
 * Parameters:
 *       data: Data for the icon (ICO on OS/2 or Windows, XPM on Unix, PNG on Mac)
 *       len: Length of the passed in data.
 * Returns:
 *       Handle to the created icon or NULL on error.
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
    return 0;
}

/*
 * Frees a loaded icon resource.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 * Returns:
 *       Handle to the created MDI widget or NULL on error.
 */
HWND API dw_mdi_new(unsigned long cid)
{
    return 0;
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
HWND API dw_splitbar_new(int type, HWND topleft, HWND bottomright, unsigned long cid)
{
    return 0;
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 *       percent: The position of the splitbar.
 */
void API dw_splitbar_set(HWND handle, float percent)
{
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 * Returns:
 *       Position of the splitbar (percentage).
 */
float API dw_splitbar_get(HWND handle)
{
    return 0;
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created bitmap widget or NULL on error.
 */
HWND API dw_bitmap_new(ULONG cid)
{
    return 0;
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
    return 0;
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
    return 0;
}

/*
 * Creates a pixmap from data in memory.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of the image data
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       len: Length of data
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new_from_data(HWND handle, const char *data, int len)
{
    return 0;
}

/*
 * Sets the transparent color for a pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  Transparent RGB color
 * Note: This is only necessary on platforms that
 *       don't handle transparency automatically
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
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
HPIXMAP API dw_pixmap_grab(HWND handle, ULONG resid)
{
    return 0;
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
    return DW_ERROR_GENERAL;
}

/*
 * Create a new calendar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created calendar or NULL on error.
 */
HWND API dw_calendar_new(ULONG cid)
{
    return 0;
}

/*
 * Sets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year, month, day: To set the calendar to display.
 */
void API dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
}

/*
 * Gets the year, month and day set in the calendar widget.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year: Variable to store the year or NULL.
 *       month: Variable to store the month or NULL.
 *       day: Variable to store the day or NULL.
 */
void API dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
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
    return DW_ERROR_GENERAL;
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
    return DW_ERROR_GENERAL;
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
int API dw_html_javascript_run(HWND handle, const char *script, void *scriptdata)
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
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 * Returns:
 *       Handle to the created html widget or NULL on error.
 */
HWND API dw_html_new(unsigned long cid)
{
    return 0;
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate or NULL.
 *       y: Pointer to variable to store Y coordinate or NULL.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_pointer_set_pos(long x, long y)
{
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used associated with this menu.
 * Returns:
 *       Handle to the created menu or NULL on error.
 */
HMENUI API dw_menu_new(ULONG cid)
{
    return 0;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 * Returns:
 *       Handle to the created menu bar or NULL on error.
 */
HMENUI API dw_menubar_new(HWND location)
{
    return 0;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns: 
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menux, unsigned long id)
{
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
 * Returns:
 *       Handle to the created menu item or NULL on error.
 */
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG itemid, ULONG flags, int end, int check, HMENUI submenux)
{
    return 0;
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
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 * Returns:
 *       Handle to the created notebook or NULL on error.
 */
HWND API dw_notebook_new(ULONG cid, int top)
{
    return 0;
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 * Returns:
 *       ID of newly created notebook page.
 */
unsigned long API dw_notebook_page_new(HWND handle, ULONG flags, int front)
{
    return 0;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned long pageid)
{
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 * Returns:
 *       ID of visible notebook page.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
    return 0;
}

/*
 * Sets the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned long pageid)
{
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
void API dw_notebook_pack(HWND handle, ULONG pageid, HWND page)
{
}

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags.
 * Returns:
 *       Handle to the created window or NULL on error.
 */
HWND API dw_window_new(HWND hwndOwner, const char *title, ULONG flStyle)
{
    return 0;
}

/*
 * Call a function from the window (widget)'s context (typically the message loop thread).
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
void API dw_window_function(HWND handle, void *function, void *data)
{
}


/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_show(HWND handle)
{
    return DW_ERROR_GENERAL;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to hide.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_hide(HWND handle)
{
    return DW_ERROR_GENERAL;
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in DW_RGB format or a default color index.
 *          back: Background color in DW_RGB format or a default color index.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
    return DW_ERROR_GENERAL;
}

/*
 * Sets the border size of a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_border(HWND handle, int border)
{
    return DW_ERROR_GENERAL;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          style: Style features enabled or disabled.
 *          mask: Corresponding bitmask of features to be changed.
 */
void API dw_window_set_style(HWND handle, ULONG style, ULONG mask)
{
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
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND handle, HWND defaultitem)
{
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND handle, HWND next)
{
}

/*
 * Captures the mouse input to this window even if it is outside the bounds.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_set_font(HWND handle, const char *fontname)
{
    return DW_ERROR_GENERAL;
}

/*
 * Returns the current font for the specified window.
 * Parameters:
 *           handle: The window handle from which to obtain the font.
 * Returns:
 *       A malloc()ed font name string to be dw_free()ed or NULL on error.
 */
char * API dw_window_get_font(HWND handle)
{
    return NULL;
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
    return NULL;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_destroy(HWND handle)
{
    return DW_ERROR_GENERAL;
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       The text associsated with a given window or NULL on error.
 */
char * API dw_window_get_text(HWND handle)
{
    return NULL;
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
void API dw_window_set_text(HWND handle, const char *text)
{
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
void API dw_window_set_tooltip(HWND handle, const char *bubbletext)
{
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_disable(HWND handle)
{
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
void API dw_window_enable(HWND handle)
{
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       data: memory buffer containing image (Bitmap on OS/2 or
 *                 Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 *       len: Length of data passed
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long cid, const char *data, int len)
{
   return DW_ERROR_UNKNOWN;
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
   return DW_ERROR_UNKNOWN;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       icon: Handle to icon to be used.
 */
void API dw_window_set_icon(HWND handle, HICN icon)
{
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 * Returns:
 *       HWND of window with ID or NULL on error.
 */
HWND API dw_window_from_id(HWND handle, int id)
{
   return 0;
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_minimize(HWND handle)
{
    return DW_ERROR_GENERAL;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_raise(HWND handle)
{
    return DW_ERROR_GENERAL;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_window_lower(HWND handle)
{
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
}

/*
 * Gets the size the system thinks the widget should be.
 * Parameters:
 *       handle: Window (widget) handle of the item to query.
 *       width: Width in pixels of the item or NULL if not needed.
 *       height: Height in pixels of the item or NULL if not needed.
 */
void API dw_window_get_preferred_size(HWND handle, int *width, int *height)
{
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
}

/*
 * Gets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left or NULL.
 *          y: Y location from the bottom left or NULL.
 *          width: Width of the widget or NULL.
 *          height: Height of the widget or NULL.
 */
void API dw_window_get_pos_size(HWND handle, LONG *x, LONG *y, ULONG *width, ULONG *height)
{
}

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
    return 0;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
    return 0;
}

/* This should return the current color depth. */
unsigned long API dw_color_depth_get(void)
{
   return 0;
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
    strcpy(env->osName, "Unknown");

    strcpy(env->buildDate, __DATE__);
    strcpy(env->buildTime, __TIME__);
    env->DWMajorVersion = DW_MAJOR_VERSION;
    env->DWMinorVersion = DW_MINOR_VERSION;
    env->DWSubVersion = DW_SUB_VERSION;

    env->MajorVersion = 0; /* Operating system major */
    env->MinorVersion = 0; /* Operating system minor */
    env->MajorBuild = 0; /* Build versions... if available */
    env->MinorBuild = 0;
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void API dw_flush(void)
{
}

/*
 * Add a named user data item to a window handle.
 * Parameters:
 *       window: Window handle to save data to.
 *       dataname: A string pointer identifying which data to be saved.
 *       data: User data to be saved to the window handle.
 */
void API dw_window_set_data(HWND window, const char *dataname, void *data)
{
}

/*
 * Gets a named user data item from a window handle.
 * Parameters:
 *       window: Window handle to get data from.
 *       dataname: A string pointer identifying which data to get.
 * Returns:
 *       Pointer to data or NULL if no data is available.
 */
void * API dw_window_get_data(HWND window, const char *dataname)
{
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
    return 0;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(HTIMER timerid)
{
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
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       signame: Signal name to be matched on window.
 */
void API dw_signal_disconnect_by_name(HWND window, const char *signame)
{
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_window(HWND window)
{
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void API dw_signal_disconnect_by_data(HWND window, void *data)
{
}

/* Open a shared library and return a handle.
 * Parameters:
 *         name: Base name of the shared library.
 *         handle: Pointer to a module handle,
 *                 will be filled in with the handle.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
*/
int API dw_module_load(const char *name, HMOD *handle)
{
   return DW_ERROR_UNKNOWN;
}

/* Queries the address of a symbol within open handle.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 *         name: Name of the symbol you want the address of.
 *         func: A pointer to a function pointer, to obtain
 *               the address.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_module_symbol(HMOD handle, const char *name, void**func)
{
   return DW_ERROR_UNKNOWN;
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_module_close(HMOD handle)
{
   return DW_ERROR_GENERAL;
}

/*
 * Returns the handle to an unnamed mutex semaphore or NULL on error.
 */
HMTX API dw_mutex_new(void)
{
    return NULL;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_lock(HMTX mutex)
{
#if 0
    /* We need to handle locks from the main thread differently...
     * since we can't stop message processing... otherwise we
     * will deadlock... so try to acquire the lock and continue
     * processing messages in between tries.
     */
    if(_dw_thread == dw_thread_id())
    {
        while(/* Attempt to lock the mutex */)
        {
            /* Process any pending events */
            while(dw_main_iteration())
            {
                /* Just loop */
            }
        }
    }
    else
    {
        /* Lock the mutex */
    }
#endif
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
    return DW_ERROR_GENERAL;
}

/*
 * Reliquishes the access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_unlock(HMTX mutex)
{
}

/*
 * Returns the handle to an unnamed event semaphore or NULL on error.
 */
HEV API dw_event_new(void)
{
   return NULL;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_event_reset (HEV eve)
{
   return DW_ERROR_GENERAL;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_event_post (HEV eve)
{
   return DW_ERROR_GENERAL;
}

/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 *   timeout: Number of milliseconds before timing out
 *                  or -1 if indefinite.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 *       DW_ERROR_TIMEOUT (2) if the timeout has expired.
 *       Other values on other error.
 */
int API dw_event_wait(HEV eve, unsigned long timeout)
{
   return DW_ERROR_GENERAL;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_event_close(HEV *eve)
{
   return DW_ERROR_GENERAL;
}

/* Create a named event semaphore which can be
 * opened from other processes.
 * Parameters:
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 * Returns:
 *       Handle to event semaphore or NULL on error.
 */
HEV API dw_named_event_new(const char *name)
{
    return NULL;
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 * Returns:
 *       Handle to event semaphore or NULL on error.
 */
HEV API dw_named_event_get(const char *name)
{
    return NULL;
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an get or new call.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_named_event_reset(HEV eve)
{
   return DW_ERROR_GENERAL;
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an get or new call.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_named_event_post(HEV eve)
{
   return DW_ERROR_GENERAL;
}

/* Waits on the specified semaphore until it becomes
 * posted, or returns immediately if it already is posted.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an get or new call.
 *         timeout: Number of milliseconds before timing out
 *                  or -1 if indefinite.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_named_event_wait(HEV eve, unsigned long timeout)
{
   return DW_ERROR_UNKNOWN;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an get or new call.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_named_event_close(HEV eve)
{
    return DW_ERROR_UNKNOWN;
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 *           argc: Passed in from main()
 *           argv: Passed in from main()
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
    return DW_ERROR_NONE;
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 * Returns:
 *       Handle to shared memory or NULL on error.
 */
HSHM API dw_named_memory_new(void **dest, int size, const char *name)
{
   return NULL;
}

/*
 * Aquires shared memory region with a name.
 * Parameters:
 *       dest: A pointer to a pointer to receive the memory address.
 *       size: Size in bytes of the shared memory region to requested.
 *       name: A string pointer to a unique memory name.
 * Returns:
 *       Handle to shared memory or NULL on error.
 */
HSHM API dw_named_memory_get(void **dest, int size, const char *name)
{
   return NULL;
}

/*
 * Frees a shared memory region previously allocated.
 * Parameters:
 *       handle: Handle obtained from dw_named_memory_new().
 *       ptr: The memory address aquired with dw_named_memory_new().
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_named_memory_free(HSHM handle, void *ptr)
{
   int rc = DW_ERROR_UNKNOWN;

   return rc;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
}

/*
 * Creates a new thread with a starting point of func.
 * Parameters:
 *       func: Function which will be run in the new thread.
 *       data: Parameter(s) passed to the function.
 *       stack: Stack size of new thread (OS/2 and Windows only).
 * Returns:
 *       Thread ID on success or DW_ERROR_UNKNOWN (-1) on error.
 */
DWTID API dw_thread_new(void *func, void *data, int stack)
{
   return (DWTID)DW_ERROR_UNKNOWN;
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
   return (DWTID)0;
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
}

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       Process ID on success or DW_ERROR_UNKNOWN (-1) on error.
 */
int API dw_exec(const char *program, int type, char **params)
{
    int ret = DW_ERROR_UNKNOWN;

    return ret;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 * Returns:
 *       DW_ERROR_UNKNOWN (-1) on error; DW_ERROR_NONE (0) or a positive Process ID on success.
 */
int API dw_browse(const char *url)
{
    return DW_ERROR_UNKNOWN;
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
   return DW_ERROR_UNKNOWN;
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
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
   return 0;
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
   return DW_ERROR_UNKNOWN;
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
    return NULL;
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
    return NULL;
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
#if 0
        case DW_FEATURE_HTML:                    /* Supports the HTML Widget */
        case DW_FEATURE_HTML_RESULT:             /* Supports the DW_SIGNAL_HTML_RESULT callback */
        case DW_FEATURE_WINDOW_BORDER:           /* Supports custom window border sizes */
        case DW_FEATURE_WINDOW_TRANSPARENCY:     /* Supports window frame transparency */
        case DW_FEATURE_DARK_MODE:               /* Supports Dark Mode user interface */
        case DW_FEATURE_MLE_AUTO_COMPLETE:       /* Supports auto completion in Multi-line Edit boxes */
        case DW_FEATURE_MLE_WORD_WRAP:           /* Supports word wrapping in Multi-line Edit boxes */
        case DW_FEATURE_CONTAINER_STRIPE:        /* Supports striped line display in container widgets */ 
        case DW_FEATURE_MDI:                     /* Supports Multiple Document Interface window frame */
        case DW_FEATURE_NOTEBOOK_STATUS_TEXT:    /* Supports status text area on notebook/tabbed controls */
        case DW_FEATURE_NOTIFICATION:            /* Supports sending system notifications */
        case DW_FEATURE_UTF8_UNICODE:            /* Supports UTF8 encoded Unicode text */
        case DW_FEATURE_MLE_RICH_EDIT:           /* Supports Rich Edit based MLE control (Windows) */
        case DW_FEATURE_TASK_BAR:                /* Supports icons in the taskbar or similar system widget */
        case DW_FEATURE_TREE:                    /* Supports the Tree Widget */
        case DW_FEATURE_WINDOW_PLACEMENT:        /* Supports arbitrary window placement */
            return DW_FEATURE_ENABLED;
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
#if 0
        case DW_FEATURE_HTML:                    /* Supports the HTML Widget */
        case DW_FEATURE_HTML_RESULT:             /* Supports the DW_SIGNAL_HTML_RESULT callback */
        case DW_FEATURE_WINDOW_BORDER:           /* Supports custom window border sizes */
        case DW_FEATURE_WINDOW_TRANSPARENCY:     /* Supports window frame transparency */
        case DW_FEATURE_DARK_MODE:               /* Supports Dark Mode user interface */
        case DW_FEATURE_MLE_AUTO_COMPLETE:       /* Supports auto completion in Multi-line Edit boxes */
        case DW_FEATURE_MLE_WORD_WRAP:           /* Supports word wrapping in Multi-line Edit boxes */
        case DW_FEATURE_CONTAINER_STRIPE:        /* Supports striped line display in container widgets */ 
        case DW_FEATURE_MDI:                     /* Supports Multiple Document Interface window frame */
        case DW_FEATURE_NOTEBOOK_STATUS_TEXT:    /* Supports status text area on notebook/tabbed controls */
        case DW_FEATURE_NOTIFICATION:            /* Supports sending system notifications */
        case DW_FEATURE_UTF8_UNICODE:            /* Supports UTF8 encoded Unicode text */
        case DW_FEATURE_MLE_RICH_EDIT:           /* Supports Rich Edit based MLE control (Windows) */
        case DW_FEATURE_TASK_BAR:                /* Supports icons in the taskbar or similar system widget */
        case DW_FEATURE_TREE:                    /* Supports the Tree Widget */
        case DW_FEATURE_WINDOW_PLACEMENT:        /* Supports arbitrary window placement */
            return DW_ERROR_GENERAL;
#endif
        /* These features are supported and configurable */
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}
