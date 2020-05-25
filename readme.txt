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

Known limitations:

It is not safe on all platforms to operate on widgets before they
are packed.  For portability pack widgets before operating on them.

Future features:

OS/2 is currently missing the HTML widget because the system does 
not support it by default. Looking into importing functionality 
from available libraries (Firefox, Webkit, Qt, etc).

Changes from version 3.0:
Added support for MacOS versions through Catalina 10.15, 
    Windows versions through 10 build 1909.
Fixed handle leak on OS/2 when built with (Open)Watcom.
Added dark mode support on MacOS Mojave 10.14 and later. 
Added experimental dark mode support on Windows 10 build 1809 (disabled by default).
Added embedding Microsoft Edge (Chromium) support on Windows 7 and higher.
    Requires Windows 8 or higher SDK to build and the nuget package from:
    https://www.nuget.org/packages/Microsoft.Web.WebView2 unzipped into
    .\packages\Microsoft.Web.WebView2
Added notification APIs: dw_notification_new() dw_notification_send() dw_app_id_set()
    Requires Windows 8 or higher. MacOS 10.8 or higher. GLib 2.40 or higher on Unix.
    MacOS also requires the application bundle being signed or self-signed.
    Unix requires a desktop file link with the application ID used in dw_app_id_set().

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
