#include "graphics.h"

void graphics_init_video(){
    vga_init_modex();
}

void wait_for_retrace(){
    vga_wait_for_retrace();
}