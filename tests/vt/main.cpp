#include "vt100.hpp"

#define DPSG_COMPILE_LINUX_TERM
#include "linux_term.hpp"

#include <iostream>

void process_inputs(auto &ctx) {
  using namespace dpsg;
  using namespace dpsg::vt100;
  std::cout << red << "Hello VT World!" << reset << '\n' << std::flush;
  auto input = ctx.input_stream();
  auto mouse_enabled = ctx.enable_mouse_tracking();
  while (input) {
    char in = input();

    if (in == 4) { // ctrl+D
      return;
    }

    if (in == '\x1b') {
      std::cout << setf(128, 128, 128) << "^[" << reset << std::flush;
    } else if (isprint(in) != 0) {
      std::cout << in << std::flush;
    } else {
      std::cout << yellow << (uint16_t)in << reset << std::flush;
    }
  }
}

void print_key(dpsg::event in) {
  std::cout << std::format(
      "Key: \"{}\"\n\t- Alt: {}\n\t- Ctrl: {}\n\t- Shift: {}\n",
      (char)in.key.code, in.alt_pressed(), in.ctrl_pressed(),
      in.shift_pressed());
}

void print_mouse(dpsg::event in) {
  std::cout << std::format("Mouse: {}\n\t- x: {}\n\t- y: {}\n",
                           (int)in.mouse.mods, (int)in.mouse.x,
                           (int)in.mouse.y);
}

void process_events(auto &ctx) {
  using namespace dpsg;
  using namespace dpsg::vt100;
  std::cout << red << "Hello VT World!" << reset << '\n' << std::flush;
  auto input = ctx.event_stream();
  auto mouse_enabled = ctx.enable_mouse_tracking();
  while (input) {
    event in = input();

    if (in == event::arrow_up) {
      std::cout << "<UP>\n";
    } else if (in == event::arrow_down) {
      std::cout << "<DOWN>\n";
    } else if (in == event::arrow_left) {
      std::cout << "<LEFT>\n";
    } else if (in == event::arrow_right) {
      std::cout << "<RIGHT>\n";
    } else if (in.is_key_event()) {
      print_key(in);
    } else if (in.is_mouse_event()) {
      print_mouse(in);
    }

    if (in == (event_key{'d'} | event_key::modifiers::Ctrl)) {
      return;
    }

    std::cout << std::flush;
  }
}

int main() {
  using namespace dpsg;
  with_raw_mode([&](raw_mode_context &ctx) { process_events(ctx); });
  std::cout << '\n';
}
