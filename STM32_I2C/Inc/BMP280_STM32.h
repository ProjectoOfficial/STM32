/*
 ***************************************************************************************************************
 ***************************************************************************************************************
 ***************************************************************************************************************

  File:		  BMP280_STM32.h
  Author:     ControllersTech.com
  Updated:    Dec 14, 2021

 ***************************************************************************************************************
  Copyright (C) 2017 ControllersTech.com

  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.

 ***************************************************************************************************************
 */


#ifndef BMP280_STM32_H_
#define BMP280_STM32_H_

#include "stm32f4xx_hal.h"

/* Configuration for the BMP280

 * @osrs is the oversampling to improve the accuracy
 *       if osrs is set to OSRS_OFF, the respective measurement will be skipped
 *       It can be set to OSRS_1, OSRS_2, OSRS_4, etc. Check the header file
 *
 * @mode can be used to set the mode for the device
 *       MODE_SLEEP will put the device in sleep
 *       MODE_FORCED device goes back to sleep after one measurement. You need to use the BMP280_WakeUP() function before every measurement
 *       MODE_NORMAL device performs measurement in the normal mode. Check datasheet page no 16
 *
 * @t_sb is the standby time. The time sensor waits before performing another measurement
 *       It is used along with the normal mode. Check datasheet page no 16 and page no 30
 *
 * @filter is the IIR filter coefficients
 *         IIR is used to avoid the short term fluctuations
 *         Check datasheet page no 18 and page no 30
 */

// Oversampling definitions

enum OSRS{
	OSRS_OFF =   	0x00,
	OSRS_1 =      	0x01,
	OSRS_2 =     	0x02,
	OSRS_4 =     	0x03,
	OSRS_8 =     	0x04,
	OSRS_16 =     	0x05,
};

enum MODES{
	MODE_SLEEP =      0x00,
	MODE_FORCED =     0x01,
	MODE_NORMAL =     0x03

};

// MODE Definitions

// Standby Time
enum T_SB{
	T_SB_0p5 =    	0x00,
	T_SB_62p5 =   	0x01,
	T_SB_125 =    	0x02,
	T_SB_250 =    	0x03,
	T_SB_500 =    	0x04,
	T_SB_1000 =   	0x05,
	T_SB_10 =    	0x06,
	T_SB_20 =     	0x07,
};

// IIR Filter Coefficients
enum IIR{
	IIR_OFF =     	0x00,
	IIR_2 =       	0x01,
	IIR_4 =      	0x02,
	IIR_8 =      	0x03,
	IIR_16 =      	0x04,
};


// REGISTERS DEFINITIONS
#define ID_REG      	0xD0
#define RESET_REG  		0xE0
#define CTRL_HUM_REG    0xF2
#define STATUS_REG      0xF3
#define CTRL_MEAS_REG   0xF4
#define CONFIG_REG      0xF5
#define PRESS_MSB_REG   0xF7


int BMP280_Config (uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, uint8_t mode, uint8_t t_sb, uint8_t filter);


// Read the Trimming parameters saved in the NVM ROM of the device
void TrimRead(void);

/* To be used when doing the force measurement
 * the Device need to be put in forced mode every time the measurement is needed
 */
void BMP280_WakeUP(void);

/* measure the temp, pressure and humidity
 * the values will be stored in the parameters passed to the function
 */
void BMP280_Measure (void);

#endif /* BMP280_STM32_H_ */
