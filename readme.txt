This is a pre-release of Dynamic Windows version 2.4.

The current Dynamic Windows source base is considered stable on:
OS/2, Mac, Windows, Linux, FreeBSD and Solaris.

Known problems:

Boxes with no expandable items will have their contents centered on 
    GTK2 instead of top or left justified on the other platforms.
GTK3 due to changes in the core architecture does not support
    widgets that are smaller than what is contained within them
    unless they use scrolled windows. GTK2 and other platforms do.

Known limitations:

It is not safe on all platforms to operate on widgets before they
are packed.  For portability pack widgets before operating on them.

Future features:

OS/2 is currently missing the HTML widget because the system does 
not support it by default. Looking into importing functionality 
from available libraries (Firefox, Webkit, Qt, etc).

Changes from version 2.3:
Added fullscreen support on Mac for resizable windows.
Added UNICODE build mode on Windows allowing UTF-8 encoded text.
   ANSI builds are supported by removing -DUNICODE -D_UNICODE and -DAEROGLASS
Added support for antialiased drawing on Windows via GDI+.
Added codepage 1208 (UTF-8) as the default codepage on OS/2.
Added support for Control-Click on Mac for button press events.
Added DW_POINTER() macro for casting parameters to (void *).
Added dw_box_remove() and dw_box_remove_at_index() for removing items
   from boxes without destroying them. Also allows removal of padding.
Fixed dwindows-config --version not returning the version at all.
Fixed value changed events not working for spinbuttons on OS/2 and Windows.
Fixed issues drawing arcs on GTK2, GTK3 and Mac.
Fixed a crash in the color chooser on Mac running Lion.
Fixed a layout issue with render widgets on OS/2.
Fixed an expose event issue on OS/2.
Fixed an issue with GTK 3.4 due to properties being inherited from the parent.
Fixed issues with bitmap buttons using icon/pointers on OS/2 and Windows.
Fixed an issue with dw_window_destroy() on Mac.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
