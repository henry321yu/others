/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2017
 *****************************************************************************/

/**
 * @brief
 * This file implement magnetic sensor driver APIs. 
 * Modified history: 
 * V1.1: Add the interrupt function on 20190307
 * V1.2: Add the SET-RESET function on 20190312
 */ 
 
typedef   signed char  int8_t; 		// signed 8-bit number    (-128 to +127)
typedef unsigned char  uint8_t; 	// unsigned 8-bit number  (+0 to +255)
typedef   signed short int16_t; 	// signed 16-bt number    (-32768 to +32767)
typedef unsigned short uint16_t; 	// unsigned 16-bit number (+0 to +65535)
typedef   signed int   int32_t; 	// signed 32-bt number    (-2,147,483,648 to +2,147,483,647)
typedef unsigned int   uint32_t; 	// unsigned 32-bit number (+0 to +4,294,967,295)

#define MMC5883MA_7BITI2C_ADDRESS	0x30

#define MMC5883MA_REG_DATA			0x00
#define MMC5883MA_REG_XL			0x00
#define MMC5883MA_REG_XH			0x01
#define MMC5883MA_REG_YL			0x02
#define MMC5883MA_REG_YH			0x03
#define MMC5883MA_REG_ZL			0x04
#define MMC5883MA_REG_ZH			0x05
#define MMC5883MA_REG_TEMP			0x06
#define MMC5883MA_REG_STATUS		0x07
#define MMC5883MA_REG_CTRL0			0x08
#define MMC5883MA_REG_CTRL1			0x09
#define MMC5883MA_REG_CTRL2			0x0A
#define MMC5883MA_REG_X_THD			0x0B
#define MMC5883MA_REG_Y_THD			0x0C
#define MMC5883MA_REG_Z_THD			0x0D
#define MMC5883MA_REG_PRODUCTID		0x2F
 
#define MMC5883MA_CMD_RESET         0x10
#define MMC5883MA_CMD_SET			0x08
#define MMC5883MA_CMD_TM_M			0x01
#define MMC5883MA_CMD_TM_T			0x02
#define MMC5883MA_CMD_START_MDT		0x04
#define MMC5883MA_CMD_100HZ			0x00
#define MMC5883MA_CMD_200HZ			0x01
#define MMC5883MA_CMD_400HZ			0x02
#define MMC5883MA_CMD_600HZ			0x03
#define MMC5883MA_CMD_CM_14HZ		0x01
#define MMC5883MA_CMD_CM_5HZ		0x02
#define MMC5883MA_CMD_CM_1HZ		0x04
#define MMC5883MA_CMD_SW_RST		0x80
#define MMC5883MA_CMD_INT_MD_EN		0x40
#define MMC5883MA_CMD_INT_MDT_EN	0x20

#define MMC5883MA_PRODUCT_ID		0x0C
#define MMC5883MA_OTP_READ_DONE_BIT	0x10
#define MMC5883MA_PUMP_ON_BIT		0x08
#define MMC5883MA_MDT_BIT			0x04
#define MMC5883MA_MEAS_T_DONE_BIT	0x02
#define MMC5883MA_MEAS_M_DONE_BIT	0x01

// 16-bit mode, null field output (32768)
#define MMC5883MA_OFFSET			32768
#define MMC5883MA_SENSITIVITY		4096
#define MMC5883MA_T_ZERO			(-75)
#define MMC5883MA_T_SENSITIVITY		0.7
	
/**
 * @brief Enable the sensor
 */
int MMC5883MA_Enable(void);

/**
 * @brief Disable the sensor
 */
void MMC5883MA_Disable(void);

/**
 * @brief SET operation
 */
void MMC5883MA_SET(void);

/**
 * @brief RESET operation
 */
void MMC5883MA_RESET(void);

/**
 * @brief Get the temperature output
 * @param t_out[0] is the temperature, unit is degree Celsius
 */
void MMC5883MA_GetTemperature(float *t_out);

/**
 * @brief Get sensor data
 * @param mag_out is the magnetic field vector, unit is gauss
 */
void MMC5883MA_GetData(float *mag_out);

/**
 * @brief Get sensor data with SET and RESET function 
 * @param mag_out is the magnetic field vector, unit is gauss
 */
void MMC5883MA_GetData_With_SET_RESET(float *mag_out);

