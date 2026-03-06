/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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

#include "../include/isp_ini_parse.h"
#include "../include/isp_manage.h"
#include "../include/isp_debug.h"
#include "../../../platform/platform_cfg.h"

#if (ISP_VERSION == 600 || ISP_VERSION == 601)
#if defined CONFIG_SENSOR_GC2053_MIPI || defined CONFIG_SENSOR_GC4663_MIPI || defined CONFIG_SENSOR_SC5336_MIPI || \
	defined CONFIG_SENSOR_IMX319_MIPI || defined CONFIG_SENSOR_SC035HGS_MIPI || defined CONFIG_SENSOR_SC3336_MIPI || \
	defined CONFIG_SENSOR_IMX335_MIPI || defined CONFIG_SENSOR_SC2336_MIPI || defined CONFIG_SENSOR_GC1084_MIPI || \
	defined CONFIG_SENSOR_BF2257CS_MIPI || defined CONFIG_SENSOR_SC2355_MIPI

#ifdef CONFIG_SENSOR_GC4663_MIPI
#include "SENSOR_H/gc4663_mipi_default_ini_v853.h"
#include "SENSOR_H/gc4663_120fps_mipi_default_ini_v853.h"
#include "SENSOR_H/gc4663_120fps_mipi_linear_to_wdr_ini_v853.h"
//#include "SENSOR_H/gc4663_mipi_wdr_default_v853.h"
#include "SENSOR_H/gc4663_mipi_wdr_auto_ratio_v853.h"
#include "SENSOR_REG_H/gc4663_mipi_120fps_720p_day_reg.h"
#include "SENSOR_REG_H/gc4663_mipi_2560_1440_15fps_day_reg.h"
//#include "SENSOR_REG_H/gc4663_mipi_2560_1440_wdr_15fps_day_reg.h"
#include "SENSOR_REG_H/gc4663_mipi_2560_1440_wdr_auto_ratio_15fps_day_reg.h"
#endif // CONFIG_SENSOR_GC4663_MIPI

#ifdef CONFIG_SENSOR_GC2053_MIPI
#ifdef CONFIG_SENSOR_GC2053_8BIT_MIPI
#include "SENSOR_H/gc2053_120fps_mipi_default_ini_v853_8bit.h"
#include "SENSOR_H/gc2053_mipi_isp600_20231212_111630_day_8bit.h"
#include "SENSOR_REG_H/gc2053_mipi_120fps_480p_day_reg_8bit.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg_8bit.h"
#else
#include "SENSOR_H/gc2053_120fps_mipi_default_ini_v853.h"
#include "SENSOR_H/gc2053_mipi_isp600_20231212_111630_day.h"
#include "SENSOR_REG_H/gc2053_mipi_120fps_480p_day_reg.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg.h"
#endif
#ifdef CONFIG_ENABLE_AIISP
#include "SENSOR_H/gc2053_mipi_isp600_20231218_105501_aiisp.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_aiisp_reg.h"
#endif
#endif //CONFIG_SENSOR_GC2053_MIPI

#ifdef CONFIG_SENSOR_SC2336_MIPI
#include "SENSOR_H/sc2336_120fps_mipi_default_ini_v853.h"
#include "SENSOR_H/sc2336_mipi_default_ini_v853.h"
#include "SENSOR_REG_H/sc2336_mipi_120fps_960_280_day_reg.h"
#include "SENSOR_REG_H/sc2336_mipi_1080p_day_reg.h"
#endif //CONFIG_SENSOR_SC2336_MIPI

#ifdef CONFIG_SENSOR_SC3336_MIPI
#include "SENSOR_H/sc3336_120fps_mipi_default_ini_v853.h"
#include "SENSOR_H/sc3336_mipi_default_ini_v853.h"
#include "SENSOR_REG_H/sc3336_mipi_120fps_1152_320_day_reg.h"
#include "SENSOR_REG_H/sc3336_mipi_2304_1296_day_reg.h"
#endif //CONFIG_SENSOR_SC3336_MIPI

#ifdef CONFIG_SENSOR_SC5336_MIPI
#include "SENSOR_H/sc5336_mipi_default_ini_v853.h"
#include "SENSOR_H/sc5336_130fps_mipi_default_ini_v853.h"
#include "SENSOR_REG_H/sc5336_mipi_2880_1620_day_reg.h"
#include "SENSOR_REG_H/sc5336_mipi_130fps_1440_400_day_reg.h"
#endif // CONFIG_SENSOR_SC5336_MIPI

#ifdef CONFIG_SENSOR_IMX335_MIPI
#include "SENSOR_REG_H/imx335_mipi_2592_1944_day_reg.h"
#include "SENSOR_REG_H/imx335_mipi_2592_1944_wdr_day_reg.h"
#include "SENSOR_H/imx335_mipi_default_ini_v853.h"
#include "SENSOR_H/imx335_mipi_wdr_default_v853.h"
#endif // CONFIG_SENSOR_IMX335_MIPI

#ifdef CONFIG_SENSOR_IMX319_MIPI
#include "SENSOR_H/imx319_mipi_default_ini_a523.h"
#endif // CONFIG_SENSOR_IMX319_MIPI

#ifdef CONFIG_SENSOR_SC035HGS_MIPI
#include "SENSOR_H/sc035hgs_mipi_isp601_20230626.h"
#endif // CONFIG_SENSOR_SC035HGS_MIPI

#ifdef CONFIG_SENSOR_GC1084_MIPI
#ifdef CONFIG_SENSOR_GC1084_8BIT_MIPI
#include "SENSOR_H/gc1084_120fps_mipi_default_ini_v853_8bit.h"
#include "SENSOR_H/gc1084_mipi_v853_20230410_164555_day_8bit.h"
#include "SENSOR_REG_H/gc1084_mipi_120fps_360p_day_reg_8bit.h"
#include "SENSOR_REG_H/gc1084_mipi_720p_day_reg_8bit.h"
#else
#include "SENSOR_H/gc1084_120fps_mipi_default_ini_v853.h"
#include "SENSOR_H/gc1084_mipi_v853_20230410_164555_day.h"
#include "SENSOR_REG_H/gc1084_mipi_120fps_360p_day_reg.h"
#include "SENSOR_REG_H/gc1084_mipi_720p_day_reg.h"
#endif
#ifdef CONFIG_ENABLE_AIISP
#include "SENSOR_H/gc1084_mipi_isp600_20230703_152809_aiisp.h"
#include "SENSOR_REG_H/gc1084_mipi_720p_aiisp_reg.h"
#else
#include "SENSOR_H/gc1084_mipi_v853_20230410_164555_ir.h"
#endif
#endif // CONFIG_SENSOR_GC1084_MIPI

#ifdef CONFIG_SENSOR_BF2257CS_MIPI
#include "SENSOR_H/bf2257cs_mipi_2_isp600_20231219_201540_RGB_new.h"
#include "SENSOR_H/bf2257cs_mipi_2_isp600_20231205_154925_ir.h"
#endif // CONFIG_SENSOR_BF2257CS_MIPI

#ifdef CONFIG_SENSOR_SC2355_MIPI
#include "SENSOR_H/sc2355_mipi_isp600_20220726_230636_RGB_LSC_V18.h"
#include "SENSOR_H/sc2355_mipi_isp600_20220720_235901_IR_V42.h"
#endif // CONFIG_SENSOR_SC2355_MIPI

#else
#include "SENSOR_H/gc2053_mipi_default_ini_v853.h"
#include "SENSOR_REG_H/gc2053_mipi_default_ini_v853_reg_day.h"
#endif
//if (ISP_VERSION == 600 || ISP_VERSION == 601)

#elif (ISP_VERSION == 603)
#if defined CONFIG_SENSOR_GC2053_MIPI || defined CONFIG_SENSOR_GC1084_MIPI || defined CONFIG_SENSOR_GC2083_MIPI \
		|| defined CONFIG_SENSOR_GC05A2_MIPI || defined CONFIG_SENSOR_GC4663_MIPI || defined CONFIG_SENSOR_SC2336_MIPI \
		|| CONFIG_SENSOR_OV02B10_MIPI || CONFIG_SENSOR_IMX219_MIPI || CONFIG_SENSOR_SC2337P_MIPI || defined CONFIG_SENSOR_SC2331_MIPI

#ifdef CONFIG_SENSOR_GC2053_MIPI
#include "SENSOR_H/gc2053_120fps_mipi_default_ini_v821.h"
#include "SENSOR_H/gc2053_mipi_default_ini_v821.h"
#include "SENSOR_REG_H/gc2053_mipi_120fps_480p_day_reg.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg.h"
#endif //CONFIG_SENSOR_GC2053_MIPI

#ifdef CONFIG_SENSOR_GC2083_MIPI
#include "SENSOR_H/gc2083_mipi_isp603_20241205_171656_final_rgb_suit.h"
#include "SENSOR_REG_H/gc2083_mipi_1080p_day_reg_v821.h"
#include "SENSOR_REG_H/gc2083_mipi_120fps_360p_day_reg_v821.h"
#endif //CONFIG_SENSOR_GC2083_MIPI

#ifdef CONFIG_SENSOR_GC1084_MIPI
#include "SENSOR_H/gc1084_120fps_mipi_default_ini_v821.h"
#include "SENSOR_H/gc1084_mipi_isp603_20250217_102037_lens3_V32_rgb.h"
#include "SENSOR_REG_H/gc1084_mipi_120fps_360p_day_reg_v821.h"
#include "SENSOR_REG_H/gc1084_mipi_720p_day_reg_v821.h"
#endif //CONFIG_SENSOR_GC1084_MIPI

#ifdef CONFIG_SENSOR_GC05A2_MIPI
#include "SENSOR_H/gc05a2_mipi_isp603_500w_30fps_rgb.h"
#include "SENSOR_H/gc05a2_mipi_isp603_1080p_30fps_rgb.h"
#include "SENSOR_REG_H/gc05a2_mipi_2k_day_reg_2in1_v821.h"
#include "SENSOR_REG_H/gc05a2_mipi_1080p_day_reg_30fps.h"

#include "SENSOR_H/gc05a2_mipi_isp603_20250122_60fps.h"
#include "SENSOR_REG_H/gc05a2_mipi_720p_day_reg_2in1_v821.h"
#endif //CONFIG_SENSOR_GC05A2_MIPI

#ifdef CONFIG_SENSOR_GC4663_MIPI
#include "SENSOR_H/gc4663_mipi_isp603_20241224_101421_final_rgb.h"
#include "SENSOR_REG_H/gc4663_mipi_2560_1440_15fps_day_reg_v821.h"
#endif //CONFIG_SENSOR_GC4663_MIPI

#ifdef CONFIG_SENSOR_SC2336_MIPI
#include "SENSOR_H/sc2336_mipi_isp603_20250215_171639_V3_rgb.h"
#include "SENSOR_H/sc2336_mipi_isp603_20250215_171639_V3_ir.h"
#endif //CONFIG_SENSOR_SC2336_MIPI

#ifdef CONFIG_SENSOR_OV02B10_MIPI
#include "SENSOR_H/ov02b1b_mipi_default_ini_v821_color.h"
#include "SENSOR_H/ov02b1b_mipi_default_ini_v821_ir.h"
#endif // CONFIG_SENSOR_OV02B10_MIPI

#ifdef CONFIG_SENSOR_IMX219_MIPI
#include "SENSOR_H/imx219_mipi_3280_2464_30fps_rgb_isp603.h"
#include "SENSOR_H/imx219_mipi_1600_1200_30fps_rgb_isp603.h"
#ifdef CONFIG_SENSOR_IMX219_VERTICAL
#include "SENSOR_H/imx219_mipi_1440_1920_30fps_rgb_isp603.h"
#endif
#include "SENSOR_H/imx219_mipi_800_600_80fps_rgb_isp603.h"
//#include "SENSOR_H/imx219_mipi_1280_720_60fps_rgb_isp603.h"
//#include "SENSOR_REG_H/imx219_mipi_1280_720_60fps_rgb_reg.h"
#include "SENSOR_REG_H/imx219_mipi_1600_1200_30fps_rgb_reg.h"
#ifdef CONFIG_SENSOR_IMX219_VERTICAL
#include "SENSOR_REG_H/imx219_mipi_1440_1920_30fps_rgb_reg.h"
#endif
#include "SENSOR_REG_H/imx219_mipi_3280_2464_30fps_rgb_reg.h"
#endif // CONFIG_SENSOR_IMX219_MIPI

#ifdef CONFIG_SENSOR_SC2337P_MIPI
#include "SENSOR_H/sc2337p_mipi_isp603_20251022_103319_Day.h"
#include "SENSOR_H/sc2337p_mipi_isp603_20250925_170115_Night.h"
#include "SENSOR_H/sc2337p_mipi_isp603_120fps_20251022_103319_Day.h"

#include "SENSOR_REG_H/sc2337p_mipi_1080p_rgb_reg_20251022.h"
#include "SENSOR_REG_H/sc2337p_mipi_1080p_ir_reg_20251022.h"
#include "SENSOR_REG_H/sc2337p_mipi_1080p_rgb_120fps_reg_20250925.h"
#endif // CONFIG_SENSOR_SC2337P_MIPI

#ifdef CONFIG_SENSOR_SC2331_MIPI
#include "SENSOR_H/sc2331_mipi_isp603_20250828_105529_low.h"
#include "SENSOR_H/sc2331_mipi_isp603_20250910_110159_ir.h"
#include "SENSOR_REG_H/sc2331_mipi_1080p_day_reg_v821.h"
#endif //CONFIG_SENSOR_SC2331_MIPI

#else
#include "SENSOR_H/gc2053_mipi_default_ini_v821.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg.h"
#endif
//if (ISP_VERSION == 603)

#else
#include "SENSOR_H/gc2053_mipi_default_ini_v821.h"
#include "SENSOR_REG_H/gc2053_mipi_1080p_day_reg.h"
#endif

unsigned int isp_cfg_log_param = ISP_LOG_CFG;

#define SIZE_OF_LSC_TBL     (12*768*2)
#define SIZE_OF_GAMMA_TBL   (5*1024*3*2)

struct isp_cfg_array cfg_arr[] = {
#if (ISP_VERSION == 600 || ISP_VERSION == 601)
#if defined CONFIG_SENSOR_GC2053_MIPI || defined CONFIG_SENSOR_GC4663_MIPI || defined CONFIG_SENSOR_SC5336_MIPI || \
	defined CONFIG_SENSOR_IMX319_MIPI || defined CONFIG_SENSOR_SC035HGS_MIPI || defined CONFIG_SENSOR_SC3336_MIPI || \
	defined CONFIG_SENSOR_IMX335_MIPI || defined CONFIG_SENSOR_SC2336_MIPI || defined CONFIG_SENSOR_GC1084_MIPI || \
	defined CONFIG_SENSOR_BF2257CS_MIPI || defined CONFIG_SENSOR_SC2355_MIPI

#ifdef CONFIG_SENSOR_GC2053_MIPI
#ifdef CONFIG_SENSOR_GC2053_8BIT_MIPI
	{"gc2053_mipi", "gc2053_mipi_isp600_20231212_111630_day_8bit", 1920, 1088, 20, 0, 0, &gc2053_mipi_v853_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_day_8bit", 640, 480, 120, 0, 0, &gc2053_mipi_120fps_v853_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_night_8bit", 640, 480, 120, 0, 1, &gc2053_mipi_120fps_v853_isp_cfg},
#else
	{"gc2053_mipi", "gc2053_mipi_isp600_20231212_111630_day", 1920, 1088, 20, 0, 0, &gc2053_mipi_v853_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_day", 640, 480, 120, 0, 0, &gc2053_mipi_120fps_v853_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_night", 640, 480, 120, 0, 1, &gc2053_mipi_120fps_v853_isp_cfg},
#endif
#ifdef CONFIG_ENABLE_AIISP
	{"gc2053_mipi", "gc2053_mipi_isp600_20231218_105501_aiisp", 1920, 1088, 10, 0, 2, &gc2053_mipi_aiisp_isp_cfg},
#else
	{"gc2053_mipi", "gc2053_mipi_default_ini_v853_night", 1920, 1088, 20, 0, 1, &gc2053_mipi_v853_isp_cfg},
#endif //CONFIG_ENABLE_AIISP
#endif //CONFIG_SENSOR_GC2053_MIPI

#ifdef CONFIG_SENSOR_GC4663_MIPI
	{"gc4663_mipi", "gc4663_mipi_default_ini_day", 2560, 1440, 15, 0, 0, &gc4663_mipi_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_mipi_default_ini_night", 2560, 1440, 15, 0, 1, &gc4663_mipi_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_mipi_wdr_v853_isp_cfg_day", 2560, 1440, 15, 1, 0, &gc4663_mipi_wdr_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_mipi_wdr_v853_isp_cfg_night", 2560, 1440, 15, 1, 1, &gc4663_mipi_wdr_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_120fps_mipi_default_ini_day", 1280, 720, 120, 0, 0, &gc4663_120fps_mipi_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_120fps_mipi_default_ini_night", 1280, 720, 120, 0, 1, &gc4663_120fps_mipi_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_120fps_linear_to_wdr_day", 1280, 720, 120, 1, 0, &gc4663_120fps_mipi_linear_to_wdr_v853_isp_cfg},
	{"gc4663_mipi", "gc4663_120fps_linear_to_wdr_night", 1280, 720, 120, 1, 1, &gc4663_120fps_mipi_linear_to_wdr_v853_isp_cfg},
#endif // CONFIG_SENSOR_GC4663_MIPI

#ifdef CONFIG_SENSOR_SC2336_MIPI
	{"sc2336_mipi", "sc2336_120fps_mipi_default_ini_day", 960, 280, 120, 0, 0, &sc2336_mipi_120fps_isp_cfg},
	{"sc2336_mipi", "sc2336_120fps_mipi_default_ini_night", 960, 280, 120, 0, 1, &sc2336_mipi_120fps_isp_cfg},
	{"sc2336_mipi", "sc2336_mipi_default_ini_day", 1920, 1080, 20, 0, 0, &sc2336_mipi_isp_cfg},
	{"sc2336_mipi", "sc2336_mipi_default_ini_night", 1920, 1080, 20, 0, 1, &sc2336_mipi_isp_cfg},
#endif //CONFIG_SENSOR_SC2336_MIPI

#ifdef CONFIG_SENSOR_SC3336_MIPI
	{"sc3336_mipi", "sc3336_120fps_mipi_default_ini_day", 1152, 320, 120, 0, 0, &sc3336_mipi_120fps_isp_cfg},
	{"sc3336_mipi", "sc3336_120fps_mipi_default_ini_night", 1152, 320, 120, 0, 1, &sc3336_mipi_120fps_isp_cfg},
	{"sc3336_mipi", "sc3336_mipi_default_ini_day", 2304, 1296, 20, 0, 0, &sc3336_mipi_isp_cfg},
	{"sc3336_mipi", "sc3336_mipi_default_ini_night", 2304, 1296, 20, 0, 1, &sc3336_mipi_isp_cfg},
#endif //CONFIG_SENSOR_SC3336_MIPI

#ifdef CONFIG_SENSOR_SC5336_MIPI
	{"sc5336_mipi", "sc5336_130fps_mipi_default_ini_day", 1440, 400, 130, 0, 0, &sc5336_mipi_130fps_isp_cfg},
	{"sc5336_mipi", "sc5336_130fps_mipi_default_ini_night", 1440, 400, 130, 0, 1, &sc5336_mipi_130fps_isp_cfg},
	{"sc5336_mipi", "sc5336_mipi_default_ini_day", 2880, 1620, 20, 0, 0, &sc5336_mipi_isp_cfg},
	{"sc5336_mipi", "sc5336_mipi_default_ini_night", 2880, 1620, 20, 0, 1, &sc5336_mipi_isp_cfg},
#endif // CONFIG_SENSOR_SC5336_MIPI

#ifdef CONFIG_SENSOR_IMX335_MIPI
	{"imx335_mipi", "imx335_mipi_default_ini_v853", 2592, 1944, 25, 0, 0, &imx335_mipi_v853_isp_cfg},
	{"imx335_mipi", "imx335_mipi_wdr_default_v853", 2592, 1944, 25, 1, 0, &imx335_mipi_wdr_v853_isp_cfg},
#endif // CONFIG_SENSOR_IMX335_MIPI

#ifdef CONFIG_SENSOR_IMX319_MIPI
	{"imx319_mipi", "imx319_mipi_default_ini_a523", 3264, 2448, 30, 0, 0, &imx319_mipi_a523_isp_cfg},
#endif // CONFIG_SENSOR_IMX319_MIPI

#ifdef CONFIG_SENSOR_SC035HGS_MIPI
	{"sc035hgs_mipi", "sc035hgs_mipi_default_ini_a523", 640, 480, 120, 0, 0, &sc035hgs_mipi_isp_cfg},
#endif // CONFIG_SENSOR_SC035HGS_MIPI

#ifdef CONFIG_SENSOR_BF2257CS_MIPI
	{"bf2257cs_mipi", "bf2257cs_mipi_2_isp600_20231219_201540_RGB_new", 1600, 1200, 30, 0, 0, &bf2257cs_mipi_rgb_isp_cfg},
	{"bf2257cs_mipi", "bf2257cs_mipi_2_isp600_20231205_154925_ir", 1600, 1200, 30, 1, 0, &bf2257cs_mipi_ir_isp_cfg},
#endif // CONFIG_SENSOR_BF2257CS_MIPI

#ifdef CONFIG_SENSOR_SC2355_MIPI
	{"sc2355_mip", "sc2355_mipi_isp600_20220726_230636_RGB_LSC_V18", 1920, 1080, 15, 0, 0, &sc2355_mipi_rgb_isp_cfg},
	{"sc2355_mip", "sc2355_mipi_isp600_20220720_235901_IR_V42", 1920, 1080, 15, 1, 0, &sc2355_mipi_ir_isp_cfg},
#endif // CONFIG_SENSOR_SC2355_MIPI

#ifdef CONFIG_SENSOR_GC1084_MIPI
#ifdef CONFIG_SENSOR_GC1084_8BIT_MIPI
	{"gc1084_mipi", "gc1084_mipi_v853_20230410_164555_day_8bit", 1280, 720, 15, 0, 0, &gc1084_mipi_v853_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v853_day_8bit", 640, 360, 120, 0, 0, &gc1084_mipi_120fps_v853_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v853_night_8bit", 640, 360, 120, 0, 1, &gc1084_mipi_120fps_v853_isp_cfg},
#else
	{"gc1084_mipi", "gc1084_mipi_v853_20230410_164555_day", 1280, 720, 15, 0, 0, &gc1084_mipi_v853_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v853_day", 640, 360, 120, 0, 0, &gc1084_mipi_120fps_v853_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v853_night", 640, 360, 120, 0, 1, &gc1084_mipi_120fps_v853_isp_cfg},
#endif
#ifdef CONFIG_ENABLE_AIISP
	{"gc1084_mipi", "gc1084_mipi_isp600_20230703_152809_aiisp", 1280, 720, 10, 0, 2, &gc1084_mipi_aiisp_isp_cfg},
#else
	{"gc1084_mipi", "gc1084_mipi_v853_20230410_164555_ir", 1280, 720, 15, 0, 1, &gc1084_mipi_v853_ir_isp_cfg},
#endif
#endif //CONFIG_SENSOR_GC1084_MIPI

#else
	{"gc2053_mipi", "gc2053_mipi_default_ini_v853", 1920, 1088, 20, 0, 0, &gc2053_mipi_v853_isp_cfg},
#endif
//if (ISP_VERSION == 600 || ISP_VERSION == 601)

#elif (ISP_VERSION == 603)
#if defined CONFIG_SENSOR_GC2053_MIPI || defined CONFIG_SENSOR_GC1084_MIPI || defined CONFIG_SENSOR_GC2083_MIPI || \
		defined CONFIG_SENSOR_GC05A2_MIPI || defined CONFIG_SENSOR_GC4663_MIPI || defined CONFIG_SENSOR_SC2336_MIPI || \
		defined CONFIG_SENSOR_OV02B10_MIPI ||  defined CONFIG_SENSOR_IMX219_MIPI || CONFIG_SENSOR_SC2337P_MIPI || defined CONFIG_SENSOR_SC2331_MIPI

#ifdef CONFIG_SENSOR_GC2053_MIPI
	{"gc2053_mipi", "gc2053_mipi_default_ini_v821_day", 1920, 1088, 20, 0, 0, &gc2053_mipi_v821_isp_cfg},
	{"gc2053_mipi", "gc2053_mipi_default_ini_v821_night", 1920, 1088, 20, 0, 1, &gc2053_mipi_v821_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_day", 640, 480, 120, 0, 0, &gc2053_mipi_120fps_v821_isp_cfg},
	{"gc2053_mipi", "gc2053_120fps_mipi_default_ini_v853_night", 640, 480, 120, 0, 1, &gc2053_mipi_120fps_v821_isp_cfg},
#endif //CONFIG_SENSOR_GC2053_MIPI

#ifdef CONFIG_SENSOR_GC2083_MIPI
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 1080, 15, 0, 0, &gc2083_mipi_rgb_isp_cfg},
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 1080, 15, 0, 1, &gc2083_mipi_rgb_isp_cfg},
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 360, 90, 0, 0, &gc2083_mipi_rgb_isp_cfg},
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 360, 90, 0, 1, &gc2083_mipi_rgb_isp_cfg},
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 360, 120, 0, 0, &gc2083_mipi_rgb_isp_cfg},
	{"gc2083_mipi", "gc2083_mipi_isp603_20241205_171656_final_rgb_suit", 1920, 360, 120, 0, 1, &gc2083_mipi_rgb_isp_cfg},
#endif //CONFIG_SENSOR_GC2083_MIPI

#ifdef CONFIG_SENSOR_GC1084_MIPI
	{"gc1084_mipi", "gc1084_mipi_isp603_20250217_102037_lens3_V32_rgb", 1280, 720, 15, 0, 0, &gc1084_mipi_rgb_isp_cfg},
	{"gc1084_mipi", "gc1084_mipi_isp603_20250217_102037_lens3_V32_rgb", 1280, 720, 15, 0, 1, &gc1084_mipi_rgb_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v821_day", 640, 360, 120, 0, 0, &gc1084_mipi_120fps_v821_isp_cfg},
	{"gc1084_mipi", "gc1084_120fps_mipi_default_ini_v821_night", 640, 360, 120, 0, 1, &gc1084_mipi_120fps_v821_isp_cfg},
#endif //CONFIG_SENSOR_GC1084_MIPI

#ifdef CONFIG_SENSOR_GC4663_MIPI
	{"gc4663_mipi", "gc4663_mipi_isp603_20241224_101421_final_rgb", 2560, 1440, 15, 0, 0, &gc4663_mipi_rgb_isp_cfg},
#endif

#ifdef CONFIG_SENSOR_SC2336_MIPI
	{"sc2336_mipi", "sc2336_mipi_isp603_20250215_171639_V3_rgb", 1920, 1080, 20, 0, 0, &sc2336_mipi_rgb_isp_cfg},
	{"sc2336_mipi", "sc2336_mipi_isp603_20250215_171639_V3_ir", 1920, 1080, 20, 0, 1, &sc2336_mipi_ir_isp_cfg},
#endif

#ifdef CONFIG_SENSOR_GC05A2_MIPI
	{"gc05a2_mipi", "gc05a2_mipi_isp603_500w_30fps_rgb", 2592, 1944, 30, 0, 0, &gc05a2_mipi_500w_30fps_rgb_isp_cfg},
	{"gc05a2_mipi", "gc05a2_mipi_isp603_1080p_30fps_rgb", 1920, 1080, 30, 0, 0, &gc05a2_mipi_1080p_30fps_rgb_isp_cfg},
	{"gc05a2_mipi", "gc05a2_mipi_isp603_20250122_60fps", 1280, 720, 60, 0, 0, &gc05a2_mipi_60fps_isp_cfg},
#endif //CONFIG_SENSOR_GC05A2_MIPI

#ifdef CONFIG_SENSOR_OV02B10_MIPI
       {"ov02b10_mipi", "ov02b1b_mipi_default_ini_v821_color", 1600, 1200, 30, 0, 0, &ov02b1b_mipi_isp_cfg},
       {"ov02b1b_mipi", "ov02b1b_mipi_default_ini_v821_ir", 1600, 1200, 30, 0, 1, &ov02b1b_mipi_ir_isp_cfg},
       {"ov02b1b_mipi", "ov02b1b_mipi_default_ini_v821_ir", 1600, 1200, 30, 0, 0, &ov02b1b_mipi_ir_isp_cfg},
#endif // CONFIG_SENSOR_OV02B10_MIPI

#ifdef CONFIG_SENSOR_IMX219_MIPI
	{"imx219_mipi", "imx219_mipi_3280_2464_30fps_rgb_isp603", 3280, 2464, 10, 0, 0, &imx219_mipi_3280_2464_30fps_rgb_isp_cfg},
	{"imx219_mipi", "imx219_mipi_1600_1200_30fps_rgb_isp603", 1600, 1200, 30, 0, 0, &imx219_mipi_1600_1200_30fps_rgb_isp_cfg},
#ifdef CONFIG_SENSOR_IMX219_VERTICAL
	{"imx219_mipi", "imx219_mipi_1440_1920_30fps_rgb_isp603", 1440, 1920, 30, 0, 0, &imx219_mipi_1440_1920_30fps_rgb_isp_cfg},
#endif
	{"imx219_mipi", "imx219_mipi_800_600_80fps_rgb_isp603", 800, 600, 80, 0, 0, &imx219_mipi_800_600_80fps_rgb_isp_cfg},
	//{"imx219_mipi", "imx219_mipi_1280_720_60fps_rgb_isp603", 1280, 720, 60, 0, 0, &imx219_mipi_1280_720_60fps_rgb_isp_cfg},
#endif //CONFIG_SENSOR_IMX219_MIPI

#ifdef CONFIG_SENSOR_SC2337P_MIPI
	{"sc2337p_mipi", "sc2337p_mipi_isp603_120fps_20251022_103319_Day", 480, 270, 120, 0, 0, &sc2337p_mipi_120fps_isp_cfg},
	// {"sc2337p_mipi", "sc2337p_mipi_isp603_120fps_20251022_103319_Day", 480, 270, 120, 0, 1, &sc2337p_mipi_120fps_isp_cfg},
	{"sc2337p_mipi", "sc2337p_mipi_isp603_20251022_103319_Day", 1920, 1080, 15, 0, 0, &sc2337p_mipi_isp_cfg_day},
	// {"sc2337p_mipi", "sc2337p_mipi_isp603_20250925_170115_Night", 1920, 1080, 15, 0, 1, &sc2337p_mipi_isp_cfg_night},
#endif // CONFIG_SENSOR_SC2337P_MIPI

#ifdef CONFIG_SENSOR_SC2331_MIPI
	{"sc2331_mipi", "sc2331_mipi_isp603_20250828_105529_low", 1920, 1080, 15, 0, 0, &sc2331_mipi_isp_cfg},
	{"sc2331_mipi", "sc2331_mipi_isp603_20250910_110159_ir", 1920, 1080, 15, 0, 1, &sc2331_mipi_ir_isp_cfg},
#endif //CONFIG_SENSOR_SC2331_MIPI

#else
	{"gc2053_mipi", "gc2053_mipi_default_ini_v821", 1920, 1088, 20, 0, 0, &gc2053_mipi_v821_isp_cfg},
#endif
//(ISP_VERSION == 603)

#else
	{"gc2053_mipi", "gc2053_mipi_default_ini_v821", 1920, 1088, 20, 0, 0, &gc2053_mipi_v821_isp_cfg},
#endif
};

#if defined CONFIG_ISP_FAST_CONVERGENCE  || defined CONFIG_ISP_HARD_LIGHTADC
struct isp_hfr_lfr_link_array hfr_lfr_link_array[] = {

};
#endif

int read_flash_parameter(struct isp_param_config *param, char *isp_cfg_name)
{
	char time[20], notes[50];
	unsigned int size;

	size = *((unsigned int *)ISP_IQ_FILE_ADDR);
	if (size != sizeof(struct isp_param_config)) {
		//ISP_ERR("flash_param:0x%08x read size %d != isp_param size %d!\n", ISP_IQ_FILE_ADDR, size, (unsigned int)sizeof(struct isp_param_config));
		return -1;
	}
	memcpy(time, (void*)(ISP_IQ_FILE_ADDR + sizeof(size)), sizeof(time));
	memcpy(notes, (void*)(ISP_IQ_FILE_ADDR + sizeof(size) + sizeof(time)), sizeof(notes));
	if (param) {
		memcpy(param, (void*)(ISP_IQ_FILE_ADDR + sizeof(size) + sizeof(time) + sizeof(notes)), sizeof(struct isp_param_config));
	} else {
		ISP_ERR("param is NULL!\n");
		return -1;
	}
	memcpy(isp_cfg_name, notes, sizeof(notes));
	ISP_PRINT("Read 0x%08x seccess... Time:%s  Notes:%s\n", ISP_IQ_FILE_ADDR, time, notes);
	return 0;
}

int parser_ini_info(struct isp_param_config *param, char *isp_cfg_name, char *sensor_name,
			int w, int h, int fps, int wdr, int ir, int sync_mode, int isp_id)
{
	int i;
	struct isp_cfg_pt *cfg = NULL;

	//load reserved address parameter
	if (fps < ISP_FAST_CONVERGENCE_FPS) {
		if (!read_flash_parameter(param, isp_cfg_name))
			return 0;
	}

	//load header parameter
#if defined CONFIG_ISP_FAST_CONVERGENCE  || defined CONFIG_ISP_HARD_LIGHTADC
	//HFR use 2in1 mode
	int j, width_true, blw;
	struct isp_cfg_pt *hfr_cfg = NULL;
	struct isp_cfg_pt *lfr_cfg = NULL;
	if (fps >= ISP_FAST_CONVERGENCE_FPS && (((sync_mode >> 16) & 0xff) > 1)) {
		for (i = 0; i < ARRAY_SIZE(hfr_lfr_link_array); i++) {
			if (!strncmp(sensor_name, hfr_lfr_link_array[i].sensor_name, 20) &&
				(w == hfr_lfr_link_array[i].hfr_width) && (h == hfr_lfr_link_array[i].hfr_height) &&
				(fps == hfr_lfr_link_array[i].hfr_fps) && (wdr == hfr_lfr_link_array[i].hfr_wdr)) {
				for (j = 0; j < ARRAY_SIZE(cfg_arr); j++) {
					if (!strncmp(sensor_name, cfg_arr[j].sensor_name, 20) &&
						(hfr_lfr_link_array[i].hfr_width == cfg_arr[j].width) &&
						(hfr_lfr_link_array[i].hfr_height == cfg_arr[j].height) &&
						(hfr_lfr_link_array[i].hfr_fps == cfg_arr[j].fps) &&
						(hfr_lfr_link_array[i].hfr_wdr == cfg_arr[j].wdr)) {
						hfr_cfg = cfg_arr[j].cfg;
						ISP_PRINT("find %s_%d_%d_%d_%d [%s] hfr isp config\n", cfg_arr[j].sensor_name,
							cfg_arr[j].width, cfg_arr[j].height, cfg_arr[j].fps, cfg_arr[j].wdr, cfg_arr[j].isp_cfg_name);
						break;
					}
				}
				for (j = 0; j < ARRAY_SIZE(cfg_arr); j++) {
					if (!strncmp(sensor_name, cfg_arr[j].sensor_name, 20) &&
						(hfr_lfr_link_array[i].lfr_width == cfg_arr[j].width) &&
						(hfr_lfr_link_array[i].lfr_height == cfg_arr[j].height) &&
						(hfr_lfr_link_array[i].lfr_fps == cfg_arr[j].fps) &&
						(hfr_lfr_link_array[i].lfr_wdr == cfg_arr[j].wdr)) {
						lfr_cfg = cfg_arr[j].cfg;
						ISP_PRINT("find %s_%d_%d_%d_%d [%s] lfr isp config\n", cfg_arr[j].sensor_name,
							cfg_arr[j].width, cfg_arr[j].height, cfg_arr[j].fps, cfg_arr[j].wdr, cfg_arr[j].isp_cfg_name);
						break;
					}
				}
				break;
			}
		}

		if (hfr_cfg && lfr_cfg) {
			cfg = hfr_cfg;
			width_true = vin_set_large_overlayer(w);
			blw = width_true / 2 / 8;
			param->isp_test_settings = *cfg->isp_test_settings;
			param->isp_3a_settings = *cfg->isp_3a_settings;
			param->isp_iso_settings = *cfg->isp_iso_settings;
			param->isp_tunning_settings = *cfg->isp_tunning_settings;

			for (i = 0; i < 8; i++) {
				param->isp_tunning_settings.msc_blw_lut[i] = blw;
			}
			param->isp_tunning_settings.msc_blw_lut[0] += (width_true / 2) - (blw * 8);
			param->isp_tunning_settings.lsc_mode = lfr_cfg->isp_tunning_settings->lsc_mode;
			param->isp_tunning_settings.ff_mod = lfr_cfg->isp_tunning_settings->ff_mod;
			param->isp_tunning_settings.msc_mode = lfr_cfg->isp_tunning_settings->msc_mode;
			param->isp_tunning_settings.mff_mod = lfr_cfg->isp_tunning_settings->mff_mod;
			memcpy(&param->isp_tunning_settings.lsc_tbl[0][0], &lfr_cfg->isp_tunning_settings->lsc_tbl[0][0], sizeof(param->isp_tunning_settings.lsc_tbl));
			memcpy(&param->isp_tunning_settings.lsc_trig_cfg[0], &lfr_cfg->isp_tunning_settings->lsc_trig_cfg[0], sizeof(param->isp_tunning_settings.lsc_trig_cfg));
			memcpy(&param->isp_tunning_settings.msc_tbl[0][0], &lfr_cfg->isp_tunning_settings->msc_tbl[0][0], sizeof(param->isp_tunning_settings.msc_tbl));
			memcpy(&param->isp_tunning_settings.msc_trig_cfg[0], &lfr_cfg->isp_tunning_settings->msc_trig_cfg[0], sizeof(param->isp_tunning_settings.msc_trig_cfg));
			ISP_PRINT("isp use hfr config and lfr msc param\n");
			return 0;
		} else {
			ISP_WARN("cannot find hfr&lfr link.(hfr_cfg = %p, lfr_cfg = %p)\n", hfr_cfg, lfr_cfg);
		}
	}
#endif
	for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
		if (!strncmp(sensor_name, cfg_arr[i].sensor_name, 20) &&
		    (w == cfg_arr[i].width) && (h == cfg_arr[i].height) &&
		    (fps == cfg_arr[i].fps) && (wdr == cfg_arr[i].wdr) &&
		    (ir == cfg_arr[i].ir)) {
				cfg = cfg_arr[i].cfg;
				ISP_PRINT("find %s_%d_%d_%d_%d [%s] isp config\n", cfg_arr[i].sensor_name,
					cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr, cfg_arr[i].isp_cfg_name);
				break;
		}
	}

	if (i == ARRAY_SIZE(cfg_arr)) {
		for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
			if (!strncmp(sensor_name, cfg_arr[i].sensor_name, 20) && (wdr == cfg_arr[i].wdr)) {
				cfg = cfg_arr[i].cfg;
				ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use %s_%d_%d_%d_%d_%d -> [%s]\n", sensor_name, w, h, fps, wdr, ir,
				         cfg_arr[i].sensor_name, cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr,
				         cfg_arr[i].ir, cfg_arr[i].isp_cfg_name);
				break;
			}
		}
		if (i == ARRAY_SIZE(cfg_arr)) {
			for (i = 0; i < ARRAY_SIZE(cfg_arr); i++) {
				if (wdr == cfg_arr[i].wdr) {
					cfg = cfg_arr[i].cfg;
					ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use %s_%d_%d_%d_%d_%d -> [%s]\n", sensor_name, w, h, fps, wdr, ir,
					         cfg_arr[i].sensor_name, cfg_arr[i].width, cfg_arr[i].height, cfg_arr[i].fps, cfg_arr[i].wdr,
					         cfg_arr[i].ir, cfg_arr[i].isp_cfg_name);
					break;
				}
			}
		}
		if (i == ARRAY_SIZE(cfg_arr)) {
			ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp config, use default config [%s]\n",
				sensor_name, w, h, fps, wdr, ir, cfg_arr[i-1].isp_cfg_name);
			cfg = cfg_arr[i-1].cfg;// use the last one
		}
	}

	if (cfg != NULL) {
		strcpy(isp_cfg_name, cfg_arr[i].isp_cfg_name);
		param->isp_test_settings = *cfg->isp_test_settings;
		param->isp_3a_settings = *cfg->isp_3a_settings;
		param->isp_iso_settings = *cfg->isp_iso_settings;
		param->isp_tunning_settings = *cfg->isp_tunning_settings;
	}

	return 0;
}

struct isp_reg_array reg_arr[] = {
#if (ISP_VERSION == 600)
#ifdef CONFIG_SENSOR_GC2053_MIPI
#ifdef CONFIG_SENSOR_GC2053_8BIT_MIPI
	{"gc2053_mipi", "gc2053_mipi_120fps_480p_day_reg_8bit", 640, 480, 120, 0, 0, &gc2053_mipi_480p_isp_day_reg},
	{"gc2053_mipi", "gc2053_mipi_120fps_480p_night_reg_8bit", 640, 480, 120, 0, 1, &gc2053_mipi_480p_isp_day_reg},
	{"gc2053_mipi", "gc2053_mipi_1080p_20fps_day_reg_day_8bit", 1920, 1088, 20, 0, 0, &gc2053_mipi_isp_day_reg},
#else
	{"gc2053_mipi", "gc2053_mipi_120fps_480p_day_reg", 640, 480, 120, 0, 0, &gc2053_mipi_480p_isp_day_reg},
	{"gc2053_mipi", "gc2053_mipi_120fps_480p_night_reg", 640, 480, 120, 0, 1, &gc2053_mipi_480p_isp_day_reg},
	{"gc2053_mipi", "gc2053_mipi_1080p_20fps_day_reg_day", 1920, 1088, 20, 0, 0, &gc2053_mipi_isp_day_reg},
#endif
#ifdef CONFIG_ENABLE_AIISP
	{"gc2053_mipi", "gc2053_mipi_1080p_aiisp_reg", 1920, 1088, 10, 0, 2, &gc2053_mipi_aiisp_isp_reg},
#else
	{"gc2053_mipi", "gc2053_mipi_1080p_20fps_day_reg_night", 1920, 1088, 20, 0, 1, &gc2053_mipi_isp_day_reg},
#endif
#endif //CONFIG_SENSOR_GC2053_MIPI

#ifdef CONFIG_SENSOR_GC4663_MIPI
	{"gc4663_mipi", "gc4663_mipi_720p_120fps_day_reg", 1280, 720, 120, 0, 0, &gc4663_mipi_720p_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_720p_120fps_night_reg", 1280, 720, 120, 0, 1, &gc4663_mipi_720p_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_1440p_15fps_day_reg_day", 2560, 1440, 15, 0, 0, &gc4663_mipi_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_1440p_15fps_day_reg_night", 2560, 1440, 15, 0, 1, &gc4663_mipi_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_1440p_wdr_15fps_day_reg_day", 2560, 1440, 15, 1, 0, &gc4663_mipi_wdr_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_1440p_wdr_15fps_day_reg_night", 2560, 1440, 15, 1, 1, &gc4663_mipi_wdr_isp_day_reg},
#endif //CONFIG_SENSOR_GC4663_MIPI

#ifdef CONFIG_SENSOR_SC2336_MIPI
	{"sc2336_mipi", "sc2336_mipi_120fps_960_280_day_reg", 960, 280, 120, 0, 0, &sc2336_mipi_960_280_isp_day_reg},
	{"sc2336_mipi", "sc2336_mipi_120fps_960_280_night_reg", 960, 280, 120, 0, 1, &sc2336_mipi_960_280_isp_day_reg},
	{"sc2336_mipi", "sc2336_mipi_1080p_day_reg_day", 1920, 1080, 20, 0, 0, &sc2336_mipi_isp_day_reg},
	{"sc2336_mipi", "sc2336_mipi_1080p_day_reg_night", 1920, 1080, 20, 0, 1, &sc2336_mipi_isp_day_reg},
#endif //CONFIG_SENSOR_SC2336_MIPI

#ifdef CONFIG_SENSOR_SC3336_MIPI
	{"sc3336_mipi", "sc3336_mipi_120fps_1152_320_day_reg", 1152, 320, 120, 0, 0, &sc3336_mipi_1152_320_isp_day_reg},
	{"sc3336_mipi", "sc3336_mipi_120fps_1152_320_night_reg", 1152, 320, 120, 0, 1, &sc3336_mipi_1152_320_isp_day_reg},
	{"sc3336_mipi", "sc3336_mipi_2304_1296_day_reg_day", 2304, 1296, 20, 0, 0, &sc3336_mipi_isp_day_reg},
	{"sc3336_mipi", "sc3336_mipi_2304_1296_day_reg_night", 2304, 1296, 20, 0, 1, &sc3336_mipi_isp_day_reg},
#endif //CONFIG_SENSOR_SC3336_MIPI

#ifdef CONFIG_SENSOR_SC5336_MIPI
	{"sc5336_mipi", "sc5336_mipi_130fps_1440_400_day_reg", 1440, 400, 130, 0, 0, &sc5336_mipi_1440_400_isp_day_reg},
	{"sc5336_mipi", "sc5336_mipi_130fps_1440_400_night_reg", 1440, 400, 130, 0, 1, &sc5336_mipi_1440_400_isp_day_reg},
	{"sc5336_mipi", "sc5336_mipi_2880_1620_day_reg_day", 2880, 1620, 20, 0, 0, &sc5336_mipi_isp_day_reg},
	{"sc5336_mipi", "sc5336_mipi_2880_1620_day_reg_night", 2880, 1620, 20, 0, 1, &sc5336_mipi_isp_day_reg},
#endif // CONFIG_SENSOR_SC5336_MIPI

#ifdef CONFIG_SENSOR_IMX335_MIPI
	{"imx335_mipi", "imx335_mipi_2592_1944_day_reg", 2592, 1944, 25, 0, 0, &imx335_mipi_isp_day_reg},
	{"imx335_mipi", "imx335_mipi_2592_1944_night_reg", 2592, 1944, 25, 0, 1, &imx335_mipi_isp_day_reg},
	{"imx335_mipi", "imx335_mipi_wdr_2592_1944_day_reg", 2592, 1944, 25, 1, 0, &imx335_mipi_wdr_isp_day_reg},
	{"imx335_mipi", "imx335_mipi_wdr_2592_1944_night_reg", 2592, 1944, 25, 1, 1, &imx335_mipi_wdr_isp_day_reg},
#endif //CONFIG_SENSOR_IMX335_MIPI

#ifdef CONFIG_SENSOR_GC1084_MIPI
#ifdef CONFIG_SENSOR_GC1084_8BIT_MIPI
    {"gc1084_mipi", "gc1084_mipi_120fps_360p_day_reg_8bit", 640, 360, 120, 0, 0, &gc1084_mipi_360p_isp_day_reg},
    {"gc1084_mipi", "gc1084_mipi_120fps_360p_night_reg_8bit", 640, 360, 120, 0, 1, &gc1084_mipi_360p_isp_day_reg},
    {"gc1084_mipi", "gc1084_mipi_720p_day_reg_8bit", 1280, 720, 15, 0, 0, &gc1084_mipi_isp_day_reg},
#else
    {"gc1084_mipi", "gc1084_mipi_120fps_360p_day_reg", 640, 360, 120, 0, 0, &gc1084_mipi_360p_isp_day_reg},
    {"gc1084_mipi", "gc1084_mipi_120fps_360p_night_reg", 640, 360, 120, 0, 1, &gc1084_mipi_360p_isp_day_reg},
    {"gc1084_mipi", "gc1084_mipi_720p_day_reg", 1280, 720, 15, 0, 0, &gc1084_mipi_isp_day_reg},
#endif
#ifdef CONFIG_ENABLE_AIISP
    {"gc1084_mipi", "gc1084_mipi_720p_aiisp_reg", 1280, 720, 10, 0, 2, &gc1084_mipi_isp_aiisp_reg},
#else
    {"gc1084_mipi", "gc1084_mipi_720p_night_reg", 1280, 720, 15, 0, 1, &gc1084_mipi_isp_day_reg},
#endif
#endif //CONFIG_SENSOR_GC1084_MIPI

//(ISP_VERSION >= 600)

#elif (ISP_VERSION == 603)
#ifdef CONFIG_SENSOR_GC2083_MIPI
	{"gc2083_mipi", "gc2083_mipi_120fps_360p_day_reg_v821", 1920, 360, 120, 0, 0, &gc2083_mipi_120fps_360p_isp_day_reg},
	{"gc2083_mipi", "gc2083_mipi_120fps_360p_day_reg_v821", 1920, 360, 120, 0, 1, &gc2083_mipi_120fps_360p_isp_day_reg},
	{"gc2083_mipi", "gc2083_mipi_1080p_day_reg_v821", 1920, 1080, 15, 0, 0, &gc2083_mipi_isp_day_reg},
	{"gc2083_mipi", "gc2083_mipi_1080p_day_reg_v821", 1920, 1080, 15, 0, 1, &gc2083_mipi_isp_day_reg},
#endif

#ifdef CONFIG_SENSOR_GC1084_MIPI
	{"gc1084_mipi", "gc1084_mipi_120fps_360p_day_reg_v821", 640, 360, 120, 0, 0, &gc1084_mipi_360p_isp_day_reg},
	{"gc1084_mipi", "gc1084_mipi_120fps_360p_night_reg_v821", 640, 360, 120, 0, 1, &gc1084_mipi_360p_isp_day_reg},
	{"gc1084_mipi", "gc1084_mipi_720p_day_reg_v821", 1280, 720, 15, 0, 0, &gc1084_mipi_isp_day_reg},
	{"gc1084_mipi", "gc1084_mipi_720p_night_reg_v821", 1280, 720, 15, 0, 1, &gc1084_mipi_isp_day_reg},
#endif

#ifdef CONFIG_SENSOR_GC05A2_MIPI
	{"gc05a2_mipi", "gc05a2_mipi_2k_day_reg_2in1_v821", 2592, 1944, 30, 0, 0, &gc05a2_mipi_isp_day_reg},
	{"gc05a2_mipi", "gc05a2_mipi_1080p_day_reg_30fps", 1920, 1080, 30, 0, 0, &gc05a2_mipi_1080p_30fps_isp_day_reg},
	{"gc05a2_mipi", "gc05a2_mipi_2k_night_reg_2in1_v821", 2592, 1944, 30, 0, 1, &gc05a2_mipi_isp_day_reg},
	{"gc05a2_mipi", "gc05a2_mipi_720p_day_reg_2in1_v821", 1280, 720, 60, 0, 0, &gc05a2_mipi_720p_isp_day_reg},
	{"gc05a2_mipi", "gc05a2_mipi_720p_day_reg_2in1_v821", 1280, 720, 60, 0, 0, &gc05a2_mipi_720p_isp_day_reg},
#endif

#ifdef CONFIG_SENSOR_GC4663_MIPI
	{"gc4663_mipi", "gc4663_mipi_isp_save_day_reg", 2560, 1440, 15, 0, 0, &gc4663_mipi_isp_day_reg},
	{"gc4663_mipi", "gc4663_mipi_isp_save_night_reg", 2560, 1440, 15, 0, 1, &gc4663_mipi_isp_day_reg},
#endif

#ifdef CONFIG_SENSOR_SC2337P_MIPI
	// {"sc2337p_mipi", "sc2337p_mipi_1080p_rgb_120fps_reg_20250925", 480, 270, 120, 0, 0, &sc2337p_mipi_isp_120fps_rgb_reg},
	{"sc2337p_mipi", "sc2337p_mipi_1080p_rgb_reg_20251022", 1920, 1080, 15, 0, 0, &sc2337p_mipi_isp_rgb_reg},
	// {"sc2337p_mipi", "sc2337p_mipi_1080p_ir_reg_20251022", 1920, 1080, 15, 0, 1, &sc2337p_mipi_isp_ir_reg},
#endif

#ifdef CONFIG_SENSOR_IMX219_MIPI
	//{"imx219_mipi", "imx219_mipi_1280_720_60fps_rgb_reg", 1280, 720, 60, 0, 0, &imx219_mipi_720p_isp_day_reg},
	{"imx219_mipi", "imx219_mipi_1600_1200_30fps_rgb_reg", 1600, 1200, 30, 0, 0, &imx219_mipi_1080p_isp_day_reg},
#ifdef CONFIG_SENSOR_IMX219_VERTICAL
	{"imx219_mipi", "imx219_mipi_1440_1920_30fps_rgb_reg", 1440, 1920, 30, 0, 0, &imx219_mipi_1440p_isp_day_reg},
#endif
	{"imx219_mipi", "imx219_mipi_3280_2464_30fps_rgb_reg", 3280, 2464, 10, 0, 0, &imx219_mipi_2464p_isp_day_reg},
#endif

#ifdef CONFIG_SENSOR_SC2331_MIPI
	{"sc2331_mipi", "sc2331_mipi_1080p_day_reg_v821", 1920, 1080, 15, 0, 0, &sc2331_mipi_isp_day_reg},
#endif

#endif //(ISP_VERSION == 603)

};

int parser_ini_regs_info(struct isp_lib_context *ctx, char *sensor_name,
			int w, int h, int fps, int wdr, int ir)
{
#if defined CONFIG_ARCH_SUN20IW3 || defined CONFIG_ARCH_SUN300IW1
	int i;
	struct isp_reg_pt *reg = NULL;

	for (i = 0; i < ARRAY_SIZE(reg_arr); i++) {
		if (!strncmp(sensor_name, reg_arr[i].sensor_name, 6) &&
			(w == reg_arr[i].width) && (h == reg_arr[i].height) &&// (fps == reg_arr[i].fps) &&
			(wdr == reg_arr[i].wdr)) {

			if (reg_arr[i].ir == ir) {
				reg = reg_arr[i].reg;
				ISP_PRINT("find %s_%d_%d_%d_%d ---- [%s] isp reg\n", reg_arr[i].sensor_name,
					reg_arr[i].width, reg_arr[i].height, reg_arr[i].fps, reg_arr[i].wdr, reg_arr[i].isp_cfg_name);
				break;
			}
		}
	}

	if (i == ARRAY_SIZE(reg_arr)) {
		ISP_WARN("cannot find %s_%d_%d_%d_%d_%d isp reg!!!\n", sensor_name, w, h, fps, wdr, ir);
		return -1;
	}

	if (reg != NULL) {
		if (reg->isp_save_reg)
			memcpy(ctx->load_reg_base, reg->isp_save_reg, ISP_LOAD_REG_SIZE);
		if (reg->isp_save_fe_table)
			memcpy(ctx->module_cfg.fe_table, reg->isp_save_fe_table, ISP_FE_TABLE_SIZE);
		if (reg->isp_save_bayer_table) {
			memcpy(ctx->module_cfg.bayer_table, reg->isp_save_bayer_table, ISP_BAYER_TABLE_SIZE);
			if (ctx->stitch_mode == STITCH_2IN1_LINNER && ctx->isp_id == 0) {
#if (ISP_VERSION == 600 || ISP_VERSION == 601)
				memcpy(ctx->load_reg_base + 0x20, ctx->load_reg_base + 0x40, 0x20);
#else
				memcpy(ctx->load_reg_base + 0x20, ctx->load_reg_base + 0x50, 0x30);
#endif
				memcpy(ctx->module_cfg.msc_table, ctx->module_cfg.lens_table, ISP_RSC_TBL_SIZE);
			}
		}
		if (reg->isp_save_rgb_table)
			memcpy(ctx->module_cfg.rgb_table, reg->isp_save_rgb_table, ISP_RGB_TABLE_SIZE);
		if (reg->isp_save_yuv_table)
			memcpy(ctx->module_cfg.yuv_table, reg->isp_save_yuv_table, ISP_YUV_TABLE_SIZE);
	}
#else
	void *isp_save_reg, *isp_save_fe_table, *isp_save_bayer_table, *isp_save_rgb_table, *isp_save_yuv_table;

	isp_save_reg = (void *)ISP_PARA_READ;
	isp_save_fe_table = isp_save_reg + ISP_LOAD_REG_SIZE;
	isp_save_bayer_table = isp_save_fe_table + ISP_FE_TABLE_SIZE;
	isp_save_rgb_table = isp_save_bayer_table + ISP_BAYER_TABLE_SIZE;
	isp_save_yuv_table = isp_save_rgb_table + ISP_RGB_TABLE_SIZE;

	if (*((unsigned int *)isp_save_reg) != 0xAAEEBBEE) {
		ISP_WARN("cannot get reg&table from memory&reserved addr\n");
		return -1;
	} else
		ISP_PRINT("get reg&table from memory&reserved addr\n");

	memcpy(ctx->load_reg_base, isp_save_reg, ISP_LOAD_REG_SIZE);
	memcpy(ctx->module_cfg.fe_table, isp_save_fe_table, ISP_FE_TABLE_SIZE);
	memcpy(ctx->module_cfg.bayer_table, isp_save_bayer_table, ISP_BAYER_TABLE_SIZE);
	memcpy(ctx->module_cfg.rgb_table, isp_save_rgb_table, ISP_RGB_TABLE_SIZE);
	memcpy(ctx->module_cfg.yuv_table, isp_save_yuv_table, ISP_YUV_TABLE_SIZE);
#endif
	return 0;
}
