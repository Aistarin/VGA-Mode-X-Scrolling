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

# File Transfer Over Serial
If you have a USB to Serial adapter and Null Modem serial cable, you can use Kermit to easily transfer files between your development machine and your real DOS hardware.
- Ensure your DOS machine is powered down first before trying to connect the Null Modem serial cable to it
- Set up your USB to Serial adapter on your development machine
    - Under Linux, you'll have to add yourself to the group that owns whatever device your adapter maps to before you can use the device as a non-root user (may vary depending on your disto)
```
usermod -a -G dialout $USER
```
- Install Kermit on both your development machine anf your DOS machine
    - Under Linux, you can simply install ckermit via `sudo apt-get install ckermit`
    - Under DOS, you can download it from here: http://www.columbia.edu/kermit/mskermit.html
- Update the `.env ` file to point to your local serial port and set the transfer speed (bps)
```
SERIAL_PORT = /dev/ttyUSB0
SERIAL_SPEED = 115200
```
- Boot into your DOS machine and go into the directory where you want to put your build (e.g. `game.exe`)
- On your development machine, initiate the file file transfer with `make send`
- On your DOS machine, run the kermit command to receive the file
```
kermit set speed 115200, rec game.exe
```
    - NOTE: subsequent runs can omit the `set speed 115200`
- Wait for the file to transfer
    - This process should begin immediately, if not you may want to troubleshoot your serial connection or set a lower baud rate on both ends (e.g. 9600bps)

Alternatively, you can also set up DOSBox to use your serial port and kick off the file transfer from there.
```
serial1=directserial realport:ttyUSB0
kermit set speed 115200, send game.exe
```

# ATTRIBUTIONS
A lot of this code is loosely based on several sources, most derived from Michael Abrash's works featured in _Graphics Programming Black Book_.

The following projects have been used as references:
- David Brackeen's 256-Color VGA Programming in C (http://www.brackeen.com/vga/)
- @mills32's Little Game Engine for VGA (https://github.com/mills32/Little-Game-Engine-for-VGA)
- @64Mega's Venture-Out! LD44 project (https://github.com/64Mega/ld44-venture-out)
- @root42's Let's Code MS-DOS series (https://github.com/root42/letscode-breakout)

# LICENSE
All source code + files outside of the `/res` folder adheres to the MIT License