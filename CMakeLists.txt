cmake_minimum_required(VERSION 3.20)
project(cf_compiler_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CF_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# Warnings
if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (CF_WARNINGS_AS_ERRORS)
  if (MSVC)
    add_compile_options(/WX)
  else()
    add_compile_options(-Werror)
  endif()
endif()

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(tests)

enable_testing()
