CC      ?= i686-w64-mingw32-gcc
ifeq ($(origin CC),default)
CC = i686-w64-mingw32-gcc
endif
CFLAGS  = -O2 -Wall -Wextra -Wno-unused-parameter
LDFLAGS = -shared -static-libgcc -s -Wl,--enable-stdcall-fixup
SRC     = src/highfps.c src/proxy.c src/proxy_stubs.S src/winmm.def

build/winmm.dll: $(SRC) src/highfps.h src/proxy_names.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

dist: build/winmm.dll
	rm -rf dist && mkdir -p dist/isaac-highfps
	cp build/winmm.dll highfps.ini dist/isaac-highfps/
	cp dist-readme.txt dist/isaac-highfps/README.txt
	cd dist && zip -r isaac-highfps.zip isaac-highfps

clean:
	rm -rf build dist
.PHONY: dist clean
