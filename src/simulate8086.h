/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/16/2023 1:49:06 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#if !defined(SIMULATE8086_H)
#include "decode8086.h"

#define WIDE_REGISTER_START_IDX 8

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

/* NOTE(Abid): Below are all the flags in the register:
    NOTE(Abid): The following flags are set indirectly through arithmatic ops
    flag_cf = 1 << 0,  Carry Flag (Carry out of or borrow into the high order bit of result)
    flag_pf = 1 << 2,  Parity Flag (Whether the low 8-bit count of the result is even or odd)
    flag_af = 1 << 4,  Auxiliary Carry Flag (Carry from the low nibble to high nibble, or borrow)
    flag_zf = 1 << 6,  Zero Flag (1 when arithmatic result is zero, and 0 otherwise)
    flag_sf = 1 << 7,  Sign Flag (Whether result is negative = 1 or positive = 0)
    flag_of = 1 << 11, Overflow Flag (When most significant bit has been lost due to precision)

    NOTE(Abid): Below 3 flags are set directly 
    flag_tf = 1 << 8,  Trap Flag (Debugging mode, CPU creates interrupt for each instruction)
    flag_if = 1 << 9,  Interrupt-Enable Flag (Recognize external interrupt request)
    flag_df = 1 << 10, Direction Flag (0 = process string from left of right, 1 = vice versa)
*/
#define FLAGS_POSITION_MAPPING \
    ENUM(c) POS(0)             \
    ENUM(p) POS(2)             \
    ENUM(a) POS(4)             \
    ENUM(z) POS(6)             \
    ENUM(s) POS(7)             \
    ENUM(o) POS(11)            \
    ENUM(t) POS(8)             \
    ENUM(i) POS(9)             \
    ENUM(d) POS(10)            \

typedef enum {
#define ENUM(Enum) flag_##Enum##f =
#define POS(Value) Value,
    FLAGS_POSITION_MAPPING
#undef ENUM
#undef POS
} flags_idx;

uint8 GLOBALRegisters[] = {
    0, 0, // ax (al, ah)
    0, 0, // bx (bl, bh)
    0, 0, // cx (cl, ch)
    0, 0, // dx (dl, dh)
    0, 0, // sp
    0, 0, // bp
    0, 0, // si
    0, 0, // di
    0, 0, // cs (code segment)
    0, 0, // ds (data segment)
    0, 0, // ss (stack segment)
    0, 0, // es (extra segment)
    0, 0, // ip (instruction pointer)
    0, 0, // fr (flags register)
};

#define SIGN_OF_INT(Value, Type) (((Value) >> (sizeof(Type)*8 - 1)) & 0b1)

#define GET_FLAG_VALUE(Enum) \
    ((*(int16 *)(GLOBALRegisters + 26) & (1 << (Enum))) == (1 << (Enum)))

#define SET_FLAG_VALUE(Enum, Bool)                                                             \
    do {                                                                                       \
        if(GET_FLAG_VALUE(Enum)) *(int16 *)(GLOBALRegisters + 26) ^= ((int16)(!Bool) << Enum); \
        else *(int16 *)(GLOBALRegisters + 26) ^= ((int16)Bool << Enum);                        \
    } while(0)

uint8 RegIdxToRegMem[] = {
#define X(Enum) [Enum] =
#define Y(Idx) Idx,
    X_REGISTERS_MAPPING
#undef Y
#undef X
};

char *OpToStr[] = {
#define X(Value) #Value,
    X_OPS
#undef X
};

char *RETInstToStr[] = {
    "none",
#define X(Enum) #Enum,
#define Y(Value)
    X_RET_INSTRUCTION
#undef X
#undef Y
};

char *RegIdxToRegStr[] = {
#define X(Val) #Val,
#define Y(Value)
    X_REGISTERS_MAPPING
#undef Y
#undef X
};

// TODO(Abid): Super janky last minute hacks. MUST be removed
char *GLOBALRegToStr[] = {"ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "cs", "ds", "ss", "es", "ip", "fr"};
char *EffectiveAddCalc[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

#define SIMULATE8086_H
#endif
