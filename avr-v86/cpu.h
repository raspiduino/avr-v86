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

#include "config.h"
#include "sd.h"

void set_opcode(unsigned char opcode);
unsigned char r_m_push(unsigned short a);
unsigned char r_m_pop(unsigned short a);
char set_AF_OF_arith();
char set_CF(int new_CF);
char set_AF(int new_AF);
char set_OF(int new_OF);
unsigned short segreg(int reg_seg, int reg_ofs, int op);
void make_flags();
char pc_interrupt(unsigned char interrupt_num);
void set_flags(int new_flags);
int AAA_AAS(char which_operation);
void v86();
