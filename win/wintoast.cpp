/* Simple WinToast forwarder from Dynamic Windows APIs */

#include "dw.h"
#include "wintoastlib.h"

using namespace WinToastLib;

extern "C" {
   LRESULT CALLBACK _dw_wndproc(HWND hWnd, UINT msg, WPARAM mp1, LPARAM mp2);
}

class DWHandler : public IWinToastHandler {
public:
    WinToastTemplate *templ;

    void toastActivated() const {
        // The user clicked in this toast
        _dw_wndproc((HWND)templ, WM_USER+102, 0, 0);
        dw_signal_disconnect_by_window((HWND)templ);
        delete templ;
    }

    void toastActivated(int actionIndex) const {
        // The user clicked on action
        _dw_wndproc((HWND)templ, WM_USER+102, 0, 0);
        dw_signal_disconnect_by_window((HWND)templ);
        delete templ;
    }

    void toastDismissed(WinToastDismissalReason state) const {
        switch (state) {
        case UserCanceled:
            // The user dismissed this toast
            dw_signal_disconnect_by_window((HWND)templ);
            delete templ;
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
         // Generate a Microsoft compatible Application User Model ID
         LPWSTR company = wcschr(AppID, '.');
         *company = 0;
         LPWSTR product = wcschr(++company, '.');
         *product = 0;
         LPWSTR subproduct = wcschr(++product, '.');
         if(subproduct)
                *subproduct = 0;
         LPWSTR version = subproduct ? wcschr(++subproduct, '.') : NULL;

         WinToast::instance()->setAppName(AppName);
         WinToast::instance()->setAppUserModelId(WinToast::instance()->configureAUMI(company, product, 
             subproduct ? subproduct : L"", (version && version++ && _wtoi(version) > 0) ? version : L""));
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
         {
            WCHAR fullpath[MAX_PATH+1] = {0};
            
            GetFullPathNameW(image, MAX_PATH, fullpath, NULL);
            templ->setImagePath(fullpath);
         }
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
            return DW_ERROR_NONE;
      }
      return DW_ERROR_UNKNOWN;
   }

   BOOL _dw_toast_is_compatible(void)
   {
       return WinToast::isCompatible();
   }
} 
