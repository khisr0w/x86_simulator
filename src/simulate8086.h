/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/16/2023 1:49:06 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#if !defined(SIMULATE8086_H)
#include "decode8086.h"

typedef struct {
    union {
        int16 ax;
        struct { int8 al; int8 ah; };
    };
    union {
        int16 bx;
        struct { int8 bl; int8 bh; };
    };
    union {
        int16 cx;
        struct { int8 cl; int8 ch; };
    };
    union {
        int16 dx;
        struct { int8 dl; int8 dh; };
    };
    int16 sp;
    int16 bp;
    int16 si;
    int16 di;
} registers;

uint8 GLOBALRegisters[] = {
    0, 0, // ax (al, ah)
    0, 0, // bx (bl, bh)
    0, 0, // cx (cl, ch)
    0, 0, // dx (dl, dh)
    0, 0, // sp
    0, 0, // bp
    0, 0, // si
    0, 0, // di
};

// TODO(Abid): Super janky last minute hack. MUST be removed
char *GLOBALRegToStr[] = {"ax", "bx", "cx", "dx", "sp", "bp", "si", "di"};

/* TODO(Abid): This thing is super janky. I'm being super lazy and just doing mapping 
 *             based on the already existent register macro. Its a good idea to come
 *             back to this and make an effecient mapping that doesn't require a
 *             mapping table after decoding.
 */
uint8 ToGLOBALRegIdx[] = {
#define X(Enum) [Enum] =
#define Y(Idx) Idx,
    X_REGISTERS_MAPPING
#undef Y
#undef X
};

#define SIMULATE8086_H
#endif
