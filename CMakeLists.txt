cmake_minimum_required(VERSION 3.10)

project(CimpleSand C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Ofast -march=native")

add_executable(CimpleSand
    src/main.c
    src/term_ops.c
    src/input.c
    src/render.c
    src/sim.c
    src/element.c
    src/element_registry.c
    src/element_utils.c
)

