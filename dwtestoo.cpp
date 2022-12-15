#include <dw.hpp>

class MyWindow : public DW::Window
{
public:
  MyWindow();
};

MyWindow::MyWindow()
{
  SetText("Basic application");
  SetSize(200, 200);
}

int main(int argc, char* argv[])
{
  DW::App *app = new DW::App(argc, argv, "org.dbsoft.dwindows.dwtestoo");
  MyWindow *window = new MyWindow();

  window->Show();

  app->Main();
  app->Exit(0);

  return 0;
}
