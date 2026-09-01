#include "bus_interface.hpp"
#include "i2c.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

extern "C" {
#include <errno.h>
#include <i2c/smbus.h>
}

#include "lcd_2x16.hpp"

LCD::LCD(int bus_number, uint8_t address)
    : _addr(address), _i2c(std::make_shared<I2C>(bus_number)) {
    if (init() != Status::Success) {
        exit(-1);
    }

    begin();
}

void LCD::sleep_ms(uint16_t ms) {
    usleep(static_cast<useconds_t>(ms) * 1000U);
}
void LCD::sleep_us(uint16_t us) {
    usleep(us);
}

Status LCD::init() {
    _i2c->setAddress(_addr);
    if (_i2c->connect()) {
        reportError(errno, "Failed  to started I2C.");
        return Status::Failure;
    }
    return Status::Success;
}

void LCD::begin() {
    _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
    sleep(50);
    expanderWrite(_backlight);
    sleep(1000);

    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x02 << 4);

    command(LCD_FUNCTIONSET | _displayfunction);
    _displaycontrol = LCD_DISPLAYON;
    display(Switch::On);
    clear();

    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
    home();
}

void LCD::clear() {
    command(LCD_CLEARDISPLAY);
    sleep(2);
}

void LCD::home() {
    command(LCD_RETURNHOME);
    sleep(2);
}

void LCD::setCursor(uint8_t col, uint8_t row) {
    int row_offsets[] = {0x00, 0x40};
    if (row > _rows) {
        row = _rows - 1;
    }
    command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LCD::display(Switch state) {
    if (state == Switch::On)
        _displaycontrol |= LCD_DISPLAYON;
    else
        _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::cursor(Switch state) {
    if (state == Switch::On)
        _displaycontrol |= LCD_CURSORON;
    else
        _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::blink(Switch state) {
    if (state == Switch::On)
        _displaycontrol |= LCD_BLINKON;
    else
        _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::autoscroll(Switch state) {
    if (state == Switch::On)
        _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    else
        _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::backlight(Switch state) {
    if (state == Switch::On)
        _backlight = LCD_BACKLIGHT;
    else
        _backlight = LCD_NOBACKLIGHT;
    expanderWrite(_backlight);
}

void LCD::scrollDisplayLeft() {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE);
}

void LCD::scrollDisplayRight() {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LCD::leftToRight() {
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::rightToLeft() {
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7;
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
        command(charmap[i], Rs);
    }
}

Status LCD::writeString(char* text) {
    if (!_i2c || !_i2c->isOpenPort()) {
        return Status::PortClosed;
    }

    unsigned char ucTemp[2];
    int i = 0;
    if (!_i2c->isOpenPort() || text == NULL) {
        return Status::Failure;
    }

    while (i < 16 && *text) {
        ucTemp[0] = _backlight | 1 | (*text & 0xf0);
        _i2c->writeByte(ucTemp);
        sleep(5);
        ucTemp[0] |= 4;
        _i2c->writeByte(ucTemp);
        sleep(5);
        ucTemp[0] &= ~4;
        _i2c->writeByte(ucTemp);
        sleep(5);
        ucTemp[0] = _backlight | 1 | (*text << 4);
        _i2c->writeByte(ucTemp);
        ucTemp[0] |= 4; // pulse E
        _i2c->writeByte(ucTemp);
        sleep(5);
        ucTemp[0] &= ~4;
        _i2c->writeByte(ucTemp);
        usleep(5);
        text++;
        i++;
    }

    return Status::Success;
}

//////////////////////////////////////
void LCD::write4bits(uint8_t value) {
    expanderWrite(value);
    pulseEnable(value);
}

Status LCD::expanderWrite(uint8_t data) {
    if (_i2c->writeByte(&data) > 0) {
        reportError(errno, "Error writing data over I2C");
        return Status::Failure;
    }
    return Status::Success;
}

void LCD::pulseEnable(uint8_t data) {
    expanderWrite(data | En);
    usleep(1);
    expanderWrite(data & ~En);
    usleep(50);
}

void LCD::command(uint8_t value, uint8_t mode) {
    write4bits((value & 0xf0) | mode);
    write4bits(((value << 4) & 0xf0) | mode);
}

/////////////////////////////////////////
void LCD::sleep(uint16_t ms_time) {
    usleep(ms_time * 1000);
}

void LCD::reportError(int error, std::string info) {
    std::cerr << "Error! " << info << " : " << strerror(error);
}