
#include "Chip8.hpp"

#include <fstream>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>

const unsigned START_ADDRESS = 0x200;
const unsigned FONT_START_ADDRESS = 0x50;
const unsigned FONTSET_SIZE = 80;

// We need to define the fontset. Each character is represented
// as a series of 5 bytes, where each bit 1 is a pixel on and
// each 0 a pixel off. For example, F is 0xF0, 0x80, 0xF0, 0x80,
// 0x80, which in binary gives:
//  11110000
//  10000000
//  11110000
//  10000000
//  10000000
// You might see the F in there.
uint8_t fontset[FONTSET_SIZE] =
{
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(): randGen(std::chrono::system_clock::now().time_since_epoch().count())
{
    // The first instruction executed is at START_ADDRESS
    pc = START_ADDRESS;

    for (int i = 0; i < FONTSET_SIZE; ++i)
    {
        memory[FONT_START_ADDRESS + i] = fontset[i];
    }

    // Random number generation between 0 and 255
    randByte = std::uniform_int_distribution<uint8_t>(0, 255u);

    table[0x0] = [this] { table0[opcode & 0x000Fu](); };
    table[0x1] = [this] { op_1nnn(); };
    table[0x2] = [this] { op_2nnn(); };
    table[0x3] = [this] { op_3xkk(); };
    table[0x4] = [this] { op_4xkk(); };
    table[0x5] = [this] { op_5xy0(); };
    table[0x6] = [this] { op_6xkk(); };
    table[0x7] = [this] { op_7xkk(); };
    table[0x8] = [this] { table8[opcode & 0x000Fu](); };
    table[0x9] = [this] { op_9xy0(); };
    table[0xA] = [this] { op_Annn(); };
    table[0xB] = [this] { op_Bnnn(); };
    table[0xC] = [this] { op_Cxkk(); };
    table[0xD] = [this] { op_Dxyn(); };
    table[0xE] = [this] { tableE[opcode & 0x000Fu](); };
    table[0xF] = [this] { tableF[opcode & 0x00FFu](); };

    table0[0x0] = [this] { op_00E0(); };
    table0[0xE] = [this] { op_00EE(); };

    table8[0x0] = [this] { op_8xy0(); };
    table8[0x1] = [this] { op_8xy1(); };
    table8[0x2] = [this] { op_8xy2(); };
    table8[0x3] = [this] { op_8xy3(); };
    table8[0x4] = [this] { op_8xy4(); };
    table8[0x5] = [this] { op_8xy5(); };
    table8[0x6] = [this] { op_8xy6(); };
    table8[0x7] = [this] { op_8xy7(); };
    table8[0xE] = [this] { op_8xyE(); };

    tableE[0x1] = [this] { op_ExA1(); };
    tableE[0xE] = [this] { op_Ex9E(); };

    tableF[0x07] = [this] { op_Fx07(); };
    tableF[0x0A] = [this] { op_Fx0A(); };
    tableF[0x15] = [this] { op_Fx15(); };
    tableF[0x18] = [this] { op_Fx18(); };
    tableF[0x1E] = [this] { op_Fx1E(); };
    tableF[0x29] = [this] { op_Fx29(); };
    tableF[0x33] = [this] { op_Fx33(); };
    tableF[0x55] = [this] { op_Fx55(); };
    tableF[0x65] = [this] { op_Fx65(); };
}

void Chip8::load_ROM(const char* filename)
{
    // Open the file as a strem of binary (std::ios::binary) and
    // move the file pointer to the end (std::ios::ate)
    std::ifstream file {filename, std::ios::binary | std::ios::ate};

    if(file.is_open())
    {
        // tellg() returns the current read position, which is the end,
        // thus the size of the file.
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        // Seek (place the cursor) at offset 0 from std::ios::beg
        // (that is, at the beginning of the file), and then fill
        // 'buffer' with the file data.
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        // Fill the CHIP-8 memory with the ROM data starting at
        // adress 0x200.
        for (int i = 0; i < size; ++i)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
    }
}

void Chip8::op_00E0()
{
    // Clear the screen: set the entire video buffer to zeros.
    std::memset(video, 0, sizeof(video));
}

void Chip8::op_00EE()
{
    // Return from a subroutine: the stack pointer goes one level down
    // and the program counter is set to the instruction next to the
    // one that called the subroutine in the first place (that is,
    // the value stored at the index SP of the stack)
    --sp;
    pc = stack[sp];
}

void Chip8::op_1nnn()
{
    // Jumpt to location nnn: the opcode is in the form 1nnn, where the
    // last three digits correspond to the adress we want to jump to;
    // we get those digits thanks to the '& 0x0FFF', which is the same
    // as AND'ing with ones only in three last nibbles.
    pc = opcode & 0x0FFFu;
}

void Chip8::op_2nnn()
{
    // Call subroutine at nnn: we get the adress from the opcode...
    uint16_t adress = opcode & 0x0FFFu;

    //...then put the PC on the stack, get one stack level up, and
    // put the adress in the PC, so the next instruction called is
    // the one at 'nnn'.
    stack[sp] = pc;
    ++sp;
    pc = adress;
}

void Chip8::op_3xkk()
{
    // Skip next instruction if Vx == kk: we get the register number,
    // the byte, and check if they are equal; if they are, we can
    // increment the PC by 2 again (first time in the system loop)
    // to skip to the next instruction (the reason the PC is
    // increased by two and not one for each new instruction is
    // that an opcode is two bytes, so when we fetch an instruction
    // and move the PC to the next one the adress has to be increased
    // by the number of bytes of the instruction, which is two)
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if(registers[Vx] == byte)
        pc += 2;
}

void Chip8::op_4xkk()
{
    // Skip next instruction if Vx != kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if(registers[Vx] != byte)
        pc += 2;
}

void Chip8::op_5xy0()
{
    // Skip next instruction if Vx == Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] == registers[Vy])
        pc += 2;
}

void Chip8::op_6xkk()
{
    // Set Vx = kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = byte;
}

void Chip8::op_7xkk()
{
    // Set Vx += kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

void Chip8::op_8xy0()
{
    // Set Vx = Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

void Chip8::op_8xy1()
{
    // Set Vx |= Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

void Chip8::op_8xy2()
{
    // Set Vx &= Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

void Chip8::op_8xy3()
{
    // Set Vx ^= Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

void Chip8::op_8xy4()
{
    // Set Vx += Vy and set VF = carry: add Vx and Vy, put the result
    // in Vx, and if there is overflow (result > 8 bits = 255), set
    // the carry flag to 1.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint16_t sum = registers[Vx] + registers[Vy];

    registers[15] = (sum > 255u);
    registers[Vx] = sum & 0xFFu;
}

void Chip8::op_8xy5()
{
    // Set Vx -= Vy, set VF = NOT borrow: if Vx > Vy, then VF is set
    // to 1, otherwise 0.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[15] = (registers[Vx] > registers[Vy]);
    registers[Vx] -= registers[Vy];
}

void Chip8::op_8xy6()
{
    // Set Vx = Vx SHR 1: the SHR instruction shifts the register
    // bits right by the number of bits specified in the second
    // operand and puts bits shifted out into the carry flag. Here
    // we SHR by 1, so Vx is right-shifted by 1 and VF is set to the
    // shifted-out bit.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[15] = (registers[Vx] & 0x1u); // shifted-out bit into VF
    registers[Vx] >>= 1;
}

void Chip8::op_8xy7()
{
    // Set Vx = Vy - Vx, set VF = NOT borrow: if Vy > Vx, then VF is
    // set to 1, otherwise 0.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[15] = (registers[Vy] > registers[Vx]);
    registers[Vx] = registers[Vy] - registers[Vx];
}

void Chip8::op_8xyE()
{
    // Set Vx = Vx SHL 1: left shift by 1 Vx, and put the most
    // significant bit into VF.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[15] = (registers[Vx] & 0x80u) >> 7u; // shifted-out bit into VF
    registers[Vx] <<= 1;
}

void Chip8::op_9xy0()
{
    // Skip next instruction if Vx != Vy
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] != registers[Vy])
        pc += 2;
}

void Chip8::op_Annn()
{
    // Set index = nnn: set the index counter to the adress 'nnn'.
    index = opcode & 0x0FFFu;
}

void Chip8::op_Bnnn()
{
    // Jump to location V0 + nnn
    pc = registers[0] + opcode & 0x0FFFu;
}

void Chip8::op_Cxkk()
{
    // Set Vx = random byte AND kk
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

void Chip8::op_Dxyn()
{
    // Display from (Vx, Vy) a n-byte sprite starting at memory
    // location 'index' and set VF = collision: we start iterating
    // from (Vx, Vy) to (Vx + 8, Vy + n) (a sprite is guaranteed to
    // be height pixels wide) to check if there is already set pixels
    // in there, in which case the VF register is set ('collision').
    // Then sprite pixels and screen pixels are XOR'ed one another.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;

    // The sprite pixels are required to wrap around the screen if
    // they go beyond the boundary
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    // VF is by default 0
    registers[15] = 0;

    for (int j = 0; j < height; ++j)
    {
        // Each byte in memory starting at 'index' is interpreted as
        // a row of the sprite...
        uint8_t sprite_byte = memory[index + j];

        for (int i = 0; i < 8; ++i)
        {
            // ...where each 1 is a pixel on and each 0 a pixel off.
            // (for example, the two bytes 0xF 0xE7 would make the
            // shape :::..:::)
            uint8_t sprite_pixel = sprite_byte & (0x80u >> i);

            auto pos = (xPos + i) + (yPos + j)*VIDEO_WIDTH;
            auto& screen_pixel = video[pos];

            if(sprite_pixel)
            {
                // If the screen pixel and the sprite pixel are both
                // set, there is collision and the VF is set to 1
                if(screen_pixel == 0xFFFFFFFF)
                    registers[15] = 1;

                screen_pixel ^= 0xFFFFFFFF;
            }
        }
    }
}

void Chip8::op_Ex9E()
{
    // Skip the next instruction if a key with the value
    // of Vx is pressed.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if(keypad[key])
        pc += 2;
}

void Chip8::op_ExA1()
{
    // Skip the next instruction if a key with the value
    // of Vx is not pressed
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];

    if(!keypad[key])
        pc += 2;
}

void Chip8::op_Fx07()
{
    // Set Vx to the value of the delay timer.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

void Chip8::op_Fx0A()
{
    // Wait for a key press and store the value of the key in Vx.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i < 15; ++i)
    {
        if(keypad[i])
        {
            registers[Vx] = i;
            return;
        }
    }

    // If no key is pressed, the PC (which had been increment by 2
    // precedently) is decremented by 2, which has the effect of
    // running again the same instruction (so the program "waits").
    pc -= 2;
}

void Chip8::op_Fx15()
{
    // Set the delay timer to the value stored in Vx.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}

void Chip8::op_Fx18()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
}

void Chip8::op_Fx1E()
{
    // Set index += Vx.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

void Chip8::op_Fx29()
{
    // Set index to the location of the font character Vx.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    // Each character is 5 bytes each
    index = FONT_START_ADDRESS + 5 * digit;
}

void Chip8::op_Fx33()
{
    // Store the BCD representation of Vx starting at the
    // adress 'index': the BCD representation of a number is
    // a binary representation which consists in representing
    // each digit of the number as a group of 4 bits (for example,
    // 0010 0101 1000 is translated as the number 258, instead of
    // the actual decimal equivalent of the binary number).
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    // The modulo give us the right-most digit (258 % 10 = 8,
    // because the closest multiple of 10 is 250 and the rest
    // is 8; the reasoning stays the same for any number), and
    // then dividing by 10 gets rid of it (because these are
    // integral values).
    memory[index + 2] = value % 10;
    value /= 10;

    memory[index + 1] = value % 10;
    value /= 10;

    memory[index] = value % 10;
}

void Chip8::op_Fx55()
{
    // Store registers V0 through Vx in memory starting at
    // location 'index'.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i <= Vx; ++i)
    {
        memory[index + i] = registers[i];
    }
}

void Chip8::op_Fx65()
{
    // Read registers V0 through Vx from memory starting
    // at location 'index'.
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i <= Vx; ++i)
    {
        registers[i] = memory[index + i];
    }
}

void Chip8::cycle()
{
    // A cycle of the CHIP-8 CPU consists of three things: fetching
    // the next instruction in the form of an opcode, decoding it,
    // and executing it through our function pointer table.

    // Fetch the opcode: it consists of two bytes in memory, at the
    // 'next instruction' adress, stored in the PC...
    opcode = (memory[pc] << 8u) | memory[pc + 1];
    // ...which is incremented by 2 to point to the next instruction.
    pc += 2;

    // Decode and execute
    table[(opcode & 0xF000u) >> 12u]();

    // Delay timer...
    if(delayTimer > 0)
        --delayTimer;

    // ...and sound timer updates.
    if(soundTimer > 0)
        --soundTimer;
}