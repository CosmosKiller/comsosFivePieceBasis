/**
 * @file factory_reset_task.h
 * @brief Long-press GPIO handler that triggers a Matter factory reset.
 */

#ifndef COSMOS_FACTORY_RESET_TASK_H_
#define COSMOS_FACTORY_RESET_TASK_H_

#include <hal/gpio_types.h>
#include <sdkconfig.h>

/** Factory-reset button GPIO (see CONFIG_FACTORY_RESET_BUTTON_GPIO). */
#define FACTORY_RESET_BUTTON_PIN ((gpio_num_t)CONFIG_FACTORY_RESET_BUTTON_GPIO)

/**
 * @brief Start monitoring the factory-reset button (long-press erases fabrics and reboots).
 */
void factory_reset_task(void);

#endif /* COSMOS_FACTORY_RESET_TASK_H_ */
