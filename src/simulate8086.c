/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /optimization_practice                                        |
    |    Creation date:  Di 02 Mai 2023 15:10:01 CEST                                  |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#include "simulate8086.h"

internal u16
CalculateEffectiveAddress(field Operand, field Dist) {
    u16 Result = 0;

    u8 RM = Operand.Bytes8[0];
    u8 Mod = Operand.Bytes8[1];
    Assert((Mod & 0b11) != 0b11, "mod cannot be 0b11 when effective address calc.");

    registers *Register = (registers *)GLOBALRegisters;
    switch(RM) {
        case 0b000: Result = Register->BX + Register->SI; break;
        case 0b001: Result = Register->BX + Register->DI; break;
        case 0b010: Result = Register->BP + Register->SI; break;
        case 0b011: Result = Register->BP + Register->DI; break;
        case 0b100: Result = Register->SI; break;
        case 0b101: Result = Register->DI; break;
        case 0b110: { if(Mod != 0b00) Result = Register->BP; } break;
        case 0b111: Result = Register->BX; break;
    }

    if(Dist.FieldType != ft_invalid) {
        Assert(Dist.FieldType == ft_disp, "incorrect field type. Expected displacement");
        if(Dist.IsBYTE) Result += Dist.Bytes8[0];
        else Result += Dist.Bytes16;
    }

    return Result;
}

/* TODO(Abid): Refactor the JUMP instructions to remove the dumb control flow change below */
internal void
PrintNext(instruction *Inst) {
    field Operands[] = {Inst->Operand1, Inst->Operand2};
    printf("%s ", OpToStr[Inst->Op]);
    field Extended = Inst->Extended;

    for(u32 Idx = 0; Idx < ArraySize(Operands); ++Idx) {
        field Operand = Operands[Idx];
        bool IsPrintComma = Idx == 0;

        switch(Operand.FieldType) {
            case ft_reg: {
                printf("%s", RegIdxToRegStr[Operand.Bytes8[0]]);
            } break;
            case ft_seg_reg: {
                printf("%s", RegIdxToRegStr[Operand.Bytes8[0]]); 
            } break;
            case ft_mem:
            case ft_mem_sized: {
                i16 Addr = 0;
                // Operand.IsBYTE ? Operand.Bytes8[0] : Operand.Bytes16;
                if(Inst->DirectAddress) { /* Direct address */
                    Addr = Operand.Bytes16;
                    Operand.IsBYTE ? printf("byte [%d]", Addr)
                                   : printf("word [%d]", Addr);
                } else Assert(0, "Not Implemented");
                

            } break;
            case ft_effe: { 
                printf("[%s", EffectiveAddCalc[Operand.Bytes16]);
                if(Extended.FieldType == ft_disp) {
                    // NOTE(Abid): We have displacement!
                    i32 Disp = Extended.IsBYTE ? Extended.Bytes8[0] : Extended.Bytes16;
                    Disp < 0 ? printf(" - %i", Disp*-1) :  printf("+ %i", Disp);
                }
                printf("]");
            } break;
            case ft_effe_sized: { 
                /* TODO(Abid): Fix this print routine to reflect the changes made. */
                // Operand.IsBYTE ? printf("byte [%s", EffectiveAddCalc[Operand.Bytes16])
                //                : printf("word [%s", EffectiveAddCalc[Operand.Bytes16]);
                // if(Extended.FieldType == ft_disp) { // if we have memory displacement field!
                //     i16 Address = (i16)CalculateEffectiveAddress(Operand, Inst->Extended);
                //     // printf("%i", Disp*-1) :  printf(" + %i", Disp);
                //     Address < 0 ? printf(" - %i", Address*-1) :  printf(" + %i", Address);
                // }
                // printf("]");
            } break;
            case ft_imme: {
                i32 Immediate = Operand.IsBYTE ? Operand.Bytes8[0] : Operand.Bytes16;
                printf("%d", Immediate);
            } break;
            case ft_imme_sized: {
                Operand.IsBYTE ? printf("byte %d", Operand.Bytes8[0])
                               : printf("word %d", Operand.Bytes16);
            } break;
            case ft_jump: {
                /* NOTE(Abid): The jump value address is added with +2, since nasm considers the relative 
                 *             jump position from the start of the jump instruction (2 bytes), instead
                 *             of the end. So when assembly says -6, nasm will compute that to be -8.
                 *             In here, we add 2 in order to replicate the original assembly that was
                 *             given to nasm.
                 */
                i8 Data = Operand.Bytes8[0] + 2;
                (Data >= 0) ? printf("$+%d", Data) : printf("$-%d", -Data);
                Idx++; // WARNING(Abid): Changing the control flow here, this ain't nice.
            } break;
            case ft_disp: {
                Assert(0, "displacement in operands not allowed.");
            } break;
            case ft_empty: IsPrintComma = false; break;
            case ft_invalid:
            default : Assert(0, "invalid path.")
        }
        if(IsPrintComma) printf(", ");
    }
}

inline internal void
PrintRegister(u16 Idx, bool IsHex) {
    u16 Value = *(u16 *)(GLOBALRegisters + Idx);
    IsHex ? printf("0x%X", Value) : printf("%d", Value);
}

inline internal void
PrintFlags() {
    local_persist char *FlagIdxToStr[] = {
    #define ENUM(Enum) #Enum,
    #define POS(Value)
        FLAGS_POSITION_MAPPING
    #undef ENUM
    #undef POS
    };

    local_persist i32 FlagShiftPosition[] = {
    #define ENUM(Enum)
    #define POS(Value) Value,
        FLAGS_POSITION_MAPPING
    #undef ENUM
    #undef POS
    };

    printf("flags-> ");
    for(i32 Idx = 0; Idx < ArraySize(FlagShiftPosition); ++Idx) {
        if(GET_FLAG_VALUE(FlagShiftPosition[Idx])) printf("%s ", FlagIdxToStr[Idx]);
    }
}

#define GET_REGISTER(Name) ((registers *)GLOBALRegisters)->Name
#define SET_REGISTER(Name, Value) ((registers *)GLOBALRegisters)->Name = Value
/* NOTE(Abid): The portion related to resolving the source and destination is the same,
 *             therefore, it is separated out from the rest of the switch statement. */
/* TODO(Abid): Operations are either wide (16-bit) or not (8-bit), there is no need to keep operand-specific data
 *             regrading the size of the op */
internal void
SimulateNext(instruction *Inst) {
    PrintNext(Inst); printf(" ; ");

    /* NOTE(Abid): Source resolution */
    i32 Source = 0;
    bool IsSrcByte = Inst->Operand2.IsBYTE;
    switch(Inst->Operand2.FieldType) {
        case ft_seg_reg:
        case ft_reg: {
            i16 Idx = Inst->Operand2.Bytes8[0];
            Idx = RegIdxToRegMem[Idx];
            Source = IsSrcByte ? *(i8 *)(GLOBALRegisters + Idx) : *(i16 *)(GLOBALRegisters + Idx);
        } break;
        case ft_effe:
        case ft_effe_sized: {
            u16 Address = CalculateEffectiveAddress(Inst->Operand2, Inst->Extended);
            Source = IsSrcByte ? *(i8 *)(GLOBALMemory + Address)
                               : *(i16 *)(GLOBALMemory + Address);

        } break;
        case ft_imme_sized:
        case ft_imme: {
            Source = IsSrcByte ? Inst->Operand2.Bytes8[0] : Inst->Operand2.Bytes16;
        } break;
        case ft_jump: Source = Inst->Operand2.Bytes8[0]; break;
        case ft_empty: break;
        case ft_invalid:
        default : Assert(0, "invalid path")
    }

    /* NOTE(Abid): Destination resolution */
    /* NOTE(Abid): Each case is responsible for printing its pre-modified value */
    void *Dest = NULL;
    bool IsDestReg = false;
    bool IsDestByte = false;
    i16 Idx = -1; 
    switch(Inst->Operand1.FieldType) {
        case ft_seg_reg:
        case ft_reg: {
            IsDestReg = true;
            Idx = Inst->Operand1.Bytes8[0];
            IsDestByte = Idx < WIDE_REGISTER_START_IDX; 

            Idx = RegIdxToRegMem[Idx];
            Dest = GLOBALRegisters + Idx;

            printf("%s:", GLOBALRegToStr[(Idx - Idx % 2)/2]);
            printf("->"); PrintRegister(Idx - Idx % 2, true);
        } break;
        case ft_mem_sized:
        case ft_effe:
        case ft_effe_sized: {
            u16 Address = CalculateEffectiveAddress(Inst->Operand1, Inst->Extended);
            Dest = GLOBALMemory + Address;
        } break;
        case ft_imme_sized:
        case ft_imme: Assert(0, "cannot have immediate value as destination"); break;
        case ft_empty: break;
        case ft_jump:
        case ft_invalid:
        default : Assert(0, "invalid path");
    }

    /* NOTE(Abid): Op simulation */
    i32 InitDestValue = 0;
    i32 PostDestValue = 0;
    bool IsImplicitFlagOp = false;
    bool IsCarry = false;
    switch(Inst->Op) {
        case op_mov: {
            if(IsDestByte) {
                InitDestValue = *(u8 *)(Dest);
                *(u8 *)(Dest) = (u8)Source; 
            } else {
                InitDestValue = *(u16 *)(Dest);
                *(u16 *)(Dest) = (i16)Source;
            }
        } break;
        case op_add: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(i8 *)(Dest);
                PostDestValue = InitDestValue + (i8)Source;
                *(u8 *)(Dest) = (i8)PostDestValue;

                i32 ExpandedValue = ((i8)InitDestValue & 0xff) + ((i8)Source & 0xff);
                IsCarry = (ExpandedValue >> 8) > 0;
            } else {
                InitDestValue = *(i16 *)(Dest);
                PostDestValue = InitDestValue + (i16)Source;
                *(u16 *)(Dest) = (i16)PostDestValue;

                i32 ExpandedValue = ((i16)InitDestValue & 0xffff) + ((i16)Source & 0xffff);
                IsCarry = (ExpandedValue >> 16) > 0;
            }
        } break;
        case op_sub: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(i8 *)(Dest);
                PostDestValue = InitDestValue - (i8)Source;
                *(u8 *)(Dest) = (i8)PostDestValue;

                i32 ExpandedValue = ((i8)InitDestValue & 0xff) - ((i8)Source & 0xff);
                IsCarry = (((i8)InitDestValue & 0xff) < ((i8)Source & 0xff)) || (ExpandedValue >> 8) > 0;
            } else {
                InitDestValue = *(i16 *)(Dest);
                PostDestValue = InitDestValue - (i16)Source;
                *(u16 *)(Dest) = (i16)PostDestValue;

                i32 ExpandedValue = ((i16)InitDestValue & 0xffff) - ((i16)Source & 0xffff);
                IsCarry = (((i16)InitDestValue & 0xffff) < ((i16)Source & 0xffff)) || (ExpandedValue >> 16) > 0;
            }
        } break;
        case op_cmp: {
            IsImplicitFlagOp = true;
            if(IsDestByte) {
                InitDestValue = *(i8 *)(Dest);
                PostDestValue = InitDestValue - (i8)Source;

                i32 ExpandedValue = ((i8)InitDestValue & 0xff) - ((i8)Source & 0xff);
                IsCarry = (((i8)InitDestValue & 0xff) < ((i8)Source & 0xff)) || (ExpandedValue >> 8) > 0;
            } else {
                InitDestValue = *(i16 *)(Dest);
                PostDestValue = InitDestValue - (i16)Source;

                i32 ExpandedValue = ((i16)InitDestValue & 0xffff) - ((i16)Source & 0xffff);
                IsCarry = (((i16)InitDestValue & 0xffff) < ((i16)Source & 0xffff)) || (ExpandedValue >> 16) > 0;
            }
        } break;
        case op_jne: {
            if(!GET_FLAG_VALUE(flag_zf)) {
                i32 JumpLoc = Source;
                ((u16 *)GLOBALRegisters)[IP_REG_16_IDX] =
                    (u16)((i32)(((u16 *)GLOBALRegisters)[IP_REG_16_IDX]) + JumpLoc);
            }
        } break;
        case op_je: {
            if(GET_FLAG_VALUE(flag_zf)) {
                i32 JumpLoc = Source;
                ((u16 *)GLOBALRegisters)[IP_REG_16_IDX] =
                    (u16)((i32)(((u16 *)GLOBALRegisters)[IP_REG_16_IDX]) + JumpLoc);
            }
        } break;
        case op_jp: {
            if(GET_FLAG_VALUE(flag_pf)) {
                i32 JumpLoc = Source;
                ((u16 *)GLOBALRegisters)[IP_REG_16_IDX] =
                    (u16)((i32)(((u16 *)GLOBALRegisters)[IP_REG_16_IDX]) + JumpLoc);
            }
        } break;
        case op_jb: {
            if(GET_FLAG_VALUE(flag_cf)) {
                i32 JumpLoc = Source;
                ((u16 *)GLOBALRegisters)[IP_REG_16_IDX] =
                    (u16)((i32)(((u16 *)GLOBALRegisters)[IP_REG_16_IDX]) + JumpLoc);
            }
        } break;
        case op_loopnz: {
            /* NOTE(Abid): Decrement the cx and jump if cx != 0 && flag_zf == 0 */
            i16 CX = (i16)(*(u16 *)Dest);
            *(u16 *)Dest = (u16)(CX - 1);
            if((((u16 *)GLOBALRegisters)[2] != 0) && GET_FLAG_VALUE(flag_zf) == 0) {
                i32 JumpLoc = Source;
                ((u16 *)GLOBALRegisters)[IP_REG_16_IDX] =
                    (u16)((i32)(((u16 *)GLOBALRegisters)[IP_REG_16_IDX]) + JumpLoc);
            }
            IsDestReg = true;
        } break;
        default : Assert(0, "invalid path")
    }

    /* NOTE(Abid): In case we have an op that changes the flags */
    if(IsImplicitFlagOp) {
        bool IsZeroFlag = PostDestValue == 0;
        SET_FLAG_VALUE(flag_zf, IsZeroFlag);

        bool IsSignFlag = (PostDestValue >> 15) & 0b1;
        SET_FLAG_VALUE(flag_sf, IsSignFlag);

        bool IsOverFlow = false;
        if(IsDestByte) {
            i32 SourceSign = Inst->Op == op_add ? SIGN_OF_INT((i8)(Source), i8) :
                                                !(SIGN_OF_INT((i8)(Source), i8));
            bool IsOperandSignSame = SourceSign == SIGN_OF_INT((i8)(InitDestValue), i8);
            IsOverFlow = (IsOperandSignSame) && (SourceSign != SIGN_OF_INT(*(i8 *)(Dest), i8));
        }
        else {
            i32 SourceSign = Inst->Op == op_add ? SIGN_OF_INT((i16)(Source & 0xffff), i16) :
                                                !(SIGN_OF_INT((i16)(Source & 0xffff), i16));
            bool IsOperandSignSame = SourceSign == SIGN_OF_INT((i16)(InitDestValue & 0xffff), i16);
            IsOverFlow = (IsOperandSignSame) && (SourceSign != SIGN_OF_INT(*(i16 *)(Dest), i16));
        }
        SET_FLAG_VALUE(flag_of, IsOverFlow);
        SET_FLAG_VALUE(flag_cf, IsCarry);

        i32 ParityCount = 0;
        for(i32 I = 0; I < 8; ++I) ParityCount += (PostDestValue >> I) & 0b1;
        bool IsParity = ParityCount % 2 == 0;
        SET_FLAG_VALUE(flag_pf, IsParity);

        bool IsAuxiliary = (Inst->Op == op_add && ((((Source & 0xf) + (InitDestValue & 0xf)) >> 4) & 0b1)) ||
                              ((Inst->Op == op_sub || Inst->Op == op_cmp)  && ((Source & 0xf) > (InitDestValue & 0xf)));
        SET_FLAG_VALUE(flag_af, IsAuxiliary);
    }

    // NOTE(Abid): Post-modified print here
    if(Dest) {
        if(IsDestReg) { printf("->"); PrintRegister(Idx - Idx % 2, true); }
        // else Assert(0, "not implemented");
        printf(" ; "); 
    }

    /* NOTE(Abid): Print the ip register */
    printf("ip=0x%X", ((u16 *)GLOBALRegisters)[IP_REG_16_IDX]);

    printf(" ; "); PrintFlags(); printf("\n"); 
}
