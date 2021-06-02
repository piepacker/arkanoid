#include <stdint.h>
#include <stdio.h>

#include "./audio.h"

void audio_sample(int16_t left, int16_t right) {
    int16_t sample[] = { left, right };

    js_audio_push_sample_batch(sample, 1);
}

size_t audio_sample_batch(int16_t const * samples, size_t count) {
    js_audio_push_sample_batch(samples, count);

    return count;
}