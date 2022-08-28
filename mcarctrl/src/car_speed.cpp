// SPDX-License-Identifier: GPL-2.0
#include <poll.h>
#include <iostream>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_speed.hpp"
#include "car_ctrl.hpp"

CarSpeed::CarSpeed(asio::io_service& io_service) :
    m_ioService(io_service),
    m_timerSpeed(io_service, timerSpeedCallback, this, true)
{
    std::string filename{"car.json"};
    ParamJson param(filename);
    uint32_t left, right;

    bool ret = param.getJsonParam("car.ir_left", left);
    if (ret)
        m_gpioSpeed[MOTOR_LEFT] = new Gpio(left, GPIO_DIR_IN, GPIO_EDGE_RISING);
    ret = param.getJsonParam("car.ir_right", right);
    if (ret)
        m_gpioSpeed[MOTOR_RIGHT] = new Gpio(right, GPIO_DIR_IN, GPIO_EDGE_RISING);

    if ((m_gpioSpeed[MOTOR_LEFT] == nullptr) || (m_gpioSpeed[MOTOR_RIGHT] == nullptr))
        std::cout << "CarSpeed: fail to create gpio object" << std::endl;

    if (pthread_create(&m_tipd, nullptr, CarSpeed::carSpeedThread, this) < 0)
        std::cout << "CarSpeed: fail to create thread" << std::endl;

    m_timerSpeed.start(1000);
}

CarSpeed::~CarSpeed()
{
    m_timerSpeed.stop();

    delete m_gpioSpeed[MOTOR_LEFT];
    delete m_gpioSpeed[MOTOR_RIGHT];
}

int32_t CarSpeed::getActualSpeed(int32_t motor, int32_t *speed)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarSpeed: motor " << motor << " error" << std::endl;
        return -1;
    }

    *speed = m_actualSpeed[motor];
    return 0;
}

int32_t CarSpeed::setCtrlSteps(int32_t motor, int32_t steps)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarSpeed: error motor " << motor << std::endl;
        return -1;
    }

    m_ctrlSetSteps[motor] = steps;
    if (steps > 10)
        m_ctrlSteps[motor] = steps - 10;
    else if (steps > 5)
        m_ctrlSteps[motor] = steps - 5;
    else if ((steps >= -5) && (steps <= 5))
        m_ctrlSteps[motor] = steps;
    else if ((steps >= -10) && (steps < -5))
        m_ctrlSteps[motor] = steps + 5;
    else if (steps < -10)
        m_ctrlSteps[motor] = steps + 10;

    m_actualSteps[motor] = 0;

    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();
    carCtrl.setCarState(motor, steps);

    return 0;
}

int32_t CarSpeed::getCtrlSteps(int32_t motor)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarSpeed:error motor " << motor << std::endl;
        return 0;
    }

    return m_ctrlSetSteps[motor];
}

int32_t CarSpeed::getActualSteps(int32_t motor)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarSpeed:error motor " << motor << std::endl;
        return 0;
    }

    return (m_ctrlSetSteps[motor] > 0) ? m_actualSteps[motor] : -m_actualSteps[motor];
}

void CarSpeed::timerSpeedCallback(const asio::error_code &e, void *ctxt)
{
    CarSpeed* speedObj = static_cast<CarSpeed*>(ctxt);
    static int32_t counter[] {0, 0};

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        speedObj->m_actualSpeed[i] = speedObj->m_swCounter[i] - counter[i];
        counter[i] = speedObj->m_swCounter[i];
        /*if (speedObj->m_actualSpeed[i])
            std::cout << "CarSpeed: motor=" << i << " counter=" << counter[i]
                      << " speed=" << speedObj->m_actualSpeed[i] << std::endl;*/
    }

    cmn::getSingletonInstance<CarCtrl>().calculateSpeedCtrl();
}

void *CarSpeed::carSpeedThread(void *args)
{
    CarSpeed *pSpeed = static_cast<CarSpeed *>(args);
    struct pollfd fds[MOTOR_MAX];
    char buffer[16];
    int i, ret;

    for (i = 0; i < MOTOR_MAX; i++) {
        fds[i].fd = pSpeed->m_gpioSpeed[i]->getGpioFd();
        fds[i].events = POLLPRI;
    }

    while(1) {
        usleep(1000);
        ret = poll(fds, MOTOR_MAX, 0);
        if (ret < 0) {
            std::cout << "CarSpeed: fail to poll" << std::endl;
            continue;
        }

        for (i = 0; i < MOTOR_MAX; i++) {
            if (fds[i].revents & POLLPRI) {
                if (lseek(fds[i].fd, 0, SEEK_SET) < 0) {
                    std::cout << "CarSpeed: seek failed" << std::endl;
                    continue;
                }
                int len = read(fds[i].fd, buffer, sizeof(buffer));
                if (len < 0) {
                    std::cout << "CarSpeed: read failed" << std::endl;
                    continue;
                }
                pSpeed->m_swCounter[i]++;
                pSpeed->m_actualSteps[i]++;
                if (pSpeed->m_actualSteps[i] >= std::abs(pSpeed->m_ctrlSteps[i])) {
                    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();
                    carCtrl.setCarState(i, 0);
                }
            }
        }
    }
    
    return NULL;
}
