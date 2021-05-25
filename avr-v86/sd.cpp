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
File file;

//char filepath[12]; // Virtual disk filepath
//char membuffer[BUFFER] = ""; // Memory buffer

void sdinit()
{
    /* Init the SD card */
    Serial.print(F("Init SD card"));
    while(!SD.begin(CHIP_SELECT)) Serial.print("."); // Init SD card
    Serial.println(F(". Done!"));
}

//void fillmem()
//{
//    Serial.print(F("Filling memory"));
//
//    if(!file.open(RAM_FILE, O_WRITE|O_CREAT|O_EXCL))
//        file.remove();
//
//    file.close();
//
//    file.open(RAM_FILE, O_WRITE|O_CREAT);
//    file.seekSet(0);
//    
//    while(1)
//    {
//        if(!file.write((byte)0))
//            Serial.println(F("Error!")); // Fill with zero
//        Serial.print(F("."));
//
//        if(file.curPosition() == RAM_SIZE)
//            break;
//    }
//
//    file.close();
//    Serial.println(F(" OK!"));
//    Serial.println(F("Done!"));
//}
//
//void loadbios()
//{
//    // Load BIOS image into F000:0100 (0x100 of regs8)
//    // Only need to read 0x1DF0 bytes from BIOS image
//
//    fillmem(); // Fill mem
//    
//    Serial.print(F("Loading BIOS..."));
//    
//    for(long i = 0; i <= RAM_SIZE;)
//    {
//        file.open(BIOS_FILE, O_READ); // Open BIOS file in read mode
//        file.seekSet(i);
//        int br = file.read(membuffer, BUFFER); // Read to buffer
//        file.close(); // Close that file
//
//        // Load init ram to virtual RAM
//        file.open(RAM_FILE, O_WRITE); // Open ram file in write mode
//        file.seekSet(REGS_BASE + 0x100 + i); // Set the cusor to the register mapped location + the byte read
//        file.write(membuffer, br); // Write to virtual RAM
//        file.close(); // Close that file
//
//        if (br < BUFFER)
//            break;
//        else
//            i += br;
//
//        Serial.print(F("."));
//    }
//    
//    Serial.println();
//    Serial.println(F("Booting up..."));
//}

#ifdef HARDDISK
unsigned short hdsize()
{
    // Get the hard disk size
    file = SD.open(DISK_FILE, FILE_READ); // Open hard disk file
    return (unsigned short)file.size();
    file.close();
}
#endif

unsigned char readmem(unsigned short addr)
{
    // Read the memory at the addr and return it
    file = SD.open(RAM_FILE, FILE_READ); // Open virtual ram file in read mode
    if(!file){
        Serial.println(F("Error reading RAM!"));
        while(1);
    }
    file.seek(addr); // Seek to addr
    unsigned char val;
    val = file.read(); // Read
    file.close(); // Close the file
    return val; // Return that value
}

unsigned char writemem(unsigned short addr, unsigned char value)
{
    // Write the value to memory at the addr
    file = SD.open(RAM_FILE, FILE_READ|FILE_WRITE); // Open virtual ram file in write mode
    if(!file){
        Serial.println(F("Error reading RAM!"));
        while(1);
    }
    file.seek(addr); // Seek to addr
    file.write(value); // Write the value to file
    file.close();
    return value;
}

bool seekandcheck(int disknum, unsigned short addr)
{
    // Seek to the location in disk and check if it exist
    file = SD.open(DISK_FILE, FILE_READ);
    bool exist = file.seek(addr);
    file.close();
    return exist;
}
