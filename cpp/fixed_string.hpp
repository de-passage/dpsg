#ifndef HEADER_GUARD_DPSG_FIXED_STRING_HPP
#define HEADER_GUARD_DPSG_FIXED_STRING_HPP

#include <cstddef>
namespace dpsg {

namespace meta {
template <size_t N> struct fixed_string {
  using array_type = char[N];
  using const_array = const array_type;
  using const_array_ref = const_array &;
  array_type data;
  constexpr fixed_string(const char (&s)[N]) {
    for (int i = 0; i < N; ++i) {
      data[i] = s[i];
    }
  }

  constexpr operator const_array_ref() const { return data; }

  constexpr char operator[](size_t i) const { return data[i]; }
};
template <size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

template <size_t N, size_t M>
  requires(N != M)
constexpr bool operator==(fixed_string<N> s1, fixed_string<M> s2) {
  return false;
}

template <size_t N>
constexpr bool operator==(fixed_string<N> s1, fixed_string<N> s2) {
  for (int i = 0; i < N; ++i) {
    if (s1[i] != s2[i]) {
      return false;
    }
  }
  return true;
}

template <size_t S1, size_t S2>
constexpr bool operator!=(fixed_string<S1> s1, fixed_string<S2> s2) {
  return !(s1 == s2);
}

template <size_t N1>
constexpr bool operator==(const char *s1, const fixed_string<N1> &s2) {
  for (int i = 0; i < N1; ++i) {
    if (s1[i] == '\0' && i != N1 - 1) {
      return false;
    }
    if (s1[i] != s2[i]) {
      return false;
    }
  }
  return true;
}

template <size_t N1, size_t N2>
constexpr fixed_string<N1 + N2 - 1> operator+(const char (&s1)[N1],
                                              const fixed_string<N2> &s2) {
  return fixed_string<N1>{s1} + s2;
}

template <size_t N1, size_t N2>
constexpr fixed_string<N1 + N2 - 1> operator+(const fixed_string<N1> &s1,
                                              const char (&s2)[N2]) {
  return s1 + fixed_string<N2>{s2};
}

template <size_t N1, size_t N2>
constexpr fixed_string<N1 + N2 - 1> operator+(const fixed_string<N1> &s1,
                                              const fixed_string<N2> &s2) {
  char res[N1 + N2 - 1];
  for (int i = 0; i < N1 - 1; ++i) {
    res[i] = s1[i];
  }
  for (int i = 0; i < N2; ++i) {
    res[i + N1 - 1] = s2[i];
  }
  return fixed_string{res};
}

template <size_t N> constexpr size_t size([[maybe_unused]] fixed_string<N>) {
  return N;
}

constexpr static inline meta::fixed_string s = "abc";
constexpr static inline meta::fixed_string s2 = "def";
static_assert(s == "abc");
static_assert(size(s) == 4);
static_assert(s != "acb");
static_assert(s != "ab");
static_assert(s2 == "def");
static_assert(s2 != "dfe");
static_assert(size(s2) == 4);
static_assert(size(s + s2) == 7);
static_assert(s + s2 == "abcdef");
} // namespace meta

namespace literals {
template <::dpsg::meta::fixed_string S> consteval auto operator""_fs() {
  return S;
}
} // namespace literals
} // namespace dpsg

#endif // HEADER_GUARD_DPSG_FIXED_STRING_HPP
