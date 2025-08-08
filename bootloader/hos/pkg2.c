/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <bdk.h>

#include "hos.h"
#include "pkg2.h"
#include "pkg2_ini_kippatch.h"

#include "../config.h"
#include <libs/compr/blz.h>
#include <libs/fatfs/ff.h>
#include "../storage/emummc.h"

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

extern const u8 package2_keyseed[];

u32 pkg2_newkern_ini1_info;
u32 pkg2_newkern_ini1_start;
u32 pkg2_newkern_ini1_end;
u32 pkg2_newkern_ini1_rela;

#define KIP_PATCH_SECTION_SHIFT  (29)
#define KIP_PATCH_SECTION_MASK   (7 << KIP_PATCH_SECTION_SHIFT)
#define KIP_PATCH_OFFSET_MASK    (~KIP_PATCH_SECTION_MASK)
#define GET_KIP_PATCH_SECTION(x) (((x) >> KIP_PATCH_SECTION_SHIFT) & 7)
#define GET_KIP_PATCH_OFFSET(x)  ((x) & KIP_PATCH_OFFSET_MASK)
#define KPS(x)                   ((u32)(x) << KIP_PATCH_SECTION_SHIFT)

#include "pkg2_patches.inl"

static kip1_id_t *_kip_id_sets = (kip1_id_t *)_kip_ids;
static u32 _kip_id_sets_cnt = ARRAY_SIZE(_kip_ids);

void pkg2_get_ids(kip1_id_t **ids, u32 *entries)
{
	*ids = _kip_id_sets;
	*entries = _kip_id_sets_cnt;
}

static void parse_external_kip_patches()
{
	static bool ext_patches_parsed = false;

	if (ext_patches_parsed)
		return;

	LIST_INIT(ini_kip_sections);
	if (ini_patch_parse(&ini_kip_sections, "bootloader/patches.ini"))
	{
		// Copy ids into a new patchset.
		_kip_id_sets = zalloc(sizeof(kip1_id_t) * 256); // Max 256 kip ids.
		memcpy(_kip_id_sets, _kip_ids, sizeof(_kip_ids));

		// Parse patchsets and glue them together.
		LIST_FOREACH_ENTRY(ini_kip_sec_t, ini_psec, &ini_kip_sections, link)
		{
			kip1_id_t *kip = NULL;
			bool found = false;
			for (u32 kip_idx = 0; kip_idx < _kip_id_sets_cnt + 1; kip_idx++)
			{
				kip = &_kip_id_sets[kip_idx];

				// Check if reached the end of predefined list.
				if (!kip->name)
					break;

				// Check if name and hash match.
				if (!strcmp(kip->name, ini_psec->name) && !memcmp(kip->hash, ini_psec->hash, 8))
				{
					found = true;
					break;
				}
			}

			if (!kip)
				continue;

			// If not found, create a new empty entry.
			if (!found)
			{
				kip->name = ini_psec->name;
				memcpy(kip->hash, ini_psec->hash, 8);
				kip->patchset = zalloc(sizeof(kip1_patchset_t));

				_kip_id_sets_cnt++;
			}

			kip1_patchset_t *patchsets = (kip1_patchset_t *)zalloc(sizeof(kip1_patchset_t) * 16); // Max 16 patchsets per kip.

			u32 patchset_idx;
			for (patchset_idx = 0; kip->patchset[patchset_idx].name != NULL; patchset_idx++)
			{
				patchsets[patchset_idx].name    = kip->patchset[patchset_idx].name;
				patchsets[patchset_idx].patches = kip->patchset[patchset_idx].patches;
			}

			kip->patchset = patchsets;
			bool first_ext_patch = true;
			u32 patch_idx = 0;

			// Parse patches and glue them together to a patchset.
			kip1_patch_t *patches = zalloc(sizeof(kip1_patch_t) * 32); // Max 32 patches per set.
			LIST_FOREACH_ENTRY(ini_patchset_t, pt, &ini_psec->pts, link)
			{
				if (first_ext_patch)
				{
					first_ext_patch = false;
					patchsets[patchset_idx].name    = pt->name;
					patchsets[patchset_idx].patches = patches;
				}
				else if (strcmp(pt->name, patchsets[patchset_idx].name))
				{
					// New patchset name found, create a new set.
					patchset_idx++;
					patch_idx = 0;
					patches = zalloc(sizeof(kip1_patch_t) * 32); // Max 32 patches per set.

					patchsets[patchset_idx].name    = pt->name;
					patchsets[patchset_idx].patches = patches;
				}

				if (pt->length)
				{
					patches[patch_idx].offset = pt->offset;
					patches[patch_idx].length = pt->length;

					patches[patch_idx].src_data = (char *)pt->src_data;
					patches[patch_idx].dst_data = (char *)pt->dst_data;
				}
				else
					patches[patch_idx].src_data = malloc(1); // Empty patches check. Keep everything else as 0.

				patch_idx++;
			}
			patchset_idx++;
			patchsets[patchset_idx].name = NULL;
			patchsets[patchset_idx].patches = NULL;
		}
	}

	ext_patches_parsed = true;
}

const pkg2_kernel_id_t *pkg2_identify(const u8 *hash)
{
	for (u32 i = 0; i < ARRAY_SIZE(_pkg2_kernel_ids); i++)
	{
		if (!memcmp(hash, _pkg2_kernel_ids[i].hash, sizeof(_pkg2_kernel_ids[0].hash)))
			return &_pkg2_kernel_ids[i];
	}
	return NULL;
}

static u32 _pkg2_calc_kip1_size(pkg2_kip1_t *kip1)
{
	u32 size = sizeof(pkg2_kip1_t);
	for (u32 j = 0; j < KIP1_NUM_SECTIONS; j++)
		size += kip1->sections[j].size_comp;
	return size;
}

static void _pkg2_get_newkern_info(u8 *kern_data)
{
	u32 crt_start  = 0;
	pkg2_newkern_ini1_info  = 0;
	pkg2_newkern_ini1_start = 0;
	pkg2_newkern_ini1_rela  = 0;

	u32 first_op = *(u32 *)kern_data;
	if ((first_op & 0xFE000000) == 0x14000000)
		crt_start = (first_op & 0x1FFFFFF) << 2;

	// Find static OP offset that is close to INI1 offset.
	u32 counter_ops = 0x100;
	while (counter_ops)
	{
		if (*(u32 *)(kern_data + crt_start + 0x100 - counter_ops) == PKG2_NEWKERN_GET_INI1_HEURISTIC)
		{
			// OP found. Add 12 for the INI1 info offset.
			pkg2_newkern_ini1_info = crt_start + 0x100 - counter_ops + 12;

			// On v2 kernel with dynamic crt there's a NOP after heuristic. Offset one op.
			if (crt_start)
				pkg2_newkern_ini1_info += 4;
			break;
		}

		counter_ops -= 4;
	}

	// Offset not found?
	if (!counter_ops)
		return;

	u32 info_op = *(u32 *)(kern_data + pkg2_newkern_ini1_info);
	pkg2_newkern_ini1_info += ((info_op & 0xFFFF) >> 3); // Parse ADR and PC.

	pkg2_newkern_ini1_start = *(u32 *)(kern_data + pkg2_newkern_ini1_info);
	pkg2_newkern_ini1_end   = *(u32 *)(kern_data + pkg2_newkern_ini1_info + 0x8);

	// On v2 kernel with dynamic crt, values are relative to value address.
	if (crt_start)
	{
		pkg2_newkern_ini1_rela   = pkg2_newkern_ini1_info;
		pkg2_newkern_ini1_start += pkg2_newkern_ini1_info;
		pkg2_newkern_ini1_end   += pkg2_newkern_ini1_info + 0x8;
	}
}

bool pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2)
{
	u8 *ptr;
	// Check for new pkg2 type.
	if (!pkg2->sec_size[PKG2_SEC_INI1])
	{
		_pkg2_get_newkern_info(pkg2->data);

		if (!pkg2_newkern_ini1_start)
			return false;

		ptr = pkg2->data + pkg2_newkern_ini1_start;
		*new_pkg2 = true;
	}
	else
		ptr = pkg2->data + pkg2->sec_size[PKG2_SEC_KERNEL];

	pkg2_ini1_t *ini1 = (pkg2_ini1_t *)ptr;
	ptr += sizeof(pkg2_ini1_t);

	for (u32 i = 0; i < ini1->num_procs; i++)
	{
		pkg2_kip1_t *kip1 = (pkg2_kip1_t *)ptr;
		pkg2_kip1_info_t *ki = (pkg2_kip1_info_t *)malloc(sizeof(pkg2_kip1_info_t));
		ki->kip1 = kip1;
		ki->size = _pkg2_calc_kip1_size(kip1);
		list_append(info, &ki->link);
		ptr += ki->size;
DPRINTF(" kip1 %d:%s @ %08X (%08X)\n", i, kip1->name, (u32)kip1, ki->size);
	}

	return true;
}

int pkg2_has_kip(link_t *info, u64 tid)
{
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
		if (ki->kip1->tid == tid)
			return 1;
	return 0;
}

void pkg2_replace_kip(link_t *info, u64 tid, pkg2_kip1_t *kip1)
{
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
	{
		if (ki->kip1->tid == tid)
		{
			ki->kip1 = kip1;
			ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("replaced kip %s (new size %08X)\n", kip1->name, ki->size);
			return;
		}
	}
}

void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1)
{
	pkg2_kip1_info_t *ki = (pkg2_kip1_info_t *)malloc(sizeof(pkg2_kip1_info_t));
	ki->kip1 = kip1;
	ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("added kip %s (size %08X)\n", kip1->name, ki->size);
	list_append(info, &ki->link);
}

void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1)
{
	if (pkg2_has_kip(info, kip1->tid))
		pkg2_replace_kip(info, kip1->tid, kip1);
	else
		pkg2_add_kip(info, kip1);
}

static int _decompress_kip(pkg2_kip1_info_t *ki, u32 sectsToDecomp)
{
	u32 compClearMask = ~sectsToDecomp;
	if ((ki->kip1->flags & compClearMask) == ki->kip1->flags)
		return 0; // Already decompressed, nothing to do.

	pkg2_kip1_t hdr;
	memcpy(&hdr, ki->kip1, sizeof(hdr));

	u32 new_kip_size = sizeof(hdr);
	for (u32 sect_idx = 0; sect_idx < KIP1_NUM_SECTIONS; sect_idx++)
	{
		u32 comp_bit_mask = BIT(sect_idx);
		// For compressed, cant get actual decompressed size without doing it, so use safe "output size".
		if (sect_idx < 3 && (sectsToDecomp & comp_bit_mask) && (hdr.flags & comp_bit_mask))
			new_kip_size += hdr.sections[sect_idx].size_decomp;
		else
			new_kip_size += hdr.sections[sect_idx].size_comp;
	}

	pkg2_kip1_t *new_kip = malloc(new_kip_size);
	u8 *dst_data = new_kip->data;
	const u8 *src_data = ki->kip1->data;
	for (u32 sect_idx = 0; sect_idx < KIP1_NUM_SECTIONS; sect_idx++)
	{
		u32 comp_bit_mask = BIT(sect_idx);
		// Easy copy path for uncompressed or ones we dont want to uncompress.
		if (sect_idx >= 3 || !(sectsToDecomp & comp_bit_mask) || !(hdr.flags & comp_bit_mask))
		{
			u32 dataSize = hdr.sections[sect_idx].size_comp;
			if (dataSize == 0)
				continue;

			memcpy(dst_data, src_data, dataSize);
			src_data += dataSize;
			dst_data += dataSize;
			continue;
		}

		u32 comp_size = hdr.sections[sect_idx].size_comp;
		u32 output_size = hdr.sections[sect_idx].size_decomp;
		gfx_printf("Decomping '%s', sect %d, size %d..\n", (char *)hdr.name, sect_idx, comp_size);
		if (blz_uncompress_srcdest(src_data, comp_size, dst_data, output_size) == 0)
		{
			gfx_con.mute = false;
			gfx_printf("%kERROR decomping sect %d of '%s'!%k\n", TXT_CLR_ERROR, sect_idx, (char *)hdr.name, TXT_CLR_DEFAULT);
			free(new_kip);

			return 1;
		}
		else
		{
			DPRINTF("Done! Decompressed size is %d!\n", output_size);
		}
		hdr.sections[sect_idx].size_comp = output_size;
		src_data += comp_size;
		dst_data += output_size;
	}

	hdr.flags &= compClearMask;
	memcpy(new_kip, &hdr, sizeof(hdr));
	new_kip_size = dst_data - (u8 *)(new_kip);

	free(ki->kip1);
	ki->kip1 = new_kip;
	ki->size = new_kip_size;

	return 0;
}

static int _kipm_inject(const char *kipm_path, char *target_name, pkg2_kip1_info_t *ki)
{
	if (!strcmp((char *)ki->kip1->name, target_name))
	{
		u32 size = 0;
		u8 *kipm_data = (u8 *)sd_file_read(kipm_path, &size);
		if (!kipm_data)
			return 1;

		u32 inject_size = size - sizeof(ki->kip1->caps);
		u8 *kip_patched_data = (u8 *)malloc(ki->size + inject_size);

		// Copy headers.
		memcpy(kip_patched_data, ki->kip1, sizeof(pkg2_kip1_t));

		pkg2_kip1_t *fs_kip = ki->kip1;
		ki->kip1 = (pkg2_kip1_t *)kip_patched_data;
		ki->size = ki->size + inject_size;

		// Patch caps.
		memcpy(&ki->kip1->caps, kipm_data, sizeof(ki->kip1->caps));
		// Copy our .text data.
		memcpy(&ki->kip1->data, kipm_data + sizeof(ki->kip1->caps), inject_size);

		u32 new_offset = 0;

		for (u32 section_idx = 0; section_idx < KIP1_NUM_SECTIONS - 2; section_idx++)
		{
			if (!section_idx) // .text.
			{
				memcpy(ki->kip1->data + inject_size, fs_kip->data, fs_kip->sections[0].size_comp);
				ki->kip1->sections[0].size_decomp += inject_size;
				ki->kip1->sections[0].size_comp += inject_size;
			}
			else // Others.
			{
				if (section_idx < 3)
					memcpy(ki->kip1->data + new_offset + inject_size, fs_kip->data + new_offset, fs_kip->sections[section_idx].size_comp);
				ki->kip1->sections[section_idx].offset += inject_size;
			}
			new_offset += fs_kip->sections[section_idx].size_comp;
		}

		// Patch PMC capabilities for 1.0.0.
		if (!emu_cfg.fs_ver)
		{
			for (u32 i = 0; i < 0x20; i++)
			{
				if (ki->kip1->caps[i] == 0xFFFFFFFF)
				{
					ki->kip1->caps[i] = 0x07000E7F;
					break;
				}
			}
		}

		free(kipm_data);
		return 0;
	}

	return 1;
}

const char *pkg2_patch_kips(link_t *info, char *patch_names)
{
	bool emummc_patch_selected = false;

	if (patch_names == NULL || patch_names[0] == 0)
		return NULL;

	gfx_printf("%kPatching kips%k\n", TXT_CLR_ORANGE, TXT_CLR_DEFAULT);

	static const u32 MAX_NUM_PATCHES_REQUESTED = sizeof(u32) * 8;
	char *patches[MAX_NUM_PATCHES_REQUESTED];

	u32 patches_num = 1;
	patches[0] = patch_names;
	{
		for (char *p = patch_names; *p != 0; p++)
		{
			if (*p == ',')
			{
				*p = 0;
				patches[patches_num++] = p + 1;
				if (patches_num >= MAX_NUM_PATCHES_REQUESTED)
					return "too_many_patches";
			}
			else if (*p >= 'A' && *p <= 'Z') // Convert to lowercase.
				*p += 0x20;
		}
	}

	u32 patches_applied = 0; // Bitset over patches.
	for (u32 i = 0; i < patches_num; i++)
	{
		// Eliminate leading spaces.
		for (const char *p = patches[i]; *p != 0; p++)
		{
			if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
				patches[i]++;
			else
				break;
		}

		int patch_len = strlen(patches[i]);
		if (patch_len == 0)
			continue;

		// Eliminate trailing spaces.
		for (int chIdx = patch_len - 1; chIdx >= 0; chIdx--)
		{
			const char *p = patches[i] + chIdx;
			if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
				patch_len = chIdx;
			else
				break;
		}
		patches[i][patch_len] = 0;

		DPRINTF("Requested patch: '%s'\n", patches[i]);
	}

	// Parse external patches if needed.
	for (u32 i = 0; i < patches_num; i++)
	{
		if (!strcmp(patches[i], "emummc"))
		{
			// emuMMC patch is managed on its own.
			emummc_patch_selected = true;
			patches_applied |= BIT(i);
			continue;
		}

		if (strcmp(patches[i], "nogc"))
			parse_external_kip_patches();
	}

	u32 kip_hash[SE_SHA_256_SIZE / sizeof(u32)];
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
	{
		// Reset hash so it can be calculated for the new kip.
		kip_hash[0] = 0;

		bool emummc_patch_apply = emummc_patch_selected && !strcmp((char *)ki->kip1->name, "FS");

		// Check all SHA256 ID sets. (IDs are grouped per KIP. IDs are still unique.)
		for (u32 kip_id_idx = 0; kip_id_idx < _kip_id_sets_cnt; kip_id_idx++)
		{
			// Check if KIP name macthes ID's KIP name.
			if (strcmp((char *)ki->kip1->name, _kip_id_sets[kip_id_idx].name) != 0)
				continue;

			// Check if there are patches to apply.
			bool patches_found = false;
			const kip1_patchset_t *patchset = _kip_id_sets[kip_id_idx].patchset;
			while (patchset != NULL && patchset->name != NULL && !patches_found)
			{
				for (u32 i = 0; i < patches_num; i++)
				{
					// Continue if patch name does not match.
					if (strcmp(patchset->name, patches[i]) != 0)
						continue;

					patches_found = true;
					break;
				}
				patchset++;
			}

			// Don't bother hashing this KIP if no patches are enabled for it.
			if (!patches_found && !emummc_patch_apply)
				continue;

			// Check if current KIP not hashed and hash it.
			if (kip_hash[0] == 0)
				if (!se_calc_sha256_oneshot(kip_hash, ki->kip1, ki->size))
					memset(kip_hash, 0, sizeof(kip_hash));

			// Check if kip is the expected version.
			if (memcmp(kip_hash, _kip_id_sets[kip_id_idx].hash, sizeof(_kip_id_sets[0].hash)) != 0)
				continue;

			// Find out which sections are affected by the enabled patches, in order to decompress them.
			u32 sections_affected = 0;
			patchset = _kip_id_sets[kip_id_idx].patchset;
			while (patchset != NULL && patchset->name != NULL)
			{
				if (patchset->patches != NULL)
				{
					for (u32 patch_idx = 0; patch_idx < patches_num; patch_idx++)
					{
						if (strcmp(patchset->name, patches[patch_idx]))
							continue;

						for (const kip1_patch_t *patch = patchset->patches; patch != NULL && (patch->length != 0); patch++)
							sections_affected |= BIT(GET_KIP_PATCH_SECTION(patch->offset));
					}
				}
				patchset++;
			}

			// If emuMMC is enabled, set its affected section.
			if (emummc_patch_apply)
				sections_affected |= BIT(KIP_TEXT);

			// Got patches to apply to this kip, have to decompress it.
			if (_decompress_kip(ki, sections_affected))
				return (char *)ki->kip1->name; // Failed to decompress.

			// Apply all patches for matched ID.
			patchset = _kip_id_sets[kip_id_idx].patchset;
			while (patchset != NULL && patchset->name != NULL)
			{
				for (u32 patch_idx = 0; patch_idx < patches_num; patch_idx++)
				{
					// Check if patchset name matches requested patch.
					if (strcmp(patchset->name, patches[patch_idx]))
						continue;

					u32 applied_mask = BIT(patch_idx);

					// Check if patchset is empty.
					if (patchset->patches == NULL)
					{
						DPRINTF("Patch '%s' not necessary for %s\n", patchset->name, (char *)ki->kip1->name);
						patches_applied |= applied_mask;

						continue; // Continue in case it's double defined.
					}

					// Apply patches per section.
					u8 *kip_sect_data = ki->kip1->data;
					for (u32 section_idx = 0; section_idx < KIP1_NUM_SECTIONS; section_idx++)
					{
						if (sections_affected & BIT(section_idx))
						{
							gfx_printf("Applying '%s' on %s, sect %d\n", patchset->name, (char *)ki->kip1->name, section_idx);
							for (const kip1_patch_t *patch = patchset->patches; patch != NULL && patch->src_data != NULL; patch++)
							{
								// Check if patch is in current section.
								if (GET_KIP_PATCH_SECTION(patch->offset) != section_idx)
									continue;

								// Check if patch is empty.
								if (!patch->length)
								{
									gfx_con.mute = false;
									gfx_printf("%kPatch empty!%k\n", TXT_CLR_ERROR, TXT_CLR_DEFAULT);
									return patchset->name; // MUST stop here as it's not probably intended.
								}

								// If source does not match and is not already patched, throw an error.
								u32 patch_offset = GET_KIP_PATCH_OFFSET(patch->offset);
								if (patch->src_data != KIP1_PATCH_SRC_NO_CHECK                                  &&
									(memcmp(&kip_sect_data[patch_offset], patch->src_data, patch->length) != 0) &&
									(memcmp(&kip_sect_data[patch_offset], patch->dst_data, patch->length) != 0))
								{
									gfx_con.mute = false;
									gfx_printf("%kPatch mismatch at 0x%x!%k\n", TXT_CLR_ERROR, patch_offset, TXT_CLR_DEFAULT);
									return patchset->name; // MUST stop here as kip is likely corrupt.
								}
								else
								{
									DPRINTF("Patching %d bytes at offset 0x%x\n", patch->length, patch_offset);
									memcpy(&kip_sect_data[patch_offset], patch->dst_data, patch->length);
								}
							}
						}
						kip_sect_data += ki->kip1->sections[section_idx].size_comp;
					}

					patches_applied |= applied_mask;
				}

				patchset++;
			}

			// emuMMC must be applied after all other patches, since it affects TEXT offset.
			if (emummc_patch_apply)
			{
				// Encode ID.
				emu_cfg.fs_ver = kip_id_idx;
				if (kip_id_idx)
					emu_cfg.fs_ver--;
				if (kip_id_idx > 17)
					emu_cfg.fs_ver -= 2;

				// Inject emuMMC code.
				gfx_printf("Injecting emuMMC. FS ID: %d\n", emu_cfg.fs_ver);
				if (_kipm_inject("bootloader/sys/emummc.kipm", "FS", ki))
					return "emummc";

				// Skip checking again.
				emummc_patch_selected = false;
				emummc_patch_apply    = false;
			}
		}
	}

	// Check if all patches were applied.
	for (u32 i = 0; i < patches_num; i++)
	{
		if ((patches_applied & BIT(i)) == 0)
			return patches[i];
	}

	// Check if emuMMC was applied.
	if (emummc_patch_selected)
		return "emummc";

	return NULL;
}

// Master key 7 encrypted with 8.  (7.0.0 with 8.1.0). AES-ECB
static const u8 mkey_vector_7xx[SE_KEY_128_SIZE] =
	{ 0xEA, 0x60, 0xB3, 0xEA, 0xCE, 0x8F, 0x24, 0x46, 0x7D, 0x33, 0x9C, 0xD1, 0xBC, 0x24, 0x98, 0x29 };

u8 pkg2_keyslot;
pkg2_hdr_t *pkg2_decrypt(void *data, u8 mkey, bool is_exo)
{
	u8 *pdata = (u8 *)data;

	// Skip signature.
	pdata += 0x100;

	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdata;

	// Skip header.
	pdata += sizeof(pkg2_hdr_t);

	// Set pkg2 key slot to default. If 7.0.0 it will change to 9.
	pkg2_keyslot = 8;

	// Decrypt 7.0.0 pkg2 via 8.1.0 mkey on Erista.
	if (!h_cfg.t210b01 && mkey == HOS_MKEY_VER_700)
	{
		u8 tmp_mkey[SE_KEY_128_SIZE];

		// Decrypt 7.0.0 encrypted mkey.
		se_aes_crypt_ecb(!is_exo ? 7 : 13, DECRYPT, tmp_mkey, SE_KEY_128_SIZE, mkey_vector_7xx, SE_KEY_128_SIZE);

		// Set and unwrap pkg2 key.
		se_aes_key_set(9, tmp_mkey, SE_KEY_128_SIZE);
		se_aes_unwrap_key(9, 9, package2_keyseed);

		pkg2_keyslot = 9;
	}

	// Decrypt header.
	se_aes_crypt_ctr(pkg2_keyslot, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	if (hdr->magic != PKG2_MAGIC)
		return NULL;

	// Decrypt sections.
	for (u32 i = 0; i < 4; i++)
	{
DPRINTF("sec %d has size %08X\n", i, hdr->sec_size[i]);
		if (!hdr->sec_size[i])
			continue;

		se_aes_crypt_ctr(pkg2_keyslot, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * SE_AES_IV_SIZE]);

		pdata += hdr->sec_size[i];
	}

	return hdr;
}

static u32 _pkg2_ini1_build(u8 *pdst, u8 *psec, pkg2_hdr_t *hdr, link_t *kips_info, bool new_pkg2)
{
	// Calculate INI1 size.
	u32 ini1_size = sizeof(pkg2_ini1_t);
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, kips_info, link)
	{
		ini1_size += ki->size;
	}

	// Align size and set it.
	ini1_size = ALIGN(ini1_size, 4);

	// For new kernel if INI1 fits in the old one, use it.
	bool use_old_ini_region = psec && ini1_size <= (pkg2_newkern_ini1_end - pkg2_newkern_ini1_start);
	if (use_old_ini_region)
		pdst = psec + pkg2_newkern_ini1_start;

	// Set initial header and magic.
	pkg2_ini1_t *ini1 = (pkg2_ini1_t *)pdst;
	memset(ini1, 0, sizeof(pkg2_ini1_t));
	ini1->magic = INI1_MAGIC;
	ini1->size  = ini1_size;
	pdst += sizeof(pkg2_ini1_t);

	// Merge KIPs into INI1.
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, kips_info, link)
	{
DPRINTF("adding kip1 '%s' @ %08X (%08X)\n", (char *)ki->kip1->name, (u32)ki->kip1, ki->size);
		memcpy(pdst, ki->kip1, ki->size);
		pdst += ki->size;
		ini1->num_procs++;
	}

	// Encrypt INI1 in its own section if old pkg2. Otherwise it gets embedded into Kernel.
	if (!new_pkg2)
	{
		hdr->sec_size[PKG2_SEC_INI1] = ini1_size;
		hdr->sec_off[PKG2_SEC_INI1] = 0x14080000;
		se_aes_crypt_ctr(8, ini1, ini1_size, ini1, ini1_size, &hdr->sec_ctr[PKG2_SEC_INI1 * SE_AES_IV_SIZE]);
	}
	else
	{
		hdr->sec_size[PKG2_SEC_INI1] = 0;
		hdr->sec_off[PKG2_SEC_INI1] = 0;
	}

	return !use_old_ini_region ? ini1_size : 0;
}

void pkg2_build_encrypt(void *dst, void *hos_ctxt, link_t *kips_info, bool is_exo)
{
	launch_ctxt_t *ctxt = (launch_ctxt_t *)hos_ctxt;
	u32 meso_magic = *(u32 *)(ctxt->kernel + 4);
	u32 kernel_size = ctxt->kernel_size;
	u8 mkey = ctxt->pkg1_id->mkey;
	u8 *pdst = (u8 *)dst;

	// Force new Package2 if Mesosphere.
	bool is_meso = (meso_magic & 0xF0FFFFFF) == ATM_MESOSPHERE;
	if (is_meso)
		ctxt->new_pkg2 = true;

	// Set key version. For Erista 7.0.0, use 8.1.0 because of a bug in Exo2?
	u8 key_ver = mkey ? mkey + 1 : 0;
	if (pkg2_keyslot == 9)
	{
		key_ver = HOS_MKEY_VER_810 + 1;
		pkg2_keyslot = 8;
	}

	// Signature.
	memset(pdst, 0, 0x100);
	pdst += 0x100;

	// Header.
	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdst;
	memset(hdr, 0, sizeof(pkg2_hdr_t));

	// Set initial header values.
	hdr->magic = PKG2_MAGIC;
	hdr->bl_ver = 0;
	hdr->pkg2_ver = 0xFF;

	if (!ctxt->new_pkg2)
		hdr->base = 0x10000000;
	else
		hdr->base = 0x60000;
DPRINTF("%s @ %08X (%08X)\n", is_meso ? "Mesosphere": "kernel",(u32)ctxt->kernel, kernel_size);

	pdst += sizeof(pkg2_hdr_t);

	// Kernel.
	memcpy(pdst, ctxt->kernel, kernel_size);
	if (!ctxt->new_pkg2)
		hdr->sec_off[PKG2_SEC_KERNEL] = 0x10000000;
	else
	{
		// Build INI1 for new Package2.
		u32 ini1_size = _pkg2_ini1_build(pdst + kernel_size, is_meso ? NULL : pdst, hdr, kips_info, true);
		hdr->sec_off[PKG2_SEC_KERNEL] = 0x60000;

		// Set new INI1 offset to kernel.
		u32 meso_meta_offset = *(u32 *)(pdst + 8);
		if (is_meso && (meso_magic & 0x0F000000)) // MSS1.
			*(u32 *)(pdst + meso_meta_offset) = kernel_size - meso_meta_offset;
		else if (ini1_size)
		{
			if (is_meso) // MSS0.
				*(u32 *)(pdst + 8) = kernel_size;
			else
				*(u32 *)(pdst + pkg2_newkern_ini1_info) = kernel_size - pkg2_newkern_ini1_rela;
		}

		kernel_size += ini1_size;
	}
	hdr->sec_size[PKG2_SEC_KERNEL] = kernel_size;
	se_aes_crypt_ctr(pkg2_keyslot, pdst, kernel_size, pdst, kernel_size, &hdr->sec_ctr[PKG2_SEC_KERNEL * SE_AES_IV_SIZE]);
	pdst += kernel_size;
DPRINTF("kernel encrypted\n");

	// Build INI1 for old Package2.
	u32 ini1_size = 0;
	if (!ctxt->new_pkg2)
		ini1_size = _pkg2_ini1_build(pdst, NULL, hdr, kips_info, false);
DPRINTF("INI1 encrypted\n");

	if (!is_exo) // Not needed on Exosphere 1.0.0 and up.
	{
		// Calculate SHA256 over encrypted sections. Only 3 have valid hashes.
		u8 *pk2_hash_data = (u8 *)dst + 0x100 + sizeof(pkg2_hdr_t);
		for (u32 i = PKG2_SEC_KERNEL; i <= PKG2_SEC_UNUSED; i++)
		{
			se_calc_sha256_oneshot(&hdr->sec_sha256[SE_SHA_256_SIZE * i], (void *)pk2_hash_data, hdr->sec_size[i]);
			pk2_hash_data += hdr->sec_size[i];
		}
	}

	// Encrypt header.
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
	hdr->ctr[4] = key_ver;
	se_aes_crypt_ctr(pkg2_keyslot, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	memset(hdr->ctr, 0 , SE_AES_IV_SIZE);
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
	hdr->ctr[4] = key_ver;
}
