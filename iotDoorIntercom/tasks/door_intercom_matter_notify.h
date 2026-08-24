#ifndef DOOR_INTERCOM_MATTER_NOTIFY_H_
#define DOOR_INTERCOM_MATTER_NOTIFY_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Task-context Matter updates (never call from GPIO ISR). */
void door_intercom_matter_notify_doorbell(bool pressed);
void door_intercom_matter_notify_occupancy(bool occupancy);
void door_intercom_matter_notify_tamper(bool tampered);

#ifdef __cplusplus
}
#endif

#endif /* DOOR_INTERCOM_MATTER_NOTIFY_H_ */
