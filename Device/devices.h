#ifndef DEVICES_H
#define DEVICES_H


#define I2C_DEVICE "/dev/i2c-1"

#define LEDSW_ADDR  0x20
#define MOD_ROTARY  0x21
#define TEMP_ADDR   0x48
#define ADC_CDS     0x4C
#define EPR_ADDR    0x50
#define RTC_ADDR    0x68
#define IMU_ADDR    0x69
#define IXP_ADDR    0x73

#define GP_LED0 0
#define GP_LED1 1
#define GP_LED2 2
#define GP_LED3 3
#define GP_LEDA  4

#define B_ON false
#define B_OFF true


#endif // DEVICES_H
