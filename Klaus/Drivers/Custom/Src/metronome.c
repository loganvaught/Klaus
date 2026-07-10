/*
 * metronome.c
 *
 *  Created on: Jul 10, 2026
 *      Author: Logan Vaught
 */

#include "metronome.h"

/**
  * @brief  Changes the metronome tempo based on the specified direction
  * @param  device Pointer to the metronome handle
  * @param	direction The metronome_direction_t to move in
  * @retval Nothing
  */
void metronome_change_tempo(metronome_t *device, metronome_direction_t direction) {
	int8_t change = 0;
	switch (direction) {
	case METRONOME_FORWARD:
		change = 1;
		break;
	case METRONOME_BACKWARD:
		change = -1;
		break;
	default:
		// Invalid direction. Do nothing
		return;
	}

	uint16_t new_tempo = device->tempo + change;

	// Make sure tempo in range
	// And, multiply change to make twisting knob faster for higher tempos

	if (new_tempo >= 30 && new_tempo < 60) {
		new_tempo += 1 * change;
		device->tempo = new_tempo;
	}
	else if (new_tempo >= 60 && new_tempo < 72) {
		new_tempo += 2 * change;
		device->tempo = new_tempo;
	}
	else if (new_tempo >= 72 && new_tempo < 120) {
		new_tempo += 3 * change;
		device->tempo = new_tempo;
	}
	else if (new_tempo >= 120 && new_tempo < 144) {
		new_tempo += 5 * change;
		device->tempo = new_tempo;
	}
	else if (new_tempo >= 144 && new_tempo < 208) {
		new_tempo += 7 * change;
		device->tempo = new_tempo;
	}

  	device->initialized_timestamps = 0;
}

/**
  * @brief  Resets the RF traits the metronome depends on. Forces re-alignment with a master device, if one is present
  * @param  device Pointer to the metronome handle
  * @retval Nothing
  */
void metronome_reset_rf(metronome_t *device) {
	device->timestamp_offset_from_master = 0;
	device->initialized_timestamps = 0;
	device->initialized_offset = 0;
}

// Call the pulse callback, and schedule the next pulse
static void metronome_pulse(metronome_t *device) {
	if (device->pulse_callback) {
		device->pulse_callback(device->tempo, device->tim->Instance->CNT);
	}
	// Create new pulse timestamp
	device->pulse_timestamps[device->pulse_index_on] = device->pulse_timestamps[(device->pulse_index_on + 2)%3] + TIMER_TICKS_PER_MINUTE/device->tempo;
	// Increment index for pulse buffer
	device->pulse_index_on = (device->pulse_index_on + 1)%3;
}

/**
  * @brief  Initialize future pulse timestamps if necessary and generates a pulse callback
  * 		when a scheduled pulse is reached,
  * @param  device Pointer to the metronome handle
  * @retval Nothing
  */
void metronome_update(metronome_t *device) {
	// Initialize timestamps if not initialized
	if (device->initialized_timestamps == 0) {
		device->initialized_timestamps = 1;
		uint32_t this_timestamp = device->tim->Instance->CNT;
		uint16_t tempo = device->tempo;
		uint32_t offset = device->timestamp_offset_from_master;
		device->pulse_timestamps[0] = this_timestamp + TIMER_TICKS_PER_MINUTE/tempo - offset;
		device->pulse_timestamps[1] = this_timestamp + 2 * (TIMER_TICKS_PER_MINUTE/tempo) - offset;
		device->pulse_timestamps[2] = this_timestamp + 3 * (TIMER_TICKS_PER_MINUTE/tempo) - offset;
		device->pulse_index_on = 0;
	}

	// Check if new pulse is due
	if (device->tim->Instance->CNT >= device->pulse_timestamps[device->pulse_index_on]) {
		metronome_pulse(device);
	}
}

/**
  * @brief  Synchronize the metronome with a master, if one is present
  * @param  device Pointer to the metronome handle
  * @param  new_tempo The tempo the master has set
  * @param  master_timestamp The timestamp of the master
  * @retval Nothing
  */
void metronome_process_rf(metronome_t *device, uint16_t new_tempo, uint32_t master_timestamp) {
	device->tempo = new_tempo;

	// Calculate difference in timestamp
	uint32_t this_timestamp = device->tim->Instance->CNT;
	int32_t difference = (int32_t) (master_timestamp - this_timestamp);

	// Check if just entered RX for first time, or if TX is reconnected / there is a new TX
	if (device->initialized_offset == 0 || (difference - device->timestamp_offset_from_master) > RF_RESYNC_THRESHOLD || (difference - device->timestamp_offset_from_master) < -RF_RESYNC_THRESHOLD) {
		// Forces recalculation of future pulses
		device->initialized_offset = 1;
		// Flag the reinitialization of the next 3 pulses.
		device->initialized_timestamps = 0;
		// Snap the timestamp
		device->tim->Instance->CNT = master_timestamp;
	}
	else {
		// Gradually "glide" the offset as one gets ahead of the other, to account for clock drift
		device->timestamp_offset_from_master += (difference - device->timestamp_offset_from_master) * OFFSET_GLIDE_FACTOR; // Smooth out offset over many beats.

		// Adjust future pulse timestamp calculations
		device->pulse_timestamps[(device->pulse_index_on + 1)%3] = this_timestamp + TIMER_TICKS_PER_MINUTE/new_tempo - device->timestamp_offset_from_master;
		device->pulse_timestamps[(device->pulse_index_on + 2)%3] = this_timestamp + 2*(TIMER_TICKS_PER_MINUTE/new_tempo) - device->timestamp_offset_from_master;
	}
}

/**
  * @brief  Initialize a metronome_t handle with default values to begin generating pulses.
  * @param  device Pointer to the metronome handle
  * @param  tim The address of the htim to use for generating pulses. Should run at 10 MHz, or adjust TIMER_TICKS_PER_MINUTE
  * @retval Nothing
  */
void metronome_init(metronome_t *device, TIM_HandleTypeDef *tim) {
	device->tim = tim;
	device->tempo = DEFAULT_TEMPO;
	device->initialized_timestamps = 0; // Flag generation of pulse timestamps
	device->timestamp_offset_from_master = 0;
	device->initialized_offset = 0; // Flag for establishing a reliable "timestamp connection" with master when RF is turned on
	device->pulse_callback = NULL; // To be set by main.c
}
