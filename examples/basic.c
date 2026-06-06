#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char **argv)
{
    char R = 0, G = 0, B = 0;
    float x = 0.0f, y = 0.0f;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *w = SDL_CreateWindow("Basic example", 640, 480, 0);
    SDL_Renderer *r = SDL_CreateRenderer(w, NULL);

    SDL_SetRenderDrawColor(r, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(r);
    SDL_RenderPresent(r);

    SDL_HideCursor();

    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
            case SDL_EVENT_QUIT:
                goto quit;

            case SDL_EVENT_MOUSE_MOTION:
                R = x = e.motion.x;
                G = y = e.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                // This will cause a division by zero if R + 1 == G
                B = 255 / (R - G + 1);
                break;
            }
        }

        SDL_SetRenderDrawColor(r, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(r);

        SDL_SetRenderDrawColor(r, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderDebugTextFormat(r, 4, 4, "R = %d", R);
        SDL_RenderDebugTextFormat(r, 4, 16, "G = %d", G);
        SDL_RenderDebugTextFormat(r, 4, 28, "B = 255 / (R - G + 1) = %d", B);
        SDL_RenderDebugText(r, 4, 40, "Click to update B.");

        SDL_FRect rect = { x - 3.0f, y - 3.0f, 7.0f, 7.0f };
        SDL_SetRenderDrawColor(r, R, G, B, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(r, &rect);

        SDL_RenderPresent(r);

        SDL_Delay(10);
    }

quit:
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();

    return 0;
}
