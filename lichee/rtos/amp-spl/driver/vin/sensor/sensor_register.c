#include "sensor_register.h"

struct sensor_cfg_array sensor_array[] = {
#ifdef CONFIG_SENSOR_OV7251_MIPI
        {"ov7251_mipi", &ov7251_core},
#endif
#ifdef CONFIG_SENSOR_GC1084_MIPI
        {"gc1084_mipi", &gc1084_core},
#endif
};

struct sensor_fuc_core *find_sensor_func(char *sensor_name)
{
        int i;

        if (!ARRAY_SIZE(sensor_array)) {
                vin_err("%s:sensor_array is NULL\n", __func__);
                return NULL;
        }

	for (i = 0; i < ARRAY_SIZE(sensor_array); i++) {
                if (!strncmp(sensor_name, sensor_array[i].sensor_name, 6)) {
                        vin_print("%s: find %s sensor core function!\n", __func__, sensor_name);
                        return sensor_array[i].sensor_core;
                }
        }

        return NULL;
}
