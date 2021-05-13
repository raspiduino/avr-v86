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

#include <Fat16.h> // Fat16 library, you can get it at https://github.com/greiman/Fat16
#include "config.h"
#include "devices.h"

void sdinit();
void loadbios();

#ifdef HARDDISK
unsigned short hdsize();
#endif

unsigned char readmem(unsigned short addr);
unsigned char writemem(unsigned short addr, unsigned char value);
unsigned char readregs8(unsigned short addr);
unsigned char writeregs8(unsigned short addr, unsigned char value);
unsigned short readregs16(unsigned short addr);
unsigned short writeregs16(unsigned short addr, unsigned short value);
unsigned char bios_table_lookup(unsigned int i, unsigned int j);
