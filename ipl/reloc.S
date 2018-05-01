/*
* Copyright (c) 2018 naehrwert
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
* TODO: the placement of the relocator is a bit fragile atm, maybe we 
*       should include it in start.S and copy it to some known good 
*       place in IRAM instead. Basically we want it as far back atm 
*       as it might be overwritten during relocation.
*/

.section .text.reloc
.arm

.globl _reloc_ipl
.type _reloc_ipl, %function
_reloc_ipl:
	LDMIA R0!, {R4-R7}
	STMIA R1!, {R4-R7}
	SUBS R2, #0x10
	BNE _reloc_ipl
	BX R3
