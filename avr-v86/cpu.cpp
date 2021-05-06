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

// Reinterpretation cast
#define CAST(a) *(a*)&

// Emulator vars
unsigned char *opcode_stream, xlat_opcode_id, raw_opcode_id, extra, i_reg4bit, i_w, i_d, seg_override_en, rep_override_en, i_reg, i_mod, i_mod_size, i_rm, seg_override, scratch_uchar;
unsigned short reg_ip;
unsigned int set_flags_type, i_data0, i_data1, i_data2, scratch_uint, scratch2_uint, op_to_addr, op_from_addr, rm_addr;

// Helper macros

// Convert segment:offset to linear address in emulator memory space
#define SEGREG(reg_seg,reg_ofs,op) 16 * readregs(reg_seg) + (unsigned short)(op readregs(reg_ofs))

// Return memory-mapped register location (offset into mem array) for register #reg_id
#define GET_REG_ADDR(reg_id) (REGS_BASE + (i_w ? 2 * reg_id : 2 * reg_id + reg_id / 4 & 7))

// Decode mod, r_m and reg fields in instruction
#define DECODE_RM_REG scratch2_uint = 4 * !i_mod, \
                      op_to_addr = rm_addr = i_mod < 3 ? SEGREG(seg_override_en ? seg_override : bios_table_lookup(scratch2_uint + 3, i_rm), bios_table_lookup(scratch2_uint, i_rm), readregs(bios_table_lookup(scratch2_uint + 1, i_rm)) + bios_table_lookup(scratch2_uint + 2, i_rm) * i_data1+) : GET_REG_ADDR(i_rm), \
                      op_from_addr = GET_REG_ADDR(i_reg), \
                      i_d && (scratch_uint = op_from_addr, op_from_addr = rm_addr, op_to_addr = scratch_uint)

// Opcode execution unit helpers
#define OPCODE ;break; case
#define OPCODE_CHAIN ; case

// Convert raw opcode to translated opcode index. This condenses a large number of different encodings of similar
// instructions into a much smaller number of distinct functions, which we then execute
void set_opcode(unsigned char opcode)
{
    xlat_opcode_id = bios_table_lookup(TABLE_XLAT_OPCODE, raw_opcode_id = opcode);
    extra = bios_table_lookup(TABLE_XLAT_SUBFUNCTION, opcode);
    i_mod_size = bios_table_lookup(TABLE_I_MOD_SIZE, opcode);
    set_flags_type = bios_table_lookup(TABLE_STD_FLAGS, opcode);
}

void v86()
{
    // Main emulator function

    // CS is initialised to F000
    writeregs(REG_CS, 0xF000);

    // Trap flag off
    writeregs(FLAG_TF, 0);

    // Set DL equal to the boot device: 0 for the FD, or 0x80 for the HD. Normally, boot from the HD, but you can change to 0 (FD) if you want
    writeregs(REG_DL, 0x80);

    #ifdef HARDDISK
    // Set CX:AX equal to the hard disk image size, if present
    writeregs(REG_AX, hdsize());
    #endif

    // Load BIOS image into F000:0100, and set IP to 0100
    loadbios();
    reg_ip = 0x100;

    // Instruction execution loop
    for(; opcode_stream = readmem(16 * readregs(REG_CS) + reg_ip);)
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
                reg_ip += (char)i_data0 * (i_w ^ ((unsigned char)readregs(bios_table_lookup(TABLE_COND_JUMP_DECODE_A, scratch_uchar)) || (unsigned char)readregs(bios_table_lookup(TABLE_COND_JUMP_DECODE_B, scratch_uchar)) || (unsigned char)readregs(bios_table_lookup(TABLE_COND_JUMP_DECODE_C, scratch_uchar)) ^ (unsigned char)readregs(bios_table_lookup(TABLE_COND_JUMP_DECODE_D, scratch_uchar))));

        }
    }
}
