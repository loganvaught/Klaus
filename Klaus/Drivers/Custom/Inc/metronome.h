/*
 * metronome.h
 *
 *  Created on: Jul 10, 2026
 *      Author: Logan Vaught
 */

#ifndef METRONOME_H
#define METRONOME_H

#include "main.h"

#define TIMER_TICKS_PER_MINUTE 600000
#define DEFAULT_TEMPO 60
#define RF_RESYNC_THRESHOLD 10000 // In units of 1/10 microseconds.
#define OFFSET_GLIDE_FACTOR 0.1

typedef struct {
	TIM_HandleTypeDef *tim;
	uint16_t tempo;
	// Create array for future timestamps in which a metronome pulse will be generated
	// Uses 3 to handle both personal mode (RF off) use and slave (RX) mode use
	uint32_t pulse_timestamps[3];
	uint8_t pulse_index_on;
	uint8_t initialized_timestamps; // Flag for checking if the first timetsamps have been generated. Used to generate the 3 future pulse timestamps
	int32_t timestamp_offset_from_master; // How far apart the master is from this device. Positive = master ahead. Negative = slave ahead
	uint8_t initialized_offset; // Flag for establishing a reliable "timestamp connection" with master
	// Pulse callback
	void (*pulse_callback)(uint16_t tempo, uint32_t timestamp);
} metronome_t;

typedef enum {
	METRONOME_FORWARD,
	METRONOME_BACKWARD,
	METRONOME_NONE,
} metronome_direction_t;

void metronome_change_tempo(metronome_t *device, metronome_direction_t direction);
void metronome_init(metronome_t *device, TIM_HandleTypeDef *tim);
void metronome_reset_rf(metronome_t *device);
void metronome_update(metronome_t *device);
void metronome_process_rf(metronome_t *device, uint16_t new_tempo, uint32_t master_timestamp);


#endif /* METRONOME_H */
