/* Copyright  2016 MediaTek Inc.
 * Author: Carlos Huang <carlos.huang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "raether.h"
#include "ra_switch.h"
#include "ra_mac.h"
#include "raeth_reg.h"

void reg_bit_zero(void __iomem *addr, unsigned int bit, unsigned int len)
{
	int reg_val;
	int i;

	reg_val = sys_reg_read(addr);
	for (i = 0; i < len; i++)
		reg_val &= ~(1 << (bit + i));
	sys_reg_write(addr, reg_val);
}

void reg_bit_one(void __iomem *addr, unsigned int bit, unsigned int len)
{
	unsigned int reg_val;
	unsigned int i;

	reg_val = sys_reg_read(addr);
	for (i = 0; i < len; i++)
		reg_val |= 1 << (bit + i);
	sys_reg_write(addr, reg_val);
}

void tc_phy_write_g_reg(u8 port_num, u8 page_num,
			u8 reg_num, u32 reg_data)
{
	u32 r31 = 0;

	r31 |= 0 << 15;	/* global */
	r31 |= ((page_num & 0x7) << 12);	/* page no */
	mii_mgr_write(port_num, 31, r31);	/* change Global page */
	mii_mgr_write(port_num, reg_num, reg_data);
}

void tc_phy_write_l_reg(u8 port_no, u8 page_no,
			u8 reg_num, u32 reg_data)
{
	u32 r31 = 0;

	r31 |= 1 << 15;	/* local */
	r31 |= ((page_no & 0x7) << 12);	/* page no */
	mii_mgr_write(port_no, 31, r31); /* select local page x */
	mii_mgr_write(port_no, reg_num, reg_data);
}

u32 tc_phy_read_g_reg(u8 port_num, u8 page_num, u8 reg_num)
{
	u32 phy_val;

	u32 r31 = 0;

	r31 |= 0 << 15;	/* global */
	r31 |= ((page_num & 0x7) << 12);	/* page no */
	mii_mgr_write(port_num, 31, r31);	/* change Global page */
	mii_mgr_read(port_num, reg_num, &phy_val);
	return phy_val;
}

u32 tc_phy_read_l_reg(u8 port_no, u8 page_no, u8 reg_num)
{
	u32 phy_val;
	u32 r31 = 0;

	r31 |= 1 << 15;	/* local */
	r31 |= ((page_no & 0x7) << 12);	/* page no */
	mii_mgr_write(port_no, 31, r31); /* select local page x */
	mii_mgr_read(port_no, reg_num, &phy_val);
	return phy_val;
}

u32 tc_phy_read_dev_reg(u32 port_num, u32 dev_addr, u32 reg_addr)
{
	u32 phy_val;

	mii_mgr_read_cl45(port_num, dev_addr, reg_addr, &phy_val);
	return phy_val;
}

void tc_phy_write_dev_reg(u32 port_num, u32 dev_addr, u32 reg_addr, u32 write_data)
{
	mii_mgr_write_cl45(port_num, dev_addr, reg_addr, write_data);
}

u32 tc_mii_read(u32 phy_addr, u32 phy_register)
{
	u32 phy_val;

	mii_mgr_read(phy_addr, phy_register, &phy_val);
	return phy_val;
}

void tc_mii_write(u32 phy_addr, u32 phy_register, u32 write_data)
{
	mii_mgr_write(phy_addr, phy_register, write_data);
}

/* EPHY/GE calibration functions removed for MT7620-only build */

static void wait_loop(void)
{
	int i;
	int read_data;

	for (i = 0; i < 320; i = i + 1)
		read_data = sys_reg_read(RALINK_ETH_SW_BASE + 0x108);
}

static void trgmii_calibration_7623(void)
{
	/* minimum delay for all correct */
	unsigned int tap_a[5] = {
		0, 0, 0, 0, 0
	};
	/* maximum delay for all correct */
	unsigned int tap_b[5] = {
		0, 0, 0, 0, 0
	};
	unsigned int final_tap[5];
	unsigned int rxc_step_size;
	unsigned int rxd_step_size;
	unsigned int read_data;
	unsigned int tmp;
	unsigned int rd_wd;
	int i;
	unsigned int err_cnt[5];
	unsigned int init_toggle_data;
	unsigned int err_flag[5];
	unsigned int err_total_flag;
	unsigned int training_word;
	unsigned int rd_tap;

	void __iomem *TRGMII_7623_base;
	void __iomem *TRGMII_7623_RD_0;
	void __iomem *temp_addr;

	TRGMII_7623_base = ETHDMASYS_ETH_SW_BASE + 0x0300;
	TRGMII_7623_RD_0 = TRGMII_7623_base + 0x10;
	rxd_step_size = 0x1;
	rxc_step_size = 0x4;
	init_toggle_data = 0x00000055;
	training_word = 0x000000AC;

	/* RX clock gating in MT7623 */
	reg_bit_zero(TRGMII_7623_base + 0x04, 30, 2);
	/* Assert RX  reset in MT7623 */
	reg_bit_one(TRGMII_7623_base + 0x00, 31, 1);
	/* Set TX OE edge in  MT7623 */
	reg_bit_one(TRGMII_7623_base + 0x78, 13, 1);
	/* Disable RX clock gating in MT7623 */
	reg_bit_one(TRGMII_7623_base + 0x04, 30, 2);
	/* Release RX reset in MT7623 */
	reg_bit_zero(TRGMII_7623_base, 31, 1);

	for (i = 0; i < 5; i++)
		/* Set bslip_en = 1 */
		reg_bit_one(TRGMII_7623_RD_0 + i * 8, 31, 1);

	/* Enable Training Mode in MT7530 */
	mii_mgr_read(0x1F, 0x7A40, &read_data);
	read_data |= 0xc0000000;
	mii_mgr_write(0x1F, 0x7A40, read_data);

	err_total_flag = 0;
	read_data = 0x0;
	while (err_total_flag == 0 && read_data != 0x68) {
		/* Enable EDGE CHK in MT7623 */
		for (i = 0; i < 5; i++) {
			reg_bit_zero(TRGMII_7623_RD_0 + i * 8, 28, 4);
			reg_bit_one(TRGMII_7623_RD_0 + i * 8, 31, 1);
		}
		wait_loop();
		err_total_flag = 1;
		for (i = 0; i < 5; i++) {
			tmp = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			err_cnt[i] = (tmp >> 8) & 0x0000000f;

			tmp = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			rd_wd = (tmp >> 16) & 0x000000ff;

			if (err_cnt[i] != 0)
				err_flag[i] = 1;
			else if (rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;
			err_total_flag = err_flag[i] & err_total_flag;
		}

		/* Disable EDGE CHK in MT7623 */
		for (i = 0; i < 5; i++) {
			reg_bit_one(TRGMII_7623_RD_0 + i * 8, 30, 1);
			reg_bit_zero(TRGMII_7623_RD_0 + i * 8, 28, 2);
			reg_bit_zero(TRGMII_7623_RD_0 + i * 8, 31, 1);
		}
		wait_loop();
		/* Adjust RXC delay */
		/* RX clock gating in MT7623 */
		reg_bit_zero(TRGMII_7623_base + 0x04, 30, 2);
		read_data = sys_reg_read(TRGMII_7623_base);
		if (err_total_flag == 0) {
			tmp = (read_data & 0x0000007f) + rxc_step_size;
			read_data >>= 8;
			read_data &= 0xffffff80;
			read_data |= tmp;
			read_data <<= 8;
			read_data &= 0xffffff80;
			read_data |= tmp;
			sys_reg_write(TRGMII_7623_base, read_data);
		} else {
			tmp = (read_data & 0x0000007f) + 16;
			read_data >>= 8;
			read_data &= 0xffffff80;
			read_data |= tmp;
			read_data <<= 8;
			read_data &= 0xffffff80;
			read_data |= tmp;
			sys_reg_write(TRGMII_7623_base, read_data);
		}
		read_data &= 0x000000ff;
		/* Disable RX clock gating in MT7623 */
		reg_bit_one(TRGMII_7623_base + 0x04, 30, 2);
		for (i = 0; i < 5; i++)
			reg_bit_one(TRGMII_7623_RD_0 + i * 8, 31, 1);
	}
	/* Read RD_WD MT7623 */
	for (i = 0; i < 5; i++) {
		temp_addr = TRGMII_7623_RD_0 + i * 8;
		rd_tap = 0;
		while (err_flag[i] != 0 && rd_tap != 128) {
			/* Enable EDGE CHK in MT7623 */
			tmp = sys_reg_read(temp_addr);
			tmp |= 0x40000000;
			reg_bit_zero(temp_addr, 28, 4);
			reg_bit_one(temp_addr, 30, 1);
			wait_loop();
			read_data = sys_reg_read(temp_addr);
			/* Read MT7623 Errcnt */
			err_cnt[i] = (read_data >> 8) & 0x0000000f;
			rd_wd = (read_data >> 16) & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;
			/* Disable EDGE CHK in MT7623 */
			reg_bit_zero(temp_addr, 28, 2);
			reg_bit_zero(temp_addr, 31, 1);
			tmp |= 0x40000000;
			sys_reg_write(temp_addr, tmp & 0x4fffffff);
			wait_loop();
			if (err_flag[i] != 0) {
				/* Add RXD delay in MT7623 */
				rd_tap = (read_data & 0x7f) + rxd_step_size;

				read_data = (read_data & 0xffffff80) | rd_tap;
				sys_reg_write(temp_addr, read_data);
				tap_a[i] = rd_tap;
			} else {
				rd_tap = (read_data & 0x0000007f) + 48;
				read_data = (read_data & 0xffffff80) | rd_tap;
				sys_reg_write(temp_addr, read_data);
			}
		}
		pr_info("MT7620 %dth bit  Tap_a = %d\n", i, tap_a[i]);
	}
	for (i = 0; i < 5; i++) {
		while ((err_flag[i] == 0) && (rd_tap != 128)) {
			read_data = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			/* Add RXD delay in MT7623 */
			rd_tap = (read_data & 0x7f) + rxd_step_size;

			read_data = (read_data & 0xffffff80) | rd_tap;
			sys_reg_write(TRGMII_7623_RD_0 + i * 8, read_data);

			/* Enable EDGE CHK in MT7623 */
			tmp = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			tmp |= 0x40000000;
			sys_reg_write(TRGMII_7623_RD_0 + i * 8,
				      (tmp & 0x4fffffff));
			wait_loop();
			read_data = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			/* Read MT7623 Errcnt */
			err_cnt[i] = (read_data >> 8) & 0xf;
			rd_wd = (read_data >> 16) & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;

			/* Disable EDGE CHK in MT7623 */
			tmp = sys_reg_read(TRGMII_7623_RD_0 + i * 8);
			tmp |= 0x40000000;
			sys_reg_write(TRGMII_7623_RD_0 + i * 8,
				      (tmp & 0x4fffffff));
			wait_loop();
		}
		tap_b[i] = rd_tap;	/* -rxd_step_size; */
		pr_info("MT7620 %dth bit  Tap_b = %d\n", i, tap_b[i]);
		/* Calculate RXD delay = (TAP_A + TAP_B)/2 */
		final_tap[i] = (tap_a[i] + tap_b[i]) / 2;
		read_data = (read_data & 0xffffff80) | final_tap[i];
		sys_reg_write(TRGMII_7623_RD_0 + i * 8, read_data);
	}

	mii_mgr_read(0x1F, 0x7A40, &read_data);
	read_data &= 0x3fffffff;
	mii_mgr_write(0x1F, 0x7A40, read_data);
}

static void trgmii_calibration_7530(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	unsigned int tap_a[5] = {
		0, 0, 0, 0, 0
	};
	unsigned int tap_b[5] = {
		0, 0, 0, 0, 0
	};
	unsigned int final_tap[5];
	unsigned int rxc_step_size;
	unsigned int rxd_step_size;
	unsigned int read_data;
	unsigned int tmp = 0;
	int i;
	unsigned int err_cnt[5];
	unsigned int rd_wd;
	unsigned int init_toggle_data;
	unsigned int err_flag[5];
	unsigned int err_total_flag;
	unsigned int training_word;
	unsigned int rd_tap;

	void __iomem *TRGMII_7623_base;
	u32 TRGMII_7530_RD_0;
	u32 TRGMII_7530_base;
	u32 TRGMII_7530_TX_base;

	TRGMII_7623_base = ETHDMASYS_ETH_SW_BASE + 0x0300;
	TRGMII_7530_base = 0x7A00;
	TRGMII_7530_RD_0 = TRGMII_7530_base + 0x10;
	rxd_step_size = 0x1;
	rxc_step_size = 0x8;
	init_toggle_data = 0x00000055;
	training_word = 0x000000AC;

	TRGMII_7530_TX_base = TRGMII_7530_base + 0x50;

	reg_bit_one(TRGMII_7623_base + 0x40, 31, 1);
	mii_mgr_read(0x1F, 0x7a10, &read_data);

	/* RX clock gating in MT7530 */
	mii_mgr_read(0x1F, TRGMII_7530_base + 0x04, &read_data);
	read_data &= 0x3fffffff;
	mii_mgr_write(0x1F, TRGMII_7530_base + 0x04, read_data);

	/* Set TX OE edge in  MT7530 */
	mii_mgr_read(0x1F, TRGMII_7530_base + 0x78, &read_data);
	read_data |= 0x00002000;
	mii_mgr_write(0x1F, TRGMII_7530_base + 0x78, read_data);

	/* Assert RX  reset in MT7530 */
	mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);
	read_data |= 0x80000000;
	mii_mgr_write(0x1F, TRGMII_7530_base, read_data);

	/* Release RX reset in MT7530 */
	mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);
	read_data &= 0x7fffffff;
	mii_mgr_write(0x1F, TRGMII_7530_base, read_data);

	/* Disable RX clock gating in MT7530 */
	mii_mgr_read(0x1F, TRGMII_7530_base + 0x04, &read_data);
	read_data |= 0xC0000000;
	mii_mgr_write(0x1F, TRGMII_7530_base + 0x04, read_data);

	/*Enable Training Mode in MT7623 */
	reg_bit_zero(TRGMII_7623_base + 0x40, 30, 1);
	if (ei_local->architecture & GE1_TRGMII_FORCE_2000)
		reg_bit_one(TRGMII_7623_base + 0x40, 30, 2);
	else
		reg_bit_one(TRGMII_7623_base + 0x40, 31, 1);
	reg_bit_zero(TRGMII_7623_base + 0x78, 8, 4);
	reg_bit_zero(TRGMII_7623_base + 0x50, 8, 4);
	reg_bit_zero(TRGMII_7623_base + 0x58, 8, 4);
	reg_bit_zero(TRGMII_7623_base + 0x60, 8, 4);
	reg_bit_zero(TRGMII_7623_base + 0x68, 8, 4);
	reg_bit_zero(TRGMII_7623_base + 0x70, 8, 4);
	reg_bit_one(TRGMII_7623_base + 0x78, 11, 1);

	err_total_flag = 0;
	read_data = 0x0;
	while (err_total_flag == 0 && (read_data != 0x68)) {
		/* Enable EDGE CHK in MT7530 */
		for (i = 0; i < 5; i++) {
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &err_cnt[i]);
			err_cnt[i] >>= 8;
			err_cnt[i] &= 0x0000ff0f;
			rd_wd = err_cnt[i] >> 8;
			rd_wd &= 0x000000ff;
			err_cnt[i] &= 0x0000000f;
			if (err_cnt[i] != 0)
				err_flag[i] = 1;
			else if (rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;

			if (i == 0)
				err_total_flag = err_flag[i];
			else
				err_total_flag = err_flag[i] & err_total_flag;
			/* Disable EDGE CHK in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
		}
		/*Adjust RXC delay */
		if (err_total_flag == 0) {
			/* Assert RX  reset in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);
			read_data |= 0x80000000;
			mii_mgr_write(0x1F, TRGMII_7530_base, read_data);

			/* RX clock gating in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_base + 0x04, &read_data);
			read_data &= 0x3fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_base + 0x04, read_data);

			mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);
			tmp = read_data;
			tmp &= 0x0000007f;
			tmp += rxc_step_size;
			read_data &= 0xffffff80;
			read_data |= tmp;
			mii_mgr_write(0x1F, TRGMII_7530_base, read_data);
			mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);

			/* Release RX reset in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_base, &read_data);
			read_data &= 0x7fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_base, read_data);

			/* Disable RX clock gating in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_base + 0x04, &read_data);
			read_data |= 0xc0000000;
			mii_mgr_write(0x1F, TRGMII_7530_base + 0x04, read_data);
		}
		read_data = tmp;
	}
	/* Read RD_WD MT7530 */
	for (i = 0; i < 5; i++) {
		rd_tap = 0;
		while (err_flag[i] != 0 && rd_tap != 128) {
			/* Enable EDGE CHK in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
			err_cnt[i] = (read_data >> 8) & 0x0000000f;
			rd_wd = (read_data >> 16) & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;

			if (err_flag[i] != 0) {
				/* Add RXD delay in MT7530 */
				rd_tap = (read_data & 0x7f) + rxd_step_size;
				read_data = (read_data & 0xffffff80) | rd_tap;
				mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
					      read_data);
				tap_a[i] = rd_tap;
			} else {
				/* Record the min delay TAP_A */
				tap_a[i] = (read_data & 0x0000007f);
				rd_tap = tap_a[i] + 0x4;
				read_data = (read_data & 0xffffff80) | rd_tap;
				mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
					      read_data);
			}

			/* Disable EDGE CHK in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
		}
		pr_info("MT7530 %dth bit  Tap_a = %d\n", i, tap_a[i]);
	}
	for (i = 0; i < 5; i++) {
		rd_tap = 0;
		while (err_flag[i] == 0 && (rd_tap != 128)) {
			/* Enable EDGE CHK in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
			err_cnt[i] = (read_data >> 8) & 0x0000000f;
			rd_wd = (read_data >> 16) & 0x000000ff;
			if (err_cnt[i] != 0 || rd_wd != 0x55)
				err_flag[i] = 1;
			else
				err_flag[i] = 0;

			if (err_flag[i] == 0 && (rd_tap != 128)) {
				/* Add RXD delay in MT7530 */
				rd_tap = (read_data & 0x7f) + rxd_step_size;
				read_data = (read_data & 0xffffff80) | rd_tap;
				mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
					      read_data);
			}
			/* Disable EDGE CHK in MT7530 */
			mii_mgr_read(0x1F, TRGMII_7530_RD_0 + i * 8,
				     &read_data);
			read_data |= 0x40000000;
			read_data &= 0x4fffffff;
			mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8,
				      read_data);
			wait_loop();
		}
		tap_b[i] = rd_tap;	/* - rxd_step_size; */
		pr_info("MT7530 %dth bit  Tap_b = %d\n", i, tap_b[i]);
		/* Calculate RXD delay = (TAP_A + TAP_B)/2 */
		final_tap[i] = (tap_a[i] + tap_b[i]) / 2;
		read_data = (read_data & 0xffffff80) | final_tap[i];
		mii_mgr_write(0x1F, TRGMII_7530_RD_0 + i * 8, read_data);
	}
	if (ei_local->architecture & GE1_TRGMII_FORCE_2000)
		reg_bit_zero(TRGMII_7623_base + 0x40, 31, 1);
	else
		reg_bit_zero(TRGMII_7623_base + 0x40, 30, 2);
}

#if 0
static void mt7530_trgmii_clock_setting(u32 xtal_mode)
{
	u32 reg_value;
	/* TRGMII Clock */
	mii_mgr_write_cl45(0, 0x1f, 0x410, 0x1);
	if (xtal_mode == 1) {	/* 25MHz */
		mii_mgr_write_cl45(0, 0x1f, 0x404, MT7530_TRGMII_PLL_25M);
	} else if (xtal_mode == 2) {	/* 40MHz */
		mii_mgr_write_cl45(0, 0x1f, 0x404, MT7530_TRGMII_PLL_40M);
	}
	mii_mgr_write_cl45(0, 0x1f, 0x405, 0);
	if (xtal_mode == 1)	/* 25MHz */
		mii_mgr_write_cl45(0, 0x1f, 0x409, 0x57);
	else
		mii_mgr_write_cl45(0, 0x1f, 0x409, 0x87);

	if (xtal_mode == 1)	/* 25MHz */
		mii_mgr_write_cl45(0, 0x1f, 0x40a, 0x57);
	else
		mii_mgr_write_cl45(0, 0x1f, 0x40a, 0x87);

	mii_mgr_write_cl45(0, 0x1f, 0x403, 0x1800);
	mii_mgr_write_cl45(0, 0x1f, 0x403, 0x1c00);
	mii_mgr_write_cl45(0, 0x1f, 0x401, 0xc020);
	mii_mgr_write_cl45(0, 0x1f, 0x406, 0xa030);
	mii_mgr_write_cl45(0, 0x1f, 0x406, 0xa038);
	usleep_range(120, 130);	/* for MT7623 bring up test */
	mii_mgr_write_cl45(0, 0x1f, 0x410, 0x3);

	mii_mgr_read(31, 0x7830, &reg_value);
	reg_value &= 0xFFFFFFFC;
	reg_value |= 0x00000001;
	mii_mgr_write(31, 0x7830, reg_value);

	mii_mgr_read(31, 0x7a40, &reg_value);
	reg_value &= ~(0x1 << 30);
	reg_value &= ~(0x1 << 28);
	mii_mgr_write(31, 0x7a40, reg_value);

	mii_mgr_write(31, 0x7a78, 0x55);
	usleep_range(100, 110);	/* for mt7623 bring up test */

	/* Release MT7623 RXC reset */
	reg_bit_zero(ETHDMASYS_ETH_SW_BASE + 0x0300, 31, 1);

	trgmii_calibration_7623();
	trgmii_calibration_7530();
	/* Assert RX  reset in MT7623 */
	reg_bit_one(ETHDMASYS_ETH_SW_BASE + 0x0300, 31, 1);
	/* Release RX reset in MT7623 */
	reg_bit_zero(ETHDMASYS_ETH_SW_BASE + 0x0300, 31, 1);
	mii_mgr_read(31, 0x7a00, &reg_value);
	reg_value |= (0x1 << 31);
	mii_mgr_write(31, 0x7a00, reg_value);
	mdelay(1);
	reg_value &= ~(0x1 << 31);
	mii_mgr_write(31, 0x7a00, reg_value);
	mdelay(100);
}
#endif

void trgmii_set_7621(void)
{
	u32 val = 0;
	u32 val_0 = 0;

	val = sys_reg_read(RSTCTRL);
	/* MT7621 need to reset GMAC and FE first */
	val = val | RALINK_FE_RST | RALINK_ETH_RST;
	sys_reg_write(RSTCTRL, val);

	/* set TRGMII clock */
	val_0 = sys_reg_read(CLK_CFG_0);
	val_0 &= 0xffffff9f;
	val_0 |= (0x1 << 5);
	sys_reg_write(CLK_CFG_0, val_0);
	mdelay(1);
	val_0 = sys_reg_read(CLK_CFG_0);
	pr_info("set CLK_CFG_0 = 0x%x!!!!!!!!!!!!!!!!!!1\n", val_0);
	val = val & ~(RALINK_FE_RST | RALINK_ETH_RST);
	sys_reg_write(RSTCTRL, val);
	pr_info("trgmii_set_7621 Completed!!\n");
}

void trgmii_set_7530(void)
{
	u32 regValue;

	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x404);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_read(31, 0x7800, &regValue);
	regValue = (regValue >> 9) & 0x3;
	if (regValue == 0x3)
		mii_mgr_write(0, 14, 0x0C00);/*25Mhz XTAL for 150Mhz CLK */
	 else if (regValue == 0x2)
		mii_mgr_write(0, 14, 0x0780);/*40Mhz XTAL for 150Mhz CLK */

	mdelay(1);

	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x409);
	mii_mgr_write(0, 13, 0x401f);
	if (regValue == 0x3) /* 25MHz */
		mii_mgr_write(0, 14, 0x57);
	else
		mii_mgr_write(0, 14, 0x87);
	mdelay(1);

	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x40a);
	mii_mgr_write(0, 13, 0x401f);
	if (regValue == 0x3) /* 25MHz */
		mii_mgr_write(0, 14, 0x57);
	else
		mii_mgr_write(0, 14, 0x87);

/* PLL BIAS en */
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x403);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x1800);
	mdelay(1);

/* BIAS LPF en */
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x403);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x1c00);

/* sys PLL en */
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x401);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0xc020);

/* LCDDDS PWDS */
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x406);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0xa030);
	mdelay(1);

/* GSW_2X_CLK */
	mii_mgr_write(0, 13, 0x1f);
	mii_mgr_write(0, 14, 0x410);
	mii_mgr_write(0, 13, 0x401f);
	mii_mgr_write(0, 14, 0x0003);
	mii_mgr_write_cl45(0, 0x1f, 0x410, 0x0003);

/* enable P6 */
	mii_mgr_write(31, 0x3600, 0x5e33b);

/* enable TRGMII */
	mii_mgr_write(31, 0x7830, 0x1);

	pr_info("trgmii_set_7530 Completed!!\n");
}

#if !defined (CONFIG_RAETH_ESW_CONTROL)
static void is_switch_vlan_table_busy(void)
{
	int j = 0;
	unsigned int value = 0;

	for (j = 0; j < 20; j++) {
		mii_mgr_read(31, 0x90, &value);
		if ((value & 0x80000000) == 0) {	/* table busy */
			break;
		}
		mdelay(70);
	}
	if (j == 20)
		pr_info("set vlan timeout value=0x%x.\n", value);
}

static void lan_wan_partition(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	/*Set  MT7530 */
	if (ei_local->architecture & WAN_AT_P0) {
		pr_info("set LAN/WAN WLLLL\n");
		/*WLLLL, wan at P0 */
		/*LAN/WAN ports as security mode */
		mii_mgr_write(31, 0x2004, 0xff0003);	/* port0 */
		mii_mgr_write(31, 0x2104, 0xff0003);	/* port1 */
		mii_mgr_write(31, 0x2204, 0xff0003);	/* port2 */
		mii_mgr_write(31, 0x2304, 0xff0003);	/* port3 */
		mii_mgr_write(31, 0x2404, 0xff0003);	/* port4 */
		mii_mgr_write(31, 0x2504, 0xff0003);	/* port5 */
		mii_mgr_write(31, 0x2604, 0xff0003);	/* port6 */

		/*set PVID */
		mii_mgr_write(31, 0x2014, 0x10002);	/* port0 */
		mii_mgr_write(31, 0x2114, 0x10001);	/* port1 */
		mii_mgr_write(31, 0x2214, 0x10001);	/* port2 */
		mii_mgr_write(31, 0x2314, 0x10001);	/* port3 */
		mii_mgr_write(31, 0x2414, 0x10001);	/* port4 */
		mii_mgr_write(31, 0x2514, 0x10002);	/* port5 */
		mii_mgr_write(31, 0x2614, 0x10001);	/* port6 */
		/*port6 */
		/*VLAN member */
		is_switch_vlan_table_busy();
		mii_mgr_write(31, 0x94, 0x405e0001);	/* VAWD1 */
		mii_mgr_write(31, 0x90, 0x80001001);	/* VTCR, VID=1 */
		is_switch_vlan_table_busy();

		mii_mgr_write(31, 0x94, 0x40210001);	/* VAWD1 */
		mii_mgr_write(31, 0x90, 0x80001002);	/* VTCR, VID=2 */
		is_switch_vlan_table_busy();
	}
	if (ei_local->architecture & WAN_AT_P4) {
		pr_info("set LAN/WAN LLLLW\n");
		/*LLLLW, wan at P4 */
		/*LAN/WAN ports as security mode */
		mii_mgr_write(31, 0x2004, 0xff0003);	/* port0 */
		mii_mgr_write(31, 0x2104, 0xff0003);	/* port1 */
		mii_mgr_write(31, 0x2204, 0xff0003);	/* port2 */
		mii_mgr_write(31, 0x2304, 0xff0003);	/* port3 */
		mii_mgr_write(31, 0x2404, 0xff0003);	/* port4 */
		mii_mgr_write(31, 0x2504, 0xff0003);	/* port5 */
		mii_mgr_write(31, 0x2604, 0xff0003);	/* port6 */

		/*set PVID */
		mii_mgr_write(31, 0x2014, 0x10001);	/* port0 */
		mii_mgr_write(31, 0x2114, 0x10001);	/* port1 */
		mii_mgr_write(31, 0x2214, 0x10001);	/* port2 */
		mii_mgr_write(31, 0x2314, 0x10001);	/* port3 */
		mii_mgr_write(31, 0x2414, 0x10002);	/* port4 */
		mii_mgr_write(31, 0x2514, 0x10002);	/* port5 */
		mii_mgr_write(31, 0x2614, 0x10001);	/* port6 */

		/*VLAN member */
		is_switch_vlan_table_busy();
		mii_mgr_write(31, 0x94, 0x404f0001);	/* VAWD1 */
		mii_mgr_write(31, 0x90, 0x80001001);	/* VTCR, VID=1 */
		is_switch_vlan_table_busy();
		mii_mgr_write(31, 0x94, 0x40300001);	/* VAWD1 */
		mii_mgr_write(31, 0x90, 0x80001002);	/* VTCR, VID=2 */
		is_switch_vlan_table_busy();
	}
}
#endif

static void mt7530_phy_setting(void)
{
	u32 i;
	u32 reg_value;

	for (i = 0; i < 5; i++) {
		/* Disable EEE */
		mii_mgr_write_cl45(i, 0x7, 0x3c, 0);
		/* Enable HW auto downshift */
		mii_mgr_write(i, 31, 0x1);
		mii_mgr_read(i, 0x14, &reg_value);
		reg_value |= (1 << 4);
		mii_mgr_write(i, 0x14, reg_value);
		/* Increase SlvDPSready time */
		mii_mgr_write(i, 31, 0x52b5);
		mii_mgr_write(i, 16, 0xafae);
		mii_mgr_write(i, 18, 0x2f);
		mii_mgr_write(i, 16, 0x8fae);
		/* Incease post_update_timer */
		mii_mgr_write(i, 31, 0x3);
		mii_mgr_write(i, 17, 0x4b);
		/* Adjust 100_mse_threshold */
		mii_mgr_write_cl45(i, 0x1e, 0x123, 0xffff);
		/* Disable mcc */
		mii_mgr_write_cl45(i, 0x1e, 0xa6, 0x300);
	}
}

#if 0
static void setup_internal_gsw(void)
{
	void __iomem *gpio_base_virt = ioremap(ETH_GPIO_BASE, 0x1000);
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	u32 reg_value;
	u32 xtal_mode;
	u32 i;

	if (ei_local->architecture &
	    (GE1_TRGMII_FORCE_2000 | GE1_TRGMII_FORCE_2600))
		reg_bit_one(RALINK_SYSCTL_BASE + 0x2c, 11, 1);
	else
		reg_bit_zero(RALINK_SYSCTL_BASE + 0x2c, 11, 1);
	reg_bit_one(ETHDMASYS_ETH_SW_BASE + 0x0390, 1, 1);	/* TRGMII mode */

	/*Hardware reset Switch */

	reg_bit_zero((void __iomem *)gpio_base_virt + 0x520, 1, 1);
	mdelay(1);
	reg_bit_one((void __iomem *)gpio_base_virt + 0x520, 1, 1);
	mdelay(100);

	/* Assert MT7623 RXC reset */
	reg_bit_one(ETHDMASYS_ETH_SW_BASE + 0x0300, 31, 1);
	/*For MT7623 reset MT7530 */
	reg_bit_one(RALINK_SYSCTL_BASE + 0x34, 2, 1);
	mdelay(1);
	reg_bit_zero(RALINK_SYSCTL_BASE + 0x34, 2, 1);
	mdelay(100);

	/* Wait for Switch Reset Completed */
	for (i = 0; i < 100; i++) {
		mdelay(10);
		mii_mgr_read(31, 0x7800, &reg_value);
		if (reg_value != 0) {
			pr_info("MT7530 Reset Completed!!\n");
			break;
		}
		if (i == 99)
			pr_info("MT7530 Reset Timeout!!\n");
	}

	for (i = 0; i <= 4; i++) {
		/*turn off PHY */
		mii_mgr_read(i, 0x0, &reg_value);
		reg_value |= (0x1 << 11);
		mii_mgr_write(i, 0x0, reg_value);
	}
	mii_mgr_write(31, 0x7000, 0x3);	/* reset switch */
	usleep_range(100, 110);

	/* (GE1, Force 1000M/FD, FC ON) */
	sys_reg_write(RALINK_ETH_SW_BASE + 0x100, 0x2105e33b);
	mii_mgr_write(31, 0x3600, 0x5e33b);
	mii_mgr_read(31, 0x3600, &reg_value);
	/* (GE2, Link down) */
	sys_reg_write(RALINK_ETH_SW_BASE + 0x200, 0x00008000);

	mii_mgr_read(31, 0x7804, &reg_value);
	reg_value &= ~(1 << 8);	/* Enable Port 6 */
	reg_value |= (1 << 6);	/* Disable Port 5 */
	reg_value |= (1 << 13);	/* Port 5 as GMAC, no Internal PHY */

	if (ei_local->architecture & GMAC2) {
		/*RGMII2=Normal mode */
		reg_bit_zero(RALINK_SYSCTL_BASE + 0x60, 15, 1);

		/*GMAC2= RGMII mode */
		reg_bit_zero(SYSCFG1, 14, 2);
		if (ei_local->architecture & GE2_RGMII_AN) {
			mii_mgr_write(31, 0x3500, 0x56300);
			/* (GE2, auto-polling) */
			sys_reg_write(RALINK_ETH_SW_BASE + 0x200, 0x21056300);
			reg_value |= (1 << 6);	/* disable MT7530 P5 */
			enable_auto_negotiate(ei_local);

		} else {
			/* MT7530 P5 Force 1000 */
			mii_mgr_write(31, 0x3500, 0x5e33b);
			/* (GE2, Force 1000) */
			sys_reg_write(RALINK_ETH_SW_BASE + 0x200, 0x2105e33b);
			reg_value &= ~(1 << 6);	/* enable MT7530 P5 */
			reg_value |= ((1 << 7) | (1 << 13) | (1 << 16));
			if (ei_local->architecture & WAN_AT_P0)
				reg_value |= (1 << 20);
			else
				reg_value &= ~(1 << 20);
		}
	}
	reg_value &= ~(1 << 5);
	reg_value |= (1 << 16);	/* change HW-TRAP */
	pr_info("change HW-TRAP to 0x%x\n", reg_value);
	mii_mgr_write(31, 0x7804, reg_value);
	mii_mgr_read(31, 0x7800, &reg_value);
	reg_value = (reg_value >> 9) & 0x3;
	if (reg_value == 0x3) {	/* 25Mhz Xtal */
		xtal_mode = 1;
		/*Do Nothing */
	} else if (reg_value == 0x2) {	/* 40Mhz */
		xtal_mode = 2;
		/* disable MT7530 core clock */
		mii_mgr_write_cl45(0, 0x1f, 0x410, 0x0);

		mii_mgr_write_cl45(0, 0x1f, 0x40d, 0x2020);
		mii_mgr_write_cl45(0, 0x1f, 0x40e, 0x119);
		mii_mgr_write_cl45(0, 0x1f, 0x40d, 0x2820);
		usleep_range(20, 30);	/* suggest by CD */
	#if defined(CONFIG_GE1_RGMII_FORCE_1200)
		mii_mgr_write_cl45(0, 0x1f, 0x410, 0x3);
	#else
		mii_mgr_write_cl45(0, 0x1f, 0x410, 0x1);
	#endif

	} else {
		xtal_mode = 3;
	 /* TODO */}

	/* set MT7530 central align */
	#if !defined(CONFIG_GE1_RGMII_FORCE_1200)  /* for RGMII 1000HZ */
	mii_mgr_read(31, 0x7830, &reg_value);
	reg_value &= ~1;
	reg_value |= 1 << 1;
	mii_mgr_write(31, 0x7830, reg_value);

	mii_mgr_read(31, 0x7a40, &reg_value);
	reg_value &= ~(1 << 30);
	mii_mgr_write(31, 0x7a40, reg_value);

	reg_value = 0x855;
	mii_mgr_write(31, 0x7a78, reg_value);
	#endif

	mii_mgr_write(31, 0x7b00, 0x104);	/* delay setting for 10/1000M */
	mii_mgr_write(31, 0x7b04, 0x10);	/* delay setting for 10/1000M */

	/*Tx Driving */
	mii_mgr_write(31, 0x7a54, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7a5c, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7a64, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7a6c, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7a74, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7a7c, 0x88);	/* lower GE1 driving */
	mii_mgr_write(31, 0x7810, 0x11);	/* lower GE2 driving */
	/*Set MT7623 TX Driving */
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0354, 0x88);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x035c, 0x88);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0364, 0x88);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x036c, 0x88);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0374, 0x88);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x037c, 0x88);

	/* Set GE2 driving and slew rate */
	if (ei_local->architecture & GE2_RGMII_AN)
		sys_reg_write((void __iomem *)gpio_base_virt + 0xf00, 0xe00);
	else
		sys_reg_write((void __iomem *)gpio_base_virt + 0xf00, 0xa00);
	/* set GE2 TDSEL */
	sys_reg_write((void __iomem *)gpio_base_virt + 0x4c0, 0x5);
	/* set GE2 TUNE */
	sys_reg_write((void __iomem *)gpio_base_virt + 0xed0, 0);

	if (ei_local->architecture & GE1_RGMII_FORCE_1000) {
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0350, 0x55);
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0358, 0x55);
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0360, 0x55);
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0368, 0x55);
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0370, 0x55);
		sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x0378, 0x855);
	}

#if !defined (CONFIG_RAETH_ESW_CONTROL)
	lan_wan_partition();
#endif
	mt7530_phy_setting();
	for (i = 0; i <= 4; i++) {
		/*turn on PHY */
		mii_mgr_read(i, 0x0, &reg_value);
		reg_value &= ~(0x1 << 11);
		mii_mgr_write(i, 0x0, reg_value);
	}

	mii_mgr_read(31, 0x7808, &reg_value);
	reg_value |= (3 << 16);	/* Enable INTR */
	mii_mgr_write(31, 0x7808, reg_value);

	iounmap(gpio_base_virt);
}
#endif

void setup_external_gsw(void)
{
	/* reduce RGMII2 PAD driving strength */
	reg_bit_zero(PAD_RGMII2_MDIO_CFG, 4, 2);
	/*enable MDIO */
	reg_bit_zero(RALINK_SYSCTL_BASE + 0x60, 12, 2);

	/*RGMII1=Normal mode */
	reg_bit_zero(RALINK_SYSCTL_BASE + 0x60, 14, 1);
	/*GMAC1= RGMII mode */
	reg_bit_zero(SYSCFG1, 12, 2);

	/* (GE1, Link down) */
	sys_reg_write(RALINK_ETH_SW_BASE + 0x100, 0x00008000);

	/*RGMII2=Normal mode */
	reg_bit_zero(RALINK_SYSCTL_BASE + 0x60, 15, 1);
	/*GMAC2= RGMII mode */
	reg_bit_zero(SYSCFG1, 14, 2);

	/* (GE2, Force 1000M/FD, FC ON) */
	sys_reg_write(RALINK_ETH_SW_BASE + 0x200, 0x2105e33b);

} int is_marvell_gigaphy(int ge)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	u32 phy_id0 = 0, phy_id1 = 0, phy_address;

	if (ei_local->architecture & GE1_RGMII_AN)
		phy_address = mac_to_gigaphy_mode_addr;
	else
		phy_address = mac_to_gigaphy_mode_addr2;

	if (!mii_mgr_read(phy_address, 2, &phy_id0)) {
		pr_info("\n Read PhyID 1 is Fail!!\n");
		phy_id0 = 0;
	}
	if (!mii_mgr_read(phy_address, 3, &phy_id1)) {
		pr_info("\n Read PhyID 1 is Fail!!\n");
		phy_id1 = 0;
	}

	if ((phy_id0 == EV_MARVELL_PHY_ID0) && (phy_id1 == EV_MARVELL_PHY_ID1))
		return 1;
	return 0;
}

int is_vtss_gigaphy(int ge)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	u32 phy_id0 = 0, phy_id1 = 0, phy_address;

	if (ei_local->architecture & GE1_RGMII_AN)
		phy_address = mac_to_gigaphy_mode_addr;
	else
		phy_address = mac_to_gigaphy_mode_addr2;

	if (!mii_mgr_read(phy_address, 2, &phy_id0)) {
		pr_info("\n Read PhyID 1 is Fail!!\n");
		phy_id0 = 0;
	}
	if (!mii_mgr_read(phy_address, 3, &phy_id1)) {
		pr_info("\n Read PhyID 1 is Fail!!\n");
		phy_id1 = 0;
	}

	if ((phy_id0 == EV_VTSS_PHY_ID0) && (phy_id1 == EV_VTSS_PHY_ID1))
		return 1;
	return 0;
}

void fe_sw_preinit(struct END_DEVICE *ei_local)
{
	struct device_node *np = ei_local->switch_np;
	struct platform_device *pdev = of_find_device_by_node(np);
	struct mtk_gsw *gsw;
	int ret;

	gsw = platform_get_drvdata(pdev);
	if (!gsw) {
		pr_info("Failed to get gsw\n");
		return;
	}

	regulator_set_voltage(gsw->supply, 1000000, 1000000);
	ret = regulator_enable(gsw->supply);
	if (ret)
		pr_info("Failed to enable mt7530 power: %d\n", ret);

	if (gsw->mcm) {
		regulator_set_voltage(gsw->b3v, 3300000, 3300000);
		ret = regulator_enable(gsw->b3v);
		if (ret)
			dev_err(&pdev->dev, "Failed to enable b3v: %d\n", ret);
	} else {
		ret = devm_gpio_request(&pdev->dev, gsw->reset_pin,
					"mediatek,reset-pin");
		if (ret)
			pr_info("fail to devm_gpio_request\n");

		gpio_direction_output(gsw->reset_pin, 0);
		usleep_range(1000, 1100);
		gpio_set_value(gsw->reset_pin, 1);
		mdelay(100);
		devm_gpio_free(&pdev->dev, gsw->reset_pin);
	}
}

int init_rtl8367s(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	static int switch_init;
	rtk_vlan_cfg_t vlan1, vlan2;

	if (switch_init)
		return 0;
#define GPIO_MODE2 0x10211330
#define GPIO_DIR1 0x10211010
#define GPIO_DOUT1 0x10211110

	rtk_hal_switch_init();

	switch_init = 1;

	/* Set LAN/WAN VLAN partition */
	memset(&vlan1, 0x00, sizeof(rtk_vlan_cfg_t));
	RTK_PORTMASK_PORT_SET(vlan1.mbr, EXT_PORT0);
	RTK_PORTMASK_PORT_SET(vlan1.mbr, UTP_PORT1);
	RTK_PORTMASK_PORT_SET(vlan1.mbr, UTP_PORT2);
	RTK_PORTMASK_PORT_SET(vlan1.mbr, UTP_PORT3);
	RTK_PORTMASK_PORT_SET(vlan1.untag, EXT_PORT0);
	RTK_PORTMASK_PORT_SET(vlan1.untag, UTP_PORT1);
	RTK_PORTMASK_PORT_SET(vlan1.untag, UTP_PORT2);
	RTK_PORTMASK_PORT_SET(vlan1.untag, UTP_PORT3);
	if (ei_local->architecture & WAN_AT_P0) {
		RTK_PORTMASK_PORT_SET(vlan1.mbr, UTP_PORT4);
		RTK_PORTMASK_PORT_SET(vlan1.untag, UTP_PORT4);
	} else {
		RTK_PORTMASK_PORT_SET(vlan1.mbr, UTP_PORT0);
		RTK_PORTMASK_PORT_SET(vlan1.untag, UTP_PORT0);
	}
	vlan1.ivl_en = 1;
	rtk_vlan_set(1, &vlan1);

	memset(&vlan2, 0x00, sizeof(rtk_vlan_cfg_t));
	RTK_PORTMASK_PORT_SET(vlan2.mbr, EXT_PORT1);
	RTK_PORTMASK_PORT_SET(vlan2.untag, EXT_PORT1);
	if (ei_local->architecture & WAN_AT_P0) {
		RTK_PORTMASK_PORT_SET(vlan2.mbr, UTP_PORT0);
		RTK_PORTMASK_PORT_SET(vlan2.untag, UTP_PORT0);
	} else {
		RTK_PORTMASK_PORT_SET(vlan2.mbr, UTP_PORT4);
		RTK_PORTMASK_PORT_SET(vlan2.untag, UTP_PORT4);
	}
	vlan2.ivl_en = 1;
	rtk_vlan_set(2, &vlan2);

	rtk_hal_vlan_portpvid_set(EXT_PORT0, 1, 0);
	rtk_hal_vlan_portpvid_set(UTP_PORT1, 1, 0);
	rtk_hal_vlan_portpvid_set(UTP_PORT2, 1, 0);
	rtk_hal_vlan_portpvid_set(UTP_PORT3, 1, 0);
	rtk_hal_vlan_portpvid_set(EXT_PORT1, 2, 0);
	if (ei_local->architecture & WAN_AT_P0) {
		rtk_hal_vlan_portpvid_set(UTP_PORT0, 2, 0);
		rtk_hal_vlan_portpvid_set(UTP_PORT4, 1, 0);
	} else {
		rtk_hal_vlan_portpvid_set(UTP_PORT0, 1, 0);
		rtk_hal_vlan_portpvid_set(UTP_PORT4, 2, 0);
	}

	return 0;
}

void set_rtl8367s_sgmii(void)
{
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;

	init_rtl8367s();
	mode = MODE_EXT_HSGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = PORT_SPEED_2500M;
	mac_cfg.duplex = PORT_FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = ENABLED;
	mac_cfg.rxpause = ENABLED;
	rtk_port_macForceLinkExt_set(EXT_PORT0, mode, &mac_cfg);
	rtk_port_sgmiiNway_set(EXT_PORT0, DISABLED);
	rtk_port_phyEnableAll_set(ENABLED);
}

void set_rtl8367s_rgmii(void)
{
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;

	init_rtl8367s();

	mode = MODE_EXT_RGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = PORT_SPEED_1000M;
	mac_cfg.duplex = PORT_FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = ENABLED;
	mac_cfg.rxpause = ENABLED;
	rtk_port_macForceLinkExt_set(EXT_PORT1, mode, &mac_cfg);
	rtk_port_rgmiiDelayExt_set(EXT_PORT1, 1, 3);
	rtk_port_phyEnableAll_set(ENABLED);
}

void set_sgmii_force_link(int port_num, int speed)
{
	void __iomem *virt_addr;
	unsigned int reg_value;
	unsigned int sgmii_reg_phya, sgmii_reg;

	virt_addr = ioremap(ETHSYS_BASE, 0x20);
	reg_value = sys_reg_read(virt_addr + 0x14);

	if (port_num == 1) {
		reg_value |= SGMII_CONFIG_0;
		sgmii_reg_phya = SGMII_REG_PHYA_BASE0;
		sgmii_reg = SGMII_REG_BASE0;
		set_ge1_force_1000();
	}
	if (port_num == 2) {
		reg_value |= SGMII_CONFIG_1;
		sgmii_reg_phya = SGMII_REG_PHYA_BASE1;
		sgmii_reg = SGMII_REG_BASE1;
		set_ge2_force_1000();
	}

	sys_reg_write(virt_addr + 0x14, reg_value);
	reg_value = sys_reg_read(virt_addr + 0x14);
	iounmap(virt_addr);

	/* Set SGMII GEN2 speed(2.5G) */
	virt_addr = ioremap(sgmii_reg_phya, 0x100);
	reg_value = sys_reg_read(virt_addr + 0x28);
	reg_value |= speed << 2;
	sys_reg_write(virt_addr + 0x28, reg_value);
	iounmap(virt_addr);

	virt_addr = ioremap(sgmii_reg, 0x100);
	/* disable SGMII AN */
	reg_value = sys_reg_read(virt_addr);
	reg_value &= ~(1 << 12);
	sys_reg_write(virt_addr, reg_value);
	/* SGMII force mode setting */
	reg_value = sys_reg_read(virt_addr + 0x20);
	sys_reg_write(virt_addr + 0x20, 0x31120019);
	reg_value = sys_reg_read(virt_addr + 0x20);
	/* Release PHYA power down state */
	reg_value = sys_reg_read(virt_addr + 0xe8);
	reg_value &= ~(1 << 4);
	sys_reg_write(virt_addr + 0xe8, reg_value);
	iounmap(virt_addr);
}

void set_sgmii_an(int port_num)
{
	void __iomem *virt_addr;
	unsigned int reg_value;
	unsigned int sgmii_reg, sgmii_reg_phya;

	virt_addr = ioremap(ETHSYS_BASE, 0x20);
	reg_value = sys_reg_read(virt_addr + 0x14);

	if (port_num == 1) {
		reg_value |= SGMII_CONFIG_0;
		sgmii_reg_phya = SGMII_REG_PHYA_BASE0;
		sgmii_reg = SGMII_REG_BASE0;
	}
	if (port_num == 2) {
		reg_value |= SGMII_CONFIG_1;
		sgmii_reg_phya = SGMII_REG_PHYA_BASE1;
		sgmii_reg = SGMII_REG_BASE1;
	}

	sys_reg_write(virt_addr + 0x14, reg_value);
	iounmap(virt_addr);

	/* set auto polling */
	virt_addr = ioremap(ETHSYS_MAC_BASE, 0x300);
	sys_reg_write(virt_addr + (0x100 * port_num), 0x21056300);
	iounmap(virt_addr);

	virt_addr = ioremap(sgmii_reg, 0x100);
	/* set link timer */
	sys_reg_write(virt_addr + 0x18, 0x186a0);
	/* disable remote fault */
	reg_value = sys_reg_read(virt_addr + 0x20);
	reg_value |= 1 << 8;
	sys_reg_write(virt_addr + 0x20, reg_value);
	/* restart an */
	reg_value = sys_reg_read(virt_addr);
	reg_value |= 1 << 9;
	sys_reg_write(virt_addr, reg_value);
	/* Release PHYA power down state */
	reg_value = sys_reg_read(virt_addr + 0xe8);
	reg_value &= ~(1 << 4);
	sys_reg_write(virt_addr + 0xe8, reg_value);
	iounmap(virt_addr);
}

void fe_sw_init(void)
{
	struct END_DEVICE *ei_local = netdev_priv(dev_raether);
	unsigned int reg_value = 0;

	/* Case1: MT7623/MT7622 GE1 + GigaPhy */
	if (ei_local->architecture & GE1_RGMII_AN) {
		enable_auto_negotiate(ei_local);
		if (is_marvell_gigaphy(1)) {
			if (ei_local->features & FE_FPGA_MODE) {
				mii_mgr_read(mac_to_gigaphy_mode_addr, 9,
					     &reg_value);
				/* turn off 1000Base-T Advertisement
				 * (9.9=1000Full, 9.8=1000Half)
				 */
				reg_value &= ~(3 << 8);
				mii_mgr_write(mac_to_gigaphy_mode_addr,
					      9, reg_value);

				/*10Mbps, debug */
				mii_mgr_write(mac_to_gigaphy_mode_addr,
					      4, 0x461);

				mii_mgr_read(mac_to_gigaphy_mode_addr, 0,
					     &reg_value);
				reg_value |= 1 << 9;	/* restart AN */
				mii_mgr_write(mac_to_gigaphy_mode_addr,
					      0, reg_value);
			}
		}
		if (is_vtss_gigaphy(1)) {
			mii_mgr_write(mac_to_gigaphy_mode_addr, 31, 1);
			mii_mgr_read(mac_to_gigaphy_mode_addr, 28,
				     &reg_value);
			pr_info("Vitesse phy skew: %x --> ", reg_value);
			reg_value |= (0x3 << 12);
			reg_value &= ~(0x3 << 14);
			pr_info("%x\n", reg_value);
			mii_mgr_write(mac_to_gigaphy_mode_addr, 28,
				      reg_value);
			mii_mgr_write(mac_to_gigaphy_mode_addr, 31, 0);
		}
	}

	/* Case2: RT3883/MT7621 GE2 + GigaPhy */
	if (ei_local->architecture & GE2_RGMII_AN) {
		enable_auto_negotiate(ei_local);
		set_ge2_an();
		set_ge2_gmii();

		if (is_marvell_gigaphy(2)) {
			mii_mgr_read(mac_to_gigaphy_mode_addr2, 9,
				     &reg_value);
			/* turn off 1000Base-T Advertisement
			 * (9.9=1000Full, 9.8=1000Half)
			 */
			reg_value &= ~(3 << 8);
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 9,
				      reg_value);

			mii_mgr_read(mac_to_gigaphy_mode_addr2, 20,
				     &reg_value);
			/* Add delay to RX_CLK for RXD Outputs */
			reg_value |= 1 << 7;
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 20,
				      reg_value);

			mii_mgr_read(mac_to_gigaphy_mode_addr2, 0,
				     &reg_value);
			reg_value |= 1 << 15;	/* PHY Software Reset */
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 0,
				      reg_value);
			if (ei_local->features & FE_FPGA_MODE) {
				mii_mgr_read(mac_to_gigaphy_mode_addr2,
					     9, &reg_value);
				/* turn off 1000Base-T Advertisement
				 * (9.9=1000Full, 9.8=1000Half)
				 */
				reg_value &= ~(3 << 8);
				mii_mgr_write(mac_to_gigaphy_mode_addr2,
					      9, reg_value);

				/*10Mbps, debug */
				mii_mgr_write(mac_to_gigaphy_mode_addr2,
					      4, 0x461);

				mii_mgr_read(mac_to_gigaphy_mode_addr2,
					     0, &reg_value);
				reg_value |= 1 << 9;	/* restart AN */
				mii_mgr_write(mac_to_gigaphy_mode_addr2,
					      0, reg_value);
			}
		}
		if (is_vtss_gigaphy(2)) {
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 31, 1);
			mii_mgr_read(mac_to_gigaphy_mode_addr2, 28,
				     &reg_value);
			pr_info("Vitesse phy skew: %x --> ", reg_value);
			reg_value |= (0x3 << 12);
			reg_value &= ~(0x3 << 14);
			pr_info("%x\n", reg_value);
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 28,
				      reg_value);
			mii_mgr_write(mac_to_gigaphy_mode_addr2, 31, 0);
		}
	}

	/* Case3: GE1 + Internal GigaSW */
	if (ei_local->architecture &
	    (GE1_RGMII_FORCE_1000 | GE1_TRGMII_FORCE_2000 |
	     GE1_TRGMII_FORCE_2600)) {
		/* TODO */
	}

	/* Case4: GE2 + GigaSW */
	if (ei_local->architecture & GE2_RGMII_FORCE_1000) {
		set_ge2_force_1000();
	}
	/*TODO
	 * else
	 * sys_reg_write(MDIO_CFG2, INIT_VALUE_OF_FORCE_1000_FD);
	 */

	/* Case5: MT7622 embedded switch */
	if (ei_local->architecture & RAETH_ESW) {
		reg_value = sys_reg_read(ETHDMASYS_ETH_MAC_BASE + 0xC);
		reg_value = reg_value | 0x1;
		sys_reg_write(ETHDMASYS_ETH_MAC_BASE + 0xC, reg_value);

	}

	if (ei_local->architecture & GE1_SGMII_FORCE_2500) {
		set_rtl8367s_sgmii();
		set_sgmii_force_link(1, 1);
	} else if (ei_local->architecture & GE1_SGMII_AN) {
		enable_auto_negotiate(ei_local);
		set_sgmii_an(1);
	}
	if (ei_local->architecture & GE2_SGMII_FORCE_2500) {
		set_sgmii_force_link(2, 1);
	} else if (ei_local->architecture & GE2_SGMII_AN) {
		enable_auto_negotiate(ei_local);
		set_sgmii_an(2);
	}

}

void fe_sw_deinit(struct END_DEVICE *ei_local)
{
	struct device_node *np = ei_local->switch_np;
	struct platform_device *pdev = of_find_device_by_node(np);
	struct mtk_gsw *gsw;
	int ret;

	gsw = platform_get_drvdata(pdev);
	if (!gsw)
		return;

	ret = regulator_disable(gsw->supply);
	if (ret)
		dev_err(&pdev->dev, "Failed to disable mt7530 power: %d\n", ret);

	if (gsw->mcm) {
		ret = regulator_disable(gsw->b3v);
		if (ret)
			dev_err(&pdev->dev, "Failed to disable b3v: %d\n", ret);
	}

}

void (*esw_link_status_hook)(u32 port_id, int port_link) = NULL;
EXPORT_SYMBOL(esw_link_status_hook);

static void esw_link_status_changed(int port_no, void *dev_id)
{
	unsigned int reg_val;

	mii_mgr_read(31, (0x3008 + (port_no * 0x100)), &reg_val);

	if (esw_link_status_hook)
		esw_link_status_hook(port_no, reg_val & 0x1);
}

irqreturn_t gsw_interrupt(int irq, void *resv)
{
	unsigned long flags;
	unsigned int reg_int_val;
	struct net_device *dev = dev_raether;
	struct END_DEVICE *ei_local = netdev_priv(dev);
	void *dev_id = NULL;

	spin_lock_irqsave(&ei_local->page_lock, flags);
	mii_mgr_read(31, 0x700c, &reg_int_val);

	if (reg_int_val & P4_LINK_CH)
		esw_link_status_changed(4, dev_id);

	if (reg_int_val & P3_LINK_CH)
		esw_link_status_changed(3, dev_id);
	if (reg_int_val & P2_LINK_CH)
		esw_link_status_changed(2, dev_id);
	if (reg_int_val & P1_LINK_CH)
		esw_link_status_changed(1, dev_id);
	if (reg_int_val & P0_LINK_CH)
		esw_link_status_changed(0, dev_id);

	mii_mgr_write(31, 0x700c, 0x1f);	/* ack switch link change */
	spin_unlock_irqrestore(&ei_local->page_lock, flags);

	return IRQ_HANDLED;
}

u32 phy_tr_dbg(u8 phyaddr, char *type, u32 data_addr, u8 ch_num)
{
	u16 page_reg = 31;
	u32 token_ring_debug_reg = 0x52B5;
	u32 token_ring_control_reg = 0x10;
	u32 token_ring_low_data_reg = 0x11;
	u32 token_ring_high_data_reg = 0x12;
	u16 ch_addr = 0;
	u32 node_addr = 0;
	u32 value = 0;
	u32 value_high = 0;
	u32 value_low = 0;

	if (strncmp(type, "DSPF", 4) == 0) {
		/* DSP Filter Debug Node*/
		ch_addr = 0x02;
		node_addr = 0x0D;
	} else if (strncmp(type, "PMA", 3) == 0) {
		/*PMA Debug Node*/
		ch_addr = 0x01;
		node_addr = 0x0F;
	} else if (strncmp(type, "TR", 2) == 0) {
		/* Timing Recovery  Debug Node */
		ch_addr = 0x01;
		node_addr = 0x0D;
	} else if (strncmp(type, "PCS", 3) == 0) {
		/* R1000PCS Debug Node */
		ch_addr = 0x02;
		node_addr = 0x0F;
	} else if (strncmp(type, "FFE", 3) == 0) {
		/* FFE Debug Node */
		ch_addr = ch_num;
		node_addr = 0x04;
	} else if (strncmp(type, "EC", 2) == 0) {
		/* ECC Debug Node */
		ch_addr = ch_num;
		node_addr = 0x00;
	} else if (strncmp(type, "ECT", 3) == 0) {
		/* EC/Tail Debug Node */
		ch_addr = ch_num;
		node_addr = 0x01;
	} else if (strncmp(type, "NC", 2) == 0) {
		/* EC/NC Debug Node */
		ch_addr = ch_num;
		node_addr = 0x01;
	} else if (strncmp(type, "DFEDC", 5) == 0) {
		/* DFETail/DC Debug Node */
		ch_addr = ch_num;
		node_addr = 0x05;
	} else if (strncmp(type, "DEC", 3) == 0) {
		/* R1000DEC Debug Node */
		ch_addr = 0x00;
		node_addr = 0x07;
	} else if (strncmp(type, "CRC", 3) == 0) {
		/* R1000CRC Debug Node */
		ch_addr = ch_num;
		node_addr = 0x06;
	} else if (strncmp(type, "AN", 2) == 0) {
		/* Autoneg Debug Node */
		ch_addr = 0x00;
		node_addr = 0x0F;
	} else if (strncmp(type, "CMI", 3) == 0) {
		/* CMI Debug Node */
		ch_addr = 0x03;
		node_addr = 0x0F;
	} else if (strncmp(type, "SUPV", 4) == 0) {
		/* SUPV PHY  Debug Node */
		ch_addr = 0x00;
		node_addr = 0x0D;
	} else {
		pr_info("Wrong TR register Type !");
		return 0xFFFF;
	}
	data_addr = data_addr & 0x3F;

	tc_mii_write(phyaddr, page_reg, token_ring_debug_reg);
	tc_mii_write(phyaddr, token_ring_control_reg,
		     (1 << 15) | (1 << 13) | (ch_addr << 11) | (node_addr << 7) | (data_addr << 1));

	value_low = tc_mii_read(phyaddr, token_ring_low_data_reg);
	value_high = tc_mii_read(phyaddr, token_ring_high_data_reg);
	value = value_low + ((value_high & 0x00FF) << 16);
	pr_info("*%s => Phyaddr=%d, ch_addr=%d, node_addr=0x%X, data_addr=0x%X , value=0x%X\r\n",
		type, phyaddr, ch_addr, node_addr, data_addr, value);
	tc_mii_write(phyaddr, page_reg, 0x00);/* V1.11 */

	return value;
}

void esw_show_debug_log(u32 phy_addr)
{
	u32 val;

	val = phy_tr_dbg(phy_addr, "PMA", 0x38, 0);
	pr_info("VgaStateA =0x%x\n", ((val >> 4) & 0x1F));
	pr_info("VgaStateB =0x%x\n", ((val >> 9) & 0x1F));
	pr_info("VgaStateC =0x%x\n", ((val >> 14) & 0x1F));
	pr_info("VgaStateD =0x%x\n", ((val >> 19) & 0x1F));

	/* pairA */
	val = tc_phy_read_dev_reg(phy_addr, 0x1E, 0x9B);
	pr_info("XX0 0x1E,0x9B =0x%x\n", val);
	val = (val >> 8) & 0xFF;
	pr_info("AA0 lch_mse_mdcA =0x%x\r\n", val);

	/* Pair B */
	val = tc_phy_read_dev_reg(phy_addr, 0x1E, 0x9B);
	pr_info("XX1 0x1E,0x9B =0x%x\n", val);
	val = (val) & 0xFF;	/* V1.16 */
	pr_info("AA1 lch_mse_mdcB =0x%x\r\n", val);
	/* Pair C */
	val = tc_phy_read_dev_reg(phy_addr, 0x1E, 0x9C);
	pr_info("XX2 0x1E,0x9C =0x%x\n", val);
	val = (val >> 8) & 0xFF;
	pr_info("AA2 lch_mse_mdcC =0x%x\r\n", val);

	/* Pair D */
	val = tc_phy_read_dev_reg(phy_addr, 0x1E, 0x9C);
	pr_info("XX3 0x1E,0x9C =0x%x\n", val);
	val = (val) & 0xFF;	/* V1.16 */
	pr_info("AA3 lch_mse_mdcD =0x%x\r\n", val);
}

irqreturn_t esw_interrupt(int irq, void *resv)
{
	unsigned long flags;
	u32 phy_val;
	int i;
	static unsigned int port_status[5] = {0, 0, 0, 0, 0};
	struct net_device *dev = dev_raether;
	struct END_DEVICE *ei_local = netdev_priv(dev);

	spin_lock_irqsave(&ei_local->page_lock, flags);
	/* disable irq mask and ack irq status */
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x4, 0xffffffff);
	sys_reg_write(ETHDMASYS_ETH_SW_BASE, 0x04000000);
	spin_unlock_irqrestore(&ei_local->page_lock, flags);
	for (i = 0; i < 5; i++) {
		mii_mgr_read(i, 1, &phy_val);
		if (port_status[i] != ((phy_val & 0x4) >> 2)) {
			if (port_status[i] == 0) {
				port_status[i] = 1;
				pr_info("ESW: Link Status Changed - Port%d Link Up\n", i);
			} else {
				port_status[i] = 0;
				pr_info("ESW: Link Status Changed - Port%d Link Down\n", i);
			}
		}
	}
	/* enable irq mask */
	sys_reg_write(ETHDMASYS_ETH_SW_BASE + 0x4, 0xfbffffff);
	return IRQ_HANDLED;
}

void sw_ioctl(struct ra_switch_ioctl_data *ioctl_data)
{
	unsigned int cmd;

	cmd = ioctl_data->cmd;
	switch (cmd) {
	case SW_IOCTL_DUMP_MIB:
		rtk_hal_dump_full_mib();
		break;

	case SW_IOCTL_SET_EGRESS_RATE:
		rtk_hal_set_egress_rate(ioctl_data);
		break;

	case SW_IOCTL_SET_INGRESS_RATE:
		rtk_hal_set_ingress_rate(ioctl_data);
		break;

	case SW_IOCTL_DUMP_VLAN:
		rtk_hal_dump_vlan();
		break;

	case SW_IOCTL_ADD_L2_ADDR:
		rtk_hal_add_table(ioctl_data);
		break;

	case SW_IOCTL_DEL_L2_ADDR:
		rtk_hal_del_table(ioctl_data);
		break;

	case SW_IOCTL_CLEAR_VLAN:
		rtk_hal_clear_vlan();
		break;

	case SW_IOCTL_SET_VLAN:
		rtk_hal_set_vlan(ioctl_data);
		break;

	case SW_IOCTL_DUMP_TABLE:
		rtk_hal_dump_table();
		break;

	case SW_IOCTL_CLEAR_TABLE:
		rtk_hal_clear_table();
		break;

	case SW_IOCTL_GET_PHY_STATUS:
		rtk_hal_get_phy_status(ioctl_data);
		break;

	case SW_IOCTL_SET_PORT_MIRROR:
		rtk_hal_set_port_mirror(ioctl_data);
		break;

	case SW_IOCTL_READ_REG:
		rtk_hal_read_reg(ioctl_data);
		break;

	case SW_IOCTL_WRITE_REG:
		rtk_hal_write_reg(ioctl_data);
		break;

	case SW_IOCTL_QOS_EN:
		rtk_hal_qos_en(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_TABLE2TYPE:
		rtk_hal_qos_set_table2type(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_TABLE2TYPE:
		rtk_hal_qos_get_table2type(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_PORT2TABLE:
		rtk_hal_qos_set_port2table(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_PORT2TABLE:
		rtk_hal_qos_get_port2table(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_PORT2PRI:
		rtk_hal_qos_set_port2pri(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_PORT2PRI:
		rtk_hal_qos_get_port2pri(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_DSCP2PRI:
		rtk_hal_qos_set_dscp2pri(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_DSCP2PRI:
		rtk_hal_qos_get_dscp2pri(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_PRI2QUEUE:
		rtk_hal_qos_set_pri2queue(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_PRI2QUEUE:
		rtk_hal_qos_get_pri2queue(ioctl_data);
		break;

	case SW_IOCTL_QOS_SET_QUEUE_WEIGHT:
		rtk_hal_qos_set_queue_weight(ioctl_data);
		break;

	case SW_IOCTL_QOS_GET_QUEUE_WEIGHT:
		rtk_hal_qos_get_queue_weight(ioctl_data);
		break;

	case SW_IOCTL_ENABLE_IGMPSNOOP:
		rtk_hal_enable_igmpsnoop(ioctl_data);
		break;

	case SW_IOCTL_DISABLE_IGMPSNOOP:
		rtk_hal_disable_igmpsnoop();
		break;

	case SW_IOCTL_SET_PHY_TEST_MODE:
		rtk_hal_set_phy_test_mode(ioctl_data);
		break;

	case SW_IOCTL_GET_PHY_REG:
		rtk_hal_get_phy_reg(ioctl_data);
		break;

	case SW_IOCTL_SET_PHY_REG:
		rtk_hal_set_phy_reg(ioctl_data);
		break;

	case SW_IOCTL_VLAN_TAG:
		rtk_hal_vlan_tag(ioctl_data);
		break;

	case SW_IOCTL_SET_VLAN_MODE:
		rtk_hal_vlan_mode(ioctl_data);
		break;

	case SW_IOCTL_SET_PORT_TRUNK:
		rtk_hal_set_port_trunk(ioctl_data);

	default:
		break;
	}
}

int ephy_ioctl(struct net_device *dev, struct ifreq *ifr,
	       struct ephy_ioctl_data *ioctl_data)
{
	pr_info("%s : cmd =%x (not supported on MT7620)\n", __func__, ioctl_data->cmd);
	return 1;
}

static const struct of_device_id mediatek_gsw_match[] = {
	{.compatible = "mediatek,mt7623-gsw"},
	{.compatible = "mediatek,mt7621-gsw"},
	{},
};

MODULE_DEVICE_TABLE(of, mediatek_gsw_match);

static int mtk_gsw_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *pctl;
	struct mtk_gsw *gsw;
	int err;
	const char *pm;

	gsw = devm_kzalloc(&pdev->dev, sizeof(struct mtk_gsw), GFP_KERNEL);
	if (!gsw)
		return -ENOMEM;

	gsw->dev = &pdev->dev;
	gsw->trgmii_force = 2000;
	gsw->irq = irq_of_parse_and_map(np, 0);
	if (gsw->irq < 0)
		return -EINVAL;

	err = of_property_read_string(pdev->dev.of_node, "mcm", &pm);
	if (!err && !strcasecmp(pm, "enable")) {
		gsw->mcm = true;
		pr_info("== MT7530 MCM ==\n");
	}

	gsw->ethsys = syscon_regmap_lookup_by_phandle(np, "mediatek,ethsys");
	if (IS_ERR(gsw->ethsys)) {
		pr_err("fail at %s %d\n", __func__, __LINE__);
		return PTR_ERR(gsw->ethsys);
	}

	if (!gsw->mcm) {
		gsw->reset_pin = of_get_named_gpio(np, "mediatek,reset-pin", 0);
		if (gsw->reset_pin < 0) {
			pr_err("fail at %s %d\n", __func__, __LINE__);
			return -1;
		}
		pr_debug("reset_pin_port= %d\n", gsw->reset_pin);

		pctl = of_parse_phandle(np, "mediatek,pctl-regmap", 0);
		if (IS_ERR(pctl)) {
			pr_err("fail at %s %d\n", __func__, __LINE__);
			return PTR_ERR(pctl);
		}

		gsw->pctl = syscon_node_to_regmap(pctl);
		if (IS_ERR(pctl)) {
			pr_err("fail at %s %d\n", __func__, __LINE__);
			return PTR_ERR(pctl);
		}

		gsw->pins = pinctrl_get(&pdev->dev);
		if (gsw->pins) {
			gsw->ps_reset =
			    pinctrl_lookup_state(gsw->pins, "reset");

			if (IS_ERR(gsw->ps_reset)) {
				dev_err(&pdev->dev,
					"failed to lookup the gsw_reset state\n");
				return PTR_ERR(gsw->ps_reset);
			}
		} else {
			dev_err(&pdev->dev, "gsw get pinctrl fail\n");
			return PTR_ERR(gsw->pins);
		}
	}

	gsw->supply = devm_regulator_get(&pdev->dev, "mt7530");
	if (IS_ERR(gsw->supply)) {
		pr_info("fail at %s %d\n", __func__, __LINE__);
		return PTR_ERR(gsw->supply);
	}

	if (gsw->mcm) {
		gsw->b3v = devm_regulator_get(&pdev->dev, "b3v");
		if (IS_ERR(gsw->b3v))
			return PTR_ERR(gsw->b3v);
	}

	gsw->wllll = of_property_read_bool(np, "mediatek,wllll");

	platform_set_drvdata(pdev, gsw);

	return 0;
}

static int mtk_gsw_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver gsw_driver = {
	.probe = mtk_gsw_probe,
	.remove = mtk_gsw_remove,
	.driver = {
		   .name = "mtk-gsw",
		   .owner = THIS_MODULE,
		   .of_match_table = mediatek_gsw_match,
		   },
};

module_platform_driver(gsw_driver);
