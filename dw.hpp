// Dynamic Windows C++ Language Bindings 
// Copyright (C) 2022-2023 Brian Smith
// Recommends a C++11 compatible compiler.

#ifndef _HPP_DW
#define _HPP_DW
#include <dw.h>
#include <cstring>
#include <string>
#include <vector>
// For alloca() and calloc()
#include <stdlib.h>

// Attempt to support compilers without nullptr type literal
#if __cplusplus >= 201103L 
#define DW_CPP11
#define DW_NULL nullptr
#else
#define DW_NULL NULL
#endif

// Support Lambdas on C++11, Visual C 2015 or GCC 5
#if defined(DW_CPP11) || (defined(_MSC_VER) && _MSC_VER >= 1900) || \
    (defined(__GNUC__) && __GNUC__ > 4)
#define DW_LAMBDA
#include <functional>
#endif

// Attempt to allow compilation on GCC older than 4.7
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#define override
#define final
#endif	

// Attempt to allow compilation on MSVC older than 2012
#if defined(_MSC_VER) && _MSC_VER < 1700
#define final
#endif

namespace DW 
{

#define _DW_THREAD_STACK 10000

// Macro to convert an allocated C string buffer to std::string, free the buffer and return the std::string
#define _dw_string_free_return(a) std::string utf8string = a ? std::string(a) : std::string(); if(a) dw_free(a); return utf8string;

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
    int Destroy() { int retval = dw_window_destroy(hwnd); delete this; return retval; }
    Widget *FromID(int id) { 
        HWND child = dw_window_from_id(hwnd, id);
        if(child) {
            return reinterpret_cast<Widget *>(dw_window_get_data(child, "_dw_classptr"));
        }
        return DW_NULL;
    }
    void GetPreferredSize(int *width, int *height) { dw_window_get_preferred_size(hwnd, width, height); }
    int SetColor(unsigned long fore, unsigned long back) { return dw_window_set_color(hwnd, fore, back); }
    void SetData(const char *dataname, void *data) { dw_window_set_data(hwnd, dataname, data); }
    void SetData(std::string dataname, void *data) { dw_window_set_data(hwnd, dataname.c_str(), data); }
    void *GetData(const char *dataname) { return dw_window_get_data(hwnd, dataname); }
    void *GetData(std::string dataname) { return dw_window_get_data(hwnd, dataname.c_str()); }
    void SetPointer(int cursortype) { dw_window_set_pointer(hwnd, cursortype); }
    void SetStyle(unsigned long flags, unsigned long mask) { dw_window_set_style(hwnd, flags, mask); }
    void SetStyle(unsigned long flags) { dw_window_set_style(hwnd, flags, flags); }
    void SetTooltip(const char *bubbletext) { dw_window_set_tooltip(hwnd, bubbletext); }
    void SetTooltip(std::string bubbletext) { dw_window_set_tooltip(hwnd, bubbletext.c_str()); }
    int Unpack() { return dw_box_unpack(hwnd); }
};

// Box class is a packable object
class Boxes : virtual public Widget
{
public:
    // User functions
    void PackStart(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_start(hwnd, item ? item->GetHWND() : DW_NOHWND, width, height, hsize, vsize, pad); }
    void PackStart(Widget *item, int hsize, int vsize, int pad) { 
        dw_box_pack_start(hwnd, item ? item->GetHWND() : DW_NOHWND, DW_SIZE_AUTO, DW_SIZE_AUTO, hsize, vsize, pad); }
    void PackEnd(Widget *item, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_end(hwnd, item ? item->GetHWND() : DW_NOHWND, width, height, hsize, vsize, pad); }
    void PackEnd(Widget *item, int hsize, int vsize, int pad) { 
        dw_box_pack_end(hwnd, item ? item->GetHWND() : DW_NOHWND, DW_SIZE_AUTO, DW_SIZE_AUTO, hsize, vsize, pad); }
    void PackAtIndex(Widget *item, int index, int width, int height, int hsize, int vsize, int pad) { 
        dw_box_pack_at_index(hwnd, item ? item->GetHWND() : DW_NOHWND, index, width, height, hsize, vsize, pad); }
    void PackAtIndex(Widget *item, int index, int hsize, int vsize, int pad) { 
        dw_box_pack_at_index(hwnd, item ? item->GetHWND() : DW_NOHWND, index, DW_SIZE_AUTO, DW_SIZE_AUTO, hsize, vsize, pad); }
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
#endif
    int (*_ConnectClickedOld)(Clickable *);
    static int _OnClicked(HWND window, void *data) {
        Clickable *classptr = reinterpret_cast<Clickable *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectClicked) 
            return classptr->_ConnectClicked();
#endif
        if(classptr->_ConnectClickedOld)
            return classptr->_ConnectClickedOld(classptr);
        return classptr->OnClicked(); }
protected:
    virtual ~Clickable() {}
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectClicked = 0;
#endif
        _ConnectClickedOld = 0;
        if(IsOverridden(Clickable::OnClicked, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_OnClicked), this);
            ClickedConnected = true;
        }
        else {
            ClickedConnected = false;
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
    {
        _ConnectClicked = userfunc;
        if(!ClickedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CLICKED, DW_SIGNAL_FUNC(_OnClicked), this);
            ClickedConnected = true;
        }
    }
#endif
    void ConnectClicked(int (*userfunc)(Clickable *))
    {
        _ConnectClickedOld = userfunc;
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
    MenuItem *AppendItem(const char *title, unsigned long flags, int check, Menus *submenu);
    MenuItem *AppendItem(const char *title, unsigned long flags, int check);
    MenuItem *AppendItem(const char *title, Menus *submenu);
    MenuItem *AppendItem(const char *title);
    MenuItem *AppendItem(std::string title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu);
    MenuItem *AppendItem(std::string title, unsigned long flags, int check, Menus *submenu);
    MenuItem *AppendItem(std::string title, unsigned long flags, int check);
    MenuItem *AppendItem(std::string title, Menus *submenu);
    MenuItem *AppendItem(std::string title);
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
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, id, flags, end, check, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, const char *title, unsigned long flags, int check, Menus *submenu) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, flags, TRUE, check, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, const char *title, unsigned long flags, int check) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, flags, TRUE, check, DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, const char *title, Menus *submenu) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, 0, TRUE, FALSE, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, const char *title) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title, DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, std::string title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title.c_str(), id, flags, end, check, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, std::string title, unsigned long flags, int check, Menus *submenu) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title.c_str(), DW_MENU_AUTO, flags, TRUE, check, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, std::string title, unsigned long flags, int check) { 
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title.c_str(), DW_MENU_AUTO, flags, TRUE, check, DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, std::string title, Menus *submenu) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title.c_str(), DW_MENU_AUTO, 0, TRUE, FALSE, submenu ? submenu->GetHMENUI() : DW_NOMENU)); Setup();
    }
    MenuItem(Menus *menu, std::string title) {
        SetHWND(dw_menu_append_item(menu->GetHMENUI(), title.c_str(), DW_MENU_AUTO, 0, TRUE, FALSE, DW_NOMENU)); Setup();
    }

    // User functions
    void SetState(unsigned long flags) { dw_window_set_style(hwnd, flags, flags); }
};

MenuItem *Menus::AppendItem(const char *title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu) {
    return new MenuItem(this, title, id, flags, end, check, submenu);
}

MenuItem *Menus::AppendItem(const char *title, unsigned long flags, int check, Menus *submenu) {
    return new MenuItem(this, title, flags, check, submenu);
}

MenuItem *Menus::AppendItem(const char *title, unsigned long flags, int check) {
    return new MenuItem(this, title, flags, check);
}

MenuItem *Menus::AppendItem(const char *title, Menus *submenu) {
    return new MenuItem(this, title, submenu);
}
MenuItem *Menus::AppendItem(const char *title) {
    return new MenuItem(this, title);
}

MenuItem *Menus::AppendItem(std::string title, unsigned long id, unsigned long flags, int end, int check, Menus *submenu) {
    return new MenuItem(this, title.c_str(), id, flags, end, check, submenu);
}

MenuItem *Menus::AppendItem(std::string title, unsigned long flags, int check, Menus *submenu) {
    return new MenuItem(this, title.c_str(), flags, check, submenu);
}

MenuItem *Menus::AppendItem(std::string title, unsigned long flags, int check) {
    return new MenuItem(this, title.c_str(), flags, check);
}

MenuItem *Menus::AppendItem(std::string title, Menus *submenu) {
    return new MenuItem(this, title.c_str(), submenu);
}
MenuItem *Menus::AppendItem(std::string title) {
    return new MenuItem(this, title.c_str());
}


// Top-level window class is packable
class Window : public Boxes
{
private:
    bool DeleteConnected, ConfigureConnected;
#ifdef DW_LAMBDA
    std::function<int()> _ConnectDelete;
    std::function<int(int, int)> _ConnectConfigure;
#endif
    int (*_ConnectDeleteOld)(Window *);
    int (*_ConnectConfigureOld)(Window *, int width, int height);
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectDelete = 0;
        _ConnectConfigure = 0;
#endif
        _ConnectDeleteOld = 0;
        _ConnectConfigureOld = 0;
        menu = DW_NULL;
        if(IsOverridden(Window::OnDelete, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
            DeleteConnected = true;
        } else {
            DeleteConnected = false;
        }
        if(IsOverridden(Window::OnConfigure, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        } else {
            ConfigureConnected = false;
        }
    }
    static int _OnDelete(HWND window, void *data) {
        Window *classptr = reinterpret_cast<Window *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectDelete)
            return classptr->_ConnectDelete();
#endif
        if(classptr->_ConnectDeleteOld)
            return classptr->_ConnectDeleteOld(classptr);
        return classptr->OnDelete(); }
    static int _OnConfigure(HWND window, int width, int height, void *data) { 
        Window *classptr = reinterpret_cast<Window *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectConfigure)
            return classptr->_ConnectConfigure(width, height);
#endif
        if(classptr->_ConnectConfigureOld)
            return classptr->_ConnectConfigureOld(classptr, width, height);
        return classptr->OnConfigure(width, height); }
    MenuBar *menu;
public:
    // Constructors
    Window(HWND owner, const char *title, unsigned long style) { SetHWND(dw_window_new(owner, title, style)); Setup(); }
    Window(const char *title, unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, title, style)); Setup(); }
    Window(HWND owner, std::string title, unsigned long style) { SetHWND(dw_window_new(owner, title.c_str(), style)); Setup(); }
    Window(std::string title, unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, title.c_str(), style)); Setup(); }
    Window(unsigned long style) { SetHWND(dw_window_new(HWND_DESKTOP, "", style)); Setup(); }
    Window(const char *title) { SetHWND(dw_window_new(HWND_DESKTOP, title,  DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); Setup(); }
    Window(std::string title) { SetHWND(dw_window_new(HWND_DESKTOP, title.c_str(),  DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); Setup(); }
    Window() { SetHWND(dw_window_new(HWND_DESKTOP, "", DW_FCF_SYSMENU | DW_FCF_TITLEBAR |
                        DW_FCF_TASKLIST | DW_FCF_SIZEBORDER | DW_FCF_MINMAX)); Setup(); }

    // User functions
    void SetText(const char *text) { dw_window_set_text(hwnd, text); }
    void SetText(std::string text) { dw_window_set_text(hwnd, text.c_str()); }
    char *GetCText() { return dw_window_get_text(hwnd); }
    std::string GetText() { 
        char *retval = dw_window_get_text(hwnd);
        _dw_string_free_return(retval);
    }
    void SetSize(unsigned long width, unsigned long height) { dw_window_set_size(hwnd, width, height); }
    int Show() { return dw_window_show(hwnd); }
    int Hide() { return dw_window_hide(hwnd); }
    void SetGravity(int horz, int vert) { dw_window_set_gravity(hwnd, horz, vert); }
    int Minimize() { return dw_window_minimize(hwnd); }
    int Raise() { return dw_window_raise(hwnd); }
    int Lower() { return dw_window_lower(hwnd); }
    void Redraw() { dw_window_redraw(hwnd); }
    void Default(Widget *defaultitem) { if(defaultitem) dw_window_default(hwnd, defaultitem->GetHWND()); }
    void ClickDefault(Widget *defaultitem) { if(defaultitem) dw_window_click_default(hwnd, defaultitem->GetHWND()); }
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
    {
        _ConnectDelete = userfunc;
        if(!DeleteConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
            DeleteConnected = true;
        } else {
            DeleteConnected = false;
        }
    }
#endif
    void ConnectDelete(int (*userfunc)(Window *)) 
    {
        _ConnectDeleteOld = userfunc;
        if(!DeleteConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_DELETE, DW_SIGNAL_FUNC(_OnDelete), this);
            DeleteConnected = true;
        } else {
            DeleteConnected = false;
        }
    }
#ifdef DW_LAMBDA
    void ConnectConfigure(std::function<int(int, int)> userfunc)
    {
        _ConnectConfigure = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        } else {
            ConfigureConnected = false;
        }
    }    
#endif
    void ConnectConfigure(int (*userfunc)(Window *, int, int))
    {
        _ConnectConfigureOld = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        } else {
            ConfigureConnected = false;
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
    void SetText(std::string text) { dw_window_set_text(hwnd, text.c_str()); }
    char *GetCText() { return dw_window_get_text(hwnd); }
    std::string GetText() { 
        char *retval = dw_window_get_text(hwnd);
        _dw_string_free_return(retval);
    }
};

class Button : public TextButton
{
public:
    // Constructors
    Button(const char *text, unsigned long id) { SetHWND(dw_button_new(text, id)); Setup(); }
    Button(std::string text, unsigned long id) { SetHWND(dw_button_new(text.c_str(), id)); Setup(); }
    Button(unsigned long id) { SetHWND(dw_button_new("", id)); Setup(); }
    Button(const char *text) { SetHWND(dw_button_new(text, 0)); Setup(); }
    Button(std::string text) { SetHWND(dw_button_new(text.c_str(), 0)); Setup(); }
    Button() { SetHWND(dw_button_new("", 0)); Setup(); }
};

class BitmapWidget : virtual public Widget
{
public:
    // User functions
    void Set(unsigned long id) { dw_window_set_bitmap(hwnd, id, DW_NULL); }
    void Set(const char *file) { dw_window_set_bitmap(hwnd, 0, file); }
    void Set(std::string file) { dw_window_set_bitmap(hwnd, 0, file.c_str()); }
    void Set(const char *data, int len) { dw_window_set_bitmap_from_data(hwnd, 0, data, len); }
};

// Image based button
class BitmapButton : public Clickable, public Focusable, public BitmapWidget
{
public:
    // Constructors
    BitmapButton(const char *text, unsigned long id) { SetHWND(dw_bitmapbutton_new(text, id)); Setup(); }
    BitmapButton(unsigned long id) { SetHWND(dw_bitmapbutton_new("", id)); Setup(); }
    BitmapButton(const char *text, unsigned long id, const char *file) { SetHWND(dw_bitmapbutton_new_from_file(text, id, file)); Setup(); }
    BitmapButton(const char *text, const char *file) { SetHWND(dw_bitmapbutton_new_from_file(text, 0, file)); Setup(); }
    BitmapButton(const char *text, unsigned long id, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text, id, data, len)); Setup(); }
    BitmapButton(const char *text, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text, 0, data, len)); Setup(); }
    BitmapButton(std::string text, unsigned long id) { SetHWND(dw_bitmapbutton_new(text.c_str(), id)); Setup(); }
    BitmapButton(std::string text, unsigned long id, std::string file) { SetHWND(dw_bitmapbutton_new_from_file(text.c_str(), id, file.c_str())); Setup(); }
    BitmapButton(std::string text, std::string file) { SetHWND(dw_bitmapbutton_new_from_file(text.c_str(), 0, file.c_str())); Setup(); }
    BitmapButton(std::string text, unsigned long id, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text.c_str(), id, data, len)); Setup(); }
    BitmapButton(std::string text, const char *data, int len) { SetHWND(dw_bitmapbutton_new_from_data(text.c_str(), 0, data, len)); Setup(); }
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
    CheckBox(std::string text, unsigned long id) { SetHWND(dw_checkbox_new(text.c_str(), id)); Setup(); }
    CheckBox(unsigned long id) { SetHWND(dw_checkbox_new("", id)); Setup(); }
    CheckBox(const char *text) { SetHWND(dw_checkbox_new(text, 0)); Setup(); }
    CheckBox(std::string text) { SetHWND(dw_checkbox_new(text.c_str(), 0)); Setup(); }
    CheckBox() { SetHWND(dw_checkbox_new("", 0)); Setup(); }
};

class RadioButton : public CheckBoxes
{
public:
    // Constructors
    RadioButton(const char *text, unsigned long id) { SetHWND(dw_radiobutton_new(text, id)); Setup(); }
    RadioButton(std::string text, unsigned long id) { SetHWND(dw_radiobutton_new(text.c_str(), id)); Setup(); }
    RadioButton(unsigned long id) { SetHWND(dw_radiobutton_new("", id)); Setup(); }
    RadioButton(const char *text) { SetHWND(dw_radiobutton_new(text, 0)); Setup(); }
    RadioButton(std::string text) { SetHWND(dw_radiobutton_new(text.c_str(), 0)); Setup(); }
    RadioButton() { SetHWND(dw_radiobutton_new("", 0)); Setup(); }
};

// Class for handling static text widget
class TextWidget : virtual public Widget
{
public:
    // User functions
    void SetText(const char *text) { dw_window_set_text(hwnd, text); }
    void SetText(std::string text) { dw_window_set_text(hwnd, text.c_str()); }
    char *GetCText() { return dw_window_get_text(hwnd); }
    std::string GetText() {
        char *retval = dw_window_get_text(hwnd);
        _dw_string_free_return(retval);
    }
    int SetFont(const char *font) { return dw_window_set_font(hwnd, font); }
    char *GetCFont() { return dw_window_get_font(hwnd); }
    std::string GetFont() {
        char *retval = dw_window_get_font(hwnd);
        _dw_string_free_return(retval);
    }
};

// Class for handling static text widget
class Text : public TextWidget
{
public:
    // Constructors
    Text(const char *text, unsigned long id) { SetHWND(dw_text_new(text, id)); }
    Text(const char *text) { SetHWND(dw_text_new(text, 0)); }
    Text(std::string text, unsigned long id) { SetHWND(dw_text_new(text.c_str(), id)); }
    Text(std::string text) { SetHWND(dw_text_new(text.c_str(), 0)); }
    Text(unsigned long id) { SetHWND(dw_text_new("", id)); }
    Text() { SetHWND(dw_text_new("", 0)); }
};

class StatusText : public TextWidget
{
public:
    // Constructors
    StatusText(const char *text, unsigned long id) { SetHWND(dw_status_text_new(text, id)); }
    StatusText(const char *text) { SetHWND(dw_status_text_new(text, 0)); }
    StatusText(std::string text, unsigned long id) { SetHWND(dw_status_text_new(text.c_str(), id)); }
    StatusText(std::string text) { SetHWND(dw_status_text_new(text.c_str(), 0)); }
    StatusText(unsigned long id) { SetHWND(dw_status_text_new("", id)); }
    StatusText() { SetHWND(dw_status_text_new("", 0)); }
};

// Class for handing static image widget
class Bitmap : public BitmapWidget
{
public:
    // Constructors
    Bitmap(const char *data, int len) { SetHWND(dw_bitmap_new(0)); dw_window_set_bitmap_from_data(hwnd, 0, data, len); }
    Bitmap(const char *file) { SetHWND(dw_bitmap_new(0)); dw_window_set_bitmap(hwnd, 0, file); }
    Bitmap(std::string file) { SetHWND(dw_bitmap_new(0)); dw_window_set_bitmap(hwnd, 0, file.c_str()); }
    Bitmap(unsigned long id) { SetHWND(dw_bitmap_new(id)); }
    Bitmap() { SetHWND(dw_bitmap_new(0)); }
};

// Class for handing calendar widget
class Calendar : public Widget
{
public:
    // Constructors
    Calendar(unsigned long id) { SetHWND(dw_calendar_new(id)); }
    Calendar() { SetHWND(dw_calendar_new(0)); }

    // User functions
    void GetDate(unsigned int *year, unsigned int *month, unsigned int *day) { dw_calendar_get_date(hwnd, year, month, day); }
    void SetDate(unsigned int year, unsigned int month, unsigned int day) { dw_calendar_set_date(hwnd, year, month, day); }
};


// Abstract class that defines drawing, either to screen or picture (pixmap)
class Drawable
{
public:
    virtual ~Drawable() { }
    virtual void DrawPoint(int x, int y) = 0;
    virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;
    virtual void DrawPolygon(int flags, int npoints, int x[], int y[]) = 0;
    virtual void DrawRect(int fill, int x, int y, int width, int height) = 0;
    virtual void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2) = 0;
    virtual void DrawText(int x, int y, const char *text) = 0;
    virtual void DrawText(int x, int y, std::string text) = 0;
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
    std::function<int(char c, int, int, std::string)> _ConnectKeyPress;
    std::function<int(int, int, int)> _ConnectButtonPress;
    std::function<int(int, int, int)> _ConnectButtonRelease;
    std::function<int(int, int, int)> _ConnectMotionNotify;
#endif
    int (*_ConnectExposeOld)(Render *, DWExpose *);
    int (*_ConnectConfigureOld)(Render *, int width, int height);
    int (*_ConnectCKeyPressOld)(Render *, char c, int vk, int state, char *utf8);
    int (*_ConnectKeyPressOld)(Render *, char c, int vk, int state, std::string utf8);
    int (*_ConnectButtonPressOld)(Render *, int x, int y, int buttonmask);
    int (*_ConnectButtonReleaseOld)(Render *, int x, int y, int buttonmask);
    int (*_ConnectMotionNotifyOld)(Render *, int x, int y, int buttonmask);
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectExpose = 0;
        _ConnectConfigure = 0;
        _ConnectKeyPress = 0;
        _ConnectButtonPress = 0;
        _ConnectButtonRelease = 0;
        _ConnectMotionNotify = 0;
#endif
        _ConnectExposeOld = 0;
        _ConnectConfigureOld = 0;
        _ConnectCKeyPressOld = 0;
        _ConnectKeyPressOld = 0;
        _ConnectButtonPressOld = 0;
        _ConnectButtonReleaseOld = 0;
        _ConnectMotionNotifyOld = 0;
        if(IsOverridden(Render::OnExpose, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
            ExposeConnected = true;
        } else {
            ExposeConnected = false;
        }
        if(IsOverridden(Render::OnConfigure, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        } else {
            ConfigureConnected = false;
        }
        if(IsOverridden(Render::OnKeyPress, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        } else {
            KeyPressConnected = false;
        }
        if(IsOverridden(Render::OnButtonPress, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_OnButtonPress), this);
            ButtonPressConnected = true;
        } else {
            ButtonPressConnected = false;
        }
        if(IsOverridden(Render::OnButtonRelease, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_RELEASE, DW_SIGNAL_FUNC(_OnButtonRelease), this);
            ButtonReleaseConnected = true;
        } else {
            ButtonReleaseConnected = false;
        }
        if(IsOverridden(Render::OnMotionNotify, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_MOTION_NOTIFY, DW_SIGNAL_FUNC(_OnMotionNotify), this);
            MotionNotifyConnected = true;
        } else {
            MotionNotifyConnected = false;
        }
    }
    static int _OnExpose(HWND window, DWExpose *exp, void *data) {
        Render *classptr = reinterpret_cast<Render *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectExpose)
            return classptr->_ConnectExpose(exp);
#endif
        if(classptr->_ConnectExposeOld)
            return classptr->_ConnectExposeOld(classptr, exp);
        return classptr->OnExpose(exp); }
    static int _OnConfigure(HWND window, int width, int height, void *data) {
        Render *classptr = reinterpret_cast<Render *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectConfigure)
            return classptr->_ConnectConfigure(width, height);
#endif
        if(classptr->_ConnectConfigureOld)
            return classptr->_ConnectConfigureOld(classptr, width, height);
        return classptr->OnConfigure(width, height); }
    static int _OnKeyPress(HWND window, char c, int vk, int state, void *data, char *utf8) {
        Render *classptr = reinterpret_cast<Render *>(data);
        std::string utf8string = utf8 ? std::string(utf8) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectKeyPress)
            return classptr->_ConnectKeyPress(c, vk, state, utf8string);
#endif
        if(classptr->_ConnectCKeyPressOld)
            return classptr->_ConnectCKeyPressOld(classptr, c, vk, state, utf8);
        else if(classptr->_ConnectKeyPressOld)
            return classptr->_ConnectKeyPressOld(classptr, c, vk, state, utf8string);
        return classptr->OnKeyPress(c, vk, state, utf8string); }
    static int _OnButtonPress(HWND window, int x, int y, int buttonmask, void *data) {
        Render *classptr = reinterpret_cast<Render *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectButtonPress)
            return classptr->_ConnectButtonPress(x, y, buttonmask);
#endif
        if(classptr->_ConnectButtonPressOld)
            return classptr->_ConnectButtonPressOld(classptr, x, y, buttonmask);
        return classptr->OnButtonPress(x, y, buttonmask); }
    static int _OnButtonRelease(HWND window, int x, int y, int buttonmask, void *data) {
        Render *classptr = reinterpret_cast<Render *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectButtonRelease)
            return classptr->_ConnectButtonRelease(x, y, buttonmask);
#endif
        if(classptr->_ConnectButtonReleaseOld)
            return classptr->_ConnectButtonReleaseOld(classptr, x, y, buttonmask);
        return classptr->OnButtonRelease(x, y, buttonmask); }
    static int _OnMotionNotify(HWND window, int x, int y, int  buttonmask, void *data) {
        Render *classptr = reinterpret_cast<Render *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectMotionNotify)
            return classptr->_ConnectMotionNotify(x, y, buttonmask);
#endif
        if(classptr->_ConnectMotionNotifyOld)
            return classptr->_ConnectMotionNotifyOld(classptr, x, y, buttonmask);
        return classptr->OnMotionNotify(x, y, buttonmask); }
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
    void DrawText(int x, int y, std::string text) { dw_draw_text(hwnd, DW_NULL, x, y, text.c_str()); }
    int BitBltStretch(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc, int srcwidth, int srcheight) {
        return dw_pixmap_stretch_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc, srcwidth, srcheight);
    }
    int BitBltStretch(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc, int srcwidth, int srcheight);
    void BitBlt(int xdest, int ydest, int width, int height, Render *src, int xsrc, int ysrc) {
        return dw_pixmap_bitblt(hwnd, DW_NULL, xdest, ydest, width, height, src ? src->GetHWND() : DW_NOHWND, DW_NULL, xsrc, ysrc);
    }
    void BitBlt(int xdest, int ydest, int width, int height, Pixmap *src, int xsrc, int ysrc);
    int SetFont(const char *fontname) { return dw_window_set_font(hwnd, fontname); }
    int SetFont(std::string fontname) { return dw_window_set_font(hwnd, fontname.c_str()); }
    void GetTextExtents(const char *text, int *width, int *height) { dw_font_text_extents_get(hwnd, DW_NULL, text, width, height); }
    void GetTextExtents(std::string text, int *width, int *height) { dw_font_text_extents_get(hwnd, DW_NULL, text.c_str(), width, height); }
    char *GetCFont() { return dw_window_get_font(hwnd); }
    std::string GetFont() {
        char *retval = dw_window_get_font(hwnd);
        _dw_string_free_return(retval);
    }
    void Redraw() { dw_render_redraw(hwnd); }
    void Flush() { dw_flush(); }
#ifdef DW_LAMBDA
    void ConnectExpose(std::function<int(DWExpose *)> userfunc)
    {
        _ConnectExpose = userfunc;
        if(!ExposeConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
            ExposeConnected = true;
        }
    }
#endif
    void ConnectExpose(int (*userfunc)(Render *, DWExpose *))
    {
        _ConnectExposeOld = userfunc;
        if(!ExposeConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_EXPOSE, DW_SIGNAL_FUNC(_OnExpose), this);
            ExposeConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectConfigure(std::function<int(int, int)> userfunc)
    {
        _ConnectConfigure = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
    }
#endif
    void ConnectConfigure(int (*userfunc)(Render *, int, int))
    {
        _ConnectConfigureOld = userfunc;
        if(!ConfigureConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_CONFIGURE, DW_SIGNAL_FUNC(_OnConfigure), this);
            ConfigureConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectKeyPress(std::function<int(char, int, int, std::string)> userfunc)
    {
        _ConnectKeyPress = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        }
    }
#endif
    void ConnectKeyPress(int (*userfunc)(Render *, char, int, int, char *))
    {
        _ConnectCKeyPressOld = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        }
    }
    void ConnectKeyPress(int (*userfunc)(Render *, char, int, int, std::string))
    {
        _ConnectKeyPressOld = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_KEY_PRESS, DW_SIGNAL_FUNC(_OnKeyPress), this);
            KeyPressConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectButtonPress(std::function<int(int, int, int)> userfunc)
    {
        _ConnectButtonPress = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_OnButtonPress), this);
            ButtonPressConnected = true;
        }
    }
#endif
    void ConnectButtonPress(int (*userfunc)(Render *, int, int, int))
    {
        _ConnectButtonPressOld = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_PRESS, DW_SIGNAL_FUNC(_OnButtonPress), this);
            ButtonPressConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectButtonRelease(std::function<int(int, int, int)> userfunc)
    {
        _ConnectButtonRelease = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_RELEASE, DW_SIGNAL_FUNC(_OnButtonRelease), this);
            ButtonReleaseConnected = true;
        }
    }
#endif
    void ConnectButtonRelease(int (*userfunc)(Render *, int, int, int))
    {
        _ConnectButtonReleaseOld = userfunc;
        if(!KeyPressConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_BUTTON_RELEASE, DW_SIGNAL_FUNC(_OnButtonRelease), this);
            ButtonReleaseConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectMotionNotify(std::function<int(int, int, int)> userfunc)
    {
        _ConnectMotionNotify = userfunc;
        if(!MotionNotifyConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_MOTION_NOTIFY, DW_SIGNAL_FUNC(_OnMotionNotify), this);
            MotionNotifyConnected = true;
        }
    }
#endif
    void ConnectMotionNotify(int (*userfunc)(Render *, int, int, int))
    {
        _ConnectMotionNotifyOld = userfunc;
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
    virtual int OnKeyPress(char c, int vk, int state, std::string utf8) {
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

class Pixmap final : public Drawable, public Handle
{
private:
    void SetHPIXMAP(HPIXMAP newpixmap) { 
        hpixmap = newpixmap; 
        SetHandle(reinterpret_cast<void *>(newpixmap));
    }
    HPIXMAP hpixmap; 
    unsigned long pwidth, pheight;
    bool hpmprot;
public:
    // Constructors
    Pixmap(Render *window, unsigned long width, unsigned long height, int depth) { 
        SetHPIXMAP(dw_pixmap_new(window ? window->GetHWND() : DW_NOHWND, width, height, depth)); 
        pwidth = width; pheight = height; 
        hpmprot = false;
    }
    Pixmap(Render *window, unsigned long width, unsigned long height) {
        SetHPIXMAP(dw_pixmap_new(window ? window->GetHWND() : DW_NOHWND, width, height, 32)); 
        pwidth = width; pheight = height;
        hpmprot = false;
    }
    Pixmap(Render *window, const char *filename) { 
        SetHPIXMAP(dw_pixmap_new_from_file(window ? window->GetHWND() : DW_NOHWND, filename));
        pwidth = dw_pixmap_get_width(hpixmap);
        pheight = dw_pixmap_get_height(hpixmap);
        hpmprot = false;
    }
    Pixmap(Render *window, std::string filename) { 
        SetHPIXMAP(dw_pixmap_new_from_file(window ? window->GetHWND() : DW_NOHWND, filename.c_str()));
        pwidth = dw_pixmap_get_width(hpixmap);
        pheight = dw_pixmap_get_height(hpixmap);
        hpmprot = false;
    }
    Pixmap(HPIXMAP hpm) { 
        SetHPIXMAP(hpm);
        pwidth = dw_pixmap_get_width(hpixmap);
        pheight = dw_pixmap_get_height(hpixmap);
        hpmprot = true;
    }
    // Destructor
    ~Pixmap() { if(hpmprot == false) dw_pixmap_destroy(hpixmap); hpixmap = 0; }

    // User functions
    HPIXMAP GetHPIXMAP() { return hpixmap; }
    void DrawPoint(int x, int y) { dw_draw_point(DW_NOHWND, hpixmap, x, y); }
    void DrawLine(int x1, int y1, int x2, int y2) { dw_draw_line(DW_NOHWND, hpixmap, x1, y1, x2, y2); }
    void DrawPolygon(int flags, int npoints, int x[], int y[]) { dw_draw_polygon(DW_NOHWND, hpixmap, flags, npoints, x, y); }
    void DrawRect(int fill, int x, int y, int width, int height) { dw_draw_rect(DW_NOHWND, hpixmap, fill, x, y, width, height); }
    void DrawArc(int flags, int xorigin, int yorigin, int x1, int y1, int x2, int y2) { dw_draw_arc(DW_NOHWND, hpixmap, flags, xorigin, yorigin, x1, y1, x2, y2); }
    void DrawText(int x, int y, const char *text) { dw_draw_text(DW_NOHWND, hpixmap, x, y, text); }
    void DrawText(int x, int y, std::string text) { dw_draw_text(DW_NOHWND, hpixmap, x, y, text.c_str()); }
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
    int SetFont(std::string fontname) { return dw_pixmap_set_font(hpixmap, fontname.c_str()); }
    void GetTextExtents(const char *text, int *width, int *height) { dw_font_text_extents_get(DW_NOHWND, hpixmap, text, width, height); }
    void GetTextExtents(std::string text, int *width, int *height) { dw_font_text_extents_get(DW_NOHWND, hpixmap, text.c_str(), width, height); }
    void SetTransparentColor(unsigned long color) { dw_pixmap_set_transparent_color(hpixmap, color); }
    unsigned long GetWidth() { return pwidth; }
    unsigned long GetHeight() { return pheight; }
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
    bool ChangedConnected, ResultConnected, MessageConnected;
#ifdef DW_LAMBDA
    std::function<int(int, std::string)> _ConnectChanged;
    std::function<int(int, std::string, void *)> _ConnectResult;
    std::function<int(std::string, std::string)> _ConnectMessage;
#endif
    int (*_ConnectCChangedOld)(HTML *, int status, char *url);
    int (*_ConnectCResultOld)(HTML *, int status, char *result, void *scriptdata);
    int (*_ConnectCMessageOld)(HTML *, char *name, char *message);
    int (*_ConnectChangedOld)(HTML *, int status, std::string url);
    int (*_ConnectResultOld)(HTML *, int status, std::string result, void *scriptdata);
    int (*_ConnectMessageOld)(HTML *, std::string, std::string message);
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectChanged = 0;
        _ConnectResult = 0;
        _ConnectMessage = 0;
#endif
        _ConnectCChangedOld = 0;
        _ConnectCResultOld = 0;
        _ConnectCMessageOld = 0;
        _ConnectChangedOld = 0;
        _ConnectResultOld = 0;
        _ConnectMessageOld = 0;
        if(IsOverridden(HTML::OnChanged, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        } else {
            ChangedConnected = false;
        }
        if(IsOverridden(HTML::OnResult, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_RESULT, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        } else {
            ResultConnected = false;
        }
        if(IsOverridden(HTML::OnMessage, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_MESSAGE, DW_SIGNAL_FUNC(_OnMessage), this);
            MessageConnected = true;
        } else {
            MessageConnected = false;
        }
    }
    static int _OnChanged(HWND window, int status, char *url, void *data) {
        HTML *classptr = reinterpret_cast<HTML *>(data);
        std::string utf8string = url ? std::string(url) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectChanged)
            return classptr->_ConnectChanged(status, utf8string);
#endif
        if(classptr->_ConnectCChangedOld)
            return classptr->_ConnectCChangedOld(classptr, status, url);
        else if(classptr->_ConnectChangedOld)
            return classptr->_ConnectChangedOld(classptr, status, utf8string);
        return classptr->OnChanged(status, utf8string); }
    static int _OnResult(HWND window, int status, char *result, void *scriptdata, void *data) {
        HTML *classptr = reinterpret_cast<HTML *>(data);
        std::string utf8string = result ? std::string(result) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectResult)
            return classptr->_ConnectResult(status, utf8string, scriptdata);
#endif
        if(classptr->_ConnectCResultOld)
            return classptr->_ConnectCResultOld(classptr, status, result, scriptdata);
        else if(classptr->_ConnectResultOld)
            return classptr->_ConnectResultOld(classptr, status, utf8string, scriptdata);
        return classptr->OnResult(status, utf8string, scriptdata); }
        static int _OnMessage(HWND window, char *name, char *message, void *data) {
            HTML *classptr = reinterpret_cast<HTML *>(data);
            std::string utf8name = name ? std::string(name) : std::string();
            std::string utf8message = message ? std::string(message) : std::string();
    #ifdef DW_LAMBDA
            if(classptr->_ConnectMessage)
                return classptr->_ConnectMessage(utf8name, utf8message);
    #endif
            if(classptr->_ConnectCMessageOld)
                return classptr->_ConnectCMessageOld(classptr, name, message);
            else if(classptr->_ConnectMessageOld)
                return classptr->_ConnectMessageOld(classptr, utf8name, utf8message);
            return classptr->OnMessage(utf8name, utf8message); }
public:
    // Constructors
    HTML(unsigned long id) { SetHWND(dw_html_new(id)); Setup(); }
    HTML() { SetHWND(dw_html_new(0)); Setup(); }

    // User functions
    void Action(int action) { dw_html_action(hwnd, action); }
    int JavascriptRun(const char *script, void *scriptdata) { return dw_html_javascript_run(hwnd, script, scriptdata); }
    int JavascriptRun(const char *script) { return dw_html_javascript_run(hwnd, script, NULL); }
    int JavascriptAdd(const char *name) { return dw_html_javascript_add(hwnd, name); }
    int Raw(const char *buffer) { return dw_html_raw(hwnd, buffer); }
    int URL(const char *url) { return dw_html_url(hwnd, url); }
    int JavascriptRun(std::string script, void *scriptdata) { return dw_html_javascript_run(hwnd, script.c_str(), scriptdata); }
    int JavascriptRun(std::string script) { return dw_html_javascript_run(hwnd, script.c_str(), NULL); }
    int JavascriptAdd(std::string name) { return dw_html_javascript_add(hwnd, name.c_str()); }
    int Raw(std::string buffer) { return dw_html_raw(hwnd, buffer.c_str()); }
    int URL(std::string url) { return dw_html_url(hwnd, url.c_str()); }
#ifdef DW_LAMBDA
    void ConnectChanged(std::function<int(int, std::string)> userfunc)
    { 
        _ConnectChanged = userfunc;
        if(!ChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        }
    }
#endif
    void ConnectChanged(int (*userfunc)(HTML *, int, char *))
    { 
        _ConnectCChangedOld = userfunc;
        if(!ChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        }
    }
    void ConnectChanged(int (*userfunc)(HTML *, int, std::string))
    { 
        _ConnectChangedOld = userfunc;
        if(!ChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_CHANGED, DW_SIGNAL_FUNC(_OnChanged), this);
            ChangedConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectResult(std::function<int(int, std::string, void *)> userfunc)
    {
        _ConnectResult = userfunc;
        if(!ResultConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_RESULT, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        }
    }    
#endif
    void ConnectResult(int (*userfunc)(HTML *, int, char *, void *))
    {
        _ConnectCResultOld = userfunc;
        if(!ResultConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_RESULT, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        }
    }    
    void ConnectResult(int (*userfunc)(HTML *, int, std::string, void *))
    {
        _ConnectResultOld = userfunc;
        if(!ResultConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_RESULT, DW_SIGNAL_FUNC(_OnResult), this);
            ResultConnected = true;
        }
    }    
#ifdef DW_LAMBDA
    void ConnectMessage(std::function<int(std::string, std::string)> userfunc)
    {
        _ConnectMessage = userfunc;
        if(!MessageConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_MESSAGE, DW_SIGNAL_FUNC(_OnMessage), this);
            MessageConnected = true;
        }
    }    
#endif
    void ConnectMessage(int (*userfunc)(HTML *, char *, char *))
    {
        _ConnectCMessageOld = userfunc;
        if(!MessageConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_MESSAGE, DW_SIGNAL_FUNC(_OnMessage), this);
            MessageConnected = true;
        }
    }    
    void ConnectMessage(int (*userfunc)(HTML *, std::string, std::string))
    {
        _ConnectMessageOld = userfunc;
        if(!MessageConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_HTML_MESSAGE, DW_SIGNAL_FUNC(_OnMessage), this);
            MessageConnected = true;
        }
    }    
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnChanged(int status, std::string url) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_CHANGED); 
        ChangedConnected = false;
        return FALSE;
    }
    virtual int OnResult(int status, std::string result, void *scriptdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_RESULT);
        ResultConnected = false;
        return FALSE;
    };
    virtual int OnMessage(std::string name, std::string message) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_HTML_MESSAGE);
        MessageConnected = false;
        return FALSE;
    };
};

// Base class for several widgets that allow text entry
class TextEntry : virtual public Focusable, virtual public TextWidget
{
public:
    // User functions
    void ClickDefault(Focusable *next) { if(next) dw_window_click_default(hwnd, next->GetHWND()); }
    void SetLimit(int limit) { dw_entryfield_set_limit(hwnd, limit);}
};

class Entryfield : public TextEntry
{
public:
    // Constructors
    Entryfield(const char *text, unsigned long id) { SetHWND(dw_entryfield_new(text, id)); }
    Entryfield(std::string text, unsigned long id) { SetHWND(dw_entryfield_new(text.c_str(), id)); }
    Entryfield(unsigned long id) { SetHWND(dw_entryfield_new("", id)); }
    Entryfield(const char *text) { SetHWND(dw_entryfield_new(text, 0)); }
    Entryfield(std::string text) { SetHWND(dw_entryfield_new(text.c_str(), 0)); }
    Entryfield() { SetHWND(dw_entryfield_new("", 0)); }
};

class EntryfieldPassword : public TextEntry
{
public:
    // Constructors
    EntryfieldPassword(const char *text, unsigned long id) { SetHWND(dw_entryfield_password_new(text, id)); }
    EntryfieldPassword(std::string text, unsigned long id) { SetHWND(dw_entryfield_password_new(text.c_str(), id)); }
    EntryfieldPassword(unsigned long id) { SetHWND(dw_entryfield_password_new("", id)); }
    EntryfieldPassword(const char *text) { SetHWND(dw_entryfield_password_new(text, 0)); }
    EntryfieldPassword(std::string text) { SetHWND(dw_entryfield_password_new(text.c_str(), 0)); }
    EntryfieldPassword() { SetHWND(dw_entryfield_password_new("", 0)); }
};

// Base class for several widgets that have a list of elements
class ListBoxes : virtual public Focusable
{
private:
    bool ListSelectConnected;
#ifdef DW_LAMBDA
    std::function<int(unsigned int)> _ConnectListSelect;
#endif
    int (*_ConnectListSelectOld)(ListBoxes *, unsigned int index);
    static int _OnListSelect(HWND window, int index, void *data) {
        ListBoxes *classptr = reinterpret_cast<ListBoxes *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectListSelect)
            return classptr->_ConnectListSelect((unsigned int)index);
#endif
        if(classptr->_ConnectListSelectOld)
            return classptr->_ConnectListSelectOld(classptr, (unsigned int)index);
        return classptr->OnListSelect((unsigned int)index);
    }
public:
    // User functions
    void Append(const char *text) { dw_listbox_append(hwnd, text); }
    void Append(std::string text) { dw_listbox_append(hwnd, text.c_str()); }
    void Clear() { dw_listbox_clear(hwnd); }
    int Count() { return dw_listbox_count(hwnd); }
    void Delete(int index) { dw_listbox_delete(hwnd, index); }
    void GetListText(unsigned int index, char *buffer, unsigned int length) { dw_listbox_get_text(hwnd, index, buffer, length); }
    std::string GetListText(unsigned int index) {
        int length = 1025;
        char *buffer = (char *)alloca(length);

        if(buffer) {
            memset(buffer, 0, length);
            dw_listbox_get_text(hwnd, index, buffer, length);
            return std::string(buffer);
        }
        return std::string();
    }
    void SetListText(unsigned int index, char *buffer) { dw_listbox_set_text(hwnd, index, buffer); }
    void SetListText(unsigned int index, std::string buffer) { dw_listbox_set_text(hwnd, index, buffer.c_str()); }
    void Insert(const char *text, int pos) { dw_listbox_insert(hwnd, text, pos); }
    void Insert(std::string text, int pos) { dw_listbox_insert(hwnd, text.c_str(), pos); }
    void ListAppend(char **text, int count) { dw_listbox_list_append(hwnd, text, count); }
    void ListAppend(std::vector<std::string> text) {
        int count = (int)text.size();
        const char **ctext = (const char **)alloca(sizeof(char *) * count);

        if(count > 0 && ctext) {
            for(int z=0; z<count; z++) {
                ctext[z] = text[z].c_str();
            }
            dw_listbox_list_append(hwnd, (char **)ctext, count);
        }
    }
    void Select(int index, int state) { dw_listbox_select(hwnd, index, state); }
    int Selected() { return dw_listbox_selected(hwnd); }
    int Selected(int where) { return dw_listbox_selected_multi(hwnd, where); }
    void SetTop(int top) { dw_listbox_set_top(hwnd, top); }
#ifdef DW_LAMBDA
    void ConnectListSelect(std::function<int(unsigned int)> userfunc)
    {
        _ConnectListSelect = userfunc;
        if(!ListSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
            ListSelectConnected = true;
        }
    }    
#endif
    void ConnectListSelect(int (*userfunc)(ListBoxes *, unsigned int))
    {
        _ConnectListSelectOld = userfunc;
        if(!ListSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
            ListSelectConnected = true;
        }
    }    
protected:
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectListSelect = 0;
#endif
        _ConnectListSelectOld = 0;
        if(IsOverridden(ListBoxes::OnListSelect, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_LIST_SELECT, DW_SIGNAL_FUNC(_OnListSelect), this);
            ListSelectConnected = true;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnListSelect(unsigned int index) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_LIST_SELECT);
        ListSelectConnected = false;
        return FALSE;
    }
};

class ComboBox : public TextEntry, public ListBoxes
{
public:
    // Constructors
    ComboBox(const char *text, unsigned long id) { SetHWND(dw_combobox_new(text, id)); Setup(); }
    ComboBox(std::string text, unsigned long id) { SetHWND(dw_combobox_new(text.c_str(), id)); Setup(); }
    ComboBox(unsigned long id) { SetHWND(dw_combobox_new("", id)); Setup(); }
    ComboBox(const char *text) { SetHWND(dw_combobox_new(text, 0)); Setup(); }
    ComboBox(std::string text) { SetHWND(dw_combobox_new(text.c_str(), 0)); Setup(); }
    ComboBox() { SetHWND(dw_combobox_new("", 0)); Setup(); }
};

class ListBox : public ListBoxes
{
public:
    // Constructors
    ListBox(unsigned long id, int multi) { SetHWND(dw_listbox_new(id, multi)); Setup(); }
    ListBox(unsigned long id) { SetHWND(dw_listbox_new(id, FALSE)); Setup(); }
    ListBox(int multi) { SetHWND(dw_listbox_new(0, multi)); Setup(); }
    ListBox() { SetHWND(dw_listbox_new(0, FALSE)); Setup(); }
};

// Base class for several ranged type widgets
class Ranged : virtual public Widget
{
private:
    bool ValueChangedConnected;
#ifdef DW_LAMBDA
    std::function<int(int)> _ConnectValueChanged;
#endif
    int (*_ConnectValueChangedOld)(Ranged *, int value);
    static int _OnValueChanged(HWND window, int value, void *data) {
        Ranged *classptr = reinterpret_cast<Ranged *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectValueChanged)
            return classptr->_ConnectValueChanged(value);
#endif
        if(classptr->_ConnectValueChangedOld)
            return classptr->_ConnectValueChangedOld(classptr, value);
        return classptr->OnValueChanged(value);
    }
protected:
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectValueChanged = 0;
#endif
        _ConnectValueChangedOld = 0;
        if(IsOverridden(Ranged::OnValueChanged, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(_OnValueChanged), this);
            ValueChangedConnected = true;
        } else {
            ValueChangedConnected = false;
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
    {
        _ConnectValueChanged = userfunc;
        if(!ValueChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(_OnValueChanged), this);
            ValueChangedConnected = true;
        }
    }    
#endif
    void ConnectValueChanged(int (*userfunc)(Ranged *, int))
    {
        _ConnectValueChangedOld = userfunc;
        if(!ValueChangedConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_VALUE_CHANGED, DW_SIGNAL_FUNC(_OnValueChanged), this);
            ValueChangedConnected = true;
        }
    }    
};

class Percent : public Widget
{
public:
    // Constructors
    Percent(unsigned long id) { SetHWND(dw_percent_new(id)); }
    Percent() { SetHWND(dw_percent_new(0)); }

    // User functions
    void SetPos(unsigned int position) { dw_percent_set_pos(hwnd, position); }
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

class ScrollBar : public Ranged
{
public:
    // Constructors
    ScrollBar(int orient, unsigned long id) { SetHWND(dw_scrollbar_new(orient, id)); Setup(); }
    ScrollBar(int orient) { SetHWND(dw_scrollbar_new(orient, 0)); Setup(); }

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
    SpinButton(std::string text, unsigned long id) { SetHWND(dw_spinbutton_new(text.c_str(), id)); Setup(); }
    SpinButton(unsigned long id) { SetHWND(dw_spinbutton_new("", id)); Setup(); }
    SpinButton(const char *text) { SetHWND(dw_spinbutton_new(text, 0)); Setup(); }
    SpinButton(std::string text) { SetHWND(dw_spinbutton_new(text.c_str(), 0)); Setup(); }
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
    std::string Export(int startpoint, int length) {
        char *buffer = (char *)alloca(length + 1);

        if(buffer) {
            memset(buffer, 0, length + 1);
            dw_mle_export(hwnd, buffer, startpoint, length);
            return std::string(buffer);
        }
        return std::string();
    }
    int Import(const char *buffer, int startpoint) { return dw_mle_import(hwnd, buffer, startpoint); }
    int Import(std::string buffer, int startpoint) { return dw_mle_import(hwnd, buffer.c_str(), startpoint); }
    void GetSize(unsigned long *bytes, unsigned long *lines) { dw_mle_get_size(hwnd, bytes, lines); }
    void Search(const char *text, int point, unsigned long flags) { dw_mle_search(hwnd, text, point, flags); }
    void Search(std::string text, int point, unsigned long flags) { dw_mle_search(hwnd, text.c_str(), point, flags); }
    void SetAutoComplete(int state) { dw_mle_set_auto_complete(hwnd, state); }
    void SetCursor(int point) { dw_mle_set_cursor(hwnd, point); }
    void SetEditable(int state) { dw_mle_set_editable(hwnd, state); }
    void SetVisible(int line) { dw_mle_set_visible(hwnd, line); }
    void SetWordWrap(int state) { dw_mle_set_word_wrap(hwnd, state); }
    int SetFont(const char *font) { return dw_window_set_font(hwnd, font); }
    int SetFont(std::string font) { return dw_window_set_font(hwnd, font.c_str()); }
    char *GetCFont() { return dw_window_get_font(hwnd); }
    std::string GetFont() {
        char *retval = dw_window_get_font(hwnd);
        _dw_string_free_return(retval);
    }
};

class Notebook : public Widget
{
private:
    bool SwitchPageConnected;
#ifdef DW_LAMBDA
    std::function<int(unsigned long)> _ConnectSwitchPage;
#endif
    int (*_ConnectSwitchPageOld)(Notebook *, unsigned long);
    static int _OnSwitchPage(HWND window, unsigned long pageid, void *data) {
        Notebook *classptr = reinterpret_cast<Notebook *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectSwitchPage)
            return classptr->_ConnectSwitchPage(pageid);
#endif
        if(classptr->_ConnectSwitchPageOld)
            return classptr->_ConnectSwitchPageOld(classptr, pageid);
        return classptr->OnSwitchPage(pageid);
    }
protected:
    void Setup() {
#ifdef DW_LAMBDA
        _ConnectSwitchPage = 0;
#endif
        _ConnectSwitchPageOld = 0;
        if(IsOverridden(Notebook::OnSwitchPage, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_SWITCH_PAGE, DW_SIGNAL_FUNC(_OnSwitchPage), this);
            SwitchPageConnected = true;
        } else {
            SwitchPageConnected = false;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnSwitchPage(unsigned long pageid) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_SWITCH_PAGE);
        SwitchPageConnected = false;
        return FALSE;
    }
public:
    // Constructors
    Notebook(unsigned long id, int top) { SetHWND(dw_notebook_new(id, top)); }
    Notebook(unsigned long id) { SetHWND(dw_notebook_new(id, TRUE)); }
    Notebook() { SetHWND(dw_notebook_new(0, TRUE)); }

    // User functions
    void Pack(unsigned long pageid, Widget *page) { dw_notebook_pack(hwnd, pageid, page ? page->GetHWND() : DW_NOHWND); }
    void PageDestroy(unsigned long pageid) { dw_notebook_page_destroy(hwnd, pageid); }
    unsigned long PageGet() { return dw_notebook_page_get(hwnd); }
    unsigned long PageNew(unsigned long flags, int front) { return dw_notebook_page_new(hwnd, flags, front); }
    unsigned long PageNew(unsigned long flags) { return dw_notebook_page_new(hwnd, flags, FALSE); }
    unsigned long PageNew() { return dw_notebook_page_new(hwnd, 0, FALSE); }
    void PageSet(unsigned long pageid) { dw_notebook_page_set(hwnd, pageid); }
    void PageSetStatusText(unsigned long pageid, const char *text) { dw_notebook_page_set_status_text(hwnd, pageid, text); }
    void PageSetText(unsigned long pageid, const char *text) { dw_notebook_page_set_text(hwnd, pageid, text); }
    void PageSetStatusText(unsigned long pageid, std::string text) { dw_notebook_page_set_status_text(hwnd, pageid, text.c_str()); }
    void PageSetText(unsigned long pageid, std::string text) { dw_notebook_page_set_text(hwnd, pageid, text.c_str()); }
#ifdef DW_LAMBDA
    void ConnectSwitchPage(std::function<int(unsigned long)> userfunc)
    {
        _ConnectSwitchPage = userfunc;
        if(!SwitchPageConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_SWITCH_PAGE, DW_SIGNAL_FUNC(_OnSwitchPage), this);
            SwitchPageConnected = true;
        }
    }        
#endif
    void ConnectSwitchPage(int (*userfunc)(Notebook *, unsigned long))
    {
        _ConnectSwitchPageOld = userfunc;
        if(!SwitchPageConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_SWITCH_PAGE, DW_SIGNAL_FUNC(_OnSwitchPage), this);
            SwitchPageConnected = true;
        }
    }        
};

class ObjectView : virtual public Widget
{
private:
    bool ItemSelectConnected, ItemContextConnected;
#ifdef DW_LAMBDA
    std::function<int(HTREEITEM, std::string, void *)> _ConnectItemSelect;
    std::function<int(std::string, int, int, void *)> _ConnectItemContext;
#endif
    int (*_ConnectCItemSelectOld)(ObjectView *, HTREEITEM, char *, void *);
    int (*_ConnectCItemContextOld)(ObjectView *, char *, int, int, void *);
    int (*_ConnectItemSelectOld)(ObjectView *, HTREEITEM, std::string, void *);
    int (*_ConnectItemContextOld)(ObjectView *, std::string, int, int, void *);
    static int _OnItemSelect(HWND window, HTREEITEM item, char *text, void *data, void *itemdata) {
        ObjectView *classptr = reinterpret_cast<ObjectView *>(data);
        std::string utf8string = text ? std::string(text) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectItemSelect)
            return classptr->_ConnectItemSelect(item, utf8string, itemdata);
#endif
        if(classptr->_ConnectItemSelectOld)
            return classptr->_ConnectItemSelectOld(classptr, item, text, itemdata);
        else if(classptr->_ConnectItemSelectOld)
            return classptr->_ConnectItemSelectOld(classptr, item, utf8string, itemdata);
        return classptr->OnItemSelect(item, utf8string, itemdata);
    }
    static int _OnItemContext(HWND window, char *text, int x, int y, void *data, void *itemdata) {
        ObjectView *classptr = reinterpret_cast<ObjectView *>(data);
        std::string utf8string = text ? std::string(text) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectItemContext)
            return classptr->_ConnectItemContext(utf8string, x, y, itemdata);
#endif
        if(classptr->_ConnectCItemContextOld)
            return classptr->_ConnectCItemContextOld(classptr, text, x, y, itemdata);
        else if(classptr->_ConnectItemContextOld)
            return classptr->_ConnectItemContextOld(classptr, utf8string, x, y, itemdata);
        return classptr->OnItemContext(utf8string, x, y, itemdata);
    }
protected:
    void SetupObjectView() {
#ifdef DW_LAMBDA
        _ConnectItemSelect = 0;
        _ConnectItemContext = 0;
#endif
        _ConnectCItemSelectOld = 0;
        _ConnectCItemContextOld = 0;
        _ConnectItemSelectOld = 0;
        _ConnectItemContextOld = 0;
        if(IsOverridden(ObjectView::OnItemSelect, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        } else {
            ItemSelectConnected = false;
        }
        if(IsOverridden(ObjectView::OnItemContext, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(_OnItemContext), this);
            ItemContextConnected = true;
        } else {
            ItemContextConnected = false;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnItemSelect(HTREEITEM item, std::string text, void *itemdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_ITEM_SELECT);
        ItemSelectConnected = false;
        return FALSE;
    }
    virtual int OnItemContext(std::string text, int x, int y, void *itemdata) {
        dw_signal_disconnect_by_name(hwnd, DW_SIGNAL_ITEM_CONTEXT);
        ItemContextConnected = false;
        return FALSE;
    }
public:
#ifdef DW_LAMBDA
    void ConnectItemSelect(std::function<int(HTREEITEM, std::string, void *)> userfunc)
    {
        _ConnectItemSelect = userfunc;
        if(!ItemSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        }
    }
#endif
    void ConnectItemSelect(int (*userfunc)(ObjectView *, HTREEITEM, char *, void *))
    {
        _ConnectCItemSelectOld = userfunc;
        if(!ItemSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        }
    }
    void ConnectItemSelect(int (*userfunc)(ObjectView *, HTREEITEM, std::string, void *))
    {
        _ConnectItemSelectOld = userfunc;
        if(!ItemSelectConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_SELECT, DW_SIGNAL_FUNC(_OnItemSelect), this);
            ItemSelectConnected = true;
        }
    }
#ifdef DW_LAMBDA
    void ConnectItemContext(std::function<int(std::string, int, int, void *)> userfunc)
    {
        _ConnectItemContext = userfunc;
        if(!ItemContextConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(_OnItemContext), this);
            ItemContextConnected = true;
        }
    }        
#endif
    void ConnectItemContext(int (*userfunc)(ObjectView *, char *, int, int, void *))
    {
        _ConnectCItemContextOld = userfunc;
        if(!ItemContextConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_CONTEXT, DW_SIGNAL_FUNC(_OnItemContext), this);
            ItemContextConnected = true;
        }
    }        
    void ConnectItemContext(int (*userfunc)(ObjectView *, std::string, int, int, void *))
    {
        _ConnectItemContextOld = userfunc;
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
    std::function<int(std::string, void *)> _ConnectItemEnter;
    std::function<int(int)> _ConnectColumnClick;
#endif
    int (*_ConnectCItemEnterOld)(Containers *, char *, void *);
    int (*_ConnectItemEnterOld)(Containers *, std::string, void *);
    int (*_ConnectColumnClickOld)(Containers *, int);
    static int _OnItemEnter(HWND window, char *text, void *data, void *itemdata) {
        Containers *classptr = reinterpret_cast<Containers *>(data);
        std::string utf8string = text ? std::string(text) : std::string();
#ifdef DW_LAMBDA
        if(classptr->_ConnectItemEnter)
            return classptr->_ConnectItemEnter(utf8string, itemdata);
#endif
        if(classptr->_ConnectCItemEnterOld)
            return classptr->_ConnectCItemEnterOld(classptr, text, itemdata);
        else if(classptr->_ConnectItemEnterOld)
            return classptr->_ConnectItemEnterOld(classptr, utf8string, itemdata);
        return classptr->OnItemEnter(utf8string, itemdata);
    }
    static int _OnColumnClick(HWND window, int column, void *data) {
        Containers *classptr = reinterpret_cast<Containers *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectColumnClick)
            return classptr->_ConnectColumnClick(column);
#endif
        if(classptr->_ConnectColumnClickOld)
            return classptr->_ConnectColumnClickOld(classptr, column);
        return classptr->OnColumnClick(column);
    }
protected:
    void *allocpointer;
    int allocrowcount;
    void SetupContainer() {
#ifdef DW_LAMBDA
        _ConnectItemEnter = 0;
        _ConnectColumnClick = 0;
#endif
        _ConnectItemEnterOld = 0;
        _ConnectCItemEnterOld = 0;
        _ConnectColumnClickOld = 0;
        if(IsOverridden(Container::OnItemEnter, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        } else {
            ItemEnterConnected = false;
        }
        if(IsOverridden(Container::OnColumnClick, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(_OnColumnClick), this);
            ColumnClickConnected = true;
        } else {
            ColumnClickConnected = false;
        }
    }
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnItemEnter(std::string text, void *itemdata) {
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
    void ChangeRowTitle(int row, std::string title) { dw_container_change_row_title(hwnd, row, title.c_str()); }
    void Clear(int redraw) { dw_container_clear(hwnd, redraw); }
    void Clear() { dw_container_clear(hwnd, TRUE); }
    void Cursor(const char *text) { dw_container_cursor(hwnd, text); }
    void Cursor(std::string text) { dw_container_cursor(hwnd, text.c_str()); }
    void Cursor(void *data) { dw_container_cursor_by_data(hwnd, data); }
    void Delete(int rowcount) { dw_container_delete(hwnd, rowcount); }
    void DeleteRow(char *title) { dw_container_delete_row(hwnd, title); }
    void DeleteRow(std::string title) { dw_container_delete_row(hwnd, title.c_str()); }
    void DeleteRow(void *data) { dw_container_delete_row_by_data(hwnd, data); }
    void Insert() { dw_container_insert(hwnd, allocpointer, allocrowcount); }
    void Optimize() { dw_container_optimize(hwnd); }
    char *QueryCNext(unsigned long flags) { return dw_container_query_next(hwnd, flags); }
    char *QueryCStart(unsigned long flags) { return dw_container_query_start(hwnd, flags); }
    std::string QueryNext(unsigned long flags) {
        char *retval = dw_container_query_next(hwnd, flags);
        return retval ? std::string(retval) : std::string();
    }
    std::string QueryStart(unsigned long flags) {
        char *retval = dw_container_query_start(hwnd, flags);
        return retval ? std::string(retval) : std::string();
    }
    void Scroll(int direction, long rows) { dw_container_scroll(hwnd, direction, rows); }
    void SetColumnWidth(int column, int width) { dw_container_set_column_width(hwnd, column, width); }
    void SetRowData(int row, void *data) { dw_container_set_row_data(allocpointer, row, data); }
    void SetRowTitle(int row, const char *title) { dw_container_set_row_title(allocpointer, row, title); }
    void SetRowTitle(int row, const std::string title) { dw_container_set_row_title(allocpointer, row, title.c_str()); }
    void SetStripe(unsigned long oddcolor, unsigned long evencolor) { dw_container_set_stripe(hwnd, oddcolor, evencolor); }
#ifdef DW_LAMBDA
    void ConnectItemEnter(std::function<int(std::string, void *)> userfunc)
    {
        _ConnectItemEnter = userfunc;
        if(!ItemEnterConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        }
    }        
#endif
    void ConnectItemEnter(int (*userfunc)(Containers *, char *, void *))
    {
        _ConnectCItemEnterOld = userfunc;
        if(!ItemEnterConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        }
    }        
    void ConnectItemEnter(int (*userfunc)(Containers *, std::string, void *))
    {
        _ConnectItemEnterOld = userfunc;
        if(!ItemEnterConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_ITEM_ENTER, DW_SIGNAL_FUNC(_OnItemEnter), this);
            ItemEnterConnected = true;
        }
    }        
#ifdef DW_LAMBDA
    void ConnectColumnClick(std::function<int(int)> userfunc)
    {
        _ConnectColumnClick = userfunc;
        if(!ColumnClickConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_COLUMN_CLICK, DW_SIGNAL_FUNC(_OnColumnClick), this);
            ColumnClickConnected = true;
        }
    }        
#endif
    void ConnectColumnClick(int (*userfunc)(Containers *, int))
    {
        _ConnectColumnClickOld = userfunc;
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
    Container(unsigned long id, int multi) { SetHWND(dw_container_new(id, multi)); }
    Container(int multi) { SetHWND(dw_container_new(0, multi)); }
    Container() { SetHWND(dw_container_new(0, FALSE)); }

    // User functions
    int Setup(unsigned long *flags, const char *titles[], int count, int separator) {
      int retval = dw_container_setup(hwnd, flags, (char **)titles, count, separator);
      SetupObjectView(); SetupContainer();
      return retval;
    }    
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
    int Setup(unsigned long *flags, const char *titles[], int count) { 
      int retval = dw_filesystem_setup(hwnd, flags, (char **)titles, count);
      SetupObjectView(); SetupContainer();
      return retval;
    }    
    int Setup(std::vector<unsigned long> flags, std::vector<std::string> titles) {
      unsigned int count = flags.size();

      // Use the smallest of the two lists
      if(count > titles.size()) {
          count = titles.size();
      }
      // Convert our vectors into a arrays of unsigned long and C strings
      const char **ctitles = (const char **)alloca(sizeof(char *) * count); 
      unsigned long *cflags = (unsigned long *)alloca(sizeof(unsigned long) * count);
      for(unsigned int z=0; z<count; z++) {
          ctitles[z] = titles[z].c_str();
          cflags[z] = flags[z];
      }
      int retval = dw_filesystem_setup(hwnd, cflags, (char **)ctitles, count);
      SetupObjectView(); SetupContainer();
      return retval;
    }    
    void ChangeFile(int row, const char *filename, HICN icon) { dw_filesystem_change_file(hwnd, row, filename, icon); }
    void ChangeFile(int row, std::string filename, HICN icon) { dw_filesystem_change_file(hwnd, row, filename.c_str(), icon); }
    void ChangeItem(int column, int row, void *data) { dw_filesystem_change_item(hwnd, column, row, data); }
    int GetColumnType(int column) { return dw_filesystem_get_column_type(hwnd, column); }
    void SetColumnTitle(const char *title) { dw_filesystem_set_column_title(hwnd, title); }
    void SetColumnTitle(std::string title) { dw_filesystem_set_column_title(hwnd, title.c_str()); }
    void SetFile(int row, const char *filename, HICN icon) { dw_filesystem_set_file(hwnd, allocpointer, row, filename, icon); }
    void SetFile(int row, std::string filename, HICN icon) { dw_filesystem_set_file(hwnd, allocpointer, row, filename.c_str(), icon); }
    void SetItem(int column, int row, void *data) { dw_filesystem_set_item(hwnd, allocpointer, column, row, data); }
};

class Tree : virtual public Focusable, virtual public ObjectView
{
private:
    bool TreeExpandConnected;
#ifdef DW_LAMBDA
    std::function<int(HTREEITEM)> _ConnectTreeExpand;
#endif
    int (*_ConnectTreeExpandOld)(Tree *, HTREEITEM);
    static int _OnTreeExpand(HWND window, HTREEITEM item, void *data) {
        Tree *classptr = reinterpret_cast<Tree *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectTreeExpand)
            return classptr->_ConnectTreeExpand(item);
#endif
        if(classptr->_ConnectTreeExpandOld)
            return classptr->_ConnectTreeExpandOld(classptr, item);
        return classptr->OnTreeExpand(item);
    }
    void SetupTree() {
#ifdef DW_LAMBDA
        _ConnectTreeExpand = 0;
#endif
        _ConnectTreeExpandOld = 0;
        if(IsOverridden(Tree::OnTreeExpand, this)) {
            dw_signal_connect(hwnd, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_OnTreeExpand), this);
            TreeExpandConnected = true;
        } else {
            TreeExpandConnected = false;
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
    char *GetCTitle(HTREEITEM item) { return dw_tree_get_title(hwnd, item); }
    std::string GetTitle(HTREEITEM item) {
        char *retval = dw_tree_get_title(hwnd, item);
        _dw_string_free_return(retval);
    }
    HTREEITEM Insert(const char *title, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert(hwnd, title, icon, parent, itemdata); }
    HTREEITEM Insert(const char *title, HICN icon, HTREEITEM parent) { return dw_tree_insert(hwnd, title, icon, parent, NULL); }
    HTREEITEM Insert(const char *title, HICN icon) { return dw_tree_insert(hwnd, title, icon, 0, NULL); }
    HTREEITEM InsertAfter(const char *title, HTREEITEM item, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert_after(hwnd, item, title, icon, parent, itemdata); }
    HTREEITEM InsertAfter(const char *title, HTREEITEM item, HICN icon, HTREEITEM parent) { return dw_tree_insert_after(hwnd, item, title, icon, parent, NULL); }
    HTREEITEM InsertAfter(const char *title, HTREEITEM item, HICN icon) { return dw_tree_insert_after(hwnd, item, title, icon, 0, NULL); }
    HTREEITEM Insert(std::string title, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert(hwnd, title.c_str(), icon, parent, itemdata); }
    HTREEITEM Insert(std::string title, HICN icon, HTREEITEM parent) { return dw_tree_insert(hwnd, title.c_str(), icon, parent, NULL); }
    HTREEITEM Insert(std::string title, HICN icon) { return dw_tree_insert(hwnd, title.c_str(), icon, 0, NULL); }
    HTREEITEM InsertAfter(std::string title, HTREEITEM item, HICN icon, HTREEITEM parent, void *itemdata) { return dw_tree_insert_after(hwnd, item, title.c_str(), icon, parent, itemdata); }
    HTREEITEM InsertAfter(std::string title, HTREEITEM item, HICN icon, HTREEITEM parent) { return dw_tree_insert_after(hwnd, item, title.c_str(), icon, parent, NULL); }
    HTREEITEM InsertAfter(std::string title, HTREEITEM item, HICN icon) { return dw_tree_insert_after(hwnd, item, title.c_str(), icon, 0, NULL); }
    void Change(HTREEITEM item, const char *title, HICN icon) { dw_tree_item_change(hwnd, item, title, icon); }
    void Change(HTREEITEM item, std::string title, HICN icon) { dw_tree_item_change(hwnd, item, title.c_str(), icon); }
    void Collapse(HTREEITEM item) { dw_tree_item_collapse(hwnd, item); }
    void Delete(HTREEITEM item) { dw_tree_item_delete(hwnd, item); }
    void Expand(HTREEITEM item) { dw_tree_item_expand(hwnd, item); }
    void *GetData(HTREEITEM item) { return dw_tree_item_get_data(hwnd, item); }
    void Select(HTREEITEM item) { dw_tree_item_select(hwnd, item); }
    void SetData(HTREEITEM item, void *itemdata) { dw_tree_item_set_data(hwnd, item, itemdata); }
#ifdef DW_LAMBDA
    void ConnectTreeExpand(std::function<int(HTREEITEM)> userfunc)
    {
        _ConnectTreeExpand = userfunc;
        if(!TreeExpandConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_OnTreeExpand), this);
            TreeExpandConnected = true;
        }
    }
#endif
    void ConnectTreeExpandOld(int (*userfunc)(Tree *, HTREEITEM))
    {
        _ConnectTreeExpandOld = userfunc;
        if(!TreeExpandConnected) {
            dw_signal_connect(hwnd, DW_SIGNAL_TREE_EXPAND, DW_SIGNAL_FUNC(_OnTreeExpand), this);
            TreeExpandConnected = true;
        }
    }
};

class SplitBar : public Widget
{
public:
    // Constructors
    SplitBar(int orient, Widget *topleft, Widget *bottomright, unsigned long id) { 
            SetHWND(dw_splitbar_new(orient, topleft ? topleft->GetHWND() : DW_NOHWND, bottomright ? bottomright->GetHWND() : DW_NOHWND, id)); }
    SplitBar(int orient, Widget *topleft, Widget *bottomright) { 
            SetHWND(dw_splitbar_new(orient, topleft ? topleft->GetHWND() : DW_NOHWND, bottomright ? bottomright->GetHWND() : DW_NOHWND, 0)); }

    // User functions
    float Get() { return dw_splitbar_get(hwnd); }
    void Set(float percent) { dw_splitbar_set(hwnd, percent); }
};

class Dialog : public Handle
{
private:
    DWDialog *dialog;
public:
    // Constructors
    Dialog(void *data) { dialog = dw_dialog_new(data); SetHandle(reinterpret_cast<void *>(dialog)); }
    Dialog() { dialog = dw_dialog_new(DW_NULL); SetHandle(reinterpret_cast<void *>(dialog)); }

    // User functions
    void *Wait() { void *retval = dw_dialog_wait(dialog); delete this; return retval; }
    int Dismiss(void *data) { return dw_dialog_dismiss(dialog, data); }
    int Dismiss() { return dw_dialog_dismiss(dialog, NULL); }
};

class Mutex : public Handle
{
private:
    HMTX mutex;
public:
    // Constructors
    Mutex() { mutex = dw_mutex_new(); SetHandle(reinterpret_cast<void *>(mutex)); }
    // Destructor
    ~Mutex() { dw_mutex_close(mutex); mutex = 0; }

    // User functions
    void Close() { dw_mutex_close(mutex); mutex = 0; delete this; }
    void Lock() { dw_mutex_lock(mutex); }
    int TryLock() { return dw_mutex_trylock(mutex); }
    void Unlock() { dw_mutex_unlock(mutex); }
};

class Event : public Handle
{
private:
    HEV event, named;
public:
    // Constructors
    Event() { event = dw_event_new(); named = 0; SetHandle(reinterpret_cast<void *>(event)); }
    Event(const char *name) {
        // Try to attach to an existing event
        named = dw_named_event_get(name);
        if(!named) {
            // Otherwise try to create a new one
            named = dw_named_event_new(name);
        }
        event = 0;
        SetHandle(reinterpret_cast<void *>(named));
    }
    Event(std::string name) {
        // Try to attach to an existing event
        named = dw_named_event_get(name.c_str());
        if(!named) {
            // Otherwise try to create a new one
            named = dw_named_event_new(name.c_str());
        }
        event = 0;
        SetHandle(reinterpret_cast<void *>(named));
    }
    // Destructor
    virtual ~Event() { if(event) { dw_event_close(&event); } if(named) { dw_named_event_close(named); } }

    // User functions
    int Close() { 
        int retval = DW_ERROR_UNKNOWN;
        
        if(event) {
            retval = dw_event_close(&event);
        } else if(named) {
            retval = dw_named_event_close(named);
            named = 0;
        }
        delete this;
        return retval;
    }
    int Post() { return (named ? dw_named_event_post(named) : dw_event_post(event)); }
    int Reset() { return (named ? dw_named_event_reset(named) : dw_event_reset(event)); }
    int Wait(unsigned long timeout) { return (named ? dw_named_event_wait(named, timeout) : dw_event_wait(event, timeout)); }
};

class Timer : public Handle
{
private:
    HTIMER timer;
#ifdef DW_LAMBDA
    std::function<int()> _ConnectTimer;
#endif
    int (*_ConnectTimerOld)(Timer *);
    static int _OnTimer(void *data) {
        Timer *classptr = reinterpret_cast<Timer *>(data);
#ifdef DW_LAMBDA
        if(classptr->_ConnectTimer)
            return classptr->_ConnectTimer();
#endif
        if(classptr->_ConnectTimerOld)
            return classptr->_ConnectTimerOld(classptr);
        return classptr->OnTimer();
    }
public:
    // Constructors
    Timer(int interval) {
#ifdef DW_LAMBDA
        _ConnectTimer = 0;
#endif
        _ConnectTimerOld = 0;
        timer = dw_timer_connect(interval, DW_SIGNAL_FUNC(_OnTimer), this);
        SetHandle(reinterpret_cast<void *>(timer));
    }
#ifdef DW_LAMBDA
    Timer(int interval, std::function<int()> userfunc) {
        _ConnectTimer = userfunc;
        _ConnectTimerOld = 0;
        timer = dw_timer_connect(interval, DW_SIGNAL_FUNC(_OnTimer), this);
        SetHandle(reinterpret_cast<void *>(timer));
    }
#endif
    Timer(int interval, int (*userfunc)(Timer *)) {
        _ConnectTimerOld = userfunc;
#ifdef DW_LAMBDA
        _ConnectTimer = 0;
#endif
        timer = dw_timer_connect(interval, DW_SIGNAL_FUNC(_OnTimer), this);
        SetHandle(reinterpret_cast<void *>(timer));
    }
    // Destructor
    virtual ~Timer() { if(timer) { dw_timer_disconnect(timer); timer = 0; } }

    // User functions
    HTIMER GetHTIMER() { return timer; }
    void Disconnect() { if(timer) { dw_timer_disconnect(timer); timer = 0; } delete this; }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnTimer() {
        if(timer)
            dw_timer_disconnect(timer);
        delete this;
        return FALSE;
    }
};

class Notification final : public Clickable
{
public:
    // Constructors
    Notification(const char *title, const char *imagepath, const char *description) { SetHWND(dw_notification_new(title, imagepath, description)); }
    Notification(const char *title, const char *imagepath) { SetHWND(dw_notification_new(title, imagepath, NULL)); }
    Notification(const char *title) { SetHWND(dw_notification_new(title, NULL, NULL)); }
    Notification(std::string title, std::string imagepath, std::string description) { SetHWND(dw_notification_new(title.c_str(), imagepath.c_str(), description.c_str())); }
    Notification(std::string title, std::string imagepath) { SetHWND(dw_notification_new(title.c_str(), imagepath.c_str(), NULL)); }
    Notification(std::string title) { SetHWND(dw_notification_new(title.c_str(), NULL, NULL)); }

    // User functions
    int Send() { int retval = dw_notification_send(hwnd); delete this; return retval; }
};

class Print : public Handle
{
private:
    HPRINT print;
#ifdef DW_LAMBDA
    std::function<int(Pixmap *, int)> _ConnectDrawPage;
#endif
    int (*_ConnectDrawPageOld)(Print *, Pixmap *, int);
    static int _OnDrawPage(HPRINT print, HPIXMAP hpm, int page_num, void *data) {
        int retval;
        Pixmap *pixmap = new Pixmap(hpm);
        Print *classptr = reinterpret_cast<Print *>(data);

#ifdef DW_LAMBDA
        if(classptr->_ConnectDrawPage)
            retval = classptr->_ConnectDrawPage(pixmap, page_num);
        else
#endif
        if(classptr->_ConnectDrawPageOld)
            retval = classptr->_ConnectDrawPageOld(classptr, pixmap, page_num);
        else
            retval = classptr->OnDrawPage(pixmap, page_num);

        delete pixmap;
        return retval;
    }
public:
    // Constructors
#ifdef DW_LAMBDA
    Print(const char *jobname, unsigned long flags, unsigned int pages, std::function<int(Pixmap *, int)> userfunc) {
        print = dw_print_new(jobname, flags, pages, DW_SIGNAL_FUNC(_OnDrawPage), this);
        SetHandle(reinterpret_cast<void *>(print));
        _ConnectDrawPage = userfunc;
        _ConnectDrawPageOld = 0;
    }
    Print(std::string jobname, unsigned long flags, unsigned int pages, std::function<int(Pixmap *, int)> userfunc) {
        print = dw_print_new(jobname.c_str(), flags, pages, DW_SIGNAL_FUNC(_OnDrawPage), this);
        SetHandle(reinterpret_cast<void *>(print));
        _ConnectDrawPage = userfunc;
        _ConnectDrawPageOld = 0;
    }
#endif
    Print(const char *jobname, unsigned long flags, unsigned int pages, int (*userfunc)(Print *, Pixmap *, int)) { 
        print = dw_print_new(jobname, flags, pages, DW_SIGNAL_FUNC(_OnDrawPage), this);
        SetHandle(reinterpret_cast<void *>(print));
        _ConnectDrawPageOld = userfunc;
#ifdef DW_LAMBDA
        _ConnectDrawPage = 0;
#endif
    }
    Print(std::string jobname, unsigned long flags, unsigned int pages, int (*userfunc)(Print *, Pixmap *, int)) { 
        print = dw_print_new(jobname.c_str(), flags, pages, DW_SIGNAL_FUNC(_OnDrawPage), this);
        SetHandle(reinterpret_cast<void *>(print));
        _ConnectDrawPageOld = userfunc;
#ifdef DW_LAMBDA
        _ConnectDrawPage = 0;
#endif
    }
    // Destructor
    virtual ~Print() { if(print) dw_print_cancel(print); print = 0; }

    // User functions
    void Cancel() { dw_print_cancel(print); delete this; }
    int Run(unsigned long flags) { int retval = dw_print_run(print, flags); delete this; return retval; }
    HPRINT GetHPRINT() { return print; }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual int OnDrawPage(Pixmap *pixmap, int page_num) {
        if(print)
            dw_print_cancel(print);
        delete this;
        return FALSE;
    }
};

class Thread : public Handle
{
private:
    DWTID tid;
#ifdef DW_LAMBDA
    std::function<void(Thread *)> _ConnectThread;
#endif
    void (*_ConnectThreadOld)(Thread *);
    static void _OnThread(void *data) {
        Thread *classptr = reinterpret_cast<Thread *>(data);

#ifdef DW_LAMBDA
        if(classptr->_ConnectThread)
            classptr->_ConnectThread(classptr);
        else
#endif
        if(classptr->_ConnectThreadOld)
            classptr->_ConnectThreadOld(classptr);
        else
            classptr->OnThread(classptr);

        delete classptr;
    }
public:
    // Constructors
#ifdef DW_LAMBDA
    Thread(std::function<void(Thread *)> userfunc) {
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, _DW_THREAD_STACK);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThread = userfunc;
        _ConnectThreadOld = 0;
    }
    Thread(std::function<void(Thread *)> userfunc, int stack) {
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, stack);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThread = userfunc;
        _ConnectThreadOld = 0;
    }
#endif
    Thread(void (*userfunc)(Thread *)) { 
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, _DW_THREAD_STACK);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThreadOld = userfunc;
#ifdef DW_LAMBDA
        _ConnectThread = 0;
#endif
    }
    Thread(void (*userfunc)(Thread *), int stack) { 
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, stack);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThreadOld = userfunc;
#ifdef DW_LAMBDA
        _ConnectThread = 0;
#endif
    }
    Thread(int stack) { 
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, stack);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThreadOld = 0;
#ifdef DW_LAMBDA
        _ConnectThread = 0;
#endif
    }
    Thread() { 
        tid = dw_thread_new(DW_SIGNAL_FUNC(_OnThread), this, _DW_THREAD_STACK);
        SetHandle(reinterpret_cast<void *>(tid));
        _ConnectThreadOld = 0;
#ifdef DW_LAMBDA
        _ConnectThread = 0;
#endif
    }
    // Destructor
    virtual ~Thread() { if(tid != 0 && dw_thread_id() == tid) dw_thread_end(); tid = 0; }

    // User functions
    void End() { if(tid != 0 && dw_thread_id() == tid) delete this; }
    DWTID GetTID() { return tid; }
protected:
    // Our signal handler functions to be overriden...
    // If they are not overridden and an event is generated, remove the unused handler
    virtual void OnThread(Thread *classptr) {
        delete this;
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
    static App *Init(std::string appid) { if(!_app) { _app = new App(); dw_app_id_set(appid.c_str(), DW_NULL); dw_init(TRUE, 0, DW_NULL); } return _app; }
    static App *Init(std::string appid, std::string appname) { if(!_app) { _app = new App(); dw_app_id_set(appid.c_str(), appname.c_str()); dw_init(TRUE, 0, DW_NULL); } return _app; }
    static App *Init(int argc, char *argv[], std::string appid) { if(!_app) { _app = new App(); dw_app_id_set(appid.c_str(), DW_NULL); dw_init(TRUE, argc, argv); } return _app; }
    static App *Init(int argc, char *argv[], std::string appid, std::string appname) { if(!_app) { _app = new App(); dw_app_id_set(appid.c_str(), appname.c_str()); dw_init(TRUE, argc, argv); } return _app; }
    // Destrouctor
    ~App() { dw_exit(0); }

    // User functions
    void Main() { dw_main(); }
    void MainIteration() { dw_main_iteration(); }
    void MainQuit() { dw_main_quit(); }
    void MainSleep(int milliseconds) { dw_main_sleep(milliseconds); }
    void Exit(int exitcode) { dw_exit(exitcode); }
    void Shutdown() { dw_shutdown(); }
    int MessageBox(const char *title, int flags, const char *format, ...) { 
        int retval;
        va_list args;

        va_start(args, format);
        retval = dw_vmessagebox(title, flags, format, args); 
        va_end(args);

        return retval;
    }
    int MessageBox(std::string title, int flags, std::string format, ...) { 
        int retval;
        const char *cformat = format.c_str();
        va_list args;

        va_start(args, format);
        retval = dw_vmessagebox(title.c_str(), flags, cformat, args); 
        va_end(args);

        return retval;
    }
    void Debug(const char *format, ...) { 
        va_list args;

        va_start(args, format);
        dw_vdebug(format, args); 
        va_end(args);
    }
    void Debug(std::string format, ...) {
        const char *cformat = format.c_str();
        va_list args;

        va_start(args, format);
        dw_vdebug(cformat, args); 
        va_end(args);
    }
    void Beep(int freq, int dur) { dw_beep(freq, dur); }
    char *GetCDir() { return dw_app_dir(); }
    std::string GetDir() {
        char *retval =  dw_app_dir();
        return std::string(retval);
    }
    void GetEnvironment(DWEnv *env) { dw_environment_query(env); }
    int GetScreenWidth() { return dw_screen_width(); }
    int GetScreenHeight() { return dw_screen_height(); }
    void GetPointerPos(long *x, long *y) { dw_pointer_query_pos(x, y); }
    unsigned long GetColorDepth() { return dw_color_depth_get(); }
    char *GetCClipboard() { return dw_clipboard_get_text(); }
    std::string GetClipboard() {
        char *retval = dw_clipboard_get_text();
        _dw_string_free_return(retval);
    }
    void SetClipboard(const char *text) { if(text) dw_clipboard_set_text(text, (int)strlen(text)); }
    void SetClipboard(const char *text, int len) { if(text) dw_clipboard_set_text(text, len); }
    void SetClipboard(std::string text) { dw_clipboard_set_text(text.c_str(), (int)strlen(text.c_str())); }
    void SetClipboard(std::string text, int len) { dw_clipboard_set_text(text.c_str(), len); }
    void SetDefaultFont(const char *fontname) { dw_font_set_default(fontname); }
    void SetDefaultFont(std::string fontname) { dw_font_set_default(fontname.c_str()); }
    unsigned long ColorChoose(unsigned long initial) { return dw_color_choose(initial); }
    char *FileBrowse(const char *title, const char *defpath, const char *ext, int flags) { return dw_file_browse(title, defpath, ext, flags); }
    char *FontChoose(const char *currfont) { return dw_font_choose(currfont); }
    std::string FileBrowse(std::string title, std::string defpath, std::string ext, int flags) {
        char *retval = dw_file_browse(title.c_str(), defpath.c_str(), ext.c_str(), flags);
        return retval ? std::string(retval) : std::string();
    }
    std::string FontChoose(std::string currfont) { 
        char *retval = dw_font_choose(currfont.c_str()); 
        return retval ? std::string(retval) : std::string();
    }
    void Free(void *buff) { dw_free(buff); }
    int GetFeature(DWFEATURE feature) { return dw_feature_get(feature); }
    int SetFeature(DWFEATURE feature, int state) { return dw_feature_set(feature, state); }
    HICN LoadIcon(unsigned long id) { return dw_icon_load(0, id); }
    HICN LoadIcon(const char *filename) { return dw_icon_load_from_file(filename); }
    HICN LoadIcon(std::string filename) { return dw_icon_load_from_file(filename.c_str()); }
    HICN LoadIcon(const char *data, int len) { return dw_icon_load_from_data(data, len); }
    void FreeIcon(HICN icon) { dw_icon_free(icon); }
    void TaskBarInsert(Widget *handle, HICN icon,  const char *bubbletext) { dw_taskbar_insert(handle ? handle->GetHWND() : DW_NOHWND, icon, bubbletext); }
    void TaskBarInsert(Widget *handle, HICN icon,  std::string bubbletext) { dw_taskbar_insert(handle ? handle->GetHWND() : DW_NOHWND, icon, bubbletext.c_str()); }
    void TaskBarDelete(Widget *handle, HICN icon) { dw_taskbar_delete(handle ? handle->GetHWND() : DW_NOHWND, icon); }
    char * WideToUTF8(const wchar_t * wstring) { return dw_wchar_to_utf8(wstring); }
    wchar_t *UTF8ToWide(const char * utf8string) { return dw_utf8_to_wchar(utf8string); }
};

// Static singleton reference declared outside of the class
App* App::_app = DW_NULL;

} // namespace DW
#endif
