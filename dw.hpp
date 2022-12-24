/* Dynamic Windows C++ Language Bindings 
 * Copyright 2022 Brian Smith
 * Recommends a C++11 compatible compiler.
 */

#ifndef _HPP_DW
#define _HPP_DW
#include <dw.h>

// Attempt to support compilers without nullptr type literal
#if __cplusplus >= 201103L 
#define DW_CPP11
#define DW_NULL nullptr
#else
#define DW_NULL NULL
#endif

// Support Lambdas on C++11, Visual C 2010 or GCC 4.5
#if defined(DW_CPP11) || (defined(_MSC_VER) && _MSC_VER >= 1600) || \
    (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 4)))
#define DW_LAMBDA
#include <functional>
#endif

// Attempt to allow compilation on GCC older than 4.7
#if defined(__GNUC__) && (__GNuC__ < 5 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#define override
#endif	

namespace DW 
{

// Forward declare these so they can be referenced
class Render;
class Pixmap;
class MenuItem;
class Window;

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

// Base class for several types of widgets including buttons and menu items
class Clickable : virtual public Widget
{
private:
    bool ClickedConnected;
#ifdef DW_LAMBDA
    std::function<int()> _ConnectClicked;
#else
    int (*_ConnectClicked)();
#endif
    static int _OnClicked(HWND window, void *data) { 
        if(reinterpret_cast<Clickable *>(data)->_ConnectClicked) 
            return reinterpret_cast<Clickable *>(data)->_ConnectClicked();
        return reinterpret_cast<Clickable *>(data)->OnClicked(); }
protected:
    void Setup() {	
        if(IsOverridden(Clickable::OnClicked, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_OnClicked), this);
            ClickedConnected = true;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnClicked() {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CLICKED);
        ClickedConnected = false;
        return FALSE;
    }
public:
#ifdef DW_LAMBDA
    void ConnectClicked(std::function<int()> userfunc)
#else
    void ConnectClicked(int (*userfunc)())
#endif
    {
        _ConnectClicked = userfunc;
        if(!ClickedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_OnClicked), this);
            ClickedConnected = true;
        }
    }
};

class Menus : public Handle
{
protected:
    void SetHMENUI(HMENUI newmenu) { 
        menu = newmenu; 
        SetHandle(reinterpret_cast<void *>(newmenu));
    }
    HMENUI menu; 
public:
    // User functions
    HMENUI GetHMENUI() { return menu; }
    MenuItem *AppendItem(const char *title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu);
    MenuItem *AppendItem(const char *title, Menus *submenu);
    MenuItem *AppendItem(const char *title);
};

class Menu : public Menus
{
public:
    // Constructors
    Menu(unsigned long id) { SetHMENUI(dw_menu_new(id)); }
    Menu() { SetHMENUI(dw_menu_new(0)); }

    // User functions
    void Popup(Window *window, int x, int y);
    void Popup(Window *window);
};

class MenuBar : public Menus
{
public:
    // Constructors
    MenuBar(HWND location) { SetHMENUI(dw_menubar_new(location)); }
};

class MenuItem : public Clickable
{
public:
    // Constructors
    MenuItem(Menus *menu, const char *title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, id, flags, end, check, submenu ? submenu->GetHMENUI() : DW_NOMENU)); 
    }
    MenuItem(Menus *menu, const char *title, Menus *submenu) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, 0, TRUE, FALSE, submenu ? submenu->GetHMENUI() : DW_NOMENU));
    }
    MenuItem(Menus *menu, const char *title) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU));
    }

    // User functions
    void SetState(unsigned long flags) { dw_window_set_style(hwnd, flags, flags); }
    void SetStyle(unsigned long flags, unsigned long mask) { dw_window_set_style(hwnd, flags, mask); }
};

MenuItem *Menus::AppendItem(const char *title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu) {
    return new MenuItem(this, title, id, flags, end, check, submenu);
}

MenuItem *Menus::AppendItem(const char *title, Menus *submenu) {
    return new MenuItem(this, title, submenu);
}
MenuItem *Menus::AppendItem(const char *title) {
    return new MenuItem(this, title);
}


// Top-level window class is packable
class Window : public Boxes
{
private:
    bool DeleteConnected, ConfigureConnected;
#ifdef DW_LAMBDA
    std::function<int()> _ConnectDelete;
    std::function<int(int, int)> _ConnectConfigure;
#else
    int (*_ConnectDelete)();
    int (*_ConnectConfigure)(int width, int height);
#endif
    void Setup() {	
        if(IsOverridden(Window::OnDelete, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
            DeleteConnected = true;
        }
        if(IsOverridden(Window::OnConfigure, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
    }
    static int _OnDelete(HWND window, void *data) { 
        if(reinterpret_cast<Window *>(data)->_ConnectDelete)
            return reinterpret_cast<Window *>(data)->_ConnectDelete();
        return reinterpret_cast<Window *>(data)->OnDelete(); }
    static int _OnConfigure(HWND window, int width, int height, void *data) { 
        if(reinterpret_cast<Window *>(data)->_ConnectConfigure)
            return reinterpret_cast<Window *>(data)->_ConnectConfigure(width, height);
        return reinterpret_cast<Window *>(data)->OnConfigure(width, height); }
    MenuBar *menu;
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
    MenuBar *MenuBarNew() { if(!menu) menu = new MenuBar(hwnd); return menu; }
    void Popup(Menu *menu, int x, int y) {
        if(menu) {
            HMENUI pmenu = menu->GetHMENUI();

            dw_menu_popup(&pmenu, hwnd, x, y);
            delete menu; 
        }
    }
    void Popup(Menu *menu) {
        if(menu) {
            long x, y;
            HMENUI pmenu = menu->GetHMENUI();

            dw_pointer_query_pos(&x, &y);
            dw_menu_popup(&pmenu, hwnd, (int)x, (int)y);
            delete menu;
        }
    }
#ifdef DW_LAMBDA
    void ConnectDelete(std::function<int()> userfunc)
#else
    void ConnectDelete(int (*userfunc)()) 
#endif
    {
        _ConnectDelete = userfunc;
        if(!DeleteConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
            DeleteConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectConfigure(std::function<int(int, int)> userfunc)
#else
    void ConnectConfigure(int (*userfunc)(int, int)) 
#endif
    {
        _ConnectConfigure = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
    }    
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnDelete() {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_DELETE);
        DeleteConnected = false;
        return FALSE;
    }
    virtual int OnConfigure(int width, int height) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CONFIGURE);
        ConfigureConnected = false;
        return FALSE;
    };
};

void Menu::Popup(Window *window, int x, int y) 
{
    if(window)
    {
        HMENUI pmenu = menu;

        dw_menu_popup(&pmenu, window->GetHWND(), x, y);
        delete this;
    }
}

void Menu::Popup(Window *window)
{
    if(window) {
        long x, y;
        HMENUI pmenu = menu;

        dw_pointer_query_pos(&x, &y);
        dw_menu_popup(&pmenu, window->GetHWND(), (int)x, (int)y);
        delete this;
    }
}

// Class for focusable widgets
class Focusable : virtual public Widget
{
public:
    void Enable() { dw_window_enable(hwnd); }
    void Disable() { dw_window_disable(hwnd); }
    void SetFocus() { dw_window_set_focus(hwnd); }
};

// Text based button
class TextButton : public Clickable, public Focusable
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
class BitmapButton : public Clickable, public Focusable
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

// Class for handing calendar widget
class Calendar : public Widget
{
public:
    // Constructors
    Calendar(unsigned long id) { SetHWND(dw_calendar_new(id)); }
    Calendar() { SetHWND(dw_calendar_new(0)); }

    // User functions
    void GetData(unsigned int *year, unsigned int *month, unsigned int *day) { dw_calendar_get_date(hwnd, year, month, day); }
    void SetData(unsigned int year, unsigned int month, unsigned int day) { dw_calendar_set_date(hwnd, year, month, day); }
};


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
    bool ExposeConnected, ConfigureConnected, KeyPressConnected, ButtonPressConnected, ButtonReleaseConnected, MotionNotifyConnected;
#ifdef DW_LAMBDA
    std::function<int(DWExpose *)> _ConnectExpose;
    std::function<int(int, int)> _ConnectConfigure;
    std::function<int(char c, int, int, char *)> _ConnectKeyPress;
    std::function<int(int, int, int)> _ConnectButtonPress;
    std::function<int(int, int, int)> _ConnectButtonRelease;
    std::function<int(int, int, int)> _ConnectMotionNotify;
#else
    int (*_ConnectExpose)(DWExpose *);
    int (*_ConnectConfigure)(int width, int height);
    int (*_ConnectKeyPress)(char c, int vk, int state, char *utf8);
    int (*_ConnectButtonPress)(int x, int y, int buttonmask);
    int (*_ConnectButtonRelease)(int x, int y, int buttonmask);
    int (*_ConnectMotionNotify)(int x, int y, int buttonmask);
#endif
    void Setup() {	
        if(IsOverridden(Render::OnExpose, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
            ExposeConnected = true;
        }
        if(IsOverridden(Render::OnConfigure, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
        if(IsOverridden(Render::OnKeyPress, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        }
        if(IsOverridden(Render::OnButtonPress, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_OnButtonPress), this);
            ButtonPressConnected = true;
        }
        if(IsOverridden(Render::OnButtonRelease, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_RELEASE, DW_SIGNAL_FUNC(_OnButtonRelease), this);
            ButtonReleaseConnected = true;
        }
        if(IsOverridden(Render::OnMotionNotify, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_MOTION_NOTIFY, DW_SIGNAL_FUNC(_OnMotionNotify), this);
            MotionNotifyConnected = true;
        }
    }
    static int _OnExpose(HWND window, DWExpose *exp, void *data) {
        if(reinterpret_cast<Render *>(data)->_ConnectExpose)
            return reinterpret_cast<Render *>(data)->_ConnectExpose(exp);
        return reinterpret_cast<Render *>(data)->OnExpose(exp); }
    static int _OnConfigure(HWND window, int width, int height, void *data) {
        if(reinterpret_cast<Render *>(data)->_ConnectConfigure)
            return reinterpret_cast<Render *>(data)->_ConnectConfigure(width, height);
        return reinterpret_cast<Render *>(data)->OnConfigure(width, height); }
    static int _OnKeyPress(HWND window, char c, int vk, int state, void *data, char *utf8) {
        if(reinterpret_cast<Render *>(data)->_ConnectKeyPress)
            return reinterpret_cast<Render *>(data)->_ConnectKeyPress(c, vk, state, utf8);
        return reinterpret_cast<Render *>(data)->OnKeyPress(c, vk, state, utf8); }
    static int _OnButtonPress(HWND window, int x, int y, int buttonmask, void *data) {
        if(reinterpret_cast<Render *>(data)->_ConnectButtonPress)
            return reinterpret_cast<Render *>(data)->_ConnectButtonPress(x, y, buttonmask);
        return reinterpret_cast<Render *>(data)->OnButtonPress(x, y, buttonmask); }
    static int _OnButtonRelease(HWND window, int x, int y, int buttonmask, void *data) {
        if(reinterpret_cast<Render *>(data)->_ConnectButtonRelease)
            return reinterpret_cast<Render *>(data)->_ConnectButtonRelease(x, y, buttonmask);
        return reinterpret_cast<Render *>(data)->OnButtonRelease(x, y, buttonmask); }
    static int _OnMotionNotify(HWND window, int x, int y, int  buttonmask, void *data) {
        if(reinterpret_cast<Render *>(data)->_ConnectMotionNotify)
            return reinterpret_cast<Render *>(data)->_ConnectMotionNotify(x, y, buttonmask);
        return reinterpret_cast<Render *>(data)->OnMotionNotify(x, y, buttonmask); }
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
#ifdef DW_LAMBDA
    void ConnectExpose(std::function<int(DWExpose *)> userfunc)
#else
    void ConnectExpose(int (*userfunc)(DWExpose *))
#endif
    {
        _ConnectExpose = userfunc;
        if(!ExposeConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
            ExposeConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectConfigure(std::function<int(int, int)> userfunc)
#else
    void ConnectConfigure(int (*userfunc)(int, int))
#endif
    {
        _ConnectConfigure = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectKeyPress(std::function<int(char, int, int, char *)> userfunc)
#else
    void ConnectKeyPress(int (*userfunc)(char, int, int, char *))
#endif
    {
        _ConnectKeyPress = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectButtonPress(std::function<int(int, int, int)> userfunc)
#else
    void ConnectButtonPress(int (*userfunc)(int, int, int))
#endif
    {
        _ConnectButtonPress = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_OnButtonPress), this);
            ButtonPressConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectButtonRelease(std::function<int(int, int, int)> userfunc)
#else
    void ConnectButtonRelease(int (*userfunc)(int, int, int))
#endif
    {
        _ConnectButtonRelease = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_RELEASE, DW_SIGNAL_FUNC(_OnButtonRelease), this);
            ButtonReleaseConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectMotionNotify(std::function<int(int, int, int)> userfunc)
#else
    void ConnectMotionNotify(int (*userfunc)(int, int, int))
#endif
    {
        _ConnectMotionNotify = userfunc;
        if(!MotionNotifyConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_MOTION_NOTIFY, DW_SIGNAL_FUNC(_OnMotionNotify), this);
            MotionNotifyConnected = true;
        }
    }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnExpose(DWExpose *exp) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_EXPOSE);
        ExposeConnected = false;
        return FALSE;
    }
    virtual int OnConfigure(int width, int height) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_CONFIGURE);
        ConfigureConnected = false;
        return FALSE;
    };
    virtual int OnKeyPress(char c, int vk, int state, char *utf8) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_KEY_PRESS);
        KeyPressConnected = false;
        return FALSE;
    };
    virtual int OnButtonPress(char x, int y, int buttonmask) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_BUTTON_PRESS);
        ButtonPressConnected = false;
        return FALSE;
    };
    virtual int OnButtonRelease(char x, int y, int buttonmask) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_BUTTON_RELEASE);
        ButtonReleaseConnected = false;
        return FALSE;
    };
    virtual int OnMotionNotify(char x, int y, int buttonmask) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_MOTION_NOTIFY);
        MotionNotifyConnected = false;
        return FALSE;
    };
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
    bool ChangedConnected, ResultConnected;
#ifdef DW_LAMBDA
    std::function<int(int, char *)> _ConnectChanged;
    std::function<int(int, char *, void *)> _ConnectResult;
#else
    int (*_ConnectChanged)(int status, char *url);
    int (*_ConnectResult)(int status, char *result, void *scriptdata);
#endif
    void Setup() {	
        if(IsOverridden(HTML::OnChanged, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        }
        if(IsOverridden(HTML::OnResult, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        }
    }
    static int _OnChanged(HWND window, int status, char *url, void *data) {
        if(reinterpret_cast<HTML *>(data)->_ConnectChanged)
            return reinterpret_cast<HTML *>(data)->_ConnectChanged(status, url);
        return reinterpret_cast<HTML *>(data)->OnChanged(status, url); }
    static int _OnResult(HWND window, int status, char *result, void *scriptdata, void *data) {
        if(reinterpret_cast<HTML *>(data)->_ConnectResult)
            return reinterpret_cast<HTML *>(data)->_ConnectResult(status, result, scriptdata);
        return reinterpret_cast<HTML *>(data)->OnResult(status, result, scriptdata); }
public:
    // Constructors
    HTML(unsigned long id) { SetHWND(dw_html_new(id)); Setup(); }
    HTML() { SetHWND(dw_html_new(0)); Setup(); }

    // User functions
    void Action(int action) { dw_html_action(hwnd, action); }
    int JavascriptRun(const char *script, void *scriptdata) { return dw_html_javascript_run(hwnd, script, scriptdata); }
    int Raw(const char *buffer) { return dw_html_raw(hwnd, buffer); }
    int URL(const char *url) { return dw_html_url(hwnd, url); }
#ifdef DW_LAMBDA
    void ConnectChanged(std::function<int(int, char *)> userfunc)
#else
    void ConnectChanged(int (*userfunc)(int, char *))
#endif
    { 
        _ConnectChanged = userfunc;
        if(!ChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectResult(std::function<int(int, char *, void *)> userfunc)
#else
    void ConnectResult(int (*userfunc)(int, char *, void *))
#endif
    {
        _ConnectResult = userfunc;
        if(!ResultConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        }
    }    
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnChanged(int status, char *url) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_CHANGED); 
        ChangedConnected = false;
        return FALSE;
    }
    virtual int OnResult(int status, char *result, void *scriptdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_RESULT);
        ResultConnected = false;
        return FALSE;
    };
};

// Base class for several widgets that allow text entry
class TextEntry : virtual public Focusable, virtual public TextWidget
{
public:
    // User functions
    void ClickDefault(Focusable *next) { if(next) dw_window_click_default(hwnd, next->GetHWND()); }
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
    bool ListSelectConnected;
#ifdef DW_LAMBDA
    std::function<int(int)> _ConnectListSelect;
#else
    int (*_ConnectListSelect)(int index);
#endif
    void Setup() {	
        if(IsOverridden(ListBoxes::OnListSelect, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
            ListSelectConnected = true;
        }
    }
    static int _OnListSelect(HWND window, int index, void *data) {
        if(reinterpret_cast<ListBoxes *>(data)->_ConnectListSelect)
            return reinterpret_cast<ListBoxes *>(data)->_ConnectListSelect(index);
        return reinterpret_cast<ListBoxes *>(data)->OnListSelect(index);
    }
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
#ifdef DW_LAMBDA
    void ConnectListSelect(std::function<int(int)> userfunc)
#else
    void ConnectListSelect(int (*userfunc)(int))
#endif
    {
        _ConnectListSelect = userfunc;
        if(!ListSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
            ListSelectConnected = true;
        }
    }    
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnListSelect(int index) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_LIST_SELECT);
        ListSelectConnected = false;
        return FALSE;
    }
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

// Base class for several ranged type widgets
class Ranged : virtual public Widget
{
private:
    bool ValueChangedConnected;
#ifdef DW_LAMBDA
    std::function<int(int)> _ConnectValueChanged;
#else
    int (*_ConnectValueChanged)(int value);
#endif
    static int _OnValueChanged(HWND window, int value, void *data) {
        if(reinterpret_cast<Ranged *>(data)->_ConnectValueChanged)
            return reinterpret_cast<Ranged *>(data)->_ConnectValueChanged(value);
        return reinterpret_cast<Ranged *>(data)->OnValueChanged(value);
    }
protected:
    void Setup() {	
        if(IsOverridden(Ranged::OnValueChanged, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(_OnValueChanged), this);
            ValueChangedConnected = true;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnValueChanged(int value) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_VALUE_CHANGED);
        ValueChangedConnected = false;
        return FALSE;
    }
public:
#ifdef DW_LAMBDA
    void ConnectValueChanged(std::function<int(int)> userfunc)
#else
    void ConnectValueChanged(int (*userfunc)(int))
#endif
    {
        _ConnectValueChanged = userfunc;
        if(!ValueChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(_OnValueChanged), this);
            ValueChangedConnected = true;
        }
    }    
};

class Slider : public Ranged
{
public:
    // Constructors
    Slider(int orient, int increments, unsigned long id) { SetHWND(dw_slider_new(orient, increments, id)); Setup(); }
    Slider(int orient, int increments) { SetHWND(dw_slider_new(orient, increments, 0)); Setup(); }

    // User functions
    unsigned int GetPos() { return dw_slider_get_pos(hwnd); }
    void SetPos(unsigned int position) { dw_slider_set_pos(hwnd, position); }
};

class Scrollbar : public Ranged
{
public:
    // Constructors
    Scrollbar(int orient, unsigned long id) { SetHWND(dw_scrollbar_new(orient, id)); Setup(); }
    Scrollbar(int orient) { SetHWND(dw_scrollbar_new(orient, 0)); Setup(); }

    // User functions
    unsigned int GetPos() { return dw_scrollbar_get_pos(hwnd); }
    void SetPos(unsigned int position) { dw_scrollbar_set_pos(hwnd, position); }
    void SetRange(unsigned int range, unsigned int visible) { dw_scrollbar_set_range(hwnd, range, visible); }
};

class SpinButton : public Ranged, public TextEntry
{
public:
    // Constructors
    SpinButton(const char *text, unsigned long id) { SetHWND(dw_spinbutton_new(text, id)); Setup(); }
    SpinButton(unsigned long id) { SetHWND(dw_spinbutton_new("", id)); Setup(); }
    SpinButton(const char *text) { SetHWND(dw_spinbutton_new(text, 0)); Setup(); }
    SpinButton() { SetHWND(dw_spinbutton_new("", 0)); Setup(); }

    // User functions
    long GetPos() { return dw_spinbutton_get_pos(hwnd); }
    void SetPos(long position) { dw_spinbutton_set_pos(hwnd, position); }
    void SetLimits(long upper, long lower) { dw_spinbutton_set_limits(hwnd, upper, lower); }
};

// Multi-line Edit widget
class MLE : public Focusable
{
public:
    // Constructors
    MLE(unsigned long id) { SetHWND(dw_mle_new(id)); }
    MLE() { SetHWND(dw_mle_new(0)); }

    // User functions
    void Freeze() { dw_mle_freeze(hwnd); }
    void Thaw() { dw_mle_thaw(hwnd); }
    void Clear() { dw_mle_clear(hwnd); }
    void Delete(int startpoint, int length) { dw_mle_delete(hwnd, startpoint, length); }
    void Export(char *buffer, int startpoint, int length) { dw_mle_export(hwnd, buffer, startpoint, length); }
    void Import(const char *buffer, int startpoint) { dw_mle_import(hwnd, buffer, startpoint); }
    void GetSize(unsigned long *bytes, unsigned long *lines) { dw_mle_get_size(hwnd, bytes, lines); }
    void Search(const char *text, int point, unsigned long flags) { dw_mle_search(hwnd, text, point, flags); }
    void SetAutoComplete(int state) { dw_mle_set_auto_complete(hwnd, state); }
    void SetCursor(int point) { dw_mle_set_cursor(hwnd, point); }
    void SetEditable(int state) { dw_mle_set_editable(hwnd, state); }
    void SetVisible(int line) { dw_mle_set_visible(hwnd, line); }
    void SetWordWrap(int state) { dw_mle_set_word_wrap(hwnd, state); }
};

class ObjectView : virtual public Widget
{
private:
    bool ItemSelectConnected, ItemContextConnected;
#ifdef DW_LAMBDA
    std::function<int(HTREEITEM, char *, void *)> _ConnectItemSelect;
    std::function<int(char *, int, int, void *)> _ConnectItemContext;
#else
    int (*_ConnectItemSelect)(HTREEITEM, char *, void *);
    int (*_ConnectItemContext)(char *, int, int, void *);
#endif
    static int _OnItemSelect(HWND window, HTREEITEM item, char *text, void *itemdata, void *data) {
        if(reinterpret_cast<ObjectView *>(data)->_ConnectItemSelect)
            return reinterpret_cast<ObjectView *>(data)->_ConnectItemSelect(item, text, itemdata);
        return reinterpret_cast<ObjectView *>(data)->OnItemSelect(item, text, itemdata);
    }
    static int _OnItemContext(HWND window, char *text, int x, int y, void *itemdata, void *data) {
        if(reinterpret_cast<ObjectView *>(data)->_ConnectItemContext)
            return reinterpret_cast<ObjectView *>(data)->_ConnectItemContext(text, x, y, itemdata);
        return reinterpret_cast<ObjectView *>(data)->OnItemContext(text, x, y, itemdata);
    }
protected:
    void SetupObjectView() {	
        if(IsOverridden(ObjectView::OnItemSelect, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        }
        if(IsOverridden(ObjectView::OnItemContext, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(_OnItemContext), this);
            ItemContextConnected = true;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnItemSelect(HTREEITEM item, char *text, void *itemdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_ITEM_SELECT);
        ItemSelectConnected = false;
        return FALSE;
    }
    virtual int OnItemContext(char *text, int x, int y, void *itemdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_ITEM_CONTEXT);
        ItemContextConnected = false;
        return FALSE;
    }
public:
#ifdef DW_LAMBDA
    void ConnectItemSelect(std::function<int(HTREEITEM, char *, void *)> userfunc)
#else
    void ConnectItemSelect(int (*userfunc)(HTREEITEM, char *, void *))
#endif
    {
        _ConnectItemSelect = userfunc;
        if(!ItemSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        }
    }        
#ifdef DW_LAMBDA
    void ConnectItemContext(std::function<int(char *, int, int, void *)> userfunc)
#else
    void ConnectItemContext(int (*userfunc)(char *, int, int, void *))
#endif
    {
        _ConnectItemContext = userfunc;
        if(!ItemContextConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(_OnItemContext), this);
            ItemContextConnected = true;
        }
    }        
};

class Containers : virtual public Focusable, virtual public ObjectView
{
private:
    bool ItemEnterConnected, ColumnClickConnected;
#ifdef DW_LAMBDA
    std::function<int(char *)> _ConnectItemEnter;
    std::function<int(int)> _ConnectColumnClick;
#else
    int (*_ConnectItemEnter)(char *);
    int (*_ConnectColumnClick)(int);
#endif
    static int _OnItemEnter(HWND window, char *text, void *data) {
        if(reinterpret_cast<Containers *>(data)->_ConnectItemEnter)
            return reinterpret_cast<Containers *>(data)->_ConnectItemEnter(text);
        return reinterpret_cast<Containers *>(data)->OnItemEnter(text);
    }
    static int _OnColumnClick(HWND window, int column, void *data) {
        if(reinterpret_cast<Containers *>(data)->_ConnectColumnClick)
            return reinterpret_cast<Containers *>(data)->_ConnectColumnClick(column);
        return reinterpret_cast<Containers *>(data)->OnColumnClick(column);
    }
protected:
    void *allocpointer;
    int allocrowcount;
    void SetupContainer() {	
        if(IsOverridden(Container::OnItemEnter, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        }
        if(IsOverridden(Container::OnColumnClick, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(_OnColumnClick), this);
            ColumnClickConnected = true;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnItemEnter(char *text) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_ITEM_ENTER);
        ItemEnterConnected = false;
        return FALSE;
    }
    virtual int OnColumnClick(int column) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_COLUMN_CLICK);
        ColumnClickConnected = false;
        return FALSE;
    }
public:
    // User functions
    void Alloc(int rowcount) { allocpointer = dw_container_alloc(hwnd, rowcount); allocrowcount = rowcount; }
    void ChangeRowTitle(int row, char *title) { dw_container_change_row_title(hwnd, row, title); }
    void Clear(int redraw) { dw_container_clear(hwnd, redraw); }
    void Clear() { dw_container_clear(hwnd, TRUE); }
    void Cursor(const char *text) { dw_container_cursor(hwnd, text); }
    void Cursor(void *data) { dw_container_cursor_by_data(hwnd, data); }
    void Delete(int rowcount) { dw_container_delete(hwnd, rowcount); }
    void DeleteRow(char *title) { dw_container_delete_row(hwnd, title); }
    void DeleteRow(void *data) { dw_container_delete_row_by_data(hwnd, data); }
    void Insert() { dw_container_insert(hwnd, allocpointer, allocrowcount); }
    void Optimize() { dw_container_optimize(hwnd); }
    char *QueryNext(unsigned long flags) { return dw_container_query_next(hwnd, flags); }
    char *QueryStart(unsigned long flags) { return dw_container_query_start(hwnd, flags); }
    void Scroll(int direction, long rows) { dw_container_scroll(hwnd, direction, rows); }
    void SetColumnWidth(int column, int width) { dw_container_set_column_width(hwnd, column, width); }
    void SetRowData(int row, void *data) { dw_container_set_row_data(allocpointer, row, data); }
    void SetRowTitle(int row, const char *title) { dw_container_set_row_title(allocpointer, row, title); }
    void SetStripe(unsigned long oddcolor, unsigned long evencolor) { dw_container_set_stripe(hwnd, oddcolor, evencolor); }
#ifdef DW_LAMBDA
    void ConnectItemEnter(std::function<int(char *)> userfunc)
#else
    void ConnectItemEnter(int (*userfunc)(char *))
#endif
    {
        _ConnectItemEnter = userfunc;
        if(!ItemEnterConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        }
    }        
#ifdef DW_LAMBDA
    void ConnecColumnClick(std::function<int(int)> userfunc)
#else
    void ConnectColumnClick(int (*userfunc)(int))
#endif
    {
        _ConnectColumnClick = userfunc;
        if(!ColumnClickConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(_OnColumnClick), this);
            ColumnClickConnected = true;
        }
    }        
};

class Container : public Containers
{
public:
    // Constructors
    Container(unsigned long id, int multi) { SetHWND(dw_container_new(id, multi)); SetupObjectView(); SetupContainer(); }
    Container(int multi) { SetHWND(dw_container_new(0, multi)); SetupObjectView(); SetupContainer(); }
    Container() { SetHWND(dw_container_new(0, FALSE)); SetupObjectView(); SetupContainer(); }

    // User functions
    int Setup(unsigned long *flags, char **titles, int count, int separator) { return dw_container_setup(hwnd, flags, titles, count, separator); }    
    void ChangeItem(int column, int row, void *data) { dw_container_change_item(hwnd, column, row, data); }
    int GetColumnType(int column) { return dw_container_get_column_type(hwnd, column); }
    void SetItem(int column, int row, void *data) { dw_container_set_item(hwnd, allocpointer, column, row, data); }
};

class Filesystem : public Containers
{
public:
    // Constructors
    Filesystem(unsigned long id, int multi) { SetHWND(dw_container_new(id, multi)); SetupObjectView(); SetupContainer(); }
    Filesystem(int multi) { SetHWND(dw_container_new(0, multi)); SetupObjectView(); SetupContainer(); }
    Filesystem() { SetHWND(dw_container_new(0, FALSE)); SetupObjectView(); SetupContainer(); }

    // User functions
    int Setup(unsigned long *flags, char **titles, int count) { return dw_filesystem_setup(hwnd, flags, titles, count); }    
    void ChangeFile(int row, const char *filename, HICN icon) { dw_filesystem_change_file(hwnd, row, filename, icon); }
    void ChangeItem(int column, int row, void *data) { dw_filesystem_change_item(hwnd, column, row, data); }
    int GetColumnType(int column) { return dw_filesystem_get_column_type(hwnd, column); }
    void SetColumnTitle(const char *title) { dw_filesystem_set_column_title(hwnd, title); }
    void SetFile(int row, const char *filename, HICN icon) { dw_filesystem_set_file(hwnd, allocpointer, row, filename, icon); }
    void SetItem(int column, int row, void *data) { dw_filesystem_set_item(hwnd, allocpointer, column, row, data); }
};

class Tree : virtual public Focusable, virtual public ObjectView
{
private:
    bool TreeExpandConnected;
#ifdef DW_LAMBDA
    std::function<int(HTREEITEM)> _ConnectTreeExpand;
#else
    int (*_ConnectTreeExpand)(HTREEITEM);
#endif
    static int _OnTreeExpand(HWND window, HTREEITEM item, void *data) {
        if(reinterpret_cast<Tree *>(data)->_ConnectTreeExpand)
            return reinterpret_cast<Tree *>(data)->_ConnectTreeExpand(item);
        return reinterpret_cast<Tree *>(data)->OnTreeExpand(item);
    }
    void SetupTree() {	
        if(IsOverridden(Tree::OnTreeExpand, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_OnTreeExpand), this);
            TreeExpandConnected = true;
        }
    }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnTreeExpand(HTREEITEM item) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_TREE_EXPAND);
        TreeExpandConnected = false;
        return FALSE;
    }
public:
    // Constructors
    Tree(unsigned long id) { SetHWND(dw_tree_new(id)); SetupObjectView(); SetupTree(); }
    Tree() { SetHWND(dw_tree_new(0)); SetupObjectView(); SetupTree(); }

    // User functions
    void Clear() { dw_tree_clear(hwnd); }
    HTREEITEM GetParent(HTREEITEM item) { return dw_tree_get_parent(hwnd, item); }
    char *GetTitle(HTREEITEM item) { return dw_tree_get_title(hwnd, item); }
    HTREEITEM Insert(const char *title, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert(hwnd, title, icon, parent, itemdata); }
    HTREEITEM Insert(const char *title, HTREEITEM item, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert_after(hwnd, item, title, icon, parent, itemdata); }
    void Change(HTREEITEM item, const char *title, HICN icon) { dw_tree_item_change(hwnd, item, title, icon); }
    void Collapse(HTREEITEM item) { dw_tree_item_collapse(hwnd, item); }
    void Delete(HTREEITEM item) { dw_tree_item_delete(hwnd, item); }
    void Expand(HTREEITEM item) { dw_tree_item_expand(hwnd, item); }
    void *GetData(HTREEITEM item) { return dw_tree_item_get_data(hwnd, item); }
    void Select(HTREEITEM item) { dw_tree_item_select(hwnd, item); }
    void SetData(HTREEITEM item, void *itemdata) { dw_tree_item_set_data(hwnd, item, itemdata); }
#ifdef DW_LAMBDA
    void ConnectTreeExpand(std::function<int(HTREEITEM)> userfunc)
#else
    void ConnectTreeExpand(int (*userfunc)(HTREEITEM))
#endif
    {
        _ConnectTreeExpand = userfunc;
        if(!TreeExpandConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_OnTreeExpand), this);
            TreeExpandConnected = true;
        }
    }
};


class App
{
protected:
    App() { }
    static App *_app;
public:
// Allow the code to compile if handicapped on older compilers
#ifdef DW_CPP11
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

    // User functions
    void Main() { dw_main(); }
    void MainIteration() { dw_main_iteration(); }
    void MainQuit() { dw_main_quit(); }
    void Exit(int exitcode) { dw_exit(exitcode); }
    int MessageBox(const char *title, int flags, const char *format) { return dw_messagebox(title, flags, format); }
};

// Static singleton reference declared outside of the class
App* App::_app = DW_NULL;

} /* namespace DW */
#endif