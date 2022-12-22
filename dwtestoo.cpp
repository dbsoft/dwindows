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

#ifndef DW_CPP11
int button_clicked()
{
    app->MessageBox("Button", DW_MB_OK | DW_MB_WARNING, "Clicked!"); 
    return TRUE; 
}
#endif

int dwmain(int argc, char* argv[]) 
{
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo");
    MyWindow *window = new MyWindow();
    DW::Button *button = new DW::Button("Test window");
    
    window->PackStart(button, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, TRUE, 0);
#ifdef DW_CPP11
    button->ConnectClicked([app] () -> int 
        { 
            app->MessageBox("Button", DW_MB_OK | DW_MB_WARNING, "Clicked!"); 
            return TRUE; 
        });
#else
    button ->ConnectClicked(&button_clicked);
#endif
    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
