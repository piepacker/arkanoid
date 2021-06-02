#pragma once

#include <stdint.h>

#include <libretro.h>

void video_refresh(void const * data, unsigned width, unsigned height, size_t pitch);

void js_video_set_pixel_format(unsigned format);
void js_video_set_size(unsigned width, unsigned height, unsigned pitch);
void js_video_set_data(void const * data);
void js_video_set_fps(float fps);
void js_video_render(void);
