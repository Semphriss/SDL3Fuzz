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

// Configuration options
static Uint32 config_max_delay_ms = -1;

// Current application status
static SDL_Window *target_window;
static int window_w, window_h;
static bool fetched;

static void SDLCALL SDLFuzz_FetchInfo(void *ptr)
{
    int window_count;
    SDL_Window **windows;

    fetched = false;

    windows = SDL_GetWindows(&window_count);

    if (!windows || !window_count) {
        return;
    }

    target_window = windows[SDL_rand(window_count)];
    SDL_free(windows);

    if (!SDL_GetWindowSize(target_window, &window_w, &window_h)) {
        return;
    }

    fetched = true;
}

static int SDLCALL SDLFuzz_RunThread(void *arg)
{
    SDL_Event e;
    const char *envval;
    Uint64 seed = SDL_GetPerformanceCounter();
    int loglevel = 0;

    // Wait for the app to start
    while (!SDL_WasInit(SDL_INIT_EVENTS)) {
        SDL_Delay(10);
    }

    if ((envval = SDL_getenv("SDLFUZZ_SEED"))) {
        seed = (Uint64) SDL_atoi(envval);
    }

    if ((envval = SDL_getenv("SDLFUZZ_MAX_DELAY_MS"))) {
        config_max_delay_ms = (Uint32) SDL_atoi(envval);
    }

    if ((envval = SDL_getenv("SDLFUZZ_LOGLEVEL"))) {
        loglevel = SDL_atoi(envval);
    }

    if (loglevel > 0) {
        SDL_Log("Seed: %" SDL_PRIu64 "\n", seed);
    }

    while (SDL_WasInit(SDL_INIT_EVENTS)) {
        if (!SDL_RunOnMainThread(SDLFuzz_FetchInfo, NULL, true) || !fetched) {
            continue;
        }

        switch (SDL_rand_r(&seed, 3)) {
        case 0:
            {
                int button = SDL_rand_r(&seed, 5) + 1;
                SDL_SendMouseButton(SDL_GetTicksNS(), target_window,
                                    SDL_GLOBAL_MOUSE_ID, button, true);
                SDL_SendMouseButton(SDL_GetTicksNS(), target_window,
                                    SDL_GLOBAL_MOUSE_ID, button, false);
            }
            break;

        case 1:
            SDL_SendMouseMotion(SDL_GetTicksNS(), target_window,
                                SDL_GLOBAL_MOUSE_ID, false,
                                SDL_rand_r(&seed, window_w),
                                SDL_rand_r(&seed, window_h));
            break;

        case 2:
            {
                int rawcode = SDL_rand_r(&seed, 256);
                SDL_Scancode scancode = SDL_rand_r(&seed, 100);
                if (SDL_rand_r(&seed, 2)) {
                    SDL_SendKeyboardKey(SDL_GetTicksNS(), SDL_GLOBAL_KEYBOARD_ID,
                                        rawcode, scancode, SDL_rand_r(&seed, 2));
                } else {
                    SDL_SendKeyboardKey(SDL_GetTicksNS(), SDL_GLOBAL_KEYBOARD_ID,
                                        rawcode, scancode, true);
                    SDL_SendKeyboardKey(SDL_GetTicksNS(), SDL_GLOBAL_KEYBOARD_ID,
                                        rawcode, scancode, false);
                }
            }
            break;
        }

        SDL_Delay(SDL_min((Uint32) (10.0f / SDL_randf_r(&seed)), config_max_delay_ms));
    }

    return 0;
}

__attribute__((constructor))
static void SDLFuzz_Init(void)
{
    // TODO: Check if it is safe to create threads before SDL_Init()
    SDL_Thread *thread = SDL_CreateThread(SDLFuzz_RunThread, "SDLFuzz", NULL);
    SDL_DetachThread(thread);
}
