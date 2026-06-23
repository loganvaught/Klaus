/*
 * micro_delay.h
 *
 *  Created on: Jun 17, 2026
 *      Author: Logan Vaught
 */

#ifndef MICRO_DELAY_H
#define MICRO_DELAY_H

#include "main.h"

typedef struct {
	TIM_HandleTypeDef *tim;
} micro_delay_handle_t;

typedef enum {
	MICRO_DELAY_OK,
	MICRO_DELAY_Error,
} micro_delay_result_t;

micro_delay_result_t wait_us(micro_delay_handle_t *tim_handle, uint16_t micro_seconds);
micro_delay_result_t micro_delay_init(micro_delay_handle_t *tim_handle, TIM_HandleTypeDef *tim);

#endif /* MICRO_DELAY_H */
