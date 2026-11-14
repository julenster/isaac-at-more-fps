// isaac240 (Repentance+): FPS altos con la lógica del juego intacta.
//
// Cómo funciona:
//  - El bucle de IsaacMain deja de limitarse a 60 vueltas/s (redirigimos sus 3 lecturas de 1/60);
//    lo limita el VSync del monitor o max_fps.
//  - El IsaacUpdate original (lógica, input, cámara...) se ejecuta por reloj: 60 veces por segundo
//    de tiempo real, sea cual sea el refresco del monitor.
//  - En cada frame dibujado recolocamos lo que se ve en la fracción de tiempo transcurrida:
//      * entidades: restauramos su estado tras el último tick, llamamos a su Interpolate()
//        original (avanza medio tick) y escalamos el desplazamiento a la fracción que toca.
//        Es genérico: funciona con todas las clases sin parchear su código.
//      * jugadores y cámara: interpolación real entre sus dos últimos valores de 60Hz.
//  - Antes de cada frame original restauramos exactamente el estado que el juego espera.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "morefps.h"

uintptr_t g_delta;
#define MOREFPS_VERSION "1.0.0"
static double frame_time = 1.0 / 1000.0;
static FILE *logfile;
static int fpslog;

void logf_(const char *fmt, ...) {
    if (!logfile) return;
    va_list ap; va_start(ap, fmt);
    fprintf(logfile, "[morefps] ");
    vfprintf(logfile, fmt, ap);
    fputc('\n', logfile);
    fflush(logfile);
    va_end(ap);
}

#define U8(p, o)  (*(volatile uint8_t  *)((char *)(p) + (o)))
#define I32(p, o) (*(volatile int32_t  *)((char *)(p) + (o)))
#define U32(p, o) (*(volatile uint32_t *)((char *)(p) + (o)))
#define F32(p, o) (*(volatile float    *)((char *)(p) + (o)))
#define PTR(p, o) (*(void *volatile    *)((char *)(p) + (o)))

typedef void (*fn_void)(void);
typedef int  (__attribute__((thiscall)) *fn_this_ret)(void *);
typedef void (__attribute__((thiscall)) *fn_this)(void *);

static void *mgr(void)  { return *(void **)REB(ADDR_g_Manager); }
static void *game(void) { return *(void **)REB(ADDR_g_Game); }

// ---------------------------------------------------------------- snapshots
typedef struct { float x, y; } vec2;
typedef struct {
    void *e;
    vec2 pos, backup;
    uint64_t flags;
    uint8_t interp;
    float height;
} ent_state;

#define MAX_ENTS 8192
#define HASH_SZ  16384  // potencia de 2, > 2*MAX_ENTS
typedef struct {
    ent_state s[MAX_ENTS];
    int n;
    int16_t hash[HASH_SZ];  // índice+1, 0 = vacío
    int valid;
} snapshot;

static snapshot snapA;  // tras un frame original par (tick lógico)
static snapshot snapB;  // tras un frame original impar (interpolación original)

static unsigned hptr(void *p) { return ((uintptr_t)p >> 3) * 2654435761u; }

static void snap_take(snapshot *sn, void *room) {
    void **ents = PTR(room, ROOM_ENTITIES);
    uint32_t n = U32(room, ROOM_NUM_ENTITIES);
    memset(sn->hash, 0, sizeof sn->hash);
    sn->n = 0;
    for (uint32_t i = 0; i < n && sn->n < MAX_ENTS; i++) {
        char *e = ents[i];
        if (I32(e, ENT_TYPE) == 1) continue;  // jugadores aparte
        ent_state *s = &sn->s[sn->n];
        s->e = e;
        s->pos = *(vec2 *)(e + ENT_POS);
        s->backup = *(vec2 *)(e + ENT_POS_BACKUP);
        s->flags = *(uint64_t *)(e + ENT_FLAGS);
        s->interp = U8(e, ENT_INTERPOLATED);
        s->height = F32(e, ENT_HEIGHT);
        unsigned h = hptr(e) & (HASH_SZ - 1);
        while (sn->hash[h]) h = (h + 1) & (HASH_SZ - 1);
        sn->hash[h] = (int16_t)(sn->n + 1);
        sn->n++;
    }
    sn->valid = 1;
}

static ent_state *snap_find(snapshot *sn, void *e) {
    unsigned h = hptr(e) & (HASH_SZ - 1);
    while (sn->hash[h]) {
        ent_state *s = &sn->s[sn->hash[h] - 1];
        if (s->e == e) return s;
        h = (h + 1) & (HASH_SZ - 1);
    }
    return NULL;
}

static void state_apply(const ent_state *s) {
    char *e = s->e;
    *(vec2 *)(e + ENT_POS) = s->pos;
    *(vec2 *)(e + ENT_POS_BACKUP) = s->backup;
    *(uint64_t *)(e + ENT_FLAGS) = s->flags;
    U8(e, ENT_INTERPOLATED) = s->interp;
    F32(e, ENT_HEIGHT) = s->height;
}

// Restaura las entidades que siguen vivas y estaban en el snapshot
static void snap_restore(snapshot *sn, void *room) {
    if (!sn->valid) return;
    void **ents = PTR(room, ROOM_ENTITIES);
    uint32_t n = U32(room, ROOM_NUM_ENTITIES);
    for (uint32_t i = 0; i < n; i++) {
        ent_state *s = snap_find(sn, ents[i]);
        if (s) state_apply(s);
    }
}

// ---------------------------------------------------------------- jugadores
#define MAX_PLAYERS 64
#define SNAP_DIST 40.0f
static struct { void *e; vec2 prev, cur, shown; int modified; uint8_t orig_interp; vec2 orig_backup; } pl[MAX_PLAYERS];
static int pl_n;

static void players_record(void *room) {
    void **ents = PTR(room, ROOM_ENTITIES);
    uint32_t n = U32(room, ROOM_NUM_ENTITIES);
    int m = 0;
    for (uint32_t i = 0; i < n && m < MAX_PLAYERS; i++) {
        char *e = ents[i];
        if (I32(e, ENT_TYPE) != 1) continue;
        vec2 cur = *(vec2 *)(e + ENT_POS), prev = cur;
        for (int k = 0; k < pl_n; k++)
            if (pl[k].e == e) { prev = pl[k].cur; break; }
        float dx = cur.x - prev.x, dy = cur.y - prev.y;
        if (dx * dx + dy * dy > SNAP_DIST * SNAP_DIST) prev = cur;
        pl[m].e = e; pl[m].prev = prev; pl[m].cur = cur; pl[m].modified = 0;
        m++;
    }
    pl_n = m;
}

static void players_show(float t) {
    for (int k = 0; k < pl_n; k++) {
        char *e = pl[k].e;
        vec2 *pos = (vec2 *)(e + ENT_POS);
        if (!pl[k].modified) {
            if (pos->x != pl[k].cur.x || pos->y != pl[k].cur.y) continue;  // algo lo movió
            pl[k].orig_interp = U8(e, ENT_INTERPOLATED);
            pl[k].orig_backup = *(vec2 *)(e + ENT_POS_BACKUP);
            pl[k].modified = 1;
        }
        pos->x = pl[k].prev.x + (pl[k].cur.x - pl[k].prev.x) * t;
        pos->y = pl[k].prev.y + (pl[k].cur.y - pl[k].prev.y) * t;
        pl[k].shown = *pos;
        // Por si el juego hace su revert antes que nosotros: que vuelva a la posición real
        *(vec2 *)(e + ENT_POS_BACKUP) = pl[k].cur;
        U8(e, ENT_INTERPOLATED) = 1;
    }
}

// Deshace players_show antes de un frame original
static void players_revert(void) {
    for (int k = 0; k < pl_n; k++) {
        if (!pl[k].modified) continue;
        char *e = pl[k].e;
        vec2 *pos = (vec2 *)(e + ENT_POS);
        if (pos->x == pl[k].shown.x && pos->y == pl[k].shown.y) *pos = pl[k].cur;
        *(vec2 *)(e + ENT_POS_BACKUP) = pl[k].orig_backup;
        U8(e, ENT_INTERPOLATED) = pl[k].orig_interp;
        pl[k].modified = 0;
    }
}

// ---------------------------------------------------------------- cámara
// La cámara se actualiza a 60Hz y deja su posición en room->RenderScrollOffset.
// En los subframes mostramos un valor interpolado entre sus dos últimos valores.
#define CAM_SNAP_DIST 60.0f
static struct { void *room; vec2 prev, cur, shown; int modified, have; } cam;

static void camera_record(void *room) {
    vec2 cur = *(vec2 *)((char *)room + ROOM_RENDER_SCROLL);
    vec2 prev = (cam.have && cam.room == room) ? cam.cur : cur;
    float dx = cur.x - prev.x, dy = cur.y - prev.y;
    if (dx * dx + dy * dy > CAM_SNAP_DIST * CAM_SNAP_DIST) prev = cur;
    cam.room = room; cam.prev = prev; cam.cur = cur; cam.have = 1; cam.modified = 0;
}

static void camera_show(float t) {
    if (!cam.have) return;
    vec2 *off = (vec2 *)((char *)cam.room + ROOM_RENDER_SCROLL);
    if (!cam.modified && (off->x != cam.cur.x || off->y != cam.cur.y)) return;
    off->x = cam.prev.x + (cam.cur.x - cam.prev.x) * t;
    off->y = cam.prev.y + (cam.cur.y - cam.prev.y) * t;
    cam.shown = *off;
    cam.modified = 1;
}

static void camera_revert(void) {
    if (!cam.modified) return;
    vec2 *off = (vec2 *)((char *)cam.room + ROOM_RENDER_SCROLL);
    if (off->x == cam.shown.x && off->y == cam.shown.y) *off = cam.cur;
    cam.modified = 0;
}

// ---------------------------------------------------------------- subframes
static int can_interpolate(void *m, void *g) {
    if (!g || !PTR(g, GAME_ROOM)) return 0;
    if (I32(m, MGR_STATE) != 2 || U8(m, MGR_BLOCK_GAME) || !U8(m, MGR_INTERP_ENABLED)) return 0;
    if (I32(g, GAME_TRANSITION_A) || I32(g, GAME_TRANSITION_B)) return 0;
    if (((fn_this_ret)REB(ADDR_Game_IsPaused))(g)) return 0;
    if (U8(m, MGR_GATE_A) == 0 && U8(m, MGR_GATE_B) != 0 && I32(m, MGR_GATE_C1) == I32(m, MGR_GATE_C2)) return 0;
    return 1;
}

// Interpola las entidades (no jugadores) a la fracción frac del tick de 30Hz
static void entities_subframe(void *room, float frac) {
    float s = frac / 0.5f;  // Interpolate() original avanza 0.5 ticks
    void **ents = PTR(room, ROOM_ENTITIES);
    uint32_t n = U32(room, ROOM_NUM_ENTITIES);
    // 1) volver al estado tras el tick
    for (uint32_t i = 0; i < n; i++) {
        ent_state *st = snap_find(&snapA, ents[i]);
        if (st) state_apply(st);
    }
    // 2) Interpolate() original de cada entidad
    for (uint32_t i = 0; i < n; i++) {
        char *e = ents[i];
        if (I32(e, ENT_TYPE) == 1 || !snap_find(&snapA, e)) continue;
        void **vt = *(void ***)e;
        ((fn_this)vt[VT_INTERPOLATE])(e);
    }
    // 3) escalar lo que se ha movido a la fracción que toca
    for (uint32_t i = 0; i < n; i++) {
        char *e = ents[i];
        if (I32(e, ENT_TYPE) == 1) continue;
        ent_state *st = snap_find(&snapA, e);
        if (!st) continue;
        vec2 *pos = (vec2 *)(e + ENT_POS);
        pos->x = st->pos.x + (pos->x - st->pos.x) * s;
        pos->y = st->pos.y + (pos->y - st->pos.y) * s;
        F32(e, ENT_HEIGHT) = st->height + (F32(e, ENT_HEIGHT) - st->height) * s;
    }
    U8(room, ROOM_INTERP_DONE) = 1;
}

// ---------------------------------------------------------------- bucle
static int last_phase;
static void *snap_room;

// Un frame original de 60Hz del juego, dejando antes todo como el juego lo espera
static void run_original_frame(void) {
    void *m = mgr(), *g = game();
    void *room = g ? PTR(g, GAME_ROOM) : NULL;
    int phase = I32(m, MGR_FRAMECOUNT) & 1;  // 0 = tick lógico, 1 = frame intermedio
    players_revert();
    if (room && room == cam.room) camera_revert();
    if (room && room == snap_room) {
        if (phase == 1) snap_restore(&snapA, room);   // espera el estado tras el tick
        else            snap_restore(&snapB, room);   // espera el estado tras su interpolación
    }
    ((fn_void)REB(ADDR_IsaacUpdate))();

    g = game(); room = g ? PTR(g, GAME_ROOM) : NULL;
    last_phase = phase;
    if (room) {
        if (phase == 0) { snap_take(&snapA, room); snapB.valid = 0; }
        else            { snap_take(&snapB, room); }
        snap_room = room;
        players_record(room);
        camera_record(room);
    } else {
        snapA.valid = snapB.valid = 0; snap_room = NULL; pl_n = 0; cam.have = 0;
    }
}

// ---------------------------------------------------------------- bucle por reloj
// El juego avanza exactamente 60 veces por segundo de tiempo real; se dibujan tantos
// frames como dé el monitor (VSync) o el límite configurado, cada uno en su fracción.
#define TICK        (1.0 / 60.0)
#define TOLERANCE   0.0015   // margen para no perder/duplicar frames por jitter con VSync
#define MAX_CATCHUP 3
static LARGE_INTEGER qpf, last_t;
static double acc;

static void fps_meter(double dt, int ran) {
    static double t; static int frames, logic;
    t += dt; frames++; logic += ran;
    if (t >= 2.0) {
        LOG("%.1f FPS | state=%d | logic frames: %.1f/s", frames / t, I32(mgr(), MGR_STATE), logic / t);
        t = 0; frames = logic = 0;
    }
}

static void my_isaac_update(void) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = last_t.QuadPart ? (double)(now.QuadPart - last_t.QuadPart) / qpf.QuadPart : TICK;
    last_t = now;
    if (dt > 0.25) dt = 0.25;  // tras un tirón (carga, ventana movida...) no intentar recuperar
    acc += dt;

    int ran = 0;
    while (acc >= TICK - TOLERANCE && ran < MAX_CATCHUP) {
        run_original_frame();
        acc -= TICK;
        ran++;
    }
    if (acc > TICK) acc = TICK;
    if (fpslog) fps_meter(dt, ran);

    // Dibujo: fracción del frame de 60Hz transcurrida
    void *m = mgr(), *g = game();
    void *room = g ? PTR(g, GAME_ROOM) : NULL;
    if (room && room == snap_room && snapA.valid && can_interpolate(m, g)) {
        float alpha = (float)(acc / TICK);
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        entities_subframe(room, (alpha + (float)last_phase) * 0.5f);
        players_show(alpha);
        camera_show(alpha);
    }
}

// ---------------------------------------------------------------- instalación
static int write_mem(uintptr_t addr, const void *src, size_t n) {
    DWORD old;
    if (!VirtualProtect((void *)addr, n, PAGE_EXECUTE_READWRITE, &old)) return 0;
    memcpy((void *)addr, src, n);
    VirtualProtect((void *)addr, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)addr, n);
    return 1;
}

static void install(void) {
    char ini[MAX_PATH], buf[32];
    GetModuleFileNameA(NULL, ini, MAX_PATH);
    char *slash = strrchr(ini, '\\');
    if (slash) slash[1] = 0;
    char logpath[MAX_PATH];
    lstrcpyA(logpath, ini); lstrcatA(logpath, "morefps.log");
    lstrcatA(ini, "morefps.ini");
    logfile = fopen(logpath, "w");

    int enabled = GetPrivateProfileIntA("morefps", "enabled", 1, ini);
    int max_fps = GetPrivateProfileIntA("morefps", "max_fps", 0, ini);
    fpslog      = GetPrivateProfileIntA("morefps", "log_fps", 0, ini);
    if (GetEnvironmentVariableA("MOREFPS_LOG", buf, sizeof buf) > 0) fpslog = 1;
    if (GetEnvironmentVariableA("MOREFPS_MAXFPS", buf, sizeof buf) > 0) max_fps = atoi(buf);

    g_delta = (uintptr_t)GetModuleHandleA(NULL) - 0x400000;
    QueryPerformanceFrequency(&qpf);
    LOG("Isaac at More FPS v" MOREFPS_VERSION " | base=%p | max_fps=%d (0 = unlimited / VSync)",
        GetModuleHandleA(NULL), max_fps);
    if (!enabled) { LOG("disabled in morefps.ini"); return; }

    // Verificar que es la versión esperada antes de tocar nada
    uint8_t *call = (uint8_t *)REB(ADDR_CallIsaacUpdate);
    if (call[0] != 0xE8 || (uintptr_t)(call + 5) + *(int32_t *)(call + 1) != REB(ADDR_IsaacUpdate)) {
        LOG("unsupported game version (expected Repentance+ v1.9.7.17): mod disabled"); return;
    }
    for (size_t i = 0; i < sizeof FRAMETIME_SITES / sizeof *FRAMETIME_SITES; i++) {
        uint8_t *p = (uint8_t *)REB(FRAMETIME_SITES[i]);
        if (p[0] != 0xF2 || p[1] != 0x0F || p[2] != 0x10 || *(uint32_t *)(p + 4) != REB(ADDR_FrameTime_1_60)) {
            LOG("unsupported game version (site %lx): mod disabled", (unsigned long)FRAMETIME_SITES[i]);
            return;
        }
    }

    frame_time = 1.0 / (max_fps >= 60 ? max_fps : 1000);
    for (size_t i = 0; i < sizeof FRAMETIME_SITES / sizeof *FRAMETIME_SITES; i++) {
        uint32_t a = (uint32_t)(uintptr_t)&frame_time;
        write_mem(REB(FRAMETIME_SITES[i]) + 4, &a, 4);
    }
    int32_t rel = (int32_t)((uintptr_t)my_isaac_update - (uintptr_t)(call + 5));
    write_mem((uintptr_t)(call + 1), &rel, 4);
    LOG("active");
}

int proxy_init(void);

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    (void)h; (void)r;
    if (reason == DLL_PROCESS_ATTACH) {
        if (!proxy_init()) return FALSE;
        char exe[MAX_PATH];
        GetModuleFileNameA(NULL, exe, sizeof exe);
        if (strstr(exe, "isaac-ng.exe")) install();
    }
    return TRUE;
}
