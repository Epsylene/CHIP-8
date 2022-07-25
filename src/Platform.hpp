
#pragma once

#include <string_view>
#include <SDL2/SDL.h>

class Platform
{
    public:

        Platform(const char* title, unsigned windowWidth, unsigned windowHeight,
                    unsigned textureWidth, unsigned textureHeight);

        ~Platform();

        void update(const void* buffer, int pitch);
        bool process_input(uint8_t* keys);

    private:

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
};
