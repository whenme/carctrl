// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <stdint.h>

#define GPIO_DIR_IN         0
#define GPIO_DIR_OUT        1

#define GPIO_EDGE_NONE      0
#define GPIO_EDGE_RISING    1
#define GPIO_EDGE_FALLING   2
#define GPIO_EDGE_BOTH      3


class Gpio
{
public:
    Gpio(int32_t pin, int32_t direction, int32_t edge);
    virtual ~Gpio();

    int32_t setValue(int32_t value);
    int32_t getValue(int32_t *pValue);
    int32_t getDirection()   { return m_direction; }
    int32_t getEdge()        { return m_edge; }
    int32_t getGpioFd()      { return m_gpioFd; }

private:
    int32_t  m_pin;
    int32_t  m_direction;
    int32_t  m_edge;
    int32_t  m_gpioFd;

    int32_t  setDirection(int32_t direct);
    int32_t  setEdge();
    int32_t  exportPin();
    int32_t  unexportPin();
};
