/*
 * nrf24l01.c
 *
 *  Created on: Jun 14, 2026
 *      Author: Logan Vaught
 */

#include "nrf24l01.h"

/**
  * @brief  Pull the CSN pin low on the given nrf24l01_handle. For SPI operation
  * @param  device The nrf24l01_handle to operate on
  * @retval None
  */
static void nrf24l01_csn_low(nrf24l01_handle_t *device) {
	HAL_GPIO_WritePin(device->CS_Port, device->CS_Pin, GPIO_PIN_RESET);
}
/**
  * @brief  Pull the CSN pin high on the given nrf24l01_handle. For SPI operation
  * @param  device The nrf24l01_handle to operate on
  * @retval None
  */
static void nrf24l01_csn_high(nrf24l01_handle_t *device) {
	HAL_GPIO_WritePin(device->CS_Port, device->CS_Pin, GPIO_PIN_SET);
}
/**
  * @brief  Pull the CE pin low on the given nrf24l01_handle. For TX and RX operation
  * @param  device The nrf24l01_handle to operate on
  * @retval None
  */
static void nrf24l01_ce_low(nrf24l01_handle_t *device) {
	HAL_GPIO_WritePin(device->CE_Port, device->CE_Pin, GPIO_PIN_RESET);
}
/**
  * @brief  Pull the CE pin high on the given nrf24l01_handle. For TX and RX operation
  * @param  device The nrf24l01_handle to operate on
  * @retval None
  */
static void nrf24l01_ce_high(nrf24l01_handle_t *device) {
	HAL_GPIO_WritePin(device->CE_Port, device->CE_Pin, GPIO_PIN_SET);
}

/**
  * @brief  Write a data buffer into a register(s) on the given NRF24L01
  * @param  device The nrf24l01_handle to operate on
  * @param 	addr The starting register address to write to
  * @param	data The data buffer to write from
  * @param	size The number of bytes to write
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_write(nrf24l01_handle_t *device, uint8_t addr, uint8_t *data, uint16_t size) {
	uint8_t command = NRF24L01_W_REGISTER | (addr & NRF24L01_W_MASK);
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	if (HAL_SPI_Transmit(device->spi, data, size, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief  Read to a data buffer from a register(s) on the given NRF24L01
  * @param  device The nrf24l01_handle to operate on
  * @param 	addr The starting register address to read from
  * @param	data The data buffer to read to
  * @param	size The number of bytes to read
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_read(nrf24l01_handle_t *device, uint8_t addr, uint8_t *data, uint16_t size) {
	uint8_t command = NRF24L01_R_REGISTER | (addr & NRF24L01_R_MASK);
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	if (HAL_SPI_Receive(device->spi, data, size, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief  Write to the TX FIFO on the given NRF24L01
  * @param  device The nrf24l01_handle to operate on
  * @param	data The data buffer to put into the FIFO
  * @param	size The number of bytes of data
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_w_tx_payload(nrf24l01_handle_t *device, uint8_t *data, uint16_t size) {
	if (size > 32 || size == 0) return NRF24L01_TooBig;

	uint8_t command = NRF24L01_W_TX_PAYLOAD;
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	if (HAL_SPI_Transmit(device->spi, data, size, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief  Read from the RX FIFO on the given NRF24L01
  * @param  device The nrf24l01_handle to operate on
  * @param	data The data buffer to read to from the FIFO
  * @param	size The number of bytes of data
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_r_rx_payload(nrf24l01_handle_t *device, uint8_t *data, uint16_t size) {
	if (size > 32 || size == 0) return NRF24L01_TooBig;

	uint8_t command = NRF24L01_R_RX_PAYLOAD;
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	if (HAL_SPI_Receive(device->spi, data, size, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief  Use the FLUSH_TX command to remove all data from the TX FIFO
  * @param  device The nrf24l01_handle to operate on
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_flush_tx(nrf24l01_handle_t *device) {
	uint8_t command = NRF24L01_FLUSH_TX;
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief  Use the FLUSH_RX command to remove all data from the RX FIFO
  * @param  device The nrf24l01_handle to operate on
  * @retval The nrf24l01_result indicating success or error
  */
static nrf24l01_result_t nrf24l01_flush_rx(nrf24l01_handle_t *device) {
	uint8_t command = NRF24L01_FLUSH_RX;
	uint8_t status; // Status register shifted back when sending command

	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	device->last_status = status;

	return NRF24L01_OK;
}
/**
  * @brief 	Configure the NRF24L01 for transmission mode. Required for transmission
  * @param  device The nrf24l01_handle to operate on
  * @retval The nrf24l01_result indicating success or error
  */
nrf24l01_result_t nrf24l01_enter_tx(nrf24l01_handle_t *device) {
	nrf24l01_ce_low(device);

	uint8_t config_reg;
	nrf24l01_result_t result = nrf24l01_read(device, NRF24L01_CONFIG, &config_reg, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set PWR_UP = 1
	config_reg |= NRF24L01_PWR_UP_MASK;

	// Set PRIM_RX = 0
	config_reg &= ~(NRF24L01_PRIM_RX_MASK);
	result = nrf24l01_write(device, NRF24L01_CONFIG, &config_reg, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	wait_us(device->micro_timer, NRF24L01_PWR_UP_WAIT_US); // Wait for pwr_up to settle (Takes 1.5ms)

	// Clear the FIFO
	result = nrf24l01_flush_tx(device);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Pull CE high for continuous transmission on FIFO update
	nrf24l01_ce_high(device);
	device->mode = NRF24L01_TX;

	wait_us(device->micro_timer, NRF24L01_TX_TRANSITION_WAIT_US); // Wait for device to settle in mode

	return NRF24L01_OK;
}
/**
  * @brief 	Transmit data (Up to 32 bytes) into the air. Requires TX mode (a call to nrf24l01_enter_tx)
  * @param  device The nrf24l01_handle to operate on
  * @param	size The number of bytes to transmit
  * @retval The nrf24l01_result indicating success or error
  */
nrf24l01_result_t nrf24l01_transmit(nrf24l01_handle_t *device, uint8_t *data, uint16_t size) {
	if (device == NULL) return NRF24L01_NoDevice;
	if (size > 32 || size == 0) return NRF24L01_TooBig;

	nrf24l01_result_t result;
	if (device->mode != NRF24L01_TX) return NRF24L01_IncorrectMode;

	// Check if TX FIFO is full
	uint8_t reg;
	result = nrf24l01_read(device, NRF24L01_FIFO_STATUS, &reg, 1);
	if (result != NRF24L01_OK) {
		return result;
	}
	if (reg & NRF24L01_TX_FULL_FIFO_STATUS) {
		return NRF24L01_TXFull;
	}

	// Set TX FIFO with payload
	result = nrf24l01_w_tx_payload(device, data, size);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Message should be automatically transmitted because in TX mode, CE is high.

	return NRF24L01_OK;
}
/**
  * @brief 	Configure the NRF24L01 for receiving mode. Required for receiving
  * @param  device The nrf24l01_handle to operate on
  * @retval The nrf24l01_result indicating success or error
  */
nrf24l01_result_t nrf24l01_enter_rx(nrf24l01_handle_t *device) {
	nrf24l01_ce_low(device);

	uint8_t config_reg;
	nrf24l01_result_t result = nrf24l01_read(device, NRF24L01_CONFIG, &config_reg, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set PWR_UP = 1
	config_reg |= NRF24L01_PWR_UP_MASK;

	// Set PRIM_RX = 1
	config_reg |= NRF24L01_PRIM_RX_MASK;
	result = nrf24l01_write(device, NRF24L01_CONFIG, &config_reg, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	wait_us(device->micro_timer, NRF24L01_PWR_UP_WAIT_US); // Wait for pwr_up to settle (Takes 1.5ms)

	// Clear the FIFO
	result = nrf24l01_flush_rx(device);
	if (result != NRF24L01_OK) {
		return result;
	}

	nrf24l01_ce_high(device);
	device->mode = NRF24L01_RX;

	wait_us(device->micro_timer, NRF24L01_RX_TRANSITION_WAIT_US); // Wait for device to settle in mode

	return NRF24L01_OK;
}
/**
  * @brief 	A helper function to be called when any of the STATUS IRQs are set during runtime. Clears
  * 		the IRQs and deals with receiving data in RX mode. If new data is received, passes it through
  * 		the callback in the given nrf24l01_handle (nrf24l01_handle_t.rx_callback())
  * @param  device The nrf24l01_handle to operate on
  * @retval The nrf24l01_result indicating success or error
  */
nrf24l01_result_t nrf24l01_handle_irqs(nrf24l01_handle_t *device) {
	// Read the status register by sending a NOP command.
	nrf24l01_result_t result;
	uint8_t status;
	uint8_t command = NRF24L01_NOP;
	nrf24l01_csn_low(device);
	if (HAL_SPI_TransmitReceive(device->spi, &command, &status, 1, HAL_MAX_DELAY) != HAL_OK) {
		nrf24l01_csn_high(device);
		return NRF24L01_Error;
	}
	nrf24l01_csn_high(device);

	// Check for interrupt RX_DR (Data Received)
	if (status & NRF24L01_RX_DR) {
		// Receive the data
		uint8_t data[32];
		result = nrf24l01_r_rx_payload(device, data, 32);
		if (result != NRF24L01_OK) {
			return result;
		}

		// use handle callback
		if (device->rx_callback) {
			device->rx_callback(data, 32);
		}
	}
	// TX_DS does not need to be checked for this driver.
	// In future: MAX_RT will need to be checked if auto-ack is enabled. This driver does not use auto-ack

	// Clear interrupts
	uint8_t clear = NRF24L01_RX_DR | NRF24L01_TX_DS | NRF24L01_MAX_RT;
	result = nrf24l01_write(device, NRF24L01_STATUS, &clear, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	return NRF24L01_OK;
}
/**
  * @brief 	Initializes an NRF24L01 to be operated on via a given nrf24l01_handle_t. Automatically sets up
  * 		the given handle, and configures the NRF24L01 over SPI for useage. Does not enter RX or TX mode.
  * 		Does not use Auto-Ack, or Auto-Retransmit. Only uses one pipe (Pipe 0) for RX.
  * @param  device The nrf24l01_handle to operate on. Expected to be uninitialized
  * @param	spi The address to the SPI handle to use for the NRF24L01
  * @param	micro_timer The micro_delay_handle_t to use for microsecond precision delay.
  * @param	CS_Port The GPIO_TypeDef for the CSN pin to be used with the NRF24L01
  * @param 	CS_Pin The number of the CSN pin to be used with the NRF24L01
  * @param	CE_Port The GPIO_TypeDef for the CE pin to be used with the NRF24L01
  * @param	CE_Pin The number of the CSN pin to be used with the NRF24L01
  * @param 	channel The RF channel to be used over air. Must be between 0 and 125 inclusive.
  * @param	data_rate The nrf24l01_datarate_t to be used
  * @param 	power The nrf24l01_power_t to be used
  * @param	pipe_address The pipe address to be used for TX and RX
  * @retval The nrf24l01_result indicating success or error
  */
nrf24l01_result_t nrf24l01_init(nrf24l01_handle_t *device, SPI_HandleTypeDef *spi, micro_delay_handle_t *micro_timer, GPIO_TypeDef *CS_Port, uint16_t CS_Pin, GPIO_TypeDef *CE_Port, uint16_t CE_Pin, uint8_t channel, nrf24l01_datarate_t data_rate, nrf24l01_power_t power, uint8_t *pipe_address) {
	if (device == NULL) return NRF24L01_NoDevice;
	if (micro_timer == NULL) return NRF24L01_NoDevice;
	if (channel > 125) return NRF24L01_InvalidChannel;
	// Setup handle
	device->spi = spi;
	device->CS_Port = CS_Port;
	device->CS_Pin = CS_Pin;
	device->CE_Port = CE_Port;
	device->CE_Pin = CE_Pin;
	device->micro_timer = micro_timer;
	device->mode = NRF24L01_Unknown;
	device->rx_callback = NULL;

	// Configure registers for basic usage (Disable auto ack, no retransmits, pipes, addresses)
	nrf24l01_result_t result;

	// Disable auto-ack
	uint8_t data = NRF24L01_AUTO_ACK_DISABLE; // Disable auto-ack on all pipes
	result = nrf24l01_write(device, NRF24L01_EN_AA, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Enable RX address pipes (EN_RXADDR)
	// This driver only uses pipe 0
	data = NRF24L01_EN_RX_ADDR_VALUE;
	result = nrf24l01_write(device, NRF24L01_EN_RXADDR, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set address width (SETUP_AW)
	data = NRF24L01_ADDRESS_WIDTH_VALUE; // Set width of address
	result = nrf24l01_write(device, NRF24L01_SETUP_AW, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Disable automatic retransmit (SETUP_RETR)
	data = NRF24L01_AUTO_RETR_DISABLE; // Disable automatic retransmits
	result = nrf24l01_write(device, NRF24L01_SETUP_RETR, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set the channel (RF_CH)
	data = channel & NRF24L01_CHANNEL_MASK; // Set RF channel (0-125)
	result = nrf24l01_write(device, NRF24L01_RF_CH, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// RF Setup (RF_SETUP)
	// Setup data rate
	result = nrf24l01_read(device, NRF24L01_RF_SETUP, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}
	data &= ~(NRF24L01_RF_DR_MASK);
	data |= data_rate;
	// Setup power
	data &= ~(NRF24L01_RF_PWR_MASK);
	data |= power;
	result = nrf24l01_write(device, NRF24L01_RF_SETUP, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set addresses for RX pipes (RX_ADDR_P0-5)
	// This driver currently only uses pipe 0
	result = nrf24l01_write(device, NRF24L01_RX_ADDR_P0, pipe_address, 5);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set address for TX (TX_ADDR).
	result = nrf24l01_write(device, NRF24L01_TX_ADDR, pipe_address, 5);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Set RX payload widths (RX_PW_P0-5)
	// This driver currently uses only width 32 bytes, on pipe 0
	data = NRF24L01_PAYLOAD_WIDTH;
	result = nrf24l01_write(device, NRF24L01_RX_PW_P0, &data, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Housekeeping.
	// Clear interrupts
	uint8_t clear = NRF24L01_RX_DR | NRF24L01_TX_DS | NRF24L01_MAX_RT;
	result = nrf24l01_write(device, NRF24L01_STATUS, &clear, 1);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Flush FIFOs
	result = nrf24l01_flush_tx(device);
	if (result != NRF24L01_OK) {
		return result;
	}
	result = nrf24l01_flush_rx(device);
	if (result != NRF24L01_OK) {
		return result;
	}

	// Configuration is now in NRF24L01_Unknown, and power / prim_rx are automatically set in enter_rx and enter_tx functions

	return NRF24L01_OK;
}

