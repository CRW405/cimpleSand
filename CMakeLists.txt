cmake_minimum_required(VERSION 3.10)

project(CimpleSand C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Ofast -march=native")
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)

add_executable(CimpleSand
    src/main.c
    src/term_ops.c
    src/input.c
    src/player.c
    src/render.c
    src/sim.c
    src/history.c
    src/element.c
    src/element_registry.c
    src/element_utils.c
    src/raylib_mode.c
)

target_include_directories(CimpleSand PRIVATE ${RAYLIB_INCLUDE_DIRS})
target_link_libraries(CimpleSand PRIVATE ${RAYLIB_LIBRARIES})
