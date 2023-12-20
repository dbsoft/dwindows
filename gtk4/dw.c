/*
 * Dynamic Windows:
 *          A GTK like cross-platform GUI
 *          GTK4 forwarder module for portabilty.
 *
 * (C) 2000-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2003-2022 Mark Hessling <mark@rexx.org>
 */
#include "dwconfig.h"
#include "dw.h"
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#ifdef USE_WEBKIT
#ifdef USE_WEBKIT6
#include <webkit/webkit.h>
#else
#include <webkit2/webkit2.h>
#endif
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

/* Macros to encapsulate running functions on the main thread
 * #define _DW_SINGLE_THREADED to disable thread safety encapsulation.
 * Parameters converted to a pointer array: 
 * [0] Pointer to the thread's event semaphore
 * [1] Pointer to the function's return value
 * [2] Pointer to function parameter 1
 * ...
 */
#ifndef _DW_SINGLE_THREADED
#define DW_FUNCTION_DEFINITION(func, rettype, ...)  gboolean _##func(void **_args); \
rettype API func(__VA_ARGS__) { 
#define DW_FUNCTION_ADD_PARAM void **_args = alloca(sizeof(void *)*4); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[3] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM1(param1) void **_args = alloca(sizeof(void *)*5); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[4] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM2(param1, param2) void **_args = alloca(sizeof(void *)*6); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[5] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM3(param1, param2, param3) void **_args = alloca(sizeof(void *)*7); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[6] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM4(param1, param2, param3, param4) void **_args = alloca(sizeof(void *)*8); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[7] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM5(param1, param2, param3, param4, param5) void **_args = alloca(sizeof(void *)*9); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[8] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM6(param1, param2, param3, param4, param5, param6) \
    void **_args = alloca(sizeof(void *)*10); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[9] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM7(param1, param2, param3, param4, param5, param6, param7) \
    void **_args = alloca(sizeof(void *)*11); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[10] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM8(param1, param2, param3, param4, param5, param6, param7, param8) \
    void **_args = alloca(sizeof(void *)*12); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)&param8; \
    _args[10] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[11] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM9(param1, param2, param3, param4, param5, param6, param7, param8, param9) \
    void **_args = alloca(sizeof(void *)*13); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)&param8; \
    _args[10] = (void *)&param9; \
    _args[11] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[12] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM10(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10) \
    void **_args = alloca(sizeof(void *)*14); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)&param8; \
    _args[10] = (void *)&param9; \
    _args[11] = (void *)&param10; \
    _args[12] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[13] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM11(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11) \
    void **_args = alloca(sizeof(void *)*15); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)&param8; \
    _args[10] = (void *)&param9; \
    _args[11] = (void *)&param10; \
    _args[12] = (void *)&param11; \
    _args[13] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[14] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_ADD_PARAM12(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12) \
    void **_args = alloca(sizeof(void *)*16); \
    _args[0] = (void *)pthread_getspecific(_dw_event_key); \
    _args[1] = (void *)NULL; \
    _args[2] = (void *)&param1; \
    _args[3] = (void *)&param2; \
    _args[4] = (void *)&param3; \
    _args[5] = (void *)&param4; \
    _args[6] = (void *)&param5; \
    _args[7] = (void *)&param6; \
    _args[8] = (void *)&param7; \
    _args[9] = (void *)&param8; \
    _args[10] = (void *)&param9; \
    _args[11] = (void *)&param10; \
    _args[12] = (void *)&param11; \
    _args[13] = (void *)&param12; \
    _args[14] = (void *)pthread_getspecific(_dw_fg_color_key); \
    _args[15] = (void *)pthread_getspecific(_dw_bg_color_key);
#define DW_FUNCTION_RESTORE_PARAM1(param1, vartype1) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[3]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[4];
#define DW_FUNCTION_RESTORE_PARAM2(param1, vartype1, param2, vartype2) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[4]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[5];
#define DW_FUNCTION_RESTORE_PARAM3(param1, vartype1, param2, vartype2, param3, vartype3) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[5]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[6];
#define DW_FUNCTION_RESTORE_PARAM4(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[6]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[7];
#define DW_FUNCTION_RESTORE_PARAM5(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[7]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[8];
#define DW_FUNCTION_RESTORE_PARAM6(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[8]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[9];
#define DW_FUNCTION_RESTORE_PARAM7(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[9]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[10];
#define DW_FUNCTION_RESTORE_PARAM8(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    vartype8 param8 = *((vartype8 *)_args[9]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[10]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[11];
#define DW_FUNCTION_RESTORE_PARAM9(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    vartype8 param8 = *((vartype8 *)_args[9]); \
    vartype9 param9 = *((vartype9 *)_args[10]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[11]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[12];
#define DW_FUNCTION_RESTORE_PARAM10(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    vartype8 param8 = *((vartype8 *)_args[9]); \
    vartype9 param9 = *((vartype9 *)_args[10]); \
    vartype10 param10 = *((vartype10 *)_args[11]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[12]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[13];
#define DW_FUNCTION_RESTORE_PARAM11(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10, param11, vartype11) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    vartype8 param8 = *((vartype8 *)_args[9]); \
    vartype9 param9 = *((vartype9 *)_args[10]); \
    vartype10 param10 = *((vartype10 *)_args[11]); \
    vartype11 param11 = *((vartype11 *)_args[12]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[13]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[14];
#define DW_FUNCTION_RESTORE_PARAM12(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10, param11, vartype11, param12, vartype12) \
    vartype1 param1 = *((vartype1 *)_args[2]); \
    vartype2 param2 = *((vartype2 *)_args[3]); \
    vartype3 param3 = *((vartype3 *)_args[4]); \
    vartype4 param4 = *((vartype4 *)_args[5]); \
    vartype5 param5 = *((vartype5 *)_args[6]); \
    vartype6 param6 = *((vartype6 *)_args[7]); \
    vartype7 param7 = *((vartype7 *)_args[8]); \
    vartype8 param8 = *((vartype8 *)_args[9]); \
    vartype9 param9 = *((vartype9 *)_args[10]); \
    vartype10 param10 = *((vartype10 *)_args[11]); \
    vartype11 param11 = *((vartype11 *)_args[12]); \
    vartype12 param12 = *((vartype12 *)_args[13]); \
    GdkRGBA * DW_UNUSED(_dw_fg_color) = (GdkRGBA *)_args[14]; \
    GdkRGBA * DW_UNUSED(_dw_bg_color) = (GdkRGBA *)_args[15];
#define DW_FUNCTION_END }
#define DW_FUNCTION_NO_RETURN(func) dw_event_reset((HEV)_args[0]); \
    if(_dw_thread == (pthread_t)-1 || pthread_self() == _dw_thread) \
        _##func(_args); \
    else { \
        g_idle_add_full(G_PRIORITY_HIGH_IDLE, G_SOURCE_FUNC(_##func), (gpointer)_args, NULL); \
        dw_event_wait((HEV)_args[0], DW_TIMEOUT_INFINITE); } \
    }\
gboolean _##func(void **_args) {
#define DW_FUNCTION_RETURN(func, rettype) dw_event_reset((HEV)_args[0]); \
    if(_dw_thread == (pthread_t)-1 || pthread_self() == _dw_thread) \
        _##func(_args); \
    else { \
        g_idle_add_full(G_PRIORITY_HIGH_IDLE, G_SOURCE_FUNC(_##func), (gpointer)_args, NULL); \
        dw_event_wait((HEV)_args[0], DW_TIMEOUT_INFINITE); } { \
        void *tmp = _args[1]; \
        rettype myreturn = *((rettype *)tmp); \
        free(tmp); \
        return myreturn; } \
    } \
gboolean _##func(void **_args) {
#define DW_FUNCTION_RETURN_THIS(_retvar) { void *_myreturn = malloc(sizeof(_retvar)); \
    memcpy(_myreturn, (void *)&_retvar, sizeof(_retvar)); \
    _args[1] = _myreturn; } \
    dw_event_post((HEV)_args[0]); \
    return FALSE; }
#define DW_FUNCTION_RETURN_NOTHING dw_event_post((HEV)_args[0]); \
    return FALSE; }
#else
#define DW_FUNCTION_DEFINITION(func, rettype, ...) rettype API func(__VA_ARGS__)
#define DW_FUNCTION_ADD_PARAM1(param1)
#define DW_FUNCTION_ADD_PARAM2(param1, param2)
#define DW_FUNCTION_ADD_PARAM3(param1, param2, param3)
#define DW_FUNCTION_ADD_PARAM4(param1, param2, param3, param4)
#define DW_FUNCTION_ADD_PARAM5(param1, param2, param3, param4, param5)
#define DW_FUNCTION_ADD_PARAM6(param1, param2, param3, param4, param5, param6)
#define DW_FUNCTION_ADD_PARAM7(param1, param2, param3, param4, param5, param6, param7)
#define DW_FUNCTION_ADD_PARAM8(param1, param2, param3, param4, param5, param6, param7, param8)
#define DW_FUNCTION_ADD_PARAM9(param1, param2, param3, param4, param5, param6, param7, param8, param9)
#define DW_FUNCTION_ADD_PARAM10(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10)
#define DW_FUNCTION_ADD_PARAM11(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11)
#define DW_FUNCTION_ADD_PARAM12(param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12)
#define DW_FUNCTION_RESTORE_PARAM1(param1, vartype1)
#define DW_FUNCTION_RESTORE_PARAM2(param1, vartype1, param2, vartype2)
#define DW_FUNCTION_RESTORE_PARAM3(param1, vartype1, param2, vartype2, param3, vartype3)
#define DW_FUNCTION_RESTORE_PARAM4(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4)
#define DW_FUNCTION_RESTORE_PARAM5(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5)
#define DW_FUNCTION_RESTORE_PARAM6(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6)
#define DW_FUNCTION_RESTORE_PARAM7(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7)
#define DW_FUNCTION_RESTORE_PARAM8(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8)
#define DW_FUNCTION_RESTORE_PARAM9(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9)
#define DW_FUNCTION_RESTORE_PARAM10(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10)
#define DW_FUNCTION_RESTORE_PARAM11(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10, param11, vartype11)
#define DW_FUNCTION_RESTORE_PARAM12(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9, param10, vartype10, param11, vartype11, param12, vartype12)
#define DW_FUNCTION_END
#define DW_FUNCTION_NO_RETURN(func)
#define DW_FUNCTION_RETURN(func, rettype)
#define DW_FUNCTION_RETURN_THIS(retvar) return retvar;
#define DW_FUNCTION_RETURN_NOTHING
#endif

/* ff = 255 = 1.0000
 * ee = 238 = 0.9333
 * cc = 204 = 0.8000
 * bb = 187 = 0.7333
 * aa = 170 = 0.6667
 * 77 = 119 = 0.4667
 * 00 = 0   = 0.0000
 */
GdkRGBA _dw_colors[] =
{
   { 0.0000, 0.0000, 0.0000, 1.0 },   /* 0  black */
   { 0.7333, 0.0000, 0.0000, 1.0 },   /* 1  red */
   { 0.0000, 0.7333, 0.0000, 1.0 },   /* 2  green */
   { 0.6667, 0.6667, 0.0000, 1.0 },   /* 3  yellow */
   { 0.0000, 0.0000, 0.8000, 1.0 },   /* 4  blue */
   { 0.7333, 0.0000, 0.7333, 1.0 },   /* 5  magenta */
   { 0.0000, 0.7333, 0.7333, 1.0 },   /* 6  cyan */
   { 0.7333, 0.7333, 0.7333, 1.0 },   /* 7  white */
   { 0.4667, 0.4667, 0.4667, 1.0 },   /* 8  grey */
   { 1.0000, 0.0000, 0.0000, 1.0 },   /* 9  bright red */
   { 0.0000, 1.0000, 0.0000, 1.0 },   /* 10 bright green */
   { 0.9333, 0.9333, 0.0000, 1.0 },   /* 11 bright yellow */
   { 0.0000, 0.0000, 1.0000, 1.0 },   /* 12 bright blue */
   { 1.0000, 0.0000, 1.0000, 1.0 },   /* 13 bright magenta */
   { 0.0000, 0.9333, 0.9333, 1.0 },   /* 14 bright cyan */
   { 1.0000, 1.0000, 1.0000, 1.0 },   /* 15 bright white */
};

/*
 * List those icons that have transparency first
 */
static char *_dw_image_exts[] =
{
   ".xpm",
   ".png",
   ".ico",
   ".icns",
   ".gif",
   ".jpg",
   ".jpeg",
   ".tiff",
   ".bmp",
   NULL
};

#ifndef max
# define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
# define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif

static pthread_key_t _dw_fg_color_key;
static pthread_key_t _dw_bg_color_key;
static pthread_key_t _dw_event_key;

static pthread_t _dw_thread = (pthread_t)-1;

static GList *_dw_dirty_list = NULL;

#define _DW_TREE_TYPE_CONTAINER  1
#define _DW_TREE_TYPE_TREE       2
#define _DW_TREE_TYPE_LISTBOX    3
#define _DW_TREE_TYPE_COMBOBOX   4

#define _DW_RESOURCE_PATH "/org/dbsoft/dwindows/resources/"

/* Signal forwarder prototypes */
static gint _dw_button_press_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data);
static gint _dw_button_release_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data);
static gint _dw_motion_notify_event(GtkEventControllerMotion *controller, double x, double y, gpointer data);
static gboolean _dw_delete_event(GtkWidget *window, gpointer data);
static gint _dw_key_press_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
static gint _dw_generic_event(GtkWidget *widget, gpointer data);
static gint _dw_configure_event(GtkWidget *widget, int width, int height, gpointer data);
static gint _dw_container_enter_event(GtkEventController *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
static gint _dw_combobox_select_event(GtkWidget *widget, gpointer data);
static gint _dw_expose_event(GtkWidget *widget, cairo_t *cr, int width, int height, gpointer data);
static void _dw_set_focus_event(GObject *window, GParamSpec *pspec, gpointer data);
static gint _dw_tree_context_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data);
static gint _dw_value_changed_event(GtkWidget *widget, gpointer user_data);
static gint _dw_tree_select_event(GtkTreeSelection *sel, gpointer data);
static gint _dw_tree_expand_event(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, gpointer data);
static gint _dw_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
static gint _dw_column_click_event(GtkWidget *widget, gpointer data);
#ifdef USE_WEBKIT
static void _dw_html_result_event(GObject *object, GAsyncResult *result, gpointer script_data);
static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitLoadEvent load_event, gpointer data);
#ifdef USE_WEBKIT6
static void _dw_html_message_event(WebKitUserContentManager *manager, JSCValue *result, gpointer *data);
#else
static void _dw_html_message_event(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer *data);
#endif
#endif
static void _dw_signal_disconnect(gpointer data, GClosure *closure);
static void _dw_event_coordinates_to_window(GtkWidget *widget, double *x, double *y);

GObject *_DWObject = NULL;
GApplication *_DWApp = NULL;
GMainLoop *_DWMainLoop = NULL;
static char _dw_app_id[_DW_APP_ID_SIZE+1] = { 0 };
char *_DWDefaultFont = NULL;
static char _dw_share_path[PATH_MAX+1] = { 0 };
static long _dw_mouse_last_x = 0;
static long _dw_mouse_last_y = 0;

typedef struct _dw_signal_list
{
   void *func;
   char name[30];
   char gname[30];
   GObject *(*setup)(struct _dw_signal_list *, GObject *, void *, void *, void *);

} DWSignalList;

/* Signal setup function prototypes */
GObject *_dw_key_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_button_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_mouse_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_motion_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_draw_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_value_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_tree_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
GObject *_dw_focus_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
#ifdef USE_WEBKIT
GObject *_dw_html_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data);
#endif

typedef struct
{
   HWND window;
   void *func;
   gpointer data;
   gint cid;
   void *intfunc;

} DWSignalHandler;

/* A list of signal forwarders, to account for parameter differences. */
static DWSignalList DWSignalTranslate[] = {
   { _dw_configure_event,         DW_SIGNAL_CONFIGURE,      "resize",            NULL },
   { _dw_key_press_event,         DW_SIGNAL_KEY_PRESS,      "key-pressed",       _dw_key_setup },
   { _dw_button_press_event,      DW_SIGNAL_BUTTON_PRESS,   "pressed",           _dw_mouse_setup },
   { _dw_button_release_event,    DW_SIGNAL_BUTTON_RELEASE, "released",          _dw_mouse_setup },
   { _dw_motion_notify_event,     DW_SIGNAL_MOTION_NOTIFY,  "motion",            _dw_motion_setup },
   { _dw_delete_event,            DW_SIGNAL_DELETE,         "close-request",     NULL },
   { _dw_expose_event,            DW_SIGNAL_EXPOSE,         "draw",              _dw_draw_setup },
   { _dw_generic_event,           DW_SIGNAL_CLICKED,        "clicked",           _dw_button_setup },
   { _dw_container_enter_event,   DW_SIGNAL_ITEM_ENTER,     "key-pressed",       _dw_key_setup },
   { _dw_tree_context_event,      DW_SIGNAL_ITEM_CONTEXT,   "pressed",           _dw_tree_setup },
   { _dw_combobox_select_event,   DW_SIGNAL_LIST_SELECT,    "changed",           NULL },
   { _dw_tree_select_event,       DW_SIGNAL_ITEM_SELECT,    "changed",           _dw_tree_setup },
   { _dw_set_focus_event,         DW_SIGNAL_SET_FOCUS,      "notify::is-active", _dw_focus_setup },
   { _dw_value_changed_event,     DW_SIGNAL_VALUE_CHANGED,  "value-changed",     _dw_value_setup },
   { _dw_switch_page_event,       DW_SIGNAL_SWITCH_PAGE,    "switch-page",       NULL },
   { _dw_column_click_event,      DW_SIGNAL_COLUMN_CLICK,   "activate",          _dw_tree_setup },
   { _dw_tree_expand_event,       DW_SIGNAL_TREE_EXPAND,    "row-expanded",      NULL },
#ifdef USE_WEBKIT
   { _dw_html_changed_event,      DW_SIGNAL_HTML_CHANGED,    "load-changed",     NULL },
   { _dw_html_result_event,       DW_SIGNAL_HTML_RESULT,     "",                 _dw_html_setup },
   { _dw_html_message_event,      DW_SIGNAL_HTML_MESSAGE,    "",                 _dw_html_setup },
#endif
   { NULL,                        "",                        "",                 NULL }
};

/* Alignment flags */
#define DW_CENTER 0.5f
#define DW_LEFT 0.0f
#define DW_RIGHT 1.0f
#define DW_TOP 0.0f
#define DW_BOTTOM 1.0f

static void _dw_msleep(long period)
{
#ifdef __sun__
   /* usleep() isn't threadsafe on Solaris */
   struct timespec req;

   req.tv_sec = 0;
   if(period >= 1000)
   {
      req.tv_sec = (int)(period / 1000);
      period -= (req.tv_sec * 1000);
   }
   req.tv_nsec = period * 10000000;
	
   nanosleep(&req, NULL);
#else
   usleep(period * 1000);
#endif
}

/* Finds the translation function for a given signal name */
static DWSignalList _dw_findsignal(const char *signame)
{
   int z=0;
   static DWSignalList empty = {0};

   while(DWSignalTranslate[z].func)
   {
      if(strcasecmp(signame, DWSignalTranslate[z].name) == 0)
         return DWSignalTranslate[z];
      z++;
   }
   return empty;
}

static DWSignalHandler _dw_get_signal_handler(gpointer data)
{
   DWSignalHandler sh = {0};

   if(data)
   {
      void **params = (void **)data;
      int counter = GPOINTER_TO_INT(params[0]);
      GtkWidget *widget = (GtkWidget *)params[2];
      char text[101] = {0};

      snprintf(text, 100, "_dw_sigwindow%d", counter);
      sh.window = (HWND)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigfunc%d", counter);
      sh.func = (void *)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_intfunc%d", counter);
      sh.intfunc = (void *)g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigdata%d", counter);
      sh.data = g_object_get_data(G_OBJECT(widget), text);
      snprintf(text, 100, "_dw_sigcid%d", counter);
      sh.cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));
   }
   return sh;
}

static void _dw_remove_signal_handler(GtkWidget *widget, int counter)
{
   char text[101] = {0};
   gint cid;

   snprintf(text, 100, "_dw_sigcid%d", counter);
   cid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), text));
   if(cid > 0)
      g_signal_handler_disconnect(G_OBJECT(widget), cid);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigwindow%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_intfunc%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
   snprintf(text, 100, "_dw_sigdata%d", counter);
   g_object_set_data(G_OBJECT(widget), text, NULL);
}

static int _dw_set_signal_handler(GObject *object, HWND window, void *func, gpointer data, void *intfunc, void *discfunc)
{
   int counter = GPOINTER_TO_INT(g_object_get_data(object, "_dw_sigcounter"));
   char text[101] = {0};

   snprintf(text, 100, "_dw_sigwindow%d", counter);
   g_object_set_data(object, text, (gpointer)window);
   snprintf(text, 100, "_dw_sigfunc%d", counter);
   g_object_set_data(object, text, (gpointer)func);
   snprintf(text, 100, "_dw_intfunc%d", counter);
   g_object_set_data(object, text, (gpointer)intfunc);
   snprintf(text, 100, "_dw_discfunc%d", counter);
   g_object_set_data(object, text, (gpointer)discfunc);
   snprintf(text, 100, "_dw_sigdata%d", counter);
   g_object_set_data(object, text, (gpointer)data);

   counter++;
   g_object_set_data(object, "_dw_sigcounter", GINT_TO_POINTER(counter));

   return counter - 1;
}

static void _dw_set_signal_handler_id(GObject *object, int counter, gint cid)
{
   if(cid > 0)
   {
      char text[101] = {0};

      snprintf(text, 100, "_dw_sigcid%d", counter);
      g_object_set_data(object, text, GINT_TO_POINTER(cid));
   }
   else
      dw_debug("WARNING: Dynamic Windows failed to connect signal.\n");
}

#ifdef USE_WEBKIT
static void _dw_html_result_event(GObject *object, GAsyncResult *result, gpointer script_data)
{
    pthread_t saved_thread = _dw_thread;
#ifndef USE_WEBKIT6
    WebKitJavascriptResult *js_result;
#endif
    JSCValue *value;
    GError *error = NULL;
    int (*htmlresultfunc)(HWND, int, char *, void *, void *) = NULL;
    gint handlerdata = GPOINTER_TO_INT(g_object_get_data(object, "_dw_html_result_id"));
    void *user_data = NULL;

    _dw_thread = (pthread_t)-1;
    if(handlerdata)
    {
        void *params[3] = { GINT_TO_POINTER(handlerdata-1), 0, object };
        DWSignalHandler work = _dw_get_signal_handler(params);

        if(work.window)
        {
            htmlresultfunc = work.func;
            user_data = work.data;
        }
    }

#ifdef USE_WEBKIT6
    if(!(value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error)))
#else
    if(!(js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error)))
#endif
    {
        if(htmlresultfunc)
           htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, error->message, script_data, user_data);
        g_error_free (error);
        _dw_thread = saved_thread;
        return;
    }

#ifndef USE_WEBKIT6
    value = webkit_javascript_result_get_js_value(js_result);
#endif
    if(jsc_value_is_string(value))
    {
        gchar *str_value = jsc_value_to_string(value);
        JSCException *exception = jsc_context_get_exception(jsc_value_get_context(value));

        if(htmlresultfunc)
        {
           if(exception)
               htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, (char *)jsc_exception_get_message(exception), script_data, user_data);
           else
               htmlresultfunc((HWND)object, DW_ERROR_NONE, str_value, script_data, user_data);
        }
        g_free (str_value);
    }
    else if(htmlresultfunc)
        htmlresultfunc((HWND)object, DW_ERROR_UNKNOWN, NULL, script_data, user_data);
#ifndef USE_WEBKIT6
    webkit_javascript_result_unref (js_result);
#endif
   _dw_thread = saved_thread;
}

#ifdef USE_WEBKIT6
static void _dw_html_message_event(WebKitUserContentManager *manager, JSCValue *result, gpointer *data)
#else
static void _dw_html_message_event(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer *data)
#endif
{
    HWND window = (HWND)data[0];
    int (*htmlmessagefunc)(HWND, char *, char *, void *) = NULL;
    void *user_data = NULL;
    gchar *name = (gchar *)data[1];
    gint handlerdata;

    if(window && (handlerdata = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), "_dw_html_message_id"))))
    {
        void *params[3] = { GINT_TO_POINTER(handlerdata-1), 0, window };
        DWSignalHandler work = _dw_get_signal_handler(params);

        if(work.window)
        {
            htmlmessagefunc = work.func;
            user_data = work.data;
        }
    }

    if(jsc_value_is_string(result))
    {
        gchar *str_value = jsc_value_to_string(result);
        JSCException *exception = jsc_context_get_exception(jsc_value_get_context(result));

        if(htmlmessagefunc && !exception)
            htmlmessagefunc(window, name, str_value, user_data);
            
        g_free(str_value);
        
        if(!exception)
          return;
    }
    if(htmlmessagefunc)
        htmlmessagefunc(window, name, NULL, user_data);
}

static void _dw_html_changed_event(WebKitWebView  *web_view, WebKitLoadEvent load_event, gpointer data)
{
    DWSignalHandler work = _dw_get_signal_handler(data);
    char *location = (char *)webkit_web_view_get_uri(web_view);
    int status = 0;

    switch (load_event) {
    case WEBKIT_LOAD_STARTED:
        status = DW_HTML_CHANGE_STARTED;
        break;
    case WEBKIT_LOAD_REDIRECTED:
        status = DW_HTML_CHANGE_REDIRECT;
        break;
    case WEBKIT_LOAD_COMMITTED:
        status = DW_HTML_CHANGE_LOADING;
        break;
    case WEBKIT_LOAD_FINISHED:
        status = DW_HTML_CHANGE_COMPLETE;
        break;
    }
    if(status && location && work.window && work.func)
    {
        int (*htmlchangedfunc)(HWND, int, char *, void *) = work.func;

        htmlchangedfunc(work.window, status, location, work.data);
    }
}
#endif

static void _dw_set_focus_event(GObject *window, GParamSpec *pspec, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   gboolean active;
   
   g_object_get(window, "is-active", &active, NULL);
   
   if(active && work.window)
   {
      int (*setfocusfunc)(HWND, void *) = work.func;

      setfocusfunc(work.window, work.data);
   }
}

static gint _dw_button_press_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = gtk_gesture_single_get_current_button(gesture);

      if(mybutton == 3)
         mybutton = DW_BUTTON2_MASK;
      else if(mybutton == 2)
         mybutton = DW_BUTTON3_MASK;

      retval = buttonfunc(work.window, (int)x, (int)y, mybutton, work.data);
      
      _dw_event_coordinates_to_window(work.window, &x, &y);
      
      _dw_mouse_last_x = (long)x;
      _dw_mouse_last_y = (long)y;
   }
   return retval;
}

static gint _dw_button_release_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*buttonfunc)(HWND, int, int, int, void *) = work.func;
      int mybutton = gtk_gesture_single_get_current_button(gesture);

      if(mybutton == 3)
         mybutton = DW_BUTTON2_MASK;
      else if(mybutton == 2)
         mybutton = DW_BUTTON3_MASK;

      retval = buttonfunc(work.window, (int)x, (int)y, mybutton, work.data);
      
      _dw_event_coordinates_to_window(work.window, &x, &y);
      
      _dw_mouse_last_x = (long)x;
      _dw_mouse_last_y = (long)y;
   }
   return retval;
}

static gint _dw_motion_notify_event(GtkEventControllerMotion *controller, double x, double y, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*motionfunc)(HWND, int, int, int, void *) = work.func;
      GdkEvent *event = gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(controller));
      GdkModifierType state = gdk_event_get_modifier_state(event);
      int keys = 0;

      if (state & GDK_BUTTON1_MASK)
         keys = DW_BUTTON1_MASK;
      if (state & GDK_BUTTON3_MASK)
         keys |= DW_BUTTON2_MASK;
      if (state & GDK_BUTTON2_MASK)
         keys |= DW_BUTTON3_MASK;

      retval = motionfunc(work.window, (int)x, (int)y, keys, work.data);
      
      _dw_event_coordinates_to_window(work.window, &x, &y);
      
      _dw_mouse_last_x = (long)x;
      _dw_mouse_last_y = (long)y;
   }
   return retval;
}

static gboolean _dw_delete_event(GtkWidget *window, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*closefunc)(HWND, void *) = work.func;

      retval = closefunc(work.window, work.data);
   }
   return retval;
}

static gint _dw_key_press_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*keypressfunc)(HWND, char, int, int, void *, char *) = work.func;
      guint32 unichar = gdk_keyval_to_unicode(keyval);
      char utf8[7] = { 0 };

      g_unichar_to_utf8(unichar, utf8);

      retval = keypressfunc(work.window, (char)keycode, keyval,
                       state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK), work.data, utf8);
   }
   return retval;
}

static gint _dw_generic_event(GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*genericfunc)(HWND, void *) = work.func;

      retval = genericfunc(work.window, work.data);
   }
   return retval;
}

static gint _dw_configure_event(GtkWidget *widget, int width, int height, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*sizefunc)(HWND, int, int, void *) = work.func;

      retval = sizefunc(work.window, width, height, work.data);
   }
   return retval;
}

cairo_t *_dw_cairo_update(GtkWidget *widget, int width, int height)
{
   cairo_t *wincr = g_object_get_data(G_OBJECT(widget), "_dw_cr"); 
   cairo_surface_t *surface = g_object_get_data(G_OBJECT(widget), "_dw_cr_surface"); 

   if(width == -1 && height == -1 && !wincr && g_list_find(_dw_dirty_list, widget) == NULL)
      _dw_dirty_list = g_list_append(_dw_dirty_list, widget);

   if(width == -1)
      width = gtk_widget_get_width(widget);
   if(height == -1)
      height = gtk_widget_get_height(widget);
   
   if(!surface || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_width")) != width ||
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_height")) != height)
   {
      if(surface)
         cairo_surface_destroy(surface);
      surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
      /* Save the cairo context for use in the drawing functions */
      g_object_set_data(G_OBJECT(widget), "_dw_cr_surface", (gpointer)surface);
      g_object_set_data(G_OBJECT(widget), "_dw_width", GINT_TO_POINTER(width));
      g_object_set_data(G_OBJECT(widget), "_dw_height", GINT_TO_POINTER(height));
   }
#ifdef DW_USE_CACHED_CR
   return wincr;
#else
   return NULL;
#endif
}

static gint _dw_expose_event(GtkWidget *widget, cairo_t *cr, int width, int height, gpointer data)
{
   int retval = FALSE;

   if(widget && GTK_IS_DRAWING_AREA(widget))
   {
      DWExpose exp;
      int (*exposefunc)(HWND, DWExpose *, void *) = g_object_get_data(G_OBJECT(widget), "_dw_expose_func");

      _dw_cairo_update(widget, width, height);

      /* Remove the currently drawn widget from the dirty list */
      _dw_dirty_list = g_list_remove(_dw_dirty_list, widget);

      exp.x = exp.y = 0;
      exp.width = width;
      exp.height = height;
      g_object_set_data(G_OBJECT(widget), "_dw_cr", (gpointer)cr);
      retval = exposefunc((HWND)widget, &exp, data);
      g_object_set_data(G_OBJECT(widget), "_dw_cr", NULL);

      /* Copy the cached image to the output surface */
      cairo_set_source_surface(cr, g_object_get_data(G_OBJECT(widget), "_dw_cr_surface"), 0, 0);
      cairo_rectangle(cr, 0, 0, width, height);
      cairo_fill(cr);
   }
   return retval;
}

static gint _dw_combobox_select_event(GtkWidget *widget, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(g_object_get_data(G_OBJECT(widget), "_dw_recursing"))
      return FALSE;

   if(work.window && GTK_IS_COMBO_BOX(widget))
   {
      GtkTreeModel *store = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

      if(store)
      {
         GtkTreeIter iter;
         GtkTreePath *path;

         g_object_set_data(G_OBJECT(widget), "_dw_recursing", GINT_TO_POINTER(1));

         if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
         {
            path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  int (*selectfunc)(HWND, int, void *) = work.func;

                  retval = selectfunc(work.window, indices[0], work.data);
               }
               gtk_tree_path_free(path);
            }
         }

         g_object_set_data(G_OBJECT(widget), "_dw_recursing", NULL);
      }
   }
   return retval;
}

/* Convert coordinate system from the widget to the window */
static void _dw_event_coordinates_to_window(GtkWidget *widget, double *x, double *y)
{
   GtkRoot *root = (widget && GTK_IS_WIDGET(widget)) ? gtk_widget_get_root(widget) : NULL;
   
   if(root && GTK_IS_WIDGET(root))
   {
      GtkWidget *parent = GTK_WIDGET(root);
      
      /* If the parent is a window, try to use the box attached to it... */
      if(GTK_IS_WINDOW(parent))
      {
         GtkWidget *box = g_object_get_data(G_OBJECT(parent), "_dw_grid");
         
         if(box && GTK_IS_GRID(box))
            parent = box;
      }
            
      graphene_point_t *treepoint = graphene_point_init(graphene_point_alloc(), (float)*x, (float)*y);
      graphene_point_t *windowpoint = graphene_point_alloc();
      
      if(gtk_widget_compute_point(widget, parent, treepoint, windowpoint))
      {            
         *x = (double)windowpoint->x;
         *y = (double)windowpoint->y;
      }
      
      graphene_point_free(treepoint);
      graphene_point_free(windowpoint);
   }
}

#define _DW_DATA_TYPE_STRING  0
#define _DW_DATA_TYPE_POINTER 1

static gint _dw_tree_context_event(GtkGestureSingle *gesture, int n_press, double x, double y, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int button = gtk_gesture_single_get_current_button(gesture);
      
      if(button == 3)
      {
         int (*contextfunc)(HWND, char *, int, int, void *, void *) = work.func;
         char *text = NULL;
         void *itemdata = NULL;
         GtkWidget *widget = work.window;
         
         _dw_event_coordinates_to_window(widget, &x, &y);
      
         _dw_mouse_last_x = (long)x;
         _dw_mouse_last_y = (long)y;
         
         /* Containers and trees are inside scrolled window widgets */
         if(GTK_IS_SCROLLED_WINDOW(widget))
            widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "_dw_user"));

         if(widget && GTK_IS_TREE_VIEW(widget))
         {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
            GtkTreeIter iter;

            if(sel && gtk_tree_selection_get_mode(sel) != GTK_SELECTION_MULTIPLE &&
               gtk_tree_selection_get_selected(sel, NULL, &iter))
            {
               if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
               {
                  gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, -1);
               }
               else
               {
                  gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
               }
            }
            else
            {
               GtkTreePath *path;

               gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
               if(path)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
                  {
                     if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
                     {
                        gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, -1);
                     }
                     else
                     {
                        gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
                     }
                  }
                  gtk_tree_path_free(path);
               }
            }
         }
         retval = contextfunc(work.window, text, (int)x, (int)y, work.data, itemdata);
         if(text)
            g_free(text);
      }
   }
   return retval;
}

static gint _dw_tree_select_event(GtkTreeSelection *sel, gpointer data)
{
   GtkWidget *item = NULL, *widget = (GtkWidget *)gtk_tree_selection_get_tree_view(sel);
   int retval = FALSE;

   if(widget)
   {
      DWSignalHandler work = _dw_get_signal_handler(data);

      if(work.window)
      {
         int (*treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = work.func;
         GtkTreeIter iter;
         char *text = NULL;
         void *itemdata = NULL;
         GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

         if(g_object_get_data(G_OBJECT(widget), "_dw_double_click"))
         {
            g_object_set_data(G_OBJECT(widget), "_dw_double_click", GINT_TO_POINTER(0));
            return TRUE;
         }

         if(gtk_tree_selection_get_mode(sel) != GTK_SELECTION_MULTIPLE &&
            gtk_tree_selection_get_selected(sel, NULL, &iter))
         {
            if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
            {
               gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, 3, &item, -1);
               retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
            }
            else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
            {
               gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
               retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
            }
            else
            {
               GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

               if(path)
               {
                  gint *indices = gtk_tree_path_get_indices(path);

                  if(indices)
                  {
                     int (*selectfunc)(HWND, int, void *) = work.func;

                     retval = selectfunc(work.window, indices[0], work.data);
                  }
                  gtk_tree_path_free(path);
               }
            }
         }
         else
         {
            GtkTreePath *path;

            gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
            if(path)
            {
               GtkTreeIter iter;

               if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
               {
                  if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, 2, &itemdata, 3, &item, -1);
                     retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
                  }
                  else if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &itemdata, -1);
                     retval = treeselectfunc(work.window, (HTREEITEM)item, text, work.data, itemdata);
                  }
                  else
                  {
                     gint *indices = gtk_tree_path_get_indices(path);

                     if(indices)
                     {
                        int (*selectfunc)(HWND, int, void *) = work.func;

                        retval = selectfunc(work.window, indices[0], work.data);
                     }
                  }
               }
               gtk_tree_path_free(path);
            }
         }
         if(text)
            g_free(text);
      }
   }
   return retval;
}

static gint _dw_tree_expand_event(GtkTreeView *widget, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*treeexpandfunc)(HWND, HTREEITEM, void *) = work.func;
      retval = treeexpandfunc(work.window, (HTREEITEM)iter, work.data);
   }
   return retval;
}

static gint _dw_container_enter_event(GtkEventController *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window && GTK_IS_WIDGET(work.window))
   {
      GtkWidget *user = GTK_WIDGET(g_object_get_data(G_OBJECT(work.window), "_dw_user"));
      GtkWidget *widget = user ? user : work.window;
      GdkEvent *event = gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(controller));
      GdkEventType type = gdk_event_get_event_type(event);
      gint button = (type == GDK_BUTTON_PRESS) ? gdk_button_event_get_button(event) : -1;
      
      /* Handle both key and button events together */
      if((type == GDK_BUTTON_PRESS && button == 1) || keyval == VK_RETURN)
      {
         int (*contextfunc)(HWND, char *, void *, void *) = work.func;
         char *text = NULL;
         void *data = NULL;

         /* Sanity check */
         if(GTK_IS_TREE_VIEW(widget))
         {
            GtkTreePath *path;
            GtkTreeModel *store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

            gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, NULL);
            if(path)
            {
               GtkTreeIter iter;

               if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
               {
                  if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
                  {
                     gtk_tree_model_get(store, &iter, _DW_DATA_TYPE_STRING, &text, _DW_DATA_TYPE_POINTER, &data, -1);
                     retval = contextfunc(work.window, text, work.data, data);
                     if(text)
                        g_free(text);
                  }
               }
               gtk_tree_path_free(path);
            }
         }
      }
   }
   return retval;
}

/* Just forward to the other handler, with bogus key info */
void _dw_container_enter_mouse(GtkEventController *controller, int n_press, double x, double  y, gpointer data)
{
   if(n_press == 2)
      _dw_container_enter_event(GTK_EVENT_CONTROLLER(controller), 0, 0, 0, data);
}

/* Return the logical page id from the physical page id */
int _dw_get_logical_page(HWND handle, unsigned long pageid)
{
   int z;
   GtkWidget **pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray");
   GtkWidget *thispage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), pageid);

   if(pagearray && thispage)
   {
      for(z=0;z<256;z++)
      {
         if(thispage == pagearray[z])
            return z;
      }
   }
   return 256;
}


static gint _dw_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   int retval = FALSE;

   if(work.window)
   {
      int (*switchpagefunc)(HWND, unsigned long, void *) = work.func;
      retval = switchpagefunc(work.window, _dw_get_logical_page(GTK_WIDGET(notebook), page_num), work.data);
   }
   return retval;
}

static gint _dw_column_click_event(GtkWidget *widget, gpointer data)
{
   void **params = data;
   int retval = FALSE;

   if(params && params[2])
   {
      GtkWidget *tree = (GtkWidget *)params[2];
      gint handlerdata = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tree), "_dw_column_click_id"));

      if(handlerdata)
      {
         DWSignalHandler work;

         params[0] = GINT_TO_POINTER(handlerdata-1);
         work = _dw_get_signal_handler(params);

         if(work.window)
         {
            int column_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_column"));
            int (*clickcolumnfunc)(HWND, int, void *) = work.func;
            retval = clickcolumnfunc(work.window, column_num, work.data);
         }
      }
   }
   return retval;
}

static int _dw_round_value(gfloat val)
{
   int newval = (int)val;

   if(val >= 0.5 + (gfloat)newval)
      newval++;

   return newval;
}

static gint _dw_value_changed_event(GtkWidget *widget, gpointer data)
{
   GtkWidget *slider, *spinbutton, *scrollbar;
   GtkAdjustment *adjustment = (GtkAdjustment *)widget;
   int max, val;

   if(!GTK_IS_ADJUSTMENT(adjustment))
      adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(widget), "_dw_adjustment");

   slider = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_slider");
   spinbutton = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_spinbutton");
   scrollbar = (GtkWidget *)g_object_get_data(G_OBJECT(adjustment), "_dw_scrollbar");

   max = _dw_round_value(gtk_adjustment_get_upper(adjustment));
   val = _dw_round_value(gtk_adjustment_get_value(adjustment));

   if(g_object_get_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event"))
      return FALSE;

   if(slider || spinbutton || scrollbar)
   {
      DWSignalHandler work = _dw_get_signal_handler(data);

      if (work.window)
      {
         int (*valuechangedfunc)(HWND, int, void *) = work.func;

         if(slider && gtk_orientable_get_orientation(GTK_ORIENTABLE(slider)) == GTK_ORIENTATION_VERTICAL)
            valuechangedfunc(work.window, (max - val) - 1,  work.data);
         else
            valuechangedfunc(work.window, val,  work.data);
      }
   }
   return FALSE;
}

static gint _dw_default_key_press_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data)
{
   GtkWidget *next = (GtkWidget *)data;

   if(next)
   {
      if(keyval == GDK_KEY_Return)
      {
         if(GTK_IS_BUTTON(next))
            g_signal_emit_by_name(G_OBJECT(next), "clicked");
         else
            gtk_widget_grab_focus(next);
      }
   }
   return FALSE;
}

static void _dw_dialog_response(GtkDialog *dialog, int response_id, gpointer data)
{
   DWDialog *dwdialog = (DWDialog *)data;
   
   if(dwdialog)
      dw_dialog_dismiss(dwdialog, DW_INT_TO_POINTER(response_id));
}

static GdkPixbuf *_dw_pixbuf_from_resource(unsigned int rid)
{
   char resource_path[201] = {0};

   snprintf(resource_path, 200, "%s%u.png", _DW_RESOURCE_PATH, rid);
   return gdk_pixbuf_new_from_resource(resource_path, NULL);
}

static GdkPixbuf *_dw_find_pixbuf(HICN icon, unsigned long *userwidth, unsigned long *userheight)
{
   unsigned int id = GPOINTER_TO_INT(icon);
   GdkPixbuf *icon_pixbuf = NULL;

   /* Quick dropout for non-handle */
   if(!icon)
      return NULL;

   if(id > 65535)
      icon_pixbuf = icon;
   else
      icon_pixbuf = _dw_pixbuf_from_resource(id);
   
   if(userwidth)
      *userwidth = icon_pixbuf ? gdk_pixbuf_get_width(icon_pixbuf) : 0;
   if(userheight)
      *userheight = icon_pixbuf ? gdk_pixbuf_get_height(icon_pixbuf) : 0;

   return icon_pixbuf;
}

/* Handle system notification click callbacks */
static void _dw_notification_handler(GSimpleAction *action, GVariant *param, gpointer user_data)
{
   char textbuf[101] = {0};
   int (*func)(HWND, void *);
   void *data;
   
   snprintf(textbuf, 100, "dw-notification-%llu-func", DW_POINTER_TO_ULONGLONG(g_variant_get_uint64(param)));
   func = g_object_get_data(G_OBJECT(_DWApp), textbuf);
   g_object_set_data(G_OBJECT(_DWApp), textbuf, NULL);
   snprintf(textbuf, 100, "dw-notification-%llu-data", DW_POINTER_TO_ULONGLONG(g_variant_get_uint64(param)));
   data = g_object_get_data(G_OBJECT(_DWApp), textbuf);
   g_object_set_data(G_OBJECT(_DWApp), textbuf, NULL);  
   
   if(func)
      func((HWND)DW_ULONGLONG_TO_POINTER(g_variant_get_uint64(param)), data);  
}

/* Handle menu click callbacks */
static void _dw_menu_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
   DWSignalHandler work = _dw_get_signal_handler(data);
   GVariant *action_state = g_action_get_state(G_ACTION(action));
   
   /* Handle toggling checkable menu items automatically, before the callback */
   if(action_state)
   {
      gboolean active = g_variant_get_boolean(action_state);
      GVariant *new_state = g_variant_new_boolean(!active);
      g_simple_action_set_state(action, new_state);
   }

   if(work.window)
   {
      int (*genericfunc)(HWND, void *) = work.func;

      genericfunc(work.window, work.data);
   }
}

/* Internal function to add padding to boxes or other widgets */
static void _dw_widget_set_pad(GtkWidget *widget, int pad)
{
   /* Set pad for each margin direction on the widget */
   gtk_widget_set_margin_start(widget, pad);
   gtk_widget_set_margin_end(widget, pad);
   gtk_widget_set_margin_top(widget, pad);
   gtk_widget_set_margin_bottom(widget, pad);
}

static void _dw_app_activate(GApplication *app, gpointer user_data)
{
   /* Not sure why this signal is required, but GLib gives warnings
    * when this signal is not connected, so putting this here to
    * quell the warning and can be used at a later point if needed.
    */
}

void _dw_init_path(char *arg)
{
   char path[PATH_MAX+1] = {0};

#ifdef __linux__
   if(readlink("/proc/self/exe", path, PATH_MAX) == -1)
#elif defined(__FreeBSD__)
   int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
   size_t length = PATH_MAX;

   if(sysctl(name, 4, path, &length, NULL, 0) == -1 || length <= 1)
#elif defined(__sun__)
   char procpath[101] = {0};

   snprintf(procpath, 100, "/proc/%ld/path/a.out", (long)getpid());

   if(readlink(procpath, path, PATH_MAX) == -1)
#endif
      strncpy(path, arg ? arg : "", PATH_MAX);

   if(path[0])
   {
      char *pos = strrchr(path, '/');
      char *binname = path;

      /* If we have a / then...
       * the binary name should be at the end.
       */
      if(pos)
      {
         binname = pos + 1;
         *pos = 0;
      }

      if(*binname)
      {
         char *binpos = strstr(path, "/bin");

         if(binpos)
            strncpy(_dw_share_path, path, (size_t)(binpos - path));
         else
            strcpy(_dw_share_path, "/usr/local");
         strcat(_dw_share_path, "/share/");
         strcat(_dw_share_path, binname);
         if(!_dw_app_id[0])
         {
            /* If we have a binary name, use that for the Application ID instead. */
            snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.%s", DW_APP_DOMAIN_DEFAULT, binname);
         }
      }
  }
   /* If that failed... just get the current directory */
   if(!_dw_share_path[0] && !getcwd(_dw_share_path, PATH_MAX))
      _dw_share_path[0] = '/';
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
   /* Setup the private data directory */
   _dw_init_path(argc > 0 ? argv[0] : NULL);

   gtk_init();
   
   _DWMainLoop = g_main_loop_new(NULL, FALSE);
   g_main_loop_ref(_DWMainLoop);

   pthread_key_create(&_dw_fg_color_key, NULL);
   pthread_key_create(&_dw_bg_color_key, NULL);
   pthread_key_create(&_dw_event_key, NULL);

   _dw_init_thread();

   /* Create a global object for glib activities */
   _DWObject = g_object_new(G_TYPE_OBJECT, NULL);

   if(!_dw_app_id[0])
   {
      /* Generate an Application ID based on the PID if all else fails. */
      snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, (int)getpid());
   }

   /* Initialize the application subsystem on supported versions...
    * we generate an application ID based on the binary name or PID
    * instead of passing NULL to enable full application support.
    */
#if GLIB_CHECK_VERSION(2,74,0)
   _DWApp = g_application_new(_dw_app_id, G_APPLICATION_DEFAULT_FLAGS);
#else
   _DWApp = g_application_new(_dw_app_id, G_APPLICATION_FLAGS_NONE);
#endif
   if(_DWApp && g_application_register(_DWApp, NULL, NULL))
   {
      /* Creat our notification handler for any notifications */
      GSimpleAction *action = g_simple_action_new("notification", G_VARIANT_TYPE_UINT64);

      g_set_prgname(_dw_app_id);
      g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(_dw_notification_handler), NULL);
      g_action_map_add_action(G_ACTION_MAP(_DWApp), G_ACTION(action));
      g_signal_connect(_DWApp, "activate", G_CALLBACK(_dw_app_activate), NULL);
      g_application_activate(_DWApp);
   }
   return TRUE;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
   pthread_t orig = _dw_thread;
   
   _dw_thread = pthread_self();
   g_main_loop_run(_DWMainLoop);
   _dw_thread = orig;
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
   g_main_loop_quit(_DWMainLoop);
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
   struct timeval tv, start;
   pthread_t curr = pthread_self();

   gettimeofday(&start, NULL);

   if(_dw_thread == (pthread_t)-1 || _dw_thread == curr)
   {
      pthread_t orig = _dw_thread;

      gettimeofday(&tv, NULL);

      while(((tv.tv_sec - start.tv_sec)*1000) + ((tv.tv_usec - start.tv_usec)/1000) <= milliseconds)
      {
         if(orig == (pthread_t)-1)
            _dw_thread = curr;
         if(curr == _dw_thread && g_main_context_pending(NULL))
            g_main_context_iteration(NULL, FALSE);
         else
            _dw_msleep(1);
         if(orig == (pthread_t)-1)
            _dw_thread = orig;
         gettimeofday(&tv, NULL);
      }
   }
   else
      _dw_msleep(milliseconds);
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
   pthread_t orig = _dw_thread;
   pthread_t curr = pthread_self();

   if(_dw_thread == (pthread_t)-1)
      _dw_thread = curr;
   if(curr == _dw_thread && g_main_context_pending(NULL))
      g_main_context_iteration(NULL, FALSE);
   else
      sched_yield();
   if(orig == (pthread_t)-1)
      _dw_thread = orig;
}

/*
 * Free's memory allocated by dynamic windows.
 * Parameters:
 *           ptr: Pointer to dynamic windows allocated
 *                memory to be free()'d.
 */
void API dw_free(void *ptr)
{
   free(ptr);
}

/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog * API dw_dialog_new(void *data)
{
   DWDialog *tmp = calloc(sizeof(DWDialog), 1);

   if(tmp)
   {
      tmp->eve = dw_event_new();
      dw_event_reset(tmp->eve);
      tmp->data = data;
      tmp->mainloop = g_main_loop_new(NULL, FALSE);
      g_main_loop_ref(tmp->mainloop);
   }
   return tmp;
}

/*
 * Accepts a dialog struct and returns the given data to the
 * initial called of dw_dialog_wait().
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 *           result: Data to be returned by dw_dialog_wait().
 */
int API dw_dialog_dismiss(DWDialog *dialog, void *result)
{
   dialog->result = result;
   if(dialog->method)
      g_main_loop_quit(dialog->mainloop);
   else
      dw_event_post(dialog->eve);
   dialog->done = TRUE;
   return DW_ERROR_NONE;
}

/*
 * Accepts a dialog struct waits for dw_dialog_dismiss() to be
 * called by a signal handler with the given dialog struct.
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 */
void * API dw_dialog_wait(DWDialog *dialog)
{
   void *tmp;

   if(!dialog)
      return NULL;

   if(_dw_thread == (pthread_t)-1 || pthread_self() == _dw_thread)
   {
      dialog->method = TRUE;
      g_main_loop_run(dialog->mainloop);
   }
   else
   {
      dialog->method = FALSE;
      dw_event_wait(dialog->eve, -1);
   }

   dw_event_close(&dialog->eve);
   g_main_loop_unref(dialog->mainloop);
   tmp = dialog->result;
   free(dialog);
   return tmp;
}

/*
 * Displays a debug message on the console...
 * Parameters:
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
void API dw_debug(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
   vfprintf(stderr, format, args);
}

#if GTK_CHECK_VERSION(4,10,0)
static void _dw_alert_dialog_choose_response(GObject *gobject, GAsyncResult *result, gpointer data)
{
  DWDialog *tmp = data;
  GError *error = NULL;
  int retval = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(gobject), result, &error);

  if(error != NULL)
  {
      retval = -1;
  }
  dw_dialog_dismiss(tmp, DW_INT_TO_POINTER(retval));
}

static char *_DW_BUTTON_OK = "Ok";
static char *_DW_BUTTON_YES = "Yes";
static char *_DW_BUTTON_NO = "No";
static char *_DW_BUTTON_CANCEL = "Cancel";
#endif

/* Internal version that does not use variable arguments */
DW_FUNCTION_DEFINITION(dw_messagebox_int, int, const char *title, int flags, char *outbuf)
DW_FUNCTION_ADD_PARAM3(title, flags, outbuf)
DW_FUNCTION_RETURN(dw_messagebox_int, int)
DW_FUNCTION_RESTORE_PARAM3(title, const char *, flags, int, outbuf, char *)
{
   int response, retval = DW_MB_RETURN_OK;
   DWDialog *tmp = dw_dialog_new(NULL);
#if GTK_CHECK_VERSION(4,10,0)
   GtkAlertDialog *ad = gtk_alert_dialog_new("%s", title);
   char *buttons[4] = { 0 };
   int button = 0;

   gtk_alert_dialog_set_message(ad, outbuf);
   gtk_alert_dialog_set_modal(ad, TRUE);

   if(flags & (DW_MB_OK | DW_MB_OKCANCEL))
      buttons[button++] = _DW_BUTTON_OK;
   if(flags & (DW_MB_YESNO | DW_MB_YESNOCANCEL))
      buttons[button++] = _DW_BUTTON_YES;
   if(flags & (DW_MB_YESNO | DW_MB_YESNOCANCEL))
      buttons[button++] = _DW_BUTTON_NO;
   if(flags & (DW_MB_OKCANCEL | DW_MB_YESNOCANCEL))
      buttons[button++] = _DW_BUTTON_CANCEL;
   gtk_alert_dialog_set_buttons(ad, (const char * const*)buttons);

   gtk_alert_dialog_choose(ad, NULL, NULL, (GAsyncReadyCallback)_dw_alert_dialog_choose_response, tmp);

   response = DW_POINTER_TO_INT(dw_dialog_wait(tmp));

   if(response < 0 || response >= button || buttons[response] == _DW_BUTTON_OK)
      retval = DW_MB_RETURN_OK;
   else if(buttons[response] == _DW_BUTTON_CANCEL)
      retval = DW_MB_RETURN_CANCEL;
   else if(buttons[response] == _DW_BUTTON_YES)
      retval = DW_MB_RETURN_YES;
   else if(buttons[response] == _DW_BUTTON_NO)
      retval = DW_MB_RETURN_NO;
#else
   GtkMessageType gtkicon = GTK_MESSAGE_OTHER;
   GtkButtonsType gtkbuttons = GTK_BUTTONS_OK;
   GtkWidget *dialog;
   ULONG width, height;

   if(flags & DW_MB_ERROR)
      gtkicon = GTK_MESSAGE_ERROR;
   else if(flags & DW_MB_WARNING)
      gtkicon = GTK_MESSAGE_WARNING;
   else if(flags & DW_MB_INFORMATION)
      gtkicon = GTK_MESSAGE_INFO;
   else if(flags & DW_MB_QUESTION)
      gtkicon = GTK_MESSAGE_QUESTION;

   if(flags & DW_MB_OKCANCEL)
      gtkbuttons = GTK_BUTTONS_OK_CANCEL;
   else if(flags & (DW_MB_YESNO | DW_MB_YESNOCANCEL))
      gtkbuttons = GTK_BUTTONS_YES_NO;

   dialog = gtk_message_dialog_new(NULL,
                                   GTK_DIALOG_USE_HEADER_BAR |
                                   GTK_DIALOG_MODAL, gtkicon, gtkbuttons, "%s", title);
   gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", outbuf);
   if(flags & DW_MB_YESNOCANCEL)
      gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_CANCEL);
   gtk_widget_set_visible(GTK_WIDGET(dialog), TRUE);
   g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(_dw_dialog_response), (gpointer)tmp);
   /* Center the dialog on the screen since there is no parent */
   dw_window_get_pos_size((HWND)dialog, NULL, NULL, &width, &height);
   dw_window_set_pos((HWND)dialog, (dw_screen_width() - width)/2, (dw_screen_height() - height)/2);
   response = DW_POINTER_TO_INT(dw_dialog_wait(tmp));
   if(GTK_IS_WINDOW(dialog))
      gtk_window_destroy(GTK_WINDOW(dialog));
   switch(response)
   {
      case GTK_RESPONSE_OK:
         retval = DW_MB_RETURN_OK;
         break;
      case GTK_RESPONSE_CANCEL:
         retval = DW_MB_RETURN_CANCEL;
         break;
      case GTK_RESPONSE_YES:
         retval = DW_MB_RETURN_YES;
         break;
      case GTK_RESPONSE_NO:
         retval = DW_MB_RETURN_NO;
         break;
      default:
      {
         /* Handle the destruction of the dialog result */
         if(flags & (DW_MB_OKCANCEL | DW_MB_YESNOCANCEL))
            retval = DW_MB_RETURN_CANCEL;
         else if(flags & DW_MB_YESNO)
            retval = DW_MB_RETURN_NO;
      }
   }
#endif
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: Defines buttons and icons to display
 *           format: printf style format string.
 *           ...: Additional variables for use in the format.
 */
int API dw_messagebox(const char *title, int flags, const char *format, ...)
{
   va_list args;
   int rc;

   va_start(args, format);
   rc = dw_vmessagebox(title, flags, format, args);
   va_end(args);
   return rc;
}

int API dw_vmessagebox(const char *title, int flags, const char *format, va_list args)
{
   char outbuf[1025] = {0};

   vsnprintf(outbuf, 1024, format, args);
   return dw_messagebox_int(title, flags, outbuf);
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
DW_FUNCTION_DEFINITION(dw_window_minimize, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_minimize, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_NONE;

   if(handle && GTK_IS_WINDOW(handle))
      gtk_window_minimize(GTK_WINDOW(handle));
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
#ifndef GDK_WINDOWING_X11
int API dw_window_raise(HWND handle)
{
   return DW_ERROR_UNKNOWN;
}
#else
DW_FUNCTION_DEFINITION(dw_window_raise, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_raise, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_UNKNOWN;
   GdkDisplay *display = gdk_display_get_default();

   if(handle && GTK_IS_WINDOW(handle) && display && GDK_IS_X11_DISPLAY(display))
   {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(handle));
      
      if(surface)
      {
         XRaiseWindow(GDK_SURFACE_XDISPLAY(surface), GDK_SURFACE_XID(surface));
         retval = DW_ERROR_NONE;
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}
#endif

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
#ifndef GDK_WINDOWING_X11
int API dw_window_lower(HWND handle)
{
   return DW_ERROR_UNKNOWN;
}
#else
DW_FUNCTION_DEFINITION(dw_window_lower, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_lower, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_UNKNOWN;
   GdkDisplay *display = gdk_display_get_default();

   if(handle && GTK_IS_WINDOW(handle) && display && GDK_IS_X11_DISPLAY(display))
   {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(handle));
      
      if(surface)
      {
         XLowerWindow(GDK_SURFACE_XDISPLAY(surface), GDK_SURFACE_XID(surface));
         retval = DW_ERROR_NONE;
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}
#endif

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
DW_FUNCTION_DEFINITION(dw_window_show, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_show, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_NONE;

   if(handle)
   {
      if(GTK_IS_WINDOW(handle))
      {
         GtkWidget *defaultitem = GTK_WIDGET(g_object_get_data(G_OBJECT(handle), "_dw_defaultitem"));

         gtk_window_present(GTK_WINDOW(handle));
         
         if(defaultitem)
            gtk_widget_grab_focus(defaultitem);
      }
      else if(GTK_IS_WIDGET(handle))
         gtk_widget_set_visible(handle, TRUE);
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
DW_FUNCTION_DEFINITION(dw_window_hide, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_hide, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_NONE;

   if(handle && GTK_IS_WIDGET(handle))
      gtk_widget_set_visible(handle, FALSE);
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Destroys a window and all of it's children.
 * Parameters:
 *           handle: The window handle to destroy.
 */
DW_FUNCTION_DEFINITION(dw_window_destroy, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_destroy, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = DW_ERROR_NONE;

   if(handle)
   {
      if(GTK_IS_WINDOW(handle))
         gtk_window_destroy(GTK_WINDOW(handle));
      else if(GTK_IS_WIDGET(handle))
      {
         GtkWidget *box, *handle2 = handle;

         /* Check if we are removing a widget from a box */	
         if((box = gtk_widget_get_parent(handle2)) && GTK_IS_GRID(box))
         {
            /* Get the number of items in the box... */
            int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
            int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
            int z;

            /* Figure out where in the grid this widget is and remove that row/column */
            if(boxtype == DW_VERT)
            {
               for(z=0;z<boxcount;z++)
               {
                  if(gtk_grid_get_child_at(GTK_GRID(box), 0, z) == handle2)
                  {
                     gtk_grid_remove_row(GTK_GRID(box), z);
                     handle2 = NULL;
                     break;
                  }
               }
            }
            else
            {
               for(z=0;z<boxcount;z++)
               {
                  if(gtk_grid_get_child_at(GTK_GRID(box), z, 0) == handle2)
                  {
                     gtk_grid_remove_column(GTK_GRID(box), z);
                     handle2 = NULL;
                     break;
                  }
               }
            }
			   
            if(boxcount > 0)
            {
               /* Decrease the count by 1 */
               boxcount--;
               g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
            }
         }
         /* Finally destroy the widget, make sure it is still
          * a valid widget if it got removed from the grid.
          */
         if(handle2 && GTK_IS_WIDGET(handle2))
            g_object_unref(G_OBJECT(handle2));
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
DW_FUNCTION_DEFINITION(dw_window_redraw, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_redraw)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   if(handle && GTK_IS_DRAWING_AREA(handle))
      gtk_widget_queue_draw(GTK_WIDGET(handle));
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
DW_FUNCTION_DEFINITION(dw_window_reparent, void, HWND handle, HWND newparent)
DW_FUNCTION_ADD_PARAM2(handle, newparent)
DW_FUNCTION_NO_RETURN(dw_window_reparent)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, newparent, HWND)
{
   if(handle && GTK_IS_WIDGET(handle) && newparent && GTK_IS_WIDGET(newparent))
      gtk_widget_set_parent(GTK_WIDGET(handle), GTK_WIDGET(newparent));
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
   char *oldfont = _DWDefaultFont;

   _DWDefaultFont = strdup(fontname);

   if(oldfont)
      free(oldfont);
}

/* Convert DW style font to CSS syntax (or Pango for older versions):
 * font: font-style font-variant font-weight font-size/line-height font-family
 */
char *_dw_convert_font(const char *font)
{
   char *newfont = NULL;
   
   if(font)
   {
      char *name = strchr(font, '.');
      char *Italic = strstr(font, " Italic");
      char *Bold = strstr(font, " Bold");
      
      /* Detect Dynamic Windows style font name...
       * Format: ##.Fontname
       * and convert to CSS or Pango syntax
       */
      if(name && (name++) && isdigit(*font))
      {
          int size = atoi(font);
          int len = (Italic ? (Bold ? (Italic > Bold ? (Bold - name) : (Italic - name)) : (Italic - name)) : (Bold ? (Bold - name) : strlen(name)));
          char *newname = alloca(len+1);
          
          memset(newname, 0, len+1);
          strncpy(newname, name, len);
          
          newfont = g_strdup_printf("%s normal %s %dpx \"%s\"", Italic ? "italic" : "normal",
                                    Bold ? "bold" : "normal", size, newname);
      }
   }
   return newfont;
}

/* Internal functions to convert to GTK3 style CSS */
static void _dw_override_color(GtkWidget *widget, const char *element, GdkRGBA *color)
{
   gchar *dataname = g_strdup_printf ("_dw_color_%s", element);
   GtkCssProvider *provider = g_object_get_data(G_OBJECT(widget), dataname);
   GtkStyleContext *scontext = gtk_widget_get_style_context(widget);
   
   /* If we have an old context from a previous override remove it */
   if(provider)
   {
      gtk_style_context_remove_provider(scontext, GTK_STYLE_PROVIDER(provider));
      g_object_unref(provider);
      provider = NULL;
   }
   
   /* If we have a new color, create a new provider and add it */
   if(color)
   {
      gchar *scolor = gdk_rgba_to_string(color);
      gchar *css = g_strdup_printf ("* { %s: %s; }", element, scolor);
      
      provider = gtk_css_provider_new();
      g_free(scolor);
      gtk_css_provider_load_from_data(provider, css, -1);
      g_free(css);
      gtk_style_context_add_provider(scontext, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   g_object_set_data(G_OBJECT(widget), dataname, (gpointer)provider);
   g_free(dataname);
}

static void _dw_override_font(GtkWidget *widget, const char *font)
{
   GtkCssProvider *provider = g_object_get_data(G_OBJECT(widget), "_dw_font");
   GtkStyleContext *scontext = gtk_widget_get_style_context(widget);
   
   /* If we have an old context from a previous override remove it */
   if(provider)
   {
      gtk_style_context_remove_provider(scontext, GTK_STYLE_PROVIDER(provider));
      g_object_unref(provider);
      provider = NULL;
   }
   
   /* If we have a new font, create a new provider and add it */
   if(font)
   {
      gchar *css = g_strdup_printf ("* { font: %s; }", font);
      
      provider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(provider, css, -1);
      g_free(css);
      gtk_style_context_add_provider(scontext, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
   }
   g_object_set_data(G_OBJECT(widget), "_dw_font", (gpointer)provider);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
DW_FUNCTION_DEFINITION(dw_window_set_font, int, HWND handle, const char *fontname)
DW_FUNCTION_ADD_PARAM2(handle, fontname)
DW_FUNCTION_RETURN(dw_window_set_font, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, fontname, const char *)
{
   GtkWidget *handle2 = handle;
   char *font = _dw_convert_font(fontname);
   gpointer data;
   int retval = DW_ERROR_NONE;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* If it is a groupox we want to operate on the frame label */
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = gtk_frame_get_label_widget(GTK_FRAME(handle));
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_COMBO_BOX(handle))
   {
      GtkWidget *tmp = gtk_combo_box_get_child(GTK_COMBO_BOX(handle));
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_BUTTON(handle))
   {
      GtkWidget *tmp = gtk_button_get_child(GTK_BUTTON(handle));
      if(tmp)
         handle2 = tmp;
   }

   /* Free old font name if one is allocated */
   data = g_object_get_data(G_OBJECT(handle2), "_dw_fontname");
   g_object_set_data(G_OBJECT(handle2), "_dw_fontname", (gpointer)font);
   if(data)
      free(data);

   if(!GTK_IS_DRAWING_AREA(handle2))
      _dw_override_font(handle2, font);

   DW_FUNCTION_RETURN_THIS(retval);
}

/* Internal function to convert from a pango font description to Dynamic Windows style font string */
char *_dw_font_from_pango_font_description(PangoFontDescription *pfont)
{
  char *retfont = NULL;

  if(pfont)
  {
     char *font = pango_font_description_to_string(pfont);
     int len, x;

     retfont = strdup(font);
     len = strlen(font);
  
     /* Convert to Dynamic Windows format if we can... */
     if(len > 0 && isdigit(font[len-1]))
     {
        int size;

        x=len-1;
        while(x > 0 && font[x] != ' ')
        {
           x--;
        }
        size = atoi(&font[x]);
        /* If we were able to find a valid size... */
        if(size > 0)
        {
           /* Null terminate after the name...
            * and create the Dynamic Windows style font.
            */
           font[x] = 0;
           snprintf(retfont, len+1, "%d.%s", size, font);
        }
     }
     g_free(font);
  }
  return retfont;
}

#if GTK_CHECK_VERSION(4,10,0)
static void _dw_font_choose_response(GObject *gobject, GAsyncResult *result, gpointer data)
{
  DWDialog *tmp = data;
  GError *error = NULL;
  char *fontname = NULL;
  PangoFontDescription *pfd = gtk_font_dialog_choose_font_finish(GTK_FONT_DIALOG(gobject), result, &error);

  if(error == NULL && pfd != NULL)
  {
    fontname = _dw_font_from_pango_font_description(pfd);
    pango_font_description_free(pfd);
  }
  dw_dialog_dismiss(tmp, fontname);
}
#endif

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
DW_FUNCTION_DEFINITION(dw_font_choose, char *, const char *currfont)
DW_FUNCTION_ADD_PARAM1(currfont)
DW_FUNCTION_RETURN(dw_font_choose, char *)
DW_FUNCTION_RESTORE_PARAM1(currfont, const char *)
{
   char *retfont = NULL;
   DWDialog *tmp = dw_dialog_new(NULL);
#if GTK_CHECK_VERSION(4,10,0)
   char *font = _dw_convert_font(currfont);
   PangoFontDescription *pfd = font ? pango_font_description_from_string(font) : NULL;
   GtkFontDialog *fd = gtk_font_dialog_new();

   gtk_font_dialog_choose_font(fd, NULL, pfd, NULL, (GAsyncReadyCallback)_dw_font_choose_response, tmp);
   
   retfont = dw_dialog_wait(tmp);
#else
   char *font = currfont ? strdup(currfont) : NULL;
   char *name = font ? strchr(font, '.') : NULL;
   GtkFontChooser *fd;

   /* Detect Dynamic Windows style font name...
    * Format: ##.Fontname
    * and convert to a Pango name
    */
   if(name && isdigit(*font))
   {
       int size = atoi(font);
       *name = 0;
       name++;
       sprintf(font, "%s %d", name, size);
   }

   fd = (GtkFontChooser *)gtk_font_chooser_dialog_new("Choose font", NULL);
   if(font)
   {
      gtk_font_chooser_set_font(fd, font);
      free(font);
   }

   gtk_widget_set_visible(GTK_WIDGET(fd), TRUE);
   g_signal_connect(G_OBJECT(fd), "response", G_CALLBACK(_dw_dialog_response), (gpointer)tmp);

   if(DW_POINTER_TO_INT(dw_dialog_wait(tmp)) == GTK_RESPONSE_OK)
   {
      char *fontname = gtk_font_chooser_get_font(fd);
      if(fontname && (retfont = strdup(fontname)))
      {
         int len = strlen(fontname);
         /* Convert to Dynamic Windows format if we can... */
         if(len > 0 && isdigit(fontname[len-1]))
         {
            int size, x=len-1;

            while(x > 0 && fontname[x] != ' ')
            {
               x--;
            }
            size = atoi(&fontname[x]);
            /* If we were able to find a valid size... */
            if(size > 0)
            {
               /* Null terminate after the name...
                * and create the Dynamic Windows style font.
                */
               fontname[x] = 0;
               snprintf(retfont, len+1, "%d.%s", size, fontname);
            }
         }
         g_free(fontname);
      }
   }
   if(GTK_IS_WINDOW(fd))
      gtk_window_destroy(GTK_WINDOW(fd));
#endif
   DW_FUNCTION_RETURN_THIS(retfont);
}

/*
 * Gets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 */
DW_FUNCTION_DEFINITION(dw_window_get_font, char *, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_get_font, char *)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   PangoFontDescription *pfont;
   PangoContext *pcontext;
   GtkWidget *handle2 = handle;
   char *font;
   char *retfont=NULL;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* If it is a groupox we want to operate on the frame label */
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = gtk_frame_get_label_widget(GTK_FRAME(handle));
      if(tmp)
         handle2 = tmp;
   }

   pcontext = gtk_widget_get_pango_context(handle2);
   if(pcontext)
   {
      pfont = pango_context_get_font_description(pcontext);
      retfont = _dw_font_from_pango_font_description(pfont);
   }
   DW_FUNCTION_RETURN_THIS(retfont);
}

void _dw_free_gdk_colors(HWND handle)
{
   GdkRGBA *old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_foregdk");

   if(old)
      free(old);

   old = (GdkRGBA *)g_object_get_data(G_OBJECT(handle), "_dw_backgdk");

   if(old)
      free(old);
}

/* Free old color pointers and allocate new ones */
static void _dw_save_gdk_colors(HWND handle, GdkRGBA fore, GdkRGBA back)
{
   GdkRGBA *foregdk = malloc(sizeof(GdkRGBA));
   GdkRGBA *backgdk = malloc(sizeof(GdkRGBA));

   _dw_free_gdk_colors(handle);

   *foregdk = fore;
   *backgdk = back;

   g_object_set_data(G_OBJECT(handle), "_dw_foregdk", (gpointer)foregdk);
   g_object_set_data(G_OBJECT(handle), "_dw_backgdk", (gpointer)backgdk);
}

static int _dw_set_color(HWND handle, unsigned long fore, unsigned long back)
{
   /* Remember that each color component in X11 use 16 bit no matter
    * what the destination display supports. (and thus GDK)
    */
   GdkRGBA forecolor = {0}, backcolor = {0};

   if(fore & DW_RGB_COLOR)
   {
      forecolor.alpha = 1.0;
      forecolor.red = (gdouble)DW_RED_VALUE(fore) / 255.0;
      forecolor.green = (gdouble)DW_GREEN_VALUE(fore) / 255.0;
      forecolor.blue = (gdouble)DW_BLUE_VALUE(fore) / 255.0;
   }
   else if(fore != DW_CLR_DEFAULT)
      forecolor = _dw_colors[fore];

   _dw_override_color(handle, "color", fore != DW_CLR_DEFAULT ? &forecolor : NULL);

   if(back & DW_RGB_COLOR)
   {
      backcolor.alpha = 1.0;
      backcolor.red = (gdouble)DW_RED_VALUE(back) / 255.0;
      backcolor.green = (gdouble)DW_GREEN_VALUE(back) / 255.0;
      backcolor.blue = (gdouble)DW_BLUE_VALUE(back) / 255.0;
   }
   else if(back != DW_CLR_DEFAULT)
      backcolor = _dw_colors[back];

   _dw_override_color(handle, "background-color", back != DW_CLR_DEFAULT ? &backcolor : NULL);

   _dw_save_gdk_colors(handle, forecolor, backcolor);

   return TRUE;
}
/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in RGB format.
 *          back: Background color in RGB format.
 */
DW_FUNCTION_DEFINITION(dw_window_set_color, int, HWND handle, unsigned long fore, unsigned long back)
DW_FUNCTION_ADD_PARAM3(handle, fore, back)
DW_FUNCTION_RETURN(dw_window_set_color, int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, fore, unsigned long, back, unsigned long)
{
   GtkWidget *handle2 = handle;
   int retval = DW_ERROR_NONE;

   if(GTK_IS_SCROLLED_WINDOW(handle) || GTK_IS_BOX(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }

   _dw_set_color(handle2, fore, back);

   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
   /* TODO */
   return 0;
}

/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
DW_FUNCTION_DEFINITION(dw_window_set_pointer, void, HWND handle, int pointertype)
DW_FUNCTION_ADD_PARAM2(handle, pointertype)
DW_FUNCTION_NO_RETURN(dw_window_set_pointer)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, pointertype, int)
{
   if(pointertype > 65535)
   {
      GdkPixbuf *pixbuf = _dw_find_pixbuf(GINT_TO_POINTER(pointertype), NULL, NULL);
      GdkCursor *cursor = gdk_cursor_new_from_texture(gdk_texture_new_for_pixbuf(pixbuf), 0, 0, NULL);
      if(cursor)
         gtk_widget_set_cursor(GTK_WIDGET(handle), cursor);
   }
   if(pointertype == DW_POINTER_ARROW)
      gtk_widget_set_cursor_from_name(GTK_WIDGET(handle), "default");
   else if(pointertype == DW_POINTER_CLOCK)
      gtk_widget_set_cursor_from_name(GTK_WIDGET(handle), "wait");
   else if(pointertype == DW_POINTER_QUESTION)
      gtk_widget_set_cursor_from_name(GTK_WIDGET(handle), "help");
   else
      gtk_widget_set_cursor(GTK_WIDGET(handle), NULL);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
#ifndef GDK_WINDOWING_X11
void API dw_window_capture(HWND handle)
{
}
#else
static Display *_DWXGrabbedDisplay = NULL;

DW_FUNCTION_DEFINITION(dw_window_capture, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_capture)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   GdkDisplay *display = gdk_display_get_default();

   if(_DWXGrabbedDisplay == NULL && handle && GTK_IS_WINDOW(handle) && display && GDK_IS_X11_DISPLAY(display))
   {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(handle));
      
      if(surface)
      {
         if(XGrabPointer(GDK_SURFACE_XDISPLAY(surface), GDK_SURFACE_XID(surface), FALSE,
                         ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask,
                         GrabModeAsync,  GrabModeAsync, None, None, CurrentTime) == GrabSuccess)
            _DWXGrabbedDisplay = GDK_SURFACE_XDISPLAY(surface);
            
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}
#endif

/*
 * Releases previous mouse capture.
 */
#ifndef GDK_WINDOWING_X11
void API dw_window_release(void)
{
}
#else
DW_FUNCTION_DEFINITION(dw_window_release, void)
DW_FUNCTION_ADD_PARAM
DW_FUNCTION_NO_RETURN(dw_window_release)
{
   /* Don't need X11 test, _DWXGrabbedDisplay won't get set unless X11 */
   if(_DWXGrabbedDisplay)
   {
      XUngrabPointer(_DWXGrabbedDisplay, CurrentTime);
      _DWXGrabbedDisplay = NULL;
   }
   DW_FUNCTION_RETURN_NOTHING;
}
#endif

/* Window creation flags that will cause the window to have decorations */
#define _DW_DECORATION_FLAGS (DW_FCF_CLOSEBUTTON|DW_FCF_SYSMENU|DW_FCF_TITLEBAR|DW_FCF_MINMAX|DW_FCF_SIZEBORDER|DW_FCF_BORDER|DW_FCF_DLGBORDER)

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
DW_FUNCTION_DEFINITION(dw_window_new, HWND, DW_UNUSED(HWND hwndOwner), const char *title, ULONG flStyle)
DW_FUNCTION_ADD_PARAM3(hwndOwner, title, flStyle)
DW_FUNCTION_RETURN(dw_window_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(hwndOwner), HWND, title, char *, flStyle, ULONG)
{
   GtkWidget *box = dw_box_new(DW_VERT, 0);
   GtkWidget *grid = gtk_grid_new();
   GtkWidget *tmp = gtk_window_new();

   gtk_widget_set_visible(grid, TRUE);

   /* Handle the window style flags */
   gtk_window_set_title(GTK_WINDOW(tmp), title);
   gtk_window_set_resizable(GTK_WINDOW(tmp), (flStyle & DW_FCF_SIZEBORDER) ? TRUE : FALSE);
   /* Either the CLOSEBUTTON or SYSMENU flags should make it deletable */
   gtk_window_set_deletable(GTK_WINDOW(tmp), (flStyle & (DW_FCF_CLOSEBUTTON | DW_FCF_SYSMENU)) ? TRUE : FALSE);
   gtk_window_set_decorated(GTK_WINDOW(tmp), (flStyle & _DW_DECORATION_FLAGS) ? TRUE : FALSE);

   gtk_widget_realize(tmp);

   if(flStyle & DW_FCF_FULLSCREEN)
      gtk_window_fullscreen(GTK_WINDOW(tmp));
   else
   {
      if(flStyle & DW_FCF_MAXIMIZE)
         gtk_window_maximize(GTK_WINDOW(tmp));

      if(flStyle & DW_FCF_MINIMIZE)
         gtk_window_minimize(GTK_WINDOW(tmp));
   }

   gtk_grid_attach(GTK_GRID(grid), box, 0, 1, 1, 1);
   gtk_window_set_child(GTK_WINDOW(tmp), grid);
   g_object_set_data(G_OBJECT(tmp), "_dw_boxhandle", (gpointer)box);
   g_object_set_data(G_OBJECT(tmp), "_dw_grid", (gpointer)grid);
   g_object_set_data(G_OBJECT(tmp), "_dw_style", GINT_TO_POINTER(flStyle));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
DW_FUNCTION_DEFINITION(dw_box_new, HWND, int type, int pad)
DW_FUNCTION_ADD_PARAM2(type, pad)
DW_FUNCTION_RETURN(dw_box_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(type, int, pad, int)
{
   GtkWidget *tmp = gtk_grid_new();
   g_object_set_data(G_OBJECT(tmp), "_dw_boxtype", GINT_TO_POINTER(type));
   _dw_widget_set_pad(tmp, pad);
   gtk_widget_set_visible(tmp, TRUE);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_new, HWND, int type, int pad)
DW_FUNCTION_ADD_PARAM2(type, pad)
DW_FUNCTION_RETURN(dw_scrollbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(type, int, pad, int)
{
   GtkWidget *tmp, *box;

   tmp = gtk_scrolled_window_new();
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmp), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

   box = gtk_grid_new();

   g_object_set_data(G_OBJECT(box), "_dw_boxtype", GINT_TO_POINTER(type));
   g_object_set_data(G_OBJECT(tmp), "_dw_boxhandle", (gpointer)box);
   _dw_widget_set_pad(box, pad);
   
   gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tmp), box);
   g_object_set_data(G_OBJECT(tmp), "_dw_user", box);
   gtk_widget_set_visible(box, TRUE);
   gtk_widget_set_visible(tmp, TRUE);

   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_get_pos, int, HWND handle, int orient)
DW_FUNCTION_ADD_PARAM2(handle, orient)
DW_FUNCTION_RETURN(dw_scrollbox_get_pos, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, orient, int)
{
   int val = DW_ERROR_UNKNOWN;
   GtkAdjustment *adjustment;

   if(handle)
   {
      if(orient == DW_HORZ)
         adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(handle));
      else
         adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(handle));
      if(adjustment)
         val = _dw_round_value(gtk_adjustment_get_value(adjustment));
   }
   DW_FUNCTION_RETURN_THIS(val);
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
DW_FUNCTION_DEFINITION(dw_scrollbox_get_range, int, HWND handle, int orient)
DW_FUNCTION_ADD_PARAM2(handle, orient)
DW_FUNCTION_RETURN(dw_scrollbox_get_range, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, orient, int)
{
   int range = DW_ERROR_UNKNOWN;
   GtkAdjustment *adjustment;

   if(handle)
   {
      if(orient == DW_HORZ)
         adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(handle));
      else
         adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(handle));
      if(adjustment)
         range = _dw_round_value(gtk_adjustment_get_upper(adjustment));
   }
   DW_FUNCTION_RETURN_THIS(range);
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 */
DW_FUNCTION_DEFINITION(dw_groupbox_new, HWND, int type, int pad, const char *title)
DW_FUNCTION_ADD_PARAM3(type, pad, title)
DW_FUNCTION_RETURN(dw_groupbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(type, int, pad, int, title, const char *)
{
   GtkWidget *tmp, *frame = gtk_frame_new(NULL);
   gtk_frame_set_label(GTK_FRAME(frame), title && *title ? title : NULL);
   
   tmp = gtk_grid_new();
   g_object_set_data(G_OBJECT(tmp), "_dw_boxtype", GINT_TO_POINTER(type));
   g_object_set_data(G_OBJECT(frame), "_dw_boxhandle", (gpointer)tmp);
   _dw_widget_set_pad(tmp, pad);
   gtk_frame_set_child(GTK_FRAME(frame), tmp);
   gtk_widget_set_visible(tmp, TRUE);
   gtk_widget_set_visible(frame, TRUE);
   if(_DWDefaultFont)
      dw_window_set_font(frame, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(frame);
}

/*
 * Create a bitmap object to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_bitmap_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_bitmap_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GtkWidget *tmp = gtk_picture_new();
   gtk_picture_set_can_shrink(GTK_PICTURE(tmp), TRUE);
#if GTK_CHECK_VERSION(4,8,0)
   gtk_picture_set_content_fit(GTK_PICTURE(tmp), GTK_CONTENT_FIT_CONTAIN);
#else
   gtk_picture_set_keep_aspect_ratio(GTK_PICTURE(tmp), TRUE);
#endif
   gtk_widget_set_halign(GTK_WIDGET(tmp), GTK_ALIGN_CENTER);
   gtk_widget_set_valign(GTK_WIDGET(tmp), GTK_ALIGN_CENTER);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
DW_FUNCTION_DEFINITION(dw_notebook_new, HWND, ULONG cid, int top)
DW_FUNCTION_ADD_PARAM2(cid, top)
DW_FUNCTION_RETURN(dw_notebook_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, top, int)
{
   GtkWidget *tmp, **pagearray = calloc(sizeof(GtkWidget *), 256);

   tmp = gtk_notebook_new();
   if(top)
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tmp), GTK_POS_TOP);
   else
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tmp), GTK_POS_BOTTOM);
   gtk_notebook_set_scrollable(GTK_NOTEBOOK(tmp), TRUE);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   g_object_set_data(G_OBJECT(tmp), "_dw_pagearray", (gpointer)pagearray);
   DW_FUNCTION_RETURN_THIS(tmp);
}

static unsigned int _dw_menugroup = 0;

/* Recurse into a menu setting the action groups on the menuparent widget */
void _dw_menu_set_group_recursive(HMENUI start, GtkWidget *menuparent)
{
   int z, submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(start), "_dw_submenucount"));
   
   for(z=0;z<submenucount;z++)
   {
      char tempbuf[101] = {0};
      HMENUI submenu;

      snprintf(tempbuf, 100, "_dw_submenu%d", z);

      if((submenu = g_object_get_data(G_OBJECT(start), tempbuf)))
      {
         if(!g_object_get_data(G_OBJECT(submenu), "_dw_menuparent"))
         {
            int menugroup = DW_POINTER_TO_INT(g_object_get_data(G_OBJECT(submenu), "_dw_menugroup"));
            GSimpleActionGroup *group = g_object_get_data(G_OBJECT(submenu), "_dw_group");
            char tempbuf[25] = {0};
               
            snprintf(tempbuf, 24, "menu%d", menugroup);
            
            gtk_widget_insert_action_group(GTK_WIDGET(menuparent), tempbuf, G_ACTION_GROUP(group));
            g_object_set_data(G_OBJECT(submenu), "_dw_menuparent", (gpointer)menuparent);
         }
         _dw_menu_set_group_recursive(submenu, menuparent);
      }
   }
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
DW_FUNCTION_DEFINITION(dw_menu_new, HMENUI, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_menu_new, HMENUI)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GMenu *tmp = g_menu_new();
   /* Create the initial section and add it to the menu */
   GMenu *section = g_menu_new();
   GMenuItem *item = g_menu_item_new_section(NULL, G_MENU_MODEL(section));
   GSimpleActionGroup *group = g_simple_action_group_new();
      
   g_menu_append_item(tmp, item);

   g_object_set_data(G_OBJECT(tmp), "_dw_menugroup", GINT_TO_POINTER(++_dw_menugroup));
   g_object_set_data(G_OBJECT(tmp), "_dw_group", (gpointer)group);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   g_object_set_data(G_OBJECT(tmp), "_dw_section", (gpointer)section);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 * If there is no box already packed into the "location", the menu will not appear
 * so tell the user.
 */
DW_FUNCTION_DEFINITION(dw_menubar_new, HMENUI, HWND location)
DW_FUNCTION_ADD_PARAM1(location)
DW_FUNCTION_RETURN(dw_menubar_new, HMENUI)
DW_FUNCTION_RESTORE_PARAM1(location, HWND)
{
   HMENUI tmp = 0;
   GtkWidget *box;

   if(GTK_IS_WINDOW(location) &&
      (box = GTK_WIDGET(g_object_get_data(G_OBJECT(location), "_dw_grid"))))
   {
      /* If there is an existing menu bar, remove it */
      GtkWidget *oldmenu = GTK_WIDGET(g_object_get_data(G_OBJECT(location), "_dw_menubar"));
      GMenu *menu = g_menu_new();
      /* Create the initial section and add it to the menu */
      GMenu *section = g_menu_new();
      GMenuItem *item = g_menu_item_new_section(NULL, G_MENU_MODEL(section));
      GSimpleActionGroup *group = g_simple_action_group_new();
      char tempbuf[25] = {0};

      g_menu_append_item(menu, item);
      
      if(oldmenu && GTK_IS_WIDGET(oldmenu))
         gtk_grid_remove(GTK_GRID(box), tmp);
         
      /* Create a new menu bar */
      tmp = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menu));
      snprintf(tempbuf, 24, "menu%d", ++_dw_menugroup);
      gtk_widget_insert_action_group(GTK_WIDGET(tmp), tempbuf, G_ACTION_GROUP(group));
      gtk_widget_set_visible(tmp, TRUE);

      /* Save pointers to each other */
      g_object_set_data(G_OBJECT(location), "_dw_menubar", (gpointer)tmp);
      g_object_set_data(G_OBJECT(tmp), "_dw_window", (gpointer)location);
      g_object_set_data(G_OBJECT(tmp), "_dw_menugroup", GINT_TO_POINTER(_dw_menugroup));
      g_object_set_data(G_OBJECT(tmp), "_dw_group", (gpointer)group);
      g_object_set_data(G_OBJECT(tmp), "_dw_section", (gpointer)section);
      gtk_grid_attach(GTK_GRID(box), tmp, 0, 0, 1, 1);
      _dw_menu_set_group_recursive(tmp, GTK_WIDGET(tmp));
   }
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
DW_FUNCTION_DEFINITION(dw_menu_destroy, void, HMENUI *menu)
DW_FUNCTION_ADD_PARAM1(menu)
DW_FUNCTION_NO_RETURN(dw_menu_destroy)
DW_FUNCTION_RESTORE_PARAM1(menu, HMENUI *)
{
   if(menu && *menu)
   {
      GtkWidget *window = NULL;

      /* If it is attached to a window, try to delete the reference to it */
      if((GTK_IS_POPOVER_MENU_BAR(*menu) || GTK_IS_POPOVER_MENU(*menu)) &&
         (window = GTK_WIDGET(g_object_get_data(G_OBJECT(*menu), "_dw_window"))))
      {
            if(GTK_IS_POPOVER_MENU_BAR(*menu))
               g_object_set_data(G_OBJECT(window), "_dw_menubar", NULL);
            else
               g_object_set_data(G_OBJECT(window), "_dw_menu_popup", NULL);
      }
      /* Actually destroy the menu */
      if(GTK_IS_WIDGET(*menu) && window)
      {
         GtkWidget *box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "_dw_grid"));

         if(box && GTK_IS_GRID(box))
            gtk_grid_remove(GTK_GRID(box), GTK_WIDGET(*menu));
         else
            g_object_unref(G_OBJECT(*menu));
      }
      else if(G_IS_MENU(*menu))
         g_object_unref(G_OBJECT(*menu));
      *menu = NULL;
   }
   DW_FUNCTION_RETURN_NOTHING;
}

char _dw_removetilde(char *dest, const char *src)
{
   int z, cur=0;
   char accel = '\0';

   for(z=0;z<strlen(src);z++)
   {
      if(src[z] == '~')
      {
         dest[cur] = '_';
         accel = src[z+1];
      }
      else
         dest[cur] = src[z];
      cur++;
   }
   dest[cur] = 0;
   return accel;
}


/*
 * Adds a menuitem or submenu to an existing menu.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
DW_FUNCTION_DEFINITION(dw_menu_append_item, HWND, HMENUI menu, const char *title, unsigned long id, unsigned long flags, int end, DW_UNUSED(int check), HMENUI submenu)
DW_FUNCTION_ADD_PARAM7(menu, title, id, flags, end, check, submenu)
DW_FUNCTION_RETURN(dw_menu_append_item, HWND)
DW_FUNCTION_RESTORE_PARAM7(menu, HMENUI, title, const char *, id, unsigned long, flags, unsigned long, end, int, DW_UNUSED(check), int, submenu, HMENUI)
{
   GSimpleAction *action = NULL;
   HWND tmphandle = NULL;
   GMenuModel *menumodel;
   char *temptitle = alloca(strlen(title)+1);

   if(menu)
   {
      /* By default we add to the menu's current section */
      menumodel = g_object_get_data(G_OBJECT(menu), "_dw_section");
      _dw_removetilde(temptitle, title);

      /* To add a separator we create a new section and add it */
      if (strlen(temptitle) == 0)
      {
         GMenu *section = g_menu_new();

         /* If we are creating a new section, add it to the core menu... not the section */
         if(GTK_IS_POPOVER_MENU_BAR(menu))
            menumodel = gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(menu));
         else
            menumodel = G_MENU_MODEL(menu);

         tmphandle = (HWND)g_menu_item_new_section(NULL, G_MENU_MODEL(section));
         g_object_set_data(G_OBJECT(menu), "_dw_section", (gpointer)section);
      }
      else
      {
         char tempbuf[101] = {0};

         if(submenu)
         {
            if(G_IS_MENU(submenu))
            {
               int submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "_dw_submenucount"));
               GtkWidget *menuparent = GTK_WIDGET(g_object_get_data(G_OBJECT(menu), "_dw_menuparent"));

               /* If the menu being added to is a menu bar, that is the menuparent for submenus */            
               if(GTK_IS_POPOVER_MENU_BAR(menu))
                  menuparent = GTK_WIDGET(menu);

               snprintf(tempbuf, 100, "_dw_submenu%d", submenucount);
               submenucount++;
               tmphandle = (HWND)g_menu_item_new_submenu(temptitle, G_MENU_MODEL(submenu));
               g_object_set_data(G_OBJECT(menu), tempbuf, (gpointer)submenu);
               g_object_set_data(G_OBJECT(menu), "_dw_submenucount", GINT_TO_POINTER(submenucount));

               /* If we have a menu parent, use it to create the groups if needed */
               if(menuparent && !g_object_get_data(G_OBJECT(submenu), "_dw_menuparent"))
                  _dw_menu_set_group_recursive(menu, menuparent);
            }
         }
         else
         {
            char numbuf[25] = {0};
            GSimpleActionGroup *group = g_object_get_data(G_OBJECT(menu), "_dw_group");
            int menugroup = DW_POINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "_dw_menugroup"));
            char *actionname;

            /* Code to autogenerate a menu ID if not specified or invalid
             * First pool is smaller for transient popup menus
             */
            if(id == (ULONG)-1)
            {
               static ULONG tempid = 60000;

               tempid++;
               id = tempid;

               if(tempid > 65500)
                  tempid = 60000;
            }
            /* Second pool is larger for more static windows */
            else if(!id || id >= 30000)
            {
               static ULONG menuid = 30000;

               menuid++;
               if(menuid > 60000)
                  menuid = 30000;
               id = menuid;
            }

            snprintf(tempbuf, 100, "menu%d.action%lu", menugroup, id);
            actionname = strchr(tempbuf, '.');
            if(check)
               action = g_simple_action_new_stateful(&actionname[1], NULL, g_variant_new_boolean (FALSE));
            else
               action = g_simple_action_new(&actionname[1], NULL);
            g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
            tmphandle=(HWND)g_menu_item_new(temptitle, tempbuf);
            snprintf(numbuf, 24, "%lu", id);
            g_object_set_data(G_OBJECT(menu), numbuf, (gpointer)tmphandle);
            g_object_set_data(G_OBJECT(tmphandle), "_dw_action", (gpointer)action);
         }
      }

      if(end)
         g_menu_append_item(G_MENU(menumodel), G_MENU_ITEM(tmphandle));
      else
         g_menu_prepend_item(G_MENU(menumodel), G_MENU_ITEM(tmphandle));

      g_object_set_data(G_OBJECT(tmphandle), "_dw_id", GINT_TO_POINTER(id));
      
      if(action)
         g_simple_action_set_enabled(action, (flags & DW_MIS_DISABLED) ? FALSE : TRUE);
   }
   DW_FUNCTION_RETURN_THIS(tmphandle);
}

GMenuItem *_dw_find_submenu_id(HMENUI start, const char *name)
{
   GMenuItem *tmp;
   int z, submenucount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(start), "_dw_submenucount"));

   if((tmp = g_object_get_data(G_OBJECT(start), name)))
      return tmp;

   for(z=0;z<submenucount;z++)
   {
      char tempbuf[101] = {0};
      GMenuItem *menuitem;
      HMENUI submenu;

      snprintf(tempbuf, 100, "_dw_submenu%d", z);

      if((submenu = g_object_get_data(G_OBJECT(start), tempbuf)))
      {
         if((menuitem = _dw_find_submenu_id(submenu, name)))
            return menuitem;
      }
   }
   return NULL;
}

/*
 * Sets the state of a menu item check.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 * deprecated: use dw_menu_item_set_state()
 */
DW_FUNCTION_DEFINITION(dw_menu_item_set_check, void, HMENUI menu, ULONG cid, int check)
DW_FUNCTION_ADD_PARAM3(menu, cid, check)
DW_FUNCTION_NO_RETURN(dw_menu_item_set_check)
DW_FUNCTION_RESTORE_PARAM3(menu, HMENUI, cid, ULONG, check, int)
{
   if(menu)
   {
      char numbuf[25] = {0};
      GMenuItem *tmphandle;

      snprintf(numbuf, 24, "%lu", cid);
      tmphandle = _dw_find_submenu_id(menu, numbuf);

      if(tmphandle && G_IS_MENU_ITEM(tmphandle))
      {
         GSimpleAction *action = g_object_get_data(G_OBJECT(tmphandle), "_dw_action");
         
         if(action)
         {
            GVariant *action_state = g_action_get_state(G_ACTION(action));
            gboolean thischeck = check;

            if(!action_state || (g_variant_get_boolean(action_state) != thischeck))
            {
               GVariant *new_state = g_variant_new_boolean(thischeck);
               g_simple_action_set_state(action, new_state);
            }
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       state: TRUE for checked FALSE for not checked.
 */
DW_FUNCTION_DEFINITION(dw_menu_item_set_state, void, HMENUI menu, ULONG cid, ULONG state)
DW_FUNCTION_ADD_PARAM3(menu, cid, state)
DW_FUNCTION_NO_RETURN(dw_menu_item_set_state)
DW_FUNCTION_RESTORE_PARAM3(menu, HMENUI, cid, ULONG, state, ULONG)
{
   if(menu)
   {
      char numbuf[25] = {0};
      GMenuItem *tmphandle;

      snprintf(numbuf, 24, "%lu", cid);
      tmphandle = _dw_find_submenu_id(menu, numbuf);

      if(tmphandle && G_IS_MENU_ITEM(tmphandle))
      {
         GSimpleAction *action = g_object_get_data(G_OBJECT(tmphandle), "_dw_action");
         
         if(action)
         {
            if((state & DW_MIS_CHECKED) || (state & DW_MIS_UNCHECKED))
            {
               GVariant *action_state = g_action_get_state(G_ACTION(action));
               gboolean check = false;

               if(state & DW_MIS_CHECKED)
                  check = true;

               if(!action_state || (g_variant_get_boolean(action_state) != check))
               {
                  GVariant *new_state = g_variant_new_boolean(check);
                  g_simple_action_set_state(action, new_state);
               }
            }
            if((state & DW_MIS_ENABLED) || (state & DW_MIS_DISABLED))
            {
               if(state & DW_MIS_ENABLED)
                  g_simple_action_set_enabled(action, TRUE);
               else
                  g_simple_action_set_enabled(action, FALSE);
            }
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
DW_FUNCTION_DEFINITION(dw_menu_delete_item, int, HMENUI menu, ULONG cid)
DW_FUNCTION_ADD_PARAM2(menu, cid)
DW_FUNCTION_RETURN(dw_menu_delete_item, int)
DW_FUNCTION_RESTORE_PARAM2(menu, HMENUI, cid, ULONG)
{
   int ret = DW_ERROR_UNKNOWN;
   char numbuf[25] = {0};
   GMenuItem *tmphandle;

   if(menu)
   {
      snprintf(numbuf, 24, "%lu", cid);
      tmphandle = _dw_find_submenu_id(menu, numbuf);

      if(tmphandle && G_IS_MENU_ITEM(tmphandle))
      {
         /* g_menu_remove(menu, position); */
         g_object_unref(G_OBJECT(tmphandle));
         g_object_set_data(G_OBJECT(menu), numbuf, NULL);
         ret = DW_ERROR_NONE;
      }
   }
   DW_FUNCTION_RETURN_THIS(ret);
}

/* Delayed unparent of the popup menu from the parent */
gboolean _dw_idle_popover_unparent(gpointer data)
{
   GtkWidget *self = GTK_WIDGET(data);
   GtkWidget *box, *window = g_object_get_data(G_OBJECT(self), "_dw_window");

   if(window && GTK_IS_WINDOW(window) && 
      (box = g_object_get_data(G_OBJECT(window), "_dw_grid")) && GTK_IS_GRID(box))
      gtk_grid_remove(GTK_GRID(box), self);
   else
      gtk_widget_unparent(self);
   return false;
}

void _dw_popover_menu_closed(GtkPopover *self, gpointer data)
{
   GtkWidget *parent = GTK_WIDGET(data);
   
   /* Can't unparent immediately, since the "activate" signal happens second...
    * so we have to delay unparenting until the "activate" handler runs.
    */
   if(GTK_IS_WIDGET(parent) && GTK_IS_POPOVER(self))
      g_idle_add(G_SOURCE_FUNC(_dw_idle_popover_unparent), (gpointer)self);
}

/*
 * Pops up a context menu at given x and y coordinates.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       parent: Handle to the window initiating the popup.
 *       x: X coordinate.
 *       y: Y coordinate.
 */
DW_FUNCTION_DEFINITION(dw_menu_popup, void, HMENUI *menu, HWND parent, int x, int y)
DW_FUNCTION_ADD_PARAM4(menu, parent, x, y)
DW_FUNCTION_NO_RETURN(dw_menu_popup)
DW_FUNCTION_RESTORE_PARAM4(menu, HMENUI *, parent, HWND, x, int, y, int)
{
   if(menu && *menu && G_MENU(*menu))
   {
      GtkWidget *tmp = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(*menu), GTK_POPOVER_MENU_NESTED);
      GdkRectangle rect = { x, y, 1, 1 };

      if(GTK_IS_WINDOW(parent))
      {
         GtkWidget *box = g_object_get_data(G_OBJECT(parent), "_dw_grid");
         
         if(box && GTK_IS_GRID(box))
         {
            gtk_grid_attach(GTK_GRID(box), tmp, 65535, 65535, 1, 1);
            g_object_set_data(G_OBJECT(tmp), "_dw_window", (gpointer)parent);
            g_object_set_data(G_OBJECT(parent), "_dw_menu_popup", (gpointer)tmp);
         }
      }
      else
         gtk_widget_set_parent(tmp, GTK_WIDGET(parent));

      if(!g_object_get_data(G_OBJECT(*menu), "_dw_menuparent"))
      {
         int menugroup = DW_POINTER_TO_INT(g_object_get_data(G_OBJECT(*menu), "_dw_menugroup"));
         GSimpleActionGroup *group = g_object_get_data(G_OBJECT(*menu), "_dw_group");
         char tempbuf[25] = {0};
            
         snprintf(tempbuf, 24, "menu%d", menugroup);
         
         gtk_widget_insert_action_group(GTK_WIDGET(tmp), tempbuf, G_ACTION_GROUP(group));
         g_object_set_data(G_OBJECT(*menu), "_dw_menuparent", (gpointer)tmp);
      }
      _dw_menu_set_group_recursive(*menu, GTK_WIDGET(tmp));
      gtk_popover_set_autohide(GTK_POPOVER(tmp), TRUE);
      gtk_popover_set_has_arrow (GTK_POPOVER(tmp), FALSE);
      gtk_popover_set_pointing_to(GTK_POPOVER(tmp), &rect);
      g_signal_connect(G_OBJECT(tmp), "closed", G_CALLBACK(_dw_popover_menu_closed), (gpointer)parent);
      gtk_popover_popup(GTK_POPOVER(tmp));
      *menu = NULL;
   }
   DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
   if(x)
      *x = (long)_dw_mouse_last_x;
   if(y)
      *y = (long)_dw_mouse_last_y;
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
#ifndef GDK_WINDOWING_X11
void API dw_pointer_set_pos(long x, long y)
{
}
#else
DW_FUNCTION_DEFINITION(dw_pointer_set_pos, void, long x, long y)
DW_FUNCTION_ADD_PARAM2(x, y)
DW_FUNCTION_NO_RETURN(dw_pointer_set_pos)
DW_FUNCTION_RESTORE_PARAM2(x, long, y, long)
{
   GdkDisplay *display = gdk_display_get_default();
   
   if(display && GDK_IS_X11_DISPLAY(display))
   {
      Display *xdisplay = gdk_x11_display_get_xdisplay(display);
      Window xrootwin = gdk_x11_display_get_xrootwindow(display);
   
      if(xdisplay)
         XWarpPointer(xdisplay, None, xrootwin, 0, 0, 0, 0, (int)x, (int)y);
   }
   DW_FUNCTION_RETURN_NOTHING;
}
#endif

#define _DW_TREE_CONTAINER 1
#define _DW_TREE_TREE      2
#define _DW_TREE_LISTBOX   3

GtkWidget *_dw_tree_create(unsigned long id)
{
   GtkWidget *tmp;

   tmp = gtk_scrolled_window_new();
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (tmp),
               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(id));
   gtk_widget_set_visible(tmp, TRUE);
   return tmp;
}

GtkWidget *_dw_tree_view_setup(GtkWidget *tmp, GtkTreeModel *store)
{
   GtkWidget *tree = gtk_tree_view_new_with_model(store);
   gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);
   gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tmp), tree);
   g_object_set_data(G_OBJECT(tmp), "_dw_user", (gpointer)tree);
   return tree;
}

/*
 * Create a container object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
DW_FUNCTION_DEFINITION(dw_container_new, HWND, ULONG cid, int multi)
DW_FUNCTION_ADD_PARAM2(cid, multi)
DW_FUNCTION_RETURN(dw_container_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, multi, int)
{
   GtkWidget *tmp;

   if((tmp = _dw_tree_create(cid)))
   {
      g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER));
      g_object_set_data(G_OBJECT(tmp), "_dw_multi_sel", GINT_TO_POINTER(multi));
   }
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a tree object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
DW_FUNCTION_DEFINITION(dw_tree_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_tree_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GtkWidget *tmp, *tree;
   GtkTreeStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;

   if((tmp = _dw_tree_create(cid)))
   {
      store = gtk_tree_store_new(4, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_POINTER, G_TYPE_POINTER);
      tree = _dw_tree_view_setup(tmp, GTK_TREE_MODEL(store));
      g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_TREE));
      g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_TREE));
      col = gtk_tree_view_column_new();

      rend = gtk_cell_renderer_pixbuf_new();
      gtk_tree_view_column_pack_start(col, rend, FALSE);
      gtk_tree_view_column_add_attribute(col, rend, "pixbuf", 1);
      rend = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, rend, TRUE);
      gtk_tree_view_column_add_attribute(col, rend, "text", 0);

      gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
      gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree), col);
      gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
      gtk_widget_set_visible(tree, TRUE);

      if(_DWDefaultFont)
         dw_window_set_font(tmp, _DWDefaultFont);
   }
   DW_FUNCTION_RETURN_THIS(tmp);
}


/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_text_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_text_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp = gtk_label_new(text);

   /* Left and centered */
   gtk_label_set_xalign(GTK_LABEL(tmp), 0.0f);
   gtk_label_set_yalign(GTK_LABEL(tmp), 0.5f);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_status_text_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_status_text_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp, *frame = gtk_frame_new(NULL);
   tmp = gtk_label_new(text);
   gtk_frame_set_child(GTK_FRAME(frame), tmp);
   gtk_widget_set_visible(tmp, TRUE);
   gtk_widget_set_visible(frame, TRUE);

   /* Left and centered */
   gtk_label_set_xalign(GTK_LABEL(tmp), 0.0f);
   gtk_label_set_yalign(GTK_LABEL(tmp), 0.5f);
   g_object_set_data(G_OBJECT(frame), "_dw_id", GINT_TO_POINTER(cid));
   g_object_set_data(G_OBJECT(frame), "_dw_label", (gpointer)tmp);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(frame);
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_mle_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_mle_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GtkWidget *tmp, *tmpbox = gtk_scrolled_window_new();
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tmpbox),
               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
   tmp = gtk_text_view_new();
   gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tmpbox), tmp);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), GTK_WRAP_WORD);

   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   g_object_set_data(G_OBJECT(tmpbox), "_dw_user", (gpointer)tmp);
   gtk_widget_set_visible(tmp, TRUE);
   gtk_widget_set_visible(tmpbox, TRUE);
   if(_DWDefaultFont)
      dw_window_set_font(tmpbox, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmpbox);
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_entryfield_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkEntryBuffer *buffer = gtk_entry_buffer_new(text, -1);
   GtkWidget *tmp = gtk_entry_new_with_buffer(buffer);

   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));

    if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_password_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_entryfield_password_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkEntryBuffer *buffer = gtk_entry_buffer_new(text, -1);
   GtkWidget *tmp = gtk_entry_new_with_buffer(buffer);

   gtk_entry_set_visibility(GTK_ENTRY(tmp), FALSE);

   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));

   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new Combobox window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the combpbox widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_combobox_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_combobox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp;
   GtkEntryBuffer *buffer;
   GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
   tmp = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
   gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(tmp), 0);
   buffer = gtk_entry_get_buffer(GTK_ENTRY(gtk_combo_box_get_child(GTK_COMBO_BOX(tmp))));
   gtk_entry_buffer_set_max_length(buffer, 0);
   gtk_entry_buffer_set_text(buffer, text, -1);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_COMBOBOX));
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_button_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_button_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp = gtk_button_new_with_label(text);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_bitmapbutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp = gtk_button_new();
   GtkWidget *bitmap = dw_bitmap_new(cid);

   if(bitmap)
   {
      dw_window_set_bitmap(bitmap, cid, NULL);
      gtk_button_set_child(GTK_BUTTON(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_set_visible(tmp, TRUE);
   if(text)
      gtk_widget_set_tooltip_text(tmp, text);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       label_text: Text to display on button. TBD when Windows works
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 */
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new_from_file, HWND, const char *text, ULONG cid, const char *filename)
DW_FUNCTION_ADD_PARAM3(text, cid, filename)
DW_FUNCTION_RETURN(dw_bitmapbutton_new_from_file, HWND)
DW_FUNCTION_RESTORE_PARAM3(text, const char *, cid, ULONG, filename, const char *)
{
   GtkWidget *tmp = gtk_button_new();
   GtkWidget *bitmap = dw_bitmap_new(cid);

   if(bitmap)
   {
      dw_window_set_bitmap(bitmap, 0, filename);
      gtk_button_set_child(GTK_BUTTON(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_set_visible(tmp, TRUE);
   if(text)
      gtk_widget_set_tooltip_text(tmp, text);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: Raw data of image.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       len:  Length of raw data
 */
DW_FUNCTION_DEFINITION(dw_bitmapbutton_new_from_data, HWND, const char *text, ULONG cid, const char *data, int len)
DW_FUNCTION_ADD_PARAM4(text, cid, data, len)
DW_FUNCTION_RETURN(dw_bitmapbutton_new_from_data, HWND)
DW_FUNCTION_RESTORE_PARAM4(text, const char *, cid, ULONG, data, const char *, len, int)
{
   GtkWidget *tmp = gtk_button_new();
   GtkWidget *bitmap = dw_bitmap_new(cid);

   if(bitmap)
   {
      dw_window_set_bitmap_from_data(bitmap, 0, data, len);
      gtk_button_set_child(GTK_BUTTON(tmp), bitmap);
      g_object_set_data(G_OBJECT(tmp), "_dw_bitmap", bitmap);
   }
   gtk_widget_set_visible(tmp, TRUE);
   if(text)
      gtk_widget_set_tooltip_text(tmp, text);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_spinbutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new((float)atoi(text), -65536.0, 65536.0, 1.0, 5.0, 0.0);
   GtkWidget *tmp = gtk_spin_button_new(adj, 0, 0);

   gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tmp), TRUE);
   gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(tmp), TRUE);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adj);
   g_object_set_data(G_OBJECT(adj), "_dw_spinbutton", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_radiobutton_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_radiobutton_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp = gtk_toggle_button_new_with_label(text);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   gtk_widget_set_visible(tmp, TRUE);

   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_slider_new, HWND, int vertical, int increments, ULONG cid)
DW_FUNCTION_ADD_PARAM3(vertical, increments, cid)
DW_FUNCTION_RETURN(dw_slider_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(vertical, int, increments, int, cid, ULONG)

{
   GtkAdjustment *adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, (gfloat)increments, 1, 1, 1);
   GtkWidget *tmp = gtk_scale_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, adjustment);

   gtk_widget_set_visible(tmp, TRUE);
   gtk_scale_set_draw_value(GTK_SCALE(tmp), 0);
   gtk_scale_set_digits(GTK_SCALE(tmp), 0);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adjustment);
   g_object_set_data(G_OBJECT(adjustment), "_dw_slider", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_scrollbar_new, HWND, int vertical, ULONG cid)
DW_FUNCTION_ADD_PARAM2(vertical, cid)
DW_FUNCTION_RETURN(dw_scrollbar_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(vertical, int, cid, ULONG)
{
   GtkAdjustment *adjustment = (GtkAdjustment *)gtk_adjustment_new(0, 0, 0, 1, 1, 1);
   GtkWidget *tmp = gtk_scrollbar_new(vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, adjustment);

   gtk_widget_set_can_focus(tmp, FALSE);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_adjustment", (gpointer)adjustment);
   g_object_set_data(G_OBJECT(adjustment), "_dw_scrollbar", (gpointer)tmp);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_percent_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_percent_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)

{
   GtkWidget *tmp = gtk_progress_bar_new();
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_new, HWND, const char *text, ULONG cid)
DW_FUNCTION_ADD_PARAM2(text, cid)
DW_FUNCTION_RETURN(dw_checkbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(text, const char *, cid, ULONG)
{
   GtkWidget *tmp = gtk_check_button_new_with_label(text);
   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Create a new listbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       multi: Multiple select TRUE or FALSE.
 */
DW_FUNCTION_DEFINITION(dw_listbox_new, HWND, ULONG cid, int multi)
DW_FUNCTION_ADD_PARAM2(cid, multi)
DW_FUNCTION_RETURN(dw_listbox_new, HWND)
DW_FUNCTION_RESTORE_PARAM2(cid, ULONG, multi, int)
{
   GtkWidget *tmp, *tree;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;

   if((tmp = _dw_tree_create(cid)))
   {
      store = gtk_list_store_new(1, G_TYPE_STRING);
      tree = _dw_tree_view_setup(tmp, GTK_TREE_MODEL(store));
      g_object_set_data(G_OBJECT(tmp), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX));
      g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX));

      col = gtk_tree_view_column_new();
      rend = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, rend, TRUE);
      gtk_tree_view_column_add_attribute(col, rend, "text", 0);

      gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
      gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree), col);
      gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
      if(multi)
         gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
      else
         gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
      gtk_widget_set_visible(tree, TRUE);
      if(_DWDefaultFont)
         dw_window_set_font(tmp, _DWDefaultFont);
   }
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
DW_FUNCTION_DEFINITION(dw_window_set_icon, void, HWND handle, HICN icon)
DW_FUNCTION_ADD_PARAM2(handle, icon)
DW_FUNCTION_NO_RETURN(dw_window_set_icon)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, icon, HICN)
{
   if(handle && GTK_IS_WINDOW(handle))
   {
      int rid = GPOINTER_TO_INT(icon);

      if(rid < 65536)
      {
         GdkDisplay *display = gdk_display_get_default();
         GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);
         
         if(theme)
         {
            char resource_path[201] = {0};
            char window_icon[25] = {0};

            snprintf(resource_path, 200, "%s%u.png", _DW_RESOURCE_PATH, rid);
            gtk_icon_theme_add_resource_path(theme, _DW_RESOURCE_PATH);
            snprintf(window_icon, 24, "%u", rid);
            gtk_window_set_icon_name(GTK_WINDOW(handle), window_icon);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       filename: a path to a file (Bitmap on OS/2 or
 *                 Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
DW_FUNCTION_DEFINITION(dw_window_set_bitmap, int, HWND handle, unsigned long id, const char *filename)
DW_FUNCTION_ADD_PARAM3(handle, id, filename)
DW_FUNCTION_RETURN(dw_window_set_bitmap, int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, id, ULONG, filename, const char *)
{
   GdkPixbuf *tmp = NULL;
   int retval = DW_ERROR_UNKNOWN;

   if(id)
      tmp = _dw_find_pixbuf((HICN)id, NULL, NULL);
   else
   {
      char *file = alloca(strlen(filename) + 6);

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) != 0)
      {
         int i = 0;

         /* Try with various extentions */
         while(_dw_image_exts[i] && !tmp)
         {
            strcpy(file, filename);
            strcat(file, _dw_image_exts[i]);
            if(access(file, 04) == 0)
               tmp = gdk_pixbuf_new_from_file(file, NULL);
            i++;
         }
      }
      else
         tmp = gdk_pixbuf_new_from_file(file, NULL);
   }

   if(tmp)
   {
      if(GTK_IS_BUTTON(handle))
      {
         GtkWidget *pixmap = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_bitmap");
         if(pixmap)
         {
            gtk_picture_set_pixbuf(GTK_PICTURE(pixmap), tmp);
            retval = DW_ERROR_NONE;
         }
      }
      else if(GTK_IS_PICTURE(handle))
      {
         gtk_picture_set_pixbuf(GTK_PICTURE(handle), tmp);
         retval = DW_ERROR_NONE;
      } 
   }
   else
   	retval = DW_ERROR_GENERAL;
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the bitmap used for a given static window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon,
 *           (pass 0 if you use the filename param)
 *       data: the image data
 *                 Bitmap on Windows and a pixmap on Unix, pass
 *                 NULL if you use the id param)
 *       len: length of data
 * Returns:
 *        DW_ERROR_NONE on success.
 *        DW_ERROR_UNKNOWN if the parameters were invalid.
 *        DW_ERROR_GENERAL if the bitmap was unable to be loaded.
 */
DW_FUNCTION_DEFINITION(dw_window_set_bitmap_from_data, int, HWND handle, unsigned long id, const char *data, int len)
DW_FUNCTION_ADD_PARAM4(handle, id, data, len)
DW_FUNCTION_RETURN(dw_window_set_bitmap_from_data, int)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, id, ULONG, data, const char *, len, int)
{
   GdkPixbuf *tmp = NULL;
   int retval = DW_ERROR_UNKNOWN;

   if(data)
   {
      /*
       * A real hack; create a temporary file and write the contents
       * of the data to the file
       */
      char template[] = "/tmp/dwpixmapXXXXXX";
      int written = -1, fd = mkstemp(template);

      if(fd != -1)
      {
         written = write(fd, data, len);
         close(fd);
      }
      /* Bail if we couldn't write full file */
      if(fd != -1 && written == len)
      {
         tmp = gdk_pixbuf_new_from_file(template, NULL);
         /* remove our temporary file */
         unlink(template);
      }
   }
   else if(id)
      tmp = _dw_find_pixbuf((HICN)id, NULL, NULL);

   if(tmp)
   {
      if(GTK_IS_BUTTON(handle))
      {
         GtkWidget *pixmap = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_bitmap");

         if(pixmap)
         {
            gtk_picture_set_pixbuf(GTK_PICTURE(pixmap), tmp);
            retval = DW_ERROR_NONE;
         }
      }
      else if(GTK_IS_PICTURE(handle))
      {
         gtk_picture_set_pixbuf(GTK_PICTURE(handle), tmp);
         retval = DW_ERROR_NONE;
      }
   }
   else
   	retval = DW_ERROR_GENERAL;
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_set_text, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_window_set_text)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
   if(GTK_IS_ENTRY(handle))
   {
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(handle));
      if(buffer)
         gtk_entry_buffer_set_text(buffer, text, -1);
   }
   else if(GTK_IS_COMBO_BOX(handle))
   {
      GtkWidget *entry = gtk_combo_box_get_child(GTK_COMBO_BOX(handle));
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
      if(buffer)
         gtk_entry_buffer_set_text(buffer, text, -1);
   }
   else if(GTK_IS_LABEL(handle))
      gtk_label_set_text(GTK_LABEL(handle), text);
   else if(GTK_IS_BUTTON(handle))
      gtk_button_set_label(GTK_BUTTON(handle), text);
   else if(GTK_IS_WINDOW(handle))
      gtk_window_set_title(GTK_WINDOW(handle), text);
   else if (GTK_IS_FRAME(handle))
   {
      /*
       * This is a groupbox or status_text
       */
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_label");
      if (tmp && GTK_IS_LABEL(tmp))
         gtk_label_set_text(GTK_LABEL(tmp), text);
      else /* assume groupbox */
         gtk_frame_set_label(GTK_FRAME(handle), text && *text ? text : NULL);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text used for a given window's floating bubble help.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       bubbletext: The text in the floating bubble tooltip.
 */
DW_FUNCTION_DEFINITION(dw_window_set_tooltip, void, HWND handle, const char *bubbletext)
DW_FUNCTION_ADD_PARAM2(handle, bubbletext)
DW_FUNCTION_NO_RETURN(dw_window_set_tooltip)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, bubbletext, char *)
{
   if(bubbletext && *bubbletext)
      gtk_widget_set_tooltip_text(handle, bubbletext);
   else
      gtk_widget_set_has_tooltip(handle, FALSE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       text: The text associated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_get_text, char *, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_get_text, char *)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   const char *possible = NULL;
   char *retval = NULL;

   if(GTK_IS_ENTRY(handle))
   {
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(handle));
      possible = gtk_entry_buffer_get_text(buffer);
   }
   else if(GTK_IS_COMBO_BOX(handle))
   {
      GtkWidget *entry = gtk_combo_box_get_child(GTK_COMBO_BOX(handle));
      GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
      possible = gtk_entry_buffer_get_text(buffer);
   }
   else if(GTK_IS_LABEL(handle))
      possible = gtk_label_get_text(GTK_LABEL(handle));
   retval = strdup(possible ? possible : "");
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Disables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
DW_FUNCTION_DEFINITION(dw_window_disable, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_disable)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   gtk_widget_set_sensitive(handle, FALSE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Enables given window (widget).
 * Parameters:
 *       handle: Handle to the window.
 */
DW_FUNCTION_DEFINITION(dw_window_enable, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_enable)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   gtk_widget_set_sensitive(handle, TRUE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
DW_FUNCTION_DEFINITION(dw_window_from_id, HWND, HWND handle, int id)
DW_FUNCTION_ADD_PARAM2(handle, id)
DW_FUNCTION_RETURN(dw_window_from_id, HWND)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, id, int)
{
   GtkWidget *retval = NULL;

   if(handle && GTK_WIDGET(handle) && id)
   {
      GtkWidget *widget = gtk_widget_get_first_child(GTK_WIDGET(handle));
      
      while(widget)
      {
         if(id == GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "_dw_id")))
         {
            retval = widget;
            widget = NULL;
         }
         else
            widget = gtk_widget_get_next_sibling(GTK_WIDGET(widget));
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Adds text to an MLE box and returns the current point.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be imported.
 *          startpoint: Point to start entering text.
 */
DW_FUNCTION_DEFINITION(dw_mle_import, unsigned int, HWND handle, const char *buffer, int startpoint)
DW_FUNCTION_ADD_PARAM3(handle, buffer, startpoint)
DW_FUNCTION_RETURN(dw_mle_import, unsigned int)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, buffer, const char *, startpoint, int)
{
   unsigned int tmppoint = startpoint;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, startpoint);
         gtk_text_buffer_place_cursor(tbuffer, &iter);
         gtk_text_buffer_insert_at_cursor(tbuffer, buffer, -1);
         tmppoint = (startpoint > -1 ? startpoint : 0) + strlen(buffer);
      }
   }
   DW_FUNCTION_RETURN_THIS(tmppoint);
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported. MUST allow for trailing nul character.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
DW_FUNCTION_DEFINITION(dw_mle_export, void, HWND handle, char *buffer, int startpoint, int length)
DW_FUNCTION_ADD_PARAM4(handle, buffer, startpoint, length)
DW_FUNCTION_NO_RETURN(dw_mle_export)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, buffer, char *, startpoint, int, length, int)
{
   gchar *text;

   /* force the return value to nul in case the following tests fail */
   if(buffer)
      buffer[0] = '\0';
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter start, end;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
         gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
         text = gtk_text_iter_get_text(&start, &end);
         if(text) /* Should this get freed? */
         {
            if(buffer)
               strcpy(buffer, text);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Obtains information about an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          bytes: A pointer to a variable to return the total bytes.
 *          lines: A pointer to a variable to return the number of lines.
 */
DW_FUNCTION_DEFINITION(dw_mle_get_size, void, HWND handle, unsigned long *bytes, unsigned long *lines)
DW_FUNCTION_ADD_PARAM3(handle, bytes, lines)
DW_FUNCTION_NO_RETURN(dw_mle_get_size)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, bytes, unsigned long *, lines, unsigned long *)
{
   if(bytes)
      *bytes = 0;
   if(lines)
      *lines = 0;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));

         if(bytes)
            *bytes = gtk_text_buffer_get_char_count(buffer);
         if(lines)
            *lines = gtk_text_buffer_get_line_count(buffer);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be deleted from.
 *          startpoint: Point to start deleting text.
 *          length: Amount of text to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_mle_delete, void, HWND handle, int startpoint, int length)
DW_FUNCTION_ADD_PARAM3(handle, startpoint, length)
DW_FUNCTION_NO_RETURN(dw_mle_delete)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, startpoint, int, length, int)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter start, end;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &start, startpoint);
         gtk_text_buffer_get_iter_at_offset(tbuffer, &end, startpoint + length);
         gtk_text_buffer_delete(tbuffer, &start, &end);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Clears all text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_mle_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_mle_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int length;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tmp));

         length = -1;
         gtk_text_buffer_set_text(buffer, "", length);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          line: Line to be visible.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_visible, void, HWND handle, int line)
DW_FUNCTION_ADD_PARAM2(handle, line)
DW_FUNCTION_NO_RETURN(dw_mle_set_visible)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, line, int)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;
         GtkTextMark *mark = (GtkTextMark *)g_object_get_data(G_OBJECT(handle), "_dw_mark");

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, 0);
         gtk_text_iter_set_line(&iter, line);
         if(!mark)
         {
            mark = gtk_text_buffer_create_mark(tbuffer, NULL, &iter, FALSE);
            g_object_set_data(G_OBJECT(handle), "_dw_mark", (gpointer)mark);
         }
         else
            gtk_text_buffer_move_mark(tbuffer, mark, &iter);
         gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tmp), mark,
                               0, FALSE, 0, 0);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_editable, void, HWND handle, int state)
DW_FUNCTION_ADD_PARAM2(handle, state)
DW_FUNCTION_NO_RETURN(dw_mle_set_editable)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, state, int)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
         gtk_text_view_set_editable(GTK_TEXT_VIEW(tmp), state);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_word_wrap, void, HWND handle, int state)
DW_FUNCTION_ADD_PARAM2(handle, state)
DW_FUNCTION_NO_RETURN(dw_mle_set_word_wrap)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, state, int)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
         gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tmp), state ? GTK_WRAP_WORD : GTK_WRAP_NONE);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
void API dw_mle_set_auto_complete(HWND handle, int state)
{
}

/*
 * Sets the current cursor position of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          point: Point to position cursor.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_cursor, void, HWND handle, int point)
DW_FUNCTION_ADD_PARAM2(handle, point)
DW_FUNCTION_NO_RETURN(dw_mle_set_cursor)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, point, int)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter;
         GtkTextMark *mark = (GtkTextMark *)g_object_get_data(G_OBJECT(handle), "_dw_mark");

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
         if(!mark)
         {
            mark = gtk_text_buffer_create_mark(tbuffer, NULL, &iter, FALSE);
            g_object_set_data(G_OBJECT(handle), "_dw_mark", (gpointer)mark);
         }
         else
            gtk_text_buffer_move_mark(tbuffer, mark, &iter);
         gtk_text_buffer_place_cursor(tbuffer, &iter);
         gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tmp), mark,
                               0, FALSE, 0, 0);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Finds text in an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be cleared.
 *          text: Text to search for.
 *          point: Start point of search.
 *          flags: Search specific flags.
 */
DW_FUNCTION_DEFINITION(dw_mle_search, int, HWND handle, const char *text, int point, DW_UNUSED(unsigned long flags))
DW_FUNCTION_ADD_PARAM4(handle, text, point, flags)
DW_FUNCTION_RETURN(dw_mle_search, int)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, text, const char *, point, int, DW_UNUSED(flags), unsigned long)
{
   int retval = 0;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tmp && GTK_IS_TEXT_VIEW(tmp))
      {
         GtkTextBuffer *tbuffer;
         GtkTextIter iter, found;

         tbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tmp));
         gtk_text_buffer_get_iter_at_offset(tbuffer, &iter, point);
         gtk_text_iter_forward_search(&iter, text, GTK_TEXT_SEARCH_TEXT_ONLY, &found, NULL, NULL);
         retval = gtk_text_iter_get_offset(&found);
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
}

/* Internal function to update the progress bar
 * while in an indeterminate state.
 */
gboolean _dw_update_progress_bar(gpointer data)
{
   if(g_object_get_data(G_OBJECT(data), "_dw_alive"))
   {
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(data));
      return TRUE;
   }
   return FALSE;
}

/*
 * Sets the percent bar position.
 * Parameters:
 *          handle: Handle to the percent bar to be set.
 *          position: Position of the percent bar withing the range.
 */
DW_FUNCTION_DEFINITION(dw_percent_set_pos, void, HWND handle, unsigned int position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_percent_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, unsigned int)
{
   if(position == DW_PERCENT_INDETERMINATE)
   {
      /* Check if we are indeterminate already */
      if(!g_object_get_data(G_OBJECT(handle), "_dw_alive"))
      {
         /* If not become indeterminate... and start a timer to continue */
         gtk_progress_bar_pulse(GTK_PROGRESS_BAR(handle));
         g_object_set_data(G_OBJECT(handle), "_dw_alive", GINT_TO_POINTER(1));
         g_timeout_add(100, (GSourceFunc)_dw_update_progress_bar, (gpointer)handle);
      }
   }
   else
   {
      /* Cancel the existing timer if one is there */
      g_object_set_data(G_OBJECT(handle), "_dw_alive", NULL);
      /* Set the position like normal */
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(handle), (gfloat)position/100);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
DW_FUNCTION_DEFINITION(dw_slider_get_pos, unsigned int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_slider_get_pos, unsigned int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int val = 0;
   GtkAdjustment *adjustment;

   if(handle)
   {
      adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
      if(adjustment)
      {
         int max = _dw_round_value(gtk_adjustment_get_upper(adjustment)) - 1;
         int thisval = _dw_round_value(gtk_adjustment_get_value(adjustment));

         if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_VERTICAL)
            val = max - thisval;
           else
            val = thisval;
      }
   }
   DW_FUNCTION_RETURN_THIS(val);
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
DW_FUNCTION_DEFINITION(dw_slider_set_pos, void, HWND handle, unsigned int position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_slider_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, unsigned int)
{
   GtkAdjustment *adjustment;

   if(handle)
   {
      adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
      if(adjustment)
      {
         int max = _dw_round_value(gtk_adjustment_get_upper(adjustment)) - 1;

         if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_VERTICAL)
            gtk_adjustment_set_value(adjustment, (gfloat)(max - position));
           else
            gtk_adjustment_set_value(adjustment, (gfloat)position);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
DW_FUNCTION_DEFINITION(dw_scrollbar_get_pos, unsigned int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_scrollbar_get_pos, unsigned int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int val = 0;
   GtkAdjustment *adjustment;

   if(handle)
   {
      adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment");
      if(adjustment)
         val = _dw_round_value(gtk_adjustment_get_value(adjustment));
   }
   DW_FUNCTION_RETURN_THIS(val);
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
DW_FUNCTION_DEFINITION(dw_scrollbar_set_pos, void, HWND handle, unsigned int position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_scrollbar_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, unsigned int)

{
   GtkAdjustment *adjustment;

   if(handle)
   {
      if((adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment")))
      {
         g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event", GINT_TO_POINTER(1));
         gtk_adjustment_set_value(adjustment, (gfloat)position);
         g_object_set_data(G_OBJECT(adjustment), "_dw_suppress_value_changed_event", GINT_TO_POINTER(0));
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the scrollbar range.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          range: Maximum range value.
 *          visible: Visible area relative to the range.
 */
DW_FUNCTION_DEFINITION(dw_scrollbar_set_range, void, HWND handle, unsigned int range, unsigned int visible)
DW_FUNCTION_ADD_PARAM3(handle, range, visible)
DW_FUNCTION_NO_RETURN(dw_scrollbar_set_range)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, range, unsigned int, visible, unsigned int)
{
   GtkAdjustment *adjustment;

   if(handle)
   {
      if((adjustment = (GtkAdjustment *)g_object_get_data(G_OBJECT(handle), "_dw_adjustment")))
      {
         gtk_adjustment_set_upper(adjustment, (gdouble)range);
         gtk_adjustment_set_page_increment(adjustment,(gdouble)visible);
         gtk_adjustment_set_page_size(adjustment, (gdouble)visible);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_set_pos, void, HWND handle, long position)
DW_FUNCTION_ADD_PARAM2(handle, position)
DW_FUNCTION_NO_RETURN(dw_spinbutton_set_pos)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, position, long)
{
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(handle), (gfloat)position);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 *          position: Current value of the spinbutton.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_set_limits, void, HWND handle, long upper, long lower)
DW_FUNCTION_ADD_PARAM3(handle, upper, lower)
DW_FUNCTION_NO_RETURN(dw_spinbutton_set_limits)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, upper, long, lower, long)
{
   long curval;
   GtkAdjustment *adj;

   curval = dw_spinbutton_get_pos(handle);
   adj = (GtkAdjustment *)gtk_adjustment_new((gfloat)curval, (gfloat)lower, (gfloat)upper, 1.0, 5.0, 0.0);
   gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(handle), adj);
   /*
    * Set our internal relationships between the adjustment and the spinbutton
    */
   g_object_set_data(G_OBJECT(handle), "_dw_adjustment", (gpointer)adj);
   g_object_set_data(G_OBJECT(adj), "_dw_spinbutton", (gpointer)handle);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
DW_FUNCTION_DEFINITION(dw_entryfield_set_limit, void, HWND handle, ULONG limit)
DW_FUNCTION_ADD_PARAM2(handle, limit)
DW_FUNCTION_NO_RETURN(dw_entryfield_set_limit)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, limit, ULONG)
{
   gtk_entry_set_max_length(GTK_ENTRY(handle), limit);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
DW_FUNCTION_DEFINITION(dw_spinbutton_get_pos, long, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_spinbutton_get_pos, long)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   long retval = (long)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(handle));
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_get, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_checkbox_get, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retval = 0;

   if(handle)
   {
      if(GTK_IS_TOGGLE_BUTTON(handle))
         retval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(handle));
      retval = gtk_check_button_get_active(GTK_CHECK_BUTTON(handle));
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
DW_FUNCTION_DEFINITION(dw_checkbox_set, void, HWND handle, int value)
DW_FUNCTION_ADD_PARAM2(handle, value)
DW_FUNCTION_NO_RETURN(dw_checkbox_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, value, int)
{
   if(handle)
   {
      if(GTK_IS_TOGGLE_BUTTON(handle))
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(handle), value);
      else
         gtk_check_button_set_active(GTK_CHECK_BUTTON(handle), value);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Inserts an item into a tree window (widget) after another item.
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          item: Handle to the item to be positioned after.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 */
DW_FUNCTION_DEFINITION(dw_tree_insert_after, HTREEITEM, HWND handle, HTREEITEM item, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
DW_FUNCTION_ADD_PARAM6(handle, item, title, icon, parent, itemdata)
DW_FUNCTION_RETURN(dw_tree_insert_after, HTREEITEM)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, item, HTREEITEM, title, char *, icon, HICN, parent, HTREEITEM, itemdata, void *)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

         pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

         gtk_tree_store_insert_after(store, iter, (GtkTreeIter *)parent, (GtkTreeIter *)item);
         gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
         retval = (HTREEITEM)iter;
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Inserts an item into a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree to be inserted.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 *          parent: Parent handle or 0 if root.
 *          itemdata: Item specific data.
 */
DW_FUNCTION_DEFINITION(dw_tree_insert, HTREEITEM, HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
DW_FUNCTION_ADD_PARAM5(handle, title, icon, parent, itemdata)
DW_FUNCTION_RETURN(dw_tree_insert, HTREEITEM)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, title, char *, icon, HICN, parent, HTREEITEM, itemdata, void *)
{
   GtkWidget *tree;
   GtkTreeIter *iter;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;
   HTREEITEM retval = 0;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         iter = (GtkTreeIter *)malloc(sizeof(GtkTreeIter));

         pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

         gtk_tree_store_append (store, iter, (GtkTreeIter *)parent);
         gtk_tree_store_set (store, iter, 0, title, 1, pixbuf, 2, itemdata, 3, iter, -1);
         retval = (HTREEITEM)iter;
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the text and icon of an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          title: The text title of the entry.
 *          icon: Handle to coresponding icon.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_change, void, HWND handle, HTREEITEM item, const char *title, HICN icon)
DW_FUNCTION_ADD_PARAM4(handle, item, title, icon)
DW_FUNCTION_NO_RETURN(dw_tree_item_change)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, item, HTREEITEM, title, char *, icon, HICN)
{
   GtkWidget *tree;
   GtkTreeStore *store;
   GdkPixbuf *pixbuf;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         pixbuf = _dw_find_pixbuf(icon, NULL, NULL);

         gtk_tree_store_set(store, (GtkTreeIter *)item, 0, title, 1, pixbuf, -1);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 *          itemdata: User defined data to be associated with item.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_set_data, void, HWND handle, HTREEITEM item, void *itemdata)
DW_FUNCTION_ADD_PARAM3(handle, item, itemdata)
DW_FUNCTION_NO_RETURN(dw_tree_item_set_data)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(handle), HWND, item, HTREEITEM, itemdata, void *)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle && item)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
            gtk_tree_store_set(store, (GtkTreeIter *)item, 2, itemdata, -1);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
DW_FUNCTION_DEFINITION(dw_tree_get_title, char *, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_get_title, char *)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(handle), HWND, item, HTREEITEM)
{
   char *text = NULL;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(handle && item)
   {
      tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tree && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
         gtk_tree_model_get(store, (GtkTreeIter *)item, _DW_DATA_TYPE_STRING, &text, -1);
      if(text)
      {
         char *temp = text;
         text = strdup(temp);
         g_free(temp);
      }
   }
   DW_FUNCTION_RETURN_THIS(text);
}

/*
 * Gets the text an item in a tree window (widget).
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
DW_FUNCTION_DEFINITION(dw_tree_get_parent, HTREEITEM, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_get_parent, HTREEITEM)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
   HTREEITEM parent = (HTREEITEM)0;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(handle && item)
   {
      tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

      if(tree && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         GtkTreeIter iter;

         if(gtk_tree_model_iter_parent(store, &iter, (GtkTreeIter *)item))
            gtk_tree_model_get(store, &iter, 3, &parent, -1);
      }
   }
   DW_FUNCTION_RETURN_THIS(parent);
}

/*
 * Gets the item data of a tree item.
 * Parameters:
 *          handle: Handle to the tree containing the item.
 *          item: Handle of the item to be modified.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_get_data, void *, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_RETURN(dw_tree_item_get_data, void *)
DW_FUNCTION_RESTORE_PARAM2(DW_UNUSED(handle), HWND, item, HTREEITEM)
{
   void *ret = NULL;
   GtkWidget *tree;
   GtkTreeModel *store;

   if(handle && item)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeModel *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
            gtk_tree_model_get(store, (GtkTreeIter *)item, 2, &ret, -1);
   }
   DW_FUNCTION_RETURN_THIS(ret);
}

/*
 * Sets this item as the active selection.
 * Parameters:
 *       handle: Handle to the tree window (widget) to be selected.
 *       item: Handle to the item to be selected.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_select, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_select)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle && item)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

         gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);
         gtk_tree_selection_select_iter(sel, (GtkTreeIter *)item);
         gtk_tree_path_free(path);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

static void _dw_recursive_free(GtkTreeModel *store, GtkTreeIter parent)
{
   void *data;
   GtkTreeIter iter;

   gtk_tree_model_get(store, &parent, 3, &data, -1);
   if(data)
      free(data);
   gtk_tree_store_set(GTK_TREE_STORE(store), &parent, 3, NULL, -1);

   if(gtk_tree_model_iter_children(store, &iter, &parent))
   {
      do {
         _dw_recursive_free(GTK_TREE_MODEL(store), iter);
      } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
   }
}

/*
 * Removes all nodes from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_tree_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_tree_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
         {
            GtkTreeIter iter;

            if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
            {
               do {
                  _dw_recursive_free(GTK_TREE_MODEL(store), iter);
               } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
            }
            gtk_tree_store_clear(store);
         }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Expands a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be expanded.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_expand, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_expand)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
         gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE);
         gtk_tree_path_free(path);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Collapses a node on a tree.
 * Parameters:
 *       handle: Handle to the tree window (widget).
 *       item: Handle to node to be collapsed.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_collapse, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_collapse)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), (GtkTreeIter *)item);
         gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), path);
         gtk_tree_path_free(path);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes a node from a tree.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       item: Handle to node to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_tree_item_delete, void, HWND handle, HTREEITEM item)
DW_FUNCTION_ADD_PARAM2(handle, item)
DW_FUNCTION_NO_RETURN(dw_tree_item_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, item, HTREEITEM)
{
   GtkWidget *tree;
   GtkTreeStore *store;

   if(handle)
   {
      if((tree = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user"))
         && GTK_IS_TREE_VIEW(tree) &&
         (store = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(tree))))
      {
         gtk_tree_store_remove(store, (GtkTreeIter *)item);
         free(item);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

#define _DW_CONTAINER_STORE_EXTRA 2

static int _dw_container_setup_int(HWND handle, unsigned long *flags, char **titles, int count, int separator, int extra)
{
   int z;
   char numbuf[25] = {0};
   GtkWidget *tree;
   GtkListStore *store;
   GtkTreeViewColumn *col;
   GtkCellRenderer *rend;
   GtkTreeSelection *sel;
   GType *array = calloc(count + _DW_CONTAINER_STORE_EXTRA + 1, sizeof(GType));

   /* Save some of the info so it is easily accessible */
   g_object_set_data(G_OBJECT(handle), "_dw_cont_columns", GINT_TO_POINTER(count));
   g_object_set_data(G_OBJECT(handle), "_dw_cont_extra", GINT_TO_POINTER(extra));

   /* First param is row title */
   array[0] = G_TYPE_STRING;
   /* Second param is row data */
   array[1] = G_TYPE_POINTER;
   array[2] = G_TYPE_POINTER;
   /* First loop... create array to create the store */
   for(z=0;z<count;z++)
   {
      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         array[_DW_CONTAINER_STORE_EXTRA] = GDK_TYPE_PIXBUF;
         array[_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = GDK_TYPE_PIXBUF;
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_ULONG;
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         array[z+_DW_CONTAINER_STORE_EXTRA+1] = G_TYPE_STRING;
      }
   }
   /* Create the store and then the tree */
   store = gtk_list_store_newv(count + _DW_CONTAINER_STORE_EXTRA + 1, array);
   tree = _dw_tree_view_setup(handle, GTK_TREE_MODEL(store));
   g_object_set_data(G_OBJECT(tree), "_dw_tree_type", GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER));
   /* Second loop... create the columns */
   for(z=0;z<count;z++)
   {
      snprintf(numbuf, 24, "_dw_cont_col%d", z);
      g_object_set_data(G_OBJECT(tree), numbuf, GINT_TO_POINTER(flags[z]));
      col = gtk_tree_view_column_new();
      rend = NULL;
      void **params = calloc(sizeof(void *), 3);

      if(z == 0 && flags[z] & DW_CFA_STRINGANDICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", _DW_CONTAINER_STORE_EXTRA);
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", _DW_CONTAINER_STORE_EXTRA+1);
      }
      else if(flags[z] & DW_CFA_BITMAPORICON)
      {
         rend = gtk_cell_renderer_pixbuf_new();
         gtk_tree_view_column_pack_start(col, rend, FALSE);
         gtk_tree_view_column_add_attribute(col, rend, "pixbuf", z+_DW_CONTAINER_STORE_EXTRA+1);
      }
      else if(flags[z] & DW_CFA_STRING)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_ULONG)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_TIME)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      else if(flags[z] & DW_CFA_DATE)
      {
         rend = gtk_cell_renderer_text_new();
         gtk_tree_view_column_pack_start(col, rend, TRUE);
         gtk_tree_view_column_add_attribute(col, rend, "text", z+_DW_CONTAINER_STORE_EXTRA+1);
         gtk_tree_view_column_set_resizable(col, TRUE);
      }
      g_object_set_data(G_OBJECT(col), "_dw_column", GINT_TO_POINTER(z));
      params[2] = tree;
      g_signal_connect_data(G_OBJECT(col), "clicked", G_CALLBACK(_dw_column_click_event), (gpointer)params, _dw_signal_disconnect, 0);
      gtk_tree_view_column_set_title(col, titles[z]);
      if(flags[z] & DW_CFA_RIGHT)
      {
         gtk_tree_view_column_set_alignment(col, 1.0);
         if(rend)
            gtk_cell_renderer_set_alignment(rend, 1.0, 0.5);
      }
      else if(flags[z] & DW_CFA_CENTER)
      {
         gtk_tree_view_column_set_alignment(col, 0.5);
         if(rend)
            gtk_cell_renderer_set_alignment(rend, 0.5, 0.5);
      }
      gtk_tree_view_append_column(GTK_TREE_VIEW (tree), col);
   }
   /* Finish up */
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
   gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(tree), TRUE);
   gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tree), TRUE);
   sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
   if(g_object_get_data(G_OBJECT(handle), "_dw_multi_sel"))
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
   else
      gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
   gtk_widget_set_visible(tree, TRUE);
   free(array);
   if(_DWDefaultFont)
      dw_window_set_font(handle, _DWDefaultFont);
   return DW_ERROR_NONE;
}

/*
 * Sets up the container columns.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 *          separator: The column number that contains the main separator.
 *                     (this item may only be used in OS/2)
 */
DW_FUNCTION_DEFINITION(dw_container_setup, int, HWND handle, unsigned long *flags, char **titles, int count, int separator)
DW_FUNCTION_ADD_PARAM5(handle, flags, titles, count, separator)
DW_FUNCTION_RETURN(dw_container_setup, int)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, flags, unsigned long *, titles, char **, count, int, DW_UNUSED(separator), int)
{
   int retval = _dw_container_setup_int(handle, flags, titles, count, separator, 0);
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Configures the main filesystem columnn title for localization.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          title: The title to be displayed in the main column.
 */
void API dw_filesystem_set_column_title(HWND handle, const char *title)
{
	char *newtitle = strdup(title ? title : "");
	
	g_object_set_data(G_OBJECT(handle), "_dw_coltitle", newtitle);
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 */
DW_FUNCTION_DEFINITION(dw_filesystem_setup, int, HWND handle, unsigned long *flags, char **titles, int count)
DW_FUNCTION_ADD_PARAM4(handle, flags, titles, count)
DW_FUNCTION_RETURN(dw_filesystem_setup, int)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, flags, unsigned long *, titles, char **, count, int)
{
   char **newtitles = malloc(sizeof(char *) * (count + 1));
   unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 1));
   char *coltitle = (char *)g_object_get_data(G_OBJECT(handle), "_dw_coltitle");
   int retval;

   newtitles[0] = coltitle ? coltitle : "Filename";
   newflags[0] = DW_CFA_STRINGANDICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

   memcpy(&newtitles[1], titles, sizeof(char *) * count);
   memcpy(&newflags[1], flags, sizeof(unsigned long) * count);

   retval = _dw_container_setup_int(handle, newflags, newtitles, count + 1, 1, 1);

   if(coltitle)
   {
	  g_object_set_data(G_OBJECT(handle), "_dw_coltitle", NULL);
	  free(coltitle);
   }
   if(newtitles) 
      free(newtitles);
   if(newflags) 
      free(newflags);
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 */
HICN API dw_icon_load(unsigned long module, unsigned long id)
{
   return (HICN)id;
}

/* Internal function to keep HICNs from getting too big */
GdkPixbuf *_dw_icon_resize(GdkPixbuf *ret)
{
   if(ret)
   {
      int pwidth = gdk_pixbuf_get_width(ret);
      int pheight = gdk_pixbuf_get_height(ret);

      if(pwidth > 24 || pheight > 24)
      {
         GdkPixbuf *orig = ret;
         ret = gdk_pixbuf_scale_simple(ret, pwidth > 24 ? 24 : pwidth, pheight > 24 ? 24 : pheight, GDK_INTERP_BILINEAR);
         g_object_unref(G_OBJECT(orig));
      }
   }
   return ret;
}

/*
 * Obtains an icon from a file.
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_file(const char *filename)
{
   HICN retval = 0;

   if(filename && *filename)
   {
      char *file = alloca(strlen(filename) + 6);

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) != 0)
      {
         int i = 0;

         /* Try with various extentions */
         while(_dw_image_exts[i] && !retval)
         {
            strcpy(file, filename);
            strcat(file, _dw_image_exts[i]);
            if(access(file, 04) == 0)
               retval = _dw_icon_resize(gdk_pixbuf_new_from_file(file, NULL));
            i++;
         }
      }
      else
         retval = _dw_icon_resize(gdk_pixbuf_new_from_file(file, NULL));
   }
   return retval;
}

/*
 * Obtains an icon from data.
 * Parameters:
 *       data: Source of data for image.
 *       len:  length of data
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
   int fd, written = -1;
   char template[] = "/tmp/dwiconXXXXXX";
   HICN ret = 0;

   /*
    * A real hack; create a temporary file and write the contents
    * of the data to the file
    */
   if((fd = mkstemp(template)) != -1)
   {
      written = write(fd, data, len);
      close(fd);
   }
   /* Bail if we couldn't write full file */
   if(fd == -1 || written != len)
      return 0;
   ret = _dw_icon_resize(gdk_pixbuf_new_from_file(template, NULL));
   unlink(template);
   return ret;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
   int iicon = GPOINTER_TO_INT(handle);

   if(iicon > 65535)
   {
      g_object_unref(handle);
   }
}

/*
 * Allocates memory used to populate a container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          rowcount: The number of items to be populated.
 */
DW_FUNCTION_DEFINITION(dw_container_alloc, void *, HWND handle, int rowcount)
DW_FUNCTION_ADD_PARAM2(handle, rowcount)
DW_FUNCTION_RETURN(dw_container_alloc, void *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, rowcount, int)
{
   int z, prevrowcount = 0;
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      prevrowcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      for(z=0;z<rowcount;z++)
      {
         gtk_list_store_append(store, &iter);
      }
      g_object_set_data(G_OBJECT(cont), "_dw_insertpos", GINT_TO_POINTER(prevrowcount));
      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rowcount + prevrowcount));
   }
   DW_FUNCTION_RETURN_THIS(cont);
}

/*
 * Internal representation of dw_container_set_item() extracted so we can pass
 * two data pointers; icon and text for dw_filesystem_set_item().
 */
void _dw_container_set_item_int(HWND handle, void *pointer, int column, int row, void *data)
{
   char numbuf[25] = {0}, textbuffer[101] = {0};
   int flag = 0;
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      snprintf(numbuf, 24, "_dw_cont_col%d", column);
      flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), numbuf));
      if(pointer)
      {
         row += GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_insertpos"));
      }

      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, row))
      {
         if(flag & DW_CFA_STRINGANDICON)
         {
            void **thisdata = (void **)data;
            HICN hicon = data ? *((HICN *)thisdata[0]) : 0;
            char *tmp = data ? (char *)thisdata[1] : NULL;
            GdkPixbuf *pixbuf = hicon ? _dw_find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, _DW_CONTAINER_STORE_EXTRA, pixbuf, -1);
            gtk_list_store_set(store, &iter, _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_BITMAPORICON)
         {
            HICN hicon = data ? *((HICN *)data) : 0;
            GdkPixbuf *pixbuf = hicon ? _dw_find_pixbuf(hicon, NULL, NULL) : NULL;

            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, pixbuf, -1);
         }
         else if(flag & DW_CFA_STRING)
         {
            char *tmp = data ? *((char **)data) : NULL;
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_ULONG)
         {
            ULONG tmp = data ? *((ULONG *)data): 0;

            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, tmp, -1);
         }
         else if(flag & DW_CFA_DATE)
         {
            if(data)
            {
               struct tm curtm;
               CDATE cdate = *((CDATE *)data);

               memset( &curtm, 0, sizeof(curtm) );
               curtm.tm_mday = cdate.day;
               curtm.tm_mon = cdate.month - 1;
               curtm.tm_year = cdate.year - 1900;

               strftime(textbuffer, 100, "%x", &curtm);
            }
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, textbuffer, -1);
         }
         else if(flag & DW_CFA_TIME)
         {
            if(data)
            {
               struct tm curtm;
               CTIME ctime = *((CTIME *)data);

               memset( &curtm, 0, sizeof(curtm) );
               curtm.tm_hour = ctime.hours;
               curtm.tm_min = ctime.minutes;
               curtm.tm_sec = ctime.seconds;

               strftime(textbuffer, 100, "%X", &curtm);
            }
            gtk_list_store_set(store, &iter, column + _DW_CONTAINER_STORE_EXTRA + 1, textbuffer, -1);
         }
      }
   }
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
DW_FUNCTION_DEFINITION(dw_container_set_item, void, HWND handle, void *pointer, int column, int row, void *data)
DW_FUNCTION_ADD_PARAM5(handle, pointer, column, row, data)
DW_FUNCTION_NO_RETURN(dw_container_set_item)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pointer, void *, column, int, row, int, data, void *)
{
   _dw_container_set_item_int(handle, pointer, column, row, data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
DW_FUNCTION_DEFINITION(dw_container_change_item, void, HWND handle, int column, int row, void *data)
DW_FUNCTION_ADD_PARAM4(handle, column, row, data)
DW_FUNCTION_NO_RETURN(dw_container_change_item)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, column, int, row, int, data, void *)
{
   _dw_container_set_item_int(handle, NULL, column, row, data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Changes an existing item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_item(HWND handle, int column, int row, void *data)
{
   dw_filesystem_set_item(handle, NULL, column, row, data);
}

/*
 * Changes an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
void API dw_filesystem_change_file(HWND handle, int row, const char *filename, HICN icon)
{
   dw_filesystem_set_file(handle, NULL, row, filename, icon);
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
DW_FUNCTION_DEFINITION(dw_filesystem_set_file, void, HWND handle, void *pointer, int row, const char *filename, HICN icon)
DW_FUNCTION_ADD_PARAM5(handle, pointer, row, filename, icon)
DW_FUNCTION_NO_RETURN(dw_filesystem_set_file)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pointer, void *, row, int, filename, char *, icon, HICN)
{
   void *data[2] = { (void *)&icon, (void *)filename };

   _dw_container_set_item_int(handle, pointer, 0, row, (void *)data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets an item in specified row and column to the given data.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          column: Zero based column of data being set.
 *          row: Zero based row of data being set.
 *          data: Pointer to the data to be added.
 */
DW_FUNCTION_DEFINITION(dw_filesystem_set_item, void, HWND handle, void *pointer, int column, int row, void *data)
DW_FUNCTION_ADD_PARAM5(handle, pointer, column, row, data)
DW_FUNCTION_NO_RETURN(dw_filesystem_set_item)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pointer, void *, column, int, row, int, data, void *)
{
   _dw_container_set_item_int(handle, pointer, column + 1, row, data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets column type for a container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
DW_FUNCTION_DEFINITION(dw_container_get_column_type, int, HWND handle, int column)
DW_FUNCTION_ADD_PARAM2(handle, column)
DW_FUNCTION_RETURN(dw_container_get_column_type, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, column, int)
{
   int flag, rc = 0;
   GtkWidget *cont = handle;

   if((cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user")))
   {
      char numbuf[25] = {0};

      snprintf(numbuf, 24, "_dw_cont_col%d", column);
      flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), numbuf));

      if(flag & DW_CFA_BITMAPORICON)
         rc = DW_CFA_BITMAPORICON;
      else if(flag & DW_CFA_STRING)
         rc = DW_CFA_STRING;
      else if(flag & DW_CFA_ULONG)
         rc = DW_CFA_ULONG;
      else if(flag & DW_CFA_DATE)
         rc = DW_CFA_DATE;
      else if(flag & DW_CFA_TIME)
         rc = DW_CFA_TIME;
      else
         rc = 0;
   }
   DW_FUNCTION_RETURN_THIS(rc);
}

/*
 * Gets column type for a filesystem container column
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          column: Zero based column.
 */
int API dw_filesystem_get_column_type(HWND handle, int column)
{
   return dw_container_get_column_type( handle, column + 1 );
}

/*
 * Sets the alternating row colors for container window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          oddcolor: Odd row background color in DW_RGB format or a default color index.
 *          evencolor: Even row background color in DW_RGB format or a default color index.
 *                    DW_RGB_TRANSPARENT will disable coloring rows.
 *                    DW_CLR_DEFAULT will use the system default alternating row colors.
 */
void API dw_container_set_stripe(HWND handle, unsigned long oddcolor, unsigned long evencolor)
{
    /* TODO: If we want to accomplish this, according to mclasen we can do it
     * with CSS on a GtkListBox widget using the following CSS:
     * "row: nth-child(even) { background: red; }"
     * However he does not recommend we do it for performance reasons.
     */
}

/*
 * Sets the width of a column in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          column: Zero based column of width being set.
 *          width: Width of column in pixels.
 */
DW_FUNCTION_DEFINITION(dw_container_set_column_width, void, HWND handle, int column, int width)
DW_FUNCTION_ADD_PARAM3(handle, column, width)
DW_FUNCTION_NO_RETURN(dw_container_set_column_width)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, column, int, width, int)
{
   GtkWidget *cont;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
   {
      GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(cont), column);

      if(col && GTK_IS_TREE_VIEW_COLUMN(col))
      {
         gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(col), width);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Internal version for both */
void _dw_container_set_row_data_int(HWND handle, void *pointer, int row, int type, void *data)
{
   GtkWidget *cont = handle;
   GtkListStore *store = NULL;

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      if(pointer)
      {
         row += GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_insertpos"));
      }

      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, row))
      {
         gtk_list_store_set(store, &iter, type, (gpointer)data, -1);
      }
   }
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
DW_FUNCTION_DEFINITION(dw_container_set_row_title, void, void *pointer, int row, const char *title)
DW_FUNCTION_ADD_PARAM3(pointer, row, title)
DW_FUNCTION_NO_RETURN(dw_container_set_row_title)
DW_FUNCTION_RESTORE_PARAM3(pointer, void *, row, int, title, char *)
{
   _dw_container_set_row_data_int(pointer, pointer, row, _DW_DATA_TYPE_STRING, (void *)title);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Changes the title of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          title: String title of the item.
 */
DW_FUNCTION_DEFINITION(dw_container_change_row_title, void, HWND handle, int row, const char *title)
DW_FUNCTION_ADD_PARAM3(handle, row, title)
DW_FUNCTION_NO_RETURN(dw_container_change_row_title)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, row, int, title, char *)
{
   _dw_container_set_row_data_int(handle, NULL, row, _DW_DATA_TYPE_STRING, (void *)title);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the data of a row in the container.
 * Parameters:
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
DW_FUNCTION_DEFINITION(dw_container_set_row_data, void, void *pointer, int row, void *data)
DW_FUNCTION_ADD_PARAM3(pointer, row, data)
DW_FUNCTION_NO_RETURN(dw_container_set_row_data)
DW_FUNCTION_RESTORE_PARAM3(pointer, void *, row, int, data, void *)
{
   _dw_container_set_row_data_int(pointer, pointer, row, _DW_DATA_TYPE_POINTER, data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Changes the data of a row already inserted in the container.
 * Parameters:
 *          handle: Handle to window (widget) of container.
 *          row: Zero based row of data being set.
 *          data: Data pointer.
 */
DW_FUNCTION_DEFINITION(dw_container_change_row_data, void, HWND handle, int row, void *data)
DW_FUNCTION_ADD_PARAM3(handle, row, data)
DW_FUNCTION_NO_RETURN(dw_container_change_row_data)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, row, int, data, void *)
{
   _dw_container_set_row_data_int(handle, NULL, row, _DW_DATA_TYPE_POINTER, data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          rowcount: The number of rows to be inserted.
 */
void API dw_container_insert(HWND handle, void *pointer, int rowcount)
{
   /* Don't need to do anything here */
}

/*
 * Removes the first x rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be deleted from.
 *       rowcount: The number of rows to be deleted.
 */
DW_FUNCTION_DEFINITION(dw_container_delete, void, HWND handle, int rowcount)
DW_FUNCTION_ADD_PARAM2(handle, rowcount)
DW_FUNCTION_NO_RETURN(dw_container_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, rowcount, int)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;
      int rows, z;

      rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      for(z=0;z<rowcount;z++)
      {
         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0))
            gtk_list_store_remove(store, &iter);
      }

      if(rows - rowcount < 0)
         rows = 0;
      else
         rows -= rowcount;

      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rows));
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
DW_FUNCTION_DEFINITION(dw_container_clear, void, HWND handle, DW_UNUSED(int redraw))
DW_FUNCTION_ADD_PARAM2(handle, redraw)
DW_FUNCTION_NO_RETURN(dw_container_clear)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, DW_UNUSED(redraw), int)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(0));
      g_object_set_data(G_OBJECT(cont), "_dw_insertpos", GINT_TO_POINTER(0));

      gtk_list_store_clear(store);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Scrolls container up or down.
 * Parameters:
 *       handle: Handle to the window (widget) to be scrolled.
 *       direction: DW_SCROLL_UP, DW_SCROLL_DOWN, DW_SCROLL_TOP or
 *                  DW_SCROLL_BOTTOM. (rows is ignored for last two)
 *       rows: The number of rows to be scrolled.
 */
DW_FUNCTION_DEFINITION(dw_container_scroll, void, HWND handle, int direction, long rows)
DW_FUNCTION_ADD_PARAM3(handle, direction, rows)
DW_FUNCTION_NO_RETURN(dw_container_scroll)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, direction, int, rows, long)
{
   GtkWidget *cont;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
   {
      GtkAdjustment *adjust = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(cont));

      if(adjust)
      {
         gint rowcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));
         gdouble currpos = gtk_adjustment_get_value(adjust);
         gdouble upper = gtk_adjustment_get_upper(adjust);
         gdouble lower = gtk_adjustment_get_lower(adjust);
         gdouble change;

         /* Safety check */
         if(rowcount > 0)
         {
            change = ((gdouble)rows/(gdouble)rowcount) * (upper - lower);

            switch(direction)
            {
              case DW_SCROLL_TOP:
              {
                  gtk_adjustment_set_value(adjust, lower);
                  break;
              }
              case DW_SCROLL_BOTTOM:
              {
                  gtk_adjustment_set_value(adjust, upper);
                  break;
              }
              case DW_SCROLL_UP:
              {
                  gdouble newpos = currpos - change;
                  if(newpos < lower)
                  {
                      newpos = lower;
                  }
                  gtk_adjustment_set_value(adjust, newpos);
                  break;
              }
              case DW_SCROLL_DOWN:
              {
                  gdouble newpos = currpos + change;
                  if(newpos > upper)
                  {
                      newpos = upper;
                  }
                  gtk_adjustment_set_value(adjust, newpos);
                  break;
              }
            }
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Starts a new query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
DW_FUNCTION_DEFINITION(dw_container_query_start, char *, HWND handle, unsigned long flags)
DW_FUNCTION_ADD_PARAM2(handle, flags)
DW_FUNCTION_RETURN(dw_container_query_start, char *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, flags, unsigned long)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   char *retval = NULL;
   int type = flags & DW_CR_RETDATA ? _DW_DATA_TYPE_POINTER : _DW_DATA_TYPE_STRING;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      /* These should be separate but right now this will work */
      if(flags & DW_CRA_SELECTED)
      {
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cont));
         GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);
         if(list)
         {
            GtkTreePath *path = g_list_nth_data(list, 0);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, indices[0]))
                  {
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
                     g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(1));
                  }
               }
            }
            g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free(list);
         }
      }
      else if(flags & DW_CRA_CURSORED)
      {
         GtkTreePath *path;

         gtk_tree_view_get_cursor(GTK_TREE_VIEW(cont), &path, NULL);
         if(path)
         {
            GtkTreeIter iter;

            if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path))
            {
               gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            }
            gtk_tree_path_free(path);
         }
      }
      else
      {
         GtkTreeIter iter;

         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0))
         {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(1));
         }
      }
   }
   /* If there is a title, duplicate it and free the temporary copy */
   if(retval && type == _DW_DATA_TYPE_STRING)
   {
      char *temp = retval;
      retval = strdup(temp);
      g_free(temp);
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Continues an existing query of a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       flags: If this parameter is DW_CRA_SELECTED it will only
 *              return items that are currently selected.  Otherwise
 *              it will return all records in the container.
 */
DW_FUNCTION_DEFINITION(dw_container_query_next, char *, HWND handle, unsigned long flags)
DW_FUNCTION_ADD_PARAM2(handle, flags)
DW_FUNCTION_RETURN(dw_container_query_next, char *)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, flags, unsigned long)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;
   char *retval = NULL;
   int type = flags & DW_CR_RETDATA ? _DW_DATA_TYPE_POINTER : _DW_DATA_TYPE_STRING;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      int pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_querypos"));
      int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);

      /* These should be separate but right now this will work */
      if(flags & DW_CRA_SELECTED)
      {
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cont));
         GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);

         if(list)
         {
            GtkTreePath *path = g_list_nth_data(list, pos);

            if(path)
            {
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  GtkTreeIter iter;

                  if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, indices[0]))
                  {
                     gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
                     g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(pos+1));
                  }
               }
            }
            g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free(list);
         }
      }
      else if(flags & DW_CRA_CURSORED)
      {
         /* There will only be one item cursored,
          * retrieve it with dw_container_query_start()
          */
         retval = NULL;
      }
      else
      {
         GtkTreeIter iter;

         if(pos < count && gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, pos))
         {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, type, &retval, -1);
            g_object_set_data(G_OBJECT(cont), "_dw_querypos", GINT_TO_POINTER(pos+1));
         }
      }
   }
   /* If there is a title, duplicate it and free the temporary copy */
   if(retval && type == _DW_DATA_TYPE_STRING)
   {
      char *temp = retval;
      retval = strdup(temp);
      g_free(temp);
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

int _dw_find_iter(GtkListStore *store, GtkTreeIter *iter, void *data, int textcomp)
{
   int z, rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
   void *thisdata;
   int retval = FALSE;

   for(z=0;z<rows;z++)
   {
      if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, NULL, z))
      {
         /* Get either string from position 0 or pointer from position 1 */
         gtk_tree_model_get(GTK_TREE_MODEL(store), iter, textcomp ? _DW_DATA_TYPE_STRING : _DW_DATA_TYPE_POINTER, &thisdata, -1);
         if((textcomp && thisdata && strcmp((char *)thisdata, (char *)data) == 0) || (!textcomp && thisdata == data))
         {
            retval = TRUE;
            z = rows;
         }
         if(textcomp && thisdata)
            g_free(thisdata);
      }
   }
   return retval;
}

void _dw_container_cursor_int(HWND handle, void *data, int textcomp)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;

      if(_dw_find_iter(store, &iter, data, textcomp))
      {
         GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);

         if(path)
         {
            gtk_tree_view_row_activated(GTK_TREE_VIEW(cont), path, NULL);
            gtk_tree_path_free(path);
         }
      }
   }
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_cursor, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_container_cursor)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
   _dw_container_cursor_int(handle, (void *)text, TRUE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Cursors the item with the text speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_cursor_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_cursor_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
   _dw_container_cursor_int(handle, data, FALSE);
   DW_FUNCTION_RETURN_NOTHING;
}

void _dw_container_delete_row_int(HWND handle, void *data, int textcomp)
{
   GtkWidget *cont;
   GtkListStore *store = NULL;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cont));

   if(store)
   {
      GtkTreeIter iter;
      int rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cont), "_dw_rowcount"));

      if(_dw_find_iter(store, &iter, data, textcomp))
      {
         gtk_list_store_remove(store, &iter);
         rows--;
      }

      g_object_set_data(G_OBJECT(cont), "_dw_rowcount", GINT_TO_POINTER(rows));
   }
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_delete_row, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_container_delete_row)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
   _dw_container_delete_row_int(handle, (void *)text, TRUE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with the text speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       text:  Text usually returned by dw_container_query().
 */
DW_FUNCTION_DEFINITION(dw_container_delete_row_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_delete_row_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
   _dw_container_delete_row_int(handle, data, FALSE);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Optimizes the column widths so that all data is visible.
 * Parameters:
 *       handle: Handle to the window (widget) to be optimized.
 */
DW_FUNCTION_DEFINITION(dw_container_optimize, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_container_optimize)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   GtkWidget *cont;

   cont = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(cont && GTK_IS_TREE_VIEW(cont) && g_object_get_data(G_OBJECT(cont), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_CONTAINER))
         gtk_tree_view_columns_autosize(GTK_TREE_VIEW(cont));
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Inserts an icon into the taskbar.
 * Parameters:
 *       handle: Window handle that will handle taskbar icon messages.
 *       icon: Icon handle to display in the taskbar.
 *       bubbletext: Text to show when the mouse is above the icon.
 */
void API dw_taskbar_insert(HWND handle, HICN icon, const char *bubbletext)
{
   /* TODO: Removed in GTK4.... no replacement? */
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
   /* TODO: Removed in GTK4.... no replacement? */
}

/* Make sure the widget is out of the dirty list if it is destroyed */
static void _dw_render_destroy(GtkWidget *widget, gpointer data)
{
   cairo_surface_t *surface = (cairo_surface_t *)g_object_get_data(G_OBJECT(widget), "_dw_cr_surface");

   if(surface)
      cairo_surface_destroy(surface);

   _dw_dirty_list = g_list_remove(_dw_dirty_list, widget);
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_render_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_render_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GtkWidget *tmp = gtk_drawing_area_new();
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   g_signal_connect(G_OBJECT(tmp), "destroy", G_CALLBACK(_dw_render_destroy), NULL);
   gtk_widget_set_can_focus(tmp, TRUE);
   gtk_widget_set_visible(tmp, TRUE);
   if(_DWDefaultFont)
      dw_window_set_font(tmp, _DWDefaultFont);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Invalidate the render widget triggering an expose event.
 * Parameters:
 *       handle: A handle to a render widget to be redrawn.
 */
DW_FUNCTION_DEFINITION(dw_render_redraw, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_render_redraw)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   if(handle && GTK_IS_WIDGET(handle))
      gtk_widget_queue_draw(handle);
   DW_FUNCTION_RETURN_NOTHING;
}

/* Returns a GdkRGBA from a DW color */
static GdkRGBA _dw_internal_color(unsigned long value)
{
   if(DW_RGB_COLOR & value)
   {
      GdkRGBA color = { (gfloat)DW_RED_VALUE(value) / 255.0, (gfloat)DW_GREEN_VALUE(value) / 255.0, (gfloat)DW_BLUE_VALUE(value) / 255.0, 1.0 };
      return color;
   }
   if (value < 16)
      return _dw_colors[value];
   return _dw_colors[0];
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
   GdkRGBA color = _dw_internal_color(value);
   GdkRGBA *foreground = pthread_getspecific(_dw_fg_color_key);

   *foreground = color;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
   GdkRGBA *background = pthread_getspecific(_dw_bg_color_key);

   if(value == DW_CLR_DEFAULT)
   {
      if(background)
      {
         pthread_setspecific(_dw_bg_color_key, NULL);
         free(background);
      }
   }
   else
   {
      GdkRGBA color = _dw_internal_color(value);

      if(!background)
      {
         background = malloc(sizeof(GdkRGBA));
         pthread_setspecific(_dw_bg_color_key, background);
      }
      *background = color;
   }
}

#if GTK_CHECK_VERSION(4,10,0)
static void _dw_color_choose_response(GObject *gobject, GAsyncResult *result, gpointer data)
{
  DWDialog *tmp = data;
  GError *error = NULL;
  GdkRGBA *newcol = gtk_color_dialog_choose_rgba_finish(GTK_COLOR_DIALOG(gobject), result, &error);

  if(error != NULL && newcol != NULL)
  {
      g_free(newcol);
      newcol = NULL;
  }
  dw_dialog_dismiss(tmp, newcol);
}
#endif

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
DW_FUNCTION_DEFINITION(dw_color_choose, ULONG, ULONG value)
DW_FUNCTION_ADD_PARAM1(value)
DW_FUNCTION_RETURN(dw_color_choose, ULONG)
DW_FUNCTION_RESTORE_PARAM1(value, ULONG)
{
   GdkRGBA color = _dw_internal_color(value);
   unsigned long retcolor = value;
   DWDialog *tmp = dw_dialog_new(NULL);
#if GTK_CHECK_VERSION(4,10,0)
   GtkColorDialog *cd = gtk_color_dialog_new();
   GdkRGBA *newcol;

   gtk_color_dialog_choose_rgba(cd, NULL, &color, NULL, (GAsyncReadyCallback)_dw_color_choose_response, tmp);

   newcol = dw_dialog_wait(tmp);

   if(newcol)
   {
      retcolor = DW_RGB((int)(newcol->red * 255), (int)(newcol->green * 255), (int)(newcol->blue * 255));
      g_free(newcol);
   }
#else
   GtkColorChooser *cd;

   cd = (GtkColorChooser *)gtk_color_chooser_dialog_new("Choose color", NULL);
   gtk_color_chooser_set_use_alpha(cd, FALSE);
   gtk_color_chooser_set_rgba(cd, &color);

   gtk_widget_set_visible(GTK_WIDGET(cd), TRUE);
   g_signal_connect(G_OBJECT(cd), "response", G_CALLBACK(_dw_dialog_response), (gpointer)tmp);

   if(DW_POINTER_TO_INT(dw_dialog_wait(tmp)) == GTK_RESPONSE_OK)
   {
      gtk_color_chooser_get_rgba(cd, &color);
      retcolor = DW_RGB((int)(color.red * 255), (int)(color.green * 255), (int)(color.blue * 255));
   }
   if(GTK_IS_WINDOW(cd))
      gtk_window_destroy(GTK_WINDOW(cd));
#endif
   DW_FUNCTION_RETURN_THIS(retcolor);
}

/* Draw a point on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
 */
DW_FUNCTION_DEFINITION(dw_draw_point, void, HWND handle, HPIXMAP pixmap, int x, int y)
DW_FUNCTION_ADD_PARAM4(handle, pixmap, x, y)
DW_FUNCTION_NO_RETURN(dw_draw_point)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, pixmap, HPIXMAP, x, int, y, int)
{
   cairo_t *cr = NULL;
   int cached = FALSE;

   if(handle)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(handle, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
#ifdef _DW_SINGLE_THREADED
      GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

      gdk_cairo_set_source_rgba(cr, _dw_fg_color);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x, y);
      cairo_stroke(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Draw a line on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x1: First X coordinate.
 *       y1: First Y coordinate.
 *       x2: Second X coordinate.
 *       y2: Second Y coordinate.
 */
DW_FUNCTION_DEFINITION(dw_draw_line, void, HWND handle, HPIXMAP pixmap, int x1, int y1, int x2, int y2)
DW_FUNCTION_ADD_PARAM6(handle, pixmap, x1, y1, x2, y2)
DW_FUNCTION_NO_RETURN(dw_draw_line)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, pixmap, HPIXMAP, x1, int, y1, int, x2, int, y2, int)
{
   cairo_t *cr = NULL;
   int cached = FALSE;

   if(handle)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(handle, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
#ifdef _DW_SINGLE_THREADED
      GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

      gdk_cairo_set_source_rgba(cr, _dw_fg_color);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x1, y1);
      cairo_line_to(cr, x2, y2);
      cairo_stroke(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Draw a closed polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       number of points
 *       x[]: X coordinates.
 *       y[]: Y coordinates.
 */
DW_FUNCTION_DEFINITION(dw_draw_polygon, void, HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
DW_FUNCTION_ADD_PARAM6(handle, pixmap, flags, npoints, x, y)
DW_FUNCTION_NO_RETURN(dw_draw_polygon)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, pixmap, HPIXMAP, flags, int, npoints, int, x, int *, y, int *)
{
   cairo_t *cr = NULL;
   int z;
   int cached = FALSE;

   if(handle)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(handle, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
#ifdef _DW_SINGLE_THREADED
      GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, _dw_fg_color);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x[0], y[0]);
      for(z=1;z<npoints;z++)
      {
         cairo_line_to(cr, x[z], y[z]);
      }
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Draw a rectangle on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the box or DW_DRAW_DEFAULT (0).
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
DW_FUNCTION_DEFINITION(dw_draw_rect, void, HWND handle, HPIXMAP pixmap, int flags, int x, int y, int width, int height)
DW_FUNCTION_ADD_PARAM7(handle, pixmap, flags, x, y, width, height)
DW_FUNCTION_NO_RETURN(dw_draw_rect)
DW_FUNCTION_RESTORE_PARAM7(handle, HWND, pixmap, HPIXMAP, flags, int, x, int, y, int, width, int, height, int)
{
   cairo_t *cr = NULL;
   int cached = FALSE;

   if(handle)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(handle, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
#ifdef _DW_SINGLE_THREADED
      GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, _dw_fg_color);
      cairo_set_line_width(cr, 1);
      cairo_move_to(cr, x, y);
      cairo_line_to(cr, x, y + height);
      cairo_line_to(cr, x + width, y + height);
      cairo_line_to(cr, x + width, y);
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Draw an arc on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the arc or DW_DRAW_DEFAULT (0).
 *              DW_DRAW_FULL will draw a complete circle/elipse.
 *       xorigin: X coordinate of center of arc.
 *       yorigin: Y coordinate of center of arc.
 *       x1: X coordinate of first segment of arc.
 *       y1: Y coordinate of first segment of arc.
 *       x2: X coordinate of second segment of arc.
 *       y2: Y coordinate of second segment of arc.
 */
DW_FUNCTION_DEFINITION(dw_draw_arc, void, HWND handle, HPIXMAP pixmap, int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2)
DW_FUNCTION_ADD_PARAM9(handle, pixmap, flags, xorigin, yorigin, x1, y1, x2, y2)
DW_FUNCTION_NO_RETURN(dw_draw_arc)
DW_FUNCTION_RESTORE_PARAM9(handle, HWND, pixmap, HPIXMAP, flags, int, xorigin, int, yorigin, int, x1, int, y1, int, x2, int, y2, int)
{
   cairo_t *cr = NULL;
   int cached = FALSE;

   if(handle)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(handle, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(pixmap)
      cr = cairo_create(pixmap->image);
   if(cr)
   {
#ifdef _DW_SINGLE_THREADED
      GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif
      int width = abs(x2-x1);
      float scale = fabs((float)(y2-y1))/(float)width;

      if(flags & DW_DRAW_NOAA)
         cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

      gdk_cairo_set_source_rgba(cr, _dw_fg_color);
      cairo_set_line_width(cr, 1);
      if(scale != 1.0)
         cairo_scale(cr, 1.0, scale);
      if(flags & DW_DRAW_FULL)
         cairo_arc(cr, xorigin, yorigin / scale, width/2, 0, M_PI*2);
      else
      {
         double dx = xorigin - x1;
         double dy = yorigin - y1;
         double r = sqrt(dx*dx + dy*dy);
         double a1 = atan2((y1-yorigin), (x1-xorigin));
         double a2 = atan2((y2-yorigin), (x2-xorigin));

         cairo_arc(cr, xorigin, yorigin, r, a1, a2);
      }
      if(flags & DW_DRAW_FILL)
         cairo_fill(cr);
      cairo_stroke(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Draw text on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       x: X coordinate.
 *       y: Y coordinate.
*       text: Text to be displayed.
  */
DW_FUNCTION_DEFINITION(dw_draw_text, void, HWND handle, HPIXMAP pixmap, int x, int y, const char *text)
DW_FUNCTION_ADD_PARAM5(handle, pixmap, x, y, text)
DW_FUNCTION_NO_RETURN(dw_draw_text)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pixmap, HPIXMAP, x, int, y, int, text, const char *)
{
   if(text)
   {
      cairo_t *cr = NULL;
      PangoFontDescription *font;
      char *tmpname, *fontname = "monospace 10";
      int cached = FALSE;

      if(handle)
      {
         cairo_surface_t *surface;

         if((cr = _dw_cairo_update(handle, -1, -1)))
            cached = TRUE;
         else if((surface = g_object_get_data(G_OBJECT(handle), "_dw_cr_surface")))
            cr = cairo_create(surface);
         if((tmpname = (char *)g_object_get_data(G_OBJECT(handle), "_dw_fontname")))
            fontname = tmpname;
      }
      else if(pixmap)
      {
         if(pixmap->font)
            fontname = pixmap->font;
         else if(pixmap->handle && (tmpname = (char *)g_object_get_data(G_OBJECT(pixmap->handle), "_dw_fontname")))
            fontname = tmpname;
         cr = cairo_create(pixmap->image);
      }
      if(cr)
      {
         font = pango_font_description_from_string(fontname);
         if(font)
         {
            PangoContext *context = pango_cairo_create_context(cr);

            if(context)
            {
               PangoLayout *layout = pango_layout_new(context);

               if(layout)
               {
#ifdef _DW_SINGLE_THREADED
                  GdkRGBA *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
                  GdkRGBA *_dw_bg_color = pthread_getspecific(_dw_bg_color_key);
#endif

                  pango_layout_set_font_description(layout, font);
                  pango_layout_set_text(layout, text, strlen(text));

                  gdk_cairo_set_source_rgba(cr, _dw_fg_color);
                  /* Create a background color attribute if required */
                  if(_dw_bg_color)
                  {
                     PangoAttrList *list = pango_layout_get_attributes(layout);
                     PangoAttribute *attr = pango_attr_background_new((guint16)(_dw_bg_color->red * 65535),
                                                                      (guint16)(_dw_bg_color->green * 65535),
                                                                      (guint16)(_dw_bg_color->blue* 65535));
                     if(!list)
                     {
                        list = pango_attr_list_new();
                     }
                     pango_attr_list_change(list, attr);
                     pango_layout_set_attributes(layout, list);
                  }
                  /* Do the drawing */
                  cairo_move_to(cr, x, y);
                  pango_cairo_show_layout (cr, layout);

                  g_object_unref(layout);
               }
               g_object_unref(context);
            }
            pango_font_description_free(font);
         }
         /* If we are using a drawing context...
          * we don't own the cairo context so don't destroy it.
          */
         if(!cached)
            cairo_destroy(cr);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Query the width and height of a text string.
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       text: Text to be queried.
 *       width: Pointer to a variable to be filled in with the width.
 *       height Pointer to a variable to be filled in with the height.
 */
DW_FUNCTION_DEFINITION(dw_font_text_extents_get, void, HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
DW_FUNCTION_ADD_PARAM5(handle, pixmap, text, width, height)
DW_FUNCTION_NO_RETURN(dw_font_text_extents_get)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, pixmap, HPIXMAP, text, const char *, width, int *, height, int *)
{
   PangoFontDescription *font;
   char *fontname = NULL;
   int free_fontname = FALSE;

   if(text)
   {
      if(handle)
      {
         fontname = (char *)g_object_get_data(G_OBJECT(handle), "_dw_fontname");
         if(fontname == NULL)
         {
            fontname = dw_window_get_font(handle);
            free_fontname = TRUE;
         }
      }
      else if(pixmap)
      {
         if(pixmap->font)
            fontname = pixmap->font;
         else if(pixmap->handle)
            fontname = (char *)g_object_get_data(G_OBJECT(pixmap->handle), "_dw_fontname");
      }

      font = pango_font_description_from_string(fontname ? fontname : "monospace 10");
      if(font)
      {
         PangoContext *context = gtk_widget_get_pango_context(pixmap ? pixmap->handle : handle);

         if(context)
         {
            PangoLayout *layout = pango_layout_new(context);

            if(layout)
            {
               PangoRectangle rect;

               pango_layout_set_font_description(layout, font);
               pango_layout_set_text(layout, text, -1);
               pango_layout_get_pixel_extents(layout, NULL, &rect);

               if(width)
                  *width = rect.width;
               if(height)
                  *height = rect.height;

               g_object_unref(layout);
            }
         }
         pango_font_description_free(font);
      }
      if(free_fontname)
         free(fontname);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Creates a pixmap with given parameters.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *               or zero to enable this pixmap to be written
 *               off the screen to reduce flicker
 *       width: Width of the pixmap in pixels.
 *       height: Height of the pixmap in pixels.
 *       depth: Color depth of the pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_pixmap_new, HPIXMAP, HWND handle, unsigned long width, unsigned long height, DW_UNUSED(int depth))
DW_FUNCTION_ADD_PARAM4(handle, width, height, depth)
DW_FUNCTION_RETURN(dw_pixmap_new, HPIXMAP)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, width, unsigned long, height, unsigned long, DW_UNUSED(depth), int)
{
   HPIXMAP pixmap = 0;

   if((pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      pixmap->width = width;
      pixmap->height = height;
      pixmap->handle = handle;
      /* Depth needs to be divided by 3... but for the RGB colorspace...
       * only 8 bits per sample is allowed, so to avoid issues just pass 8 for now.
       */
      pixmap->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
      pixmap->image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
   }
   DW_FUNCTION_RETURN_THIS(pixmap);
}

/*
 * Creates a pixmap from a file.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_pixmap_new_from_file, HPIXMAP, HWND handle, const char *filename)
DW_FUNCTION_ADD_PARAM2(handle, filename)
DW_FUNCTION_RETURN(dw_pixmap_new_from_file, HPIXMAP)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, filename, const char *)
{
   HPIXMAP pixmap = 0;

   if(filename && *filename && (pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      char *file = alloca(strlen(filename) + 6);

      strcpy(file, filename);

      /* check if we can read from this file (it exists and read permission) */
      if(access(file, 04) != 0)
      {
         int i = 0;

         /* Try with various extentions */
         while(_dw_image_exts[i] && !pixmap->pixbuf)
         {
            strcpy(file, filename);
            strcat(file, _dw_image_exts[i]);
            if(access(file, 04) == 0)
               pixmap->pixbuf = gdk_pixbuf_new_from_file(file, NULL);
            i++;
         }
      }
      else
         pixmap->pixbuf = gdk_pixbuf_new_from_file(file, NULL);

      if(pixmap->pixbuf)
      {
         pixmap->image = cairo_image_surface_create_from_png(file);
         pixmap->width = gdk_pixbuf_get_width(pixmap->pixbuf);
         pixmap->height = gdk_pixbuf_get_height(pixmap->pixbuf);
         pixmap->handle = handle;
      }
      else
      {
         free(pixmap);
         pixmap = 0;
      }
   }
   DW_FUNCTION_RETURN_THIS(pixmap);
}

/*
 * Creates a pixmap from data
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of image data
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_pixmap_new_from_data, HPIXMAP, HWND handle, const char *data, int len)
DW_FUNCTION_ADD_PARAM3(handle, data, len)
DW_FUNCTION_RETURN(dw_pixmap_new_from_data, HPIXMAP)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, data, const char *, len, int)
{
   HPIXMAP pixmap = 0;

   if(data && len > 0 && (pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      int fd, written = -1;
      char template[] = "/tmp/dwpixmapXXXXXX";

      /*
       * A real hack; create a temporary file and write the contents
       * of the data to the file
       */
      if((fd = mkstemp(template)) != -1)
      {
         written = write(fd, data, len);
         close(fd);
      }
      /* Bail if we couldn't write full file */
      if(fd != -1 && written == len)
      {
         pixmap->pixbuf = gdk_pixbuf_new_from_file(template, NULL);
         pixmap->image = cairo_image_surface_create_from_png(template);
         pixmap->width = gdk_pixbuf_get_width(pixmap->pixbuf);
         pixmap->height = gdk_pixbuf_get_height(pixmap->pixbuf);
         /* remove our temporary file */
         unlink(template);
         pixmap->handle = handle;
      }
      else
      {
         free(pixmap);
         pixmap = 0;
      }
   }
   DW_FUNCTION_RETURN_THIS(pixmap);
}

/*
 * Sets the transparent color for a pixmap
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  transparent color
 * Note: This does nothing on GTK+ as transparency
 *       is handled automatically
 */
void API dw_pixmap_set_transparent_color(HPIXMAP pixmap, unsigned long color)
{
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_pixmap_grab, HPIXMAP, HWND handle, ULONG id)
DW_FUNCTION_ADD_PARAM2(handle, id)
DW_FUNCTION_RETURN(dw_pixmap_grab, HPIXMAP)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, id, ULONG)
{
   HPIXMAP pixmap = 0;

   if((pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      pixmap->pixbuf = gdk_pixbuf_copy(_dw_find_pixbuf((HICN)id, &pixmap->width, &pixmap->height));
      pixmap->handle = handle;
   }
   DW_FUNCTION_RETURN_THIS(pixmap);
}

static void _dw_flush_dirty(gpointer widget, gpointer data)
{
   if(widget && GTK_IS_WIDGET(widget))
      gtk_widget_queue_draw(GTK_WIDGET(widget));
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
DW_FUNCTION_DEFINITION(dw_flush, void)
DW_FUNCTION_ADD_PARAM
DW_FUNCTION_NO_RETURN(dw_flush)
{
   g_list_foreach(_dw_dirty_list, _dw_flush_dirty, NULL);
   g_list_free(_dw_dirty_list);
   _dw_dirty_list = NULL;
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the font used by a specified pixmap.
 * Normally the pixmap font is obtained from the associated window handle.
 * However this can be used to override that, or for pixmaps with no window.
 * Parameters:
 *          pixmap: Handle to a pixmap returned by dw_pixmap_new() or
 *                  passed to the application via a callback.
 *          fontname: Name and size of the font in the form "size.fontname"
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
int API dw_pixmap_set_font(HPIXMAP pixmap, const char *fontname)
{
    if(pixmap)
    {
         char *oldfont = pixmap->font;

         pixmap->font = _dw_convert_font(fontname);

         if(oldfont)
             free(oldfont);
         return DW_ERROR_NONE;
    }
    return DW_ERROR_GENERAL;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
DW_FUNCTION_DEFINITION(dw_pixmap_destroy, void, HPIXMAP pixmap)
DW_FUNCTION_ADD_PARAM1(pixmap)
DW_FUNCTION_NO_RETURN(dw_pixmap_destroy)
DW_FUNCTION_RESTORE_PARAM1(pixmap, HPIXMAP)
{
   g_object_unref(G_OBJECT(pixmap->pixbuf));
   cairo_surface_destroy(pixmap->image);
   if(pixmap->font)
      free(pixmap->font);
   free(pixmap);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the width of the pixmap, same as the DW_PIXMAP_WIDTH() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_width(HPIXMAP pixmap)
{
    return pixmap ? pixmap->width : 0;
}

/*
 * Returns the height of the pixmap, same as the DW_PIXMAP_HEIGHT() macro,
 * but exported as an API, for non-C language bindings.
 */
unsigned long API dw_pixmap_get_height(HPIXMAP pixmap)
{
    return pixmap ? pixmap->height : 0;
}

/*
 * Copies from one item to another.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of area to copy.
 *       height: Height of area to copy.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 */
void API dw_pixmap_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc)
{
    dw_pixmap_stretch_bitblt(dest, destp, xdest, ydest, width, height, src, srcp, xsrc, ysrc, -1, -1);
}

/*
 * Copies from one surface to another allowing for stretching.
 * Parameters:
 *       dest: Destination window handle.
 *       destp: Destination pixmap. (choose only one).
 *       xdest: X coordinate of destination.
 *       ydest: Y coordinate of destination.
 *       width: Width of the target area.
 *       height: Height of the target area.
 *       src: Source window handle.
 *       srcp: Source pixmap. (choose only one).
 *       xsrc: X coordinate of source.
 *       ysrc: Y coordinate of source.
 *       srcwidth: Width of area to copy.
 *       srcheight: Height of area to copy.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
DW_FUNCTION_DEFINITION(dw_pixmap_stretch_bitblt, int, HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc, int srcwidth, int srcheight)
DW_FUNCTION_ADD_PARAM12(dest, destp, xdest, ydest, width, height, src, srcp, xsrc, ysrc, srcwidth, srcheight)
DW_FUNCTION_RETURN(dw_pixmap_stretch_bitblt, int)
DW_FUNCTION_RESTORE_PARAM12(dest, HWND, destp, HPIXMAP, xdest, int, ydest, int, width, int, height, int, src, HWND, srcp, HPIXMAP, xsrc, int, ysrc, int, srcwidth, int, srcheight, int)

{
   cairo_t *cr = NULL;
   int retval = DW_ERROR_GENERAL;
   int cached = FALSE;

   if(dest)
   {
      cairo_surface_t *surface;

      if((cr = _dw_cairo_update(dest, -1, -1)))
         cached = TRUE;
      else if((surface = g_object_get_data(G_OBJECT(dest), "_dw_cr_surface")))
         cr = cairo_create(surface);
   }
   else if(destp)
      cr = cairo_create(destp->image);

   if(cr)
   {
      double xscale = 1, yscale = 1;

      if(srcwidth != -1 && srcheight != -1)
      {
         xscale = (double)width / (double)srcwidth;
         yscale = (double)height / (double)srcheight;
         cairo_scale(cr, xscale, yscale);
      }

      if(src)
      {
         cairo_surface_t *surface = g_object_get_data(G_OBJECT(src), "_dw_cr_surface");
         if(surface)
            cairo_set_source_surface(cr, surface, (xdest + xsrc) / xscale, (ydest + ysrc) / yscale);
      }        
      else if(srcp)
         cairo_set_source_surface(cr, srcp->image, (xdest + xsrc) / xscale, (ydest + ysrc) / yscale);

      cairo_rectangle(cr, xdest / xscale, ydest / yscale, width, height);
      cairo_fill(cr);
      /* If we are using a drawing context...
       * we don't own the cairo context so don't destroy it.
       */
      if(!cached)
         cairo_destroy(cr);
      retval = DW_ERROR_NONE;
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
   gdk_display_beep(gdk_display_get_default());
}

void _dw_strlwr(char *buf)
{
   int z, len = strlen(buf);

   for(z=0;z<len;z++)
   {
      if(buf[z] >= 'A' && buf[z] <= 'Z')
         buf[z] -= 'A' - 'a';
   }
}

/* Open a shared library and return a handle.
 * Parameters:
 *         name: Base name of the shared library.
 *         handle: Pointer to a module handle,
 *                 will be filled in with the handle.
 */
int API dw_module_load(const char *name, HMOD *handle)
{
   int len;
   char *newname;
   char errorbuf[1025] = {0};


   if(!handle)
      return -1;

   if((len = strlen(name)) == 0)
      return   -1;

   /* Lenth + "lib" + ".so" + NULL */
   newname = malloc(len + 7);

   if(!newname)
      return -1;

   sprintf(newname, "lib%s.so", name);
   _dw_strlwr(newname);

   *handle = dlopen(newname, RTLD_NOW);
   if(*handle == NULL)
   {
      strncpy(errorbuf, dlerror(), 1024);
      printf("%s\n", errorbuf);
      sprintf(newname, "lib%s.so", name);
      *handle = dlopen(newname, RTLD_NOW);
   }

   free(newname);

   return (NULL == *handle) ? -1 : 0;
}

/* Queries the address of a symbol within open handle.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 *         name: Name of the symbol you want the address of.
 *         func: A pointer to a function pointer, to obtain
 *               the address.
 */
int API dw_module_symbol(HMOD handle, const char *name, void**func)
{
   if(!func || !name)
      return   -1;

   if(strlen(name) == 0)
      return   -1;

   *func = (void*)dlsym(handle, name);
   return   (NULL == *func);
}

/* Frees the shared library previously opened.
 * Parameters:
 *         handle: Module handle returned by dw_module_load()
 */
int API dw_module_close(HMOD handle)
{
   if(handle)
      return dlclose(handle);
   return 0;
}

/*
 * Returns the handle to an unnamed mutex semaphore.
 */
HMTX API dw_mutex_new(void)
{
   HMTX mutex = malloc(sizeof(pthread_mutex_t));

   pthread_mutex_init(mutex, NULL);
   return mutex;
}

/*
 * Closes a semaphore created by dw_mutex_new().
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_close(HMTX mutex)
{
   if(mutex)
   {
      pthread_mutex_destroy(mutex);
      free(mutex);
   }
}

/*
 * Tries to gain access to the semaphore, if it can't it blocks.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_lock(HMTX mutex)
{
    /* We need to handle locks from the main thread differently...
     * since we can't stop message processing... otherwise we
     * will deadlock... so try to acquire the lock and continue
     * processing messages in between tries.
     */
    if(_dw_thread == pthread_self())
    {
        while(pthread_mutex_trylock(mutex) != 0)
        {
            /* Process any pending events */
            if(g_main_context_pending(NULL))
            {
               do 
               {
                  g_main_context_iteration(NULL, FALSE);
               } 
               while(g_main_context_pending(NULL));
            }
            else
               sched_yield();
        }
    }
    else
    {
        pthread_mutex_lock(mutex);
    }
}

/*
 * Tries to gain access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 * Returns:
 *       DW_ERROR_NONE on success, DW_ERROR_TIMEOUT if it is already locked.
 */
int API dw_mutex_trylock(HMTX mutex)
{
    if(pthread_mutex_trylock(mutex) == 0)
        return DW_ERROR_NONE;
    return DW_ERROR_TIMEOUT;
}

/*
 * Reliquishes the access to the semaphore.
 * Parameters:
 *       mutex: The handle to the mutex returned by dw_mutex_new().
 */
void API dw_mutex_unlock(HMTX mutex)
{
   pthread_mutex_unlock(mutex);
}

/*
 * Returns the handle to an unnamed event semaphore.
 */
HEV API dw_event_new(void)
{
   HEV eve = (HEV)malloc(sizeof(struct _dw_unix_event));

   if(!eve)
      return NULL;

   /* We need to be careful here, mutexes on Linux are
    * FAST by default but are error checking on other
    * systems such as FreeBSD and OS/2, perhaps others.
    */
   pthread_mutex_init (&(eve->mutex), NULL);
   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_init (&(eve->event), NULL);

   pthread_mutex_unlock (&(eve->mutex));
   eve->alive = 1;
   eve->posted = 0;

   return eve;
}

/*
 * Resets a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_reset (HEV eve)
{
   if(!eve)
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   pthread_cond_init (&(eve->event), NULL);
   eve->posted = 0;
   pthread_mutex_unlock (&(eve->mutex));
   return DW_ERROR_NONE;
}

/*
 * Posts a semaphore created by dw_event_new(). Causing all threads
 * waiting on this event in dw_event_wait to continue.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_post (HEV eve)
{
   if(!eve)
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   eve->posted = 1;
   pthread_mutex_unlock (&(eve->mutex));
   return DW_ERROR_NONE;
}

/*
 * Waits on a semaphore created by dw_event_new(), until the
 * event gets posted or until the timeout expires.
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_wait(HEV eve, unsigned long timeout)
{
   int rc;

   if(!eve)
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&(eve->mutex));

   if(eve->posted)
   {
      pthread_mutex_unlock (&(eve->mutex));
      return DW_ERROR_NONE;
   }

   if(timeout != -1)
   {
      struct timeval now;
      struct timespec timeo;

      gettimeofday(&now, 0);
      timeo.tv_sec = now.tv_sec + (timeout / 1000);
      timeo.tv_nsec = now.tv_usec * 1000;
      rc = pthread_cond_timedwait(&(eve->event), &(eve->mutex), &timeo);
   }
   else
      rc = pthread_cond_wait(&(eve->event), &(eve->mutex));

   pthread_mutex_unlock (&(eve->mutex));
   if(!rc)
      return DW_ERROR_NONE;
   if(rc == ETIMEDOUT)
      return DW_ERROR_TIMEOUT;
   return DW_ERROR_GENERAL;
}

/*
 * Closes a semaphore created by dw_event_new().
 * Parameters:
 *       eve: The handle to the event returned by dw_event_new().
 */
int API dw_event_close(HEV *eve)
{
   if(!eve || !(*eve))
      return DW_ERROR_NON_INIT;

   pthread_mutex_lock (&((*eve)->mutex));
   pthread_cond_destroy (&((*eve)->event));
   pthread_mutex_unlock (&((*eve)->mutex));
   pthread_mutex_destroy (&((*eve)->mutex));
   free(*eve);
   *eve = NULL;

   return DW_ERROR_NONE;
}

struct _dw_seminfo {
   int fd;
   int waiting;
};

static void _dw_handle_sem(int *tmpsock)
{
   fd_set rd;
   struct _dw_seminfo *array = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo));
   int listenfd = tmpsock[0];
   int bytesread, connectcount = 1, maxfd, z, posted = 0;
   char command;
   sigset_t mask;

   sigfillset(&mask); /* Mask all allowed signals */
   pthread_sigmask(SIG_BLOCK, &mask, NULL);

   /* problems */
   if(tmpsock[1] == -1)
   {
      free(array);
      return;
   }

   array[0].fd = tmpsock[1];
   array[0].waiting = 0;

   /* Free the memory allocated in dw_named_event_new. */
   free(tmpsock);

   while(1)
   {
      FD_ZERO(&rd);
      FD_SET(listenfd, &rd);
      int DW_UNUSED(result);

      maxfd = listenfd;

      /* Added any connections to the named event semaphore */
      for(z=0;z<connectcount;z++)
      {
         if(array[z].fd > maxfd)
            maxfd = array[z].fd;

         FD_SET(array[z].fd, &rd);
      }

      if(select(maxfd+1, &rd, NULL, NULL, NULL) == -1)
      {
         free(array);
         return;
      }

      if(FD_ISSET(listenfd, &rd))
      {
         struct _dw_seminfo *newarray;
         int newfd = accept(listenfd, 0, 0);

         if(newfd > -1)
         {
            /* Add new connections to the set */
            newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount+1));
            memcpy(newarray, array, sizeof(struct _dw_seminfo)*(connectcount));

            newarray[connectcount].fd = newfd;
            newarray[connectcount].waiting = 0;

            connectcount++;

            /* Replace old array with new one */
            free(array);
            array = newarray;
         }
      }

      /* Handle any events posted to the semaphore */
      for(z=0;z<connectcount;z++)
      {
         if(FD_ISSET(array[z].fd, &rd))
         {
            if((bytesread = read(array[z].fd, &command, 1)) < 1)
            {
               struct _dw_seminfo *newarray;

               /* Remove this connection from the set */
               newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount-1));
               if(!z)
                  memcpy(newarray, &array[1], sizeof(struct _dw_seminfo)*(connectcount-1));
               else
               {
                  memcpy(newarray, array, sizeof(struct _dw_seminfo)*z);
                  if(z!=(connectcount-1))
                     memcpy(&newarray[z], &array[z+1], sizeof(struct _dw_seminfo)*(connectcount-(z+1)));
               }
               connectcount--;

               /* Replace old array with new one */
               free(array);
               array = newarray;
            }
            else if(bytesread == 1)
            {
               switch(command)
               {
               case 0:
                  {
                  /* Reset */
                  posted = 0;
                  }
                  break;
               case 1:
                  /* Post */
                  {
                     int s;
                     char tmp = (char)0;

                     posted = 1;

                     for(s=0;s<connectcount;s++)
                     {
                        /* The semaphore has been posted so
                         * we tell all the waiting threads to
                         * continue.
                         */
                        if(array[s].waiting)
                           result = write(array[s].fd, &tmp, 1);
                     }
                  }
                  break;
               case 2:
                  /* Wait */
                  {
                     char tmp = (char)0;

                     array[z].waiting = 1;

                     /* If we are posted exit immeditately */
                     if(posted)
                        result = write(array[z].fd, &tmp, 1);
                  }
                  break;
               case 3:
                  {
                     /* Done Waiting */
                     array[z].waiting = 0;
                  }
                  break;
               }
            }
         }
      }
   }
}

/* Using domain sockets on unix for IPC */
/* Create a named event semaphore which can be
 * opened from other processes.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_new(const char *name)
{
   struct sockaddr_un un;
   int ev, *tmpsock = (int *)malloc(sizeof(int)*2);
   DWTID dwthread;

   if(!tmpsock)
      return NULL;

   tmpsock[0] = socket(AF_UNIX, SOCK_STREAM, 0);
   ev = socket(AF_UNIX, SOCK_STREAM, 0);
   memset(&un, 0, sizeof(un));
   un.sun_family=AF_UNIX;
   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   strcpy(un.sun_path, "/tmp/.dw/");
   strcat(un.sun_path, name);

   /* just to be safe, this should be changed
    * to support multiple instances.
    */
   remove(un.sun_path);

   bind(tmpsock[0], (struct sockaddr *)&un, sizeof(un));
   listen(tmpsock[0], 0);
   connect(ev, (struct sockaddr *)&un, sizeof(un));
   tmpsock[1] = accept(tmpsock[0], 0, 0);

   if(tmpsock[0] < 0 || tmpsock[1] < 0 || ev < 0)
   {
      if(tmpsock[0] > -1)
         close(tmpsock[0]);
      if(tmpsock[1] > -1)
         close(tmpsock[1]);
      if(ev > -1)
         close(ev);
      free(tmpsock);
      return NULL;
   }

   /* Create a thread to handle this event semaphore */
   pthread_create(&dwthread, NULL, (void *)_dw_handle_sem, (void *)tmpsock);
   return GINT_TO_POINTER(ev);
}

/* Open an already existing named event semaphore.
 * Parameters:
 *         eve: Pointer to an event handle to receive handle.
 *         name: Name given to semaphore which can be opened
 *               by other processes.
 */
HEV API dw_named_event_get(const char *name)
{
   struct sockaddr_un un;
   int ev = socket(AF_UNIX, SOCK_STREAM, 0);
   if(ev < 0)
      return NULL;

   un.sun_family=AF_UNIX;
   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   strcpy(un.sun_path, "/tmp/.dw/");
   strcat(un.sun_path, name);
   connect(ev, (struct sockaddr *)&un, sizeof(un));
   return GINT_TO_POINTER(ev);
}

/* Resets the event semaphore so threads who call wait
 * on this semaphore will block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_reset(HEV eve)
{
   /* signal reset */
   char tmp = (char)0;

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NONE;

   if(write(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
}

/* Sets the posted state of an event semaphore, any threads
 * waiting on the semaphore will no longer block.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_post(HEV eve)
{

   /* signal post */
   char tmp = (char)1;

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NONE;

   if(write(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
      return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
}

/* Waits on the specified semaphore until it becomes
 * posted, or returns immediately if it already is posted.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 *         timeout: Number of milliseconds before timing out
 *                  or -1 if indefinite.
 */
int API dw_named_event_wait(HEV eve, unsigned long timeout)
{
   fd_set rd;
   struct timeval tv, *useme = NULL;
   int retval = 0;
   char tmp;

   if(GPOINTER_TO_INT(eve) < 0)
      return DW_ERROR_NON_INIT;

   /* Set the timout or infinite */
   if(timeout != -1)
   {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;

      useme = &tv;
   }

   FD_ZERO(&rd);
   FD_SET(GPOINTER_TO_INT(eve), &rd);

   /* Signal wait */
   tmp = (char)2;
   retval = write(GPOINTER_TO_INT(eve), &tmp, 1);

   if(retval == 1)
      retval = select(GPOINTER_TO_INT(eve)+1, &rd, NULL, NULL, useme);

   /* Signal done waiting. */
   tmp = (char)3;
   if(retval == 1)
      retval = write(GPOINTER_TO_INT(eve), &tmp, 1);

   if(retval == 0)
      return DW_ERROR_TIMEOUT;
   else if(retval == -1)
      return DW_ERROR_INTERRUPT;

   /* Clear the entry from the pipe so
    * we don't loop endlessly. :)
    */
   if(read(GPOINTER_TO_INT(eve), &tmp, 1) == 1)
	return DW_ERROR_NONE;
   return DW_ERROR_GENERAL;
}

/* Release this semaphore, if there are no more open
 * handles on this semaphore the semaphore will be destroyed.
 * Parameters:
 *         eve: Handle to the semaphore obtained by
 *              an open or create call.
 */
int API dw_named_event_close(HEV eve)
{
   /* Finally close the domain socket,
    * cleanup will continue in _dw_handle_sem.
    */
   close(GPOINTER_TO_INT(eve));
   return DW_ERROR_NONE;
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
   GdkRGBA *foreground = malloc(sizeof(GdkRGBA));
   HEV event = dw_event_new();

   foreground->alpha = foreground->red = foreground->green = foreground->blue = 0.0;
   pthread_setspecific(_dw_fg_color_key, foreground);
   pthread_setspecific(_dw_bg_color_key, NULL);
   pthread_setspecific(_dw_event_key, event);
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
   GdkRGBA *foreground, *background;
   HEV event;

   if((foreground = pthread_getspecific(_dw_fg_color_key)))
      free(foreground);
   if((background = pthread_getspecific(_dw_bg_color_key)))
      free(background);
   if((event = pthread_getspecific(_dw_event_key)))
      dw_event_close(&event);
}

/*
 * Setup thread independent color sets.
 */
void _dwthreadstart(void *data)
{
   void (*threadfunc)(void *) = NULL;
   void **tmp = (void **)data;

   threadfunc = (void (*)(void *))tmp[0];

   /* Initialize colors */
   _dw_init_thread();

   threadfunc(tmp[1]);
   free(tmp);

   /* Free colors */
   _dw_deinit_thread();
}

/*
 * Allocates a shared memory region with a name.
 * Parameters:
 *         handle: A pointer to receive a SHM identifier.
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to allocate.
 *         name: A string pointer to a unique memory name.
 */
HSHM API dw_named_memory_new(void **dest, int size, const char *name)
{
   char namebuf[1025];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
   {
      free(handle);
      return NULL;
   }

   if(ftruncate(handle->fd, size))
   {
       close(handle->fd);
       free(handle);
       return NULL;
   }

   /* attach the shared memory segment to our process's address space. */
   *dest = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

   if(*dest == MAP_FAILED)
   {
      close(handle->fd);
      *dest = NULL;
      free(handle);
      return NULL;
   }

   handle->size = size;
   handle->sid = getsid(0);
   handle->path = strdup(namebuf);

   return handle;
}

/*
 * Aquires shared memory region with a name.
 * Parameters:
 *         dest: A pointer to a pointer to receive the memory address.
 *         size: Size in bytes of the shared memory region to requested.
 *         name: A string pointer to a unique memory name.
 */
HSHM API dw_named_memory_get(void **dest, int size, const char *name)
{
   char namebuf[1025];
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_RDWR)) < 0)
   {
      free(handle);
      return NULL;
   }

   /* attach the shared memory segment to our process's address space. */
   *dest = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle->fd, 0);

   if(*dest == MAP_FAILED)
   {
      close(handle->fd);
      *dest = NULL;
      free(handle);
      return NULL;
   }

   handle->size = size;
   handle->sid = -1;
   handle->path = NULL;

   return handle;
}

/*
 * Frees a shared memory region previously allocated.
 * Parameters:
 *         handle: Handle obtained from DB_named_memory_allocate.
 *         ptr: The memory address aquired with DB_named_memory_allocate.
 */
int API dw_named_memory_free(HSHM handle, void *ptr)
{
   struct _dw_unix_shm *h = handle;
   int rc = munmap(ptr, h->size);

   close(h->fd);
   if(h->path)
   {
      /* Only remove the actual file if we are the
       * creator of the file.
       */
      if(h->sid != -1 && h->sid == getsid(0))
         remove(h->path);
      free(h->path);
   }
   return rc;
}
/*
 * Creates a new thread with a starting point of func.
 * Parameters:
 *       func: Function which will be run in the new thread.
 *       data: Parameter(s) passed to the function.
 *       stack: Stack size of new thread (OS/2 and Windows only).
 */
DWTID API dw_thread_new(void *func, void *data, int stack)
{
   DWTID gtkthread;
   void **tmp = malloc(sizeof(void *) * 2);
   int rc;

   tmp[0] = func;
   tmp[1] = data;

   rc = pthread_create(&gtkthread, NULL, (void *)_dwthreadstart, (void *)tmp);
   if ( rc == 0 )
      return gtkthread;
   return (DWTID)DW_ERROR_UNKNOWN;
}

/*
 * Ends execution of current thread immediately.
 */
void API dw_thread_end(void)
{
   pthread_exit(NULL);
}

/*
 * Returns the current thread's ID.
 */
DWTID API dw_thread_id(void)
{
   return (DWTID)pthread_self();
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
   g_main_loop_unref(_DWMainLoop);
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 * Parameters:
 *       exitcode: Exit code reported to the operating system.
 */
void API dw_exit(int exitcode)
{
   dw_shutdown();
   exit(exitcode);
}

/* Internal function to get the recommended size of scrolled items */
void _dw_get_scrolled_size(GtkWidget *item, gint *thiswidth, gint *thisheight)
{
   GtkWidget *widget = g_object_get_data(G_OBJECT(item), "_dw_user");

   *thisheight = *thiswidth = 0;

   if(widget)
   {
      if(g_object_get_data(G_OBJECT(widget), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_TREE))
      {
         /* Set to half for tree */
         *thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
         *thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
      }
      else if(GTK_IS_TEXT_VIEW(widget))
      {
         unsigned long bytes;
         int height, width;
         char *buf, *ptr;
         int wrap = (gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(widget)) == GTK_WRAP_WORD);
         static char testtext[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
         int hscrolled = FALSE;

         *thiswidth = *thisheight = 0;

         dw_mle_get_size(item, &bytes, NULL);

         ptr = buf = alloca(bytes + 2);
         dw_mle_export(item, buf, 0, (int)bytes);
         buf[bytes] = 0;
         strcat(buf, "\r");

         /* MLE */
         while((ptr = strstr(buf, "\r")))
         {
            ptr[0] = 0;
            width = 0;
            if(strlen(buf))
               dw_font_text_extents_get(item, NULL, buf, &width, &height);
            else
               dw_font_text_extents_get(item, NULL, testtext, NULL, &height);

            if(wrap && width > _DW_SCROLLED_MAX_WIDTH)
            {
               *thiswidth = _DW_SCROLLED_MAX_WIDTH;
               *thisheight += height * (width / _DW_SCROLLED_MAX_WIDTH);
            }
            else
            {
               if(width > *thiswidth)
               {
                  if(width > _DW_SCROLLED_MAX_WIDTH)
                  {
                     *thiswidth = _DW_SCROLLED_MAX_WIDTH;
                     hscrolled = TRUE;
                  }
                  else
                     *thiswidth = width;
               }
            }
            *thisheight += height;
            if(ptr[1] == '\n')
               buf = &ptr[2];
            else
               buf = &ptr[1];
         }
         if(hscrolled)
            *thisheight += 10;
      }
      else
      {
         gtk_widget_measure(GTK_WIDGET(widget), GTK_ORIENTATION_HORIZONTAL, -1, thiswidth, NULL, NULL, NULL);
         gtk_widget_measure(GTK_WIDGET(widget), GTK_ORIENTATION_VERTICAL, -1, thisheight, NULL, NULL, NULL);

         *thisheight += 20;
         *thiswidth += 20;
      }
   }
   if(*thiswidth < _DW_SCROLLED_MIN_WIDTH)
      *thiswidth = _DW_SCROLLED_MIN_WIDTH;
   if(*thiswidth > _DW_SCROLLED_MAX_WIDTH)
      *thiswidth = _DW_SCROLLED_MAX_WIDTH;
   if(*thisheight < _DW_SCROLLED_MIN_HEIGHT)
      *thisheight = _DW_SCROLLED_MIN_HEIGHT;
   if(*thisheight > _DW_SCROLLED_MAX_HEIGHT)
      *thisheight = _DW_SCROLLED_MAX_HEIGHT;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
   GtkWidget *tmp, *tmpitem;

   if(!box)
      return;

      /*
       * If you try and pack an item into itself VERY bad things can happen; like at least an
       * infinite loop on GTK! Lets be safe!
       */
   if(box == item)
   {
      dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
      return;
   }

   /* If this is a special box, like: Window, Groupbox, Scrollbox...
    * get the internal box handle.
    */
   if((tmp  = g_object_get_data(G_OBJECT(box), "_dw_boxhandle")))
      box = tmp;

   /* Can't pack nothing with GTK, so create an empty label */
   if(!item)
   {
      item = gtk_label_new("");
      g_object_set_data(G_OBJECT(item), "_dw_padding", GINT_TO_POINTER(1));
      gtk_widget_set_visible(item, TRUE);
   }

   /* Check if the item to be packed is a special box */
   tmpitem = (GtkWidget *)g_object_get_data(G_OBJECT(item), "_dw_boxhandle");

   /* Make sure our target box is valid */
   if(GTK_IS_GRID(box))
   {
      int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
      int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));

      /* If the item being packed is a box, then we use it's padding
       * instead of the padding specified on the pack line, this is
       * due to a bug in the OS/2 and Win32 renderer and a limitation
       * of the GtkTable class.
       */
      if(GTK_IS_GRID(item) || (tmpitem && GTK_IS_GRID(tmpitem)))
      {
         /* NOTE: I left in the ability to pack boxes with a size,
          *       this eliminates that by forcing the size to 0.
          */
         height = width = 0;
      }

      /* Do some sanity bounds checking */
      if(index < 0)
         index = 0;
      if(index > boxcount)
         index = boxcount;

      g_object_set_data(G_OBJECT(item), "_dw_table", box);
      /* Set the expand attribute on the widgets now instead of the container */
      gtk_widget_set_vexpand(item, vsize);
      gtk_widget_set_hexpand(item, hsize);
      /* Don't clobber the center alignment on pictures */
      if(!GTK_IS_PICTURE(item))
      {
         gtk_widget_set_valign(item, vsize ? GTK_ALIGN_FILL : GTK_ALIGN_START);
         gtk_widget_set_halign(item, hsize ? GTK_ALIGN_FILL : GTK_ALIGN_START);
      }
      /* Set pad for each margin direction on the widget */
      _dw_widget_set_pad(item, pad);
      /* Add to the grid using insert...
       * rows for vertical boxes and columns for horizontal.
       */
      if(boxtype == DW_VERT)
      {
         gtk_grid_insert_row(GTK_GRID(box), index);
         gtk_grid_attach(GTK_GRID(box), item, 0, index, 1, 1);
      }
      else
      {
         gtk_grid_insert_column(GTK_GRID(box), index);
         gtk_grid_attach(GTK_GRID(box), item, index, 0, 1, 1);
      }
      g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount + 1));
      /* Special case for scrolled windows */
      if(GTK_IS_SCROLLED_WINDOW(item))
      {
         gint scrolledwidth = 0, scrolledheight = 0;

         /* Pre-run the calculation code for MLE/Container/Tree if needed */
         if((width < 1 && !hsize) || (height < 1 && !vsize))
            _dw_get_scrolled_size(item, &scrolledwidth, &scrolledheight);

         if(width > 0)
            gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(item), width);
         else if(!hsize)
            gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(item), scrolledwidth);
         if(height > 0)
            gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(item), height);
         else if(!vsize)
            gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(item), scrolledheight);
      }
      else
      {
         /* Set the requested size of the widget */
         if(width == -1 && (GTK_IS_COMBO_BOX(item) || GTK_IS_ENTRY(item)))
            gtk_widget_set_size_request(item, 150, height);
         else if(width == -1 && GTK_IS_SPIN_BUTTON(item))
            gtk_widget_set_size_request(item, 50, height);
         else      	
            gtk_widget_set_size_request(item, width, height);
      }
      if(GTK_IS_TOGGLE_BUTTON(item))
      {
         GtkToggleButton *groupstart = (GtkToggleButton *)g_object_get_data(G_OBJECT(box), "_dw_group");

         if(groupstart)
            gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(item), groupstart);
         else
            g_object_set_data(G_OBJECT(box), "_dw_group", (gpointer)item);
      }
      /* If we previously incremented the reference count... drop it now that it is packed */
      if(g_object_get_data(G_OBJECT(item), "_dw_refed"))
      {
         g_object_unref(G_OBJECT(item));
         g_object_set_data(G_OBJECT(item), "_dw_refed", NULL);
      }
   }
}

/*
 * Remove windows (widgets) from the box they are packed into.
 * Parameters:
 *       handle: Window handle of the packed item to be removed.
 * Returns:
 *       DW_ERROR_NONE on success and DW_ERROR_GENERAL on failure.
 */
DW_FUNCTION_DEFINITION(dw_box_unpack, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_box_unpack, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   int retcode = DW_ERROR_GENERAL;

   if(GTK_IS_WIDGET(handle))
   {
      GtkWidget *box, *handle2 = handle;

      /* Check if we are removing a widget from a box */	
      if((box = gtk_widget_get_parent(handle2)) && GTK_IS_GRID(box))
      {
         /* Get the number of items in the box... */
         int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
         int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
			
         if(boxcount > 0)
         {
            /* Decrease the count by 1 */
            boxcount--;
            g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
         }
         /* If we haven't incremented the reference count... raise it before removal */
         if(!g_object_get_data(G_OBJECT(handle2), "_dw_refed"))
         {
            g_object_ref(G_OBJECT(handle2));
            g_object_set_data(G_OBJECT(handle2), "_dw_refed", GINT_TO_POINTER(1));
         }
         /* Remove the widget from the box */
         /* Figure out where in the grid this widget is and remove that row/column */
         if(boxtype == DW_VERT)
         {
            int z;

            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), 0, z) == handle2)
               {
                  gtk_grid_remove_row(GTK_GRID(box), z);
                  break;
               }
            }
         }
         else
         {
            int z;

            for(z=0;z<boxcount;z++)
            {
               if(gtk_grid_get_child_at(GTK_GRID(box), z, 0) == handle2)
               {
                  gtk_grid_remove_column(GTK_GRID(box), z);
                  break;
               }
            }
         }
         retcode = DW_ERROR_NONE;
      }
   }
    DW_FUNCTION_RETURN_THIS(retcode);
}

/*
 * Remove windows (widgets) from a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be removed from.
 *       index: 0 based index of packed items.
 * Returns:
 *       Handle to the removed item on success, 0 on failure or padding.
 */
DW_FUNCTION_DEFINITION(dw_box_unpack_at_index, HWND, HWND box, int index)
DW_FUNCTION_ADD_PARAM2(box, index)
DW_FUNCTION_RETURN(dw_box_unpack_at_index, HWND)
DW_FUNCTION_RESTORE_PARAM2(box, HWND, index, int)
{
   HWND retval = 0;

   /* Check if we are removing a widget from a box */	
   if(GTK_IS_GRID(box))
   {
      /* Get the number of items in the box... */
      int boxcount = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxcount"));
      int boxtype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "_dw_boxtype"));
      GtkWidget *item = (boxtype == DW_VERT) ? gtk_grid_get_child_at(GTK_GRID(box), 0, index) : gtk_grid_get_child_at(GTK_GRID(box), index, 0);

      if(boxcount > 0)
      {
         /* Decrease the count by 1 */
         boxcount--;
         g_object_set_data(G_OBJECT(box), "_dw_boxcount", GINT_TO_POINTER(boxcount));
      }
      /* If we haven't incremented the reference count... raise it before removal */
      if(item && g_object_get_data(G_OBJECT(item), "_dw_padding") && GTK_IS_WIDGET(item))
         g_object_unref(G_OBJECT(item));
      else if(item)
      {
         if(!g_object_get_data(G_OBJECT(item), "_dw_refed"))
         {
            g_object_ref(G_OBJECT(item));
            g_object_set_data(G_OBJECT(item), "_dw_refed", GINT_TO_POINTER(1));
         }
         /* Remove the widget from the box */
         gtk_grid_remove(GTK_GRID(box), item);
         if(boxtype == DW_VERT)
            gtk_grid_remove_row(GTK_GRID(box), index);
         else
            gtk_grid_remove_column(GTK_GRID(box), index);
         retval = item;
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Pack windows (widgets) into a box at an arbitrary location.
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       index: 0 based index of packed items.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
DW_FUNCTION_DEFINITION(dw_box_pack_at_index, void, HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM8(box, item, index, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_at_index)
DW_FUNCTION_RESTORE_PARAM8(box, HWND, item, HWND, index, int, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    _dw_box_pack(box, item, index, width, height, hsize, vsize, pad, "dw_box_pack_at_index()");
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Pack windows (widgets) into a box from the start (or top).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
DW_FUNCTION_DEFINITION(dw_box_pack_start, void, HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM7(box, item, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_start)
DW_FUNCTION_RESTORE_PARAM7(box, HWND, item, HWND, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    /* 65536 is the table limit on GTK...
     * seems like a high enough value we will never hit it here either.
     */
    _dw_box_pack(box, item, 65536, width, height, hsize, vsize, pad, "dw_box_pack_start()");
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Pack windows (widgets) into a box from the end (or bottom).
 * Parameters:
 *       box: Window handle of the box to be packed into.
 *       item: Window handle of the item to pack.
 *       width: Width in pixels of the item or -1 to be self determined.
 *       height: Height in pixels of the item or -1 to be self determined.
 *       hsize: TRUE if the window (widget) should expand horizontally to fill space given.
 *       vsize: TRUE if the window (widget) should expand vertically to fill space given.
 *       pad: Number of pixels of padding around the item.
 */
DW_FUNCTION_DEFINITION(dw_box_pack_end, void, HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
DW_FUNCTION_ADD_PARAM7(box, item, width, height, hsize, vsize, pad)
DW_FUNCTION_NO_RETURN(dw_box_pack_end)
DW_FUNCTION_RESTORE_PARAM7(box, HWND, item, HWND, width, int, height, int, hsize, int, vsize, int, pad, int)
{
    _dw_box_pack(box, item, 0, width, height, hsize, vsize, pad, "dw_box_pack_end()");
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
DW_FUNCTION_DEFINITION(dw_window_set_size, void, HWND handle, ULONG width, ULONG height)
DW_FUNCTION_ADD_PARAM3(handle, width, height)
DW_FUNCTION_NO_RETURN(dw_window_set_size)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, width, ULONG, height, ULONG)
{
   if(handle)
   {
      if(GTK_IS_WINDOW(handle))
         gtk_window_set_default_size(GTK_WINDOW(handle), width, height);
      else if(GTK_IS_WIDGET(handle))
         gtk_widget_set_size_request(GTK_WIDGET(handle), width, height);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the size the system thinks the widget should be.
 * Parameters:
 *       handle: Window handle of the item to be back.
 *       width: Width in pixels of the item or NULL if not needed.
 *       height: Height in pixels of the item or NULL if not needed.
 */
DW_FUNCTION_DEFINITION(dw_window_get_preferred_size, void, HWND handle, int *width, int *height)
DW_FUNCTION_ADD_PARAM3(handle, width, height)
DW_FUNCTION_NO_RETURN(dw_window_get_preferred_size)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, width, int *, height, int *)
{
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      gint scrolledwidth, scrolledheight;

      _dw_get_scrolled_size(handle, &scrolledwidth, &scrolledheight);

      if(width)
         *width = scrolledwidth;
      if(height)
         *height = scrolledheight;
   }
   else
   {
     if(width)
         gtk_widget_measure(GTK_WIDGET(handle), GTK_ORIENTATION_HORIZONTAL, -1, width, NULL, NULL, NULL);
      if(height)
         gtk_widget_measure(GTK_WIDGET(handle), GTK_ORIENTATION_VERTICAL, -1, height, NULL, NULL, NULL);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Internal version to simplify the code with multiple versions of GTK */
int _dw_get_screen_width(void)
{
   GdkDisplay *display = gdk_display_get_default();

   if(display)
   {
      GListModel *monitors = gdk_display_get_monitors(display);
      GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_object(monitors, 0));

      if(monitor)
      {
         GdkRectangle rc = { 0, 0, 0 ,0 };
         gdk_monitor_get_geometry(monitor, &rc);
         return rc.width;
      }
   }
   return 0;
}

/*
 * Returns the width of the screen.
 */
DW_FUNCTION_DEFINITION(dw_screen_width, int)
DW_FUNCTION_ADD_PARAM
DW_FUNCTION_RETURN(dw_screen_width, int)
{
   int retval = _dw_get_screen_width();
   DW_FUNCTION_RETURN_THIS(retval);
}

/* Internal version to simplify the code with multiple versions of GTK */
int _dw_get_screen_height(void)
{
   GdkDisplay *display = gdk_display_get_default();

   if(display)
   {
      GListModel *monitors = gdk_display_get_monitors(display);
      GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_object(monitors, 0));

      if(monitor)
      {
         GdkRectangle rc = { 0, 0, 0 ,0 };
         gdk_monitor_get_geometry(monitor, &rc);
         return rc.height;
      }
   }
   return 0;
}

/*
 * Returns the height of the screen.
 */
DW_FUNCTION_DEFINITION(dw_screen_height, int)
DW_FUNCTION_ADD_PARAM
DW_FUNCTION_RETURN(dw_screen_height, int)
{
   int retval = _dw_get_screen_height();
   DW_FUNCTION_RETURN_THIS(retval);
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
   /* TODO: Make this work on GTK4... with no GdkVisual */
   return 32;
}

/*
 * Sets the gravity of a given window (widget).
 * Gravity controls which corner of the screen and window the position is relative to.
 * Parameters:
 *          handle: Window (widget) handle.
 *          horz: DW_GRAV_LEFT (default), DW_GRAV_RIGHT or DW_GRAV_CENTER.
 *          vert: DW_GRAV_TOP (default), DW_GRAV_BOTTOM or DW_GRAV_CENTER.
 */
void API dw_window_set_gravity(HWND handle, int horz, int vert)
{
   dw_window_set_data(handle, "_dw_grav_horz", DW_INT_TO_POINTER(horz));
   dw_window_set_data(handle, "_dw_grav_vert", DW_INT_TO_POINTER(vert));
}

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
#ifndef GDK_WINDOWING_X11
void API dw_window_set_pos(HWND handle, long x, long y)
{
}
#else
DW_FUNCTION_DEFINITION(dw_window_set_pos, void, HWND handle, long x, long y)
DW_FUNCTION_ADD_PARAM3(handle, x, y)
DW_FUNCTION_NO_RETURN(dw_window_set_pos)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, x, long, y, long)
{
   GdkDisplay *display = gdk_display_get_default();
   
   if(handle && GTK_IS_WINDOW(handle) && display && GDK_IS_X11_DISPLAY(display))
   {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(handle));
      
      if(surface)
      {
         XMoveWindow(GDK_SURFACE_XDISPLAY(surface),
                     GDK_SURFACE_XID(surface),
                     x, y);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}
#endif

/*
 * Sets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
void API dw_window_set_pos_size(HWND handle, long x, long y, unsigned long width, unsigned long height)
{
   dw_window_set_size(handle, width, height);
   dw_window_set_pos(handle, x, y);
}

/*
 * Gets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
DW_FUNCTION_DEFINITION(dw_window_get_pos_size, void, HWND handle, long *x, long *y, ULONG *width, ULONG *height)
DW_FUNCTION_ADD_PARAM5(handle, x, y, width, height)
DW_FUNCTION_NO_RETURN(dw_window_get_pos_size)
DW_FUNCTION_RESTORE_PARAM5(handle, HWND, x, long *, y, long *, width, ULONG *, height, ULONG *)
{
   if(handle && GTK_IS_WIDGET(handle))
   {
      GtkRequisition size;

      /* If the widget hasn't been shown, it returns 0 so use this as backup */
      gtk_widget_get_preferred_size(GTK_WIDGET(handle), NULL, &size);

      if(width)
      {
         *width = (ULONG)gtk_widget_get_width(GTK_WIDGET(handle));
         if(!*width)
            *width = (ULONG)size.width;
      }
      if(height)
      {
         *height = (ULONG)gtk_widget_get_height(GTK_WIDGET(handle));
         if(!*height)
            *height = (ULONG)size.height;
      }

#ifdef GDK_WINDOWING_X11
      if(x || y)
      {
         GdkDisplay *display = gdk_display_get_default();
         GdkSurface *surface;
   
         if(display && GDK_IS_X11_DISPLAY(display) && (surface = gtk_native_get_surface(GTK_NATIVE(handle))))
         {
            XWindowAttributes xwa;
            int ix = 0, iy = 0;
            Window child, xrootwin = gdk_x11_display_get_xrootwindow(display);

            XTranslateCoordinates(GDK_SURFACE_XDISPLAY(surface), GDK_SURFACE_XID(surface), 
                                  xrootwin, 0, 0, &ix, &iy, &child);
            XGetWindowAttributes(GDK_SURFACE_XDISPLAY(surface), 
                                 GDK_SURFACE_XID(surface), &xwa);

            if(x)
               *x = (long)(ix - xwa.x);
            if(y)
               *y = (long)(ix - xwa.y);
          }
      }
#else                                 
      if(x)
         *x = 0;
      if(y)
         *y = 0;
#endif
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
DW_FUNCTION_DEFINITION(dw_window_set_style, void, HWND handle, ULONG style, ULONG mask)
DW_FUNCTION_ADD_PARAM3(handle, style, mask)
DW_FUNCTION_NO_RETURN(dw_window_set_style)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, style, ULONG, mask, ULONG)
{
   GtkWidget *handle2 = handle;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   else if(GTK_IS_FRAME(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_label");
      if(tmp && GTK_IS_LABEL(tmp))
         handle2 = tmp;
   }
   else if(GTK_IS_BUTTON(handle))
   {
        if(mask & DW_BS_NOBORDER)
        {
            GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(handle));
  
            if(style & DW_BS_NOBORDER)
               gtk_style_context_add_class(context, "flat");
            else
               gtk_style_context_remove_class(context, "flat");
        }
   }
   if(GTK_IS_LABEL(handle2))
   {
      gfloat x=DW_LEFT, y=DW_CENTER;
      /* horizontal... */
      if(style & DW_DT_CENTER)
         x = DW_CENTER;
      if(style & DW_DT_RIGHT)
         x = DW_RIGHT;
      if(style & DW_DT_LEFT)
         x = DW_LEFT;
      /* vertical... */
      if(style & DW_DT_VCENTER)
         y = DW_CENTER;
      if(style & DW_DT_TOP)
         y = DW_TOP;
      if(style & DW_DT_BOTTOM)
         y = DW_BOTTOM;
      gtk_label_set_xalign(GTK_LABEL(handle2), x);
      gtk_label_set_yalign(GTK_LABEL(handle2), y);
      if(style & DW_DT_WORDBREAK)
         gtk_label_set_wrap(GTK_LABEL(handle), TRUE);
   }
   if(G_IS_MENU_ITEM(handle2))
   {
      GSimpleAction *action = g_object_get_data(G_OBJECT(handle2), "_dw_action");
      
      if(action)
      {
         if(mask & (DW_MIS_ENABLED | DW_MIS_DISABLED))
         {
            if((style & DW_MIS_ENABLED) || (style & DW_MIS_DISABLED))
            {
               if(style & DW_MIS_ENABLED)
                  g_simple_action_set_enabled(action, TRUE);
               else
                  g_simple_action_set_enabled(action, FALSE);
            }
         }
         if(mask & (DW_MIS_CHECKED | DW_MIS_UNCHECKED))
         {
            GVariant *action_state = g_action_get_state(G_ACTION(action));
            gboolean check = false;

            if(style & DW_MIS_CHECKED)
               check = true;

            if(!action_state || (g_variant_get_boolean(action_state) != check))
            {
               GVariant *new_state = g_variant_new_boolean(check);
               g_simple_action_set_state(action, new_state);
            }
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_new, ULONG, HWND handle, DW_UNUSED(ULONG flags), int front)
DW_FUNCTION_ADD_PARAM3(handle, flags, front)
DW_FUNCTION_RETURN(dw_notebook_page_new, ULONG)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, DW_UNUSED(flags), ULONG, front, int)
{
   GtkWidget **pagearray;
   ULONG retval = 256;
   int z;

   pagearray = (GtkWidget **)g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(pagearray)
   {
      for(z=0;z<256;z++)
      {
         if(!pagearray[z])
         {
            char text[101] = {0};
            int num = z;

            if(front)
               num |= 1 << 16;

            snprintf(text, 100, "_dw_page%d", z);
            /* Save the real id and the creation flags */
            g_object_set_data(G_OBJECT(handle), text, GINT_TO_POINTER(num));
            retval = (ULONG)z;
            z = 256;
         }
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/* Return the physical page id from the logical page id */
int _dw_get_physical_page(HWND handle, unsigned long pageid)
{
   int z;
   GtkWidget *thispage, **pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(pagearray)
   {
      for(z=0;z<256;z++)
      {
         if((thispage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), z)))
         {
            if(thispage == pagearray[pageid])
               return z;
         }
      }
   }
   return 256;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_destroy, void, HWND handle, unsigned long pageid)
DW_FUNCTION_ADD_PARAM2(handle, pageid)
DW_FUNCTION_NO_RETURN(dw_notebook_page_destroy)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, pageid, unsigned long)
{
   GtkWidget **pagearray;
   int realpage = _dw_get_physical_page(handle, pageid);

   if(realpage > -1 && realpage < 256)
   {
      gtk_notebook_remove_page(GTK_NOTEBOOK(handle), realpage);
      if((pagearray = g_object_get_data(G_OBJECT(handle), "_dw_pagearray")))
         pagearray[pageid] = NULL;
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_get, ULONG, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_notebook_page_get, ULONG)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   ULONG retval;
   int phys = gtk_notebook_get_current_page(GTK_NOTEBOOK(handle));

   retval = _dw_get_logical_page(handle, phys);
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_set, void, HWND handle, unsigned long pageid)
DW_FUNCTION_ADD_PARAM2(handle, pageid)
DW_FUNCTION_NO_RETURN(dw_notebook_page_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, pageid, unsigned long)
{
   int realpage = _dw_get_physical_page(handle, pageid);

   if(realpage > -1 && realpage < 256)
      gtk_notebook_set_current_page(GTK_NOTEBOOK(handle), pageid);
   DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
DW_FUNCTION_DEFINITION(dw_notebook_page_set_text, void, HWND handle, ULONG pageid, const char *text)
DW_FUNCTION_ADD_PARAM3(handle, pageid, text)
DW_FUNCTION_NO_RETURN(dw_notebook_page_set_text)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, pageid, ULONG, text, const char *)
{
   GtkWidget *child;
   int realpage = _dw_get_physical_page(handle, pageid);

   if(realpage < 0 || realpage > 255)
   {
      char ptext[101] = {0};
      int num;

      snprintf(ptext, 100, "_dw_page%d", (int)pageid);
      num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), ptext));
      realpage = 0xFF & num;
   }

   if(realpage > -1 && realpage < 256)
   {
      child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), realpage);
      if(child)
         gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(handle), child, text);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_status_text(HWND handle, unsigned long pageid, const char *text)
{
   /* TODO (if possible) */
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
DW_FUNCTION_DEFINITION(dw_notebook_pack, void, HWND handle, ULONG pageid, HWND page)
DW_FUNCTION_ADD_PARAM3(handle, pageid, page)
DW_FUNCTION_NO_RETURN(dw_notebook_pack)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, pageid, ULONG, page, HWND)
{
   GtkWidget *label, *child, *oldlabel, **pagearray;
   const gchar *text = NULL;
   int num, z, realpage = -1;
   char ptext[101] = {0};

   snprintf(ptext, 100, "_dw_page%d", (int)pageid);
   num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(handle), ptext));
   g_object_set_data(G_OBJECT(handle), ptext, NULL);
   pagearray = (GtkWidget **)g_object_get_data(G_OBJECT(handle), "_dw_pagearray");

   if(pagearray)
   {
      /* The page already exists... so get it's current page */
      if(pagearray[pageid])
      {
         for(z=0;z<256;z++)
         {
            child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(handle), z);
            if(child == pagearray[pageid])
            {
               oldlabel = gtk_notebook_get_tab_label(GTK_NOTEBOOK(handle), child);
               if(oldlabel)
                  text = gtk_label_get_text(GTK_LABEL(oldlabel));
               gtk_notebook_remove_page(GTK_NOTEBOOK(handle), z);
               realpage = z;
               break;
            }
         }
      }

      pagearray[pageid] = page;

      label = gtk_label_new(text ? text : "");

      if(realpage != -1)
         gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, realpage);
      else if(num & ~(0xFF))
         gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, 0);
      else
         gtk_notebook_insert_page(GTK_NOTEBOOK(handle), page, label, 256);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
void API dw_listbox_append(HWND handle, const char *text)
{
   dw_listbox_insert(handle, text, -1);
}

/*
 * Inserts the specified text int the listbox's (or combobox) entry list at the
 * position indicated.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to insert into listbox.
 *          pos: 0-based index into listbox. -1 will append
 */
DW_FUNCTION_DEFINITION(dw_listbox_insert, void, HWND handle, const char *text, int pos)
DW_FUNCTION_ADD_PARAM3(handle, text, pos)
DW_FUNCTION_NO_RETURN(dw_listbox_insert)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, text, const char *, pos, int)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      GtkTreeIter iter;

      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         if(pos < 0)
         {
            /* Insert an entry at the end */
            gtk_list_store_append(store, &iter);
         }
         else
         {
            /* Insert at position */
            gtk_list_store_insert(store, &iter, pos);
         }
         gtk_list_store_set (store, &iter, 0, text, -1);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Appends the specified text items to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text strings to append into listbox.
 *          count: Number of text strings to append
 */
DW_FUNCTION_DEFINITION(dw_listbox_list_append, void, HWND handle, char **text, int count)
DW_FUNCTION_ADD_PARAM3(handle, text, count)
DW_FUNCTION_NO_RETURN(dw_listbox_list_append)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, text, char **, count, int)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      int z;
      GtkTreeIter iter;

      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         /* Insert entries at the end */
         for(z=0;z<count;z++)
         {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set (store, &iter, 0, text[z], -1);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Clears the listbox's (or combobox) list of all entries.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_listbox_clear, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_listbox_clear)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)

{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         /* Clear the list */
         gtk_list_store_clear(store);
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be counted
 */
DW_FUNCTION_DEFINITION(dw_listbox_count, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_listbox_count, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   int retval = 0;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         /* Get the number of children at the top level */
         retval = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the topmost item in the viewport.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 *          top: Index to the top item.
 */
DW_FUNCTION_DEFINITION(dw_listbox_set_top, void, HWND handle, int top)
DW_FUNCTION_ADD_PARAM2(handle, top)
DW_FUNCTION_NO_RETURN(dw_listbox_set_top)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, top, int)

{
   GtkWidget *handle2 = handle;

   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   /* Make sure it is the correct tree type */
   if(handle2 && GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
   {
      GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(handle));
      GtkListStore *store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));

      if(store && adjust)
      {
         /* Get the number of children at the top level */
         gint rowcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
         gdouble pagesize = gtk_adjustment_get_page_size(adjust);
         gdouble upper = gtk_adjustment_get_upper(adjust) - pagesize;
         gdouble lower = gtk_adjustment_get_lower(adjust);
         gdouble change;

         /* Safety check */
         if(rowcount > 1)
         {
            /* Verify the range */
            rowcount--;
            if(top > rowcount)
               top = rowcount;

            change = ((gdouble)top/(gdouble)rowcount) * (upper - lower);

            gtk_adjustment_set_value(adjust, change + lower);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Copies the given index item's text into buffer.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 *          length: Length of the buffer (including NULL).
 */
DW_FUNCTION_DEFINITION(dw_listbox_get_text, void, HWND handle, unsigned int index, char *buffer, unsigned int length)
DW_FUNCTION_ADD_PARAM4(handle, index, buffer, length)
DW_FUNCTION_NO_RETURN(dw_listbox_get_text)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, index, unsigned int, buffer, char *, length, unsigned int)

{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            /* Get the text */
            gchar *text;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, _DW_DATA_TYPE_STRING, &text, -1);
            if(text)
            {
               strncpy(buffer, text, length);
               g_free(text);
            }
         }
      }
   }
   buffer[0] = '\0';
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the text of a given listbox entry.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          index: Index into the list to be queried.
 *          buffer: Buffer where text will be copied.
 */
DW_FUNCTION_DEFINITION(dw_listbox_set_text, void, HWND handle, unsigned int index, const char *buffer)
DW_FUNCTION_ADD_PARAM3(handle, index, buffer)
DW_FUNCTION_NO_RETURN(dw_listbox_set_text)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, index, unsigned int, buffer, char *)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            /* Update the text */
            gtk_list_store_set(store, &iter, 0, buffer, -1);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the index to the current selected item or -1 when done.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 *          where: Either the previous return or -1 to restart.
 */
DW_FUNCTION_DEFINITION(dw_listbox_selected_multi, int, HWND handle, int where)
DW_FUNCTION_ADD_PARAM2(handle, where)
DW_FUNCTION_RETURN(dw_listbox_selected_multi, int)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, where, int)
{
   GtkWidget *handle2;
   GtkListStore *store = NULL;
   int retval = DW_LIT_NONE;

   handle2 = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");

   /* Make sure it is the correct tree type */
   if(handle2 && GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));

   if(store)
   {
      GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
      GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);

      if(list)
      {
         int counter = 0;
         GtkTreePath *path = g_list_nth_data(list, 0);

         while(path)
         {
            gint *indices = gtk_tree_path_get_indices(path);

            if(indices && indices[0] > where)
            {
               retval = indices[0];
               break;
            }

            counter++;
            path = g_list_nth_data(list, counter);
         }

         g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
         g_list_free(list);
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Returns the index to the item in the list currently selected.
 * Parameters:
 *          handle: Handle to the listbox to be queried.
 */
DW_FUNCTION_DEFINITION(dw_listbox_selected, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_listbox_selected, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;
   unsigned int retval = 0;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         if(GTK_IS_TREE_VIEW(handle2))
         {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
            GList *list = gtk_tree_selection_get_selected_rows(sel, NULL);
            if(list)
            {
               GtkTreePath *path = g_list_nth_data(list, 0);
               gint *indices = gtk_tree_path_get_indices(path);

               if(indices)
               {
                  retval = indices[0];
               }

               g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
               g_list_free(list);
            }
         }
         else
         {
            GtkTreeIter iter;
            GtkTreePath *path;

            if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(handle2), &iter))
            {
               path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
               if(path)
               {
                  gint *indices = gtk_tree_path_get_indices(path);

                  if(indices)
                  {
                     retval = indices[0];
                  }
                  gtk_tree_path_free(path);
               }
            }
         }
      }
   }
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the selection state of a given index.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 *          state: TRUE if selected FALSE if unselected.
 */
DW_FUNCTION_DEFINITION(dw_listbox_select, void, HWND handle, int index, int state)
DW_FUNCTION_ADD_PARAM3(handle, index, state)
DW_FUNCTION_NO_RETURN(dw_listbox_select)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, index, int, state, int)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store && index < gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL))
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            if(GTK_IS_COMBO_BOX(handle2))
            {
               gtk_combo_box_set_active_iter(GTK_COMBO_BOX(handle2), &iter);
            }
            else
            {
               GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(handle2));
               if(state)
               {
                  /* Select the item */
                  gtk_tree_selection_select_iter(sel, &iter);
               }
               else
               {
                  /* Deselect the item */
                  gtk_tree_selection_unselect_iter(sel, &iter);
               }
            }
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with given index from the list.
 * Parameters:
 *          handle: Handle to the listbox to be set.
 *          index: Item index.
 */
DW_FUNCTION_DEFINITION(dw_listbox_delete, void, HWND handle, int index)
DW_FUNCTION_ADD_PARAM2(handle, index)
DW_FUNCTION_NO_RETURN(dw_listbox_delete)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, index, int)
{
   GtkWidget *handle2 = handle;
   GtkListStore *store = NULL;

   /* Get the inner handle for scrolled controls */
   if(GTK_IS_SCROLLED_WINDOW(handle))
   {
      GtkWidget *tmp = (GtkWidget *)g_object_get_data(G_OBJECT(handle), "_dw_user");
      if(tmp)
         handle2 = tmp;
   }
   if(handle2)
   {
      /* Make sure it is the correct tree type */
      if(GTK_IS_TREE_VIEW(handle2) && g_object_get_data(G_OBJECT(handle2), "_dw_tree_type") == GINT_TO_POINTER(_DW_TREE_TYPE_LISTBOX))
         store = (GtkListStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(handle2));
      else if(GTK_IS_COMBO_BOX(handle2))
         store = (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(handle2));

      if(store)
      {
         GtkTreeIter iter;

         /* Get the nth child at the top level */
         if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, index))
         {
            gtk_list_store_remove(store, &iter);
         }
      }
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/* Function to do delayed positioning */
gboolean _dw_splitbar_set_percent(gpointer data)
{
   GtkWidget *widget = data;
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   if(percent)
   {
     int width, height;
#if GTK_CHECK_VERSION(4,12,0)
     width = gtk_widget_get_width(widget);
     height = gtk_widget_get_height(widget);
#else
      GtkAllocation alloc;

      gtk_widget_get_allocation(widget, &alloc);
      width = alloc.width;
      height = alloc.height;
#endif

      if(width > 10 && height > 10)
      {
         if(gtk_orientable_get_orientation(GTK_ORIENTABLE(widget)) == GTK_ORIENTATION_HORIZONTAL)
            gtk_paned_set_position(GTK_PANED(widget), (int)(width * (*percent / 100.0)));
         else
            gtk_paned_set_position(GTK_PANED(widget), (int)(height * (*percent / 100.0)));
         g_object_set_data(G_OBJECT(widget), "_dw_percent", NULL);
         free(percent);
      }
      else
         return TRUE;
   }
   return FALSE;
}

/* Reposition the bar according to the percentage */
static gint _dw_splitbar_realize(GtkWidget *widget, gpointer data)
{
   float *percent = (float *)g_object_get_data(G_OBJECT(widget), "_dw_percent");

   /* Prevent infinite recursion ;) */
   if(!percent)
      return FALSE;

   g_idle_add(_dw_splitbar_set_percent, widget);
   return FALSE;
}

/*
 * Creates a splitbar window (widget) with given parameters.
 * Parameters:
 *       type: Value can be DW_VERT or DW_HORZ.
 *       topleft: Handle to the window to be top or left.
 *       bottomright:  Handle to the window to be bottom or right.
 * Returns:
 *       A handle to a splitbar window or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_splitbar_new, HWND, int type, HWND topleft, HWND bottomright, unsigned long cid)
DW_FUNCTION_ADD_PARAM4(type, topleft, bottomright, cid)
DW_FUNCTION_RETURN(dw_splitbar_new, HWND)
DW_FUNCTION_RESTORE_PARAM4(type, int, topleft, HWND, bottomright, HWND, cid, unsigned long)
{
   GtkWidget *tmp = NULL;
   float *percent = malloc(sizeof(float));

   tmp = gtk_paned_new(type == DW_HORZ ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
   gtk_paned_set_start_child(GTK_PANED(tmp), topleft);
   gtk_paned_set_resize_start_child(GTK_PANED(tmp), TRUE);
   gtk_paned_set_shrink_start_child(GTK_PANED(tmp), FALSE);
   gtk_paned_set_end_child(GTK_PANED(tmp), bottomright);
   gtk_paned_set_resize_end_child(GTK_PANED(tmp), TRUE);
   gtk_paned_set_shrink_end_child(GTK_PANED(tmp), FALSE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   *percent = 50.0;
   g_object_set_data(G_OBJECT(tmp), "_dw_percent", (gpointer)percent);
   g_signal_connect(G_OBJECT(tmp), "realize", G_CALLBACK(_dw_splitbar_realize), NULL);
   gtk_widget_set_visible(tmp, TRUE);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
DW_FUNCTION_DEFINITION(dw_splitbar_set, void, HWND handle, float percent)
DW_FUNCTION_ADD_PARAM2(handle, percent)
DW_FUNCTION_NO_RETURN(dw_splitbar_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, percent, float)
{
   float *mypercent = (float *)dw_window_get_data(handle, "_dw_percent");
   int size = 0, position;

   if(gtk_orientable_get_orientation(GTK_ORIENTABLE(handle)) == GTK_ORIENTATION_HORIZONTAL)
   {
#if GTK_CHECK_VERSION(4,12,0)
      size = gtk_widget_get_width(handle);
#else
      size = gtk_widget_get_allocated_width(handle);
#endif
   }
   else
   {
#if GTK_CHECK_VERSION(4,12,0)
      size = gtk_widget_get_height(handle);
#else
      size = gtk_widget_get_allocated_height(handle);
#endif
   }

   if(mypercent)
      *mypercent = percent;

   if(size > 10)
   {
      position = (int)((float)size * (percent / 100.0));

      gtk_paned_set_position(GTK_PANED(handle), position);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
DW_FUNCTION_DEFINITION(dw_splitbar_get, float, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_splitbar_get, float)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
   float *percent = (float *)dw_window_get_data(handle, "_dw_percent");
   float retval = 0.0;

   if(percent)
      retval = *percent;
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Creates a calendar window (widget) with given parameters.
 * Parameters:
 *       id: Unique identifier for calendar widget
 * Returns:
 *       A handle to a calendar window or NULL on failure.
 */
DW_FUNCTION_DEFINITION(dw_calendar_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_calendar_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(cid, ULONG)
{
   GtkWidget *tmp = gtk_calendar_new();
   GTimeZone *tz = g_time_zone_new_local();
   GDateTime *now = g_date_time_new_now(tz);

   gtk_widget_set_visible(tmp, TRUE);
   g_object_set_data(G_OBJECT(tmp), "_dw_id", GINT_TO_POINTER(cid));
   /* select today */
   gtk_calendar_set_show_day_names(GTK_CALENDAR(tmp), TRUE);
   gtk_calendar_set_show_heading(GTK_CALENDAR(tmp), TRUE);
   gtk_calendar_select_day(GTK_CALENDAR(tmp), now);
   g_date_time_unref(now);
   g_time_zone_unref(tz);
   DW_FUNCTION_RETURN_THIS(tmp);
}

/*
 * Sets the current date of a calendar
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
DW_FUNCTION_DEFINITION(dw_calendar_set_date, void, HWND handle, unsigned int year, unsigned int month, unsigned int day)
DW_FUNCTION_ADD_PARAM4(handle, year, month, day)
DW_FUNCTION_NO_RETURN(dw_calendar_set_date)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, year, unsigned int, month, unsigned int, day, unsigned int)
{
   if(GTK_IS_CALENDAR(handle))
   {
      GTimeZone *tz = g_time_zone_new_local();
      GDateTime *datetime = g_date_time_new(tz, year, month, day, 0, 0, 0);
      gtk_calendar_select_day(GTK_CALENDAR(handle), datetime);
      g_date_time_unref(datetime);
      g_time_zone_unref(tz);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
DW_FUNCTION_DEFINITION(dw_calendar_get_date, void, HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
DW_FUNCTION_ADD_PARAM4(handle, year, month, day)
DW_FUNCTION_NO_RETURN(dw_calendar_get_date)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, year, unsigned int *, month, unsigned int *, day, unsigned int *)
{
   if(GTK_IS_CALENDAR(handle))
   {
      GDateTime *datetime = gtk_calendar_get_date(GTK_CALENDAR(handle));
      if(year) 
         *year = g_date_time_get_year(datetime);
      if(month)
         *month = g_date_time_get_month(datetime);
      if(day)
         *day = g_date_time_get_day_of_month(datetime);
   }
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the current focus item for a window/dialog.
 * Parameters:
 *         handle: Handle to the dialog item to be focused.
 * Remarks:
 *          This is for use after showing the window/dialog.
 */
DW_FUNCTION_DEFINITION(dw_window_set_focus, void, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_NO_RETURN(dw_window_set_focus)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    if(handle)
       gtk_widget_grab_focus(handle);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND window, HWND defaultitem)
{
   if(window)
      g_object_set_data(G_OBJECT(window),  "_dw_defaultitem", (gpointer)defaultitem);
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
DW_FUNCTION_DEFINITION(dw_window_click_default, void, HWND window, HWND next)
DW_FUNCTION_ADD_PARAM2(window, next)
DW_FUNCTION_NO_RETURN(dw_window_click_default)
DW_FUNCTION_RESTORE_PARAM2(window, HWND, next, HWND)
{
   if(window && next && GTK_IS_WIDGET(window) && GTK_IS_WIDGET(next))
   {
      GtkEventController *controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(GTK_WIDGET(window), controller);
      g_signal_connect(G_OBJECT(controller), "key-pressed", G_CALLBACK(_dw_default_key_press_event), next);
   }
   DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Creates a new system notification if possible.
 * Parameters:
 *         title: The short title of the notification.
 *         imagepath: Path to an image to display or NULL if none.
 *         description: A longer description of the notification,
 *                      or NULL if none is necessary.
 * Returns:
 *         A handle to the notification which can be used to attach a "clicked" event if desired,
 *         or NULL if it fails or notifications are not supported by the system.
 * Remarks:
 *          This will create a system notification that will show in the notifaction panel
 *          on supported systems, which may be clicked to perform another task.
 */
HWND API dw_notification_new(const char *title, const char *imagepath, const char *description, ...)
{
   GNotification *notification = g_notification_new(title);

   if(notification)
   {
      if(description)
      {
         va_list args;
         char outbuf[1025] = {0};

         va_start(args, description);
         vsnprintf(outbuf, 1024, description, args);
         va_end(args);

         g_notification_set_body(notification, outbuf);
      }
      if(imagepath)
      {
         GFile *file = g_file_new_for_path(imagepath);
         GBytes *bytes = g_file_load_bytes(file, NULL, NULL, NULL);
         
         if(bytes)
         {
            GIcon *icon = g_bytes_icon_new(bytes);
            
            if(icon)
               g_notification_set_icon(notification, G_ICON(icon));
         }
      }
      g_notification_set_default_action_and_target(notification, "app.notification", "t", 
                                                  (guint64)DW_POINTER_TO_ULONGLONG(notification)); 
   }
   return (HWND)notification;
}

/*
 * Sends a notification created by dw_notification_new() after attaching signal handler.
 * Parameters:
 *         notification: The handle to the notification returned by dw_notification_new().
 * Returns:
 *         DW_ERROR_NONE on success, DW_ERROR_UNKNOWN on error or not supported.
 */
int API dw_notification_send(HWND notification)
{
   if(notification)
   {
      char id[101] = {0};

      /* Generate a unique ID based on the notification handle,
       * so we can use it to remove the notification in dw_window_destroy().
       */
      snprintf(id, 100, "dw-notification-%llu", DW_POINTER_TO_ULONGLONG(notification));
      g_application_send_notification(_DWApp, id, (GNotification *)notification);
      return DW_ERROR_NONE;
   }
   return DW_ERROR_UNKNOWN;
}

/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
   struct utsname name;
   char tempbuf[_DW_ENV_STRING_SIZE] = { 0 }, *dot;

   uname(&name);
   memset(env, '\0', sizeof(DWEnv));
   strncpy(env->osName, name.sysname, sizeof(env->osName)-1);
   strncpy(tempbuf, name.release, sizeof(tempbuf)-1);

   strncpy(env->buildDate, __DATE__, sizeof(env->buildDate)-1);
   strncpy(env->buildTime, __TIME__, sizeof(env->buildTime)-1);
#ifdef USE_WEBKIT
   strncpy(env->htmlEngine, "WEBKIT2", sizeof(env->htmlEngine)-1);
#else
   strncpy(env->htmlEngine, "N/A", sizeof(env->htmlEngine)-1);
#endif
   env->DWMajorVersion = DW_MAJOR_VERSION;
   env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
   env->DWSubVersion = VER_REV;
#else
   env->DWSubVersion = DW_SUB_VERSION;
#endif

   if((dot = strchr(tempbuf, '.')) != NULL)
   {
      *dot = '\0';
      env->MajorVersion = atoi(tempbuf);
      env->MinorVersion = atoi(&dot[1]);
      return;
   }
   env->MajorVersion = atoi(tempbuf);
}

#if GTK_CHECK_VERSION(4,10,0)
static void _dw_file_browse_response(GObject *gobject, GAsyncResult *result, gpointer data)
{
  DWDialog *tmp = data;
  GError *error = NULL;
  char *filename = NULL;
  GFile *file = NULL;

  /* Bail out if there is no DWDialog */
  if(!tmp)
    return;

  switch(DW_POINTER_TO_INT(tmp->data))
  {
     case DW_DIRECTORY_OPEN:
        file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(gobject), result, &error);
        break;
     case DW_FILE_OPEN:
        file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(gobject), result, &error);
        break;
     case DW_FILE_SAVE:
        file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(gobject), result, &error);
        break;
     default:
        break;
   }

  if(error == NULL && file != NULL)
  {
    filename = g_file_get_path(file);
    g_object_unref(G_OBJECT(file));
  }
  dw_dialog_dismiss(tmp, filename);
}
#endif

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extension.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE or DW_DIRECTORY_OPEN
 * Returns:
 *       NULL on error. A malloced buffer containing
 *       the file path on success.
 *
 */
DW_FUNCTION_DEFINITION(dw_file_browse, char *, const char *title, const char *defpath, const char *ext, int flags)
DW_FUNCTION_ADD_PARAM4(title, defpath, ext, flags)
DW_FUNCTION_RETURN(dw_file_browse, char *)
DW_FUNCTION_RESTORE_PARAM4(title, const char *, defpath, const char *, ext, const char *, flags, int)
{
   char buf[1001] = {0};
   char *filename = NULL;
   DWDialog *tmp = dw_dialog_new(DW_INT_TO_POINTER(flags));
#if GTK_CHECK_VERSION(4,10,0)
   GtkFileDialog *dialog = gtk_file_dialog_new();

   gtk_file_dialog_set_title(dialog, title);
   if(defpath)
   {
      GFile *path = g_file_new_for_path(defpath);

      /* See if the path exists */
      if(path)
      {
         /* If the path is a directory... set the current folder */
         if(g_file_query_file_type(path, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
            gtk_file_dialog_set_initial_folder(dialog, path);
         else
            gtk_file_dialog_set_initial_file(dialog, path);

         g_object_unref(G_OBJECT(path));
      }
   }
   if(ext)
   {
      GListStore *filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
      GtkFileFilter *filter = gtk_file_filter_new();
      snprintf(buf, 1000, "*.%s", ext);
      gtk_file_filter_add_pattern(filter, (gchar *)buf);
      snprintf(buf, 1000, "\"%s\" files", ext);
      gtk_file_filter_set_name(filter, (gchar *)buf);
      g_list_store_append(filters, filter);
      filter = gtk_file_filter_new();
      gtk_file_filter_add_pattern(filter, (gchar *)"*");
      gtk_file_filter_set_name(filter, (gchar *)"All Files");
      g_list_store_append(filters, filter);
      gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
   }

   switch(flags)
   {
      case DW_DIRECTORY_OPEN:
         gtk_file_dialog_select_folder(dialog, NULL, NULL, (GAsyncReadyCallback)_dw_file_browse_response, tmp);
         break;
      case DW_FILE_OPEN:
         gtk_file_dialog_open(dialog, NULL, NULL, (GAsyncReadyCallback)_dw_file_browse_response, tmp);
         break;
      case DW_FILE_SAVE:
         gtk_file_dialog_save(dialog, NULL, NULL, (GAsyncReadyCallback)_dw_file_browse_response, tmp);
         break;
      default:
         dw_messagebox( "Coding error", DW_MB_OK|DW_MB_ERROR, "dw_file_browse() flags argument invalid.");
         tmp = NULL;
         break;
   }

   if(tmp)
     filename = dw_dialog_wait(tmp);
#else
   GtkWidget *filew;
   GtkFileChooserAction action;
   gchar *button = NULL;

   switch(flags)
   {
      case DW_DIRECTORY_OPEN:
         action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
         button = _("_Open");
         break;
      case DW_FILE_OPEN:
         action = GTK_FILE_CHOOSER_ACTION_OPEN;
         button = _("_Open");
         break;
      case DW_FILE_SAVE:
         action = GTK_FILE_CHOOSER_ACTION_SAVE;
         button = _("_Save");
         break;
      default:
         dw_messagebox( "Coding error", DW_MB_OK|DW_MB_ERROR, "dw_file_browse() flags argument invalid.");
         break;
   }

   if(button)
   {
      filew = gtk_file_chooser_dialog_new(title,
                                          NULL,
                                          action,
                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                          button, GTK_RESPONSE_ACCEPT,
                                          NULL);

      if(ext)
      {
         GtkFileFilter *filter = gtk_file_filter_new();
         snprintf(buf, 1000, "*.%s", ext);
         gtk_file_filter_add_pattern(filter, (gchar *)buf);
         snprintf(buf, 1000, "\"%s\" files", ext);
         gtk_file_filter_set_name(filter, (gchar *)buf);
         gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filew), filter);
         filter = gtk_file_filter_new();
         gtk_file_filter_add_pattern(filter, (gchar *)"*");
         gtk_file_filter_set_name(filter, (gchar *)"All Files");
         gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filew), filter);
      }

      if(defpath)
      {
         GFile *path = g_file_new_for_path(defpath);

         /* See if the path exists */
         if(path)
         {
            /* If the path is a directory... set the current folder */
            if(g_file_query_file_type(path, G_FILE_QUERY_INFO_NONE, NULL) == G_FILE_TYPE_DIRECTORY)
               gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filew), path, NULL);
            else
               gtk_file_chooser_set_file(GTK_FILE_CHOOSER(filew), path, NULL);

            g_object_unref(G_OBJECT(path));
         }
      }

      gtk_widget_set_visible(GTK_WIDGET(filew), TRUE);
      g_signal_connect(G_OBJECT(filew), "response", G_CALLBACK(_dw_dialog_response), (gpointer)tmp);

      if(DW_POINTER_TO_INT(dw_dialog_wait(tmp)) == GTK_RESPONSE_ACCEPT)
      {
         GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(filew));
         filename = g_file_get_path(file);
         g_object_unref(G_OBJECT(file));
      }

      if(GTK_IS_WINDOW(filew))
         gtk_window_destroy(GTK_WINDOW(filew));
   }
#endif
   DW_FUNCTION_RETURN_THIS(filename);
}

static void _dw_exec_launched(GAppLaunchContext *context, GAppInfo *info, GVariant *platform_data, gpointer data)
{
   g_variant_lookup(platform_data, "pid", "i", data);
}

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       DW_ERROR_UNKNOWN on error.
 */
int API dw_exec(const char *program, int type, char **params)
{
   GAppInfo *appinfo = NULL;
   char *commandline;
   int retval = DW_ERROR_UNKNOWN;
   
   /* Generate a command line from the parameters */
   if(params && *params)
   {
      int z = 0, len = 0;
      
      while(params[z])
      {
         len+=strlen(params[z]) + 1;
         z++;
      }
      z=1;
      commandline = calloc(1, len);
      strcpy(commandline, params[0]);
      while(params[z])
      {
         strcat(commandline, " ");
         strcat(commandline, params[z]);
         z++;
      }
   }
   else
      commandline = strdup(program);

   /* Attempt to use app preferences to launch the application, using the selected Terminal if necessary */
   if((appinfo = g_app_info_create_from_commandline(commandline, NULL, 
      type == DW_EXEC_CON ? G_APP_INFO_CREATE_NEEDS_TERMINAL : G_APP_INFO_CREATE_NONE, NULL)))
   {
      GAppLaunchContext *context = g_app_launch_context_new();

      g_signal_connect(G_OBJECT(context), "launched", G_CALLBACK(_dw_exec_launched), (gpointer)&retval);

      g_app_info_launch(appinfo, NULL, context, NULL);

      g_object_unref(appinfo);
      g_object_unref(context);
   }
   free(commandline);
   return retval;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(const char *url)
{
   /* If possible load the URL/URI using gvfs... */
   gtk_show_uri(NULL, url, GDK_CURRENT_TIME);
   return DW_ERROR_NONE;
}

#ifdef USE_WEBKIT
/* Helper function to get the web view handle */
WebKitWebView *_dw_html_web_view(GtkWidget *widget)
{
   if(widget)
   {
      WebKitWebView *web_view = (WebKitWebView *)widget;
      if(WEBKIT_IS_WEB_VIEW(web_view))
         return web_view;
#ifndef USE_WEBKIT2
      web_view = (WebKitWebView *)g_object_get_data(G_OBJECT(widget), "_dw_web_view");
      if(WEBKIT_IS_WEB_VIEW(web_view))
         return web_view;
#endif
   }
   return NULL;
}
#endif
/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
#ifdef USE_WEBKIT
   WebKitWebView *web_view;

   if((web_view = _dw_html_web_view(handle)))
   {
      switch(action)
      {
         case DW_HTML_GOBACK:
            webkit_web_view_go_back(web_view);
            break;
         case DW_HTML_GOFORWARD:
            webkit_web_view_go_forward(web_view);
            break;
         case DW_HTML_GOHOME:
            webkit_web_view_load_uri(web_view, DW_HOME_URL);
            break;
         case DW_HTML_RELOAD:
            webkit_web_view_reload(web_view);
            break;
         case DW_HTML_STOP:
            webkit_web_view_stop_loading(web_view);
            break;
         case DW_HTML_PRINT:
            {
               WebKitPrintOperation *operation = webkit_print_operation_new(web_view);
               webkit_print_operation_run_dialog(operation, NULL);
               g_object_unref(operation);
            }
            break;
      }
   }
#endif
}

/*
 * Render raw HTML code in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       string: String buffer containt HTML code to
 *               be rendered.
 * Returns:
 *       0 on success.
 */
int API dw_html_raw(HWND handle, const char *string)
{
#ifdef USE_WEBKIT
   WebKitWebView *web_view;

   if((web_view = _dw_html_web_view(handle)))
   {
      webkit_web_view_load_html(web_view, string, "file:///");
      gtk_widget_set_visible(GTK_WIDGET(handle), TRUE);
   }
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/*
 * Render file or web page in the embedded HTML widget..
 * Parameters:
 *       handle: Handle to the window.
 *       url: Universal Resource Locator of the web or
 *               file object to be rendered.
 * Returns:
 *       0 on success.
 */
int API dw_html_url(HWND handle, const char *url)
{
#ifdef USE_WEBKIT
   WebKitWebView *web_view;

   if((web_view = _dw_html_web_view(handle)))
   {
      webkit_web_view_load_uri(web_view, url);
      gtk_widget_set_visible(GTK_WIDGET(handle), TRUE);
   }
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/*
 * Executes the javascript contained in "script" in the HTML window.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       script: Javascript code to execute.
 *       scriptdata: Data passed to the signal handler.
 * Notes: A DW_SIGNAL_HTML_RESULT event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_javascript_run(HWND handle, const char *script, void *scriptdata)
{
#ifdef USE_WEBKIT
   WebKitWebView *web_view;

   if(script && (web_view = _dw_html_web_view(handle)))
#ifdef USE_WEBKIT6
      webkit_web_view_evaluate_javascript(web_view, script, strlen(script), NULL, NULL, NULL, _dw_html_result_event, scriptdata);
#else
      webkit_web_view_run_javascript(web_view, script, NULL, _dw_html_result_event, scriptdata);
#endif
   return DW_ERROR_NONE;
#else
   return DW_ERROR_UNKNOWN;
#endif
}

/* Free the name when the signal disconnects */
void _dw_html_message_disconnect(gpointer gdata, GClosure *closure)
{
    gpointer *data = (gpointer *)gdata;

    if(data)
    {
        gchar *name = (gchar *)data[1];
        
        if(name)
            g_free(name);
        free(data);
    }
}

/*
 * Install a javascript function with name that can call native code.
 * Parameters:
 *       handle: Handle to the HTML window.
 *       name: Javascript function name.
 * Notes: A DW_SIGNAL_HTML_MESSAGE event will be raised with scriptdata.
 * Returns:
 *       DW_ERROR_NONE (0) on success.
 */
int API dw_html_javascript_add(HWND handle, const char *name)
{
#ifdef USE_WEBKIT
   WebKitWebView *web_view= _dw_html_web_view(handle);
   WebKitUserContentManager *manager;

    if(web_view && (manager = webkit_web_view_get_user_content_manager(web_view)) && name) 
    {
        /* Script to inject that will call the handler we are adding */
        gchar *script = g_strdup_printf("function %s(body) {window.webkit.messageHandlers.%s.postMessage(body);}", 
                            name, name);
        gchar *signal = g_strdup_printf("script-message-received::%s", name);
        WebKitUserScript *userscript = webkit_user_script_new(script, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                              WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
        gpointer *data = calloc(sizeof(gpointer), 2);
        
        data[0] = handle;
        data[1] = g_strdup(name);
        g_signal_connect_data(manager, signal, G_CALLBACK(_dw_html_message_event), data, _dw_html_message_disconnect, 0);
        webkit_user_content_manager_register_script_message_handler(manager, name
#if USE_WEBKIT6
                                                                    , NULL
#endif
        );
        webkit_user_content_manager_add_script(manager, userscript);

        g_free(script);
        g_free(signal);
        return DW_ERROR_NONE;
    }
#endif
    return DW_ERROR_UNKNOWN;
}

/*
 * Create a new HTML window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_html_new, HWND, DW_UNUSED(ULONG cid))
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_html_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(DW_UNUSED(cid), ULONG)
{
   GtkWidget *widget = NULL;
#ifdef USE_WEBKIT
   WebKitWebView *web_view;
   WebKitSettings *settings;

   web_view = (WebKitWebView *)webkit_web_view_new();
   settings = webkit_web_view_get_settings(web_view);
   /* Make sure java script is enabled */
   webkit_settings_set_enable_javascript(settings, TRUE);
   webkit_web_view_set_settings(web_view, settings);
   widget = (GtkWidget *)web_view;
   g_object_set_data(G_OBJECT(widget), "_dw_id", GINT_TO_POINTER(cid));
   gtk_widget_set_visible(widget, TRUE);
#else
   dw_debug( "HTML widget not available; you do not have access to webkit.\n" );
#endif
   DW_FUNCTION_RETURN_THIS(widget);
}

static void _dw_clipboard_callback(GObject *object, GAsyncResult *res, gpointer data)
{
   DWDialog *tmp = (DWDialog *)data;
   
   if(tmp && tmp->data)
   {
      char *text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(tmp->data), res, NULL);
      
      dw_dialog_dismiss(tmp, text ? strdup(text) : text);
   }
}

/*
 * Gets the contents of the default clipboard as text.
 * Parameters:
 *       None.
 * Returns:
 *       Pointer to an allocated string of text or NULL if clipboard empty or contents could not
 *       be converted to text.
 */
char * API dw_clipboard_get_text(void)
{
   GdkDisplay *display = gdk_display_get_default();
   GdkClipboard *clipboard;
   char *ret = NULL;

   if((clipboard = gdk_display_get_clipboard(display)))
   {
      DWDialog *tmp = dw_dialog_new(DW_POINTER(clipboard));
      
      gdk_clipboard_read_text_async(clipboard, NULL, _dw_clipboard_callback, (gpointer)tmp);
      
      ret = (char *)dw_dialog_wait(tmp);
   }
   return ret;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
   GdkDisplay *display = gdk_display_get_default();
   GdkClipboard *clipboard;

   if((clipboard = gdk_display_get_clipboard(display)))
      gdk_clipboard_set_text(clipboard, str);
}

/* Internal function to create the drawable pixmap and call the function */
static void _dw_draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr)
{
   cairo_t *cr = gtk_print_context_get_cairo_context(context);
   void *drawdata = g_object_get_data(G_OBJECT(operation), "_dw_drawdata");
   int (*drawfunc)(HPRINT, HPIXMAP, int, void *) = g_object_get_data(G_OBJECT(operation), "_dw_drawfunc");
   int result = 0;
   HPIXMAP pixmap;

   if(cr && drawfunc && (pixmap = calloc(1,sizeof(struct _hpixmap))))
   {
      pixmap->image = cairo_get_group_target(cr);
      pixmap->handle = (HWND)operation;
      pixmap->width = gtk_print_context_get_width(context);
      pixmap->height = gtk_print_context_get_height(context);
      result = drawfunc((HPRINT)operation, pixmap, page_nr, drawdata);
      if(result)
         gtk_print_operation_draw_page_finish(operation);
      free(pixmap);
   }
}

/*
 * Creates a new print object.
 * Parameters:
 *       jobname: Name of the print job to show in the queue.
 *       flags: Flags to initially configure the print object.
 *       pages: Number of pages to print.
 *       drawfunc: The pointer to the function to be used as the callback.
 *       drawdata: User data to be passed to the handler function.
 * Returns:
 *       A handle to the print object or NULL on failure.
 */
HPRINT API dw_print_new(const char *jobname, unsigned long flags, unsigned int pages, void *drawfunc, void *drawdata)
{
   GtkPrintOperation *op;

   if(!drawfunc)
      return NULL;

   if((op = gtk_print_operation_new()))
   {
      gtk_print_operation_set_n_pages(op, pages);
      gtk_print_operation_set_job_name(op, jobname ? jobname : "Dynamic Windows Print Job");
      g_object_set_data(G_OBJECT(op), "_dw_drawfunc", drawfunc);
      g_object_set_data(G_OBJECT(op), "_dw_drawdata", drawdata);
      g_signal_connect(op, "draw_page", G_CALLBACK(_dw_draw_page), NULL);
   }
   return (HPRINT)op;
}

/*
 * Runs the print job, causing the draw page callbacks to fire.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 *       flags: Flags to run the print job.
 * Returns:
 *       DW_ERROR_UNKNOWN on error or DW_ERROR_NONE on success.
 */
int API dw_print_run(HPRINT print, unsigned long flags)
{
   GtkPrintOperationResult res;
   GtkPrintOperation *op = (GtkPrintOperation *)print;

   res = gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);
   return (res == GTK_PRINT_OPERATION_RESULT_ERROR ? DW_ERROR_UNKNOWN : DW_ERROR_NONE);
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
   GtkPrintOperation *op = (GtkPrintOperation *)print;

   gtk_print_operation_cancel(op);
}

/*
 * Returns a pointer to a static buffer which contains the
 * current user directory.  Or the root directory (C:\ on
 * OS/2 and Windows).
 */
char * API dw_user_dir(void)
{
   static char _user_dir[1024] = "";

   if(!_user_dir[0])
   {
      char *home = getenv("HOME");

      if(home)
         strcpy(_user_dir, home);
      else
         strcpy(_user_dir, "/");
   }
   return _user_dir;
}

/*
 * Returns a pointer to a static buffer which containes the
 * private application data directory.
 */
char * API dw_app_dir(void)
{
    return _dw_share_path;
}

/*
 * Sets the application ID used by this Dynamic Windows application instance.
 * Parameters:
 *         appid: A string typically in the form: com.company.division.application
 *         appname: The application name used on Windows or NULL.
 * Returns:
 *         DW_ERROR_NONE after successfully setting the application ID.
 *         DW_ERROR_UNKNOWN if unsupported on this system.
 *         DW_ERROR_GENERAL if the application ID is not allowed.
 * Remarks:
 *          This must be called before dw_init().  If dw_init() is called first
 *          it will create a unique ID in the form: org.dbsoft.dwindows.application
 *          or if the application name cannot be detected: org.dbsoft.dwindows.pid.#
 *          The appname is only required on Windows.  If NULL is passed the detected
 *          application name will be used, but a prettier name may be desired.
 */
int API dw_app_id_set(const char *appid, const char *appname)
{
   if(g_application_id_is_valid(appid))
   {
      strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
      if(appname)
      	g_set_application_name(appname);
      return DW_ERROR_NONE;
   }
   return DW_ERROR_GENERAL;
}

/*
 * Call a function from the window (widget)'s context.
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
DW_FUNCTION_DEFINITION(dw_window_function, void, DW_UNUSED(HWND handle), void *function, void *data)
DW_FUNCTION_ADD_PARAM3(handle, function, data)
DW_FUNCTION_NO_RETURN(dw_window_function)
DW_FUNCTION_RESTORE_PARAM3(DW_UNUSED(handle), HWND, function, void *, data, void *)
{
   void (* windowfunc)(void *);

   windowfunc = function;

   if(windowfunc)
      windowfunc(data);
   DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Add a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void API dw_window_set_data(HWND window, const char *dataname, void *data)
{
   HWND thiswindow = window;

   if(!window)
      return;

   if(GTK_IS_SCROLLED_WINDOW(window))
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }
   if(thiswindow && G_IS_OBJECT(thiswindow))
      g_object_set_data(G_OBJECT(thiswindow), dataname, (gpointer)data);
}

/*
 * Gets a named user data item to a window handle.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       dataname: A string pointer identifying which signal to be hooked.
 *       data: User data to be passed to the handler function.
 */
void * API dw_window_get_data(HWND window, const char *dataname)
{
   HWND thiswindow = window;
   void *ret = NULL;

   if(!window)
      return NULL;

   if(GTK_IS_SCROLLED_WINDOW(window))
   {
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   }
   if(G_IS_OBJECT(thiswindow))
      ret = (void *)g_object_get_data(G_OBJECT(thiswindow), dataname);
   return ret;
}

/*
 * Compare two window handles.
 * Parameters:
 *       window1: First window handle to compare.
 *       window2: Second window handle to compare.
 * Returns:
 *       TRUE if the windows are the same object, FALSE if not.
 */
int API dw_window_compare(HWND window1, HWND window2)
{
    /* If anything special is require to compare... do it
     * here otherwise just compare the handles.
     */
    if(window1 && window2 && window1 == window2)
        return TRUE;
    return FALSE;
}

/* Internal function to get the state of the timer before firing */
gboolean _dw_timer_func(gpointer data)
{
   void (*sigfunc)(void *data) = NULL;
   void *sdata;
   char tmpbuf[31] = {0};
   int *tag = data;

   if(tag)
   {
      snprintf(tmpbuf, 30, "_dw_timer%d", *tag);
      sigfunc = g_object_get_data(G_OBJECT(_DWObject), tmpbuf);
      snprintf(tmpbuf, 30, "_dw_timerdata%d", *tag);
      sdata = g_object_get_data(G_OBJECT(_DWObject), tmpbuf);
   }
   if(!sigfunc)
   {
      if(tag)
         free(tag);
      return FALSE;
   }
   sigfunc(sdata);
   return TRUE;
}

/*
 * Add a callback to a timer event.
 * Parameters:
 *       interval: Milliseconds to delay between calls.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 * Returns:
 *       Timer ID for use with dw_timer_disconnect(), 0 on error.
 */
HTIMER API dw_timer_connect(int interval, void *sigfunc, void *data)
{
   int *tag;
   char tmpbuf[31] = {0};

   tag = calloc(1, sizeof(int));

   *tag = g_timeout_add(interval, (GSourceFunc)_dw_timer_func, (gpointer)tag);
   snprintf(tmpbuf, 30, "_dw_timer%d", *tag);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, sigfunc);
   snprintf(tmpbuf, 30, "_dw_timerdata%d", *tag);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, data);
   return *tag;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(HTIMER id)
{
   char tmpbuf[31] = {0};

   snprintf(tmpbuf, 30, "_dw_timer%d", id);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, NULL);
   snprintf(tmpbuf, 30, "_dw_timerdata%d", id);
   g_object_set_data(G_OBJECT(_DWObject), tmpbuf, NULL);
}

/* Get the actual signal window handle not the user window handle
 * Should mimic the code in dw_signal_connect() below.
 */
static HWND _dw_find_signal_window(HWND window, const char *signame)
{
   HWND thiswindow = window;

   if(GTK_IS_SCROLLED_WINDOW(thiswindow))
      thiswindow = (HWND)g_object_get_data(G_OBJECT(window), "_dw_user");
   else if(GTK_IS_SCALE(thiswindow) || GTK_IS_SCROLLBAR(thiswindow) || GTK_IS_SPIN_BUTTON(thiswindow))
      thiswindow = (GtkWidget *)g_object_get_data(G_OBJECT(thiswindow), "_dw_adjustment");
   else if(GTK_IS_TREE_VIEW(thiswindow) && strcmp(signame, DW_SIGNAL_ITEM_SELECT) == 0)
      thiswindow = (GtkWidget *)gtk_tree_view_get_selection(GTK_TREE_VIEW(thiswindow));
   return thiswindow;
}

/*
 * Add a callback to a window event.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect(HWND window, const char *signame, void *sigfunc, void *data)
{
   dw_signal_connect_data(window, signame, sigfunc, NULL, data);
}

/* Internal function to free any allocated signal data..
 * and call any required function to free additional memory.
 */
static void _dw_signal_disconnect(gpointer data, GClosure *closure)
{
   if(data)
   {
      void **params = (void **)data;
      void (*discfunc)(HWND, void *) = params[1];

      if(discfunc)
      {
         DWSignalHandler work = _dw_get_signal_handler(data);

         if(work.window)
         {
            discfunc(work.window, work.data);
         }
      }
      free(data);
   }
}

#define _DW_INTERNAL_CALLBACK_PARAMS 4

/* Signal setup functions */
GObject *_dw_key_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   /* Special case for item enter... we need a mouse callback too */
   if(strcmp(signal->name, DW_SIGNAL_ITEM_ENTER) == 0)
   {
      GtkGesture *gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(GTK_WIDGET(object), GTK_EVENT_CONTROLLER(gesture));
      int cid, sigid = _dw_set_signal_handler(G_OBJECT(object), (HWND)object, sigfunc, data, (gpointer)_dw_container_enter_mouse, discfunc);
      void **newparams = calloc(sizeof(void *), 3);

      newparams[0] = DW_INT_TO_POINTER(sigid);
      newparams[2] = DW_POINTER(object);
      cid = g_signal_connect_data(G_OBJECT(gesture), "pressed", G_CALLBACK(_dw_container_enter_mouse), newparams, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(object, sigid, cid);
   }
   if(GTK_IS_WIDGET(object))
   {
      GtkEventController *controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(GTK_WIDGET(object), controller);
      return G_OBJECT(controller);
   }
   return object;
}

GObject *_dw_button_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   /* Special case for handling notification signals, which aren't really signals */
   if(G_IS_NOTIFICATION(object) && strcmp(signal->name, DW_SIGNAL_CLICKED) == 0)
   {
      char textbuf[101] = {0};
      snprintf(textbuf, 100, "dw-notification-%llu-func", DW_POINTER_TO_ULONGLONG(object));
      g_object_set_data(G_OBJECT(_DWApp), textbuf, DW_POINTER(sigfunc));
      snprintf(textbuf, 100, "dw-notification-%llu-data", DW_POINTER_TO_ULONGLONG(object));
      g_object_set_data(G_OBJECT(_DWApp), textbuf, DW_POINTER(data)); 
      return NULL;
   }
   /* GTK signal name for check buttons is "toggled" not "clicked" */
   else if(GTK_IS_CHECK_BUTTON(object) && strcmp(signal->name, DW_SIGNAL_CLICKED) == 0)
      strcpy(signal->gname, "toggled");
   /* For menu items, get the G(Simple)Action and the signal is "activate" */ 
   else if(G_IS_MENU_ITEM(object) && strcmp(signal->name, DW_SIGNAL_CLICKED) == 0)
   {
      GSimpleAction *action = G_SIMPLE_ACTION(g_object_get_data(object, "_dw_action"));
      
      if(action)
      {
         int cid, sigid = _dw_set_signal_handler(G_OBJECT(object), (HWND)object, sigfunc, data, (gpointer)_dw_menu_handler, discfunc);
         void **newparams = calloc(sizeof(void *), 3);

         newparams[0] = DW_INT_TO_POINTER(sigid);
         newparams[1] = discfunc;
         newparams[2] = DW_POINTER(object);
         cid = g_signal_connect_data(G_OBJECT(action), "activate", G_CALLBACK(_dw_menu_handler), newparams, _dw_signal_disconnect, 0);
         _dw_set_signal_handler_id(object, sigid, cid);
      }
      return NULL;
   } 
   return object;
}

GObject *_dw_mouse_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_WIDGET(object))
   {
      GtkGesture *gesture = gtk_gesture_click_new();
      gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
      gtk_widget_add_controller(GTK_WIDGET(object), GTK_EVENT_CONTROLLER(gesture));
      return G_OBJECT(gesture);
   }
   return object;
}

GObject *_dw_motion_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_WIDGET(object))
   {
      GtkEventController *controller = gtk_event_controller_motion_new();
      gtk_widget_add_controller(GTK_WIDGET(object), controller);
      return G_OBJECT(controller);
   }
   return object;
}

GObject *_dw_draw_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_DRAWING_AREA(object))
   {
      g_object_set_data(object, "_dw_expose_func", sigfunc);
      gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(object), signal->func, data, NULL);
      return NULL;
   }
   return object;
}

GObject *_dw_tree_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_TREE_VIEW(object))
   {
      if(strcmp(signal->name, DW_SIGNAL_COLUMN_CLICK) == 0)
      {
         /* We don't actually need a signal handler here... just need to assign the handler ID
          * Since the handlers for the columns were already created in _dw_container_setup()
          */
         int sigid = _dw_set_signal_handler(object, (HWND)object, sigfunc, data, signal->func, discfunc);
         g_object_set_data(object, "_dw_column_click_id", GINT_TO_POINTER(sigid+1));
         return NULL;
      }
      else if(strcmp(signal->name, DW_SIGNAL_ITEM_SELECT) == 0)
      {
         GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(object));
         return G_OBJECT(sel);
      }
      else
      {
         GtkGesture *gesture = gtk_gesture_click_new();
         /* Set button to return to 3 for context secondary clicks */
         if(strcmp(signal->name, DW_SIGNAL_ITEM_CONTEXT) == 0)
            gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 3);
         gtk_widget_add_controller(GTK_WIDGET(object), GTK_EVENT_CONTROLLER(gesture));
         return G_OBJECT(gesture);
      }
   }
   return object;
}

GObject *_dw_value_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_SCALE(object) || GTK_IS_SCROLLBAR(object) || GTK_IS_SPIN_BUTTON(object))
      return G_OBJECT(g_object_get_data(object, "_dw_adjustment"));
   return object;
}

GObject *_dw_focus_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(GTK_IS_COMBO_BOX(object) && strcmp(signal->name, DW_SIGNAL_SET_FOCUS) == 0)
      return G_OBJECT(gtk_combo_box_get_child(GTK_COMBO_BOX(object)));
   return object;
}

#ifdef USE_WEBKIT
GObject *_dw_html_setup(struct _dw_signal_list *signal, GObject *object, void *sigfunc, void *discfunc, void *data)
{
   if(WEBKIT_IS_WEB_VIEW(object))
   {
       if(strcmp(signal->name, DW_SIGNAL_HTML_RESULT) == 0)
       {
          /* We don't actually need a signal handler here... just need to assign the handler ID
           * Since the handler is created in dw_html_javasript_run()
           */
          int sigid = _dw_set_signal_handler(object, (HWND)object, sigfunc, data, signal->func, discfunc);
          g_object_set_data(object, "_dw_html_result_id", GINT_TO_POINTER(sigid+1));
          return NULL;
       }
       else if(strcmp(signal->name, DW_SIGNAL_HTML_MESSAGE) == 0)
       {
          /* We don't actually need a signal handler here... just need to assign the handler ID
           * Since the handler is created in dw_html_javasript_add()
           */
          int sigid = _dw_set_signal_handler(object, (HWND)object, sigfunc, data, signal->func, discfunc);
          g_object_set_data(object, "_dw_html_message_id", GINT_TO_POINTER(sigid+1));
          return NULL;
      }
   }
   return object;
}
#endif

/*
 * Add a callback to a window event with a closure callback.
 * Parameters:
 *       window: Window handle of signal to be called back.
 *       signame: A string pointer identifying which signal to be hooked.
 *       sigfunc: The pointer to the function to be used as the callback.
 *       discfunc: The pointer to the function called when this handler is removed.
 *       data: User data to be passed to the handler function.
 */
void API dw_signal_connect_data(HWND window, const char *signame, void *sigfunc, void *discfunc, void *data)
{
   DWSignalList signal = _dw_findsignal(signame);
   
   if(signal.func)
   {
      GObject *object = (GObject *)window;
      void **params;
      int sigid;
      gint cid;

      /*
       * If the window we are setting the signal on is a scrolled window we need to get
       * the "real" widget type. thiswindow is the "real" widget type
       */
      if (GTK_IS_SCROLLED_WINDOW(window)
   #ifdef USE_WEBKIT
          && !(object = G_OBJECT(_dw_html_web_view(window)))
   #endif
      )
         object = (GObject *)g_object_get_data(G_OBJECT(window), "_dw_user");

      /* Do object class specific setup */
      if(signal.setup)
         object = signal.setup(&signal, object, sigfunc, discfunc, data);

      if(!object)
         return;

      params = calloc(_DW_INTERNAL_CALLBACK_PARAMS, sizeof(void *));
      sigid = _dw_set_signal_handler(object, window, sigfunc, data, signal.func, discfunc);
      params[0] = DW_INT_TO_POINTER(sigid);
      /* Save the disconnect function pointer */
      params[1] = discfunc;
      params[2] = DW_POINTER(object);
      cid = g_signal_connect_data(object, signal.gname, G_CALLBACK(signal.func), params, _dw_signal_disconnect, 0);
      _dw_set_signal_handler_id(object, sigid, cid);
   }
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_name(HWND window, const char *signame)
{
   int z, count;
   DWSignalList signal;
   void **params = alloca(sizeof(void *) * 3);

   params[2] = _dw_find_signal_window(window, signame);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(params[2]), "_dw_sigcounter"));
   signal = _dw_findsignal(signame);
   
   if(signal.func)
   {
      for(z=0;z<count;z++)
      {
         DWSignalHandler sh;

         params[0] = GINT_TO_POINTER(z);
         sh = _dw_get_signal_handler(params);

         if(sh.intfunc == signal.func)
            _dw_remove_signal_handler((HWND)params[2], z);
      }
   }
}

/*
 * Removes all callbacks for a given window.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_window(HWND window)
{
   HWND thiswindow;
   int z, count;

   thiswindow = _dw_find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(thiswindow), "_dw_sigcounter"));

   for(z=0;z<count;z++)
      _dw_remove_signal_handler(thiswindow, z);
   g_object_set_data(G_OBJECT(thiswindow), "_dw_sigcounter", NULL);
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void API dw_signal_disconnect_by_data(HWND window, void *data)
{
   int z, count;
   void **params = alloca(sizeof(void *) * 3);

   params[2] = _dw_find_signal_window(window, NULL);
   count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(params[2]), "_dw_sigcounter"));

   for(z=0;z<count;z++)
   {
      DWSignalHandler sh;

      params[0] = GINT_TO_POINTER(z);
      sh = _dw_get_signal_handler(params);

      if(sh.data == data)
         _dw_remove_signal_handler((HWND)params[2], z);
   }
}

/*
 * Converts a UTF-8 encoded string into a wide string.
 * Parameters:
 *       utf8string: UTF-8 encoded source string.
 * Returns:
 *       Wide string that needs to be freed with dw_free()
 *       or NULL on failure.
 */
wchar_t * API dw_utf8_to_wchar(const char *utf8string)
{
   wchar_t *retval = NULL, *freeme;

   if(sizeof(wchar_t) == sizeof(gunichar))
      freeme = retval = (wchar_t *)g_utf8_to_ucs4(utf8string, -1, NULL, NULL, NULL);
   else if(sizeof(wchar_t) == sizeof(gunichar2))
      freeme = retval = (wchar_t *)g_utf8_to_utf16(utf8string, -1, NULL, NULL, NULL);
   if(retval)
   {
      retval = wcsdup(retval);
      g_free(freeme);
   }
   return retval;
}

/*
 * Converts a wide string into a UTF-8 encoded string.
 * Parameters:
 *       wstring: Wide source string.
 * Returns:
 *       UTF-8 encoded string that needs to be freed with dw_free()
 *       or NULL on failure.
 */
char * API dw_wchar_to_utf8(const wchar_t *wstring)
{
   char *retval = NULL, *freeme;

   if(sizeof(wchar_t) == sizeof(gunichar))
      freeme = retval = g_ucs4_to_utf8((gunichar *)wstring, -1, NULL, NULL, NULL);
   else if(sizeof(wchar_t) == sizeof(gunichar2))
      freeme = retval = g_utf16_to_utf8((gunichar2 *)wstring, -1, NULL, NULL, NULL);
   if(retval)
   {
      retval = strdup(retval);
      g_free(freeme);
   }
   return retval;
}

DW_FUNCTION_DEFINITION(dw_x11_check, int, int trueresult, int falseresult)
DW_FUNCTION_ADD_PARAM2(trueresult, falseresult)
DW_FUNCTION_RETURN(dw_x11_check, int)
DW_FUNCTION_RESTORE_PARAM2(trueresult, int, falseresult, int)
{
   int retval = falseresult;
#ifdef GDK_WINDOWING_X11
   GdkDisplay *display = gdk_display_get_default();

   if(display && GDK_IS_X11_DISPLAY(display))
   {
      retval = trueresult;
   }
#endif
   DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Gets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_FEATURE_DISABLED if the feature is supported but disabled.
 *       DW_FEATURE_ENABLED if the feature is supported and enabled.
 *       Other value greater than 1, same as enabled but with extra info.
 */
int API dw_feature_get(DWFEATURE feature)
{
    switch(feature)
    {
#ifdef USE_WEBKIT
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
#endif
        case DW_FEATURE_NOTIFICATION:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_TREE:
        case DW_FEATURE_RENDER_SAFE:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_WINDOW_PLACEMENT:
            return dw_x11_check(DW_FEATURE_ENABLED, DW_FEATURE_UNSUPPORTED);
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}

/*
 * Sets the state of the requested library feature.
 * Parameters:
 *       feature: The requested feature for example DW_FEATURE_DARK_MODE
 *       state: DW_FEATURE_DISABLED, DW_FEATURE_ENABLED or any value greater than 1.
 * Returns:
 *       DW_FEATURE_UNSUPPORTED if the library or OS does not support the feature.
 *       DW_ERROR_NONE if the feature is supported and successfully configured.
 *       DW_ERROR_GENERAL if the feature is supported but could not be configured.
 * Remarks: 
 *       These settings are typically used during dw_init() so issue before 
 *       setting up the library with dw_init().
 */
int API dw_feature_set(DWFEATURE feature, int state)
{
    switch(feature)
    {
        /* These features are supported but not configurable */
#ifdef USE_WEBKIT
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
        case DW_FEATURE_HTML_MESSAGE:
#endif
        case DW_FEATURE_NOTIFICATION:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_TREE:
        case DW_FEATURE_RENDER_SAFE:
            return DW_ERROR_GENERAL;
        case DW_FEATURE_WINDOW_PLACEMENT:
            return dw_x11_check(DW_ERROR_GENERAL, DW_FEATURE_UNSUPPORTED);
        /* These features are supported and configurable */
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}


