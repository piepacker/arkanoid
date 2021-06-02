#pragma once

#include <stdbool.h>
#include <stdint.h>

void audio_sample(int16_t left, int16_t right);
size_t audio_sample_batch(int16_t const * samples, size_t count);

void js_audio_configure(unsigned rate, unsigned channels);
void js_audio_push_sample_batch(int16_t const * samples, unsigned count);
