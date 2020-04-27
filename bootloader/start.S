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

.section .text._start
.arm

.extern _reloc_ipl
.type _reloc_ipl, %function

.extern memset
.type memset, %function

.extern _irq_setup
.type _irq_setup, %function

.globl _start
.type _start, %function
_start:
	ADR R0, _start
	LDR R1, =__ipl_start
	CMP R0, R1
	BEQ _real_start

	/* If we are not in the right location already, copy a relocator to upper IRAM. */
	ADR R2, _reloc_ipl
	LDR R3, =0x4003FF00
	MOV R4, #(_real_start - _reloc_ipl)
_copy_loop:
	LDMIA R2!, {R5}
	STMIA R3!, {R5}
	SUBS R4, #4
	BNE _copy_loop

	/* Use the relocator to copy ourselves into the right place. */
	LDR R2, =__ipl_end
	SUB R2, R2, R1
	LDR R3, =_real_start
	LDR R4, =0x4003FF00
	BX R4

_reloc_ipl:
	LDMIA R0!, {R4-R7}
	STMIA R1!, {R4-R7}
	SUBS R2, #0x10
	BNE _reloc_ipl
	/* Jump to the relocated entry. */
	BX R3

_real_start:
	/* Initially, we place our stack in IRAM but will move it to SDRAM later. */
	LDR SP, =0x4003FF00
	LDR R0, =__bss_start
	EOR R1, R1, R1
	LDR R2, =__bss_end
	SUB R2, R2, R0
	BL memset
	BL _irq_setup
	B .

.globl pivot_stack
.type pivot_stack, %function
pivot_stack:
	MOV SP, R0
	BX LR
