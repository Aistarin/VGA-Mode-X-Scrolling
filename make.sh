source /usr/bin/watcom/owsetenv.sh

wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/main.c -c -fo=obj/main.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/vga.c -c -fo=obj/vga.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a src/gfx.c -c -fo=obj/gfx.obj
wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=dos32a obj/*.obj -fe=build/scroll.exe