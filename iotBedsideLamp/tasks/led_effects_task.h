/**
 * @file led_effects_task.h
 * @brief Local color presets cycled from the user button.
 */

#ifndef LED_EFFECTS_TASK_H_
#define LED_EFFECTS_TASK_H_

#include <stdint.h>

/**
 * @brief Advance to the next built-in preset and push it to Matter attributes.
 *
 * @param endpoint_id Extended color light endpoint.
 */
void led_effects_cycle_preset(uint16_t endpoint_id);

#endif /* LED_EFFECTS_TASK_H_ */
