#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emscripten.h>
#include <libretro.h>

#include "./frontend.h"
#include "./video.h"
#include "./audio.h"
#include "./options.h"

static char const * g_state_path = NULL;
static void * g_state = NULL;
static size_t g_state_size = 0;

struct retro_audio_callback audio_callback;
struct retro_frame_time_callback frame_time_callback;
extern retro_time_t retro_get_time_usec();

static struct retro_system_av_info s_avi;

bool paused;

static int read_file(char const * path, void ** data, size_t * size)
{
    FILE * file = fopen(path, "rb");

    if (! file)
        return fprintf(stderr, "Cannot open file %s (%s)\n", path, strerror(errno)), fclose(file), -1;

    fseek(file, 0, SEEK_END);

    *size = ftell(file);
    *data = malloc(*size);

    fseek(file, 0, SEEK_SET);

    if (fread(*data, *size, 1, file) != 1)
        return fprintf(stderr, "Cannot read file %s (%s)\n", path, strerror(ferror(file))), fclose(file), -2;

    return fclose(file), 0;
}

static int write_file(char const * path, void const * data, size_t size)
{
    FILE * file = fopen(path, "wb+");

    if (! file)
        return fprintf(stderr, "Cannot open file %s (%s)\n", path, strerror(errno)), fclose(file), -1;

    if (fwrite(data, size, 1, file) != 1)
        return fprintf(stderr, "Cannot write file %s (%s)\n", path, strerror(ferror(file))), fclose(file), -2;

    return fclose(file), 0;
}

void EMSCRIPTEN_KEEPALIVE frontend_iterate(void)
{
    if (paused)
        return;
    struct retro_system_av_info avi = s_avi;

    double eachframe = 1.0 / avi.timing.fps;

    if (frame_time_callback.callback)
        frame_time_callback.callback(eachframe*1000000.0);

    retro_run();

    js_video_render();

    if (audio_callback.callback) {
        audio_callback.callback();
    }
}

void EMSCRIPTEN_KEEPALIVE frontend_set_paused(bool p) {
    paused = p;
}

void EMSCRIPTEN_KEEPALIVE frontend_loop(float fps) {
    // It is illegal to call twice set_main_loop. Cancel any previous main loop
    emscripten_cancel_main_loop();
    emscripten_set_main_loop(frontend_iterate, fps, 1);
}

static void video_refresh_dummy(void const * data, unsigned width, unsigned height, size_t pitch) {}
static void audio_sample_dummy(int16_t left, int16_t right) {}
static size_t audio_sample_batch_dummy(int16_t const * samples, size_t count) {
    return count;
}

void EMSCRIPTEN_KEEPALIVE frontend_set_fast()
{
    retro_set_video_refresh(video_refresh_dummy);
    retro_set_audio_sample(audio_sample_dummy);
    retro_set_audio_sample_batch(audio_sample_batch_dummy);
}

void EMSCRIPTEN_KEEPALIVE frontend_unset_fast()
{
    retro_set_video_refresh(video_refresh);
    retro_set_audio_sample(audio_sample);
    retro_set_audio_sample_batch(audio_sample_batch);
}

void EMSCRIPTEN_KEEPALIVE frontend_skip_frame(unsigned frames)
{
    printf("[Core] Skipping Frame %u\n", frames);

    frontend_set_fast();

    for (unsigned f = 0; f < frames; f++)
    {
        retro_run();
        if (frame_time_callback.callback)
            frame_time_callback.callback(0);
    }

    frontend_unset_fast();

    printf("[Core] Skipping Frame Done\n");
}

int EMSCRIPTEN_KEEPALIVE frontend_load_game(char const * path)
{
    struct retro_system_info si;
    retro_get_system_info(&si);

    printf("[Core] Name: %s\n", si.library_name);
    printf("[Core] Version: %s\n", si.library_version);
    printf("[Core] Need fullpath: %d\n", si.need_fullpath);

    void * data = NULL;
    size_t size = 0;

    if (!si.need_fullpath) {
        if (read_file(path, &data, &size) < 0)
            return fprintf(stderr, "Cannot read game %s\n", path), -1;
    }

    struct retro_game_info gi = { .path = path, .data = data, .size = size, .meta = "" };
    printf("[Game] Path: %s\n", gi.path);
    printf("[Game] Size: %zu\n", gi.size);

    if (!retro_load_game(&gi))
        return fprintf(stderr, "retro_load_game failed %s\n", gi.path), -2;

    printf("[Core] Game loaded\n");

    retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD);

    g_state_size = retro_serialize_size();
    g_state = malloc(g_state_size);

    struct retro_system_av_info avi;
    retro_get_system_av_info(&avi);
    s_avi = avi;

    printf("[AV Info] FPS: %f\n", avi.timing.fps);
    printf("[AV Info] Sample rate: %f\n", avi.timing.sample_rate);

    js_video_set_fps(avi.timing.fps);
    js_audio_configure(avi.timing.sample_rate, 2);

    if (audio_callback.set_state) {
        audio_callback.set_state(true);
    }

    return 0;
}

int EMSCRIPTEN_KEEPALIVE frontend_unload_game(void)
{
    emscripten_cancel_main_loop();

    retro_unload_game();

    return 0;
}

void EMSCRIPTEN_KEEPALIVE frontend_set_state_path(char const * path)
{
    g_state_path = path;
}

int EMSCRIPTEN_KEEPALIVE frontend_load_state(void)
{
    if (!g_state_path)
        return fprintf(stderr, "Cannot load state - no path specified\n"), -1;

    void * data = NULL;
    size_t size = 0;

    if (read_file(g_state_path, &data, &size) < 0)
        return free(data), -2;

    if (frontend_set_state(data, size) < 0)
        return free(data), -3;

    return free(data), 0;
}

int EMSCRIPTEN_KEEPALIVE frontend_save_state(void)
{
    if (!g_state_path)
        return fprintf(stderr, "Cannot save state - no path specified\n"), -1;

    void const * data = NULL;
    size_t size = 0;

    if (frontend_get_state(&data, &size) < 0)
        return -2;

    if (write_file(g_state_path, data, size) < 0)
        return -3;

    return 0;
}

int EMSCRIPTEN_KEEPALIVE frontend_set_state(void const * state, size_t size)
{
    if (!retro_unserialize(state, size))
        return -1;

    return 0;
}

int EMSCRIPTEN_KEEPALIVE frontend_get_state(void const ** state, size_t * size)
{
    if (!retro_serialize(g_state, g_state_size))
        return -1;

    *state = g_state;
    *size = g_state_size;

    return 0;
}

int EMSCRIPTEN_KEEPALIVE frontend_reset(void)
{
    retro_reset();
    return 0;
}

size_t EMSCRIPTEN_KEEPALIVE frontend_get_memory_size(unsigned id)
{
    return retro_get_memory_size(id);
}

void* EMSCRIPTEN_KEEPALIVE frontend_get_memory_data(unsigned id)
{
    return retro_get_memory_data(id);
}

void EMSCRIPTEN_KEEPALIVE frontend_set_options(char const * key, char const * value)
{
	printf("Set options %s => %s\n", key, value);
	set_option(key, value);
}
