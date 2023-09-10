AC_INIT([dwindows],[3.3])
AC_CONFIG_HEADERS([dwconfig.h])

TARGET="dwindows"
TARGET2="dwcompat"
AC_SUBST(TARGET)
AC_SUBST(TARGET2)

AC_DEFUN([AC_DATAROOTDIR_CHECKED])

DW_DIR=gtk3
DW_SRC=dw.c
DW_DEFINE=__UNIX__
SHAREDFLAG=-shared
SOSUFFIX=so
LIBSUFFIX=a
LIBPREFIX=lib
INSTALL_TEST="installdwtest"
INCLUDES="-I."

AC_PROG_CC
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(11,,optional)

AC_CHECK_PROG(MAKE, gmake, gmake)
if test x"$MAKE" = x; then
   AC_CHECK_PROG(MAKE, make, make)
fi
if test -d .svn; then 
   AC_CHECK_PROG(SVNVERSION, svnversion, svnversion)
fi
if test x"$SVNVERSION" = x; then
   SVNVERSION=0
else
   SVNVERSION="\` $SVNVERSION -n . | cut -d \":\" -f 1 | tr -d MS \`"
fi

AC_PROG_INSTALL

AC_SUBST(MAKE)

dnl Checks for system
AC_CANONICAL_TARGET

dnl ---------------------- check for '--with-dwcompat' switch -----------------
AC_ARG_WITH(dwcompat,
   [  --with-dwcompat         specify this to build the optional dwcompat library],
   [with_dwcompat=$withval],
   [with_dwcompat=no],
)

dnl ---------------------- check for '--with-gtk2' switch -----------------
AC_ARG_WITH(gtk2,
   [  --with-gtk2             specify this to build with GTK+ 2.x if available],
   [with_gtk2=$withval],
   [with_gtk2=no],
)

dnl ---------------------- check for '--with-gtk4' switch -----------------
AC_ARG_WITH(gtk4,
   [  --with-gtk4             specify this to build with GTK+ 4.x if available],
   [with_gtk4=$withval],
   [with_gtk4=no],
)

dnl ---------------------- check for '--with-deprecated' switch -----------------
AC_ARG_WITH(deprecated,
   [  --with-deprecated       specify this to build with deprecated functions],
   [with_deprecated=$withval],
   [with_deprecated=no],
)

dnl ---------------------- check for '--with-arch' switch -----------------
AC_ARG_WITH(arch,
   [  --with-arch             specify MacOS architecture: one of modern, classic, 32bit, intel, arm, powerpc],
   [with_arch=$withval],
   [with_arch=no],
)

dnl ---------------------- check for '--with-sdk' switch -----------------
AC_ARG_WITH(sdk,
   [  --with-sdk              specify this to build with an alternate MacOS SDK location],
   [with_sdk=$withval],
   [with_sdk=no],
)

dnl ---------------------- check for '--with-minver' switch -----------------
AC_ARG_WITH(minver,
   [  --with-minver           specify this to build with a minimum MacOS version],
   [with_minver=$withval],
   [with_minver=no],
)

AC_HEADER_DIRENT
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(sys/stat.h)

AC_CHECK_FUNCS(pipe, AC_DEFINE(HAVE_PIPE,1,Determine whether we have the pipe function))
AC_CHECK_FUNCS(vsnprintf, AC_DEFINE(HAVE_VSNPRINTF,1,Determine whether we have the vsnprintf function))

AC_CHECK_FUNCS(connect)
if test x"$ac_cv_func_connect" = x"no"; then
   if test -z "$libsocket"; then
      AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket",)
   fi

   dnl this is for isc.  need the nsl_s library as well.
   if test -z "$libinet"; then
      AC_CHECK_LIB(inet, socket, libnsl=1; LIBS="$LIBS -linet -lnsl_s",)
   fi

   if test -z "$libnsl"; then
      AC_CHECK_LIB(nsl, gethostname, LIBS="$LIBS -lnsl",)
   fi
fi

AC_CHECK_LIB(sun, getpwnam, LIBS="$LIBS -lsun",)
AC_CHECK_LIB(dgc, inet_addr, LIBS="$LIBS -ldgc",)
AC_CHECK_LIB(resolv, gethostbyname, LIBS="$LIBS -lresolv",)
AC_CHECK_LIB(m, atan2, LIBS="$LIBS -lm",)

build_gtk="yes"
check_pthreads="yes"
PLATLIBS=""
SONAME=""
PLATCCFLAGS="-g -O2 -fPIC -Wall"
ARFLAGS="cqs"
ARCH=""

case "$target" in
   *linux*)
      SONAME="-Wl,-soname,lib$TARGET.so.\$(DW_MAJOR_VERSION)"
   ;;
   *apple-darwin*)
      case "$with_arch" in
         no)
            ARCH=""
            ;;
         classic)
            ARCH="-arch ppc -arch x86_64 -arch i386"
            ;;
         32bit)
            ARCH="-arch ppc -arch i386"
            ;;
         intel)
            ARCH="-arch i386 -arch x86_64"
            ;;
         powerpc)
            ARCH="-arch ppc -arch ppc64"
            ;;
         arm)
            ARCH="-arch arm64"
            ;;
         modern)
            ARCH="-arch x86_64 -arch arm64"
            ;;
         *)
            ARCH="-arch $with_arch"
            ;;
      esac
      case "$with_sdk" in
         no)
            ;;
         *)
            CC="$CC -isysroot$with_sdk"
            ;;
      esac
      case "$with_minver" in
         no)
            ;;
         *)
            CFLAGS="$CFLAGS -mmacosx-version-min=$with_minver"
            LIBS="$LIBS -mmacosx-version-min=$with_minver"
            ;;
      esac
      SHAREDFLAG="-dynamiclib -flat_namespace -undefined suppress -headerpad_max_install_names"
      SOSUFFIX=dylib
      DW_DIR=mac
      DW_SRC=dw.m
      DW_DEFINE=__MAC__
      LIBS="$LIBS -framework Cocoa -framework WebKit"
      save_libs="$LIBS"
      LIBS="$LIBS -weak_framework UserNotifications"
      INSTALL_TEST=""
      AC_CHECK_FUNC(exit, , [LIBS="$save_libs"])
      build_gtk="no"
   ;;
   *)
   ;;
esac

dnl ---------------------- default targets to build -----------------
if test $with_dwcompat = yes; then
   COMPAT_OBJECT="dwcompat.o"
   INSTALL_COMPAT="installdwcompat"
   SYSCONF_LINK_TARGET_SHARED2="lib$TARGET2.$SOSUFFIX.\$(DW_MAJOR_VERSION).\$(DW_MINOR_VERSION)"
else
   COMPAT_OBJECT=""
   INSTALL_COMPAT=""
   SYSCONF_LINK_TARGET_SHARED2=""
fi
SYSCONF_LINK_TARGET_SHARED="lib$TARGET.$SOSUFFIX.\$(DW_MAJOR_VERSION).\$(DW_MINOR_VERSION)"
SYSCONF_LINK_TARGET_STATIC="lib$TARGET.a"

TARGETS="$SYSCONF_LINK_TARGET_SHARED $SYSCONF_LINK_TARGET_SHARED2"

if test $build_gtk = "yes"; then
   AC_PATH_XTRA
   LIBS="$LIBS $X_LIBS -lX11"
   AC_CHECK_PROG(PKG_CFG, pkg-config, pkg-config)
   if test x"$PKG_CFG" != x; then
      # Figure out the best available GTK packages...
      # Preference order GTK3>GTK4>GTK2, WebKit2>WebKit1.1>WebKit1.0>WebKit
      GTK_LIBS=""
      GTK_PACKAGES=""
      WEBKIT_PKG=""
      WEBKIT_ALT_PKG=""      
      if test $with_gtk4 = "yes"; then
         # First try the X11 version so we can call Xlib directly 
         # to fill in the holes not available with Wayland
         GTK_PACKAGES="gtk4-x11"
         GTK_LIBS=`$PKG_CFG --silence-errors --libs $GTK_PACKAGES`
         if test x"$GTK_LIBS" = x; then 
            GTK_PACKAGES="gtk4"
            GTK_LIBS=`$PKG_CFG --silence-errors --libs $GTK_PACKAGES`
         fi
         if test x"$GTK_LIBS" != x; then 
            DW_DIR=gtk4
            # WebKit2GTK built for GTK4 becomes 5.0 or 6.0
            WEBKIT_PKG="webkitgtk-6.0"
            WEBKIT_ALT_PKG="webkit2gtk-5.0"
         fi
      else
         # Put the GTK2 test here since --with-gtk2 is mutually
         # exclusive with --with-gtk4
         if test $with_gtk2 = "yes"; then
            GTK_PACKAGES="gtk+-2.0 gthread-2.0"
            GTK_LIBS=`$PKG_CFG --silence-errors --libs $GTK_PACKAGES`
            if test x"$GTK_LIBS" != x; then 
               DW_DIR=gtk
               WEBKIT_PKG="webkit-1.1"
               WEBKIT_ALT_PKG="webkit-1.0"
            fi
         fi
      fi
      # We did not successfully find a GTK installation after
      # handling --with-gtk2 and --with-gtk4 so try GTK3
      if test x"$GTK_LIBS" = x; then
         # GTK3 Defaults section
         GTK_PACKAGES="gtk+-3.0"
         GTK_LIBS=`$PKG_CFG --silence-errors --libs $GTK_PACKAGES`     
         WEBKIT_PKG="webkit2gtk-4.0"
         WEBKIT_ALT_PKG="webkitgtk-3.0"
      fi
      echo "checking for GTK version: $DW_DIR"
      # Test for the preferred WebKit package
      WEBKIT_LIBS=`$PKG_CFG --silence-errors --libs $WEBKIT_PKG`
      if test x"$WEBKIT_LIBS" = x; then
         # Preferred package is not available, try the alternate if defined
         if test x"$WEBKIT_ALT_PKG" != x; then
            WEBKIT_LIBS=`$PKG_CFG --silence-errors --libs $WEBKIT_ALT_PKG`
            if test x"$WEBKIT_LIBS" != x; then
               WEBKIT_PKG=$WEBKIT_ALT_PKG
            fi
         fi
      fi
      echo "checking for WebKit package: $WEBKIT_PKG"
      WEBKIT_CFLAGS=`$PKG_CFG --silence-errors --cflags $WEBKIT_PKG`
      # If we got a WebKit package that might work... 
      # Add defintitions necessary for the correct code paths      
      if test x"$WEBKIT_LIBS" != x; then
         if test "$WEBKIT_PKG" = "webkit-1.0"; then
            AC_DEFINE(USE_WEBKIT10,1,Uses WebKit 1.0)
         fi
         if test "$WEBKIT_PKG" = "webkit-1.1"; then
            AC_DEFINE(USE_WEBKIT11,1,Uses WebKit 1.1)
         fi
         if test "$WEBKIT_PKG" = "webkit2gtk-4.0"; then
            AC_DEFINE(USE_WEBKIT2,1,Uses WebKit 2)
         fi
         if test "$WEBKIT_PKG" = "webkitgtk-6.0"; then
            AC_DEFINE(USE_WEBKIT6,1,Uses WebKit 6)
         fi
      fi
      if test x"$RPATH" != x; then
         RPATH="-Wl,-R$RPATH"
      fi
   fi

   GTK_CFLAGS=`$PKG_CFG --cflags $GTK_PACKAGES`
   GTK_LIBS=`$PKG_CFG --libs $GTK_PACKAGES`
   PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32.0], [DW_GRESOURCE="enabled"], [CFLAGS="$CFLAGS -DDW_INCLUDE_DEPRECATED_RESOURCES=1"])
fi

if test $check_pthreads = "yes"; then
   AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([pthread header not found. Dynamic Windows cannot build without it.]))
   AC_CHECK_LIB(pthread,
                pthread_attr_init,
                PTHREAD_LIBS="-lpthread",
                AC_CHECK_LIB(pthreads,
                             pthread_attr_init,
                             PTHREAD_LIBS="-lpthreads",
                             AC_CHECK_LIB(c_r,
                                          pthread_attr_init,
                                          PTHREAD_LIBS="-lc_r",
                                          AC_CHECK_LIB(c,
                                                       pthread_attr_init,
                                                       PTHREAD_LIBS="-lc",
                                                       AC_MSG_ERROR(pthread library not found. dwindows cannot build without it.)
                                                      )
                                         )
                            )
               )
fi

CFLAGS="$CFLAGS $GTK_CFLAGS $GDK_IMLIB_FLAGS $WEBKIT_CFLAGS"

if test $with_deprecated = yes; then
   CFLAGS="$CFLAGS -DDW_INCLUDE_DEPRECATED"
fi
if test x"$WEBKIT_LIBS" != x; then
   WEBKIT_LIB=`$PKG_CFG --silence-errors --libs-only-l $WEBKIT_PKG | cut -b 3-`
   AC_CHECK_LIB($WEBKIT_LIB,webkit_web_view_new,AC_DEFINE(USE_WEBKIT,1,Uses some flavor of WebKit),)
fi

LIBS="$RPATH $LIBS $GTK_LIBS $PTHREAD_LIBS $GDK_IMLIB_LIBS $WEBKIT_LIBS $PLATLIBS"

AC_CHECK_FUNCS(ubuntu_overlay_scrollbar_set_enabled, AC_DEFINE(HAVE_OVERLAY_SCROLLBARS,1,Define if we have Ubuntu overlay scrollbars))
AC_CHECK_FUNCS(ubuntu_gtk_set_use_overlay_scrollbar, AC_DEFINE(HAVE_OVERLAY_SCROLLBARS2,1,Define if we have Ubuntu overlay scrollbars))

RM="rm -f"
LN="ln -s"
CP="cp"
MV="mv"
MKDIR="mkdir"

AC_SUBST(SYSCONF_LINK_TARGET_SHARED)
AC_SUBST(SYSCONF_LINK_TARGET_SHARED2)
AC_SUBST(SYSCONF_LINK_TARGET_STATIC)

AC_SUBST(INCLUDES)
AC_SUBST(PLATCCFLAGS)
AC_SUBST(ARFLAGS)
AC_SUBST(RM)
AC_SUBST(LN)
AC_SUBST(CP)
AC_SUBST(MV)
AC_SUBST(MKDIR)
AC_SUBST(SHAREDFLAG)
AC_SUBST(SOSUFFIX)
AC_SUBST(LIBSUFFIX)
AC_SUBST(LIBPREFIX)
AC_SUBST(SONAME)
AC_SUBST(TARGETS)

AC_SUBST(DW_SRC)
AC_SUBST(DW_DIR)
AC_SUBST(DW_DEFINE)
AC_SUBST(DW_GRESOURCE)
AC_SUBST(BROWSER_OBJECT)
AC_SUBST(COMPAT_OBJECT)
AC_SUBST(INSTALL_COMPAT)
AC_SUBST(INSTALL_TEST)

AC_SUBST(ARCH)

AC_CONFIG_FILES([
Makefile
dwindows1.pc
dwindows-config1
org.dbsoft.dwindows.dwtest.desktop ])
AC_OUTPUT
