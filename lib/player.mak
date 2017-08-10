
WATT32_INC = ..\..\WATT32\INC

device.obj: device.c device.h
	wcc386 device.c -i=$(WATT32_INC) -dSYNC_PLAYER

track.obj: track.c track.h
	wcc386 track.c -i=$(WATT32_INC) -dSYNC_PLAYER

test.obj: test.cpp
	wpp386 test.cpp -i=$(WATT32_INC) -dSYNC_PLAYER

player.exe: test.obj device.obj track.obj
	wlink @objs.arg @libs.arg system pmodew name player.exe

all: player.exe device.obj track.obj test.obj

clean: .SYMBOLIC
	del *.exe
	del *.obj


