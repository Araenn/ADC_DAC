#include <stdint.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <time.h>
#include "stdio.h"
#include <string.h>

#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

// GPIO config

#define DEV_RST_PIN 18
#define DEV_CS_PIN 22
#define DEV_DRDY_PIN 17

UBYTE ScanMode = 0;

/* gain channel*/
typedef enum
{	ADS1256_GAIN_1			= 0,	/* GAIN   1 */
	ADS1256_GAIN_2			= 1,	/*GAIN   2 */
	ADS1256_GAIN_4			= 2,	/*GAIN   4 */
	ADS1256_GAIN_8			= 3,	/*GAIN   8 */
	ADS1256_GAIN_16			= 4,	/* GAIN  16 */
	ADS1256_GAIN_32			= 5,	/*GAIN    32 */
	ADS1256_GAIN_64			= 6,	/*GAIN    64 */
}ADS1256_GAIN;

typedef enum
{	ADS1256_30000SPS = 0,
	ADS1256_15000SPS,
	ADS1256_7500SPS,
	ADS1256_3750SPS,
	ADS1256_2000SPS,
	ADS1256_1000SPS,
	ADS1256_500SPS,
	ADS1256_100SPS,
	ADS1256_60SPS,
	ADS1256_50SPS,
	ADS1256_30SPS,
	ADS1256_25SPS,
	ADS1256_15SPS,
	ADS1256_10SPS,
	ADS1256_5SPS,
	ADS1256_2d5SPS,  
	ADS1256_DRATE_MAX
}ADS1256_DRATE;

typedef enum
{/*Register address, followed by reset the default values */
	REG_STATUS = 0,	// x1H
	REG_MUX    = 1, // 01H
	REG_ADCON  = 2, // 20H
	REG_DRATE  = 3, // F0H
	REG_IO     = 4, // E0H
	REG_OFC0   = 5, // xxH
	REG_OFC1   = 6, // xxH
	REG_OFC2   = 7, // xxH
	REG_FSC0   = 8, // xxH
	REG_FSC1   = 9, // xxH
	REG_FSC2   = 10, // xxH
}ADS1256_REG;

typedef enum
{	CMD_WAKEUP  = 0x00,	// Completes SYNC and Exits Standby Mode 0000  0000 (00h)
	CMD_RDATA   = 0x01, // Read Data 0000  0001 (01h)
	CMD_RDATAC  = 0x03, // Read Data Continuously 0000   0011 (03h)
	CMD_SDATAC  = 0x0F, // Stop Read Data Continuously 0000   1111 (0Fh)
	CMD_RREG    = 0x10, // Read from REG rrr 0001 rrrr (1xh)
	CMD_WREG    = 0x50, // Write to REG rrr 0101 rrrr (5xh)
	CMD_SELFCAL = 0xF0, // Offset and Gain Self-Calibration 1111    0000 (F0h)
	CMD_SELFOCAL= 0xF1, // Offset Self-Calibration 1111    0001 (F1h)
	CMD_SELFGCAL= 0xF2, // Gain Self-Calibration 1111    0010 (F2h)
	CMD_SYSOCAL = 0xF3, // System Offset Calibration 1111   0011 (F3h)
	CMD_SYSGCAL = 0xF4, // System Gain Calibration 1111    0100 (F4h)
	CMD_SYNC    = 0xFC, // Synchronize the A/D Conversion 1111   1100 (FCh)
	CMD_STANDBY = 0xFD, // Begin Standby Mode 1111   1101 (FDh)
	CMD_RESET   = 0xFE, // Reset to Power-Up Values 1111   1110 (FEh)
}ADS1256_CMD;

static const uint8_t ADS1256_DRATE_E[ADS1256_DRATE_MAX] =
{   0xF0,		/*reset the default values  */
	0xE0,
	0xD0,
	0xC0,
	0xB0,
	0xA1,
	0x92,
	0x82,
	0x72,
	0x63,
	0x53,
	0x43,
	0x33,

	0x20,
	0x13,
	0x03
};



void SPI_WriteByte(uint8_t value) {
	int read_data;
	read_data = wiringPiSPIDataRW(0, &value, 1);
	if (read_data < 0)
		perror("wiringPiSPIDataRW failed\r\n");
}
UBYTE SPI_ReadByte() {
	UBYTE read_data, value = 0xff;
	read_data = wiringPiSPIDataRW(0, &value, 1);
	if (read_data < 0)
		perror("wiringPiSPIDataRW failed\r\n");
	return value;
}
void ADS1256_WriteCmd(UBYTE Cmd) {
	digitalWrite(DEV_CS_PIN, 0);
	SPI_WriteByte(Cmd);
	digitalWrite(DEV_CS_PIN, 1);
}
void ADS1256_WriteReg(UBYTE Reg, UBYTE data) {
	digitalWrite(DEV_CS_PIN, 0);
	SPI_WriteByte(CMD_WREG|Reg);
	SPI_WriteByte(0x00);
	SPI_WriteByte(data);
	digitalWrite(DEV_CS_PIN, 1);
}
UBYTE ADS1256_Read_data(UBYTE Reg) {
	UBYTE temp = 0;
	digitalWrite(DEV_CS_PIN, 1);
	SPI_WriteByte(CMD_RREG|Reg);
	SPI_WriteByte(0x00);
	delay(1);
	temp = SPI_ReadByte();
	digitalWrite(DEV_CS_PIN, 1);
	return temp;
}
void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate) {
	UDOUBLE i = 0;
	for (i = 0; i<4000000; i++) {
		if(digitalRead(DEV_DRDY_PIN) == 0)
			break;
	}
	UBYTE buf[4] = {0, 0, 0, 0};
	buf[0] = (0<<3) | (1<<2) | (0<<1);
	buf[1] = 0x08;
	buf[2] = (0<<5) | (0<<3) | (gain<<0);
	buf[3] = ADS1256_DRATE_E[drate];
	digitalWrite(DEV_CS_PIN, 0);
	SPI_WriteByte(CMD_WREG | 0);
	SPI_WriteByte(0x03);
	SPI_WriteByte(buf[0]);
	SPI_WriteByte(buf[1]);
	SPI_WriteByte(buf[2]);
	SPI_WriteByte(buf[3]);
	digitalWrite(DEV_CS_PIN, 1);
	delay(1);
}
UBYTE ADS1256_init(void) {
	digitalWrite(DEV_RST_PIN, 1);
	delay(200);
	digitalWrite(DEV_RST_PIN, 0);
	delay(200);
	digitalWrite(DEV_RST_PIN, 1);
	ADS1256_ConfigADC(ADS1256_GAIN_1, ADS1256_30000SPS);
	return 0;
}
UDOUBLE ADS1256_GetChannelValue(UBYTE Channel) {
	UDOUBLE Value = 0;
	while (digitalRead(DEV_DRDY_PIN) == 1);
		ADS1256_WriteReg(REG_MUX, (Channel<<4) | (1<<3));
		ADS1256_WriteCmd(CMD_SYNC);
		ADS1256_WriteCmd(CMD_WAKEUP);
		
		UDOUBLE read = 0;
	UBYTE buf[3] = {0,0,0};
	delay(1);
	digitalWrite(DEV_CS_PIN, 0);
	SPI_WriteByte(CMD_RDATA);
	delay(1);
	buf[0]=SPI_ReadByte();
	buf[1]=SPI_ReadByte();
	buf[2]=SPI_ReadByte();
	digitalWrite(DEV_CS_PIN, 1);
	read=((UDOUBLE)buf[0]<<16)& 0x00FF0000;
	read |= ((UDOUBLE)buf[1]<<8);
	read |= buf[2];
	if (read & 0x800000)
	read &= 0xFF00000;
	Value=read;
	return Value;
}
int main(void)
{UDOUBLE ADC[8],i;
	printf("demo\r\n");
	if(wiringPiSetupGpio()<0){
		printf("set wiringPi lib failed ||| \r\n");
		return 1;
	} else{
	printf("set wiringPi lib success ||| \r\n");
}
pinMode(DEV_RST_PIN, OUTPUT);
pinMode(DEV_CS_PIN, OUTPUT);
pinMode(DEV_DRDY_PIN, INPUT);
wiringPiSPISetupMode(0,320000,1);
ADS1256_init();
while(1){
	for(i=0;i<8;i++){
		ADC[i]=ADS1256_GetChannelValue(i);
		printf("%d %f \r\n", i, ADC[i]*5.0/0x7fffff);
	}
	printf("\33[8A]");
}
return 0;
}
