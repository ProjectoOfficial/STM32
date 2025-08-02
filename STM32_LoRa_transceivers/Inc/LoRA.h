/*
 * LoRA.h
 *
 *  Created on: Jul 20, 2025
 *      Author: daniel
 */

#ifndef INC_LORA_H_
#define INC_LORA_H_

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

struct rcv_data{
	int address;
	int length;
	char payload[240]; // Max payload length
	int rssi; // Received Signal Strength Indicator
	int snr;  // Signal-to-Noise Ratio
};

// Base communication function
int send_at_command(UART_HandleTypeDef *huart, const char *command);

char * get_UID(UART_HandleTypeDef *huart);
char * get_version(UART_HandleTypeDef *huart);

// Specialized functions
int test_connection(UART_HandleTypeDef *huart);
int factory_reset(UART_HandleTypeDef *huart);
int set_transmission_freq(UART_HandleTypeDef *huart, uint32_t frequency);
int set_network_id(UART_HandleTypeDef *huart, uint8_t network_id);
int set_address(UART_HandleTypeDef *huart, uint8_t address);
int set_power(UART_HandleTypeDef *huart, uint8_t power);
int set_mode(UART_HandleTypeDef *huart, uint8_t mode); // 0,1,2


int send_data(UART_HandleTypeDef *huart, uint8_t address, const char *data);
struct rcv_data receive_data(UART_HandleTypeDef *huart);

#endif /* INC_LORA_H_ */
