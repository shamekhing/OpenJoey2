cmake_minimum_required(VERSION 3.21)
project(OpenJoey2 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(openjoey_core STATIC
  src/cpp/WasmApi.cpp
)

target_include_directories(openjoey_core PUBLIC
  src/cpp
  src/cpp/third_party
)

add_executable(OpenJoey2
  src/cpp/NativeSmoke.cpp
)

target_link_libraries(OpenJoey2 PRIVATE openjoey_core)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(openjoey_core PRIVATE
    -O0 -g3 -ggdb
    -fno-omit-frame-pointer
    -fno-inline
    -fno-optimize-sibling-calls
    -Wall -Wextra -Wpedantic
  )
  target_compile_options(OpenJoey2 PRIVATE
    -O0 -g3 -ggdb
    -fno-omit-frame-pointer
    -fno-inline
    -fno-optimize-sibling-calls
    -Wall -Wextra -Wpedantic
  )
endif()
