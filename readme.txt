This is a stable release of Dynamic Windows version 3.2.

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
    7-10: Visual Studio 2017-2019, WebView2 and WinToast.
        Should run on Vista and later, supports domain sockets
        on Windows 10, oldsockpipe() on older versions.
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

Changes from version 3.1:
Added support for dw_window_set_font() with a NULL font paramaeter.
    This resets the font used on the widget to the default font.
Fixed GTK warnings on GTK3 caused by using Pango style font syntax.
Fixed GTK3 leaks when setting fonts or colors on a widget repeatedly.
Fixed incorrect reporting of word wrap support on Windows.
Fixed a number of misbehaviors with dw_window_set_font() and 
    dw_window_set_color() on various platforms.
Added missing DW_CLR_DEFAULT support on OS/2, it previously only
    worked to avoid setting a color, not resetting them to default.
Added oldsockpipe() macro which will be used as a fallback.
    This allows us to use domain socket sockpipe() when available...
    and fall back to the old version when not, letting us have
    Windows 10 domain socket builds that work on earlier versions.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
