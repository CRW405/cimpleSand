# CimpleSand

**CimpleSand** is a real-time falling-sand cellular automaton simulation written in pure C that runs entirely in the terminal.

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

Optional flags:

| Flag | Description |
|------|-------------|
| `-w <width>` | Simulation grid width (auto-fits terminal if omitted) |
| `-h <height>` | Simulation grid height (auto-fits terminal if omitted) |
| `-f <fps>`  | Target FPS (default 60; `-f 0` resets to 60; negative = uncapped) |

## Controls

| Input | Action |
|---|---|
| `q` | Quit |
| `1` | Wall |
| `2` | Sand |
| `3` | Water |
| `4` | Wood |
| `5` | Steam |
| `6` | Oil |
| `7` | Gunpowder |
| `Shift+1` (`!`) | Stone |
| `Shift+2` (`@`) | Ash |
| `Shift+3` (`#`) | Lava |
| `Shift+4` (`$`) | Ember |
| `Shift+5` (`%`) | Fire |
| `-` / `_` | Decrease brush size |
| `+` / `=` | Increase brush size |
| Left click / drag | Paint selected material |
| Right click / drag | Erase (paint Empty) |
| Mouse wheel | Cycle selected material |

## Elements (13 types)

| Element | Type | Behavior |
|---|---|---|
| **Wall** | Static | Indestructible barrier |
| **Sand** | Solid | Falls straight down, slides diagonally |
| **Stone** | Solid | Heavy — falls straight down |
| **Gunpowder** | Solid (explosive) | Falls like sand; ignites with chain-reaction explosion when near Fire/Lava |
| **Ash** | Solid | Falls down/diagonal |
| **Water** | Liquid | Falls, spreads laterally (up to 10 cells), evaporates when isolated |
| **Oil** | Liquid | Lighter than water — floats on top; limited lateral spread (1 cell) |
| **Lava** | Liquid | Falls and spreads slowly; turns Water to Steam (solidifying into Stone), ignites Oil/Wood |
| **Fire** | Gas | Rises, drifts, ignites Oil/Wood/Gunpowder, turns Water to Steam, extinguishes randomly |
| **Steam** | Gas | Rises, drifts laterally (up to 5 cells), condenses back to Water |
| **Wood** | Static | Ignites (becomes Ember) when adjacent to Fire/Lava |
| **Ember** | Static | Burning Wood — spawns Fire above, collapses to Ash after ~2 seconds |
| **Empty** | — | Erasable space |

## Technical notes

### Terminal setup

At startup the app switches to the alternate screen buffer, hides the cursor, enables SGR mouse + motion reporting + focus events, and enables raw terminal mode (`ICANON` + `ECHO` disabled). On exit (`q` or `SIGINT`) everything is restored.

### Data layout

- The world is a contiguous `unsigned char *grid`.
- One byte stores: lower 7 bits = element ID, top bit = active marker used during per-frame swaps.
- Elements are declared in `element_registry[]` with name, ANSI colors, density, and simulation function pointer.

### Simulation step

The main loop runs `simulate()` → `render()` → `handle_input()`.

- Processed bottom-up so gravity looks stable.
- Horizontal scan direction alternates every frame to reduce directional bias.
- Movement is density-based — heavier cells displace lighter ones.
- Simulation work is restricted to an active region (`min/max_active_*`) with a one-cell margin instead of scanning the full grid.
- Swaps set active bits on both cells to prevent same-frame double updates; bits are cleared after the region pass.

### Explosion system (Gunpowder)

When ignited, Gunpowder triggers a BFS-based chain reaction:
- Scans outward through adjacent Gunpowder cells up to a configurable limit.
- Blast force is density-scaled and applied radially — nearby cells are destroyed, damaged, or scorched depending on distance.
- Debris (Ember/Fire) is scattered outward from the blast center.

### Rendering

Uses Unicode half-block `▄` to pack two sim cells per terminal character row (top → background, bottom → foreground). For each frame:
- Cursor resets to top (`\e[H`).
- Rows are rasterized into a `frame_buffer` with redundant ANSI color writes skipped by tracking last fg/bg per row.
- A HUD line is appended (FPS, cell count, selected element, brush size, mouse coordinates).
- A single `write()` flushes the full frame buffer.

### Input

Non-blocking (`select()`) in raw mode:
- Single-key bindings for material selection.
- Escape sequence parsing for SGR mouse (`\e[<...M/m`) — supports click/drag painting and scroll wheel.
- Right-click erases (paints Empty).

## Project structure

```
src/
  main.c             # Entry point, grid init, CLI args, terminal lifecycle, frame timing
  sim.c              # Core simulation engine, active-region tracking
  sim.h              # Simulation API
  element.c          # Per-element behavior functions
  element.h          # Element simulation declarations
  element_utils.c    # Movement primitives (fall, flow, rise, drift) and explosion system
  element_utils.h    # Movement utility declarations
  element_registry.c # Static element definitions table (name, color, density, sim_fn)
  render.c           # Frame assembly, ANSI optimization, HUD
  render.h           # Render API
  input.c            # Non-blocking input, keyboard bindings, SGR mouse parsing
  input.h            # Input API
  term_ops.c         # Terminal mode setup/teardown, ANSI helpers
  term_ops.h         # Terminal ops API
  common.h           # Shared constants, types, globals, ANSI codes
```

## Known problems

- **Stripey/glitchy display** — try zooming out your terminal.
- **Water evaporating in a column** — im aware of this and looking into it.
- **Paint/erase stuck** — try clicking erase or paint again to reset state.

## Planned work

### TODO:

- More elements: Acid, Lightning, Life
- Player character
- Multithreading
- Pause / step mode
- GUI improvements: menu system, help screen (`h`), options (`o`)
- Velocity system
- Graphical rendering
- World larger than terminal bounds with camera scrolling
- Save/load worlds
