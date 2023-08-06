/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /optimization_practice                                        |
    |    Creation date:  Di 02 Mai 2023 15:10:01 CEST                                  |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t uint32;
typedef int32_t int32;
typedef int64_t int64;
typedef uintptr_t uintptr;
typedef float float32;
typedef double float64;
typedef int8_t boolean;
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
#define internal static
#define local_persist static
#define global_var static
#define true 1
#define false 0

#define ArraySize(Arr) sizeof((Arr)) / sizeof((Arr)[0])

#define Assert(Expr, ErrorStr) if(!(Expr)) {fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\nExiting...\n", __FILE__, __LINE__); exit(-1);}

char *Registers[2][8] = 
{
    {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}
};

char *EffectiveAddCalc[] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

typedef struct instructions instructions;

struct instructions
{
    boolean Loaded;

    uint8 *Bytes;
    size_t NumBytes;

    uint8 CurByteIdx;
};

enum op_code
{
    /* NOTE(Abid): mov */
    op_regmem_reg_mov = 0b100010,
    op_immed_reg_mov = 0b1011,
    op_immed_regmem_mov = 0b1100011,
    op_mem_accum_mov = 0b1010000,
    op_accum_mem_mov = 0b1010001,

    /* NOTE(Abid): non-mov (mathy stuff) */
    op_immed_regmem = 0b100000,
    op_immed_accum =  0b0000010,
    op_regmem_reg = 0b0,
    op_add = 0b000,
    op_sub = 0b101,
    op_cmp = 0b111,

    /* NOTE(Abid): return from call (jumps) */
    op_je =     0b01110100,
    op_jl =     0b01111100,
    op_jle =    0b01111110,
    op_jb =     0b01110010,
    op_jbe =    0b01110110,
    op_jp =     0b01111010,
    op_jo =     0b01110000,
    op_js =     0b01111000,
    op_jne =    0b01110101,
    op_jnl =    0b01111101,
    op_jg =     0b01111111,
    op_jnb =    0b01110011,
    op_ja =     0b01110111,
    op_jnp =    0b01111011,
    op_jno =    0b01110001,
    op_jns =    0b01111001,
    op_loop =   0b11100010,
    op_loopz =  0b11100001,
    op_loopnz = 0b11100000,
    op_jcxz =   0b11100011,
};

enum mod_flags
{
    mod_mem_no_dis = 0b00,
    mod_mem_8_dis = 0b01,
    mod_mem_16_dis = 0b10,
    mod_reg = 0b11,
};

internal inline uint16
GetNext16Bit(instructions Inst)
{
    uint16 FirstByte = Inst.Bytes[Inst.CurByteIdx++];
    uint16 SecondByte = Inst.Bytes[Inst.CurByteIdx++];

    uint16 Result = (FirstByte << 8) | SecondByte;
    return Result;
}

internal inline instructions
ReadBinaryIntoInstructions(char *FileName)
{
    instructions Inst = {0};

    FILE* File;
    if(fopen_s(&File, FileName, "rb") != 0) return Inst;

    /* NOTE(Abid): Get the size of the File */
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    rewind(File);

    uint8* Content = (uint8 *)malloc(FileSize);
    if (Content == NULL)
    {
        fclose(File);
        return Inst;
    }

    size_t BytesRead = fread(Content, 1, FileSize, File);
    if (BytesRead != FileSize)
    {
        fclose(File);
        free(Content);
        return Inst;
    }
    fclose(File);

    Inst.Bytes = Content;
    Inst.NumBytes = BytesRead;
    Inst.Loaded = true;

    return Inst;
}

internal inline char *
IsRETInstruction(uint8 Op_Byte)
{
    uint8 RETInst[] =
    {
        op_je, op_jl, op_jle, op_jb, op_jbe, op_jp, op_jo, op_js,
        op_jne, op_jnl, op_jg, op_jnb, op_ja, op_jnp, op_jno,
        op_jns, op_loop, op_loopz, op_loopnz, op_jcxz,
    };

    char *RETInstStr[] =
    {
        "je", "jl", "jle", "jb", "jbe", "jp", "jo", "js", "jne",
        "jnl", "jg", "jnb", "ja", "jnp", "jno", "jns", "loop", 
        "loopz", "loopnz", "jcxz",
    };

    for(int Idx = 0; Idx < ArraySize(RETInst); ++Idx) if(RETInst[Idx] == Op_Byte) return RETInstStr[Idx];

    return 0;

}

internal void
_RegMem_Reg_Output(instructions *Inst, uint8 FirstByte)
{
    uint8 WFlag = FirstByte & 0b1;
    uint8 DFlag = (FirstByte & 0b10) == 0b10;

    uint8 SecondByte = Inst->Bytes[Inst->CurByteIdx++];
    uint8 Reg = ((uint8)(SecondByte << 2) >> 5);
    uint8 RM = SecondByte & 0b111;
    uint8 Mod = SecondByte >> 6;

    switch(Mod)
    {
        case mod_reg:
        {
            uint8 REGRMIdx[] = {Reg, RM};

            printf("%s, ", Registers[FirstByte & WFlag][REGRMIdx[1-DFlag]]);
            printf("%s\n", Registers[FirstByte & WFlag][REGRMIdx[DFlag]]);

        } break;
        case mod_mem_no_dis:
        {
            boolean DirectAddress = RM == 0b110;
            if(DFlag) 
            {
                printf("%s, ", Registers[WFlag][Reg]);
                if(DirectAddress) 
                {
                    int16 Data = *(int16 *)(Inst->Bytes + Inst->CurByteIdx++); Inst->CurByteIdx++;
                    printf("[%i]\n", Data);
                }
                else printf("[%s]\n", EffectiveAddCalc[RM]);
            }
            else
            {
                if(DirectAddress) 
                {
                    int16 Data = *(int16 *)(Inst->Bytes + Inst->CurByteIdx++); Inst->CurByteIdx++;
                    printf("[%i], ", Data);
                }
                else printf("[%s], ", EffectiveAddCalc[RM]);
                printf("%s\n", Registers[WFlag][Reg]);
            }
        } break;
        case mod_mem_8_dis:
        {
            if(DFlag) 
            {
                printf("%s, [%s ", Registers[WFlag][Reg], EffectiveAddCalc[RM]);
                int8 Data = *(int8 *)(Inst->Bytes + Inst->CurByteIdx++);
                Data < 0 ?  printf("- %i]\n", Data*-1) : printf("+ %i]\n", Data);
            }
            else
            {
                printf("[%s ", EffectiveAddCalc[RM]);
                int8 Data = *(int8 *)(Inst->Bytes + Inst->CurByteIdx++);
                Data < 0 ? printf("- %i], ", Data*-1) :  printf("+ %i], ", Data);
                printf("%s\n", Registers[WFlag][Reg]);
            }
        } break;
        case mod_mem_16_dis:
        {
            if(DFlag) 
            {
                printf("%s, [%s ", Registers[WFlag][Reg], EffectiveAddCalc[RM]);
                uint8 LowByte = *(int8 *)(Inst->Bytes + Inst->CurByteIdx++);
                uint16 HighByte = *(int8 *)(Inst->Bytes + Inst->CurByteIdx++);
                int16 Data = (HighByte << 8) | LowByte;
                Data < 0 ?  printf("- %i]\n", Data*-1) : printf("+ %i]\n", Data);
            }
            else
            {
                printf("[%s ", EffectiveAddCalc[RM]);
                uint8 LowByte = *(uint8 *)(Inst->Bytes + Inst->CurByteIdx++);
                uint16 HighByte = *(uint8 *)(Inst->Bytes + Inst->CurByteIdx++);
                int16 Data = (HighByte << 8) | LowByte;
                Data < 0 ? printf("- %i], ", Data*-1) :  printf("+ %i], ", Data);
                printf("%s\n", Registers[WFlag][Reg]);
            }
        } break;
        default: Assert(0, "Invalid Code Path");
    }
}

/* 1st Byte = 0b100010 0 1 (D: 0) (W: 1)
 * 2nd Byte = 0b11 011 001 (Mod: 11) (REG: 011) (R/M: 001)
 */

/* NOTE(Abid): We are assuming 16-bit instructions for now */
int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Please provide an asm file.");
        return 0;
    }

    char *FileName = argv[1];
    instructions Inst = ReadBinaryIntoInstructions(FileName);
    Assert(Inst.Loaded, "could not open the binary file.");

#if 0
    char *PostFix = "_decoded.asm";
    char DecodedFileName[256];
    uint32 StrIdx;
    for(StrIdx = 0; FileName[StrIdx]; ++StrIdx) DecodedFileName[StrIdx] = FileName[StrIdx];
    for(; *PostFix; ++StrIdx)
    {
        DecodedFileName[StrIdx] = *PostFix;
        ++PostFix;
    }
    DecodedFileName[StrIdx] = '\0';

    // NOTE(Abid): Open file for writing
    FILE *File = NULL;
    if(fopen_s(&File, DecodedFileName, "w")) Assert(0, "could not create file");
#endif

    /* NOTE(Abid): Main decoding loop */
    printf("bits 16\n\n");
    while(Inst.CurByteIdx != Inst.NumBytes)
    {
        uint8 FirstByte = Inst.Bytes[Inst.CurByteIdx++];

        if((FirstByte >> 2) == op_regmem_reg_mov) // Register/Memory or Register move
        {
            printf("mov ");
            _RegMem_Reg_Output(&Inst, FirstByte);
        }
        else if((FirstByte >> 4) == op_immed_reg_mov) // Immediate to register move
        {
            printf("mov ");
            uint8 WFlag = (FirstByte & 0b1000) == 0b1000;
            uint8 Reg = FirstByte & 0b111;
            printf("%s, ", Registers[WFlag][Reg]);

            if(WFlag)
            {
                int16 *Data = (int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                printf("%i\n", *Data);
            }
            else
            {
                int8 *Data = (int8 *)Inst.Bytes + Inst.CurByteIdx++;
                printf("%i\n", *Data);
            }
        }
        else if((FirstByte >> 1) == op_immed_regmem_mov) // Immediate to register/memory move
        {
            printf("mov ");

            uint8 SecondByte = Inst.Bytes[Inst.CurByteIdx++];
            uint8 Mod = SecondByte >> 6;
            uint8 WFlag = FirstByte & 0b1;
            uint8 RM = SecondByte & 0b111;

            switch(Mod)
            {
                case mod_reg:
                {
                    /* NOTE(Abid): The reg version should have been done above in immediate to register mode */
                    Assert(0, "Cannot have this mod here");
                } break;
                case mod_mem_8_dis:
                {
                    printf("[%s ", EffectiveAddCalc[RM]);
                    int8 Disp = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                    Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
                    if(WFlag)
                    {
                        uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("word %i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("byte %i\n", Data);
                    }

                } break;
                case mod_mem_16_dis:
                {
                    printf("[%s ", EffectiveAddCalc[RM]);

                    uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                    uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                    int16 Disp = (HighByte << 8) | LowByte;
                    Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
                    if(WFlag)
                    {
                        LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("word %i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("byte %i\n", Data);
                    }

                } break;
                case mod_mem_no_dis:
                {
                    printf("[%s], ", EffectiveAddCalc[RM]);
                    if(WFlag)
                    {
                        uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("word %i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("byte %i\n", Data);
                    }
                } break;
            }
        }
        else if((FirstByte >> 1) == op_mem_accum_mov) // Memory to accumulator
        {
            printf("mov ax, ");
            uint8 WFlag = FirstByte & 0b1;
            if(WFlag)
            {
                uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                int16 Addr = (HighByte << 8) | LowByte;
                printf("[%i]\n", Addr);
            }
            else
            {
                int8 Addr = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                printf("[%i]\n", Addr);
            }
        }
        else if((FirstByte >> 1) == op_accum_mem_mov) // Accumulator to memory
        {
            printf("mov ");
            uint8 WFlag = FirstByte & 0b1;
            if(WFlag)
            {
                uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                int16 Addr = (HighByte << 8) | LowByte;
                printf("[%i], ", Addr);
            }
            else
            {
                int8 Addr = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                printf("[%i], ", Addr);
            }
            printf("ax\n");
        }
        else if((FirstByte >> 2) == op_immed_regmem) // non-move immediate to reg/mem; will apply to add, sub, cmp and some other math ops
        {
            uint8 SecondByte = Inst.Bytes[Inst.CurByteIdx++];
            uint8 Op = ((uint8)(SecondByte << 2) >> 5);
            uint8 WFlag = (FirstByte & 0b1) == 0b1;
            uint8 SFlag = (FirstByte & 0b10) == 0b10;
            uint8 Mod = SecondByte >> 6;
            uint8 RM = SecondByte & 0b111;

            switch(Op) // Reg field encodes the op
            {
                case op_add: { printf("add "); } break;
                case op_sub: { printf("sub "); } break;
                case op_cmp: { printf("cmp "); } break;
            }

            switch(Mod)
            {
                case mod_reg:
                {
                    printf("%s, ", Registers[WFlag][RM]);
                    if(WFlag && !SFlag)
                    {
                        int16 *Data = (int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                        printf("%i\n", *Data);
                    }
                    else
                    {
                        int8 *Data = (int8 *)Inst.Bytes + Inst.CurByteIdx++;
                        printf("%i\n", *Data);
                    }
                } break;
                case mod_mem_8_dis:
                {
                    int8 Disp = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;

                    WFlag ? printf("word ") : printf("byte ");
                    printf("[%s ", EffectiveAddCalc[RM]);
                    Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
                    if(WFlag && !SFlag)
                    {
                        uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("%i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("%i\n", Data);
                    }
                } break;
                case mod_mem_16_dis:
                {
                    uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                    uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                    int16 Disp = (HighByte << 8) | LowByte;

                    WFlag ? printf("word ") : printf("byte ");
                    printf("[%s ", EffectiveAddCalc[RM]);
                    Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
                    if(WFlag && !SFlag)
                    {
                        LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("%i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("%i\n", Data);
                    }
                } break;

                case mod_mem_no_dis:
                {
                    WFlag ? printf("word ") : printf("byte ");

                    boolean DirectAddress = RM == 0b110;
                    if(DirectAddress)
                    {
                        int16 Data = *(int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                        printf("[%i], ", Data);
                    }
                    else printf("[%s], ", EffectiveAddCalc[RM]);
                    if(WFlag && !SFlag)
                    {
                        uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        printf("%i\n", Data);
                    }
                    else
                    {
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        printf("%i\n", Data);
                    }
                } break;
            }
        }
        else if((FirstByte >> 1) == (op_immed_accum | (op_add << 2)) || // non-mov immedidate to accumulator
                (FirstByte >> 1) == (op_immed_accum | (op_sub << 2)) ||
                (FirstByte >> 1) == (op_immed_accum | (op_cmp << 2)))
        {
            switch((FirstByte << 2) >> 5) // op codes
            {
                case op_add: { printf("add "); } break;
                case op_sub: { printf("sub "); } break;
                case op_cmp: { printf("cmp "); } break;
            }
            uint8 WFlag = (FirstByte & 0b1) == 0b1;
            if(WFlag)
            {
                int16 Data = *(int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                printf("ax, %d\n", Data);
            }
            else
            {
                int8 Data = *(int8*)(Inst.Bytes + Inst.CurByteIdx++);
                printf("al, %d\n", Data);
            }
        }
        else if((FirstByte >> 2) == (op_regmem_reg | (op_add << 1)) || // non-mov reg/mem to reg
                (FirstByte >> 2) == (op_regmem_reg | (op_sub << 1)) ||
                (FirstByte >> 2) == (op_regmem_reg | (op_cmp << 1)))
        {
            switch((FirstByte << 2) >> 5) // op codes
            {
                case op_add: { printf("add "); } break;
                case op_sub: { printf("sub "); } break;
                case op_cmp: { printf("cmp "); } break;
            }
            _RegMem_Reg_Output(&Inst, FirstByte);
        }
        else 
        {
            char *RETOp = IsRETInstruction(FirstByte);
            if(RETOp != 0)
            {
                /* NOTE(Abid): The jump value address added with 2, since nasm computes the relative 
                 *             jump position from the start of the jump instruction instead of the
                 *             end.
                 */
                int8 Data = *(int8*)(Inst.Bytes + Inst.CurByteIdx++);
                Data += 2;
                (Data >= 0) ? printf("%s $+%d\n", RETOp, Data) : printf("%s $-%d\n", RETOp, -Data);
            }
            else Assert(0, "invalid opcode");
        }
    }
    // printf("\nBinary file successfully decoded!");

    return 0;
}
