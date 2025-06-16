#include "chip8.h"
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2/SDL.h"

const char chip8_default_character_set[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0, //0//0
    0x20, 0x60, 0x20, 0x20, 0x70, //1//5
    0xf0, 0x10, 0xf0, 0x80, 0xf0, //2//A
    0xf0, 0x10, 0xf0, 0x10, 0xf0, //3//F
    0x90, 0x90, 0x40, 0x10, 0x10, //4//15
    0xf0, 0x80, 0xf0, 0x10, 0xf0, //5
    0xf0, 0x80, 0xf0, 0x90, 0xf0, //6
    0xf0, 0x10, 0x20, 0x40, 0x40, //7
    0xf0, 0x90, 0xf0, 0x90, 0xf0, //8
    0xf0, 0x90, 0xf0, 0x10, 0xf0, //9
    0xf0, 0x90, 0xf0, 0x90, 0x90, //a
    0xe0, 0x90, 0xe0, 0x90, 0xe0, //b
    0xf0, 0x80, 0x80, 0x80, 0xf0, //c
    0xe0, 0x90, 0x90, 0x90, 0xe0, //d
    0xf0, 0x80, 0xf0, 0x80, 0xf0, //e
    0xf0, 0x80, 0xf0, 0x80, 0x80  //f
};
void chip8_init(struct chip8 *chip8)
{
    memset(chip8, 0, sizeof(struct chip8));
    memcpy(&chip8->memory.memory, chip8_default_character_set, sizeof(chip8_default_character_set));
}

static void chip8_execute_extended_8(struct chip8 *chip8, unsigned short opcode)
{
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char final4 = opcode & 0x000f;
    unsigned short temp = 0;
    switch (final4)
    {
    case 0x00: //8xy0 LD Vx, Vy, Vx=Vy
        chip8->registers.V[x] = chip8->registers.V[y];
        break;

    case 0x01: //8xy1 bitwise OR Vx and Vy and stores in Vx
        chip8->registers.V[x] = chip8->registers.V[x] | chip8->registers.V[y];
        break;

    case 0x02: //8xy2 bitwise AND Vx and Vy and store in Vx
        chip8->registers.V[x] = chip8->registers.V[x] & chip8->registers.V[y];
        break;

    case 0x03: //8xy3 bitwise XOR Vx and Vy and store in Vx
        chip8->registers.V[x] = chip8->registers.V[x] ^ chip8->registers.V[y];
        break;

    case 0x04: //8xy4 ADD Vx with Vy, Vx=Vx +Vy and set Vf = carry
        temp = chip8->registers.V[x] + chip8->registers.V[y];
        chip8->registers.V[0x0f] = false;
        if (temp > 0xff)
        {
            chip8->registers.V[0x0f] = true;
        }
        chip8->registers.V[x] = temp;
        break;

    case 0x05: //8xy5 SUB Vx and Vy, set Vx = Vx-Vy and Vf = Not Borrow
        chip8->registers.V[0x0f] = false;
        if (chip8->registers.V[x] > chip8->registers.V[y])
        {
            chip8->registers.V[0x0f] = true;
        }
        chip8->registers.V[x] = chip8->registers.V[x] - chip8->registers.V[y];
        break;

    case 0x06: //8xy6 if LSB of Vx is 1, it sets the Vf is set to 1
        chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x01;
        chip8->registers.V[x] /= 2;
        break;

    case 0x07: //8xy7 SUBN Vx = Vy-Vx, Vf=not borrow, If Vy>Vx then Vf=1, or 0
        chip8->registers.V[0x0f] = false;
        if (chip8->registers.V[y] > chip8->registers.V[x])
        {
            chip8->registers.V[0x0f] = true;
        }
        chip8->registers.V[x] = chip8->registers.V[y] - chip8->registers.V[x];
        break;

    case 0x0E:                                                   //if MSB of Vx is 1 then Vf=1; then Vx * 2
        chip8->registers.V[0x0f] = chip8->registers.V[x] & 0x80; //1000000(bin);
        chip8->registers.V[x] = chip8->registers.V[x] * 2;
        break;
    }
}

static char chip8_waitfor_keypress(struct chip8 *chip8)
{
    SDL_Event event;
    while (SDL_WaitEvent(&event))
    {
        if (event.type != SDL_KEYDOWN)
            continue;

        char c = event.key.keysym.sym;
        char chip8_key = chip8_keyboard_map(&chip8->keyboard, c);
        if (chip8_key != -1)
        {
            return chip8_key;
        }
    }
    return -1;
}

static void chip8_execute_extended_f(struct chip8 *chip8, unsigned short opcode)
{
    unsigned char x = (opcode >> 8) & 0x000f;
    switch (opcode & 0x00ff)
    {
    case 0x07: // LD Vx, DT, Vx=Delay timer(DT) value
        chip8->registers.V[x] = chip8->registers.delay_timer;
        break;

    case 0x0A: //LD Vx, K,  wait for a key press and then stoe it in Vx
    {
        char pressed_key = chip8_waitfor_keypress(chip8);
        chip8->registers.V[x] = pressed_key;
    }
    break;

    case 0x15: //LD DT,Vx DT is det the value of Vx
        chip8->registers.delay_timer = chip8->registers.V[x];
        break;

    case 0x18: //LD ST, Vx,  Sound timer is Set to value of Vx
        chip8->registers.sound_timer = chip8->registers.V[x];
        break;

    case 0x1E: //ADD I, Vx, I and Vx are added and result is stored in I
        chip8->registers.I += chip8->registers.V[x];
        break;

    case 0x29: //LD F Vx,  set I=Location of sprite for digit Vx
        chip8->registers.I = chip8->registers.V[x] * CHIP8_DEFAULT_SPRITE_HEIGHT;
        break;

    case 0x33: //LD, B, Vx Store BCD(Vx) in I, I+1, I+2
    {
        unsigned char hundreds = chip8->registers.V[x] / 100;
        unsigned char tens = chip8->registers.V[x] / 10 % 10;
        unsigned char units = chip8->registers.V[x] % 10;
        chip8_memory_set(&chip8->memory, chip8->registers.I, hundreds);
        chip8_memory_set(&chip8->memory, chip8->registers.I + 1, tens);
        chip8_memory_set(&chip8->memory, chip8->registers.I + 2, units);
    }

    break;

    case 0x55: //LD[I] Vx, stores Reg V0 to Vx in memory starting at locaton I
    {
        for (int i = 0; i <= x; i++)
        {
            chip8_memory_set(&chip8->memory, chip8->registers.I + i, chip8->registers.V[i]);
        }
    }
    break;

    case 0x65: // LD Vx, I, read registers V0 to Vx From mem starting at I
    {
        for (int i = 0; i <= x; i++)
        {
            chip8->registers.V[i] = chip8_memory_get(&chip8->memory, chip8->registers.I + i);
        }
        break;
    }
    }
}

static void chip8_execute_extended(struct chip8 *chip8, unsigned short opcode)
{
    unsigned short nnn = opcode & 0x0fff;
    unsigned char x = (opcode >> 8) & 0x000f;
    unsigned char y = (opcode >> 4) & 0x000f;
    unsigned char kk = opcode & 0x00ff;
    unsigned char n = opcode & 0x000f;
    switch (opcode & 0xf000)
    //check cowgod manual for more info -- ROOT
    {
    case 0x1000: //JP addr Jump to the location of nnn
        chip8->registers.PC = nnn;
        break;

    case 0x2000: //CALL addr , calls subroutine at location nnn
        chip8_stack_push(chip8, chip8->registers.PC);
        chip8->registers.PC = nnn;
        break;

    case 0x3000: //3xkk  SKIP if equal, Vx=kk, 3xSkip inst if Vx=kk
        if (chip8->registers.V[x] == kk)
        {
            chip8->registers.PC += 2;
        }
        break;

    case 0x4000: //4xkk SKIP if not equal, Vx!=kk, 4xSkip inst if Vx1=kk
        if (chip8->registers.V[x] != kk)
        {
            chip8->registers.PC += 2;
        }
        break;

    case 0x5000: // SE 5xyo  check if V[x] = V[y], and skip next instr and increment PC by 2
        if (chip8->registers.V[x] == chip8->registers.V[y])
        {
            chip8->registers.PC += 2;
        }
        break;

    case 0x6000: // 6xkk , LD Vx, byte, Vx=kk
        chip8->registers.V[x] = kk;
        break;

    case 0x7000: // 7xkk ADD Vx with the value of kk(set Vx = Vx+kk)
        chip8->registers.V[x] += kk;
        break;

    case 0x8000:
        chip8_execute_extended_8(chip8, opcode);
        break;

    case 0x9000: //9xy0, SNE Vx,Vy ,Skip next instruction if Vx!=Vy
        if (chip8->registers.V[x] != chip8->registers.V[y])
        {
            chip8->registers.PC += 2;
        }
        break;

    case 0xA000: // value of I register is set to nnn
        chip8->registers.I = nnn;
        break;

    case 0xB000: //jump to location nnn +V0 i.e PC is set to V0+nnn
        chip8->registers.PC = nnn + chip8->registers.V[0x00];
        break;

    case 0xC000: /*  RND Cxkk Interpreter, generates random number from 0 to 255, ANDed with 
        value of  kk and the results stored in Vx */
        srand(clock());
        chip8->registers.V[x] = (rand() % 255) & kk;
        break;

    case 0xD000: //Dxyn draw Vx, Vy and nibble, draw n-byte sprite at I(Vx, Vy)
    {
        const char *sprite = (const char *)&chip8->memory.memory[chip8->registers.I];
        chip8->registers.V[0x0f] = chip8_screen_draw_sprite(&chip8->screen, chip8->registers.V[x], chip8->registers.V[y], sprite, n);
    }
    break;

    case 0xE000: //keyboard implementation
    {
        switch (opcode & 0x00ff)
        {
        case 0x9e: //Ex9E - SKP, Skip nest instr, if Key with Vx val,  is pressed.
            if (chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
            {
                chip8->registers.PC += 2;
            }
            break;

        case 0xa1: //ExA1 - SKP, Skip nest instr, if Key with Vx val is not pressed,  is pressed.
            if (!chip8_keyboard_is_down(&chip8->keyboard, chip8->registers.V[x]))
            {
                chip8->registers.PC += 2;
            }
            break;
        }
    }
    break;

    case 0xF000:
        chip8_execute_extended_f(chip8, opcode);
        break;
    }
}

void chip8_exec(struct chip8 *chip8, unsigned short opcode)
{
    switch (opcode)
    {
    case 0x00E0: //clearscr
        chip8_screen_clear(&chip8->screen);
        break;

    case 0x00EE: //return from  subroutine
        chip8->registers.PC = chip8_stack_pop(chip8);
        break;

    default:
        chip8_execute_extended(chip8, opcode);
    }
}
void chip8_load(struct chip8 *chip8, const char *buf, size_t size)
{
    assert(size + CHIP8_PROGRAM_LOAD_ADDRESS < CHIP8_MEMORY_SIZE);
    memcpy(&chip8->memory.memory[CHIP8_PROGRAM_LOAD_ADDRESS], buf, size);
    chip8->registers.PC = CHIP8_PROGRAM_LOAD_ADDRESS;
}
