/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
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

#include <hal_osal.h>
#include <openamp/sunxi_helper/openamp.h>
#include <hal_msgbox.h>
#include <hal_time.h>
#include <hal_event.h>
#include <aw_list.h>

#ifndef min
#define min(x, y)			((x) <= (y) ? (x) : (y))
#endif

struct rpmsg_bind {
	char name[RPMSG_NAME_SIZE];
	struct rpmsg_device *rdev;
	uint32_t dest;
	struct list_head node;
};

#ifdef CONFIG_MULTI_OPENAMP_SUPPORT
static LIST_HEAD(g_rpmsg_drivers);
static LIST_HEAD(g_rpmsg_devices);
static hal_mutex_t rpmsg_bus_lock;

void rpmsg_register_driver(struct rpmsg_driver *driver)
{
	struct rpmsg_bind *pos, *tmp;
	struct rpmsg_endpoint *ept;
	int ret;

	hal_mutex_lock(rpmsg_bus_lock);
	list_add(&driver->node, &g_rpmsg_drivers);
	hal_mutex_unlock(rpmsg_bus_lock);

	hal_mutex_lock(rpmsg_bus_lock);
	list_for_each_entry_safe(pos, tmp, &g_rpmsg_devices, node) {
		if (!strncmp(pos->name, driver->name,
					min(strlen(pos->name), strlen(driver->name))) && driver->probe)
			goto probe;
	}
	pos = NULL;
probe:
	hal_mutex_unlock(rpmsg_bus_lock);

	if (!pos)
		return;

	hal_mutex_lock(rpmsg_bus_lock);
	list_del_init(&pos->node);
	hal_mutex_unlock(rpmsg_bus_lock);

	ept = metal_allocate_memory(sizeof(*ept));
	if (!ept) {
		openamp_err("Failed to allocate memory for ept %s\n", pos->name);
		goto err_out;
	}

	ret = rpmsg_create_ept(ept, pos->rdev, pos->name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
					driver->callback, driver->remove);
	if (ret) {
		openamp_err("Failed to create ept %s (ret: %d)\n", pos->name, ret);
		goto err_free_ept;
	}
	ept->dest_addr = pos->dest;

	openamp_info("create ept %s src: 0x%x, dest: 0x%x\n",
					pos->name, ept->addr, pos->dest);

	ept->priv = driver->priv;
	ret = driver->probe(pos->rdev, ept, ept->addr, ept->dest_addr);
	if (ret) {
		openamp_err("Failed to create ept %s (ret: %d)\n", pos->name, ret);
		goto err_destroy_ept;
	}

	return;

err_destroy_ept:
	rpmsg_destroy_ept(ept);
err_free_ept:
	metal_free_memory(ept);
err_out:
	return;
}

void rpmsg_unregister_driver(struct rpmsg_driver *driver)
{
	hal_mutex_lock(rpmsg_bus_lock);
	list_del_init(&driver->node);
	hal_mutex_unlock(rpmsg_bus_lock);
}

static struct rpmsg_driver *rpmsg_match_driver(const char *name)
{
	struct rpmsg_driver *pos, *tmp;

	hal_mutex_lock(rpmsg_bus_lock);
	list_for_each_entry_safe(pos, tmp, &g_rpmsg_drivers, node) {
		if (!strncmp(pos->name, name, min(strlen(pos->name), strlen(name)))
						&& pos->probe) {
			hal_mutex_unlock(rpmsg_bus_lock);
			return pos;
		}
	}
	hal_mutex_unlock(rpmsg_bus_lock);

	return NULL;
}

void master_rpmsg_ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	struct rpmsg_driver *driver;
	struct rpmsg_bind *dev;
	struct rpmsg_endpoint *ept;
	int ret;

	openamp_dbg("ept %s ns_bind_cb (dest: 0x%x)\n", name, dest);

	if (!rpmsg_bus_lock) {
		rpmsg_bus_lock = hal_mutex_create();
		if (!rpmsg_bus_lock) {
			openamp_err("Failed to allocate memory for rpmsg_bus_lock\r\n");
			return;
		}
	}

	driver = rpmsg_match_driver(name);
	if (!driver) {
		openamp_info("ept %s no match any driver\n", name);
		dev = metal_allocate_memory(sizeof(*dev));
		if (!dev) {
			openamp_err("Failed to allocate memory for dev %s\n", name);
			return;
		}
		strncpy(dev->name, name, RPMSG_NAME_SIZE);
		dev->dest = dest;
		dev->rdev = rdev;
		hal_mutex_lock(rpmsg_bus_lock);
		list_add(&dev->node, &g_rpmsg_devices);
		hal_mutex_unlock(rpmsg_bus_lock);
		return;
	}

	ept = metal_allocate_memory(sizeof(*ept));
	if (!ept) {
		openamp_err("Failed to allocate memory for ept %s\n", name);
		goto err_out;
	}

	/*
	 * we set the destination address RPMSG_ADDR_ANY in order to send
	 * NS announcement back to update the remote endpoint address.
	 */
	ret = rpmsg_create_ept(ept, rdev, name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
					driver->callback, driver->remove);
	if (ret) {
		openamp_err("Failed to create ept %s (ret: %d)\n", name, ret);
		goto err_free_ept;
	}
	ept->dest_addr = dest;

	openamp_info("create ept %s src: 0x%x, dest: 0x%x\n", name, ept->addr, dest);

	ept->priv = driver->priv;
	ret = driver->probe(rdev, ept, ept->addr, ept->dest_addr);
	if (ret) {
		openamp_err("Failed to create ept %s (ret: %d)\n", name, ret);
		goto err_destroy_ept;
	}

	return;

err_destroy_ept:
	rpmsg_destroy_ept(ept);
err_free_ept:
	metal_free_memory(ept);
err_out:
	return;
}
#else
void rpmsg_register_driver(struct rpmsg_driver *driver)
{
	openamp_info("Unsupport rpmsg_register_driver\r\n");
}
void rpmsg_unregister_driver(struct rpmsg_driver *driver)
{
	openamp_info("Unsupport rpmsg_unregister_driver\r\n");
}
#endif /* CONFIG_MULTI_OPENAMP_SUPPORT */
