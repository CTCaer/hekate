/*
 * Copyright (c) 2019 shchmue
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

#ifndef NX_EMMC_BIS_H
#define NX_EMMC_BIS_H

#include <storage/emmc.h>
#include <storage/sdmmc.h>

#define NAND_PATROL_SECTOR   0xC20

typedef struct _nx_emmc_cal0_spk_t
{
	u16 unk0;
	u16 unk1;
	u16 eq_bw_lop;
	u16 eq_gn_lop;
	u16 eq_fc_bp1;
	u16 eq_bw_bp1;
	u16 eq_gn_bp1;
	u16 eq_fc_bp2;
	u16 eq_bw_bp2;
	u16 eq_gn_bp2;
	u16 eq_fc_bp3;
	u16 eq_bw_bp3;
	u16 eq_gn_bp3;
	u16 eq_fc_bp4;
	u16 eq_bw_bp4;
	u16 eq_gn_bp4;
	u16 eq_fc_hip1;
	u16 eq_gn_hip1;
	u16 eq_fc_hip2;
	u16 eq_bw_hip2;
	u16 eq_gn_hip2;
	u16 eq_pre_vol;
	u16 eq_pst_vol;
	u16 eq_ctrl2;
	u16 eq_ctrl1;
	u16 drc_agc_2;
	u16 drc_agc_3;
	u16 drc_agc_1;
	u16 spk_vol;
	u16 hp_vol;
	u16 dac1_min_vol_spk;
	u16 dac1_max_vol_spk;
	u16 dac1_min_vol_hp;
	u16 dac1_max_vol_hp;
	u16 in1_in2;
	u16 adc_vol_min;
	u16 adc_vol_max;
	u8  unk4[16];
} __attribute__((packed)) nx_emmc_cal0_spk_t;

typedef struct _nx_emmc_cal0_t
{
	// Header.
	u32  magic; // 'CAL0'.
	u32  version;
	u32  body_size;
	u16  model;
	u16  update_cnt;
	u8   pad_crc16_hdr[0x10];

	// SHA256 for body.
	u8   body_sha256[0x20];

	// Body.
	char cfg_id1[0x1E];
	u8   crc16_pad1[2];
	u8   rsvd0[0x20];
	u32  wlan_cc_num;
	u32  wlan_cc_last;
	char wlan_cc[128][3];
	u8   crc16_pad2[8];
	u8   wlan_mac[6];
	u8   crc16_pad3[2];
	u8   rsvd1[8];
	u8   bd_mac[6];
	u8   crc16_pad4[2];
	u8   rsvd2[8];
	u16  acc_offset[3];
	u8   crc16_pad5[2];
	u16  acc_scale[3];
	u8   crc16_pad6[2];
	u16  gyro_offset[3];
	u8   crc16_pad7[2];
	u16  gyro_scale[3];
	u8   crc16_pad8[2];
	char serial_number[0x18];
	u8   crc16_pad9[8];

	u8   ecc_p256_device_key[0x30];
	u8   crc16_pad10[0x10];
	u8   ecc_p256_device_cert[0x180];
	u8   crc16_pad11[0x10];
	u8   ecc_p233_device_key[0x30];
	u8   crc16_pad12[0x10];
	u8   ecc_p33_device_cert[0x180];
	u8   crc16_pad13[0x10];
	u8   ecc_p256_ticket_key[0x30];
	u8   crc16_pad14[0x10];
	u8   ecc_p256_ticket_cert[0x180];
	u8   crc16_pad15[0x10];
	u8   ecc_p233_ticket_key[0x30];
	u8   crc16_pad16[0x10];
	u8   ecc_p33_ticket_cert[0x180];
	u8   crc16_pad17[0x10];
	u8   ssl_key[0x110];
	u8   crc16_pad18[0x10];
	u32  ssl_cert_size;
	u8   crc16_pad19[0xC];
	u8   ssl_cert[0x800];
	u8   ssl_sha256[0x20];
	u8   random_number[0x1000];
	u8   random_number_sha256[0x20];
	u8   gc_key[0x110];
	u8   crc16_pad20[0x10];
	u8   gc_cert[0x400];
	u8   gc_cert_sha256[0x20];
	u8   rsa2048_eticket_key[0x220];
	u8   crc16_pad21[0x10];
	u8   rsa2048_eticket_cert[0x240];
	u8   crc16_pad22[0x10];

	char battery_lot[0x1E];
	u8   crc16_pad23[2];
	nx_emmc_cal0_spk_t spk_cal;
	u8   spk_cal_rsvd[0x800 - sizeof(nx_emmc_cal0_spk_t)];
	u8   crc16_pad24[0x10];
	u32  region_code;
	u8   crc16_pad25[0xC];

	u8   amiibo_key[0x50];
	u8   crc16_pad26[0x10];
	u8   amiibo_ecqv_cert[0x14];
	u8   crc16_pad27[0xC];
	u8   amiibo_ecqdsa_cert[0x70];
	u8   crc16_pad28[0x10];
	u8   amiibo_ecqv_bls_key[0x40];
	u8   crc16_pad29[0x10];
	u8   amiibo_ecqv_bls_cert[0x20];
	u8   crc16_pad30[0x10];
	u8   amiibo_ecqv_bls_root_cert[0x90];
	u8   crc16_pad31[0x10];

	u32  product_model; // 1: Nx, 2: Copper, 4: Hoag.
	u8   crc16_pad32[0xC];
	u8   home_menu_scheme_main_color[6];
	u8   crc16_pad33[0xA];
	u32  lcd_bl_brightness_mapping[3]; // Floats. Normally 100%, 0% and 2%.
	u8   crc16_pad34[0x4];

	u8   ext_ecc_b233_device_key[0x50];
	u8   crc16_pad35[0x10];
	u8   ext_ecc_p256_eticket_key[0x50];
	u8   crc16_pad36[0x10];
	u8   ext_ecc_b233_eticket_key[0x50];
	u8   crc16_pad37[0x10];
	u8   ext_ecc_rsa2048_eticket_key[0x240];
	u8   crc16_pad38[0x10];
	u8   ext_ssl_key[0x130];
	u8   crc16_pad39[0x10];
	u8   ext_gc_key[0x130];
	u8   crc16_pad40[0x10];

	u32  lcd_vendor;
	u8   crc16_pad41[0xC];

	// 5.0.0 and up.
	u8   ext_rsa2048_device_key[0x240];
	u8   crc16_pad42[0x10];
	u8   rsa2048_device_cert[0x240];
	u8   crc16_pad43[0x10];

	u8   usbc_pwr_src_circuit_ver;
	u8   crc16_pad44[0xF];

	// 9.0.0 and up.
	u32  home_menu_scheme_sub_color;
	u8   crc16_pad45[0xC];
	u32  home_menu_scheme_bezel_color;
	u8   crc16_pad46[0xC];
	u32  home_menu_scheme_main_color1;
	u8   crc16_pad47[0xC];
	u32  home_menu_scheme_main_color2;
	u8   crc16_pad48[0xC];
	u32  home_menu_scheme_main_color3;
	u8   crc16_pad49[0xC];

	u8   analog_stick_type_l;
	u8   crc16_pad50[0xF];
	u8   analog_stick_param_l[0x12];
	u8   crc16_pad51[0xE];
	u8   analog_stick_cal_l[0x9];
	u8   crc16_pad52[0x7];
	u8   analog_stick_type_r;
	u8   crc16_pad53[0xF];
	u8   analog_stick_param_r[0x12];
	u8   crc16_pad54[0xE];
	u8   analog_stick_cal_r[0x9];
	u8   crc16_pad55[0x7];
	u8   console_6axis_sensor_type;
	u8   crc16_pad56[0xF];
	u8   console_6axis_sensor_hor_off[0x6];
	u8   crc16_pad57[0xA];

	// 6.0.0 and up.
	u8   battery_ver;
	u8   crc16_pad58[0xF];

	// 10.0.0 and up.
	u8   touch_ic_vendor_id;
	u8   crc16_pad59[0xF];

	// 9.0.0 and up.
	u32  color_model;
	u8   crc16_pad60[0xC];

	// 10.0.0 and up.
	u8   console_6axis_sensor_mount_type;
	u8   crc16_pad61[0xF];
} __attribute__((packed)) nx_emmc_cal0_t;

int  nx_emmc_bis_read(u32 sector, u32 count, void *buff);
int  nx_emmc_bis_write(u32 sector, u32 count, void *buff);
void nx_emmc_bis_init(emmc_part_t *part, bool enable_cache, u32 emummc_offset);
void nx_emmc_bis_end();

#endif
