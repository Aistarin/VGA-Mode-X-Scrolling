;; Utility macros for use in other assembly files

%ifndef UTIL_ASM
%define UTIL_ASM

;; This macro does the usual C-callable function boilerplate.
;; You'll still have to push/pop any registers you clobber manually. Some are safe to clobber,
;; others are not.
%macro FUNCTION 0
    %push
    %stacksize flat
    push ebp
    mov ebp, esp
%endmacro

%macro ENDFUNCTION 0
    mov esp, ebp
    pop ebp
    %pop
    ret
%endmacro

%macro SETREG 2
    mov     dx, %1
    mov     ah, al
    mov     al, %2
    out     dx, ax
%endmacro

%endif