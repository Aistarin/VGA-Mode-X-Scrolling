#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"

#define TIMER_INTERRUPT         0x08

#pragma aux timer_clearInterrupt =              \
    "mov al,20H",                               \
    "out 20H,al"

#pragma aux timer_cli =                         \
    "cli"

#pragma aux timer_sti =                         \
    "sti"

void timer_clearInterrupt();
void timer_cli();
void timer_sti();

void timer_init();
void timer_shutdown();

void timer_set_interval(int32_t new_delta);
uint32_t timer_get();
void timer_start();
bool timer_step();
void timer_end();
void timer_reset();

#endif
