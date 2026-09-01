#ifndef __BMP180_TYPE_HPP__
#define __BMP180_TYPE_HPP__

#define BMP180_SLAVE_ADDR (0x77)

// Calibration Coefficients
#define BMP180_AC1_H (0xAA)
#define BMP180_AC2_H (0xAC)
#define BMP180_AC3_H (0xAE)
#define BMP180_AC4_H (0xB0)
#define BMP180_AC5_H (0xB2)
#define BMP180_AC6_H (0xB4)
#define BMP180_B1_H (0xB6)
#define BMP180_B2_H (0xB8)
#define BMP180_MB_H (0xBA)
#define BMP180_MC_H (0xBC)
#define BMP180_MD_H (0xBE)

#define BMP180_CONTROL (0xF4)

#define BMP180_TEMP_CMD (0x2E)
#define BMP180_TEMP_H (0xF6)
#define BMP180_TEMP_L (0xF7)
#define BMP180_TEMP_XL (0xF8)

#define BMP180_READ_PRESSURE_CMD 0x34
#define BMP180_PRESSUREDATA 0xF6

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