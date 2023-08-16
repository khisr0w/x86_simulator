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
#define Assert(Expr, ErrorStr) if(!(Expr)) {fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\nExiting...\n", __FILE__, __LINE__); *(int *)0 = 0;}
#define WHOLE_REGISTER_BEGIN_IDX 8

#include "simulate8086.h"

#include "decode8086.c"

/* TODO(Abid): Refactor the RET instructions to remove the dumb control flow change below */
internal void
PrintNext(instruction *Inst)
{
    field Operands[] = {Inst->Operand1, Inst->Operand2};
    if(Operands[0].FieldType != ft_ret) printf("%s ", OpToStr[Inst->Op]);
    field Extended = Inst->Extended;

    for(uint32 Idx = 0; Idx < ArraySize(Operands); ++Idx) {
        field Operand = Operands[Idx];
        switch(Operand.FieldType)
        {
            case ft_reg: {
                printf("%s", RegIdxToRegStr[Operand.Bytes16]);
            } break;
            case ft_mem: {
                int16 Addr = Operand.IsBYTE ? Operand.Bytes8[0] : Operand.Bytes16;
                printf("[%d]", Addr);
            } break;
            case ft_mem_sized: {
                Operand.IsBYTE ? printf("byte [%d]", Operand.Bytes16)
                               : printf("word [%d]", Operand.Bytes16);
            } break;
            case ft_effe: { 
                printf("[%s", EffectiveAddCalc[Operand.Bytes16]);
                if(Extended.FieldType == ft_disp) {
                    // NOTE(Abid): We have displacement!
                    int32 Disp = Extended.IsBYTE ? Extended.Bytes8[0] : Extended.Bytes16;
                    Disp < 0 ? printf(" - %i", Disp*-1) :  printf("+ %i", Disp);
                }
                printf("]");
            } break;
            case ft_effe_sized: { 
                Operand.IsBYTE ? printf("byte [%s", EffectiveAddCalc[Operand.Bytes16])
                               : printf("word [%s", EffectiveAddCalc[Operand.Bytes16]);
                if(Extended.FieldType == ft_disp) { // if we have memory displacement field!
                    int32 Disp = Extended.IsBYTE ? Extended.Bytes8[0] : Extended.Bytes16;
                    Disp < 0 ? printf(" - %i", Disp*-1) :  printf(" + %i", Disp);
                }
                printf("]");
            } break;
            case ft_imme: {
                int32 Immediate = Operand.IsBYTE ? Operand.Bytes8[0] : Operand.Bytes16;
                printf("%d", Immediate);
            } break;
            case ft_imme_sized: {
                Operand.IsBYTE ? printf("byte %d", Operand.Bytes8[0])
                               : printf("word %d", Operand.Bytes16);
            } break;
            case ft_disp: {
                Assert(0, "displacement in operands not allowed.");
            } break;
            case ft_ret: {
                int16 Data = Operands[1].Bytes16;
                (Data >= 0) ? printf("%s $+%d", RETInstToStr[Operand.Bytes16], Data)
                            : printf("%s $-%d", RETInstToStr[Operand.Bytes16], -Data);
                Idx++; // WARNING(Abid): Changing the control flow here, this ain't nice.
            } break;
            case ft_none: { Assert(0, "invalid operand.") } break;
            default : Assert(0, "invalid path.")
        }
        if(Idx == 0) printf(", ");
    }
}

inline internal void
PrintRegister(uint16 Idx, boolean IsHex) {
    uint16 Value = *(uint16 *)(GLOBALRegisters + Idx);
    IsHex ? printf("0x%X", Value) : printf("%d", Value);
}

internal void
SimulateNext(instruction *Inst)
{
    PrintNext(Inst); printf(" ; ");
    switch(Inst->Op) {
        case op_mov: {
            // NOTE(Abid): Source value checking
            int16 Source = 0;
            switch(Inst->Operand2.FieldType) {
                case ft_reg: {
                    int16 Idx = Inst->Operand2.Bytes16;
                    Idx = ToGLOBALRegIdx[Idx];
                    Source = Inst->Operand2.IsBYTE ? *(int8 *)(GLOBALRegisters + Idx)
                                                   : *(int16 *)(GLOBALRegisters + Idx);
                } break;
                case ft_imme_sized:
                case ft_imme: {
                    Source = Inst->Operand2.IsBYTE ? Inst->Operand2.Bytes8[0]
                                                   : Inst->Operand2.Bytes16;
                } break;
                default : Assert(0, "invalid path")
            }

            // NOTE(Abid): Dest checking
            switch(Inst->Operand1.FieldType) {
                case ft_reg: {
                    int16 Idx = Inst->Operand1.Bytes16;
                    boolean IsWholeRegister = Idx >= WHOLE_REGISTER_BEGIN_IDX;
                    Idx = ToGLOBALRegIdx[Idx];
                    printf("%s:", GLOBALRegToStr[(Idx - Idx % 2)/2]); // Print the whole register only
                    PrintRegister(Idx - Idx % 2, true);               // Print the whole register only
                    if(IsWholeRegister) *(uint16 *)(GLOBALRegisters + Idx) = Source;
                    else *(uint8 *)(GLOBALRegisters + Idx) = (uint8)Source; 
                    printf("->");
                    PrintRegister(Idx - Idx % 2, true);
                    printf("\n");
                } break;
                case ft_imme_sized:
                case ft_imme: Assert(0, "cannot have immediate value as destination"); break;
                default : Assert(0, "invalid path");
            }

            if(Inst->Operand1.FieldType == ft_reg) { // Dest is a register
            }
        } break;
        default : Assert(0, "invalid path")
    }
}

/* NOTE(Abid): We are assuming 16-bit instructions for now */
int main(int argc, char* argv[])
{
    if(argc < 2) {
        printf("Please provide an asm file.");
        return 0;
    }

    char *FileName = argv[1];
    byte_stream ByteStream = ReadBinaryFileIntoStream(FileName);
    Assert(ByteStream.Loaded, "could not open the binary file.");

    /* NOTE(Abid): Instruction stream is 1 Megabytes */
    // instruction_stream InstStream = {
    //     .Base = (instruction *)malloc(1024 * 1024 * sizeof(uint8)),
    //     .CurrentIdx = 0,
    //     .NumInstructionsDecoded = 0
    // };

    while(ByteStream.CurByteIdx != ByteStream.NumBytes) {
        instruction Inst = DecodeNext(&ByteStream);
        SimulateNext(&Inst);
    }

    printf("\nFinal registers:\n");
    for(uint16 Idx = 0; Idx < ArraySize(GLOBALRegisters)/2; ++Idx)
    {
        printf("\t%s: ", GLOBALRegToStr[Idx]);
        /* NOTE(Abid): Print the whole register only */
        PrintRegister(2*Idx, true); printf(" ("); PrintRegister(2*Idx, false); printf(")\n");
    }

    return 0;
}
