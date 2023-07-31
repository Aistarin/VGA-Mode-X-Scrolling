#include "spr.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// mov dx, 0x3c4  ; SC_INDEX
// mov al, 0x02   ; MAP_MASK
// out dx, al
// mov dx, 0x03c5 ; SC_DATA
// mov al, bl     ; lower 8 bytes of ebx holds destination plane number
// out dx, al
// ror ebx, 8     ; grab next plane number
byte plane_select_opcodes[] = {
    0x66, 0xba, 0xc4, 0x03, 0xb0, 0x02, 0xee, 0x66, 
    0xba, 0xc5, 0x03, 0x88, 0xd8, 0xee, 0xc1, 0xcb, 
    0x08, 
};

dword spr_compile_planar_sprite_scheme_1(byte *sprite_buffer, word width, word height, byte *output_buffer, dword *plane_offsets) {
    byte plane, pixel;
    word x, y;
    dword offset = 0, output_pos = 0;
    bool calculate_only = output_buffer == NULL;

    for(plane = 0; plane < 4; plane++){
        if(!calculate_only) {
            plane_offsets[plane] = output_pos;
            // memcpy(&output_buffer[output_pos], &plane_select_opcodes, 17);
            // printf("asdfasdf\n");
        }
        // output_pos += 17;
        for(x = plane; x < width; x += 4) {
            offset = 0;
            for(y = 0; y < height; y++) {
                pixel = sprite_buffer[y * width + x];
                if(pixel != 0) {
                    if(!calculate_only) {
                        output_buffer[output_pos++] = 0xc6;
                        output_buffer[output_pos++] = 0x87;
                        output_buffer[output_pos++] = (byte) offset;
                        output_buffer[output_pos++] = (byte) (offset >> 8);
                        output_buffer[output_pos++] = (byte) (offset >> 16);
                        output_buffer[output_pos++] = (byte) (offset >> 24);
                        output_buffer[output_pos++] = (byte) pixel;
                        // printf("mov byte [edi + 0x%x], 0x%x\n", offset, pixel);
                    } else {
                        output_pos += 7;
                    }
                }
                offset += PAGE_WIDTH >> 2;  // divide by 4
            }
            if(!calculate_only) {
                output_buffer[output_pos++] = 0x01;
                output_buffer[output_pos++] = 0xDF;
                // printf("add edi, ebx\n");
            } else {
                output_pos += 2;
            }
        }
        if(!calculate_only) {
            output_buffer[output_pos++] = 0xc3;  // ret
            // printf("ret\n");
        } else {
            output_pos++;
        }
    }

    return output_pos;
}

dword spr_compile_planar_sprite_scheme_2(byte *sprite_buffer, word width, word height, byte *output_buffer, dword *plane_offsets) {
    byte plane, pixel;
    word x, y;
    dword src_offset, dest_offset, buffer_length = 0, sprite_offset = 0;
    dword dest_row_offset = (PAGE_WIDTH - width) >> 2; // divide by 4
    bool calculate_only = output_buffer == NULL;

    for(plane = 0; plane < 4; plane++){
        if(!calculate_only) {
            plane_offsets[plane] = buffer_length;
            sprite_offset = buffer_length;
        }
        buffer_length += (width * height) >> 2;
        src_offset = 0;
        dest_offset = 0;
        for(y = 0; y < height; y++)
        {
            for(x = plane; x < width; x += 4) {
                pixel = sprite_buffer[y * width + x];
                if(!calculate_only) {
                    output_buffer[sprite_offset++] = pixel;
                }
                if(pixel != 0) {
                    if(src_offset > 0) {
                        if(!calculate_only) {
                            output_buffer[buffer_length++] = src_offset > 127 ? 0x81 : 0x83;
                            output_buffer[buffer_length++] = 0xc6;
                            output_buffer[buffer_length++] = (byte) src_offset;
                            if(src_offset > 127) {
                                output_buffer[buffer_length++] = (byte) (src_offset >> 8);
                                output_buffer[buffer_length++] = (byte) (src_offset >> 16);
                                output_buffer[buffer_length++] = (byte) (src_offset >> 24);
                            }
                            // printf("add esi, %d\n", src_offset);
                        } else {
                            buffer_length += src_offset > 127 ? 6 : 3;
                        }
                        src_offset = 0;
                    }
                    if(dest_offset > 0) {
                        if(!calculate_only) {
                            output_buffer[buffer_length++] = dest_offset > 127 ? 0x81 : 0x83;
                            output_buffer[buffer_length++] = 0xc7;
                            output_buffer[buffer_length++] = (byte) dest_offset;
                            if(dest_offset > 127) {
                                output_buffer[buffer_length++] = (byte) (dest_offset >> 8);
                                output_buffer[buffer_length++] = (byte) (dest_offset >> 16);
                                output_buffer[buffer_length++] = (byte) (dest_offset >> 24);
                            }
                            // printf("add edi, %d\n", dest_offset);
                        } else {
                            buffer_length += dest_offset > 127 ? 6 : 3;
                        }
                        dest_offset = 0;
                    }
                    if(!calculate_only) {
                        output_buffer[buffer_length++] = 0xa4;
                        // printf("movsb\n");
                    } else {
                        buffer_length++;
                    }
                } else {
                    src_offset++;
                    dest_offset++;
                }
            }
            dest_offset += dest_row_offset;
        }
        if(!calculate_only) {
            output_buffer[buffer_length++] = 0xc3;
            // printf("ret\n");
        } else {
            buffer_length++;
        }
    }

    return buffer_length;
}