/*
 * drv2605l.c
 *
 *  Created on: Jun 10, 2026
 *      Author: Logan Vaught
 */

#include "drv2605l.h"

static I2C_HandleTypeDef *haptic_i2c = NULL;

/**
  * @brief  Writes a byte of data to a specified memory address on the DRV2605L
  * @param  mAddr the memory address to write to
  * @param  data the byte of data to write
  * @retval 1 = Success, 0 = Failure
  */
uint8_t haptic_write_byte(uint8_t mAddr, uint8_t data) {
	if (haptic_i2c == NULL) return 0;
	if (HAL_I2C_Mem_Write(haptic_i2c, HAPTIC_DEV_ADDR << 1, mAddr, 1, &data, 1, HAL_MAX_DELAY) != HAL_OK) {
		return 0;
	}
	return 1;
}

/**
  * @brief  Reads a byte of data from a specified memory address on the DRV2605L
  * @param  mAddr the memory address to read from
  * @param 	data a pointer to the buffer to store read data
  * @retval 1 = Success, 0 = Failure
  */
uint8_t haptic_read_byte(uint8_t mAddr, uint8_t *data) {
	if (haptic_i2c == NULL) return 0;
	if (HAL_I2C_Mem_Read(haptic_i2c, HAPTIC_DEV_ADDR << 1, mAddr, 1, data, 1, HAL_MAX_DELAY) != HAL_OK) {
		return 0;
	}
	return 1;
}

/**
  * @brief  Set up DRV2605L registers for ELV1411A auto-calibration
  * @retval None
  */
void haptic_autoconfig() {
	// N_ERM_LRA Set 7th bit for LRA
	uint8_t data = 0;
	haptic_read_byte(FEEDBACK_CONTROL, &data);
	data = data | (1 << 7);
	haptic_write_byte(FEEDBACK_CONTROL, data);

	// FB_BRAKE_FACTOR[2:0] - 0x1A[6:4].
	// LOOP_GAIN[1:0] - 0x1A[3:2]
	// RATED_VOLTAGE[7:0] - 0x16[7:0]. SET TO 0x56 FOR 2.0VRms (See formula on DRV datasheet)
	haptic_write_byte(RATED_VOLTAGE, RATED_VOLTAGE_VALUE);

	// OD_CLAMP[7:0] - 0x17[7:0]. SET TO 0x84 FOR 2.8V clamp. (See formula on DRV datasheet)
	haptic_write_byte(OD_CLAMP, OD_CLAMP_VALUE);

	// AUTO_CAL_TIME[1:0] - 0x1E[5:4]
	// DRIVE_TIME[4:0] - 0x1B[4:0]. SET TO 0x1C FOR 150Hz
	haptic_read_byte(CONTROL1, &data);
	data = data & ~(0b11111); // Remove bits 4:0
	data = data | DRIVE_TIME_VALUE;
	haptic_write_byte(CONTROL1, data);

	// SAMPLE_TIME[1:0] - 0x1C[5:4]
	// BLANKING_TIME[3:2] - 0x1F[3:2]
	// BLANKING_TIME[1:0] - 0x1C[3:2]
	// IDISS_TIME[3:2] - 0x1F[1:0]
	// IDISS_TIME[1:0] - 0x1C[1:0]
	// ZC_DET_TIME[1:0] - 0x1E[7:6]
}

/**
  * @brief  Begin DRV2605L auto-calibration. Requires a call to haptic_autoconfig() to set required registers
  * 		are set.
  * @retval 1 = Success, 0 = Failure
  */
uint8_t haptic_calibrate() {
	// Set configuration mode (Bits 2:0 set to 0x7), remove from standby (Bit 6 set to 0)
	uint8_t data = 0;
	haptic_read_byte(MODE, &data);
	data = data & ~(0b111); // Remove bits 2:0 (Mode field)
	data = data | AUTO_CALIBRATION_MODE; // Set for auto calibration mode
	data = data & ~(1 << 6); // Remove from standby
	haptic_write_byte(MODE, data);

	// Set values for my specific LRA: ELV1411A
	haptic_autoconfig();

	// Begin Auto-Calibration. Set GO bit
	haptic_read_byte(GO, &data);
	haptic_write_byte(GO, data | (1 << 0));

	// Poll the go-bit to detect when calibration is completed
	uint8_t finished = 0;
	uint8_t checks = 0;
	while (finished == 0 && checks < MAX_WAIT_CALIBRATE_CYCLES){
		HAL_Delay(WAIT_TIME_CALIBRATE_MS);
		haptic_read_byte(GO, &data);
		if ((data & (1 << 0)) == 0) finished = 1;
		checks ++;
	}

	if (checks >= MAX_WAIT_CALIBRATE_CYCLES) return 0;

	// Check whether or not the DIAG_RESULT bit (Bit 3) shows successful completion
	haptic_read_byte(STATUS, &data);
	uint8_t result = (data & (1 << 3));
	if (result) return 0; // DIAG_RESULT = 1 == Failure

	return 1;
}

/**
  * @brief  Begin the initialization process: Waits for the DRV2605L to be ready, begins auto-calibration, selects
  * 		waveform library no. 6 (Linear Resonant Actuators), and sets up vibrations.
  * @param hi2c the hi2c handler to use for the DRV2605L.
  * @retval None
  */
void haptic_init(I2C_HandleTypeDef *hi2c) {
	haptic_i2c = hi2c;

	// Wait for DRV2065L to be ready
	HAL_Delay(WAIT_TIME_READY_MS);

	// Calibrate
	haptic_calibrate();

	// Remove calibration mode (set bits 2:0 to 0 for internal trigger mode).
	uint8_t data = 0;
	haptic_read_byte(MODE, &data);
	data = data & ~(0b111);
	haptic_write_byte(MODE, data | INTERNAL_TRIGGER_MODE);

	// Library selection. LRA = Library No. 6
	haptic_read_byte(LIBRARY_SELECTION, &data);
	data = data & ~(0b111); // Clear library field (bits 2:0)
	data = data | LIBRARY_SELECTION_VALUE;
	haptic_write_byte(LIBRARY_SELECTION, data);

	// Set up for vibration
	haptic_read_byte(MODE, &data);
	data = data & ~(0b111); // Clear MODE field
	data = data | 0; // Select "Internal Trigger"
	haptic_write_byte(MODE, data);
}

/**
  * @brief  Play a waveform from the selected library (No. 6 for Linear Resonant Actuators). Does not wait for
  * 		or verify completion of played waveform.
  * @param num The waveform to play. Must be between WAVEFORM_SELECT_MIN and
  * 		WAVEFORM_SELECT_MAX for library 6 on the DRV2605L.
  * @retval 1 = Success, 0 = Failure
  */
uint8_t haptic_play(uint8_t num) {
	if (num < WAVEFORM_SELECT_MIN || num > WAVEFORM_SELECT_MAX) return 0; // Invalid waveform

	// Check if go bit is 0
	uint8_t data = 0;
	haptic_read_byte(GO, &data);
	if (data & (1 << 0)) return 0; // Was busy

	haptic_write_byte(WAVEFORM_SEQUENCER_1, num);

	// Set GO bit
	haptic_write_byte(GO, data | 1 << 0);

	return 1;
}
