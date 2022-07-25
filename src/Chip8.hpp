
#pragma once

#include <cstdint>
#include <random>
#include <unordered_map>
#include <functional>

const unsigned VIDEO_WIDTH = 64;
const unsigned VIDEO_HEIGHT = 32;
const unsigned KEY_COUNT = 16;
const unsigned MEMORY_SIZE = 4096;
const unsigned REGISTER_COUNT = 16;
const unsigned STACK_LEVELS = 16;

// The CHIP-8 is a virtual machine developped in the 1970s to
// ease game programming on early computers. What we are writing
// here is then actually an interpreter; however, understanding
// of both the architecture and the interpreter's code will be
// useful to later write a real emulator.
class Chip8
{
    public:

        // The CHIP-8 architecture is comprised of:
        //  - 16 8-bit registers, labeled V0 to VF;
        //  - 4K bytes of memory, where 0x000-0x1FF is reserved space,
        //  originally for the interpreter (in our case, we will never
        //  write there, except for 0x050-0x0A0, where the 16-built
        //  characters 0 through F are stored). Instructions from the
        //  ROM are stored starting at 0x200;
        //  - a 16-bit index register, where memory adresses for use
        //  in the operations are stored;
        //  - a 16-bit program counter (PC), where is hold the adress
        //  of the next instruction to be executed;
        //  - a 16-level stack: when we call an instruction in another
        //  region of the program, the program must be able to return
        //  back to where it was before calling this instruction; the
        //  stack holds the PC value when CALL was executed, and RET
        //  pulls that adress from the stack back into the PC. 16 levels
        //  of stack means that there can be 16 call levels (a function
        //  calls a function that calls a function that...);
        //  - an 8-bit stack pointer (SP), to tell us where in the 16
        //  levels of stack the program is at the moment;
        //  - an 8-bit delay timer, used for timing: if the timer value
        //  is 0, it stays 0; else, it decrements at a constant rate of
        //  60 Hz (1 decrement per 1/60 of a second);
        //  - an 8-bit sound timer, used for *sound* timing, with the
        //  same behavior; a single tone will buzz if it's non-zero;
        //  - 16 input keys, mapped from 1-F to 1234QWERASDFZXCV;
        //  - a 64x32 monochrome display memory, with each pixel either
        //  on or off.
        uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT] {};
        uint16_t index, pc, opcode, stack[STACK_LEVELS] {};
        uint8_t sp, delayTimer, soundTimer;
        uint8_t registers[REGISTER_COUNT] {}, memory[MEMORY_SIZE] {}, keypad[KEY_COUNT] {};

        std::default_random_engine randGen;
        std::uniform_int_distribution<uint8_t> randByte;

        std::array<std::function<void(void)>, 0xF + 1> table {};
        std::unordered_map<uint16_t, std::function<void(void)>> table0 {}, table8 {}, tableE {}, tableF {};

        Chip8();

        void load_ROM(const char* filename);
        void cycle();

        void op_00E0(); // CLS
        void op_00EE(); // RET
        void op_1nnn(); // JP nnn
        void op_2nnn(); // CALL nnn
        void op_3xkk(); // SE Vx, kk
        void op_4xkk(); // SNE Vx, kk
        void op_5xy0(); // SE Vx, Vy
        void op_6xkk(); // LD Vx, kk
        void op_7xkk(); // ADD Vx, byte
        void op_8xy0(); // LD Vx, Vy
        void op_8xy1(); // OR Vx, Vy
        void op_8xy2(); // AND Vx, Vy
        void op_8xy3(); // XOR Vx, Vy
        void op_8xy4(); // ADD Vx, Vy
        void op_8xy5(); // SUB Vx, Vy
        void op_8xy6(); // SHR Vx, 1
        void op_8xy7(); // SUBN Vx, Vy
        void op_8xyE(); // SHL Vx, 1
        void op_9xy0(); // SNE Vx, Vy
        void op_Annn(); // LD index, nnn
        void op_Bnnn(); // JP V0, nnn
        void op_Cxkk(); // RND Vx, kk
        void op_Dxyn(); // DRW Vx, Vy, n
        void op_Ex9E(); // SKP Vx
        void op_ExA1(); // SKNP Vx
        void op_Fx07(); // LD Vx, DT
        void op_Fx0A(); // LD Vx, K
        void op_Fx15(); // LD DT, Vx
        void op_Fx18(); // LD ST, Vx
        void op_Fx1E(); // ADD index, Vx
        void op_Fx29(); // LD F, Vx
        void op_Fx33(); // LD B, Vx
        void op_Fx55(); // LD [index], Vx
        void op_Fx65(); // LD Vx, [index]
};