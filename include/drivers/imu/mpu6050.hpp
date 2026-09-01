#ifndef __MPU6050_HPP__
#define __MPU6050_HPP__

#include "mpu6050_type.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class I2C;

class MPU6050 {
public:
    MPU6050(int bus_number = 1);
    ~MPU6050() = default;
    Status init();

    void readRangeConfig();
    void setGyroscopeRange(GyroRange range);
    void setAccelerometerRange(AccelRange range);
    void setDlpfBandwidth(DlpfBandwidth bandwidth);

    double getAccelerationX() const;
    double getAccelerationY() const;
    double getAccelerationZ() const;

    double getAngularVelocityX() const;
    double getAngularVelocityY() const;
    double getAngularVelocityZ() const;

    double convertRawAccData(int16_t accel_raw) const;
    double convertRawGyroData(int16_t gyro_raw) const;

    void calibrate();
    void setGyroOffset(double* offset);
    void setAccOffset(double* offset);
    void printConfig() const;
    void printOffsets() const;
    void cleanTerminal() const;
    void printAcceleration() const;
    void printAngularVelocity() const;
    void reportError(int error, std::string error_info = "Errno");

private:
    uint8_t _addr;
    std::shared_ptr<I2C> _i2c{nullptr};
    int barWidth = 50;
    char filename[11];
    bool calibrated = false;
    int ranges[R_ALL] = {2, 250, 260};
    double gyro_offset[C_ALL] = {0.0, 0.0, 0.0};
    double acc_offset[C_ALL] = {0.0, 0.0, 0.0};

    const std::array<int, 4> accel_ranges{2, 4, 8, 16};
    const std::array<int, 4> gyro_ranges{250, 500, 1000, 2000};
    const std::array<int, 7> dlpf_ranges{260, 184, 94, 44, 21, 10, 5};

    const std::unordered_map<int, int> accel_map{{2, 16384}, {4, 8192}, {8, 4096}, {16, 2048}};
    const std::unordered_map<int, double> gyro_map{
        {250, 131}, {500, 65.5}, {1000, 32.8}, {2000, 16.4}};
};

#endif