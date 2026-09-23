/**
 * Arcade Bitmaps and Icons (16x16 ARGB 0xRRGGBB)
 */

#ifndef ARCADE_BITMAPS_H
#define ARCADE_BITMAPS_H

#include <stdint.h>

#define _ 0x00000000 // Transparent
#define K 0xFF0A0A10 // Cabinet dark
#define G 0xFF3E3E50 // Cabinet gray
#define Y 0xFFFBBF24 // Amber/Gold
#define R 0xFFEF4444 // Red button
#define B 0xFF3B82F6 // Blue button
#define W 0xFFFFFFFF // White screen glare
#define C 0xFF06B6D4 // Cyan CRT screen
#define D 0xFF0E3A47 // Dark cyan CRT
#define J 0xFFF59E0B // Joystick ball

// 16x16 Classic Arcade Cabinet
static const uint32_t icon_cabinet[256] = {
    _,_,_,_,Y,Y,Y,Y,Y,Y,Y,Y,_,_,_,_,
    _,_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_,_,
    _,_,K,K,K,K,K,K,K,K,K,K,K,K,_,_,
    _,_,K,C,C,C,C,C,C,C,C,W,W,K,_,_,
    _,_,K,C,C,C,C,C,C,C,W,W,C,K,_,_,
    _,_,K,C,C,C,C,C,C,W,W,C,C,K,_,_,
    _,_,K,D,D,D,D,D,D,D,D,D,D,K,_,_,
    _,K,K,K,K,K,K,K,K,K,K,K,K,K,K,_,
    _,K,K,J,_,R,_,B,_,R,_,B,K,K,K,_,
    _,K,K,G,_,K,_,K,_,K,_,K,K,K,K,_,
    _,_,K,K,K,K,K,K,K,K,K,K,K,K,_,_,
    _,_,K,K,K,Y,Y,K,K,Y,Y,K,K,K,_,_,
    _,_,K,K,K,Y,Y,K,K,Y,Y,K,K,K,_,_,
    _,_,K,K,K,K,K,K,K,K,K,K,K,K,_,_,
    _,_,K,K,_,_,_,_,_,_,_,_,K,K,_,_,
    _,K,K,K,_,_,_,_,_,_,_,_,K,K,K,_
};

// 16x16 CRT Monitor with Scanlines
static const uint32_t icon_crt[256] = {
    _,_,G,G,G,G,G,G,G,G,G,G,G,G,_,_,
    _,G,G,G,G,G,G,G,G,G,G,G,G,G,G,_,
    G,G,K,K,K,K,K,K,K,K,K,K,K,K,G,G,
    G,G,K,C,C,C,C,C,C,C,C,W,W,K,G,G,
    G,G,K,D,D,D,D,D,D,D,D,D,D,K,G,G,
    G,G,K,C,C,C,C,C,C,C,C,C,C,K,G,G,
    G,G,K,D,D,D,D,D,D,D,D,D,D,K,G,G,
    G,G,K,C,C,C,C,C,C,C,C,C,C,K,G,G,
    G,G,K,D,D,D,D,D,D,D,D,D,D,K,G,G,
    G,G,K,C,C,C,C,C,C,C,C,C,C,K,G,G,
    G,G,K,K,K,K,K,K,K,K,K,K,K,K,G,G,
    _,G,G,G,G,G,G,G,G,G,G,G,G,G,G,_,
    _,_,_,_,_,G,G,G,G,G,G,_,_,_,_,_,
    _,_,_,_,G,G,G,G,G,G,G,G,_,_,_,_,
    _,_,_,G,G,G,G,G,G,G,G,G,G,_,_,_,
    _,_,_,G,G,G,G,G,G,G,G,G,G,_,_,_
};

#undef _
#undef K
#undef G
#undef Y
#undef R
#undef B
#undef W
#undef C
#undef D
#undef J

#endif
