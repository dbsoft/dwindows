/* Simple WinToast forwarder from Dynamic Windows APIs */

#include "wintoastlib.h"

using namespace WinToastLib;

extern "C" {
   LRESULT CALLBACK _wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
   void dw_signal_disconnect_by_window(HWND window);
#ifdef DEBUG
   void dw_debug(const char *format, ...);
#endif
}

class DWHandler : public IWinToastHandler {
public:
    WinToastTemplate *templ;

    void toastActivated() const {
        // The user clicked in this toast
#ifdef DEBUG
        dw_debug("Sending notification to DW eventhandler\n");
#endif
        _wndproc((HWND)templ, WM_USER+102, 0, 0);
    }

    void toastActivated(int actionIndex) const {
        // The user clicked on action
#ifdef DEBUG
        dw_debug("Sending notification to DW eventhandler via action\n");
#endif
        _wndproc((HWND)templ, WM_USER+102, 0, 0);
    }

    void toastDismissed(WinToastDismissalReason state) const {
        switch (state) {
        case UserCanceled:
            // The user dismissed this toast
#ifdef DEBUG
            dw_debug("The user dismissed this toast\n");
#endif
            delete templ;
            break;
        case TimedOut:
            // The toast has timed out
#ifdef DEBUG
            dw_debug("The toast has timed out\n");
#endif
            break;
        case ApplicationHidden:
            // The application hid the toast using ToastNotifier.hide()
#ifdef DEBUG
            dw_debug("The application hid the toast using ToastNotifier.hide()\n");
#endif
            break;
        default:
            // Toast not activated
            break;
        }
    }

    void toastFailed() const {
        // Error showing current toast
#ifdef DEBUG
        dw_debug("Toast failed freeing template\n");
#endif
        delete templ;
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
         DWHandler *handler = new DWHandler();
         handler->templ = templ;

         if(templ && WinToast::instance()->showToast(*templ, handler) >= 0)
            return 0; // DW_ERROR_NONE
      }
      return -1; // DW_ERROR_UNKNOWN
   }

   BOOL _dw_toast_is_compatible(void)
   {
       return WinToast::isCompatible();
   }
} 
