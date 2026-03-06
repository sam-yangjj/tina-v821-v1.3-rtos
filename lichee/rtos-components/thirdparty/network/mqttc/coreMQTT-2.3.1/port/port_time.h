#ifndef PORT_TIME_H_
#define PORT_TIME_H_

#include "stdint.h"
#include "task.h"

#define portGetTimeMs mqttGetTimeMs

uint32_t portGetTimeMs(void);

#endif /* PORT_TIME_H_ */
