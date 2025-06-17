#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "SDL2/SDL.h"
#include "chip8.h"
#include "chip8keyboard.h"

const char keyboard_map[CHIP8_TOTAL_KEYS] = {
    SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y,
    SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_z,
    SDLK_x, SDLK_c, SDLK_v, SDLK_b};

// const char keyboard_map[CHIP8_TOTAL_KEYS] = {
//     SDLK_KP_7, SDLK_KP_8, SDLK_KP_9, SDLK_KP_DIVIDE,       // 1 2 3 C
//     SDLK_KP_4, SDLK_KP_5, SDLK_KP_6, SDLK_KP_MULTIPLY,     // 4 5 6 D
//     SDLK_KP_1, SDLK_KP_2, SDLK_KP_3, SDLK_KP_MINUS,        // 7 8 9 E
//     SDLK_KP_0, SDLK_KP_PERIOD, SDLK_KP_PLUS, SDLK_KP_ENTER // A 0 B F
// };

int main(int argc, char **argv)
{

    const char *filename = argv[1];
    printf("The file name is: %s\n", filename);

    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Failed to open file");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char buf[size];
    int res = fread(buf, size, 1, f);

    if (res != 1)
    {
        printf("Failed to read from file");
        return -1;
    }

    // printf("%s\n", buf);

    struct chip8 chip8;
    chip8_init(&chip8);
    chip8_load(&chip8, buf, size);
    chip8_keyboard_set_map(&chip8.keyboard, keyboard_map);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        CHIP8_WIDTH * CHIP8_WINDOW_MULTIPLIER,
        CHIP8_HEIGHT * CHIP8_WINDOW_MULTIPLIER, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_TEXTUREACCESS_TARGET);

    while (1)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                goto out;
                break;

            case SDL_KEYDOWN:
            {
                char key = event.key.keysym.sym;
                int vkey = chip8_keyboard_map(&chip8.keyboard, key);

                if (vkey != -1)
                {
                    chip8_keyboard_down(&chip8.keyboard, vkey);
                }
                // printf("Key is down %x\n", vkey);
            }
            break;

            case SDL_KEYUP:
            {
                char key = event.key.keysym.sym;
                int vkey = chip8_keyboard_map(&chip8.keyboard, key);

                if (vkey != -1)
                {
                    chip8_keyboard_up(&chip8.keyboard, vkey);
                }
                // printf("Key is up %x\n", vkey);
            }
            break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

        for (int x = 0; x < CHIP8_WIDTH; x++)
        {
            for (int y = 0; y < CHIP8_HEIGHT; y++)
            {
                if (chip8_screen_is_set(&chip8.screen, x, y))
                {
                    SDL_Rect r;
                    r.x = x * CHIP8_WINDOW_MULTIPLIER;
                    r.y = y * CHIP8_WINDOW_MULTIPLIER;
                    r.w = CHIP8_WINDOW_MULTIPLIER;
                    r.h = CHIP8_WINDOW_MULTIPLIER;
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }
        SDL_RenderPresent(renderer);
        if (chip8.registers.delay_timer > 0)
        {
            Sleep(1);
            chip8.registers.delay_timer -= 1;
            // printf("DelaySuccessful\n");
        }

        if (chip8.registers.sound_timer > 0)
        {
            Beep(15000, 10 * chip8.registers.sound_timer);
            chip8.registers.sound_timer = 0;
        }
        unsigned short opcode = chip8_memory_get_short(&chip8.memory, chip8.registers.PC);
        chip8.registers.PC += 2;
        chip8_exec(&chip8, opcode);

        // printf("%x\n", opcode);
    }

out:
    SDL_DestroyWindow(window);
    return 0;
}