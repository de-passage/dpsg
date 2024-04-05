#pragma once

#include "generator.hpp"
#include "types.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace dpsg {

struct errno_exception : std::runtime_error {
  errno_exception() : std::runtime_error(strerror(errno)) {}

  int code{errno};
};

extern "C" {
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
}

template <int NewMode = ECHO | ICANON>
inline void raw_mode_enable(struct termios *ctx) {
  tcgetattr(STDIN_FILENO, ctx);
  struct termios raw = *ctx;
  raw.c_lflag &= ~(NewMode);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

inline void raw_mode_disable(struct termios *ctx) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, ctx);
}

struct cursor_position {
  int col;
  int row;
};
struct terminal_size {
  int col;
  int row;
};

template <int Mode> struct raw_mode_context_basic {
  struct termios orig_termios;

  raw_mode_context_basic() noexcept { raw_mode_enable<Mode>(&orig_termios); }

  raw_mode_context_basic(const raw_mode_context_basic &) = delete;
  raw_mode_context_basic &operator=(const raw_mode_context_basic &) = delete;
  raw_mode_context_basic(raw_mode_context_basic &&) = delete;
  raw_mode_context_basic &operator=(raw_mode_context_basic &&) = delete;

  ~raw_mode_context_basic() noexcept { raw_mode_disable(&orig_termios); }

  struct enable_mouse_t {
    enable_mouse_t() noexcept {
      write(STDOUT_FILENO, "\033[?1003h", 8);
    }
    ~enable_mouse_t() noexcept {
      write(STDOUT_FILENO, "\033[?1003l", 8);
    }
    enable_mouse_t(const enable_mouse_t&) noexcept = delete;
    enable_mouse_t(enable_mouse_t&&) noexcept = delete;
    const enable_mouse_t& operator=(const enable_mouse_t&) noexcept = delete;
    const enable_mouse_t& operator=(enable_mouse_t&&) noexcept = delete;
  };

  [[nodiscard]] enable_mouse_t enable_mouse_tracking() {
    return {};
  }


  [[nodiscard]] struct cursor_position cursor_position() const {
    (void)this; // This is intentionally not static
    struct cursor_position p = {.col = 0, .row = 0};
    char buf[32];
    ssize_t i = 0; // Number of known characters in the buffer
    write(STDOUT_FILENO, "\033[6n", 4);

    enum class state : char {
      start,
      esc,
      bracket,
      semicolon,
      end
    } state = state::start;
    for (;;) {
      const auto e = read(STDIN_FILENO, buf + i, sizeof(buf) - i);
      if (e < 0) {
        // There was an error
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // But we can recover
          continue;
        } // We can't recover, return an invalid position
        p = {-1, -1};
        goto abort;

      } else if (e == 0) [[unlikely]] {
        // Not enough characters to form a valid response
        p = {-1, -1};
        goto abort;
      } else [[likely]] {
        // We need to extract the response from the buffer
        // The response is of the form \033[<row>;<col>R
        char *const end = buf + i + e;
        char *begin = buf + i;
        if (state == state::start) [[likely]] {
          begin = std::find(begin, end, '\033');
          if (begin == end) {
            // No response yet
            continue;
          }
          state = state::esc;
          if (++begin == end) [[unlikely]] {
            continue;
          }
        }

        // consume the bracket, if not present, return an invalid position
        if (state == state::esc) [[likely]] {
          if (*begin != '[') [[unlikely]] {
            // Not a valid response
            p = {-1, -1};
            goto abort;
          }
          state = state::bracket;
          if (++begin == end) [[unlikely]] {
            continue;
          }
        }

        // consume the row, if not present, return an invalid position
        if (state == state::bracket) [[likely]] {
          for (; begin != end; ++begin) {
            if (isdigit(*begin) != 0) {
              p.row = p.row * 10 + (*begin - '0');
            } else if (*begin == ';') {
              begin++;
              state = state::semicolon;
              break;
            } else [[unlikely]] {
              // Not a valid response
              p = {-1, -1};
              goto abort;
            }
          }
        }

        // consume the column, if not present, return an invalid position
        if (state == state::semicolon) [[likely]] {
          for (; begin != end; ++begin) {
            if (isdigit(*begin) != 0) {
              p.col = p.col * 10 + (*begin - '0');
            } else if (*begin == 'R') {
              state = state::end;
              goto abort;
            } else [[unlikely]] {
              // Not a valid response
              p = {-1, -1};
              goto abort;
            }
          }
        }
      }
    }
  abort:
    return p;
  }

  template <size_t BufSize = 32, int Timeout = 0>
  ::dpsg::generator<char> input_stream() {
    (void)this;
    pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;

    for (;;) {
      auto poll_result = poll(&fds, 1, Timeout);
      if (poll_result == -1) {
        if (errno == EINTR) {
          continue;
        }
        throw errno_exception{};
      }

      if (poll_result == 0) {
        continue;
      }

      int last = 0;
      int current = 0;
      char buffer[BufSize];
      last = read(STDIN_FILENO, buffer, sizeof(buffer));
      if (last == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }

      current = 0;
      while (current < last) {
        char c = buffer[current++];
        co_yield (char) c;
      }
    }

    throw errno_exception{};
  }

  enum class modifiers {
    Ctrl = 1,
    Alt = 2,
  };

  struct event {
    union {
      struct {
        i8 code;
        i8 mods;
      } key;
      struct {
        u16 x;
        u16 y;
        u8 button;
        u8 padding[3];
      } mouse;
    };
  };
  static_assert(sizeof(event) == 8);

  template <size_t BufSize = 32, int Timeout = 0>
  ::dpsg::generator<event> input_events() {
    (void)this;
    pollfd fds;
    fds.fd = STDIN_FILENO;
    fds.events = POLLIN;

    for (;;) {
      auto poll_result = poll(&fds, 1, Timeout);
      if (poll_result == -1) {
        if (errno == EINTR) {
          continue;
        }
        throw errno_exception{};
      }

      if (poll_result == 0) {
        continue;
      }

      int last = 0;
      int current = 0;
      char buffer[BufSize];
      last = read(STDIN_FILENO, buffer, sizeof(buffer));
      if (last == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }

      current = 0;
      while (current < last) {
        char c = buffer[current++];
        co_yield (char) c;
      }
    }

    throw errno_exception{};
  }
};

using raw_mode_context = raw_mode_context_basic<ISIG | ECHO | ICANON>;
using cbreak_mode_context = raw_mode_context_basic<ECHO | ICANON>;

template <std::invocable<raw_mode_context &> F>
std::invoke_result_t<F, raw_mode_context &> with_raw_mode(F &&f) {
  raw_mode_context ctx;
  return f(ctx);
}

template <std::invocable<cbreak_mode_context &> F>
std::invoke_result_t<F, cbreak_mode_context &> with_raw_mode(F &&f) {
  cbreak_mode_context ctx;
  return f(ctx);
}

inline terminal_size get_terminal_size() {
  struct winsize w;
  char *col = getenv("COLUMNS");
  char *row = getenv("LINES");
  if (col != nullptr && row != nullptr) {
    return {std::atoi(col), std::atoi(row)};
  }

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
    return {-1, -1};
  }
  return {w.ws_col, w.ws_row};
}

} // namespace dpsg
