
WATT32_INC = ..\..\WATT32\INC

device.obj: device.c device.h
	wcc386 device.c -i=$(WATT32_INC) -dUSE_WATTCP32

track.obj: track.c track.h
	wcc386 track.c -i=$(WATT32_INC) -dUSE_WATTCP32

test.obj: test.cpp
	wpp386 test.cpp -i=$(WATT32_INC) -dUSE_WATTCP32

test.exe: test.obj device.obj track.obj
	wlink @objs.arg @libs.arg system pmodew name test.exe

rockedit.lib: device.obj track.obj
	wlib rockplay.lib +-device.obj +-track.obj

all: test.exe device.obj track.obj test.obj

clean: .SYMBOLIC
	del *.exe
	del *.obj



