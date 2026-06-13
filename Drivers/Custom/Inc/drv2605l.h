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
#define MODE 0x01
#define FEEDBACK_CONTROL 0x1A
#define RATED_VOLTAGE 0x16
#define OD_CLAMP 0x17
#define CONTROL1 0x1B
#define GO 0x0C
#define STATUS 0x00
#define LIBRARY_SELECTION 0x03
#define WAVEFORM_SEQUENCER_1 0x04

// Constants for calibration of ELV1411A
#define RATED_VOLTAGE_VALUE 0x56
#define OD_CLAMP_VALUE 0x84
#define DRIVE_TIME_VALUE 0x1C
#define AUTO_CALIBRATION_MODE 0x07
#define WAIT_TIME_CALIBRATE_MS 100
#define MAX_WAIT_CALIBRATE_CYCLES 10

// Constants for initialization of ELV1411A
#define WAIT_TIME_READY_MS 500
#define LIBRARY_SELECTION_VALUE 0x06
#define INTERNAL_TRIGGER_MODE 0

// Constants for metronome
#define WAVEFORM_SELECT_MIN 1
#define WAVEFORM_SELECT_MAX 123
#define TEMPO_MAX 208
#define TEMPO_MIN 30

// Functions
drv2605l_result_t drv2605l_write_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t data);
drv2605l_result_t drv2605l_read_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t *data);
drv2605l_result_t drv2605l_autoconfig(drv2605l_handle_t *device);
drv2605l_result_t drv2605l_calibrate(drv2605l_handle_t *device);
drv2605l_result_t drv2605l_init(drv2605l_handle_t *device, I2C_HandleTypeDef *i2c);
drv2605l_result_t drv2605l_play(drv2605l_handle_t *device, uint8_t num);




#endif /* DRV2605L_H */
