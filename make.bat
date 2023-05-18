@ECHO OFF

SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a
REM SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -za -bt=dos -l=dos32a
REM SET WCL386=-zdp -wcd=138 -ecc -4s -mf -fp3 -od -d2 -za -bt=dos -l=dos4g 
REM "C:\Program Files\NASM\nasm" modex.asm -fobj -o obj\modex.obj -g

WCL386 src\main.c -c -fo=obj\main.obj
WCL386 src\vga.c -c -fo=obj\vga.obj
WCL386 src\gfx.c -c -fo=obj\gfx.obj
WCL386 src\spr.c -c -fo=obj\spr.obj


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