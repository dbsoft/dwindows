This is a stable release of Dynamic Windows version 3.2.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD and OpenSolaris.
The source base is considered beta on: iOS and Android.

Build Recommendations:
MacOS:
    11-12: configure --with-arch=modern --with-minver=10.14
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
GTK3/4 due to changes in the core architecture does not support
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

Changes from version 3.1:
Changed handling of button 3 press and release events.
    WARNING: This could be a breaking change for apps handling
    mouse button 3.  Windows, Mac and GTK now pass button 3 as
    the mask (value 4) instead of the button number (value 3).
    OS/2 and the related motion event passed buttons as a mask.
    In these value 3 is buttons 1 and 2 together.  Button 
    press and release events are only for a single button...
    so this would never happen allowing value 3 for button 3.
    To allow the same code to work for button presses and motion
    events, I am standardizing on using the masks for both.
    If you use button 3 press/release, please audit code for 3.2.
Added initial support for GTK4. --with-gtk4 is now available.
    GTK4 support is less complete than GTK3, a number of features
    of the GTK3 version are no longer possible in GTK4 like:
    Taskbar icon support, MDI, gravity and non-callback drawing.
Added initial support for Wayland on GTK3 and GTK4.
    Wayland is not in my opinion ready for prime time, many features,
    possibly a half a dozen functions, dealing with coordinates, 
    mouse pointer grabbing and such are unable to function.
    I recommend sticking with X11 for now, but built with GTK3 or
    GTK4 Dynamic Windows applications will run on Wayland now.
Added initial iOS support, kicking off a push for mobile.
    iOS requires 13.0 or later due use of SF Symbols and features
    introduced with iOS 13 and Mac Catalyst. Several widgets are
    currently unsupported: Tree, MDI and Taskbar.
    Command line builds not supported, create an Xcode project.
Added initial Android support, Android Studio with Kotlin required.
    API 23 (Android 6) or later is required to run the apps.
    Like iOS several widgets are not supported: Tree, Taskbar, MDI.
    Command line builds not supported, create a JNI project.
Added DW_FEATURE_WINDOW_PLACEMENT to test to see if we can get or
    set the positions of the windows on the screen.  Unavailable 
    on the following: iOS, Android, GTK3 or GTK4 with Wayland.
Added DW_FEATURE_TREE to test to see if we can use the tree widget.
    The tree widget is unsupported on: iOS and Android.
Added DW_FEATURE_TASKBAR to test to see if we can use a taskbar icon.
    The only platform that supports this feature on all supported
    versions is Windows. OS/2 requires installed software, Mac
    requires Yosemite and later.  GTK requires GTK2 or GTK3.
    iOS and Android are unsupported.
Added dw_render_redraw() function to trigger a DW_SIGNAL_EXPOSE 
    event on render widgets allowing drawing to happen in the
    callback. GTK4 and GTK3 with Wayland require drawing to be 
    done in the callback, necessitating a dw_render_redraw() or
    dw_flush() call.  dw_flush() may cause multiple draw passes.
Added new function dw_window_compare() to check if two window handles
    reference the same object.  Necessary in the Android port since
    handles passed to callbacks can be local references, so they
    don't always match the handles saved during window creation.
Added support for dw_window_set_font() with a NULL font parameter.
    This resets the font used on the widget to the default font.
Added a new constant DW_SIZE_AUTO to pass to box packing functions.
Added a new constant DW_DIR_SEPARATOR which is char that defines
    the path directory separator on the running platform.
Changed the entrypoint to be dwmain() instead of main().
    This allows special handling of the entrypoint on systems
    requiring it such as: Windows, iOS and Android.
    As such Windows ports no longer require winmain.c. 
Changed dw_timer_connnect() and dw_timer_disconnect() to use HTIMER.
    This allows newer ports to use object handles that won't fit in
    what had been an integer reference.  OS/2 will use its native 
    HTIMER and other existing platforms will continue to use "int"
    for compatibility. Other platforms may change in the future.
Changed dw_exec() with DW_EXEC_CON will now open the Terminal.app 
    on Mac and the system (Gnome) terminal on Unix via GLib's 
    G_APP_INFO_CREATE_NEEDS_TERMINAL instead of launching an xterm.
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
Fixed building and several issues related to MacOS 10.5 now that I
    have a PowerMac G5 running 10.5.8 to test on.
Fixed a bug in dw_listbox_set_text() on GTK3.
Fixed sockpipe() functionality with gcc on OS/2.
Ongoing work to have a more consistent code style across platforms.
    See the accompanying style.txt file for more information.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
