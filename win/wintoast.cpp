/* Simple WinToast forwarder from Dynamic Windows APIs */

#include "wintoastlib.h"

using namespace WinToastLib;

class DWHandler : public IWinToastHandler {
public:
    void toastActivated() const {
        // The user clicked in this toast
    }

    void toastActivated(int actionIndex) const {
        // The user clicked on action
    }

    void toastDismissed(WinToastDismissalReason state) const {
        switch (state) {
        case UserCanceled:
            // The user dismissed this toast"
            break;
        case TimedOut:
            // The toast has timed out
            break;
        case ApplicationHidden:
            // The application hid the toast using ToastNotifier.hide()
            break;
        default:
            // Toast not activated
            break;
        }
    }

    void toastFailed() const {
        // Error showing current toast
    }
};


enum Results {
	ToastClicked,					// user clicked on the toast
	ToastDismissed,					// user dismissed the toast
	ToastTimeOut,					// toast timed out
	ToastHided,						// application hid the toast
	ToastNotActivated,				// toast was not activated
	ToastFailed,					// toast failed
	SystemNotSupported,				// system does not support toasts
	UnhandledOption,				// unhandled option
	MultipleTextNotSupported,		// multiple texts were provided
	InitializationFailure,			// toast notification manager initialization failure
	ToastNotLaunched				// toast could not be launched
};

extern "C" {
   
   void _dw_toast_init(LPWSTR AppName, LPWSTR AppID)
   {
      if(WinToast::isCompatible()) 
      {
         WinToast::instance()->setAppName(AppName);
         WinToast::instance()->setAppUserModelId(AppID);
         WinToast::instance()->initialize();
      }
   }

   void *_dw_notification_new(LPWSTR title, LPWSTR image, LPWSTR description)
   {
      if(WinToast::isCompatible()) 
      {
         WinToastTemplate *templ = new WinToastTemplate(image ? WinToastTemplate::ImageAndText02 : WinToastTemplate::Text02);
         templ->setTextField(title, WinToastTemplate::FirstLine);
         templ->setAttributionText(description);
         if(image)
            templ->setImagePath(image);
         return (void *)templ;
      }
      return NULL;
   }

   int _dw_notification_send(void *notification)
   {
      if(WinToast::isCompatible()) 
      {
         WinToastTemplate *templ = (WinToastTemplate *)notification;
         
         if(templ && WinToast::instance()->showToast(*templ, new DWHandler()) >= 0)
            return 0; // DW_ERROR_NONE
      }
      return -1; // DW_ERROR_UNKNOWN
   }
   
   BOOL _dw_toast_is_compatible(void)
   {
       return WinToast::isCompatible();
   }
} 
