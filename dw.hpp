/* Dynamic Windows C++ Language Bindings 
 * Copyright 2022 Brian Smith
 * Requires a C++11 compatible compiler.
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
protected:
	void *handle;
	void SetHandle(void *newhandle) { handle = newhandle; }
public:
	void *GetHandle() { return handle; }
};

// Widget class allows packing and style
class Widget : public Handle
{
protected:
	void SetHWND(HWND newhwnd) { hwnd = newhwnd; 
		SetHandle(reinterpret_cast<void *>(newhwnd));
		// Save the C++ class pointer in the window data for later
		dw_window_set_data(hwnd, "_dw_classptr", this);
	}
	HWND hwnd; 
public:
	HWND GetHWND() { return hwnd; }
	int Unpack() { return dw_box_unpack(hwnd); }
	void SetStyle(unsigned long style, unsigned long mask) { dw_window_set_style(hwnd, style, mask); }
	void SetTooltip(char *bubbletext) { dw_window_set_tooltip(hwnd, bubbletext); }
};

// Box class is a packable object
class Box : public Widget
{
public:
	void PackStart(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
		dw_box_pack_start(hwnd, item ? item->GetHWND() : nullptr, width, height, hsize, vsize, pad); }
	void PackEnd(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
		dw_box_pack_end(hwnd, item ? item->GetHWND() : nullptr, width, height, hsize, vsize, pad); }
	void PackAtIndex(Widget *item, int index, int width, int height, int hsize, int vsize, int pad) { 
		dw_box_pack_at_index(hwnd, item ? item->GetHWND() : nullptr, index, width, height, hsize, vsize, pad); }
	Widget *UnpackAtIndex(int index) { HWND widget = dw_box_unpack_at_index(hwnd, index);
		void *classptr = widget ? dw_window_get_data(widget, "_dw_classptr") : nullptr;
		return reinterpret_cast<Widget *>(classptr);
	}
};

// TODO: Find a way to implement this cross platform...
// That way we can skip adding unused signal handlers
#define IsOverridden(a, b) true

// Top-level window class is packable
class Window : public Box
{
private:
	void Setup() {	
		if(IsOverridden(Window::OnConfigure, this))
			dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
		if(IsOverridden(Window::OnConfigure, this))
			dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
	}
	static int _OnDelete(HWND window, void *data) { return reinterpret_cast<Window *>(data)->OnDelete(); }
	static int _OnConfigure(HWND window, int width, int height, void *data) { return reinterpret_cast<Window *>(data)->OnConfigure(width, height); }
public:
	// Constructors
	Window(HWND owner, const char *title, unsigned long style) { SetHWND(dw_window_new(owner, title, style)); Setup(); }
	Window(const char *title, unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, title, style)); Setup(); }
	Window(unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, "", style)); Setup(); }
	Window(const char *title) { SetHWND(dw_window_new(HWND_DESKTOP, title,  DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); Setup(); }
	Window() { SetHWND(dw_window_new(HWND_DESKTOP, "", DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); Setup(); }

	// User functions
	void SetText(const char *text) { dw_window_set_text(hwnd, text); }
	void SetSize(unsigned long width, unsigned long height) { dw_window_set_size(hwnd, width, height); }
	int Show() { return dw_window_show(hwnd); }
	int Hide() { return dw_window_hide(hwnd); }
	void SetGravity(int horz, int vert) { dw_window_set_gravity(hwnd, horz, vert); }
	int Minimize() { return dw_window_minimize(hwnd); }
	int Raise() { return dw_window_raise(hwnd); }
	int Lower() { return dw_window_lower(hwnd); }
protected:
	// Our signal handler functions to be overriden...
	// If they are not overridden and an event is generated, remove the unused handler
	virtual int OnDelete() { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_DELETE); return FALSE; }
	virtual int OnConfigure(int width, int height) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CONFIGURE); return FALSE; };
};

// Class for handling static text widget
class Text : public Widget
{
public:
	Text(char *text, unsigned long id) { SetHWND(dw_text_new(text, id)); }
	Text(char *text) { SetHWND(dw_text_new(text, 0)); }
	Text(unsigned long id) { SetHWND(dw_text_new("", id)); }
	Text() { SetHWND(dw_text_new("", 0)); }
	void SetText(const char *text) { dw_window_set_text(hwnd, text); }
	int SetFont(const char *font) { return dw_window_set_font(hwnd, font); }
	char *GetFont() { return dw_window_get_font(hwnd); }
};


class App
{
protected:
	App() { }
	static App *_app;
public:
	// Singletons should not be cloneable.
	App(App &other) = delete;
	// Singletons should not be assignable.
	void operator=(const App &) = delete;
	// Initialization functions for creating App
	static App *Init() { if(!_app) { _app = new App; dw_init(TRUE, 0, NULL); } return _app; }
	static App *Init(const char *appid) { if(!_app) { _app = new App(); dw_app_id_set(appid, NULL); dw_init(TRUE, 0, NULL); } return _app; }
	static App *Init(const char *appid, const char *appname) { if(!_app) { _app = new App(); dw_app_id_set(appid, appname); dw_init(TRUE, 0, NULL); } return _app; }
	static App *Init(int argc, char *argv[]) { if(!_app) { _app = new App(); dw_init(TRUE, argc, argv); } return _app; }
	static App *Init(int argc, char *argv[], const char *appid) { if(!_app) { _app = new App(); dw_app_id_set(appid, NULL); dw_init(TRUE, argc, argv); } return _app; }
	static App *Init(int argc, char *argv[], const char *appid, const char *appname) { if(!_app) { _app = new App(); dw_app_id_set(appid, appname); dw_init(TRUE, argc, argv); } return _app; }

	void Main() { dw_main(); }
	void MainIteration() { dw_main_iteration(); }
	void MainQuit() { dw_main_quit(); }
	void Exit(int exitcode) { dw_exit(exitcode); }
};

// Static singleton reference declared outside of the class
App* App::_app = nullptr;

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