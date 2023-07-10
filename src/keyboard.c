/*
 DOS keyboard interrupt implementation from StackOverflow

 https://stackoverflow.com/a/40963633
*/

#include "keyboard.h"

#include <conio.h>
#include <dos.h>
#include <stdio.h>

byte normal_keys[0x60];
byte extended_keys[0x60];

static void (__interrupt __far *old_dos_keyboard_interrupt)();

static void __interrupt __far keyboard_interrupt() {
    static byte buffer;
    byte rawcode;
    byte make_break;
    int scancode;

    rawcode = inp(0x60); /* read scancode from keyboard controller */
    make_break = !(rawcode & 0x80); /* bit 7: 0 = make, 1 = break */
    scancode = rawcode & 0x7F;

    if (buffer == 0xE0) { /* second byte of an extended key */
        if (scancode < 0x60) {
            extended_keys[scancode] = make_break;
        }
        buffer = 0;
    } else if (buffer >= 0xE1 && buffer <= 0xE2) {
        buffer = 0; /* ingore these extended keys */
    } else if (rawcode >= 0xE0 && rawcode <= 0xE2) {
        buffer = rawcode; /* first byte of an extended key */
    } else if (scancode < 0x60) {
        normal_keys[scancode] = make_break;
    }

    outp(0x20, 0x20); /* must send EOI to finish interrupt */
}

// saves current keyboard handler and hooks new one
void keyboard_init() {
    old_dos_keyboard_interrupt = _dos_getvect(0x09);
    _dos_setvect(0x09, keyboard_interrupt);
}

// unhooks keyboard handler and restores old one
void keyboard_shutdown() {
    if (old_dos_keyboard_interrupt != NULL) {
        _dos_setvect(0x09, old_dos_keyboard_interrupt);
        old_dos_keyboard_interrupt = NULL;
    }
}

bool is_pressing_escape() {
    return normal_keys[0x01];
}

bool is_pressing_w() {
    return normal_keys[0x11];
}

bool is_pressing_a() {
    return normal_keys[0x1E];
}

bool is_pressing_s() {
    return normal_keys[0x1F];
}

bool is_pressing_d() {
    return normal_keys[0x20];
}

bool is_pressing_lshift() {
    return normal_keys[0x2A];
}

byte* get_normal_keys() {
    return &normal_keys;
}
