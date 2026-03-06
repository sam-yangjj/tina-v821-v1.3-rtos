#include "port_time.h"

uint32_t portGetTimeMs(void)
{
	return (uint32_t)xTaskGetTickCount();
}
