    SECTION .data
    PAGE_WIDTH      equ 84              ;; page width per plane, 336 / 4
    sprite_offset   equ 1611154
    initial_vga_offset equ 678216
    sprite_width    equ 8
    x_min           equ 0
    x_max           equ 0
    y_min           equ 0
    y_max           equ 56

    SECTION .text
    global _start

_start:
    ;; clear EAX
    mov eax, 0

    mov esi, sprite_offset
    mov edi, initial_vga_offset

    ;; store x_min and x_max in lower bytes of EBX
    mov bl, byte x_min
    mov bh, byte x_max

    ;; subtract sprite width and page offset by the difference of
    ;; x_min and x_max and store in the upper bytes of DX
    ;; NOTE: since we are defining PAGE_WIDTH to be 1/4th of the
    ;; true page width (336px), this should always fit within a byte
    mov dl, byte sprite_width
    sub dl, bh
    add dl, bl
    mov dh, PAGE_WIDTH
    sub dh, bh
    add dh, bl

    ;; store y_min and y_max in upper bytes of EBX
    rol ebx, 16
    mov bl, byte y_min
    mov bh, byte y_max

    ;; set upper counter bits to y_min
    mov ch, bl

    outerClipLoop:
        rol ebx, 16                     ;; move x_min and x_max into lower bits of EBX
        mov cl, bl                      ;; set lower counter bits to x_min
    innerClipLoop:
        inc esi                         ;; increments ESI
        test al, al
        jz skipClipPixel                ;; if pixel stored in AL is 0, increment EDI
        inc edi                         ;; increments EDI
    innerClipLoopEnd:
        inc cl
        cmp cl, bh
        jl innerClipLoop                ;; loop again if lower counter != x_max
        mov al, dl                      ;; sprite_width - x_max + x_min
        add esi, eax
        mov al, dh                      ;; PAGE_WIDTH - x_max + x_min
        add edi, eax
        rol ebx, 16                     ;; move y_min and y_max into lower bits of EBX
        inc ch
        cmp ch, bh
        jl outerClipLoop                ;; loop again if upper counter != y_max
        jmp clipDone
    skipClipPixel:
        inc edi
        jmp innerClipLoopEnd
    clipDone:
        ret
