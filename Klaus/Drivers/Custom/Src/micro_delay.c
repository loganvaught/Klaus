/*
 * micro_delay.c
 *
 *  Created on: Jun 17, 2026
 *      Author: Logan Vaught
 */

#include "micro_delay.h"

/*
  * @brief Wait for a given number of microseconds. Requires that the given micro_delay_handle_t is set up with
  * a timer running at 1MHz.
  * @param tim_handle the micro_delay_handle_t variable to operate on
  * @param microseconds the number of microseconds to wait
  * @retval Either MICRO_DELAY_OK or MICRO_DELAY_Error
 */
micro_delay_result_t wait_us(micro_delay_handle_t *tim_handle, uint16_t microseconds) {
	if (tim_handle == NULL) return MICRO_DELAY_Error;
	TIM_TypeDef *timer = tim_handle->tim->Instance;

	uint16_t start = timer->CNT;
    while ((uint16_t)(timer->CNT - start) < microseconds);

    return MICRO_DELAY_OK;
}

/*
  * @brief Setup a timer to be used with wait_us. Must be on and running at 1MHz for wait_us to run correctly.
  * @param tim_handle the micro_delay_handle_t variable to operate on
  * @param tim the address of the TIM_HandleTypeDef to set for the micro_delay_handle_t
  * @retval Either MICRO_DELAY_OK or MICRO_DELAY_Error
 */
micro_delay_result_t micro_delay_init(micro_delay_handle_t *tim_handle, TIM_HandleTypeDef *tim) {
	if (tim_handle == NULL) return MICRO_DELAY_Error;

	tim_handle->tim = tim;

	return MICRO_DELAY_OK;
}
