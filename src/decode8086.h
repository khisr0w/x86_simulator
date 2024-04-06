/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/12/2023 3:45:38 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#if !defined(DECODE8086_H)

/* NOTE(Abid): 8 registers with 4 having 8-bit low and high values
 * An overview: https://www.includehelp.com/embedded-system/jump-instructions-in-8086-microprocessor.aspx
 * */

#define X_JUMP_INSTRUCTION \
    X(je)     Y(0b01110100) \
    X(jl)     Y(0b01111100) \
    X(jle)    Y(0b01111110) \
    X(jb)     Y(0b01110010) \
    X(jbe)    Y(0b01110110) \
    X(jp)     Y(0b01111010) /* jpe as well */ \
    X(jo)     Y(0b01110000) \
    X(js)     Y(0b01111000) \
    X(jne)    Y(0b01110101) /* jnz as well */ \
    X(jnl)    Y(0b01111101) \
    X(jg)     Y(0b01111111) \
    X(jnb)    Y(0b01110011) \
    X(ja)     Y(0b01110111) \
    X(jnp)    Y(0b01111011) /* npo as well */ \
    X(jno)    Y(0b01110001) \
    X(jns)    Y(0b01111001) \
    X(loop)   Y(0b11100010) \
    X(loopz)  Y(0b11100001) \
    X(loopnz) Y(0b11100000) \
    X(jcxz)   Y(0b11100011)

/* WARNING(Abid): Do not change the order of the following macro, neither input an entry
 *                interspersed in the middle of it. */
#define X_REGISTERS_MAPPING \
    X(al) Y(0)  \
    X(cl) Y(4)  \
    X(dl) Y(6)  \
    X(bl) Y(2)  \
    X(ah) Y(1)  \
    X(ch) Y(5)  \
    X(dh) Y(7)  \
    X(bh) Y(3)  \
    X(ax) Y(0)  \
    X(cx) Y(4)  \
    X(dx) Y(6)  \
    X(bx) Y(2)  \
    X(sp) Y(8)  \
    X(bp) Y(10) \
    X(si) Y(12) \
    X(di) Y(14) \
    X(es) Y(22) \
    X(cs) Y(16) \
    X(ss) Y(20) \
    X(ds) Y(18)

typedef enum {
#define X(Enum) Enum,
#define Y(Value)
    X_REGISTERS_MAPPING
#undef Y
#undef X
} register_idx;

typedef enum {
    /* NOTE(Abid): mov */
    opcode_regmem_reg_mov = 0b100010,

    opcode_regmem_seg_mov = 0b10001110,
    opcode_seg_regmem_mov = 0b10001100,

    opcode_immed_reg_mov = 0b1011,
    opcode_immed_regmem_mov = 0b1100011,
    opcode_mem_accum_mov = 0b1010000,
    opcode_accum_mem_mov = 0b1010001,

    /* NOTE(Abid): non-mov (mathy stuff) */
    opcode_immed_regmem = 0b100000,
    opcode_immed_accum =  0b0000010,
    opcode_regmem_reg = 0b0,
    opcode_add = 0b000,
    opcode_sub = 0b101,
    opcode_cmp = 0b111,

    /* NOTE(Abid): jumps */
#define X(Enum) opcode_##Enum =
#define Y(Value) Value,
    X_JUMP_INSTRUCTION
#undef X

#undef Y
} op_code;


#define X_OPS \
    X(none)   \
    X(mov)    \
    X(add)    \
    X(sub)    \
    X(cmp)    \

typedef enum {
#define X(Enum) op_##Enum,
    X_OPS
#undef X
    _op_normal_to_jump_cutoff, /* WARNING(Abid): This value must not be moved. */
#define X(Enum) op_##Enum,
#define Y(Value)
    X_JUMP_INSTRUCTION
#undef X
#undef Y
} op;

typedef enum {
    mod_mem_no_dis = 0b00,
    mod_mem_8_dis = 0b01,
    mod_mem_16_dis = 0b10,
    mod_reg = 0b11,
} mod_flags;

typedef enum {
    ft_invalid,     // Invalid
    ft_empty,       // Empty (When not needed)
    ft_reg,         // Register
    ft_seg_reg,     // Segment Register
    ft_mem,         // Memory
    ft_mem_sized,   // Memory (byte/word)
    ft_effe,        // Effective Address
    ft_effe_sized,  // Effective Address (byte/word)
    ft_imme,        // Immediate
    ft_imme_sized,  // Immediate (byte/word delineation)
    ft_disp,        // Displacement
    ft_jump,        // Jumps Index
} field_type;

typedef struct {
    union {
        i8 Bytes8[2];
        i16 Bytes16; // For the most part, this will index into registers, in which case idx 0 is dest.
    };
    bool IsBYTE; // true if the 1st operand is used as 8 bytes
    field_type FieldType;
} field;

typedef struct {
    op Op;
    mod_flags Mod;
    bool DirectAddress;
    bool IsOpWide; /* TODO(Abid): Use this instead of operand-specific ones. */
    field Operand1;
    field Operand2;
    field Extended;
} instruction;

typedef struct {
    instruction *Base;
    size_t CurrentIdx;
    size_t NumInstructionsDecoded;
} instruction_stream;

typedef struct {
    bool Loaded;

    u8 *Bytes;
    size_t NumBytes;
} byte_stream;

#define DECODE8086_H
#endif
