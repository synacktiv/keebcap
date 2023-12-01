.PHONY: all

all: bins

clean:
	rm -rf ./bins

bins:
	mkdir -p ./bins
	x86_64-w64-mingw32-g++ -Os -flto -mconsole -static -fPIC -fPIE	-o ./bins/dmp.exe         dumper.cpp
	x86_64-w64-mingw32-gcc -Os -flto -mconsole                      -o ./bins/cap_rid.exe     cap_rid.c process.c
	x86_64-w64-mingw32-gcc -Os -flto -mconsole                      -o ./bins/cap_llh.exe     cap_llh.c process.c
	x86_64-w64-mingw32-gcc -Os -flto -mconsole                      -o ./bins/cap_gks.exe     cap_gks.c process.c
