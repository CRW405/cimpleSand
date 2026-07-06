#ifndef RENDER_H
#define RENDER_H
#include "common.h"

char *get_cell_color(int cell);
char *get_cell_bg_color(int cell);
char *random_color(void);
char *random_bg_color(void);
char *gui(void);
void render(void);

#endif
