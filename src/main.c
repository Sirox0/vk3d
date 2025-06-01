#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#include <stdlib.h>

#include "config.h"
#include "vkInit.h"
#include "vkFunctions.h"
#include "game.h"

SDL_Window* window;
u8 loopActive = 1;
u32 deltaTime = 0;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_EVENTS);
    window = SDL_CreateWindow(TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    SDL_SetWindowRelativeMouseMode(window, true);

    if (window == NULL) {
        printf("failed to create window\n");
        exit(1);
    }

    vkInit();
    gameInit();

    SDL_Event event;
    while (loopActive) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                loopActive = 0;
                break;
            } else gameEvent(&event);
        }

        u32 startTime = SDL_GetTicks();

        gameRender();

        deltaTime = SDL_GetTicks() - startTime;
    }

    vkDeviceWaitIdle(vkglobals.device);
    gameQuit();
    vkQuit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}