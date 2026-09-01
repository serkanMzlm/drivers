#ifndef __LCD_2X16_HPP__
#define __LCD_2X16_HPP__

#include "lcd_type.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>

class I2C;

class LCD {
private:
    uint8_t _cols{LCD_COLUMN};
    uint8_t _rows{LCD_ROW};
    uint8_t _addr;
    uint8_t _backlight{LCD_BACKLIGHT};
    uint8_t _displayfunction;
    uint8_t _displaymode;
    uint8_t _displaycontrol;
    std::shared_ptr<I2C> _i2c{nullptr};

public:
    LCD(int bus_number, uint8_t address = LCD_SLAVE_ADDR);
    ~LCD() = default;
    Status init();
    void begin();
    void clear();
    void home();
    void setCursor(uint8_t col, uint8_t row);
    void display(Switch state);
    void cursor(Switch state);
    void blink(Switch state);
    void autoscroll(Switch state);
    void backlight(Switch state);
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void leftToRight();
    void rightToLeft();
    void createChar(uint8_t location, uint8_t charmap[]);
    Status writeString(char* text);

private:
    static void sleep_ms(uint16_t ms);
    static void sleep_us(uint16_t us);
    void write4bits(uint8_t value);
    Status expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);
    void command(uint8_t value, uint8_t mode = 0);
    void sleep(uint16_t ms_time);
    void reportError(int error, std::string info = "Errno");
};

#endif