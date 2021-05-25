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

#include "devices.h"

void terminal_putchar(unsigned char value)
{
    Serial.print((char)value);
}

unsigned char getch()
{
    if(Serial.available() > 0)
    {
        return Serial.read();
    }
}

int disk(bool op, int disknum, unsigned short offset, unsigned short b)
{
    unsigned short i = 0;
    char membuffer[BUFFER] = ""; // Memory buffer
    
    if(op)
    {
        // Write to disk
        for(i = 0; i <= b;){
            file = SD.open(RAM_FILE, FILE_READ);
            file.seek(offset + i);
            file.read(membuffer, BUFFER);
            file.close();

            file = SD.open(DISK_FILE, FILE_READ|FILE_WRITE);
            file.seek(((unsigned)readregs16(REG_BP) << 9) + i);
            file.write(membuffer);
            file.close();
            
            i += BUFFER;
        }

        return b;
    }

    else
    {
        // Read from disk
        for(i = 0; i <= b;){
            file = SD.open(DISK_FILE, FILE_READ);
            file.seek(((unsigned)readregs16(REG_BP) << 9) + i);
            file.read(membuffer, BUFFER);
            file.close();

            file = SD.open(RAM_FILE, FILE_READ|FILE_WRITE);
            file.seek(offset + i);
            file.write(membuffer);
            file.close();
            
            i += BUFFER;
        }

        return b;
    }

    return i;
}
