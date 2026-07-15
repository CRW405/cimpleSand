# CimpleSand

**CimpleSand** is a real-time falling-sand simulation written in pure C that runs entirely in the terminal.

## Description

This is a fun project for me to get better with C by building an idea I have wanted to make for a while.

The main highlight is the technical side: custom terminal rendering, raw input handling (including SGR mouse), and a density-based cellular sim loop tuned to run at high FPS in a normal terminal.

Now has turned into a side project and "proper" C and optimization practice.

## Build and run

### Requirements

- C compiler (GCC/Clang)
- CMake 3.10+
- Linux/macOS terminal with ANSI escape + SGR mouse reporting support (developed in kitty)
  - Windows might work via WSL + a compatible terminal

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/CimpleSand
```

Optional simulation size:

```bash
./build/CimpleSand -w 100 -h 100
```

- If `-w` / `-h` are omitted, the sim auto-fits terminal bounds.
- If you pass values larger than terminal bounds, they are clamped down to fit.

Optional FPS target:

```bash
./build/CimpleSand -f 120
```

- `-f 0` resets to default (`240`)
- Negative values effectively run uncapped (no sleep throttle)

## Controls

| Input | Action |
|---|---|
| `q` | Quit |
| `1` | Select Wall |
| `2` | Select Sand |
| `3` | Select Water |
| `4` | Select Stone |
| `5` | Select Oil |
| `-` / `_` | Decrease brush size |
| `+` / `=` | Increase brush size |
| Left click / drag | Paint selected material |
| Right click / drag | Erase (paint Empty) |

## Technical notes (how it works)

### 1. Terminal setup and lifecycle

At startup the app:

- switches to the alternate screen buffer
- hides the cursor
- enables SGR mouse + mouse motion reporting + focus events
- enables raw terminal mode (`ICANON` and `ECHO` disabled)

On exit (`q` or `SIGINT`), terminal state is restored (cursor, main screen, modes, termios).

### 2. Data layout

- The world is one contiguous `unsigned char *grid`.
- One byte stores:
  - lower 7 bits: element id
  - top bit: active marker used during per-frame swaps
- Elements are declared in `element_registry[]` with:
  - display name
  - ANSI fg/bg colors (plus cached string lengths)
  - density
  - optional simulation function pointer

This keeps lookups cheap and makes element behavior table-driven.

### 3. Simulation step

The main loop does: `simulate()` -> `render()` -> `handle_input()`.

Core simulation behavior:

- processed bottom-up (`y = height - 1` to `0`) so gravity looks stable
- horizontal scan direction alternates every frame to reduce directional bias
- movement is density-based using precomputed `cell_densities[]`
- simulation work is restricted to an active region (`min/max_active_*`) with a
  one-cell margin, instead of scanning the full grid every frame
- swaps set active bits on both cells to prevent same-frame double updates
- active bits are cleared after the region pass

Current element rules:

- **Sand**: falls down, then tries diagonal down-left/down-right
- **Stone**: heavy solid that currently only falls straight down
- **Water**:
  - falls down, then tries diagonals
  - lateral spread search (up to 10 cells) for pooling behavior
  - isolated cells can evaporate randomly (simple anti-stranding behavior)

### 4. Rendering pipeline

Rendering uses the Unicode half block `▄` and packs two sim cells into one terminal character row:

- top sim cell -> terminal background color
- bottom sim cell -> terminal foreground color

For each frame:

- cursor is reset to top (`\e[H`)
- rows are rasterized into one large `frame_buffer`
- redundant ANSI color writes are skipped by tracking last fg/bg per row
- a HUD line is appended (FPS, selected element, brush size, mouse info)
- one `write()` flushes the full frame buffer to stdout

This reduces terminal I/O calls and keeps flicker low.

### 5. Input handling

Input is non-blocking (`select`) in raw mode:

- single-key controls for tools/materials
- escape sequence parsing for arrows/mouse
- SGR mouse parsing (`\e[<...M/m`) for button press/release + drag painting

Mouse coordinates are mapped from terminal rows to sim rows (`sim_y = (mouse_y - 1) * 2`) to match the half-block render model.

## Project structure

```text
src/
  main.c       # entrypoint, grid init, terminal lifecycle, frame timing
  sim.c        # element registry, simulation rules, active-region stepping
  sim.h        # simulation API and state bounds
  render.c     # frame assembly, ANSI optimization, HUD output
  input.c      # non-blocking key handling + SGR mouse parsing/painting
  term_ops.c   # terminal mode setup/teardown and escape helpers
  common.h     # shared constants, globals, types, escape codes
```

## Problems
- Screen stripey / glitchy / broken.
    - Try zooming out your terminal.
- Water in a column is slowly evaporating away when it shouldnt.
    - Im aware of this and am looking into it.

## Notes / planned work

- Add more elements (life)
- Tweak liquids, find a better way to handle surface settling
- *Future* Velocity
- *Future* Graphical rendering
