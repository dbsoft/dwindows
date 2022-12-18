#include <dw.hpp>

class MyWindow : public DW::Window
{
public:
  MyWindow() {
	SetText("Basic application");
	SetSize(200, 200);
	}
	virtual int OnDelete() override { DW::App *app = DW::App::Init(); app->MainQuit(); return FALSE; }
	virtual int OnConfigure(int width, int height) override { return FALSE; }
};

int dwmain(int argc, char* argv[])
{
  DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo");
  MyWindow *window = new MyWindow();

  window->Show();

  app->Main();
  app->Exit(0);

  return 0;
}
