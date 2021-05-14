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

/*
    INTRODUCE:
    This is a quick and dirty Intel 8086 emulator which is designed to be:
    - As smallest as possible
    - As much portable as possible (Easy to port to other platforms, especially embed
      platforms like AVR, PIC,...)
    - As much functional as possible (Full 8086 based computer including monitor, mouse,
      speaker, ...) while also meet the first condition.
    This is just a hobby project by me (Raspiduino, at https://github.com/raspiduino)
    The original purpose of this program is to be ported to Arduino (AVR) (Arduino UNO
    atmega328(p)) platform, so we can run x86 OSes on our Arduino!
    --------------------------------------------------------------------------------------
    WHAT DOES THIS EMULATOR INCLUDE?
    - Intel 8086 CPU emulator (opcode)
    - A simple monitor (VGA text mode in this version)
    - A mouse (Future version)
    - A keyboard
    - A speaker
    - A floppy disk and a hard disk
    --------------------------------------------------------------------------------------
    HOW TO PORT?
    For each of the input/output to outside the emulator, we will call different functions
    to handle that. For example, you can redirect the monitor to the MCU's serial port in
    text mode. The full instruction will be included in each of these functions.
    --------------------------------------------------------------------------------------
    CREDITS
    Super thanks to:
    - Adrian Cable (https://github.com/adriancable) and 
    8086tiny (https://github.com/adriancable/8086tiny) contributers

    - Bill Greiman (https://github.com/greiman) and
    Fat16 (https://github.com/greiman/Fat16) contributers

    avr-v86 is mostly based on 8086tiny project!
*/

#include "cpu.h"
#include "sd.h"

void setup() {
    Serial.begin(9600); // Start the serial at the default baudrate 9600bps
    Serial.println(F("avr-v86 - Intel 8086 emulator running on Arduino UNO"));
    Serial.println(F("Copyright (C) 2021 @raspiduino, under GPL-v3"));

    // Init SD card
    sdinit();
    
    // Start the emulator
    Serial.println(F("Starting v86..."));
    v86();

    // If the emulator quit
    Serial.println(F("Machine halted!")); // This probably never happend!
}

void loop() {
    // Do nothing :)))
}
