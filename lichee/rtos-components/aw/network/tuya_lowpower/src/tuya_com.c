#include "tuya_ipc_low_power_api.h"

void tuya_lp_reboot(void)
{
	extern void HAL_RTCWDG_Reboot();
	HAL_RTCWDG_Reboot();
}
