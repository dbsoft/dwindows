This is a stable release of Dynamic Windows version 3.1.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD and Solaris.

Known problems:

Boxes with no expandable items will have their contents centered on 
    GTK2 instead of top or left justified on the other platforms.
GTK3 due to changes in the core architecture does not support
    widgets that are smaller than what is contained within them
    unless they use scrolled windows. GTK2 and other platforms do.
In Unicode mode on OS/2 there are some bugs in the input controls,
    minor bugs in entryfield based controls and major bugs in the MLE.
    The text displays properly but the cursor and selection jumps
    around oddly when passing over multibyte characters.
Building for MacOS 10.14 or later may prevent apps from running on
    earlier versions.  Building for versions prior to 10.14 should
    work on any supported version but dark mode may not be available
    for these apps running on 10.14 and later.  If you wish to support
    dark mode I suggest building 2 versions, one for 64 bit intel 
    only for 10.14 and later. Then a fat binary for prior to 10.14.

Known limitations:

It is not safe on all platforms to operate on widgets before they
are packed.  For portability pack widgets before operating on them.

Future features:

OS/2 is currently missing the HTML widget because the system does 
not support it by default. Looking into importing functionality 
from available libraries (Firefox, Webkit, Qt, etc).

Changes from version 3.0:
Added support for MacOS versions through Big Sur 11.0, 
    Windows versions through 10 build 20H2.
Fixed handle leak on OS/2 when built with (Open)Watcom.
Added dark mode support on MacOS Mojave 10.14 and later. 
Added experimental dark mode support on Windows 10 build 1809 (disabled by default).
Added embedding Microsoft Edge (Chromium) support on Windows 7 and higher.
    Requires Windows 8 or higher SDK to build and the nuget package from:
    https://www.nuget.org/packages/Microsoft.Web.WebView2 unzipped into
    .\packages\Microsoft.Web.WebView2
    Install runtime: https://developer.microsoft.com/en-us/microsoft-edge/webview2/
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
    the glass effect, the main reason for the flag was removed in 8.
Fixed many small bugs, too numerous to list here.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
