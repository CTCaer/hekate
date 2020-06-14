/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <m4x@m4xw.net> wrote this file. As long as you retain this notice you can do
 * whatever you want with this stuff. If we meet some day, and you think this
 * stuff is worth it, you can buy me a beer in return.                     M4xw
 * ----------------------------------------------------------------------------
 */

#include "elfload.h"

#if defined(__arm__)

// Taken from http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044f/IHI0044F_aaelf.pdf
#define R_ARM_NONE 0
#define R_ARM_ABS32 2
#define R_ARM_JUMP_SLOT 22
#define R_ARM_GLOB_DAT 21
#define R_ARM_RELATIVE 23

el_status el_applyrel(el_ctx *ctx, Elf_Rel *rel)
{
	uint32_t sym = ELF_R_SYM(rel->r_info);                              // Symbol offset
	uint32_t type = ELF_R_TYPE(rel->r_info);                            // Relocation Type
	uintptr_t *p = (uintptr_t *)(rel->r_offset + ctx->base_load_paddr); // Target Addr

#if 0 // For later symbol usage
	Elf32_Sym *elfSym;
	const char *symbolName;

	// We resolve relocs from the originating elf-image
	elfSym = (Elf32_Sym *)(ctx->symtab.sh_offset + (char *)buffteg) + sym;
	int strtab_offset = ctx->shstr.sh_offset;
	char *strtab = (char *)buffteg + strtab_offset;
	symbolName = strtab + elfSym->st_name;
	//EL_DEBUG("Str: %s sz: %x val: %x\n", symbolName, elfSym->st_size, elfSym->st_value);
#endif

	switch (type)
	{
	case R_ARM_NONE:
		EL_DEBUG("R_ARM_NONE\n");
		break;
	case R_ARM_JUMP_SLOT:
	case R_ARM_ABS32:
	case R_ARM_GLOB_DAT:
		// Stubbed for later purpose
		//*p += elfSym->st_value; // + vaddr from sec
		//*p |= 0; // 1 if Thumb && STT_FUNC, ignored for now
		break;
	case R_ARM_RELATIVE: // Needed for PIE
		if (sym)
		{
			return EL_BADREL;
		}
		*p += ctx->base_load_vaddr;
		break;

	default:
		return EL_BADREL;
	}

	return EL_OK;
}

#endif
