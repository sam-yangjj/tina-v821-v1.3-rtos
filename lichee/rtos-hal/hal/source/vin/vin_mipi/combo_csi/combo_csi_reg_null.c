/*
 * combo csi module(null)
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn <zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sunxi_hal_common.h>
#include "combo_csi_reg.h"

int cmb_csi_set_top_base_addr(unsigned long addr)
{
	return 0;
}

int cmb_csi_set_phy_base_addr(unsigned int sel, unsigned long addr)
{
	return 0;
}

int cmb_csi_set_port_base_addr(unsigned int sel, unsigned long addr)
{
	return 0;
}

void cmb_phy_reset_assert(void)
{

}

void cmb_phy_reset_deassert(void)
{

}

void cmb_phy_link_mode_set(enum phy_link_mode phy_link_mode)
{

}

void cmb_phy_link_mode_get(int *cur_phy_link)
{

}

void cmb_phy_power_enable(void)
{

}
void cmb_phy_power_disable(void)
{

}

void cmb_phy_set_vref_0p9(unsigned int step)
{

}

void cmb_phy_set_vref_0p2(unsigned int step)
{

}

void cmb_phy_set_trescal(unsigned int val)
{

}

void cmb_phya_trescal_reset_assert(void)
{

}

void cmb_phya_trescal_reset_deassert(void)
{

}

void cmb_phy_top_enable(void)
{

}

void cmb_phy_top_disable(void)
{

}

/*
 * Detail function information of registers----PHYA/B
 */
void cmb_phy0_en(unsigned int sel, unsigned int en)
{

}

void cmb_phy_lane_num_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy0_work_mode(unsigned int sel, unsigned int mode) /* mode 0 --mipi, mode 1 --sub-lvds/hispi*/
{

}

void cmb_phy0_ofscal_cfg(unsigned int sel)
{

}

void cmb_phy_deskew_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy_set_deskew_laned1(unsigned int sel, unsigned int delay)
{

}

void cmb_phy_set_deskew_laned0(unsigned int sel, unsigned int delay)
{

}

void cmb_phy_set_deskew_laneck0(unsigned int sel, unsigned int delay)
{

}

void cmb_phy_deskew1_cfg(unsigned int sel, unsigned int deskew, bool deskew_lane_cfg)
{

}

void cmb_phy0_term_dly(unsigned int sel, unsigned int dly)
{

}

void cmb_phy_mipi_termnum_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_term_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy0_hs_dly(unsigned int sel, unsigned int dly)
{

}

void cmb_phy_hs_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_hs_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy_s2p_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy0_s2p_width(unsigned int sel, unsigned int width)
{

}

void cmb_phy0_s2p_dly(unsigned int sel, unsigned int dly)
{

}

void cmb_s2p_ctl(unsigned int sel, unsigned int dly, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy_mipi_lpnum_en(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

void cmb_phy0_mipilp_dbc_en(unsigned int sel, unsigned int en)
{

}

void cmb_phy0_mipihs_sync_mode(unsigned int sel, unsigned int mode)
{

}

void cmb_mipirx_ctl(unsigned int sel, struct phy_lane_cfg phy_lane_cfg)
{

}

/*
 * Detail function information of registers----PORT0/1
 */
void cmb_port_enable(unsigned int sel)
{

}

void cmb_port_disable(unsigned int sel)
{

}

void cmb_port_lane_num(unsigned int sel, unsigned int num)
{

}

void cmb_port_out_num(unsigned int sel, enum cmb_csi_pix_num cmb_csi_pix_num)
{

}

void cmb_port_out_chnum(unsigned int sel, unsigned int chnum)
{

}

unsigned char cmb_port_set_lane_map(unsigned int phy, unsigned int ch)
{
	return 0;
}

void cmb_port_lane_map(unsigned int sel, unsigned char *mipi_lane)
{

}

void cmb_port_set_wdr_mode(unsigned int sel, unsigned int mode)
{

}

void cmb_port_set_fid_mode(unsigned int sel, unsigned int mode)
{

}

void cmb_port_set_fid_ch_map(unsigned int sel, unsigned int ch)
{

}

void cmb_port_mipi_unpack_enable(unsigned int sel)
{

}

void cmb_port_mipi_unpack_disable(unsigned int sel)
{

}

void cmb_port_mipi_yuv_seq(unsigned int sel, enum cmb_mipi_yuv_seq seq)
{

}

void cmb_port_mipi_ph_bitord(unsigned int sel, unsigned int order)
{

}

void cmb_port_mipi_cfg(unsigned int sel, enum cmb_mipi_yuv_seq seq)
{

}

void cmb_port_set_mipi_datatype(unsigned int sel, struct combo_csi_cfg *combo_csi_cfg)
{

}

void cmb_port_mipi_ch_trigger_en(unsigned int sel, unsigned int ch, unsigned int en)
{

}

void cmb_port_set_mipi_wdr(unsigned int sel, unsigned int mode, unsigned int ch)
{

}

