#pragma once

#include <concepts>
#include <functional>
#include <iostream>  // needed for Printable<_>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace sundry {  // namespace sundry
  /**
   * @brief Concept ensuring that a given type can be pushed to
   * `std::stringstream`.
   *
   * @tparam `T` type to be pushed to the stream.
   */
  template<typename T>
  concept Printable = requires(T value) {
    std::cout << value;  // TODO: problem - this requires cout object
  };

  /**
   * @brief Wrapper around okay value.
   *
   * @tparam T Type of stored value.
   */
  template<typename T>
  struct Ok {
    using value_t = T;  ///< Alias for type of stored value.
    T value;            ///< Stored value.

    template<typename U,
             typename = std::enable_if_t<std::equality_comparable_with<T, U>>>
    bool operator==(const Ok<U> &other) const noexcept {
      return value == other.value;
    }
  };

  /**
   * @brief Wrapped aroud ok with void value.
   */
  template<>
  struct Ok<void> {
    using value_t = void;
    Ok() = default;
  };

  constexpr bool operator==(const Ok<void> &lhs, const Ok<void> &rhs) {
    return true;
  }

  template<typename T>
  constexpr bool operator==(const Ok<T> &lhs, const Ok<void>) {
    return false;
  }

  /**
   * @brief Wrapper around error value.
   *
   * @tparam T Type of stored value.
   */
  template<typename T>
  struct Err {
    using value_t = T;
    T value;

    template<typename U,
             typename = std::enable_if_t<std::equality_comparable_with<T, U>>>
    bool operator==(const Err<U> &other) const noexcept {
      return value == other.value;
    }
  };

  /**
   * @brief Wrapper arround error with void value.
   */
  template<>
  struct Err<void> {
    using value_t = void;
    Err() = default;
  };

  constexpr bool operator==(const Err<void> &lhs, const Err<void> &rhs) {
    return true;
  }

  template<typename T>
  constexpr bool operator==(const Err<T> &lhs, const Err<void>) {
    return false;
  }

  /**
   * @brief Monadic result type.
   *
   * @tparam T Ok value type.
   * @tparam E Error value type.
   */
  template<typename T, typename E>
  struct Result {
    using ok_t = Ok<T>;     ///< Alias for `Ok<T>`.
    using err_t = Err<E>;   ///< Alias for `Err<E>`.
    using ok_value_t = T;   ///< Alias for type stored in `T`.
    using err_value_t = E;  ///< Alias for type stored in `E`.

    union {
      Ok<T> ok_value_;
      Err<E> err_value_;
    };
    const bool ok_flag_;  ///< Status flag. `true` if result contains `Ok`,
                          ///< `false` if `Err`.
    // TODO: add constructors
    // NOTE: consider placement new
    // Result(Ok<T> value) : ok_flag_(true), ok_value_(value) {}
    // Result(Err<E> value) : ok_flag_(false), err_value_(value) {}
    Result(const Ok<T> &value) : ok_flag_(true), ok_value_(value) {}
    Result(const Err<E> &value) : ok_flag_(false), err_value_(value) {}
    Result(Ok<T> &&value) : ok_flag_(true), ok_value_(value) {}
    Result(Err<E> &&value) : ok_flag_(false), err_value_(value) {}
    Result(const Result<T, E> &other) : ok_flag_(other.ok_flag_) {
      if(ok_flag_)
        ok_value_ = other.ok_value_;
      else
        err_value_ = other.err_value_;
    }

    Result(Result<T, E> &&other) : ok_flag_(ok_flag_) {
      if(ok_flag_)
        ok_value_ = std::move(other.ok_value_);
      else
        err_value_ = std::move(other.err_value_);
    }

    Result<T, E> &operator=(const Ok<T> &other) {
      if(!is_ok())
        throw std::runtime_error(
            "Attempt to assign `Ok` value to `Result` with error flag set.");
      ok_value_ = other;
      return *this;
    }

    Result<T, E> &operator=(const Err<E> &other) {
      if(!is_err())
        throw std::runtime_error(
            "Attempt to assign `Err` value to `Result` with error flag set.");
      err_value_ = other;
      return *this;
    }

    Result<T, E> &operator=(Ok<T> &&other) {
      if(!is_ok())
        throw std::runtime_error(
            "Attempt to assign `Ok` value to `Result` with error flag set.");
      ok_value_ = std::move(other);
      return *this;
    }

    Result<T, E> &operator=(Err<E> &&other) {
      if(!is_err())
        throw std::runtime_error(
            "Attempt to assign `Err` value to `Result` with error flag set.");
      err_value_ = std::move(other);
      return *this;
    }

    // Warning: this can throw
    Result<T, E> &operator=(const Result<T, E> &other) {
      if(this != &other) {
        if(is_ok() != other.is_ok()) {
          throw std::runtime_error(
              "Assigned Result has success flag which is different from "
              "the variable it is being assigned to.");
        }
        if(is_ok())
          ok_value_ = (Ok<T>) other.ok_value_;
        else
          err_value_ = (Err<E>) other.err_value_;
      }
      return *this;
    }

    Result<T, E> &operator=(Result<T, E> &&other) {
      if(is_ok() != other.is_ok()) {
        throw std::runtime_error(
            "Assigned Result has success flag which is different from "
            "the variable it is being assigned to.");
      }
      if(is_ok())
        ok_value_ = (Ok<T>) std::move(other.ok_value_);
      else
        err_value_ = (Err<E>) std::move(other.err_value_);
      return *this;
    }

    /**
     * @brief Checks if `Ok` contains void.
     *
     * @return `true` if `T` is void.
     * @return `false` if `T` is non-void.
     */
    static constexpr bool has_void_ok() { return std::is_void_v<T>; }

    /**
     * @brief Checks if `Err` contains void.
     *
     * @return `true` if `E` is void.
     * @return `false` if `E` is non-void.
     */
    static constexpr bool has_void_err() { return std::is_void_v<E>; }

    /**
     * @brief Checks if result contains `Ok`.
     *
     * @return `true` if result contains `Ok`.
     * @return `false` if result contains `Err`.
     */
    bool is_ok() const noexcept { return ok_flag_; }

    /**
     * @brief Checks result contains `Err`.
     *
     * @return `true` if result contains `Err`.
     * @return `false` if result contains `Ok`.
     */
    bool is_err() const noexcept { return !ok_flag_; }

    /**
     * @brief Checks if contents of `Ok` are equal to provided value.
     *
     * @tparam U type of compared value.
     * @param[in] value value to compare `Ok.value` to.
     * @return `true` if contents of `Ok` are equal to `value`
     * @return `false` if result contains `Err` or `value` is not equal to
     * contents of `Ok`.
     */
    // TODO: add equally comparable
    template<typename U = T>
    bool contains(const U &value) {
      return is_ok() && ok_value_.value == value;
    }

    /**
     * @brief Checks if contents of `Err` are equal to provided value.
     *
     * @tparam U type of compared value.
     * @param[in] value value to compare `Err.value` to.
     * @return `true` if contents of `Err` are equal to `param` value
     * @return `false` if result contains `Ok` or `value` is not equal to
     * contents of `Err`.
     */
    // TODO: add equally comparable
    template<typename U = E>
    bool contains_err(const U &value) {
      return is_err() && err_value_.value == value;
    }

    /**
     * @brief Attempts to return value contained inside`Ok`. Throws exception on
     * failure with  \p arg as an argument.
     *
     * @tparam U error contents type.
     * @param[in] arg content of `std::runtime_error`.
     * @return `T` value contained inside `Ok`; `void` if `Ok` contains `void`.
     * @throw `std::runtime_error` with \p arg value if result has contains
     * `Err`.
     */
    template<typename U>
    T expect(U &&arg) const {
      if(is_err()) throw std::runtime_error(std::forward<U>(arg));
      if constexpr(!has_void_ok()) return ok_value_.value;
      else
        return;
    }

    /**
     * @brief Attempts to return value contained inside `Err`. Throws expection
     * on failure with \p arg as an argument.
     *
     * @tparam `U` error contents type.
     * @param[in] arg content of `std::runtime_error`.
     * @return `E` value contained inside `Err`; `void` if `Err` contains
     * `void`.
     * @throw `std::runtime_error` with \p arg value if result contains `Ok`.
     */
    template<typename U>
    E expect_err(U &&arg) const {
      if(is_ok()) throw std::runtime_error(std::forward<U>(arg));
      if constexpr(!has_void_err())
        return err_value_.value;
      else
        return;
    }

    /**
     * @brief Returns `std::optional` with contents of `Ok`. Returns empty
     * option if result contains `Err`.
     *
     * @tparam `U` `std::optional` contained type.
     * @note \p U is required for type deduction to avoid substitution failure
     * when `T` is `void`. Shall not be supplied by user.
     * @return `std::optional<U>` with `Ok` contents if result contains `Ok`,
     * empty if result contains `Err`.
     */
    template<typename U = T>
    std::optional<U> ok() const noexcept {
      if(is_ok())
        return std::optional<U>(ok_value_.value);
      return std::optional<U>();
    }

    /**
     * @brief Returns `std::optional` with contents of `Err`. Returns empty
     * option if result contains `Ok`.
     *
     * @tparam `U` `std::optional` contained type.
     * @note \p U is required for type deduction to avoid substitution failure
     * when `E` is `void`. Shall not be supplied by user.
     * @return `std::optional<U>` with `Err` contents if result contains `Err`,
     * empty if result contains `Ok`.
     */
    template<typename U = E>
    std::optional<U> err() const noexcept {
      if(is_err())
        return std::optional<U>(err_value_.value);
      return std::optional<U>();
    }

    /**
     * @brief Attempts to return contents of `Ok`. Throws exception on failure.
     *
     * @return `T` contents of `Ok`. `void` if `Ok` contains `void`.
     * @throws `std::runtime_error` with relevant error message if result
     * contains `Err`.
     */
    T unwrap() const {
      if(is_ok()) return ok_value_.value;
      std::stringstream err_msg;
      err_msg << "called `Result::unwrap()` on `Err` value";
      if constexpr(Printable<E>)
        err_msg << ' ' << err_value_.value;
      err_msg << std::endl;
      throw std::runtime_error(err_msg.str());
    }

    /**
     * @brief Attempts to return contents of `Err`. Throws exception on failure.
     *
     * @return `E` contents of `Err`. `void` if `Err` contains `void`.
     * @throws `std::runtime_error` with relevant error message if result
     * contains `Ok`.
     */
    E unwrap_err() const {
      if(is_err()) return err_value_.value;
      std::stringstream err_msg;
      err_msg << "called `Result::unwrap_err()` on `Ok` value";
      if constexpr(Printable<T>) {
        err_msg << ' ' << ok_value_.value;
      }
      err_msg << std::endl;
      throw std::runtime_error(err_msg.str());
    }

    template<typename F, typename V>
    using RType = typename std::invoke_result<F, V>::type;

    template<typename F>
    auto map(F const &func) const {
      using Res = RType<F, T>;
      if(!ok_flag_) return make_err<Res, E>(unwrap_err());
      return make_ok<Res, E>(func(unwrap()));
    }

    template<typename F>
    decltype(auto) map_or(RType<F, T> val, F const &func) const {
      if(!ok_flag_) return val;
      return func(unwrap());
    }

    template<typename D, typename F>
    decltype(auto) map_or_else(D const &func, F const &fallback) const {
      if(ok_flag_) return func(unwrap());
      return fallback(unwrap_err());
    }  // TODO: add consistency check for return of

    template<typename F>
    auto map_err(F const &func) const {
      using Res = RType<F, E>;
      if(ok_flag_) return make_ok<T, Res>(unwrap());
      return make_err<T, Res>(func(unwrap_err()));
    }

    template<typename F>
    auto map_err_or(RType<F, E> val, F const &func) const {
      if(ok_flag_) return val;
      return func(unwrap_err());
    }
  };

  template<typename T, typename E>
  Result<T, E> make_ok(const T &value) {
    return {Ok(value)};
  }

  template<typename T, typename E>
  Result<void, E> make_ok() {
    return {Ok<void> {}};
  }

  template<typename T, typename E>
  Result<T, E> make_err(const E &value) {
    return {Err(value)};
  }

  template<typename T, typename E>
  Result<T, E> make_err() {
    return {Err<void> {}};
  }
}  // namespace sundry
