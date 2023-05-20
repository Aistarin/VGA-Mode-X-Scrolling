/*
 DOS Timer implementation adapted from ExpiredPopsicle's
 code to reprogram the Intel 8253/8254 to run at a custom
 interval and hooking in a custom timer interrupt that
 still calls the default one at 18.2 Hz
 
 https://expiredpopsicle.com/articles/2017-04-13-DOS_Timer_Stuff/2017-04-13-DOS_Timer_Stuff.html
*/

#include "timer.h"

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>

static int32_t nextOldTimer = 0;
static uint32_t timerInitCounter = 0;

static uint32_t timer_current_time = 0;
static uint32_t timer_start_time = 0;
static uint32_t timer_end_time = 0;
static int32_t timer_delta = 0;
static int32_t timer_accumulator = 0;

void timer_reset() {
    timer_accumulator = 0;
    timer_start_time = 0;
    timer_end_time = 0;
}

void timer_set_interval(int32_t new_delta) {
    timer_delta = new_delta;
    timer_accumulator = 0;
}

void timer_start() {
    timer_start_time = timer_current_time;
    timer_accumulator += timer_start_time - timer_end_time;
}

void timer_end() {
    timer_end_time = timer_start_time;
}

bool timer_step() {
    if (timer_accumulator > timer_delta) {
        timer_accumulator -= timer_delta;
        return TRUE;
    }
    return FALSE;
}

static void (__interrupt __far *old_dos_timer_interrupt)();

static void __interrupt __far new_custom_timer_interrupt()
{
    timer_current_time++;

    nextOldTimer -= 10;
    if(nextOldTimer <= 0) {

        nextOldTimer += 182;
        old_dos_timer_interrupt();

    } else {

        // Make sure we still execute the "HEY I'M DONE WITH THIS
        // INTERRUPT" signal.

        timer_clearInterrupt();
    }
}

uint32_t timer_get(void)
{
    return timer_current_time;
}

void timer_init(void)
{
    // The clock we're dealing with here runs at 1.193182mhz, so we
    // just divide 1.193182 by the number of triggers we want per
    // second to get our divisor.
    uint32_t c = 1193181 / (uint32_t)1000;

    // Increment ref count and refuse to init if we're already
    // initialized.
    timerInitCounter++;
    if(timerInitCounter > 1) {
        return;
    }

    // Swap out interrupt handlers.
    old_dos_timer_interrupt = _dos_getvect(TIMER_INTERRUPT);
    _dos_setvect(TIMER_INTERRUPT, new_custom_timer_interrupt);

    timer_cli();

    // There's a ton of options encoded into this one byte I'm going
    // to send to the PIT here so...

    // 0x34 = 0011 0100 in binary.

    // 00  = Select counter 0 (counter divisor)
    // 11  = Command to read/write counter bits (low byte, then high
    //       byte, in sequence).
    // 010 = Mode 2 - rate generator.
    // 0   = Binary counter 16 bits (instead of BCD counter).

    outp(0x43, 0x34);

    // Set divisor low byte.
    outp(0x40, (uint8_t)(c & 0xff));

    // Set divisor high byte.
    outp(0x40, (uint8_t)((c >> 8) & 0xff));

    timer_sti();
}

void timer_shutdown(void)
{
    // Decrement ref count and refuse to shut down if we're still in
    // use.
    timerInitCounter--;
    if(timerInitCounter > 0) {
        return;
    }

    timer_cli();

    // Send the same command we sent in timer_init() just so we can
    // set the timer divisor back.
    outp(0x43, 0x34);

    // FIXME: I guess giving zero here resets it? Not sure about this.
    // Maybe we should save the timer values first.
    outp(0x40, 0);
    outp(0x40, 0);

    timer_sti();

    // Restore original timer interrupt handler.
    _dos_setvect(TIMER_INTERRUPT, old_dos_timer_interrupt);
}

void timer_delay(uint32_t ms)
{
    uint32_t startTimer = timer_get();
    while(timer_get() - startTimer < ms) {
    }
}
