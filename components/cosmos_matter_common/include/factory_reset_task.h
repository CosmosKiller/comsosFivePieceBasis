/**
 * @file factory_reset_task.h
 * @brief Long-press GPIO handler that triggers a Matter factory reset.
 */

#ifndef COSMOS_FACTORY_RESET_TASK_H_
#define COSMOS_FACTORY_RESET_TASK_H_

#include <hal/gpio_types.h>

#define FACTORY_RESET_BUTTON_PIN GPIO_NUM_9 /*!< Factory-reset button GPIO */

/**
 * @brief Start monitoring the factory-reset button (long-press erases fabrics and reboots).
 */
void factory_reset_task(void);

#endif /* COSMOS_FACTORY_RESET_TASK_H_ */
