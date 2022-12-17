#include <dw.hpp>

DW::App *app;

class MyWindow : public DW::Window
{
public:
  MyWindow() {
	SetText("Basic application");
	SetSize(200, 200);
	}
protected:
	virtual int OnDelete() { app->MainQuit(); return FALSE; }
	virtual int OnConfigure(int width, int height)  { return FALSE; }
};

int dwmain(int argc, char* argv[])
{
  app = new DW::App(argc, argv, "org.dbsoft.dwindows.dwtestoo");
  MyWindow *window = new MyWindow();

  window->Show();

  app->Main();
  app->Exit(0);

  return 0;
}
