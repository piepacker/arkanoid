#pragma once
#include <stdint.h>

void input_poll(void);
int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
void input_set_controller_port_device(unsigned port, unsigned device);

void js_input_poll(void);
bool js_input_state(unsigned port, unsigned id);
int16_t js_analogL_input_state(unsigned port, unsigned id);
int16_t js_analogR_input_state(unsigned port, unsigned id);
