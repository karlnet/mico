/**
******************************************************************************
* @file    DHT11.h 
* @author  Eshen Wang
* @version V1.0.0
* @date    1-May-2015
* @brief   DHT11 operation. 
  operation
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDATAG CUSTOMERS
* WITH CODATAG INFORMATION REGARDATAG THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODATAG INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#ifndef __AM2301_H_
#define __AM2301_H_

#include "common.h"
#include "platform.h"

#ifndef AM2301
#define AM2301                  MICO_GPIO_17
#endif

// Set GPIO Direction
#define AM2301_IO_IN()          MicoGpioInitialize( (mico_gpio_t)AM2301, INPUT_PULL_UP );										 
#define AM2301_IO_OUT()         MicoGpioInitialize( (mico_gpio_t)AM2301, OUTPUT_PUSH_PULL );
	
// Set Data output state
#define AM2301_DATA_Clr()       MicoGpioOutputLow(AM2301) 
#define AM2301_DATA_Set()       MicoGpioOutputHigh(AM2301)

// get DATA input state
#define	AM2301_DATA            MicoGpioInputGet(AM2301)

//-------------------------------- USER INTERFACES -----------------------------

uint8_t AM2301_Init(void); //Init AM2301
uint8_t AM2301_Read_Data(float *temperature,float *humidity); //Read AM2301 Value
//uint8_t AM2301_Read_Byte(void);         //Read One Byte
//uint8_t AM2301_Read_Bit(void);          //Read One Bit
//uint8_t AM2301_Check(void);             //Chack AM2301
//void AM2301_Rst(void);                  //Reset AM2301  


#endif  // __AM2301_H_
