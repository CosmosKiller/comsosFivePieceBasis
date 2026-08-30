#ifndef DOOR_SENSOR_MATTER_NOTIFY_H_
#define DOOR_SENSOR_MATTER_NOTIFY_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Task-context Matter updates (never call from GPIO ISR). */
void door_sensor_matter_notify_panic(bool active);

/** Latch or reflect siren OnOff (e.g. after intrusion). */
void door_sensor_matter_notify_siren(bool on);

#ifdef __cplusplus
}
#endif

#endif /* DOOR_SENSOR_MATTER_NOTIFY_H_ */
