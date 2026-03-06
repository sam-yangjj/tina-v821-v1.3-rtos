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

#include "clk_core.h"

#include <stdlib.h>
#include <sunxi_hal_common.h>

#include "clk_source.h"

#include <stdarg.h>


//#define CLK_CORE_DUMP_CLK_INFO

void dump_clk_hw_info(const clk_hw_info_t *hw_info)
{
	uint8_t i;
	clk_number_t clk_number;
	clk_info("Clock Hardware Info(%p):", hw_info);
#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
	clk_info("name: '%s'", hw_info->name);
#endif
	clk_info("ops: %p", hw_info->ops);

	clk_info("parent num: %u", hw_info->num_parents);
	for (i = 0; i < hw_info->num_parents; i++)
	{
		clk_number = hw_info->parents[i];
		clk_info("parent[%u]: %u.%u", i, CC_ID(clk_number), CLK_ID(clk_number));
	}
}

void dump_clk(const clk_t *clk)
{
	clk_info("Clock(%p):", clk);
	clk_info("cc_id: %u", clk->cc_id);
	clk_info("clk_id: %u", clk->clk_id);
	clk_info("enable_count: %u", clk->enable_count);
	//clk_info("rate: %u", clk->rate);
	clk_info("parent: %p", clk->parent);
}

void dump_clk_hw(const clk_hw_t *hw)
{
	clk_info("----------------Clock Hardware object info----------------");
	clk_info("Clock Hardware(%p):", hw);
	dump_clk_hw_info(&hw->info);
	dump_clk(&hw->clk);
}


void dump_clk_controller(const clk_controller_t *controller)
{
	clk_info("----------------Clock Controller object info----------------");
	clk_info("Clock Controller(%p):", controller);
	clk_info("id: %u", controller->id);
	clk_info("reg_base: 0x%lx", controller->reg_base);

	clk_info("clk_num: %u", controller->clk_num);
	clk_info("clk_hws: %p", controller->clk_hws);

	clk_info("reset_num: %u", controller->reset_num);
	clk_info("reset_hws: %p", controller->reset_hws);

	clk_info("ops: %p", controller->ops);
}

unsigned long clk_hw_get_flags(const struct clk_hw *hw)
{
	if (!hw)
		return 0;

	return hw->info.flags;
}

//int clk_hw_get_name(const clk_hw_t *hw, const char **name)
const char *clk_hw_get_name(const clk_hw_t *hw)
{
	if (!hw)
		return NULL;

#ifdef CLK_HW_INFO_STRUCT_HAS_NAME_MEMBER
	return hw->info.name;
#else
	return "CLK_HW_INFO_NO_NAME_MEMBER";
#endif
}

static int clk_core_get_controller(clk_controller_id_t id, clk_controller_t **controller)
{
	clk_controller_t *tmp = platform_get_controller(id);
	if (!tmp)
	{
		clk_err("get clk controller %u failed", id);
		return CLK_RET_CONTROLLER_NOT_FOUND;
	}

	*controller = tmp;
	return 0;
}

static inline int hw_info_get_parent_index_by_clk_num(const clk_hw_info_t *hw_info,
		clk_number_t parent_clk_num, uint8_t *parent_index)
{
	uint8_t i;
	for (i = 0; i < hw_info->num_parents; i++)
	{
		if (parent_clk_num == hw_info->parents[i])
		{
			*parent_index = i;
			return 0;
		}
	}

	return CLK_RET_PARENT_NOT_FOUND;
}

static inline int hw_info_get_parent_clk_num_by_index(const clk_hw_info_t *hw_info,
		uint8_t parent_index, clk_number_t *parent_clk_num)
{
	if (parent_index >= hw_info->num_parents)
		return CLK_RET_INVALID_PARENT_INDEX;

	*parent_clk_num = hw_info->parents[parent_index];
	return 0;
}

static inline int hw_info_get_parent_clk_by_index(const clk_hw_info_t *hw_info,
		uint8_t parent_index, clk_t **parent_clk)
{
	int ret;
	clk_number_t parent_clk_num = 0;

	ret = hw_info_get_parent_clk_num_by_index(hw_info, parent_index, &parent_clk_num);
	if (ret)
		return ret;

	return clk_core_get_clk(CC_ID(parent_clk_num), CLK_ID(parent_clk_num), parent_clk);
}

static inline int clk_core_get_parent_clk_by_index(const clk_hw_t *hw,
		uint8_t parent_index, clk_t **parent_clk)
{
	return hw_info_get_parent_clk_by_index(&hw->info, parent_index, parent_clk);
}

uint8_t clk_hw_get_num_parents(const struct clk_hw *hw)
{
	return hw->info.num_parents;
}

int clk_hw_get_controller(const struct clk_hw *hw, clk_controller_t **controller)
{
#ifdef CLK_CORE_DEBUG
	if (!hw)
		return CLK_RET_INVALID_CLK_HW;

	if (!controller)
		return CLK_RET_INVALID_PARAMETER;
#endif

	return clk_core_get_controller(hw->clk.cc_id, controller);
}

int clk_hw_get_controller_reg_base(const struct clk_hw *hw, unsigned long *reg_base)
{
	int ret;
	clk_controller_t *controller;

#ifdef CLK_CORE_DEBUG
	if (!reg_base)
		return CLK_RET_INVALID_PARAMETER;
#endif

	ret = clk_hw_get_controller(hw, &controller);
	if (ret)
		return ret;

	*reg_base = controller->reg_base;
	return 0;
}

int rst_hw_get_controller(const reset_hw_t *rst_hw, clk_controller_t **controller)
{
#ifdef CLK_CORE_DEBUG
	if (!controller)
		return CLK_RET_INVALID_PARAMETER;
#endif

	return clk_core_get_controller(rst_hw->rc_id, controller);
}

int clk_core_get_clk_hw(clk_controller_id_t cc_id, clk_id_t clk_id, clk_hw_t **clk_hw)
{
	int ret;
	clk_controller_t *controller;

	clk_dbg("get clk hw %u.%u", cc_id, clk_id);

	if (cc_id >= SYS_CLK_CONTROLLER_NUMBER)
		return CLK_RET_INVALID_CLK_CONTROLLER_ID;

	ret = clk_core_get_controller(cc_id, &controller);
	if (ret)
		return ret;

	if (clk_id >= controller->clk_num)
		return CLK_RET_INVALID_CLK_ID;

	*clk_hw = controller->clk_hws[clk_id];
	if (!*clk_hw)
		return CLK_RET_CLK_HW_NOT_EXIST;

	clk_hw_dbg(controller->clk_hws[clk_id], "get clk hw success!");
	return 0;
}

int clk_core_get_rst_hw(rst_controller_id_t rc_id, rst_id_t rst_id, reset_hw_t **rst_hw)
{
	int ret;
	clk_controller_t *controller;

	clk_dbg("get rst hw %u.%u", rc_id, rst_id);

	if (rc_id >= SYS_CLK_CONTROLLER_NUMBER)
		return CLK_RET_INVALID_RST_CONTROLLER_ID;

	if (rc_id == SYS_CLK_SRC_CONTROLLER_ID)
		return CLK_RET_INVALID_RST_CONTROLLER_ID;

	ret = clk_core_get_controller(rc_id, &controller);
	if (ret)
		return ret;

	if (rst_id >= controller->reset_num)
		return CLK_RET_INVALID_RST_ID;

	*rst_hw = &controller->reset_hws[rst_id];
	return 0;
}

int clk_core_enable_clk(clk_t *clk)
{
	int ret = 0;
	clk_hw_t *hw;

	if (!clk)
	{
		return CLK_RET_INVALID_CLK;
	}

	hw = clk_to_hw(clk);
	clk_hw_dbg(hw, "enable clk");

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("recurse to clk source!");
		return 0;
	}

	if (clk->enable_count == 0)
	{
		ret = clk_core_enable_clk(clk->parent);
		if (ret)
		{
			clk_hw_err(hw, "clk_core_enable_clk failed, ret: %d", ret);
			return ret;
		}

		const clk_hw_ops_t *ops = hw->info.ops;
		if (ops->set_enable_state)
		{
			clk_dbg("call hw ops to enable clk %u.%u", clk->cc_id, clk->clk_id);
			ret = ops->set_enable_state(hw, 1);
			if (ret)
			{
				clk_err("call hw ops to enable clk %u.%u failed, ret: %d", clk->cc_id, clk->clk_id, ret);
				return ret;
			}
		}
	}

	clk_hw_dbg(hw, "enable clk success");

	if (clk->enable_count == MAX_CLK_ENABLE_CNT)
	{
		clk_hw_warn(hw, "clk enable count is greater than %u", MAX_CLK_ENABLE_CNT);
		return 0;
	}

	clk->enable_count++;
	return 0;
}

int clk_core_disable_clk(clk_t *clk)
{
	int ret;
	clk_hw_t *hw;

	if (!clk)
	{
		return CLK_RET_INVALID_CLK;
	}

	clk_dbg("disable clk %u.%u", clk->cc_id, clk->clk_id);

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("recurse to clk source!");
		return 0;
	}

	if (clk->enable_count == 0)
	{
		return 0;
	}

	hw = clk_to_hw(clk);
	if (clk->enable_count == 1 && hw->info.flags & CLK_IS_CRITICAL)
	{
		return 0;
	}

	if (--clk->enable_count > 0)
	{
		return 0;
	}

	const clk_hw_ops_t *ops = hw->info.ops;
	if (ops->set_enable_state)
	{
		clk_dbg("call hw ops to disable clk %u.%u", clk->cc_id, clk->clk_id);
		ret = ops->set_enable_state(hw, 0);
		if (ret)
		{
			clk_err("call hw ops to disable clk %u.%u failed, ret: %d", clk->cc_id, clk->clk_id, ret);
			return ret;
		}

	}

	return clk_core_disable_clk(clk->parent);
}

int clk_core_get_enable_state(clk_t *clk, int *is_enabled)
{
	int ret;
	if (!clk)
	{
		return CLK_RET_INVALID_CLK;
	}

	clk_dbg("get clk %u.%u enable state", clk->cc_id, clk->clk_id);

	if (!is_enabled)
	{
		return CLK_RET_INVALID_PARAMETER;
	}

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("recurse to clk source!");
		*is_enabled = 1;
		return 0;
	}

	clk_hw_t *hw = clk_to_hw(clk);
	const struct clk_hw_ops *ops = hw->info.ops;
	if (!ops->get_enable_state)
	{
		int parent_is_enabled;

		ret = clk_core_get_enable_state(clk->parent, &parent_is_enabled);
		if (ret)
		{
			return ret;
		}

		if (parent_is_enabled)
			*is_enabled = 1;
		else
			*is_enabled = 0;
		return 0;
	}

	clk_dbg("call hw ops to get clk %u.%u enable state", clk->cc_id, clk->clk_id);
	ret = ops->get_enable_state(hw, is_enabled);
	if (ret)
	{
		clk_err("call hw ops to get clk %u.%u enable state failed, ret: %d", clk->cc_id, clk->clk_id, ret);
	}

	return ret;
}

int clk_core_set_parent(clk_t *clk, clk_t *parent)
{
	int ret;

	if (!clk || !parent)
	{
		return CLK_RET_INVALID_CLK;
	}

	clk_dbg("set clk %u.%u parent to %u.%u", clk->cc_id, clk->clk_id,
			parent->cc_id, parent->clk_id);

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		return CLK_RET_CLK_SRC_CAN_NOT_SET_PARENT;
	}

	clk_hw_t *hw = clk_to_hw(clk);
	clk_hw_info_t *hw_info = &hw->info;

	if (hw_info->num_parents == 0)
	{
		return CLK_RET_INVALID_PARENT_NUM;
	}

	clk_number_t parent_clk_num = MAKE_CLKn(parent->cc_id, parent->clk_id);

	if (hw_info->num_parents == 1)
	{
		if (hw_info->parents[0] == parent_clk_num)
		{
			clk->parent = parent;
			return 0;
		}

		clk_err("clk %u.%u only has one parent %u.%u, not %u.%u", clk->cc_id, clk->clk_id,
				CC_ID(hw_info->parents[0]), CLK_ID(hw_info->parents[0]),
				parent->cc_id, parent->clk_id);
		return CLK_RET_WRONG_SINGLE_PARENT;
	}

	const clk_hw_ops_t *ops = hw_info->ops;
	if (!ops->set_parent)
	{
		return CLK_RET_OPS_NOT_EXIST_WITH_MULTI_PARENT;
	}

	uint8_t parent_index = 0;
	ret = hw_info_get_parent_index_by_clk_num(hw_info, parent_clk_num, &parent_index);
	if (ret)
	{
		return ret;
	}

	clk_dbg("call hw ops to set clk %u.%u parent", clk->cc_id, clk->clk_id);
	ret = ops->set_parent(hw, parent_index);
	if (ret)
	{
		clk_err("call hw ops to set clk %u.%u parent failed, ret: %d",
				clk->cc_id, clk->clk_id, ret);
		return ret;
	}

	clk->parent = parent;
	return 0;
}

int clk_core_get_parent(clk_t *clk, clk_t **parent)
{
	int ret;

	if (!clk || !parent)
	{
		return CLK_RET_INVALID_CLK;
	}

	clk_dbg("get clk %u.%u parent", clk->cc_id, clk->clk_id);

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		return CLK_RET_CLK_SRC_HAVE_NO_PARENT;
	}

	clk_hw_t *hw = clk_to_hw(clk);
	clk_hw_info_t *hw_info = &hw->info;

	if (hw_info->num_parents == 0)
	{
		return CLK_RET_INVALID_PARENT_NUM;
	}

	clk_t *tmp_parent_clk;
	const clk_hw_ops_t *ops = hw_info->ops;
	if (!ops->get_parent)
	{
		if (hw_info->num_parents != 1)
		{
			return CLK_RET_OPS_NOT_EXIST_WITH_MULTI_PARENT;
		}

		ret = hw_info_get_parent_clk_by_index(hw_info, 0, &tmp_parent_clk);
		if (ret)
			return ret;

		goto update_clk_parent;
	}

	uint8_t parent_index = 0;
	clk_dbg("call hw ops to get clk %u.%u parent", clk->cc_id, clk->clk_id);
	ret = ops->get_parent(hw, &parent_index);
	if (ret)
	{
		clk_err("call hw ops to get clk %u.%u parent failed, ret: %d",
				clk->cc_id, clk->clk_id, ret);
		return ret;
	}

	ret = hw_info_get_parent_clk_by_index(hw_info, parent_index, &tmp_parent_clk);
	if (ret)
		return ret;

update_clk_parent:
	*parent = tmp_parent_clk;
	if (clk->parent != tmp_parent_clk)
	{
		clk_warn("clk %u.%u parent(%p) is no same with the real(%p)",
				 clk->cc_id, clk->clk_id, clk->parent, tmp_parent_clk);
		clk->parent = tmp_parent_clk;
	}
	return 0;
}

int clk_core_set_freq(clk_t *clk, uint32_t freq)
{
	int ret;
	clk_hw_t *hw = clk_to_hw(clk);

	if (!clk)
	{
		return CLK_RET_INVALID_CLK;
	}

	hw = clk_to_hw(clk);
	clk_hw_dbg(hw, "set clk freq to %u", freq);

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("can't change clk source' freq!");
		return CLK_RET_CLK_SRC_CAN_NOT_SET_FREQ;
	}

	const clk_hw_ops_t *ops = hw->info.ops;
	if (!ops->set_freq)
	{
		return CLK_RET_CLK_NOT_SUPPORT_SET_FREQ;
	}

	uint32_t parent_freq;
	ret = clk_core_get_freq(clk->parent, &parent_freq);
	if (ret)
		return ret;

	clk_dbg("parent_freq: %u", parent_freq);

	if (clk->enable_count > 1)
	{
		uint32_t old_freq;
		ret = clk_core_get_freq(clk, &old_freq);
		if (ret)
		{
			return ret;
		}

		clk_hw_t *parent_hw = clk_to_hw(clk->parent);
		if (old_freq != freq)
		{
			clk_warn("Warning: clk %s enable_count is %u,"
					 " parent is %s, parent rate is %u,"
					 " should not be changed from %u to %u,"
					 " please recheck whether the operation is correct\n",\
					 clk_hw_get_name(hw), hw->clk.enable_count,\
					 clk_hw_get_name(parent_hw), parent_freq,\
					 old_freq, freq);
		}
	}

	clk_dbg("call hw ops to set clk %u.%u freq", clk->cc_id, clk->clk_id);
	ret = ops->set_freq(hw, parent_freq, freq);
	if (ret)
	{
		clk_hw_err(hw, "call hw ops to set clk freq to %u failed, ret: %d", freq, ret);
		return ret;
	}

	clk_hw_dbg(hw, "set clk freq success!");
	return 0;
}

int clk_core_get_freq(clk_t *clk, uint32_t *freq)
{
	int ret;
	clk_hw_t *hw;

	if (!clk)
	{
		return CLK_RET_INVALID_CLK;
	}

	hw = clk_to_hw(clk);
	clk_hw_dbg(hw, "get clk freq");

	uint32_t parent_freq;

	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_hw_dbg(hw, "get freq recurse to clk source!");
	}
	else
	{
		ret = clk_core_get_freq(clk->parent, &parent_freq);
		if (ret)
			return ret;

		clk_hw_dbg(hw, "parent("CLK_GLOBAL_ID_FMT_STR") freq: %u",
			clk->parent->cc_id, clk->parent->clk_id, parent_freq);
	}

	const struct clk_hw_ops *ops = hw->info.ops;
	if (ops->get_freq)
	{
		uint32_t tmp_freq;
		clk_hw_dbg(hw, "hw ops get_freq start");
		ret = ops->get_freq(hw, parent_freq, &tmp_freq);
		clk_hw_dbg(hw, "hw ops get_freq end");

		if (ret)
		{
			clk_err("hw ops get_freq faile %u.%u, ret: %d", clk->cc_id, clk->clk_id, ret);
			return ret;
		}

		*freq = tmp_freq;
	}
	else
	{
		*freq = parent_freq;
	}

	clk_hw_dbg(hw, "get clk freq success!");
	return 0;
}

int clk_core_round_freq(clk_t *clk, uint32_t *freq)
{
	int ret;
	clk_hw_t *hw = clk_to_hw(clk);
	const struct clk_hw_ops *ops = hw->info.ops;
	uint32_t parent_freq;

	clk_dbg("round clk %u.%u freq", clk->cc_id, clk->clk_id);
	if (clk->cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("round freq recurse to clk source!");
		uint32_t clk_src_freq;
		ret = clk_core_get_freq(clk, &clk_src_freq);
		if (ret)
			return ret;

		*freq = clk_src_freq;
		return 0;
	}

	ret = clk_core_get_freq(clk->parent, &parent_freq);
	if (ret)
		return ret;

	if (!ops->round_freq)
	{
		return parent_freq;
	}

	return ops->round_freq(hw, parent_freq, freq);
}

static int clk_hw_init_parent(clk_hw_t *hw)
{
	int ret;
	u8 parent_index = 0;

	if (hw->clk.cc_id == SYS_CLK_SRC_CONTROLLER_ID)
	{
		clk_dbg("clk source has no parent!");
		return 0;
	}

	if (!hw->info.num_parents)
	{
		return CLK_RET_INVALID_PARENT_NUM;
	}

	if (!hw->info.parents)
	{
		return CLK_RET_INVALID_PARENT_ARRAY;
	}

	if (hw->info.num_parents > 1 && hw->info.ops->get_parent)
	{
		clk_dbg("execute clk hw(%u.%u) get parent ops", hw->clk.cc_id, hw->clk.clk_id);
		ret = hw->info.ops->get_parent(hw, &parent_index);
		if (ret)
		{
			clk_err("execute clk hw(%u.%u) get parent ops failed, ret: %d",
					hw->clk.cc_id, hw->clk.clk_id, ret);
			return ret;
		}
	}

	clk_hw_dbg(hw, "current parent's index: %u", parent_index);
	return clk_core_get_parent_clk_by_index(hw, parent_index, &hw->clk.parent);
}

int clk_hw_init(clk_hw_t *hw)
{
	int ret = -1;
	const struct clk_hw_ops *ops;

	if (!hw)
	{
		return ret;
	}

	ops = hw->info.ops;
	if (ops->init)
	{
		clk_dbg("execute clk hw(%u.%u) init ops", hw->clk.cc_id, hw->clk.clk_id);
		ret = ops->init(hw);
		if (ret)
		{
			clk_hw_err(hw, "init clk hw failed, ret: %d", ret);
			return ret;
		}
	}

	ret = clk_hw_init_parent(hw);
	if (ret)
		return ret;

	if (hw->info.flags & CLK_IS_CRITICAL)
	{
		ret = clk_core_enable_clk(&hw->clk);
		if (ret)
		{
			return ret;
		}
	}

	return 0;
}


static int clk_controller_common_init(clk_controller_t *controller)
{
	uint32_t i;
	int ret;
	clk_hw_t *hw;
	reset_hw_t *rst_hw;

	clk_dbg("execute contoller(%u) common init", controller->id);

	ret = hal_spin_lock_init(&controller->lock);
	if (ret)
		return CLK_RET_CODE_INIT_LOCK_FAILED;

	/* init all clk hw */
	for (i = 0; i < controller->clk_num; i++)
	{
		hw = controller->clk_hws[i];
		if (!hw)
		{
			clk_err("clk %u.%u not exist!", controller->id, i);
			continue;
		}

		clk_dbg("execute clk hw(%u.%u) init", controller->id, i);

		hw->clk.cc_id = controller->id;
		hw->clk.clk_id = i;
		ret = clk_hw_init(hw);
		if (ret)
		{
			return ret;
		}
#ifdef CLK_CORE_DUMP_CLK_INFO
		dump_clk_hw(hw);
#endif
	}

	for (i = 0; i < controller->reset_num; i++)
	{
		rst_hw = &controller->reset_hws[i];

		clk_dbg("execute rst hw(%u.%u) init", controller->id, i);
		rst_hw->rc_id = controller->id;
	}

	return 0;
}

int clk_core_check_ctrl_field_info(uint8_t field_offset, uint8_t field_width, uint32_t max_value_in_sw)
{
	int max_field_value;

	if (field_offset >= MAX_CLK_CTRL_REG_BITS)
	{
		return CLK_RET_INVALID_CTRL_FIELD_OFFSET;
	}

	if (field_width > MAX_CLK_CTRL_REG_BITS)
	{
		return CLK_RET_INVALID_CTRL_FIELD_WIDTH;
	}

	if ((field_offset + field_width) > MAX_CLK_CTRL_REG_BITS)
	{
		return CLK_RET_INVALID_CTRL_FIELD_INFO;
	}

	max_field_value = (1 << field_width) - 1;
	if (max_field_value > max_value_in_sw)
	{
		return CLK_RET_CTRL_FIELD_WIDTH_EXCEED_SW_LIMIT;
	}

	return 0;
}

int clk_core_init(void)
{
	uint32_t i;
	int ret;
	clk_controller_t *controller;

	for (i = 0; i < SYS_CLK_CONTROLLER_NUMBER; i++)
	{
		clk_dbg("init contoller %u", i);

		ret = clk_core_get_controller(i, &controller);
		if (ret)
		{
			//clk_err("get clk controller(%u) failed", i);
			continue;
		}

		if (controller->id != i)
		{
			clk_err("clk controller id(%u) is invalid, it should be %u",
					controller->id, i);
			continue;
		}

		if (controller->ops)
		{
			clk_dbg("execute contoller(%u) ops", i);
			ret = controller->ops->init(controller);
			if (ret)
			{
				clk_err("init clk controller specific failed, ret: %d", ret);
				continue;
			}
		}

		ret = clk_controller_common_init(controller);
		if (ret)
		{
			clk_err("init clk controller common failed, ret: %d", ret);
			continue;
		}
#ifdef CLK_CORE_DUMP_CLK_INFO
		dump_clk_controller(controller);
#endif
	}

	return 0;
}


//int clk_summary(void)

extern void dump_clk_controller_reg(void);
int clk_core_show_all_clk(void)
{
	uint32_t i, j, freq;
	int ret;
	clk_controller_t *controller;
	clk_hw_t *hw;
	clk_t *clk, *parent;
	clk_number_t clk_num, parent_clk_num;
	//clk_hw_info_t *hw_info;

	printf("       clk_num|name                |en_cnt| frequency|        parent\n");
	for (i = 0; i < SYS_CLK_CONTROLLER_NUMBER; i++)
	{
		ret = clk_core_get_controller(i, &controller);
		if (ret)
		{
			clk_err("get clk controller(%u) failed", i);
			continue;
		}

		if (controller->id != i)
		{
			clk_err("clk controller id(%u) is invalid, it should be %u",
					controller->id, i);
			continue;
		}

		//dump_clk_controller(controller);
		for (j = 0; j < controller->clk_num; j++)
		{
			hw = controller->clk_hws[j];
			//hw_info = &hw->info;
			clk = &hw->clk;
			ret = clk_core_get_freq(clk, &freq);
			if (ret)
			{
				clk_hw_err(hw, "get freq failed!");
				continue;
			}

			if (hw->clk.cc_id == SYS_CLK_SRC_CONTROLLER_ID)
			{
				parent_clk_num = MAKE_CLKn(0, MAX_CLK_ID);
			}
			else
			{
				ret = clk_core_get_parent(clk, &parent);
				if (ret)
				{
					clk_hw_err(hw, "get parent failed!");
					continue;
				}
				parent_clk_num = MAKE_CLKn(parent->cc_id, parent->clk_id);
			}

			clk_num = MAKE_CLKn(clk->cc_id, clk->clk_id);
			printf("%*u(%02u:%03u) "
				"%-*s "
				"%*u "
				"%*u ",
				6, clk_num, clk->cc_id, clk->clk_id,
				20, clk_hw_get_name(hw),
				6, clk->enable_count,
				10, freq);

			if (hw->clk.cc_id == SYS_CLK_SRC_CONTROLLER_ID)
			{
				printf("%*s\n", 6, "NONE");
			}
			else
			{
				printf("%*u(%02u:%03u)\n", 6, parent_clk_num, parent->cc_id, parent->clk_id);
			}
		}
	}

	return 0;
}
