cmake_minimum_required(VERSION 3.10)
project(sundry_result CXX)

set(CMAKE_CXX_STANDARD 20)
message(CMAKE_CXX_COMPILER_VERSION)

add_library(${PROJECT_NAME} src/result.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
add_library(sundry::result ALIAS ${PROJECT_NAME})

# add_compile_options("--coverage")

add_subdirectory(lib/doctest)
set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
add_executable(${PROJECT_TEST_NAME} test/test.cpp)
target_link_libraries(${PROJECT_NAME}_test PUBLIC ${PROJECT_NAME} doctest)
target_include_directories(${PROJECT_NAME} PUBLIC "src")

# Code Coverage
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/lib/cmake-modules)
if(CMAKE_COMPILER_IS_GNUCXX)
    include(${CMAKE_MODULE_PATH}/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    set(COVERAGE_EXCLUDES "lib/*")
    # disable optimization
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    setup_target_for_coverage_gcovr_html(NAME coverage EXECUTABLE ${PROJECT_TEST_NAME} DEPENDENCIES ${PROJECT_NAME} ${PROJECT_TEST_NAME})
endif()