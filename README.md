
# A simple sand sim in C for a proof of concept to myself and to improve my C skills

## Notes

### Description

A simple sand sim written in C running in the terminal.

### Features

- Unicode based rendering
- Sand simulation
- Mouse controls, click to place, rclick to erase

### Scratch



#### Rendering

- Get terminal bounds - didnt do this
- Map an arr to the screen - done
- Turn terminal output into a screen like cava or cmatrix - done
- frames // dirty rendering? probably way overkill - done, single buffer
- probably symbols to start with, but could use colors later. - done, did colors
- I want pixels, not rectangles, character shape may interfere - done, using unicode block characters

#### Controls

- Get mouse position on terminal window, map to index in arr - done
- Get click events - done
- Use keys for element selection - done
