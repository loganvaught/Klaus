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
	// N_ERM_LRA Set 7th bit for LRA
	uint8_t data = 0;
	if (drv2605l_read_byte(device, FEEDBACK_CONTROL, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data | (1 << 7);
	if (drv2605l_write_byte(device, FEEDBACK_CONTROL, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// FB_BRAKE_FACTOR[2:0] - 0x1A[6:4].
	// LOOP_GAIN[1:0] - 0x1A[3:2]
	// RATED_VOLTAGE[7:0] - 0x16[7:0]. SET TO 0x56 FOR 2.0VRms (See formula on DRV datasheet)
	if (drv2605l_write_byte(device, RATED_VOLTAGE, RATED_VOLTAGE_VALUE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// OD_CLAMP[7:0] - 0x17[7:0]. SET TO 0x84 FOR 2.8V clamp. (See formula on DRV datasheet)
	if (drv2605l_write_byte(device, OD_CLAMP, OD_CLAMP_VALUE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// AUTO_CAL_TIME[1:0] - 0x1E[5:4]
	// DRIVE_TIME[4:0] - 0x1B[4:0]. SET TO 0x1C FOR 150Hz
	if (drv2605l_read_byte(device, CONTROL1, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b11111); // Remove bits 4:0
	data = data | DRIVE_TIME_VALUE;
	if (drv2605l_write_byte(device, CONTROL1, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// SAMPLE_TIME[1:0] - 0x1C[5:4]
	// BLANKING_TIME[3:2] - 0x1F[3:2]
	// BLANKING_TIME[1:0] - 0x1C[3:2]
	// IDISS_TIME[3:2] - 0x1F[1:0]
	// IDISS_TIME[1:0] - 0x1C[1:0]
	// ZC_DET_TIME[1:0] - 0x1E[7:6]

	return DRV2605L_OK;
}

/**
  * @brief  Begin DRV2605L auto-calibration. Requires a call to drv2605l_autoconfig() to set required registers
  * 		are set.
  * @param device the drv2605l_handle_t variable to operate on
  * @retval Either DRV2605L_OK or DRV2605L_Error
  */
drv2605l_result_t drv2605l_calibrate(drv2605l_handle_t *device) {
	// Set configuration mode (Bits 2:0 set to 0x7), remove from standby (Bit 6 set to 0)
	uint8_t data = 0;
	if (drv2605l_read_byte(device, MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111); // Remove bits 2:0 (Mode field)
	data = data | AUTO_CALIBRATION_MODE; // Set for auto calibration mode
	data = data & ~(1 << 6); // Remove from standby
	if (drv2605l_write_byte(device, MODE, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Set values for my specific LRA: ELV1411A
	if (drv2605l_autoconfig(device) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Begin Auto-Calibration. Set GO bit
	if (drv2605l_read_byte(device, GO, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	if (drv2605l_write_byte(device, GO, data | (1 << 0)) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Poll the go-bit to detect when calibration is completed
	uint8_t finished = 0;
	uint8_t checks = 0;
	while (finished == 0 && checks < MAX_WAIT_CALIBRATE_CYCLES){
		HAL_Delay(WAIT_TIME_CALIBRATE_MS);
		if (drv2605l_read_byte(device, GO, &data) != DRV2605L_OK) {
			return DRV2605L_Error;
		}
		if ((data & (1 << 0)) == 0) finished = 1;
		checks ++;
	}

	if (checks >= MAX_WAIT_CALIBRATE_CYCLES) return DRV2605L_Error;

	// Check whether or not the DIAG_RESULT bit (Bit 3) shows successful completion
	if (drv2605l_read_byte(device, STATUS, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	uint8_t result = (data & (1 << 3));
	if (result) return DRV2605L_Error; // DIAG_RESULT = 1 == Failure

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
	// set the i2c of the drv2605l_handle_t
	device->i2c = i2c;

	// Wait for DRV2605L to be ready
	HAL_Delay(WAIT_TIME_READY_MS);

	// Calibrate
	if (drv2605l_calibrate(device) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Remove calibration mode (set bits 2:0 to 0 for internal trigger mode).
	uint8_t data = 0;
	if (drv2605l_read_byte(device, MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111);
	if (drv2605l_write_byte(device, MODE, data | INTERNAL_TRIGGER_MODE) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Library selection. LRA = Library No. 6
	if (drv2605l_read_byte(device, LIBRARY_SELECTION, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111); // Clear library field (bits 2:0)
	data = data | LIBRARY_SELECTION_VALUE;
	if (drv2605l_write_byte(device, LIBRARY_SELECTION, data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Set up for vibration
	if (drv2605l_read_byte(device, MODE, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	data = data & ~(0b111); // Clear MODE field
	data = data | 0; // Select "Internal Trigger"
	if (drv2605l_write_byte(device, MODE, data) != DRV2605L_OK) {
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
	if (num < WAVEFORM_SELECT_MIN || num > WAVEFORM_SELECT_MAX) return DRV2605L_Error; // Invalid waveform

	// Check if go bit is 0
	uint8_t data = 0;
	if (drv2605l_read_byte(device, GO, &data) != DRV2605L_OK) {
		return DRV2605L_Error;
	}
	if (data & (1 << 0)) return DRV2605L_Error; // Was busy

	if (drv2605l_write_byte(device, WAVEFORM_SEQUENCER_1, num) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	// Set GO bit
	if (drv2605l_write_byte(device, GO, data | 1 << 0) != DRV2605L_OK) {
		return DRV2605L_Error;
	}

	return DRV2605L_OK;
}
