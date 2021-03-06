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

// SD
#include <SD.h>
#include <SPI.h>

#include "config.h"

extern File file;

void sdinit();
void fillmem();
void loadbios();

#ifdef HARDDISK
unsigned short hdsize();
#endif

unsigned char readmem(unsigned short addr);
unsigned char writemem(unsigned short addr, unsigned char value);
bool seekandcheck(int disk, unsigned short addr);
