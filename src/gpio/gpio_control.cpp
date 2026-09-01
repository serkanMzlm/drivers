#include "gpio_control.hpp"

#include <cstring>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_MAX 10
#define VALUE_MAX 30
#define DIRECTION_MAX 35

int exportPin(int pin_number) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open export for writing");
        return -1;
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin_number);
    if (write(fd, buffer, bytes_written) == -1) {
        perror("Faild to write to export");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int directionPin(int pin_number, PinMode pin_mode) {
    char path[DIRECTION_MAX];
    static const char s_directions_str[] = "in\0out";
    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin_number);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open gpio direction for writing");
        return -1;
    }

    if (write(fd, &s_directions_str[pin_mode ? 0 : 3], 3) == -1) {
        perror("Failed to set direction");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int writePin(int pin_number, bool state) {
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin_number);
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open gpio value for writing!");
        return -1;
    }

    if (write(fd, &s_values_str[state ? 1 : 0], 1) == -1) {
        perror("Failed to write value!");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int readPin(int pin_number) {
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin_number);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open gpio value for reading!");
        return -1;
    }

    if (read(fd, value_str, 1) == -1) {
        perror("Failed to read value!");
        close(fd);
        return -1;
    }

    close(fd);
    return atoi(value_str);
}

int unexportPin(int pin_number) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open unexport for writing!");
        return -1;
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin_number);
    if (write(fd, buffer, bytes_written) == -1) {
        perror("Faild to write to unexport");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
