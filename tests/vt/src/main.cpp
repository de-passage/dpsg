#include <sys/ioctl.h>
#include "vt100.hpp"
#include <algorithm>
#include <bits/types/FILE.h>
#include <cctype>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <type_traits>

int main() {
  return dpsg::vt100::with_raw_mode([](auto &ctx) {
    std::cout << "Position: " << std::flush;
    auto pos = ctx.cursor_position();
    std::cout << pos.col << ", " << pos.row << '\n';
    struct winsize w;
    auto col = getenv("COLUMNS");
    auto row = getenv("LINES");
    std::cout << "COLUMNS = " << (col == nullptr ? "<null>" : col) << '\n';
    std::cout << "LINES = " << (row == nullptr ? "<null>" : row) << '\n';

    auto dim = dpsg::vt100::get_terminal_size();
    std::cout << "Rows: " << dim.row << ", Columns: " << dim.col << std::endl;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        perror("ioctl");
        return 1;
    }

    printf("Rows: %d, Columns: %d\n", w.ws_row, w.ws_col);
    return 0;
  });
}
