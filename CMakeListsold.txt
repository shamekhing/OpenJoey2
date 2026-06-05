cmake_minimum_required(VERSION 3.21)
project(OpenJoey2 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Raylib via FetchContent (reuse local copy if available) ──────────────────
include(FetchContent)
set(OPENJOEY_LOCAL_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps")

if(EXISTS "${OPENJOEY_LOCAL_DEPS}/raylib-src/CMakeLists.txt")
  message(STATUS "[openjoey] Using local raylib: ${OPENJOEY_LOCAL_DEPS}/raylib-src")
  FetchContent_Declare(raylib SOURCE_DIR "${OPENJOEY_LOCAL_DEPS}/raylib-src")
else()
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
  )
endif()
FetchContent_MakeAvailable(raylib)

# ── Sources ──────────────────────────────────────────────────────────────────
add_executable(OpenJoey2
  src/main.cpp
)

target_include_directories(OpenJoey2 PRIVATE src src/third_party)
target_link_libraries(OpenJoey2 PRIVATE raylib)

add_custom_command(TARGET OpenJoey2 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:OpenJoey2>/data
  COMMENT "Symlinking data/ into build dir"
)

# ── Debug helpers ─────────────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Line-by-line debugging enforced: no optimization, no inlining, full debug info.
  target_compile_options(OpenJoey2 PRIVATE 
    -O0 -g3 -ggdb 
    -fno-omit-frame-pointer 
    -fno-inline 
    -fno-optimize-sibling-calls 
    -Wall -Wextra -Wpedantic)
endif()