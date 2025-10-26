cmake_minimum_required(VERSION 3.16)
project(nanohft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(NANOHFT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(NANOHFT_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)
    check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
    if(HAS_ASAN)
      add_link_options(-fsanitize=address)
      add_compile_options(-fsanitize=address)
    endif()
    if(HAS_UBSAN)
      add_link_options(-fsanitize=undefined)
      add_compile_options(-fsanitize=undefined)
    endif()
  else()
    add_compile_options(-O3 -DNDEBUG)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-flto" HAS_LTO)
    if(HAS_LTO)
      add_link_options(-flto)
    endif()
    # Best-effort native tuning
    check_cxx_compiler_flag("-march=native" HAS_MARCH_NATIVE)
    if(HAS_MARCH_NATIVE)
      add_compile_options(-march=native)
    endif()
  endif()
endif()

# Embed git short hash as compile-time string
set(NANOHFT_CODE_HASH "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_SHA_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(GIT_SHA_SHORT)
    set(NANOHFT_CODE_HASH ${GIT_SHA_SHORT})
  endif()
endif()
add_compile_definitions(NANOHFT_CODE_HASH="${NANOHFT_CODE_HASH}")

# Targets
# Core library (all sources except main.cpp)
set(NANOHFT_CORE_SOURCES
  src/util.cpp
  src/metrics.cpp
  src/mdfeed.cpp
  src/strategy.cpp
  src/risk.cpp
  src/router.cpp
)

add_library(nanohft_core STATIC ${NANOHFT_CORE_SOURCES})
target_include_directories(nanohft_core PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/third_party)
if(NOT MSVC)
  target_link_libraries(nanohft_core PUBLIC pthread)
endif()

add_executable(nanohft src/main.cpp)
target_link_libraries(nanohft PRIVATE nanohft_core)
target_include_directories(nanohft PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/third_party)

# Tests
add_executable(tests
  tests/test_ringbuf.cpp
  tests/test_risk.cpp
  tests/test_determinism.cpp
)
target_link_libraries(tests PRIVATE nanohft_core)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/third_party)

enable_testing()
add_test(NAME unit COMMAND tests)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
