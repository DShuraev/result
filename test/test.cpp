#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "result.hpp"
#include <doctest/doctest.h>

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace sundry;

struct DummyPOD {
  const int x;
};

constexpr bool operator==(const DummyPOD &lhs, const DummyPOD &rhs) {
  return lhs.x == rhs.x;
}

template<typename OK, typename ERR, auto V, decltype(V) O>
struct ResultArgs {
  using T = OK;
  using E = ERR;
  constexpr static decltype(V) value = V;
  constexpr static decltype(V) other_value = O;
};

template<typename T, typename E, bool F>
constexpr Result<T, E> make_helper(auto value) {
  if constexpr(F) {
    if constexpr(!std::is_void_v<T>)
      return make_ok<T, E>(value);
    else
      return make_ok<T, E>();
  } else {
    if constexpr(!std::is_void_v<E>)
      return make_err<T, E>(value);
    else
      return make_err<T, E>();
  }
}

#define result_essential_ok_args                                               \
  ResultArgs<int, int, 2, -3>, ResultArgs<int, void, 2, -3>,                   \
      ResultArgs<void, int, 2, -3>, ResultArgs<void, void, 2, -3>,             \
      ResultArgs<DummyPOD, int, DummyPOD {2}, DummyPOD {-3}>

SCENARIO_TEMPLATE("Result - essential methods, Ok status", T,
                  result_essential_ok_args) {
  GIVEN("Result with Ok status") {
    using ok_t = typename T::T;
    using err_t = typename T::E;
    auto value = T::value;
    auto other_value = T::other_value;
    auto result = make_helper<ok_t, err_t, true>(value);
    using result_t = decltype(result);
    // Sanity checks
    REQUIRE_UNARY(std::is_same_v<ok_t, typename result_t::ok_value_t>);
    REQUIRE_UNARY(std::is_same_v<err_t, typename result_t::err_value_t>);
    // Actual tests
    WHEN("is_ok() is called") {
      THEN("true is returned") { CHECK_UNARY(result.is_ok()); }
    }
    WHEN("is_err() is called") {
      THEN("false is returned") { CHECK_UNARY_FALSE(result.is_err()); }
    }
    // -----------------Tests for non-void ok_t------------------------
    if constexpr(!result_t::has_void_ok()) {
      WHEN("Ok is non-void") {
        AND_WHEN("`contains(x)` is called") {
          AND_WHEN("`x` has the same value as Ok") {
            THEN("true is returned") { CHECK_UNARY(result.contains(value)); }
          }
          AND_WHEN("`x` has different value than Ok") {
            THEN("false is returned") {
              CHECK_UNARY_FALSE(result.contains(other_value));
            }
          }
        }
        AND_WHEN("`expect(x)` is called") {
          THEN("ok value is returned") {
            CHECK_EQ(value, result.expect("foo"));
          }
        }
        AND_WHEN("`ok()` is called") {
          THEN("return contains a value") {
            auto r = result.ok();
            CHECK_UNARY(r.has_value());
            AND_THEN("return value is correct") { CHECK_EQ(value, r.value()); }
          }
        }
        AND_WHEN("`unwrap()` is called") {
          THEN("ok value is returned") { CHECK_EQ(value, result.unwrap()); }
        }
      }
    }
    // -----------------Tests for non-void err_t-----------------------
    if constexpr(!result_t::has_void_err()) {
      WHEN("Err is non-void") {
        AND_WHEN("`contains_err(x)` is called") {
          THEN("`false` is returned") {
            CHECK_UNARY_FALSE(result.contains_err({}));
          }
        }
        AND_WHEN("`expect_err(x) is called") {
          THEN("`std::runtime_error(x)` is thrown") {
            auto msg = "msg";
            CHECK_THROWS_WITH_AS(result.expect_err(msg), "msg",
                                 const std::runtime_error &);
          }
        }
        AND_WHEN("`err()` is called") {
          THEN("return value is empty") {
            CHECK_UNARY_FALSE(result.err().has_value());
          }
        }
        AND_WHEN("`unwrap_err()` is called") {
          THEN("`std::runtime_error` is thrown") {
            CHECK_THROWS_AS(result.unwrap_err(), const std::runtime_error &);
          }
        }
      }
    }
  }
}

#define result_essential_err_args                                              \
  ResultArgs<int, int, 2, -3>, ResultArgs<int, void, 2, -3>,                   \
      ResultArgs<void, int, 2, -3>, ResultArgs<void, void, 2, -3>,             \
      ResultArgs<int, DummyPOD, DummyPOD {2}, DummyPOD {-3}>

SCENARIO_TEMPLATE("Result - essential methods, Err status", T,
                  result_essential_err_args) {
  GIVEN("Result with Ok status") {
    using ok_t = typename T::T;
    using err_t = typename T::E;
    auto value = T::value;
    auto other_value = T::other_value;
    auto result = make_helper<ok_t, err_t, false>(value);
    using result_t = decltype(result);
    // Sanity checks
    REQUIRE_UNARY(std::is_same_v<ok_t, typename result_t::ok_value_t>);
    REQUIRE_UNARY(std::is_same_v<err_t, typename result_t::err_value_t>);
    // Actual tests
    WHEN("is_ok() is called") {
      THEN("`false` is returned") { CHECK_UNARY_FALSE(result.is_ok()); }
    }
    WHEN("is_err() is called") {
      THEN("`true` is returned") { CHECK_UNARY(result.is_err()); }
    }
    // -----------------Tests for non-void ok_t------------------------
    if constexpr(!result_t::has_void_ok()) {
      WHEN("Ok is non-void") {
        AND_WHEN("`contains(x)` is called") {
          THEN("`false` is returned") {
            CHECK_UNARY_FALSE(result.contains({}));
          }
        }
        AND_WHEN("`expect(x)` is called") {
          THEN("`std::runtime_error(x)` is thrown") {
            auto msg = "msg";
            CHECK_THROWS_WITH_AS(result.expect(msg), msg,
                                 const std::runtime_error &);
          }
        }
        AND_WHEN("`ok()` is called") {
          THEN("return value is empty") {
            CHECK_UNARY_FALSE(result.ok().has_value());
          }
        }
        AND_WHEN("`unwrap()` is called") {
          THEN("`std::runtime_error` is thrown") {
            CHECK_THROWS_AS(result.unwrap(), const std::runtime_error &);
          }
        }
      }
    }
    // -----------------Tests for non-void err_t-----------------------
    if constexpr(!result_t::has_void_err()) {
      WHEN("Err is non-void") {
        AND_WHEN("`contains_err(x)` is called") {
          AND_WHEN("`x` has the same value as `Err`") {
            THEN("`true` is returned") {
              CHECK_UNARY(result.contains_err(value));
            }
          }
          AND_WHEN("`x` has different value than `Err`") {
            THEN("`false` is returned") {
              CHECK_UNARY_FALSE(result.contains_err(other_value));
            }
          }
        }
        AND_WHEN("`expect_err(x) is called") {
          THEN("error value is returned") {
            CHECK_EQ(value, result.expect_err(""));
          }
        }
        AND_WHEN("`err()` is called") {
          THEN("return value is not empty") {
            CHECK_UNARY(result.err().has_value());
            AND_THEN("returned value is correct") {
              CHECK_EQ(result.err().value(), value);
            }
          }
        }
        AND_WHEN("`unwrap_err()` is called") {
          THEN("error value is returned") {
            CHECK_EQ(result.unwrap_err(), value);
          }
        }
      }
    }
  }
}
