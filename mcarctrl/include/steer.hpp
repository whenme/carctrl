// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <vector>
#include <xapi/iotimer.hpp>
#include "gpio.hpp"

class Steer
{
public:
    Steer(asio::io_context& context, std::vector<uint32_t> port);
    virtual ~Steer();

    // time >0 turn left, <0 turn right, =0 stop. 
    void turn(int32_t time = 0);

private:
    Gpio*   m_outputGpio[2] {nullptr, nullptr};
    IoTimer m_steerTimer;
};
