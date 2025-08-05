// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <xapi/easylog.hpp>
#include "steer.hpp"

Steer::Steer(asio::io_context& context, std::vector<uint32_t> port) :
    m_steerTimer(context, [&](const asio::error_code &e, void *ctxt) {
        m_outputGpio[0]->setValue(0);
        m_outputGpio[1]->setValue(0);
    }, nullptr, false)
{
    m_outputGpio[0] = new Gpio(port.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_outputGpio[1] = new Gpio(port.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    if ((m_outputGpio[0] == nullptr) || (m_outputGpio[1] == nullptr)) {
        ctrllog::warn("fail to create motor from output gpio {},{}", port.at(0), port.at(1));
    }
}

Steer::~Steer()
{
    m_steerTimer.stop();

    delete m_outputGpio[0];
    delete m_outputGpio[1];
}

void Steer::turn(int32_t time)
{
    ctrllog::warn("set turn time {}", time);
    if (time > 0) { //turn left
        m_outputGpio[0]->setValue(0);
        m_outputGpio[1]->setValue(1);
        m_steerTimer.start(time*1000);
    } else if (time == 0) {  //stop
        m_outputGpio[0]->setValue(0);
        m_outputGpio[1]->setValue(0);
        m_steerTimer.stop();
    } else { //turn right
        m_outputGpio[0]->setValue(1);
        m_outputGpio[1]->setValue(0);
        m_steerTimer.start(-time*1000);
    }
}
