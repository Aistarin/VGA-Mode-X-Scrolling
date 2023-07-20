# VGA Mode X Scrolling
This project aims to be a template for the following:
- Setting up the tweaked VGA Mode 13 known as Mode X that allows for a 320x240 display with square pixels under DOS
- Routines for quickly blitting bitmap images from main memory to VRAM
- Smooth hardware scrolling across an infinite-sized map

# Requirements
- Open Watcom C/C++ Version 2.0 (Windows or Linux)
- DOS-based environment (MS-DOS, FreeDOS, DOSBox) to run the builds
- DOS/32 or DOS4GW 32-bit DOS extender

# How To Build
- Make a `.env` file in the root of the project by duplicating the `.env-example` file
- Point the paths in `.env` to those corresponding to your system (OpenWatcom, NASM, DOSBox, etc.)
- Place a copy of your 32-bit DOS extender of choice (`dos32a.exe` or `dos4gw.exe`) in the root directory of the project
- Update the `DOS_EXTENDER` variable in the `.env` file with your 32-bit DOS extender of choice accordingly (`dos32a` or `dos4gw`)
- Run `make` to build

# How To Run
- Run `make run` to run the built program against the defined DOSBox install
- To run directly in DOSBox, simply mount the `build` directory and run `scroll`
```
mount G /local/path/to/project/build
G:
```

# Minimum System Requirements
- IBM Compatible PC running DOS (MS-DOS, FreeDOS, etc.)
- 32-bit x86 CPU (80386SX 40 MHz or better)
- 4MB RAM
- 16-bit ISA VGA video card (Tseng Labs ET4000AX or better)
- Keyboard

# Recommended System Requirements
- 80486DX or Pentium CPU (66 MHz or better)
- 8MB RAM
- VESA Local Bus (VLB) or PCI VGA video card

# ATTRIBUTIONS
A lot of this code is loosely based on several sources, most derived from Michael Abrash's works featured in _Graphics Programming Black Book_.

The following projects have been used as references:
- David Brackeen's 256-Color VGA Programming in C (http://www.brackeen.com/vga/)
- @mills32's Little Game Engine for VGA (https://github.com/mills32/Little-Game-Engine-for-VGA)
- @64Mega's Venture-Out! LD44 project (https://github.com/64Mega/ld44-venture-out)
- @root42's Let's Code MS-DOS series (https://github.com/root42/letscode-breakout)

# LICENSE
All source code + files outside of the `/res` folder adheres to the MIT License