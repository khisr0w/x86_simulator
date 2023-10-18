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
        switch(Operand.FieldType) {
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

inline internal void
PrintFlags()
{
    local_persist char *FlagIdxToStr[] = {
    #define ENUM(Enum) #Enum,
    #define POS(Value)
        FLAGS_POSITION_MAPPING
    #undef ENUM
    #undef POS
    };

    local_persist int32 FlagShiftPosition[] = {
    #define ENUM(Enum)
    #define POS(Value) Value,
        FLAGS_POSITION_MAPPING
    #undef ENUM
    #undef POS
    };

    printf("flags-> ");
    for(int32 Idx = 0; Idx < ArraySize(FlagShiftPosition); ++Idx)
    {
        if(GET_FLAG_VALUE(FlagShiftPosition[Idx])) printf("%s ", FlagIdxToStr[Idx]);
    }
}

/* NOTE(Abid): The portion related to resolving the source and destination is the same,
 *             therefore, it is separated out from the rest of the switch statement. */
internal void
SimulateNext(instruction *Inst) {
    PrintNext(Inst); printf(" ; ");

    /* NOTE(Abid): Source resolution */
    int32 Source = 0;
    boolean IsSrcByte = Inst->Operand2.IsBYTE;
    boolean IsSrcReg = false;
    switch(Inst->Operand2.FieldType) {
        case ft_reg: {
            IsSrcReg = true;
            int16 Idx = Inst->Operand2.Bytes16;
            Idx = RegIdxToRegMem[Idx];
            Source = IsSrcByte ? *(int8 *)(GLOBALRegisters + Idx) : *(int16 *)(GLOBALRegisters + Idx);
        } break;
        case ft_imme_sized:
        case ft_imme: {
            Source = IsSrcByte ? Inst->Operand2.Bytes8[0] : Inst->Operand2.Bytes16;
        } break;
        default : Assert(0, "invalid path")
    }

    /* NOTE(Abid): Destination resolution */
    /* NOTE(Abid): Each case is responsible for printing its pre-modified value */
    void *Dest = 0;
    boolean IsDestReg = false;
    boolean IsDestByte = false;
    int16 Idx = -1; 
    switch(Inst->Operand1.FieldType) {
        case ft_reg: {
            IsDestReg = true;
            Idx = Inst->Operand1.Bytes16;
            IsDestByte = Idx < WIDE_REGISTER_START_IDX; 

            Idx = RegIdxToRegMem[Idx];
            Dest = GLOBALRegisters + Idx;

            printf("%s:", GLOBALRegToStr[(Idx - Idx % 2)/2]);
            printf("->"); PrintRegister(Idx - Idx % 2, true);
        } break;
        case ft_imme_sized:
        case ft_imme: Assert(0, "cannot have immediate value as destination"); break;
        default : Assert(0, "invalid path");
    }

    /* NOTE(Abid): Op simulation */
    int32 InitDestValue = 0;
    int32 PostDestValue = 0;
    boolean IsImplicitFlagOp = false;
    boolean IsCarry = false;
    switch(Inst->Op) {
        case op_mov: {
            if(IsDestByte) {
                InitDestValue = *(uint8 *)(Dest);
                *(uint8 *)(Dest) = (uint8)Source; 
            } else {
                InitDestValue = *(uint16 *)(Dest);
                *(uint16 *)(Dest) = (int16)Source;
            }
        } break;
        case op_add: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(int8 *)(Dest);
                PostDestValue = InitDestValue + (int8)Source;
                *(uint8 *)(Dest) = (int8)PostDestValue;

                int32 ExpandedValue = ((int8)InitDestValue & 0xff) + ((int8)Source & 0xff);
                IsCarry = (ExpandedValue >> 8) > 0;
            }
            else {
                InitDestValue = *(int16 *)(Dest);
                PostDestValue = InitDestValue + (int16)Source;
                *(uint16 *)(Dest) = (int16)PostDestValue;

                int32 ExpandedValue = ((int16)InitDestValue & 0xffff) + ((int16)Source & 0xffff);
                IsCarry = (ExpandedValue >> 16) > 0;
            }
        } break;
        case op_sub: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(int8 *)(Dest);
                PostDestValue = InitDestValue - (int8)Source;
                *(uint8 *)(Dest) = (int8)PostDestValue;

                int32 ExpandedValue = ((int8)InitDestValue & 0xff) - ((int8)Source & 0xff);
                IsCarry = (((int8)InitDestValue & 0xff) < ((int8)Source & 0xff)) || (ExpandedValue >> 8) > 0;
            }
            else {
                InitDestValue = *(int16 *)(Dest);
                PostDestValue = InitDestValue - (int16)Source;
                *(uint16 *)(Dest) = (int16)PostDestValue;

                int32 ExpandedValue = ((int16)InitDestValue & 0xffff) - ((int16)Source & 0xffff);
                IsCarry = (((int16)InitDestValue & 0xffff) < ((int16)Source & 0xffff)) || (ExpandedValue >> 16) > 0;
            }
        } break;
        case op_cmp: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(int8 *)(Dest);
                PostDestValue = InitDestValue - (int8)Source;

                int32 ExpandedValue = ((int8)InitDestValue & 0xff) - ((int8)Source & 0xff);
                IsCarry = (((int8)InitDestValue & 0xff) < ((int8)Source & 0xff)) || (ExpandedValue >> 8) > 0;
            }
            else {
                InitDestValue = *(int16 *)(Dest);
                PostDestValue = InitDestValue - (int16)Source;

                int32 ExpandedValue = ((int16)InitDestValue & 0xffff) - ((int16)Source & 0xffff);
                IsCarry = (((int16)InitDestValue & 0xffff) < ((int16)Source & 0xffff)) || (ExpandedValue >> 16) > 0;
            }
        } break;
        default : Assert(0, "invalid path")
    }

    // NOTE(Abid): Post-modified print here
    if(IsDestReg) { printf("->"); PrintRegister(Idx - Idx % 2, true); }
    else Assert(0, "not implemented");

    /* NOTE(Abid): In case we have an op that changes the flags */
    if(IsImplicitFlagOp) {
        boolean IsZeroFlag = PostDestValue == 0;
        SET_FLAG_VALUE(flag_zf, IsZeroFlag); // Works

        boolean IsSignFlag = (PostDestValue >> 15) & 0b1;
        SET_FLAG_VALUE(flag_sf, IsSignFlag); // Works

        boolean IsOverFlow = false;
        if(IsDestByte) {
            int SourceSign = Inst->Op == op_add ? SIGN_OF_INT((int8)(Source), int8) :
                                                  !(SIGN_OF_INT((int8)(Source), int8));
            boolean IsOperandSignSame = SourceSign == SIGN_OF_INT((int8)(InitDestValue), int8);
            IsOverFlow = (IsOperandSignSame) && (SourceSign != SIGN_OF_INT(*(int8 *)(Dest), int8));
        }
        else {
            int SourceSign = Inst->Op == op_add ? SIGN_OF_INT((int16)(Source & 0xffff), int16) :
                                                  !(SIGN_OF_INT((int16)(Source & 0xffff), int16));
            boolean IsOperandSignSame = SourceSign == SIGN_OF_INT((int16)(InitDestValue & 0xffff), int16);
            IsOverFlow = (IsOperandSignSame) && (SourceSign != SIGN_OF_INT(*(int16 *)(Dest), int16));
        }
        SET_FLAG_VALUE(flag_of, IsOverFlow); // Works

        SET_FLAG_VALUE(flag_cf, IsCarry); // Works

        int32 ParityCount = 0;
        for(int32 I = 0; I < 8; ++I) ParityCount += (PostDestValue >> I) & 0b1;
        boolean IsParity = ParityCount % 2 == 0;
        SET_FLAG_VALUE(flag_pf, IsParity); // Works

        boolean IsAuxiliary = (Inst->Op == op_add && ((((Source & 0xf) + (InitDestValue & 0xf)) >> 4) & 0b1)) ||
                              ((Inst->Op == op_sub || Inst->Op == op_cmp)  && ((Source & 0xf) > (InitDestValue & 0xf)));
        SET_FLAG_VALUE(flag_af, IsAuxiliary); // Works
    }
    printf(" ; "); PrintFlags(); printf("\n"); 
}

/* NOTE(Abid): We are assuming 16-bit instructions for now */
int main(int argc, char* argv[]) {
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
    for(uint16 Idx = 0; Idx < ArraySize(GLOBALRegisters)/2; ++Idx) {
        printf("\t%s: ", GLOBALRegToStr[Idx]);
        /* NOTE(Abid): Print the whole register only */
        PrintRegister(2*Idx, true); printf(" ("); PrintRegister(2*Idx, false); printf(")\n");
    }

    return 0;
}
