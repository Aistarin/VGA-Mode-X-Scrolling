@ECHO OFF

SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a
REM SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -za -bt=dos -l=dos32a
REM SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -za -bt=dos -l=dos4g
"C:\Program Files\NASM\nasm" src/gfx/gfx.asm -fobj -o obj/gfx-asm.obj -g

WCL386 src\main.c -c -fo=obj\main.obj
WCL386 src\gfx\vga.c -c -fo=obj\vga.obj
WCL386 src\gfx\gfx.c -c -fo=obj\gfx.obj
WCL386 src\gfx\spr.c -c -fo=obj\spr.obj
WCL386 src\timer.c -c -fo=obj\timer.obj
WCL386 src\keyboard.c -c -fo=obj\keyboard.obj
WCL386 src\bitmap.c -c -fo=obj\bitmap.obj


IF EXIST *.ERR GOTO CompilerError
GOTO CompilerOK

:CompilerError
    ECHO COMPILATION FAILED: ERRORS FOUND!
    ECHO =-------------------------------=
    ECHO List of error files:
    DIR
    GOTO EndOfBatch

:CompilerOK
    REM Link the EXE
    ECHO Compilation OK
    ECHO =------------=
    WCL386 obj\*.obj -fe=build\SCROLL.EXE

ECHO Done!

:EndOfBatch