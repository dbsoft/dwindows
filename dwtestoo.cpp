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

int dwmain(int argc, char* argv[]) 
{
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo");
    MyWindow *window = new MyWindow();
    DW::Text *text = new DW::Text("Test window");
    
    window->PackStart(text, DW_SIZE_AUTO, DW_SIZE_AUTO, TRUE, TRUE, 0);
    text->SetStyle(DW_DT_CENTER | DW_DT_VCENTER, DW_DT_CENTER | DW_DT_VCENTER);
    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
