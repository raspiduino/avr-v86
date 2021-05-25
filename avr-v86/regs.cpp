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

#include "regs.h"

unsigned char *regs8;
unsigned short *regs16;

// Load instruction decoding helper table
unsigned char bios_table_lookup(unsigned char i, unsigned char j)
{
    return readmem(0xF100 + readmem(0xF181 + i) + j);
}

unsigned char readregs8(unsigned short addr)
{
    // Read regs at addr
    //return readmem(REGS_BASE + addr);
    return regs8[addr];
}

unsigned char writeregs8(unsigned short addr, unsigned char value)
{
    // Write regs at addr
    regs8[addr] = value;
    return writemem(REGS_BASE + addr, value);
}

unsigned short readregs16(unsigned short addr)
{
    //return ((unsigned short)readregs8(addr) << 8) | readregs8(addr + 1); // Read 2 unsigned char and convert to 1 unsigned short
    return regs16[addr];
}

unsigned short writeregs16(unsigned short addr, unsigned short value)
{
    regs16[addr] = value;
    // Convert a unsigned short to 2 unsigned char and write
    writeregs8(addr, value & 0xff);
    writeregs8(addr+1, (value >> 8));
    return value;
}
