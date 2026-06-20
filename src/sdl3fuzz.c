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
#include <stdio.h>

#ifndef SDL_PLATFORM_LINUX
#error SDL3Fuzz currently only works on Linux.
#endif

int main(int argc, char **argv)
{
    char buffer[32];
    const char *out = NULL;
    int i;
    int test_i = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: sdl3-fuzz -o OUTPUT_DIR -- EXEC [ARGS...]\n");
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "'-o' requires an argument\n");
                return 1;
            }

            out = argv[i];
        } else if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        } else {
            fprintf(stderr, "Unknown option '%s', use '--' after options\n", argv[i]);
            return 1;
        }
    }

    if (i >= argc) {
        fprintf(stderr, "Missing executable path\n");
        return 1;
    }

    if (!out) {
        fprintf(stderr, "Missing input/output directory\n");
        return 1;
    }

    if (!SDL_CreateDirectory(out)) {
        fprintf(stderr, "Error creating directory '%s': %s\n", out, SDL_GetError());
        return 1;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv + i);
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_NULL);

    SDL_setenv_unsafe("LD_PRELOAD", LIB_PATH, 1);
    SDL_setenv_unsafe("SDLFUZZ_EVENTS_PER_FRAME", "10", 0);
    SDL_setenv_unsafe("SDLFUZZ_RANDOM_EVENTS", "10000", 0);

    SDL_snprintf(buffer, sizeof(buffer), "%s/0", out);
    for (;;) {
        SDL_setenv_unsafe("SDLFUZZ_OUT", buffer, 1);

        SDL_Process *process = SDL_CreateProcessWithProperties(props);

        if (!process) {
            fprintf(stderr, "Error executing '%s': %s\n", argv[i], SDL_GetError());
            return 1;
        }

        int code;
        SDL_WaitProcess(process, true, &code);

        if (code < 0) {
            ++test_i;
            SDL_Log("Test case %d exited with signal %d.\n", test_i, -code);
            SDL_snprintf(buffer, sizeof(buffer), "%s/%d", out, test_i);
        }
    }
}
