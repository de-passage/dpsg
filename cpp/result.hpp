#ifndef DPSG_RESULT_HEADER_GUARD
#define DPSG_RESULT_HEADER_GUARD

// See https://gist.github.com/de-passage/cabea442a4cc21fd3f0ce93e3a6ffbf3
#include "is_template_instance.hpp"
#include <type_traits>
#include <utility>
#include <variant>

/** @file
 *
 * C++20 utilities to handle function results. The aim is to provide an easy
 * to use alternative to exceptions. The main tool provided is a sequence function
 * that allows to chain functions returning variants containing either a
 * success or an error type, interrupting the chain on the first error.
 *
 * This file depends on the [is_template_instance](https://gist.github.com/de-passage/cabea442a4cc21fd3f0ce93e3a6ffbf3)
 * utility.
 *
 * ### Example
 *
 * (doesn't compile, just to give you an idea of the use case)
 *
 * @code
 * constexpr auto do_a = [] () -> result<int, string> {
 *    if (...) {
 *      return 42;
 *    } else {
 *      return "Oops, something went wrong in step A";
 *    }
 * };
 * constexpr auto do_b = [] (int result_from_a) -> result<T, string> {
 *    if (...) {
 *      return T{};
 *    } else {
 *      return "Oops, something went wrong in step B";
 *    }
 * };
 * constexpr auto do_c = [] (T result_from_b) {
 *    cout << "Sequence succeeded" << endl;
 * };
 *
 * error_handler on_error([] (string str) { cerr << str << endl; throw runtime_error(str.c_str()); });
 *
 * sequence(do_a(), error_handler, do_b, do_c);
 * @endcode
 *
 */

namespace dpsg {

/// A function can either succeed and return T or fail and return E
template <class T, class E> using result = std::variant<T, E>;

/** @brief Check whether a result expresses a success.
 *
 *  @details This basically means that the variant contains the 0th alternative
 *
 *  @param[in] r The object to inspect
 *
 *  @return true if the object contains the success value, false if it contains
 *  the error value
 */
template <class T, class U> constexpr bool ok(const result<T, U> &r) noexcept {
  return r.index() == 0;
}

/** @brief Returns the success value contained in a result object.
 *
 * @param[in] r The result object to unpack
 *
 * @throw std::bad_variant_access Throws if the object represents an error
 *
 * @return The success value contained in @p r
 */
constexpr decltype(auto)
get_result(dpsg::template_instance_of<std::variant> auto &&r) {
  return std::get<0>(std::forward<decltype(r)>(r));
}

/** @brief Returns the error value contained in a result object.
 *
 * @param[in] r The result object to unpack
 *
 * @throw std::bad_variant_access Throws if the object represents a success
 *
 * @return The error value contained in @p r
 */
constexpr decltype(auto)
get_error(dpsg::template_instance_of<std::variant> auto &&r) {
  return std::get<1>(std::forward<decltype(r)>(r));
}

/** @brief Apply a function to the content of a result object.
 *
 * @details Depending on the semantic value of the content of the result object,
 * call @p f or @p g with the content as an argument.
 *
 * @param[in] r The result object to unpack
 * @param[in] f The function to call if the content is a success value
 * @param[in] g The function to call if the content is an error value
 *
 * @return The common type of the result types of the invocation of @p f and @g with
 * the success and error values respectively.
 */
template <class R, class F, class G,
          class R1 = std::invoke_result_t<F, std::variant_alternative_t<0, R>>,
          class R2 = std::invoke_result_t<G, std::variant_alternative_t<1, R>>,
          class R0 = std::common_type_t<R1, R2>>
requires dpsg::template_instance_of<R, std::variant>
constexpr R0 either(R &&r, F &&f, G &&g) noexcept {
  if (ok(r)) {
    return std::forward<F>(f)(get_result(std::forward<decltype(r)>(r)));
  } else {
    return std::forward<G>(g)(get_error(std::forward<decltype(r)>(r)));
  }
}

/** @brief A type representing an error handler.
 *
 * @details Used by sequence to make the prototype a bit less confusing,
 * this doesn't add anything functionaly.
 */
template <class F> struct error_handler {
  template <class T>
  requires std::is_constructible_v<F, T> error_handler(T &&func)
      : f_{std::forward<T>(func)} {}

  template <class E>
  requires std::is_invocable_v<F, E>
  constexpr decltype(auto) operator()(E &&error) const
      noexcept(noexcept(f_(std::forward<E>(error)))) {
    return f_(std::forward<E>(error));
  }

private:
  F f_;
};

template <class F> error_handler(F &&) -> error_handler<std::decay_t<F>>;

/// @cond INTERNAL
template <class R, class ErrHandler, class F>
requires dpsg::template_instance_of<R, std::variant> &&
    dpsg::template_instance_of<ErrHandler, error_handler>
decltype(auto) sequence(R &&result, ErrHandler &&error_handler, F &&f) {
  return either(std::forward<R>(result), std::forward<F>(f),
                std::forward<ErrHandler>(error_handler));
}
/// @endcond

/** @brief Run a sequence of operations, interrupting at the first error
 *
 * @details @p f is call with the success value contained in @p result,
 * then every given function @p fs... will be run in order with the success
 * value of the previous call. If at any point (including the initial call
 * with @p result) the current result object contains an error, the error
 * handler (@p error_handler) is called with the error value. This implies
 * that every function must return a result<S, E> where S is a type that the
 * next function can be called with, and E a type that the error handler can
 * be called with.
 *
 * If the error handler or the last function in the sequence return a result,
 * both must return a type that is compatible with each other (as defined by
 * std::common_type).
 *
 * The last function of the sequence doesn't need to return a result object.
 *
 * @param[in] result The initial result object.
 * @param[in] error_handler The error handler, must be callable with the error
 * types of @p result and of any invocation of @p f or @p fs...
 * @param[in] f,fs The sequence of function to call. @p f must be callable with
 * the success value of @p result, and every @p fs must be callable with the
 * success value of the previous call in the sequence.
 *
 * @return The result of the invocation of the error handler if an error occured
 * in the sequence, otherwise the result of the last function call in the sequence.
 */
template <class R, class ErrHandler, class F, class... Fs>
requires dpsg::template_instance_of<R, std::variant> &&
    dpsg::template_instance_of<ErrHandler, error_handler>
decltype(auto) sequence(R &&result, ErrHandler &&error_handler, F &&f, Fs &&...fs) {
  if (!ok(result)) {
    return std::forward<ErrHandler>(error_handler)(
        get_error(std::forward<R>(result)));
  }

  return sequence(std::forward<F>(f)(get_result(std::forward<R>(result))),
                  std::forward<ErrHandler>(error_handler),
                  std::forward<Fs>(fs)...);
}

} // namespace dpsg

#endif // DPSG_RESULT_HEADER_GUARD
