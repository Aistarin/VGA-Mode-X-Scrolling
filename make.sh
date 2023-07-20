source /usr/bin/watcom/owsetenv.sh

nasm src/gfx/gfx.asm -fobj -o obj/gfx-asm.obj -g

wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/main.c -c -fo=obj/main.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/gfx/vga.c -c -fo=obj/vga.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/gfx/gfx.c -c -fo=obj/gfx.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/gfx/spr.c -c -fo=obj/spr.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/timer.c -c -fo=obj/timer.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/keyboard.c -c -fo=obj/keyboard.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/bitmap.c -c -fo=obj/bitmap.obj

wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a obj/*.obj -fe=build/scroll.exe
