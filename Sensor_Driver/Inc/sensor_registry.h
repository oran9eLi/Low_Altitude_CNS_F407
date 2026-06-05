#ifndef SENSOR_REGISTRY_H
#define SENSOR_REGISTRY_H

#include <stdint.h>
#include "sensor_driver.h"

Sensor_Severity_t Sensor_RegistryInitAll(void);
Sensor_Severity_t Sensor_RegistrySelfCheckAll(Sensor_Status_t *status_list, uint16_t max_count, uint16_t *out_count);
Sensor_Severity_t Sensor_RegistryReadAll(Sensor_Sample_t *samples, uint16_t max_sample_count, uint16_t *out_sample_count, Sensor_Status_t *status_list, uint16_t max_status_count, uint16_t *out_status_count);
Sensor_Severity_t Sensor_RegistryGetStatusAll(Sensor_Status_t *status_list, uint16_t max_count, uint16_t *out_count);
uint16_t Sensor_RegistryGetCount(void);
const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index);

#endif
