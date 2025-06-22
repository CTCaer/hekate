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

#include <mem/heap.h>
#include <power/max77620.h>
#include <rtc/max77620-rtc.h>
#include <soc/bpmp.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <storage/sd.h>
#include <utils/util.h>

u8 bit_count(u32 val)
{
	u8 cnt = 0;
	for (u32 i = 0; i < 32; i++)
	{
		if ((val >> i) & 1)
			cnt++;
	}

	return cnt;
}

u32 bit_count_mask(u8 bits)
{
	u32 val = 0;
	for (u32 i = 0; i < bits; i++)
		val |= 1 << i;

	return val;
}

char *strcpy_ns(char *dst, char *src)
{
	if (!src || !dst)
		return NULL;

	// Remove starting space.
	u32 len = strlen(src);
	if (len && src[0] == ' ')
	{
		len--;
		src++;
	}

	strcpy(dst, src);

	// Remove trailing space.
	if (len && dst[len - 1] == ' ')
		dst[len - 1] = 0;

	return dst;
}

// Approximate square root finder for a 64-bit number.
u64 sqrt64(u64 num)
{
	u64 base = 0;
	u64 limit = num;
	u64 square_root = 0;

	while (base <= limit)
	{
		u64 tmp_sqrt = (base + limit) / 2;

		if (tmp_sqrt * tmp_sqrt == num) {
			square_root = tmp_sqrt;
			break;
		}

		if (tmp_sqrt * tmp_sqrt < num)
		{
			square_root = base;
			base = tmp_sqrt + 1;
		}
		else
			limit = tmp_sqrt - 1;
	}

	return square_root;
}

#define	TULONG_MAX  ((unsigned long)((unsigned long)(~0L)))
#define	TLONG_MAX   ((long)(((unsigned long)(~0L)) >> 1))
#define	TLONG_MIN   ((long)(~TLONG_MAX))
#define ISSPACE(ch) ((ch >= '\t' && ch <= '\r') || (ch == ' '))
#define ISDIGIT(ch) ( ch >= '0'  && ch <= '9' )
#define ISALPHA(ch) ((ch >= 'a'  && ch <= 'z')  || (ch >= 'A' && ch <= 'Z'))
#define ISUPPER(ch) ( ch >= 'A'  && ch <= 'Z' )

/*
 * Avoid using reentrant newlib version of strol. It's only used for errno.
 *
 * strol/atoi:
 * Copyright (c) 1990 The Regents of the University of California.
 */
long strtol(const char *nptr, char **endptr, register int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (ISSPACE(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)TLONG_MIN : (base == 16 ? TULONG_MAX : TLONG_MAX);
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (ISDIGIT(c))
			c -= '0';
		else if (ISALPHA(c))
			c -= ISUPPER(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? TLONG_MIN : TLONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

int atoi(const char *nptr)
{
  return (int)strtol(nptr, (char **)NULL, 10);
}

void reg_write_array(u32 *base, const reg_cfg_t *cfg, u32 num_cfg)
{
	// Expected register offset is a u32 array index.
	for (u32 i = 0; i < num_cfg; i++)
		base[cfg[i].idx] = cfg[i].val;
}

u32 crc32_calc(u32 crc, const u8 *buf, u32 len)
{
	const u8 *p, *q;
	static u32 *table = NULL;

	// Calculate CRC table.
	if (!table)
	{
		table = zalloc(256 * sizeof(u32));
		for (u32 i = 0; i < 256; i++)
		{
			u32 rem = i;
			for (u32 j = 0; j < 8; j++)
			{
				if (rem & 1)
				{
					rem >>= 1;
					rem ^= 0xedb88320;
				}
				else
					rem >>= 1;
			}
			table[i] = rem;
		}
	}

	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++)
	{
		u8 oct = *p;
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ oct];
	}

	return ~crc;
}

int qsort_compare_int(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

int qsort_compare_char(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

int qsort_compare_char_case(const void *a, const void *b)
{
	return strcasecmp(*(const char **)a, *(const char **)b);
}

void panic(u32 val)
{
	// Set panic code.
	PMC(APBDEV_PMC_SCRATCH200) = val;

	// Disable SE.
	//PMC(APBDEV_PMC_CRYPTO_OP) = PMC_CRYPTO_OP_SE_DISABLE;

	// Immediately cause a full system reset.
	watchdog_start(0, TIMER_PMCRESET_EN);

	while (true);
}

void power_set_state(power_state_t state)
{
	u8 reg;

	// Unmount and power down sd card.
	sd_end();

	// De-initialize and power down various hardware.
	hw_deinit(false, 0);

	// Set power state.
	switch (state)
	{
	case REBOOT_RCM:
		PMC(APBDEV_PMC_SCRATCH0) = PMC_SCRATCH0_MODE_RCM; // Enable RCM path.
		PMC(APBDEV_PMC_CNTRL)   |= PMC_CNTRL_MAIN_RST;    // PMC reset.
		break;

	case REBOOT_BYPASS_FUSES:
		panic(PMC_NX_PANIC_BYPASS_FUSES); // Bypass fuse programming in package1.
		break;

	case POWER_OFF:
		// Initiate power down sequence and do not generate a reset (regulators retain state after POR).
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
		break;

	case POWER_OFF_RESET:
	case POWER_OFF_REBOOT:
	default:
		// Enable/Disable soft reset wake event.
		reg = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG2);
		if (state == POWER_OFF_RESET) // Do not wake up after power off.
			reg &= ~(MAX77620_ONOFFCNFG2_SFT_RST_WK | MAX77620_ONOFFCNFG2_WK_ALARM1 | MAX77620_ONOFFCNFG2_WK_ALARM2);
		else // POWER_OFF_REBOOT. Wake up after power off.
			reg |= MAX77620_ONOFFCNFG2_SFT_RST_WK;
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG2, reg);

		// Initiate power down sequence and generate a reset (regulators' state resets after POR).
		i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_SFT_RST);
		break;
	}

	while (true)
		bpmp_halt();
}

void power_set_state_ex(void *param)
{
	power_state_t *state = (power_state_t *)param;
	power_set_state(*state);
}
