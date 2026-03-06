/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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
#include <openamp/sunxi_helper/openamp_platform.h>
#include <openamp/open_amp.h>
#include <openamp/virtual_driver/virt_rproc_srm.h>
#include <start_task.h>
#include <string.h>
#include <aw_rpaf/common.h>
#include <platform.h>
#include <console.h>
#include <sunxi_hal_common.h>
#include <hal_uart.h>
/**
 * Message type used in resource manager rpmsg:
 *  RPROC_SRM_MSG_GETCONFIG:    Request to get the configuration of a resource
 *  RPROC_SRM_MSG_SETCONFIG:    Request to set the configuration of a resource
 *  RPROC_SRM_MSG_INIT_TRIGGER: Trigger to init all peripheral
 *  RPROC_SRM_MSG_ERROR:        Error when processing a request
 */
#define RPROC_SRM_MSG_GETCONFIG 	0x00
#define RPROC_SRM_MSG_SETCONFIG 	0x01
#define RPROC_SRM_MSG_INIT_TRIGGER	0x02
#define RPROC_SRM_MSG_ERROR     	0xFF

#define RPMSG_SERVICE_NAME	"sunxi-rproc-srm"
struct rpmsg_srm_msg {
	uint32_t message_type;
	uint8_t device_id[32];
	uint32_t rsc_type;
};
struct rproc_srm_dev {
	struct rpmsg_srm_msg msg;
	int (*init)(void *data);
};
static int rproc_srm_console_uart_dev_init(void *data);
static int rproc_srm_gpio_int_dev_init(void *data);
static int rproc_srm_alsa_rpaf_dev_init(void *data);

#define PERIPHERAL_MAX (3)
struct rproc_srm_dev peripheral_group[PERIPHERAL_MAX] = {
	[0] = {
			.msg = {
				.message_type = RPROC_SRM_MSG_GETCONFIG,
				.device_id = "d_console_uart"
			},
			.init = rproc_srm_console_uart_dev_init,
		},
	[1] = {
			.msg = {
				.message_type = RPROC_SRM_MSG_GETCONFIG,
				.device_id = "d_gpio_interrupt"
			},
			.init = rproc_srm_gpio_int_dev_init,
		},
	[2] = {
			.msg = {
				.message_type = RPROC_SRM_MSG_GETCONFIG,
				.device_id = "d_alsa_rpaf"
			},
			.init = rproc_srm_alsa_rpaf_dev_init,
		},
};

static int virt_rproc_srm_read_cb(struct rpmsg_endpoint *ept, void *data,
			    size_t len, uint32_t src, void *priv)
{

	int i = 0;
	struct rpmsg_srm_msg *p;
	p = (struct rpmsg_srm_msg *)data;

	printf("[%s:0x%x->0x%x] message_type = 0x%x\n", ept->name,
			ept->addr, ept->dest_addr, p->message_type);

	switch(p->message_type) {
	case RPROC_SRM_MSG_INIT_TRIGGER:
		for (i=0; i < PERIPHERAL_MAX; i++) {
			while (!is_rpmsg_ept_ready(ept)) {
			}
			if (rpmsg_send(ept, &peripheral_group[i].msg, sizeof(struct rpmsg_srm_msg)) < 0)
				printf("[%s:0x%x->0x%x] send err\n", ept->name, ept->addr, ept->dest_addr);
		}
		break;
	case RPROC_SRM_MSG_SETCONFIG:
		for (i=0; i < PERIPHERAL_MAX; i++) {
			if (strcmp((char *)peripheral_group[i].msg.device_id, (char *)p->device_id) == 0) {
				peripheral_group[i].init(data);
				break;
			}
		}
		break;
	default:
		break;

	}
	return RPMSG_SUCCESS;
}

int virt_rproc_srm_init(void)
{
	int status;
	/* Create a endpoint for rmpsg communication */
	status = openamp_platform_create_rpmsg_ept(RPMSG_VDEV0,
			RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY,RPMSG_ADDR_ANY,
			virt_rproc_srm_read_cb, NULL);

	return status;
}

static int rproc_srm_console_uart_dev_init(void *data)
{
#ifdef CONFIG_DRIVER_BOOT_DTS
#ifdef CONFIG_DRIVERS_UART
	volatile struct spare_rtos_head_t *pstr = platform_head;
	volatile struct dts_msg_t *pdts = &pstr->rtos_img_hdr.dts_msg;
	int val = 0;
	val = pdts->uart_msg.status;
	val = DTS_OPEN;
	if(val == DTS_OPEN) {
	#if 0
		/*PB8 PB9*/
		val = pdts->uart_msg.uart_port = 1;
		pdts->uart_msg.uart_pin_msg[0].port = 2;
		pdts->uart_msg.uart_pin_msg[0].port_num = 8;
		pdts->uart_msg.uart_pin_msg[0].mul_sel = 7;
		pdts->uart_msg.uart_pin_msg[1].port = 2;
		pdts->uart_msg.uart_pin_msg[1].port_num = 9;
		pdts->uart_msg.uart_pin_msg[1].mul_sel = 7;
	#else
		/*PF2 PF4*/
		val = pdts->uart_msg.uart_port = 0;
		pdts->uart_msg.uart_pin_msg[0].port = 6;
		pdts->uart_msg.uart_pin_msg[0].port_num = 2;
		pdts->uart_msg.uart_pin_msg[0].mul_sel = 3;
		pdts->uart_msg.uart_pin_msg[1].port = 6;
		pdts->uart_msg.uart_pin_msg[1].port_num = 4;
		pdts->uart_msg.uart_pin_msg[1].mul_sel = 3;
	#endif
		val = pdts->uart_msg.uart_port;
		hal_uart_init(val);
		console_uart = val;
	}else {
		console_uart = UART_UNVALID;
	}
	printf("console uart init ...\n");
#endif
#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
	if (console_uart != UART_UNVALID) {
		vUARTCommandConsoleStart(1024, 0);
	}
#endif
#endif
	return 0;
}

static int rproc_srm_gpio_int_dev_init(void *data)
{
	return 0;
}

static int rproc_srm_alsa_rpaf_dev_init(void *data)
{
#ifdef CONFIG_DRIVER_BOOT_DTS
#ifdef CONFIG_DRIVERS_SOUND
	/* should be init at last. */
	sunxi_soundcard_init();
#endif
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF
	snd_dsp_audio_remote_process_init();
#endif
#endif
	return 0;
}
