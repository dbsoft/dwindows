/*
 * Dynamic Windows:
 *          A GTK like implementation of the MacOS GUI using Cocoa
 *
 * (C) 2011-2023 Brian Smith <brian@dbsoft.org>
 * (C) 2011-2021 Mark Hessling <mark@rexx.org>
 *
 * Requires 10.5 or later.
 * clang -std=c99 -g -o dwtest -D__MAC__ -I. dwtest.c mac/dw.m -framework Cocoa -framework WebKit
 */
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "dw.h"
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>

/* Create a define to let us know to include Snow Leopard specific features */
#if defined(MAC_OS_X_VERSION_10_6) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWPasteboardTypeString NSPasteboardTypeString
#define BUILDING_FOR_SNOW_LEOPARD
#else
#define DWPasteboardTypeString NSStringPboardType
#endif

/* Create a define to let us know to include Lion specific features */
#if defined(MAC_OS_X_VERSION_10_7) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_LION
#define DW_USE_NSVIEW
#endif

/* Create a define to let us know to include Mountain Lion specific features */
#if defined(MAC_OS_X_VERSION_10_8) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_MOUNTAIN_LION
#endif

/* Macros to handle local auto-release pools */
#define DW_LOCAL_POOL_IN NSAutoreleasePool *localpool = nil; \
        if(DWThread != (DWTID)-1 && pthread_self() != DWThread) \
            localpool = [[NSAutoreleasePool alloc] init];
#define DW_LOCAL_POOL_OUT if(localpool) [localpool drain];

/* Handle deprecation of several constants in 10.10...
 * the replacements are not available in earlier versions.
 */
#if defined(MAC_OS_X_VERSION_10_10) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_10) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWModalResponseOK NSModalResponseOK
#define DWModalResponseCancel NSModalResponseCancel
#define DWPaperOrientationPortrait NSPaperOrientationPortrait
#define DWCalendarUnitDay NSCalendarUnitDay
#define DWCalendarUnitMonth NSCalendarUnitMonth
#define DWCalendarUnitYear NSCalendarUnitYear
#define DWCalendarIdentifierGregorian NSCalendarIdentifierGregorian
#define BUILDING_FOR_YOSEMITE
#else
#define DWModalResponseOK NSOKButton
#define DWModalResponseCancel NSCancelButton
#define DWPaperOrientationPortrait NSPortraitOrientation
#define DWCalendarUnitDay NSDayCalendarUnit
#define DWCalendarUnitMonth NSMonthCalendarUnit
#define DWCalendarUnitYear NSYearCalendarUnit
#define DWCalendarIdentifierGregorian (NSString *)kCFGregorianCalendar
#endif

/* Handle deprecation of several constants in 10.12...
 * the replacements are not available in earlier versions.
 */
#if defined(MAC_OS_X_VERSION_10_12) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWButtonTypeSwitch NSButtonTypeSwitch
#define DWButtonTypeRadio NSButtonTypeRadio
#define DWButtonTypeMomentaryPushIn NSButtonTypeMomentaryPushIn
#define DWBezelStyleRegularSquare NSBezelStyleRegularSquare
#define DWBezelStyleRounded NSBezelStyleRounded
#define DWEventTypeApplicationDefined NSEventTypeApplicationDefined
#define DWCompositingOperationSourceOver NSCompositingOperationSourceOver
#define DWEventTypeKeyDown NSEventTypeKeyDown
#define DWEventModifierFlagControl NSEventModifierFlagControl
#define DWEventModifierFlagCommand NSEventModifierFlagCommand
#define DWEventModifierFlagOption NSEventModifierFlagOption
#define DWEventTypeRightMouseDown NSEventTypeRightMouseDown
#define DWEventTypeRightMouseUp NSEventTypeRightMouseUp
#define DWEventTypeLeftMouseDown NSEventTypeLeftMouseDown
#define DWEventTypeLeftMouseUp NSEventTypeLeftMouseUp
#define DWEventTypeOtherMouseDown NSEventTypeOtherMouseDown
#define DWEventTypeOtherMouseUp NSEventTypeOtherMouseUp
#define DWEventModifierFlagDeviceIndependentFlagsMask NSEventModifierFlagDeviceIndependentFlagsMask
#define DWEventMaskAny NSEventMaskAny
#define DWAlertStyleCritical NSAlertStyleCritical
#define DWAlertStyleInformational NSAlertStyleInformational
#define DWAlertStyleWarning NSAlertStyleWarning
#define DWTextAlignmentCenter NSTextAlignmentCenter
#define DWTextAlignmentRight NSTextAlignmentRight
#define DWTextAlignmentLeft NSTextAlignmentLeft
#define DWEventMaskLeftMouseUp NSEventMaskLeftMouseUp
#define DWEventMaskLeftMouseDown NSEventMaskLeftMouseDown
#define DWEventMaskRightMouseUp NSEventMaskRightMouseUp
#define DWEventMaskRightMouseDown NSEventMaskRightMouseDown
#define DWWindowStyleMaskResizable NSWindowStyleMaskResizable
#define BUILDING_FOR_SIERRA
#else
#define DWButtonTypeSwitch NSSwitchButton
#define DWButtonTypeRadio NSRadioButton
#define DWButtonTypeMomentaryPushIn NSMomentaryPushInButton
#define DWBezelStyleRegularSquare NSRegularSquareBezelStyle
#define DWBezelStyleRounded NSRoundedBezelStyle
#define DWEventTypeApplicationDefined NSApplicationDefined
#define DWCompositingOperationSourceOver NSCompositeSourceOver
#define DWEventTypeKeyDown NSKeyDown
#define DWEventModifierFlagControl NSControlKeyMask
#define DWEventModifierFlagCommand NSCommandKeyMask
#define DWEventModifierFlagOption NSAlternateKeyMask
#define DWEventTypeRightMouseDown NSRightMouseDown
#define DWEventTypeRightMouseUp NSRightMouseUp
#define DWEventTypeLeftMouseDown NSLeftMouseDown
#define DWEventTypeLeftMouseUp NSLeftMouseUp
#define DWEventTypeOtherMouseDown NSOtherMouseDown
#define DWEventTypeOtherMouseUp NSOtherMouseUp
#define DWEventModifierFlagDeviceIndependentFlagsMask NSDeviceIndependentModifierFlagsMask
#define DWEventMaskAny NSAnyEventMask
#define DWAlertStyleCritical NSCriticalAlertStyle
#define DWAlertStyleInformational NSInformationalAlertStyle
#define DWAlertStyleWarning NSWarningAlertStyle
#define DWTextAlignmentCenter NSCenterTextAlignment
#define DWTextAlignmentRight NSRightTextAlignment
#define DWTextAlignmentLeft NSLeftTextAlignment
#define DWEventMaskLeftMouseUp NSLeftMouseUpMask
#define DWEventMaskLeftMouseDown NSLeftMouseDownMask
#define DWEventMaskRightMouseUp NSRightMouseUpMask
#define DWEventMaskRightMouseDown NSRightMouseDownMask
#define DWWindowStyleMaskResizable NSResizableWindowMask
#endif

/* Handle deprecation of several constants in 10.13...
 * the replacements are not available in earlier versions.
 */
#if defined(MAC_OS_X_VERSION_10_13) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_13) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWControlStateValueOff NSControlStateValueOff
#define DWControlStateValueOn NSControlStateValueOn
#define BUILDING_FOR_HIGH_SIERRA
#else
#define DWControlStateValueOff NSOffState
#define DWControlStateValueOn NSOnState
#endif

/* Handle deprecation of several constants in 10.14...
 * the replacements are not available in earlier versions.
 */
#if defined(MAC_OS_X_VERSION_10_14) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_14) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define DWProgressIndicatorStyleBar NSProgressIndicatorStyleBar
#define BUILDING_FOR_MOJAVE
#import <UserNotifications/UserNotifications.h>
#else
#define DWProgressIndicatorStyleBar NSProgressIndicatorBarStyle
#endif

/* Handle deprecation of constants in 10.15... */
#if defined(MAC_OS_X_VERSION_10_15) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_15) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_CATALINA
#define WK_API_ENABLED 1
#endif

/* Handle deprecation of constants in 11.0 (also known as 10.16)... */
#if (defined(MAC_OS_VERSION_11_0) || defined(MAC_OS_X_VERSION_10_16)) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_10_16) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#import <UniformTypeIdentifiers/UTDefines.h>
#import <UniformTypeIdentifiers/UTType.h>
#import <UniformTypeIdentifiers/UTCoreTypes.h>
#define DWPrintingPaginationModeFit NSPrintingPaginationModeFit
#define DWDatePickerModeSingle NSDatePickerModeSingle
#define DWDatePickerStyleClockAndCalendar NSDatePickerStyleClockAndCalendar
#define DWDatePickerElementFlagYearMonthDay NSDatePickerElementFlagYearMonthDay
#define BUILDING_FOR_BIG_SUR
#else
#define DWPrintingPaginationModeFit NSFitPagination
#define DWDatePickerModeSingle NSSingleDateMode
#define DWDatePickerStyleClockAndCalendar NSClockAndCalendarDatePickerStyle
#define DWDatePickerElementFlagYearMonthDay NSYearMonthDayDatePickerElementFlag
#endif

/* Handle deprecation of constants in 12.0 */
#if defined(MAC_OS_VERSION_12_0) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_12_0) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_MONTEREY
#endif

/* Handle deprecation of constants in 13.0 */
#if defined(MAC_OS_VERSION_13_0) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_13_0) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_VENTURA
#endif

/* Handle deprecation of constants in 14.0 */
#if defined(MAC_OS_VERSION_14_0) && ((defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_14_0) || !defined(MAC_OS_X_VERSION_MAX_ALLOWED))
#define BUILDING_FOR_SONOMA
#endif

#ifdef __clang__
#define _DW_ELSE_AVAILABLE  \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

#define _DW_END_AVAILABLE _Pragma("clang diagnostic pop")
#else
#define _DW_ELSE_AVAILABLE
#define _DW_END_AVAILABLE
#endif

/* Macros to encapsulate running functions on the main thread
 * on Mojave or later... and locking mutexes on earlier versions.
 */
#ifdef BUILDING_FOR_MOJAVE
#define DW_FUNCTION_INIT
#define DW_FUNCTION_DEFINITION(func, rettype, ...)  void _##func(NSPointerArray *_args); \
rettype API func(__VA_ARGS__) { \
    DW_LOCAL_POOL_IN; \
    NSPointerArray *_args = [[NSPointerArray alloc] initWithOptions:NSPointerFunctionsOpaqueMemory]; \
    [_args addPointer:(void *)_##func];
#define DW_FUNCTION_ADD_PARAM1(param1) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM2(param1, param2) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM3(param1, param2, param3) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM4(param1, param2, param3, param4) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM5(param1, param2, param3, param4, param5) [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM6(param1, param2, param3, param4, param5, param6) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM7(param1, param2, param3, param4, param5, param6, param7) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM8(param1, param2, param3, param4, param5, param6, param7, param8) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)&param8]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_ADD_PARAM9(param1, param2, param3, param4, param5, param6, param7, param8, param9) \
    [_args addPointer:(void *)&param1]; \
    [_args addPointer:(void *)&param2]; \
    [_args addPointer:(void *)&param3]; \
    [_args addPointer:(void *)&param4]; \
    [_args addPointer:(void *)&param5]; \
    [_args addPointer:(void *)&param6]; \
    [_args addPointer:(void *)&param7]; \
    [_args addPointer:(void *)&param8]; \
    [_args addPointer:(void *)&param9]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_fg_color_key)]; \
    [_args addPointer:(void *)pthread_getspecific(_dw_bg_color_key)];
#define DW_FUNCTION_RESTORE_PARAM1(param1, vartype1) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:2]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:3];
#define DW_FUNCTION_RESTORE_PARAM2(param1, vartype1, param2, vartype2) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:3]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:4];
#define DW_FUNCTION_RESTORE_PARAM3(param1, vartype1, param2, vartype2, param3, vartype3) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:4]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:5];
#define DW_FUNCTION_RESTORE_PARAM4(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:5]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:6];
#define DW_FUNCTION_RESTORE_PARAM5(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:6]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:7];
#define DW_FUNCTION_RESTORE_PARAM6(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:7]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:8];
#define DW_FUNCTION_RESTORE_PARAM7(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:8]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:9];
#define DW_FUNCTION_RESTORE_PARAM8(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    vartype8 param8 = *((vartype8 *)[_args pointerAtIndex:8]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:9]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:10];
#define DW_FUNCTION_RESTORE_PARAM9(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9) \
    vartype1 param1 = *((vartype1 *)[_args pointerAtIndex:1]); \
    vartype2 param2 = *((vartype2 *)[_args pointerAtIndex:2]); \
    vartype3 param3 = *((vartype3 *)[_args pointerAtIndex:3]); \
    vartype4 param4 = *((vartype4 *)[_args pointerAtIndex:4]); \
    vartype5 param5 = *((vartype5 *)[_args pointerAtIndex:5]); \
    vartype6 param6 = *((vartype6 *)[_args pointerAtIndex:6]); \
    vartype7 param7 = *((vartype7 *)[_args pointerAtIndex:7]); \
    vartype8 param8 = *((vartype8 *)[_args pointerAtIndex:8]); \
    vartype9 param9 = *((vartype9 *)[_args pointerAtIndex:9]); \
    NSColor * DW_UNUSED(_dw_fg_color) = (NSColor *)[_args pointerAtIndex:10]; \
    NSColor * DW_UNUSED(_dw_bg_color) = (NSColor *)[_args pointerAtIndex:11];
#define DW_FUNCTION_END }
#define DW_FUNCTION_NO_RETURN(func) [DWObj safeCall:@selector(callBack:) withObject:_args]; \
    [_args release]; \
    DW_LOCAL_POOL_OUT; } \
void _##func(NSPointerArray *_args) {
#define DW_FUNCTION_RETURN(func, rettype) [DWObj safeCall:@selector(callBack:) withObject:_args]; {\
        void *tmp = [_args pointerAtIndex:[_args count]-1]; \
        rettype myreturn = *((rettype *)tmp); \
        free(tmp); \
        [_args release]; \
        DW_LOCAL_POOL_OUT; \
        return myreturn; }} \
void _##func(NSPointerArray *_args) {
#define DW_FUNCTION_RETURN_THIS(_retvar) { void *_myreturn = malloc(sizeof(_retvar)); \
    memcpy(_myreturn, (void *)&_retvar, sizeof(_retvar)); \
    [_args addPointer:_myreturn]; }}
#define DW_FUNCTION_RETURN_NOTHING }
#else
/* Macros to protect access to thread unsafe classes */
#define DW_FUNCTION_INIT int _locked_by_me = FALSE; { \
    if(DWThread != (DWTID)-1 && pthread_self() != DWThread && pthread_self() != _dw_mutex_locked) { \
        dw_mutex_lock(DWThreadMutex); \
        _dw_mutex_locked = pthread_self(); \
        dw_mutex_lock(DWThreadMutex2); \
        [DWObj performSelectorOnMainThread:@selector(synchronizeThread:) withObject:nil waitUntilDone:NO]; \
        dw_mutex_lock(DWRunMutex); \
        _locked_by_me = TRUE; } }
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
#define DW_FUNCTION_RESTORE_PARAM1(param1, vartype1)
#define DW_FUNCTION_RESTORE_PARAM2(param1, vartype1, param2, vartype2)
#define DW_FUNCTION_RESTORE_PARAM3(param1, vartype1, param2, vartype2, param3, vartype3)
#define DW_FUNCTION_RESTORE_PARAM4(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4)
#define DW_FUNCTION_RESTORE_PARAM5(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5)
#define DW_FUNCTION_RESTORE_PARAM6(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6)
#define DW_FUNCTION_RESTORE_PARAM7(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7)
#define DW_FUNCTION_RESTORE_PARAM8(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8)
#define DW_FUNCTION_RESTORE_PARAM9(param1, vartype1, param2, vartype2, param3, vartype3, param4, vartype4, param5, vartype5, param6, vartype6, param7, vartype7, param8, vartype8, param9, vartype9)
#define DW_FUNCTION_END
#define DW_FUNCTION_NO_RETURN(func)
#define DW_FUNCTION_RETURN(func, rettype)
#define DW_FUNCTION_RETURN_THIS(retvar) { \
    if(pthread_self() != DWThread && _locked_by_me == TRUE) { \
        dw_mutex_unlock(DWRunMutex); \
        dw_mutex_unlock(DWThreadMutex2); \
        _dw_mutex_locked = (pthread_t)-1; \
        dw_mutex_unlock(DWThreadMutex); \
        _locked_by_me = FALSE; } } \
    return retvar;
#define DW_FUNCTION_RETURN_NOTHING { \
    if(pthread_self() != DWThread && _locked_by_me == TRUE) { \
        dw_mutex_unlock(DWRunMutex); \
        dw_mutex_unlock(DWThreadMutex2); \
        _dw_mutex_locked = (pthread_t)-1; \
        dw_mutex_unlock(DWThreadMutex); \
        _locked_by_me = FALSE; } }
HMTX DWRunMutex;
HMTX DWThreadMutex;
HMTX DWThreadMutex2;
DWTID _dw_mutex_locked = (DWTID)-1;
#endif

unsigned long _dw_colors[] =
{
    0x00000000,   /* 0  black */
    0x000000bb,   /* 1  red */
    0x0000bb00,   /* 2  green */
    0x0000aaaa,   /* 3  yellow */
    0x00cc0000,   /* 4  blue */
    0x00bb00bb,   /* 5  magenta */
    0x00bbbb00,   /* 6  cyan */
    0x00bbbbbb,   /* 7  white */
    0x00777777,   /* 8  grey */
    0x000000ff,   /* 9  bright red */
    0x0000ff00,   /* 10 bright green */
    0x0000eeee,   /* 11 bright yellow */
    0x00ff0000,   /* 12 bright blue */
    0x00ff00ff,   /* 13 bright magenta */
    0x00eeee00,   /* 14 bright cyan */
    0x00ffffff,   /* 15 bright white */
    0xff000000    /* 16 default color */
};

/*
 * List those icons that have transparency first
 */
#define NUM_EXTS 8
char *_dw_image_exts[NUM_EXTS+1] =
{
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

char *_dw_get_image_extension(const char *filename)
{
   char *file = alloca(strlen(filename) + 6);
   int found_ext = 0,i;

   /* Try various extentions */
   for ( i = 0; i < NUM_EXTS; i++ )
   {
      strcpy( file, filename );
      strcat( file, _dw_image_exts[i] );
      if ( access( file, R_OK ) == 0 )
      {
         found_ext = 1;
         break;
      }
   }
   if ( found_ext == 1 )
   {
      return _dw_image_exts[i];
   }
   return NULL;
}

/* Return the RGB color regardless if a predefined color was passed */
unsigned long _dw_get_color(unsigned long thiscolor)
{
    if(thiscolor & DW_RGB_COLOR)
    {
        return thiscolor & ~DW_RGB_COLOR;
    }
    else if(thiscolor < 17)
    {
        return _dw_colors[thiscolor];
    }
    return 0;
}

/* Returns TRUE if Mojave or later is in Dark Mode */
BOOL _dw_is_dark(id object)
{
#ifdef BUILDING_FOR_MOJAVE
    NSAppearance *appearance = [object effectiveAppearance];

    if(@available(macOS 10.14, *))
    {
        NSAppearanceName basicAppearance = [appearance bestMatchFromAppearancesWithNames:@[
                                                                                           NSAppearanceNameAqua,
                                                                                           NSAppearanceNameDarkAqua]];
        return [basicAppearance isEqualToString:NSAppearanceNameDarkAqua];
    }
#endif
    return NO;
}

/* Thread specific storage */
#if !defined(GARBAGE_COLLECT)
pthread_key_t _dw_pool_key;
#endif
pthread_key_t _dw_fg_color_key;
pthread_key_t _dw_bg_color_key;
static int DWOSMajor, DWOSMinor, DWOSBuild;
static char _dw_bundle_path[PATH_MAX+1] = { 0 };
static char _dw_app_id[_DW_APP_ID_SIZE+1]= {0};
static int _dw_render_safe_mode = DW_FEATURE_DISABLED;
static id _dw_render_expose = nil;

/* Return TRUE if it is safe to draw on the window handle.
 * Either we are in unsafe mode, or we are in an EXPOSE
 * event for the requested render window handle.
 */
int _dw_render_safe_check(id handle)
{
    if(_dw_render_safe_mode == DW_FEATURE_DISABLED || 
       (handle && _dw_render_expose == handle))
           return TRUE;
    return FALSE;
}

int _dw_is_render(id handle);

/* Create a default colors for a thread */
void _dw_init_colors(void)
{
    NSColor *fgcolor = [[NSColor grayColor] retain];
    pthread_setspecific(_dw_fg_color_key, fgcolor);
    pthread_setspecific(_dw_bg_color_key, NULL);
}

typedef struct _dwsighandler
{
    struct _dwsighandler   *next;
    ULONG message;
    HWND window;
    int id;
    void *signalfunction;
    void *discfunction;
    void *data;

} DWSignalHandler;

static DWSignalHandler *DWRoot = NULL;

/* Some internal prototypes */
static void _dw_do_resize(Box *thisbox, int x, int y);
void _dw_handle_resize_events(Box *thisbox);
int _dw_remove_userdata(UserData **root, const char *varname, int all);
int _dw_main_iteration(NSDate *date);
NSGraphicsContext *_dw_draw_context(NSBitmapImageRep *image);
typedef id (*DWIMP)(id, SEL, ...);

/* Internal function to queue a window redraw */
void _dw_redraw(id window, int skip)
{
    static id lastwindow = nil;
    id redraw = lastwindow;

    if(skip && window == nil)
        return;

    lastwindow = window;
    if(redraw != lastwindow && redraw != nil)
    {
        dw_window_redraw(redraw);
    }
}

DWSignalHandler *_dw_get_handler(HWND window, int messageid)
{
    DWSignalHandler *tmp = DWRoot;

    /* Find any callbacks for this function */
    while(tmp)
    {
        if(tmp->message == messageid && window == tmp->window)
        {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

typedef struct
{
    ULONG message;
    char name[30];

} DWSignalList;

/* List of signals */
static DWSignalList DWSignalTranslate[] = {
    { _DW_EVENT_CONFIGURE,       DW_SIGNAL_CONFIGURE },
    { _DW_EVENT_KEY_PRESS,       DW_SIGNAL_KEY_PRESS },
    { _DW_EVENT_BUTTON_PRESS,    DW_SIGNAL_BUTTON_PRESS },
    { _DW_EVENT_BUTTON_RELEASE,  DW_SIGNAL_BUTTON_RELEASE },
    { _DW_EVENT_MOTION_NOTIFY,   DW_SIGNAL_MOTION_NOTIFY },
    { _DW_EVENT_DELETE,          DW_SIGNAL_DELETE },
    { _DW_EVENT_EXPOSE,          DW_SIGNAL_EXPOSE },
    { _DW_EVENT_CLICKED,         DW_SIGNAL_CLICKED },
    { _DW_EVENT_ITEM_ENTER,      DW_SIGNAL_ITEM_ENTER },
    { _DW_EVENT_ITEM_CONTEXT,    DW_SIGNAL_ITEM_CONTEXT },
    { _DW_EVENT_LIST_SELECT,     DW_SIGNAL_LIST_SELECT },
    { _DW_EVENT_ITEM_SELECT,     DW_SIGNAL_ITEM_SELECT },
    { _DW_EVENT_SET_FOCUS,       DW_SIGNAL_SET_FOCUS },
    { _DW_EVENT_VALUE_CHANGED,   DW_SIGNAL_VALUE_CHANGED },
    { _DW_EVENT_SWITCH_PAGE,     DW_SIGNAL_SWITCH_PAGE },
    { _DW_EVENT_TREE_EXPAND,     DW_SIGNAL_TREE_EXPAND },
    { _DW_EVENT_COLUMN_CLICK,    DW_SIGNAL_COLUMN_CLICK },
    { _DW_EVENT_HTML_RESULT,     DW_SIGNAL_HTML_RESULT },
    { _DW_EVENT_HTML_CHANGED,    DW_SIGNAL_HTML_CHANGED },
    { _DW_EVENT_HTML_MESSAGE,    DW_SIGNAL_HTML_MESSAGE }
};

int _dw_event_handler1(id object, NSEvent *event, int message)
{
    DWSignalHandler *handler = _dw_get_handler(object, message);
    /* NSLog(@"Event handler - type %d\n", message); */

    if(handler)
    {
        switch(message)
        {
            /* Timer event */
            case _DW_EVENT_TIMER:
            {
                int (* API timerfunc)(void *) = (int (* API)(void *))handler->signalfunction;

                if(!timerfunc(handler->data))
                    dw_timer_disconnect(handler->id);
                return 0;
            }
            /* Configure/Resize event */
            case _DW_EVENT_CONFIGURE:
            {
                int (*sizefunc)(HWND, int, int, void *) = handler->signalfunction;
                NSSize size;

                if([object isKindOfClass:[NSWindow class]])
                {
                    NSWindow *window = object;
                    size = [[window contentView] frame].size;
                }
                else
                {
                    NSView *view = object;
                    size = [view frame].size;
                }

                if(size.width > 0 && size.height > 0)
                {
                    return sizefunc(object, size.width, size.height, handler->data);
                }
                return 0;
            }
            case _DW_EVENT_KEY_PRESS:
            {
                int (*keypressfunc)(HWND, char, int, int, void *, char *) = handler->signalfunction;
                NSString *nchar = [event charactersIgnoringModifiers];
                int special = (int)[event modifierFlags];
                unichar vk = [nchar characterAtIndex:0];
                char *utf8 = NULL, ch = '\0';

                /* Handle a valid key */
                if([nchar length] == 1)
                {
                    char *tmp = (char *)[nchar UTF8String];
                    if(tmp && strlen(tmp) == 1)
                    {
                        ch = tmp[0];
                    }
                    utf8 = tmp;
                }

                return keypressfunc(handler->window, ch, (int)vk, special, handler->data, utf8);
            }
            /* Button press and release event */
            case _DW_EVENT_BUTTON_PRESS:
            case _DW_EVENT_BUTTON_RELEASE:
            {
                int (* API buttonfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                NSPoint p = [NSEvent mouseLocation];
                int button = DW_BUTTON1_MASK;

                if([event isMemberOfClass:[NSEvent class]])
                {
                    id view = [[[event window] contentView] superview];
                    NSEventType type = [event type];

                    p = [view convertPoint:[event locationInWindow] toView:object];

                    if(type == DWEventTypeRightMouseDown || type == DWEventTypeRightMouseUp)
                    {
                        button = DW_BUTTON2_MASK;
                    }
                    else if(type == DWEventTypeOtherMouseDown || type == DWEventTypeOtherMouseUp)
                    {
                        button = DW_BUTTON3_MASK;
                    }
                    else if([event modifierFlags] & DWEventModifierFlagControl)
                    {
                        button = DW_BUTTON2_MASK;
                    }
                }

                return buttonfunc(object, (int)p.x, (int)p.y, button, handler->data);
            }
            /* Motion notify event */
            case _DW_EVENT_MOTION_NOTIFY:
            {
                int (* API motionfunc)(HWND, int, int, int, void *) = (int (* API)(HWND, int, int, int, void *))handler->signalfunction;
                id view = [[[event window] contentView] superview];
                NSPoint p = [view convertPoint:[event locationInWindow] toView:object];
                SEL spmb = NSSelectorFromString(@"pressedMouseButtons");
                DWIMP ipmb = [[NSEvent class] respondsToSelector:spmb] ? (DWIMP)[[NSEvent class] methodForSelector:spmb] : 0;
                NSUInteger buttonmask = ipmb ? (NSUInteger)ipmb([NSEvent class], spmb) : (1 << [event buttonNumber]);

                return motionfunc(object, (int)p.x, (int)p.y, (int)buttonmask, handler->data);
            }
            /* Window close event */
            case _DW_EVENT_DELETE:
            {
                int (* API closefunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;
                return closefunc(object, handler->data);
            }
            /* Window expose/draw event */
            case _DW_EVENT_EXPOSE:
            {
                DWExpose exp;
                int (* API exposefunc)(HWND, DWExpose *, void *) = (int (* API)(HWND, DWExpose *, void *))handler->signalfunction;
                NSRect rect = [object frame];
                id oldrender = _dw_render_expose;

                exp.x = rect.origin.x;
                exp.y = rect.origin.y;
                exp.width = rect.size.width;
                exp.height = rect.size.height;
                /* The render expose is only valid for render class windows */
                if(_dw_render_safe_mode == DW_FEATURE_ENABLED && _dw_is_render(object))
                    _dw_render_expose = object;
                int result = exposefunc(object, &exp, handler->data);
                _dw_render_expose = oldrender;
#ifndef BUILDING_FOR_MOJAVE
                [[object window] flushWindow];
#endif
                return result;
            }
            /* Clicked event for buttons and menu items */
            case _DW_EVENT_CLICKED:
            {
                int (* API clickfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return clickfunc(object, handler->data);
            }
            /* Container class selection event */
            case _DW_EVENT_ITEM_ENTER:
            {
                int (*containerselectfunc)(HWND, char *, void *, void *) = handler->signalfunction;
                void **params = (void **)event;

                return containerselectfunc(handler->window, params[0], handler->data, params[1]);
            }
            /* Container context menu event */
            case _DW_EVENT_ITEM_CONTEXT:
            {
                int (* API containercontextfunc)(HWND, char *, int, int, void *, void *) = (int (* API)(HWND, char *, int, int, void *, void *))handler->signalfunction;
                char *text = NULL;
                void *user = NULL;
                LONG x,y;

                /* Fill in both items for the tree */
                if([object isKindOfClass:[NSOutlineView class]])
                {
                    id item = event;
                    NSString *nstr = [item objectAtIndex:1];
                    text = (char *)[nstr UTF8String];
                    NSValue *value = [item objectAtIndex:2];
                    if(value && [value isKindOfClass:[NSValue class]])
                    {
                        user = [value pointerValue];
                    }
                }
                else
                {
                    void **params = (void **)event;

                    text = params[0];
                    user = params[1];
                }

                dw_pointer_query_pos(&x, &y);

                return containercontextfunc(handler->window, text, (int)x, (int)y, handler->data, user);
            }
            /* Generic selection changed event for several classes */
            case _DW_EVENT_LIST_SELECT:
            case _DW_EVENT_VALUE_CHANGED:
            {
                int (* API valuechangedfunc)(HWND, int, void *) = (int (* API)(HWND, int, void *))handler->signalfunction;
                int selected = DW_POINTER_TO_INT(event);

                return valuechangedfunc(handler->window, selected, handler->data);;
            }
            /* Tree class selection event */
            case _DW_EVENT_ITEM_SELECT:
            {
                int (* API treeselectfunc)(HWND, HTREEITEM, char *, void *, void *) = (int (* API)(HWND, HTREEITEM, char *, void *, void *))handler->signalfunction;
                char *text = NULL;
                void *user = NULL;
                id item = nil;

                if([object isKindOfClass:[NSOutlineView class]])
                {
                    item = (id)event;
                    NSString *nstr = [item objectAtIndex:1];

                    if(nstr)
                    {
                        text = strdup([nstr UTF8String]);
                    }

                    NSValue *value = [item objectAtIndex:2];
                    if(value && [value isKindOfClass:[NSValue class]])
                    {
                        user = [value pointerValue];
                    }
                    int result = treeselectfunc(handler->window, item, text, handler->data, user);
                    if(text)
                    {
                        free(text);
                    }
                    return result;
                }
                else if(event)
                {
                    void **params = (void **)event;

                    text = params[0];
                    user = params[1];
                }

                return treeselectfunc(handler->window, item, text, handler->data, user);
            }
            /* Set Focus event */
            case _DW_EVENT_SET_FOCUS:
            {
                int (* API setfocusfunc)(HWND, void *) = (int (* API)(HWND, void *))handler->signalfunction;

                return setfocusfunc(handler->window, handler->data);
            }
            /* Notebook page change event */
            case _DW_EVENT_SWITCH_PAGE:
            {
                int (* API switchpagefunc)(HWND, unsigned long, void *) = (int (* API)(HWND, unsigned long, void *))handler->signalfunction;
                int pageid = DW_POINTER_TO_INT(event);

                return switchpagefunc(handler->window, pageid, handler->data);
            }
            /* Tree expand event */
            case _DW_EVENT_TREE_EXPAND:
            {
                int (* API treeexpandfunc)(HWND, HTREEITEM, void *) = (int (* API)(HWND, HTREEITEM, void *))handler->signalfunction;

                return treeexpandfunc(handler->window, (HTREEITEM)event, handler->data);
            }
            /* Column click event */
            case _DW_EVENT_COLUMN_CLICK:
            {
                int (* API clickcolumnfunc)(HWND, int, void *) = handler->signalfunction;
                int column_num = DW_POINTER_TO_INT(event);

                return clickcolumnfunc(handler->window, column_num, handler->data);
            }
            /* HTML result event */
            case _DW_EVENT_HTML_RESULT:
            {
                int (* API htmlresultfunc)(HWND, int, char *, void *, void *) = handler->signalfunction;
                void **params = (void **)event;
                NSString *result = params[0];

                return htmlresultfunc(handler->window, [result length] ? DW_ERROR_NONE : DW_ERROR_UNKNOWN, [result length] ? 
                                      (char *)[result UTF8String] : NULL, params[1], handler->data);
            }
            /* HTML changed event */
            case _DW_EVENT_HTML_CHANGED:
            {
                int (* API htmlchangedfunc)(HWND, int, char *, void *) = handler->signalfunction;
                void **params = (void **)event;
                NSString *uri = params[1];

                return htmlchangedfunc(handler->window, DW_POINTER_TO_INT(params[0]), (char *)[uri UTF8String], handler->data);
            }
#if WK_API_ENABLED
            /* HTML message event */
            case _DW_EVENT_HTML_MESSAGE:
            {
                int (* API htmlmessagefunc)(HWND, char *, char *, void *) = handler->signalfunction;
                WKScriptMessage *message = (WKScriptMessage *)event;

                return htmlmessagefunc(handler->window, (char *)[[message name] UTF8String], [[message body] isKindOfClass:[NSString class]] ?
                                       (char *)[[message body] UTF8String] : NULL, handler->data);
            }
#endif
        }
    }
    return -1;
}

/* Sub function to handle redraws */
int _dw_event_handler(id object, NSEvent *event, int message)
{
    int ret = _dw_event_handler1(object, event, message);
    if(ret != -1)
        _dw_redraw(nil, FALSE);
    return ret;
}

/* Subclass for the Timer type */
@interface DWTimerHandler : NSObject { }
-(void)runTimer:(id)sender;
@end

@implementation DWTimerHandler
-(void)runTimer:(id)sender { _dw_event_handler(sender, nil, _DW_EVENT_TIMER); }
@end

static NSApplication *DWApp = nil;
static NSFontManager *DWFontManager = nil;
static NSMenu *DWMainMenu;
static NSFont *DWDefaultFont;
static DWTimerHandler *DWHandler;
#ifdef BUILDING_FOR_MOJAVE
static NSMutableArray *_DWDirtyDrawables;
#else
static HWND _DWLastDrawable;
#endif
static DWTID DWThread = (DWTID)-1;

/* Send fake event to make sure the loop isn't stuck */
void _dw_wakeup_app()
{
    [DWApp postEvent:[NSEvent otherEventWithType:DWEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:NULL
                                         subtype:0
                                           data1:0
                                           data2:0]
             atStart:NO];
}

/* Used for doing bitblts from the main thread */
typedef struct _bitbltinfo
{
    id src;
    id dest;
    int xdest;
    int ydest;
    int width;
    int height;
    int xsrc;
    int ysrc;
    int srcwidth;
    int srcheight;
} DWBitBlt;

/* Subclass for a test object type */
@interface DWObject : NSObject {}
-(void)uselessThread:(id)sender;
#ifndef BUILDING_FOR_MOJAVE
-(void)synchronizeThread:(id)param;
#endif
-(void)menuHandler:(id)param;
-(void)doBitBlt:(id)param;
-(void)doFlush:(id)param;
@end

@interface DWMenuItem : NSMenuItem
{
    int check;
    void *userdata;
}
-(void)setCheck:(int)input;
-(int)check;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)dealloc;
@end

/* So basically to implement our event handlers...
 * it looks like we are going to have to subclass
 * basically everything.  Was hoping to add methods
 * to the superclasses but it looks like you can
 * only add methods and no variables, which isn't
 * going to work. -Brian
 */

/* Subclass for a box type */
@interface DWBox : NSView
{
    Box *box;
    void *userdata;
    NSColor *bgcolor;
}
-(id)init;
-(void)dealloc;
-(Box *)box;
-(id)contentView;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)drawRect:(NSRect)rect;
-(BOOL)isFlipped;
-(void)mouseDown:(NSEvent *)theEvent;
-(void)mouseUp:(NSEvent *)theEvent;
-(NSMenu *)menuForEvent:(NSEvent *)theEvent;
-(void)rightMouseUp:(NSEvent *)theEvent;
-(void)otherMouseDown:(NSEvent *)theEvent;
-(void)otherMouseUp:(NSEvent *)theEvent;
-(void)keyDown:(NSEvent *)theEvent;
-(void)setColor:(unsigned long)input;
@end

@implementation DWBox
-(id)init
{
    self = [super init];

    if (self)
    {
        box = calloc(1, sizeof(Box));
        box->type = DW_VERT;
        box->vsize = box->hsize = _DW_SIZE_EXPAND;
        box->width = box->height = 1;
    }
    return self;
}
-(void)dealloc
{
    UserData *root = userdata;
    if(box->items)
        free(box->items);
    free(box);
    _dw_remove_userdata(&root, NULL, TRUE);
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(Box *)box { return box; }
-(id)contentView { return self; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)drawRect:(NSRect)rect
{
    if(bgcolor)
    {
        [bgcolor set];
        NSRectFill([self bounds]);
    }
}
-(BOOL)isFlipped { return YES; }
-(void)mouseDown:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON1_MASK), _DW_EVENT_BUTTON_PRESS); }
-(void)mouseUp:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON1_MASK), _DW_EVENT_BUTTON_RELEASE); }
-(NSMenu *)menuForEvent:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON2_MASK), _DW_EVENT_BUTTON_PRESS); return nil; }
-(void)rightMouseUp:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON2_MASK), _DW_EVENT_BUTTON_RELEASE); }
-(void)otherMouseDown:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON3_MASK), _DW_EVENT_BUTTON_PRESS); }
-(void)otherMouseUp:(NSEvent *)theEvent { _dw_event_handler(self, DW_INT_TO_POINTER(DW_BUTTON3_MASK), _DW_EVENT_BUTTON_RELEASE); }
-(void)keyDown:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_KEY_PRESS); }
-(void)setColor:(unsigned long)input
{
    id orig = bgcolor;

    if(input == _dw_colors[DW_CLR_DEFAULT])
    {
        bgcolor = nil;
    }
    else
    {
        bgcolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(input)/255.0 green: DW_GREEN_VALUE(input)/255.0 blue: DW_BLUE_VALUE(input)/255.0 alpha: 1] retain];
#ifdef BUILDING_FOR_YOSEMITE
        if([[NSGraphicsContext currentContext] CGContext])
#else
        if([[NSGraphicsContext currentContext] graphicsPort])
#endif
        {
            [bgcolor set];
            NSRectFill([self bounds]);
        }
    }
    [self setNeedsDisplay:YES];
    [orig release];
}
@end

/* Subclass for a group box type */
@interface DWGroupBox : NSBox
{
    void *userdata;
    NSColor *bgcolor;
    NSSize borderSize;
}
-(Box *)box;
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWGroupBox
-(Box *)box { return [[self contentView] box]; }
-(void *)userdata { return userdata; }
-(NSSize)borderSize { return borderSize; }
-(NSSize)initBorder
{
    NSSize frameSize = [self frame].size;

    if(frameSize.height < 20 || frameSize.width < 20)
    {
        frameSize.width = frameSize.height = 100;
        [self setFrameSize:frameSize];
    }
    NSSize contentSize = [[self contentView] frame].size;
    NSSize titleSize = [self titleRect].size;

    borderSize.width = 100-contentSize.width;
    borderSize.height = (100-contentSize.height)-titleSize.height;
    return borderSize;
}
-(void)setUserdata:(void *)input { userdata = input; }
@end

@interface DWWindow : NSWindow
{
    int redraw;
    int shown;
}
-(void)sendEvent:(NSEvent *)theEvent;
-(void)keyDown:(NSEvent *)theEvent;
-(void)mouseDragged:(NSEvent *)theEvent;
-(int)redraw;
-(void)setRedraw:(int)val;
-(int)shown;
-(void)setShown:(int)val;
@end

@implementation DWWindow
-(void)sendEvent:(NSEvent *)theEvent
{
   int rcode = -1;
   if([theEvent type] == DWEventTypeKeyDown)
   {
      rcode = _dw_event_handler(self, theEvent, _DW_EVENT_KEY_PRESS);
   }
   if ( rcode != TRUE )
      [super sendEvent:theEvent];
}
-(void)keyDown:(NSEvent *)theEvent { }
-(void)mouseDragged:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_MOTION_NOTIFY); }
-(int)redraw { return redraw; }
-(void)setRedraw:(int)val { redraw = val; }
-(int)shown { return shown; }
-(void)setShown:(int)val { shown = val; }
-(void)dealloc { dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a render area type */
@interface DWRender : NSControl
{
    void *userdata;
    NSFont *font;
    NSSize size;
#ifdef BUILDING_FOR_MOJAVE
    NSBitmapImageRep *cachedDrawingRep;
#endif
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFont:(NSFont *)input;
-(NSFont *)font;
-(void)setSize:(NSSize)input;
-(NSSize)size;
#ifdef BUILDING_FOR_MOJAVE
-(NSBitmapImageRep *)cachedDrawingRep;
#endif
-(void)mouseDown:(NSEvent *)theEvent;
-(void)mouseUp:(NSEvent *)theEvent;
-(NSMenu *)menuForEvent:(NSEvent *)theEvent;
-(void)rightMouseUp:(NSEvent *)theEvent;
-(void)otherMouseDown:(NSEvent *)theEvent;
-(void)otherMouseUp:(NSEvent *)theEvent;
-(void)drawRect:(NSRect)rect;
-(void)keyDown:(NSEvent *)theEvent;
-(BOOL)isFlipped;
@end

@implementation DWRender
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFont:(NSFont *)input { [font release]; font = input; [font retain]; }
-(NSFont *)font { return font; }
-(void)setSize:(NSSize)input {
    size = input;
#ifdef BUILDING_FOR_MOJAVE
    if(cachedDrawingRep)
    {
        NSBitmapImageRep *oldrep = cachedDrawingRep;
        cachedDrawingRep = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
        [cachedDrawingRep retain];
        [oldrep release];
    }
#endif
}
-(NSSize)size { return size; }
#ifdef BUILDING_FOR_MOJAVE
-(NSBitmapImageRep *)cachedDrawingRep {
    if(!cachedDrawingRep)
    {
        cachedDrawingRep = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
        [cachedDrawingRep retain];
    }
    /* Mark this render dirty if something is requesting it to draw */
    if(![_DWDirtyDrawables containsObject:self])
        [_DWDirtyDrawables addObject:self];
    return cachedDrawingRep;
}
#endif
-(void)mouseDown:(NSEvent *)theEvent
{
    if(![theEvent isMemberOfClass:[NSEvent class]] || !([theEvent modifierFlags] & DWEventModifierFlagControl))
        _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_PRESS);
}
-(void)mouseUp:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_RELEASE); }
-(NSMenu *)menuForEvent:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_PRESS); return nil; }
-(void)rightMouseUp:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_RELEASE); }
-(void)otherMouseDown:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_PRESS); }
-(void)otherMouseUp:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_BUTTON_RELEASE); }
-(void)mouseDragged:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_MOTION_NOTIFY); }
-(void)delayedNeedsDisplay { [self setNeedsDisplay:YES]; }
-(void)drawRect:(NSRect)rect {
    _dw_event_handler(self, nil, _DW_EVENT_EXPOSE);
#ifdef BUILDING_FOR_MOJAVE
    if (cachedDrawingRep)
    {
        [cachedDrawingRep drawInRect:self.bounds];
        [_DWDirtyDrawables removeObject:self];
        /* Work around a bug in Mojave 10.14 by delaying the setNeedsDisplay */
        if(DWOSMinor != 14)
            [self setNeedsDisplay:YES];
        else
            [self performSelector:@selector(delayedNeedsDisplay) withObject:nil afterDelay:0];
    }
#endif
}
-(void)keyDown:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_KEY_PRESS); }
-(BOOL)isFlipped { return YES; }
-(void)dealloc {
    UserData *root = userdata;
    _dw_remove_userdata(&root, NULL, TRUE);
    [font release];
    dw_signal_disconnect_by_window(self);
#ifdef BUILDING_FOR_MOJAVE
    [cachedDrawingRep release];
    [_DWDirtyDrawables removeObject:self];
#endif
    [super dealloc];
}
-(BOOL)acceptsFirstResponder { return YES; }
@end

/* So we can check before DWRender is declared */
int _dw_is_render(id handle)
{
    if([handle isMemberOfClass:[DWRender class]])
        return TRUE;
    return FALSE;
}

@implementation DWObject
-(void)uselessThread:(id)sender { /* Thread only to initialize threading */ }
#ifndef BUILDING_FOR_MOJAVE
-(void)synchronizeThread:(id)param
{
    pthread_mutex_unlock(DWRunMutex);
    pthread_mutex_lock(DWThreadMutex2);
    pthread_mutex_unlock(DWThreadMutex2);
    pthread_mutex_lock(DWRunMutex);
}
#endif
-(void)menuHandler:(id)param
{
    DWMenuItem *item = param;
    if([item check])
    {
        if([item state] == DWControlStateValueOn)
            [item setState:DWControlStateValueOff];
        else
            [item setState:DWControlStateValueOn];
    }
    _dw_event_handler(param, nil, _DW_EVENT_CLICKED);
}
#ifdef BUILDING_FOR_MOJAVE
-(void)callBack:(NSPointerArray *)params
{
    void (*mycallback)(NSPointerArray *) = [params pointerAtIndex:0];
    if(mycallback)
        mycallback(params);
}
#endif
-(void)messageBox:(NSMutableArray *)params
{
    NSInteger iResponse;
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[params objectAtIndex:0]];
    [alert setInformativeText:[params objectAtIndex:1]];
    [alert addButtonWithTitle:[params objectAtIndex:3]];
    if([params count] > 4)
        [alert addButtonWithTitle:[params objectAtIndex:4]];
    if([params count] > 5)
        [alert addButtonWithTitle:[params objectAtIndex:5]];
    [alert setAlertStyle:[[params objectAtIndex:2] integerValue]];
    iResponse = [alert runModal];
    [alert release];
    [params addObject:[NSNumber numberWithInteger:iResponse]];
}
-(void)safeCall:(SEL)sel withObject:(id)param
{
    if([self respondsToSelector:sel])
    {
        DWTID curr = pthread_self();

        if(DWThread == (DWTID)-1 || DWThread == curr)
            [self performSelector:sel withObject:param];
        else
            [self performSelectorOnMainThread:sel withObject:param waitUntilDone:YES];
    }
}
-(void)doBitBlt:(id)param
{
    NSValue *bi = (NSValue *)param;
    DWBitBlt *bltinfo = (DWBitBlt *)[bi pointerValue];
    id bltdest = bltinfo->dest;
    id bltsrc = bltinfo->src;

#ifdef BUILDING_FOR_MOJAVE
    if([bltdest isMemberOfClass:[DWRender class]])
    {
        DWRender *render = bltdest;

        bltdest = [render cachedDrawingRep];
    }
#endif
    if([bltdest isMemberOfClass:[NSBitmapImageRep class]])
    {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:_dw_draw_context(bltdest)];
        [[[NSDictionary alloc] initWithObjectsAndKeys:bltdest, NSGraphicsContextDestinationAttributeName, nil] autorelease];
    }
#ifndef BUILDING_FOR_MOJAVE
    else
    {
        if([bltdest lockFocusIfCanDraw] == NO)
        {
            free(bltinfo);
            return;
        }
        _DWLastDrawable = bltinfo->dest;
    }
#endif
    if(bltdest && [bltsrc isMemberOfClass:[NSBitmapImageRep class]])
    {
        NSBitmapImageRep *rep = bltsrc;
        NSImage *image = [NSImage alloc];
        SEL siwc = NSSelectorFromString(@"initWithCGImage");
        NSCompositingOperation op = DWCompositingOperationSourceOver;

        if([image respondsToSelector:siwc])
        {
            DWIMP iiwc = (DWIMP)[image methodForSelector:siwc];
            image = iiwc(image, siwc, [rep CGImage], NSZeroSize);
        }
        else
        {
            image = [image initWithSize:[rep size]];
            [image addRepresentation:rep];
        }
        if(bltinfo->srcwidth != -1)
        {
            [image drawInRect:NSMakeRect(bltinfo->xdest, bltinfo->ydest, bltinfo->width, bltinfo->height)
                     fromRect:NSMakeRect(bltinfo->xsrc, bltinfo->ysrc, bltinfo->srcwidth, bltinfo->srcheight)
                    operation:op fraction:1.0];
        }
        else
        {
            [image drawAtPoint:NSMakePoint(bltinfo->xdest, bltinfo->ydest)
                      fromRect:NSMakeRect(bltinfo->xsrc, bltinfo->ysrc, bltinfo->width, bltinfo->height)
                     operation:op fraction:1.0];
        }
        [bltsrc release];
        [image release];
    }
    if([bltdest isMemberOfClass:[NSBitmapImageRep class]])
    {
        [NSGraphicsContext restoreGraphicsState];
    }
#ifndef BUILDING_FOR_MOJAVE
    else
    {
        [bltdest unlockFocus];
    }
#endif
    free(bltinfo);
}
-(void)doFlush:(id)param
{
#ifdef BUILDING_FOR_MOJAVE
    NSEnumerator *enumerator = [_DWDirtyDrawables objectEnumerator];
    DWRender *rend;

    while (rend = [enumerator nextObject])
        [rend setNeedsDisplay:YES];
    [_DWDirtyDrawables removeAllObjects];
#else
    if(_DWLastDrawable)
    {
        id object = _DWLastDrawable;
        NSWindow *window = [object window];
        [window flushWindow];
        _DWLastDrawable = nil;
    }
#endif
}
-(void)doWindowFunc:(id)param
{
    NSValue *v = (NSValue *)param;
    void **params = (void **)[v pointerValue];
    void (* windowfunc)(void *);

    if(params)
    {
        windowfunc = params[0];
        if(windowfunc)
        {
            windowfunc(params[1]);
        }
    }
}
@end

DWObject *DWObj;

/* Subclass for the application class */
@interface DWAppDel : NSObject
#ifdef BUILDING_FOR_MOUNTAIN_LION
<NSApplicationDelegate, NSUserNotificationCenterDelegate>
#elif defined(BUILDING_FOR_SNOW_LEOPARD)
<NSApplicationDelegate>
#endif
{
}
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
#if defined(BUILDING_FOR_MOUNTAIN_LION) && !defined(BUILDING_FOR_BIG_SUR)
-(void)applicationDidFinishLaunching:(NSNotification *)aNotification;
-(BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification;
-(void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification;
#endif
@end

/* Apparently the WKWebKit API is only enabled on intel 64bit...
 * Causing build failures on 32bit builds, so this should allow
 * WKWebKit on intel 64 and the old WebKit on intel 32 bit and earlier.
 */
#if WK_API_ENABLED
@interface DWWebView : WKWebView <WKNavigationDelegate, WKScriptMessageHandler>
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation;
-(void)webView:(WKWebView *)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation *)navigation;
-(void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message;
@end

@implementation DWWebView : WKWebView
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_STARTED), [[self URL] absoluteString] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_COMPLETE), [[self URL] absoluteString] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_LOADING), [[self URL] absoluteString] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WKWebView *)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation *)navigation
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_REDIRECT), [[self URL] absoluteString] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    _dw_event_handler(self, (NSEvent *)message, _DW_EVENT_HTML_MESSAGE);
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end
#else
@interface DWWebView : WebView
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame;
-(void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame;
-(void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame;
@end

@implementation DWWebView : WebView
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_COMPLETE), [self mainFrameURL] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_STARTED), [self mainFrameURL] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame
{
    void *params[2] = { DW_INT_TO_POINTER(DW_HTML_CHANGE_LOADING), [self mainFrameURL] };
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_HTML_CHANGED);
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end
#endif


@implementation DWAppDel
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if(_dw_event_handler(sender, nil, _DW_EVENT_DELETE) > 0)
        return NSTerminateCancel;
    return NSTerminateNow;
}
#if defined(BUILDING_FOR_MOUNTAIN_LION) && !defined(BUILDING_FOR_BIG_SUR)
-(void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
#ifdef BUILDING_FOR_MOJAVE
    if (@available(macOS 10.14, *)) {} else
#endif
    {
        NSUserNotificationCenter* unc = [NSUserNotificationCenter defaultUserNotificationCenter];
        unc.delegate = self;
    }
    return;
}
-(BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    return YES;
}
-(void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)usernotification
{
    NSScanner *objScanner = [NSScanner scannerWithString:usernotification.identifier];
    unsigned long long handle;
    HWND notification;
    
    /* Skip the dw-notification- prefix */
    [objScanner scanString:@"dw-notification-" intoString:nil];
    [objScanner scanUnsignedLongLong:&handle];
    notification = DW_UINT_TO_POINTER(handle);
    
    switch(usernotification.activationType)
    {
        case NSUserNotificationActivationTypeContentsClicked:
            _dw_event_handler(notification, nil, _DW_EVENT_CLICKED);
            break;
        default:
            break;
    }
    dw_signal_disconnect_by_window(notification);
}
#endif
@end

/* Subclass for a top-level window */
@interface DWView : DWBox
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSWindowDelegate>
#endif
{
    NSMenu *windowmenu;
    NSSize oldsize;
}
-(BOOL)windowShouldClose:(id)sender;
-(void)setMenu:(NSMenu *)input;
-(void)windowDidBecomeMain:(id)sender;
-(void)menuHandler:(id)sender;
-(void)mouseDragged:(NSEvent *)theEvent;
@end

@implementation DWView
-(BOOL)windowShouldClose:(id)sender
{
    if(_dw_event_handler(sender, nil, _DW_EVENT_DELETE) > 0)
        return NO;
    return YES;
}
-(void)viewDidMoveToWindow
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:[self window]];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeMain:) name:NSWindowDidBecomeMainNotification object:[self window]];
}
-(void)dealloc
{
    if(windowmenu)
    {
        [windowmenu release];
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
-(void)windowResized:(NSNotification *)notification;
{
    NSSize size = [self frame].size;

    if(oldsize.width != size.width || oldsize.height != size.height)
    {
        _dw_do_resize(box, size.width, size.height);
        _dw_event_handler([self window], nil, _DW_EVENT_CONFIGURE);
        oldsize.width = size.width;
        oldsize.height = size.height;
        _dw_handle_resize_events(box);
    }
}
-(void)showWindow
{
    NSSize size = [self frame].size;

    if(oldsize.width == size.width && oldsize.height == size.height)
    {
        _dw_do_resize(box, size.width, size.height);
        _dw_handle_resize_events(box);
    }

}
-(void)windowDidBecomeMain:(id)sender
{
    if(windowmenu)
    {
        [DWApp setMainMenu:windowmenu];
    }
    else
    {
        [DWApp setMainMenu:DWMainMenu];
    }
    _dw_event_handler([self window], nil, _DW_EVENT_SET_FOCUS);
}
-(void)setMenu:(NSMenu *)input { windowmenu = input; [windowmenu retain]; }
-(void)menuHandler:(id)sender
{
    id menu = [sender menu];

    /* Find the highest menu for this item */
    while([menu supermenu])
    {
        menu = [menu supermenu];
    }

    /* Only perform the delay if this item is a child of the main menu */
    if([DWApp mainMenu] == menu)
        [DWObj performSelector:@selector(menuHandler:) withObject:sender afterDelay:0];
    else
        [DWObj menuHandler:sender];
    _dw_wakeup_app();
}
-(void)mouseDragged:(NSEvent *)theEvent { _dw_event_handler(self, theEvent, _DW_EVENT_MOTION_NOTIFY); }
-(void)mouseMoved:(NSEvent *)theEvent
{
    id hit = [self hitTest:[theEvent locationInWindow]];

    if([hit isMemberOfClass:[DWRender class]])
    {
        _dw_event_handler(hit, theEvent, _DW_EVENT_MOTION_NOTIFY);
    }
}
@end

/* Subclass for a button type */
@interface DWButton : NSButton
{
    void *userdata;
    NSButtonType buttonType;
    DWBox *parent;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)buttonClicked:(id)sender;
-(void)setButtonType:(NSButtonType)input;
-(NSButtonType)buttonType;
-(void)setParent:(DWBox *)input;
-(DWBox *)parent;
-(NSColor *)textColor;
-(void)setTextColor:(NSColor *)textColor;
@end

@implementation DWButton
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)buttonClicked:(id)sender
{
    _dw_event_handler(self, nil, _DW_EVENT_CLICKED);
    if([self buttonType] == DWButtonTypeRadio)
    {
        DWBox *viewbox = [self parent];
        Box *thisbox = [viewbox box];
        int z;

        for(z=0;z<thisbox->count;z++)
        {
            if(thisbox->items[z].type != _DW_TYPE_BOX)
            {
                id object = thisbox->items[z].hwnd;

                if([object isMemberOfClass:[DWButton class]])
                {
                    DWButton *button = object;

                    if(button != self && [button buttonType] == DWButtonTypeRadio)
                    {
                        [button setState:DWControlStateValueOff];
                    }
                }
            }
        }
    }
}
-(void)setButtonType:(NSButtonType)input { buttonType = input; [super setButtonType:input]; }
-(NSButtonType)buttonType { return buttonType; }
-(void)setParent:(DWBox *)input { parent = input; }
-(DWBox *)parent { return parent; }
-(NSColor *)textColor
{
    NSAttributedString *attrTitle = [self attributedTitle];
    NSUInteger len = [attrTitle length];
    NSRange range = NSMakeRange(0, MIN(len, 1));
    NSDictionary *attrs = [attrTitle fontAttributesInRange:range];
    NSColor *textColor = [NSColor controlTextColor];
    if (attrs) {
        textColor = [attrs objectForKey:NSForegroundColorAttributeName];
    }
    return textColor;
}
-(void)setTextColor:(NSColor *)textColor
{
    NSMutableAttributedString *attrTitle = [[NSMutableAttributedString alloc]
                                            initWithAttributedString:[self attributedTitle]];
    NSUInteger len = [attrTitle length];
    NSRange range = NSMakeRange(0, len);
    [attrTitle addAttribute:NSForegroundColorAttributeName
                      value:textColor
                      range:range];
    [attrTitle fixAttributesInRange:range];
    [self setAttributedTitle:attrTitle];
    [attrTitle release];
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(vk == VK_RETURN || vk == VK_SPACE)
    {
        if(buttonType == DWButtonTypeSwitch)
            [self setState:([self state] ? DWControlStateValueOff : DWControlStateValueOn)];
        else if(buttonType == DWButtonTypeRadio)
            [self setState:DWControlStateValueOn];
        [self buttonClicked:self];
    }
    else
    {
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
        [super keyDown:theEvent];
    }
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a progress type */
@interface DWPercent : NSProgressIndicator
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWPercent
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a menu item type */
@implementation DWMenuItem
-(void)setCheck:(int)input { check = input; }
-(int)check { return check; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a scrollbox type */
@interface DWScrollBox : NSScrollView
{
    void *userdata;
    id box;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setBox:(void *)input;
-(id)box;
@end

@implementation DWScrollBox
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setBox:(void *)input { box = input; }
-(id)box { return box; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a textfield that supports vertical centering */
@interface DWTextFieldCell : NSTextFieldCell
{
    BOOL vcenter;
}
-(NSRect)drawingRectForBounds:(NSRect)theRect;
-(void)setVCenter:(BOOL)input;
@end

@implementation DWTextFieldCell
-(NSRect)drawingRectForBounds:(NSRect)theRect
{
    /* Get the parent's idea of where we should draw */
    NSRect newRect = [super drawingRectForBounds:theRect];

    /* If we are being vertically centered */
    if(vcenter)
    {
        /* Get our ideal size for current text */
        NSSize textSize = [self cellSizeForBounds:theRect];

        /* Center that in the proposed rect */
        float heightDelta = newRect.size.height - textSize.height;	
        if (heightDelta > 0)
        {
            newRect.size.height -= heightDelta;
            newRect.origin.y += (heightDelta / 2);
        }
    }
	
    return newRect;
}
-(void)setVCenter:(BOOL)input { vcenter = input; }
@end

@interface DWEntryFieldFormatter : NSFormatter
{
    int maxLength;
}
- (void)setMaximumLength:(int)len;
- (int)maximumLength;
@end

/* This formatter subclass will allow us to limit
 * the text length in an entryfield.
 */
@implementation DWEntryFieldFormatter
-(id)init
{
    self = [super init];
    maxLength = INT_MAX;
    return self;
}
-(void)setMaximumLength:(int)len { maxLength = len; }
-(int)maximumLength { return maxLength; }
-(NSString *)stringForObjectValue:(id)object { return (NSString *)object; }
-(BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error { *object = string; return YES; }
-(BOOL)isPartialStringValid:(NSString **)partialStringPtr
       proposedSelectedRange:(NSRangePointer)proposedSelRangePtr
              originalString:(NSString *)origString
       originalSelectedRange:(NSRange)origSelRange
            errorDescription:(NSString **)error
{
    if([*partialStringPtr length] > maxLength)
    {
        return NO;
    }
    return YES;
}
-(NSAttributedString *)attributedStringForObjectValue:(id)anObject withDefaultAttributes:(NSDictionary *)attributes { return nil; }
@end

/* Subclass for a entryfield type */
@interface DWEntryField : NSTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setClickDefault:(id)input;
@end

@implementation DWEntryField
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(clickDefault && vk == VK_RETURN)
    {
        if([clickDefault isKindOfClass:[NSButton class]])
            [clickDefault buttonClicked:self];
        else
            [[self window] makeFirstResponder:clickDefault];
    } else
    {
        [super keyUp:theEvent];
    }
}
-(BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
    if(([theEvent modifierFlags] & DWEventModifierFlagDeviceIndependentFlagsMask) == DWEventModifierFlagCommand)
    {
        if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"x"])
            return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"c"])
            return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"v"])
            return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"a"])
            return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
    }
    return [super performKeyEquivalent:theEvent];
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a text and status text type */
@interface DWText : NSTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWText
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); [super dealloc]; }
@end


/* Subclass for a entryfield password type */
@interface DWEntryFieldPassword : NSSecureTextField
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setClickDefault:(id)input;
@end

@implementation DWEntryFieldPassword
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        if([clickDefault isKindOfClass:[NSButton class]])
            [clickDefault buttonClicked:self];
        else
            [[self window] makeFirstResponder:clickDefault];
    }
    else
    {
        [super keyUp:theEvent];
    }
}
-(BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
    if(([theEvent modifierFlags] & DWEventModifierFlagDeviceIndependentFlagsMask) == DWEventModifierFlagCommand)
    {
        if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"x"])
            return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"c"])
            return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"v"])
            return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
        else if ([[theEvent charactersIgnoringModifiers] isEqualToString:@"a"])
            return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
    }
    return [super performKeyEquivalent:theEvent];
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a Notebook control type */
@interface DWNotebook : NSTabView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTabViewDelegate>
#endif
{
    void *userdata;
    int pageid;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(int)pageid;
-(void)setPageid:(int)input;
-(void)tabView:(NSTabView *)notebook didSelectTabViewItem:(NSTabViewItem *)notepage;
@end

/* Subclass for a Notebook page type */
@interface DWNotebookPage : NSTabViewItem
{
    void *userdata;
    int pageid;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(int)pageid;
-(void)setPageid:(int)input;
@end

@implementation DWNotebook
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)tabView:(NSTabView *)notebook didSelectTabViewItem:(NSTabViewItem *)notepage
{
    id object = [notepage view];
    DWNotebookPage *page = (DWNotebookPage *)notepage;

    if([object isMemberOfClass:[DWBox class]])
    {
        DWBox *view = object;
        Box *box = [view box];
        NSSize size = [view frame].size;
        _dw_do_resize(box, size.width, size.height);
        _dw_handle_resize_events(box);
    }
    _dw_event_handler(self, DW_INT_TO_POINTER([page pageid]), _DW_EVENT_SWITCH_PAGE);
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    else if(vk == NSLeftArrowFunctionKey)
    {
        NSArray *pages = [self tabViewItems];
        DWNotebookPage *page = (DWNotebookPage *)[self selectedTabViewItem];
        NSUInteger index = [pages indexOfObject:page];

        if(index != NSNotFound)
        {
            if(index > 0)
               [self selectTabViewItem:[pages objectAtIndex:(index-1)]];
            else
               [self selectTabViewItem:[pages objectAtIndex:0]];

        }
    }
    else if(vk == NSRightArrowFunctionKey)
    {
        NSArray *pages = [self tabViewItems];
        DWNotebookPage *page = (DWNotebookPage *)[self selectedTabViewItem];
        NSUInteger index = [pages indexOfObject:page];
        NSUInteger count = [pages count];

        if(index != NSNotFound)
        {
            if(index + 1 < count)
                [self selectTabViewItem:[pages objectAtIndex:(index+1)]];
            else
                [self selectTabViewItem:[pages objectAtIndex:(count-1)]];

        }
    }
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

@implementation DWNotebookPage
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(int)pageid { return pageid; }
-(void)setPageid:(int)input { pageid = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a color chooser type */
@interface DWColorChoose : NSColorPanel
{
    DWDialog *dialog;
    NSColor *pickedcolor;
}
-(void)changeColor:(id)sender;
-(void)setDialog:(DWDialog *)input;
-(DWDialog *)dialog;
@end

@implementation DWColorChoose
-(void)changeColor:(id)sender
{
    if(!dialog)
        [self close];
    else
        pickedcolor = [self color];
}
-(BOOL)windowShouldClose:(id)window
{
    if(dialog)
    {
        DWDialog *d = dialog;
        dialog = nil;
        dw_dialog_dismiss(d, pickedcolor);
    }
    [self close];
    return NO;
}
-(void)setDialog:(DWDialog *)input { dialog = input; }
-(DWDialog *)dialog { return dialog; }
@end

/* Subclass for a font chooser type */
@interface DWFontChoose : NSFontPanel
{
    DWDialog *dialog;
}
-(void)setDialog:(DWDialog *)input;
-(DWDialog *)dialog;
@end

@implementation DWFontChoose
-(BOOL)windowShouldClose:(id)window
{
    DWDialog *d = dialog; dialog = nil;
    NSFont *pickedfont = [DWFontManager selectedFont];
    dw_dialog_dismiss(d, pickedfont);
    [window orderOut:nil];
    return NO;
}
-(void)setDialog:(DWDialog *)input { dialog = input; }
-(DWDialog *)dialog { return dialog; }
@end

/* Subclass for a splitbar type */
@interface DWSplitBar : NSSplitView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSSplitViewDelegate>
#endif
{
    void *userdata;
    float percent;
    NSInteger Tag;
}
-(void)splitViewDidResizeSubviews:(NSNotification *)aNotification;
-(void)setTag:(NSInteger)tag;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(float)percent;
-(void)setPercent:(float)input;
@end

@implementation DWSplitBar
-(void)splitViewDidResizeSubviews:(NSNotification *)aNotification
{
    NSArray *views = [self subviews];
    id object;

    for(object in views)
    {
        if([object isMemberOfClass:[DWBox class]])
        {
            DWBox *view = object;
            Box *box = [view box];
            NSSize size = [view frame].size;
            _dw_do_resize(box, size.width, size.height);
            _dw_handle_resize_events(box);
        }
    }
}
-(void)setTag:(NSInteger)tag { Tag = tag; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(float)percent { return percent; }
-(void)setPercent:(float)input { percent = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a slider type */
@interface DWSlider : NSSlider
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)sliderChanged:(id)sender;
@end

@implementation DWSlider
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)sliderChanged:(id)sender { _dw_event_handler(self, (void *)[self integerValue], _DW_EVENT_VALUE_CHANGED); }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a slider type */
@interface DWScrollbar : NSScroller
{
    void *userdata;
    double range;
    double visible;
    int vertical;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(float)range;
-(float)visible;
-(int)vertical;
-(void)setVertical:(int)value;
-(void)setRange:(double)input1 andVisible:(double)input2;
-(void)scrollerChanged:(id)sender;
@end

@implementation DWScrollbar
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(float)range { return range; }
-(float)visible { return visible; }
-(int)vertical { return vertical; }
-(void)setVertical:(int)value { vertical = value; }
-(void)setRange:(double)input1 andVisible:(double)input2 { range = input1; visible = input2; }
-(void)scrollerChanged:(id)sender
{
    double max = (range - visible);
    double result = ([self doubleValue] * max);
    double newpos = result;

    switch ([sender hitPart])
    {
#ifndef BUILDING_FOR_YOSEMITE
        case NSScrollerDecrementLine:
            if(newpos > 0)
            {
                newpos--;
            }
            break;

        case NSScrollerIncrementLine:
            if(newpos < max)
            {
                newpos++;
            }
            break;
#endif
        case NSScrollerDecrementPage:
            newpos -= visible;
            if(newpos < 0)
            {
                newpos = 0;
            }
            break;

        case NSScrollerIncrementPage:
            newpos += visible;
            if(newpos > max)
            {
                newpos = max;
            }
            break;

        default:
            ; /* do nothing */
    }
    int newposi = (int)newpos;
    newpos = (newpos - (double)newposi) > 0.5 ? (double)(newposi + 1) : (double)newposi;
    if(newpos != result)
    {
        [self setDoubleValue:(newpos/max)];
    }
    _dw_event_handler(self, DW_INT_TO_POINTER((int)newpos), _DW_EVENT_VALUE_CHANGED);
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a MLE type */
@interface DWMLE : NSTextView
{
    void *userdata;
    id scrollview;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(id)scrollview;
-(void)setScrollview:(id)input;
@end

@implementation DWMLE
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(id)scrollview { return scrollview; }
-(void)setScrollview:(id)input { scrollview = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [scrollview release]; [super dealloc]; }
@end

#ifndef DW_USE_NSVIEW
/* Subclass NSTextFieldCell for displaying image and text */
@interface DWImageAndTextCell : NSTextFieldCell
{
@private
    NSImage	*image;
}
-(void)setImage:(NSImage *)anImage;
-(NSImage *)image;
-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
-(NSSize)cellSize;
@end

@implementation DWImageAndTextCell
-(void)dealloc
{
    [image release];
    image = nil;
    [super dealloc];
}
-copyWithZone:(NSZone *)zone
{
    DWImageAndTextCell *cell = (DWImageAndTextCell *)[super copyWithZone:zone];
    cell->image = [image retain];
    return cell;
}
-(void)setImage:(NSImage *)anImage
{
    if(anImage != image)
    {
        [image release];
        image = [anImage retain];
    }
}
-(NSImage *)image
{
    return [[image retain] autorelease];
}
-(NSRect)imageFrameForCellFrame:(NSRect)cellFrame
{
    if(image != nil)
    {
        NSRect imageFrame;
        imageFrame.size = [image size];
        imageFrame.origin = cellFrame.origin;
        imageFrame.origin.x += 3;
        imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);
        return imageFrame;
    }
    else
        return NSZeroRect;
}
-(void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [image size].width, NSMinXEdge);
    [super editWithFrame: textFrame inView: controlView editor:textObj delegate:anObject event: theEvent];
}
-(void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [image size].width, NSMinXEdge);
    [super selectWithFrame: textFrame inView: controlView editor:textObj delegate:anObject start:selStart length:selLength];
}
-(void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    if(image != nil)
    {
        NSSize	imageSize;
        NSRect	imageFrame;
        SEL sdir = NSSelectorFromString(@"drawInRect:fromRect:operation:fraction:respectFlipped:hints:");
        NSCompositingOperation op = DWCompositingOperationSourceOver;

        imageSize = [image size];
        NSDivideRect(cellFrame, &imageFrame, &cellFrame, 3 + imageSize.width, NSMinXEdge);
        if ([self drawsBackground])
        {
            [[self backgroundColor] set];
            NSRectFill(imageFrame);
        }
        imageFrame.origin.x += 3;
        imageFrame.size = imageSize;

        /* New method for 10.6 and later */
        if([image respondsToSelector:sdir])
        {
            DWIMP idir = (DWIMP)[image methodForSelector:sdir];

            imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);

            idir(image, sdir, imageFrame, NSZeroRect, op, 1.0, YES, nil);
        }
        else
        {
            /* Old method for 10.5 */
            SEL sctp = NSSelectorFromString(@"compositeToPoint:operation:");

            if ([controlView isFlipped])
                imageFrame.origin.y += ceil((cellFrame.size.height + imageFrame.size.height) / 2);
            else
                imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);

            if([image respondsToSelector:sctp])
            {
                DWIMP ictp = (DWIMP)[image methodForSelector:sctp];

                ictp(image, sctp, imageFrame.origin, op);
            }
        }
    }
    [super drawWithFrame:cellFrame inView:controlView];
}
-(NSSize)cellSize
{
    NSSize cellSize = [super cellSize];
    cellSize.width += (image ? [image size].width : 0) + 3;
    return cellSize;
}
@end
#endif

#ifdef DW_USE_NSVIEW
NSTableCellView *_dw_table_cell_view_new(NSImage *icon, NSString *text)
{
    NSTableCellView *browsercell = [[[NSTableCellView alloc] init] autorelease];
    [browsercell setAutoresizesSubviews:YES];
    if(icon)
    {
        NSImageView *iv = [[[NSImageView alloc] init] autorelease];
        [iv setAutoresizingMask:NSViewHeightSizable|(text ? 0 :NSViewWidthSizable)];
        [iv setImage:icon];
        [browsercell setImageView:iv];
        [browsercell addSubview:iv];
    }
    if(text)
    {
        NSTextField *tf = [[[NSTextField alloc] init] autorelease];
        [tf setAutoresizingMask:NSViewHeightSizable|NSViewWidthSizable];
        [tf setStringValue:text];
        [tf setEditable:NO];
        [tf setBezeled:NO];
        [tf setBordered:NO];
        [tf setDrawsBackground:NO];
        [[tf cell] setVCenter:YES];
        [browsercell setTextField:tf];
        [browsercell addSubview:tf];
    }
    return browsercell;
}

void _dw_table_cell_view_layout(NSTableCellView *result)
{
    /* Adjust the frames of the textField and imageView when both are present */
    if([result imageView] && [result textField])
    {
        NSImageView *iv = [result imageView];
        NSImage *icon = [iv image];
        NSTextField *tf = [result textField];
        NSRect rect = result.frame;
        CGFloat width = [icon size].width;

        [iv setFrame:NSMakeRect(0,0,width,rect.size.height)];

        /* Adjust the rect to allow space for the image */
        rect.origin.x += width;
        if(rect.size.width)
            rect.size.width -= width;
        [tf setFrame:rect];
    }
}
#endif

@interface DWFocusRingScrollView : NSScrollView
{
    BOOL shouldDrawFocusRing;
    NSResponder* lastResp;
}
@end

@implementation DWFocusRingScrollView
-(BOOL)needsDisplay;
{
    NSResponder* resp = nil;
    
    if([[self window] isKeyWindow])
    {
        resp = [[self window] firstResponder];
        if (resp == lastResp)
            return [super needsDisplay];
    }
    else if (lastResp == nil)
    {
        return [super needsDisplay];
    }
    shouldDrawFocusRing = (resp != nil && [resp isKindOfClass:[NSView class]] && [(NSView*)resp isDescendantOf:self]);
    lastResp = resp;
    [self setKeyboardFocusRingNeedsDisplayInRect:[self bounds]];
    return YES;
}
-(void)drawRect:(NSRect)rect
{
    [super drawRect:rect];
                                                                                                    
    if(shouldDrawFocusRing)
    {
        NSSetFocusRingStyle(NSFocusRingOnly);
        NSRectFill(rect);
    }
}
@end

/* Subclass for a Container/List type */
@interface DWContainer : NSTableView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTableViewDataSource,NSTableViewDelegate>
#endif
{
    void *userdata;
    NSMutableArray *tvcols;
    NSMutableArray *data;
    NSMutableArray *types;
    NSPointerArray *titles;
    NSPointerArray *rowdatas;
    NSColor *fgcolor, *oddcolor, *evencolor;
    unsigned long dw_oddcolor, dw_evencolor;
    unsigned long _DW_COLOR_ROW_ODD, _DW_COLOR_ROW_EVEN;
    int lastAddPoint, lastQueryPoint;
    id scrollview;
    int filesystem;
}
-(NSInteger)numberOfRowsInTableView:(NSTableView *)aTable;
-(id)tableView:(NSTableView *)aTable objectValueForTableColumn:(NSTableColumn *)aCol row:(NSInteger)aRow;
-(BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)setFilesystem:(int)input;
-(int)filesystem;
-(id)scrollview;
-(void)setScrollview:(id)input;
-(void)addColumn:(NSTableColumn *)input andType:(int)type;
-(NSTableColumn *)getColumn:(int)col;
-(int)addRow:(NSArray *)input;
-(int)addRows:(int)number;
-(void)editCell:(id)input at:(int)row and:(int)col;
-(void)removeRow:(int)row;
-(void)setRow:(int)row title:(const char *)input;
-(void *)getRowTitle:(int)row;
-(id)getRow:(int)row and:(int)col;
-(int)cellType:(int)col;
-(int)lastAddPoint;
-(int)lastQueryPoint;
-(void)setLastQueryPoint:(int)input;
-(void)clear;
-(void)setup;
-(void)optimize;
-(NSSize)getsize;
-(void)setForegroundColor:(NSColor *)input;
-(void)doubleClicked:(id)sender;
-(void)keyUp:(NSEvent *)theEvent;
-(void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn;
-(void)selectionChanged:(id)sender;
-(NSMenu *)menuForEvent:(NSEvent *)event;
#ifdef DW_USE_NSVIEW
-(void)tableView:(NSTableView *)tableView didAddRowView:(NSTableRowView *)rowView forRow:(NSInteger)row;
-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
#else
-(void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
#endif
@end

@implementation DWContainer
-(NSInteger)numberOfRowsInTableView:(NSTableView *)aTable
{
    if(tvcols && data)
    {
        int cols = (int)[tvcols count];
        int total = (int)[data count];
        if(cols && total)
        {
            return total / cols;
        }
    }
    return 0;
}
-(id)tableView:(NSTableView *)aTable objectValueForTableColumn:(NSTableColumn *)aCol row:(NSInteger)aRow
{
    if(tvcols && data)
    {
        int z, col = -1;
        int count = (int)[tvcols count];

        for(z=0;z<count;z++)
        {
            if([tvcols objectAtIndex:z] == aCol)
            {
                col = z;
                break;
            }
        }
        if(col != -1)
        {
            int index = (int)(aRow * count) + col;
            if(index < [data count])
            {
                id this = [data objectAtIndex:index];
                return ([this isKindOfClass:[NSNull class]]) ? nil : this;
            }
        }
    }
    return nil;
}
-(BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex { return NO; }
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)setFilesystem:(int)input { filesystem = input; }
-(int)filesystem { return filesystem; }
-(id)scrollview { return scrollview; }
-(void)setScrollview:(id)input { scrollview = input; }
-(void)addColumn:(NSTableColumn *)input andType:(int)type { if(tvcols) { [tvcols addObject:input]; [types addObject:[NSNumber numberWithInt:type]]; } }
-(NSTableColumn *)getColumn:(int)col { if(tvcols) { return [tvcols objectAtIndex:col]; } return nil; }
-(void)refreshColors
{
    NSColor *oldodd = oddcolor;
    NSColor *oldeven = evencolor;
    unsigned long thisodd = dw_oddcolor == DW_CLR_DEFAULT ? _DW_COLOR_ROW_ODD : dw_oddcolor;
    unsigned long _odd = _dw_get_color(thisodd);
    unsigned long thiseven = dw_evencolor == DW_CLR_DEFAULT ? _DW_COLOR_ROW_EVEN : dw_evencolor;
    unsigned long _even = _dw_get_color(thiseven);

    /* Get the NSColor for non-default colors */
    if(thisodd != DW_RGB_TRANSPARENT)
        oddcolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(_odd)/255.0 green: DW_GREEN_VALUE(_odd)/255.0 blue: DW_BLUE_VALUE(_odd)/255.0 alpha: 1] retain];
    else
        oddcolor = NULL;
    if(thiseven != DW_RGB_TRANSPARENT)
        evencolor = [[NSColor colorWithDeviceRed: DW_RED_VALUE(_even)/255.0 green: DW_GREEN_VALUE(_even)/255.0 blue: DW_BLUE_VALUE(_even)/255.0 alpha: 1] retain];
    else
        evencolor = NULL;
    [oldodd release];
    [oldeven release];
}
-(void)setRowBgOdd:(unsigned long)oddcol andEven:(unsigned long)evencol
{
    /* Save the set colors in case we get a theme change */
    dw_oddcolor = oddcol;
    dw_evencolor = evencol;
    [self refreshColors];
}
-(void)checkDark
{
    /* Update any system colors based on the Dark Mode */
    _DW_COLOR_ROW_EVEN = DW_RGB_TRANSPARENT;
    if(_dw_is_dark(self))
        _DW_COLOR_ROW_ODD = DW_RGB(100, 100, 100);
    else
        _DW_COLOR_ROW_ODD = DW_RGB(230, 230, 230);
    /* Only refresh if we've been setup already */
    if(titles)
        [self refreshColors];
}
-(void)viewDidChangeEffectiveAppearance { [self checkDark]; }
-(int)insertRow:(NSArray *)input at:(int)index
{
    if(data)
    {
        unsigned long start = [tvcols count] * index;
        NSIndexSet *set = [[NSIndexSet alloc] initWithIndexesInRange:NSMakeRange(start, start + [tvcols count])];
        if(index < lastAddPoint)
        {
            lastAddPoint++;
        }
        [data insertObjects:input atIndexes:set];
        [titles insertPointer:NULL atIndex:index];
        [rowdatas insertPointer:NULL atIndex:index];
        [set release];
        return (int)[titles count];
    }
    return 0;
}
-(int)addRow:(NSArray *)input
{
    if(data)
    {
        lastAddPoint = (int)[titles count];
        [data addObjectsFromArray:input];
        [titles addPointer:NULL];
        [rowdatas addPointer:NULL];
        return (int)[titles count];
    }
    return 0;
}
-(int)addRows:(int)number
{
    if(tvcols)
    {
        int count = (int)(number * [tvcols count]);
        int z;

        lastAddPoint = (int)[titles count];

        for(z=0;z<count;z++)
        {
            [data addObject:[NSNull null]];
        }
        for(z=0;z<number;z++)
        {
            [titles addPointer:NULL];
            [rowdatas addPointer:NULL];
        }
        return (int)[titles count];
    }
    return 0;
}
#ifdef DW_USE_NSVIEW
-(void)tableView:(NSTableView *)tableView didAddRowView:(NSTableRowView *)rowView forRow:(NSInteger)row
{
    /* Handle drawing alternating row colors if enabled */
    if ((row % 2) == 0)
    {
        if(evencolor)
            [rowView setBackgroundColor:evencolor];
    }
    else
    {
        if(oddcolor)
            [rowView setBackgroundColor:oddcolor];
    }
}
-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
{
    /* Not reusing cell views, so get the cell from our array */
    int index = (int)(row * [tvcols count]) + (int)[tvcols indexOfObject:tableColumn];
    id celldata = [data objectAtIndex:index];

    /* The data is already a NSTableCellView so just return that */
    if([celldata isMemberOfClass:[NSTableCellView class]])
    {
        NSTableCellView *result = celldata;
        NSTextAlignment align = [[tableColumn headerCell] alignment];
        
        _dw_table_cell_view_layout(result);
        
        /* Copy the alignment setting from the column,
         * and set the text color from the container.
         */
        if([result textField])
        {
            NSTextField *tf = [result textField];
            
            [tf setAlignment:align];
            if(fgcolor)
                [tf setTextColor:fgcolor];
        }
        
        /* Return the result */
        return result;
    }
    return nil;
}
#else
-(void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    DWImageAndTextCell *bcell = cell, *tcell = cell;

    /* Handle drawing image and text if necessary */
    if([cell isMemberOfClass:[DWImageAndTextCell class]])
    {
        int index = (int)(row * [tvcols count]);
        DWImageAndTextCell *browsercell = [data objectAtIndex:index];
        NSImage *img = [browsercell image];
        [bcell setImage:img];
    }
    
    if([tcell isKindOfClass:[NSTextFieldCell class]])
    {
        /* Handle drawing alternating row colors if enabled */
        if ((row % 2) == 0)
        {
            if(evencolor)
            {
                [tcell setDrawsBackground:YES];
                [tcell setBackgroundColor:evencolor];
            }
            else
                [tcell setDrawsBackground:NO];
        }
        else
        {
            if(oddcolor)
            {
                [tcell setDrawsBackground:YES];
                [tcell setBackgroundColor:oddcolor];
            }
            else
                [tcell setDrawsBackground:NO];
        }
    }
}
#endif
-(void)editCell:(id)input at:(int)row and:(int)col
{
    if(tvcols)
    {
        int index = (int)(row * [tvcols count]) + col;
        if(index < [data count])
        {
            if(!input)
                input = [NSNull null];
            [data replaceObjectAtIndex:index withObject:input];
        }
    }
}
-(void)removeRow:(int)row
{
    if(tvcols)
    {
        int z, start, end;
        int count = (int)[tvcols count];
        void *oldtitle;

        start = (count * row);
        end = start + count;

        for(z=start;z<end;z++)
        {
            [data removeObjectAtIndex:start];
        }
        oldtitle = [titles pointerAtIndex:row];
        [titles removePointerAtIndex:row];
        [rowdatas removePointerAtIndex:row];
        if(lastAddPoint > 0 && lastAddPoint > row)
        {
            lastAddPoint--;
        }
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRow:(int)row title:(const char *)input
{
    if(titles && input)
    {
        void *oldtitle = [titles pointerAtIndex:row];
        void *newtitle = input ? (void *)strdup(input) : NULL;
        [titles replacePointerAtIndex:row withPointer:newtitle];
        if(oldtitle)
            free(oldtitle);
    }
}
-(void)setRowData:(int)row title:(void *)input { if(rowdatas && input) { [rowdatas replacePointerAtIndex:row withPointer:input]; } }
-(void *)getRowTitle:(int)row { if(titles && row > -1) { return [titles pointerAtIndex:row]; } return NULL; }
-(void *)getRowData:(int)row { if(rowdatas && row > -1) { return [rowdatas pointerAtIndex:row]; } return NULL; }
-(id)getRow:(int)row and:(int)col { if(data) { int index = (int)(row * [tvcols count]) + col; return [data objectAtIndex:index]; } return nil; }
-(int)cellType:(int)col { return [[types objectAtIndex:col] intValue]; }
-(int)lastAddPoint { return lastAddPoint; }
-(int)lastQueryPoint { return lastQueryPoint; }
-(void)setLastQueryPoint:(int)input { lastQueryPoint = input; }
-(void)clear
{
    if(data)
    {
        [data removeAllObjects];
        while([titles count])
        {
            void *oldtitle = [titles pointerAtIndex:0];
            [titles removePointerAtIndex:0];
            [rowdatas removePointerAtIndex:0];
            if(oldtitle)
                free(oldtitle);
        }
    }
    lastAddPoint = 0;
}
-(void)setup
{
    SEL swopa = NSSelectorFromString(@"pointerArrayWithWeakObjects");

    if(![[NSPointerArray class] respondsToSelector:swopa])
        swopa = NSSelectorFromString(@"weakObjectsPointerArray");
    if(![[NSPointerArray class] respondsToSelector:swopa])
        return;

    DWIMP iwopa = (DWIMP)[[NSPointerArray class] methodForSelector:swopa];

    titles = iwopa([NSPointerArray class], swopa);
    [titles retain];
    rowdatas = iwopa([NSPointerArray class], swopa);
    [rowdatas retain];
    tvcols = [[[NSMutableArray alloc] init] retain];
    data = [[[NSMutableArray alloc] init] retain];
    types = [[[NSMutableArray alloc] init] retain];
    if(!dw_oddcolor && !dw_evencolor)
        dw_oddcolor = dw_evencolor = DW_CLR_DEFAULT;
    [self checkDark];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(selectionChanged:) name:NSTableViewSelectionDidChangeNotification object:self];
}
-(NSSize)getsize
{
    int cwidth = 0, cheight = 0;

    if(tvcols)
    {
        int z;
        int colcount = (int)[tvcols count];
        int rowcount = (int)[self numberOfRowsInTableView:self];

        for(z=0;z<colcount;z++)
        {
            NSTableColumn *column = [tvcols objectAtIndex:z];
            int width = [column width];

            if(rowcount > 0)
            {
                int x;

                for(x=0;x<rowcount;x++)
                {
#ifdef DW_USE_NSVIEW
                    NSTableCellView *cell = [self viewAtColumn:z row:x makeIfNecessary:YES];
                    int thiswidth = 4, thisheight = 0;
                    
                    if([cell imageView])
                    {
                        thiswidth += [[cell imageView] image].size.width;
                        thisheight = [[cell imageView] image].size.height;
                    }
                    if([cell textField])
                    {
                        int textheight = [[cell textField] intrinsicContentSize].width;
                        thiswidth += [[cell textField] intrinsicContentSize].width;
                        if(textheight > thisheight)
                            thisheight = textheight;
                    }
                    
                    /* If on the first column... add up the heights */
                    if(z == 0)
                        cheight += thisheight;
#else
                    NSCell *cell = [self preparedCellAtColumn:z row:x];
                    int thiswidth = [cell cellSize].width;

                    /* If on the first column... add up the heights */
                    if(z == 0)
                        cheight += [cell cellSize].height;

                    /* Check the image inside the cell to get its width */
                    if([cell isMemberOfClass:[NSImageCell class]])
                    {
                        int index = (int)(x * [tvcols count]) + z;
                        NSImage *image = [data objectAtIndex:index];
                        if([image isMemberOfClass:[NSImage class]])
                        {
                            thiswidth = [image size].width;
                        }
                    }
#endif
                    
                    if(thiswidth > width)
                    {
                        width = thiswidth;
                    }
                }
                /* If the image is missing default the optimized width to 16. */
                if(!width && [[types objectAtIndex:z] intValue] & DW_CFA_BITMAPORICON)
                {
                    width = 16;
                }
            }
            if(width)
                cwidth += width;
        }
    }
    cwidth += 16;
    cheight += 16;
    return NSMakeSize(cwidth, cheight);
}
-(void)optimize
{
    if(tvcols)
    {
        int z;
        int colcount = (int)[tvcols count];
        int rowcount = (int)[self numberOfRowsInTableView:self];

        for(z=0;z<colcount;z++)
        {
            NSTableColumn *column = [tvcols objectAtIndex:z];
            if([column resizingMask] != NSTableColumnNoResizing)
            {
                if(rowcount > 0)
                {
                    int x;
                    NSCell *colcell = [column headerCell];
                    int width = [colcell cellSize].width;

                    for(x=0;x<rowcount;x++)
                    {
#ifdef DW_USE_NSVIEW
                        NSTableCellView *cell = [self viewAtColumn:z row:x makeIfNecessary:YES];
                        int thiswidth = 4;
                        
                        if([cell imageView])
                            thiswidth += [[cell imageView] image].size.width;
                        if([cell textField])
                            thiswidth += [[cell textField] intrinsicContentSize].width;
#else
                        NSCell *cell = [self preparedCellAtColumn:z row:x];
                        int thiswidth = [cell cellSize].width;

                        /* Check the image inside the cell to get its width */
                        if([cell isMemberOfClass:[NSImageCell class]])
                        {
                            int index = (int)(x * [tvcols count]) + z;
                            NSImage *image = [data objectAtIndex:index];
                            if([image isMemberOfClass:[NSImage class]])
                            {
                                thiswidth = [image size].width;
                            }
                        }
#endif
                        
                        if(thiswidth > width)
                        {
                            width = thiswidth;
                        }
                    }
                    /* If the image is missing default the optimized width to 16. */
                    if(!width && [[types objectAtIndex:z] intValue] & DW_CFA_BITMAPORICON)
                    {
                        width = 16;
                    }
                    /* Sanity check... don't set the width to 0 */
                    if(width)
                    {
                        [column setWidth:width+1];
                    }
                }
                else
                {
                    if(self.headerView)
                        [column sizeToFit];
                }
            }
        }
    }
}
-(void)setForegroundColor:(NSColor *)input
{
    int z, count = (int)[tvcols count];

    fgcolor = input;
    [fgcolor retain];

    for(z=0;z<count;z++)
    {
        NSTableColumn *tableColumn = [tvcols objectAtIndex:z];
        NSTextFieldCell *cell = [tableColumn dataCell];
        [cell setTextColor:fgcolor];
    }
}
-(void)doubleClicked:(id)sender
{
    void *params[2];

    params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
    params[1] = (void *)[self getRowData:(int)[self selectedRow]];

    /* Handler for container class */
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_ITEM_ENTER);
}
-(void)keyUp:(NSEvent *)theEvent
{
    if([[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        void *params[2];

        params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
        params[1] = (void *)[self getRowData:(int)[self selectedRow]];

        _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_ITEM_ENTER);
    }
    [super keyUp:theEvent];
}

-(void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn
{
    NSUInteger index = [tvcols indexOfObject:tableColumn];

    /* Handler for column click class */
    _dw_event_handler(self, (NSEvent *)index, _DW_EVENT_COLUMN_CLICK);
}
-(void)selectionChanged:(id)sender
{
    void *params[2];

    params[0] = (void *)[self getRowTitle:(int)[self selectedRow]];
    params[1] = (void *)[self getRowData:(int)[self selectedRow]];

    /* Handler for container class */
    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_ITEM_SELECT);
    /* Handler for listbox class */
    _dw_event_handler(self, DW_INT_TO_POINTER((int)[self selectedRow]), _DW_EVENT_LIST_SELECT);
}
-(NSMenu *)menuForEvent:(NSEvent *)event
{
    NSPoint where = [self convertPoint:[event locationInWindow] fromView:nil];
    int row = (int)[self rowAtPoint:where];
    void *params[2];

    params[0] = [self getRowTitle:row];
    params[1] = [self getRowData:row];

    _dw_event_handler(self, (NSEvent *)params, _DW_EVENT_ITEM_CONTEXT);
    return nil;
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [self clear]; [scrollview release]; [super dealloc]; }
@end

/* Dive into the tree freeing all desired child nodes */
void _dw_free_tree_recurse(NSMutableArray *node, NSMutableArray *item)
{
    if(node && ([node isKindOfClass:[NSArray class]]))
    {
        int count = (int)[node count];
        NSInteger index = -1;
        int z;

        if(item)
            index = [node indexOfObject:item];

        for(z=0;z<count;z++)
        {
            NSMutableArray *pnt = [node objectAtIndex:z];
            NSMutableArray *children = nil;

            if(pnt && [pnt isKindOfClass:[NSArray class]])
            {
                children = (NSMutableArray *)[pnt objectAtIndex:3];
            }

            if(z == index)
            {
                _dw_free_tree_recurse(children, NULL);
                [node removeObjectAtIndex:z];
                count = (int)[node count];
                index = -1;
                z--;
            }
            else if(item == NULL)
            {
                NSString *oldstr = [pnt objectAtIndex:1];
                [oldstr release];
                _dw_free_tree_recurse(children, item);
            }
            else
                _dw_free_tree_recurse(children, item);
        }
    }
    if(!item)
    {
        [node release];
    }
}

/* Subclass for a Tree type */
@interface DWTree : NSOutlineView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSOutlineViewDataSource,NSOutlineViewDelegate>
#endif
{
    void *userdata;
    NSTableColumn *treecol;
    NSMutableArray *data;
    /* Each data item consists of a linked lists of tree item data.
     * NSImage *, NSString *, Item Data *, NSMutableArray * of Children
     */
    id scrollview;
    NSColor *fgcolor;
}
-(id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item;
-(BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item;
-(int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item;
#ifdef DW_USE_NSVIEW
-(NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(NSTableColumn *)tableColumn item:(id)item;
#else
-(id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item;
#endif
-(BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item;
-(void)addTree:(NSMutableArray *)item and:(NSMutableArray *)parent after:(NSMutableArray *)after;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)treeSelectionChanged:(id)sender;
-(void)treeItemExpanded:(NSNotification *)notification;
-(NSScrollView *)scrollview;
-(void)setScrollview:(NSScrollView *)input;
-(void)deleteNode:(NSMutableArray *)item;
-(void)setForegroundColor:(NSColor *)input;
-(void)clear;
@end

@implementation DWTree
-(id)init
{
    self = [super init];

    if (self)
    {
        treecol = [[NSTableColumn alloc] initWithIdentifier:@"_DWTreeColumn"];
#ifndef DW_USE_NSVIEW
        DWImageAndTextCell *browsercell = [[[DWImageAndTextCell alloc] init] autorelease];
        [treecol setDataCell:browsercell];
#endif
        [self addTableColumn:treecol];
        [self setOutlineTableColumn:treecol];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(treeSelectionChanged:) name:NSOutlineViewSelectionDidChangeNotification object:self];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(treeItemExpanded:) name:NSOutlineViewItemDidExpandNotification object:self];
    }
    return self;
}
-(id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
    if(item)
    {
        NSMutableArray *array = [item objectAtIndex:3];
        return ([array isKindOfClass:[NSNull class]]) ? nil : [array objectAtIndex:index];
    }
    else
    {
        return [data objectAtIndex:index];
    }
}
-(BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
    return [self outlineView:outlineView numberOfChildrenOfItem:item] != 0;
}
-(int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    if(item)
    {
        if([item isKindOfClass:[NSMutableArray class]])
        {
            NSMutableArray *array = [item objectAtIndex:3];
            return ([array isKindOfClass:[NSNull class]]) ? 0 : (int)[array count];
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return data ? (int)[data count] : 0;
    }
}
#ifdef DW_USE_NSVIEW
-(NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    NSTableCellView *view = [outlineView makeViewWithIdentifier:[tableColumn identifier] owner:self];
    
    if([item isKindOfClass:[NSMutableArray class]])
    {
        NSMutableArray *this = (NSMutableArray *)item;
        NSImage *icon = [this objectAtIndex:0];
        NSString *text = [this objectAtIndex:1];
        if(![icon isKindOfClass:[NSImage class]])
            icon = nil;
        if(view)
        {
            NSTextField *tf = [view textField];
            NSImageView *iv = [view imageView];
            
            if(tf)
            {
                [tf setStringValue: text];
                if(fgcolor)
                    [tf setTextColor:fgcolor];
            }
            if(iv)
                [iv setImage:icon];
        }
        else
            view = _dw_table_cell_view_new(icon, text);
    }
    _dw_table_cell_view_layout(view);
    return view;
}
#else
-(id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if(item)
    {
        if([item isKindOfClass:[NSMutableArray class]])
        {
            NSMutableArray *this = (NSMutableArray *)item;
            return [this objectAtIndex:1];
        }
        else
        {
            return nil;
        }
    }
    return @"List Root";
}
-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if([cell isMemberOfClass:[DWImageAndTextCell class]])
    {
        NSMutableArray *this = (NSMutableArray *)item;
        NSImage *img = [this objectAtIndex:0];
        if([img isKindOfClass:[NSImage class]])
            [(DWImageAndTextCell*)cell setImage:img];
    }
}
#endif
-(BOOL)outlineView:(NSOutlineView *)outlineView shouldEditTableColumn:(NSTableColumn *)tableColumn item:(id)item { return NO; }
-(void)addTree:(NSMutableArray *)item and:(NSMutableArray *)parent after:(NSMutableArray *)after
{
    NSMutableArray *children = data;
    if(parent)
    {
        children = [parent objectAtIndex:3];
        if([children isKindOfClass:[NSNull class]])
        {
            children = [[[NSMutableArray alloc] init] retain];
            [parent replaceObjectAtIndex:3 withObject:children];
        }
    }
    else
    {
        if(!data)
        {
            children = data = [[[NSMutableArray alloc] init] retain];
        }
    }
    if(after)
    {
        NSInteger index = [children indexOfObject:after];
        int count = (int)[children count];
        if(index != NSNotFound && (index+1) < count)
            [children insertObject:item atIndex:(index+1)];
        else
            [children addObject:item];
    }
    else
    {
        [children addObject:item];
    }
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)treeSelectionChanged:(id)sender
{
    /* Handler for tree class */
    id item = [self itemAtRow:[self selectedRow]];

    if(item)
    {
        _dw_event_handler(self, (void *)item, _DW_EVENT_ITEM_SELECT);
    }
}
-(void)treeItemExpanded:(NSNotification *)notification
{
    id item = [[notification userInfo ] objectForKey: @"NSObject"];

    if(item)
    {
        _dw_event_handler(self, (void *)item, _DW_EVENT_TREE_EXPAND);
    }
}
-(NSMenu *)menuForEvent:(NSEvent *)event
{
    int row;
    NSPoint where = [self convertPoint:[event locationInWindow] fromView:nil];
    row = (int)[self rowAtPoint:where];
    id item = [self itemAtRow:row];
    _dw_event_handler(self, (NSEvent *)item, _DW_EVENT_ITEM_CONTEXT);
    return nil;
}
-(NSScrollView *)scrollview { return scrollview; }
-(void)setScrollview:(NSScrollView *)input { scrollview = input; }
-(void)deleteNode:(NSMutableArray *)item { _dw_free_tree_recurse(data, item); }
-(void)setForegroundColor:(NSColor *)input
{
    NSTextFieldCell *cell = [treecol dataCell];
    fgcolor = input;
    [fgcolor retain];
    [cell setTextColor:fgcolor];
}
-(void)clear { NSMutableArray *toclear = data; data = nil; _dw_free_tree_recurse(toclear, NULL); [self reloadData]; }
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];

    if(vk == NSTabCharacter || vk == NSBackTabCharacter)
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
    [super keyDown:theEvent];
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
-(void)dealloc
{
    UserData *root = userdata;
    _dw_remove_userdata(&root, NULL, TRUE);
    _dw_free_tree_recurse(data, NULL);
    [treecol release];
    dw_signal_disconnect_by_window(self);
    [super dealloc];
}
@end

/* Subclass for a Calendar type */
@interface DWCalendar : NSDatePicker
{
    void *userdata;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
@end

@implementation DWCalendar
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a Combobox type */
@interface DWComboBox : NSComboBox
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSComboBoxDelegate>
#endif
{
    void *userdata;
    id clickDefault;
}
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(void)comboBoxSelectionDidChange:(NSNotification *)not;
-(void)setClickDefault:(id)input;
@end

@implementation DWComboBox
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(void)comboBoxSelectionDidChange:(NSNotification *)not { _dw_event_handler(self, (void *)[self indexOfSelectedItem], _DW_EVENT_LIST_SELECT); }
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        [[self window] makeFirstResponder:clickDefault];
    } else
    {
        [super keyUp:theEvent];
    }
}
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

/* Subclass for a stepper component of the spinbutton type */
/* This is a bad way of doing this... but I can't get the other methods to work */
@interface DWStepper : NSStepper
{
    id textfield;
    id parent;
}
-(void)setTextfield:(id)input;
-(id)textfield;
-(void)setParent:(id)input;
-(id)parent;
-(void)mouseDown:(NSEvent *)event;
-(void)mouseUp:(NSEvent *)event;
@end

@implementation DWStepper
-(void)setTextfield:(id)input { textfield = input; }
-(id)textfield { return textfield; }
-(void)setParent:(id)input { parent = input; }
-(id)parent { return parent; }
-(void)mouseDown:(NSEvent *)event
{
    [super mouseDown:event];
    if([[NSApp currentEvent] type] == DWEventTypeLeftMouseUp)
        [self mouseUp:event];
}
-(void)mouseUp:(NSEvent *)event
{
    [textfield takeIntValueFrom:self];
    _dw_event_handler(parent, (void *)[self integerValue], _DW_EVENT_VALUE_CHANGED);
}
-(void)keyDown:(NSEvent *)theEvent
{
    unichar vk = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if(vk == VK_UP || vk == VK_DOWN)
    {
        if(vk == VK_UP)
            [self setIntegerValue:([self integerValue]+[self increment])];
        else
            [self setIntegerValue:([self integerValue]-[self increment])];
        [self mouseUp:theEvent];
    }
    else
    {
        [self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
        [super keyDown:theEvent];
    }
}
-(void)insertTab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectNextKeyView:self]; }
-(void)insertBacktab:(id)sender { if([[self window] firstResponder] == self) [[self window] selectPreviousKeyView:self]; }
@end

/* Subclass for a Spinbutton type */
@interface DWSpinButton : NSView
#ifdef BUILDING_FOR_SNOW_LEOPARD
<NSTextFieldDelegate>
#endif
{
    void *userdata;
    NSTextField *textfield;
    DWStepper *stepper;
    id clickDefault;
}
-(id)init;
-(void *)userdata;
-(void)setUserdata:(void *)input;
-(NSTextField *)textfield;
-(NSStepper *)stepper;
-(void)controlTextDidChange:(NSNotification *)aNotification;
-(void)setClickDefault:(id)input;
@end

@implementation DWSpinButton
-(id)init
{
    self = [super init];

    if(self)
    {
        textfield = [[[NSTextField alloc] init] autorelease];
        /* Workaround for infinite loop in Snow Leopard 10.6 */
        if(DWOSMajor == 10 && DWOSMinor < 7)
            [textfield setFrameSize:NSMakeSize(10,10)];
        [self addSubview:textfield];
        stepper = [[[DWStepper alloc] init] autorelease];
        [self addSubview:stepper];
        [stepper setParent:self];
        [stepper setTextfield:textfield];
        [textfield takeIntValueFrom:stepper];
        [textfield setDelegate:self];
    }
    return self;
}
-(void *)userdata { return userdata; }
-(void)setUserdata:(void *)input { userdata = input; }
-(NSTextField *)textfield { return textfield; }
-(NSStepper *)stepper { return stepper; }
-(void)controlTextDidChange:(NSNotification *)aNotification
{
    [stepper takeIntValueFrom:textfield];
    [textfield takeIntValueFrom:stepper];
    _dw_event_handler(self, (void *)[stepper integerValue], _DW_EVENT_VALUE_CHANGED);
}
-(void)setClickDefault:(id)input { clickDefault = input; }
-(void)keyUp:(NSEvent *)theEvent
{
    if(clickDefault && [[theEvent charactersIgnoringModifiers] characterAtIndex:0] == VK_RETURN)
    {
        [[self window] makeFirstResponder:clickDefault];
    }
    else
    {
        [super keyUp:theEvent];
    }
}
-(void)performClick:(id)sender { [textfield performClick:sender]; }
-(void)dealloc { UserData *root = userdata; _dw_remove_userdata(&root, NULL, TRUE); dw_signal_disconnect_by_window(self); [super dealloc]; }
@end

#ifdef BUILDING_FOR_MOJAVE
API_AVAILABLE(macos(10.14))
@interface DWUserNotificationCenterDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation DWUserNotificationCenterDelegate
/* Called when a notification is delivered to a foreground app. */
-(void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler API_AVAILABLE(macos(10.14))
{
    completionHandler(UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge);
}
/* Called to let your app know which action was selected by the user for a given notification. */
-(void)userNotificationCenter:(UNUserNotificationCenter *)center didReceiveNotificationResponse:(UNNotificationResponse *)response withCompletionHandler:(void(^)(void))completionHandler API_AVAILABLE(macos(10.14))
{
    NSScanner *objScanner = [NSScanner scannerWithString:response.notification.request.identifier];
    unsigned long long handle;
    HWND notification;

    /* Skip the dw-notification- prefix */
    [objScanner scanString:@"dw-notification-" intoString:nil];
    [objScanner scanUnsignedLongLong:&handle];
    notification = DW_UINT_TO_POINTER(handle);
    
    if ([response.actionIdentifier isEqualToString:UNNotificationDismissActionIdentifier])
    {
        /* The user dismissed the notification without taking action. */
        dw_signal_disconnect_by_window(notification);
    }
    else if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier])
    {
        /* The user launched the app. */
        _dw_event_handler(notification, nil, _DW_EVENT_CLICKED);
        dw_signal_disconnect_by_window(notification);
    }
    completionHandler();
}
@end
#endif

/* Subclass for a MDI type
 * This is just a box for display purposes... but it is a
 * unique class so it can be identified when creating windows.
 */
@interface DWMDI : DWBox {}
@end

@implementation DWMDI
@end

/* This function adds a signal handler callback into the linked list.
 */
void _dw_new_signal(ULONG message, HWND window, int msgid, void *signalfunction, void *discfunc, void *data)
{
    DWSignalHandler *new = malloc(sizeof(DWSignalHandler));

    new->message = message;
    new->window = window;
    new->id = msgid;
    new->signalfunction = signalfunction;
    new->discfunction = discfunc;
    new->data = data;
    new->next = NULL;

    if (!DWRoot)
        DWRoot = new;
    else
    {
        DWSignalHandler *prev = NULL, *tmp = DWRoot;
        while(tmp)
        {
            if(tmp->message == message &&
               tmp->window == window &&
               tmp->id == msgid &&
               tmp->signalfunction == signalfunction)
            {
                tmp->data = data;
                free(new);
                return;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if(prev)
            prev->next = new;
        else
            DWRoot = new;
    }
}

/* Finds the message number for a given signal name */
ULONG _dw_findsigmessage(const char *signame)
{
    int z;

    for(z=0;z<(_DW_EVENT_MAX-1);z++)
    {
        if(strcasecmp(signame, DWSignalTranslate[z].name) == 0)
            return DWSignalTranslate[z].message;
    }
    return 0L;
}

unsigned long _dw_foreground = 0xAAAAAA, _dw_background = 0;

void _dw_handle_resize_events(Box *thisbox)
{
    int z;

    for(z=0;z<thisbox->count;z++)
    {
        id handle = thisbox->items[z].hwnd;

        if(thisbox->items[z].type == _DW_TYPE_BOX)
        {
            Box *tmp = (Box *)[handle box];

            if(tmp)
            {
                _dw_handle_resize_events(tmp);
            }
        }
        else
        {
            if([handle isMemberOfClass:[DWRender class]])
            {
                DWRender *render = (DWRender *)handle;
                NSSize oldsize = [render size];
                NSSize newsize = [render frame].size;

                /* The 10.14 appkit warns this property does nothing... not sure
                 * how long that has been the case but just removing it for Mojave.
                 */
#ifndef BUILDING_FOR_MOJAVE
                NSWindow *window = [render window];

                if([window preferredBackingLocation] != NSWindowBackingLocationVideoMemory)
                {
                    [window setPreferredBackingLocation:NSWindowBackingLocationVideoMemory];
                }
#endif

                /* Eliminate duplicate configure requests */
                if(oldsize.width != newsize.width || oldsize.height != newsize.height)
                {
                    if(newsize.width > 0 && newsize.height > 0)
                    {
                        [render setSize:newsize];
                        _dw_event_handler(handle, nil, _DW_EVENT_CONFIGURE);
                    }
                }
            }
            /* Special handling for notebook controls */
            else if([handle isMemberOfClass:[DWNotebook class]])
            {
                DWNotebook *notebook = (DWNotebook *)handle;
                DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
                id view = [notepage view];

                if([view isMemberOfClass:[DWBox class]])
                {
                    Box *box = (Box *)[view box];
                    _dw_handle_resize_events(box);
                }
            }
            /* Handle laying out scrollviews... if required space is less
             * than available space, then expand.  Otherwise use required space.
             */
            else if([handle isMemberOfClass:[DWScrollBox class]])
            {
                DWScrollBox *scrollbox = (DWScrollBox *)handle;
                DWBox *contentbox = [scrollbox documentView];
                Box *thisbox = [contentbox box];

                /* Get the required space for the box */
                _dw_handle_resize_events(thisbox);
            }
        }
    }
}

/* This function calculates how much space the widgets and boxes require
 * and does expansion as necessary.
 */
static void _dw_resize_box(Box *thisbox, int *depth, int x, int y, int pass)
{
    /* Current item position */
    int z, currentx = thisbox->pad, currenty = thisbox->pad;
    /* Used x, y and padding maximum values...
     * These will be used to find the widest or
     * tallest items in a box.
     */
    int uymax = 0, uxmax = 0;
    int upymax = 0, upxmax = 0;

    /* Reset the box sizes */
    thisbox->minwidth = thisbox->minheight = thisbox->usedpadx = thisbox->usedpady = thisbox->pad * 2;

    /* Handle special groupbox case */
    if(thisbox->grouphwnd)
    {
        /* Only calculate the size on the first pass...
         * use the cached values on second.
         */
        if(pass == 1)
        {
            DWGroupBox *groupbox = thisbox->grouphwnd;
            NSSize borderSize = [groupbox borderSize];
            NSRect titleRect;

            if(borderSize.width == 0 || borderSize.height == 0)
            {
                borderSize = [groupbox initBorder];
            }
            /* Get the title size for a more accurate groupbox padding size */
            titleRect = [groupbox titleRect];

            thisbox->grouppadx = borderSize.width;
            thisbox->grouppady = borderSize.height + titleRect.size.height;
        }

        thisbox->minwidth += thisbox->grouppadx;
        thisbox->usedpadx += thisbox->grouppadx;
        thisbox->minheight += thisbox->grouppady;
        thisbox->usedpady += thisbox->grouppady;
    }

    /* Count up all the space for all items in the box */
    for(z=0;z<thisbox->count;z++)
    {
        int itempad, itemwidth, itemheight;

        if(thisbox->items[z].type == _DW_TYPE_BOX)
        {
            id box = thisbox->items[z].hwnd;
            Box *tmp = (Box *)[box box];

            if(tmp)
            {
                /* On the first pass calculate the box contents */
                if(pass == 1)
                {
                    (*depth)++;

                    /* Save the newly calculated values on the box */
                    _dw_resize_box(tmp, depth, x, y, pass);

                    /* Duplicate the values in the item list for use below */
                    thisbox->items[z].width = tmp->minwidth;
                    thisbox->items[z].height = tmp->minheight;

                    /* If the box has no contents but is expandable... default the size to 1 */
                    if(!thisbox->items[z].width && thisbox->items[z].hsize)
                       thisbox->items[z].width = 1;
                    if(!thisbox->items[z].height && thisbox->items[z].vsize)
                       thisbox->items[z].height = 1;

                    (*depth)--;
                }
            }
        }

        /* Precalculate these values, since they will
         * be used used repeatedly in the next section.
         */
        itempad = thisbox->items[z].pad * 2;
        itemwidth = thisbox->items[z].width + itempad;
        itemheight = thisbox->items[z].height + itempad;

        /* Calculate the totals and maximums */
        if(thisbox->type == DW_VERT)
        {
            if(itemwidth > uxmax)
                uxmax = itemwidth;

            if(thisbox->items[z].hsize != _DW_SIZE_EXPAND)
            {
                if(itemwidth > upxmax)
                    upxmax = itemwidth;
            }
            else
            {
                if(itempad > upxmax)
                    upxmax = itempad;
            }
            thisbox->minheight += itemheight;
            if(thisbox->items[z].vsize != _DW_SIZE_EXPAND)
                thisbox->usedpady += itemheight;
            else
                thisbox->usedpady += itempad;
        }
        else
        {
            if(itemheight > uymax)
                uymax = itemheight;
            if(thisbox->items[z].vsize != _DW_SIZE_EXPAND)
            {
                if(itemheight > upymax)
                    upymax = itemheight;
            }
            else
            {
                if(itempad > upymax)
                    upymax = itempad;
            }
            thisbox->minwidth += itemwidth;
            if(thisbox->items[z].hsize != _DW_SIZE_EXPAND)
                thisbox->usedpadx += itemwidth;
            else
                thisbox->usedpadx += itempad;
        }
    }

    /* Add the maximums which were calculated in the previous loop */
    thisbox->minwidth += uxmax;
    thisbox->minheight += uymax;
    thisbox->usedpadx += upxmax;
    thisbox->usedpady += upymax;

    /* The second pass is for actual placement. */
    if(pass > 1)
    {
        for(z=0;z<(thisbox->count);z++)
        {
            int height = thisbox->items[z].height;
            int width = thisbox->items[z].width;
            int itempad = thisbox->items[z].pad * 2;
            int thispad = thisbox->pad * 2;

            /* Calculate the new sizes */
            if(thisbox->items[z].hsize == _DW_SIZE_EXPAND)
            {
                if(thisbox->type == DW_HORZ)
                {
                    int expandablex = thisbox->minwidth - thisbox->usedpadx;

                    if(expandablex)
                        width = (int)(((float)width / (float)expandablex) * (float)(x - thisbox->usedpadx));
                }
                else
                    width = x - (itempad + thispad + thisbox->grouppadx);
            }
            if(thisbox->items[z].vsize == _DW_SIZE_EXPAND)
            {
                if(thisbox->type == DW_VERT)
                {
                    int expandabley = thisbox->minheight - thisbox->usedpady;

                    if(expandabley)
                        height = (int)(((float)height / (float)expandabley) * (float)(y - thisbox->usedpady));
                }
                else
                    height = y - (itempad + thispad + thisbox->grouppady);
            }

            /* If the calculated size is valid... */
            if(height > 0 && width > 0)
            {
                int pad = thisbox->items[z].pad;
                NSView *handle = thisbox->items[z].hwnd;
                NSPoint point;
                NSSize size;

                point.x = currentx + pad;
                point.y = currenty + pad;
                size.width = width;
                size.height = height;
                [handle setFrameOrigin:point];
                [handle setFrameSize:size];

                /* After placing a box... place its components */
                if(thisbox->items[z].type == _DW_TYPE_BOX)
                {
                    id box = thisbox->items[z].hwnd;
                    Box *tmp = (Box *)[box box];

                    if(tmp)
                    {
                        (*depth)++;
                        _dw_resize_box(tmp, depth, width, height, pass);
                        (*depth)--;
                    }
                }

                /* Special handling for notebook controls */
                if([handle isMemberOfClass:[DWNotebook class]])
                {
                    DWNotebook *notebook = (DWNotebook *)handle;
                    DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
                    id view = [notepage view];

                    if([view isMemberOfClass:[DWBox class]])
                    {
                        Box *box = (Box *)[view box];
                        NSSize size = [view frame].size;
                        _dw_do_resize(box, size.width, size.height);
                        _dw_handle_resize_events(box);
                    }
                }
                /* Handle laying out scrollviews... if required space is less
                 * than available space, then expand.  Otherwise use required space.
                 */
                else if([handle isMemberOfClass:[DWScrollBox class]])
                {
                    int depth = 0;
                    DWScrollBox *scrollbox = (DWScrollBox *)handle;
                    DWBox *contentbox = [scrollbox documentView];
                    Box *thisbox = [contentbox box];
                    NSSize contentsize = [scrollbox contentSize];

                    /* Get the required space for the box */
                    _dw_resize_box(thisbox, &depth, x, y, 1);

                    if(contentsize.width < thisbox->minwidth)
                    {
                        contentsize.width = thisbox->minwidth;
                    }
                    if(contentsize.height < thisbox->minheight)
                    {
                        contentsize.height = thisbox->minheight;
                    }
                    [contentbox setFrameSize:contentsize];

                    /* Layout the content of the scrollbox */
                    _dw_do_resize(thisbox, contentsize.width, contentsize.height);
                    _dw_handle_resize_events(thisbox);
                }
                /* Special handling for spinbutton controls */
                else if([handle isMemberOfClass:[DWSpinButton class]])
                {
                    DWSpinButton *spinbutton = (DWSpinButton *)handle;
                    NSTextField *textfield = [spinbutton textfield];
                    NSStepper *stepper = [spinbutton stepper];
                    [textfield setFrameOrigin:NSMakePoint(0,0)];
                    [textfield setFrameSize:NSMakeSize(size.width-20,size.height)];
                    [stepper setFrameOrigin:NSMakePoint(size.width-20,0)];
                    [stepper setFrameSize:NSMakeSize(20,size.height)];
                }
                else if([handle isMemberOfClass:[DWSplitBar class]])
                {
                    DWSplitBar *split = (DWSplitBar *)handle;
                    DWWindow *window = (DWWindow *)[split window];
                    float percent = [split percent];

                    if(percent > 0 && size.width > 20 && size.height > 20)
                    {
                        dw_splitbar_set(handle, percent);
                        [split setPercent:0];
                    }
                    else if([window redraw])
                    {
                        [split splitViewDidResizeSubviews:nil];
                    }
                }

                /* Advance the current position in the box */
                if(thisbox->type == DW_HORZ)
                    currentx += width + (pad * 2);
                if(thisbox->type == DW_VERT)
                    currenty += height + (pad * 2);
            }
        }
    }
}

static void _dw_do_resize(Box *thisbox, int x, int y)
{
    if(x > 0 && y > 0)
    {
        if(thisbox)
        {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, x, y, 1);

            /* Finally place all the boxes and controls */
            _dw_resize_box(thisbox, &depth, x, y, 2);
        }
    }
}

NSMenu *_dw_generate_main_menu()
{
    NSString *applicationName = nil;

    /* This only works on 10.6 so we have a backup method */
#ifdef BUILDING_FOR_SNOW_LEOPARD
    applicationName = [[NSRunningApplication currentApplication] localizedName];
#endif
    if(applicationName == nil)
    {
        applicationName = [[NSProcessInfo processInfo] processName];
    }

    /* Create the main menu */
    NSMenu * mainMenu = [[[NSMenu alloc] initWithTitle:@"MainMenu"] autorelease];

    NSMenuItem * mitem = [mainMenu addItemWithTitle:@"Apple" action:NULL keyEquivalent:@""];
    NSMenu * menu = [[[NSMenu alloc] initWithTitle:@"Apple"] autorelease];

    [DWApp performSelector:@selector(setAppleMenu:) withObject:menu];

    /* Setup the Application menu */
    NSMenuItem * item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"About", nil), applicationName]
                                        action:@selector(orderFrontStandardAboutPanel:)
                                 keyEquivalent:@""];
    [item setTarget:DWApp];

    [menu addItem:[NSMenuItem separatorItem]];

    item = [menu addItemWithTitle:NSLocalizedString(@"Services", nil)
                           action:NULL
                    keyEquivalent:@""];
    NSMenu * servicesMenu = [[[NSMenu alloc] initWithTitle:@"Services"] autorelease];
    [menu setSubmenu:servicesMenu forItem:item];
    [DWApp setServicesMenu:servicesMenu];

    [menu addItem:[NSMenuItem separatorItem]];

    item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Hide", nil), applicationName]
                           action:@selector(hide:)
                    keyEquivalent:@"h"];
    [item setTarget:DWApp];

    item = [menu addItemWithTitle:NSLocalizedString(@"Hide Others", nil)
                           action:@selector(hideOtherApplications:)
                    keyEquivalent:@"h"];
    [item setKeyEquivalentModifierMask:DWEventModifierFlagCommand | DWEventModifierFlagOption];
    [item setTarget:DWApp];

    item = [menu addItemWithTitle:NSLocalizedString(@"Show All", nil)
                           action:@selector(unhideAllApplications:)
                    keyEquivalent:@""];
    [item setTarget:DWApp];

    [menu addItem:[NSMenuItem separatorItem]];

    item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ %@", NSLocalizedString(@"Quit", nil), applicationName]
                           action:@selector(terminate:)
                    keyEquivalent:@"q"];
    [item setTarget:DWApp];

    [mainMenu setSubmenu:menu forItem:mitem];

    return mainMenu;
}

/*
 * Runs a message loop for Dynamic Windows.
 */
void API dw_main(void)
{
#ifndef BUILDING_FOR_MOJAVE
    dw_mutex_lock(DWRunMutex);
#endif
    DWThread = dw_thread_id();
    /* Make sure any queued redraws are handled */
    _dw_redraw(0, FALSE);
    [DWApp run];
    DWThread = (DWTID)-1;
#ifndef BUILDING_FOR_MOJAVE
    dw_mutex_unlock(DWRunMutex);
#endif
}

/*
 * Causes running dw_main() to return.
 */
void API dw_main_quit(void)
{
    [DWApp stop:nil];
    _dw_wakeup_app();
}

/*
 * Runs a message loop for Dynamic Windows, for a period of milliseconds.
 * Parameters:
 *           milliseconds: Number of milliseconds to run the loop for.
 */
void API dw_main_sleep(int milliseconds)
{
    DWTID curr = pthread_self();

    if(DWThread == (DWTID)-1 || DWThread == curr)
    {
        DWTID orig = DWThread;
        NSDate *until = [NSDate dateWithTimeIntervalSinceNow:(milliseconds/1000.0)];

        if(orig == (DWTID)-1)
        {
#ifndef BUILDING_FOR_MOJAVE
            dw_mutex_lock(DWRunMutex);
#endif
            DWThread = curr;
        }
       /* Process any pending events */
        while(_dw_main_iteration(until))
        {
            /* Just loop */
        }
        if(orig == (DWTID)-1)
        {
            DWThread = orig;
#ifndef BUILDING_FOR_MOJAVE
            dw_mutex_unlock(DWRunMutex);
#endif
        }
    }
    else
    {
        usleep(milliseconds * 1000);
    }
}

/* Internal version that doesn't lock the run mutex */
int _dw_main_iteration(NSDate *date)
{
    NSEvent *event = [DWApp nextEventMatchingMask:DWEventMaskAny
                                        untilDate:date
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if(event)
    {
        [DWApp sendEvent:event];
        [DWApp updateWindows];
        return 1;
    }
    return 0;
}

/*
 * Processes a single message iteration and returns.
 */
void API dw_main_iteration(void)
{
    DWTID curr = pthread_self();

    if(DWThread == (DWTID)-1)
    {
#ifndef BUILDING_FOR_MOJAVE
        dw_mutex_lock(DWRunMutex);
#endif
        DWThread = curr;
        _dw_main_iteration([NSDate distantPast]);
        DWThread = (DWTID)-1;
#ifndef BUILDING_FOR_MOJAVE
        dw_mutex_unlock(DWRunMutex);
#endif
    }
    else if(DWThread == curr)
    {
        _dw_main_iteration([NSDate distantPast]);
    }
}

/*
 * Cleanly terminates a DW session, should be signal handler safe.
 */
void API dw_shutdown(void)
{
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
#endif
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
 * Returns a pointer to a static buffer which containes the
 * current user directory.  Or the root directory (C:\ on
 * OS/2 and Windows).
 */
char * API dw_user_dir(void)
{
    static char _user_dir[PATH_MAX+1] = { 0 };

    if(!_user_dir[0])
    {
        char *home = getenv("HOME");

        if(home)
            strncpy(_user_dir, home, PATH_MAX);
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
    return _dw_bundle_path;
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
    strncpy(_dw_app_id, appid, _DW_APP_ID_SIZE);
    return DW_ERROR_NONE;
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
    dw_vdebug(format, args);
    va_end(args);
}

void API dw_vdebug(const char *format, va_list args)
{
    DW_LOCAL_POOL_IN;
    NSString *nformat = [NSString stringWithUTF8String:format];

    NSLogv(nformat, args);
    DW_LOCAL_POOL_OUT;
}

/*
 * Displays a Message Box with given text and title..
 * Parameters:
 *           title: The title of the message box.
 *           flags: flags to indicate buttons and icon
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
    DW_LOCAL_POOL_IN;
    NSInteger iResponse;
    NSString *button1 = @"OK";
    NSString *button2 = nil;
    NSString *button3 = nil;
    NSString *mtitle = [NSString stringWithUTF8String:title];
    NSString *mtext;
    NSAlertStyle mstyle = DWAlertStyleWarning;
    NSArray *params;
    int retval = 0;

    if(flags & DW_MB_OKCANCEL)
    {
        button2 = @"Cancel";
    }
    else if(flags & DW_MB_YESNO)
    {
        button1 = @"Yes";
        button2 = @"No";
    }
    else if(flags & DW_MB_YESNOCANCEL)
    {
        button1 = @"Yes";
        button2 = @"No";
        button3 = @"Cancel";
    }

    mtext = [[[NSString alloc] initWithFormat:[NSString stringWithUTF8String:format] arguments:args] autorelease];

    if(flags & DW_MB_ERROR)
        mstyle = DWAlertStyleCritical;
    else if(flags & DW_MB_INFORMATION)
        mstyle = DWAlertStyleInformational;

    params = [NSMutableArray arrayWithObjects:mtitle, mtext, [NSNumber numberWithInteger:mstyle], button1, button2, button3, nil];
    [DWObj safeCall:@selector(messageBox:) withObject:params];
    iResponse = [[params lastObject] integerValue];

    switch(iResponse)
    {
        case NSAlertFirstButtonReturn:    /* user pressed OK */
            if(flags & DW_MB_YESNO || flags & DW_MB_YESNOCANCEL)
                retval = DW_MB_RETURN_YES;
            else
                retval = DW_MB_RETURN_OK;
            break;
        case NSAlertSecondButtonReturn:  /* user pressed Cancel */
            if(flags & DW_MB_OKCANCEL)
                retval = DW_MB_RETURN_CANCEL;
            else
                retval = DW_MB_RETURN_NO;
            break;
        case NSAlertThirdButtonReturn:      /* user pressed the third button */
            retval = DW_MB_RETURN_CANCEL;
            break;
    }
    DW_LOCAL_POOL_OUT;
    return retval;
}

/*
 * Opens a file dialog and queries user selection.
 * Parameters:
 *       title: Title bar text for dialog.
 *       defpath: The default path of the open dialog.
 *       ext: Default file extention.
 *       flags: DW_FILE_OPEN or DW_FILE_SAVE.
 * Returns:
 *       NULL on error. A malloced buffer containing
 *       the file path on success.
 *
 */
char * API dw_file_browse(const char *title, const char *defpath, const char *ext, int flags)
{
    char temp[PATH_MAX+1];
    char *file = NULL, *path = NULL;
    DW_LOCAL_POOL_IN;

    /* Figure out path information...
     * These functions are only support in Snow Leopard and later...
     */
    if(defpath && *defpath && (DWOSMinor > 5 || DWOSMajor > 10))
    {
        struct stat buf;

        /* Get an absolute path */
        if(!realpath(defpath, temp))
            strcpy(temp, defpath);

        /* Check if the defpath exists */
        if(stat(temp, &buf) == 0)
        {
            /* Can be a directory or file */
            if(buf.st_mode & S_IFDIR)
                path = temp;
            else
                file = temp;
        }
        /* If it wasn't a directory... check if there is a path */
        if(!path && strchr(temp, '/'))
        {
            unsigned long x = strlen(temp) - 1;

            /* Trim off the filename */
            while(x > 0 && temp[x] != '/')
            {
                x--;
            }
            if(temp[x] == '/')
            {
                temp[x] = 0;
                /* Check to make sure the trimmed piece is a directory */
                if(stat(temp, &buf) == 0)
                {
                    if(buf.st_mode & S_IFDIR)
                    {
                        /* We now have it split */
                        path = temp;
                        file = &temp[x+1];
                    }
                }
            }
        }
    }

    if(flags == DW_FILE_OPEN || flags == DW_DIRECTORY_OPEN)
    {
        /* Create the File Open Dialog class. */
        NSOpenPanel* openDlg = [NSOpenPanel openPanel];

        if(path)
        {
            SEL ssdu = NSSelectorFromString(@"setDirectoryURL");

            if([openDlg respondsToSelector:ssdu])
            {
                DWIMP isdu = (DWIMP)[openDlg methodForSelector:ssdu];
                isdu(openDlg, ssdu, [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]]);
            }
        }

        /* Enable the selection of files in the dialog. */
        if(flags == DW_FILE_OPEN)
        {
            [openDlg setCanChooseFiles:YES];
            [openDlg setCanChooseDirectories:NO];

            /* Handle file types */
            if(ext && *ext)
            {
#ifdef BUILDING_FOR_BIG_SUR
                if (@available(macOS 11.0, *)) {
                    UTType *extuti = ext ? [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:ext]] : nil;
                    NSArray *UTIs;
                    
                    /* Try to generate a UTI for our passed extension */
                    if(extuti)
                        UTIs = [NSArray arrayWithObjects:extuti, UTTypeText, nil];
                    else
                        UTIs = @[UTTypeText];
                    [openDlg setAllowedContentTypes:UTIs];
                } else
#endif
                {
                    _DW_ELSE_AVAILABLE
                    NSArray* fileTypes = [[[NSArray alloc] initWithObjects:[NSString stringWithUTF8String:ext], nil] autorelease];
                    [openDlg setAllowedFileTypes:fileTypes];
                    _DW_END_AVAILABLE
                }
            }
        }
        else
        {
            [openDlg setCanChooseFiles:NO];
            [openDlg setCanChooseDirectories:YES];
#ifdef BUILDING_FOR_BIG_SUR
            if (@available(macOS 11.0, *)) {
                [openDlg setAllowedContentTypes:@[UTTypeFolder]];
            }
#endif
        }

        /* Disable multiple selection */
        [openDlg setAllowsMultipleSelection:NO];

        /* Display the dialog.  If the OK button was pressed,
         * process the files.
         */
        if([openDlg runModal] == DWModalResponseOK)
        {
            /* Get an array containing the full filenames of all
             * files and directories selected.
             */
            NSArray *files = [openDlg URLs];
            NSString *fileName = [[files objectAtIndex:0] path];
            if(fileName)
            {
                char *ret = strdup([ fileName UTF8String ]);
                DW_LOCAL_POOL_OUT;
                return ret;
            }
        }
    }
    else
    {
        /* Create the File Save Dialog class. */
        NSSavePanel* saveDlg = [NSSavePanel savePanel];

        if(path)
        {
            SEL ssdu = NSSelectorFromString(@"setDirectoryURL");

            if([saveDlg respondsToSelector:ssdu])
            {
                DWIMP isdu = (DWIMP)[saveDlg methodForSelector:ssdu];
                isdu(saveDlg, ssdu, [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]]);
            }
        }
        if(file)
        {
            SEL ssnfsv = NSSelectorFromString(@"setNameFieldStringValue");

            if([saveDlg respondsToSelector:ssnfsv])
            {
                DWIMP isnfsv = (DWIMP)[saveDlg methodForSelector:ssnfsv];
                isnfsv(saveDlg, ssnfsv, [NSString stringWithUTF8String:file]);
            }
        }

        /* Enable the creation of directories in the dialog. */
        [saveDlg setCanCreateDirectories:YES];

        /* Handle file types */
        if(ext && *ext)
        {
#ifdef BUILDING_FOR_BIG_SUR
            if (@available(macOS 11.0, *)) {
                UTType *extuti = ext ? [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:ext]] : nil;
                NSArray *UTIs;
                
                /* Try to generate a UTI for our passed extension */
                if(extuti)
                    UTIs = [NSArray arrayWithObjects:extuti, UTTypeText, nil];
                else
                    UTIs = @[UTTypeText];
                [saveDlg setAllowedContentTypes:UTIs];
            } else
#endif
            {
                _DW_ELSE_AVAILABLE
                NSArray* fileTypes = [[[NSArray alloc] initWithObjects:[NSString stringWithUTF8String:ext], nil] autorelease];
                [saveDlg setAllowedFileTypes:fileTypes];
                _DW_END_AVAILABLE
            }
        }

        /* Display the dialog.  If the OK button was pressed,
         * process the files.
         */
        if([saveDlg runModal] == DWModalResponseOK)
        {
            /* Get an array containing the full filenames of all
             * files and directories selected.
             */
            NSString* fileName = [[saveDlg URL] path];
            if(fileName)
            {
                char *ret = strdup([ fileName UTF8String ]);
                DW_LOCAL_POOL_OUT;
                return ret;
            }
        }
    }
    DW_LOCAL_POOL_OUT;
    return NULL;
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
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSString *str = [pasteboard stringForType:DWPasteboardTypeString];
    if(str != nil)
    {
        return strdup([ str UTF8String ]);
    }
    return NULL;
}

/*
 * Sets the contents of the default clipboard to the supplied text.
 * Parameters:
 *       Text.
 */
void API dw_clipboard_set_text(const char *str, int len)
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    SEL scc = NSSelectorFromString(@"clearContents");

    if([pasteboard respondsToSelector:scc])
    {
        DWIMP icc = (DWIMP)[pasteboard methodForSelector:scc];
        icc(pasteboard, scc);
    }

    [pasteboard setString:[ NSString stringWithUTF8String:str ] forType:DWPasteboardTypeString];
}


/*
 * Allocates and initializes a dialog struct.
 * Parameters:
 *           data: User defined data to be passed to functions.
 */
DWDialog * API dw_dialog_new(void *data)
{
    DWDialog *tmp = malloc(sizeof(DWDialog));

    if(tmp)
    {
        tmp->eve = dw_event_new();
        dw_event_reset(tmp->eve);
        tmp->data = data;
        tmp->done = FALSE;
        tmp->result = NULL;
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
    dw_event_post(dialog->eve);
    dialog->done = TRUE;
    return 0;
}

/*
 * Accepts a dialog struct waits for dw_dialog_dismiss() to be
 * called by a signal handler with the given dialog struct.
 * Parameters:
 *           dialog: Pointer to a dialog struct aquired by dw_dialog_new).
 */
void * API dw_dialog_wait(DWDialog *dialog)
{
    void *tmp = NULL;

    if(dialog)
    {
        while(!dialog->done)
        {
            _dw_main_iteration([NSDate dateWithTimeIntervalSinceNow:0.01]);
        }
        dw_event_close(&dialog->eve);
        tmp = dialog->result;
        free(dialog);
    }
    return tmp;
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
    DW_FUNCTION_INIT;
    DWBox *view = [[[DWBox alloc] init] retain];
    Box *newbox = [view box];
    memset(newbox, 0, sizeof(Box));
    newbox->pad = pad;
    newbox->type = type;
    DW_FUNCTION_RETURN_THIS(view);
}

/*
 * Create a new Group Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 *       title: Text to be displayined in the group outline.
 */
HWND API dw_groupbox_new(int type, int pad, const char *title)
{
    NSBox *groupbox = [[[DWGroupBox alloc] init] retain];
    DWBox *thisbox = dw_box_new(type, pad);
    Box *box = [thisbox box];

#ifndef BUILDING_FOR_CATALINA
    [groupbox setBorderType:NSBezelBorder];
#endif
    [groupbox setTitle:[NSString stringWithUTF8String:title]];
    box->grouphwnd = groupbox;
    [groupbox setContentView:thisbox];
    [thisbox release];
    [thisbox autorelease];
    return groupbox;
}

/*
 * Create a new scrollable Box to be packed.
 * Parameters:
 *       type: Either DW_VERT (vertical) or DW_HORZ (horizontal).
 *       pad: Number of pixels to pad around the box.
 */
HWND API dw_scrollbox_new( int type, int pad )
{
    DWScrollBox *scrollbox = [[[DWScrollBox alloc] init] retain];
    DWBox *box = dw_box_new(type, pad);
    DWBox *tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, box, 1, 1, TRUE, TRUE, 0);
    [scrollbox setHasVerticalScroller:YES];
    [scrollbox setHasHorizontalScroller:YES];
    [scrollbox setBorderType:NSNoBorder];
    [scrollbox setDrawsBackground:NO];
    [scrollbox setBox:box];
    [scrollbox setDocumentView:tmpbox];
    [tmpbox release];
    [tmpbox autorelease];
    return scrollbox;
}

/*
 * Returns the position of the scrollbar in the scrollbox
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_pos(HWND handle, int orient)
{
    DWScrollBox *scrollbox = handle;
    NSView *view = [scrollbox documentView];
    NSSize contentsize = [scrollbox contentSize];
    NSScroller *scrollbar;
    int range = 0;
    int val = 0;
    if(orient == DW_VERT)
    {
        scrollbar = [scrollbox verticalScroller];
        range = [view bounds].size.height - contentsize.height;
    }
    else
    {
        scrollbar = [scrollbox horizontalScroller];
        range = [view bounds].size.width - contentsize.width;
    }
    if(range > 0)
    {
        val = [scrollbar floatValue] * range;
    }
    return val;
}

/*
 * Gets the range for the scrollbar in the scrollbox.
 * Parameters:
 *          handle: Handle to the scrollbox to be queried.
 *          orient: The vertical or horizontal scrollbar.
 */
int API dw_scrollbox_get_range(HWND handle, int orient)
{
    DWScrollBox *scrollbox = handle;
    NSView *view = [scrollbox documentView];
    int range = 0;
    if(orient == DW_VERT)
    {
        range = [view bounds].size.height;
    }
    else
    {
        range = [view bounds].size.width;
    }
    return range;
}

/* Return the handle to the text object */
id _dw_text_handle(id object)
{
    if([object isMemberOfClass:[ DWSpinButton class]])
    {
        DWSpinButton *spinbutton = object;
        object = [spinbutton textfield];
    }
    if([object isMemberOfClass:[ NSBox class]])
    {
        NSBox *box = object;
        id content = [box contentView];

        if([content isMemberOfClass:[ DWText class]])
        {
            object = content;
        }
    }
    return object;
}

/* Internal function to calculate the widget's required size..
 * These are the general rules for widget sizes:
 *
 * Render/Unspecified: 1x1
 * Scrolled(Container,Tree,MLE): Guessed size clamped to min and max in dw.h
 * Entryfield/Combobox/Spinbutton: 150x(maxfontheight)
 * Spinbutton: 50x(maxfontheight)
 * Text/Status: (textwidth)x(textheight)
 * Ranged: 100x14 or 14x100 for vertical.
 * Buttons/Bitmaps: Size of text or image and border.
*/
void _dw_control_size(id handle, int *width, int *height)
{
    int thiswidth = 1, thisheight = 1, extrawidth = 0, extraheight = 0;
    NSString *nsstr = nil;
    id object = _dw_text_handle(handle);

    /* Handle all the different button types */
    if([object isKindOfClass:[NSButton class]])
    {
        switch([object buttonType])
        {
            case DWButtonTypeSwitch:
            case DWButtonTypeRadio:
                extrawidth = 24;
                extraheight = 4;
                nsstr = [object title];
                break;
            default:
            {
                NSImage *image = [object image];

                if(image)
                {
                    /* Image button */
                    NSSize size = [image size];
                    thiswidth = (int)size.width;
                    thisheight = (int)size.height;
                    if([object isBordered])
                    {
                        extrawidth = 4;
                        extraheight = 4;
                    }
                }
                else
                {
                    /* Text button */
                    nsstr = [object title];

                    if([object isBordered])
                    {
                        extrawidth = 30;
                        extraheight = 8;
                    }
                    else
                    {
                        extrawidth = 8;
                        extraheight = 4;
                    }
                }
                break;
            }
        }
    }
    /* If the control is an entryfield set width to 150 */
    else if([object isKindOfClass:[NSTextField class]])
    {
        NSFont *font = [object font];

        if([object isEditable])
        {
            /* Spinbuttons don't need to be as wide */
            if([handle isMemberOfClass:[DWSpinButton class]])
                thiswidth = 50;
            else
                thiswidth = 150;
            /* Comboboxes need some extra height for the border */
            if([handle isMemberOfClass:[DWComboBox class]])
                extraheight = 4;
            /* Yosemite and higher requires even more border space */
            if(DWOSMinor > 9 || DWOSMajor > 10)
                extraheight += 4;
        }
        else
            nsstr = [object stringValue];

        if(font)
            thisheight = (int)[font boundingRectForFont].size.height;
    }
    /* Handle the ranged widgets */
    else if([object isMemberOfClass:[DWPercent class]] ||
            [object isMemberOfClass:[DWSlider class]])
    {
        thiswidth = 100;
        thisheight = 20;
    }
    /* Handle the ranged widgets */
    else if([object isMemberOfClass:[DWScrollbar class]])
    {
        if([object vertical])
        {
            thiswidth = 14;
            thisheight = 100;
        }
        else
        {
            thiswidth = 100;
            thisheight = 14;
        }
    }
    /* Handle bitmap size */
    else if([object isMemberOfClass:[NSImageView class]])
    {
        NSImage *image = [object image];

        if(image)
        {
            NSSize size = [image size];
            thiswidth = (int)size.width;
            thisheight = (int)size.height;
        }
    }
    /* Handle calendar */
    else if([object isMemberOfClass:[DWCalendar class]])
    {
        NSCell *cell = [object cell];

        if(cell)
        {
            NSSize size = [cell cellSize];

            thiswidth = size.width;
            thisheight = size.height;
        }
    }
    /* MLE and Container */
    else if([object isMemberOfClass:[DWMLE class]] ||
            [object isMemberOfClass:[DWContainer class]])
    {
        NSSize size;

        if([object isMemberOfClass:[DWMLE class]])
        {
            NSScrollView *sv = [object scrollview];
            NSSize frame = [sv frame].size;
            BOOL hscroll = [sv hasHorizontalScroller];

            /* Make sure word wrap is off for the first part */
            if(!hscroll)
            {
                [[object textContainer] setWidthTracksTextView:NO];
                [[object textContainer] setContainerSize:[object maxSize]];
                [object setHorizontallyResizable:YES];
                [sv setHasHorizontalScroller:YES];
            }
            /* Size the text view to fit */
            [object sizeToFit];
            size = [object bounds].size;
            size.width += 2.0;
            size.height += 2.0;
            /* Re-enable word wrapping if it was on */
            if(!hscroll)
            {
                [[object textContainer] setWidthTracksTextView:YES];
                [sv setHasHorizontalScroller:NO];

                /* If the un wrapped it is beyond the bounds... */
                if(size.width > _DW_SCROLLED_MAX_WIDTH)
                {
                    NSSize max = [object maxSize];

                    /* Set the max size to the limit */
                    [object setMaxSize:NSMakeSize(_DW_SCROLLED_MAX_WIDTH, max.height)];
                    /* Recalculate the size */
                    [object sizeToFit];
                    size = [object bounds].size;
                    size.width += 2.0;
                    size.height += 2.0;
                    [object setMaxSize:max];
                }
            }
            [sv setFrameSize:frame];
            /* Take into account the horizontal scrollbar */
            if(hscroll && size.width > _DW_SCROLLED_MAX_WIDTH)
                size.height += 16.0;
        }
        else
            size = [object getsize];

        thiswidth = size.width;
        thisheight = size.height;

        if(thiswidth < _DW_SCROLLED_MIN_WIDTH)
            thiswidth = _DW_SCROLLED_MIN_WIDTH;
        if(thiswidth > _DW_SCROLLED_MAX_WIDTH)
            thiswidth = _DW_SCROLLED_MAX_WIDTH;
        if(thisheight < _DW_SCROLLED_MIN_HEIGHT)
            thisheight = _DW_SCROLLED_MIN_HEIGHT;
        if(thisheight > _DW_SCROLLED_MAX_HEIGHT)
            thisheight = _DW_SCROLLED_MAX_HEIGHT;
    }
    /* Tree */
    else if([object isMemberOfClass:[DWTree class]])
    {
        thiswidth = (int)((_DW_SCROLLED_MAX_WIDTH + _DW_SCROLLED_MIN_WIDTH)/2);
        thisheight = (int)((_DW_SCROLLED_MAX_HEIGHT + _DW_SCROLLED_MIN_HEIGHT)/2);
    }
    /* Any other control type */
    else if([object isKindOfClass:[NSControl class]])
        nsstr = [object stringValue];

    /* If we have a string...
     * calculate the size with the current font.
     */
    if(nsstr)
    {
        int textwidth, textheight;

        /* If we have an empty string, use "gT" to get the most height for the font */
        dw_font_text_extents_get(object, NULL, [nsstr length] ? (char *)[nsstr UTF8String] : "gT", &textwidth, &textheight);

        if(textheight > thisheight)
            thisheight = textheight;
        if(textwidth > thiswidth)
            thiswidth = textwidth;
    }

    /* Handle static text fields */
    if([object isKindOfClass:[NSTextField class]] && ![object isEditable])
    {
        id border = handle;

        extrawidth = 10;

        /* Handle status bar field */
        if([border isMemberOfClass:[NSBox class]])
        {
            extrawidth += 2;
            extraheight = 8;
        }
    }

    /* Set the requested sizes */
    if(width)
        *width = thiswidth + extrawidth;
    if(height)
        *height = thisheight + extraheight;
}

/* Internal box packing function called by the other 3 functions */
void _dw_box_pack(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad, char *funcname)
{
    id object = box;
    DWBox *view = box;
    DWBox *this = item;
    Box *thisbox;
    int z, x = 0;
    Item *tmpitem, *thisitem;

    /*
     * If you try and pack an item into itself VERY bad things can happen; like at least an
     * infinite loop on GTK! Lets be safe!
     */
    if(box == item)
    {
        dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Danger! Danger! Will Robinson; box and item are the same!");
        return;
    }

    /* Query the objects */
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = box;
        view = [window contentView];
    }
    else if([ object isMemberOfClass:[ DWScrollBox class ] ])
    {
        DWScrollBox *scrollbox = box;
        view = [scrollbox box];
    }

    thisbox = [view box];
    thisitem = thisbox->items;
    object = item;

    /* Query the objects */
    if([ object isMemberOfClass:[ DWContainer class ] ] ||
       [ object isMemberOfClass:[ DWTree class ] ] ||
       [ object isMemberOfClass:[ DWMLE class ] ])
    {
        this = item = [object scrollview];
    }

    /* Do some sanity bounds checking */
    if(!thisitem)
        thisbox->count = 0;
    if(index < 0)
        index = 0;
    if(index > thisbox->count)
        index = thisbox->count;

    /* Duplicate the existing data */
    tmpitem = calloc(sizeof(Item), (thisbox->count+1));

    for(z=0;z<thisbox->count;z++)
    {
        if(z == index)
            x++;
        tmpitem[x] = thisitem[z];
        x++;
    }

    /* Sanity checks */
    if(vsize && !height)
       height = 1;
    if(hsize && !width)
       width = 1;

    /* Fill in the item data appropriately */
    if([object isKindOfClass:[DWBox class]] || [object isMemberOfClass:[DWGroupBox class]])
       tmpitem[index].type = _DW_TYPE_BOX;
    else
    {
        if(width == 0 && hsize == FALSE)
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Width and expand Horizonal both unset for box: %x item: %x",box,item);
        if(height == 0 && vsize == FALSE)
            dw_messagebox(funcname, DW_MB_OK|DW_MB_ERROR, "Height and expand Vertical both unset for box: %x item: %x",box,item);

        tmpitem[index].type = _DW_TYPE_ITEM;
    }

    tmpitem[index].hwnd = item;
    tmpitem[index].origwidth = tmpitem[index].width = width;
    tmpitem[index].origheight = tmpitem[index].height = height;
    tmpitem[index].pad = pad;
    tmpitem[index].hsize = hsize ? _DW_SIZE_EXPAND : _DW_SIZE_STATIC;
    tmpitem[index].vsize = vsize ? _DW_SIZE_EXPAND : _DW_SIZE_STATIC;

    /* If either of the parameters are -1 ... calculate the size */
    if(width == DW_SIZE_AUTO || height ==  DW_SIZE_AUTO)
        _dw_control_size(object, width == DW_SIZE_AUTO ? &tmpitem[index].width : NULL, height == DW_SIZE_AUTO ? &tmpitem[index].height : NULL);

    thisbox->items = tmpitem;

    /* Update the item count */
    thisbox->count++;

    /* Add the item to the box */
    [view addSubview:this];
    [this release];
    /* Enable autorelease on the item...
     * so it will get destroyed when the parent is.
     */
    [this autorelease];
    /* If we are packing a button... */
    if([this isMemberOfClass:[DWButton class]])
    {
        DWButton *button = (DWButton *)this;

        /* Save the parent box so radio
         * buttons can use it later.
         */
        [button setParent:view];
    }
    /* Queue a redraw on the top-level window */
    _dw_redraw([view window], TRUE);

    /* Free the old data */
    if(thisitem)
       free(thisitem);
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id object = handle;
    int retval = DW_ERROR_NONE;

    if([object isKindOfClass:[NSView class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if(([object isKindOfClass:[NSTableView class]] || [object isMemberOfClass:[DWMLE class]])
           && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            id window = [object window];
            Box *thisbox = [parent box];
            int z, index = -1;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            if(!thisitem)
                thisbox->count = 0;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    index = z;
            }

            if(index == -1)
                retval = DW_ERROR_GENERAL;
            else
            {
                [object retain];
                [object removeFromSuperview];
                [object retain];

                if(thisbox->count > 1)
                {
                    tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                    /* Copy all but the current entry to the new list */
                    for(z=0;z<index;z++)
                    {
                        tmpitem[z] = thisitem[z];
                    }
                    for(z=index+1;z<thisbox->count;z++)
                    {
                        tmpitem[z-1] = thisitem[z];
                    }
                }

                thisbox->items = tmpitem;
                if(thisitem)
                    free(thisitem);
                if(tmpitem)
                    thisbox->count--;
                else
                    thisbox->count = 0;
                /* Queue a redraw on the top-level window */
                _dw_redraw(window, TRUE);
            }
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWBox *parent = (DWBox *)box;
    id object = nil;

    if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
    {
        id window = [parent window];
        Box *thisbox = [parent box];

        if(thisbox && index > -1 && index < thisbox->count)
        {
            int z;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            object = thisitem[index].hwnd;

            if(object)
            {
                [object retain];
                [object removeFromSuperview];
                [object retain];
            }

            if(thisbox->count > 1)
            {
                tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                /* Copy all but the current entry to the new list */
                for(z=0;thisitem && z<index;z++)
                {
                    tmpitem[z] = thisitem[z];
                }
                for(z=index+1;z<thisbox->count;z++)
                {
                    tmpitem[z-1] = thisitem[z];
                }
            }

            thisbox->items = tmpitem;
            if(thisitem)
                free(thisitem);
            if(tmpitem)
                thisbox->count--;
            else
                thisbox->count = 0;

            /* Queue a redraw on the top-level window */
            _dw_redraw(window, TRUE);
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(object);
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
void API dw_box_pack_at_index(HWND box, HWND item, int index, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, index, width, height, hsize, vsize, pad, "dw_box_pack_at_index()");
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
void API dw_box_pack_start(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    /* 65536 is the table limit on GTK...
     * seems like a high enough value we will never hit it here either.
     */
    _dw_box_pack(box, item, 65536, width, height, hsize, vsize, pad, "dw_box_pack_start()");
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
void API dw_box_pack_end(HWND box, HWND item, int width, int height, int hsize, int vsize, int pad)
{
    _dw_box_pack(box, item, 0, width, height, hsize, vsize, pad, "dw_box_pack_end()");
}

/* Internal function to create a basic button, used by all button types */
HWND _dw_button_new(const char *text, ULONG cid)
{
    DWButton *button = [[[DWButton alloc] init] retain];
    if(text)
    {
        [button setTitle:[ NSString stringWithUTF8String:text ]];
    }
    [button setTarget:button];
    [button setAction:@selector(buttonClicked:)];
    [button setTag:cid];
    [button setButtonType:DWButtonTypeMomentaryPushIn];
    [button setBezelStyle:DWBezelStyleRegularSquare];
    /* TODO: Reenable scaling in the future if it is possible on other platforms.
    [[button cell] setImageScaling:NSImageScaleProportionallyDown]; */
    if(DWDefaultFont)
    {
        [[button cell] setFont:DWDefaultFont];
    }
    return button;
}

/*
 * Create a new button window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_button_new(const char *text, ULONG cid)
{
    DWButton *button = _dw_button_new(text, cid);
    [button setButtonType:DWButtonTypeMomentaryPushIn];
    [button setBezelStyle:DWBezelStyleRounded];
    [button setImagePosition:NSNoImage];
    [button setAlignment:DWTextAlignmentCenter];
    [[button cell] setControlTint:NSBlueControlTint];
    return button;
}

/*
 * Create a new Entryfield window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_new(const char *text, ULONG cid)
{
    DWEntryField *entry = [[[DWEntryField alloc] init] retain];
    [entry setStringValue:[ NSString stringWithUTF8String:text ]];
    [entry setTag:cid];
    [[entry cell] setScrollable:YES];
    [[entry cell] setWraps:NO];
    return entry;
}

/*
 * Create a new Entryfield (password) window (widget) to be packed.
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_entryfield_password_new(const char *text, ULONG cid)
{
    DWEntryFieldPassword *entry = [[[DWEntryFieldPassword alloc] init] retain];
    [entry setStringValue:[ NSString stringWithUTF8String:text ]];
    [entry setTag:cid];
    [[entry cell] setScrollable:YES];
    [[entry cell] setWraps:NO];
    return entry;
}

/*
 * Sets the entryfield character limit.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          limit: Number of characters the entryfield will take.
 */
void API dw_entryfield_set_limit(HWND handle, ULONG limit)
{
    DWEntryField *entry = handle;
    DWEntryFieldFormatter *formatter = [[[DWEntryFieldFormatter alloc] init] autorelease];

    [formatter setMaximumLength:(int)limit];
    [[entry cell] setFormatter:formatter];
}

/*
 * Create a new bitmap button window (widget) to be packed.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID of a bitmap in the resource file.
 */
HWND API dw_bitmapbutton_new(const char *text, ULONG resid)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:filepath];
    DWButton *button = _dw_button_new("", resid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
}

/*
 * Create a new bitmap button window (widget) to be packed from a file.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 */
HWND API dw_bitmapbutton_new_from_file(const char *text, unsigned long cid, const char *filename)
{
    char *ext = _dw_get_image_extension(filename);

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:nstr];

    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[NSImage alloc] initWithContentsOfFile:nstr];
    }
    DWButton *button = _dw_button_new("", cid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
}

/*
 * Create a new bitmap button window (widget) to be packed from data.
 * Parameters:
 *       text: Bubble help text to be displayed.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 *       data: The contents of the image
 *            (BMP or ICO on OS/2 or Windows, XPM on Unix)
 *       len: length of str
 */
HWND API dw_bitmapbutton_new_from_data(const char *text, unsigned long cid, const char *data, int len)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *image = [[NSImage alloc] initWithData:thisdata];
    DWButton *button = _dw_button_new("", cid);
    if(image)
    {
        [button setImage:image];
    }
    if(text)
        [button setToolTip:[NSString stringWithUTF8String:text]];
    [image release];
    return button;
}

/*
 * Create a new spinbutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_spinbutton_new(const char *text, ULONG cid)
{
    DWSpinButton *spinbutton = [[[DWSpinButton alloc] init] retain];
    NSStepper *stepper = [spinbutton stepper];
    NSTextField *textfield = [spinbutton textfield];
    [stepper setIncrement:1];
    [stepper setTag:cid];
    [stepper setMinValue:-65536];
    [stepper setMaxValue:65536];
    [stepper setIntValue:atoi(text)];
    [textfield takeIntValueFrom:stepper];
    return spinbutton;
}

/*
 * Sets the spinbutton value.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          position: Current value of the spinbutton.
 */
void API dw_spinbutton_set_pos(HWND handle, long position)
{
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    NSTextField *textfield = [spinbutton textfield];
    [stepper setIntValue:(int)position];
    [textfield takeIntValueFrom:stepper];
}

/*
 * Sets the spinbutton limits.
 * Parameters:
 *          handle: Handle to the spinbutton to be set.
 *          upper: Upper limit.
 *          lower: Lower limit.
 */
void API dw_spinbutton_set_limits(HWND handle, long upper, long lower)
{
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    [stepper setMinValue:(double)lower];
    [stepper setMaxValue:(double)upper];
}

/*
 * Returns the current value of the spinbutton.
 * Parameters:
 *          handle: Handle to the spinbutton to be queried.
 */
long API dw_spinbutton_get_pos(HWND handle)
{
    DWSpinButton *spinbutton = handle;
    NSStepper *stepper = [spinbutton stepper];
    return (long)[stepper integerValue];
}

/*
 * Create a new radiobutton window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_radiobutton_new(const char *text, ULONG cid)
{
    DWButton *button = _dw_button_new(text, cid);
    [button setButtonType:DWButtonTypeRadio];
    return button;
}

/*
 * Create a new slider window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if slider is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_slider_new(int vertical, int increments, ULONG cid)
{
    DWSlider *slider = [[[DWSlider alloc] init] retain];
    [slider setMaxValue:(double)increments];
    [slider setMinValue:0];
    [slider setContinuous:YES];
    [slider setTarget:slider];
    [slider setAction:@selector(sliderChanged:)];
    [slider setTag:cid];
    return slider;
}

/*
 * Returns the position of the slider.
 * Parameters:
 *          handle: Handle to the slider to be queried.
 */
unsigned int API dw_slider_get_pos(HWND handle)
{
    DWSlider *slider = handle;
    double val = [slider doubleValue];
    return (int)val;
}

/*
 * Sets the slider position.
 * Parameters:
 *          handle: Handle to the slider to be set.
 *          position: Position of the slider withing the range.
 */
void API dw_slider_set_pos(HWND handle, unsigned int position)
{
    DWSlider *slider = handle;
    [slider setDoubleValue:(double)position];
}

/*
 * Create a new scrollbar window (widget) to be packed.
 * Parameters:
 *       vertical: TRUE or FALSE if scrollbar is vertical.
 *       increments: Number of increments available.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_scrollbar_new(int vertical, ULONG cid)
{
    DWScrollbar *scrollbar;
    if(vertical)
    {
        scrollbar = [[[DWScrollbar alloc] init] retain];
        [scrollbar setVertical:YES];
    }
    else
    {
        scrollbar = [[[DWScrollbar alloc] initWithFrame:NSMakeRect(0,0,100,5)] retain];
    }
#ifndef BUILDING_FOR_YOSEMITE
    [scrollbar setArrowsPosition:NSScrollerArrowsDefaultSetting];
#endif
    [scrollbar setRange:0.0 andVisible:0.0];
    [scrollbar setKnobProportion:1.0];
    [scrollbar setTarget:scrollbar];
    [scrollbar setAction:@selector(scrollerChanged:)];
    [scrollbar setTag:cid];
    [scrollbar setEnabled:YES];
    return scrollbar;
}

/*
 * Returns the position of the scrollbar.
 * Parameters:
 *          handle: Handle to the scrollbar to be queried.
 */
unsigned int API dw_scrollbar_get_pos(HWND handle)
{
    DWScrollbar *scrollbar = handle;
    float range = [scrollbar range];
    float fresult = [scrollbar doubleValue] * range;
    return (int)fresult;
}

/*
 * Sets the scrollbar position.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          position: Position of the scrollbar withing the range.
 */
void API dw_scrollbar_set_pos(HWND handle, unsigned int position)
{
    DWScrollbar *scrollbar = handle;
    double range = [scrollbar range];
    double visible = [scrollbar visible];
    double newpos = (double)position/(range-visible);
    [scrollbar setDoubleValue:newpos];
}

/*
 * Sets the scrollbar range.
 * Parameters:
 *          handle: Handle to the scrollbar to be set.
 *          range: Maximum range value.
 *          visible: Visible area relative to the range.
 */
void API dw_scrollbar_set_range(HWND handle, unsigned int range, unsigned int visible)
{
    DWScrollbar *scrollbar = handle;
    double knob = (double)visible/(double)range;
    [scrollbar setRange:(double)range andVisible:(double)visible];
    [scrollbar setKnobProportion:knob];
}

/*
 * Create a new percent bar window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_percent_new(ULONG cid)
{
    DWPercent *percent = [[[DWPercent alloc] init] retain];
    [percent setStyle:DWProgressIndicatorStyleBar];
    /* This is deprecated in 14.0 and does nothing in 10.15 and later...
     * So don't compile if targeting 10.15 or higher and using SDK 14 or later
     */
#if !defined(MAC_OS_VERSION_14_0) || (defined(MAC_OS_X_VERSION_MIN_REQUIRED) && defined(MAC_OS_X_VERSION_10_15) && \
    MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_15)
    [percent setBezeled:YES];
#endif
    [percent setMaxValue:100];
    [percent setMinValue:0];
    [percent incrementBy:1];
    [percent setIndeterminate:NO];
    [percent setDoubleValue:0];
    /*[percent setTag:cid]; Why doesn't this work? */
    return percent;
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
    DW_FUNCTION_INIT;
    DWPercent *percent = handle;

    /* Handle indeterminate */
    if(position == DW_PERCENT_INDETERMINATE)
    {
        [percent setIndeterminate:YES];
        [percent startAnimation:percent];
    }
    else
    {
        /* Handle normal */
        if([percent isIndeterminate])
        {
            [percent stopAnimation:percent];
            [percent setIndeterminate:NO];
        }
        [percent setDoubleValue:(double)position];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Create a new checkbox window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_checkbox_new(const char *text, ULONG cid)
{
    DWButton *button = _dw_button_new(text, cid);
    [button setButtonType:DWButtonTypeSwitch];
    [button setBezelStyle:DWBezelStyleRegularSquare];
    return button;
}

/*
 * Returns the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 */
int API dw_checkbox_get(HWND handle)
{
    DWButton *button = handle;
    if([button state])
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * Sets the state of the checkbox.
 * Parameters:
 *          handle: Handle to the checkbox to be queried.
 *          value: TRUE for checked, FALSE for unchecked.
 */
void API dw_checkbox_set(HWND handle, int value)
{
    DWButton *button = handle;
    if(value)
    {
        [button setState:DWControlStateValueOn];
    }
    else
    {
        [button setState:DWControlStateValueOff];
    }

}

/* Internal common function to create containers and listboxes */
HWND _dw_cont_new(ULONG cid, int multi)
{
    DWFocusRingScrollView *scrollview  = [[[DWFocusRingScrollView alloc] init] retain];
    DWContainer *cont = [[[DWContainer alloc] init] retain];

    [cont setScrollview:scrollview];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];
    [cont setAllowsMultipleSelection:(multi ? YES : NO)];
    [cont setAllowsColumnReordering:NO];
    [cont setDataSource:cont];
    [cont setDelegate:cont];
    [scrollview setDocumentView:cont];
    [cont setTag:cid];
    [cont autorelease];
    [cont setRowBgOdd:DW_RGB_TRANSPARENT andEven:DW_RGB_TRANSPARENT];
    return cont;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = _dw_cont_new(cid, multi);
    [cont setHeaderView:nil];
    int type = DW_CFA_STRING;
    [cont setup];
    NSTableColumn *column = [[[NSTableColumn alloc] initWithIdentifier:@"_DWListboxColumn"] autorelease];
    [column setEditable:NO];
    [cont addTableColumn:column];
    [cont addColumn:column andType:type];
    DW_FUNCTION_RETURN_THIS(cont);
}

/*
 * Appends the specified text to the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be appended to.
 *          text: Text to append into listbox.
 */
DW_FUNCTION_DEFINITION(dw_listbox_append, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_listbox_append)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo addItemWithObjectValue:[NSString stringWithUTF8String:text]];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [NSString stringWithUTF8String:text];
#ifdef DW_USE_NSVIEW
        NSArray *newrow = [NSArray arrayWithObject:_dw_table_cell_view_new(nil, nstr)];
#else
        NSArray *newrow = [NSArray arrayWithObject:nstr];
#endif

        [cont addRow:newrow];
        [cont reloadData];
        [cont optimize];
        [cont setNeedsDisplay:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Inserts the specified text into the listbox's (or combobox) entry list.
 * Parameters:
 *          handle: Handle to the listbox to be inserted into.
 *          text: Text to insert into listbox.
 *          pos: 0-based position to insert text
 */
DW_FUNCTION_DEFINITION(dw_listbox_insert, void, HWND handle, const char *text, int pos)
DW_FUNCTION_ADD_PARAM3(handle, text, pos)
DW_FUNCTION_NO_RETURN(dw_listbox_insert)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, text, const char *, pos, int)
{
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo insertItemWithObjectValue:[NSString stringWithUTF8String:text] atIndex:pos];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSString *nstr = [NSString stringWithUTF8String:text];
#ifdef DW_USE_NSVIEW
        NSArray *newrow = [NSArray arrayWithObject:_dw_table_cell_view_new(nil, nstr)];
#else
        NSArray *newrow = [NSArray arrayWithObject:nstr];
#endif

        [cont insertRow:newrow at:pos];
        [cont reloadData];
        [cont optimize];
        [cont setNeedsDisplay:YES];
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int z;

        for(z=0;z<count;z++)
        {
            [combo addItemWithObjectValue:[ NSString stringWithUTF8String:text[z] ]];
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int z;

        for(z=0;z<count;z++)
        {
            NSString *nstr = [NSString stringWithUTF8String:text[z]];
#ifdef DW_USE_NSVIEW
            NSArray *newrow = [NSArray arrayWithObjects:_dw_table_cell_view_new(nil, nstr),nil];
#else
            NSArray *newrow = [NSArray arrayWithObjects:nstr,nil];
#endif

            [cont addRow:newrow];
        }
        [cont reloadData];
        [cont optimize];
        [cont setNeedsDisplay:YES];
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo removeAllItems];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont clear];
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Returns the listbox's item count.
 * Parameters:
 *          handle: Handle to the listbox to be cleared.
 */
DW_FUNCTION_DEFINITION(dw_listbox_count, int, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_listbox_count, int)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = handle;
    int result = 0;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        result = (int)[combo numberOfItems];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        result = (int)[cont numberOfRowsInTableView:cont];
    }
    DW_FUNCTION_RETURN_THIS(result);
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo scrollItemAtIndexToTop:top];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont scrollRowToVisible:top];
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int count = (int)[combo numberOfItems];

        if(index > count)
        {
            *buffer = '\0';
        }
        else
        {
            NSString *nstr = [combo itemObjectValueAtIndex:index];
            strncpy(buffer, [ nstr UTF8String ], length - 1);
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInTableView:cont];

        if(index > count)
        {
            *buffer = '\0';
        }
        else
        {
#ifdef DW_USE_NSVIEW
            NSTableCellView *cell = [cont getRow:index and:0];
            NSString *nstr = [[cell textField] stringValue];
#else
            NSString *nstr = [cont getRow:index and:0];
#endif

            strncpy(buffer, [nstr UTF8String], length - 1);
        }
    }
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        int count = (int)[combo numberOfItems];

        if(index <= count)
        {
            [combo removeItemAtIndex:index];
            [combo insertItemWithObjectValue:[NSString stringWithUTF8String:buffer] atIndex:index];
        }
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        int count = (int)[cont numberOfRowsInTableView:cont];

        if(index <= count)
        {
            NSString *nstr = [NSString stringWithUTF8String:buffer];
#ifdef DW_USE_NSVIEW
            NSTableCellView *cell = [cont getRow:index and:0];
            
            [[cell textField] setStringValue:nstr];
#else
            [cont editCell:nstr at:index and:0];
#endif
            [cont reloadData];
            [cont optimize];
            [cont setNeedsDisplay:YES];
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    id object = handle;
    int result = -1;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        result = (int)[combo indexOfSelectedItem];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        result = (int)[cont selectedRow];
    }
    DW_FUNCTION_RETURN_THIS(result);
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
    DW_FUNCTION_INIT;
    id object = handle;
    int retval = -1;

    if([object isMemberOfClass:[DWContainer class]])
    {
        NSUInteger result;
        DWContainer *cont = handle;
        NSIndexSet *selected = [cont selectedRowIndexes];
        if( where == -1 )
           result = [selected indexGreaterThanOrEqualToIndex:0];
        else
           result = [selected indexGreaterThanIndex:where];

        if(result != NSNotFound)
        {
            retval = (int)result;
        }
    }
    DW_FUNCTION_RETURN_THIS(retval)
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;
        if(state)
            [combo selectItemAtIndex:index];
        else
            [combo deselectItemAtIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;
        NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)index];

        [cont selectRowIndexes:selected byExtendingSelection:YES];
        [selected release];
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[DWComboBox class]])
    {
        DWComboBox *combo = handle;

        [combo removeItemAtIndex:index];
    }
    else if([object isMemberOfClass:[DWContainer class]])
    {
        DWContainer *cont = handle;

        [cont removeRow:index];
        [cont reloadData];
        [cont setNeedsDisplay:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWComboBox *combo = [[[DWComboBox alloc] init] retain];
    [combo setStringValue:[NSString stringWithUTF8String:text]];
    [combo setDelegate:combo];
    [combo setTag:cid];
    DW_FUNCTION_RETURN_THIS(combo);
}

/*
 * Create a new Multiline Editbox window (widget) to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_mle_new(ULONG cid)
{
    DWMLE *mle = [[[DWMLE alloc] init] retain];
    NSScrollView *scrollview  = [[[NSScrollView alloc] init] retain];
    NSSize size = [mle maxSize];

    size.width = size.height;
    [mle setMaxSize:size];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];
    [scrollview setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [scrollview setDocumentView:mle];
    [mle setVerticallyResizable:YES];
    [mle setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [mle setScrollview:scrollview];
#ifdef BUILDING_FOR_SNOW_LEOPARD
    [mle setAutomaticQuoteSubstitutionEnabled:NO];
    [mle setAutomaticDashSubstitutionEnabled:NO];
    [mle setAutomaticTextReplacementEnabled:NO];
#endif
    /* [mle setTag:cid]; Why doesn't this work? */
    [mle autorelease];
    return mle;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWMLE *mle = handle;
    unsigned int retval;
    NSTextStorage *ts = [mle textStorage];
    NSString *nstr = [NSString stringWithUTF8String:buffer];
    NSColor *fgcolor = [ts foregroundColor];
    NSFont *font = [ts font];
    NSMutableDictionary *attributes = [[[NSMutableDictionary alloc] init] autorelease];
    [attributes setObject:(fgcolor ? fgcolor : [NSColor textColor]) forKey:NSForegroundColorAttributeName];
    if(font)
        [attributes setObject:font forKey:NSFontAttributeName];
    NSAttributedString *nastr = [[[NSAttributedString alloc] initWithString:nstr attributes:attributes] autorelease];
    NSUInteger length = [ts length];
    if(startpoint < 0)
        startpoint = 0;
    if(startpoint > length)
        startpoint = (int)length;
    [ts insertAttributedString:nastr atIndex:startpoint];
    retval = (unsigned int)strlen(buffer) + startpoint;
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Grabs text from an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be queried.
 *          buffer: Text buffer to be exported.
 *          startpoint: Point to start grabbing text.
 *          length: Amount of text to be grabbed.
 */
DW_FUNCTION_DEFINITION(dw_mle_export, void, HWND handle, char *buffer, int startpoint, int length)
DW_FUNCTION_ADD_PARAM4(handle, buffer, startpoint, length)
DW_FUNCTION_NO_RETURN(dw_mle_export)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, buffer, char *, startpoint, int, length, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    const char *tmp = [ms UTF8String];
    strncpy(buffer, tmp+startpoint, length);
    buffer[length] = '\0';
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
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger numberOfLines, index, stringLength = [ms length];

    if(bytes)
        *bytes = stringLength;
    if(lines)
    {
        for(index=0, numberOfLines=0; index < stringLength; numberOfLines++)
            index = NSMaxRange([ms lineRangeForRange:NSMakeRange(index, 0)]);

        *lines = numberOfLines;
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
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger mslength = [ms length];
    if(startpoint < 0)
        startpoint = 0;
    if(startpoint > mslength)
        startpoint = (int)mslength;
    if(startpoint + length > mslength)
        length = (int)mslength - startpoint;
    [ms deleteCharactersInRange:NSMakeRange(startpoint, length)];
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
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    [ms deleteCharactersInRange:NSMakeRange(0, length)];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the visible line of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to be positioned.
 *          line: Line to be visible.
 */
DW_FUNCTION_DEFINITION(dw_mle_set_visible, void, HWND handle, int line)
DW_FUNCTION_ADD_PARAM2(handle, line)
DW_FUNCTION_NO_RETURN(dw_mle_set_visible)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, line, int)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger numberOfLines, index, stringLength = [ms length];

    for(index=0, numberOfLines=0; index < stringLength && numberOfLines < line; numberOfLines++)
        index = NSMaxRange([ms lineRangeForRange:NSMakeRange(index, 0)]);

    if(line == numberOfLines)
    {
        [mle scrollRangeToVisible:[ms lineRangeForRange:NSMakeRange(index, 0)]];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the editablity of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it can be edited, FALSE for readonly.
 */
void API dw_mle_set_editable(HWND handle, int state)
{
    DWMLE *mle = handle;
    if(state)
    {
        [mle setEditable:YES];
    }
    else
    {
        [mle setEditable:NO];
    }
}

/*
 * Sets the word wrap state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: TRUE if it wraps, FALSE if it doesn't.
 */
void API dw_mle_set_word_wrap(HWND handle, int state)
{
    DWMLE *mle = handle;
    NSScrollView *sv = [mle scrollview];

    if(state)
    {
        NSSize newsize = NSMakeSize([sv contentSize].width,[mle maxSize].height);
        NSRect newrect = NSMakeRect(0, 0, [sv contentSize].width, 0);
        
        [[mle textContainer] setWidthTracksTextView:YES];
        [mle setFrame:newrect];
        [[mle textContainer] setContainerSize:newsize];
        [mle setHorizontallyResizable:NO];
        [sv setHasHorizontalScroller:NO];
    }
    else
    {
        [[mle textContainer] setWidthTracksTextView:NO];
        [[mle textContainer] setContainerSize:[mle maxSize]];
        [mle setHorizontallyResizable:YES];
        [sv setHasHorizontalScroller:YES];
    }
}

/*
 * Sets the word auto complete state of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE.
 *          state: Bitwise combination of DW_MLE_COMPLETE_TEXT/DASH/QUOTE
 */
void API dw_mle_set_auto_complete(HWND handle, int state)
{
#ifdef BUILDING_FOR_SNOW_LEOPARD
    DWMLE *mle = handle;
    [mle setAutomaticQuoteSubstitutionEnabled:(state & DW_MLE_COMPLETE_QUOTE ? YES : NO)];
    [mle setAutomaticDashSubstitutionEnabled:(state & DW_MLE_COMPLETE_DASH ? YES : NO)];
    [mle setAutomaticTextReplacementEnabled:(state & DW_MLE_COMPLETE_TEXT ? YES : NO)];
#endif
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
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSUInteger length = [ms length];
    if(point < 0)
        point = 0;
    if(point > length)
        point = (int)length;
    [mle setSelectedRange: NSMakeRange(point,point)];
    [mle scrollRangeToVisible:NSMakeRange(point,point)];
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
DW_FUNCTION_DEFINITION(dw_mle_search, int, HWND handle, const char *text, int point, unsigned long flags)
DW_FUNCTION_ADD_PARAM4(handle, text, point, flags)
DW_FUNCTION_RETURN(dw_mle_search, int)
DW_FUNCTION_RESTORE_PARAM4(handle, HWND, text, const char *, point, int, flags, unsigned long)
{
    DW_FUNCTION_INIT;
    DWMLE *mle = handle;
    NSTextStorage *ts = [mle textStorage];
    NSMutableString *ms = [ts mutableString];
    NSString *searchForMe = [NSString stringWithUTF8String:text];
    NSRange searchRange = NSMakeRange(point, [ms length] - point);
    NSRange range = NSMakeRange(NSNotFound, 0);
    NSUInteger options = flags ? flags : NSCaseInsensitiveSearch;
    int retval = -1;

    if(ms)
        range = [ms rangeOfString:searchForMe options:options range:searchRange];
    if(range.location == NSNotFound)
        retval = (int)range.location;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Stops redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to freeze.
 */
void API dw_mle_freeze(HWND handle)
{
    /* Don't think this is necessary */
}

/*
 * Resumes redrawing of an MLE box.
 * Parameters:
 *          handle: Handle to the MLE to thaw.
 */
void API dw_mle_thaw(HWND handle)
{
    /* Don't think this is necessary */
}

/*
 * Create a new status text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_status_text_new(const char *text, ULONG cid)
{
    NSBox *border = [[[NSBox alloc] init] retain];
    NSTextField *textfield = dw_text_new(text, cid);

#ifndef BUILDING_FOR_CATALINA
    [border setBorderType:NSGrooveBorder];
#endif
    [border setTitlePosition:NSNoTitle];
    [border setContentView:textfield];
    [textfield release];
    [border setContentViewMargins:NSMakeSize(1.5,1.5)];
    [textfield autorelease];
    [textfield setBackgroundColor:[NSColor clearColor]];
    [[textfield cell] setVCenter:YES];
    return border;
}

/*
 * Create a new static text window (widget) to be packed.
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_text_new(const char *text, ULONG cid)
{
    DWText *textfield = [[[DWText alloc] init] retain];
    [textfield setEditable:NO];
    [textfield setSelectable:NO];
    [textfield setBordered:NO];
    [textfield setDrawsBackground:NO];
    [textfield setStringValue:[ NSString stringWithUTF8String:text ]];
    [textfield setTag:cid];
    if(DWDefaultFont)
    {
        [[textfield cell] setFont:DWDefaultFont];
    }
    [[textfield cell] setWraps:NO];
    return textfield;
}

/*
 * Creates a rendering context widget (window) to be packed.
 * Parameters:
 *       id: An id to be used with dw_window_from_id.
 * Returns:
 *       A handle to the widget or NULL on failure.
 */
HWND API dw_render_new(unsigned long cid)
{
    DWRender *render = [[[DWRender alloc] init] retain];
    [render setTag:cid];
    return render;
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
    DW_FUNCTION_INIT;
    DWRender *render = (DWRender *)handle;

    [render setNeedsDisplay:YES];
    DW_FUNCTION_RETURN_NOTHING;
}

/* Sets the current foreground drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_foreground_set(unsigned long value)
{
    NSColor *oldcolor = pthread_getspecific(_dw_fg_color_key);
    NSColor *newcolor;
    DW_LOCAL_POOL_IN;

    _dw_foreground = _dw_get_color(value);

    newcolor = [[NSColor colorWithDeviceRed:    DW_RED_VALUE(_dw_foreground)/255.0 green:
                                                DW_GREEN_VALUE(_dw_foreground)/255.0 blue:
                                                DW_BLUE_VALUE(_dw_foreground)/255.0 alpha: 1] retain];
    pthread_setspecific(_dw_fg_color_key, newcolor);
    [oldcolor release];
    DW_LOCAL_POOL_OUT;
}

/* Sets the current background drawing color.
 * Parameters:
 *       red: red value.
 *       green: green value.
 *       blue: blue value.
 */
void API dw_color_background_set(unsigned long value)
{
    NSColor *oldcolor = pthread_getspecific(_dw_bg_color_key);
    NSColor *newcolor;
    DW_LOCAL_POOL_IN;

    if(value == DW_CLR_DEFAULT)
    {
        pthread_setspecific(_dw_bg_color_key, NULL);
    }
    else
    {
        _dw_background = _dw_get_color(value);

        newcolor = [[NSColor colorWithDeviceRed:    DW_RED_VALUE(_dw_background)/255.0 green:
                                                    DW_GREEN_VALUE(_dw_background)/255.0 blue:
                                                    DW_BLUE_VALUE(_dw_background)/255.0 alpha: 1] retain];
        pthread_setspecific(_dw_bg_color_key, newcolor);
    }
    [oldcolor release];
    DW_LOCAL_POOL_OUT;
}

/* Allows the user to choose a color using the system's color chooser dialog.
 * Parameters:
 *       value: current color
 * Returns:
 *       The selected color or the current color if cancelled.
 */
unsigned long API dw_color_choose(unsigned long value)
{
    /* Create the Color Chooser Dialog class. */
    DWColorChoose *colorDlg;
    DWDialog *dialog;
    DW_LOCAL_POOL_IN;

    if(![DWColorChoose sharedColorPanelExists])
    {
        colorDlg = (DWColorChoose *)[DWColorChoose sharedColorPanel];
        /* Set defaults for the dialog. */
        [colorDlg setContinuous:NO];
        [colorDlg setTarget:colorDlg];
        [colorDlg setAction:@selector(changeColor:)];
    }
    else
        colorDlg = (DWColorChoose *)[DWColorChoose sharedColorPanel];

    /* If someone is already waiting just return */
    if([colorDlg dialog])
    {
        DW_LOCAL_POOL_OUT;
        return value;
    }

    unsigned long tempcol = _dw_get_color(value);
    NSColor *color = [[NSColor colorWithDeviceRed: DW_RED_VALUE(tempcol)/255.0 green: DW_GREEN_VALUE(tempcol)/255.0 blue: DW_BLUE_VALUE(tempcol)/255.0 alpha: 1] retain];
    [colorDlg setColor:color];

    dialog = dw_dialog_new(colorDlg);
    [colorDlg setDialog:dialog];
    [colorDlg makeKeyAndOrderFront:nil];

    /* Wait for them to pick a color */
    color = (NSColor *)dw_dialog_wait(dialog);

    /* Figure out the value of what they returned */
    CGFloat red, green, blue;
    [color getRed:&red green:&green blue:&blue alpha:NULL];
    value = DW_RGB((int)(red * 255), (int)(green *255), (int)(blue *255));
    DW_LOCAL_POOL_OUT;
    return value;
}

/* Set the current context to be a flipped image,
 * On 10.10 or higher use CoreGraphics otherwise Graphics Port
 */
NSGraphicsContext *_dw_draw_context(NSBitmapImageRep *image)
{
#ifdef BUILDING_FOR_YOSEMITE
    return [NSGraphicsContext graphicsContextWithCGContext:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] CGContext] flipped:YES];
#else
    return [NSGraphicsContext graphicsContextWithGraphicsPort:[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort] flipped:YES];
#endif
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSBitmapImageRep *bi = nil;
    bool bCanDraw = YES;

    if(pixmap)
        bi = (id)pixmap->image;
    else
    {
        if(_dw_render_safe_check(handle))
        {
#ifdef BUILDING_FOR_MOJAVE
            if([image isMemberOfClass:[DWRender class]])
            {
                DWRender *render = image;

                bi = [render cachedDrawingRep];
            }
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
                _DWLastDrawable = handle;
#endif
        } 
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:_dw_draw_context(bi)];
    }
    if(bCanDraw == YES)
    {
        NSBezierPath* aPath = [NSBezierPath bezierPath];
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

        [aPath setLineWidth: 0.5];
        [_dw_fg_color set];

        [aPath moveToPoint:NSMakePoint(x, y)];
        [aPath stroke];
    }
    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSBitmapImageRep *bi = nil;
    bool bCanDraw = YES;

    if(pixmap)
        bi = (id)pixmap->image;
    else
    {
        if(_dw_render_safe_check(handle))
        {
#ifdef BUILDING_FOR_MOJAVE
            if([image isMemberOfClass:[DWRender class]])
            {
                DWRender *render = image;

                bi = [render cachedDrawingRep];
            }
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
                _DWLastDrawable = handle;
#endif
        }
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:_dw_draw_context(bi)];
    }
    if(bCanDraw == YES)
    {
        NSBezierPath* aPath = [NSBezierPath bezierPath];
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

        [_dw_fg_color set];

        [aPath moveToPoint:NSMakePoint(x1 + 0.5, y1 + 0.5)];
        [aPath lineToPoint:NSMakePoint(x2 + 0.5, y2 + 0.5)];
        [aPath stroke];
    }

    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSString *nstr = [ NSString stringWithUTF8String:text ];
    NSBitmapImageRep *bi = nil;
    NSFont *font = nil;
    DWRender *render;
    bool bCanDraw = YES;

    if(pixmap)
    {
        bi = (id)pixmap->image;
        font = pixmap->font;
        render = pixmap->handle;
        if(!font && [render isMemberOfClass:[DWRender class]])
        {
            font = [render font];
        }
    }
    else if(image && [image isMemberOfClass:[DWRender class]])
    {
        if(_dw_render_safe_check(handle))
        {
            render = image;
            font = [render font];
#ifdef BUILDING_FOR_MOJAVE
            bi = [render cachedDrawingRep];
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
                _DWLastDrawable = handle;
#endif
        }
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:_dw_draw_context(bi)];
    }

    if(bCanDraw == YES)
    {
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
        NSColor *_dw_bg_color = pthread_getspecific(_dw_bg_color_key);
#endif
        NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithObjectsAndKeys:_dw_fg_color, NSForegroundColorAttributeName, nil];
        if(_dw_bg_color)
            [dict setValue:_dw_bg_color forKey:NSBackgroundColorAttributeName];
        if(font)
            [dict setValue:font forKey:NSFontAttributeName];
        [nstr drawAtPoint:NSMakePoint(x, y) withAttributes:dict];
        [dict release];
    }

    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
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
void API dw_font_text_extents_get(HWND handle, HPIXMAP pixmap, const char *text, int *width, int *height)
{
    id object = handle;
    NSString *nstr;
    NSFont *font = nil;
    DW_LOCAL_POOL_IN;

    nstr = [NSString stringWithUTF8String:text];

    /* Check the pixmap for associated object or font */
    if(pixmap)
    {
        object = pixmap->handle;
        font = pixmap->font;
    }
    NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
    /* If we didn't get a font from the pixmap... try the associated object */
    if(!font && ([object isMemberOfClass:[DWRender class]] || [object isKindOfClass:[NSControl class]]))
    {
        font = [object font];
    }
    /* If we got a font... add it to the dictionary */
    if(font)
    {
        [dict setValue:font forKey:NSFontAttributeName];
    }
    /* Calculate the size of the string */
    NSSize size = [nstr sizeWithAttributes:dict];
    [dict release];
    /* Return whatever information we can */
    if(width)
    {
        *width = size.width;
    }
    if(height)
    {
        *height = size.height;
    }
    DW_LOCAL_POOL_OUT;
}

/* Internal function to create an image graphics context...
 * with or without antialiasing enabled.
 */
id _dw_create_gc(id image, bool antialiased)
{
#ifdef BUILDING_FOR_YOSEMITE
    CGContextRef  context = (CGContextRef)[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] CGContext];
    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithCGContext:context flipped:YES];
#else
    CGContextRef  context = (CGContextRef)[[NSGraphicsContext graphicsContextWithBitmapImageRep:image] graphicsPort];
    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
#endif
    [gc setShouldAntialias:antialiased];
    CGContextSetAllowsAntialiasing(context, antialiased);
    return gc;
}

/* Draw a polygon on a window (preferably a render window).
 * Parameters:
 *       handle: Handle to the window.
 *       pixmap: Handle to the pixmap. (choose only one of these)
 *       flags: DW_DRAW_FILL (1) to fill the polygon or DW_DRAW_DEFAULT (0).
 *       x: X coordinate.
 *       y: Y coordinate.
 *       width: Width of rectangle.
 *       height: Height of rectangle.
 */
DW_FUNCTION_DEFINITION(dw_draw_polygon, void, HWND handle, HPIXMAP pixmap, int flags, int npoints, int *x, int *y)
DW_FUNCTION_ADD_PARAM6(handle, pixmap, flags, npoints, x, y)
DW_FUNCTION_NO_RETURN(dw_draw_polygon)
DW_FUNCTION_RESTORE_PARAM6(handle, HWND, pixmap, HPIXMAP, flags, int, npoints, int, x, int *, y, int *)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSBitmapImageRep *bi = nil;
    bool bCanDraw = YES;
    int z;

    if(pixmap)
        bi = (id)pixmap->image;
    else
    {
        if(_dw_render_safe_check(handle))
        {
#ifdef BUILDING_FOR_MOJAVE
            if([image isMemberOfClass:[DWRender class]])
            {
                DWRender *render = image;

                bi = [render cachedDrawingRep];
            }
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
            {
                _DWLastDrawable = handle;
                [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
            }
#endif
        }
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        id gc = _dw_create_gc(bi, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }

    if(bCanDraw == YES)
    {
        NSBezierPath* aPath = [NSBezierPath bezierPath];
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

        [_dw_fg_color set];

        [aPath moveToPoint:NSMakePoint(*x + 0.5, *y + 0.5)];
        for(z=1;z<npoints;z++)
        {
            [aPath lineToPoint:NSMakePoint(x[z] + 0.5, y[z] + 0.5)];
        }
        [aPath closePath];
        if(flags & DW_DRAW_FILL)
        {
            [aPath fill];
        }
        [aPath stroke];
    }

    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSBitmapImageRep *bi = nil;
    bool bCanDraw = YES;

    if(pixmap)
        bi = (id)pixmap->image;
    else
    {
        if(_dw_render_safe_check(handle))
        {
#ifdef BUILDING_FOR_MOJAVE
            if([image isMemberOfClass:[DWRender class]])
            {
                DWRender *render = image;

                bi = [render cachedDrawingRep];
            }
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
            {
                _DWLastDrawable = handle;
                [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
            }
#endif
        }
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        id gc = _dw_create_gc(bi, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }

    if(bCanDraw == YES)
    {
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

        [_dw_fg_color set];

        if(flags & DW_DRAW_FILL)
            [NSBezierPath fillRect:NSMakeRect(x, y, width, height)];
        else
            [NSBezierPath strokeRect:NSMakeRect(x, y, width, height)];
    }

    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id image = handle;
    NSBitmapImageRep *bi = nil;
    bool bCanDraw = YES;

    if(pixmap)
        bi = (id)pixmap->image;
    else
    {
        if(_dw_render_safe_check(handle))
        {
#ifdef BUILDING_FOR_MOJAVE
            if([image isMemberOfClass:[DWRender class]])
            {
                DWRender *render = image;

                bi = [render cachedDrawingRep];
            }
#else
            if((bCanDraw = [image lockFocusIfCanDraw]) == YES)
            {
                _DWLastDrawable = handle;
                [[NSGraphicsContext currentContext] setShouldAntialias:(flags & DW_DRAW_NOAA ? NO : YES)];
            }
#endif
        }
        else
            bCanDraw = NO;
    }
    if(bi)
    {
        id gc = _dw_create_gc(bi, flags & DW_DRAW_NOAA ? NO : YES);
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:gc];
    }

    if(bCanDraw)
    {
        NSBezierPath* aPath = [NSBezierPath bezierPath];
#ifndef BUILDING_FOR_MOJAVE
        NSColor *_dw_fg_color = pthread_getspecific(_dw_fg_color_key);
#endif

        [_dw_fg_color set];

        /* Special case of a full circle/oval */
        if(flags & DW_DRAW_FULL)
        {
            [aPath appendBezierPathWithOvalInRect:NSMakeRect(x1, y1, x2 - x1, y2 - y1)];
        }
        else
        {
            double a1 = atan2((y1-yorigin), (x1-xorigin));
            double a2 = atan2((y2-yorigin), (x2-xorigin));
            double dx = xorigin - x1;
            double dy = yorigin - y1;
            double r = sqrt(dx*dx + dy*dy);

            /* Convert to degrees */
            a1 *= (180.0 / M_PI);
            a2 *= (180.0 / M_PI);

            /* Prepare to draw */
            [aPath appendBezierPathWithArcWithCenter:NSMakePoint(xorigin, yorigin)
                                              radius:r startAngle:a1 endAngle:a2];
        }
        /* If the fill flag is passed */
        if(flags & DW_DRAW_FILL)
        {
            [aPath fill];
        }
        /* Actually do the drawing */
        [aPath stroke];
    }

    if(bi)
        [NSGraphicsContext restoreGraphicsState];
#ifndef BUILDING_FOR_MOJAVE
    if(bCanDraw == YES && !pixmap)
        [image unlockFocus];
#endif
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    NSScrollView *scrollview  = [[[NSScrollView alloc] init] retain];
    DWTree *tree = [[DWTree alloc] init];

    [tree setScrollview:scrollview];
    [scrollview setBorderType:NSBezelBorder];
    [scrollview setHasVerticalScroller:YES];
    [scrollview setAutohidesScrollers:YES];

    [tree setAllowsMultipleSelection:NO];
    [tree setDataSource:tree];
    [tree setDelegate:tree];
    [scrollview setDocumentView:tree];
    [tree setHeaderView:nil];
    [tree setTag:cid];
    [tree autorelease];
    DW_FUNCTION_RETURN_THIS(tree);
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    NSString *nstr = [[NSString stringWithUTF8String:title] retain];
    NSMutableArray *treenode = [[[NSMutableArray alloc] init] retain];
    if(icon)
        [treenode addObject:icon];
    else
        [treenode addObject:[NSNull null]];
    [treenode addObject:nstr];
    [treenode addObject:[NSValue valueWithPointer:itemdata]];
    [treenode addObject:[NSNull null]];
    [tree addTree:treenode and:parent after:item];
    if(parent)
        [tree reloadItem:parent reloadChildren:YES];
    else
        [tree reloadData];
    DW_FUNCTION_RETURN_THIS(treenode);
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
HTREEITEM API dw_tree_insert(HWND handle, const char *title, HICN icon, HTREEITEM parent, void *itemdata)
{
    return dw_tree_insert_after(handle, NULL, title, icon, parent, itemdata);
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
    DW_FUNCTION_INIT;
    char *retval = NULL;
    NSMutableArray *array = (NSMutableArray *)item;
    NSString *nstr = (NSString *)[array objectAtIndex:1];
    retval = strdup([nstr UTF8String]);
    DW_FUNCTION_RETURN_THIS(retval);
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
    DW_FUNCTION_INIT;
    HTREEITEM parent;
    DWTree *tree = handle;

    parent = [tree parentForItem:item];
    DW_FUNCTION_RETURN_THIS(parent);
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    NSMutableArray *array = (NSMutableArray *)item;
    DW_LOCAL_POOL_IN;

    if(title)
    {
        NSString *oldstr = [array objectAtIndex:1];
        NSString *nstr = [[NSString stringWithUTF8String:title] retain];
        [array replaceObjectAtIndex:1 withObject:nstr];
        [oldstr release];
    }
    if(icon)
    {
        [array replaceObjectAtIndex:0 withObject:icon];
    }
#ifdef BUILDING_FOR_SNOW_LEOPARD
    NSInteger row = [tree rowForItem:item];
    [tree reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:row]
                    columnIndexes:[NSIndexSet indexSetWithIndex:0]];
#else
    [tree reloadData];
#endif
    DW_LOCAL_POOL_OUT;
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
    DW_FUNCTION_INIT;
    NSMutableArray *array = (NSMutableArray *)item;
    [array replaceObjectAtIndex:2 withObject:[NSValue valueWithPointer:itemdata]];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    void *result = NULL;
    NSMutableArray *array = (NSMutableArray *)item;
    NSValue *value = [array objectAtIndex:2];
    if(value)
        result = [value pointerValue];
    DW_FUNCTION_RETURN_THIS(result);
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    NSInteger itemIndex = [tree rowForItem:item];
    if(itemIndex > -1)
    {
        [tree selectRowIndexes:[NSIndexSet indexSetWithIndex:itemIndex] byExtendingSelection:NO];
    }
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    [tree clear];
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    [tree expandItem:item];
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    [tree collapseItem:item];
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
    DW_FUNCTION_INIT;
    DWTree *tree = handle;
    [tree deleteNode:item];
    [tree reloadData];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = _dw_cont_new(cid, multi);
    NSScrollView *scrollview = [cont scrollview];
    [scrollview setHasHorizontalScroller:YES];
    NSTableHeaderView *header = [[[NSTableHeaderView alloc] init] autorelease];
    [cont setHeaderView:header];
    [cont setTarget:cont];
    [cont setDoubleAction:@selector(doubleClicked:)];
    DW_FUNCTION_RETURN_THIS(cont);
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
    DW_FUNCTION_INIT;
    int z, retval = DW_ERROR_NONE;
    DWContainer *cont = handle;

    [cont setup];

    for(z=0;z<count;z++)
    {
        NSString *title = [NSString stringWithUTF8String:titles[z]];
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:title];
#ifdef BUILDING_FOR_YOSEMITE
        [column setTitle:title];
#else
        [[column headerCell] setStringValue:title];
#endif
#ifndef DW_USE_NSVIEW
        if(flags[z] & DW_CFA_BITMAPORICON)
        {
            NSImageCell *imagecell = [[NSImageCell alloc] init];
            [column setDataCell:imagecell];
            [imagecell release];
        }
        else if(flags[z] & DW_CFA_STRINGANDICON)
        {
            DWImageAndTextCell *browsercell = [[DWImageAndTextCell alloc] init];
            [column setDataCell:browsercell];
            [browsercell release];
        }
#endif
        /* Defaults to left justified so just handle right and center */
        if(flags[z] & DW_CFA_RIGHT)
        {
#ifndef DW_USE_NSVIEW
            [(NSCell *)[column dataCell] setAlignment:DWTextAlignmentRight];
#endif
            [(NSCell *)[column headerCell] setAlignment:DWTextAlignmentRight];
        }
        else if(flags[z] & DW_CFA_CENTER)
        {
#ifndef DW_USE_NSVIEW
            [(NSCell *)[column dataCell] setAlignment:DWTextAlignmentCenter];
#endif
            [(NSCell *)[column headerCell] setAlignment:DWTextAlignmentCenter];
        }
        [column setEditable:NO];
        [cont addTableColumn:column];
        [cont addColumn:column andType:(int)flags[z]];
        [column release];
    }
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
	
	dw_window_set_data(handle, "_dw_coltitle", newtitle);
}

/*
 * Sets up the filesystem columns, note: filesystem always has an icon/filename field.
 * Parameters:
 *          handle: Handle to the container to be configured.
 *          flags: An array of unsigned longs with column flags.
 *          titles: An array of strings with column text titles.
 *          count: The number of columns (this should match the arrays).
 */
int API dw_filesystem_setup(HWND handle, unsigned long *flags, char **titles, int count)
{
    char **newtitles = malloc(sizeof(char *) * (count + 1));
    unsigned long *newflags = malloc(sizeof(unsigned long) * (count + 1));
    char *coltitle = (char *)dw_window_get_data(handle, "_dw_coltitle");
    DWContainer *cont = handle;

    newtitles[0] = coltitle ? coltitle : "Filename";

    newflags[0] = DW_CFA_STRINGANDICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR;

    memcpy(&newtitles[1], titles, sizeof(char *) * count);
    memcpy(&newflags[1], flags, sizeof(unsigned long) * count);

    dw_container_setup(handle, newflags, newtitles, count + 1, 0);
    [cont setFilesystem:YES];

    if(coltitle)
    {
        dw_window_set_data(handle, "_dw_coltitle", NULL);
        free(coltitle);
    }
    free(newtitles);
    free(newflags);
    return DW_ERROR_NONE;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont addRows:rowcount];
    DW_FUNCTION_RETURN_THIS(cont);
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    id icon = nil, text = nil;
    int type = [cont cellType:column];
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
        lastadd = [cont lastAddPoint];

    if(data)
    {
        if(type & DW_CFA_BITMAPORICON)
        {
            icon = *((NSImage **)data);
        }
        else if(type & DW_CFA_STRING)
        {
            char *str = *((char **)data);
            text = [ NSString stringWithUTF8String:str ];
        }
        else
        {
            char textbuffer[101] = {0};

            if(type & DW_CFA_ULONG)
            {
                ULONG tmp = *((ULONG *)data);

                snprintf(textbuffer, 100, "%lu", tmp);
            }
            else if(type & DW_CFA_DATE)
            {
                struct tm curtm;
                CDATE cdate = *((CDATE *)data);

                memset( &curtm, 0, sizeof(curtm) );
                curtm.tm_mday = cdate.day;
                curtm.tm_mon = cdate.month - 1;
                curtm.tm_year = cdate.year - 1900;

                strftime(textbuffer, 100, "%x", &curtm);
            }
            else if(type & DW_CFA_TIME)
            {
                struct tm curtm;
                CTIME ctime = *((CTIME *)data);

                memset( &curtm, 0, sizeof(curtm) );
                curtm.tm_hour = ctime.hours;
                curtm.tm_min = ctime.minutes;
                curtm.tm_sec = ctime.seconds;

                strftime(textbuffer, 100, "%X", &curtm);
            }
            if(textbuffer[0])
                text = [ NSString stringWithUTF8String:textbuffer ];
        }
    }

#ifdef DW_USE_NSVIEW
    id object = [cont getRow:(row+lastadd) and:column];
    
    /* If it is a cell, change the content of the cell */
    if([object isMemberOfClass:[NSTableCellView class]])
    {
        NSTableCellView *cell = object;
        if(icon)
            [[cell imageView] setImage:icon];
        else
            [[cell textField] setStringValue:text];
    }
    else /* Otherwise replace it with a new cell */
        [cont editCell:_dw_table_cell_view_new(icon, text) at:(row+lastadd) and:column];
#else
    [cont editCell:(icon ? icon : text) at:(row+lastadd) and:column];
#endif
    [cont setNeedsDisplay:YES];
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
void API dw_container_change_item(HWND handle, int column, int row, void *data)
{
    dw_container_set_item(handle, NULL, column, row, data);
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
    dw_container_change_item(handle, column+1, row, data);
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    NSString *text = filename ? [NSString stringWithUTF8String:filename] : nil;
    int lastadd = 0;

    /* If pointer is NULL we are getting a change request instead of set */
    if(pointer)
        lastadd = [cont lastAddPoint];

#ifdef DW_USE_NSVIEW
    id object = [cont getRow:(row+lastadd) and:0];
    
    /* If it is a cell, change the content of the cell */
    if([object isMemberOfClass:[NSTableCellView class]])
    {
        NSTableCellView *cell = object;
        
        if(icon)
            [[cell imageView] setImage:icon];
        if(text)
            [[cell textField] setStringValue:text];
    }
    else /* Otherwise replace it with a new cell */
        [cont editCell:_dw_table_cell_view_new(icon, text) at:(row+lastadd) and:0];
#else
    DWImageAndTextCell *browsercell = [[[DWImageAndTextCell alloc] init] autorelease];
    [browsercell setImage:icon];
    [browsercell setStringValue:text];
    [cont editCell:browsercell at:(row+lastadd) and:0];
#endif
    [cont setNeedsDisplay:YES];
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
void API dw_filesystem_set_item(HWND handle, void *pointer, int column, int row, void *data)
{
    dw_container_set_item(handle, pointer, column+1, row, data);
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int rc;
    int flag = [cont cellType:column];
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
    return dw_container_get_column_type(handle, column+1);
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
DW_FUNCTION_DEFINITION(dw_container_set_stripe, void, HWND handle, unsigned long oddcolor, unsigned long evencolor)
DW_FUNCTION_ADD_PARAM3(handle, oddcolor, evencolor)
DW_FUNCTION_NO_RETURN(dw_container_set_stripe)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, oddcolor, unsigned long, evencolor, unsigned long)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRowBgOdd:oddcolor
              andEven:evencolor];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    if([cont filesystem])
        column++;
    NSTableColumn *col = [cont getColumn:column];

    [col setWidth:width];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRow:(row+lastadd) title:title];
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Sets the data pointer of a row in the container.
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
    DW_FUNCTION_INIT;
    DWContainer *cont = pointer;
    int lastadd = [cont lastAddPoint];
    [cont setRowData:(row+lastadd) title:data];
    DW_FUNCTION_RETURN_NOTHING;
}


/*
 * Sets the title of a row in the container.
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRow:row title:title];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the data pointer of a row in the container.
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont setRowData:row title:data];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the title of a row in the container.
 * Parameters:
 *          handle: Handle to the container window (widget).
 *          pointer: Pointer to the allocated memory in dw_container_alloc().
 *          rowcount: The number of rows to be inserted.
 */
DW_FUNCTION_DEFINITION(dw_container_insert, void, HWND handle, void *pointer, int rowcount)
DW_FUNCTION_ADD_PARAM3(handle, pointer, rowcount)
DW_FUNCTION_NO_RETURN(dw_container_insert)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, DW_UNUSED(pointer), void *, DW_UNUSED(rowcount), int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont reloadData];
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Removes all rows from a container.
 * Parameters:
 *       handle: Handle to the window (widget) to be cleared.
 *       redraw: TRUE to cause the container to redraw immediately.
 */
DW_FUNCTION_DEFINITION(dw_container_clear, void, HWND handle, int redraw)
DW_FUNCTION_ADD_PARAM2(handle, redraw)
DW_FUNCTION_NO_RETURN(dw_container_clear)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, redraw, int)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont clear];
    if(redraw)
        [cont reloadData];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int x;

    for(x=0;x<rowcount;x++)
    {
        [cont removeRow:0];
    }
    [cont reloadData];
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    NSScrollView *sv = [cont scrollview];
    NSScroller *scrollbar = [sv verticalScroller];
    int rowcount = (int)[cont numberOfRowsInTableView:cont];
    float currpos = [scrollbar floatValue];
    float change;

    /* Safety check */
    if(rowcount < 1)
    {
        return;
    }

    change = (float)rows/(float)rowcount;

    switch(direction)
    {
        case DW_SCROLL_TOP:
        {
            [scrollbar setFloatValue:0];
            break;
        }
        case DW_SCROLL_BOTTOM:
        {
            [scrollbar setFloatValue:1];
            break;
        }
        case DW_SCROLL_UP:
        {
            float newpos = currpos - change;
            if(newpos < 0)
            {
                newpos = 0;
            }
            [scrollbar setFloatValue:newpos];
            break;
        }
        case DW_SCROLL_DOWN:
        {
            float newpos = currpos + change;
            if(newpos > 1)
            {
                newpos = 1;
            }
            [scrollbar setFloatValue:newpos];
            break;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    NSIndexSet *selected = [cont selectedRowIndexes];
    NSUInteger result = [selected indexGreaterThanOrEqualToIndex:0];
    void *retval = NULL;

    if(result != NSNotFound)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result];
        else
        {
            char *temp = [cont getRowTitle:(int)result];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:(int)result];
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    int lastQueryPoint = [cont lastQueryPoint];
    NSIndexSet *selected = [cont selectedRowIndexes];
    NSUInteger result = [selected indexGreaterThanIndex:lastQueryPoint];
    void *retval = NULL;

    if(result != NSNotFound)
    {
        if(flags & DW_CR_RETDATA)
            retval = [cont getRowData:(int)result];
        else
        {
            char *temp = [cont getRowTitle:(int)result];
            if(temp)
               retval = strdup(temp);
        }
        [cont setLastQueryPoint:(int)result];
    }
    DW_FUNCTION_RETURN_THIS(retval);
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)x];

            [cont selectRowIndexes:selected byExtendingSelection:YES];
            [selected release];
            [cont scrollRowToVisible:x];
            x=count;
            break;
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Cursors the item with the data speficied, and scrolls to that item.
 * Parameters:
 *       handle: Handle to the window (widget) to be queried.
 *       data: Data associated with the row.
 */
DW_FUNCTION_DEFINITION(dw_container_cursor_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_cursor_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            NSIndexSet *selected = [[NSIndexSet alloc] initWithIndex:(NSUInteger)x];

            [cont selectRowIndexes:selected byExtendingSelection:YES];
            [selected release];
            [cont scrollRowToVisible:x];
            x=count;
            break;
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    char *thistext;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thistext = [cont getRowTitle:x];

        if(thistext && strcmp(thistext, text) == 0)
        {
            [cont removeRow:x];
            [cont reloadData];
            x=count;
            break;
        }
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Deletes the item with the data speficied.
 * Parameters:
 *       handle: Handle to the window (widget).
 *       data: Data specified.
 */
DW_FUNCTION_DEFINITION(dw_container_delete_row_by_data, void, HWND handle, void *data)
DW_FUNCTION_ADD_PARAM2(handle, data)
DW_FUNCTION_NO_RETURN(dw_container_delete_row_by_data)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, data, void *)
{
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    void *thisdata;
    int x, count = (int)[cont numberOfRowsInTableView:cont];

    for(x=0;x<count;x++)
    {
        thisdata = [cont getRowData:x];

        if(thisdata == data)
        {
            [cont removeRow:x];
            [cont reloadData];
            x=count;
            break;
        }
    }
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
    DW_FUNCTION_INIT;
    DWContainer *cont = handle;
    [cont optimize];
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
#ifdef BUILDING_FOR_YOSEMITE
    NSStatusItem *item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
    NSStatusBarButton *button = [item button];
    NSImage *image = icon;
    [button setImage:image];
    if(bubbletext)
        [button setToolTip:[NSString stringWithUTF8String:bubbletext]];
    [button setTarget:handle];
    [button setEnabled:YES];
    [[button cell] setHighlighted:YES];
    [button sendActionOn:(DWEventMaskLeftMouseUp|DWEventMaskLeftMouseDown|DWEventMaskRightMouseUp|DWEventMaskRightMouseDown)];
    [button setAction:@selector(mouseDown:)];
    dw_window_set_data(handle, "_dw_taskbar", item);
#endif
}

/*
 * Deletes an icon from the taskbar.
 * Parameters:
 *       handle: Window handle that was used with dw_taskbar_insert().
 *       icon: Icon handle that was used with dw_taskbar_insert().
 */
void API dw_taskbar_delete(HWND handle, HICN icon)
{
#ifdef BUILDING_FOR_YOSEMITE
    NSStatusItem *item = dw_window_get_data(handle, "_dw_taskbar");
    DW_LOCAL_POOL_IN;
    [item release];
    DW_LOCAL_POOL_OUT;
#endif
}

/* Internal function to keep HICNs from getting too big */
void _icon_resize(NSImage *image)
{
    if(image)
    {
        NSSize size = [image size];
        if(size.width > 24 || size.height > 24)
        {
            if(size.width > 24)
                size.width = 24;
            if(size.height > 24)
                size.height = 24;
            [image setSize:size];
        }
    }
}

/* Internal version that does not resize the image */
HICN _dw_icon_load(unsigned long resid)
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:filepath];
    return image;
}

/*
 * Obtains an icon from a module (or header in GTK).
 * Parameters:
 *          module: Handle to module (DLL) in OS/2 and Windows.
 *          id: A unsigned long id int the resources on OS/2 and
 *              Windows, on GTK this is converted to a pointer
 *              to an embedded XPM.
 */
HICN API dw_icon_load(unsigned long module, unsigned long resid)
{
    NSImage *image = _dw_icon_load(resid);
    _icon_resize(image);
    return image;
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
    char *ext = _dw_get_image_extension( filename );

    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:nstr];
    if(!image && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        image = [[NSImage alloc] initWithContentsOfFile:nstr];
    }
    _icon_resize(image);
    return image;
}

/*
 * Obtains an icon from data
 * Parameters:
 *       filename: Name of the file, omit extention to have
 *                 DW pick the appropriate file extension.
 *                 (ICO on OS/2 or Windows, XPM on Unix)
 */
HICN API dw_icon_load_from_data(const char *data, int len)
{
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *image = [[NSImage alloc] initWithData:thisdata];
    _icon_resize(image);
    return image;
}

/*
 * Frees a loaded resource in OS/2 and Windows.
 * Parameters:
 *          handle: Handle to icon returned by dw_icon_load().
 */
void API dw_icon_free(HICN handle)
{
    NSImage *image = handle;
    DW_LOCAL_POOL_IN;
    [image release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Create a new MDI Frame to be packed.
 * Parameters:
 *       id: An ID to be used with dw_window_from_id or 0L.
 */
HWND API dw_mdi_new(unsigned long cid)
{
    /* There isn't anything like quite like MDI on MacOS...
     * However we will make floating windows that hide
     * when the application is deactivated to simulate
     * similar behavior.
     */
    DWMDI *mdi = [[[DWMDI alloc] init] retain];
    /* [mdi setTag:cid]; Why doesn't this work? */
    return mdi;
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
    DW_FUNCTION_INIT;
    id tmpbox = dw_box_new(DW_VERT, 0);
    DWSplitBar *split = [[[DWSplitBar alloc] init] retain];
    [split setDelegate:split];
    dw_box_pack_start(tmpbox, topleft, 0, 0, TRUE, TRUE, 0);
    [split addSubview:tmpbox];
    [tmpbox release];
    [tmpbox autorelease];
    tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(tmpbox, bottomright, 0, 0, TRUE, TRUE, 0);
    [split addSubview:tmpbox];
    [tmpbox release];
    [tmpbox autorelease];
    if(type == DW_VERT)
    {
        [split setVertical:NO];
    }
    else
    {
        [split setVertical:YES];
    }
    /* Set the default percent to 50% split */
    [split setPercent:50.0];
    [split setTag:cid];
    DW_FUNCTION_RETURN_THIS(split);
}

/*
 * Sets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 *       percent: The position of the splitbar.
 */
DW_FUNCTION_DEFINITION(dw_splitbar_set, void, HWND handle, float percent)
DW_FUNCTION_ADD_PARAM2(handle, percent)
DW_FUNCTION_NO_RETURN(dw_splitbar_set)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, percent, float)
{
    DW_FUNCTION_INIT;
    DWSplitBar *split = handle;
    NSRect rect = [split frame];
    float pos;
    /* Calculate the position based on the size */
    if([split isVertical])
    {
        pos = rect.size.width * (percent / 100.0);
    }
    else
    {
        pos = rect.size.height * (percent / 100.0);
    }
    if(pos > 0)
    {
        [split setPosition:pos ofDividerAtIndex:0];
    }
    else
    {
        /* If we have no size.. wait until the resize
         * event when we get an actual size to try
         * to set the splitbar again.
         */
        [split setPercent:percent];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Gets the position of a splitbar (pecentage).
 * Parameters:
 *       handle: The handle to the splitbar returned by dw_splitbar_new().
 */
float API dw_splitbar_get(HWND handle)
{
    DWSplitBar *split = handle;
    NSRect rect1 = [split frame];
    NSArray *subviews = [split subviews];
    NSView *view = [subviews objectAtIndex:0];
    NSRect rect2 = [view frame];
    float pos, total, retval = 0.0;
    if([split isVertical])
    {
        total = rect1.size.width;
        pos = rect2.size.width;
    }
    else
    {
        total = rect1.size.height;
        pos = rect2.size.height;
    }
    if(total > 0)
    {
        retval = pos / total;
    }
    return retval;
}

/* Internal function to convert fontname to NSFont */
NSFont *_dw_font_by_name(const char *fontname)
{
    NSFont *font = DWDefaultFont;
    
    if(fontname)
    {
        char *name = strchr(fontname, '.');

        if(name && (name++))
        {
            int size = atoi(fontname);
            char *Italic = strstr(name, " Italic");
            char *Bold = strstr(name, " Bold");
            size_t len = (Italic ? (Bold ? (Italic > Bold ? (Bold - name) : (Italic - name)) : (Italic - name)) : (Bold ? (Bold - name) : strlen(name)));
            char *newname = alloca(len+1);

            memset(newname, 0, len+1);
            strncpy(newname, name, len);
            
            font = [DWFontManager fontWithFamily:[NSString stringWithUTF8String:newname]
                                          traits:(Italic ? NSItalicFontMask : 0)|(Bold ? NSBoldFontMask : 0)
                                          weight:5
                                            size:(float)size];
        }
    }
    return font;
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
    DW_FUNCTION_INIT;
    NSImageView *bitmap = [[[NSImageView alloc] init] retain];
    [bitmap setImageFrameStyle:NSImageFrameNone];
    [bitmap setImageScaling:NSImageScaleNone];
    [bitmap setEditable:NO];
    [bitmap setTag:cid];
    DW_FUNCTION_RETURN_THIS(bitmap);
}

/*
 * Creates a pixmap with given parameters.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       width: Width of the pixmap in pixels.
 *       height: Height of the pixmap in pixels.
 *       depth: Color depth of the pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new(HWND handle, unsigned long width, unsigned long height, int depth)
{
    HPIXMAP pixmap;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
        return NULL;
    pixmap->width = width;
    pixmap->height = height;
    pixmap->handle = handle;
    pixmap->image = [[NSBitmapImageRep alloc]
                                    initWithBitmapDataPlanes:NULL
                                    pixelsWide:width
                                    pixelsHigh:height
                                    bitsPerSample:8
                                    samplesPerPixel:4
                                    hasAlpha:YES
                                    isPlanar:NO
                                    colorSpaceName:NSDeviceRGBColorSpace
                                    bytesPerRow:0
                                    bitsPerPixel:0];
    return pixmap;
}

/* Function takes an NSImage and copies it into a flipped NSBitmapImageRep */
void _dw_flip_image(NSImage *tmpimage, NSBitmapImageRep *image, NSSize size)
{
    NSCompositingOperation op = DWCompositingOperationSourceOver;
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:_dw_draw_context(image)];
    [[[NSDictionary alloc] initWithObjectsAndKeys:image, NSGraphicsContextDestinationAttributeName, nil] autorelease];
    /* Make a new transform */
    NSAffineTransform *t = [NSAffineTransform transform];

    /* By scaling Y negatively, we effectively flip the image */
    [t scaleXBy:1.0 yBy:-1.0];

    /* But we also have to translate it back by its height */
    [t translateXBy:0.0 yBy:-size.height];

    /* Apply the transform */
    [t concat];
    [tmpimage drawAtPoint:NSMakePoint(0, 0) fromRect:NSMakeRect(0, 0, size.width, size.height)
                operation:op fraction:1.0];
    [NSGraphicsContext restoreGraphicsState];
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
HPIXMAP API dw_pixmap_new_from_file(HWND handle, const char *filename)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;
    char *ext = _dw_get_image_extension( filename );

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSString *nstr = [ NSString stringWithUTF8String:filename ];
    NSImage *tmpimage = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
    if(!tmpimage && ext)
    {
        nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
        tmpimage = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
    }
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSSize size = [tmpimage size];
    NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                               initWithBitmapDataPlanes:NULL
                               pixelsWide:size.width
                               pixelsHigh:size.height
                               bitsPerSample:8
                               samplesPerPixel:4
                               hasAlpha:YES
                               isPlanar:NO
                               colorSpaceName:NSDeviceRGBColorSpace
                               bytesPerRow:0
                               bitsPerPixel:0];
    _dw_flip_image(tmpimage, image, size);
    pixmap->width = size.width;
    pixmap->height = size.height;
    pixmap->image = image;
    pixmap->handle = handle;
    DW_LOCAL_POOL_OUT;
    return pixmap;
}

/*
 * Creates a pixmap from memory.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       data: Source of the image data
 *                 (BMP on OS/2 or Windows, XPM on Unix)
 *       le: length of data
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_new_from_data(HWND handle, const char *data, int len)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSData *thisdata = [NSData dataWithBytes:data length:len];
    NSImage *tmpimage = [[[NSImage alloc] initWithData:thisdata] autorelease];
    if(!tmpimage)
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }
    NSSize size = [tmpimage size];
    NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                               initWithBitmapDataPlanes:NULL
                               pixelsWide:size.width
                               pixelsHigh:size.height
                               bitsPerSample:8
                               samplesPerPixel:4
                               hasAlpha:YES
                               isPlanar:NO
                               colorSpaceName:NSDeviceRGBColorSpace
                               bytesPerRow:0
                               bitsPerPixel:0];
    _dw_flip_image(tmpimage, image, size);
    pixmap->width = size.width;
    pixmap->height = size.height;
    pixmap->image = image;
    pixmap->handle = handle;
    DW_LOCAL_POOL_OUT;
    return pixmap;
}

/*
 * Sets the transparent color for a pixmap
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 *       color:  transparent color
 * Note: This does nothing on Mac as transparency
 *       is handled automatically
 */
void API dw_pixmap_set_transparent_color( HPIXMAP pixmap, ULONG color )
{
    /* Don't do anything */
}

/*
 * Creates a pixmap from internal resource graphic specified by id.
 * Parameters:
 *       handle: Window handle the pixmap is associated with.
 *       id: Resource ID associated with requested pixmap.
 * Returns:
 *       A handle to a pixmap or NULL on failure.
 */
HPIXMAP API dw_pixmap_grab(HWND handle, ULONG resid)
{
    HPIXMAP pixmap;
    DW_LOCAL_POOL_IN;

    if(!(pixmap = calloc(1,sizeof(struct _hpixmap))))
    {
        DW_LOCAL_POOL_OUT;
        return NULL;
    }

    NSBundle *bundle = [NSBundle mainBundle];
    NSString *respath = [bundle resourcePath];
    NSString *filepath = [respath stringByAppendingFormat:@"/%lu.png", resid];
    NSImage *temp = [[NSImage alloc] initWithContentsOfFile:filepath];

    if(temp)
    {
        NSSize size = [temp size];
        NSBitmapImageRep *image = [[NSBitmapImageRep alloc]
                                   initWithBitmapDataPlanes:NULL
                                   pixelsWide:size.width
                                   pixelsHigh:size.height
                                   bitsPerSample:8
                                   samplesPerPixel:4
                                   hasAlpha:YES
                                   isPlanar:NO
                                   colorSpaceName:NSDeviceRGBColorSpace
                                   bytesPerRow:0
                                   bitsPerPixel:0];
        _dw_flip_image(temp, image, size);
        pixmap->width = size.width;
        pixmap->height = size.height;
        pixmap->image = image;
        pixmap->handle = handle;
        [temp release];
        DW_LOCAL_POOL_OUT;
        return pixmap;
    }
    free(pixmap);
    DW_LOCAL_POOL_OUT;
    return NULL;
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
        NSFont *font = _dw_font_by_name(fontname);

        if(font)
        {
            DW_LOCAL_POOL_IN;
            NSFont *oldfont = pixmap->font;
            [font retain];
            pixmap->font = font;
            if(oldfont)
                [oldfont release];
            DW_LOCAL_POOL_OUT;
            return DW_ERROR_NONE;
        }
    }
    return DW_ERROR_GENERAL;
}

/*
 * Destroys an allocated pixmap.
 * Parameters:
 *       pixmap: Handle to a pixmap returned by
 *               dw_pixmap_new..
 */
void API dw_pixmap_destroy(HPIXMAP pixmap)
{
    if(pixmap)
    {
        NSBitmapImageRep *image = (NSBitmapImageRep *)pixmap->image;
        NSFont *font = pixmap->font;
        DW_LOCAL_POOL_IN;
        [image release];
        [font release];
        free(pixmap);
        DW_LOCAL_POOL_OUT;
    }
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
int API dw_pixmap_stretch_bitblt(HWND dest, HPIXMAP destp, int xdest, int ydest, int width, int height, HWND src, HPIXMAP srcp, int xsrc, int ysrc, int srcwidth, int srcheight)
{
    DWBitBlt *bltinfo;
    NSValue* bi;
    DW_LOCAL_POOL_IN;

    /* Sanity checks */
    if((!dest && !destp) || (!src && !srcp) ||
       ((srcwidth == -1 || srcheight == -1) && srcwidth != srcheight) ||
           (dest && !_dw_render_safe_check(dest)))
    {
        DW_LOCAL_POOL_OUT;
        return DW_ERROR_GENERAL;
    }

    bltinfo = calloc(1, sizeof(DWBitBlt));
    bi = [NSValue valueWithPointer:bltinfo];

    /* Fill in the information */
    bltinfo->dest = dest;
    bltinfo->src = src;
    bltinfo->xdest = xdest;
    bltinfo->ydest = ydest;
    bltinfo->width = width;
    bltinfo->height = height;
    bltinfo->xsrc = xsrc;
    bltinfo->ysrc = ysrc;
    bltinfo->srcwidth = srcwidth;
    bltinfo->srcheight = srcheight;

    if(destp)
    {
        bltinfo->dest = (id)destp->image;
    }
    if(srcp)
    {
        id object = bltinfo->src = (id)srcp->image;
        [object retain];
    }
    [DWObj safeCall:@selector(doBitBlt:) withObject:bi];
    DW_LOCAL_POOL_OUT;
    return DW_ERROR_NONE;
}

/*
 * Create a new static text window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The text to be display by the static text widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
HWND API dw_calendar_new(ULONG cid)
{
    DWCalendar *calendar = [[[DWCalendar alloc] init] retain];
    [calendar setDatePickerMode:DWDatePickerModeSingle];
    [calendar setDatePickerStyle:DWDatePickerStyleClockAndCalendar];
    [calendar setDatePickerElements:DWDatePickerElementFlagYearMonthDay];
    [calendar setTag:cid];
    [calendar setDateValue:[NSDate date]];
    return calendar;
}

/*
 * Sets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 *       year...
 */
void API dw_calendar_set_date(HWND handle, unsigned int year, unsigned int month, unsigned int day)
{
    DWCalendar *calendar = handle;
    NSDate *date;
    char buffer[101];
    DW_LOCAL_POOL_IN;

    snprintf(buffer, 100, "%04d-%02d-%02d", year, month, day);

    NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
    dateFormatter.dateFormat = @"yyyy-MM-dd";

    date = [dateFormatter dateFromString:[NSString stringWithUTF8String:buffer]];
    [calendar setDateValue:date];
    [date release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Gets the current date of a calendar.
 * Parameters:
 *       handle: The handle to the calendar returned by dw_calendar_new().
 */
void API dw_calendar_get_date(HWND handle, unsigned int *year, unsigned int *month, unsigned int *day)
{
    DWCalendar *calendar = handle;
    DW_LOCAL_POOL_IN;
    NSCalendar *mycalendar = [[NSCalendar alloc] initWithCalendarIdentifier:DWCalendarIdentifierGregorian];
    NSDate *date = [calendar dateValue];
    NSDateComponents* components = [mycalendar components:DWCalendarUnitDay|DWCalendarUnitMonth|DWCalendarUnitYear fromDate:date];
    *day = (unsigned int)[components day];
    *month = (unsigned int)[components month];
    *year = (unsigned int)[components year];
    [mycalendar release];
    DW_LOCAL_POOL_OUT;
}

/*
 * Causes the embedded HTML widget to take action.
 * Parameters:
 *       handle: Handle to the window.
 *       action: One of the DW_HTML_* constants.
 */
void API dw_html_action(HWND handle, int action)
{
    DWWebView *html = handle;
    switch(action)
    {
        case DW_HTML_GOBACK:
            [html goBack];
            break;
        case DW_HTML_GOFORWARD:
            [html goForward];
            break;
        case DW_HTML_GOHOME:
            dw_html_url(handle, DW_HOME_URL);
            break;
        case DW_HTML_SEARCH:
            break;
        case DW_HTML_RELOAD:
            [html reload:html];
            break;
        case DW_HTML_STOP:
            [html stopLoading:html];
            break;
        case DW_HTML_PRINT:
            break;
    }
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
    DWWebView *html = handle;
#if WK_API_ENABLED
    [html loadHTMLString:[ NSString stringWithUTF8String:string ] baseURL:nil];
#else
    [[html mainFrame] loadHTMLString:[ NSString stringWithUTF8String:string ] baseURL:nil];
#endif
    return DW_ERROR_NONE;
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
    DWWebView *html = handle;
#if WK_API_ENABLED
    [html loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[ NSString stringWithUTF8String:url ]]]];
#else
    [[html mainFrame] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[ NSString stringWithUTF8String:url ]]]];
#endif
    return DW_ERROR_NONE;
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
    DWWebView *html = handle;
    DW_LOCAL_POOL_IN;

#if WK_API_ENABLED
    [html evaluateJavaScript:[NSString stringWithUTF8String:script] completionHandler:^(NSString *result, NSError *error)
    {
        void *params[2] = { result, scriptdata };
        _dw_event_handler(html, (NSEvent *)params, _DW_EVENT_HTML_RESULT);
    }];
#else
    NSString *result = [html stringByEvaluatingJavaScriptFromString:[NSString stringWithUTF8String:script]];
    void *params[2] = { result, scriptdata };
    _dw_event_handler(html, (NSEvent *)params, _DW_EVENT_HTML_RESULT);
#endif
    DW_LOCAL_POOL_OUT;
    return DW_ERROR_NONE;
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
#if WK_API_ENABLED
    DWWebView *html = handle;
    WKUserContentController *controller = [[html configuration] userContentController];
    
    if(controller && name) 
    {
        DW_LOCAL_POOL_IN;
        /* Script to inject that will call the handler we are adding */
        NSString *script = [NSString stringWithFormat:@"function %s(body) {window.webkit.messageHandlers.%s.postMessage(body);}", 
                            name, name];
        [controller addScriptMessageHandler:handle name:[NSString stringWithUTF8String:name]];
        [controller addUserScript:[[WKUserScript alloc] initWithSource:script 
                injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:NO]];
        DW_LOCAL_POOL_OUT;
        return DW_ERROR_NONE;
    }
#endif
    return DW_ERROR_UNKNOWN;
}

/*
 * Create a new HTML window (widget) to be packed.
 * Not available under OS/2, eCS
 * Parameters:
 *       text: The default text to be in the entryfield widget.
 *       id: An ID to be used with dw_window_from_id() or 0L.
 */
DW_FUNCTION_DEFINITION(dw_html_new, HWND, ULONG cid)
DW_FUNCTION_ADD_PARAM1(cid)
DW_FUNCTION_RETURN(dw_html_new, HWND)
DW_FUNCTION_RESTORE_PARAM1(DW_UNUSED(cid), ULONG)
{
    DW_FUNCTION_INIT;
    DWWebView *web = [[[DWWebView alloc] init] retain];
#if WK_API_ENABLED
    web.navigationDelegate = web;
#else
    web.frameLoadDelegate = (id)web;
#endif
    /* [web setTag:cid]; Why doesn't this work? */
    DW_FUNCTION_RETURN_THIS(web);
}

/*
 * Returns the current X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: Pointer to variable to store X coordinate.
 *       y: Pointer to variable to store Y coordinate.
 */
void API dw_pointer_query_pos(long *x, long *y)
{
    NSPoint mouseLoc;
    mouseLoc = [NSEvent mouseLocation];
    if(x)
    {
        *x = mouseLoc.x;
    }
    if(y)
    {
        *y = [[NSScreen mainScreen] frame].size.height - mouseLoc.y;
    }
}

/*
 * Sets the X and Y coordinates of the mouse pointer.
 * Parameters:
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_pointer_set_pos(long x, long y)
{
    /* From what I have read this isn't possible, agaist human interface rules */
}

/*
 * Create a menu object to be popped up.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HMENUI API dw_menu_new(ULONG cid)
{
    NSMenu *menu = [[NSMenu alloc] init];
    [menu setAutoenablesItems:NO];
    /* [menu setTag:cid]; Why doesn't this work? */
    return menu;
}

/*
 * Create a menubar on a window.
 * Parameters:
 *       location: Handle of a window frame to be attached to.
 */
HMENUI API dw_menubar_new(HWND location)
{
    NSWindow *window = location;
    NSMenu *windowmenu = _dw_generate_main_menu();
    [[window contentView] setMenu:windowmenu];
    return (HMENUI)windowmenu;
}

/*
 * Destroys a menu created with dw_menubar_new or dw_menu_new.
 * Parameters:
 *       menu: Handle of a menu.
 */
void API dw_menu_destroy(HMENUI *menu)
{
    if(menu)
    {
        NSMenu *thismenu = *menu;
        DW_LOCAL_POOL_IN;
        [thismenu release];
        DW_LOCAL_POOL_OUT;
        *menu = NULL;
    }
}

/* Handle deprecation of convertScreenToBase in 10.10 yet still supporting
 * 10.6 and earlier since convertRectFromScreen was introduced in 10.7.
 */
NSPoint _dw_windowPointFromScreen(id window, NSPoint p)
{
    SEL crfs = NSSelectorFromString(@"convertRectFromScreen:");

    if([window respondsToSelector:crfs])
    {
        NSRect (* icrfs)(id, SEL, NSRect) = (NSRect (*)(id, SEL, NSRect))[window methodForSelector:crfs];
        NSRect rect = icrfs(window, crfs, NSMakeRect(p.x, p.y, 1, 1));
        return rect.origin;
    }
    else
    {
        SEL cstb = NSSelectorFromString(@"convertScreenToBase:");

        if([window respondsToSelector:cstb])
        {
            NSPoint (* icstb)(id, SEL, NSPoint) = (NSPoint (*)(id, SEL, NSPoint))[window methodForSelector:cstb];
            return icstb(window, cstb, p);
        }
    }
    return NSMakePoint(0,0);
}

/*
 * Pops up a context menu at given x and y coordinates.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       parent: Handle to the window initiating the popup.
 *       x: X coordinate.
 *       y: Y coordinate.
 */
void API dw_menu_popup(HMENUI *menu, HWND parent, int x, int y)
{
    if(menu)
    {
        NSMenu *thismenu = (NSMenu *)*menu;
        id object = parent;
        NSView *view = [object isKindOfClass:[NSWindow class]] ? [object contentView] : parent;
        NSWindow *window = [view window];
        NSEvent *event = [DWApp currentEvent];
        if(!window)
            window = [event window];
        [thismenu autorelease];
        NSPoint p = NSMakePoint(x, [[NSScreen mainScreen] frame].size.height - y);
        NSEvent* fake = [NSEvent mouseEventWithType:DWEventTypeRightMouseDown
                                           location:_dw_windowPointFromScreen(window, p)
                                      modifierFlags:0
                                          timestamp:[event timestamp]
                                       windowNumber:[window windowNumber]
                                            context:[NSGraphicsContext currentContext]
                                        eventNumber:1
                                         clickCount:1
                                           pressure:0.0];
        [NSMenu popUpContextMenu:thismenu withEvent:fake forView:view];
        *menu = NULL;
    }
}

char _dw_removetilde(char *dest, const char *src)
{
    int z, cur=0;
    char accel = '\0';

    for(z=0;z<strlen(src);z++)
    {
        if(src[z] != '~')
        {
            dest[cur] = src[z];
            cur++;
        }
        else
        {
            accel = src[z+1];
        }
    }
    dest[cur] = 0;
    return accel;
}

/*
 * Adds a menuitem or submenu to an existing menu.
 * Parameters:
 *       menu: The handle the the existing menu.
 *       title: The title text on the menu item to be added.
 *       id: An ID to be used for message passing.
 *       flags: Extended attributes to set on the menu.
 *       end: If TRUE memu is positioned at the end of the menu.
 *       check: If TRUE menu is "check"able.
 *       flags: Extended attributes to set on the menu.
 *       submenu: Handle to an existing menu to be a submenu or NULL.
 */
HWND API dw_menu_append_item(HMENUI menux, const char *title, ULONG itemid, ULONG flags, int end, int check, HMENUI submenux)
{
    NSMenu *menu = menux;
    NSMenu *submenu = submenux;
    DWMenuItem *item = NULL;
    if(strlen(title) == 0)
    {
        [menu addItem:[DWMenuItem separatorItem]];
    }
    else
    {
        char accel[2];
        char *newtitle = malloc(strlen(title)+1);
        NSString *nstr;

        accel[0] = _dw_removetilde(newtitle, title);
        accel[1] = 0;

        nstr = [ NSString stringWithUTF8String:newtitle ];
        free(newtitle);

        item = [[[DWMenuItem alloc] initWithTitle:nstr
                            action:@selector(menuHandler:)
                            keyEquivalent:[ NSString stringWithUTF8String:accel ]] autorelease];
        [menu addItem:item];

        [item setTag:itemid];
        if(check)
        {
            [item setCheck:YES];
            if(flags & DW_MIS_CHECKED)
            {
                [item setState:DWControlStateValueOn];
            }
        }
        if(flags & DW_MIS_DISABLED)
        {
            [item setEnabled:NO];
        }

        if(submenux)
        {
            [submenu setTitle:nstr];
            [menu setSubmenu:submenu forItem:item];
        }
        return item;
    }
    return item;
}

/*
 * Sets the state of a menu item check.
 * Deprecated; use dw_menu_item_set_state()
 * Parameters:
 *       menu: The handle the the existing menu.
 *       id: Menuitem id.
 *       check: TRUE for checked FALSE for not checked.
 */
void API dw_menu_item_set_check(HMENUI menux, unsigned long itemid, int check)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        if(check)
        {
            [menuitem setState:DWControlStateValueOn];
        }
        else
        {
            [menuitem setState:DWControlStateValueOff];
        }
    }
}

/*
 * Deletes the menu item specified.
 * Parameters:
 *       menu: The handle to the  menu in which the item was appended.
 *       id: Menuitem id.
 * Returns:
 *       DW_ERROR_NONE (0) on success or DW_ERROR_UNKNOWN on failure.
 */
int API dw_menu_delete_item(HMENUI menux, unsigned long itemid)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        [menu removeItem:menuitem];
        return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Sets the state of a menu item.
 * Parameters:
 *       menu: The handle to the existing menu.
 *       id: Menuitem id.
 *       flags: DW_MIS_ENABLED/DW_MIS_DISABLED
 *              DW_MIS_CHECKED/DW_MIS_UNCHECKED
 */
void API dw_menu_item_set_state(HMENUI menux, unsigned long itemid, unsigned long state)
{
    id menu = menux;
    NSMenuItem *menuitem = (NSMenuItem *)[menu itemWithTag:itemid];

    if(menuitem != nil)
    {
        if(state & DW_MIS_CHECKED)
        {
            [menuitem setState:DWControlStateValueOn];
        }
        else if(state & DW_MIS_UNCHECKED)
        {
            [menuitem setState:DWControlStateValueOff];
        }
        if(state & DW_MIS_ENABLED)
        {
            [menuitem setEnabled:YES];
        }
        else if(state & DW_MIS_DISABLED)
        {
            [menuitem setEnabled:NO];
        }
    }
}

/* Gets the notebook page from associated ID */
DWNotebookPage *_dw_notepage_from_id(DWNotebook *notebook, unsigned long pageid)
{
    NSArray *pages = [notebook tabViewItems];
    for(DWNotebookPage *notepage in pages)
    {
        if([notepage pageid] == pageid)
        {
            return notepage;
        }
    }
    return nil;
}

/*
 * Create a notebook object to be packed.
 * Parameters:
 *       id: An ID to be used for getting the resource from the
 *           resource file.
 */
HWND API dw_notebook_new(ULONG cid, int top)
{
    DWNotebook *notebook = [[[DWNotebook alloc] init] retain];
    [notebook setDelegate:notebook];
    /* [notebook setTag:cid]; Why doesn't this work? */
    return notebook;
}

/*
 * Adds a new page to specified notebook.
 * Parameters:
 *          handle: Window (widget) handle.
 *          flags: Any additional page creation flags.
 *          front: If TRUE page is added at the beginning.
 */
unsigned long API dw_notebook_page_new(HWND handle, ULONG flags, int front)
{
    DWNotebook *notebook = handle;
    NSInteger page = [notebook pageid];
    DWNotebookPage *notepage = [[DWNotebookPage alloc] initWithIdentifier:[NSString stringWithFormat: @"pageid:%d", (int)page]];
    [notepage setPageid:(int)page];
    if(front)
    {
        [notebook insertTabViewItem:notepage atIndex:(NSInteger)0];
    }
    else
    {
        [notebook addTabViewItem:notepage];
    }
    [notepage autorelease];
    [notebook setPageid:(int)(page+1)];
    return (unsigned long)page;
}

/*
 * Remove a page from a notebook.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be destroyed.
 */
void API dw_notebook_page_destroy(HWND handle, unsigned long pageid)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);
    DW_LOCAL_POOL_IN;

    if(notepage != nil)
    {
        [notebook removeTabViewItem:notepage];
    }
    DW_LOCAL_POOL_OUT;
}

/*
 * Queries the currently visible page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 */
unsigned long API dw_notebook_page_get(HWND handle)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = (DWNotebookPage *)[notebook selectedTabViewItem];
    return [notepage pageid];
}

/*
 * Sets the currently visibale page ID.
 * Parameters:
 *          handle: Handle to the notebook widget.
 *          pageid: ID of the page to be made visible.
 */
void API dw_notebook_page_set(HWND handle, unsigned long pageid)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        [notebook selectTabViewItem:notepage];
    }
}

/*
 * Sets the text on the specified notebook tab.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_text(HWND handle, ULONG pageid, const char *text)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        [notepage setLabel:[ NSString stringWithUTF8String:text ]];
    }
}

/*
 * Sets the text on the specified notebook tab status area.
 * Parameters:
 *          handle: Notebook handle.
 *          pageid: Page ID of the tab to set.
 *          text: Pointer to the text to set.
 */
void API dw_notebook_page_set_status_text(HWND handle, ULONG pageid, const char *text)
{
    /* Note supported here... do nothing */
}

/*
 * Packs the specified box into the notebook page.
 * Parameters:
 *          handle: Handle to the notebook to be packed.
 *          pageid: Page ID in the notebook which is being packed.
 *          page: Box handle to be packed.
 */
void API dw_notebook_pack(HWND handle, ULONG pageid, HWND page)
{
    DWNotebook *notebook = handle;
    DWNotebookPage *notepage = _dw_notepage_from_id(notebook, pageid);

    if(notepage != nil)
    {
        HWND tmpbox = dw_box_new(DW_VERT, 0);
        DWBox *box = tmpbox;

        dw_box_pack_start(tmpbox, page, 0, 0, TRUE, TRUE, 0);
        [notepage setView:box];
        [box release];
        [box autorelease];
    }
}

#ifndef NSWindowCollectionBehaviorFullScreenPrimary
#define NSWindowCollectionBehaviorFullScreenPrimary (1 << 7)
#endif

/*
 * Create a new Window Frame.
 * Parameters:
 *       owner: The Owner's window handle or HWND_DESKTOP.
 *       title: The Window title.
 *       flStyle: Style flags, see the PM reference.
 */
DW_FUNCTION_DEFINITION(dw_window_new, HWND, HWND hwndOwner, const char *title, ULONG flStyle)
DW_FUNCTION_ADD_PARAM3(hwndOwner, title, flStyle)
DW_FUNCTION_RETURN(dw_window_new, HWND)
DW_FUNCTION_RESTORE_PARAM3(hwndOwner, HWND, title, char *, flStyle, ULONG)
{
    DW_FUNCTION_INIT;
    NSRect frame = NSMakeRect(1,1,1,1);
    DWWindow *window = [[DWWindow alloc]
                        initWithContentRect:frame
                        styleMask:(flStyle)
                        backing:NSBackingStoreBuffered
                        defer:false];

    [window setTitle:[ NSString stringWithUTF8String:title ]];

    DWView *view = [[DWView alloc] init];

    [window setContentView:view];
    [window setDelegate:view];
    [window setAutorecalculatesKeyViewLoop:YES];
    [window setAcceptsMouseMovedEvents:YES];
    [window setReleasedWhenClosed:YES];
    [view autorelease];

    /* Enable full screen mode on resizeable windows */
    if(flStyle & DW_FCF_SIZEBORDER)
    {
        [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    }

    /* If it isn't a toplevel window... */
    if(hwndOwner)
    {
        id object = hwndOwner;

        /* Check to see if the parent is an MDI window */
        if([object isMemberOfClass:[DWMDI class]])
        {
            /* Set the window level to be floating */
            [window setLevel:NSFloatingWindowLevel];
            [window setHidesOnDeactivate:YES];
        }
    }
    DW_FUNCTION_RETURN_THIS(window);
}

/*
 * Call a function from the window (widget)'s context.
 * Parameters:
 *       handle: Window handle of the widget.
 *       function: Function pointer to be called.
 *       data: Pointer to the data to be passed to the function.
 */
void API dw_window_function(HWND handle, void *function, void *data)
{
    void **params = calloc(2, sizeof(void *));
    NSValue *v;
    DW_LOCAL_POOL_IN;
    v = [NSValue valueWithPointer:params];
    params[0] = function;
    params[1] = data;
    [DWObj performSelectorOnMainThread:@selector(doWindowFunc:) withObject:v waitUntilDone:YES];
    free(params);
    DW_LOCAL_POOL_OUT;
}


/*
 * Changes the appearance of the mouse pointer.
 * Parameters:
 *       handle: Handle to widget for which to change.
 *       cursortype: ID of the pointer you want.
 */
void API dw_window_set_pointer(HWND handle, int pointertype)
{
    id object = handle;

    if([ object isKindOfClass:[ NSView class ] ])
    {
        NSView *view = handle;

        if(pointertype == DW_POINTER_DEFAULT)
        {
            [view discardCursorRects];
        }
        else if(pointertype == DW_POINTER_ARROW)
        {
            NSRect rect = [view frame];
            NSCursor *cursor = [NSCursor arrowCursor];

            [view addCursorRect:rect cursor:cursor];
        }
        /* No cursor for DW_POINTER_CLOCK? */
    }
}

/*
 * Makes the window visible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_show(HWND handle)
{
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        NSRect rect = [[window contentView] frame];
        id defaultitem = [window initialFirstResponder];

        if([window isMiniaturized])
        {
            [window deminiaturize:nil];
        }
        /* If we haven't been sized by a call.. */
        if(rect.size.width <= 1 || rect.size.height <= 1)
        {
            /* Determine the contents size */
            dw_window_set_size(handle, 0, 0);
        }
        /* If the position was not set... generate a default
         * default one in a similar pattern to SHELLPOSITION.
         */
        if(![window shown])
        {
            static int defaultx = 0, defaulty = 0;
            int cx = dw_screen_width(), cy = dw_screen_height();
            int maxx = cx / 4, maxy = cy / 4;
            NSPoint point;

            rect = [window frame];

            defaultx += 20;
            defaulty += 20;
            if(defaultx > maxx)
                defaultx = 20;
            if(defaulty > maxy)
                defaulty = 20;

            point.x = defaultx;
            /* Take into account menu bar and inverted Y */
            point.y = cy - defaulty - (int)rect.size.height - 22;

            [window setFrameOrigin:point];
            [window setShown:YES];
        }
        [[window contentView] showWindow];
        [window makeKeyAndOrderFront:nil];

        if(!([window styleMask] & DWWindowStyleMaskResizable))
        {
            /* Fix incorrect repeat in displaying textured windows */
            [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMinYEdge];
            [window setContentBorderThickness:0.0 forEdge:NSMinYEdge];
        }
        if(defaultitem)
        {
            /* If there is a default item set, make it first responder */
            [window makeFirstResponder:defaultitem];
        }
    }
    return 0;
}

/*
 * Makes the window invisible.
 * Parameters:
 *           handle: The window handle to make visible.
 */
int API dw_window_hide(HWND handle)
{
    NSObject *object = handle;

    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = handle;

        [window orderOut:nil];
    }
    return 0;
}

/*
 * Sets the colors used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fore: Foreground color in DW_RGB format or a default color index.
 *          back: Background color in DW_RGB format or a default color index.
 */
int API dw_window_set_color(HWND handle, ULONG fore, ULONG back)
{
    id object = handle;
    unsigned long _fore = _dw_get_color(fore);
    unsigned long _back = _dw_get_color(back);
    NSColor *fg = NULL;
    NSColor *bg = NULL;

    /* Get the NSColor for non-default colors */
    if(fore != DW_CLR_DEFAULT)
    {
        fg = [NSColor colorWithDeviceRed: DW_RED_VALUE(_fore)/255.0 green: DW_GREEN_VALUE(_fore)/255.0 blue: DW_BLUE_VALUE(_fore)/255.0 alpha: 1];
    }
    if(back != DW_CLR_DEFAULT)
    {
        bg = [NSColor colorWithDeviceRed: DW_RED_VALUE(_back)/255.0 green: DW_GREEN_VALUE(_back)/255.0 blue: DW_BLUE_VALUE(_back)/255.0 alpha: 1];
    }

    /* Get the textfield from the spinbutton */
    if([object isMemberOfClass:[DWSpinButton class]])
    {
        object = [object textfield];
    }
    /* Get the cell on classes using NSCell */
    if([object isKindOfClass:[NSTextField class]])
    {
        id cell = [object cell];

        [object setTextColor:(fg ? fg : [NSColor controlTextColor])];
        [cell setTextColor:(fg ? fg : [NSColor controlTextColor])];
    }
    if([object isMemberOfClass:[DWButton class]])
    {
        [object setTextColor:(fg ? fg : [NSColor controlTextColor])];
    }
    if([object isKindOfClass:[NSTextField class]] || [object isKindOfClass:[NSButton class]])
    {
        id cell = [object cell];

        [cell setBackgroundColor:(bg ? bg : [NSColor controlColor])];
    }
    else if([object isMemberOfClass:[DWBox class]])
    {
        DWBox *box = object;

        [box setColor:_back];
    }
    else if([object isKindOfClass:[NSTableView class]])
    {
        DWContainer *cont = handle;

        [cont setBackgroundColor:(bg ? bg : [NSColor controlBackgroundColor])];
        [cont setForegroundColor:(fg ? fg : [NSColor controlTextColor])];
    }
    else if([object isMemberOfClass:[DWMLE class]])
    {
        DWMLE *mle = handle;
        [mle setBackgroundColor:(bg ? bg : [NSColor controlBackgroundColor])];
        NSTextStorage *ts = [mle textStorage];
        [ts setForegroundColor:(fg ? fg : [NSColor controlTextColor])];
    }
    return 0;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          border: Size of the window border in pixels.
 */
int API dw_window_set_border(HWND handle, int border)
{
    return 0;
}

/*
 * Sets the style of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          width: New width in pixels.
 *          height: New height in pixels.
 */
void API dw_window_set_style(HWND handle, ULONG style, ULONG mask)
{
    id object = _dw_text_handle(handle);

    if([object isMemberOfClass:[DWWindow class]])
    {
        DWWindow *window = object;
        SEL sssm = NSSelectorFromString(@"setStyleMask");

        if([window respondsToSelector:sssm])
        {
            DWIMP issm = (DWIMP)[window methodForSelector:sssm];
            int currentstyle = (int)[window styleMask];
            int tmp;

            tmp = currentstyle | (int)mask;
            tmp ^= mask;
            tmp |= style;

            issm(window, sssm, tmp);
        }
    }
    else if([object isKindOfClass:[NSTextField class]])
    {
        NSTextField *tf = object;
        DWTextFieldCell *cell = [tf cell];

        [cell setAlignment:(style & 0xF)];
        if(mask & DW_DT_VCENTER && [cell isMemberOfClass:[DWTextFieldCell class]])
        {
            [cell setVCenter:(style & DW_DT_VCENTER ? YES : NO)];
        }
        if(mask & DW_DT_WORDBREAK && [cell isMemberOfClass:[DWTextFieldCell class]])
        {
            [cell setWraps:(style & DW_DT_WORDBREAK ? YES : NO)];
        }
    }
    else if([object isMemberOfClass:[NSTextView class]])
    {
        NSTextView *tv = handle;
        [tv setAlignment:(style & mask & (DWTextAlignmentLeft|DWTextAlignmentCenter|DWTextAlignmentRight))];
    }
    else if([object isMemberOfClass:[DWButton class]])
    {
        DWButton *button = handle;

        if(mask & DW_BS_NOBORDER)
        {
            if(style & DW_BS_NOBORDER)
                [button setBordered:NO];
            else
                [button setBordered:YES];
        }
    }
    else if([object isMemberOfClass:[DWMenuItem class]])
    {
        if(mask & (DW_MIS_CHECKED | DW_MIS_UNCHECKED))
        {
            if(style & DW_MIS_CHECKED)
                [object setState:DWControlStateValueOn];
            else if(style & DW_MIS_UNCHECKED)
                [object setState:DWControlStateValueOff];
        }
        if(mask & (DW_MIS_ENABLED | DW_MIS_DISABLED))
        {
            if(style & DW_MIS_ENABLED)
                [object setEnabled:YES];
            else if(style & DW_MIS_DISABLED)
                [object setEnabled:NO];
        }
    }
}

/*
 * Sets the current focus item for a window/dialog.
 * Parameters:
 *         handle: Handle to the dialog item to be focused.
 * Remarks:
 *          This is for use after showing the window/dialog.
 */
void API dw_window_set_focus(HWND handle)
{
    id object = handle;

    [[object window] makeFirstResponder:object];
}

/*
 * Sets the default focus item for a window/dialog.
 * Parameters:
 *         window: Toplevel window or dialog.
 *         defaultitem: Handle to the dialog item to be default.
 * Remarks:
 *          This is for use before showing the window/dialog.
 */
void API dw_window_default(HWND handle, HWND defaultitem)
{
    DWWindow *window = handle;
    id object = defaultitem;

    if([window isKindOfClass:[NSWindow class]] && [object isKindOfClass:[NSControl class]])
    {
        [window setInitialFirstResponder:defaultitem];
    }
}

/*
 * Sets window to click the default dialog item when an ENTER is pressed.
 * Parameters:
 *         window: Window (widget) to look for the ENTER press.
 *         next: Window (widget) to move to next (or click)
 */
void API dw_window_click_default(HWND handle, HWND next)
{
    id object = handle;
    id control = next;

    if([object isMemberOfClass:[DWWindow class]])
    {
        if([control isMemberOfClass:[DWButton class]])
        {
            NSWindow *window = object;

            [window setDefaultButtonCell:[control cell]];
        }
    }
    else
    {
        if([control isMemberOfClass:[DWSpinButton class]])
        {
            control = [control textfield];
        }
        else if([control isMemberOfClass:[DWComboBox class]])
        {
            /* TODO: Figure out why the combobox can't be
             * focused using makeFirstResponder method.
             */
            control = [control textfield];
        }
        [object setClickDefault:control];
    }
}

/*
 * Captures the mouse input to this window.
 * Parameters:
 *       handle: Handle to receive mouse input.
 */
void API dw_window_capture(HWND handle)
{
    /* Don't do anything for now */
}

/*
 * Releases previous mouse capture.
 */
void API dw_window_release(void)
{
    /* Don't do anything for now */
}

/*
 * Changes a window's parent to newparent.
 * Parameters:
 *           handle: The window handle to destroy.
 *           newparent: The window's new parent window.
 */
void API dw_window_reparent(HWND handle, HWND newparent)
{
    id object = handle;

    if([object isMemberOfClass:[DWWindow class]])
    {
        /* We can't actually reparent on MacOS but if the
         * new parent is an MDI window, change to be a
         * floating window... otherwise set it to normal.
         */
        NSWindow *window = handle;

        /* If it isn't a toplevel window... */
        if(newparent)
        {
            object = newparent;

            /* Check to see if the parent is an MDI window */
            if([object isMemberOfClass:[DWMDI class]])
            {
                /* Set the window level to be floating */
                [window setLevel:NSFloatingWindowLevel];
                [window setHidesOnDeactivate:YES];
                return;
            }
        }
        /* Set the window back to a normal window */
        [window setLevel:NSNormalWindowLevel];
        [window setHidesOnDeactivate:NO];
    }
}

/* Allows the user to choose a font using the system's font chooser dialog.
 * Parameters:
 *       currfont: current font
 * Returns:
 *       A malloced buffer with the selected font or NULL on error.
 */
char * API dw_font_choose(const char *currfont)
{
    /* Create the Font Chooser Dialog class. */
    static DWFontChoose *fontDlg = nil;
    DWDialog *dialog;
    NSFont *font = nil;

    if(currfont)
        font = _dw_font_by_name(currfont);

    if(fontDlg)
    {
        dialog = [fontDlg dialog];
        /* If someone is already waiting just return */
        if(dialog)
        {
            return NULL;
        }
    }
    else
    {
        [NSFontManager setFontPanelFactory:[DWFontChoose class]];
        fontDlg = (DWFontChoose *)[DWFontManager fontPanel:YES];
    }

    dialog = dw_dialog_new(fontDlg);
    if(font)
        [DWFontManager setSelectedFont:font isMultiple:NO];
    else
        [DWFontManager setSelectedFont:[NSFont fontWithName:@"Helvetica" size:9.0] isMultiple:NO];
    [fontDlg setDialog:dialog];
    [DWFontManager orderFrontFontPanel:DWFontManager];


    /* Wait for them to pick a color */
    font = (NSFont *)dw_dialog_wait(dialog);
    if(font)
    {
        NSString *fontname = [font displayName];
        NSString *output = [NSString stringWithFormat:@"%d.%s", (int)[font pointSize], [fontname UTF8String]];
        return strdup([output UTF8String]);
    }
    return NULL;
}

/* Internal function to return a pointer to an item struct
 * with information about the packing information regarding object.
 */
Item *_dw_box_item(id object)
{
    /* Find the item within the box it is packed into */
    if([object isKindOfClass:[DWBox class]] || [object isKindOfClass:[DWGroupBox class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if([object isKindOfClass:[NSTableView class]] && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            Box *thisbox = [parent box];
            Item *thisitem = thisbox->items;
            int z;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    return &thisitem[z];
            }
        }
    }
    return NULL;
}

/*
 * Sets the font used by a specified window (widget) handle.
 * Parameters:
 *          handle: The window (widget) handle.
 *          fontname: Name and size of the font in the form "size.fontname"
 */
int API dw_window_set_font(HWND handle, const char *fontname)
{
    NSFont *font = fontname ? _dw_font_by_name(fontname) :
    (DWDefaultFont ? DWDefaultFont : [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]);

    if(font)
    {
        id object = _dw_text_handle(handle);
        if([object window])
        {
            [object lockFocus];
            [font set];
            [object unlockFocus];
        }
        if([object isMemberOfClass:[DWGroupBox class]])
        {
            [object setTitleFont:font];
        }
        else if([object isMemberOfClass:[DWMLE class]])
        {
            DWMLE *mle = object;
            
            [[mle textStorage] setFont:font];
        }
        else if([object isKindOfClass:[NSControl class]])
        {
            [object setFont:font];
            [[object cell] setFont:font];
        }
        else if([object isMemberOfClass:[DWRender class]])
        {
            DWRender *render = object;

            [render setFont:font];
        }
        else
            return DW_ERROR_UNKNOWN;
        /* If we changed the text... */
        Item *item = _dw_box_item(handle);

        /* Check to see if any of the sizes need to be recalculated */
        if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
        {
            _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
            /* Queue a redraw on the top-level window */
            _dw_redraw([object window], TRUE);
        }
        return DW_ERROR_NONE;
    }
    return DW_ERROR_UNKNOWN;
}

/*
 * Returns the current font for the specified window
 * Parameters:
 *           handle: The window handle from which to obtain the font.
 */
char * API dw_window_get_font(HWND handle)
{
    id object = _dw_text_handle(handle);
    NSFont *font = nil;

    if([object isMemberOfClass:[DWGroupBox class]])
    {
        font = [object titleFont];
    }
    else if([object isKindOfClass:[NSControl class]] || [object isMemberOfClass:[DWRender class]])
    {
         font = [object font];
    }
    if(font)
    {
        NSString *fontname = [font displayName];
        NSString *output = [NSString stringWithFormat:@"%d.%s", (int)[font pointSize], [fontname UTF8String]];
        return strdup([output UTF8String]);
    }
    return NULL;
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
    DW_FUNCTION_INIT;
    DW_LOCAL_POOL_IN;
    id object = handle;
    int retval = 0;

    /* Handle destroying a top-level window */
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        DWWindow *window = handle;
        [window close];
    }
    /* Handle removing menu items from menus */
    else if([ object isKindOfClass:[NSMenuItem class]])
    {
        NSMenu *menu = [object menu];

        [menu removeItem:object];
    }
    /* Handle destroying a control or box */
    else if([object isKindOfClass:[NSView class]] || [object isKindOfClass:[NSControl class]])
    {
        DWBox *parent = (DWBox *)[object superview];

        /* Some controls are embedded in scrollviews...
         * so get the parent of the scrollview in that case.
         */
        if(([object isKindOfClass:[NSTableView class]] || [object isMemberOfClass:[DWMLE class]])
            && [parent isMemberOfClass:[NSClipView class]])
        {
            object = [parent superview];
            parent = (DWBox *)[object superview];
        }

        if([parent isKindOfClass:[DWBox class]] || [parent isKindOfClass:[DWGroupBox class]])
        {
            id window = [object window];
            Box *thisbox = [parent box];
            int z, index = -1;
            Item *tmpitem = NULL, *thisitem = thisbox->items;

            if(!thisitem)
                thisbox->count = 0;

            for(z=0;z<thisbox->count;z++)
            {
                if(thisitem[z].hwnd == object)
                    index = z;
            }

            if(index != -1)
            {
                [object removeFromSuperview];

                if(thisbox->count > 1)
                {
                    tmpitem = calloc(sizeof(Item), (thisbox->count-1));

                    /* Copy all but the current entry to the new list */
                    for(z=0;z<index;z++)
                    {
                        tmpitem[z] = thisitem[z];
                    }
                    for(z=index+1;z<thisbox->count;z++)
                    {
                        tmpitem[z-1] = thisitem[z];
                    }
                }

                thisbox->items = tmpitem;
                if(thisitem)
                    free(thisitem);
                if(tmpitem)
                    thisbox->count--;
                else
                    thisbox->count = 0;

                /* Queue a redraw on the top-level window */
                _dw_redraw(window, TRUE);
            }
        }
    }
    DW_LOCAL_POOL_OUT;
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Gets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 * Returns:
 *       text: The text associsated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_get_text, char *, HWND handle)
DW_FUNCTION_ADD_PARAM1(handle)
DW_FUNCTION_RETURN(dw_window_get_text, char *)
DW_FUNCTION_RESTORE_PARAM1(handle, HWND)
{
    DW_FUNCTION_INIT;
    id object = _dw_text_handle(handle);
    char *retval = NULL;

    if([ object isKindOfClass:[ NSWindow class ] ] || [ object isKindOfClass:[ NSButton class ] ])
    {
        id window = object;
        NSString *nsstr = [ window title];

        retval = strdup([ nsstr UTF8String ]);
    }
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = object;
        NSString *nsstr = [ control stringValue];

        retval = strdup([ nsstr UTF8String ]);
    }
    DW_FUNCTION_RETURN_THIS(retval);
}

/*
 * Sets the text used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       text: The text associsated with a given window.
 */
DW_FUNCTION_DEFINITION(dw_window_set_text, void, HWND handle, const char *text)
DW_FUNCTION_ADD_PARAM2(handle, text)
DW_FUNCTION_NO_RETURN(dw_window_set_text)
DW_FUNCTION_RESTORE_PARAM2(handle, HWND, text, char *)
{
    DW_FUNCTION_INIT;
    id object = _dw_text_handle(handle);

    if([ object isKindOfClass:[ NSWindow class ] ] || [ object isKindOfClass:[ NSButton class ] ])
        [object setTitle:[ NSString stringWithUTF8String:text ]];
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = object;
        [control setStringValue:[ NSString stringWithUTF8String:text ]];
    }
    else if([object isMemberOfClass:[DWGroupBox class]])
    {
       DWGroupBox *groupbox = object;
       [groupbox setTitle:[NSString stringWithUTF8String:text]];
    }
    else
        return;
    /* If we changed the text... */
    Item *item = _dw_box_item(handle);

    /* Check to see if any of the sizes need to be recalculated */
    if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
    {
      int newwidth, newheight;

      _dw_control_size(handle, &newwidth, &newheight);

      /* Only update the item and redraw the window if it changed */
      if((item->origwidth == DW_SIZE_AUTO && item->width != newwidth) ||
         (item->origheight == DW_SIZE_AUTO && item->height != newheight))
      {
         if(item->origwidth == DW_SIZE_AUTO)
            item->width = newwidth;
         if(item->origheight == DW_SIZE_AUTO)
            item->height = newheight;
         /* Queue a redraw on the top-level window */
         _dw_redraw([object window], TRUE);
      }
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
    DW_FUNCTION_INIT;
    id object = handle;
    if(bubbletext && *bubbletext)
        [object setToolTip:[NSString stringWithUTF8String:bubbletext]];
    else
        [object setToolTip:nil];
    DW_FUNCTION_RETURN_NOTHING;
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = handle;
        object = [sv documentView];
    }
    if([object isKindOfClass:[NSControl class]] || [object isKindOfClass:[NSMenuItem class]])
    {
        [object setEnabled:NO];
    }
    if([object isKindOfClass:[NSTextView class]])
    {
        NSTextView *mle = object;

        [mle setEditable:NO];
    }
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
    DW_FUNCTION_INIT;
    id object = handle;

    if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = handle;
        object = [sv documentView];
    }
    if([object isKindOfClass:[NSControl class]] || [object isKindOfClass:[NSMenuItem class]])
    {
        [object setEnabled:YES];
    }
    if([object isKindOfClass:[NSTextView class]])
    {
        NSTextView *mle = object;

        [mle setEditable:YES];
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
int API dw_window_set_bitmap_from_data(HWND handle, unsigned long cid, const char *data, int len)
{
    id object = handle;
    int retval = DW_ERROR_UNKNOWN;

    if([object isKindOfClass:[NSImageView class]] || [object isKindOfClass:[NSButton class]])
    {
        if(data)
        {
            DW_LOCAL_POOL_IN;
            NSData *thisdata = [NSData dataWithBytes:data length:len];
            NSImage *pixmap = [[[NSImage alloc] initWithData:thisdata] autorelease];

            if(pixmap)
            {
                [object setImage:pixmap];
                /* If we changed the bitmap... */
                Item *item = _dw_box_item(handle);

                /* Check to see if any of the sizes need to be recalculated */
                if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
                {
                    _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
                    /* Queue a redraw on the top-level window */
                    _dw_redraw([object window], TRUE);
                }
                retval = DW_ERROR_NONE;
            }
            else
                retval = DW_ERROR_GENERAL;
            DW_LOCAL_POOL_OUT;
        }
        else
            return dw_window_set_bitmap(handle, cid, NULL);
    }
    return retval;
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
int API dw_window_set_bitmap(HWND handle, unsigned long resid, const char *filename)
{
    id object = handle;
    int retval = DW_ERROR_UNKNOWN;
    DW_LOCAL_POOL_IN;

    if([object isKindOfClass:[NSImageView class]] || [object isKindOfClass:[NSButton class]])
    {
        NSImage *bitmap = nil;

        if(filename)
        {
            char *ext = _dw_get_image_extension( filename );
            NSString *nstr = [ NSString stringWithUTF8String:filename ];

            bitmap = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];

            if(!bitmap && ext)
            {
                nstr = [nstr stringByAppendingString: [NSString stringWithUTF8String:ext]];
                bitmap = [[[NSImage alloc] initWithContentsOfFile:nstr] autorelease];
            }
            if(!bitmap)
                retval = DW_ERROR_GENERAL;
        }
        if(!bitmap && resid > 0 && resid < 65536)
        {
            bitmap = _dw_icon_load(resid);
            if(!bitmap)
                retval = DW_ERROR_GENERAL;
        }

        if(bitmap)
        {
            [object setImage:bitmap];

            /* If we changed the bitmap... */
            Item *item = _dw_box_item(handle);

            /* Check to see if any of the sizes need to be recalculated */
            if(item && (item->origwidth == DW_SIZE_AUTO || item->origheight == DW_SIZE_AUTO))
            {
                _dw_control_size(handle, item->origwidth == DW_SIZE_AUTO ? &item->width : NULL, item->origheight == DW_SIZE_AUTO ? &item->height : NULL);
                /* Queue a redraw on the top-level window */
                _dw_redraw([object window], TRUE);
            }
            retval = DW_ERROR_NONE;
        }
    }
    DW_LOCAL_POOL_OUT;
    return retval;
}

/*
 * Sets the icon used for a given window.
 * Parameters:
 *       handle: Handle to the window.
 *       id: An ID to be used to specify the icon.
 */
void API dw_window_set_icon(HWND handle, HICN icon)
{
    /* This isn't needed, it is loaded from the bundle */
}

/*
 * Gets the child window handle with specified ID.
 * Parameters:
 *       handle: Handle to the parent window.
 *       id: Integer ID of the child.
 */
HWND API dw_window_from_id(HWND handle, int cid)
{
    NSObject *object = handle;
    NSView *view = handle;
    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = handle;
        view = [window contentView];
    }
    return [view viewWithTag:cid];
}

/*
 * Minimizes or Iconifies a top-level window.
 * Parameters:
 *           handle: The window handle to minimize.
 */
int API dw_window_minimize(HWND handle)
{
    NSWindow *window = handle;
    [window miniaturize:nil];
    return 0;
}

/* Causes entire window to be invalidated and redrawn.
 * Parameters:
 *           handle: Toplevel window handle to be redrawn.
 */
void API dw_window_redraw(HWND handle)
{
    DWWindow *window = handle;
    [window setRedraw:YES];
    [[window contentView] showWindow];
#ifndef BUILDING_FOR_MOJAVE
    [window flushWindow];
#endif
    [window setRedraw:NO];
}

/*
 * Makes the window topmost.
 * Parameters:
 *           handle: The window handle to make topmost.
 */
int API dw_window_raise(HWND handle)
{
    NSWindow *window = handle;
    [window orderFront:nil];
    return DW_ERROR_NONE;
}

/*
 * Makes the window bottommost.
 * Parameters:
 *           handle: The window handle to make bottommost.
 */
int API dw_window_lower(HWND handle)
{
    NSWindow *window = handle;
    [window orderBack:nil];
    return DW_ERROR_NONE;
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
    DW_FUNCTION_INIT;
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        Box *thisbox;
        NSRect content, frame = NSMakeRect(0, 0, width, height);

        /* Convert the external frame size to internal content size */
        content = [NSWindow contentRectForFrameRect:frame styleMask:[window styleMask]];

        /*
         * The following is an attempt to dynamically size a window based on the size of its
         * children before realization. Only applicable when width or height is less than one.
         */
        if((width < 1 || height < 1) && (thisbox = (Box *)[[window contentView] box]))
        {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, (int)width, (int)height, 1);

            /* Update components that need auto-sizing */
            if(width < 1) content.size.width = thisbox->minwidth;
            if(height < 1) content.size.height = thisbox->minheight;
        }

        /* Finally set the size */
        [window setContentSize:content.size];
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
void API dw_window_get_preferred_size(HWND handle, int *width, int *height)
{
    id object = handle;

    if([object isMemberOfClass:[DWWindow class]])
    {
        Box *thisbox;

        if((thisbox = (Box *)[[object contentView] box]))
        {
            int depth = 0;
            NSRect frame;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, 0, 0, 1);

            /* Figure out the border size */
            frame = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, thisbox->minwidth, thisbox->minheight) styleMask:[object styleMask]];

            /* Return what was requested */
            if(width) *width = frame.size.width;
            if(height) *height = frame.size.height;
        }
    }
    else if([object isMemberOfClass:[DWBox class]])
    {
        Box *thisbox;

        if((thisbox = (Box *)[object box]))
        {
            int depth = 0;

            /* Calculate space requirements */
            _dw_resize_box(thisbox, &depth, 0, 0, 1);

            /* Return what was requested */
            if(width) *width = thisbox->minwidth;
            if(height) *height = thisbox->minheight;
        }
    }
    else
        _dw_control_size(handle, width, height);
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

/* Convert the coordinates based on gravity */
void _handle_gravity(HWND handle, long *x, long *y, unsigned long width, unsigned long height)
{
    int horz = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_horz"));
    int vert = DW_POINTER_TO_INT(dw_window_get_data(handle, "_dw_grav_vert"));
    id object = handle;

    /* Do any gravity calculations */
    if(horz || (vert & 0xf) != DW_GRAV_BOTTOM)
    {
        long newx = *x, newy = *y;

        /* Handle horizontal center gravity */
        if((horz & 0xf) == DW_GRAV_CENTER)
            newx += ((dw_screen_width() / 2) - (width / 2));
        /* Handle right gravity */
        else if((horz & 0xf) == DW_GRAV_RIGHT)
            newx = dw_screen_width() - width - *x;
        /* Handle vertical center gravity */
        if((vert & 0xf) == DW_GRAV_CENTER)
            newy += ((dw_screen_height() / 2) - (height / 2));
        else if((vert & 0xf) == DW_GRAV_TOP)
            newy = dw_screen_height() - height - *y;

        /* Save the new values */
        *x = newx;
        *y = newy;
    }
    /* Adjust the values to avoid Dock/Menubar if requested */
    if((horz | vert) & DW_GRAV_OBSTACLES)
    {
        NSRect visiblerect = [[object screen] visibleFrame];
        NSRect totalrect = [[object screen] frame];

        if(horz & DW_GRAV_OBSTACLES)
        {
            if((horz & 0xf) == DW_GRAV_LEFT)
                *x += visiblerect.origin.x;
            else if((horz & 0xf) == DW_GRAV_RIGHT)
                *x -= (totalrect.origin.x + totalrect.size.width) - (visiblerect.origin.x + visiblerect.size.width);
        }
        if(vert & DW_GRAV_OBSTACLES)
        {
            if((vert & 0xf) == DW_GRAV_BOTTOM)
                *y += visiblerect.origin.y;
            else if((vert & 0xf) == DW_GRAV_TOP)
                *y -= (totalrect.origin.y + totalrect.size.height) - (visiblerect.origin.y + visiblerect.size.height);
        }
    }
}

/*
 * Sets the position of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 */
DW_FUNCTION_DEFINITION(dw_window_set_pos, void, HWND handle, LONG x, LONG y)
DW_FUNCTION_ADD_PARAM3(handle, x, y)
DW_FUNCTION_NO_RETURN(dw_window_set_pos)
DW_FUNCTION_RESTORE_PARAM3(handle, HWND, x, LONG, y, LONG)
{
    DW_FUNCTION_INIT;
    NSObject *object = handle;

    if([ object isMemberOfClass:[ DWWindow class ] ])
    {
        DWWindow *window = handle;
        NSPoint point;
        NSSize size = [[window contentView] frame].size;

        /* Can't position an unsized window, so attempt to auto-size */
        if(size.width <= 1 || size.height <= 1)
        {
            /* Determine the contents size */
            dw_window_set_size(handle, 0, 0);
        }

        size = [window frame].size;
        _handle_gravity(handle, &x, &y, (unsigned long)size.width, (unsigned long)size.height);

        point.x = x;
        point.y = y;

        [window setFrameOrigin:point];
        /* Position set manually... don't auto-position */
        [window setShown:YES];
    }
    DW_FUNCTION_RETURN_NOTHING;
}

/*
 * Sets the position and size of a given window (widget).
 * Parameters:
 *          handle: Window (widget) handle.
 *          x: X location from the bottom left.
 *          y: Y location from the bottom left.
 *          width: Width of the widget.
 *          height: Height of the widget.
 */
void API dw_window_set_pos_size(HWND handle, LONG x, LONG y, ULONG width, ULONG height)
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
void API dw_window_get_pos_size(HWND handle, LONG *x, LONG *y, ULONG *width, ULONG *height)
{
    NSObject *object = handle;

    if([ object isKindOfClass:[ NSWindow class ] ])
    {
        NSWindow *window = handle;
        NSRect rect = [window frame];
        if(x)
            *x = rect.origin.x;
        if(y)
            *y = [[window screen] frame].size.height - rect.origin.y - rect.size.height;
        if(width)
            *width = rect.size.width;
        if(height)
            *height = rect.size.height;
        return;
    }
    else if([ object isKindOfClass:[ NSControl class ] ])
    {
        NSControl *control = handle;
        NSRect rect = [control frame];
        if(x)
            *x = rect.origin.x;
        if(y)
            *y = rect.origin.y;
        if(width)
            *width = rect.size.width;
        if(height)
            *height = rect.size.height;
        return;
    }
}

/*
 * Returns the width of the screen.
 */
int API dw_screen_width(void)
{
    NSRect screenRect = [[NSScreen mainScreen] frame];
    return screenRect.size.width;
}

/*
 * Returns the height of the screen.
 */
int API dw_screen_height(void)
{
    NSRect screenRect = [[NSScreen mainScreen] frame];
    return screenRect.size.height;
}

/* This should return the current color depth */
unsigned long API dw_color_depth_get(void)
{
    NSWindowDepth screenDepth = [[NSScreen mainScreen] depth];
    return NSBitsPerPixelFromDepth(screenDepth);
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
#ifdef BUILDING_FOR_MOUNTAIN_LION
    char outbuf[1025] = {0};
    HWND retval = NULL;

    if(description)
    {
        va_list args;

        va_start(args, description);
        vsnprintf(outbuf, 1024, description, args);
        va_end(args);
    }

#ifdef BUILDING_FOR_MOJAVE
    /* Configure the notification's payload. */
    if (@available(macOS 10.14, *))
    {
        if([[NSBundle mainBundle] bundleIdentifier] != nil)
        {
            UNMutableNotificationContent* notification = [[UNMutableNotificationContent alloc] init];

            if(notification)
            {
                notification.title = [NSString stringWithUTF8String:title];
                if(description)
                    notification.body = [NSString stringWithUTF8String:outbuf];
                if(imagepath)
                {
                    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:imagepath]];
                    NSError *error;
                    UNNotificationAttachment *attachment = [UNNotificationAttachment attachmentWithIdentifier:@"imageID"
                                                                URL:url
                                                            options:nil
                                                              error:&error];
                    if(attachment)
                        notification.attachments = @[attachment];
                }
                retval = notification;
            }
        }
    }
    else
#endif
    {
#ifndef BUILDING_FOR_BIG_SUR
        /* Fallback on earlier versions */
        NSUserNotification *notification = [[NSUserNotification alloc] init];

        if(notification)
        {
            notification.title = [NSString stringWithUTF8String:title];
            notification.informativeText = [NSString stringWithUTF8String:outbuf];
            if(imagepath)
                notification.contentImage = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:imagepath]];
            retval = notification;
        }
#endif
    }
    return retval;
#else
   return NULL;
#endif
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
#ifdef BUILDING_FOR_MOUNTAIN_LION
    if(notification)
    {
        NSString *notid = [NSString stringWithFormat:@"dw-notification-%llu", DW_POINTER_TO_ULONGLONG(notification)];
        
#ifdef BUILDING_FOR_MOJAVE
        /* Schedule the notification. */
        if (@available(macOS 10.14, *))
        {
            if([[NSBundle mainBundle] bundleIdentifier] != nil)
            {
                UNMutableNotificationContent* content = (UNMutableNotificationContent *)notification;
                UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:notid content:content trigger:nil];

                UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
                [center addNotificationRequest:request withCompletionHandler:nil];
            }
        }
        else
#endif
        {
#ifndef BUILDING_FOR_BIG_SUR
            /* Fallback on earlier versions */
            NSUserNotification *request = (NSUserNotification *)notification;
            request.identifier = notid;
            [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:request];
#endif
        }
        return DW_ERROR_NONE;
    }
#endif
    return DW_ERROR_UNKNOWN;
}


/*
 * Returns some information about the current operating environment.
 * Parameters:
 *       env: Pointer to a DWEnv struct.
 */
void API dw_environment_query(DWEnv *env)
{
    memset(env, '\0', sizeof(DWEnv));
    strcpy(env->osName, "MacOS");

    strncpy(env->buildDate, __DATE__, sizeof(env->buildDate)-1);
    strncpy(env->buildTime, __TIME__, sizeof(env->buildTime)-1);
#ifdef WK_API_ENABLED
   strncpy(env->htmlEngine, "WEBKIT", sizeof(env->htmlEngine)-1);
#else
   strncpy(env->htmlEngine, "WEBVIEW", sizeof(env->htmlEngine)-1);
#endif
    env->DWMajorVersion = DW_MAJOR_VERSION;
    env->DWMinorVersion = DW_MINOR_VERSION;
#ifdef VER_REV
    env->DWSubVersion = VER_REV;
#else
    env->DWSubVersion = DW_SUB_VERSION;
#endif

    env->MajorVersion = DWOSMajor;
    env->MinorVersion = DWOSMinor;
    env->MajorBuild = DWOSBuild;
}

/*
 * Emits a beep.
 * Parameters:
 *       freq: Frequency.
 *       dur: Duration.
 */
void API dw_beep(int freq, int dur)
{
    NSBeep();
}

/* Call this after drawing to the screen to make sure
 * anything you have drawn is visible.
 */
void API dw_flush(void)
{
    /* This may need to be thread specific */
    [DWObj performSelectorOnMainThread:@selector(doFlush:) withObject:nil waitUntilDone:NO];
}

/* Functions for managing the user data lists that are associated with
 * a given window handle.  Used in dw_window_set_data() and
 * dw_window_get_data().
 */
UserData *_dw_find_userdata(UserData **root, const char *varname)
{
    UserData *tmp = *root;

    while(tmp)
    {
        if(strcasecmp(tmp->varname, varname) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

int _dw_new_userdata(UserData **root, const char *varname, void *data)
{
    UserData *new = _dw_find_userdata(root, varname);

    if(new)
    {
        new->data = data;
        return TRUE;
    }
    else
    {
        new = malloc(sizeof(UserData));
        if(new)
        {
            new->varname = strdup(varname);
            new->data = data;

            new->next = NULL;

            if (!*root)
                *root = new;
            else
            {
                UserData *prev = *root, *tmp = prev->next;

                while(tmp)
                {
                    prev = tmp;
                    tmp = tmp->next;
                }
                prev->next = new;
            }
            return TRUE;
        }
    }
    return FALSE;
}

int _dw_remove_userdata(UserData **root, const char *varname, int all)
{
    UserData *prev = NULL, *tmp = *root;

    while(tmp)
    {
        if(all || strcasecmp(tmp->varname, varname) == 0)
        {
            if(!prev)
            {
                *root = tmp->next;
                free(tmp->varname);
                free(tmp);
                if(!all)
                    return 0;
                tmp = *root;
            }
            else
            {
                /* If all is true we should
                 * never get here.
                 */
                prev->next = tmp->next;
                free(tmp->varname);
                free(tmp);
                return 0;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
    return 0;
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
    id object = window;
    if([object isMemberOfClass:[DWWindow class]])
    {
        NSWindow *win = window;
        object = [win contentView];
    }
    else if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = window;
        object = [sv documentView];
    }
    else if([object isMemberOfClass:[NSBox class]])
    {
        NSBox *box = window;
        object = [box contentView];
    }
    /* Failsafe so we don't crash */
    if(![object respondsToSelector:NSSelectorFromString(@"userdata")])
    {
#ifdef DEBUG
        NSLog(@"WARNING: Object class %@ does not support dw_window_set_data()\n", [object className]);
#endif
        return;
    }
    WindowData *blah = (WindowData *)[object userdata];

    if(!blah)
    {
        if(!dataname)
            return;

        blah = calloc(1, sizeof(WindowData));
        [object setUserdata:blah];
    }

    if(data)
        _dw_new_userdata(&(blah->root), dataname, data);
    else
    {
        if(dataname)
            _dw_remove_userdata(&(blah->root), dataname, FALSE);
        else
            _dw_remove_userdata(&(blah->root), NULL, TRUE);
    }
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
    id object = window;
    if([object isMemberOfClass:[DWWindow class]])
    {
        NSWindow *win = window;
        object = [win contentView];
    }
    else if([object isMemberOfClass:[NSScrollView class]])
    {
        NSScrollView *sv = window;
        object = [sv documentView];
    }
    else if([object isMemberOfClass:[NSBox class]])
    {
        NSBox *box = window;
        object = [box contentView];
    }
    /* Failsafe so we don't crash */
    if(![object respondsToSelector:NSSelectorFromString(@"userdata")])
    {
#ifdef DEBUG
        NSLog(@"WARNING: Object class %@ does not support dw_window_get_data()\n", [object className]);
#endif
        return NULL;
    }
    WindowData *blah = (WindowData *)[object userdata];

    if(blah && blah->root && dataname)
    {
        UserData *ud = _dw_find_userdata(&(blah->root), dataname);
        if(ud)
            return ud->data;
    }
    return NULL;
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

#define DW_TIMER_MAX 64
static NSTimer *DWTimers[DW_TIMER_MAX];

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
    int z;

    for(z=0;z<DW_TIMER_MAX;z++)
    {
        if(!DWTimers[z])
        {
            break;
        }
    }

    if(sigfunc && !DWTimers[z])
    {
        NSTimeInterval seconds = (double)interval / 1000.0;
        NSTimer *thistimer = DWTimers[z] = [NSTimer scheduledTimerWithTimeInterval:seconds target:DWHandler selector:@selector(runTimer:) userInfo:nil repeats:YES];
        _dw_new_signal(0, thistimer, z+1, sigfunc, NULL, data);
        return z+1;
    }
    return 0;
}

/*
 * Removes timer callback.
 * Parameters:
 *       id: Timer ID returned by dw_timer_connect().
 */
void API dw_timer_disconnect(HTIMER timerid)
{
    DWSignalHandler *prev = NULL, *tmp = DWRoot;
    NSTimer *thistimer;

    /* 0 is an invalid timer ID */
    if(timerid < 1 || !DWTimers[timerid-1])
        return;

    thistimer = DWTimers[timerid-1];
    DWTimers[timerid-1] = nil;

    [thistimer invalidate];

    while(tmp)
    {
        if(tmp->id == timerid && tmp->window == thistimer)
        {
            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
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
    ULONG message = 0, msgid = 0;

    /* Handle special case of application delete signal */
    if(!window && signame && strcmp(signame, DW_SIGNAL_DELETE) == 0)
    {
        window = DWApp;
    }

    if(window && signame && sigfunc)
    {
        if((message = _dw_findsigmessage(signame)) != 0)
        {
            _dw_new_signal(message, window, (int)msgid, sigfunc, discfunc, data);
        }
    }
}

/*
 * Removes callbacks for a given window with given name.
 * Parameters:
 *       window: Window handle of callback to be removed.
 */
void API dw_signal_disconnect_by_name(HWND window, const char *signame)
{
    DWSignalHandler *prev = NULL, *tmp = DWRoot;
    ULONG message;

    if(!window || !signame || (message = _dw_findsigmessage(signame)) == 0)
        return;

    while(tmp)
    {
        if(tmp->window == window && tmp->message == message)
        {
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
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
    DWSignalHandler *prev = NULL, *tmp = DWRoot;

    while(tmp)
    {
        if(tmp->window == window)
        {
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

/*
 * Removes all callbacks for a given window with specified data.
 * Parameters:
 *       window: Window handle of callback to be removed.
 *       data: Pointer to the data to be compared against.
 */
void API dw_signal_disconnect_by_data(HWND window, void *data)
{
    DWSignalHandler *prev = NULL, *tmp = DWRoot;

    while(tmp)
    {
        if(tmp->window == window && tmp->data == data)
        {
            void (*discfunc)(HWND, void *) = tmp->discfunction;

            if(discfunc)
            {
                discfunc(tmp->window, tmp->data);
            }

            if(prev)
            {
                prev->next = tmp->next;
                free(tmp);
                tmp = prev->next;
            }
            else
            {
                DWRoot = tmp->next;
                free(tmp);
                tmp = DWRoot;
            }
        }
        else
        {
            prev = tmp;
            tmp = tmp->next;
        }
    }
}

void _dw_my_strlwr(char *buf)
{
   int z, len = (int)strlen(buf);

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
   char errorbuf[1025];


   if(!handle)
      return -1;

   if((len = (int)strlen(name)) == 0)
      return   -1;

   /* Lenth + "lib" + ".dylib" + NULL */
   newname = malloc(len + 10);

   if(!newname)
      return -1;

   sprintf(newname, "lib%s.dylib", name);
   _dw_my_strlwr(newname);

   *handle = dlopen(newname, RTLD_NOW);
   if(*handle == NULL)
   {
      strncpy(errorbuf, dlerror(), 1024);
      printf("%s\n", errorbuf);
      sprintf(newname, "lib%s.dylib", name);
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
    if(DWThread == pthread_self())
    {
        while(pthread_mutex_trylock(mutex) != 0)
        {
            /* Process any pending events */
            while(_dw_main_iteration([NSDate dateWithTimeIntervalSinceNow:0.01]))
            {
                /* Just loop */
            }
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
int API dw_event_reset(HEV eve)
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
int API dw_event_post(HEV eve)
{
   if(!eve)
      return FALSE;

   pthread_mutex_lock (&(eve->mutex));
   pthread_cond_broadcast (&(eve->event));
   eve->posted = 1;
   pthread_mutex_unlock (&(eve->mutex));
   return 0;
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
            if((bytesread = (int)read(array[z].fd, &command, 1)) < 1)
            {
               struct _dw_seminfo *newarray = NULL;

               /* Remove this connection from the set */
               if(connectcount > 1)
               {
                   newarray = (struct _dw_seminfo *)malloc(sizeof(struct _dw_seminfo)*(connectcount-1));
                   if(!z)
                       memcpy(newarray, &array[1], sizeof(struct _dw_seminfo)*(connectcount-1));
                   else
                   {
                       memcpy(newarray, array, sizeof(struct _dw_seminfo)*z);
                       if(z!=(connectcount-1))
                           memcpy(&newarray[z], &array[z+1], sizeof(struct _dw_seminfo)*(connectcount-(z+1)));
                   }
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
                           write(array[s].fd, &tmp, 1);
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
                        write(array[z].fd, &tmp, 1);
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
    HEV eve;
    DWTID dwthread;

    if(!tmpsock)
        return NULL;

    eve = (HEV)malloc(sizeof(struct _dw_unix_event));

    if(!eve)
    {
        free(tmpsock);
        return NULL;
    }

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
        free(eve);
        return NULL;
    }

    /* Create a thread to handle this event semaphore */
    pthread_create(&dwthread, NULL, (void *)_dw_handle_sem, (void *)tmpsock);
    eve->alive = ev;
    return eve;
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
    HEV eve;
    int ev = socket(AF_UNIX, SOCK_STREAM, 0);
    if(ev < 0)
        return NULL;

    eve = (HEV)malloc(sizeof(struct _dw_unix_event));

    if(!eve)
    {
        close(ev);
        return NULL;
    }

    un.sun_family=AF_UNIX;
    mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
    strcpy(un.sun_path, "/tmp/.dw/");
    strcat(un.sun_path, name);
    connect(ev, (struct sockaddr *)&un, sizeof(un));
    eve->alive = ev;
    return eve;
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

   if(!eve || eve->alive < 0)
      return 0;

   if(write(eve->alive, &tmp, 1) == 1)
      return 0;
   return 1;
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

   if(!eve || eve->alive < 0)
      return 0;

   if(write(eve->alive, &tmp, 1) == 1)
      return 0;
   return 1;
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

   if(!eve || eve->alive < 0)
      return DW_ERROR_NON_INIT;

   /* Set the timout or infinite */
   if(timeout != -1)
   {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (int)timeout % 1000;

      useme = &tv;
   }

   FD_ZERO(&rd);
   FD_SET(eve->alive, &rd);

   /* Signal wait */
   tmp = (char)2;
   write(eve->alive, &tmp, 1);

   retval = select(eve->alive+1, &rd, NULL, NULL, useme);

   /* Signal done waiting. */
   tmp = (char)3;
   write(eve->alive, &tmp, 1);

   if(retval == 0)
      return DW_ERROR_TIMEOUT;
   else if(retval == -1)
      return DW_ERROR_INTERRUPT;

   /* Clear the entry from the pipe so
    * we don't loop endlessly. :)
    */
   read(eve->alive, &tmp, 1);
   return 0;
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
    if(eve)
    {
        close(eve->alive);
        free(eve);
    }
    return 0;
}

/* Mac specific function to cause garbage collection */
void _dw_pool_drain(void)
{
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
    pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif
}

/*
 * Generally an internal function called from a newly created
 * thread to setup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they create threads that require access to Dynamic Windows.
 */
void API _dw_init_thread(void)
{
    /* If we aren't using garbage collection we need autorelease pools */
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif
    _dw_init_colors();
}

/*
 * Generally an internal function called from a terminating
 * thread to cleanup the Dynamic Windows environment for the thread.
 * However it is exported so language bindings can call it when
 * they exit threads that require access to Dynamic Windows.
 */
void API _dw_deinit_thread(void)
{
    NSColor *color;

    /* Release the pool when we are done so we don't leak */
    color = pthread_getspecific(_dw_fg_color_key);
    [color release];
    color = pthread_getspecific(_dw_bg_color_key);
    [color release];
#if !defined(GARBAGE_COLLECT)
    NSAutoreleasePool *pool = pthread_getspecific(_dw_pool_key);
    [pool drain];
#endif
}

/*
 * Setup thread independent pools.
 */
void _dwthreadstart(void *data)
{
    void (*threadfunc)(void *) = NULL;
    void **tmp = (void **)data;

    _dw_init_thread();

    threadfunc = (void (*)(void *))tmp[0];

    /* Start our thread function */
    threadfunc(tmp[1]);

    free(tmp);

    _dw_deinit_thread();
}

/*
 * Sets the default font used on text based widgets.
 * Parameters:
 *           fontname: Font name in Dynamic Windows format.
 */
void API dw_font_set_default(const char *fontname)
{
    NSFont *oldfont = DWDefaultFont;
    DWDefaultFont = nil;
    if(fontname)
    {
        DWDefaultFont = _dw_font_by_name(fontname);
        [DWDefaultFont retain];
    }
    [oldfont release];
}

/* If DWApp is uninitialized, initialize it */
void _dw_app_init(void)
{
    if(!DWApp)
    {
        DWApp = [NSApplication sharedApplication];
        DWAppDel *del = [[DWAppDel alloc] init];
        [DWApp setDelegate:del];
    }
}

/*
 * Initializes the Dynamic Windows engine.
 * Parameters:
 *           newthread: True if this is the only thread.
 *                      False if there is already a message loop running.
 */
int API dw_init(int newthread, int argc, char *argv[])
{
    char *lang = getenv("LANG");

    /* Correct the startup path if run from a bundle */
    if(argc > 0 && argv[0])
    {
        char *pathcopy = strdup(argv[0]);
        char *app = strstr(pathcopy, ".app/");
        char *binname = strrchr(pathcopy, '/');

        if(binname && (binname++) && !_dw_app_id[0])
        {
            /* If we have a binary name, use that for the Application ID instead. */
            snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.%s", DW_APP_DOMAIN_DEFAULT, binname);
        }
        if(app)
        {
            char pathbuf[PATH_MAX+1] = { 0 };
            size_t len = (size_t)(app - pathcopy);

            if(len > 0)
            {
                strncpy(_dw_bundle_path, pathcopy, len + 4);
                strcat(_dw_bundle_path, "/Contents/Resources");
            }
            *app = 0;

            getcwd(pathbuf, PATH_MAX);

            /* If run from a bundle the path seems to be / */
            if(strcmp(pathbuf, "/") == 0)
            {
                char *pos = strrchr(pathcopy, '/');

                if(pos)
                {
                    strncpy(pathbuf, pathcopy, (size_t)(pos - pathcopy));
                    chdir(pathbuf);
                }
            }
        }
        if(pathcopy)
            free(pathcopy);
    }

    /* Just in case we can't obtain a path */
    if(!_dw_bundle_path[0])
        getcwd(_dw_bundle_path, PATH_MAX);

    /* Set the locale... if it is UTF-8 pass it
     * directly, otherwise specify UTF-8 explicitly.
     */
    setlocale(LC_ALL, lang && strstr(lang, ".UTF-8") ? lang : "UTF-8");

    /* If we aren't using garbage collection we need autorelease pools */
#if !defined(GARBAGE_COLLECT)
    pthread_key_create(&_dw_pool_key, NULL);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    pthread_setspecific(_dw_pool_key, pool);
#endif

    /* Get the operating system version */
    NSString *version = [[NSProcessInfo processInfo] operatingSystemVersionString];
    const char *versionstr = [version UTF8String];
    sscanf(versionstr, "Version %d.%d.%d", &DWOSMajor, &DWOSMinor, &DWOSBuild);

    /* Create the application object */
    _dw_app_init();
    /* Create object for handling timers */
    DWHandler = [[DWTimerHandler alloc] init];

    pthread_key_create(&_dw_fg_color_key, NULL);
    pthread_key_create(&_dw_bg_color_key, NULL);
    _dw_init_colors();
    /* Create a default main menu, with just the application menu */
    DWMainMenu = _dw_generate_main_menu();
    [DWMainMenu retain];
    [DWApp setMainMenu:DWMainMenu];
    DWObj = [[DWObject alloc] init];
    DWDefaultFont = nil;
    DWFontManager = [NSFontManager sharedFontManager];
#ifdef BUILDING_FOR_MOJAVE
    if (@available(macOS 10.14, *))
    {
        if([[NSBundle mainBundle] bundleIdentifier] != nil)
        {
            UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
            if(center)
            {
                [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert|UNAuthorizationOptionSound
                        completionHandler:^(BOOL granted, NSError * _Nullable error) {
                            if (granted)
                            {
                                center.delegate = [[[DWUserNotificationCenterDelegate alloc] init] autorelease];
                            }
                            else
                            {
                                NSLog(@"WARNING: Unable to get notification permission. %@", error.localizedDescription);
                            }
                }];
            }
        }
    }
    _DWDirtyDrawables = [[NSMutableArray alloc] init];
#else
    /* Create mutexes for thread safety */
    DWRunMutex = dw_mutex_new();
    DWThreadMutex = dw_mutex_new();
    DWThreadMutex2 = dw_mutex_new();
#endif
    /* Use NSThread to start a dummy thread to initialize the threading subsystem */
    NSThread *thread = [[NSThread alloc] initWithTarget:DWObj selector:@selector(uselessThread:) object:nil];
    [thread start];
    [thread release];
    [NSTextField setCellClass:[DWTextFieldCell class]];
    if(!_dw_app_id[0])
    {
        /* Generate an Application ID based on the PID if all else fails. */
        snprintf(_dw_app_id, _DW_APP_ID_SIZE, "%s.pid.%d", DW_APP_DOMAIN_DEFAULT, getpid());
    }
    return DW_ERROR_NONE;
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
   char namebuf[1025] = {0};
   struct _dw_unix_shm *handle = malloc(sizeof(struct _dw_unix_shm));

   mkdir("/tmp/.dw", S_IWGRP|S_IWOTH);
   snprintf(namebuf, 1024, "/tmp/.dw/%s", name);

   if((handle->fd = open(namebuf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
   {
      free(handle);
      return NULL;
   }

   ftruncate(handle->fd, size);

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
    DWTID thread;
    void **tmp = malloc(sizeof(void *) * 2);
    int rc;

   tmp[0] = func;
   tmp[1] = data;

   rc = pthread_create(&thread, NULL, (void *)_dwthreadstart, (void *)tmp);
   if(rc == 0)
      return thread;
   return (DWTID)-1;
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

#ifdef BUILDING_FOR_CATALINA
NSURL *_dw_url_from_program(NSString *nsprogram, NSWorkspace *ws)
{
    NSURL *retval = [ws URLForApplicationWithBundleIdentifier:nsprogram];
    
    if(!retval)
    {
        SEL sfpfa = NSSelectorFromString(@"fullPathForApplication:");

        if([ws respondsToSelector:sfpfa])
        {
            DWIMP ifpfa = (DWIMP)[ws methodForSelector:sfpfa];
            NSString *apppath = ifpfa(ws, sfpfa, nsprogram);
            
            if(apppath)
                retval = [NSURL fileURLWithPath:apppath];
        }
        if(!retval)
            retval = [NSURL fileURLWithPath:nsprogram];
    }
    return retval;
}
#endif

/*
 * Execute and external program in a seperate session.
 * Parameters:
 *       program: Program name with optional path.
 *       type: Either DW_EXEC_CON or DW_EXEC_GUI.
 *       params: An array of pointers to string arguements.
 * Returns:
 *       DW_ERROR_UNKNOWN (-1) on error.
 */
int dw_exec(const char *program, int type, char **params)
{
    int ret = DW_ERROR_UNKNOWN;

    if(type == DW_EXEC_GUI)
    {
        NSString *nsprogram = [NSString stringWithUTF8String:program];
        NSWorkspace *ws = [NSWorkspace sharedWorkspace];

        if(params && params[0] && params[1])
        {
#ifdef BUILDING_FOR_CATALINA
            if(@available(macOS 10.15, *))
            {
                NSURL *url = _dw_url_from_program(nsprogram, ws);
                NSMutableArray *array = [[NSMutableArray alloc] init];
                __block DWDialog *dialog = dw_dialog_new(NULL);
                int z = 1;

                while(params[z])
                {
                    NSString *thisfile = [NSString stringWithUTF8String:params[z]];
                    NSURL *nsfile = [NSURL fileURLWithPath:thisfile];

                    [array addObject:nsfile];
                    z++;
                }

                [ws openURLs:array withApplicationAtURL:url
                                          configuration:[NSWorkspaceOpenConfiguration configuration]
                                      completionHandler:^(NSRunningApplication *app, NSError *error) {
                    int pid = DW_ERROR_UNKNOWN;

                    if(error)
                        NSLog(@"openURLs: %@", [error localizedDescription]);
                    else
                        pid = [app processIdentifier];
                    dw_dialog_dismiss(dialog, DW_INT_TO_POINTER(pid));
                }];
                ret = DW_POINTER_TO_INT(dw_dialog_wait(dialog));
            }
            else
#endif
            {
                SEL sofwa = NSSelectorFromString(@"openFile:withApplication:");

                if([ws respondsToSelector:sofwa])
                {
                    int z = 1;

                    while(params[z])
                    {
                        NSString *file = [NSString stringWithUTF8String:params[z]];
                        DWIMP iofwa = (DWIMP)[ws methodForSelector:sofwa];

                        iofwa(ws, sofwa, file, nsprogram);
                        ret = DW_ERROR_NONE;
                        z++;
                    }
                }
            }
        }
        else
        {
#ifdef BUILDING_FOR_CATALINA
            if(@available(macOS 10.15, *))
            {
                NSURL *url = _dw_url_from_program(nsprogram, ws);
                __block DWDialog *dialog = dw_dialog_new(NULL);

                [ws openApplicationAtURL:url
                           configuration:[NSWorkspaceOpenConfiguration configuration]
                       completionHandler:^(NSRunningApplication *app, NSError *error) {
                    int pid = DW_ERROR_UNKNOWN;

                    if(error)
                        NSLog(@"openApplicationAtURL: %@", [error localizedDescription]);
                    else
                        pid = [app processIdentifier];
                    dw_dialog_dismiss(dialog, DW_INT_TO_POINTER(pid));
                }];
                ret = DW_POINTER_TO_INT(dw_dialog_wait(dialog));
            }
            else
#endif
            {
                SEL sla = NSSelectorFromString(@"launchApplication:");

                if([ws respondsToSelector:sla])
                {
                    DWIMP ila = (DWIMP)[ws methodForSelector:sla];
                    ila(ws, sla, nsprogram);
                    ret = DW_ERROR_NONE;
                }
            }
        }
    }
    else
    {
        /* Use AppleScript to Open Terminal.app or contact an existing copy,
         * and execute the command passed in params[].
         */
        char *commandline;
        char *format = "osascript -e \'tell app \"Terminal\" to do script \"";
        int len = (int)strlen(format) + 3;

        /* Generate a command line from the parameters */
        if(params && *params)
        {
            int z = 0;

            while(params[z])
            {
                len+=strlen(params[z]) + 1;
                z++;
            }
            z=1;
            commandline = calloc(1, len);
            strcpy(commandline, format);
            strcat(commandline, params[0]);
            while(params[z])
            {
                strcat(commandline, " ");
                strcat(commandline, params[z]);
                z++;
            }
        }
        else
        {
            len += strlen(program);
            commandline = calloc(1, len);
            strcpy(commandline, format);
            strcat(commandline, program);
        }
        strcat(commandline, "\"\'");

        /* Attempt to execute the commmand, DW_ERROR_NONE on success */
        if(system(commandline) != -1)
            ret = DW_ERROR_NONE;
        free(commandline);
    }
    return ret;
}

/*
 * Loads a web browser pointed at the given URL.
 * Parameters:
 *       url: Uniform resource locator.
 */
int API dw_browse(const char *url)
{
    NSURL *myurl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    [[NSWorkspace sharedWorkspace] openURL:myurl];
    return DW_ERROR_NONE;
}

typedef struct _dwprint
{
    NSPrintInfo *pi;
    int (*drawfunc)(HPRINT, HPIXMAP, int, void *);
    void *drawdata;
    unsigned long flags;
} DWPrint;

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
    DWPrint *print;
    NSPrintPanel *panel;
    PMPrintSettings settings;
    NSPrintInfo *pi;

    if(!drawfunc || !(print = calloc(1, sizeof(DWPrint))))
    {
        return NULL;
    }

    if(!jobname)
        jobname = "Dynamic Windows Print Job";

    print->drawfunc = drawfunc;
    print->drawdata = drawdata;
    print->flags = flags;

    /* Get the page range */
    pi = [NSPrintInfo sharedPrintInfo];
    [pi setHorizontalPagination:DWPrintingPaginationModeFit];
    [pi setHorizontallyCentered:YES];
    [pi setVerticalPagination:DWPrintingPaginationModeFit];
    [pi setVerticallyCentered:YES];
    [pi setOrientation:DWPaperOrientationPortrait];
    [pi setLeftMargin:0.0];
    [pi setRightMargin:0.0];
    [pi setTopMargin:0.0];
    [pi setBottomMargin:0.0];

    settings = [pi PMPrintSettings];
    PMSetPageRange(settings, 1, pages);
    PMSetFirstPage(settings, 1, true);
    PMSetLastPage(settings, pages, true);
    PMPrintSettingsSetJobName(settings, (CFStringRef)[NSString stringWithUTF8String:jobname]);
    [pi updateFromPMPrintSettings];

    /* Create and show the print panel */
    panel = [NSPrintPanel printPanel];
    if(!panel || [panel runModalWithPrintInfo:pi] == DWModalResponseCancel)
    {
        free(print);
        return NULL;
    }
    /* Put the print info from the panel into the operation */
    print->pi = pi;

    return print;
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
    DWPrint *p = print;
    NSBitmapImageRep *rep, *rep2;
    NSPrintInfo *pi;
    NSPrintOperation *po;
    HPIXMAP pixmap, pixmap2;
    NSImage *image, *flipped;
    NSImageView *iv;
    NSSize size;
    PMPrintSettings settings;
    int x, result = DW_ERROR_UNKNOWN;
    UInt32 start, end;

    if(!p)
        return result;

    DW_LOCAL_POOL_IN;

    /* Figure out the printer/paper size */
    pi = p->pi;
    size = [pi paperSize];

    /* Get the page range */
    settings = [pi PMPrintSettings];
    PMGetFirstPage(settings, &start);
    if(start > 0)
        start--;
    PMGetLastPage(settings, &end);
    PMSetPageRange(settings, 1, 1);
    PMSetFirstPage(settings, 1, true);
    PMSetLastPage(settings, 1, true);
    [pi updateFromPMPrintSettings];

    /* Create an image view to print and a pixmap to draw into */
    iv = [[NSImageView alloc] init];
    pixmap = dw_pixmap_new(iv, (int)size.width, (int)size.height, 8);
    rep = pixmap->image;
    pixmap2 = dw_pixmap_new(iv, (int)size.width, (int)size.height, 8);
    rep2 = pixmap2->image;

    /* Create an image with the data from the pixmap
     * to go into the image view.
     */
    image = [[NSImage alloc] initWithSize:[rep size]];
    flipped = [[NSImage alloc] initWithSize:[rep size]];
    [image addRepresentation:rep];
    [flipped addRepresentation:rep2];
    [iv setImage:flipped];
    [iv setImageScaling:NSImageScaleProportionallyDown];
    [iv setFrameOrigin:NSMakePoint(0,0)];
    [iv setFrameSize:size];

    /* Create the print operation using the image view and
     * print info obtained from the panel in the last call.
     */
    po = [NSPrintOperation printOperationWithView:iv printInfo:pi];
    [po setShowsPrintPanel:NO];

    /* Cycle through each page */
    for(x=start; x<end && p->drawfunc; x++)
    {
        /* Call the application's draw function */
        p->drawfunc(print, pixmap, x, p->drawdata);
        if(p->drawfunc)
        {
           /* Internal representation is flipped... so flip again so we can print */
           _dw_flip_image(image, rep2, size);
#ifdef DEBUG_PRINT
           /* Save it to file to see what we have */
           NSData *data = [rep2 representationUsingType: NSPNGFileType properties: nil];
           [data writeToFile: @"print.png" atomically: NO];
#endif
           /* Print the image view */
           [po runOperation];
           /* Fill the pixmap with white in case we are printing more pages */
           dw_color_foreground_set(DW_CLR_WHITE);
           dw_draw_rect(0, pixmap, TRUE, 0, 0, (int)size.width, (int)size.height);
        }
    }
    if(p->drawfunc)
        result = DW_ERROR_NONE;
    /* Free memory */
    [image release];
    [flipped release];
    dw_pixmap_destroy(pixmap);
    dw_pixmap_destroy(pixmap2);
    free(p);
    [iv release];
    DW_LOCAL_POOL_OUT;
    return result;
}

/*
 * Cancels the print job, typically called from a draw page callback.
 * Parameters:
 *       print: Handle to the print object returned by dw_print_new().
 */
void API dw_print_cancel(HPRINT print)
{
    DWPrint *p = print;

    if(p)
        p->drawfunc = NULL;
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
    size_t buflen = strlen(utf8string) + 1;
    wchar_t *temp = malloc(buflen * sizeof(wchar_t));
    if(temp)
        mbstowcs(temp, utf8string, buflen);
    return temp;
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
    size_t bufflen = 8 * wcslen(wstring) + 1;
    char *temp = malloc(bufflen);
    if(temp)
        wcstombs(temp, wstring, bufflen);
    return temp;
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
#ifdef BUILDING_FOR_MOUNTAIN_LION
        case DW_FEATURE_NOTIFICATION:
#endif
#ifdef BUILDING_FOR_YOSEMITE
        case DW_FEATURE_TASK_BAR:
#endif
#ifdef BUILDING_FOR_SNOW_LEOPARD
        case DW_FEATURE_MLE_AUTO_COMPLETE:
#endif
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
#if WK_API_ENABLED
        case DW_FEATURE_HTML_MESSAGE:
#endif
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_FEATURE_ENABLED;
        case DW_FEATURE_RENDER_SAFE:
            return _dw_render_safe_mode;
#ifdef BUILDING_FOR_MOJAVE
        case DW_FEATURE_DARK_MODE:
        {
            if(@available(macOS 10.14, *))
            {
                /* Make sure DWApp is initialized */
                _dw_app_init();
                /* Get the current appearance */
                NSAppearance *appearance = [DWApp appearance];
                NSAppearanceName basicAppearance;

                if(appearance)
                {
                    basicAppearance = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua,
                                                                                      NSAppearanceNameDarkAqua]];

                    if([basicAppearance isEqualToString:NSAppearanceNameDarkAqua])
                        return DW_DARK_MODE_FORCED;
                    if([basicAppearance isEqualToString:NSAppearanceNameAqua])
                        return DW_FEATURE_DISABLED;
                }
#ifdef BUILDING_FOR_BIG_SUR
                /* Configure the notification's payload. */
                if (@available(macOS 11.0, *)) {
                    appearance = [NSAppearance currentDrawingAppearance];
                } else
#endif
                {
                    _DW_ELSE_AVAILABLE
                    appearance = [NSAppearance currentAppearance];
                    _DW_END_AVAILABLE
                }
                basicAppearance = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua,
                                                                                  NSAppearanceNameDarkAqua]];
                if([basicAppearance isEqualToString:NSAppearanceNameDarkAqua])
                    return DW_DARK_MODE_FULL;
                return DW_DARK_MODE_BASIC;
            }
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
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
#ifdef BUILDING_FOR_MOUNTAIN_LION
        case DW_FEATURE_NOTIFICATION:
#endif
#ifdef BUILDING_FOR_YOSEMITE
        case DW_FEATURE_TASK_BAR:
#endif
#ifdef BUILDING_FOR_SNOW_LEOPARD
        case DW_FEATURE_MLE_AUTO_COMPLETE:
#endif
        case DW_FEATURE_HTML:
        case DW_FEATURE_HTML_RESULT:
#if WK_API_ENABLED
        case DW_FEATURE_HTML_MESSAGE:
#endif
        case DW_FEATURE_CONTAINER_STRIPE:
        case DW_FEATURE_MLE_WORD_WRAP:
        case DW_FEATURE_UTF8_UNICODE:
        case DW_FEATURE_TREE:
        case DW_FEATURE_WINDOW_PLACEMENT:
            return DW_ERROR_GENERAL;
        /* These features are supported and configurable */
        case DW_FEATURE_RENDER_SAFE:
        {
            if(state == DW_FEATURE_ENABLED || state == DW_FEATURE_DISABLED)
            {
                _dw_render_safe_mode = state;
                return DW_ERROR_NONE;
            }
            return DW_ERROR_GENERAL;
        }
#ifdef BUILDING_FOR_MOJAVE
        case DW_FEATURE_DARK_MODE:
        {
            if(@available(macOS 10.14, *))
            {
                /* Make sure DWApp is initialized */
                _dw_app_init();
                /* Disabled forces the non-dark aqua theme */
                if(state == DW_FEATURE_DISABLED)
                   [DWApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
                /* Enabled lets the OS decide the mode */
                else if(state == DW_FEATURE_ENABLED || state == DW_DARK_MODE_FULL)
                   [DWApp setAppearance:nil];
                /* 2 forces dark mode aqua appearance */
                else if(state == DW_DARK_MODE_FORCED)
                    [DWApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
                else
                    return DW_ERROR_GENERAL;
                return DW_ERROR_NONE;
            }
            return DW_FEATURE_UNSUPPORTED;
        }
#endif
        default:
            return DW_FEATURE_UNSUPPORTED;
    }
}
