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
    cld

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

GLOBAL _gfx_blit_clipped_sprite
_gfx_blit_clipped_sprite: FUNCTION
    %arg initial_vga_offset:dword, sprite_offset:dword, sprite_width:byte, x_min:byte, x_max:byte, y_min:byte, y_max:byte
    pusha
    cld

    ;; clear EAX
    mov eax, 0

    mov esi, dword [sprite_offset]
    mov edi, dword [initial_vga_offset]

    ;; store x_min and x_max in lower bytes of EBX
    mov bl, byte [x_min]
    mov bh, byte [x_max]

    ;; subtract sprite width and page offset by the difference of
    ;; x_min and x_max and store in the upper bytes of DX
    ;; NOTE: since we are defining PAGE_WIDTH to be 1/4th of the
    ;; true page width (336px), this should always fit within a byte
    mov dl, byte [sprite_width]
    sub dl, bh
    add dl, bl
    mov dh, PAGE_WIDTH
    sub dh, bh
    add dh, bl

    ;; store y_min and y_max in upper bytes of EBX
    rol ebx, 16
    mov bl, byte [y_min]
    mov bh, byte [y_max]

    ;; set upper counter bits to y_min
    mov ch, bl

    outerClipLoop:
        cmp ch, bh
        jge clipDone                    ;; break out of outer loop if upper counter >= y_max
        rol ebx, 16                     ;; move x_min and x_max into lower bits of EBX
        mov cl, bl                      ;; set lower counter bits to x_min
    innerClipLoop:
        cmp cl, bh
        jge innerClipLoopEnd            ;; break out of inner loop if lower counter >= x_max
        lodsb                           ;; increments ESI
        test al, al
        jz skipClipPixel                ;; if pixel stored in AL is 0, increment EDI
        stosb                           ;; increments EDI
        inc cl
        jmp innerClipLoop
    skipClipPixel:
        inc edi
        inc cl
        jmp innerClipLoop
    innerClipLoopEnd:
        mov al, dl                      ;; sprite_width - x_max + x_min
        add esi, eax
        mov al, dh                      ;; PAGE_WIDTH - x_max + x_min
        add edi, eax
        rol ebx, 16                     ;; move y_min and y_max into lower bits of EBX
        inc ch
        jmp outerClipLoop
    clipDone:
        popa

ENDFUNCTION

GLOBAL _gfx_blit_16_x_16_tile
_gfx_blit_16_x_16_tile: FUNCTION
    %arg vga_offset:dword, tile_offset:dword
    pusha
    cld

    mov edi, dword [vga_offset]         ;; set destination to where tile will be drawn in VRAM
    mov esi, dword [tile_offset]        ;; set source to where tile is located in VRAM

    ;; unrolled loop where we latch copy 4 pixels for each movsb (16 in total), incrementing
    ;; esi and edi by the page width to go down into the next row
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb
    add edi, PAGE_WIDTH - 4
    movsb
    movsb
    movsb
    movsb

    popa
ENDFUNCTION

GLOBAL _gfx_blit_8_x_8_tile
_gfx_blit_8_x_8_tile: FUNCTION
    %arg vga_offset:dword, tile_offset:dword
    pusha
    cld

    mov edi, dword [vga_offset]         ;; set destination to where tile will be drawn in VRAM
    mov esi, dword [tile_offset]        ;; set source to where tile is located in VRAM

    ;; unrolled loop where we latch copy 4 pixels for each movsb (8 in total), incrementing
    ;; esi and edi by the page width to go down into the next row
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb
    add edi, PAGE_WIDTH - 2
    movsb
    movsb

    popa
ENDFUNCTION

GLOBAL _gfx_blit_compiled_planar_sprite_scheme_1
_gfx_blit_compiled_planar_sprite_scheme_1: FUNCTION
    %arg vga_offset:dword, sprite_offset:dword, iter:dword
    pusha
    cld

    mov ebx, [iter]
    mov edi, dword [vga_offset]         ;; set destination to where sprite will be drawn in VRAM

    call dword [sprite_offset]

    popa
ENDFUNCTION

GLOBAL _gfx_blit_compiled_planar_sprite_scheme_2
_gfx_blit_compiled_planar_sprite_scheme_2: FUNCTION
    %arg vga_offset:dword, sprite_offset:dword, sprite_data_offset:dword
    pusha
    cld

    mov esi, [sprite_data_offset]
    mov edi, dword [vga_offset]         ;; set destination to where sprite will be drawn in VRAM

    call dword [sprite_offset]

    popa
ENDFUNCTION
