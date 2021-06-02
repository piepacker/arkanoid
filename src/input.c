#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <emscripten.h>
#include <libretro.h>

#include "input.h"

void input_poll(void) {
    js_input_poll();
}

int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	// 5 player max
	if (port >= 5)
		return 0;

	if (device == RETRO_DEVICE_JOYPAD) {
		if (index != 0)
			return 0;

		return js_input_state(port, id);
	}
	if (device == RETRO_DEVICE_ANALOG) {
		switch (index) {
			case RETRO_DEVICE_INDEX_ANALOG_LEFT:
				return js_analogL_input_state(port, id);
			case RETRO_DEVICE_INDEX_ANALOG_RIGHT:
				return js_analogR_input_state(port, id);
		}
	}

    return 0;
}

void EMSCRIPTEN_KEEPALIVE input_set_controller_port_device(unsigned port, unsigned device) {
	retro_set_controller_port_device(port, device);
}
