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

void timer_clearInterrupt(void);
void timer_cli(void);
void timer_sti(void);

void timer_init(void);
void timer_shutdown(void);

void timer_set_interval(int32_t new_delta);
uint32_t timer_get(void);
void timer_start(void);
bool timer_step(void);
void timer_end(void);
void timer_reset(void);

#endif
