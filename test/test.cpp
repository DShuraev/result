#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "result.hpp"
#include <doctest/doctest.h>

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace sundry;

SCENARIO("Ok/Err conversion") {
  GIVEN("compatible types T,U") {
    using T = int;
    using U = float;
    REQUIRE_UNARY(std::is_convertible_v<U, T>);
    auto val = 1.0;
    WHEN("Ok<T> and Ok<U> are formed") {
      auto u = Ok<U> {(U) val};
      auto t = Ok<T> {(T) val};
      THEN("Ok<U> can be converted to Ok<T>") {
        CHECK_EQ(((Ok<T>) u).value, t.value);
      }
      THEN("Ok<U> can be explicitly converted to T") {
        CHECK_EQ((T) u, t.value);
      }
    }
    WHEN("Err<T> and Err<U> are formed") {
      auto u = Err<U> {(U) val};
      auto t = Err<T> {(T) val};
      THEN("Err<U> can be converted to Err<T>") {
        CHECK_EQ(((Err<T>) u).value, t.value);
      }
      THEN("Err<U> can be explicitly converted to T") {
        CHECK_EQ((T) u, t.value);
      }
    }
  }
}

SCENARIO("Ok/Err construction/assignment") {
  GIVEN("value of type `T`") {
    using T = int;
    T value = 1;
    auto ok = Ok<T> {value};
    auto err = Err<T> {value};
    WHEN("calling constructor with lvalue") {
      auto r = Ok<T>(ok);
      auto e = Err<T>(err);
      THEN("default copy constructor is called") {
        CHECK_EQ(r.value, ok.value);
        CHECK_EQ(e.value, err.value);
      }
    }
    WHEN("calling constructor with rvalue") {
      auto r = Ok<T>(std::move(ok));
      auto e = Err<T>(std::move(ok));
      THEN("default move constructor is called") {
        CHECK_EQ(r.value, value);
        CHECK_EQ(e.value, value);
      }
    }
    WHEN("assigning from lvalue") {
      auto r = Ok<T> {};
      r = ok;
      auto e = Err<T> {};
      e = err;
      THEN("default copy assignment operator is called") {
        CHECK_EQ(r.value, ok.value);
        CHECK_EQ(e.value, err.value);
      }
    }
    WHEN("assigning from rvalue") {
      auto r = Ok<T> {};
      r = std::move(ok);
      auto e = Err<T> {};
      e = std::move(err);
      THEN("default copy assignment operator is called") {
        CHECK_EQ(r.value, value);
        CHECK_EQ(e.value, value);
      }
    }
  }
}

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

template<typename OK, typename ERR, auto S, auto F>
struct ConstructorArgs {
  using T = OK;
  using E = ERR;
  constexpr static decltype(S) ok_value = S;
  constexpr static decltype(F) err_value = F;
};

#define result_constructor_args ConstructorArgs<int, int, 1, 2>

SCENARIO_TEMPLATE("Result - constructor/assignment test, same type", T,
                  result_constructor_args) {
  using ok_t = typename T::T;
  using err_t = typename T::E;
  auto ok_v = T::ok_value;
  auto err_v = T::err_value;

  GIVEN("Ok<T>, Err<E>") {
    auto ok = Ok<ok_t> {ok_v};
    auto err = Err<err_t> {err_v};
    WHEN("passing object to constructor as lvalue") {
      auto ok_r = Result<ok_t, err_t>(ok);
      auto err_r = Result<ok_t, err_t>(err);
      THEN("copy constructor is called") {
        CHECK_UNARY(ok_r.contains(ok_v));
        CHECK_UNARY(err_r.contains_err(err_v));
      }
    }
    WHEN("passing object to constructor as rvalue") {
      auto ok_r = Result<ok_t, err_t>(std::move(ok));
      auto err_r = Result<ok_t, err_t>(std::move(err));
      THEN("move constructor is called") {
        CHECK_UNARY(ok_r.contains(ok_v));
        CHECK_UNARY(err_r.contains_err(err_v));
      }
    }
    WHEN("assigning to Result variable from lvalue") {
      auto ok_r = make_ok<ok_t, err_t>({});
      auto err_r = make_err<ok_t, err_t>({});
      REQUIRE_UNARY_FALSE(ok_r.contains(ok_v));
      REQUIRE_UNARY_FALSE(err_r.contains_err(err_v));
      AND_WHEN("ok flag is the same") {
        THEN("copy assignment is called") {
          ok_r = ok;
          CHECK_UNARY(ok_r.contains(ok_v));
          err_r = err;
          CHECK_UNARY(err_r.contains_err(err_v));
        }
      }
      AND_WHEN("ok flag is different") {
        THEN("`std::runtime_error` is throws") {
          CHECK_THROWS_AS(ok_r = err, const std::runtime_error &);
          CHECK_THROWS_AS(err_r = ok, const std::runtime_error &);
        }
      }
    }
    WHEN("assigning to Result variable from rvalue") {
      auto ok_r = make_ok<ok_t, err_t>({});
      auto err_r = make_err<ok_t, err_t>({});
      REQUIRE_UNARY_FALSE(ok_r.contains(ok_v));
      REQUIRE_UNARY_FALSE(err_r.contains_err(err_v));
      AND_WHEN("ok flag is the same") {
        THEN("move assignment is called") {
          ok_r = std::move(ok);
          CHECK_UNARY(ok_r.contains(ok_v));
          err_r = std::move(err);
          CHECK_UNARY(err_r.contains_err(err_v));
        }
      }
      AND_WHEN("ok flag is different") {
        THEN("`std::runtime_error` is thrown") {
          CHECK_THROWS_AS(ok_r = std::move(err), const std::runtime_error &);
          CHECK_THROWS_AS(err_r = std::move(ok), const std::runtime_error &);
        }
      }
    }
  }
  GIVEN("Result<T,E>") {
    auto ok = make_ok<ok_t, err_t>(ok_v);
    auto err = make_err<ok_t, err_t>(err_v);
    REQUIRE_UNARY(ok.contains(ok_v));
    REQUIRE_UNARY(err.contains_err(err_v));
    WHEN("passing object to constructor as lvalue") {
      auto ok_r = Result<ok_t, err_t>(ok);
      auto err_r = Result<ok_t, err_t>(err);
      THEN("copy constructor is called") {
        CHECK_UNARY(ok_r.contains(ok_v));
        CHECK_UNARY(err_r.contains_err(err_v));
      }
    }
    WHEN("passing object to constructor as rvalue") {
      auto ok_r = Result<ok_t, err_t>(std::move(ok));
      auto err_r = Result<ok_t, err_t>(std::move(err));
      THEN("move constructor is called") {
        CHECK_UNARY(ok_r.contains(ok_v));
        CHECK_UNARY(err_r.contains_err(err_v));
      }
    }
    WHEN("assigning to Result lvalue") {
      auto ok_r = make_ok<ok_t, err_t>({});
      auto err_r = make_err<ok_t, err_t>({});
      AND_WHEN("success flag is the same") {
        REQUIRE_EQ(ok.is_ok(), ok_r.is_ok());
        REQUIRE_EQ(err.is_ok(), err_r.is_ok());
        THEN("copy assignment is called") {
          ok_r = ok;
          CHECK_UNARY(ok_r.contains(ok_v));
          err_r = err;
          CHECK_UNARY(err_r.contains_err(err_v));
        }
      }
      AND_WHEN("success flag is different") {
        REQUIRE_NE(err.is_ok(), ok_r.is_ok());
        REQUIRE_NE(ok.is_ok(), err_r.is_ok());
        THEN("`std::runtime_error` is throws") {
          CHECK_THROWS_AS(err_r = ok, const std::runtime_error &);
          CHECK_THROWS_AS(ok_r = err, const std::runtime_error &);
        }
      }
    }
    WHEN("assigning to Result rvalue") {
      auto ok_r = make_ok<ok_t, err_t>({});
      auto err_r = make_err<ok_t, err_t>({});
      AND_WHEN("success flag is the same") {
        REQUIRE_EQ(ok.is_ok(), ok_r.is_ok());
        REQUIRE_EQ(err.is_ok(), err_r.is_ok());
        THEN("copy assignment is called") {
          ok_r = std::move(ok);
          CHECK_UNARY(ok_r.contains(ok_v));
          err_r = std::move(err);
          CHECK_UNARY(err_r.contains_err(err_v));
        }
      }
      AND_WHEN("success flag is different") {
        REQUIRE_NE(err.is_ok(), ok_r.is_ok());
        REQUIRE_NE(ok.is_ok(), err_r.is_ok());
        THEN("`std::runtime_error` is throws") {
          CHECK_THROWS_AS(err_r = std::move(ok), const std::runtime_error &);
          CHECK_THROWS_AS(ok_r = std::move(err), const std::runtime_error &);
        }
      }
    }
  }
}