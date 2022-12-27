This is a stable release of Dynamic Windows version 3.3.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD and OpenSolaris.
The source base is considered beta on: iOS, Android and GTK4.

Build Recommendations:
MacOS:
    11-13: configure --with-arch=modern --with-minver=10.14
        64bit Intel and Apple Silicon (ARM64) with Dark Mode.
    10.13-10.15: configure --with-minver=10.8
        64bit Intel with Notifications, Dark Mode for 10.14-15.
    10.8-10.12: configure --with-arch=intel --with-minver=10.8
        64 and 32bit Intel with Notifications but no Dark Mode.
    10.5-10.6: configure --with-arch=classic --with-minver=10.5
        32bit PowerPC, 64bit and 32bit Intel classic support.
        No Notifications, Dark Mode nor NSView container/trees.
Windows:
    7-11: Visual Studio 2017-2022, WebView2 and WinToast.
        Should run on Vista and later, supports domain sockets
        on Windows 10, oldsockpipe() on older versions.
    XP: Visual Studio 2010.
        Should run on XP and later, with Aero on Vista and 7.
        No Notifications nor WebView2 and oldsockpipe() on all.
    2000: Visual Studio 2005. Remove -DAEROGLASS from CFLAGS.
        Should run on 2000 and later, no Aero, Notifications, 
        WebView2 and oldsockpipe() on all versions.

Known problems:

Boxes with no expandable items will have their contents centered on 
    GTK2 instead of top or left justified on the other platforms.
    Pack an expandable NULL item at the end of the box to keep the
    same appearance as other platforms. 
GTK3/4 due to changes in the core architecture does not support
    widgets that are smaller than what is contained within them,
    unless they use scrolled windows. GTK2 and other platforms do.
    Therefore windows or other elements may expand their size to 
    fit the contents, overriding requested size settings.
In Unicode mode on OS/2 there are some bugs in the input controls,
    minor bugs in entryfield based controls and major bugs in the MLE.
    The text displays properly but the cursor and selection jumps
    around oddly when passing over multibyte characters.
System scaling on Windows versions earlier than 10 will scale the
    individual controls, but will not scale the top-level window size.
    Windows 10 and higher will scale both the controls and window.

Known limitations:

It is not safe on all platforms to operate on widgets before they
are packed.  For portability pack widgets before operating on them.

Changes from version 3.2:
Added tree widget/control support for iOS and Android.
Removed the lib and dll directories previously used on Windows and OS/2.
    On Windows x86 and x64 subdirectories are created automatically
    This allows platform specific versions to be accessible without a 
    rebuild. Also removed the DWDLLDIR variable on Windows. If you have
    DWLIBDIR pointing to the "lib" subdirectory please remove "\lib".
Added DW_FEATURE_CONTAINER_MODE on Mobile platforms: iOS and Android.
    DW_CONTAINER_MODE_DEFAULT: Minimal container; icon and text only.
    DW_CONTAINER_MODE_EXTRA: Extra columns displayed on a second line.
    DW_CONTAINER_MODE_MULTI: A separate clickable line for each column.
Added return values to several functions previously returning void.
    Previous code should just be able to ignore the new return values.
    Currently affected: dw_window_set_bitmap(_from_data)
Added C++ language bindings in dw.hpp and an example C++ test
    application in the form of dwtestoo.cpp, similar to godwindows.
Added variadic versions of dw_debug() and dw_messagebox().
    This is how the standard library does it so we can call the new
    va_list versions from C++: dw_vdebug() and dw_vmessagebox().
Added support for MacOS 13 Ventura and iOS 16.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
