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

#include "sd.h"

/*Sd card*/
SdCard card;
Fat16 file;

//char filepath[12]; // Virtual disk filepath
char membuffer[BUFFER] = ""; // Memory buffer

void sdinit()
{
    /* Init the SD card */
    Serial.print(F("Init SD card"));
    while(!card.begin(CHIP_SELECT)) Serial.print("."); // Mount filesystem
    Serial.print(F(". Done! Init filesystem"));
    while(!Fat16::init(&card)) Serial.print(".");
    Serial.println(F(". Done!"));
}

void fillmem()
{
    Serial.print(F("Filling memory"));

    file.open(RAM_FILE, O_WRITE);
    
    while(1)
    {
        file.write((byte)0); // Fill with zero
        Serial.print(F("."));

        if(file.curPosition() == RAM_SIZE)
            break;
    }

    file.close();
    Serial.println(F(". Done!"));
}

void loadbios()
{
    // Load BIOS image into F000:0100 (0x100 of regs8)
    // Only need to read 0x1DF0 bytes from BIOS image

    fillmem(); // Fill mem
    
    Serial.print(F("Loading BIOS..."));
    
    for(long i = 0; i <= RAM_SIZE;)
    {
        file.open(BIOS_FILE, O_READ); // Open BIOS file in read mode
        file.seekSet(i);
        int br = file.read(membuffer, BUFFER); // Read to buffer
        file.close(); // Close that file

        // Load init ram to virtual RAM
        file.open(RAM_FILE, O_WRITE); // Open ram file in write mode
        file.seekSet(REGS_BASE + i); // Set the cusor to the register mapped location + the byte read
        file.write(membuffer, br); // Write to virtual RAM
        file.close(); // Close that file

        if (br < BUFFER)
            break;
        else
            i += br;

        Serial.print(F("."));
    }

    Serial.println(F("\nBooting up..."));
}

// Load instruction decoding helper table
unsigned char bios_table_lookup(unsigned int i, unsigned int j)
{
    return readregs8(readregs16(0x81 + i) + j);
}

#ifdef HARDDISK
unsigned short hdsize()
{
    // Get the hard disk size
    file.open(DISK_FILE, O_READ|O_WRITE); // Open hard disk file
    return (unsigned short)file.fileSize();
    file.close();
}
#endif

unsigned char readmem(unsigned short addr)
{
    // Read the memory at the addr and return it
    file.open(RAM_FILE, O_READ); // Open virtual ram file in read mode
    file.seekSet(addr); // Seek to addr
    unsigned char val = (unsigned char)file.read(); // Read
    file.close(); // Close the file
    return val; // Return that value
}

unsigned char writemem(unsigned short addr, unsigned char value)
{
    // Write the value to memory at the addr
    file.open(RAM_FILE, O_WRITE); // Open virtual ram file in write mode
    file.seekSet(addr); // Seek to addr
    file.write(value, 1); // Write the value to file
    file.close();
    return value;
}

unsigned char readregs8(unsigned short addr)
{
    // Read regs at addr
    return readmem(REGS_BASE + addr);
}

unsigned char writeregs8(unsigned short addr, unsigned char value)
{
    // Write regs at addr
    return writemem(REGS_BASE + addr, value);
}

unsigned short readregs16(unsigned short addr)
{
    return ((unsigned short)readregs8(addr) << 8) | readregs8(addr + 1); // Read 2 unsigned char and convert to 1 unsigned short
}

unsigned short writeregs16(unsigned short addr, unsigned short value)
{
    // Convert a unsigned short to 2 unsigned char and write
    writeregs8(addr, value & 0xff);
    writeregs8(addr+1, (value >> 8));
    return value;
}

bool seekandcheck(int disknum, unsigned short addr)
{
    // Seek to the location in disk and check if it exist
    file.open(DISK_FILE, O_READ);
    bool exist = file.seekSet(addr);
    file.close();
    return exist;
}

int disk(bool op, int disknum, unsigned short offset, unsigned short b)
{
    unsigned short i;
    
    if(op)
    {
        // Write to disk
        for(i = 0; i <= b;){
            file.open(RAM_FILE, O_READ);
            file.seekSet(offset + i);
            int br = file.read(membuffer, BUFFER);
            file.close();

            file.open(DISK_FILE, O_WRITE);
            file.seekSet(((unsigned)readregs16(REG_BP) << 9) + i);
            file.write(membuffer, br);
            file.close();
            
            if (br < BUFFER)
                return i + br;
            else
                i += br;
        }
    }

    else
    {
        // Read from disk
        for(i = 0; i <= b;){
            file.open(DISK_FILE, O_READ);
            file.seekSet(((unsigned)readregs16(REG_BP) << 9) + i);
            int br = file.read(membuffer, BUFFER);
            file.close();

            file.open(RAM_FILE, O_WRITE);
            file.seekSet(offset + i);
            file.write(membuffer, br);
            file.close();
            
            if (br < BUFFER)
                return i + br;
            else
                i += br;
        }
    }

    return i;
}
