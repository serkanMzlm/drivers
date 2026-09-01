#ifndef __BMP180_HPP__
#define __BMP180_HPP__

#include "bmp180_type.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

class I2C;

class BMP180 {
public:
    BMP180(int bus_number = 1);
    ~BMP180() = default;
    Status init();

    void readCalibrationCoef();
    int16_t readRawTemperature();
    uint32_t readRawPressure();
    float readTemperature();
    int32_t readPressure();

    float readAltitude(float sealevelPressure = 101325);
    float readSealevelPressure(float altitude_meters = 0);

    void prinfCalbrationCoef() const;
    void reportError(int error, std::string error_info = "errno") const;

private:
    int8_t read8(uint8_t reg);
    int16_t read16(uint8_t reg);
    int32_t computeB5(int32_t UT);

private:
    uint8_t mode = 0;
    int16_t temp_data;
    int16_t AC1, AC2, AC3;
    uint16_t AC4, AC5, AC6;
    int16_t B1, B2;
    int16_t MB, MC, MD;
    uint8_t _addr;
    std::shared_ptr<I2C> _i2c{nullptr};
};

#endif