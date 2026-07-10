# CimpleSand

**CimpleSand** is a real-time falling-sand simulation written in pure C that runs entirely in the terminal.

## Description

This is just a fun project for me to get better with C by tackling an idea I've had floating around in my head for a while.

Simple falling sand sim being rendered by a custom made Unicode based rendering engine that uses Unicode escape sequences on supporting terminal emulators.

## Build and run

### Requirements

- C compiler (GCC/Clang)
- CMake 3.10+
- Linux/macOS terminal with ANSI + mouse reporting support. (I used kitty to develop this)
    - Not sure how you would / if you can run this on windows but maybe try WSL?

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/CimpleSand
```

Optional grid size:

```bash
./build/CimpleSand -w 100 -h 100 # or some n. The actual width is clamped from 1 to the screen width or height
```

Optional FPS target:

```bash
./build/CimpleSand -f 120 # will affect simulation speed. Set to a negative number for uncapped, 0 sets to default
```

If `-w` / `-h` are omitted, the simulation now auto-fits to terminal bounds.

## Controls

| Input | Action |
|---|---|
| `q` | Quit |
| `1` | Select Wall |
| `2` | Select Sand |
| `3` | Select Water |
| `4` | Select Stone |
| `-` / `_` | Decrease brush size |
| `+` / `=` | Increase brush size |
| Left click / drag | Paint selected material |
| Right click / drag | Erase (paint Empty) |

## Rendering engine notes

The renderer maps two simulation cells into one terminal character row using the Unicode lower-half block (`▄`):

- top simulation cell -> terminal **background**
- bottom simulation cell -> terminal **foreground**

This gives a compact, pixel-like output while keeping writes sequential and cache-friendly. Each frame is assembled into a single `frame_buffer` and flushed in one print pass to reduce terminal I/O overhead and flicker.

## Simulation model

- Grid stored as a contiguous `unsigned char` buffer (`grid`)
- Elements defined in a global `element_registry` (name, colors, density, sim function)
- Movement uses density checks (`can_displace`) and cell swaps
- Bottom-up update order to produce stable falling behavior

## Project structure

```text
src/
  main.c       # main loop, element registry, simulation behaviors
  render.c     # frame assembly + terminal rasterization + HUD
  input.c      # keyboard and SGR mouse parsing, painting tools
  term_ops.c   # terminal mode setup/teardown and ANSI op helpers
  common.h     # shared constants, globals, types, escape codes
```

## Issues

## Notes

### Planned Improvements

- Smart Simulating, simulate only active areas
- Better / faster water settling
- More elements: Fire, Steam, Lava, Life
- Dirty rendering

### TODO:

- Optimize the render hot path (`render.c`) by reducing per-cell formatting/output overhead.
- Optimize simulation inner loops (`simulate` / `sim_*`) to cut repeated cell access and bounds-check cost.
