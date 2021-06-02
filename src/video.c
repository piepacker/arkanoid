#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "./video.h"

void video_refresh(void const * data, unsigned width, unsigned height, size_t pitch) {
    js_video_set_size(width, height, pitch);
    js_video_set_data(data);
}
