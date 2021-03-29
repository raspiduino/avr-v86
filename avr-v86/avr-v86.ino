/*
    avr-v86 - i8086 emulator running on Arduino UNO
    Copyright (C) 2021  @raspiduino

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
#include "ram.h"

void setup() {
    Serial.begin(9600); // Start the serial at the default baudrate 9600bps
    Serial.println(F("avr-v86 - i8086 emulator running on Arduino UNO"));
    Serial.println(F("Copyright (C) 2021 @raspiduino, under GPL-v3"));

    
}

void loop() {
    // Do nothing :)))
}
