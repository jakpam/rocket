
WATT32_INC = ..\..\WATT32\INC

device.obj: device.c device.h
	wcc386 device.c -i=$(WATT32_INC)

track.obj: track.c track.h
	wcc386 track.c -i=$(WATT32_INC)

test.obj: test.cpp
	wpp386 test.cpp -i=$(WATT32_INC)

test.exe: test.obj device.obj track.obj
	wlink @objs.arg @libs.arg system pmodew name test.exe


