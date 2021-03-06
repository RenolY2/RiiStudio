#pragma once

#include <core/window/gl_window.hpp>
#include <core/window/window.hpp>

namespace riistudio::core {

class Applet : public Window, public GLWindow {
public:
  Applet(const char* name);
  ~Applet();

  void frameProcess() override;
  void frameRender() override;
};

} // namespace riistudio::core