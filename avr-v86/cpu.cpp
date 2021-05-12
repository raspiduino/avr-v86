/*
    avr-v86 - Intel(R) 8086 emulator running on Arduino UNO
    Copyright (C) 2021 @raspiduino

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cpu.h"

// Emulator vars
unsigned char tmpvar1, tmpvar2, *opcode_stream, xlat_opcode_id, raw_opcode_id, extra, i_reg4bit, i_w, i_d, seg_override_en, rep_override_en, i_reg, i_mod, i_mod_size, i_rm, seg_override, scratch_uchar, rep_mode;
unsigned short reg_ip;
unsigned int set_flags_type, i_data0, i_data1, i_data2, scratch_uint, scratch2_uint, op_to_addr, op_from_addr, rm_addr, op_dest, op_source;
int op_result, scratch_int;

// Helper macros

// Reinterpretation cast
#define CAST(a) *(a*)&

// Return memory-mapped register location (offset into mem array) for register #reg_id
#define GET_REG_ADDR(reg_id) (REGS_BASE + (i_w ? 2 * reg_id : 2 * reg_id + reg_id / 4 & 7))

// Decode mod, r_m and reg fields in instruction
#define DECODE_RM_REG scratch2_uint = 4 * !i_mod, \
                      op_to_addr = rm_addr = i_mod < 3 ? segreg(seg_override_en ? seg_override : bios_table_lookup(scratch2_uint + 3, i_rm), bios_table_lookup(scratch2_uint, i_rm), +readregs16(bios_table_lookup(scratch2_uint + 1, i_rm))+bios_table_lookup(scratch2_uint + 2, i_rm)*i_data1) : GET_REG_ADDR(i_rm), \
                      op_from_addr = GET_REG_ADDR(i_reg), \
                      i_d && (scratch_uint = op_from_addr, op_from_addr = rm_addr, op_to_addr = scratch_uint)

// Opcode execution unit helpers
#define OPCODE ;break; case
#define OPCODE_CHAIN ; case

// Execute arithmetic/logic operations in emulator memory/registers
#define R_M_OP(dest,op,src) (i_w ? op_dest = CAST(unsigned short)dest, op_result = CAST(unsigned short)dest op (op_source = CAST(unsigned short)src) \
                                 : (op_dest = dest, op_result = dest op (op_source = CAST(unsigned char)src)))

#define MEM_OP(dest,op,src) tmpvar2 = readmem(src), \
                            R_M_OP(tmpvar1,op,tmpvar2), \
                            writemem(dest, tmpvar1)

#define OP(op) MEM_OP(op_to_addr,op,op_from_addr)

// Returns number of top bit in operand (i.e. 8 for 8-bit operands, 16 for 16-bit operands)
#define TOP_BIT 8*(i_w + 1)

// [I]MUL/[I]DIV/DAA/DAS/ADC/SBB helpers
#define MUL_MACRO(op_data_type,out_regs) (set_opcode(0x10), \
                                          out_regs ? writeregs16(i_w + 1, (op_result = (op_data_type)readmem[rm_addr] * (op_data_type)readregs16(0)) >> 16) \
                                                   : writeregs8(i_w + 1, (op_result = (op_data_type)readmem[rm_addr] * (op_data_type)readregs8(0)) >> 16), \
                                          writeregs16(REG_AX, op_result), \
                                          set_OF(set_CF(op_result - (op_data_type)op_result)))

#define DIV_MACRO(out_data_type,in_data_type,out_regs) out_regs ? ((scratch_int = (out_data_type)readmem(rm_addr)) && !(scratch2_uint = (in_data_type)(scratch_uint = (readregs16(i_w+1) << 16) + readregs16(REG_AX)) / scratch_int, scratch2_uint - (out_data_type)scratch2_uint) ? writeregs16(i_w+1, scratch_uint - scratch_int * (writeregs16(0, scratch2_uint))) : pc_interrupt(0)) \
                                                                : ((scratch_int = (out_data_type)readmem(rm_addr)) && !(scratch2_uint = (in_data_type)(scratch_uint = (readregs8(i_w+1) << 16) + readregs16(REG_AX)) / scratch_int, scratch2_uint - (out_data_type)scratch2_uint) ? writeregs8(i_w+1, scratch_uint - scratch_int * (writeregs8(0, scratch2_uint))) : pc_interrupt(0))

#define ADC_SBB_MACRO(a) OP(a##= readregs8(FLAG_CF) +), \
                         set_CF(readregs8(FLAG_CF) && (op_result == op_dest) || (a op_result < a(int)op_dest)), \
                         set_AF_OF_arith()

#define DAA_DAS(op1,op2,mask,minval) tmpvar1 = readregs8(REG_AL), \
                                     set_AF((((scratch2_uint = readregs8(REG_AL)) & 0x0F) > 9) || readregs8(FLAG_AF)) && (op_result = tmpvar1 op1 6, set_CF(readregs8(FLAG_CF) || (tmpvar1 op2 scratch2_uint))), \
                                     writeregs8(REG_AL, tmpvar1), \
                                     tmpvar1 = readregs8(REG_AL), \
                                     set_CF((((mask & 1 ? scratch2_uint : readregs8(REG_AL)) & mask) > minval) || readregs8(FLAG_CF)) && (op_result = tmpvar1 op1 0x60), \
                                     writeregs8(REG_AL, tmpvar1)

// Increment or decrement a register #reg_id (usually SI or DI), depending on direction flag and operand size (given by i_w)
#define INDEX_INC(reg_id) writeregs16(reg_id, (readregs16(reg_id) - (2 * readregs8(FLAG_DF) - 1)*(i_w + 1)))

// Convert raw opcode to translated opcode index. This condenses a large number of different encodings of similar
// instructions into a much smaller number of distinct functions, which we then execute
void set_opcode(unsigned char opcode)
{
    xlat_opcode_id = bios_table_lookup(TABLE_XLAT_OPCODE, raw_opcode_id = opcode);
    extra = bios_table_lookup(TABLE_XLAT_SUBFUNCTION, opcode);
    i_mod_size = bios_table_lookup(TABLE_I_MOD_SIZE, opcode);
    set_flags_type = bios_table_lookup(TABLE_STD_FLAGS, opcode);
}

// Helpers for stack operations
unsigned char r_m_push(unsigned short a)
{
    i_w = 1;
    tmpvar1 = tmpvar2 = readmem(segreg(REG_SS, REG_SP, -1));
    R_M_OP(tmpvar1, =, a);
    if (tmpvar1 != tmpvar2)
    {
        writemem(segreg(REG_SS, REG_SP, -1), tmpvar1);
    }
    i_w = 0; // Set it back to 0 for other commands
    return tmpvar1;
}
unsigned char r_m_pop(unsigned short a)
{
    i_w = 1;
    writeregs16(REG_SP, readregs16(REG_SP) + 2);
    tmpvar1 = tmpvar2 = readmem(segreg(REG_SS, REG_SP, -2));
    R_M_OP(a, =, tmpvar1);
    if (tmpvar1 != tmpvar2)
    {
        writemem(segreg(REG_SS, REG_SP, -2), tmpvar1);
    }
    i_w = 0; // Set it back to 0 for other commands
    return tmpvar1;
}

// Set carry flag
char set_CF(int new_CF)
{
    writeregs8(FLAG_CF, !!new_CF);
}

// Set auxiliary flag
char set_AF(int new_AF)
{
    writeregs8(FLAG_AF, !!new_AF);
}

// Set overflow flag
char set_OF(int new_OF)
{
    writeregs8(FLAG_OF, !!new_OF);
}

// Set auxiliary and overflow flag after arithmetic operations
char set_AF_OF_arith()
{
    set_AF((op_source ^= op_dest ^ op_result) & 0x10);
    if (op_result == op_dest)
        return set_OF(0);
    else
        return set_OF(1 & (readregs8(FLAG_CF) ^ op_source >> (TOP_BIT - 1)));
}

// Convert segment:offset to linear address in emulator memory space
unsigned short segreg(int reg_seg, int reg_ofs, int op)
{
    return 16 * readregs16(reg_seg) + (unsigned short)(readregs16(reg_ofs) + op);
}

// Assemble and return emulated CPU FLAGS register in scratch_uint
void make_flags()
{
    scratch_uint = 0xF002; // 8086 has reserved and unused flags set to 1
    for (int i = 9; i--;)
        scratch_uint += readregs8(FLAG_CF + i) << bios_table_lookup(TABLE_FLAGS_BITFIELDS, i);
}

// Execute INT #interrupt_num on the emulated machine
char pc_interrupt(unsigned char interrupt_num)
{
    set_opcode(0xCD); // Decode like INT

    make_flags();
    r_m_push(scratch_uint);
    r_m_push(readregs16(REG_CS));
    r_m_push(reg_ip);
    MEM_OP(REGS_BASE + 2 * REG_CS, =, 4 * interrupt_num + 2);
    R_M_OP(reg_ip, =, readmem[4 * interrupt_num]);
    writeregs8(FLAG_TF, 0);
    writeregs8(FLAG_IF, 0);
    return 0;
}

// Set emulated CPU FLAGS register from regs8[FLAG_xx] values
void set_flags(int new_flags)
{
    for (int i = 9; i--;)
        writeregs8(FLAG_CF + i, !!(1 << bios_table_lookup(TABLE_FLAGS_BITFIELDS, i) & new_flags));
}

void v86()
{
    // Main emulator function

    // CS is initialised to F000
    writeregs16(REG_CS, 0xF000);

    // Trap flag off
    writeregs8(FLAG_TF, 0);

    // Set DL equal to the boot device: 0 for the FD, or 0x80 for the HD. Normally, boot from the HD, but you can change to 0 (FD) if you want
    writeregs8(REG_DL, 0x80);

    #ifdef HARDDISK
    // Set CX:AX equal to the hard disk image size, if present
    writeregs16(REG_AX, hdsize());
    #endif

    // Load BIOS image into F000:0100, and set IP to 0100
    loadbios();
    reg_ip = 0x100;

    // Instruction execution loop
    for(; opcode_stream = readmem(16 * readregs16(REG_CS) + reg_ip);)
    {
        // Set up variables to prepare for decoding an opcode
        set_opcode(opcode_stream);

        // Extract i_w and i_d fields from instruction
        i_w = (i_reg4bit = raw_opcode_id & 7) & 1;
        i_d = i_reg4bit / 2 & 1;

        // Extract instruction data fields
        i_data0 = CAST(short)opcode_stream[1];
        i_data1 = CAST(short)opcode_stream[2];
        i_data2 = CAST(short)opcode_stream[3];

        // seg_override_en and rep_override_en contain number of instructions to hold segment override and REP prefix respectively
        if (seg_override_en)
            seg_override_en--;
        if (rep_override_en)
            rep_override_en--;

        // i_mod_size > 0 indicates that opcode uses i_mod/i_rm/i_reg, so decode them
        if (i_mod_size)
        {
            i_mod = (i_data0 & 0xFF) >> 6;
            i_rm = i_data0 & 7;
            i_reg = i_data0 / 8 & 7;

            if ((!i_mod && i_rm == 6) || (i_mod == 2))
                i_data2 = CAST(short)opcode_stream[4];
            else if (i_mod != 1)
                i_data2 = i_data1;
            else // If i_mod is 1, operand is (usually) 8 bits rather than 16 bits
                i_data1 = (char)i_data1;

            DECODE_RM_REG;   
        }

        // Instruction execution unit
        switch (xlat_opcode_id)
        {
            OPCODE_CHAIN 0: // Conditional jump (JAE, JNAE, etc.)
                // i_w is the invert flag, e.g. i_w == 1 means JNAE, whereas i_w == 0 means JAE 
                scratch_uchar = raw_opcode_id / 2 & 7;
                reg_ip += (char)i_data0 * (i_w ^ ((unsigned char)readregs8(bios_table_lookup(TABLE_COND_JUMP_DECODE_A, scratch_uchar)) || (unsigned char)readregs8(bios_table_lookup(TABLE_COND_JUMP_DECODE_B, scratch_uchar)) || (unsigned char)readregs8(bios_table_lookup(TABLE_COND_JUMP_DECODE_C, scratch_uchar)) ^ (unsigned char)readregs16(bios_table_lookup(TABLE_COND_JUMP_DECODE_D, scratch_uchar))));
            OPCODE 1: // MOV reg, imm
                i_w = !!(raw_opcode_id & 8);
                tmpvar1 = tmpvar2 = readmem(GET_REG_ADDR(i_reg4bit));
                R_M_OP(tmpvar1, =, i_data0);
                writemem(GET_REG_ADDR(i_reg4bit), tmpvar1);
            OPCODE 3: // PUSH regs16
                r_m_push(readregs16(i_reg4bit));
            OPCODE 4: // POP regs16
                r_m_pop(readregs16(i_reg4bit));
            OPCODE 2: // INC|DEC regs16
                i_w = 1;
                i_d = 0;
                i_reg = i_reg4bit;
                DECODE_RM_REG;
                i_reg = extra;
            OPCODE_CHAIN 5: // INC|DEC|JMP|CALL|PUSH
                if (i_reg < 2)// INC|DEC
                    MEM_OP(op_from_addr, += 1 - 2 * i_reg +, REGS_BASE + 2 * REG_ZERO),
                    op_source = 1,
                    set_AF_OF_arith(),
                    set_OF(op_dest + 1 - i_reg == 1 << (TOP_BIT - 1)),
                    set_OF(op_dest + 1 - i_reg == 1 << (TOP_BIT - 1)),
                    (xlat_opcode_id == 5) && (set_opcode(0x10), 0); // Decode like ADC
                else if (i_reg != 6) // JMP|CALL
                    i_reg - 3 || r_m_push(readregs16(REG_CS)), // CALL (far)
                    i_reg & 2 && r_m_push(reg_ip + 2 + i_mod*(i_mod != 3) + 2*(!i_mod && i_rm == 6)), // CALL (near or far)
                    i_reg & 1 && readmem(op_from_addr + 2),
                    writeregs16(REG_CS, readmem(op_from_addr + 2)), // JMP|CALL (far)
                    tmpvar1 = readmem(op_from_addr),
                    R_M_OP(reg_ip, =, tmpvar1),
                    set_opcode(0x9A); // Decode like CALL
                else // PUSH
                    tmpvar1 = readmem(rm_addr);
                    r_m_push(tmpvar1);
            OPCODE 6: // TEST r/m, imm16 / NOT|NEG|MUL|IMUL|DIV|IDIV reg
                op_to_addr = op_from_addr;

                switch (i_reg)
                {
                    OPCODE_CHAIN 0: // TEST
                        set_opcode(0x20); // Decode like AND
                        reg_ip += i_w + 1;
                        tmpvar1 = readmem(op_to_addr);
                        R_M_OP(tmpvar1, &, i_data2);
                        writemem(op_to_addr, tmpvar1);
                    OPCODE 2: // NOT
                        OP(=~)
                    OPCODE 3: // NEG
                        OP(=-);
                        op_dest = 0;
                        set_opcode(0x28); // Decode like SUB
                        set_CF(op_result > op_dest)
                    OPCODE 4: // MUL
                        i_w ? MUL_MACRO(unsigned short, 1) : MUL_MACRO(unsigned char, 0)
                    OPCODE 5: // IMUL
                        i_w ? MUL_MACRO(short, 1) : MUL_MACRO(char, 0)
                    OPCODE 6: // DIV
                        i_w ? DIV_MACRO(unsigned short, unsigned, 1) : DIV_MACRO(unsigned char, unsigned short, 0)
                    OPCODE 7: // IDIV
                        i_w ? DIV_MACRO(short, int, 1) : DIV_MACRO(char, short, 0);
                }
            OPCODE 7: // ADD|OR|ADC|SBB|AND|SUB|XOR|CMP AL/AX, immed
                rm_addr = REGS_BASE;
                i_data2 = i_data0;
                i_mod = 3;
                i_reg = extra;
                reg_ip--;
            OPCODE_CHAIN 8: // ADD|OR|ADC|SBB|AND|SUB|XOR|CMP reg, immed
                op_to_addr = rm_addr;
                writeregs16(REG_SCRATCH, (i_d |= !i_w) ? (char)i_data2 : i_data2);
                op_from_addr = REGS_BASE + 2 * REG_SCRATCH;
                reg_ip += !i_d + 1;
                set_opcode(0x08 * (extra = i_reg));
            OPCODE_CHAIN 9: // ADD|OR|ADC|SBB|AND|SUB|XOR|CMP|MOV reg, r/m
                switch (extra)
                {
                    OPCODE_CHAIN 0: // ADD
                        OP(+=),
                        set_CF(op_result < op_dest)
                    OPCODE 1: // OR
                        OP(|=)
                    OPCODE 2: // ADC
                        ADC_SBB_MACRO(+)
                    OPCODE 3: // SBB
                        ADC_SBB_MACRO(-)
                    OPCODE 4: // AND
                        OP(&=)
                    OPCODE 5: // SUB
                        OP(-=),
                        set_CF(op_result > op_dest)
                    OPCODE 6: // XOR
                        OP(^=)
                    OPCODE 7: // CMP
                        OP(-),
                        set_CF(op_result > op_dest)
                    OPCODE 8: // MOV
                        OP(=);
                }
            OPCODE 10: // MOV sreg, r/m | POP r/m | LEA reg, r/m
                if (!i_w) // MOV
                    i_w = 1,
                    i_reg += 8,
                    DECODE_RM_REG,
                    OP(=);
                else if (!i_d) // LEA
                    seg_override_en = 1,
                    seg_override = REG_ZERO,
                    DECODE_RM_REG,
                    tmpvar1 = readmem(op_from_addr),
                    R_M_OP(tmpvar1, =, rm_addr),
                    writemem(op_from_addr, tmpvar1);
                else // POP
                    r_m_pop(readmem(rm_addr));
            OPCODE 11: // MOV AL/AX, [loc]
                i_mod = i_reg = 0;
                i_rm = 6;
                i_data1 = i_data0;
                DECODE_RM_REG;
                MEM_OP(op_from_addr, =, op_to_addr);
            // OPCODE 12 here, come back later. It's too long and I'm too lazy!
            OPCODE 13: // LOOPxx|JCZX
                tmpvar1 = readregs16(REG_CX);
                writeregs16(REG_CX, tmpvar1-1);
                scratch_uint = tmpvar1-1;

                switch(i_reg4bit)
                {
                    OPCODE_CHAIN 0: // LOOPNZ
                        scratch_uint &= !readregs8(FLAG_ZF)
                    OPCODE 1: // LOOPZ
                        scratch_uint &= readregs8(FLAG_ZF)
                    OPCODE 3: // JCXXZ
                        tmpvar1 = readregs16(REG_CX);
                        writeregs16(REG_CX, tmpvar1+1);
                        scratch_uint = !tmpvar1+1;
                }
                reg_ip += scratch_uint*(char)i_data0;
            OPCODE 14: // JMP | CALL short/near
                reg_ip += 3 - i_d;
                if (!i_w)
                {
                    if (i_d) // JMP far
                        reg_ip = 0,
                        writeregs16(REG_CS, i_data2);
                    else // CALL
                        r_m_push(reg_ip);
                }
                reg_ip += i_d && i_w ? (char)i_data0 : i_data0;
            OPCODE 15: // TEST reg, r/m
                MEM_OP(op_from_addr, &, op_to_addr);
            OPCODE 16: // XCHG AX, regs16
                i_w = 1;
                op_to_addr = REGS_BASE;
                op_from_addr = GET_REG_ADDR(i_reg4bit);
            OPCODE_CHAIN 24: // NOP|XCHG reg, r/m
                if (op_to_addr != op_from_addr)
                    OP(^=),
                    MEM_OP(op_from_addr, ^=, op_to_addr),
                    OP(^=);
            OPCODE 17: // MOVSx (extra=0)|STOSx (extra=1)|LODSx (extra=2)
                scratch2_uint = seg_override_en ? seg_override : REG_DS;

                for (scratch_uint = rep_override_en ? readregs16(REG_CX) : 1; scratch_uint; scratch_uint--)
                {
                    MEM_OP(extra < 2 ? segreg(REG_ES, REG_DI, 0) : REGS_BASE, =, extra & 1 ? REGS_BASE : segreg(scratch2_uint, REG_SI, 0)),
                    extra & 1 || INDEX_INC(REG_SI),
                    extra & 2 || INDEX_INC(REG_DI);
                }

                if (rep_override_en)
                    writeregs16(REG_CX, 0);
            OPCODE 18: // CMPSx (extra=0)|SCASx (extra=1)
                scratch2_uint = seg_override_en ? seg_override : REG_DS;

                if ((scratch_uint = rep_override_en ? readregs16(REG_CX) : 1))
                {
                    for (; scratch_uint; rep_override_en || scratch_uint--)
                    {
                        MEM_OP(extra ? REGS_BASE : segreg(scratch2_uint, REG_SI,0), -, segreg(REG_ES, REG_DI,0)),
                        extra || INDEX_INC(REG_SI),
                        tmpvar1 = readregs16(REG_CX),
                        writeregs16(REG_CX, tmpvar1-1),
                        INDEX_INC(REG_DI), rep_override_en && !((tmpvar1 - 1) && (!op_result == rep_mode)) && (scratch_uint = 0);
                    }

                    set_flags_type = FLAGS_UPDATE_SZP | FLAGS_UPDATE_AO_ARITH; // Funge to set SZP/AO flags
                    set_CF(op_result > op_dest);
                }
            OPCODE 19: // RET|RETF|IRET
                i_d = i_w;
                r_m_pop(reg_ip);
                if (extra) // IRET|RETF|RETF imm16
                    r_m_pop(readregs16(REG_CS));
                if (extra & 2) // IRET
                    set_flags(r_m_pop(scratch_uint));
                else if (!i_d) // RET|RETF imm16
                    writeregs16(REG_SP, readregs16(REG_SP)+i_data0);
            OPCODE 20: // MOV r/m, immed
                tmpvar1 = readmem(op_from_addr);
                R_M_OP(tmpvar1, =, i_data2);
                writemem(op_from_addr, tmpvar1);
            // Opcode 21 here, come back later
            // Opcode 22 here, come back later
            OPCODE 23: // REPxx
                rep_override_en = 2;
                rep_mode = i_w;
                seg_override_en && seg_override_en++;
            OPCODE 25: // PUSH reg
                r_m_push(readregs16(extra));
            OPCODE 26: // POP reg
                r_m_pop(readregs16(extra));
            OPCODE 27: // xS: segment overrides
                seg_override_en = 2;
                seg_override = extra;
                rep_override_en && rep_override_en++;
            OPCODE 28: // DAA/DAS
                i_w = 0;
                extra ? DAA_DAS(-=, >=, 0xFF, 0x99) : DAA_DAS(+=, <, 0xF0, 0x90); // extra = 0 for DAA, 1 for DAS
            
        }
    }
}
