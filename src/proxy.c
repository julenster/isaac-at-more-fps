// Proxy de winmm.dll: reenvía todas las funciones a la winmm.dll real del sistema.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "proxy_names.h"

FARPROC proxy_table[PROXY_COUNT];  // usada por los stubs de proxy_stubs.S
static HMODULE real;

static void __stdcall missing(void) {}  // por si alguna función no existe en la DLL real

int proxy_init(void) {
    char path[MAX_PATH];
    UINT n = GetSystemDirectoryA(path, MAX_PATH);
    if (!n || n > MAX_PATH - 16) return 0;
    lstrcatA(path, "\\" PROXY_DLL);
    real = LoadLibraryA(path);
    if (!real) return 0;
    for (int i = 0; i < PROXY_COUNT; i++) {
        proxy_table[i] = GetProcAddress(real, proxy_names[i]);
        if (!proxy_table[i]) proxy_table[i] = (FARPROC)missing;
    }
    return 1;
}
