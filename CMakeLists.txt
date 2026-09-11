cmake_minimum_required(VERSION 3.21)

# ── Build type: explicit Debug default (single-config generators) ────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
project(OpenJoey2 VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Sanitizers (opt-in): -DOPENJOEY_SANITIZE=address,undefined ───────────────
set(OPENJOEY_SANITIZE "" CACHE STRING "Sanitizers, e.g. address,undefined")
if(OPENJOEY_SANITIZE)
  add_compile_options(-fsanitize=${OPENJOEY_SANITIZE} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${OPENJOEY_SANITIZE})
endif()

# ── nlohmann/json: package when available, else fetch (same pin everywhere) ──
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(json)
endif()

# ── openjoey_headers — the one namespace-mirrored include tree (include/) ────
add_library(openjoey_headers INTERFACE)
target_compile_features(openjoey_headers INTERFACE cxx_std_17)
target_include_directories(openjoey_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(openjoey_headers INTERFACE nlohmann_json::nlohmann_json)
add_library(openjoey::headers ALIAS openjoey_headers)

add_library(openjoey_cards
  src/cards/Cards.cpp
  src/cards/CardDatabase.cpp
  src/cards/CardEnums.cpp
  src/cards/CardParser.cpp
  src/cards/JsonUtils.cpp)
target_compile_features(openjoey_cards PUBLIC cxx_std_17)
target_include_directories(openjoey_cards PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(openjoey_cards PUBLIC openjoey::headers)
add_library(openjoey::cards ALIAS openjoey_cards)

enable_testing()

# ── Tests: core (Config + ActionId pins), cards, engine ──────────────────────
set(OPENJOEY_TEST_FLAGS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
  "$<$<CXX_COMPILER_ID:MSVC>:/W4>")

add_executable(openjoey_core_tests tests/core.cpp)
target_include_directories(openjoey_core_tests PRIVATE tests/catch2)
target_compile_options(openjoey_core_tests PRIVATE ${OPENJOEY_TEST_FLAGS})
target_link_libraries(openjoey_core_tests PRIVATE openjoey::headers openjoey::cards)
add_test(NAME openjoey_core_tests COMMAND openjoey_core_tests)

add_executable(openjoey_cards_tests tests/cards/cards.cpp)
target_include_directories(openjoey_cards_tests PRIVATE tests/catch2)
target_compile_options(openjoey_cards_tests PRIVATE ${OPENJOEY_TEST_FLAGS})
target_link_libraries(openjoey_cards_tests PRIVATE openjoey::headers openjoey::cards)
add_test(NAME openjoey_cards_tests COMMAND openjoey_cards_tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/cards)

add_executable(openjoey_engine_tests
  tests/engine/main.cpp
  tests/engine/test_actions.cpp
  tests/engine/test_battle.cpp
  tests/engine/test_cards.cpp
  tests/engine/test_catalog.cpp
  tests/engine/test_chains.cpp
  tests/engine/test_config.cpp
  tests/engine/test_duel.cpp
  tests/engine/test_moves.cpp
  tests/engine/test_summons.cpp
  tests/engine/test_turn.cpp
  tests/engine/test_zones.cpp)
target_include_directories(openjoey_engine_tests PRIVATE tests/catch2)
target_compile_options(openjoey_engine_tests PRIVATE ${OPENJOEY_TEST_FLAGS})
target_link_libraries(openjoey_engine_tests PRIVATE openjoey::headers openjoey::cards)
add_test(NAME openjoey_engine_tests COMMAND openjoey_engine_tests)

# ── App: raylib UI (FetchContent, pinned 5.5; web build via emcmake) ─────────
# OPENJOEY_HEADLESS=ON skips the raylib UI entirely: the headless test targets
# need no GUI toolchain (CI uses this to avoid X11-only configure failures).
option(OPENJOEY_HEADLESS "Build only headless targets (no raylib UI)" OFF)
if(NOT OPENJOEY_HEADLESS)
  include(FetchContent)
  if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
  endif()
  set(OPENJOY_LOCAL_DEPS "${CMAKE_BINARY_DIR}/_deps" CACHE PATH
      "Dir containing raylib-src (reuse an existing FetchContent checkout)")
  if(EXISTS "${OPENJOY_LOCAL_DEPS}/raylib-src/CMakeLists.txt")
    message(STATUS "[openjoey] Using local raylib: ${OPENJOY_LOCAL_DEPS}/raylib-src")
    FetchContent_Declare(raylib SOURCE_DIR "${OPENJOY_LOCAL_DEPS}/raylib-src")
  else()
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5)
  endif()
  FetchContent_MakeAvailable(raylib)

  add_executable(OpenJoey2 src/main.cpp)
  target_compile_options(OpenJoey2 PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W4>")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN)
    target_compile_options(OpenJoey2 PRIVATE -g -O0)
  endif()
  target_link_libraries(OpenJoey2 PRIVATE openjoey::headers openjoey::cards raylib)

  if(EMSCRIPTEN)
    set(OPENJOEY_WEB_OUT "${CMAKE_BINARY_DIR}/web" CACHE PATH "Web build output dir")
    # Emit .js (no emcc-generated HTML — we ship our own shell in web/index.html).
    set_target_properties(OpenJoey2 PROPERTIES
      SUFFIX ".js"
      RUNTIME_OUTPUT_DIRECTORY "${OPENJOEY_WEB_OUT}")
    target_link_options(OpenJoey2 PRIVATE
      -sALLOW_MEMORY_GROWTH=1
      -sFORCE_FILESYSTEM=1
      -sEXIT_RUNTIME=1
      "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/data@/data"
      -sENVIRONMENT=web)
    add_custom_command(TARGET OpenJoey2 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html" "${OPENJOEY_WEB_OUT}/index.html"
      COMMENT "Copying web shell")
  else()
    # Content: data/ beside the executable (paths resolve relative to argv[0]).
    add_custom_command(TARGET OpenJoey2 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:OpenJoey2>/data"
      COMMENT "Symlinking data/ into the build dir")
  endif()
endif()