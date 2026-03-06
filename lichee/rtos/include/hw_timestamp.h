#ifndef __HW_TIMESTAMP_H__
#define __HW_TIMESTAMP_H__

#include <stdint.h>
#include <inttypes.h>

struct clock_source;

typedef struct clock_source {
	uint64_t (*read)(struct clock_source *cs);
	struct {
		uint32_t mult;
		uint32_t shift;
	} cyc2us;
	struct {
		uint32_t mult;
		uint32_t shift;
	} us2cyc;

	struct {
		uint32_t mult;
		uint32_t shift;
	} cyc2ns;
	struct {
		uint32_t mult;
		uint32_t shift;
	} ns2cyc;

	const char *name;
	uint64_t mask;
	uint32_t freq;
} clock_source_t;

int hw_timestamp_init(clock_source_t *cs);

uint64_t hw_get_timestamp_us(void);
uint64_t hw_get_timestamp_ns(void);
uint64_t hw_get_count_value(void);
uint32_t hw_get_count_freq(void);

#endif  /* __HW_TIMESTAMP_H__ */
