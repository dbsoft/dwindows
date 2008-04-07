#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define MOZILLA_INTERNAL_API

#include <gtk/gtk.h>
#include <gtkmozembed.h>
#include <gtkmozembed_internal.h>
#include "nsIDOMMouseEvent.h"

/**
 * Takes a pointer to a mouse event and returns the mouse
 *  button number or -1 on error.
 */
extern "C" gint mozilla_get_mouse_event_button(gpointer event)
{
   gint  button = 0;
   glong x,y;

   g_return_val_if_fail (event, -1);

   /* the following lines were found in the Galeon source */
   nsIDOMMouseEvent *aMouseEvent = (nsIDOMMouseEvent *) event;
   aMouseEvent->GetButton ((PRUint16 *) &button);
   aMouseEvent->GetClientX ((PRInt32 *) &x);
   aMouseEvent->GetClientY ((PRInt32 *) &y);


   /* for some reason we get different numbers on PPC, this fixes
    * that up... -- MattA */
   if (button == 65536)
   {
      button = 1;
   }
   else if (button == 131072)
   {
      button = 2;
   }

   return button;
}
extern "C" gint mozilla_get_mouse_location( gpointer event, glong *x, glong *y)
{
   g_return_val_if_fail (event, -1);

   /* the following lines were found in the Galeon source */
   nsIDOMMouseEvent *aMouseEvent = (nsIDOMMouseEvent *) event;
   aMouseEvent->GetClientX ((PRInt32 *) x);
   aMouseEvent->GetClientY ((PRInt32 *) y);
   return 0;
}
