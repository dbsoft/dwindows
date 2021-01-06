This is a stable release of Dynamic Windows version 3.1.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD and OpenSolaris.

Build Recommendations:
MacOS:
    11.0: configure --with-arch=modern --with-minver=10.14
        64bit Intel and Apple Silicon support with Dark Mode.
    10.13-10.15: configure --with-minver=10.8
        64bit Intel with Notifications, Dark Mode for 10.14-15.
    10.8-10.12: configure --with-arch=intel --with-minver=10.8
        64 and 32bit Intel with Notifications but no Dark Mode.
    10.6: configure --with-arch=classic --with-minver=10.5
        32bit PowerPC, 64bit and 32bit Intel classic support.
        No Notifications, Dark Mode nor NSView container/trees.
Windows:
    10: Visual Studio 2017-2019, WebView2 and WinToast.
        Should run on Vista and later, but sockpipe() only on 10.
        Supports domain sockets on Windows 10 dwcompat sockpipe().
    7-8.1: Visual Studio 2015, WebView2 and WinToast with 8 SDK.
        Should run on Vista and later, old sockpipe() on all.
    XP: Visual Studio 2010.  Old sockpipe() on all versions.
        Should run on XP and later, with Aero on Vista and 7.
        No Notifications nor WebView2 and old sockpipe() on all.
    2000: Visual Studio 2005. Remove -DAEROGLASS from CFLAGS.
        Should run on 2000 and later, no Aero, Notifications, 
        WebView2 and old sockpipe() on all versions.

Known problems:

Boxes with no expandable items will have their contents centered on 
    GTK2 instead of top or left justified on the other platforms.
GTK3 due to changes in the core architecture does not support
    widgets that are smaller than what is contained within them,
    unless they use scrolled windows. GTK2 and other platforms do.
    Therefore windows or other elements may expand their size to 
    fit the contents, overriding requested size settings.
In Unicode mode on OS/2 there are some bugs in the input controls,
    minor bugs in entryfield based controls and major bugs in the MLE.
    The text displays properly but the cursor and selection jumps
    around oddly when passing over multibyte characters.

Known limitations:

It is not safe on all platforms to operate on widgets before they
are packed.  For portability pack widgets before operating on them.

Future features:

OS/2 is currently missing the HTML widget because the system does 
not support it by default. Looking into importing functionality 
from available libraries (Firefox, Webkit, Qt, etc).
OS/2 is also missing a notification system, so the new notification
APIs are not yet supported on OS/2.  May implement our own system 
if a popular notification system is not already in existance.
Ports to iOS and Android are underway.

Changes from version 3.0:
Added support for MacOS versions through Big Sur 11.0, 
    Windows versions through 10 build 20H2.
Fixed a handle leak on OS/2 when built with (Open)Watcom.
Added dark mode support on MacOS Mojave 10.14 and later. 
Added experimental dark mode support on Windows 10 build 1809 (disabled by default).
Added embedding Microsoft Edge (Chromium) support on Windows 7 and higher.
    Requires Windows 8 or higher SDK to build and the nuget package from:
    https://www.nuget.org/packages/Microsoft.Web.WebView2 unzipped into
    .\packages\Microsoft.Web.WebView2
    Install runtime: https://developer.microsoft.com/en-us/microsoft-edge/webview2/
    Will prefer to use the runtime above, but will fallback to an installed browser.
Added notification APIs: dw_notification_new() dw_notification_send() dw_app_id_set()
    Requires Windows 8 or higher. MacOS 10.8 or higher. GLib 2.40 or higher on Unix.
    MacOS also requires the application bundle being signed or self-signed.
    Unix requires a desktop file link with the application ID used in dw_app_id_set().
    Unzip WinToast from https://github.com/mohabouje/WinToast into  .\packages\WinToast
Added webkit2gtk support and removed dead gtkmozembed and libgtkhtml2 support on Unix.
Added embedded HTML javascript support on Mac, Windows and Unix with webkit(2)gtk. 
    Added function dw_html_javascript_run() to execute javascript code.
    Added DW_SIGNAL_HTML_RESULT signal for getting the results from javascript.
    DW_SIGNAL_HTML_RESULT requires webkit2gtk on Unix.
Added DW_SIGNAL_HTML_CHANGED signal handler for getting the status of embedded HTML.
    Status can be: DW_HTML_CHANGE_STARTED/REDIRECT/LOADING/COMPLETE
Fixed international calendar issues on Mac.
Added dw_mle_set_auto_complete() to enable completion, only available on Mac.
Changed to using GTK3 by default instead of GTK2.  --with-gtk2 is now available.
Changed to using winsock2 on Windows ending support for Win95 and NT 3.5.
Changed to using Rich Edit for MLE controls on Windows. This can be disabled with:
    dw_feature_set(DW_FEATURE_MLE_RICH_EDIT, DW_FEATURE_DISABLED);
Added support for domain sockets on Windows 10 in the dwcompat sockpipe() macro.
    If compiled with Visual Studio 2017 or later, otherwise the old method is used.
Added dw_feature_set/get() to test if certain features are available on the current
    library and operating system combination at runtime or enabled/disable features.
Added support for GResource embedded images on GTK2/3 with GLib 2.32. The old 
    resouce system is still available via configure --with-deprecated if needed.
Added support for NSView based Tree, Container and Listbox widgets on Mac 10.7+.
Removed DW_FCF_COMPOSITED support from Windows 8 and higher. 
    This flag will still function to create a glass effect on Windows Vista and 7.
    The transparent key feature used to create it causes issues on 8 and 10, plus
    the glass effect, the main reason for the flag was removed in Windows 8.
Removed the incomplete Photon port.
Fixed many small bugs, too numerous to list here.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
