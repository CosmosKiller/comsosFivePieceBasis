#ifndef FACTORY_RESET_TASK_H_
#define FACTORY_RESET_TASK_H_

#include <hal/gpio_types.h>

#define FACTORY_RESET_BUTTON_PIN GPIO_NUM_9

/**
 * @brief Factory reset task.
 *        In the Matter protocol, a factory reset clears the device's fabric table,
 *        erases its operational credentials
 *        and automatically drops it from all ecosystems
 */
void factory_reset_task(void);

#endif /* FACTORY_RESET_TASK_H_ */