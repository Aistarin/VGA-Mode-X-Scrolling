#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "src/common.h"

void keyboard_init();
void keyboard_shutdown();
byte* get_normal_keys();

bool is_pressing_escape();
bool is_pressing_w();
bool is_pressing_a();
bool is_pressing_s();
bool is_pressing_d();
bool is_pressing_lshift();

#endif
