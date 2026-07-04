/*
 * nrf24l01.h
 *
 *  Created on: Jun 14, 2026
 *      Author: Logan Vaught
 */

#ifndef NRF24L01_H
#define NRF24L01_H

#include "main.h"
#include "micro_delay.h"
#include <stdio.h>

// Register Addresses

#define NRF24L01_CONFIG 0x00
#define NRF24L01_STATUS 0x07
#define NRF24L01_EN_AA 0x01
#define NRF24L01_EN_RXADDR 0x02
#define NRF24L01_FIFO_STATUS 0x17
#define NRF24L01_SETUP_AW 0x03
#define NRF24L01_SETUP_RETR 0x04
#define NRF24L01_RF_CH 0x05
#define NRF24L01_RF_SETUP 0x06
#define NRF24L01_RX_ADDR_P0 0x0A
#define NRF24L01_TX_ADDR 0x10
#define NRF24L01_RX_PW_P0 0x11

// Field Masks
#define NRF24L01_PWR_UP_MASK (1 << 1)
#define NRF24L01_PRIM_RX_MASK (1 << 0)
#define NRF24L01_RX_DR (1 << 6)
#define NRF24L01_TX_DS (1 << 5)
#define NRF24L01_MAX_RT (1 << 4)
#define NRF24L01_TX_FULL_STATUS (1 << 0)
#define NRF24L01_TX_FULL_FIFO_STATUS (1 << 5)
#define NRF24L01_CHANNEL_MASK 0x7F
#define NRF24L01_RF_PWR_MASK (0b11 << 1)
#define NRF24L01_RF_DR_MASK ((1 << 5) | (1 << 3))

// Register values
#define NRF24L01_ADDRESS_WIDTH_VALUE 0x03 // 5 byte addresses
#define NRF24L01_AUTO_ACK_DISABLE 0x00 // Disable auto ack
#define NRF24L01_AUTO_RETR_DISABLE 0x00 // Disable auto retransmit
#define NRF24L01_PAYLOAD_WIDTH 6
#define NRF24L01_EN_RX_ADDR_VALUE 0x01 // Only use pipe 0

// Commands
#define NRF24L01_R_REGISTER 0b00000000 // For the read register command
#define NRF24L01_W_REGISTER 0b00100000 // For the write register command
#define NRF24L01_W_TX_PAYLOAD 0b10100000
#define NRF24L01_R_RX_PAYLOAD 0b01100001
#define NRF24L01_FLUSH_TX 0b11100001
#define NRF24L01_FLUSH_RX 0b11100010
#define NRF24L01_NOP 0b11111111

// Command Masks
#define NRF24L01_R_MASK 0x1F // For the address bits in the 5 least-significant bits
#define NRF24L01_W_MASK 0x1F // For the address bits in the 5 least-significant bits

// Constants for functionality
#define NRF24L01_PWR_UP_WAIT_US 2000 // It takes 1500 microseconds for the PWR_UP pin to settle
#define NRF24L01_CE_PIN_WAIT_US 15 // CE must be pulsed high for at least 10 microseconds to register as high
#define NRF24L01_TRANSMIT_MAX_WAIT_CYCLES 100 // How many wait cycles to poll the TX_DS for successful transmission
#define NRF24L01_TRANSMIT_WAIT_US 10 // How many microseconds to wait per cycle
#define NRF24L01_RX_TRANSITION_WAIT_US 150
#define NRF24L01_TX_TRANSITION_WAIT_US 150

// Handlers
typedef enum {
	NRF24L01_OK,
	NRF24L01_Error,
	NRF24L01_NoDevice,
	NRF24L01_TooBig,
	NRF24L01_Timeout,
	NRF24L01_IncorrectMode,
	NRF24L01_TXFull,
	NRF24L01_NoCallback,
	NRF24L01_InvalidChannel,
} nrf24l01_result_t;

typedef enum {
	NRF24L01_TX,
	NRF24L01_RX,
	NRF24L01_Off,
} nrf24l01_mode_t;

typedef struct {
	SPI_HandleTypeDef *spi;
	GPIO_TypeDef *CS_Port;
	uint16_t CS_Pin;
	GPIO_TypeDef *CE_Port;
	uint16_t CE_Pin;
	uint8_t last_status;
	nrf24l01_mode_t mode;
	micro_delay_handle_t *micro_timer;
	void (*rx_callback)(uint8_t *data, uint16_t length); // Get received data in RX mode
	void (*tx_callback)(); // May be useful for debugging successful transmits during TX mode
} nrf24l01_handle_t;

typedef enum {
    NRF24L01_DR_1MBPS  = 0b00000000,
    NRF24L01_DR_2MBPS  = 0b00001000,
	NRF24L01_DR_250KBPS = 0b00100000,
} nrf24l01_datarate_t;

typedef enum {
    NRF24L01_PWR_NEG18DBM = 0b00000000,
    NRF24L01_PWR_NEG12DBM = 0b00000010,
    NRF24L01_PWR_NEG6DBM  = 0b00000100,
    NRF24L01_PWR_0DBM     = 0b00000110,
} nrf24l01_power_t;

// User Functions
nrf24l01_result_t nrf24l01_enter_rx(nrf24l01_handle_t *device);
nrf24l01_result_t nrf24l01_enter_tx(nrf24l01_handle_t *device);
nrf24l01_result_t nrf24l01_transmit(nrf24l01_handle_t *device, uint8_t *data, uint16_t size);
nrf24l01_result_t nrf24l01_handle_irqs(nrf24l01_handle_t *device);
nrf24l01_result_t nrf24l01_enter_off(nrf24l01_handle_t *device);
nrf24l01_result_t nrf24l01_init(nrf24l01_handle_t *device, SPI_HandleTypeDef *spi, micro_delay_handle_t *micro_timer, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, GPIO_TypeDef *CE_Port, uint16_t CE_Pin, uint8_t channel, nrf24l01_datarate_t data_rate, nrf24l01_power_t power, uint8_t *pipe_address);
void nrf24l01_print_status(nrf24l01_handle_t *device);
void nrf24l01_print_config(nrf24l01_handle_t *device);
void nrf24l01_debug_print_all_important_regs(nrf24l01_handle_t *device);

#endif /* NRF24L01_H */
