This is a stable release of Dynamic Windows version 3.4.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD, OpenSolaris and iOS.
The source base is considered beta on Android, alpha on C++.

Build Recommendations:
MacOS:
    11-13: configure --with-arch=modern --with-minver=10.15
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
    2000: Visual Studio 2005. Should run on 2000 and later,
        no Aero, no Notifications, no WebView2 and oldsockpipe()
        on all versions.
C++: Recommends a C++11 compatible compiler.
    MacOS: PowerPC GCC 6 from Tiger Brew.
           Intel Apple Clang from Xcode 4.4 or later.
           Apple Silicon any supported Apple Clang.
    Windows: Visual Studio 2015, recent Clang-cl or MingW32.
    Linux/FreeBSD: GCC 5 or Clang 3.3 recommended.
    OS/2: GCC 9.2 from Bitwise Works recommended.

    If you build with a pre-C++11 compiler features will be 
    disabled, and you may ended up building an extremely 
    simplified sample application instead of the full one.

Known problems:

Boxes with no expandable items will have their contents centered on 
    GTK2 instead of top or left justified on the other platforms.
    Pack an expandable DW_NOHWND item at the end of the box to keep
    the same appearance as other platforms. 
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

Some widget may not be completely created when the widget handle is 
returned.  Some need to be setup such as Container controls on GTK, 
and others need to be packed before they are finalized.  Once setup
and packed it is completely safe to operate on widgets.  If you choose
to operate on widgets before setup and/or packing, then it depends 
on the platform if it will work or not.

Changes from version 3.3:
Pushed up the Android requirements to Android 8 (API 26 Oreo).
Added dw_html_javascript_add() and DW_SIGNAL_HTML_MESSAGE.
    This allows HTML embedded javascript to call into native code.
    Supported on Windows, Mac, GTK3/4, iOS and Android.
    Windows 7+ with Edge WebView2. MacOS 10.10+.
    GTK3/4 with WebKitGTK 2 or higher.
    iOS and Android, all supported versions.
Added DW_FEATURE_RENDER_SAFE that requires safe rendering.
    This means only allowing drawing in the EXPOSE callback.
    On Android this also enables off main thread expose events.
    Added high and low priority event queues on Android and
    increased the queue length. This should prevent important
    events from being dropped, only superfluous expose events.
    This feature is enabled by default on Android.
    This feature is disabled by default or unavailable on others.
Added support for MacOS 14 Sonoma and iOS 17.

Dynamic Windows Documentation is available at:

https://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
