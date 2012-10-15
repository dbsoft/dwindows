This is a pre-release of Dynamic Windows version 2.5.

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

Changes from version 2.4:
Added dw_window_set_focus() to focus a widget after the window is shown.
Added DW_FCF_TEXTURED flag on Mac, which enables textured backgrounds
   which had been the default on Mac prior to 2.5.
Added keyboard support for non-entryfield controls on Mac.
Added tab support for notebook controls on Windows and OS/2 and in 
   the process rewrote and optimized the existing tab code.
Fixed tab support for bitmap buttons which broke in 2.4 on Windows.
Fixed a notebook crash early in creation on Mac.
Fixed unusable scrollbars on Ubuntu Linux when overlay scrollbars
   are enabled.  We now disable overlay scrollbars when creating.
Fixed dw_window_function() not working on non-toplevel windows on
   Windows and OS/2.

Dynamic Windows Documentation is available at:

http://dbsoft.org/dw_help.php

If you have any questions or suggestions feel free to email me at:

brian@dbsoft.org

Thanks!

Brian Smith
