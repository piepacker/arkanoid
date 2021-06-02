#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emscripten.h>
#include <libretro.h>

#include "./video.h"
#include "./options.h"
#include "./environment.h"

extern struct retro_audio_callback audio_callback;
extern struct retro_frame_time_callback frame_time_callback;

void retro_log(enum retro_log_level level, char const * format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

retro_time_t retro_get_time_usec() {
    return emscripten_get_now() * 1000;
}

bool set_controller_info(struct retro_controller_info* info) {
	if (!info)
		return false;

	js_env_controller_info_reset();

	for (int port = 0; port < 16; port++) {
		if (!info->types || !info->num_types)
			break;

		for (unsigned i = 0; i < info->num_types; i++) {
			const struct retro_controller_description d = info->types[i];
			if (d.desc) {
				printf("Set controller info port %d, id %d, name %s\n", port, d.id, d.desc);
				js_env_controller_info_set(port, d.id, d.desc, strlen(d.desc));
			}
		}

		info++;
	}

	return true;
}

bool environment(unsigned command, void * data) {
    switch (command) {

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
        ((struct retro_log_callback *) data)->log = retro_log;
        return true;
    }
	break;

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        const char **dir = (const char**)data;
        *dir = ".";
        return true;
    }
    break;

    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
        ((struct retro_perf_callback *) data)->get_time_usec = retro_get_time_usec;
        return true;
    }
	break;

    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
        const struct retro_frame_time_callback *frame_time_cb =
            (const struct retro_frame_time_callback*)data;
        frame_time_callback = *frame_time_cb;
        return true;
    }
    break;

    case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
        struct retro_audio_callback *audio_cb = (struct retro_audio_callback*)data;
        audio_callback = *audio_cb;
        return true;
    }
    break;

    case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
        *(bool*) data = true;
        return true;
    }
	break;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
        enum retro_pixel_format const * format = (enum retro_pixel_format *) data;
        js_video_set_pixel_format(*format);
        return true;
    }
	break;

	case RETRO_ENVIRONMENT_GET_VARIABLE: {
		struct retro_variable *var = (struct retro_variable*) data;
		return get_option(var->key, &var->value);
	}
	break;

	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
		return set_controller_info((struct retro_controller_info*)data);
	}
	break;

    default:
        return false;
    }
}
