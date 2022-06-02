#include <stdint.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t
#define DEV_CS_PIN 23
#define channel_A 0x30
#define channel_B 0x34
#define DAC_Value_Max 65535
#define DAC_VREF 3.3

void SPI_WriteByte(uint8_t value) {
	int read_data;
	read_data = wiringPiSPIDataRW(0, &value, 1);
	if (read_data < 0)
		perror("wiringPiSPIDataRW dailed \r\n");
}
UBYTE SPI_ReadByte() {
	UBYTE read_data, value = 0xff;
	read_data = wiringPiSPIDataRW(0, &value, 1);
	if(read_data < 0)
		perror("wiringPiSPIDataRW failed \r\n");
}
void Write_DAC8532(UBYTE Channel, UWORD Data) {
	digitalWrite(DEV_CS_PIN, 0);
	SPI_WriteByte(Channel);
	SPI_WriteByte((Data>>8));
	SPI_WriteByte((Data&0xff));
	digitalWrite(DEV_CS_PIN, 1);
}
void DAC8532_Out_Voltage(UBYTE Channel, float Voltage) {
	UWORD temp = 0;
	if ((Voltage <= DAC_VREF) && (Voltage >= 0)) {
		temp = (UWORD)(Voltage*DAC_Value_Max/DAC_VREF);
		Write_DAC8532(Channel, temp);
	}
}
int main(void) {
	UDOUBLE i;
	if (wiringPiSetupGpio() < 0) {
		printf("set wiringPi lib failed !!!\r\n");
		return 1;
	} else {
		printf("set wiringPi lib success !!!\r\n");
	}
	pinMode(DEV_CS_PIN, OUTPUT);
	wiringPiSPISetupMode(0, 32000000, 1);
	printf("programm start\r\n");
	DAC8532_Out_Voltage(channel_A, 0);
	float x[128];
	for (i=0;i<128;i++) {
		x[i] = (float)sin(2*M_PI*i/128)*1.5+1.5;
	}
	while(1) {
		for (i = 0; i < 128; i++) {
			DAC8532_Out_Voltage(channel_A, x[i]);
			DAC8532_Out_Voltage(channel_B, x[i]);
			delay(5);
		}
	}
	return 0;
}
