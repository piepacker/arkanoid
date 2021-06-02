#pragma once

#include <stdint.h>

#include <libretro.h>

void retro_log(enum retro_log_level level, char const * format, ...);

bool environment(unsigned command, void * data);

extern void js_env_controller_info_reset();
extern void js_env_controller_info_set(unsigned port, unsigned id, const char * name_ptr, unsigned name_len);
