#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Gpio.hpp"

Gpio::Gpio(int32_t pin, int32_t direction, int32_t edge)
:m_pin(pin),
 m_direction(direction),
 m_edge(edge),
 m_gpioFd(0)
{
    char path[32];

    exportPin();
    setDirection(m_direction);

    //only input pin need to setEdge
    if (m_direction == GPIO_DIR_IN) {
        setEdge();
    }

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", m_pin);
    m_gpioFd = open(path, O_RDWR);
    if (m_gpioFd < 0) {
        printf("Gpio::%s: fail to open gpio value\r\n", __func__);
    }
}

Gpio::~Gpio()
{
    unexportPin();

    if (m_gpioFd > 0)
        close(m_gpioFd);
}

int32_t Gpio::setValue(int32_t value)
{
    char str[] = "01";
    if ((m_gpioFd <= 0) || (m_direction == GPIO_DIR_IN)) {
        printf("Gpio::%s: gpio fd error or input pin\r\n", __func__);
        return -1;
    }

    if (write(m_gpioFd, &str[value==0?0:1], 1) < 0) {
        printf("Gpio::%s: fail to write gpio %d value\r\n", __func__, m_pin);
        return -1;
    }

    return 0;
}

int32_t Gpio::getValue(int32_t *pValue)
{
    char value[8];
    if (m_gpioFd <= 0) {
        printf("Gpio::%s: gpio fd error\r\n", __func__);
        return -1;
    }

    memset(value, 0, sizeof(value));
    if (read(m_gpioFd, value, 3) < 0) {
        printf("Gpio::%s: fail to read gpio %d value\r\n", __func__, m_pin);
        return -1;
    }

    *pValue = atoi(value);
    return 0;
}

int32_t Gpio::exportPin()
{
    char buffer[32];
    int32_t fd, len;
    int32_t ret = 0;

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "/sys/class/gpio/gpio%d", m_pin);
    if (access(buffer, F_OK) == 0) {
        //directory exist, no need export
        return 0;
    }

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        printf("Gpio::%s: fail to open export for writing\r\n", __func__);
        return -1;
    }
    
    memset(buffer, 0, sizeof(buffer));
    len = snprintf(buffer, sizeof(buffer), "%d", m_pin);
    if (write(fd, buffer, len) < 0) {
        printf("Gpio::%s: fail to export gpio %d\r\n", __func__, m_pin);
        ret = -1;
    }
    
    close(fd);
    return ret;
}

int32_t Gpio::unexportPin()
{
    char buffer[32];
    int32_t fd, len;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        printf("Gpio::%s: fail to open unexport for writing\r\n", __func__);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    len = snprintf(buffer, sizeof(buffer), "%d", m_pin);
    if (write(fd, buffer, len) < 0) {
        printf("Gpio::%s: fail to unport gpio %d\r\n", __func__, m_pin);
        return -1;
    }

    close(fd);
    return 0;
}

int32_t Gpio::setDirection(int32_t direct)
{
    char path[64];
    int32_t fd, ret = 0;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", m_pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Gpio::%s: fail to open gpio %d direction for writing\r\n", __func__, m_pin);
        return -1;
    }

    if (direct == GPIO_DIR_OUT) {
        if (write(fd, "out", 3) < 0) {
            printf("Gpio::%s: fail to set direction for gpio %d\r\n", __func__, m_pin);
            ret = -1;
        }
    } else {
        if (write(fd, "in", 2) < 0) {
            printf("Gpio::%s: fail to set direction for gpio %d\r\n", __func__, m_pin);
            ret = -1;
        }
    }
    close(fd);
    return ret;
}

int32_t Gpio::setEdge()
{
    char edgeStr[][8] = {"none", "rising", "falling", "both"};
    char path[64];
    int32_t fd, ret;
 
    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", m_pin); 
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Gpio::%s: fail to open edge for write\r\n", __func__);
        return -1;
    }

    if (write(fd, edgeStr[m_edge], strlen(edgeStr[m_edge])) < 0) {
        printf("Gpio::%s: fail to set edge\r\n", __func__);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
