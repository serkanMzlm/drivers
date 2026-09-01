#ifndef __GPIO_CONTROL__
#define __GPIO_CONTROL__

typedef enum { OUTPUT, INPUT } PinMode;
int directionPin(int pin_number, PinMode pin_mode);
int exportPin(int pin_number);
int unexportPin(int pin_number);
int writePin(int pin_number, bool state);
int readPin(int pin_number);

#endif