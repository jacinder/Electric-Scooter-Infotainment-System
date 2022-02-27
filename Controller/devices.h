#ifndef DEVICES_H
#define DEVICES_H

#define I2C_DEVICE "/dev/i2c-1"
#define CAN_DEVICE "can0"

#define CAN_OWN_ID 0x10
#define CAN_DEV_ID 0x20

#define CAN_DEF_PACKET_SIZE 0x8

#define LEDSW_ADDR  0x20
#define MOD_ROTARY  0x21
#define TEMP_ADDR   0x49
#define ADC_CDS     0x4C
#define EPR_ADDR    0x50
#define RTC_ADDR    0x68
#define IMU_ADDR    0x69
#define IXP_ADDR    0x73

#define GP_LED0 0
#define GP_LED1 1
#define GP_LED2 2
#define GP_LED3 3
#define GP_LEDA 5

#define DS1337_REG_SEC 0
#define DS1337_REG_MIN 1
#define DS1337_REG_HOUR 2
#define DS1337_REG_DAY 4
#define DS1337_REG_MONTH 5
#define DS1337_REG_YEAR 6

#define RED 0x00
#define BLUE 0x01
#define GREEN 0x02


#define M_OFF 0x00
#define M_ON 0x01
#define A_OFF 0x02
#define A_ON 0x03

//
// 0000. sw5 sw4 00

// RGB
#define START 0b0101
#define OFF 0b0111
#define FIRE 0b0110
#define FREZZING 0b0011

// SW4
#define LIGHT_ON 0b0
#define LIGHT_OFF 0b1

// SW5
#define CAN_OK 0b0
#define CAN_ERR 0b1

#endif // DEVICES_H
