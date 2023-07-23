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

internal inline uint16
GetNext16Bit(instructions Inst)
{
    uint16 FirstByte = Inst.Bytes[Inst.CurByteIdx++];
    uint16 SecondByte = Inst.Bytes[Inst.CurByteIdx++];

    uint16 Result = (FirstByte << 8) | SecondByte;
    return Result;
}

static instructions
ReadBinaryIntoInstructions(char *FileName)
{
    instructions Inst = {0};

    FILE* File;
    if(fopen_s(&File, FileName, "r") != 0) return Inst;

    // NOTE(Abid): Get the size of the File
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

enum op_code
{
    op_regmem_reg_mov = 0b100010,
    op_immed_reg_mov = 0b1011,
    op_immed_regmem_mov = 0b1100011,
    op_mem_accum_mov = 0b1010000,
    op_accum_mem_mov = 0b1010001,
};

enum mod_flags
{
    mod_mem_no_dis = 0b00,
    mod_mem_8_dis = 0b01,
    mod_mem_16_dis = 0b10,
    mod_reg = 0b11,
};

// 1st Byte = 0b100010 0 1 (D: 0) (W: 1)
// 2nd Byte = 0b11 011 001 (Mod: 11) (REG: 011) (R/M: 001)

// NOTE(Abid): We are assuming 16-bit instructions for now
int main(int argc, char* argv[])
{
    // NOTE(Abid): Open file for reading
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

    // NOTE(Abid): Main decoding loop
    printf("bits 16\n\n");
    while(Inst.CurByteIdx != Inst.NumBytes)
    {
        uint8 FirstByte = Inst.Bytes[Inst.CurByteIdx++];

        if((FirstByte >> 2) == op_regmem_reg_mov) /* NOTE(Abid): Register/Memory or Register move */
        {
            printf("mov ");
            uint8 WFlag = FirstByte & 0b1;
            uint8 DFlag = (FirstByte & 0b10) == 0b10;

            uint8 SecondByte = Inst.Bytes[Inst.CurByteIdx++];
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
                            int16 Data = *(int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
                            printf("[%i]\n", Data);
                        }
                        else printf("[%s]\n", EffectiveAddCalc[RM]);
                    }
                    else
                    {
                        if(DirectAddress) 
                        {
                            int16 Data = *(int16 *)(Inst.Bytes + Inst.CurByteIdx++); Inst.CurByteIdx++;
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
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        Data < 0 ?  printf("- %i]\n", Data*-1) : printf("+ %i]\n", Data);
                    }
                    else
                    {
                        printf("[%s ", EffectiveAddCalc[RM]);
                        int8 Data = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        Data < 0 ? printf("- %i], ", Data*-1) :  printf("+ %i], ", Data);
                        printf("%s\n", Registers[WFlag][Reg]);
                    }
                } break;
                case mod_mem_16_dis:
                {
                    if(DFlag) 
                    {
                        printf("%s, [%s ", Registers[WFlag][Reg], EffectiveAddCalc[RM]);
                        uint8 LowByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(int8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        Data < 0 ?  printf("- %i]\n", Data*-1) : printf("+ %i]\n", Data);
                    }
                    else
                    {
                        printf("[%s ", EffectiveAddCalc[RM]);
                        uint8 LowByte = *(uint8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        uint16 HighByte = *(uint8 *)(Inst.Bytes + Inst.CurByteIdx++);
                        int16 Data = (HighByte << 8) | LowByte;
                        Data < 0 ? printf("- %i], ", Data*-1) :  printf("+ %i], ", Data);
                        printf("%s\n", Registers[WFlag][Reg]);
                    }
                } break;
                default: Assert(0, "Invalid Code Path");
            }
        }
        else if((FirstByte >> 4) == op_immed_reg_mov) /* NOTE(Abid): Immediate to register move*/
        {
            // NOTE(Abid): Immediate to register move
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
        else if((FirstByte >> 1) == op_immed_regmem_mov) /* NOTE(Abid): Immediate to register/memory move*/
        {
            // NOTE(Abid): Immediate to register move
            printf("mov ");

            uint8 SecondByte = Inst.Bytes[Inst.CurByteIdx++];
            uint8 Mod = SecondByte >> 6;
            uint8 WFlag = FirstByte & 0b1;
            uint8 RM = SecondByte & 0b111;

            switch(Mod)
            {
                case mod_reg:
                {
                    // NOTE(Abid): The reg version should have been done above in immediate to register mode
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
                        Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
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
                        Disp < 0 ? printf("- %i], ", Disp*-1) :  printf("+ %i], ", Disp);
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
        else if((FirstByte >> 1) == op_mem_accum_mov) /* NOTE(Abid): Memory to accumulator */
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
        else if((FirstByte >> 1) == op_accum_mem_mov) /* NOTE(Abid): Accumulator to memory */
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
        else Assert(0, "invalid opcode");
    }
    // printf("\nBinary file successfully decoded!");

    return 0;
}
