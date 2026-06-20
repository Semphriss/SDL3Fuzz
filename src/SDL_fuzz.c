/*
  SDL3Fuzz - A fuzzer for SDL3
  Copyright (C) 2026 Semphris <semphris@protonmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <SDL3/SDL.h>

// Internal SDL data
#define SDL_GLOBAL_MOUSE_ID 0
#define SDL_DEFAULT_MOUSE_ID 1
#define SDL_GLOBAL_KEYBOARD_ID 0
#define SDL_DEFAULT_KEYBOARD_ID 1

static size_t config_events_per_frame = 1;
static SDL_IOStream *config_events_in = NULL;
static SDL_IOStream *config_events_out = NULL;
static size_t config_events_in_limit = (size_t) -1;
static size_t config_events_random_events = (size_t) -1;

typedef enum {
  SDLFUZZ_EVENT_END_FRAME = 0,
  SDLFUZZ_EVENT_MOUSE_BUTTON,
  SDLFUZZ_EVENT_MOUSE_MOTION,
  SDLFUZZ_EVENT_KEYBOARD,
  SDLFUZZ_EVENT_MAX = SDLFUZZ_EVENT_KEYBOARD,
} SDLFuzz_EventType;

typedef struct {
  SDLFuzz_EventType type;
  int window;
  union {
    struct {
      int button;
    } button;
    struct {
      float x;
      float y;
    } motion;
    struct {
      int rawcode;
      SDL_Scancode scancode;
      bool down;
      bool up;
    } key;
    char padding[24];
  };
} SDLFuzz_Event;

static bool SDLFuzz_MakeRandomEvent(SDLFuzz_Event *event, Uint64 *seed)
{
    int window_count;
    SDL_Window **windows;
    int target_window;

    windows = SDL_GetWindows(&window_count);

    if (!windows || window_count <= 0) {
        return false;
    }

    target_window = SDL_rand_r(seed, window_count);

    SDLFuzz_EventType type = SDL_rand_r(seed, ((Uint32) SDLFUZZ_EVENT_MAX)) + 1;

    SDL_zerop(event);
    event->type = type;
    event->window = target_window;

    switch (type) {
    case SDLFUZZ_EVENT_MOUSE_BUTTON:
        event->button.button = SDL_rand_r(seed, 5) + 1;
        return true;

    case SDLFUZZ_EVENT_MOUSE_MOTION:
        event->motion.x = SDL_randf_r(seed);
        event->motion.y = SDL_randf_r(seed);
        return true;

    case SDLFUZZ_EVENT_KEYBOARD:
        event->key.rawcode = SDL_rand_r(seed, 256);
        event->key.scancode = SDL_rand_r(seed, 100);
        switch (SDL_rand_r(seed, 3)) {
        case 0:
            event->key.down = true;
            event->key.up = false;
            break;

        case 1:
            event->key.down = false;
            event->key.up = true;
            break;

        case 2:
            event->key.down = true;
            event->key.up = true;
            break;
        }
        return true;

    default:
        return false;
    }
}

static bool SDLFuzz_PlayEvent(SDLFuzz_Event *event)
{
    int window_count;
    SDL_Window **windows;
    SDL_Window *target_window;
    int width, height;

    windows = SDL_GetWindows(&window_count);

    if (!windows || window_count <= 0) {
        return false;
    }

    if (event->window >= window_count) {
        SDL_Log("[SDL3Fuzz] Inconsistency: event for window #%d with only %d windows\n",
                event->window, window_count);
        return false;
    }

    target_window = windows[event->window];

    if (!SDL_GetWindowSize(target_window, &width, &height)) {
        return false;
    }

    switch ((SDLFuzz_EventType) event->type) {
    case SDLFUZZ_EVENT_MOUSE_BUTTON:
        {
            int button = event->button.button;
            SDL_SendMouseButton(SDL_GetTicksNS(), target_window,
                                SDL_GLOBAL_MOUSE_ID, button, true);
            SDL_SendMouseButton(SDL_GetTicksNS(), target_window,
                                SDL_GLOBAL_MOUSE_ID, button, false);
        }
        break;

    case SDLFUZZ_EVENT_MOUSE_MOTION:
        SDL_SendMouseMotion(SDL_GetTicksNS(), target_window,
                            SDL_GLOBAL_MOUSE_ID, false,
                            width * event->motion.x,
                            height * event->motion.y);
        break;

    case SDLFUZZ_EVENT_KEYBOARD:
        {
            int rawcode = event->key.rawcode;
            SDL_Scancode scancode = event->key.scancode;

            if (event->key.down) {
                SDL_SendKeyboardKey(SDL_GetTicksNS(), SDL_GLOBAL_KEYBOARD_ID,
                                    rawcode, scancode, true);
            }

            if (event->key.up) {
                SDL_SendKeyboardKey(SDL_GetTicksNS(), SDL_GLOBAL_KEYBOARD_ID,
                                    rawcode, scancode, false);
            }
        }
        break;

    default:
        return false;
    }

    return true;
}

static void SDLFuzz_RunFrame(void *arg)
{
    Uint64 *seed = arg;
    SDLFuzz_Event event;
    SDL_zero(event);

    while (config_events_in && config_events_in_limit) {
        --config_events_in_limit;

        if (SDL_ReadIO(config_events_in, &event, sizeof(event)) != sizeof(event)) {
            SDL_CloseIO(config_events_in);
            config_events_in = NULL;
            break;
        }

        if (config_events_out) {
            SDL_WriteIO(config_events_out, &event, sizeof(event));
            SDL_FlushIO(config_events_out);
        }

        if (event.type == SDLFUZZ_EVENT_END_FRAME) {
            return;
        }

        SDLFuzz_PlayEvent(&event);
    }

    for (size_t i = 0; i < config_events_per_frame; i++) {
        if (config_events_random_events) {
            --config_events_random_events;
            while (!SDLFuzz_MakeRandomEvent(&event, seed))
                ;

            if (config_events_out) {
                SDL_WriteIO(config_events_out, &event, sizeof(event));
                SDL_FlushIO(config_events_out);
            }

            SDLFuzz_PlayEvent(&event);
        } else {
            SDL_Event e;
            e.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&e);
            return;
        }
    }

    if (config_events_out) {
        SDL_zero(event);
        event.type = SDLFUZZ_EVENT_END_FRAME;
        SDL_WriteIO(config_events_out, &event, sizeof(event));
        SDL_FlushIO(config_events_out);
    }
}

static int SDLCALL SDLFuzz_RunThread(void *arg)
{
    SDL_Event e;
    const char *envval;
    Uint64 seed = SDL_GetPerformanceCounter();
    int loglevel = 0;

    if ((envval = SDL_getenv("SDLFUZZ_SEED"))) {
        seed = (Uint64) SDL_strtoull(envval, NULL, 0);
    }

    if ((envval = SDL_getenv("SDLFUZZ_EVENTS_PER_FRAME"))) {
        config_events_per_frame = (Uint32) SDL_atoi(envval);
        if (!config_events_per_frame || config_events_per_frame > 1000000) {
            SDL_Log("[SDL3Fuzz] Invalid events per frame, defaulting to 1\n");
            config_events_per_frame = 1;
        }
    }

    if ((envval = SDL_getenv("SDLFUZZ_LOGLEVEL"))) {
        loglevel = SDL_atoi(envval);
    }

    if ((envval = SDL_getenv("SDLFUZZ_IN"))) {
        config_events_in = SDL_IOFromFile(envval, "rb");
        if (!config_events_in) {
            SDL_Log("[SDL3Fuzz] Couldn't open '%s': %s\n", envval, SDL_GetError());
        } else if ((envval = SDL_getenv("SDLFUZZ_IN_LIMIT"))) {
            config_events_in_limit = SDL_strtoull(envval, NULL, 0);
            if (!config_events_in_limit) {
                SDL_Log("[SDL3Fuzz] Invalid event input limit, defaulting to all\n");
            }
        }
    }

    if ((envval = SDL_getenv("SDLFUZZ_OUT"))) {
        config_events_out = SDL_IOFromFile(envval, "wb");
        if (!config_events_out) {
            SDL_Log("[SDL3Fuzz] Couldn't open '%s': %s\n", envval, SDL_GetError());
        }
    }

    if ((envval = SDL_getenv("SDLFUZZ_RANDOM_EVENTS"))) {
        config_events_random_events = (size_t) SDL_strtoull(envval, NULL, 0);
    }

    if (loglevel >= 1) {
        SDL_Log("[SDL3Fuzz] Seed: %" SDL_PRIu64 "\n", seed);
    }

    while (!SDL_WasInit(SDL_INIT_EVENTS)) {
        SDL_Delay(0);
    }

    while (SDL_WasInit(SDL_INIT_EVENTS)) {
        SDL_RunOnMainThread(SDLFuzz_RunFrame, &seed, true);
        SDL_Delay(0);
    }
}

__attribute__((constructor))
static void SDLFuzz_Init(void)
{
    // TODO: Check if it is safe to create threads before SDL_Init()
    SDL_Thread *thread = SDL_CreateThread(SDLFuzz_RunThread, "SDLFuzz", NULL);
    SDL_DetachThread(thread);
}
