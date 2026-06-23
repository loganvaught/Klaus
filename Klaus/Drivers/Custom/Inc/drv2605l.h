/*
 * drv2605l.h
 *
 *  Created on: Jun 10, 2026
 *      Author: Logan Vaught
 */

#ifndef DRV2605L_H
#define DRV2605L_H

#include "main.h"

// Device address (NOT already shifted for I2C 7-bit addressing)
#define HAPTIC_DEV_ADDR 0x5A

// Handlers
typedef struct {
	I2C_HandleTypeDef *i2c;
} drv2605l_handle_t;

typedef enum {
	DRV2605L_OK,
	DRV2605L_Error
} drv2605l_result_t;

// Register addresses
#define DRV2605L_MODE 0x01
#define DRV2605L_FEEDBACK_CONTROL 0x1A
#define DRV2605L_RATED_VOLTAGE 0x16
#define DRV2605L_OD_CLAMP 0x17
#define DRV2605L_CONTROL1 0x1B
#define DRV2605L_GO 0x0C
#define DRV2605L_STATUS 0x00
#define DRV2605L_LIBRARY_SELECTION 0x03
#define DRV2605L_WAVEFORM_SEQUENCER_1 0x04

// Constants for calibration of ELV1411A
#define DRV2605L_RATED_VOLTAGE_VALUE 0x56
#define DRV2605L_OD_CLAMP_VALUE 0x84
#define DRV2605L_DRIVE_TIME_VALUE 0x1C
#define DRV2605L_AUTO_CALIBRATION_MODE 0x07
#define DRV2605L_WAIT_TIME_CALIBRATE_MS 100
#define DRV2605L_MAX_WAIT_CALIBRATE_CYCLES 10

// Constants for initialization of ELV1411A
#define DRV2605L_WAIT_TIME_READY_MS 500
#define DRV2605L_LIBRARY_SELECTION_VALUE 0x06
#define DRV2605L_INTERNAL_TRIGGER_MODE 0

// Constants for metronome
#define DRV2605L_WAVEFORM_SELECT_MIN 1
#define DRV2605L_WAVEFORM_SELECT_MAX 123
#define DRV2605L_TEMPO_MAX 208
#define DRV2605L_TEMPO_MIN 30

// Functions
drv2605l_result_t drv2605l_write_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t data);
drv2605l_result_t drv2605l_read_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t *data);
drv2605l_result_t drv2605l_autoconfig(drv2605l_handle_t *device);
drv2605l_result_t drv2605l_calibrate(drv2605l_handle_t *device);
drv2605l_result_t drv2605l_init(drv2605l_handle_t *device, I2C_HandleTypeDef *i2c);
drv2605l_result_t drv2605l_play(drv2605l_handle_t *device, uint8_t num);

#endif /* DRV2605L_H */
