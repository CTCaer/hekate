/*
 * Copyright (c) 2018 M4xw
 * Copyright (c) 2018-2026 CTCaer
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

#include <string.h>

#include "ianos.h"
#include "elfload/elfload.h"
#include <module.h>
#include <mem/heap.h>
#include <storage/sd.h>
#include <utils/types.h>

#include <gfx_utils.h>

extern heap_t _heap;

static bdk_params_t _bdk_params = {
	.gfx_con = (void *)&gfx_con,
	.gfx_ctx = (void *)&gfx_ctxt,
	.heap    = &_heap,
	.memcpy  = (memcpy_t)&memcpy,
	.memset  = (memset_t)&memset,

	.extension_magic = 0
};

static void *_ianos_alloc_cb(el_ctx *ctx, Elf_Addr phys, Elf_Addr virt, Elf_Addr size)
{
	return (void *)virt;
}

static el_status _ianos_read_cb(el_ctx *ctx, void *dest, size_t nb, size_t offset)
{
	memcpy(dest, (void *)(ctx->eaddr + offset), nb);

	return EL_OK;
}

//TODO: Support shared libraries.
int ianos_loader(ianos_lib_t *lib, char *path)
{
	el_ctx ctx;
	lib->buf = NULL;
	if (!lib->bdk)
		lib->bdk = &_bdk_params;

	// Read library.
	ctx.eaddr = (Elf_Addr)sd_file_read(path, NULL);
	if (!ctx.eaddr)
		goto error;

	ctx.pread = _ianos_read_cb;

	if (el_init(&ctx))
		goto error;

	if (lib->type & IA_SHARED_LIB)
		goto error; // No support for shared libs now.

	// Set our relocated library's buffer.
	switch (lib->type & ~IA_SHARED_LIB)
	{
	case IA_DRAM_LIB:
		lib->buf = malloc(ctx.memsz); // Aligned to 0x10 by default.
		break;

	case IA_IRAM_LIB:
		break;

	case IA_AUTO_LIB: // Default to DRAM for now.
	default:
		lib->buf = malloc(ctx.memsz); // Aligned to 0x10 by default.
		break;
	}

	if (!lib->buf)
		goto error;

	// Load and relocate library.
	ctx.base_load_vaddr = ctx.base_load_paddr = (Elf_Addr)lib->buf;
	if (el_load(&ctx, _ianos_alloc_cb))
		goto error;

	if (el_relocate(&ctx))
		goto error;

	free((void *)ctx.eaddr);

	// Launch.
	Elf_Addr epaddr = ctx.ehdr.e_entry + (Elf_Addr)lib->buf;
	moduleEntrypoint ep = (moduleEntrypoint)epaddr;
	ep(lib->private, lib->bdk);

	return 0;

error:
	free((void *)ctx.eaddr);
	free(lib->buf);

	return 1;
}

uintptr_t ianos_static_module(char *path, void *private)
{
	el_ctx ctx;
	Elf_Addr buf = 0;
	Elf_Addr epaddr = 0;

	// Read library.
	ctx.eaddr = (Elf_Addr)sd_file_read(path, NULL);
	if (!ctx.eaddr)
		goto error;

	ctx.pread = _ianos_read_cb;

	// Initialize elfload context.
	if (el_init(&ctx))
		goto error;

	// Set our relocated library's buffer.
	buf = (Elf_Addr)malloc(ctx.memsz); // Aligned to 0x10 by default.
	if (!buf)
		goto error;

	// Load and relocate library.
	ctx.base_load_vaddr = ctx.base_load_paddr = buf;
	if (el_load(&ctx, _ianos_alloc_cb))
		goto error;

	if (el_relocate(&ctx))
		goto error;

	free((void *)ctx.eaddr);

	// Launch.
	epaddr = ctx.ehdr.e_entry + buf;
	moduleEntrypoint ep = (moduleEntrypoint)epaddr;
	ep(private, &_bdk_params);

	return (uintptr_t)epaddr;

error:
	free((void *)ctx.eaddr);
	free((void *)buf);

	return 0;
}
