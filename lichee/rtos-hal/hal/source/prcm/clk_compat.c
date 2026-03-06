/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sunxi_hal_common.h>
#include <hal_clk.h>
#include <hal_prcm.h>

void hal_clock_init(void)
{
	ccu_init();
}

hal_clk_status_t hal_clk_set_parent(hal_clk_id_t clk, hal_clk_id_t parent)
{
	return ccu_set_mclk_src(clk, parent) == OK ?
			HAL_CLK_STATUS_OK :
			HAL_CLK_STATUS_INVALID_PARAMETER;
}

hal_clk_id_t hal_clk_get_parent(hal_clk_id_t clk)
{
	return ccu_get_mclk_src(clk);
}

u32 hal_clk_recalc_rate(hal_clk_id_t clk)
{
	return ccu_get_sclk_freq(clk);
}

u32 hal_clk_round_rate(hal_clk_id_t clk, u32 rate)
{
	/* TODO */
	return HAL_CLK_RATE_UNINITIALIZED;
}

u32 hal_clk_get_rate(hal_clk_id_t clk)
{
	return ccu_get_sclk_freq(clk);
}

hal_clk_status_t hal_clk_set_rate(hal_clk_id_t clk, u32 rate)
{
	return ccu_set_sclk_freq(clk, rate) == OK ?
			HAL_CLK_STATUS_OK :
			HAL_CLK_STATUS_INVALID_PARAMETER;
}

hal_clk_status_t hal_clock_is_enabled(hal_clk_id_t clk)
{
	/* FIXME: always return disabled */
	return HAL_CLK_STATUS_DISABLED;
}

hal_clk_status_t hal_clock_enable(hal_clk_id_t clk)
{
	if (ccu_set_mclk_reset(clk, CCU_CLK_NRESET) != OK)
		return HAL_CLK_STATUS_ERROR_CLK_ENABLED_FAILED;

	if (ccu_set_mclk_onoff(clk, CCU_CLK_ON) != OK)
		return HAL_CLK_STATUS_ERROR_CLK_ENABLED_FAILED;

	return HAL_CLK_STATUS_OK;
}

hal_clk_status_t hal_clock_disable(hal_clk_id_t clk)
{
	if (ccu_set_mclk_onoff(clk, CCU_CLK_OFF) != OK)
		return HAL_CLK_STATUS_ERROR_CLK_ENABLED_FAILED;

	if (ccu_set_mclk_reset(clk, CCU_CLK_RESET) != OK)
		return HAL_CLK_STATUS_ERROR_CLK_ENABLED_FAILED;

	return HAL_CLK_STATUS_OK;
}
