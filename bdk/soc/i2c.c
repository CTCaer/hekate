/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#include <soc/i2c.h>
#include <utils/util.h>

#define I2C_PACKET_PROT_I2C  BIT(4)
#define I2C_HEADER_CONT_XFER BIT(15)
#define I2C_HEADER_REP_START BIT(16)
#define I2C_HEADER_IE_ENABLE BIT(17)
#define I2C_HEADER_READ      BIT(19)

#define I2C_CNFG              (0x00 / 4)
#define  CMD1_WRITE           (0 << 6)
#define  CMD1_READ            BIT(6)
#define  NORMAL_MODE_GO       BIT(9)
#define  PACKET_MODE_GO       BIT(10)
#define  NEW_MASTER_FSM       BIT(11)
#define  DEBOUNCE_CNT_4T      (2 << 12)

#define I2C_CMD_ADDR0         (0x04 / 4)
#define  ADDR0_WRITE          0
#define  ADDR0_READ           1

#define I2C_CMD_DATA1         (0x0C / 4)
#define I2C_CMD_DATA2         (0x10 / 4)

#define I2C_STATUS            (0x1C / 4)
#define  I2C_STATUS_NOACK     (0xF << 0)
#define  I2C_STATUS_BUSY      BIT(8)

#define I2C_TX_FIFO           (0x50 / 4)
#define I2C_RX_FIFO           (0x54 / 4)

#define I2C_FIFO_CONTROL      (0x5C / 4)
#define  RX_FIFO_FLUSH        BIT(0)
#define  TX_FIFO_FLUSH        BIT(1)

#define I2C_FIFO_STATUS       (0x60 / 4)
#define  RX_FIFO_FULL_CNT     (0xF << 0)
#define  TX_FIFO_EMPTY_CNT    (0xF << 4)

#define I2C_INT_EN            (0x64 / 4)
#define I2C_INT_STATUS        (0x68 / 4)
#define I2C_INT_SOURCE        (0x70 / 4)
#define  RX_FIFO_DATA_REQ     BIT(0)
#define  TX_FIFO_DATA_REQ     BIT(1)
#define  ARB_LOST             BIT(2)
#define  NO_ACK               BIT(3)
#define  RX_FIFO_UNDER        BIT(4)
#define  TX_FIFO_OVER         BIT(5)
#define  ALL_PACKETS_COMPLETE BIT(6)
#define  PACKET_COMPLETE      BIT(7)
#define  BUS_CLEAR_DONE       BIT(11)

#define I2C_CLK_DIVISOR       (0x6C / 4)

#define I2C_BUS_CLEAR_CONFIG  (0x84 / 4)
#define  BC_ENABLE            BIT(0)
#define  BC_TERMINATE         BIT(1)

#define I2C_BUS_CLEAR_STATUS  (0x88 / 4)

#define I2C_CONFIG_LOAD       (0x8C / 4)
#define  MSTR_CONFIG_LOAD     BIT(0)
#define  TIMEOUT_CONFIG_LOAD  BIT(2)

static const u32 i2c_addrs[] = {
	0x7000C000, // I2C_1.
	0x7000C400, // I2C_2.
	0x7000C500, // I2C_3.
	0x7000C700, // I2C_4.
	0x7000D000, // I2C_5.
	0x7000D100  // I2C_6.
};

static void _i2c_load_cfg_wait(vu32 *base)
{
	base[I2C_CONFIG_LOAD] = BIT(5) | TIMEOUT_CONFIG_LOAD | MSTR_CONFIG_LOAD;
	for (u32 i = 0; i < 20; i++)
	{
		usleep(1);
		if (!(base[I2C_CONFIG_LOAD] & MSTR_CONFIG_LOAD))
			break;
	}
}

static int _i2c_send_single(u32 i2c_idx, u32 dev_addr, u8 *buf, u32 size)
{
	if (size > 8)
		return 0;

	u32 tmp = 0;

	vu32 *base = (vu32 *)i2c_addrs[i2c_idx];

	// Set device address and send mode.
	base[I2C_CMD_ADDR0] = dev_addr << 1 | ADDR0_WRITE;

	if (size > 4)
	{
		memcpy(&tmp, buf, 4);
		base[I2C_CMD_DATA1] = tmp; //Set value.
		tmp = 0;
		memcpy(&tmp, buf + 4, size - 4);
		base[I2C_CMD_DATA2] = tmp;
	}
	else
	{
		memcpy(&tmp, buf, size);
		base[I2C_CMD_DATA1] = tmp; //Set value.
	}

	// Set size and send mode.
	base[I2C_CNFG] = ((size - 1) << 1) | DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_WRITE;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on normal mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | NORMAL_MODE_GO;

	u32 timeout = get_tmr_us() + 200000; // Actual for max 8 bytes at 100KHz is 0.74ms.
	while (base[I2C_STATUS] & I2C_STATUS_BUSY)
	{
		if (get_tmr_us() > timeout)
			return 0;
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK)
		return 0;

	return 1;
}

static int _i2c_recv_single(u32 i2c_idx, u8 *buf, u32 size, u32 dev_addr)
{
	if (size > 8)
		return 0;

	vu32 *base = (vu32 *)i2c_addrs[i2c_idx];

	// Set device address and recv mode.
	base[I2C_CMD_ADDR0] = (dev_addr << 1) | ADDR0_READ;

	// Set size and recv mode.
	base[I2C_CNFG] = ((size - 1) << 1) | DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_READ;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on normal mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | NORMAL_MODE_GO;

	u32 timeout = get_tmr_us() + 200000; // Actual for max 8 bytes at 100KHz is 0.74ms.
	while (base[I2C_STATUS] & I2C_STATUS_BUSY)
	{
		if (get_tmr_us() > timeout)
			return 0;
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK)
		return 0;

	u32 tmp = base[I2C_CMD_DATA1]; // Get LS value.
	if (size > 4)
	{
		memcpy(buf, &tmp, 4);
		tmp = base[I2C_CMD_DATA2]; // Get MS value.
		memcpy(buf + 4, &tmp, size - 4);
	}
	else
		memcpy(buf, &tmp, size);

	return 1;
}

static int _i2c_send_pkt(u32 i2c_idx, u8 *buf, u32 size, u32 dev_addr)
{
	if (size > 32)
		return 0;

	int res = 0;

	vu32 *base = (vu32 *)i2c_addrs[i2c_idx];

	// Enable interrupts.
	base[I2C_INT_EN] = ALL_PACKETS_COMPLETE | PACKET_COMPLETE | NO_ACK |
		ARB_LOST | TX_FIFO_OVER | RX_FIFO_UNDER | TX_FIFO_DATA_REQ;
	base[I2C_INT_STATUS] = base[I2C_INT_STATUS];

	// Set device address and recv mode.
	base[I2C_CMD_ADDR0] = (dev_addr << 1) | ADDR0_READ;

	// Set recv mode.
	base[I2C_CNFG] = DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_WRITE;

	// Set and flush FIFO.
	base[I2C_FIFO_CONTROL] = RX_FIFO_FLUSH | TX_FIFO_FLUSH;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on packet mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | PACKET_MODE_GO;

	u32 hdr[3];
	hdr[0] = I2C_PACKET_PROT_I2C;
	hdr[1] = size - 1;
	hdr[2] = I2C_HEADER_IE_ENABLE | I2C_HEADER_CONT_XFER | (dev_addr << 1);

	// Send header with request.
	base[I2C_TX_FIFO] = hdr[0];
	base[I2C_TX_FIFO] = hdr[1];
	base[I2C_TX_FIFO] = hdr[2];

	u32 timeout = get_tmr_ms() + 400;
	while (size)
	{
		if (base[I2C_FIFO_STATUS] & TX_FIFO_EMPTY_CNT)
		{
			u32 tmp = 0;
			u32 snd_size = MIN(size, 4);
			memcpy(&tmp, buf, snd_size);
			base[I2C_TX_FIFO] = tmp;
			buf += snd_size;
			size -= snd_size;
		}

		if (get_tmr_ms() > timeout)
		{
			res = 1;
			break;
		}
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK || base[I2C_INT_STATUS] & NO_ACK)
		res = 1;

	// Disable packet mode.
	usleep(20);
	base[I2C_CNFG] &= 0xFFFFF9FF;

	// Disable interrupts.
	base[I2C_INT_EN] = 0;

	return res;
}

static int _i2c_recv_pkt(u32 i2c_idx, u8 *buf, u32 size, u32 dev_addr, u32 reg)
{
	if (size > 32)
		return 0;

	int res = 0;

	vu32 *base = (vu32 *)i2c_addrs[i2c_idx];

	// Enable interrupts.
	base[I2C_INT_EN] = ALL_PACKETS_COMPLETE | PACKET_COMPLETE | NO_ACK |
		ARB_LOST | TX_FIFO_OVER | RX_FIFO_UNDER | RX_FIFO_DATA_REQ;
	base[I2C_INT_STATUS] = base[I2C_INT_STATUS];

	// Set device address and recv mode.
	base[I2C_CMD_ADDR0] = (dev_addr << 1) | ADDR0_READ;

	// Set recv mode.
	base[I2C_CNFG] = DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_READ;

	// Set and flush FIFO.
	base[I2C_FIFO_CONTROL] = RX_FIFO_FLUSH | TX_FIFO_FLUSH;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on packet mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | PACKET_MODE_GO;

	// Send reg request.
	u32 hdr[3];
	hdr[0] = I2C_PACKET_PROT_I2C;
	hdr[1] = 1 - 1;
	hdr[2] = I2C_HEADER_REP_START | (dev_addr << 1);

	// Send header with reg request.
	base[I2C_TX_FIFO] = hdr[0];
	base[I2C_TX_FIFO] = hdr[1];
	base[I2C_TX_FIFO] = hdr[2];
	base[I2C_TX_FIFO] = reg;

	u32 timeout = get_tmr_ms() + 400;
	while (!(base[I2C_FIFO_STATUS] & TX_FIFO_EMPTY_CNT))
		if (get_tmr_ms() > timeout)
			break;

	// Send read request.
	hdr[1] = size - 1;
	hdr[2] = I2C_HEADER_READ | (dev_addr << 1);

	// Send header with read request.
	base[I2C_TX_FIFO] = hdr[0];
	base[I2C_TX_FIFO] = hdr[1];
	base[I2C_TX_FIFO] = hdr[2];

	timeout = get_tmr_ms() + 400;
	while (size)
	{
		if (base[I2C_FIFO_STATUS] & RX_FIFO_FULL_CNT)
		{
			u32 rcv_size = MIN(size, 4);
			u32 tmp = base[I2C_RX_FIFO];
			memcpy(buf, &tmp, rcv_size);
			buf += rcv_size;
			size -= rcv_size;
		}

		if (get_tmr_ms() > timeout)
		{
			res = 1;
			break;
		}
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK || base[I2C_INT_STATUS] & NO_ACK)
		res = 1;

	// Disable packet mode.
	usleep(20);
	base[I2C_CNFG] &= 0xFFFFF9FF;

	// Disable interrupts.
	base[I2C_INT_EN] = 0;

	return res;
}

void i2c_init(u32 i2c_idx)
{
	vu32 *base = (vu32 *)i2c_addrs[i2c_idx];

	base[I2C_CLK_DIVISOR] = (5 << 16) | 1; // SF mode Div: 6, HS mode div: 2.
	base[I2C_BUS_CLEAR_CONFIG] = (9 << 16) | BC_TERMINATE | BC_ENABLE;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	for (u32 i = 0; i < 10; i++)
	{
		usleep(20000);
		if (base[I2C_INT_STATUS] & BUS_CLEAR_DONE)
			break;
	}

	(vu32)base[I2C_BUS_CLEAR_STATUS];
	base[I2C_INT_STATUS] = base[I2C_INT_STATUS];
}

int i2c_recv_buf(u8 *buf, u32 size, u32 i2c_idx, u32 dev_addr)
{
	return _i2c_recv_single(i2c_idx, buf, size, dev_addr);
}

int i2c_send_buf_big(u32 i2c_idx, u32 dev_addr, u8 *buf, u32 size)
{
	if (size > 32)
		return 0;

	return _i2c_send_pkt(i2c_idx, buf, size, dev_addr);
}

int i2c_recv_buf_big(u8 *buf, u32 size, u32 i2c_idx, u32 dev_addr, u32 reg)
{
	return _i2c_recv_pkt(i2c_idx, buf, size, dev_addr, reg);
}

int i2c_send_buf_small(u32 i2c_idx, u32 dev_addr, u32 reg, u8 *buf, u32 size)
{
	u8 tmp[4];

	if (size > 7)
		return 0;

	tmp[0] = reg;
	memcpy(tmp + 1, buf, size);

	return _i2c_send_single(i2c_idx, dev_addr, tmp, size + 1);
}

int i2c_recv_buf_small(u8 *buf, u32 size, u32 i2c_idx, u32 dev_addr, u32 reg)
{
	int res = _i2c_send_single(i2c_idx, dev_addr, (u8 *)&reg, 1);
	if (res)
		res = _i2c_recv_single(i2c_idx, buf, size, dev_addr);
	return res;
}

int i2c_send_byte(u32 i2c_idx, u32 dev_addr, u32 reg, u8 val)
{
	return i2c_send_buf_small(i2c_idx, dev_addr, reg, &val, 1);
}

u8 i2c_recv_byte(u32 i2c_idx, u32 dev_addr, u32 reg)
{
	u8 tmp = 0;
	i2c_recv_buf_small(&tmp, 1, i2c_idx, dev_addr, reg);
	return tmp;
}

