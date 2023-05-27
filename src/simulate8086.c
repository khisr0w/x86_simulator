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
typedef uint16_t uint16;
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

    Assert(BytesRead % 2 == 0, "number of bytes must be even, 16 bit ops only");
    Inst.Bytes = Content;
    Inst.NumBytes = BytesRead;
    Inst.Loaded = true;

    return Inst;
}

enum op_code
{
    op_reg_mem_mov = 0b100010
};

enum mod
{
    mod_mem_no_dis = 0b00,
    mod_mem_8_dis = 0b01,
    mod_mem_16_dis = 0b10,
    mod_reg = 0b11,
};

enum flags
{
    flag_w = 0b1,
    flag_d = (0b1 << 1),
};

// 1st Byte = 0b100010 0 1 (D: 0) (W: 1)
// 2nd Byte = 0b11 011 001 (Mod: 11) (REG: 011) (R/M: 001)

// NOTE(Abid): We are assuming 16-bit instructions for now
int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Please provide an asm file. Exiting...\n");
        return 0;
    }

    char *FileName = argv[1];
    instructions Inst = ReadBinaryIntoInstructions(FileName);
    Assert(Inst.Loaded, "could not open the binary file. Exiting...\n");

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

    FILE *File = NULL;
    if(fopen_s(&File, DecodedFileName, "w")) Assert(0, "could not create file");

    fprintf(File, "bits 16\n\n");
    while(Inst.CurByteIdx != Inst.NumBytes)
    {
        uint8 FirstByte = Inst.Bytes[Inst.CurByteIdx++];
        uint8 SecondByte = Inst.Bytes[Inst.CurByteIdx++];

        // NOTE(Abid): If opcode is `mov` in register/memory mode
        if(((FirstByte >> 2) & op_reg_mem_mov) == op_reg_mem_mov)
        {
            fprintf(File, "mov ");
            // NOTE(Abid): If We don't care about the MOD for now
            uint8 REGRMIdx[] = {((uint8)(SecondByte << 2) >> 5), ((uint8)(SecondByte << 5) >> 5)};
            uint8 DFlag = FirstByte & flag_d;

            fprintf(File, "%s, ", Registers[FirstByte & flag_w][REGRMIdx[1-DFlag]]);
            fprintf(File, "%s\n", Registers[FirstByte & flag_w][REGRMIdx[DFlag]]);
        }

    }
    printf("Binary file successfully decoded!");

    return 0;
}
