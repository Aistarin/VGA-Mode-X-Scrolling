include .env

export PATH := $(WATCOM_PATH):$(PATH)
export INCLUDE := $(WATCOM_INCLUDE):$(INCLUDE)
export WATCOM := $(WATCOM)
export EDPATH := $(EDPATH)
export WIPFC := $(WIPFC)

scroll : main.obj vga.obj gfx.obj gfx-asm.obj spr.obj timer.obj keyboard.obj bitmap.obj
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) obj/*.obj -fe=build/scroll.exe

main.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/main.c -c -fo=obj/main.obj

gfx.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/gfx/gfx.c -c -fo=obj/gfx.obj

gfx-asm.obj :
	$(NASM) src/gfx/gfx.asm -fobj -o obj/gfx-asm.obj -g

spr.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/gfx/spr.c -c -fo=obj/spr.obj

vga.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/gfx/vga.c -c -fo=obj/vga.obj

timer.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/io/timer.c -c -fo=obj/timer.obj

keyboard.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/io/keyboard.c -c -fo=obj/keyboard.obj

bitmap.obj :
	wcl386 -zdp -wcd=138 -ecc -4s -mf -fp3 -za -bt=dos -l=$(DOS_EXTENDER) src/io/bitmap.c -c -fo=obj/bitmap.obj

run :
	$(DOSBOX) build/scroll.exe

clean :
	rm obj/*.obj build/scroll.exe
