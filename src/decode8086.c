/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/12/2023 3:31:40 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#include "decode8086.h"

internal inline bool
IsJumpInstruction(u8 OpByte, i32 *JumpIdx) {
    local_persist u8 JumpInst[] = {
    #define X(Enum)
    #define Y(Value) Value,
        X_JUMP_INSTRUCTION
    #undef Y
    #undef X
    };

    for(i32 Idx = 0; Idx < ArraySize(JumpInst); ++Idx) {
        if(JumpInst[Idx] == OpByte) {
            *JumpIdx = Idx + _op_normal_to_jump_cutoff + 1;
            return true;
        }
    }
    return false;
}

internal void
ImmedRegMemToRegMem(byte_stream *ByteStream, u16 *InstPointer, instruction *Inst, u8 FirstByte,
                    bool IsSegmentReg, bool IsImmediate) {

    u8 WFlag = FirstByte & 0b1;
    u8 DFlag = (FirstByte & 0b10) == 0b10;
    u8 SecondByte = ByteStream->Bytes[(*InstPointer)++];
    u8 Mod = SecondByte >> 6;
    u8 Reg = ((u8)(SecondByte << 2) >> 5);
    u8 RM = SecondByte & 0b111;

    if(IsSegmentReg) WFlag = 0b1; /* In segment register, we always operate on WORD length. */
    if(IsImmediate) DFlag = false; /* In immediate, we always make the RM the destination */

    u8 RFlag = DFlag ^ 0b1; /* NOTE(Abid): Index of where register resides */

    i16 RegisterLocOffset = 8*WFlag;

    Inst->Mod = Mod;
    Inst->IsOpWide = WFlag;

    field *Operands[] = {&Inst->Operand1, &Inst->Operand2};
    Operands[RFlag]->Bytes16 = RegisterLocOffset + Reg;
    Operands[RFlag]->FieldType = ft_reg;

    switch(Inst->Mod) {
        case mod_reg: {
            /* NOTE(Abid): In case of immediate to memory, this case wouldn't hit. */
            Operands[1-RFlag]->Bytes8[0] = (i8)(RegisterLocOffset + RM);
            Operands[1-RFlag]->FieldType = ft_reg;
        } break;
        case mod_mem_8_dis: {
            Operands[1-RFlag]->Bytes8[0] = RM;
            Operands[1-RFlag]->Bytes8[1] = Mod;
            Operands[1-RFlag]->FieldType = ft_effe_sized;

            i8 Disp = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Inst->Extended.Bytes8[0] = Disp;
            Inst->Extended.IsBYTE = true;
            Inst->Extended.FieldType = ft_disp;
        } break;
        case mod_mem_no_dis: {
            Operands[1-RFlag]->Bytes8[0] = RM;
            Operands[1-RFlag]->Bytes8[1] = Mod;
            Operands[1-RFlag]->FieldType = ft_effe_sized;

            if((RM == 0b110) && (Mod != 0b11)) { /* Direct address */
                u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                i16 Disp = (HighByte << 8) | LowByte;
                Inst->Extended.Bytes16 = Disp;
                Inst->Extended.FieldType = ft_disp;
                Inst->Extended.IsBYTE = false;
            }
        } break;
        case mod_mem_16_dis: {
            Operands[1-RFlag]->Bytes8[0] = RM;
            Operands[1-RFlag]->Bytes8[1] = Mod;
            Operands[1-RFlag]->FieldType = ft_effe_sized;

            u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            i16 Disp = (HighByte << 8) | LowByte;
            Inst->Extended.Bytes16 = Disp;
            Inst->Extended.FieldType = ft_disp;
            Inst->Extended.IsBYTE = false;
        } break;
        default: Assert(0, "invalid code path");
    }

    if(IsImmediate) {
        if(WFlag) {
            u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Operands[1]->Bytes16 = (HighByte << 8) | LowByte;
            Operands[1]->IsBYTE = false;
        } else {
            Operands[1]->Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Operands[1]->IsBYTE = true;
        }
        Operands[1]->FieldType = ft_imme;
    }
}

/* TODO(Abid): Start using this routine, instead of `ImmedRegMemToRegMem` and leave the operand
 *             guessing to each condition statement. */
internal void
DecodeOperandMod(byte_stream *ByteStream, u16 *InstPointer, instruction *Inst, i32 OperandIdx,
                 mod_flags Mod, u8 RM, u8 WFlag) {
    Assert(OperandIdx < 3, "invalid operand idx.");

    Inst->Mod = Mod;
    field *Operands[] = {&Inst->Operand1, &Inst->Operand2};

    i16 RegisterLocOffset = 8*WFlag;

    switch(Inst->Mod) {
        case mod_reg: {
            /* NOTE(Abid): In case of immediate to memory, this case wouldn't hit. */
            Operands[OperandIdx]->Bytes8[0] = (i8)(RegisterLocOffset + RM);
            Operands[OperandIdx]->FieldType = ft_reg;
        } break;
        case mod_mem_8_dis: {
            i8 Disp = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Inst->Extended.Bytes8[0] = Disp;
            Inst->Extended.IsBYTE = true;
            Inst->Extended.FieldType = ft_disp;
        } break;
        case mod_mem_no_dis: {
            Operands[OperandIdx]->Bytes8[0] = RM;
            Operands[OperandIdx]->Bytes8[1] = Mod;
            Operands[OperandIdx]->Bytes16 = ft_effe_sized;

            if((RM == 0b110) && (Mod != 0b11)) { /* Direct address */
                u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                i16 Disp = (HighByte << 8) | LowByte;
                Inst->Extended.Bytes16 = Disp;
                Inst->Extended.FieldType = ft_disp;
                Inst->Extended.IsBYTE = false;
            }
        } break;
        case mod_mem_16_dis: {
            Operands[OperandIdx]->Bytes8[0] = RM;
            Operands[OperandIdx]->Bytes8[1] = Mod;
            Operands[OperandIdx]->Bytes8[8] = ft_effe_sized;

            u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            i16 Disp = (HighByte << 8) | LowByte;
            Inst->Extended.Bytes16 = Disp;
            Inst->Extended.FieldType = ft_disp;
            Inst->Extended.IsBYTE = false;
        } break;
        default: Assert(0, "invalid code path");
    }
}


/* NOTE(Abid): Here's how it works for now:
 *             The decoder, decodes the binary and transforms it into the intermediate representation
 *             that we have here. As the decoder is adding more instructions, the simulator starts
 *             executing the instruction and forwarding the pointer to the next instruction that
 *             should be executed. As of now, the program will take turns in decoding and simulating
 *             the code. For later, there could be 2 threads, one for simulation and another for
 *             decoding.
 *             Initially, we decode one instruction and simulate it immediately.
 */

internal instruction 
DecodeNext(byte_stream *ByteStream, u16 *InstPointer) {
    u8 FirstByte = ByteStream->Bytes[(*InstPointer)++];
    instruction Instruction = {0};

    if((FirstByte >> 2) == opcode_regmem_reg_mov) { // Register/Memory to/from Register move
        Instruction.Op = op_mov;
        ImmedRegMemToRegMem(ByteStream, InstPointer, &Instruction, FirstByte, false, false);

    } else if(FirstByte == opcode_regmem_seg_mov) { // Register/Memory to Segment Register move
        Instruction.Op = op_mov;
        ImmedRegMemToRegMem(ByteStream, InstPointer, &Instruction, FirstByte, true, false);

        /* TODO(Abid): This is really ugly and should be done inside the function itself. */
        Instruction.Operand1.FieldType = ft_seg_reg;
        Instruction.Operand1.Bytes16 += 8; /* 8 + 8 = 16 (First 8 from the function) */
    } else if(FirstByte == opcode_seg_regmem_mov) { // Segment to Register/Memory move
        Instruction.Op = op_mov;
        ImmedRegMemToRegMem(ByteStream, InstPointer, &Instruction, FirstByte, true, false);

        /* TODO(Abid): This is really ugly and should be done inside the function itself. */
        Instruction.Operand2.FieldType = ft_seg_reg;
        Instruction.Operand2.Bytes16 += 8;
    } else if((FirstByte >> 4) == opcode_immed_reg_mov) { // Immediate to register move
        Instruction.Op = op_mov;

        u8 WFlag = (FirstByte & 0b1000) == 0b1000;
        u8 Reg = FirstByte & 0b111;
        Instruction.Operand1.Bytes16 = 8*WFlag + Reg;
        Instruction.Operand1.FieldType = ft_reg;
        if(WFlag) {
            Instruction.Operand2.Bytes16 = *(i16 *)(ByteStream->Bytes + (*InstPointer)++); (*InstPointer)++;
        } else {
            Instruction.Operand2.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_imme;
    } else if((FirstByte >> 1) == opcode_immed_regmem_mov) { // Immediate to **memory**/register move
        Instruction.Op = op_mov;
        ImmedRegMemToRegMem(ByteStream, InstPointer, &Instruction, FirstByte, false, true);
    } else if((FirstByte >> 1) == opcode_mem_accum_mov) { // Memory to accumulator move
        Instruction.Op = op_mov;
        u8 WFlag = FirstByte & 0b1;

        Instruction.Operand1.Bytes16 = WFlag ? ax : al;
        Instruction.Operand1.FieldType = ft_reg;
        if(WFlag) {
            u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte; // Address to move from
        } else {
            Instruction.Operand2.Bytes16 = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_mem;
    } else if((FirstByte >> 1) == opcode_accum_mem_mov) { // Accumulator to memory move
        Instruction.Op = op_mov;
        u8 WFlag = FirstByte & 0b1;

        Instruction.Operand2.Bytes16 = WFlag ? ax : al;
        Instruction.Operand2.FieldType = ft_reg;
        if(WFlag) {
            u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand1.Bytes16 = (HighByte << 8) | LowByte;
        } else {
            Instruction.Operand1.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand1.IsBYTE = true;
        }
        Instruction.Operand1.FieldType = ft_mem;
    } else if((FirstByte >> 2) == opcode_immed_regmem) {
        // NOTE(Abid): non-move immediate to reg/mem; will apply to add, sub, cmp and some other math ops
        u8 SecondByte = ByteStream->Bytes[(*InstPointer)++];
        u8 Op = ((u8)(SecondByte << 2) >> 5);
        u8 WFlag = (FirstByte & 0b1) == 0b1;
        u8 SFlag = (FirstByte & 0b10) == 0b10;
        u8 Mod = SecondByte >> 6;
        u8 RM = SecondByte & 0b111;
        switch(Op) {
            case opcode_add: Instruction.Op = op_add; break;
            case opcode_sub: Instruction.Op = op_sub; break;
            case opcode_cmp: Instruction.Op = op_cmp; break;
            default: Assert(0, "invalid path");
        }

        switch(Mod) {
            case mod_reg: {
                Instruction.Mod = mod_reg;
                Instruction.Operand1.Bytes16 = 8*WFlag + RM;
                Instruction.Operand1.FieldType = ft_reg;
                if(WFlag && !SFlag) {
                    Instruction.Operand2.Bytes16 = *(i16 *)(ByteStream->Bytes + (*InstPointer)++); (*InstPointer)++;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
            case mod_mem_8_dis: {
                Instruction.Mod = mod_mem_8_dis;
                Instruction.Operand1.Bytes16 = RM;
                Instruction.Operand1.FieldType = ft_effe_sized;
                Instruction.Operand1.IsBYTE = !WFlag;
                Instruction.Extended.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++); (*InstPointer)++;
                Instruction.Extended.IsBYTE = true;
                Instruction.Extended.FieldType = ft_disp;
                if(WFlag && !SFlag) {
                    u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
            case mod_mem_16_dis: {
                Instruction.Mod = mod_mem_16_dis;

                Instruction.Operand1.Bytes16 = RM;
                Instruction.Operand1.FieldType = ft_effe_sized;
                Instruction.Operand1.IsBYTE = !WFlag;
                u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                Instruction.Extended.Bytes16 = (HighByte << 8) | LowByte;
                Instruction.Extended.FieldType = ft_disp;
                if(WFlag && !SFlag) {
                    LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;

            case mod_mem_no_dis: {
                Instruction.Mod = mod_mem_no_dis;
                Instruction.DirectAddress = RM == 0b110;
                if(Instruction.DirectAddress) {
                    Instruction.Operand1.Bytes16 = *(i16 *)(ByteStream->Bytes + (*InstPointer)++); (*InstPointer)++;
                    Instruction.Operand1.FieldType = ft_mem_sized;
                    Instruction.Operand1.IsBYTE = !WFlag;
                } else {
                    Instruction.Operand1.Bytes16 = RM;
                    Instruction.Operand1.FieldType = ft_effe_sized;
                    Instruction.Operand1.IsBYTE = !WFlag;
                }
                if(WFlag && !SFlag) {
                    u8 LowByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    u16 HighByte = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(i8 *)(ByteStream->Bytes + (*InstPointer)++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
        }
    } else if((FirstByte >> 1) == (opcode_immed_accum | (opcode_add << 2)) || // non-mov immedidate to accumulator
              (FirstByte >> 1) == (opcode_immed_accum | (opcode_sub << 2)) ||
              (FirstByte >> 1) == (opcode_immed_accum | (opcode_cmp << 2))) {
        u8 WFlag = (FirstByte & 0b1) == 0b1;
        Instruction.Operand1.Bytes16 = WFlag ? ax : al;
        Instruction.Operand1.FieldType = ft_reg;
        switch((FirstByte << 2) >> 5) {
            case opcode_add: Instruction.Op = op_add; break;
            case opcode_sub: Instruction.Op = op_sub; break;
            case opcode_cmp: Instruction.Op = op_cmp; break;
            default: Assert(0, "invalid path");
        }
        if(WFlag) {
            Instruction.Operand2.Bytes16 = *(i16 *)(ByteStream->Bytes + (*InstPointer)++); (*InstPointer)++;
        } else {
            Instruction.Operand2.Bytes8[0] = *(i8*)(ByteStream->Bytes + (*InstPointer)++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_imme;
    } else if((FirstByte >> 2) == (opcode_regmem_reg | (opcode_add << 1)) || // non-mov reg/mem to reg
              (FirstByte >> 2) == (opcode_regmem_reg | (opcode_sub << 1)) ||
              (FirstByte >> 2) == (opcode_regmem_reg | (opcode_cmp << 1))) {
        switch((FirstByte << 2) >> 5) {
            case opcode_add: Instruction.Op = op_add; break;
            case opcode_sub: Instruction.Op = op_sub; break;
            case opcode_cmp: Instruction.Op = op_cmp; break;
            default: Assert(0, "invalid path");
        }
        ImmedRegMemToRegMem(ByteStream, InstPointer, &Instruction, FirstByte, false, false);
    } else {
        i32 JumpOpIdx = 0;
        if(IsJumpInstruction(FirstByte, &JumpOpIdx)) {

            Instruction.Op = JumpOpIdx;
            i8 Data = (*(i8*)(ByteStream->Bytes + (*InstPointer)++));

            Instruction.Operand2.FieldType = ft_jump;
            Instruction.Operand2.IsBYTE = true;
            Instruction.Operand2.Bytes8[0] = Data;

            if((Instruction.Op == op_loop) || (Instruction.Op == op_loopz) ||
               (Instruction.Op == op_loopnz)) {
                Instruction.Operand1.FieldType = ft_reg;
                Instruction.Operand1.Bytes16 = 9; /* cx register */
            } else {
                Instruction.Operand1.FieldType = ft_empty;
            }

        } else Assert(0, "invalid opcode");
    }

    return Instruction;
}
