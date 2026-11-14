#pragma once
// isaac240 para The Binding of Isaac: Repentance+ v1.9.7.17 (J460), isaac-ng.exe 32 bits.
// Todas las direcciones son con la base preferida 0x400000; se recolocan con REB().
#include <stdint.h>
#include <stddef.h>

extern uintptr_t g_delta;  // base real - 0x400000 (ASLR)
#define REB(a) ((uintptr_t)(a) + g_delta)

void logf_(const char *fmt, ...);
#define LOG(...) logf_(__VA_ARGS__)

// ---- Código ----
#define ADDR_CallIsaacUpdate   0x9312ecUL  // E8 rel32 -> IsaacUpdate, dentro de IsaacMain
#define ADDR_IsaacUpdate       0x9ab6d0UL  // void(void): EngineUpdate(false) + Manager::Update(true)
#define ADDR_Game_IsPaused     0x6fd350UL  // bool __thiscall(Game*)
static const uintptr_t FRAMETIME_SITES[] = {0x93129fUL, 0x93136eUL, 0x931445UL};  // movsd [1/60],xmm
#define ADDR_FrameTime_1_60    0xbaa498UL  // double 1/60 (compartida: no se modifica)

// ---- Datos globales ----
#define ADDR_g_Manager         0xc7169cUL  // Manager*
#define ADDR_g_Game            0xc71678UL  // Game*

// Manager
#define MGR_STATE              0x8         // int: 2 = en partida
#define MGR_FRAMECOUNT         0x4abbc     // int: frames 60Hz (par = tick lógico)
#define MGR_BLOCK_GAME         0x4b130     // byte: si !=0 no se actualiza la partida
#define MGR_INTERP_ENABLED     0x2a3c0     // byte
#define MGR_GATE_A             0x4b3ca     // condiciones copiadas de Game::InterpolatePositions
#define MGR_GATE_B             0x2a3a5
#define MGR_GATE_C1            0x4b3d8
#define MGR_GATE_C2            0x4b3dc

// Game
#define GAME_ROOM              0x18300     // Room*
#define GAME_TRANSITION_A      0x1b83c     // int: transición de sala activa
#define GAME_TRANSITION_B      0x1ba78     // int: otro tipo de transición

// Room
#define ROOM_ENTITIES          0x125c      // Entity**
#define ROOM_NUM_ENTITIES      0x1264      // uint32
#define ROOM_RENDER_SCROLL     0x1204      // Vector2: desplazamiento de cámara usado al renderizar
#define ROOM_INTERP_DONE       0x7220      // byte

// Entity
#define ENT_FLAGS              0x168       // uint64
#define ENT_INTERPOLATED       0x175       // byte
#define ENT_TYPE               0x28        // int: 1 = jugador
#define ENT_POS                0x33c       // Vector2
#define ENT_POS_BACKUP         0x344       // Vector2 (lo que restaura el revert)
#define ENT_HEIGHT             0x350       // float: offset vertical
#define VT_INTERPOLATE         4           // índice del slot virtual Interpolate()
