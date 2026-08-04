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
| `-p` | Enable playable character |
| `--step` | Enable pause/step mode (`p` to pause, `[`/`]` to scrub time) |
| `--memory <n>` | Past frames retained in `--step` mode (default 100) |

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
| `8` | Acid |
| `9` | Lightning |
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
| Shift + mouse wheel | Adjust brush size |
| `A` / `D` | Move player left / right |
| `W` | Jump (grounded/airborne); swim up while in a liquid |
| Space | Jump |
| `S` | Swim down while in a liquid (no effect on dry ground) |
| Shift + `A`/`D` | Sprint |
| `R` | Respawn player at the top of the screen |
| `p` | Pause/Resume simulation (`--step` mode) |
| `[` | Step back one frame (`--step` mode, while paused) |
| `]` | Step forward one frame (`--step` mode, while paused) |

## Elements (15 types)

| Element | Type | Behavior |
|---|---|---|
| **Wall** | Static | Barrier |
| **Sand** | Solid | Falls straight down, slides diagonally |
| **Stone** | Solid | Heavy — falls straight down |
| **Gunpowder** | Solid (explosive) | Falls like sand; ignites with chain-reaction explosion when near Fire/Lava |
| **Ash** | Solid | Falls down/diagonal, created from burning wood |
| **Water** | Liquid | Falls, spreads laterally (up to 10 cells), evaporates when isolated |
| **Oil** | Liquid | Lighter than water — floats on top |
| **Lava** | Liquid | Falls and spreads slowly; turns Water to Steam (solidifying into Stone), ignites Oil/Wood |
| **Acid** | Liquid | Falls/flows like a liquid and dissolves adjacent materials; denser materials resist it and wear the acid out faster (tune `acid_power` inside `sim_acid()` in `element.c` |
| **Fire** | Gas | Rises, drifts, ignites Oil/Wood/Gunpowder, turns Water to Steam, extinguishes randomly |
| **Steam** | Gas | Rises, drifts laterally (up to 5 cells), condenses back to Water |
| **Lightning** | Gas | Bolts straight down in a jagged pattern; immediately dissipates on impact, igniting Wood/Oil/Gunpowder and steaming Water. Its path is a pure function of its spawn cell, so bolts from the same point always retrace the same pattern |
| **Wood** | Static | Ignites (becomes Ember) when adjacent to Fire/Lava |
| **Ember** | Static | Burning Wood — spawns Fire above, collapses to Ash after ~2 seconds |
| **Empty** | — | Empty space |

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

### Player

- Each element carries a `category` (solid/liquid/gas/empty) independent of density, used purely for player collision: solids block movement and provide footing, liquids are swimmable (no collision, but W/S swim up/down and gravity is replaced by direct vertical control), gases are passed through freely.
- Movement/gravity/jumping run on an integer fixed-point accumulator (matching the rest of the sim's non-dt, per-tick model) so sub-cell speeds still move smoothly frame to frame.
- Jump count refills to its max whenever a cell directly below the player's feet is solid *at the start of a tick* (checked before that tick's own jump can consume a charge) — this is what gives double/triple jumping (tunable `MAX_JUMPS` in `player.c`).
- Horizontal movement auto-steps up onto a solid bump up to `MAX_STEP_HEIGHT` cells tall instead of blocking outright, so slopes/staircases built from elements are walkable; taller ledges still require a jump. Tune `MAX_STEP_HEIGHT` in `player.c` to taste.
- `R` clears the player's spawn cells before placing them, so respawning always frees a buried player rather than reburying them.
- The player isn't part of the grid — it's overlaid onto the rendered frame by position, so it never displaces or gets displaced by simulated terrain outside of normal collision checks.

### Pause / step mode (`--step`)

`--step` turns on a ring-buffer of world snapshots so you can freeze the sim and scrub through time:

- `p` toggles pause. While paused the sim and player are frozen; painting still works.
- `]` advances one frame — restores a saved "future" snapshot if you've rewound, otherwise runs one real sim step and snapshots it.
- `[` rewinds one frame to the previous snapshot.
- History is capped at `--memory <n>` frames (default 100); the newest frames evict the oldest once full, and rewound "futures" survive until pushed out by new frames.
- Editing the world while viewing a rewound frame (or resuming from one) invalidates every snapshot after it — the next steps run the sim forward from the changed state instead of restoring stale frames that would clobber your edit.
- Each snapshot stores the full grid plus the player state, and restoring one recomputes the cell count and resets the active region so the next step re-simulates correctly.

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

#### Player movement input

Plain terminal bytes have no key-up event and Shift+letter just changes the byte — there's no way to know a movement key is *still* held from raw bytes alone. Player input requires the [Kitty keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/): at startup the app unconditionally pushes disambiguate + report-events + report-all-keys + report-text flags (`\e[>27u`), so every key gets real press/repeat/release reporting with its resulting text — held state (and Shift, tracked per-key for sprint) is exact, with no timeout/decay heuristics. Non-movement keys decoded from these events are re-dispatched through the same single-character handling a plain byte would have used, so every other control (quit, material selection, brush size) works unchanged. On a terminal without support, movement/jump/sprint/respawn simply won't respond (mouse painting and single-key material bindings are unaffected, since those aren't gated on this protocol). The flags are popped (`\e[<u`) on exit.

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
  input.c            # Non-blocking input, keyboard bindings, SGR mouse parsing, Kitty keyboard protocol
  input.h            # Input API
  player.c           # Player physics, collision, respawn
  player.h           # Player API
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

- More elements: Life, dirt, mud, metal
- Rework explosions to account for thin lines of explosives
- Multithreading
- GUI improvements: menu system, help screen (`h`), options (`o`)
- Velocity system
- Graphical rendering
- World larger than terminal bounds with camera scrolling
- Save/load worlds
