/*
 * Simple C++ Dynamic Windows Example
 */
#include "dw.hpp"

class MyWindow : public DW::Window
{
public:
    MyWindow() {
        SetText("Basic application");
        SetSize(200, 200);
     }
     int OnDelete() override { DW::App *app = DW::App::Init(); app->MainQuit(); return FALSE; }
     int OnConfigure(int width, int height) override { return FALSE; }
};

#ifndef DW_LAMBDA
int button_clicked()
{
    DW::App *app = DW::App::Init();
    app->MessageBox("Button", DW_MB_OK | DW_MB_INFORMATION, "Clicked!"); 
    return TRUE; 
}

int exit_handler()
{
    DW::App *app = DW::App::Init();
    if(app->MessageBox("dwtest", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?") != 0) {
        app->MainQuit();
    }
    return TRUE; 
}
#endif

int dwmain(int argc, char* argv[]) 
{
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo");
    MyWindow *window = new MyWindow();
    DW::Button *button = new DW::Button("Test window");

    window->PackStart(button, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, TRUE, 0);
#ifdef DW_LAMBDA
    button->ConnectClicked([app] () -> int 
        { 
            app->MessageBox("Button", DW_MB_OK | DW_MB_WARNING, "Clicked!"); 
            return TRUE; 
        });
#else
    button->ConnectClicked(&button_clicked);
#endif

    DW::MenuBar *mainmenubar = window->MenuBarNew();

    // add menus to the menubar
    DW::Menu *menu = new DW::Menu();
    DW::MenuItem *menuitem = menu->AppendItem("~Quit");
#ifdef DW_LAMBDA
    menuitem->ConnectClicked([app] () -> int 
        { 
            if(app->MessageBox("dwtest", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?") != 0) {
                app->MainQuit();
            }
            return TRUE;
        });
#else
    menuitem->ConnectClicked(&exit_handler);
#endif

    // Add the "File" menu to the menubar...
    mainmenubar->AppendItem("~File", menu);

    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
