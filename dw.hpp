/* Dynamic Windows C++ Language Bindings 
 * Copyright 2022 Brian Smith
 * Recommends a C++11 compatible compiler.
 */

#ifndef _HPP_DW
#define _HPP_DW
#include <dw.h>

// Attempt to support compilers without nullptr type literal
#if __cplusplus >= 201103L
#define DW_NULL nullptr
#else
#define DW_NULL NULL
#endif

// Attempt to allow compilation on GCC older than 4.7
#if defined(__GNUC__) && (__GNuC__ < 5 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#define override
#endif	

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
    void SetHWND(HWND newhwnd) { 
        hwnd = newhwnd; 
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
    int SetColor(unsigned long fore, unsigned long back) { return dw_window_set_color(hwnd, fore, back); }
    void SetData(const char *dataname, void *data) { dw_window_set_data(hwnd, dataname, data); }
    void *GetData(const char *dataname) { return dw_window_get_data(hwnd, dataname); }
    void SetPointer(int cursortype) { dw_window_set_pointer(hwnd, cursortype); }
    Widget *FromID(int id) { 
        HWND child = dw_window_from_id(hwnd, id);
        if(child) {
            return reinterpret_cast<Widget *>(dw_window_get_data(child, "_dw_classptr"));
        }
        return DW_NULL;
    }
    void GetPreferredSize(int *width, int *height) { dw_window_get_preferred_size(hwnd, width, height); }
};

// Box class is a packable object
class Boxes : virtual public Widget
{
public:
    // User functions
    void PackStart(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_start(hwnd, item ? item->GetHWND() : DW_NOHWND, width, height, hsize, vsize, pad); }
    void PackEnd(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_end(hwnd, item ? item->GetHWND() : DW_NOHWND, width, height, hsize, vsize, pad); }
    void PackAtIndex(Widget *item, int index, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_at_index(hwnd, item ? item->GetHWND() : DW_NOHWND, index, width, height, hsize, vsize, pad); }
    Widget *UnpackAtIndex(int index) { HWND widget = dw_box_unpack_at_index(hwnd, index);
        void *classptr = widget ? dw_window_get_data(widget, "_dw_classptr") : DW_NULL;
        return reinterpret_cast<Widget *>(classptr);
    }
};

class Box : public Boxes
{
public:
    // Constructors
    Box(int type, int pad) { SetHWND(dw_box_new(type, pad)); }
    Box(int type) { SetHWND(dw_box_new(type, 0)); }
};

// Special scrollable box
class ScrollBox : public Boxes
{
public:
    // Constructors
    ScrollBox(int type, int pad) { SetHWND(dw_scrollbox_new(type, pad)); }
    ScrollBox(int type) { SetHWND(dw_scrollbox_new(type, 0)); }

    // User functions
    int GetRange(int orient) { return dw_scrollbox_get_range(hwnd, orient); }
    int GetPos(int orient) { return dw_scrollbox_get_pos(hwnd, orient); }
};

// TODO: Find a way to implement this cross platform...
// That way we can skip adding unused signal handlers
#define IsOverridden(a, b) true

// Top-level window class is packable
class Window : public Boxes
{
private:
    void Setup() {	
        if(IsOverridden(Window::OnDelete, this))
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
    char *GetText() { return dw_window_get_text(hwnd); }
    void SetSize(unsigned long width, unsigned long height) { dw_window_set_size(hwnd, width, height); }
    int Show() { return dw_window_show(hwnd); }
    int Hide() { return dw_window_hide(hwnd); }
    void SetGravity(int horz, int vert) { dw_window_set_gravity(hwnd, horz, vert); }
    int Minimize() { return dw_window_minimize(hwnd); }
    int Raise() { return dw_window_raise(hwnd); }
    int Lower() { return dw_window_lower(hwnd); }
    void Redraw() { dw_window_redraw(hwnd); }
    void Default(Widget *defaultitem) { if(defaultitem) dw_window_default(hwnd, defaultitem->GetHWND()); }
    void SetIcon(HICN icon) { dw_window_set_icon(hwnd, icon); }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnDelete() { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_DELETE); return FALSE; }
    virtual int OnConfigure(int width, int height) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CONFIGURE); return FALSE; };
};

// Class for focusable widgets
class Focusable : virtual public Widget
{
public:
    void Enable() { dw_window_enable(hwnd); }
    void Disable() { dw_window_disable(hwnd); }
    void SetFocus() { dw_window_set_focus(hwnd); }
};

// Base class for several types of buttons
class Buttons : virtual public Focusable
{
private:
    static int _OnClicked(HWND window, void *data) { return reinterpret_cast<Buttons *>(data)->OnClicked(); }
protected:
    void Setup() {	
        if(IsOverridden(Buttons::OnClicked, this))
            dw_signal_connect(hwnd, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_OnClicked), this);
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnClicked() { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CLICKED); return FALSE; }
};

// Text based button
class TextButton : public Buttons
{
public:
    // User functions
    void SetText(const char *text) { dw_window_set_text(hwnd, text); }
    char *GetText() { return dw_window_get_text(hwnd); }
};

class Button : public TextButton
{
public:
    // Constructors
    Button(const char *text, unsigned long id) { SetHWND(dw_button_new(text, id)); Setup(); }
    Button(unsigned long id) { SetHWND(dw_button_new("", id)); Setup(); }
    Button(const char *text) { SetHWND(dw_button_new(text, 0)); Setup(); }
    Button() { SetHWND(dw_button_new("", 0)); Setup(); }
};

// Image based button
class BitmapButton : public Buttons
{
public:
    // Constructors
    BitmapButton(const char *text, unsigned long id) { SetHWND(dw_bitmapbutton_new(text, id)); Setup(); }
    BitmapButton(unsigned long id) { SetHWND(dw_bitmapbutton_new("", id)); Setup(); }
    BitmapButton(const char *text, unsigned long id, const char *file) { SetHWND(dw_bitmapbutton_new_from_file(text, id, file)); Setup(); }
    BitmapButton(const char *text, const char *file) { SetHWND(dw_bitmapbutton_new_from_file(text, 0, file)); Setup(); }
    BitmapButton(const char *text, unsigned long id, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text, id, data, len)); Setup(); }
    BitmapButton(const char *text, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text, 0, data, len)); Setup(); }
};

class CheckBoxes : virtual public TextButton
{
public:
    // User functions
    void Set(int value) { dw_checkbox_set(hwnd, value); }
    int Get() { return dw_checkbox_get(hwnd); }
};

class CheckBox : public CheckBoxes
{
public:
    // Constructors
    CheckBox(const char *text, unsigned long id) { SetHWND(dw_checkbox_new(text, id)); Setup(); }
    CheckBox(unsigned long id) { SetHWND(dw_checkbox_new("", id)); Setup(); }
    CheckBox(const char *text) { SetHWND(dw_checkbox_new(text, 0)); Setup(); }
    CheckBox() { SetHWND(dw_checkbox_new("", 0)); Setup(); }
};

class RadioButton : public CheckBoxes
{
public:
    // Constructors
    RadioButton(const char *text, unsigned long id) { SetHWND(dw_radiobutton_new(text, id)); Setup(); }
    RadioButton(unsigned long id) { SetHWND(dw_radiobutton_new("", id)); Setup(); }
    RadioButton(const char *text) { SetHWND(dw_radiobutton_new(text, 0)); Setup(); }
    RadioButton() { SetHWND(dw_radiobutton_new("", 0)); Setup(); }
};

// Class for handling static text widget
class TextWidget : virtual public Widget
{
public:
    // User functions
    void SetText(const char *text) { dw_window_set_text(hwnd, text); }
    char *GetText() { return dw_window_get_text(hwnd); }
    int SetFont(const char *font) { return dw_window_set_font(hwnd, font); }
    char *GetFont() { return dw_window_get_font(hwnd); }
};

// Class for handling static text widget
class Text : public TextWidget
{
public:
    // Constructors
    Text(const char *text, unsigned long id) { SetHWND(dw_text_new(text, id)); }
    Text(const char *text) { SetHWND(dw_text_new(text, 0)); }
    Text(unsigned long id) { SetHWND(dw_text_new("", id)); }
    Text() { SetHWND(dw_text_new("", 0)); }
};

class StatusText : public TextWidget
{
public:
    // Constructors
    StatusText(const char *text, unsigned long id) { SetHWND(dw_status_text_new(text, id)); }
    StatusText(const char *text) { SetHWND(dw_status_text_new(text, 0)); }
    StatusText(unsigned long id) { SetHWND(dw_status_text_new("", id)); }
    StatusText() { SetHWND(dw_status_text_new("", 0)); }
};

// Class for handing static image widget
class Bitmap : public Widget
{
public:
    // Constructors
    Bitmap(const char *data, int len) { SetHWND(dw_bitmap_new(0)); dw_window_set_bitmap_from_data(hwnd, 0, data, len); }
    Bitmap(const char *file) { SetHWND(dw_bitmap_new(0)); dw_window_set_bitmap(hwnd, 0, file); }
    Bitmap(unsigned long id) { SetHWND(dw_bitmap_new(id)); }
    Bitmap() { SetHWND(dw_bitmap_new(0)); }

    // User functions
    void Set(unsigned long id) { dw_window_set_bitmap(hwnd, id, DW_NULL); }
    void Set(const char *file) { dw_window_set_bitmap(hwnd, 0, file); }
    void Set(const char *data, int len) { dw_window_set_bitmap_from_data(hwnd, 0, data, len); }
};

// Forward declare these so our Drawable abstract class can reference
class Render;
class Pixmap;

// Abstract class that defines drawing, either to screen or picture (pixmap)
class Drawable
{
public:
    virtual void DrawPoint(int x, int y) = 0;
    virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;
    virtual void DrawPolygon(int flags, int npoints, int x[], int y[]) = 0;
    virtual void DrawRect(int fill, int x, int y, int width, int height) = 0;
    virtual void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2) = 0;
    virtual void DrawText(int x, int y, const char *text) = 0;
    virtual int BitBltStretch(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc, int srcwidth, int srcheight) = 0;
    virtual int BitBltStretch(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc, int srcwidth, int srcheight) = 0;
    virtual void BitBlt(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc) = 0;
    virtual void BitBlt(int xdest, int ydest, int width, int height, Pixmap *srcp, int xsrc, int ysrc) = 0;
    void SetColor(unsigned long fore, unsigned long back) { dw_color_foreground_set(fore); dw_color_background_set(back); }    
    void SetBackgroundColor(unsigned long back) { dw_color_background_set(back); }  
    void SetForegroundColor(unsigned long fore) { dw_color_foreground_set(fore); }      
};

class Render : public Drawable, public Widget
{
private:
    void Setup() {	
        if(IsOverridden(Render::OnExpose, this))
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
        if(IsOverridden(Render::OnConfigure, this))
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
    }
    static int _OnExpose(HWND window, DWExpose *exp, void *data) { return reinterpret_cast<Render *>(data)->OnExpose(exp); }
    static int _OnConfigure(HWND window, int width, int height, void *data) { return reinterpret_cast<Render *>(data)->OnConfigure(width, height); }
public:
    // Constructors
    Render(unsigned long id) { SetHWND(dw_render_new(id)); Setup(); }
    Render() { SetHWND(dw_render_new(0)); Setup(); }

    // User functions
    void DrawPoint(int x, int y) { dw_draw_point(hwnd, DW_NULL, x, y); }
    void DrawLine(int x1, int y1, int x2, int y2) { dw_draw_line(hwnd, DW_NULL, x1, y1, x2, y2); }
    void DrawPolygon(int flags, int npoints, int x[], int y[]) { dw_draw_polygon(hwnd, DW_NULL, flags, npoints, x, y); }
    void DrawRect(int fill, int x, int y, int width, int height) { dw_draw_rect(hwnd, DW_NULL, fill, x, y, width, height); }
    void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2) { dw_draw_arc(hwnd, DW_NULL, flags, xorigin, yorigin, x1, y1, x2, y2); }
    void DrawText(int x, int y, const char *text) { dw_draw_text(hwnd, DW_NULL, x, y, text); }
    int BitBltStretch(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc, int srcwidth, int srcheight) {
        return dw_pixmap_stretch_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc, srcwidth, srcheight);
    }
    int BitBltStretch(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc, int srcwidth, int srcheight);
    void BitBlt(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc) {
        return dw_pixmap_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc);
    }
    void BitBlt(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc);
    int SetFont(const char *fontname) { return dw_window_set_font(hwnd, fontname); }
    void GetTextExtents(const char *text, int *width, int *height) { dw_font_text_extents_get(hwnd, DW_NULL, text, width, height); }
    char *GetFont() { return dw_window_get_font(hwnd); }
    void Redraw() { dw_render_redraw(hwnd); }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnExpose(DWExpose *exp) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_EXPOSE); return FALSE; }
    virtual int OnConfigure(int width, int height) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CONFIGURE); return FALSE; };
};

class Pixmap : public Drawable, public Handle
{
private:
    void SetHPIXMAP(HPIXMAP newpixmap) { 
        hpixmap = newpixmap; 
        SetHandle(reinterpret_cast<void *>(newpixmap));
    }
    HPIXMAP hpixmap; 
public:
    // Constructors
    Pixmap(HWND window, unsigned long width, unsigned long height, int depth) { SetHPIXMAP(dw_pixmap_new(window, width, height, depth)); }
    Pixmap(HWND window, unsigned long width, unsigned long height) { SetHPIXMAP(dw_pixmap_new(window, width, height, 32)); }

    // User functions
    HPIXMAP GetHPIXMAP() { return hpixmap; }
    void DrawPoint(int x, int y) { dw_draw_point(DW_NOHWND, hpixmap, x, y); }
    void DrawLine(int x1, int y1, int x2, int y2) { dw_draw_line(DW_NOHWND, hpixmap, x1, y1, x2, y2); }
    void DrawPolygon(int flags, int npoints, int x[], int y[]) { dw_draw_polygon(DW_NOHWND, hpixmap, flags, npoints, x, y); }
    void DrawRect(int fill, int x, int y, int width, int height) { dw_draw_rect(DW_NOHWND, hpixmap, fill, x, y, width, height); }
    void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2) { dw_draw_arc(DW_NOHWND, hpixmap, flags, xorigin, yorigin, x1, y1, x2, y2); }
    void DrawText(int x, int y, const char *text) { dw_draw_text(DW_NOHWND, hpixmap, x, y, text); }
    int BitBltStretch(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc, int srcwidth, int srcheight) {
        return dw_pixmap_stretch_bitblt(DW_NOHWND, hpixmap, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc, srcwidth, srcheight);
    }
    int BitBltStretch(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc, int srcwidth, int srcheight) {
        return dw_pixmap_stretch_bitblt(DW_NOHWND, hpixmap, xdest, ydest, width, height, DW_NOHWND, src ? src->GetHPIXMAP() : DW_NULL, xsrc, ysrc, srcwidth, srcheight);
    }
    void BitBlt(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc) {
        dw_pixmap_bitblt(DW_NOHWND, hpixmap, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc);
    }
    void BitBlt(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc) {
        dw_pixmap_bitblt(DW_NOHWND, hpixmap, xdest, ydest, width, height, DW_NOHWND, src ? src->GetHPIXMAP() : DW_NULL, xsrc, ysrc);
    }
    int SetFont(const char *fontname) { return dw_pixmap_set_font(hpixmap, fontname); }
    void GetTextExtents(const char *text, int *width, int *height) { dw_font_text_extents_get(DW_NOHWND, hpixmap, text, width, height); }
};

// Need to declare these here after Pixmap is defined
int Render::BitBltStretch(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc, int srcwidth, int srcheight)
{
    return dw_pixmap_stretch_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, DW_NOHWND, src ? src->GetHPIXMAP() : DW_NULL, xsrc, ysrc, srcwidth, srcheight);
}

void Render::BitBlt(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc)
{
    dw_pixmap_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, DW_NOHWND, src ? src->GetHPIXMAP() : DW_NULL, xsrc, ysrc);
}

// Class for the HTML rendering widget
class HTML : public Widget
{
private:
    void Setup() {	
        if(IsOverridden(HTML::OnChanged, this))
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
        if(IsOverridden(HTML::OnResult, this))
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnResult), this);
    }
    static int _OnChanged(HWND window, int status, char *url, void *data) { return reinterpret_cast<HTML *>(data)->OnChanged(status, url); }
    static int _OnResult(HWND window, int status, char *result, void *scriptdata, void *data) { return reinterpret_cast<HTML *>(data)->OnResult(status, result, scriptdata); }
public:
    // Constructors
    HTML(unsigned long id) { SetHWND(dw_html_new(id)); Setup(); }
    HTML() { SetHWND(dw_html_new(0)); Setup(); }

    // User functions
    void Action(int action) { dw_html_action(hwnd, action); }
    int JavascriptRun(const char *script, void *scriptdata) { return dw_html_javascript_run(hwnd, script, scriptdata); }
    int Raw(const char *buffer) { return dw_html_raw(hwnd, buffer); }
    int URL(const char *url) { return dw_html_url(hwnd, url); }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnChanged(int status, char *url) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_CHANGED); return FALSE; }
    virtual int OnResult(int status, char *result, void *scriptdata) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_RESULT); return FALSE; };
};

// Base class for several widgets that allow text entry
class TextEntry : virtual public Focusable, virtual public TextWidget
{
public:
    // User functions
    void ClickDefault(Focusable *next) { dw_window_click_default(hwnd, next); }
};

class Entryfield : public TextEntry
{
public:
    // Constructors
    Entryfield(const char *text, unsigned long id) { SetHWND(dw_entryfield_new(text, id)); }
    Entryfield(unsigned long id) { SetHWND(dw_entryfield_new("", id)); }
    Entryfield(const char *text) { SetHWND(dw_entryfield_new(text, 0)); }
    Entryfield() { SetHWND(dw_entryfield_new("", 0)); }
};

class EntryfieldPassword : public TextEntry
{
public:
    // Constructors
    EntryfieldPassword(const char *text, unsigned long id) { SetHWND(dw_entryfield_password_new(text, id)); }
    EntryfieldPassword(unsigned long id) { SetHWND(dw_entryfield_password_new("", id)); }
    EntryfieldPassword(const char *text) { SetHWND(dw_entryfield_password_new(text, 0)); }
    EntryfieldPassword() { SetHWND(dw_entryfield_password_new("", 0)); }
};

// Base class for several widgets that have a list of elements
class ListBoxes : virtual public Focusable
{
private:
    void Setup() {	
        if(IsOverridden(ListBoxes::OnListSelect, this))
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
    }
    static int _OnListSelect(HWND window, int index, void *data) { return reinterpret_cast<ListBoxes *>(data)->OnListSelect(index); }
public:
    // User functions
    void Append(const char *text) { dw_listbox_append(hwnd, text); }
    void Clear() { dw_listbox_clear(hwnd); }
    int Count() { return dw_listbox_count(hwnd); }
    void Delete(int index) { dw_listbox_delete(hwnd, index); }
    void GetText(unsigned int index, char *buffer, unsigned int length) { dw_listbox_get_text(hwnd, index, buffer, length); }
    void SetText(unsigned int index, char *buffer) { dw_listbox_set_text(hwnd, index, buffer); }
    void Insert(const char *text, int pos) { dw_listbox_insert(hwnd, text, pos); }
    void ListAppend(char **text, int count) { dw_listbox_list_append(hwnd, text, count); }
    void Select(int index, int state) { dw_listbox_select(hwnd, index, state); }
    int Selected() { return dw_listbox_selected(hwnd); }
    int Selected(int where) { return dw_listbox_selected_multi(hwnd, where); }
    void SetTop(int top) { dw_listbox_set_top(hwnd, top); }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnListSelect(int index) { dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_LIST_SELECT); return FALSE; }
};

class Combobox : public TextEntry, public ListBoxes
{
public:
    // Constructors
    Combobox(const char *text, unsigned long id) { SetHWND(dw_combobox_new(text, id)); }
    Combobox(unsigned long id) { SetHWND(dw_combobox_new("", id)); }
    Combobox(const char *text) { SetHWND(dw_combobox_new(text, 0)); }
    Combobox() { SetHWND(dw_combobox_new("", 0)); }
};

class Listbox : public ListBoxes
{
public:
    // Constructors
    Listbox(unsigned long id, int multi) { SetHWND(dw_listbox_new(id, multi)); }
    Listbox(unsigned long id) { SetHWND(dw_listbox_new(id, FALSE)); }
    Listbox(int multi) { SetHWND(dw_listbox_new(0, multi)); }
    Listbox() { SetHWND(dw_listbox_new(0, FALSE)); }
};

class App
{
protected:
    App() { }
    static App *_app;
public:
// Allow the code to compile if handicapped on older compilers
#if __cplusplus >= 201103L
    // Singletons should not be cloneable.
    App(App &other) = delete;
    // Singletons should not be assignable.
    void operator=(const App &) = delete;
#endif
    // Initialization functions for creating App
    static App *Init() { if(!_app) { _app = new App; dw_init(TRUE, 0, DW_NULL); } return _app; }
    static App *Init(const char *appid) { if(!_app) { _app = new App(); dw_app_id_set(appid, DW_NULL); dw_init(TRUE, 0, DW_NULL); } return _app; }
    static App *Init(const char *appid, const char *appname) { if(!_app) { _app = new App(); dw_app_id_set(appid, appname); dw_init(TRUE, 0, DW_NULL); } return _app; }
    static App *Init(int argc, char *argv[]) { if(!_app) { _app = new App(); dw_init(TRUE, argc, argv); } return _app; }
    static App *Init(int argc, char *argv[], const char *appid) { if(!_app) { _app = new App(); dw_app_id_set(appid, DW_NULL); dw_init(TRUE, argc, argv); } return _app; }
    static App *Init(int argc, char *argv[], const char *appid, const char *appname) { if(!_app) { _app = new App(); dw_app_id_set(appid, appname); dw_init(TRUE, argc, argv); } return _app; }

    void Main() { dw_main(); }
    void MainIteration() { dw_main_iteration(); }
    void MainQuit() { dw_main_quit(); }
    void Exit(int exitcode) { dw_exit(exitcode); }
};

// Static singleton reference declared outside of the class
App* App::_app = DW_NULL;

} /* namespace DW */
#endif