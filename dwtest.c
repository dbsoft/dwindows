/*
 * An example Dynamic Windows application and
 * testing ground for Dynamic Windows features.
 * By: Brian Smith and Mark Hessling
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifdef __ANDROID__
#include <fcntl.h>
#include <unistd.h>
#endif
#include "dw.h"
/* For snprintf, strdup etc on old Windows SDK */
#if defined(__WIN32__) || defined(__OS2__)
#include "dwcompat.h"
#endif

/* Select a fixed width font for our platform */
#ifdef __OS2__
#define FIXEDFONT "5.System VIO"
#define PLATFORMFOLDER "os2\\"
#elif defined(__WIN32__)
#define FIXEDFONT "10.Lucida Console"
#define PLATFORMFOLDER "win\\"
#elif defined(__MAC__)
#define FIXEDFONT "9.Monaco"
#define PLATFORMFOLDER "mac/"
#elif defined(__IOS__)
#define FIXEDFONT "9.Monaco"
#elif defined(__ANDROID__)
#define FIXEDFONT "10.Monospace"
#elif GTK_MAJOR_VERSION > 1
#define FIXEDFONT "10.monospace"
#define PLATFORMFOLDER "gtk/"
#else
#define FIXEDFONT "fixed"
#endif

char fileiconpath[1025] = "file";
char foldericonpath[1025] = "folder";

#define MAX_WIDGETS 20

unsigned long flStyle = DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_DLGBORDER;

unsigned long current_color = DW_RGB(100,100,100);

int iteration = 0;
void create_button( int);

static char folder_ico[1718] = {
    0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x05, 0x00, 0x00, 0x4E, 0x01, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00,
    0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30, 0x03, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3B, 0x37, 0x77, 0x77, 0x77, 0x77, 0xB7, 0x33, 0x3B, 0x37, 0x77, 0x77, 0x77, 0x77, 0xB7, 0x33, 0x3B, 0x37, 0x77, 0x77, 0x77, 0x77, 0xB7, 0x33, 0x3B, 0x37,
    0x77, 0x77, 0x77, 0x77, 0xB7, 0x33, 0x37, 0x37, 0x77, 0x77, 0x77, 0x77, 0xB7, 0x33, 0x37, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x33, 0x37, 0x73, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30, 0x37, 0x77, 0x77, 0x7F, 0xFF, 0xFF, 0x30, 0x00, 0x3F, 0x77, 0x77, 0xF3, 0x33, 0x33, 0x30, 0x00, 0x03, 0xFF, 0xFF, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x81, 0xFF, 0x00, 0x00, 0xC3, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80,
    0x00, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0xC0, 0xDC, 0xC0, 0x00, 0xF0, 0xCA, 0xA6, 0x00, 0x04, 0x04, 0x04, 0x00, 0x08, 0x08, 0x08, 0x00, 0x0C, 0x0C, 0x0C, 0x00, 0x11, 0x11, 0x11, 0x00, 0x16, 0x16, 0x16, 0x00, 0x1C, 0x1C, 0x1C, 0x00, 0x22, 0x22, 0x22, 0x00, 0x29, 0x29, 0x29, 0x00, 0x55, 0x55, 0x55, 0x00, 0x4D, 0x4D, 0x4D, 0x00, 0x42, 0x42, 0x42, 0x00, 0x39, 0x39, 0x39, 0x00, 0x80, 0x7C, 0xFF, 0x00, 0x50, 0x50, 0xFF, 0x00, 0x93, 0x00, 0xD6, 0x00, 0xFF, 0xEC, 0xCC, 0x00, 0xC6, 0xD6, 0xEF, 0x00, 0xD6, 0xE7, 0xE7, 0x00, 0x90, 0xA9, 0xAD, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00,
    0x99, 0x00, 0x00, 0x00, 0xCC, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x33, 0x33, 0x00, 0x00, 0x33, 0x66, 0x00, 0x00, 0x33, 0x99, 0x00, 0x00, 0x33, 0xCC, 0x00, 0x00, 0x33, 0xFF, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x66, 0x33, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x66, 0x99, 0x00, 0x00, 0x66, 0xCC, 0x00, 0x00, 0x66, 0xFF, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x99, 0x33, 0x00, 0x00, 0x99, 0x66, 0x00, 0x00, 0x99, 0x99, 0x00, 0x00, 0x99, 0xCC, 0x00, 0x00, 0x99, 0xFF, 0x00, 0x00, 0xCC, 0x00, 0x00, 0x00, 0xCC, 0x33, 0x00, 0x00, 0xCC, 0x66, 0x00, 0x00, 0xCC, 0x99, 0x00, 0x00, 0xCC, 0xCC, 0x00, 0x00, 0xCC,
    0xFF, 0x00, 0x00, 0xFF, 0x66, 0x00, 0x00, 0xFF, 0x99, 0x00, 0x00, 0xFF, 0xCC, 0x00, 0x33, 0x00, 0x00, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x66, 0x00, 0x33, 0x00, 0x99, 0x00, 0x33, 0x00, 0xCC, 0x00, 0x33, 0x00, 0xFF, 0x00, 0x33, 0x33, 0x00, 0x00, 0x33, 0x33, 0x33, 0x00, 0x33, 0x33, 0x66, 0x00, 0x33, 0x33, 0x99, 0x00, 0x33, 0x33, 0xCC, 0x00, 0x33, 0x33, 0xFF, 0x00, 0x33, 0x66, 0x00, 0x00, 0x33, 0x66, 0x33, 0x00, 0x33, 0x66, 0x66, 0x00, 0x33, 0x66, 0x99, 0x00, 0x33, 0x66, 0xCC, 0x00, 0x33, 0x66, 0xFF, 0x00, 0x33, 0x99, 0x00, 0x00, 0x33, 0x99, 0x33, 0x00, 0x33, 0x99, 0x66, 0x00, 0x33, 0x99,
    0x99, 0x00, 0x33, 0x99, 0xCC, 0x00, 0x33, 0x99, 0xFF, 0x00, 0x33, 0xCC, 0x00, 0x00, 0x33, 0xCC, 0x33, 0x00, 0x33, 0xCC, 0x66, 0x00, 0x33, 0xCC, 0x99, 0x00, 0x33, 0xCC, 0xCC, 0x00, 0x33, 0xCC, 0xFF, 0x00, 0x33, 0xFF, 0x33, 0x00, 0x33, 0xFF, 0x66, 0x00, 0x33, 0xFF, 0x99, 0x00, 0x33, 0xFF, 0xCC, 0x00, 0x33, 0xFF, 0xFF, 0x00, 0x66, 0x00, 0x00, 0x00, 0x66, 0x00, 0x33, 0x00, 0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x99, 0x00, 0x66, 0x00, 0xCC, 0x00, 0x66, 0x00, 0xFF, 0x00, 0x66, 0x33, 0x00, 0x00, 0x66, 0x33, 0x33, 0x00, 0x66, 0x33, 0x66, 0x00, 0x66, 0x33, 0x99, 0x00, 0x66, 0x33, 0xCC, 0x00, 0x66, 0x33,
    0xFF, 0x00, 0x66, 0x66, 0x00, 0x00, 0x66, 0x66, 0x33, 0x00, 0x66, 0x66, 0x66, 0x00, 0x66, 0x66, 0x99, 0x00, 0x66, 0x66, 0xCC, 0x00, 0x66, 0x99, 0x00, 0x00, 0x66, 0x99, 0x33, 0x00, 0x66, 0x99, 0x66, 0x00, 0x66, 0x99, 0x99, 0x00, 0x66, 0x99, 0xCC, 0x00, 0x66, 0x99, 0xFF, 0x00, 0x66, 0xCC, 0x00, 0x00, 0x66, 0xCC, 0x33, 0x00, 0x66, 0xCC, 0x99, 0x00, 0x66, 0xCC, 0xCC, 0x00, 0x66, 0xCC, 0xFF, 0x00, 0x66, 0xFF, 0x00, 0x00, 0x66, 0xFF, 0x33, 0x00, 0x66, 0xFF, 0x99, 0x00, 0x66, 0xFF, 0xCC, 0x00, 0xCC, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xCC, 0x00, 0x99, 0x99, 0x00, 0x00, 0x99, 0x33, 0x99, 0x00, 0x99, 0x00,
    0x99, 0x00, 0x99, 0x00, 0xCC, 0x00, 0x99, 0x00, 0x00, 0x00, 0x99, 0x33, 0x33, 0x00, 0x99, 0x00, 0x66, 0x00, 0x99, 0x33, 0xCC, 0x00, 0x99, 0x00, 0xFF, 0x00, 0x99, 0x66, 0x00, 0x00, 0x99, 0x66, 0x33, 0x00, 0x99, 0x33, 0x66, 0x00, 0x99, 0x66, 0x99, 0x00, 0x99, 0x66, 0xCC, 0x00, 0x99, 0x33, 0xFF, 0x00, 0x99, 0x99, 0x33, 0x00, 0x99, 0x99, 0x66, 0x00, 0x99, 0x99, 0x99, 0x00, 0x99, 0x99, 0xCC, 0x00, 0x99, 0x99, 0xFF, 0x00, 0x99, 0xCC, 0x00, 0x00, 0x99, 0xCC, 0x33, 0x00, 0x66, 0xCC, 0x66, 0x00, 0x99, 0xCC, 0x99, 0x00, 0x99, 0xCC, 0xCC, 0x00, 0x99, 0xCC, 0xFF, 0x00, 0x99, 0xFF, 0x00, 0x00, 0x99, 0xFF,
    0x33, 0x00, 0x99, 0xCC, 0x66, 0x00, 0x99, 0xFF, 0x99, 0x00, 0x99, 0xFF, 0xCC, 0x00, 0x99, 0xFF, 0xFF, 0x00, 0xCC, 0x00, 0x00, 0x00, 0x99, 0x00, 0x33, 0x00, 0xCC, 0x00, 0x66, 0x00, 0xCC, 0x00, 0x99, 0x00, 0xCC, 0x00, 0xCC, 0x00, 0x99, 0x33, 0x00, 0x00, 0xCC, 0x33, 0x33, 0x00, 0xCC, 0x33, 0x66, 0x00, 0xCC, 0x33, 0x99, 0x00, 0xCC, 0x33, 0xCC, 0x00, 0xCC, 0x33, 0xFF, 0x00, 0xCC, 0x66, 0x00, 0x00, 0xCC, 0x66, 0x33, 0x00, 0x99, 0x66, 0x66, 0x00, 0xCC, 0x66, 0x99, 0x00, 0xCC, 0x66, 0xCC, 0x00, 0x99, 0x66, 0xFF, 0x00, 0xCC, 0x99, 0x00, 0x00, 0xCC, 0x99, 0x33, 0x00, 0xCC, 0x99, 0x66, 0x00, 0xCC, 0x99,
    0x99, 0x00, 0xCC, 0x99, 0xCC, 0x00, 0xCC, 0x99, 0xFF, 0x00, 0xCC, 0xCC, 0x00, 0x00, 0xCC, 0xCC, 0x33, 0x00, 0xCC, 0xCC, 0x66, 0x00, 0xCC, 0xCC, 0x99, 0x00, 0xCC, 0xCC, 0xCC, 0x00, 0xCC, 0xCC, 0xFF, 0x00, 0xCC, 0xFF, 0x00, 0x00, 0xCC, 0xFF, 0x33, 0x00, 0x99, 0xFF, 0x66, 0x00, 0xCC, 0xFF, 0x99, 0x00, 0xCC, 0xFF, 0xCC, 0x00, 0xCC, 0xFF, 0xFF, 0x00, 0xCC, 0x00, 0x33, 0x00, 0xFF, 0x00, 0x66, 0x00, 0xFF, 0x00, 0x99, 0x00, 0xCC, 0x33, 0x00, 0x00, 0xFF, 0x33, 0x33, 0x00, 0xFF, 0x33, 0x66, 0x00, 0xFF, 0x33, 0x99, 0x00, 0xFF, 0x33, 0xCC, 0x00, 0xFF, 0x33, 0xFF, 0x00, 0xFF, 0x66, 0x00, 0x00, 0xFF, 0x66,
    0x33, 0x00, 0xCC, 0x66, 0x66, 0x00, 0xFF, 0x66, 0x99, 0x00, 0xFF, 0x66, 0xCC, 0x00, 0xCC, 0x66, 0xFF, 0x00, 0xFF, 0x99, 0x00, 0x00, 0xFF, 0x99, 0x33, 0x00, 0xFF, 0x99, 0x66, 0x00, 0xFF, 0x99, 0x99, 0x00, 0xFF, 0x99, 0xCC, 0x00, 0xFF, 0x99, 0xFF, 0x00, 0xFF, 0xCC, 0x00, 0x00, 0xFF, 0xCC, 0x33, 0x00, 0xFF, 0xCC, 0x66, 0x00, 0xFF, 0xCC, 0x99, 0x00, 0xFF, 0xCC, 0xCC, 0x00, 0xFF, 0xCC, 0xFF, 0x00, 0xFF, 0xFF, 0x33, 0x00, 0xCC, 0xFF, 0x66, 0x00, 0xFF, 0xFF, 0x99, 0x00, 0xFF, 0xFF, 0xCC, 0x00, 0x66, 0x66, 0xFF, 0x00, 0x66, 0xFF, 0x66, 0x00, 0x66, 0xFF, 0xFF, 0x00, 0xFF, 0x66, 0x66, 0x00, 0xFF, 0x66,
    0xFF, 0x00, 0xFF, 0xFF, 0x66, 0x00, 0x21, 0x00, 0xA5, 0x00, 0x5F, 0x5F, 0x5F, 0x00, 0x77, 0x77, 0x77, 0x00, 0x86, 0x86, 0x86, 0x00, 0x96, 0x96, 0x96, 0x00, 0xCB, 0xCB, 0xCB, 0x00, 0xB2, 0xB2, 0xB2, 0x00, 0xD7, 0xD7, 0xD7, 0x00, 0xDD, 0xDD, 0xDD, 0x00, 0xE3, 0xE3, 0xE3, 0x00, 0xEA, 0xEA, 0xEA, 0x00, 0xF1, 0xF1, 0xF1, 0x00, 0xF8, 0xF8, 0xF8, 0x00, 0xF0, 0xFB, 0xFF, 0x00, 0xA4, 0xA0, 0xA0, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x0A, 0x0A,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x0A, 0x0A, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xA0, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x58, 0xA0, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xA0, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x58, 0xA0, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xA0, 0x9A, 0x9A,
    0x9A, 0x9A, 0x9A, 0x9A, 0x9A, 0xA0, 0x58, 0xA0, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x79, 0xA0, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x7A, 0xA0, 0x52, 0x4B, 0x52, 0x7A, 0x52, 0xFF, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xFF, 0xFF, 0xFF, 0x9A, 0xF6, 0x52, 0x4B, 0x52, 0xA0, 0x9A, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x0A, 0x52, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2A, 0x0A, 0x0A, 0x0A, 0x52, 0xFF, 0xA0, 0xA0, 0xA0, 0xA0, 0xFF, 0x52, 0x52, 0x52,
    0x52, 0x52, 0x2A, 0x0A, 0x0A, 0x0A, 0x0A, 0x52, 0xFF, 0xFF, 0xFF, 0xF6, 0x52, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x52, 0x52, 0x52, 0x52, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07,
    0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x81, 0xFF, 0x00, 0x00, 0xC3, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};

HWND mainwindow,
    copypastefield,
    entryfield,
    checkable_menuitem,
    noncheckable_menuitem,
    cursortogglebutton,
    colorchoosebutton,
    okbutton,
    cancelbutton,
    lbbox,
    combox,
    combobox1,
    combobox2,
    spinbutton,
    slider,
    percent,
    notebookbox,
    notebookbox1,
    notebookbox2,
    notebookbox3,
    notebookbox4,
    notebookbox5,
    notebookbox6,
    notebookbox7,
    notebookbox8,
    notebookbox9,
    html,
    rawhtml,
    notebook,
    vscrollbar,
    hscrollbar,
    status1,
    status2,
    rendcombo,
    imagexspin,
    imageyspin,
    imagestretchcheck,
    container_status,
    tree_status,
    stext,
    tree,
    container,
    container_mle,
    pagebox,
    containerbox,
    textbox1, textbox2, textboxA,
    buttonbox,
    buttonsbox,
    buttonboxperm,
    cal,
    scrollbox,
    labelarray[MAX_WIDGETS],
    entryarray[MAX_WIDGETS],
    filetoolbarbox;

HMENUI mainmenubar,changeable_menu;
#define CHECKABLE_MENUITEMID    2001
#define NONCHECKABLE_MENUITEMID 2002

#define SHAPES_DOUBLE_BUFFERED  0
#define SHAPES_DIRECT           1
#define DRAW_FILE               2

void *containerinfo;

int menu_enabled = 1;

HPIXMAP text1pm,text2pm,image;
HICN fileicon,foldericon;
int mle_point=-1;
int image_x = 20, image_y = 20, image_stretch = 0;

int font_width = 8;
int font_height=12;
int rows=10,width1=6,cols=80;
char *current_file = NULL;
HTIMER timerid;
int num_lines=0;
int max_linewidth=0;
int current_row=0,current_col=0;
int cursor_arrow = 1;
int render_type = SHAPES_DOUBLE_BUFFERED;

FILE *fp=NULL;
char **lp;

char *resolve_keyname(int vk)
{
    char *keyname;
    switch(vk)
    {
        case  VK_LBUTTON : keyname =  "VK_LBUTTON"; break;
        case  VK_RBUTTON : keyname =  "VK_RBUTTON"; break;
        case  VK_CANCEL  : keyname =  "VK_CANCEL"; break;
        case  VK_MBUTTON : keyname =  "VK_MBUTTON"; break;
        case  VK_TAB     : keyname =  "VK_TAB"; break;
        case  VK_CLEAR   : keyname =  "VK_CLEAR"; break;
        case  VK_RETURN  : keyname =  "VK_RETURN"; break;
        case  VK_PAUSE   : keyname =  "VK_PAUSE"; break;
        case  VK_CAPITAL : keyname =  "VK_CAPITAL"; break;
        case  VK_ESCAPE  : keyname =  "VK_ESCAPE"; break;
        case  VK_SPACE   : keyname =  "VK_SPACE"; break;
        case  VK_PRIOR   : keyname =  "VK_PRIOR"; break;
        case  VK_NEXT    : keyname =  "VK_NEXT"; break;
        case  VK_END     : keyname =  "VK_END"; break;
        case  VK_HOME    : keyname =  "VK_HOME"; break;
        case  VK_LEFT    : keyname =  "VK_LEFT"; break;
        case  VK_UP      : keyname =  "VK_UP"; break;
        case  VK_RIGHT   : keyname =  "VK_RIGHT"; break;
        case  VK_DOWN    : keyname =  "VK_DOWN"; break;
        case  VK_SELECT  : keyname =  "VK_SELECT"; break;
        case  VK_PRINT   : keyname =  "VK_PRINT"; break;
        case  VK_EXECUTE : keyname =  "VK_EXECUTE"; break;
        case  VK_SNAPSHOT: keyname =  "VK_SNAPSHOT"; break;
        case  VK_INSERT  : keyname =  "VK_INSERT"; break;
        case  VK_DELETE  : keyname =  "VK_DELETE"; break;
        case  VK_HELP    : keyname =  "VK_HELP"; break;
        case  VK_LWIN    : keyname =  "VK_LWIN"; break;
        case  VK_RWIN    : keyname =  "VK_RWIN"; break;
        case  VK_NUMPAD0 : keyname =  "VK_NUMPAD0"; break;
        case  VK_NUMPAD1 : keyname =  "VK_NUMPAD1"; break;
        case  VK_NUMPAD2 : keyname =  "VK_NUMPAD2"; break;
        case  VK_NUMPAD3 : keyname =  "VK_NUMPAD3"; break;
        case  VK_NUMPAD4 : keyname =  "VK_NUMPAD4"; break;
        case  VK_NUMPAD5 : keyname =  "VK_NUMPAD5"; break;
        case  VK_NUMPAD6 : keyname =  "VK_NUMPAD6"; break;
        case  VK_NUMPAD7 : keyname =  "VK_NUMPAD7"; break;
        case  VK_NUMPAD8 : keyname =  "VK_NUMPAD8"; break;
        case  VK_NUMPAD9 : keyname =  "VK_NUMPAD9"; break;
        case  VK_MULTIPLY: keyname =  "VK_MULTIPLY"; break;
        case  VK_ADD     : keyname =  "VK_ADD"; break;
        case  VK_SEPARATOR: keyname = "VK_SEPARATOR"; break;
        case  VK_SUBTRACT: keyname =  "VK_SUBTRACT"; break;
        case  VK_DECIMAL : keyname =  "VK_DECIMAL"; break;
        case  VK_DIVIDE  : keyname =  "VK_DIVIDE"; break;
        case  VK_F1      : keyname =  "VK_F1"; break;
        case  VK_F2      : keyname =  "VK_F2"; break;
        case  VK_F3      : keyname =  "VK_F3"; break;
        case  VK_F4      : keyname =  "VK_F4"; break;
        case  VK_F5      : keyname =  "VK_F5"; break;
        case  VK_F6      : keyname =  "VK_F6"; break;
        case  VK_F7      : keyname =  "VK_F7"; break;
        case  VK_F8      : keyname =  "VK_F8"; break;
        case  VK_F9      : keyname =  "VK_F9"; break;
        case  VK_F10     : keyname =  "VK_F10"; break;
        case  VK_F11     : keyname =  "VK_F11"; break;
        case  VK_F12     : keyname =  "VK_F12"; break;
        case  VK_F13     : keyname =  "VK_F13"; break;
        case  VK_F14     : keyname =  "VK_F14"; break;
        case  VK_F15     : keyname =  "VK_F15"; break;
        case  VK_F16     : keyname =  "VK_F16"; break;
        case  VK_F17     : keyname =  "VK_F17"; break;
        case  VK_F18     : keyname =  "VK_F18"; break;
        case  VK_F19     : keyname =  "VK_F19"; break;
        case  VK_F20     : keyname =  "VK_F20"; break;
        case  VK_F21     : keyname =  "VK_F21"; break;
        case  VK_F22     : keyname =  "VK_F22"; break;
        case  VK_F23     : keyname =  "VK_F23"; break;
        case  VK_F24     : keyname =  "VK_F24"; break;
        case  VK_NUMLOCK : keyname =  "VK_NUMLOCK"; break;
        case  VK_SCROLL  : keyname =  "VK_SCROLL"; break;
        case  VK_LSHIFT  : keyname =  "VK_LSHIFT"; break;
        case  VK_RSHIFT  : keyname =  "VK_RSHIFT"; break;
        case  VK_LCONTROL: keyname =  "VK_LCONTROL"; break;
        case  VK_RCONTROL: keyname =  "VK_RCONTROL"; break;
        default: keyname = "<unknown>"; break;
    }
    return keyname;
}

char *resolve_keymodifiers(int mask)
{
    if((mask & KC_CTRL) && (mask & KC_SHIFT) && (mask & KC_ALT))
        return "KC_CTRL KC_SHIFT KC_ALT";
    else if((mask & KC_CTRL) && (mask & KC_SHIFT))
        return "KC_CTRL KC_SHIFT";
    else if((mask & KC_CTRL) && (mask & KC_ALT))
        return "KC_CTRL KC_ALT";
    else if((mask & KC_SHIFT) && (mask & KC_ALT))
        return "KC_SHIFT KC_ALT";
    else if((mask & KC_SHIFT))
        return "KC_SHIFT";
    else if((mask & KC_CTRL))
        return "KC_CTRL";
    else if((mask & KC_ALT))
        return "KC_ALT";
    else return "none";
}

void update_render(void);

/* This gets called when a part of the graph needs to be repainted. */
int DWSIGNAL text_expose(HWND hwnd, DWExpose *exp, void *data)
{
    if(render_type != SHAPES_DIRECT)
    {
        HPIXMAP hpm;
        unsigned long width,height;

        if(dw_window_compare(hwnd, textbox1))
            hpm = text1pm;
        else if(dw_window_compare(hwnd, textbox2))
            hpm = text2pm;
        else
            return TRUE;

        width = (int)DW_PIXMAP_WIDTH(hpm);
        height = (int)DW_PIXMAP_HEIGHT(hpm);

        dw_pixmap_bitblt(hwnd, NULL, 0, 0, (int)width, (int)height, 0, hpm, 0, 0 );
        dw_flush();
    }
    else
    {
        update_render();
    }
    return TRUE;
}

char *read_file(char *filename)
{
    char *errors = NULL;
#ifdef __ANDROID__
    int fd = -1;

    /* Special way to open for URIs on Android */
    if(strstr(filename, "://"))
    {
        fd = dw_file_open(filename, O_RDONLY);
        fp = fdopen(fd, "r");
    }
    else
#endif
        fp = fopen(filename, "r");
    if(!fp)
        errors = strerror(errno);
    else
    {
        int i,len;

        lp = (char **)calloc(1000,sizeof(char *));
        /* should test for out of memory */
        max_linewidth=0;
        for(i=0; i<1000; i++)
        {
           lp[i] = (char *)calloc(1, 1025);
           if (fgets(lp[i], 1024, fp) == NULL)
               break;
           len = (int)strlen(lp[i]);
           if (len > max_linewidth)
               max_linewidth = len;
           if(lp[i][len - 1] == '\n')
               lp[i][len - 1] = '\0';
        }
        num_lines = i;
        fclose(fp);
        dw_scrollbar_set_range(hscrollbar, max_linewidth, cols);
        dw_scrollbar_set_pos(hscrollbar, 0);
        dw_scrollbar_set_range(vscrollbar, num_lines, rows);
        dw_scrollbar_set_pos(vscrollbar, 0);
    }
#ifdef __ANDROID__
    if(fd != -1)
        close(fd);
#endif
    return errors;
}

/* When hpma is not NULL we are printing.. so handle things differently */
void draw_file(int row, int col, int nrows, int fheight, HPIXMAP hpma)
{
    HPIXMAP hpm = hpma ? hpma : text2pm;
    char buf[15] = {0};
    int i,y,fileline;
    char *pLine;

    if(current_file)
    {
        dw_color_foreground_set(DW_CLR_WHITE);
        if(!hpma)
            dw_draw_rect(0, text1pm, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)DW_PIXMAP_WIDTH(text1pm), (int)DW_PIXMAP_HEIGHT(text1pm));
        dw_draw_rect(0, hpm, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)DW_PIXMAP_WIDTH(hpm), (int)DW_PIXMAP_HEIGHT(hpm));

        for(i = 0;(i < nrows) && (i+row < num_lines); i++)
        {
            fileline = i + row - 1;
            y = i*fheight;
            dw_color_background_set(1 + (fileline % 15));
            dw_color_foreground_set(fileline < 0 ? DW_CLR_WHITE : fileline % 16);
            if(!hpma)
            {
                snprintf(buf, 15, "%6.6d", i+row);
                dw_draw_text(0, text1pm, 0, y, buf);
            }
            pLine = lp[i+row];
            dw_draw_text(0, hpm, 0, y, pLine+col);
        }
    }
}

/* When hpma is not NULL we are printing.. so handle things differently */
void draw_shapes(int direct, HPIXMAP hpma)
{
    HPIXMAP hpm = hpma ? hpma : text2pm;
    int width = (int)DW_PIXMAP_WIDTH(hpm), height = (int)DW_PIXMAP_HEIGHT(hpm);
    HPIXMAP pixmap = direct ? NULL : hpm;
    HWND window = direct ? textbox2 : 0;
    int x[7] = { 20, 180, 180, 230, 180, 180, 20 };
    int y[7] = { 50, 50, 20, 70, 120, 90, 90 };

    image_x = (int)dw_spinbutton_get_pos(imagexspin);
    image_y = (int)dw_spinbutton_get_pos(imageyspin);
    image_stretch = dw_checkbox_get(imagestretchcheck);

    dw_color_foreground_set(DW_CLR_WHITE);
    dw_draw_rect(window, pixmap, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, width, height);
    dw_color_foreground_set(DW_CLR_DARKPINK);
    dw_draw_rect(window, pixmap, DW_DRAW_FILL | DW_DRAW_NOAA, 10, 10, width - 20, height - 20);
    dw_color_foreground_set(DW_CLR_GREEN);
    dw_color_background_set(DW_CLR_DARKRED);
    dw_draw_text(window, pixmap, 10, 10, "This should be aligned with the edges.");
    dw_color_foreground_set(DW_CLR_YELLOW);
    dw_draw_line(window, pixmap, width - 10, 10, 10, height - 10);
    dw_color_foreground_set(DW_CLR_BLUE);
    dw_draw_polygon(window, pixmap, DW_DRAW_FILL, 7, x, y);
    dw_color_foreground_set(DW_CLR_BLACK);
    dw_draw_rect(window, pixmap, DW_DRAW_FILL | DW_DRAW_NOAA, 80, 80, 80, 40);
    dw_color_foreground_set(DW_CLR_CYAN);
    /* Bottom right corner */
    dw_draw_arc(window, pixmap, 0, width - 30, height - 30, width - 10, height - 30, width - 30, height - 10);
    /* Top right corner */
    dw_draw_arc(window, pixmap, 0, width - 30, 30, width - 30, 10, width - 10, 30);
    /* Bottom left corner */
    dw_draw_arc(window, pixmap, 0, 30, height - 30, 30, height - 10, 10, height - 30);
    /* Full circle in the left top area */
    dw_draw_arc(window, pixmap, DW_DRAW_FULL, 120, 100, 80, 80, 160, 120);
    if(image)
    {
        if(image_stretch)
            dw_pixmap_stretch_bitblt(window, pixmap, 10, 10, width - 20, height - 20, 0, image, 0, 0, (int)DW_PIXMAP_WIDTH(image), (int)DW_PIXMAP_HEIGHT(image));
        else
            dw_pixmap_bitblt(window, pixmap, image_x, image_y, (int)DW_PIXMAP_WIDTH(image), (int)DW_PIXMAP_HEIGHT(image), 0, image, 0, 0);
    }
}

void update_render(void)
{
    switch(render_type)
    {
        case SHAPES_DOUBLE_BUFFERED:
            draw_shapes(FALSE, NULL);
            break;
        case SHAPES_DIRECT:
            draw_shapes(TRUE, NULL);
            break;
        case DRAW_FILE:
            draw_file(current_row, current_col, rows, font_height, NULL);
            break;
    }
}

/* Request that the render widgets redraw...
 * If not using direct rendering, call update_render() to
 * redraw the in memory pixmaps. Then trigger the expose events.
 * Expose will call update_render() to draw directly or bitblt the pixmaps.
 */
void render_draw(void)
{
    /* If we are double buffered, draw to the pixmaps */
    if(render_type != SHAPES_DIRECT)
        update_render();
    /* Trigger expose event */
    dw_render_redraw(textbox1);
    dw_render_redraw(textbox2);
}

int DWSIGNAL draw_page(HPRINT print, HPIXMAP pixmap, int page_num, void *data)
{
   dw_pixmap_set_font(pixmap, FIXEDFONT);
   if(page_num == 0)
   {
       draw_shapes(FALSE, pixmap);
   }
   else if(page_num == 1)
   {
       /* Get the font size for this printer context... */
       int fheight, fwidth;

       /* If we have a file to display... */
       if(current_file)
       {
           int nrows;

           /* Calculate new dimensions */
           dw_font_text_extents_get(0, pixmap, "(g", NULL, &fheight);
           nrows = (int)(DW_PIXMAP_HEIGHT(pixmap) / fheight);

           /* Do the actual drawing */
           draw_file(0, 0, nrows, fheight, pixmap);
       }
       else
       {
           /* We don't have a file so center an error message on the page */
           char *text = "No file currently selected!";
           int posx, posy;

           dw_font_text_extents_get(0, pixmap, text, &fwidth, &fheight);

           posx = (int)(DW_PIXMAP_WIDTH(pixmap) - fwidth)/2;
           posy = (int)(DW_PIXMAP_HEIGHT(pixmap) - fheight)/2;

           dw_color_foreground_set(DW_CLR_BLACK);
           dw_color_background_set(DW_CLR_WHITE);
           dw_draw_text(0, pixmap, posx, posy, text);
       }
   }
   return TRUE;
}

int DWSIGNAL print_callback(HWND window, void *data)
{
   HPRINT print = dw_print_new("DWTest Job", 0, 2, DW_SIGNAL_FUNC(draw_page), NULL);
   dw_print_run(print, 0);
   return FALSE;
}

int DWSIGNAL refresh_callback(HWND window, void *data)
{
    render_draw();
    return FALSE;
}

int DWSIGNAL render_select_event_callback(HWND window, int index)
{
    if(index != render_type)
    {
        if(index == DRAW_FILE)
        {
            dw_scrollbar_set_range(hscrollbar, max_linewidth, cols);
            dw_scrollbar_set_pos(hscrollbar, 0);
            dw_scrollbar_set_range(vscrollbar, num_lines, rows);
            dw_scrollbar_set_pos(vscrollbar, 0);
            current_col = current_row = 0;
        }
        else
        {
            dw_scrollbar_set_range(hscrollbar, 0, 0);
            dw_scrollbar_set_pos(hscrollbar, 0);
            dw_scrollbar_set_range(vscrollbar, 0, 0);
            dw_scrollbar_set_pos(vscrollbar, 0);
        }
        render_type = index;
        render_draw();
    }
    return FALSE;
}

int DWSIGNAL colorchoose_callback(HWND window, void *data)
{
    current_color = dw_color_choose(current_color);
    return FALSE;
}

int DWSIGNAL cursortoggle_callback(HWND window, void *data)
{
    if(cursor_arrow)
    {
        dw_window_set_text((HWND)cursortogglebutton,"Set Cursor pointer - ARROW");
        dw_window_set_pointer((HWND)data,DW_POINTER_CLOCK);
        cursor_arrow = 0;
    }
    else
    {
        dw_window_set_text((HWND)cursortogglebutton,"Set Cursor pointer - CLOCK");
        dw_window_set_pointer((HWND)data,DW_POINTER_DEFAULT);
        cursor_arrow = 1;
    }
    return FALSE;
}

int DWSIGNAL beep_callback(HWND window, void *data)
{
    dw_timer_disconnect(timerid);
    timerid = 0;
    return TRUE;
}

int DWSIGNAL keypress_callback(HWND window, char ch, int vk, int state, void *data, char *utf8)
{
    char tmpbuf[100];
    if ( ch )
        sprintf(tmpbuf, "Key: %c(%d) Modifiers: %s(%d) utf8 %s", ch, ch, resolve_keymodifiers(state), state,  utf8);
    else
        sprintf(tmpbuf, "Key: %s(%d) Modifiers: %s(%d) utf8 %s", resolve_keyname(vk), vk, resolve_keymodifiers(state), state, utf8);
    dw_window_set_text(status1, tmpbuf);
    return 0;
}

int DWSIGNAL menu_callback(HWND window, void *data)
{
    char buf[100];

    sprintf(buf, "%s menu item selected", (char *)data);
    dw_messagebox("Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, buf);
    return 0;
}

int DWSIGNAL menutoggle_callback(HWND window, void *data)
{
    if (menu_enabled)
    {
        dw_menu_item_set_state(changeable_menu, CHECKABLE_MENUITEMID, DW_MIS_DISABLED);
        dw_menu_item_set_state(changeable_menu, NONCHECKABLE_MENUITEMID, DW_MIS_DISABLED);
        menu_enabled = 0;
    }
    else
    {
        dw_menu_item_set_state(changeable_menu, CHECKABLE_MENUITEMID, DW_MIS_ENABLED);
        dw_menu_item_set_state(changeable_menu, NONCHECKABLE_MENUITEMID, DW_MIS_ENABLED);
        menu_enabled = 1;
    }
    return 0;
}

int DWSIGNAL helpabout_callback(HWND window, void *data)
{
    DWEnv env;

    dw_environment_query(&env);
    dw_messagebox("About dwindows", DW_MB_OK | DW_MB_INFORMATION, "dwindows test\n\nOS: %s %s %s Version: %d.%d.%d.%d\n\nHTML: %s\n\ndwindows Version: %d.%d.%d\n\nScreen: %dx%d %dbpp",
                   env.osName, env.buildDate, env.buildTime,
                   env.MajorVersion, env.MinorVersion, env.MajorBuild, env.MinorBuild,
                   env.htmlEngine,
                   env.DWMajorVersion, env.DWMinorVersion, env.DWSubVersion,
                   dw_screen_width(), dw_screen_height(), dw_color_depth_get());
    return 0;
}

int DWSIGNAL exit_callback(HWND window, void *data)
{
    if(dw_messagebox("dwtest", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?"))
    {
        dw_main_quit();
    }
    return TRUE;
}

int DWSIGNAL notification_clicked_callback(HWND notification, void *data)
{
    dw_debug("Notification clicked\n");
    return TRUE;
}

int DWSIGNAL browse_file_callback(HWND window, void *data)
{
    char *tmp = dw_file_browse("Pick a file", "dwtest.c", "c", DW_FILE_OPEN);
    if(tmp)
    {
        char *errors = read_file(tmp);
        char *title = "New file load";
        char *image = "image/test.png";
        HWND notification;

        if(errors)
            notification = dw_notification_new(title, image,"dwtest failed to load \"%s\" into the file browser, %s.", tmp, errors);
        else
            notification = dw_notification_new(title, image,"dwtest loaded \"%s\" into the file browser on the Render tab, with \"File Display\" selected from the drop down list.", tmp);

        if(current_file)
            dw_free(current_file);
        current_file = tmp;
        dw_window_set_text(entryfield, current_file);
        current_col = current_row = 0;
        render_draw();
        dw_signal_connect(notification, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(notification_clicked_callback), NULL);
        dw_notification_send(notification);
    }
    dw_window_set_focus(copypastefield);
    return 0;
}

int DWSIGNAL browse_folder_callback(HWND window, void *data)
{
    char *tmp = dw_file_browse("Pick a folder", ".", "c", DW_DIRECTORY_OPEN);
    dw_debug("Folder picked: %s\n", tmp ? tmp : "None");
    return 0;
}

int DWSIGNAL button_callback(HWND window, void *data)
{
    unsigned int y,m,d;
    unsigned int idx;
    int len;
    long spvalue;
    char buf1[100] = {0};
    char buf2[100] = {0};
    char buf3[500] = {0};

    idx = dw_listbox_selected(combobox1);
    dw_listbox_get_text(combobox1, idx, buf1, 99);
    idx = dw_listbox_selected(combobox2);
    dw_listbox_get_text(combobox2, idx, buf2, 99);
    dw_calendar_get_date(cal, &y, &m, &d);
    spvalue = dw_spinbutton_get_pos(spinbutton);
    len = sprintf(buf3, "spinbutton: %ld\ncombobox1: \"%s\"\ncombobox2: \"%s\"\ncalendar: %d-%d-%d",
                  spvalue,
                  buf1, buf2,
                  y, m, d);
    dw_messagebox("Values", DW_MB_OK | DW_MB_INFORMATION, buf3);
    dw_clipboard_set_text(buf3, len);
    return 0;
}

int DWSIGNAL bitmap_toggle_callback(HWND window, void *data)
{
    static int isfoldericon = 1;

    if(isfoldericon)
    {
        isfoldericon = 0;
        dw_window_set_bitmap(window, 0, fileiconpath);
        dw_window_set_tooltip(window, "File Icon");
    }
    else
    {
        isfoldericon = 1;
        dw_window_set_bitmap_from_data(window, 0, folder_ico, sizeof(folder_ico));
        dw_window_set_tooltip(window, "Folder Icon");
    }
    return 0;
}

int DWSIGNAL percent_button_box_callback(HWND window, void *data)
{
    dw_percent_set_pos(percent, DW_PERCENT_INDETERMINATE);
    return 0;
}

int DWSIGNAL change_color_red_callback(HWND window, void *data)
{
    dw_window_set_color(buttonsbox, DW_CLR_RED, DW_CLR_RED);
    return 0;
}

int DWSIGNAL change_color_yellow_callback(HWND window, void *data)
{
    dw_window_set_color(buttonsbox, DW_CLR_YELLOW, DW_CLR_YELLOW);
    return 0;
}


/* Callback to handle user selection of the scrollbar position */
void DWSIGNAL scrollbar_valuechanged_callback(HWND hwnd, int value, void *data)
{
    if(data)
    {
        HWND stext = (HWND)data;
        char tmpbuf[100];

        if(hwnd == vscrollbar)
        {
            current_row = value;
        }
        else
        {
            current_col = value;
        }
        sprintf(tmpbuf, "Row:%d Col:%d Lines:%d Cols:%d", current_row,current_col,num_lines,max_linewidth);
        dw_window_set_text(stext, tmpbuf);
        render_draw();
    }
}

/* Callback to handle user selection of the spinbutton position */
void DWSIGNAL spinbutton_valuechanged_callback(HWND hwnd, int value, void *data)
{
    dw_messagebox("DWTest", DW_MB_OK, "New value from spinbutton: %d\n", value);
}

/* Callback to handle user selection of the slider position */
void DWSIGNAL slider_valuechanged_callback(HWND hwnd, int value, void *data)
{
    dw_percent_set_pos(percent, value * 10);
}

/* Handle size change of the main render window */
int DWSIGNAL configure_event(HWND hwnd, int width, int height, void *data)
{
    HPIXMAP old1 = text1pm, old2 = text2pm;
    unsigned long depth = dw_color_depth_get();

    rows = height / font_height;
    cols = width / font_width;

    /* Create new pixmaps with the current sizes */
    text1pm = dw_pixmap_new(textbox1, (unsigned long)(font_width*(width1)), (unsigned long)height, (int)depth);
    text2pm = dw_pixmap_new(textbox2, (unsigned long)width, (unsigned long)height, (int)depth);

    /* Make sure the side area is cleared */
    dw_color_foreground_set(DW_CLR_WHITE);
    dw_draw_rect(0, text1pm, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)DW_PIXMAP_WIDTH(text1pm), (int)DW_PIXMAP_HEIGHT(text1pm));

   /* Destroy the old pixmaps */
    dw_pixmap_destroy(old1);
    dw_pixmap_destroy(old2);

    /* Update scrollbar ranges with new values */
    dw_scrollbar_set_range(hscrollbar, max_linewidth, cols);
    dw_scrollbar_set_range(vscrollbar, num_lines, rows);

    /* Redraw the render widgets */
    render_draw();
    return TRUE;
}

int DWSIGNAL item_enter_cb(HWND window, char *text, void *data, void *itemdata)
{
    char buf[200];
    HWND statline = (HWND)data;

    sprintf(buf,"DW_SIGNAL_ITEM_ENTER: Window: %x Text: %s Itemdata: %x", DW_POINTER_TO_UINT(window), text, DW_POINTER_TO_UINT(itemdata));
    dw_window_set_text(statline, buf);
    return 0;
}

/* Context menus */
int DWSIGNAL context_menu_cb(HWND hwnd, void *data)
{
    char buf[200];
    HWND statline = (HWND)data;

    sprintf(buf,"DW_SIGNAL_CLICKED: Menu: %x Container context menu clicked", DW_POINTER_TO_UINT(hwnd));
    dw_window_set_text(statline, buf);
    return 0;
}

HMENUI item_context_menu_new(char *text, void *data)
{
    HMENUI hwndMenu = dw_menu_new(0L);
    HMENUI hwndSubMenu = dw_menu_new(0L);
    HWND menuitem = dw_menu_append_item(hwndSubMenu, "File", DW_MENU_POPUP, 0L, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);
    menuitem = dw_menu_append_item(hwndSubMenu, "Date", DW_MENU_POPUP, 0L, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);
    menuitem = dw_menu_append_item(hwndSubMenu, "Size", DW_MENU_POPUP, 0L, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);
    menuitem = dw_menu_append_item(hwndSubMenu, "None", DW_MENU_POPUP, 0L, TRUE, TRUE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);

    menuitem = dw_menu_append_item(hwndMenu, "Sort", DW_MENU_POPUP, 0L, TRUE, FALSE, hwndSubMenu);

    menuitem = dw_menu_append_item(hwndMenu, "Make Directory", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);

    dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, FALSE, DW_NOMENU);
    menuitem = dw_menu_append_item(hwndMenu, "Rename Entry", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);

    menuitem = dw_menu_append_item(hwndMenu, "Delete Entry", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);

    dw_menu_append_item(hwndMenu, "", 0L, 0L, TRUE, FALSE, DW_NOMENU);
    menuitem = dw_menu_append_item(hwndMenu, "View File", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(context_menu_cb), data);

    return hwndMenu;
}

int DWSIGNAL item_context_cb(HWND window, char *text, int x, int y, void *data, void *itemdata)
{
    char buf[200];
    HWND statline = (HWND)data;
    HMENUI popupmenu = item_context_menu_new(text, data);

    sprintf(buf,"DW_SIGNAL_ITEM_CONTEXT: Window: %x Text: %s x: %d y: %d Itemdata: %x", DW_POINTER_TO_UINT(window), text, x, y, DW_POINTER_TO_UINT(itemdata));
    dw_window_set_text(statline, buf);
    dw_menu_popup(&popupmenu, mainwindow, x, y);
    return 0;
}

int DWSIGNAL list_select_cb(HWND window, int item, void *data)
{
    char buf[200];
    HWND statline = (HWND)data;

    sprintf(buf,"DW_SIGNAL_LIST_SELECT: Window: %d Item: %d", DW_POINTER_TO_UINT(window), item);
    dw_window_set_text(statline, buf);
    return 0;
}

int DWSIGNAL item_select_cb(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
    char buf[200];
    HWND statline = (HWND)data;

    sprintf(buf,"DW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x", DW_POINTER_TO_UINT(window),
            DW_POINTER_TO_UINT(item), text, DW_POINTER_TO_UINT(itemdata));
    dw_window_set_text(statline, buf);
    return 0;
}

int DWSIGNAL container_select_cb(HWND window, HTREEITEM item, char *text, void *data, void *itemdata)
{
    char buf[200];
    char *str;
    HWND statline = (HWND)data;
    unsigned long size;

    sprintf(buf,"DW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x", DW_POINTER_TO_UINT(window),
            DW_POINTER_TO_UINT(item), text, DW_POINTER_TO_UINT(itemdata));
    dw_window_set_text( statline, buf);
    sprintf(buf,"\r\nDW_SIGNAL_ITEM_SELECT: Window: %x Item: %x Text: %s Itemdata: %x\r\n", DW_POINTER_TO_UINT(window),
            DW_POINTER_TO_UINT(item), text, DW_POINTER_TO_UINT(itemdata));
    mle_point = dw_mle_import( container_mle, buf, mle_point);
    str = dw_container_query_start(container, DW_CRA_SELECTED);
    while(str)
    {
        sprintf(buf,"Selected: %s\r\n", str);
        mle_point = dw_mle_import( container_mle, buf, mle_point);
        dw_free(str);
        str = dw_container_query_next(container, DW_CRA_SELECTED);
    }
    /* Make the last inserted point the cursor location */
    dw_mle_set_cursor(container_mle, mle_point);
    /* set the details of item 0 to new data */
    dw_debug("In cb: container: %x containerinfo: %x icon: %x\n", DW_POINTER_TO_INT(container),
            DW_POINTER_TO_INT(containerinfo), DW_POINTER_TO_INT(fileicon));
    dw_filesystem_change_file(container, 0, "new data", fileicon);
    size = 999;
    dw_debug("In cb: container: %x containerinfo: %x icon: %x\n", DW_POINTER_TO_INT(container),
            DW_POINTER_TO_INT(containerinfo), DW_POINTER_TO_INT(fileicon));
    dw_filesystem_change_item(container, 1, 0, &size);
    return 0;
}

int DWSIGNAL switch_page_cb(HWND window, unsigned long page_num, void *itemdata)
{
    dw_debug("DW_SIGNAL_SWITCH_PAGE: Window: %x PageNum: %u Itemdata: %x\n", DW_POINTER_TO_UINT(window),
              DW_POINTER_TO_UINT(page_num), DW_POINTER_TO_UINT(itemdata));
    return 0;
}

int DWSIGNAL column_click_cb(HWND window, int column_num, void *data)
{
    char buf[200], buf1[100];
    HWND statline = (HWND)data;
    int column_type;

    if(column_num == 0)
        strcpy(buf1,"Filename");
    else
    {
        column_type = dw_filesystem_get_column_type(window, column_num-1);
        if(column_type == DW_CFA_STRING)
            strcpy(buf1,"String");
        else if(column_type == DW_CFA_ULONG)
            strcpy(buf1,"ULong");
        else if(column_type == DW_CFA_DATE)
            strcpy(buf1,"Date");
        else if(column_type == DW_CFA_TIME)
            strcpy(buf1,"Time");
        else if(column_type == DW_CFA_BITMAPORICON)
            strcpy(buf1,"BitmapOrIcon");
        else
            strcpy(buf1,"Unknown");
    }
    sprintf(buf,"DW_SIGNAL_COLUMN_CLICK: Window: %x Column: %d Type: %s Itemdata: %x", DW_POINTER_TO_UINT(window),
            column_num, buf1, DW_POINTER_TO_UINT(data) );
    dw_window_set_text( statline, buf);
    return 0;
}

int DWSIGNAL combobox_select_event_callback(HWND window, int index)
{
    dw_debug("got combobox_select_event for index: %d, iteration: %d\n", index, iteration++);
    return FALSE;
}

int DWSIGNAL copy_clicked_callback(HWND button, void *data)
{
    char *test = dw_window_get_text(copypastefield);

    if(test)
    {
        dw_clipboard_set_text(test, (int)strlen(test));
        dw_free(test);
    }
    dw_window_set_focus(entryfield);
    return TRUE;
}

int DWSIGNAL paste_clicked_callback(HWND button, void *data)
{
    char *test = dw_clipboard_get_text();
    if(test)
    {
        dw_window_set_text(copypastefield, test);
        dw_free(test);
    }
    return TRUE;
}

void archive_add(void)
{
    HWND browsefilebutton, browsefolderbutton, copybutton, pastebutton, browsebox;

    lbbox = dw_box_new(DW_VERT, 10);

    dw_box_pack_start(notebookbox1, lbbox, 150, 70, TRUE, TRUE, 0);

    /* Copy and Paste */
    browsebox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(lbbox, browsebox, 0, 0, FALSE, FALSE, 0);

    copypastefield = dw_entryfield_new("", 0);

    dw_entryfield_set_limit(copypastefield, 260);

    dw_box_pack_start(browsebox, copypastefield, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 4);

    copybutton = dw_button_new("Copy", 0);

    dw_box_pack_start(browsebox, copybutton, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);

    pastebutton = dw_button_new("Paste", 0);

    dw_box_pack_start(browsebox, pastebutton, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);

    /* Archive Name */
    stext = dw_text_new("File to browse", 0);

    dw_window_set_style(stext, DW_DT_VCENTER, DW_DT_VCENTER);

    dw_box_pack_start(lbbox, stext, 130, 15, TRUE, TRUE, 2);

    browsebox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(lbbox, browsebox, 0, 0, TRUE, TRUE, 0);

    entryfield = dw_entryfield_new("", 100L);

    dw_entryfield_set_limit(entryfield, 260);

    dw_box_pack_start(browsebox, entryfield, 100, 15, TRUE, TRUE, 4);

    browsefilebutton = dw_button_new("Browse File", 1001L);

    dw_box_pack_start(browsebox, browsefilebutton, 40, 15, TRUE, TRUE, 0);

    browsefolderbutton = dw_button_new("Browse Folder", 1001L);

    dw_box_pack_start(browsebox, browsefolderbutton, 40, 15, TRUE, TRUE, 0);

    dw_window_set_color(browsebox, DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
    dw_window_set_color(stext, DW_CLR_BLACK, DW_CLR_PALEGRAY);

    /* Buttons */
    buttonbox = dw_box_new(DW_HORZ, 10);

    dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, TRUE, 0);

    cancelbutton = dw_button_new("Exit", 1002L);
    dw_box_pack_start(buttonbox, cancelbutton, 130, 30, TRUE, TRUE, 2);

    cursortogglebutton = dw_button_new("Set Cursor pointer - CLOCK", 1003L);
    dw_box_pack_start(buttonbox, cursortogglebutton, 130, 30, TRUE, TRUE, 2);

    okbutton = dw_button_new("Turn Off Annoying Beep!", 1001L);
    dw_box_pack_start(buttonbox, okbutton, 130, 30, TRUE, TRUE, 2);

    dw_box_unpack(cancelbutton);
    dw_box_pack_start(buttonbox, cancelbutton, 130, 30, TRUE, TRUE, 2);
    dw_window_click_default( mainwindow, cancelbutton );

    colorchoosebutton = dw_button_new("Color Chooser Dialog", 1004L);
    dw_box_pack_at_index(buttonbox, colorchoosebutton, 1, 130, 30, TRUE, TRUE, 2);

    /* Set some nice fonts and colors */
    dw_window_set_color(lbbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
    dw_window_set_color(buttonbox, DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
    dw_window_set_color(okbutton, DW_CLR_PALEGRAY, DW_CLR_DARKCYAN);
#ifdef COLOR_DEBUG
    dw_window_set_color(copypastefield, DW_CLR_WHITE, DW_CLR_RED);
    dw_window_set_color(copybutton, DW_CLR_WHITE, DW_CLR_RED);
    /* Set a color then clear it to make sure it clears correctly */
    dw_window_set_color(entryfield, DW_CLR_WHITE, DW_CLR_RED);
    dw_window_set_color(entryfield, DW_CLR_DEFAULT, DW_CLR_DEFAULT);
    /* Set a color then clear it to make sure it clears correctly... again */
    dw_window_set_color(pastebutton, DW_CLR_WHITE, DW_CLR_RED);
    dw_window_set_color(pastebutton, DW_CLR_DEFAULT, DW_CLR_DEFAULT);
#endif

    dw_signal_connect(browsefilebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(browse_file_callback), DW_POINTER(notebookbox1));
    dw_signal_connect(browsefolderbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(browse_folder_callback), DW_POINTER(notebookbox1));
    dw_signal_connect(copybutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(copy_clicked_callback), DW_POINTER(copypastefield));
    dw_signal_connect(pastebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(paste_clicked_callback), DW_POINTER(copypastefield));
    dw_signal_connect(okbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(beep_callback), DW_POINTER(notebookbox1));
    dw_signal_connect(cancelbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(exit_callback), DW_POINTER(mainwindow));
    dw_signal_connect(cursortogglebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(cursortoggle_callback), DW_POINTER(mainwindow));
    dw_signal_connect(colorchoosebutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(colorchoose_callback), DW_POINTER(mainwindow));
}

int API motion_notify_event(HWND window, int x, int y, int buttonmask, void *data)
{
    char buf[200];
    sprintf(buf, "%s: %dx%d buttons %d", data ? "motion_notify" : "button_press", x, y, buttonmask);
    dw_window_set_text(status2, buf);
    return 0;
}

int DWSIGNAL show_window_callback(HWND window, void *data)
{
    HWND thiswindow = (HWND)data;
    if(thiswindow)
    {
        dw_window_show(thiswindow);
        dw_window_raise(thiswindow);
    }
    return TRUE;
}

int API context_menu_event(HWND window, int x, int y, int buttonmask, void *data)
{
    HMENUI hwndMenu = dw_menu_new(0L);
    HWND menuitem = dw_menu_append_item(hwndMenu, "~Quit", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    long px, py;

    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(exit_callback), DW_POINTER(mainwindow));
    dw_menu_append_item(hwndMenu, DW_MENU_SEPARATOR, DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    menuitem = dw_menu_append_item(hwndMenu, "~Show Window", DW_MENU_POPUP, 0L, TRUE, FALSE, DW_NOMENU);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(show_window_callback), DW_POINTER(mainwindow));
    dw_pointer_query_pos(&px, &py);
    /* Use the toplevel window handle here.... because on the Mac..
     * using the control itself, when a different tab is active
     * the control is removed from the window and can no longer
     * handle the messages.
     */
    dw_menu_popup(&hwndMenu, mainwindow, (int)px, (int)py);
    return TRUE;
}

void text_add(void)
{
    unsigned long depth = dw_color_depth_get();
    HWND vscrollbox, hbox, button1, button2, label;
    int vscrollbarwidth, hscrollbarheight;
    wchar_t widestring[100] = L"DWTest Wide";
    char *utf8string = dw_wchar_to_utf8(widestring);

    /* create a box to pack into the notebook page */
    pagebox = dw_box_new(DW_HORZ, 2);
    dw_box_pack_start(notebookbox2, pagebox, 0, 0, TRUE, TRUE, 0);
    /* now a status area under this box */
    hbox = dw_box_new(DW_HORZ, 1 );
    dw_box_pack_start(notebookbox2, hbox, 100, 20, TRUE, FALSE, 1);
    status1 = dw_status_text_new("", 0);
    dw_box_pack_start(hbox, status1, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);
    status2 = dw_status_text_new("", 0);
    dw_box_pack_start(hbox, status2, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);
    /* a box with combobox and button */
    hbox = dw_box_new(DW_HORZ, 1 );
    dw_box_pack_start(notebookbox2, hbox, 100, 25, TRUE, FALSE, 1);
    rendcombo = dw_combobox_new( "Shapes Double Buffered", 0 );
    dw_box_pack_start(hbox, rendcombo, 80, 25, TRUE, TRUE, 0);
    dw_listbox_append(rendcombo, "Shapes Double Buffered");
    dw_listbox_append(rendcombo, "Shapes Direct");
    dw_listbox_append(rendcombo, "File Display");
    label = dw_text_new("Image X:", 100);
    dw_window_set_style(label, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(hbox, label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);
    imagexspin = dw_spinbutton_new("20", 1021);
    dw_box_pack_start(hbox, imagexspin, 25, 25, TRUE, TRUE, 0);
    label = dw_text_new("Y:", 100);
    dw_window_set_style(label, DW_DT_VCENTER | DW_DT_CENTER, DW_DT_VCENTER | DW_DT_CENTER);
    dw_box_pack_start(hbox, label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);
    imageyspin = dw_spinbutton_new("20", 1021);
    dw_box_pack_start(hbox, imageyspin, 25, 25, TRUE, TRUE, 0);
    dw_spinbutton_set_limits(imagexspin, 2000, 0);
    dw_spinbutton_set_limits(imageyspin, 2000, 0);
    dw_spinbutton_set_pos(imagexspin, 20);
    dw_spinbutton_set_pos(imageyspin, 20);
    imagestretchcheck = dw_checkbox_new("Stretch", 1021);
    dw_box_pack_start(hbox, imagestretchcheck, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

    button1 = dw_button_new("Refresh", 1223L );
    dw_box_pack_start(hbox, button1, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);
    button2 = dw_button_new("Print", 1224L );
    dw_box_pack_start(hbox, button2, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

    /* Pre-create the scrollbars so we can query their sizes */
    vscrollbar = dw_scrollbar_new(DW_VERT, 50);
    hscrollbar = dw_scrollbar_new(DW_HORZ, 50);
    dw_window_get_preferred_size(vscrollbar, &vscrollbarwidth, NULL);
    dw_window_get_preferred_size(hscrollbar, NULL, &hscrollbarheight);

    /* On GTK with overlay scrollbars enabled this returns us 0...
     * so in that case we need to give it some real values.
     */
    if(!vscrollbarwidth)
        vscrollbarwidth = 8;
    if(!hscrollbarheight)
        hscrollbarheight = 8;

    /* create render box for number pixmap */
    textbox1 = dw_render_new(100);
    dw_window_set_font(textbox1, FIXEDFONT);
    dw_font_text_extents_get(textbox1, NULL, "(g", &font_width, &font_height);
    font_width = font_width / 2;
    vscrollbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(vscrollbox, textbox1, font_width*width1, font_height*rows, FALSE, TRUE, 0);
    dw_box_pack_start(vscrollbox, 0, (font_width*(width1+1)), hscrollbarheight, FALSE, FALSE, 0);
    dw_box_pack_start(pagebox, vscrollbox, 0, 0, FALSE, TRUE, 0);

    /* pack empty space 1 character wide */
    dw_box_pack_start(pagebox, 0, font_width, 0, FALSE, TRUE, 0);

    /* create box for filecontents and horz scrollbar */
    textboxA = dw_box_new(DW_VERT,0 );
    dw_box_pack_start(pagebox, textboxA, 0, 0, TRUE, TRUE, 0);

    /* create render box for filecontents pixmap */
    textbox2 = dw_render_new(101);
    dw_box_pack_start(textboxA, textbox2, 10, 10, TRUE, TRUE, 0);
    dw_window_set_font(textbox2, FIXEDFONT);
    /* create horizonal scrollbar */
    dw_box_pack_start(textboxA, hscrollbar, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);

    /* create vertical scrollbar */
    vscrollbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(vscrollbox, vscrollbar, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, TRUE, 0);
    /* Pack an area of empty space 14x14 pixels */
    dw_box_pack_start(vscrollbox, 0, vscrollbarwidth, hscrollbarheight, FALSE, FALSE, 0);
    dw_box_pack_start(pagebox, vscrollbox, 0, 0, FALSE, TRUE, 0);

    text1pm = dw_pixmap_new(textbox1, font_width*width1, font_height*rows, (int)depth);
    text2pm = dw_pixmap_new(textbox2, font_width*cols, font_height*rows, (int)depth);
    image = dw_pixmap_new_from_file(textbox2, "image/test");
    if(!image)
        image = dw_pixmap_new_from_file(textbox2, "~/test");
    if(!image)
    {
        char *appdir = dw_app_dir();
        char pathbuff[1025] = {0};
        int pos = (int)strlen(appdir);
        
        strncpy(pathbuff, appdir, 1024);
        pathbuff[pos] = DW_DIR_SEPARATOR;
        pos++;
        strncpy(&pathbuff[pos], "test", 1024-pos);
        image = dw_pixmap_new_from_file(textbox2, pathbuff);
    }
    if(image)
        dw_pixmap_set_transparent_color(image, DW_CLR_WHITE);

    dw_messagebox(utf8string ? utf8string : "DWTest", DW_MB_OK|DW_MB_INFORMATION, "Width: %d Height: %d\n", font_width, font_height);
    if(utf8string)
        dw_free(utf8string);
    dw_draw_rect(0, text1pm, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*width1, font_height*rows);
    dw_draw_rect(0, text2pm, DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*cols, font_height*rows);
    dw_signal_connect(textbox1, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(context_menu_event), NULL);
    dw_signal_connect(textbox1, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(text_expose), NULL);
    dw_signal_connect(textbox2, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(text_expose), NULL);
    dw_signal_connect(textbox2, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(configure_event), text2pm);
    dw_signal_connect(textbox2, DW_SIGNAL_MOTION_NOTIFY, DW_SIGNAL_FUNC(motion_notify_event), DW_INT_TO_POINTER(1));
    dw_signal_connect(textbox2, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(motion_notify_event), DW_INT_TO_POINTER(0));
    dw_signal_connect(hscrollbar, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(scrollbar_valuechanged_callback), DW_POINTER(status1));
    dw_signal_connect(vscrollbar, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(scrollbar_valuechanged_callback), DW_POINTER(status1));
    dw_signal_connect(imagestretchcheck, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_callback), NULL);
    dw_signal_connect(button1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(refresh_callback), NULL);
    dw_signal_connect(button2, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(print_callback), NULL);
    dw_signal_connect(rendcombo, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(render_select_event_callback), NULL );
    dw_signal_connect(mainwindow, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(keypress_callback), NULL);

    dw_taskbar_insert(textbox1, fileicon, "DWTest");
}

void tree_add(void)
{
    HTREEITEM t1,t2;
    HWND listbox;

    /* create a box to pack into the notebook page */
    listbox = dw_listbox_new(1024, TRUE);
    dw_box_pack_start(notebookbox3, listbox, 500, 200, TRUE, TRUE, 0);
    dw_listbox_append(listbox, "Test 1");
    dw_listbox_append(listbox, "Test 2");
    dw_listbox_append(listbox, "Test 3");
    dw_listbox_append(listbox, "Test 4");
    dw_listbox_append(listbox, "Test 5");

    /* now a tree area under this box */
    tree = dw_tree_new(101);
    if(tree)
    {
        char *title;

        dw_box_pack_start(notebookbox3, tree, 500, 200, TRUE, TRUE, 1);

        /* and a status area to see whats going on */
        tree_status = dw_status_text_new("", 0);
        dw_box_pack_start(notebookbox3, tree_status, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

        /* set up our signal trappers... */
        dw_signal_connect(tree, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(item_context_cb), DW_POINTER(tree_status));
        dw_signal_connect(tree, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(item_select_cb), DW_POINTER(tree_status));

        t1 = dw_tree_insert(tree, "tree folder 1", foldericon, NULL, DW_INT_TO_POINTER(1));
        t2 = dw_tree_insert(tree, "tree folder 2", foldericon, NULL, DW_INT_TO_POINTER(2));
        dw_tree_insert(tree, "tree file 1", fileicon, t1, DW_INT_TO_POINTER(3));
        dw_tree_insert(tree, "tree file 2", fileicon, t1, DW_INT_TO_POINTER(4));
        dw_tree_insert(tree, "tree file 3", fileicon, t2, DW_INT_TO_POINTER(5));
        dw_tree_insert(tree, "tree file 4", fileicon, t2, DW_INT_TO_POINTER(6));
        dw_tree_item_change(tree, t1, "tree folder 1", foldericon);
        dw_tree_item_change(tree, t2, "tree folder 2", foldericon);
        dw_tree_item_set_data(tree, t2, DW_INT_TO_POINTER(100));
        dw_tree_item_expand(tree, t1);
        title = dw_tree_get_title(tree, t1);
        dw_debug("t1 title \"%s\" data %d t2 data %d\n", title, DW_POINTER_TO_INT(dw_tree_item_get_data(tree, t1)),
                 DW_POINTER_TO_INT(dw_tree_item_get_data(tree, t2)));
        dw_free(title);
    }
    else
    {
        tree = dw_text_new("Tree widget not available.", 0);
        dw_box_pack_start(notebookbox3, tree, 500, 200, TRUE, TRUE, 1);
    }
}

int DWSIGNAL word_wrap_click_cb(HWND wordwrap, void *data)
{
    HWND container_mle = (HWND)data;

    dw_mle_set_word_wrap(container_mle, dw_checkbox_get(wordwrap));
    return TRUE;
}

HWND color_combobox(void)
{
    HWND combobox = dw_combobox_new("DW_CLR_DEFAULT", 0);

    dw_listbox_append(combobox, "DW_CLR_DEFAULT");
    dw_listbox_append(combobox, "DW_CLR_BLACK");
    dw_listbox_append(combobox, "DW_CLR_DARKRED");
    dw_listbox_append(combobox, "DW_CLR_DARKGREEN");
    dw_listbox_append(combobox, "DW_CLR_BROWN");
    dw_listbox_append(combobox, "DW_CLR_DARKBLUE");
    dw_listbox_append(combobox, "DW_CLR_DARKPINK");
    dw_listbox_append(combobox, "DW_CLR_DARKCYAN");
    dw_listbox_append(combobox, "DW_CLR_PALEGRAY");
    dw_listbox_append(combobox, "DW_CLR_DARKGRAY");
    dw_listbox_append(combobox, "DW_CLR_RED");
    dw_listbox_append(combobox, "DW_CLR_GREEN");
    dw_listbox_append(combobox, "DW_CLR_YELLOW");
    dw_listbox_append(combobox, "DW_CLR_BLUE");
    dw_listbox_append(combobox, "DW_CLR_PINK");
    dw_listbox_append(combobox, "DW_CLR_CYAN");
    dw_listbox_append(combobox, "DW_CLR_WHITE");
    return combobox;
}

ULONG combobox_color(char *colortext)
{
    ULONG color = DW_CLR_DEFAULT;

    if(strcmp(colortext, "DW_CLR_BLACK") == 0)
        color = DW_CLR_BLACK;
    else if(strcmp(colortext, "DW_CLR_DARKRED") == 0)
        color = DW_CLR_DARKRED;
    else if(strcmp(colortext, "DW_CLR_DARKGREEN") == 0)
        color = DW_CLR_DARKGREEN;
    else if(strcmp(colortext, "DW_CLR_BROWN") == 0)
        color = DW_CLR_BROWN;
    else if(strcmp(colortext, "DW_CLR_DARKBLUE") == 0)
        color = DW_CLR_DARKBLUE;
    else if(strcmp(colortext, "DW_CLR_DARKPINK") == 0)
        color = DW_CLR_DARKPINK;
    else if(strcmp(colortext, "DW_CLR_DARKCYAN") == 0)
        color = DW_CLR_DARKCYAN;
    else if(strcmp(colortext, "DW_CLR_PALEGRAY") == 0)
        color = DW_CLR_PALEGRAY;
    else if(strcmp(colortext, "DW_CLR_DARKGRAY") == 0)
        color = DW_CLR_DARKGRAY;
    else if(strcmp(colortext, "DW_CLR_RED") == 0)
        color = DW_CLR_RED;
    else if(strcmp(colortext, "DW_CLR_GREEN") == 0)
        color = DW_CLR_GREEN;
    else if(strcmp(colortext, "DW_CLR_YELLOW") == 0)
        color = DW_CLR_YELLOW;
    else if(strcmp(colortext, "DW_CLR_BLUE") == 0)
        color = DW_CLR_BLUE;
    else if(strcmp(colortext, "DW_CLR_PINK") == 0)
        color = DW_CLR_PINK;
    else if(strcmp(colortext, "DW_CLR_CYAN") == 0)
        color = DW_CLR_CYAN;
    else if(strcmp(colortext, "DW_CLR_WHITE") == 0)
        color = DW_CLR_WHITE;

    return color;
}

int DWSIGNAL mle_color_cb(HWND hwnd, int pos, void *data)
{
    HWND hbox = (HWND)data;
    HWND mlefore = (HWND)dw_window_get_data(hbox, "mlefore");
    HWND mleback = (HWND)dw_window_get_data(hbox, "mleback");
    char colortext[101] = {0};
    ULONG fore = DW_CLR_DEFAULT, back = DW_CLR_DEFAULT;

    if(hwnd == mlefore)
    {
        dw_listbox_get_text(mlefore, pos, colortext, 100);
        fore = combobox_color(colortext);
    }
    else
    {
        char *text = dw_window_get_text(mlefore);

        if(text)
        {
            fore = combobox_color(text);
            dw_free(text);
        }
    }
    if(hwnd == mleback)
    {
        dw_listbox_get_text(mleback, pos, colortext, 100);
        back = combobox_color(colortext);
    }
    else
    {
        char *text = dw_window_get_text(mleback);

        if(text)
        {
            back = combobox_color(text);
            dw_free(text);
        }
    }

    dw_window_set_color(container_mle, fore, back);
    return 0;
}

void mle_font_set(HWND mle, int fontsize, char *fontname)
{
    char font[101] = {0};

    if(fontname)
        snprintf(font, 100, "%d.%s", fontsize, fontname);
    dw_window_set_font(mle, fontname ? font : NULL);
}

int DWSIGNAL mle_fontname_cb(HWND hwnd, int pos, void *data)
{
    HWND hbox = (HWND)data;
    HWND fontsize = (HWND)dw_window_get_data(hbox, "fontsize");
    HWND fontname = (HWND)dw_window_get_data(hbox, "fontname");
    char font[101] = {0};

    dw_listbox_get_text(fontname, pos, font, 100);
    mle_font_set(container_mle, (int)dw_spinbutton_get_pos(fontsize), strcmp(font, "Default") == 0 ? NULL : font);
    return 0;
}

int mle_fontsize_cb(HWND hwnd, int size, void *data)
{
    HWND hbox = (HWND)data;
    HWND fontname = (HWND)dw_window_get_data(hbox, "fontname");
    char *font = dw_window_get_text(fontname);

    if(font)
    {
        mle_font_set(container_mle, size, strcmp(font, "Default") == 0 ? NULL : font);
        dw_free(font);
    }
    else
        mle_font_set(container_mle, size, NULL);
    return 0;
}

void container_add(void)
{
    char *titles[4];
    char buffer[100];
    unsigned long flags[4] = {  DW_CFA_BITMAPORICON | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
                                DW_CFA_ULONG | DW_CFA_RIGHT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
                                DW_CFA_TIME | DW_CFA_CENTER | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
                                DW_CFA_DATE | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR };
    int z;
    CTIME time;
    CDATE date;
    unsigned long size, newpoint;
    HICN thisicon;
    HWND checkbox, mlefore, mleback, fontsize, fontname, hbox;

    /* create a box to pack into the notebook page */
    containerbox = dw_box_new(DW_HORZ, 2);
    dw_box_pack_start(notebookbox4, containerbox, 500, 200, TRUE, TRUE, 0);

    /* Add a word wrap checkbox */
    {
        HWND text;

        hbox = dw_box_new(DW_HORZ, 0);

        checkbox = dw_checkbox_new("Word wrap", 0);
        dw_box_pack_start(hbox, checkbox, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, TRUE, 1);
        text = dw_text_new("Foreground:", 0);
        dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
        dw_box_pack_start(hbox, text, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, TRUE, 1);
        mlefore = color_combobox();
        dw_box_pack_start(hbox, mlefore, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        text = dw_text_new("Background:", 0);
        dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
        dw_box_pack_start(hbox, text, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, TRUE, 1);
        mleback = color_combobox();
        dw_box_pack_start(hbox, mleback, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        dw_checkbox_set(checkbox, TRUE);
        text = dw_text_new("Font:", 0);
        dw_window_set_style(text, DW_DT_VCENTER, DW_DT_VCENTER);
        dw_box_pack_start(hbox, text, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, TRUE, 1);
        fontsize = dw_spinbutton_new("9", 0);
        dw_box_pack_start(hbox, fontsize, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 1);
        dw_spinbutton_set_limits(fontsize, 100, 5);
        dw_spinbutton_set_pos(fontsize, 9);
        fontname = dw_combobox_new("Default", 0);
        dw_listbox_append(fontname, "Default");
        dw_listbox_append(fontname, "Arial");
        dw_listbox_append(fontname, "Geneva");
        dw_listbox_append(fontname, "Verdana");
        dw_listbox_append(fontname, "Helvetica");
        dw_listbox_append(fontname, "DejaVu Sans");
        dw_listbox_append(fontname, "Times New Roman");
        dw_listbox_append(fontname, "Times New Roman Bold");
        dw_listbox_append(fontname, "Times New Roman Italic");
        dw_listbox_append(fontname, "Times New Roman Bold Italic");
        dw_box_pack_start(hbox, fontname, 150, DW_SIZE_AUTO, TRUE, FALSE, 1);
        dw_box_pack_start(notebookbox4, hbox, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 1);

        dw_window_set_data(hbox, "mlefore", DW_POINTER(mlefore));
        dw_window_set_data(hbox, "mleback", DW_POINTER(mleback));
        dw_window_set_data(hbox, "fontsize", DW_POINTER(fontsize));
        dw_window_set_data(hbox, "fontname", DW_POINTER(fontname));
    }

    /* now a container area under this box */
    container = dw_container_new(100, TRUE);
    dw_box_pack_start(notebookbox4, container, 500, 200, TRUE, FALSE, 1);

    /* and a status area to see whats going on */
    container_status = dw_status_text_new("", 0);
    dw_box_pack_start(notebookbox4, container_status, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

    titles[0] = "Type";
    titles[1] = "Size";
    titles[2] = "Time";
    titles[3] = "Date";

    dw_filesystem_set_column_title(container, "Test");
    dw_filesystem_setup(container, flags, titles, 4);
    dw_container_set_stripe(container, DW_CLR_DEFAULT, DW_CLR_DEFAULT);
    containerinfo = dw_container_alloc(container, 3);

    for(z=0;z<3;z++)
    {
        char names[100];

        sprintf(names, "We can now allocate from the stack: Item: %d", z);
        size = z*100;
        sprintf(buffer, "Filename %d", z+1);
        if (z == 0 ) thisicon = foldericon;
        else thisicon = fileicon;
        dw_debug("Initial: container: %x containerinfo: %x icon: %x\n", DW_POINTER_TO_INT(container),
                  DW_POINTER_TO_INT(containerinfo), DW_POINTER_TO_INT(thisicon));
        dw_filesystem_set_file(container, containerinfo, z, buffer, thisicon);
        dw_filesystem_set_item(container, containerinfo, 0, z, &thisicon);
        dw_filesystem_set_item(container, containerinfo, 1, z, &size);

        time.seconds = z+10;
        time.minutes = z+10;
        time.hours = z+10;
        dw_filesystem_set_item(container, containerinfo, 2, z, &time);

        date.day = z+10;
        date.month = z+10;
        date.year = z+2000;
        dw_filesystem_set_item(container, containerinfo, 3, z, &date);

        dw_container_set_row_title(containerinfo, z, names);
        dw_container_set_row_data(containerinfo, z, DW_INT_TO_POINTER(z));
    }

    dw_container_insert(container, containerinfo, 3);

    containerinfo = dw_container_alloc(container, 1);
    dw_filesystem_set_file(container, containerinfo, 0, "Yikes", foldericon);
    size = 324;
    dw_filesystem_set_item(container, containerinfo, 0, 0, &foldericon);
    dw_filesystem_set_item(container, containerinfo, 1, 0, &size);
    dw_filesystem_set_item(container, containerinfo, 2, 0, &time);
    dw_filesystem_set_item(container, containerinfo, 3, 0, &date);
    dw_container_set_row_title(containerinfo, 0, "Extra");

    dw_container_insert(container, containerinfo, 1);
    dw_container_optimize(container);

    container_mle = dw_mle_new(111);
    dw_box_pack_start(containerbox, container_mle, 500, 200, TRUE, TRUE, 0);

    mle_point = dw_mle_import(container_mle, "", -1);
    sprintf(buffer, "[%d]", mle_point);
    mle_point = dw_mle_import(container_mle, buffer, mle_point);
    sprintf(buffer, "[%d]abczxydefijkl", mle_point);
    mle_point = dw_mle_import(container_mle, buffer, mle_point);
    dw_mle_delete(container_mle, 9, 3);
    mle_point = dw_mle_import(container_mle, "gh", 12);
    dw_mle_get_size(container_mle, &newpoint, NULL);
    mle_point = (int)newpoint;
    sprintf(buffer, "[%d]\r\n\r\n", mle_point);
    mle_point = dw_mle_import(container_mle, buffer, mle_point);
    dw_mle_set_cursor(container_mle, mle_point);

    /* connect our event trappers... */
    dw_signal_connect(container, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(item_enter_cb), DW_POINTER(container_status));
    dw_signal_connect(container, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(item_context_cb), DW_POINTER(container_status));
    dw_signal_connect(container, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(container_select_cb), DW_POINTER(container_status));
    dw_signal_connect(container, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(column_click_cb), DW_POINTER(container_status));
    dw_signal_connect(checkbox, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(word_wrap_click_cb), DW_POINTER(container_mle));
    dw_signal_connect(mlefore, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(mle_color_cb), DW_POINTER(hbox));
    dw_signal_connect(mleback, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(mle_color_cb), DW_POINTER(hbox));
    dw_signal_connect(fontname, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(mle_fontname_cb), DW_POINTER(hbox));
    dw_signal_connect(fontsize, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(mle_fontsize_cb), DW_POINTER(hbox));
}

/* Beep every second */
int DWSIGNAL timer_callback(void *data)
{
    dw_beep(200, 200);

    /* Return TRUE so we get called again */
    return TRUE;
}


void buttons_add(void)
{
    HWND abutton1,abutton2,calbox;
    int i;
    char **text;

    /* create a box to pack into the notebook page */
    buttonsbox = dw_box_new(DW_VERT, 2);
    dw_box_pack_start(notebookbox5, buttonsbox, 25, 200, TRUE, TRUE, 0);
    dw_window_set_color(buttonsbox, DW_CLR_RED, DW_CLR_RED);

    calbox = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(notebookbox5, calbox, 0, 0, TRUE, FALSE, 1);
    cal = dw_calendar_new(100);
    dw_box_pack_start(calbox, cal, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);

    dw_calendar_set_date(cal, 2019, 4, 30);

    /*
     * Create our file toolbar boxes...
     */
    buttonboxperm = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(buttonsbox, buttonboxperm, 25, 0, FALSE, TRUE, 2);
    dw_window_set_color(buttonboxperm, DW_CLR_WHITE, DW_CLR_WHITE);
    abutton1 = dw_bitmapbutton_new_from_file("Top Button", 0, fileiconpath);
    dw_box_pack_start(buttonboxperm, abutton1, 100, 30, FALSE, FALSE, 0 );
    dw_signal_connect(abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(button_callback), NULL);
    dw_box_pack_start(buttonboxperm, 0, 25, 5, FALSE, FALSE, 0);
    abutton2 = dw_bitmapbutton_new_from_data( "Folder Icon", 0, folder_ico, sizeof(folder_ico));
    dw_box_pack_start(buttonsbox, abutton2, 25, 25, FALSE, FALSE, 0);
    dw_signal_connect(abutton2, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(bitmap_toggle_callback), NULL);

    create_button(0);
    /* make a combobox */
    combox = dw_box_new(DW_VERT, 2);
    dw_box_pack_start(notebookbox5, combox, 25, 200, TRUE, FALSE, 0);
    combobox1 = dw_combobox_new("fred", 0); /* no point in specifying an initial value */
    dw_listbox_append(combobox1, "fred");
    dw_box_pack_start(combox, combobox1, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);
    /*
     dw_window_set_text( combobox, "initial value");
     */
    dw_signal_connect(combobox1, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(combobox_select_event_callback), NULL);
#if 0
    /* add LOTS of items */
    dw_debug("before appending 100 items to combobox using dw_listbox_append()\n");
    for(i = 0; i < 100; i++)
    {
        sprintf( buf, "item %d", i);
        dw_listbox_append(combobox1, buf);
    }
    dw_debug("after appending 100 items to combobox\n");
#endif

    combobox2 = dw_combobox_new("joe", 0); /* no point in specifying an initial value */
    dw_box_pack_start(combox, combobox2, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);
    /*
     dw_window_set_text(combobox, "initial value");
     */
    dw_signal_connect(combobox2, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(combobox_select_event_callback), NULL);
    /* add LOTS of items */
    dw_debug("before appending 500 items to combobox using dw_listbox_list_append()\n");
    text = (char **)malloc(500*sizeof(char *));
    for(i = 0; i < 500; i++)
    {
        text[i] = (char *)malloc(50);
        sprintf( text[i], "item %d", i);
    }
    dw_listbox_list_append(combobox2, text, 500);
    dw_debug("after appending 500 items to combobox\n");
    for(i = 0; i < 500; i++)
    {
        free(text[i]);
    }
    free(text);
    /* now insert a couple of items */
    dw_listbox_insert(combobox2, "inserted item 2", 2);
    dw_listbox_insert(combobox2, "inserted item 5", 5);
    /* make a spinbutton */
    spinbutton = dw_spinbutton_new("", 0); /* no point in specifying text */
    dw_box_pack_start(combox, spinbutton, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);
    dw_spinbutton_set_limits(spinbutton, 100, 1);
    dw_spinbutton_set_pos(spinbutton, 30);
    dw_signal_connect(spinbutton, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(spinbutton_valuechanged_callback), NULL);
    /* make a slider */
    slider = dw_slider_new(FALSE, 11, 0); /* no point in specifying text */
    dw_box_pack_start(combox, slider, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);
    dw_signal_connect(slider, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(slider_valuechanged_callback), NULL);
    /* make a percent */
    percent = dw_percent_new(0);
    dw_box_pack_start(combox, percent, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);
}

void create_button(int redraw)
{
    HWND abutton1;
    filetoolbarbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(buttonboxperm, filetoolbarbox, 0, 0, TRUE, TRUE, 0);

    abutton1 = dw_bitmapbutton_new_from_file("Empty image. Should be under Top button", 0, "junk");
    dw_box_pack_start(filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
    dw_signal_connect(abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(change_color_red_callback), NULL);
    dw_box_pack_start(filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0 );

    abutton1 = dw_bitmapbutton_new_from_data("A borderless bitmapbitton", 0, folder_ico, 1718 );
    dw_box_pack_start(filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
    dw_signal_connect(abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(change_color_yellow_callback), NULL);
    dw_box_pack_start(filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0);
    dw_window_set_style(abutton1, DW_BS_NOBORDER, DW_BS_NOBORDER);

    abutton1 = dw_bitmapbutton_new_from_data("A button from data", 0, folder_ico, 1718);
    dw_box_pack_start(filetoolbarbox, abutton1, 25, 25, FALSE, FALSE, 0);
    dw_signal_connect(abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(percent_button_box_callback), NULL);
    dw_box_pack_start(filetoolbarbox, 0, 25, 5, FALSE, FALSE, 0 );
    if(redraw)
    {
        dw_window_redraw(filetoolbarbox);
        dw_window_redraw(mainwindow);
    }
}

#ifdef DW_INCLUDE_DEPRECATED
void mdi_add(void)
{
    HWND mdibox, mdi, mdi1w, mdi1box, ef, mdi2w, mdi2box, bb;

    /* create a box to pack into the notebook page */
    mdibox = dw_box_new(DW_HORZ, 0);

    dw_box_pack_start(notebookbox6, mdibox, 500, 200, TRUE, TRUE, 1);

    /* now a mdi under this box */
    mdi = dw_mdi_new(333);
    dw_box_pack_start(mdibox, mdi, 500, 200, TRUE, TRUE, 2);

    mdi1w = dw_window_new(mdi, "MDI1", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
    mdi1box = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(mdi1w, mdi1box, 0, 0, TRUE, TRUE, 0);
    ef = dw_entryfield_new("", 0);
    dw_box_pack_start(mdi1box, ef, 100, 20, FALSE, FALSE, 4);
    dw_window_set_size(mdi1w, 200, 100);
    dw_window_show(mdi1w);

    mdi2w = dw_window_new(mdi, "MDI2", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
    mdi2box = dw_box_new(DW_HORZ, 0);
    dw_box_pack_start(mdi2w, mdi2box, 0, 0, TRUE, TRUE, 0);
    ef = dw_entryfield_new("", 0);
    dw_box_pack_start(mdi2box, ef, 150, 30, FALSE, FALSE, 4);
    bb = dw_button_new("Browse", 0);
    dw_box_pack_start(mdi2box, bb, 60, 30, FALSE, FALSE, 4);
    dw_window_set_size(mdi2w, 200, 200);
    dw_window_show(mdi2w);
}
#endif

void menu_add(void)
{
    HMENUI menuitem,menu;

    mainmenubar = dw_menubar_new(mainwindow);
    /* add menus to the menubar */
    menu = dw_menu_new(0);
    menuitem = dw_menu_append_item(menu, "~Quit", 1019, 0, TRUE, FALSE, 0);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(exit_callback), DW_POINTER(mainwindow));
    /*
     * Add the "File" menu to the menubar...
     */
    dw_menu_append_item(mainmenubar, "~File", 1010, 0, TRUE, FALSE, menu);

    changeable_menu = dw_menu_new(0);
    checkable_menuitem = dw_menu_append_item(changeable_menu, "~Checkable Menu Item", CHECKABLE_MENUITEMID, 0, TRUE, TRUE, 0);
    dw_signal_connect( checkable_menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_callback), DW_POINTER("checkable"));
    noncheckable_menuitem = dw_menu_append_item(changeable_menu, "~Non-checkable Menu Item", NONCHECKABLE_MENUITEMID, 0, TRUE, FALSE, 0);
    dw_signal_connect(noncheckable_menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menu_callback), DW_POINTER("non-checkable"));
    dw_menu_append_item(changeable_menu, "~Disabled menu Item", 2003, DW_MIS_DISABLED|DW_MIS_CHECKED, TRUE, TRUE, 0);
    /* seperator */
    dw_menu_append_item(changeable_menu, DW_MENU_SEPARATOR, 3999, 0, TRUE, FALSE, 0);
    menuitem = dw_menu_append_item(changeable_menu, "~Menu Items Disabled", 2009, 0, TRUE, TRUE, 0);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(menutoggle_callback), NULL);
    /*
     * Add the "Menu" menu to the menubar...
     */
    dw_menu_append_item(mainmenubar, "~Menu", 1020, 0, TRUE, FALSE, changeable_menu);

    menu = dw_menu_new(0);
    menuitem = dw_menu_append_item(menu, "~About", 1091, 0, TRUE, FALSE, 0);
    dw_signal_connect(menuitem, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(helpabout_callback), DW_POINTER(mainwindow));
    /*
     * Add the "Help" menu to the menubar...
     */
    dw_menu_append_item(mainmenubar, "~Help", 1090, 0, TRUE, FALSE, menu);
}

int DWSIGNAL scrollbox_button_callback(HWND window, void *data)
{
    int pos, range;

    pos = dw_scrollbox_get_pos(scrollbox, DW_VERT);
    range = dw_scrollbox_get_range(scrollbox, DW_VERT);
    dw_debug("Pos %d Range %d\n", pos, range);
    return 0;
}

void scrollbox_add(void)
{
    HWND tmpbox,abutton1;
    char buf[100];
    int i;

    /* create a box to pack into the notebook page */
    scrollbox = dw_scrollbox_new(DW_VERT, 0);
    dw_box_pack_start(notebookbox8, scrollbox, 0, 0, TRUE, TRUE, 1);

    abutton1 = dw_button_new("Show Adjustments", 0);
    dw_box_pack_start(scrollbox, abutton1, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
    dw_signal_connect(abutton1, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(scrollbox_button_callback), NULL);

    for(i = 0; i < MAX_WIDGETS; i++)
    {
        tmpbox = dw_box_new(DW_HORZ, 0);
        dw_box_pack_start(scrollbox, tmpbox, 0, 0, TRUE, FALSE, 2);
        sprintf(buf, "Label %d", i);
        labelarray[i] = dw_text_new(buf , 0);
        dw_box_pack_start(tmpbox, labelarray[i], 0, DW_SIZE_AUTO, TRUE, FALSE, 0);
        sprintf(buf, "Entry %d", i);
        entryarray[i] = dw_entryfield_new(buf , i);
        dw_box_pack_start(tmpbox, entryarray[i], 0, DW_SIZE_AUTO, TRUE, FALSE, 0);
    }
}

/* Section for thread/event test */
HWND threadmle, startbutton;
HMTX mutex;
HEV workevent, controlevent;
int finished = FALSE;
int ready = 0;
#define BUF_SIZE 1024
void DWSIGNAL run_thread(void *data);
void DWSIGNAL control_thread(void *data);

void update_mle(char *text, int lock)
{
    static unsigned int pos = 0;

    /* Protect pos from being changed by different threads */
    if(lock)
        dw_mutex_lock(mutex);
    pos = dw_mle_import(threadmle, text, pos);
    dw_mle_set_cursor(threadmle, pos);
    if(lock)
        dw_mutex_unlock(mutex);
}

int DWSIGNAL start_threads_button_callback(HWND window, void *data)
{
    dw_window_disable(startbutton);
    dw_mutex_lock(mutex);
    controlevent = dw_event_new();
    dw_event_reset(workevent);
    finished = FALSE;
    ready = 0;
    update_mle("Starting thread 1\r\n", FALSE);
    dw_thread_new(DW_SIGNAL_FUNC(run_thread), DW_INT_TO_POINTER(1), 10000);
    update_mle("Starting thread 2\r\n", FALSE);
    dw_thread_new(DW_SIGNAL_FUNC(run_thread), DW_INT_TO_POINTER(2), 10000);
    update_mle("Starting thread 3\r\n", FALSE);
    dw_thread_new(DW_SIGNAL_FUNC(run_thread), DW_INT_TO_POINTER(3), 10000);
    update_mle("Starting thread 4\r\n", FALSE);
    dw_thread_new(DW_SIGNAL_FUNC(run_thread), DW_INT_TO_POINTER(4), 10000);
    update_mle("Starting control thread\r\n", FALSE);
    dw_thread_new(DW_SIGNAL_FUNC(control_thread), DW_INT_TO_POINTER(0), 10000);
    dw_mutex_unlock(mutex);
    return 0;
}

void thread_add(void)
{
    HWND tmpbox;

    /* create a box to pack into the notebook page */
    tmpbox = dw_box_new(DW_VERT, 0);
    dw_box_pack_start(notebookbox9, tmpbox, 0, 0, TRUE, TRUE, 1);

    startbutton = dw_button_new( "Start Threads", 0 );
    dw_box_pack_start(tmpbox, startbutton, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
    dw_signal_connect(startbutton, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(start_threads_button_callback), NULL);

    /* Create the base threading components */
    threadmle = dw_mle_new(0);
    dw_box_pack_start(tmpbox, threadmle, 1, 1, TRUE, TRUE, 0);
    mutex = dw_mutex_new();
    workevent = dw_event_new();
}

void DWSIGNAL run_thread(void *data)
{
    int threadnum = DW_POINTER_TO_INT(data);
    char buf[BUF_SIZE];

    sprintf(buf, "Thread %d started.\r\n", threadnum);
    update_mle(buf, TRUE);

    /* Increment the ready count while protected by mutex */
    dw_mutex_lock(mutex);
    ready++;
    /* If all 4 threads have incrememted the ready count...
     * Post the control event semaphore so things will get started.
     */
    if(ready == 4)
        dw_event_post(controlevent);
    dw_mutex_unlock(mutex);

    while(!finished)
    {
        int result = dw_event_wait(workevent, 2000);

        if(result == DW_ERROR_TIMEOUT)
        {
            sprintf(buf, "Thread %d timeout waiting for event.\r\n", threadnum);
            update_mle(buf, TRUE);
        }
        else if(result == DW_ERROR_NONE)
        {
            sprintf(buf, "Thread %d doing some work.\r\n", threadnum);
            update_mle(buf, TRUE);
            /* Pretend to do some work */
            dw_main_sleep(1000 * threadnum);

            /* Increment the ready count while protected by mutex */
            dw_mutex_lock(mutex);
            ready++;
            sprintf(buf, "Thread %d work done. ready=%d", threadnum, ready);
            /* If all 4 threads have incrememted the ready count...
             * Post the control event semaphore so things will get started.
             */
            if(ready == 4)
            {
                dw_event_post(controlevent);
                strcat(buf, " Control posted.");
            }
            dw_mutex_unlock(mutex);
            strcat(buf, "\r\n");
            update_mle(buf, TRUE);
        }
        else
        {
            sprintf(buf, "Thread %d error %d.\r\n", threadnum, result);
            update_mle(buf, TRUE);
            dw_main_sleep(10000);
        }
    }
    sprintf(buf, "Thread %d finished.\r\n", threadnum);
    update_mle(buf, TRUE);
}

void DWSIGNAL control_thread(void *data)
{
    int inprogress = 5;
    char buf[BUF_SIZE];

    while(inprogress)
    {
        int result = dw_event_wait(controlevent, 2000);

        if(result == DW_ERROR_TIMEOUT)
        {
            update_mle("Control thread timeout waiting for event.\r\n", TRUE);
        }
        else if(result == DW_ERROR_NONE)
        {
            /* Reset the control event */
            dw_event_reset(controlevent);
            ready = 0;
            sprintf(buf,"Control thread starting worker threads. Inprogress=%d\r\n", inprogress);
            update_mle(buf, TRUE);
            /* Start the work threads */
            dw_event_post(workevent);
            dw_main_sleep(100);
            /* Reset the work event */
            dw_event_reset(workevent);
            inprogress--;
        }
        else
        {
            sprintf(buf, "Control thread error %d.\r\n", result);
            update_mle(buf, TRUE);
            dw_main_sleep(10000);
        }
    }
    /* Tell the other threads we are done */
    finished = TRUE;
    dw_event_post(workevent);
    /* Close the control event */
    dw_event_close(&controlevent);
    update_mle("Control thread finished.\r\n", TRUE);
    dw_window_enable(startbutton);
}

/* Handle web back navigation */
int DWSIGNAL web_back_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;

    dw_html_action(html, DW_HTML_GOBACK);
    return FALSE;
}

/* Handle web forward navigation */
int DWSIGNAL web_forward_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;

    dw_html_action(html, DW_HTML_GOFORWARD);
    return FALSE;
}

/* Handle web reload */
int DWSIGNAL web_reload_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;

    dw_html_action(html, DW_HTML_RELOAD);
    return FALSE;
}

/* Handle web run */
int DWSIGNAL web_run_clicked(HWND button, void *data)
{
    HWND html = (HWND)data;
    HWND javascript = (HWND)dw_window_get_data(button, "javascript");
    char *script = dw_window_get_text(javascript);

    dw_html_javascript_run(html, script, NULL);
    dw_free(script);
    return FALSE;
}

/* Handle web javascript result */
int DWSIGNAL web_html_result(HWND html, int status, char *result, void *script_data, void *user_data)
{
    dw_messagebox("Javascript Result", DW_MB_OK | (status ? DW_MB_ERROR : DW_MB_INFORMATION),
                  result ? result : "Javascript result is not a string value");
    return TRUE;
}

/* Handle web html changed */
int DWSIGNAL web_html_changed(HWND html, int status, char *url, void *data)
{
    HWND hwndstatus = (HWND)data;
    char *statusnames[] = { "none", "started", "redirect", "loading", "complete", NULL };

    if(hwndstatus && url && status < 5)
    {
        int length = (int)strlen(url) + (int)strlen(statusnames[status]) + 10;
        char *text = calloc(1, length+1);

        snprintf(text, length, "Status %s: %s", statusnames[status], url);
        dw_window_set_text(hwndstatus, text);
        free(text);
    }
    return FALSE;
}

/* Handle web javascript message */
int DWSIGNAL web_html_message(HWND html, char *name, char *message, void *data)
{
    dw_messagebox("Javascript Message", DW_MB_OK | DW_MB_INFORMATION, "Name: %s Message: %s",
                  name, message);
    return TRUE;
}

void html_add(void)
{
    rawhtml = dw_html_new(1001);
    if(rawhtml)
    {
        HWND htmlstatus;
        HWND hbox = dw_box_new(DW_HORZ, 0);
        HWND item;
        HWND javascript = dw_combobox_new("", 0);

        dw_listbox_append(javascript, "window.scrollTo(0,500);");
        dw_listbox_append(javascript, "window.document.title;");
        dw_listbox_append(javascript, "window.navigator.userAgent;");

        dw_box_pack_start(notebookbox7, rawhtml, 0, 100, TRUE, FALSE, 0);
        dw_html_javascript_add(rawhtml, "test");
        dw_signal_connect(rawhtml, DW_SIGNAL_HTML_MESSAGE, DW_SIGNAL_FUNC(web_html_message), DW_POINTER(javascript));
        dw_html_raw(rawhtml, dw_feature_get(DW_FEATURE_HTML_MESSAGE) == DW_FEATURE_ENABLED ?
                    "<html><body><center><h1><a href=\"javascript:test('This is the message');\">dwtest</a></h1></center></body></html>" :
                    "<html><body><center><h1>dwtest</h1></center></body></html>");
        html = dw_html_new(1002);

        dw_box_pack_start(notebookbox7, hbox, 0, 0, TRUE, FALSE, 0);

        /* Add navigation buttons */
        item = dw_button_new("Back", 0);
        dw_box_pack_start(hbox, item, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_back_clicked), DW_POINTER(html));

        item = dw_button_new("Forward", 0);
        dw_box_pack_start(hbox, item, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_forward_clicked), DW_POINTER(html));

        /* Put in some extra space */
        dw_box_pack_start(hbox, 0, 5, 1, FALSE, FALSE, 0);

        item = dw_button_new("Reload", 0);
        dw_box_pack_start(hbox, item, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_reload_clicked), DW_POINTER(html));

        /* Put in some extra space */
        dw_box_pack_start(hbox, 0, 5, 1, FALSE, FALSE, 0);
        dw_box_pack_start(hbox, javascript, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, FALSE, 0);

        item = dw_button_new("Run", 0);
        dw_window_set_data(item, "javascript", DW_POINTER(javascript));
        dw_box_pack_start(hbox, item, DW_SIZE_AUTO, DW_SIZE_AUTO, FALSE, FALSE, 0);
        dw_signal_connect(item, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(web_run_clicked), DW_POINTER(html));
        dw_window_click_default(javascript, item);

        dw_box_pack_start(notebookbox7, html, 0, 100, TRUE, TRUE, 0);
        dw_html_url(html, "https://dbsoft.org/dw_help.php");
        htmlstatus = dw_status_text_new("HTML status loading...", 0);
        dw_box_pack_start(notebookbox7, htmlstatus, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);
        dw_signal_connect(html, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(web_html_changed), DW_POINTER(htmlstatus));
        dw_signal_connect(html, DW_SIGNAL_HTML_RESULT, DW_SIGNAL_FUNC(web_html_result), DW_POINTER(javascript));
    }
    else
    {
        html = dw_text_new("HTML widget not available.", 0);
        dw_box_pack_start(notebookbox7, html, 0, 100, TRUE, TRUE, 0);
    }
}

/* Pretty list of features corresponding to the DWFEATURE enum in dw.h */
char *DWFeatureList[] = {
    "Supports the HTML Widget",
    "Supports the DW_SIGNAL_HTML_RESULT callback",
    "Supports custom window border sizes",
    "Supports window frame transparency",
    "Supports Dark Mode user interface",
    "Supports auto completion in Multi-line Edit boxes",
    "Supports word wrapping in Multi-line Edit boxes",
    "Supports striped line display in container widgets",
    "Supports Multiple Document Interface window frame",
    "Supports status text area on notebook/tabbed controls",
    "Supports sending system notifications",
    "Supports UTF8 encoded Unicode text",
    "Supports Rich Edit based MLE control (Windows)",
    "Supports icons in the taskbar or similar system widget",
    "Supports the Tree Widget",
    "Supports arbitrary window placement",
    "Supports alternate container view modes",
    "Supports the DW_SIGNAL_HTML_MESSAGE callback",
    "Supports render safe drawing mode, limited to expose",
    NULL };

/*
 * Let's demonstrate the functionality of this library. :)
 */
int dwmain(int argc, char *argv[])
{
    ULONG notebookpage1;
    ULONG notebookpage2;
    ULONG notebookpage3;
    ULONG notebookpage4;
    ULONG notebookpage5;
#ifdef DW_INCLUDE_DEPRECATED
    ULONG notebookpage6;
#endif
    ULONG notebookpage7;
    ULONG notebookpage8;
    ULONG notebookpage9;
    DWFEATURE feat;

    /* Setup the Application ID for sending notifications */
    dw_app_id_set("org.dbsoft.dwindows.dwtest", "Dynamic Windows Test");

    /* Enable full dark mode on platforms that support it */
    if(getenv("DW_DARK_MODE"))
        dw_feature_set(DW_FEATURE_DARK_MODE, DW_DARK_MODE_FULL);

#ifdef DW_MOBILE
    /* Enable multi-line container display on Mobile platforms */
    dw_feature_set(DW_FEATURE_CONTAINER_MODE, DW_CONTAINER_MODE_MULTI);
#endif

    /* Initialize the Dynamic Windows engine */
    dw_init(TRUE, argc, argv);

    /* Test all the features and display the results */
    for(feat=0;feat<DW_FEATURE_MAX && DWFeatureList[feat];feat++)
    {
        int result = dw_feature_get(feat);
        char *status = "Unsupported";

        if(result == 0)
            status = "Disabled";
        else if(result > 0)
            status = "Enabled";

        dw_debug("%s: %s (%d)\n", DWFeatureList[feat], status, result);
    }

    /* Create our window */
    mainwindow = dw_window_new(HWND_DESKTOP, "dwindows test UTF8 中国語 (繁体) cañón", flStyle | DW_FCF_SIZEBORDER | DW_FCF_MINMAX);
    dw_window_set_icon(mainwindow, fileicon);

    menu_add();

    notebookbox = dw_box_new(DW_VERT, 5);
    dw_box_pack_start(mainwindow, notebookbox, 0, 0, TRUE, TRUE, 0);

    /* First try the current directory */
    foldericon = dw_icon_load_from_file(foldericonpath);
    fileicon = dw_icon_load_from_file(fileiconpath);

#ifdef PLATFORMFOLDER
    /* In case we are running from the build directory...
     * also check the appropriate platform subfolder
     */
    if(!foldericon)
    {
        strncpy(foldericonpath, PLATFORMFOLDER "folder", 1024);
        foldericon = dw_icon_load_from_file(foldericonpath);
    }
    if(!fileicon)
    {
        strncpy(fileiconpath, PLATFORMFOLDER "file", 1024);
        fileicon = dw_icon_load_from_file(fileiconpath);
    }
#endif

    /* Finally try from the platform application directory */
    if(!foldericon && !fileicon)
    {
        char *appdir = dw_app_dir();
        char pathbuff[1025] = {0};
        int pos = (int)strlen(appdir);

        strncpy(pathbuff, appdir, 1024);
        pathbuff[pos] = DW_DIR_SEPARATOR;
        pos++;
        strncpy(&pathbuff[pos], "folder", 1024-pos);
        foldericon = dw_icon_load_from_file(pathbuff);
        if(foldericon)
            strncpy(foldericonpath, pathbuff, 1025);
        strncpy(&pathbuff[pos], "file", 1024-pos);
        fileicon = dw_icon_load_from_file(pathbuff);
        if(fileicon)
            strncpy(fileiconpath, pathbuff, 1025);
    }

    notebook = dw_notebook_new(1, TRUE);
    dw_box_pack_start(notebookbox, notebook, 100, 100, TRUE, TRUE, 0);
    dw_signal_connect(notebook, DW_SIGNAL_SWITCH_PAGE, DW_SIGNAL_FUNC(switch_page_cb), NULL);

    notebookbox1 = dw_box_new(DW_VERT, 5);
    notebookpage1 = dw_notebook_page_new( notebook, 0, TRUE);
    dw_notebook_pack( notebook, notebookpage1, notebookbox1);
    dw_notebook_page_set_text( notebook, notebookpage1, "buttons and entry");
    archive_add();

    notebookbox2 = dw_box_new(DW_VERT, 5);
    notebookpage2 = dw_notebook_page_new( notebook, 1, FALSE);
    dw_notebook_pack( notebook, notebookpage2, notebookbox2);
    dw_notebook_page_set_text(notebook, notebookpage2, "render");
    text_add();

    notebookbox3 = dw_box_new(DW_VERT, 5);
    notebookpage3 = dw_notebook_page_new( notebook, 1, FALSE);
    dw_notebook_pack( notebook, notebookpage3, notebookbox3);
    dw_notebook_page_set_text( notebook, notebookpage3, "tree");
    tree_add();

    notebookbox4 = dw_box_new(DW_VERT, 5);
    notebookpage4 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack( notebook, notebookpage4, notebookbox4 );
    dw_notebook_page_set_text( notebook, notebookpage4, "container");
    container_add();

    notebookbox5 = dw_box_new(DW_VERT, 5);
    notebookpage5 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack(notebook, notebookpage5, notebookbox5);
    dw_notebook_page_set_text(notebook, notebookpage5, "buttons");
    buttons_add();

#ifdef DW_INCLUDE_DEPRECATED
    notebookbox6 = dw_box_new(DW_VERT, 5);
    notebookpage6 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack(notebook, notebookpage6, notebookbox6);
    dw_notebook_page_set_text(notebook, notebookpage6, "mdi");
    mdi_add();
#endif

    notebookbox7 = dw_box_new(DW_VERT, 6);
    notebookpage7 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack(notebook, notebookpage7, notebookbox7);
    dw_notebook_page_set_text(notebook, notebookpage7, "html");
    html_add();

    notebookbox8 = dw_box_new(DW_VERT, 7);
    notebookpage8 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack(notebook, notebookpage8, notebookbox8);
    dw_notebook_page_set_text(notebook, notebookpage8, "scrollbox");
    scrollbox_add();

    notebookbox9 = dw_box_new(DW_VERT, 8);
    notebookpage9 = dw_notebook_page_new(notebook, 1, FALSE);
    dw_notebook_pack(notebook, notebookpage9, notebookbox9);
    dw_notebook_page_set_text(notebook, notebookpage9, "thread/event");
    thread_add();

    /* Set the default field */
    dw_window_default(mainwindow, copypastefield);

    dw_signal_connect(mainwindow, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(exit_callback), DW_POINTER(mainwindow));
    /*
     * The following is a special case handler for the Mac and other platforms which contain
     * an application object which can be closed.  It functions identically to a window delete/close
     * request except it applies to the entire application not an individual window. If it is not
     * handled or you allow the default handler to take place the entire application will close.
     * On platforms which do not have an application object this line will be ignored.
     */
    dw_signal_connect(DW_DESKTOP, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(exit_callback), DW_POINTER(mainwindow));
    timerid = dw_timer_connect(2000, DW_SIGNAL_FUNC(timer_callback), 0);
    dw_window_set_size(mainwindow, 640, 550);
    dw_window_show(mainwindow);

    /* Now that the window is created and shown...
     * run the main loop until we get dw_main_quit()
     */
    dw_main();

    /* Now that the loop is done we can cleanup */
    dw_taskbar_delete(textbox1, fileicon);
    dw_window_destroy(mainwindow);

    dw_debug("dwtest exiting...\n");
    /* Call dw_exit() to shutdown the Dynamic Windows engine */
    dw_exit(0);
    return 0;
}
