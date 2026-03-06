/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Debug GPIO driver implementation.
 *
 *  \author     Xradio BT Team
 *
 *  \Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*************************************************************************************************/

#include <hal_def.h>
#include "pal_dbg_io.h"
#include <gpio/gpio.h>
#include "hal_time.h"
#include <hal_gpio.h>

#define PAL_DBG_IO_EN   0

/* please select the board you use:
 * R128 VER ANA: 0
 * XR875 VER   : 1
 */
#define CFG_BOARD_ID    1

#define EXTERNAL_DBG_IO_NUM 6

#ifndef nitems
#define nitems(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#ifdef CONFIG_ARCH_SUN300IW1
#define SYSCTRL_TEST_DBG_REG5   (0x430000a4)
#define SYSCTRL_DEBUG_IO_MASK   (0x00001fe0)
#endif

#if PAL_DBG_IO_EN
typedef struct {
	gpio_pin_t    pin;
	gpio_muxsel_t  fun;
	gpio_driving_level_t  level;
	gpio_pull_status_t  pull;
} GPIO_PinMuxParam;

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/
static const GPIO_PinMuxParam g_dbg_gpio[] = {
	//usual io
	{GPIO_PD11, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD12, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD13, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD14, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD15, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	//{GPIO_PD16, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},

	//dbgio pinmux
	{GPIO_PD1, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD2, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD3, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD4, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD5, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
	{GPIO_PD6, GPIO_MUXSEL_FUNCTION13, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},
};

/*************************************************************************************************/
/*!
 *  \brief      Initialize Debug GPIO.
 */
/*************************************************************************************************/

static void PalIOPinMuxConfig(uint8_t io_num)
{
	hal_gpio_pinmux_set_function(g_dbg_gpio[io_num].pin, g_dbg_gpio[io_num].fun);
	hal_gpio_set_driving_level(g_dbg_gpio[io_num].pin, g_dbg_gpio[io_num].level);
	hal_gpio_set_pull(g_dbg_gpio[io_num].pin, g_dbg_gpio[io_num].pull);
}

static void PalIOPinMuxDeConfig(uint8_t io_num)
{
	hal_gpio_pinmux_set_function(g_dbg_gpio[io_num].pin, GPIO_MUXSEL_DISABLED);
	hal_gpio_set_driving_level(g_dbg_gpio[io_num].pin, GPIO_DRIVING_LEVEL1);
	hal_gpio_set_pull(g_dbg_gpio[io_num].pin, GPIO_PULL_DOWN_DISABLED);
}

/*************************************************************************************************/
/*!
 *  \brief      Debug GPIO Controller.
 */
/*************************************************************************************************/
__intr_text
static void PalIoCtl(uint8_t io_num, gpio_data_t io_state)
{
	hal_gpio_set_data(g_dbg_gpio[io_num].pin, io_state);
}

void PalIoInit(void)
{
	int i;
	int gpio_num;

	gpio_num = nitems(g_dbg_gpio);

	for (i = 0; i < gpio_num; i++) {
		PalIOPinMuxConfig(i);
		PalIoCtl(i, GPIO_DATA_HIGH);
		PalIoCtl(i, GPIO_DATA_LOW);
	}
	PalIoOnAndOff(4, 10);
	PalIoOnAndOff(5, 10);
	PalIoOnAndOff(4, 10);
	PalIoOnAndOff(5, 10);

#ifdef CONFIG_ARCH_SUN300IW1
	HAL_SET_BIT(HAL_REG_32BIT(SYSCTRL_TEST_DBG_REG5), SYSCTRL_DEBUG_IO_MASK);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize Debug GPIO.
 */
/*************************************************************************************************/
void PalIoDeInit(void)
{
	int i;
	int gpio_num;

#ifdef CONFIG_ARCH_SUN300IW1
	HAL_CLR_BIT(HAL_REG_32BIT(SYSCTRL_TEST_DBG_REG5), SYSCTRL_DEBUG_IO_MASK);
#endif

	gpio_num = nitems(g_dbg_gpio);

	for (i = 0; i < gpio_num; i++) {
		PalIoCtl(i, GPIO_DATA_LOW);
		PalIOPinMuxDeConfig(i);
	}
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO up.
 *
 *  \param      Id           GPIO ID.
 */
/*************************************************************************************************/
__intr_text
void PalIoOn(uint8_t Id)
{
	if (Id >= EXTERNAL_DBG_IO_NUM) {
		return;
	}

	PalIoCtl(Id, GPIO_DATA_HIGH);
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO down.
 *
 *  \param      Id           GPIO ID.
 */
/*************************************************************************************************/
__intr_text
void PalIoOff(uint8_t Id)
{
	if (Id >= EXTERNAL_DBG_IO_NUM) {
		return;
	}

	PalIoCtl(Id, GPIO_DATA_LOW);
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO up and down.
 *
 *  \param      Id           GPIO ID.
 *  \param      delay        delay time.
 */
/*************************************************************************************************/

__intr_text
void PalIoOnAndOff(uint8_t Id, uint32_t delay)
{
	if (Id >= EXTERNAL_DBG_IO_NUM) {
		return;
	}

	PalIoCtl(Id, GPIO_DATA_HIGH);
	hal_udelay(delay);
	PalIoCtl(Id, GPIO_DATA_LOW);
}
#else

void PalIoInit(void)
{

}

void PalIoDeInit(void)
{

}

void PalIoOn(uint8_t id)
{

}

void PalIoOff(uint8_t id)
{

}

void PalIoOnAndOff(uint8_t id, uint32_t delay)
{

}

#endif
