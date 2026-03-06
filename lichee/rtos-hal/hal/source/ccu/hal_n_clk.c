/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#include <hal_clk.h>
#include "clk_core/clk_core.h"

/* new API which provide uniform format with hal_n_clk_ prefix and better prototype */
int hal_n_clk_framework_init(void)
{
	return clk_core_init();
}

int hal_n_clk_get(clk_controller_id_t cc_id, clk_id_t clk_id, hal_clk_t *clk)
{
	int ret;
	clk_t *clk_tmp;

	ret = clk_core_get_clk(cc_id, clk_id, &clk_tmp);
	if (ret)
		return ret;

	*clk = clk_tmp;
	return 0;
}

int hal_n_clk_put(hal_clk_t clk)
{
	return clk_core_put_clk(clk);
}

int hal_n_clk_enable(hal_clk_t clk)
{
	return clk_core_enable_clk(clk);
}

int hal_n_clk_disable(hal_clk_t clk)
{
	return clk_core_disable_clk(clk);
}

int hal_n_clk_get_enable_state(hal_clk_t clk, int *is_enabled)
{
	return clk_core_get_enable_state(clk, is_enabled);
}

int hal_n_clk_set_parent(hal_clk_t clk, hal_clk_t parent)
{
	return clk_core_set_parent(clk, parent);
}

int hal_n_clk_get_parent(hal_clk_t clk, hal_clk_t *parent)
{
	return clk_core_get_parent(clk, (clk_t **)parent);
}

int hal_n_clk_set_freq(hal_clk_t clk, uint32_t freq)
{
	return clk_core_set_freq(clk, freq);
}

int hal_n_clk_get_freq(hal_clk_t clk, uint32_t *freq)
{
	return clk_core_get_freq(clk, freq);
}

int hal_n_clk_round_freq(hal_clk_t clk, uint32_t *freq)
{
	return clk_core_round_freq(clk, freq);
}

int hal_n_clk_get_name(hal_clk_t clk, const char **name)
{
	*name = clk_hw_get_name(clk_to_hw(clk));
	return 0;
}

int hal_n_clk_get_id_info(hal_clk_t clk, clk_controller_id_t *cc_id, clk_id_t *clk_id)
{
	clk_t *clk_tmp = clk;
	*cc_id = clk_tmp->cc_id;
	*clk_id = clk_tmp->clk_id;
	return 0;
}

int hal_n_clk_get_hosc_freq(uint32_t *freq)
{
	int ret;
	hal_clk_t clk;
	clk_controller_id_t cc_id;
	clk_id_t clk_id;

	cc_id = PLAT_HIGH_FREQ_CRYSTAL_CC_ID;
	clk_id = PLAT_HIGH_FREQ_CRYSTAL_CLK_ID;
	ret = hal_n_clk_get(cc_id, clk_id, &clk);
	if (ret)
		return ret;

	ret = hal_n_clk_get_freq(clk, freq);
	if (ret)
		return ret;

	return 0;
}


#include <console.h>

static int cmd_show_all_clk(int argc, char *argv[])
{
	clk_core_show_all_clk();
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_show_all_clk, clk_dump, show all clock info);

