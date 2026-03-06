#ifndef __VIN_H__
#define __VIN_H__

#include <include.h>

extern unsigned int vin_log_mask;

typedef enum
{
    GPIO_PULL_DOWN_DISABLED    = 0,        /**< Defines GPIO pull up and pull down disable.  */
    GPIO_PULL_UP          = 1,        /**< Defines GPIO is pull up state.  */
    GPIO_PULL_DOWN            = 2,        /**< Defines GPIO is pull down state. */
} gpio_pull_status_t;

typedef enum
{
    GPIO_DIRECTION_INPUT  = 0,              /**<  GPIO input direction. */
    GPIO_DIRECTION_OUTPUT = 1               /**<  GPIO output direction. */
} gpio_direction_t;

typedef enum
{
    GPIO_DATA_LOW  = 0,                     /**<  GPIO data low. */
    GPIO_DATA_HIGH = 1                      /**<  GPIO data high. */
} gpio_data_t;

#define VIN_LOG_MD				(1 << 0) 	/*0x1 */
#define VIN_LOG_FLASH				(1 << 1) 	/*0x2 */
#define VIN_LOG_CCI				(1 << 2) 	/*0x4 */
#define VIN_LOG_CSI				(1 << 3) 	/*0x8 */
#define VIN_LOG_MIPI				(1 << 4) 	/*0x10*/
#define VIN_LOG_ISP				(1 << 5) 	/*0x20*/
#define VIN_LOG_STAT				(1 << 6) 	/*0x40*/
#define VIN_LOG_SCALER				(1 << 7) 	/*0x80*/
#define VIN_LOG_POWER				(1 << 8) 	/*0x100*/
#define VIN_LOG_CONFIG				(1 << 9) 	/*0x200*/
#define VIN_LOG_VIDEO				(1 << 10)	/*0x400*/
#define VIN_LOG_FMT				(1 << 11)	/*0x800*/
#define VIN_LOG_TDM				(1 << 12)	/*0x1000*/
#define VIN_LOG_STAT1				(1 << 13) 	/*0x2000*/

#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			printk("[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			printk("[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			printk("[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			printk("[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			printk("[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			printk("[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			printk("[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			printk("[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			printk("[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			printk("[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			printk("[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			printk("[VIN_LOG_FMT]" arg); \
			break; \
		default: \
			printk("[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)

#define vin_err(x, arg...) printk("[VIN_ERR]%s, line: %d," x, __FUNCTION__, __LINE__, ##arg)
#define vin_warn(x, arg...) printk("[VIN_WRN]" x, ##arg)
#define vin_print(x, arg...) printk("[VIN]" x, ##arg)


int csi_init(void);
#endif
