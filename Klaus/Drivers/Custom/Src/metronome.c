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

	// Make sure tempo in desired range to avoid glitches / too slow / too fast
	// Accelerate the amount of "tempo" changed depending on current tempo
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

static void metronome_pulse(metronome_t *device) {
	if (device->pulse_callback) {
		device->pulse_callback(device->tempo, device->tim->Instance->CNT);
	}

	device->pulse_timestamps[device->pulse_index_on] = device->pulse_timestamps[(device->pulse_index_on + 2)%3] + TIMER_TICKS_PER_MINUTE/device->tempo;
	device->pulse_index_on = (device->pulse_index_on + 1)%3;
}

/**
  * @brief  Initialize future pulse timestamps if necessary and generates a pulse callback
  * 		when a scheduled pulse is reached,
  * @param  device Pointer to the metronome handle
  * @retval Nothing
  */
void metronome_update(metronome_t *device) {
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

	// Calculate difference in timestamp between this device and master
	uint32_t this_timestamp = device->tim->Instance->CNT;
	int32_t difference = (int32_t) (master_timestamp - this_timestamp);

	// Check if just entered RX for first time, or if TX is reconnected, or there is a new TX device
	if (device->initialized_offset == 0 || (difference - device->timestamp_offset_from_master) > RF_RESYNC_THRESHOLD || (difference - device->timestamp_offset_from_master) < -RF_RESYNC_THRESHOLD) {
		// Force recalculation of future pulses and snap to master for smoother operation
		device->initialized_offset = 1;
		device->initialized_timestamps = 0;
		device->tim->Instance->CNT = master_timestamp;
	}
	else {
		// Gradually "glide" the offset as one gets ahead of the other, to make up for gradual clock drifting
		device->timestamp_offset_from_master += (difference - device->timestamp_offset_from_master) * OFFSET_GLIDE_FACTOR;

		// Adjust future pulse timestamps
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
	device->initialized_timestamps = 0;
	device->timestamp_offset_from_master = 0;
	device->initialized_offset = 0;
	device->pulse_callback = NULL; // To be set by main.c
}
