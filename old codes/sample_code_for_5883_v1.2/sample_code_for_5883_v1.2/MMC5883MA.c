/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright, and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2019
 *****************************************************************************/

/**
 * @brief
 * This file implement magnetic sensor driver APIs. 
 * Modified history: 
 * V1.1: Add the interrupt function on 20190307
 * V1.2: Add the SET-RESET function on 20190312
 */ 

#include "MMC5883MA.h"
#include "Customer.h"

/* Function declaration */

/**
 * @brief OTP read done check
 */
int MMC5883MA_Check_OTP(void);

/**
 * @brief Check Product ID
 */
int MMC5883MA_CheckID(void);
	
/**
 * @brief Reset the sensor by software
 */
void MMC5883MA_Software_Reset(void);

/**
 * @brief Set the output resolution
 */
void MMC5883MA_SetOutputResolution(unsigned char res);

/**
 * @brief Enable the meas_done interrupt 
 */
void MMC5883MA_INT_Meas_Done_Enable(void);
	
/**
 * @brief Enable the MDT interrupt
 */
void MMC5883MA_INT_MDT_Enable(void);

/**
 * @brief Clear Meas_T_Done interrupt
 */
void MMC5883MA_INT_Meas_T_Done_Clear(void);

/**
 * @brief Clear Meas_M_Done interrupt
 */
void MMC5883MA_INT_Meas_M_Done_Clear(void);

/**
 * @brief Clear MDT interrupt
 */
void MMC5883MA_INT_MDT_Clear(void);

/*********************************************************************************
* decription: OTP read done check
*********************************************************************************/
int MMC5883MA_Check_OTP(void)
{
	unsigned char reg_val = 0;
	
	Delay_Ms(5);
	/* Read register 0x07, check OTP_Read_Done bit */
	I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_STATUS, &reg_val);	
	if((reg_val&0x10) != MMC5883MA_OTP_READ_DONE_BIT)
		return -1;
	
	return 1;
}

/*********************************************************************************
* decription: Check Product ID
*********************************************************************************/
int MMC5883MA_CheckID(void)
{
	unsigned char pro_id = 0;
	
	/* Read register 0x2F, check product ID */
	I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_PRODUCTID, &pro_id);	
	if(pro_id != MMC5883MA_PRODUCT_ID)
		return -1;
	
	return 1;
}

/*********************************************************************************
* decription: Reset the sensor by software
*********************************************************************************/
void MMC5883MA_Software_Reset(void)
{
	/* Write 0x80 to register 0x09, set SW_RST bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL1, MMC5883MA_CMD_SW_RST);
	
	/* Delay at 50ms to finish the software reset operation */
	Delay_Ms(50);
	
	return;	
}

/*********************************************************************************
* decription: SET operation
*********************************************************************************/
void MMC5883MA_SET(void)
{	
	/* Write 0x08 to register 0x08, set SET bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_SET);
	
	/* Delay at 1ms to finish the SET operation */
	Delay_Ms(1);
	
	return;	
}

/*********************************************************************************
* decription: RESET operation
*********************************************************************************/
void MMC5883MA_RESET(void)
{	
	/* Write 0x10 to register 0x08, set RESET bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_RESET);
	
	/* Delay at 1ms to finish the RESET operation */
	Delay_Ms(1);
	
	return;	
}

/*********************************************************************************
* decription: Set the output resolution
*********************************************************************************/
void MMC5883MA_SetOutputResolution(unsigned char res)
{
	/* Write register 0x09, Set BW<1:0> = 0x00, 0x01, 0x02, or 0x03 */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL1, res);
	
	return;		
}

/*********************************************************************************
* decription: Enable the int when a mag or temp measuremet event is completed
*********************************************************************************/
void MMC5883MA_INT_Meas_Done_Enable(void)
{
	/* Write register 0x0A, Set INT_Meas_Done_EN high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL2, MMC5883MA_CMD_INT_MD_EN);
	
	return;		
}

/*********************************************************************************
* decription: Enable the int when a motion is detected 
*********************************************************************************/
void MMC5883MA_INT_MDT_Enable(void)
{
	/* Step size is 1mG, threshold = mdt_threshold*1mG */
	unsigned char mdt_threshold = 0x3C;
	
	/* Write register 0x0B, Set X, Y and Z threshold */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_X_THD, mdt_threshold);
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_Y_THD, mdt_threshold);
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_Z_THD, mdt_threshold);
	
	/* Write register 0x0A, Set CM_Freq [3:0], Set INT_MDT_EN high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL2, MMC5883MA_CMD_CM_14HZ | MMC5883MA_CMD_INT_MDT_EN);
	
	/* Write register 0x08, Set Start_MDT high, Start the motion detector */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_START_MDT);
	
	return;		
}

/*********************************************************************************
* decription: Clear Meas_T_Done interrupt
*********************************************************************************/
void MMC5883MA_INT_Meas_T_Done_Clear(void)
{
	/* Write register 0x07, Set Meas_T_Done bit high, clear Meas_T_Done interrupt */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_STATUS, MMC5883MA_MEAS_T_DONE_BIT);
	
	return;		
}

/*********************************************************************************
* decription: Clear Meas_M_Done interrupt
*********************************************************************************/
void MMC5883MA_INT_Meas_M_Done_Clear(void)
{
	/* Write register 0x07, Set Meas_M_Done bit high, clear Meas_M_Done interrupt */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_STATUS, MMC5883MA_MEAS_M_DONE_BIT);
	
	return;		
}

/*********************************************************************************
* decription: Clear MDT interrupt
*********************************************************************************/
void MMC5883MA_INT_MDT_Clear(void)
{
	/* Write register 0x07, Set Motion Detected bit high, clear MDT interrupt */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_STATUS, MMC5883MA_MDT_BIT);
	
	return;		
}

/*********************************************************************************
* decription: Enable sensor
*********************************************************************************/
int MMC5883MA_Enable(void)
{
	int ret = 0;
	
	/* Check OTP Read status */
	ret = MMC5883MA_Check_OTP();
	if(ret<0)
		return ret;	

	/* Check product ID */
	ret = MMC5883MA_CheckID();
	if(ret<0)
		return ret;
	
	/* SET operation */
	MMC5883MA_SET();
	
	/* Set output resolution */
	MMC5883MA_SetOutputResolution(MMC5883MA_CMD_100HZ);

	/* Write 0x01 to register 0x08, set TM_M bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_TM_M);
	Delay_Ms(10);
	
	return 1;
}

/*********************************************************************************
* decription: Disable sensor
*********************************************************************************/
void MMC5883MA_Disable(void)
{
	return;
}

/*********************************************************************************
* decription: Read the temperature output
*********************************************************************************/
void MMC5883MA_GetTemperature(float *t_out)
{
	uint8_t reg_status = 0;
	uint8_t reg_t = 0;
	
	/* Write 0x02 to register 0x08, set TM_T bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_TM_T);
	Delay_Ms(1);
	
	/* Read register 0x07, check Meas_T_Done bit */		
	I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS,MMC5883MA_REG_STATUS,&reg_status);	
	while((reg_status&0x02) != 0x02){
		Delay_Ms(1);
		I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS,MMC5883MA_REG_STATUS,&reg_status);	
	}
	
	/* Read register 0x06 */	
	I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_TEMP, &reg_t);
		
	/* The temperature output has not been calibrated, can not present the ambient temperature*/
	t_out[0] = (float)reg_t*MMC5883MA_T_SENSITIVITY + MMC5883MA_T_ZERO;//unit is degree Celsius
	
	return;
}

/*********************************************************************************
* decription: Read the data register and convert to magnetic field vector
*********************************************************************************/
void MMC5883MA_GetData(float *mag_out)
{
	uint8_t reg_status = 0;
	
	uint8_t data_reg[6] ={0};
	uint16_t data_temp[3] = {0};
	
	/* Write 0x01 to register 0x08, set TM_M bit high */
	I2C_Write_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_CTRL0, MMC5883MA_CMD_TM_M);

	/* Read register 0x07, check Meas_M_Done bit */		
	I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS,MMC5883MA_REG_STATUS,&reg_status);	
	while((reg_status&0x01) != 0x01){
		Delay_Ms(1);
		I2C_Read_Reg(MMC5883MA_7BITI2C_ADDRESS,MMC5883MA_REG_STATUS,&reg_status);	
	}
	
	/* Read register 0x00-0x05 */	
	I2C_MultiRead_Reg(MMC5883MA_7BITI2C_ADDRESS, MMC5883MA_REG_DATA, 6, data_reg);
		
	/* The output raw data unit is "count or LSB" */ 
	data_temp[0]=(uint16_t)(data_reg[1]<< 8 | data_reg[0]);
	data_temp[1]=(uint16_t)(data_reg[3]<< 8 | data_reg[2]);
	data_temp[2]=(uint16_t)(data_reg[5]<< 8 | data_reg[4]);

	/* Transform to unit Gauss */
	mag_out[0] = ((float)data_temp[0] - MMC5883MA_OFFSET)/MMC5883MA_SENSITIVITY; //unit Gauss
	mag_out[1] = ((float)data_temp[1] - MMC5883MA_OFFSET)/MMC5883MA_SENSITIVITY;
	mag_out[2] = ((float)data_temp[2] - MMC5883MA_OFFSET)/MMC5883MA_SENSITIVITY;
	
	return;
}

/*********************************************************************************
* decription: Read the data register with SET and RESET function 
*********************************************************************************/
void MMC5883MA_GetData_With_SET_RESET(float *mag_out)
{
	float magnetic_set_field[3];
	float magnetic_reset_field[3];
	
	/* Do RESET operation before TM and read data */
	MMC5883MA_RESET();
	
	/* TM and read data */
	MMC5883MA_GetData(magnetic_reset_field);
	
	/* Do SET operation before TM and read data */
	MMC5883MA_SET();
	
	/* TM and read data */
	MMC5883MA_GetData(magnetic_set_field);

	/* Get external magnetic field with (SET-REST)/2 to remove the sensor bridge offset */
	mag_out[0] = (magnetic_set_field[0] - magnetic_reset_field[0]) / 2;
	mag_out[1] = (magnetic_set_field[1] - magnetic_reset_field[1]) / 2;
	mag_out[2] = (magnetic_set_field[2] - magnetic_reset_field[2]) / 2;		

	return;		
}
