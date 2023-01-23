%include "src/util.asm"

SEGMENT _DATA PUBLIC ALIGN=4 USE32 class=DATA
    VGA_START       equ 0xA0000

    PAGE_WIDTH      equ 84              ;; page width per plane, 336 / 4

    GLOBAL _modexvar_active_start
    GLOBAL _modexvar_visible_start

    _modexvar_active_start  dd 0
    _modexvar_visible_start dd 0

SEGMENT _TEXT PUBLIC ALIGN=4 USE32 class=CODE
GROUP DGROUP _DATA

GLOBAL _gfx_blit_sprite
_gfx_blit_sprite: FUNCTION
    %arg initial_vga_offset:dword, sprite_offset:dword, sprite_width:byte, sprite_height:byte
    pusha

    mov ebx, dword [initial_vga_offset] ;; set initial vga offset aside

    mov esi, dword [sprite_offset]      ;; source set to sprite buffer

    mov dl, byte [sprite_height]        ;; save sprite height into register

    mov ch, byte [sprite_width]         ;; set high counter byte to sprite width

    outerLoop:
        mov cl, dl                      ;; set low counter byte to sprite height
        mov edi, ebx                    ;; load initial vga offset
    loadFirstPixel:
        lodsb                           ;; load pixel that sprite_offset points to into al
        test al, al
        jnz storeFirstPixel
        inc edi
        jmp loadSecondPixel
    storeFirstPixel:
        stosb                           ;; save pixel loaded into al to vga offset
    loadSecondPixel:
        add edi, PAGE_WIDTH - 1         ;; move down one pixel by adding a row of pixels
        lodsb                           ;; load pixel that sprite_offset points to into al
        test al, al
        jnz storeSecondPixel
        inc edi
        jmp innerLoopEnd
    storeSecondPixel:
        stosb                           ;; save pixel loaded into al to vga offset
    innerLoopEnd:
        add edi, PAGE_WIDTH - 1         ;; move down one pixel by adding a row of pixels
        sub cl, 2
        jnz loadFirstPixel
    innerLoopDone:
        inc ebx                         ;; move initial vga offset to next row
        dec ch
        jnz outerLoop
    done:
        popa
ENDFUNCTION
