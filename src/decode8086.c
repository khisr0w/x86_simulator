/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  8/12/2023 3:31:40 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#include "decode8086.h"

internal inline byte_stream
ReadBinaryFileIntoStream(char *FileName) {
    byte_stream Stream = {0};

    FILE* File;
    if(fopen_s(&File, FileName, "rb") != 0) return Stream;

    /* NOTE(Abid): Get the size of the File */
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    rewind(File);

    uint8* Content = (uint8 *)malloc(FileSize);
    if (Content == NULL) {
        fclose(File);
        return Stream;
    }

    size_t BytesRead = fread(Content, 1, FileSize, File);
    if (BytesRead != FileSize) {
        fclose(File);
        free(Content);
        return Stream;
    }
    fclose(File);

    Stream.Bytes = Content;
    Stream.NumBytes = BytesRead;
    Stream.Loaded = true;

    return Stream;
}

internal inline uint16
IsRETInstruction(uint8 Op_Byte) {

    local_persist uint8 RETInst[] = {
    #define X(Enum) op_##Enum,
    #define Y(Value)
        X_RET_INSTRUCTION
    #undef Y
    #undef X
    };

    for(int Idx = 0; Idx < ArraySize(RETInst); ++Idx)
        if(RETInst[Idx] == Op_Byte) return (uint16)(Idx+1); // Adding 1 since the first str element is "none"
    return 0;
}

internal void
_RegMem_Reg_Output(byte_stream *Stream, instruction *Inst, uint8 FirstByte) {
    uint8 WFlag = FirstByte & 0b1;
    uint8 DFlag = (FirstByte & 0b10) == 0b10;

    uint8 SecondByte = Stream->Bytes[Stream->CurByteIdx++];
    uint8 Reg = ((uint8)(SecondByte << 2) >> 5);
    uint8 RM = SecondByte & 0b111;
    uint8 Mod = SecondByte >> 6;
    Inst->Mod = Mod;

    /* NOTE(Abid): Operands[0] : destination
     *             Operands[1] : source
     */
    field *InstOperands[] = {&Inst->Operand1, &Inst->Operand2};
    switch(Inst->Mod) {
        case mod_reg: {
            uint8 REGRMIdx[] = {Reg, RM};
            InstOperands[0]->Bytes16 = 8*WFlag + REGRMIdx[1-DFlag];
            InstOperands[0]->FieldType = ft_reg;

            InstOperands[1]->Bytes16 = 8*WFlag + REGRMIdx[DFlag];
            InstOperands[1]->FieldType = ft_reg;
        } break;
        case mod_mem_no_dis: {
            uint32 RegAsDest = DFlag ? 0 : 1;
            InstOperands[RegAsDest]->Bytes16 = 8*WFlag + Reg;
            InstOperands[RegAsDest]->FieldType = ft_reg;

            Inst->DirectAddress = RM == 0b110;
            if(Inst->DirectAddress) {
                InstOperands[1-RegAsDest]->Bytes16 = *(int16 *)(Stream->Bytes + Stream->CurByteIdx++); Stream->CurByteIdx++;
                InstOperands[1-RegAsDest]->FieldType = ft_mem;
            } else {
                InstOperands[1-RegAsDest]->Bytes16 = RM; // Effective address calculation
                InstOperands[1-RegAsDest]->FieldType = ft_effe;
            }
        } break;
        case mod_mem_8_dis: {
            // TODO(Abid): Check the low and high bits see if they conform for the endian stuff.
            uint32 RegAsDest = DFlag ? 0 : 1;
            InstOperands[RegAsDest]->Bytes16 = 8*WFlag + Reg;
            InstOperands[RegAsDest]->FieldType = ft_reg;
            InstOperands[1-RegAsDest]->Bytes16 = RM;
            InstOperands[1-RegAsDest]->FieldType = ft_effe;

            Inst->Extended.Bytes8[0] = *(int8 *)(Stream->Bytes + Stream->CurByteIdx++);
            Inst->Extended.IsBYTE = true;
            Inst->Extended.FieldType = ft_disp;
        } break;
        case mod_mem_16_dis: {
            uint32 RegAsDest = DFlag ? 0 : 1;
            InstOperands[RegAsDest]->Bytes16 = 8*WFlag + Reg;
            InstOperands[RegAsDest]->FieldType = ft_reg;
            InstOperands[1-RegAsDest]->Bytes16 = RM;
            InstOperands[1-RegAsDest]->FieldType = ft_effe;

            uint8 LowByte = *(int8 *)(Stream->Bytes + Stream->CurByteIdx++);
            uint16 HighByte = *(int8 *)(Stream->Bytes + Stream->CurByteIdx++);
            Inst->Extended.Bytes16 = (HighByte << 8) | LowByte;
            Inst->Extended.FieldType = ft_disp;
        } break;
        default: Assert(0, "invalid code path");
    }
}

/* NOTE(Abid): Here's how I would want it to work in the end:
 *             The decoder, decodes the binary and transforms it into the intermediate representation
 *             that we have here. As the decoder is adding more instructions, the simulator starts
 *             executing the instruction and forwarding the pointer to the next instruction that
 *             should be executed. As of now, the program will take turns in decoding and simulating
 *             the code. For later, there could be 2 threads, one for simulation and another for
 *             decoding.
 *             Initially, we decode one instruction and simulate it immediately.
 */
internal instruction
DecodeNext(byte_stream *ByteStream)
{
    uint8 FirstByte = ByteStream->Bytes[ByteStream->CurByteIdx++];
    instruction Instruction = {0};

    if((FirstByte >> 2) == opcode_regmem_reg_mov) { // Register/Memory or Register move
        Instruction.Op = op_mov;
        _RegMem_Reg_Output(ByteStream, &Instruction, FirstByte);
    } else if((FirstByte >> 4) == opcode_immed_reg_mov) { // Immediate to register move
        Instruction.Op = op_mov;

        uint8 WFlag = (FirstByte & 0b1000) == 0b1000;
        uint8 Reg = FirstByte & 0b111;
        Instruction.Operand1.Bytes16 = 8*WFlag + Reg;
        Instruction.Operand1.FieldType = ft_reg;
        if(WFlag) {
            Instruction.Operand2.Bytes16 = *(int16 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
        } else {
            Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_imme;
    } else if((FirstByte >> 1) == opcode_immed_regmem_mov) { // Immediate to register/memory move
        Instruction.Op = op_mov;
        uint8 SecondByte = ByteStream->Bytes[ByteStream->CurByteIdx++];
        uint8 Mod = SecondByte >> 6;
        uint8 WFlag = FirstByte & 0b1;
        uint8 RM = SecondByte & 0b111;

        Instruction.Operand1.Bytes16 = RM; // Effective address calculation
        Instruction.Operand1.FieldType = ft_effe;
        switch(Mod) {
            /* NOTE(Abid): The reg mod should've been done above in immediate to register mode */
            case mod_reg: { Assert(0, "Cannot have this mod here"); } break;
            case mod_mem_8_dis: {
                Instruction.Mod = mod_mem_8_dis;
                int8 Disp = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
                Instruction.Extended.Bytes8[0] = Disp;
                Instruction.Extended.IsBYTE = true;
                Instruction.Extended.FieldType = ft_disp;
            } break;
            case mod_mem_16_dis: {
                Instruction.Mod = mod_mem_16_dis;
                uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                int16 Disp = (HighByte << 8) | LowByte;
                Instruction.Extended.Bytes16 = Disp;
                Instruction.Extended.FieldType = ft_disp;
            } break;
            case mod_mem_no_dis: { Instruction.Mod = mod_mem_no_dis; }break;
        }

        if(WFlag) {
            uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
        } else {
            Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_imme_sized;
    } else if((FirstByte >> 1) == opcode_mem_accum_mov) { // Memory to accumulator
        Instruction.Op = op_mov;
        uint8 WFlag = FirstByte & 0b1;

        Instruction.Operand1.Bytes16 = WFlag ? ax : al;
        Instruction.Operand1.FieldType = ft_reg;
        if(WFlag) {
            uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte; // Address to move from
        } else {
            Instruction.Operand2.Bytes16 = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand2.IsBYTE = true;
        }
        Instruction.Operand2.FieldType = ft_mem;
    } else if((FirstByte >> 1) == opcode_accum_mem_mov) { // Accumulator to memory
        Instruction.Op = op_mov;
        uint8 WFlag = FirstByte & 0b1;

        Instruction.Operand2.Bytes16 = WFlag ? ax : al;
        Instruction.Operand2.FieldType = ft_reg;
        if(WFlag) {
            uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand1.Bytes16 = (HighByte << 8) | LowByte;
        } else {
            Instruction.Operand1.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
            Instruction.Operand1.IsBYTE = true;
        }
        Instruction.Operand1.FieldType = ft_mem;
    } else if((FirstByte >> 2) == opcode_immed_regmem) {
        // NOTE(Abid): non-move immediate to reg/mem; will apply to add, sub, cmp and some other math ops
        uint8 SecondByte = ByteStream->Bytes[ByteStream->CurByteIdx++];
        uint8 Op = ((uint8)(SecondByte << 2) >> 5);
        uint8 WFlag = (FirstByte & 0b1) == 0b1;
        uint8 SFlag = (FirstByte & 0b10) == 0b10;
        uint8 Mod = SecondByte >> 6;
        uint8 RM = SecondByte & 0b111;
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
                    Instruction.Operand2.Bytes16 = *(int16 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
            case mod_mem_8_dis: {
                Instruction.Mod = mod_mem_8_dis;
                Instruction.Operand1.Bytes16 = RM;
                Instruction.Operand1.FieldType = ft_effe_sized;
                Instruction.Operand1.IsBYTE = !WFlag;
                Instruction.Extended.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
                Instruction.Extended.IsBYTE = true;
                Instruction.Extended.FieldType = ft_disp;
                if(WFlag && !SFlag) {
                    uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
            case mod_mem_16_dis: {
                Instruction.Mod = mod_mem_16_dis;

                Instruction.Operand1.Bytes16 = RM;
                Instruction.Operand1.FieldType = ft_effe_sized;
                Instruction.Operand1.IsBYTE = !WFlag;
                uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                Instruction.Extended.Bytes16 = (HighByte << 8) | LowByte;
                Instruction.Extended.FieldType = ft_disp;
                if(WFlag && !SFlag) {
                    LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;

            case mod_mem_no_dis: {
                Instruction.Mod = mod_mem_no_dis;
                Instruction.DirectAddress = RM == 0b110;
                if(Instruction.DirectAddress) {
                    Instruction.Operand1.Bytes16 = *(int16 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
                    Instruction.Operand1.FieldType = ft_mem_sized;
                    Instruction.Operand1.IsBYTE = !WFlag;
                } else {
                    Instruction.Operand1.Bytes16 = RM;
                    Instruction.Operand1.FieldType = ft_effe_sized;
                    Instruction.Operand1.IsBYTE = !WFlag;
                }
                if(WFlag && !SFlag) {
                    uint8 LowByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    uint16 HighByte = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.Bytes16 = (HighByte << 8) | LowByte;
                } else {
                    Instruction.Operand2.Bytes8[0] = *(int8 *)(ByteStream->Bytes + ByteStream->CurByteIdx++);
                    Instruction.Operand2.IsBYTE = true;
                }
                Instruction.Operand2.FieldType = ft_imme;
            } break;
        }
    } else if((FirstByte >> 1) == (opcode_immed_accum | (opcode_add << 2)) || // non-mov immedidate to accumulator
              (FirstByte >> 1) == (opcode_immed_accum | (opcode_sub << 2)) ||
              (FirstByte >> 1) == (opcode_immed_accum | (opcode_cmp << 2))) {
        uint8 WFlag = (FirstByte & 0b1) == 0b1;
        Instruction.Operand1.Bytes16 = WFlag ? ax : al;
        Instruction.Operand1.FieldType = ft_reg;
        switch((FirstByte << 2) >> 5) {
            case opcode_add: Instruction.Op = op_add; break;
            case opcode_sub: Instruction.Op = op_sub; break;
            case opcode_cmp: Instruction.Op = op_cmp; break;
            default: Assert(0, "invalid path");
        }
        if(WFlag) {
            Instruction.Operand2.Bytes16 = *(int16 *)(ByteStream->Bytes + ByteStream->CurByteIdx++); ByteStream->CurByteIdx++;
        } else {
            Instruction.Operand2.Bytes8[0] = *(int8*)(ByteStream->Bytes + ByteStream->CurByteIdx++);
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
        _RegMem_Reg_Output(ByteStream, &Instruction, FirstByte);
    } else {
        uint16 RETOpIdx = IsRETInstruction(FirstByte);
        if(RETOpIdx != 0) {
            /* NOTE(Abid): The jump value address is added with 2, since nasm computes the relative 
             *             jump position from the start of the jump instruction (2 bytes), instead
             *             of the end.
             */
            Instruction.Op = op_ret;
            Instruction.Operand1.Bytes16 = RETOpIdx;
            Instruction.Operand1.FieldType = ft_ret;

            int8 Data = *(int8*)(ByteStream->Bytes + ByteStream->CurByteIdx++) + 2;
            Instruction.Operand2.Bytes16 = Data;
            Instruction.Operand2.FieldType = ft_imme;
        } else Assert(0, "invalid opcode");
    }

    return Instruction;
}
