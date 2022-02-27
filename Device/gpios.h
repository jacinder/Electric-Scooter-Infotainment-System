#ifndef GPIOS_H
#define GPIOS_H


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// See the bcm2711-peripherals.pdf file - page 65.
#define GPIO_BASE 0xfe200000

#define SET_REG32(x) (1<<x)

#define GPIO_FUNCTION(x,y,z) x&=~(0x07<<y)|z<<y

#define GPFSEL0 0x00 // Defines the offset of the GPIO Function Select Register0
#define GPFSEL1 0x04 // Defines the offset of the GPIO Function Select Register1
#define GPSET0 0x1c  // Defines the offset of the GPIO Pin Output Set0 Register
#define GPCLR0 0x28  // Defines the offset of the GPIO Pin Output Clear Register

// See the AIoT_Development_Kit_Schematics_3rd.pdf file - page 10.
#define LED0  6
#define LED1 16
#define LED2 17
#define LED3 18


#endif // GPIOS_H


