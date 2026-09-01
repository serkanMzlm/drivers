extern "C" {
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
}

#include "bmp180.hpp"
#include "bus_interface.hpp"
#include "i2c.hpp"

BMP180::BMP180(int bus_number) : _addr(BMP180_SLAVE_ADDR), _i2c(std::make_shared<I2C>(bus_number)) {
    if (init() != Status::Success) {
        exit(-1);
    }
}

Status BMP180::init() {
    _i2c->setAddress(_addr);
    if (_i2c->connect()) {
        reportError(errno, "Failed  to started I2C.");
        return Status::Failure;
    }
    return Status::Success;
}

void BMP180::readCalibrationCoef() {
    AC1 = read16(BMP180_AC1_H);
    AC2 = read16(BMP180_AC2_H);
    AC3 = read16(BMP180_AC3_H);
    AC4 = static_cast<uint16_t>(read16(BMP180_AC4_H));
    AC5 = static_cast<uint16_t>(read16(BMP180_AC4_H));
    AC6 = static_cast<uint16_t>(read16(BMP180_AC4_H));

    B1 = read16(BMP180_B1_H);
    B2 = read16(BMP180_B2_H);

    MB = read16(BMP180_MB_H);
    MC = read16(BMP180_MC_H);
    MD = read16(BMP180_MD_H);
}

int16_t BMP180::readRawTemperature() {
    int result = _i2c->writeRegByte(BMP180_CONTROL, BMP180_TEMP_CMD);
    if (result < 0) {
        reportError(errno);
    }
    usleep(5 * 1000);
    return read16(BMP180_TEMP_H);
    return 0;
}

uint32_t BMP180::readRawPressure() {
    _i2c->writeRegByte(BMP180_CONTROL, BMP180_READ_PRESSURE_CMD + (mode << 6));
    switch (mode) {
    case 0:
        usleep(5 * 1000);
        break;
    case 1:
        usleep(8 * 1000);
        break;
    case 2:
        usleep(14 * 1000);
        break;
    case 3:
        usleep(26 * 1000);
        break;
    }

    uint32_t raw = read16(BMP180_PRESSUREDATA);
    raw <<= 8;
    raw |= read8(BMP180_PRESSUREDATA + 2);
    raw >>= (8 - mode);
    return raw;
}

int32_t BMP180::computeB5(int32_t UT) {
    int32_t X1 = (UT - (int32_t)AC6) * ((int32_t)AC5) >> 15;
    int32_t X2 = ((int32_t)MC << 11) / (X1 + (int32_t)MD);
    return X1 + X2;
}

float BMP180::readTemperature() {
    int32_t UT, B5;
    float temp;
    UT = readRawTemperature();
    B5 = computeB5(UT);
    temp = ((B5 + 8) >> 4);
    temp /= 10.0;
    return temp;
}

int32_t BMP180::readPressure() {
    int32_t UT, UP, p;
    int32_t B3, B5, B6;
    uint32_t B4, B7;
    int32_t X1, X2, X3;
    UT = readRawTemperature();
    UP = readRawPressure();
    B5 = computeB5(UT);
    B6 = B5 - 4000;
    X1 = ((int32_t)B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = ((int32_t)AC2 * B6) >> 11;
    X3 = X1 + X2;
    B3 = ((((int32_t)AC1 * 4 + X3) << mode) + 2) / 4;

    X1 = ((int32_t)AC3 * B6) >> 13;
    X2 = ((int32_t)B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    B4 = ((uint32_t)AC4 * (uint32_t)(X3 + 32768)) >> 15;
    B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL >> mode);

    if (B7 < 0x80000000) {
        p = (B7 * 2) / B4;
    } else {
        p = (B7 / B4) * 2;
    }
    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + (int32_t)3791) >> 4);

    return p;
}

float BMP180::readAltitude(float sealevelPressure) {
    float pressure = readPressure();
    float altitude = 44330 * (1.0 - pow(pressure / sealevelPressure, 0.1903));
    return altitude;
}

float BMP180::readSealevelPressure(float altitude_meters) {
    float pressure = readPressure();
    return (int32_t)(pressure / pow(1.0 - altitude_meters / 44330, 5.255));
}

void BMP180::prinfCalbrationCoef() const {
    std::cout << "AC1: " << AC1 << std::endl;
    std::cout << "AC2: " << AC2 << std::endl;
    std::cout << "AC3: " << AC3 << std::endl;
    std::cout << "AC4: " << AC4 << std::endl;
    std::cout << "AC5: " << AC5 << std::endl;
    std::cout << "AC6: " << AC6 << std::endl;

    std::cout << "B1: " << B1 << std::endl;
    std::cout << "B2: " << B2 << std::endl;

    std::cout << "MB: " << MB << std::endl;
    std::cout << "MC: " << MC << std::endl;
    std::cout << "MD: " << MD << std::endl;
}

void BMP180::reportError(int error, std::string error_info) const {
    std::cerr << "Error! " << error_info << ": " << strerror(error);
}

int8_t BMP180::read8(uint8_t reg) {
    return static_cast<int8_t>(_i2c->readRegByte(reg));
}

int16_t BMP180::read16(uint8_t reg) {
    int16_t reg_h = _i2c->readRegByte(reg);
    int16_t reg_l = _i2c->readRegByte(reg + 1);
    return reg_l | reg_h << 8;
}