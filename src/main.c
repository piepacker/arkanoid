#include <stdio.h>
#include <unistd.h>

#include <libretro.h>

#include "./environment.h"
#include "./video.h"
#include "./input.h"
#include "./audio.h"
#include "./frontend.h"

int main(void) {
    retro_set_environment(environment);
    retro_init();

    retro_set_video_refresh(video_refresh);
    retro_set_input_poll(input_poll);
    retro_set_input_state(input_state);
    retro_set_audio_sample(audio_sample);
    retro_set_audio_sample_batch(audio_sample_batch);

    return 0;
}
