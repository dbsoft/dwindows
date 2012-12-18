This package contains Dynamic Windows (dwindows) v 2.5 for OS/2 and eCS.

It contains:

dw.dll - main dwindows DLL
dwcompat.dll - compatibility DLL (optional)
dw.h - header file
dw.lib - import library for functions in dw.dll
dwcompat.lib - import library for functions in dwcompat.dll
readme-os2.txt - this file

This binary package was compiled with gcc 4.4.4.
It requires libc065.dll available at ftp://ftp.netlabs.org/pub/libc

Dynamic Windows may take advantage of features from these packages: 

XCenter/eCenter with the System Tray plugin
http://www.eros2.info/systray_widget_en.shtml

Generalized Bitmap Module - For additional image formats
http://heikon.home.tlink.de/

-- Special notes for version 2.5 --
Version 2.4 has started a transition to Unicode on OS/2. Warp 4 and 
later have fairly decent support for UTF-8 via codepage 1208, however 
there are some known input problems, which shall be worked around
in future versions.  For English or non-input applications Unicode
is the recommended version, however for applications that have not 
been converted to UTF-8 or which are experiencing input problems,
a non-Unicode version of the DLL is included. 
