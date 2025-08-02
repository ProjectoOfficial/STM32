/*
 * LoRA.c
 *
 *  Created on: Jul 20, 2025
 *      Author: daniel
 */

#include "LoRA.h"

// Base communication function that handles AT command sending and response checking
int send_at_command(UART_HandleTypeDef *huart, const char *command) {
    char rx_buffer[100] = {0};
    HAL_StatusTypeDef status;
    
    // Send AT command
    status = HAL_UART_Transmit(huart, (uint8_t*)command, strlen(command), 1000);
    if (status != HAL_OK) {
        return -1;  // Transmission failed
    }
    
    // Wait and receive response
    status = HAL_UART_Receive(huart, (uint8_t*)rx_buffer, sizeof(rx_buffer)-1, 1000);
    if (status != HAL_OK) {
        return -1;  // Reception failed or timeout
    }
    
    // Check for OK response
    if (strstr(rx_buffer, "OK") != NULL) {
        return 1;   // Success
    }
    
    // Check for error responses
    if (strstr(rx_buffer, "+ERR=") != NULL) {
        // Extract error code
        char *err_ptr = strstr(rx_buffer, "+ERR=");
        int error_code = atoi(err_ptr + 5);
        return -error_code;  // Return negative error code
    }
    
    return 0;  // Unknown response
}

int factory_reset(UART_HandleTypeDef *huart) {
	return send_at_command(huart, "AT+FACTORY\r\n");
}

int test_connection(UART_HandleTypeDef *huart) {
    return send_at_command(huart, "AT\r\n");
}

char * get_UID(UART_HandleTypeDef *huart) {
    static char buffer[128] = {0};
    char at_command[] = "AT+UID?\r\n";

    if (HAL_UART_Transmit(huart, (uint8_t*)at_command, strlen(at_command), 1000) != HAL_OK) {
        return NULL;
    }

    // Ricevi carattere per carattere fino a '\n' o max buffer
    uint16_t i = 0;
    uint8_t ch;
    while (i < sizeof(buffer) - 1) {
        if (HAL_UART_Receive(huart, &ch, 1, 100) != HAL_OK) break;
        buffer[i++] = ch;
        if (ch == '\n') break;
    }
    buffer[i] = '\0';

    // Cerca "+UID="
    char *ptr = strstr(buffer, "+UID=");
    if (ptr) {
        // Sposta la stringa per ritornare solo il valore
        return ptr + 5;  // ritorna dopo "+UID="
    }
    return NULL;
}

char * get_version(UART_HandleTypeDef *huart) {
    static char buffer[128] = {0};
    char at_command[] = "AT+VER?\r\n";

    if (HAL_UART_Transmit(huart, (uint8_t*)at_command, strlen(at_command), 1000) != HAL_OK) {
        return NULL;
    }

    uint16_t i = 0;
    uint8_t ch;
    while (i < sizeof(buffer) - 1) {
        if (HAL_UART_Receive(huart, &ch, 1, 100) != HAL_OK) break;
        buffer[i++] = ch;
        if (ch == '\n') break;
    }
    buffer[i] = '\0';

    char *ptr = strstr(buffer, "+VER=");
    if (ptr) {
        return ptr + 5;  // ritorna dopo "+VER="
    }
    return NULL;
}

int set_transmission_freq(UART_HandleTypeDef *huart, uint32_t frequency) {
    char at_command[64] = {0};

    // Format: AT+BAND=<frequency>\r\n
    snprintf(at_command, sizeof(at_command), "AT+BAND=%lu\r\n", frequency);

    return send_at_command(huart, at_command);
}

int set_network_id(UART_HandleTypeDef *huart, uint8_t network_id) {
    char at_command[32] = {0};

    // Format: AT+ID=<network_id>\r\n
    snprintf(at_command, sizeof(at_command), "AT+NETWORKID=%d\r\n", network_id);

    return send_at_command(huart, at_command);
}

int set_address(UART_HandleTypeDef *huart, uint8_t address) {
	char at_command[32] = {0};

	// Format: AT+ADDR=<address>\r\n
	snprintf(at_command, sizeof(at_command), "AT+ADDRESS=%d\r\n", address);

	return send_at_command(huart, at_command);
}

int set_power(UART_HandleTypeDef *huart, uint8_t power) {
	char at_command[32] = {0};

	// Format: AT+POWER=<power>\r\n
	snprintf(at_command, sizeof(at_command), "AT+POWER=%d\r\n", power);

	return send_at_command(huart, at_command);
}

int set_mode(UART_HandleTypeDef *huart, uint8_t mode) {
	char at_command[32] = {0};

	// Format: AT+MODE=<mode>\r\n
	snprintf(at_command, sizeof(at_command), "AT+MODE=%d\r\n", mode);

	return send_at_command(huart, at_command);
}

int send_data(UART_HandleTypeDef *huart, uint8_t address, const char *data) {
	int datalen = strlen(data);

	if (datalen > 240) {
		return -1; // Data too long
	}

	// Format: AT+SEND=<address><payload length><data>\r\n
	char at_command[260] = {0};
	snprintf(at_command, sizeof(at_command), "AT+SEND=%d,%d,%s\r\n", address, datalen, data);
	return send_at_command(huart, at_command);
}


// Simple blocking receive function that actually works
struct rcv_data receive_data(UART_HandleTypeDef *huart) {
    struct rcv_data rd = {0};
    char rx_buffer[256] = {0};
    uint8_t ch;
    int idx = 0;
    
    // Read characters until we get a complete line or timeout
    while (idx < sizeof(rx_buffer) - 1) {
        // Use short timeout per character - if no data available, return quickly
        if (HAL_UART_Receive(huart, &ch, 1, 50) == HAL_OK) {
            rx_buffer[idx++] = ch;
            if (ch == '\n') {
                rx_buffer[idx] = '\0';
                break;
            }
	   } else {
		   rx_buffer[idx] = '\0';
		   break;
	   }
    }
    
    // Check if it's a +RCV message
    if (strstr(rx_buffer, "+RCV") != NULL) {  // CORRETTO: != NULL invece di == 0
        // Parse using sscanf
    	int parsed = sscanf(rx_buffer,
    	       "+RCV=%d,%d,%239[^,],%d,%d",
    	       &rd.address,
    	       &rd.length,
    	       rd.payload,
    	       &rd.rssi,
    	       &rd.snr);
        
        if (parsed == 5) {  // Deve essere esattamente 5 campi
            // Remove any trailing whitespace from payload
            char *end = rd.payload + strlen(rd.payload) - 1;
            while (end > rd.payload && (*end == ' ' || *end == '\r')) {
                *end-- = '\0';
            }
        } else {
            // Reset if parsing failed
            memset(&rd, 0, sizeof(rd));
        }
    } else if (strstr(rx_buffer, "+ERR=") != NULL) {
		//copy error in payload
		strncpy(rd.payload, rx_buffer, sizeof(rd.payload) - 1);
		rd.payload[sizeof(rd.payload) - 1] = '\0'; // Ensure null termination
		rd.address = 0; // No address in error response
		rd.length = sizeof(rd.payload) - 1; // Set length to max payload size
		rd.rssi = 0; // No RSSI in error response
		rd.snr = 0; // No SNR in error response
	} else {
		// Unknown response, reset data
		memset(&rd, 0, sizeof(rd));
	}

    
    return rd;
}
