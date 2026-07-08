#ifndef TERM_OPS_H
#define TERM_OPS_H
#include "common.h"

void term_op(int op_count, bool flush, ...);
void init_screen(void);
void reset_term(void);
Termios enable_raw_term(void);
bool get_terminal_bounds(int *width, int *height);

#endif
