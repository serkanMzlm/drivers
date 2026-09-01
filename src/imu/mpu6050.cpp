#include "mpu6050.hpp"

#include <iomanip>
extern "C" {
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

#include "bus_interface.hpp"
#include "i2c.hpp"

MPU6050::MPU6050(int bus_number) : _addr(MPU6050_ADDR), _i2c(std::make_shared<I2C>(bus_number)) {
    if (init() != Status::Success) {
        exit(-1);
    }

    readRangeConfig();
}

Status MPU6050::init() {
    _i2c->setAddress(_addr);
    if (_i2c->connect()) {
        reportError(errno, "Failed  to started I2C.");
        return Status::Failure;
    }
    return Status::Success;
}

void MPU6050::readRangeConfig() {
    for (int i = 0; i < R_ALL; i++) {
        int result = _i2c->readRegByte(DLPF_CONFIG + i);
        if (result < 0) {
            reportError(errno, "Configuration reading process failed (" + std::to_string(i) + ")");
        }
        result = result >> 3;
        switch (i) {
        case 0:
            ranges[DLPF_R] = dlpf_ranges[result];
            break;
        case 1:
            ranges[GYR_R] = gyro_ranges[result];
            break;
        case 2:
            ranges[ACC_R] = accel_ranges[result];
            break;
        }
    }
}

void MPU6050::setGyroscopeRange(GyroRange range) {
    if (!_i2c->writeRegByte(GYRO_CONFIG, range << 3)) {
        reportError(errno);
    }
    ranges[GYR_R] = gyro_ranges[static_cast<size_t>(range)];
}

void MPU6050::setAccelerometerRange(AccelRange range) {
    if (!_i2c->writeRegByte(ACC_CONFIG, range << 3)) {
        reportError(errno);
    }
    ranges[ACC_R] = accel_ranges[static_cast<size_t>(range)];
}

void MPU6050::setDlpfBandwidth(DlpfBandwidth bandwidth) {
    if (!_i2c->writeRegByte(DLPF_CONFIG, bandwidth)) {
        reportError(errno);
    }
    ranges[DLPF_R] = dlpf_ranges[static_cast<size_t>(bandwidth)];
}

double MPU6050::getAccelerationX() const {
    int16_t acc_x_h = _i2c->readRegByte(ACC_X_H);
    int16_t acc_x_l = _i2c->readRegByte(ACC_X_H + 1);
    int16_t acc_x = acc_x_l | acc_x_h << 8;
    double accel_x_converted = convertRawAccData(acc_x);
    if (calibrated) {
        return accel_x_converted - acc_offset[X];
    }
    return accel_x_converted;
}

double MPU6050::getAccelerationY() const {
    int16_t accel_y_msb = _i2c->readRegByte(ACC_Y_H);
    int16_t accel_y_lsb = _i2c->readRegByte(ACC_Y_H + 1);
    int16_t accel_y = accel_y_lsb | accel_y_msb << 8;
    double accel_y_converted = convertRawAccData(accel_y);
    if (calibrated) {
        return accel_y_converted - acc_offset[Y];
    }
    return accel_y_converted;
}

double MPU6050::getAccelerationZ() const {
    int16_t accel_z_msb = _i2c->readRegByte(ACC_Z_H);
    int16_t accel_z_lsb = _i2c->readRegByte(ACC_Z_H + 1);
    int16_t accel_z = accel_z_lsb | accel_z_msb << 8;
    double accel_z_converted = convertRawAccData(accel_z);
    if (calibrated) {
        return accel_z_converted - acc_offset[Z];
    }
    return accel_z_converted;
}

double MPU6050::getAngularVelocityX() const {
    int16_t gyro_x_msb = _i2c->readRegByte(GYRO_X_H);
    int16_t gyro_x_lsb = _i2c->readRegByte(GYRO_X_H + 1);
    int16_t gyro_x = gyro_x_lsb | gyro_x_msb << 8;
    double gyro_x_converted = convertRawGyroData(gyro_x);
    if (calibrated) {
        return gyro_x_converted - gyro_offset[X];
    }
    return gyro_x_converted;
}

double MPU6050::getAngularVelocityY() const {
    int16_t gyro_y_msb = _i2c->readRegByte(GYRO_Y_H);
    int16_t gyro_y_lsb = _i2c->readRegByte(GYRO_Y_H + 1);
    int16_t gyro_y = gyro_y_lsb | gyro_y_msb << 8;
    double gyro_y_converted = convertRawGyroData(gyro_y);
    if (calibrated) {
        return gyro_y_converted - gyro_offset[Y];
    }
    return gyro_y_converted;
}

double MPU6050::getAngularVelocityZ() const {
    int16_t gyro_z_msb = _i2c->readRegByte(GYRO_Z_H);
    int16_t gyro_z_lsb = _i2c->readRegByte(GYRO_Z_H + 1);
    int16_t gyro_z = gyro_z_lsb | gyro_z_msb << 8;
    double gyro_z_converted = convertRawGyroData(gyro_z);
    if (calibrated) {
        return gyro_z_converted - gyro_offset[Z];
    }
    return gyro_z_converted;
}

double MPU6050::convertRawGyroData(int16_t gyro_raw) const {
    const double gyro = static_cast<double>(gyro_raw) / gyro_map.at(ranges[GYR_R]);
    return gyro; // angular velocity (deg/s)
}

double MPU6050::convertRawAccData(int16_t accel_raw) const {
    const double acc = static_cast<double>(accel_raw) / accel_map.at(ranges[ACC_R]) /* * GRAVITY */;
    return acc; // (m/s^2)
}

void MPU6050::setGyroOffset(double* offset) {
    gyro_offset[X] = offset[X];
    gyro_offset[Y] = offset[Y];
    gyro_offset[Z] = offset[Z];
}

void MPU6050::setAccOffset(double* offset) {
    acc_offset[X] = offset[X];
    acc_offset[Y] = offset[Y];
    acc_offset[Z] = offset[Z];
}

void MPU6050::calibrate() {
    for (int i = 0; i <= COUNT; ++i) {
        gyro_offset[X] += getAngularVelocityX();
        gyro_offset[Y] += getAngularVelocityY();
        gyro_offset[Z] += getAngularVelocityZ();
        acc_offset[X] += getAccelerationX();
        acc_offset[Y] += getAccelerationY();
        acc_offset[Z] += getAccelerationZ();
        usleep(200);

        float percentage = static_cast<float>(i) / COUNT * 100.0;
        std::cout << "\r" << "|";
        int pos = barWidth * percentage / 100.0;
        for (int j = 0; j < barWidth; ++j) {
            if (j < pos) {
                std::cout << "#";
            } else {
                std::cout << ".";
            }
        }
        std::cout << "| " << i << "/" << COUNT << " [" << std::fixed << std::setprecision(2)
                  << percentage << "%]";
        std::flush(std::cout);
    }
    std::cout << std::endl;

    gyro_offset[X] /= COUNT;
    gyro_offset[Y] /= COUNT;
    gyro_offset[Z] /= COUNT;
    acc_offset[X] /= COUNT;
    acc_offset[Y] /= COUNT;
    acc_offset[Z] /= COUNT;
    acc_offset[Z] -= 1;
    // acc_offset[Z] -= GRAVITY;
    calibrated = true;
}

void MPU6050::printConfig() const {
    std::cout << "Accelerometer Range: +-" << ranges[ACC_R] << "g\n";
    std::cout << "Gyroscope Range: +-" << ranges[GYR_R] << " degree per sec\n";
    std::cout << "DLPF Range: " << ranges[DLPF_R] << " Hz\n";
}

void MPU6050::printOffsets() const {
    std::cout << "Accelerometer Offsets: x: " << acc_offset[X] << ", y: " << acc_offset[Y]
              << ", z: " << acc_offset[Z] << "\n";
    std::cout << "Gyroscope Offsets: x: " << gyro_offset[X] << ", y: " << gyro_offset[Y]
              << ", z: " << gyro_offset[Z] << "\n";
}

void MPU6050::cleanTerminal() const {
    std::cout << "\033[8A";
}

void MPU6050::printAcceleration() const {
    std::cout << "Acc\n";
    std::cout << "X: " << getAccelerationX() << "\n";
    std::cout << "Y: " << getAccelerationY() << "\n";
    std::cout << "Z: " << getAccelerationZ() << "\n";
}

void MPU6050::printAngularVelocity() const {
    std::cout << "Gyro\n";
    std::cout << "X: " << getAngularVelocityX() << "\n";
    std::cout << "Y: " << getAngularVelocityY() << "\n";
    std::cout << "Z: " << getAngularVelocityZ() << "\n";
}

void MPU6050::reportError(int error, std::string error_info) {
    std::cerr << "Error! " << error_info << ": " << strerror(error);
}