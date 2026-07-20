#ifndef ELEMENT_UTILS_H
#define ELEMENT_UTILS_H

#include "common.h"

bool try_fall_down(int x, int y, char element_id);
bool try_fall_diagonal(int x, int y, char element_id);
bool try_liquid_flow(int x, int y, char element_id, int flow_limit);
bool try_liquid_evaporation(int x, int y, char element_id, int chance);
bool try_rise_up(int x, int y, char element_id);
bool try_rise_diagonal(int x, int y, char element_id);
bool try_gas_drift(int x, int y, char element_id, int drift_limit);
bool try_gas_flow(int x, int y, char element_id, int flow_limit);

void trigger_explosion(int start_x, int start_y, int depth);
void explode_at(int cx, int cy, int radius, int power, int depth);

#endif
