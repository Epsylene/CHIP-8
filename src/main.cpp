
#include <chrono>
#include <iostream>

#include "Platform.hpp"
#include "Chip8.hpp"

int main(int argc, char** argv)
{
    if(argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int scale = std::stoi(argv[1]);
    int delay = std::stoi(argv[2]);
    const char* rom = argv[3];

    Platform platform {"CHIP-8 emulator", VIDEO_WIDTH * scale, VIDEO_HEIGHT * scale, VIDEO_WIDTH, VIDEO_HEIGHT};

    Chip8 chip8 {};
    chip8.load_ROM(rom);

    int pitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

    auto lastCycle = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while(!quit)
    {
        quit = platform.process_input(chip8.keypad);

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(now - lastCycle).count();

        if(dt > delay)
        {
            lastCycle = now;
            chip8.cycle();
            platform.update(chip8.video, pitch);
        }
    }

    return 0;
}
