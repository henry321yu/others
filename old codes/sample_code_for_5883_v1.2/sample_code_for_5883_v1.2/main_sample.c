#include "Customer.h"
#include "MMC5883MA.h" 

static int set_loop = 250;
static int set_cnt = 0;

float magnetic_field_x;
float magnetic_field_y;
float magnetic_field_z;

int main(void)
{
	float mag_raw_data[3] = {0.0};	//magnetic field vector, unit is gauss
	
	/* enable the sensor. */
	MMC5883MA_Enable();	
	
	while(1)
	{		
		/* get the MMC5883MA data, unit is gauss */
		MMC5883MA_GetData(mag_raw_data);	

		magnetic_field_x = mag_raw_data[0];	//unit is gauss
		magnetic_field_y = mag_raw_data[1];	//unit is gauss
		magnetic_field_z = mag_raw_data[2];	//unit is gauss
		
		/* if sample rate is 50Hz, so here do SET action every 5 seconds*/
		set_cnt++;
		if(set_cnt>=set_loop){
			set_cnt = 0;
			MMC5883MA_SET();	
		}	
		
		/* sampling interval is 20ms. */
		Delay_Ms(20);
	}
}

