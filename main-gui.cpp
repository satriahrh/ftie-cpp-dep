#include "gui/tampilan.h"
#include <gtkmm/application.h>

// g++ ftie/*.h ftie/*.cpp gui/tampilan.h gui/tampilan.cpp main-gui.cpp -o bin/main-gui.o -std=c++17 `pkg-config gtkmm-3.0 libpng --cflags --libs`

int main(int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "com.satriahrharsono.ftie");

  Tampilan tampilan;
  tampilan.set_default_size(400, 300);
  return app->run(tampilan);
}
