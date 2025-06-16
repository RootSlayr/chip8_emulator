# chip8_emulator
A simple and efficient CHIP-8 emulator written in C.  

What is CHIP-8?

This emulator replicates the CHIP-8 virtual machine originally used on 1970s microcomputers like the COSMAC VIP.

### This has

- **4KB memory** (0x000–0xFFF)
- **16 general-purpose 8-bit registers** (V0–VF)
- **16-bit index register** (`I`)
- **Program counter** (`PC`)
- **Stack & stack pointer**
- **Delay and sound timers**
- **Hex keypad input** (mapped to keyboard)
- **Monochrome 64×32 display**
The implementation includes a full interpreter for all 35 opcodes of the CHIP-8 instruction set.

## Features

- Full opcode support (35 opcodes)
- SDL2-based graphics and input
- Configurable clock speed
- Cross-platform support (Windows, Linux), probably, should work, I did not test

## Getting Started

### Requirements

- `gcc` or any C compiler
- `SDL2` (e.g. install via `pacman -S mingw-w64-x86_64-SDL2` on MSYS2)
- `mingw32-make` or `make` this important for the make file, use makefile for building exe file
