#pragma once

#include <stdbool.h>
#include <stddef.h>

void frontend_loop(float);
void frontend_iterate(void);
void frontend_skip_frame(unsigned frames);
void frontend_set_paused(bool p);

int frontend_load_game(char const * path);
int frontend_unload_game(void);

void frontend_set_state_path(char const * path);

int frontend_load_state(void);
int frontend_save_state(void);

int frontend_set_state(void const * state, size_t size);
int frontend_get_state(void const ** state, size_t * size);
int frontend_reset(void);

void frontend_set_options(char const * key, char const * value);
