
#include <fcntl.h>
#include <unistd.h>
#include <spdlog/easylog.hpp>
#include <cstring>
#include <iostream>
#include "gpio.hpp"

Gpio::Gpio(int32_t pin, int32_t direction, int32_t edge) :
    m_pin(pin),
    m_direction(direction),
    m_edge(edge),
    m_gpioFd(0)
{
    char path[32] {0};

    exportPin();
    setDirection(m_direction);

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", m_pin);
    m_gpioFd = open(path, O_RDWR);
    if (m_gpioFd < 0)
        easylog::error("fail to open gpio {}", m_pin);

    if (m_direction == GPIO_DIR_IN) {
        // input pin need to setEdge
        setEdge();
    } else {
        // output set default value
        setValue(0);
    }
}

Gpio::~Gpio()
{
    setValue(0);
    unexportPin();

    if (m_gpioFd > 0)
        close(m_gpioFd);
}

int32_t Gpio::setValue(int32_t value)
{
    char str[] = "01";
    if ((m_gpioFd <= 0) || (m_direction == GPIO_DIR_IN))
        return -1;

    if (write(m_gpioFd, &str[value==0?0:1], 1) < 0) {
        easylog::warn("fail to write gpio {}", m_pin);
        return -1;
    }

    return 0;
}

int32_t Gpio::getValue(int32_t *pValue)
{
    char value[8] {0};
    if (m_gpioFd <= 0) {
        easylog::warn("gpio fd error {}", m_gpioFd);
        return -1;
    }

    if (read(m_gpioFd, value, 3) < 0) {
        easylog::warn("fail to read gpio {}", m_pin);
        return -1;
    }

    *pValue = atoi(value);
    return 0;
}

int32_t Gpio::exportPin()
{
    char buffer[32] {0};
    int32_t fd, len;
    int32_t ret = 0;

    snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d", m_pin);
    if (access(buffer, F_OK) == 0) {
        //directory exist, no need export
        return 0;
    }

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        easylog::warn("fail to open export for writing gpio {}", m_pin);
        return -1;
    }
    
    memset(buffer, 0, sizeof(buffer));
    len = snprintf(buffer, sizeof(buffer), "%d", m_pin);
    if (write(fd, buffer, len) < 0) {
        easylog::warn("fail to open export gpio {}", m_pin);
        ret = -1;
    }
    
    close(fd);
    return ret;
}

int32_t Gpio::unexportPin()
{
    char buffer[32] {0};
    int32_t fd, len;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        easylog::warn("fail to open unexport for writing for gpio {}", m_pin);
        return -1;
    }

    len = snprintf(buffer, sizeof(buffer), "%d", m_pin);
    if (write(fd, buffer, len) < 0) {
        easylog::warn("fail to unport gpio {}", m_pin);
        return -1;
    }

    close(fd);
    return 0;
}

int32_t Gpio::setDirection(int32_t direct)
{
    char path[64] {0};
    int32_t fd, ret = 0;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", m_pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        easylog::warn("fail to open gpio {} direction for writing", m_pin);
        return -1;
    }

    if (direct == GPIO_DIR_OUT) {
        if (write(fd, "out", 3) < 0) {
            easylog::warn("fail to set direction out for gpio {}", m_pin);
            ret = -1;
        }
    } else {
        if (write(fd, "in", 2) < 0) {
            easylog::warn("fail to set direction in for gpio {}", m_pin);
            ret = -1;
        }
    }
    close(fd);
    return ret;
}

int32_t Gpio::setEdge()
{
    char edgeStr[][8] = {"none", "rising", "falling", "both"};
    char path[64] {0};
 
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", m_pin); 
    int32_t fd = open(path, O_WRONLY);
    if (fd < 0) {
        easylog::warn("fail to open edge for write gpio {}", m_pin);
        return -1;
    }

    if (write(fd, edgeStr[m_edge], strlen(edgeStr[m_edge])) < 0) {
        easylog::warn("fail to set edge for gpio {}", m_pin);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
