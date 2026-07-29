/*
 * drv2605l.c
 *
 *  Created on: Jun 10, 2026
 *      Author: Logan Vaught
 */

#include "drv2605l.h"

/**
  * @brief  Writes a byte of data to a specified memory address on the DRV2605L
  * @param device the drv2605l_handle_t variable to operate on
  * @param  mAddr the memory address to write to
  * @param  data the byte of data to write
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_write_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t data) {
	if (HAL_I2C_Mem_Write(device->i2c, HAPTIC_DEV_ADDR << 1, mAddr, 1, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
		return DRV2605L_Error;
	}
	return DRV2605L_OK;
}

/**
  * @brief  Reads a byte of data from a specified memory address on the DRV2605L
  * @param device the drv2605l_handle_t variable to operate on
  * @param  mAddr the memory address to read from
  * @param 	data a pointer to the buffer to store read data
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_read_byte(drv2605l_handle_t *device, uint8_t mAddr, uint8_t *data) {
	if (HAL_I2C_Mem_Read(device->i2c, HAPTIC_DEV_ADDR << 1, mAddr, 1, data, 1, HAL_MAX_DELAY) != HAL_OK) {
		return DRV2605L_Error;
	}
	return DRV2605L_OK;
}

/**
  * @brief  Set up DRV2605L registers for ELV1411A auto-calibration
  * @param device the drv2605l_handle_t variable to operate on
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_autoconfig(drv2605l_handle_t *device) {
	uint8_t data = 0;
	if (drv2605l_read_byte(device, DRV2605L_FEEDBACK_CONTROL, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data | (1 << 7);
	if (drv2605l_write_byte(device, DRV2605L_FEEDBACK_CONTROL, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_write_byte(device, DRV2605L_RATED_VOLTAGE, DRV2605L_RATED_VOLTAGE_VALUE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_write_byte(device, DRV2605L_OD_CLAMP, DRV2605L_OD_CLAMP_VALUE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_read_byte(device, DRV2605L_CONTROL1, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b11111);
	data = data | DRV2605L_DRIVE_TIME_VALUE;
	if (drv2605l_write_byte(device, DRV2605L_CONTROL1, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	return DRV2605L_OK;
}

/**
  * @brief  Begin DRV2605L auto-calibration. Requires a call to drv2605l_autoconfig() to set required registers
  * 		are set.
  * @param device the drv2605l_handle_t variable to operate on
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_calibrate(drv2605l_handle_t *device) {
	uint8_t data = 0;
	if (drv2605l_read_byte(device, DRV2605L_MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111);
	data = data | DRV2605L_AUTO_CALIBRATION_MODE;
	data = data & ~(1 << 6);
	if (drv2605l_write_byte(device, DRV2605L_MODE, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Set values for my specific LRA: ELV1411A
	if (drv2605l_autoconfig(device) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Set the go bit in order to trigger auto-calibration
	if (drv2605l_read_byte(device, DRV2605L_GO, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	if (drv2605l_write_byte(device, DRV2605L_GO, data | (1 << 0)) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Need to poll go bit in order to detect calibration completion
	uint8_t finished = 0;
	uint8_t checks = 0;
	while (finished == 0 && checks < DRV2605L_MAX_WAIT_CALIBRATE_CYCLES){
		HAL_Delay(DRV2605L_WAIT_TIME_CALIBRATE_MS);
		if (drv2605l_read_byte(device, DRV2605L_GO, &data) != DRV2605L_OK) {
			return DRV2605L_Error;
		}
		if ((data & (1 << 0)) == 0) finished = 1;
		checks ++;
	}

	if (checks >= DRV2605L_MAX_WAIT_CALIBRATE_CYCLES) return DRV2605L_Error;

	// Check whether or not the DIAG_RESULT bit (Bit 3) shows successful completion
	if (drv2605l_read_byte(device, DRV2605L_STATUS, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	uint8_t result = (data & (1 << 3));
	if (result) return DRV2605L_Error;

	return DRV2605L_OK;
}

/**
  * @brief  Begin the initialization process: Waits for the DRV2605L to be ready, begins auto-calibration, selects
  * 		waveform library no. 6 (Linear Resonant Actuators), and sets up vibrations.
  * @param device the drv2605l_handle_t variable to operate on
  * @param i2c the address of the I2C_HandleTypeDef to set for the drv2605l
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_init(drv2605l_handle_t *device, I2C_HandleTypeDef *i2c) {
	device->i2c = i2c;

	// DRV2605L requires a certain amount of time to wait for being "ready"
	HAL_Delay(DRV2605L_WAIT_TIME_READY_MS);

	if (drv2605l_calibrate(device) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Must exit calibration mode
	uint8_t data = 0;
	if (drv2605l_read_byte(device, DRV2605L_MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111);
	if (drv2605l_write_byte(device, DRV2605L_MODE, data | DRV2605L_INTERNAL_TRIGGER_MODE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_read_byte(device, DRV2605L_LIBRARY_SELECTION, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111);
	data = data | DRV2605L_LIBRARY_SELECTION_VALUE;
	if (drv2605l_write_byte(device, DRV2605L_LIBRARY_SELECTION, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_read_byte(device, DRV2605L_MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111);
	data = data | 0;
	if (drv2605l_write_byte(device, DRV2605L_MODE, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	return DRV2605L_OK;
}

/**
  * @brief  Play a waveform from the selected library (No. 6 for Linear Resonant Actuators). Does not wait for
  * 		or verify completion of played waveform.
  * @param device the drv2605l_handle_t variable to operate on
  * @param num The waveform to play. Must be between WAVEFORM_SELECT_MIN and
  * 		WAVEFORM_SELECT_MAX for library 6 on the DRV2605L.
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_play(drv2605l_handle_t *device, uint8_t num) {
	if (num < DRV2605L_WAVEFORM_SELECT_MIN || num > DRV2605L_WAVEFORM_SELECT_MAX) return DRV2605L_Error; // Invalid waveform

	// Must check that DRV2605L is not currently busy
	uint8_t data = 0;
	if (drv2605l_read_byte(device, DRV2605L_GO, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	if (data & (1 << 0)) return DRV2605L_Error; // Was busy

	if (drv2605l_write_byte(device, DRV2605L_WAVEFORM_SEQUENCER_1, num) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	if (drv2605l_write_byte(device, DRV2605L_GO, data | 1 << 0) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	return DRV2605L_OK;
}
