[![GitHub license](https://img.shields.io/github/license/Naereen/StrapDown.js.svg)](https://github.com/Naereen/StrapDown.js/blob/master/LICENSE)

# About

C++ analog of Rust's `std::result` class.

Currently, under active development.

# Building

`CMakeList.txt` in the root folder has three targets

1. `sundry_result` – library itself
2. `sundry_result_test` – tests
3. `coverage` – coverage of tests.

If you only want to build the library, `GCC-10` and `CMake` are minimum requirements.

If you intend to build tests, you need to initialize `doctest` submodule first with

```shell
git submodule init
```

If you want to build coverage you will need `gcov` and `gcovr` installed on your `PATH`.

# Documentation

To build documentation, install `Docygen`. You might want to install `graphviz` as well.

