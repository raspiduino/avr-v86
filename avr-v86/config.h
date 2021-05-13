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

#define RAM_FILE "ram.bin" // Contain RAM data & memory mapped things
#define BIOS_FILE "bios.bin" // You must use the bios from 8086tiny (https://github.com/adriancable/8086tiny) project in order to make this emulator work!
#define DISK_FILE "disk.img"

#define BOOT_DEVICE 0x80 // Select the boot device, 0 for FD and 0x80 for HD. Default is boot from HD (0x80)

#define BUFFER 128 // Buffer for memory copy, ... in bytes

#define CHIP_SELECT 10 // Chip select pin for SD card

#define AUDIO // Enable this if you want to have sound support
//#define VGATEXTMODE // Enable this if you want only VGA in text mode. Normally disabled
#ifndef VGATEXTMODE
#define MOUSE // Enalbe this if you want to have mouse support. In text mode, this function is useless
#endif
#define KEYBOARD // Wait, what? Don't disable this unless you know what you are doing!
// Note about disks:
// You MUST't disable both floppy disk and hard disk unless you know what you are doing!
#define HARDDISK // Enable this if you want to support hard disk
#define BOOT_DEVICE 0 // Boot from FD or HD. 0 for FD and 1 for HD

/************************** Emulator constant. DO NOT edit! ***************************/

#define REGS_BASE 0xF0000 // Register location in memory mapped mode
#define IO_PORT_COUNT 0x10000
#define KEYBOARD_TIMER_UPDATE_DELAY 20000

// 16-bit register decodes
#define REG_AX 0
#define REG_CX 1
#define REG_DX 2
#define REG_BX 3
#define REG_SP 4
#define REG_BP 5
#define REG_SI 6
#define REG_DI 7

#define REG_ES 8
#define REG_CS 9
#define REG_SS 10
#define REG_DS 11

#define REG_ZERO 12
#define REG_SCRATCH 13

// 8-bit register decodes
#define REG_AL 0
#define REG_AH 1
#define REG_CL 2
#define REG_CH 3
#define REG_DL 4
#define REG_DH 5
#define REG_BL 6
#define REG_BH 7

// FLAGS register decodes
#define FLAG_CF 40
#define FLAG_PF 41
#define FLAG_AF 42
#define FLAG_ZF 43
#define FLAG_SF 44
#define FLAG_TF 45
#define FLAG_IF 46
#define FLAG_DF 47
#define FLAG_OF 48

// Lookup tables in the BIOS binary
#define TABLE_XLAT_OPCODE 8
#define TABLE_XLAT_SUBFUNCTION 9
#define TABLE_STD_FLAGS 10
#define TABLE_PARITY_FLAG 11
#define TABLE_BASE_INST_SIZE 12
#define TABLE_I_W_SIZE 13
#define TABLE_I_MOD_SIZE 14
#define TABLE_COND_JUMP_DECODE_A 15
#define TABLE_COND_JUMP_DECODE_B 16
#define TABLE_COND_JUMP_DECODE_C 17
#define TABLE_COND_JUMP_DECODE_D 18
#define TABLE_FLAGS_BITFIELDS 19

// Bitfields for TABLE_STD_FLAGS values
#define FLAGS_UPDATE_SZP 1
#define FLAGS_UPDATE_AO_ARITH 2
#define FLAGS_UPDATE_OC_LOGIC 4
