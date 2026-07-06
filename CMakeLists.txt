cmake_minimum_required(VERSION 3.10)

project(CimpleSand C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

add_executable(CimpleSand
    src/main.c
    src/term_ops.c
    src/input.c
    src/render.c
)

