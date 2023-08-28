/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/12/2023 3:45:38 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#if !defined(DECODE8086_H)

/* NOTE(Abid): 8 registers with 4 having 8-bit low and high values */
#define X_RET_INSTRUCTION \
    X(je)     Y(0b01110100) \
    X(jl)     Y(0b01111100) \
    X(jle)    Y(0b01111110) \
    X(jb)     Y(0b01110010) \
    X(jbe)    Y(0b01110110) \
    X(jp)     Y(0b01111010) \
    X(jo)     Y(0b01110000) \
    X(js)     Y(0b01111000) \
    X(jne)    Y(0b01110101) \
    X(jnl)    Y(0b01111101) \
    X(jg)     Y(0b01111111) \
    X(jnb)    Y(0b01110011) \
    X(ja)     Y(0b01110111) \
    X(jnp)    Y(0b01111011) \
    X(jno)    Y(0b01110001) \
    X(jns)    Y(0b01111001) \
    X(loop)   Y(0b11100010) \
    X(loopz)  Y(0b11100001) \
    X(loopnz) Y(0b11100000) \
    X(jcxz)   Y(0b11100011)

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
    X(di) Y(14) 

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

    /* NOTE(Abid): return from call (jumps) */
#define X(Enum) op_##Enum =
#define Y(Value) Value,
    X_RET_INSTRUCTION
#undef X
#undef Y
} op_code;


#define X_OPS \
    X(none) \
    X(mov)  \
    X(add)  \
    X(sub)  \
    X(cmp)  \
    X(ret)

typedef enum {
#define X(Enum) op_##Enum,
    X_OPS
#undef X
} op;

typedef enum {
    mod_mem_no_dis = 0b00,
    mod_mem_8_dis = 0b01,
    mod_mem_16_dis = 0b10,
    mod_reg = 0b11,
} mod_flags;

typedef enum {
    ft_none,
    ft_reg,         // Register
    ft_mem,         // Memory
    ft_mem_sized,   // Memory (byte/word)
    ft_effe,        // Effective Address
    ft_effe_sized,  // Effective Address (byte/word)
    ft_imme,        // Immediate
    ft_imme_sized,  // Immediate (byte/word delianation)
    ft_disp,        // Displacement
    ft_ret,         // RET Index
} field_type;

typedef struct {
    union {
        int8 Bytes8[2];
        int16 Bytes16; // For the most part, this will index into registers, in which case idx 0 is dest.
    };
    boolean IsBYTE; // true if the 1st operand is used as 8 bytes
    field_type FieldType;
} field;

typedef struct {
    op Op;
    mod_flags Mod;
    boolean DirectAddress;
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
    boolean Loaded;

    uint8 *Bytes;
    size_t NumBytes;

    uint8 CurByteIdx;
} byte_stream;

#define DECODE8086_H
#endif
