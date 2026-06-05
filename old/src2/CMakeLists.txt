cmake_minimum_required(VERSION 3.21)
project(OpenJoey2Src2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(openjoey2_core
  cpp/DeckCore.cpp
)
target_include_directories(openjoey2_core PUBLIC cpp)

add_executable(openjoey2_smoke
  cpp/NativeSmoke.cpp
)
target_link_libraries(openjoey2_smoke PRIVATE openjoey2_core)
