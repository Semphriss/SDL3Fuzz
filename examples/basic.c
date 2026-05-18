#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Beware of flashing lights! This application will flash bright colors at a
// high frequency when fuzzing.

int main(int argc, char **argv)
{
    char R = 0, G = 0, B = 0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *w = SDL_CreateWindow("Basic example", 640, 480, 0);
    SDL_Renderer *r = SDL_CreateRenderer(w, NULL);

    SDL_SetRenderDrawColor(r, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(r);
    SDL_RenderPresent(r);

    int mode = 0;

    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                goto quit;

            case SDL_EVENT_MOUSE_MOTION:
                R = e.motion.x;
                G = e.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                // This will cause a division by zero if R + 1 == G
                B = 255 / (R - G + 1);
                break;
            }
        }

        SDL_SetRenderDrawColor(r, R, G, B, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderDebugText(r, 4, 4, "This will flash bright colors!");
        SDL_SetRenderDrawColor(r, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderDebugText(r, 4, 16, "This will flash bright colors!");
        SDL_RenderPresent(r);

        SDL_Delay(10);
    }

quit:
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();

    return 0;
}
