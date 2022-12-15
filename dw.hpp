/* Dynamic Windows C++ Language Bindings 
 * Copyright 2022 Brian Smith
 */

#ifndef _HPP_DW
#define _HPP_DW
#include <dw.h>

namespace DW 
{

// Base handle class which allows opaque access to 
// The base system handles
class Handle 
{
private:
	void *handle;
public:
	void SetHandle(void *newhandle) { handle = newhandle; }
	void *GetHandle() { return handle; }
};

// Widget class allows packing and style
class Widget : public Handle
{
public:
	HWND hwnd; 
	HWND GetHWND() { return reinterpret_cast<HWND>(GetHandle()); }
	void SetHWND(HWND newhwnd) { hwnd = newhwnd; SetHandle(reinterpret_cast<void *>(newhwnd)); }
};

// Box class is a packable object
class Box : public Widget
{
};
	
// Top-level window class is packable
class Window : public Box
{
public:
	Window(HWND owner, const char *title, unsigned long style) { SetHWND(dw_window_new(owner, title, style)); }
	Window(const char *title, unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, title, style)); }
	Window(unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, "", style)); }
	Window(const char *title) { SetHWND(dw_window_new(HWND_DESKTOP, title,  DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); }
	Window() { SetHWND(dw_window_new(HWND_DESKTOP, "", DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); }

	void SetText(const char *text) { dw_window_set_text(hwnd, text); }
	void SetSize(unsigned long width, unsigned long height) { dw_window_set_size(hwnd, width, height); }
	void Show() { dw_window_show(hwnd); }
};

class App
{
public:
	App() { dw_init(TRUE, 0, NULL); }
	App(const char *appid) { dw_app_id_set(appid, NULL); dw_init(TRUE, 0, NULL); }
	App(const char *appid, const char *appname) { dw_app_id_set(appid, appname); dw_init(TRUE, 0, NULL); }
	App(int argc, char *argv[]) { dw_init(TRUE, argc, argv); }
	App(int argc, char *argv[], const char *appid) { dw_app_id_set(appid, NULL); dw_init(TRUE, argc, argv); }
	App(int argc, char *argv[], const char *appid, const char *appname) { dw_app_id_set(appid, appname); dw_init(TRUE, argc, argv); }

	void Main() { dw_main(); }
	void MainIteration() { dw_main_iteration(); }
	void MainQuit() { dw_main_quit(); }
	void Exit(int exitcode) { dw_exit(exitcode); }
};

#if 0
// Class that allows drawing, either to screen or picture (pixmap)
class Drawable
{
	void DrawPoint(int x, int y);
	void DrawLine(int x1, int y1, int x2, int y2);
	void DrawPolygon(int flags, int x[], int y[]);
	void DrawRect(int fill, int x, int y, int width, int height);
	void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2);
	void DrawText(int x, int y, std::string text);
	int BitBltStretch(int xdest, int ydest, int width, int height, HWND src, int xsrc, int ysrc, int srcwidth, int srcheight);
	int BitBltStretch(int xdest, int ydest, int width, int height, HPIXMAP srcp, int xsrc, int ysrc, int srcwidth, int srcheight);
	void BitBlt(int xdest, int ydest, int width, int height, HWND src, int xsrc, int ysrc);
	void BitBlt(int xdest, int ydest, int width, int height, HPIXMAP srcp, int xsrc, int ysrc);
};

class Render : public Drawable, public Widget
{
};

class Pixmap : public Drawable, public Handle
{
};
#endif

} /* namespace DW */
#endif