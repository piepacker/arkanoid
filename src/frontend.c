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

static int s_reset_limiter_timing = 1;
static double s_audio_prefill_latency = 0.045;


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

// iterate each vsync - only calling retro_run at the rate matching the libretro core's specified
// video fps. retro_run() is conditionally invoked based on current time vs. expected vsync time.
void EMSCRIPTEN_KEEPALIVE frontend_iterate(void)
{
    if (paused)
        return;

    // FPS information should be collected every frame, as some emulators can mode-change at runtime.
    // Unfortunately some emus (blastem) have side-effects for calling retro_get_system_av_info() which
    // makes it unsafe to call at runtime, for now. Solutions are either fix emulators or make it optional
    // to call it per-frame just for emulators that need it. --jstine 20210206
    struct retro_system_av_info avi = s_avi;

    static int s_force_vsync = 0;

    static double last_expected_frame_time = -100000.0;   // default value to ensure audio_prefill_fail_guess test fails
    double eachframe = 1.0 / avi.timing.fps;
    double current   = emscripten_get_now() / 1000.0;
    double expected  = last_expected_frame_time;

    //printf( "avi.timing.fps = %f\n", avi.timing.fps );
    //printf( "[PACING] current=%f  force=%d\n", current, s_force_vsync );

    // Check to see if the pacing system has failed, eg. the audio system's buffers have been drained.
    // Typical causes of this condition:
    //  - the browser process hung for some extended period of time
    //  - user changed windows/tabs and browser doesn't have robust JS suspend/resume handling
    //  - misbehaving JS in our domain that takes too long to run
    //  - non-standard sub-60hz monitor physical refresh rate (ex: 59.94hz will cause audio drain errors
    //    every few minutes, 30hz refresh will cause immediate and continuous failure, see above for
    //    more info on why we can't do anything about it)

    double audio_prefill_fail_guess = s_audio_prefill_latency + (eachframe/2);
    if ((current - expected) >= audio_prefill_fail_guess) {
        // TODO: add JS callback to allow JS frontend to do its own accounting and handling of audio buffer failures.
        //js_on_audiobuf_failure();
        s_reset_limiter_timing = 1;
        printf( "[PACING] forced reset: current=%f expected=%f\n", current, expected);
    }

    if (s_reset_limiter_timing) {
        expected = current - s_audio_prefill_latency;
        s_reset_limiter_timing = 0;

        if (s_force_vsync) {
            // in this case the previous reset failed before the prefill finished
            // this probably means the user's monitor refresh rate is too low. I found that
            // setting up a smaller forced vsync helps (this also varies by browser, if they
            // have two or three fbo's in their WebGL backend)
            s_force_vsync = 2;
        }
        else {
            s_force_vsync = 3;
        }
    }

    if (s_force_vsync && (current < expected)) {
        s_force_vsync = 0;      // prefill is complete.
    }

    if(s_force_vsync < 0) {
        s_force_vsync = 0;
    }

    // Pacing Goals Summary:
    //  - run libretro core only if we've passed the expected frame time criteria.
    //  - run libretro core multiple times if necessary to solve problem of low-vsync displays, for example a
    //    display running at 30hz or a display running at 59.94hz (some older CRTs on VGA connectors). It can
    //    also compensate for cases when the browser might miss some vsync(s) due to internal javascript mono-
    //    thread wobbliness.

    int threshold = 3;      // allow only this many invocations of retro_run per vsync.
    while (current >= expected && (--threshold >= 0)) {
        // TODO: add JS callback to allow tracking and graphing emulator fps, and also so that JS can optionally 
        //   set a flag to render a vsync tool based on emux_fps rather than host browser fps.
        //js_on_retro_run(current, expected);

        if (frame_time_callback.callback) {
            // frame_time aka "deltaTime" ...
            // In a vsync paced system the delta time should always be assumed to be a monotick increment based on host vsync.
            // This reflects the cadence at which frames are actually issued to the user. Calls to retro_run() may occur in
            // bursts of 2 or 3 at a time to fill the fbo queue in the GPU backend, but each frame will be presented over time
            // according to vsync timing. Therefore each frame should be treated by our native engines as if they are being
            // rendered according to the time that *will* have passed when the frame is finally presented to the user. This
            // will produce consistently smoother output animations and smarter behavior during prediction rollback and replay.
            //
            // *unfortunately* WebGL doesn't provide us information about the host vsync, making it impossible to know what
            // the actual delivery time of our rendered frames will be to the user (ugh). Our two options are to pass the
            // delta time of calls to retro_run (likely jittery, especially thanks to spectre mitigation interfering with now()
            // result), or give up and just lock our refresh rate to 60hz. Luckily for Piepacker, none of our games are delta-
            // time based. (lutro supports LoVE API's getDelta() but it's always expected to return 1/60th second).

            // note: according to libretro docs, frame_time_callback is expected to be invoked BEFORE retro_run().
            frame_time_callback.callback(eachframe*1000000.0);
        }

        retro_run();
        expected += eachframe;

        if (s_force_vsync > 0) {
            // to avoid conflicting with the audio prefill period, which is meant to adhere to vsync
            break;
        }
    }

    if (threshold < 2) {
        //printf( "[SKIP] skipped frames = %d\n", 2 - threshold);
    }

    // the vsync force is meant to ensure the host vsync queue is filled, which gets submitted regardless if we retro_run() or not.
    s_force_vsync -= (s_force_vsync > 0);

    last_expected_frame_time = expected;

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

void EMSCRIPTEN_KEEPALIVE frontend_skip_frame(unsigned frames)
{
    printf("[Core] Skipping Frame %u\n", frames);

    retro_set_video_refresh(video_refresh_dummy);
    retro_set_audio_sample(audio_sample_dummy);
    retro_set_audio_sample_batch(audio_sample_batch_dummy);

    for (unsigned f = 0; f < frames; f++)
    {
        retro_run();
        if (frame_time_callback.callback)
            frame_time_callback.callback(0);
    }

    retro_set_video_refresh(video_refresh);
    retro_set_audio_sample(audio_sample);
    retro_set_audio_sample_batch(audio_sample_batch);

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
    s_reset_limiter_timing = 1;
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
