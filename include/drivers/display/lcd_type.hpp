#ifndef __LCD_TYPE_HPP__
#define __LCD_TYPE_HPP__

#define LCD_ROW 2
#define LCD_COLUMN 16
#define LCD_SLAVE_ADDR (0x27)

// Command
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_CURSORON 0x02
#define LCD_BLINKON 0x01

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_MOVERIGHT 0x04

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0b00000100 // Enable bit
#define Rw 0b00000010 // Read/Write bit
#define Rs 0b00000001 // Register select bit

enum class Switch : uint8_t {
    On = 0x00,
    Off = 0x01,
};

enum class Status : uint8_t {
    Success = 0,
    Waiting,
    Failure,
    Aborted,
    Retrying,
    PortClosed,
    WriteFail,
    ParseFail,
    ChecksumFail,
    Unsupported,
    OutOfRange,
    Error,
    Warning,
    Timeout
};

#endif