#ifndef __INCLUDE_H__
#define __INCLUDE_H__

/* general header */
#include <stdarg.h>		/* use gcc stardard header file */
#include <inttypes.h>		/* use gcc stardard header file */
#include <stddef.h>		/* use gcc stardard define keyword, like size_t, ptrdiff_t */
#include "autoconf.h"
#include "./types.h"
#include "./error.h"

/* system headers */
#include <arch/cpu.h>

/* libary */
#include "./library.h"

/* driver headers */
#if defined(CONFIG_PIN_USED)
#include <driver/gpio.h>
#endif
#if defined(CONFIG_IRQCHIP_USED)
#include <driver/intc.h>
#endif
#if defined(CONFIG_UART_USED)
#include <driver/uart.h>
#endif
#if defined(CONFIG_TWI_USED)
#include <driver/twi.h>
#endif
#if defined(CONFIG_PMU_USED)
#include <driver/axp.h>
#endif
#if defined(CONFIG_VIN_USED)
#include <driver/vin.h>
#endif
#endif /* __INCLUDE_H__ */
