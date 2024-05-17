/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/16/2023 1:49:06 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#define IP_REG_16_IDX 12
#define IP_REG_8_IDX 24

#if !defined(SIMULATE8086_H)
#include "decode8086.h"

#define WIDE_REGISTER_START_IDX 8

typedef struct {
    union {
        i16 AX;
        struct { i8 AL; i8 AH; };
    };
    union {
        i16 BX;
        struct { i8 BL; i8 BH; };
    };
    union {
        i16 CX;
        struct { i8 CL; i8 CH; };
    };
    union {
        i16 DX;
        struct { i8 DL; i8 DH; };
    };
    i16 SP;
    i16 BP;
    i16 SI;
    i16 DI;
    i16 CS;
    i16 DS;
    i16 SS;
    i16 ES;
    i16 IP;
    i16 FR;
} registers;

/*
 * Mnemonic        Condition tested  Description  
 * jo              OF = 1            overflow 
 * jno             OF = 0            not overflow 
 * jc, jb, jnae    CF = 1            carry / below / not above nor equal
 * jnc, jae, jnb   CF = 0            not carry / above or equal / not below
 * je, jz          ZF = 1            equal / zero
 * jne, jnz        ZF = 0            not equal / not zero
 * jbe, jna        CF or ZF = 1      below or equal / not above
 * ja, jnbe        CF or ZF = 0      above / not below or equal
 * js              SF = 1            sign 
 * jns             SF = 0            not sign 
 * jp, jpe         PF = 1            parity / parity even 
 * jnp, jpo        PF = 0            not parity / parity odd 
 * jl, jnge        SF xor OF = 1     less / not greater nor equal
 * jge, jnl        SF xor OF = 0     greater or equal / not less
 * jle, jng    (SF xor OF) or ZF = 1 less or equal / not greater
 * jg, jnle    (SF xor OF) or ZF = 0 greater / not less nor equal 
 *
 * */

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

u8 GLOBALRegisters[] = {
    0, 0, // ax (al, ah)
    0, 0, // bx (bl, bh)
    0, 0, // cx (cl, ch)
    0, 0, // dx (dl, dh)
    0, 0, // sp (stack pointer)
    0, 0, // bp (base pointer)
    0, 0, // si (source index)
    0, 0, // di (destination index)
    0, 0, // cs (code segment)
    0, 0, // ds (data segment)
    0, 0, // ss (stack segment)
    0, 0, // es (extra segment)
    0, 0, // ip (instruction pointer)
    0, 0, // fr (flags register)
};

u8 GLOBALMemory[1024*1024]; /* 1 MB */

#define SIGN_OF_INT(Value, Type) (((Value) >> (sizeof(Type)*8 - 1)) & 0b1)

#define GET_FLAG_VALUE(Enum) \
    ((*(i16 *)(GLOBALRegisters + 26) & (1 << (Enum))) == (1 << (Enum)))

#define SET_FLAG_VALUE(Enum, Bool)                                                         \
    do {                                                                                   \
        if(GET_FLAG_VALUE(Enum)) *(i16 *)(GLOBALRegisters + 26) ^= ((i16)(!Bool) << Enum); \
        else *(i16 *)(GLOBALRegisters + 26) ^= ((i16)Bool << Enum);                        \
    } while(0)

u8 RegIdxToRegMem[] = {
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
    "invalid op, normal to jump cuttoff value printed",
#define X(Enum) #Enum,
#define Y(Value)
    X_JUMP_INSTRUCTION
#undef X
#undef Y
};

char *JumpInstToStr[] = {
#define X(Enum) #Enum,
#define Y(Value)
    X_JUMP_INSTRUCTION
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

/* TODO(Abid): Super janky last minute hacks. MUST be removed */
char *GLOBALRegToStr[] = {"ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "cs", "ds", "ss", "es", "ip", "fr"};

char *EffectiveAddCalc[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};
usize GLOBALClockCount = 0;

#define SIMULATE8086_H
#endif
