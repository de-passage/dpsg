#include "vt100.hpp"

#define DPSG_COMPILE_LINUX_TERM
#include "linux_term.hpp"

#include <iostream>
#include <sstream>

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

std::string print_code(dpsg::event::key key) {
  std::stringstream iss;
  using namespace dpsg::term_events;
  if (key.same_key(arrow_up)) {
    return "<UP>";
  }
  if (key.same_key(arrow_left)) {
    return "<LEFT>";
  }
  if (key.same_key(arrow_down)) {
    return "<DOWN>";
  }
  if (key.same_key(arrow_right)) {
    return "<RIGHT>";
  }
  if (key.same_key(f1)) {
    return "<F1>";
  }
  if (key.same_key(f2)) {
    return "<F2>";
  }
  if (key.same_key(f3)) {
    return "<F3>";
  }
  if (key.same_key(f4)) {
    return "<F4>";
  }
  if (key.is_unicode()) {
    iss << dpsg::vt100::magenta;
    for (auto c : key.code_points()) {
      iss << '<' << (int)c << '>';
    }
    iss << " (" << key.code_points() << ")" << dpsg::vt100::reset;
  } else {
    iss << dpsg::vt100::cyan << key.code << dpsg::vt100::reset
        << dpsg::vt100::bold;
  }

  return iss.str();
}

void print_key(dpsg::event::key in) {
  std::cout << std::format("Key: \"{}\"\n\t- Unicode: {}\n\t- Alt: {}\n\t- "
                           "Ctrl: {}\n\t- Shift: {}\n",
                           print_code(in), in.is_unicode(), in.alt_pressed(),
                           in.ctrl_pressed(), in.shift_pressed());
}

void print_mouse(dpsg::event::mouse in) {
  std::cout << std::format("Mouse: {}\n\t- x: {}\n\t- y: {}\n", (int)in.mods,
                           (int)in.x, (int)in.y);
}

void process_events(auto &ctx) {
  using namespace dpsg;
  using namespace dpsg::vt100;
  using namespace dpsg::term_events;
  std::cout << red << "Hello VT World!" << reset << '\n' << std::flush;
  auto input = ctx.event_stream();
  auto mouse_enabled = ctx.enable_mouse_tracking();
  try {
    while (input) {
      auto [in, buffer] = input();
      std::cout << cyan << std::format("Buffer (size: {}):\t", buffer.size());
      for (char c : buffer) {
        if (isprint(c) != 0) {
          std::cout.put(c);
        } else {
          std::cout << red << '<' << (int)c << '>' << reset;
        }
      }
      std::cout << '\n' << (bold | white);

      if (in.is_key_event()) {
        print_key(in.get_key());
      } else if (in.is_mouse_event()) {
        print_mouse(in.get_mouse());
      }

      if (in == ctrl + 'd') {
        return;
      }

      std::cout << reset << std::flush;
    }
  } catch (invalid_sequence &err) {
    std::cerr << red << err.what();
    std::cerr << "\nBuffer was: ";
    for (auto c : err.buffer()) {
      if (isprint(c)) {
        std::cerr << yellow << c;
      } else {
        std::cerr << setf(240, 95, 0) << '<' << (int)c << '>';
      }
    }
    std::cerr << reset << "\n";
  }
}

int main() {
  using namespace dpsg;
  with_raw_mode([&](raw_mode_context &ctx) { process_events(ctx); });
  std::cout << '\n';
}
