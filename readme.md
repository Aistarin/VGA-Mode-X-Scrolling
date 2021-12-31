# VGA Mode X Scrolling
This project aims to be a template for the following:
- Setting up the tweaked VGA Mode 13 known as Mode X that allows for a 320x240 display with square pixels under DOS
- Routines for quickly blitting bitmap images from main memory to VRAM
- Smooth hardware scrolling across an infinite-sized map

# Requirements
- Open Watcom C/C++ Version 2.0
- DOS-based environment (MS-DOS, FreeDOS) to run the builds
- 32-bit x86 CPU (80386 or better)
- DOS/32 or DOS4GW 32-bit DOS extender

# How To Build
- Place a copy of your 32-bit DOS extender of choice (`dos32a.exe` or `dos4gw.exe`) in the root directory of the project
- Update `make.bat` with your 32-bit DOS extender of choice
```
SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -bt=dos -l=dos32a
REM SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -bt=dos -l=dos4g
```
- Run `make.bat`

# ATTRIBUTIONS
A lot of this code is loosely based on several sources, most derived from Michael Abrash's works featured in _Graphics Programming Black Book_.

The following projects have been used as references:
- David Brackeen's 256-Color VGA Programming in C (http://www.brackeen.com/vga/)
- @mills32's Little Game Engine for VGA (https://github.com/mills32/Little-Game-Engine-for-VGA)
- @64Mega's Venture-Out! LD44 project (https://github.com/64Mega/ld44-venture-out)
- @root42's Let's Code MS-DOS series (https://github.com/root42/letscode-breakout)

# TODO
- TODO

# LICENSE
All source code + files outside of the `/res` folder adheres to the MIT License