CC      ?= i686-w64-mingw32-gcc
ifeq ($(origin CC),default)
CC = i686-w64-mingw32-gcc
endif
CFLAGS  = -O2 -Wall -Wextra -Wno-unused-parameter
LDFLAGS = -shared -static-libgcc -s -Wl,--enable-stdcall-fixup
SRC     = src/morefps.c src/proxy.c src/proxy_stubs.S src/winmm.def

build/winmm.dll: $(SRC) src/morefps.h src/proxy_names.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

dist: build/winmm.dll
	rm -rf dist && mkdir -p dist/isaac-at-more-fps
	cp build/winmm.dll morefps.ini dist/isaac-at-more-fps/
	cp installer/install.bat installer/uninstall.bat installer/install.ps1 installer/install-linux.sh dist/isaac-at-more-fps/
	cp dist-readme.txt dist/isaac-at-more-fps/README.txt
	cd dist && zip -r isaac-at-more-fps.zip isaac-at-more-fps

clean:
	rm -rf build dist
.PHONY: dist clean
