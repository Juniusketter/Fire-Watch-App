
cmake_minimum_required(VERSION 3.16)
project(cpp_starter LANGUAGES CXX)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app src/main.cpp)

if (MSVC)
  target_compile_options(app PRIVATE /W4)
  if (ENABLE_WARNINGS_AS_ERRORS)
    target_compile_options(app PRIVATE /WX)
  endif()
else()
  target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
  if (ENABLE_WARNINGS_AS_ERRORS)
    target_compile_options(app PRIVATE -Werror)
  endif()
endif()

install(TARGETS app RUNTIME DESTINATION bin)
