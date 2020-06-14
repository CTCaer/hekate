/*
 * Copyright (c) 2019 CTCaer
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
 * Armv7tdmi Status register.
 *
 * bit0: Mode 0.
 * bit1: Mode 1.
 * bit2: Mode 2.
 * bit3: Mode 3.
 * bit4: Mode 4.
 * bit5: Thumb state.
 * bit6: FIQ disable.
 * bit7: IRQ disable.
 * bit8-27: Reserved.
 * bit28: Overflow condition.
 * bit29: Carry/Borrow/Extend condition.
 * bit30: Zero condition.
 * bit31: Negative/Less than condition.
 *
 * M[4:0] | Mode | Visible Thumb-state registers             | Visible ARM-state registers
 * 10000  | USER | r0–r7, SP,     LR,     PC, CPSR           | r0–r14,                   PC, CPSR
 * 10001  | FIQ  | r0–r7, SP_fiq, LR_fiq, PC, CPSR, SPSR_fiq | r0–r7,  r8_fiq–r14_fiq,   PC, CPSR, SPSR_fiq
 * 10010  | IRQ  | r0–r7, SP_irq, LR_irq, PC, CPSR, SPSR_irq | r0–r12, r13_irq, r14_irq, PC, CPSR, SPSR_irq
 * 10011  | SVC  | r0–r7, SP_svc, LR_svc, PC, CPSR, SPSR_svc | r0–r12, r13_svc, r14_svc, PC, CPSR, SPSR_svc
 * 10111  | ABRT | r0–r7, SP_abt, LR_abt, PC, CPSR, SPSR_abt | r0–r12, r13_abt, r14_abt, PC, CPSR, SPSR_abt
 * 11011  | UNDF | r0–r7, SP_und, LR_und, PC, CPSR, SPSR_und | r0–r12, r13_und, r14_und, PC, CPSR, SPSR_und
 * 11111  | SYS  | r0–r7, SP,     LR,     PC, CPSR           | r0–r14,                   PC, CPSR
 */

#define EXCP_EN_ADDR   0x4003FFFC
#define EXCP_TYPE_ADDR 0x4003FFF8
#define EXCP_LR_ADDR   0x4003FFF4

#define EXCP_VEC_BASE  0x6000F000
#define EVP_COP_RESET_VECTOR          0x200
#define EVP_COP_UNDEF_VECTOR          0x204
#define EVP_COP_SWI_VECTOR            0x208
#define EVP_COP_PREFETCH_ABORT_VECTOR 0x20C
#define EVP_COP_DATA_ABORT_VECTOR     0x210
#define EVP_COP_RSVD_VECTOR           0x214
#define EVP_COP_IRQ_VECTOR            0x218
#define EVP_COP_FIQ_VECTOR            0x21C

#define MODE_USR  0x10
#define MODE_FIQ  0x11
#define MODE_IRQ  0x12
#define MODE_SVC  0x13
#define MODE_ABT  0x17
#define MODE_UDF  0x1B
#define MODE_SYS  0x1F
#define MODE_MASK 0x1F

#define FIQ 0x40
#define IRQ 0x80

.section .text._irq_setup
.arm

.extern ipl_main
.type ipl_main, %function

.extern svc_handler
.type svc_handler, %function

.extern irq_handler
.type irq_handler, %function

.extern fiq_setup
.type fiq_setup, %function

.extern fiq_handler
.type fiq_handler, %function

.globl _irq_setup
.type _irq_setup, %function
_irq_setup:
	MRS R0, CPSR
	BIC R0, R0, #MODE_MASK              /* Clear mode bits */
	ORR R0, R0, #(MODE_SVC | IRQ | FIQ) /* SUPERVISOR mode, IRQ/FIQ disabled */
	MSR CPSR, R0

	/* Setup IRQ stack pointer */
	MSR CPSR, #(MODE_IRQ | IRQ | FIQ) /* IRQ mode, IRQ/FIQ disabled */
	LDR SP, =0x40040000

	/* Setup SYS stack pointer */
	MSR CPSR, #(MODE_SYS | IRQ | FIQ) /* SYSTEM mode, IRQ/FIQ disabled */
	LDR SP, =0x4003FF00               /* Will be changed later to DRAM */

	MOV LR, PC
	BL setup_vectors
	/*BL fiq_setup*/

	/* Enable interrupts */
	BL irq_enable_cpu_irq_exceptions

	B ipl_main
	B .

_reset:
	LDR R0, =EXCP_EN_ADDR
	LDR R1, =0x30505645 /* EVP0 */
	STR R1, [R0]        /* EVP0 in EXCP_EN_ADDR */
	LDR R0, =EXCP_LR_ADDR
	MOV R1, LR
	STR R1, [R0]        /* Save LR in EXCP_LR_ADDR */
	LDR R0, =__bss_start
	EOR R1, R1, R1
	LDR R2, =__bss_end
	SUB R2, R2, R0
	BL memset
	B _irq_setup

_reset_handler:
	LDR R0, =EXCP_TYPE_ADDR
	LDR R1, =0x545352   /* RST */
	STR R1, [R0]        /* RST in EXCP_TYPE_ADDR */
	B _reset

_undefined_handler:
	LDR R0, =EXCP_TYPE_ADDR
	LDR R1, =0x464455   /* UDF */
	STR R1, [R0]        /* UDF in EXCP_TYPE_ADDR */
	B _reset

_prefetch_abort_handler:
	LDR R0, =EXCP_TYPE_ADDR
	LDR R1, =0x54424150 /* PABT */
	STR R1, [R0]        /* PABT in EXCP_TYPE_ADDR */
	B _reset

_data_abort_handler:
	LDR R0, =EXCP_TYPE_ADDR
	LDR R1, =0x54424144 /* DABT */
	STR R1, [R0]        /* DABT in EXCP_TYPE_ADDR */
	B _reset

.globl irq_enable_cpu_irq_exceptions
.type irq_enable_cpu_irq_exceptions, %function
irq_enable_cpu_irq_exceptions:
	MRS R12, CPSR
	BIC R12, R12, #(IRQ | FIQ) /* IRQ/FIQ enabled */
	MSR CPSR, R12
	BX LR

.globl irq_disable_cpu_irq_exceptions
.type irq_disable_cpu_irq_exceptions, %function
irq_disable_cpu_irq_exceptions:
	MRS R12, CPSR
	ORR R12, R12, #(IRQ | FIQ) /* IRQ/FIQ disabled */
	MSR CPSR, R12
	BX LR

_irq_handler:
	MOV R13, R0    /* Save R0 in R13_IRQ */
	SUB R0, LR, #4 /* Put return address in R0_SYS */
	MOV LR, R1     /* Save R1 in R14_IRQ (LR) */
	MRS R1, SPSR   /* Put the SPSR in R1_SYS */

	MSR CPSR_c, #(MODE_SYS | IRQ)  /* SYSTEM mode, IRQ disabled */
	STMFD SP!, {R0, R1}            /* SPSR and PC */
	STMFD SP!, {R2-R3, R12, LR}    /* AAPCS-clobbered registers */
	MOV R0, SP                     /* Make SP_SYS visible to IRQ mode */
	SUB SP, SP, #8                 /* Make room for stacking R0 and R1 */

	MSR CPSR_c, #(MODE_IRQ | IRQ) /* IRQ mode, IRQ disabled */
	STMFD R0!, {R13, R14}         /* Finish saving the context (R0, R1) */

	MSR CPSR_c, #(MODE_SYS | IRQ) /* SYSTEM mode, IRQ disabled */
	LDR R12, =irq_handler
	MOV LR, PC                    /* Copy the return address to link register */
	BX R12                        /* Call the C IRQ handler (ARM/THUMB) */

	MSR CPSR_c, #(MODE_SYS | IRQ | FIQ) /* SYSTEM mode, IRQ/FIQ disabled */
	MOV R0, SP                          /* Make SP_SYS visible to IRQ mode */
	ADD SP, SP, #32                     /* Fake unstacking 8 registers from SP_SYS */

	MSR CPSR_c, #(MODE_IRQ | IRQ | FIQ) /* IRQ mode, IRQ/FIQ disabled */
	MOV SP, R0                          /* Copy SP_SYS to SP_IRQ */
	LDR R0, [SP, #28]                   /* Load the saved SPSR from the stack */
	MSR SPSR_cxsf, R0                   /* Copy it into SPSR_IRQ */

	LDMFD SP, {R0-R3, R12, LR}^ /* Unstack all saved USER/SYSTEM registers */
	NOP                         /* Cant access barked registers immediately */
	LDR LR, [SP, #24]           /* Load return address from the SYS stack */
	MOVS PC, LR                 /* Return restoring CPSR from SPSR */

_fiq_handler:
	BL fiq_handler

setup_vectors:
	/* Setup vectors */
	LDR R0, =EXCP_VEC_BASE

	LDR R1, =_reset_handler
	STR R1, [R0, #EVP_COP_RESET_VECTOR]

	LDR R1, =_undefined_handler
	STR R1, [R0, #EVP_COP_UNDEF_VECTOR]

	LDR R1, =_reset_handler
	STR R1, [R0, #EVP_COP_SWI_VECTOR]

	LDR R1, =_prefetch_abort_handler
	STR R1, [R0, #EVP_COP_PREFETCH_ABORT_VECTOR]

	LDR R1, =_data_abort_handler
	STR R1, [R0, #EVP_COP_DATA_ABORT_VECTOR]

	LDR R1, =_reset_handler
	STR R1, [R0, #EVP_COP_RSVD_VECTOR]

	LDR R1, =_irq_handler
	STR R1, [R0, #EVP_COP_IRQ_VECTOR]

	LDR R1, =_fiq_handler
	STR R1, [R0, #EVP_COP_FIQ_VECTOR]

	BX LR
