#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>

#include "CarSpeed.hpp"
#include "Gpio.hpp"
#include "CarCtrl.hpp"

CarSpeed *CarSpeed::m_pInstance = NULL;

CarSpeed::CarSpeed()
{
    initApp();
    initSpeedTimer();
}

CarSpeed::~CarSpeed()
{
    if (m_gpioSpeed[MOTOR_LEFT] != NULL) {
        delete m_gpioSpeed[MOTOR_LEFT];
        m_gpioSpeed[MOTOR_LEFT] = NULL;
    }

    if (m_gpioSpeed[MOTOR_RIGHT] != NULL) {
        delete m_gpioSpeed[MOTOR_RIGHT];
        m_gpioSpeed[MOTOR_RIGHT] = NULL;
    }
}

CarSpeed *CarSpeed::getInstance()
{
    if (m_pInstance == NULL) {
        m_pInstance = new CarSpeed();
        if (m_pInstance == NULL)
            printf("CarSpeed::%s: fail to create CarCtrl\r\n", __func__);
    }

    return m_pInstance;
}

void CarSpeed::initApp()
{
    memset(m_actualSpeed, 0, sizeof(m_actualSpeed));
    memset(m_swCounter, 0, sizeof(m_swCounter));

    m_gpioSpeed[MOTOR_LEFT] = new Gpio(200, GPIO_DIR_IN, GPIO_EDGE_RISING);
    m_gpioSpeed[MOTOR_RIGHT] = new Gpio(201, GPIO_DIR_IN, GPIO_EDGE_RISING);
    if ((m_gpioSpeed[MOTOR_LEFT] == NULL) || (m_gpioSpeed[MOTOR_RIGHT] == NULL)) {
        printf("CarSpeed::%s: fail to create gpio object\r\n", __func__);
    }

    if (pthread_create(&m_tipd, NULL, CarSpeed::carSpeedThread, this) < 0) {
        printf("CarSpeed::%s: fail to create thread\r\n", __func__);
    }
}

void CarSpeed::initSpeedTimer()
{
    struct itimerval val, ovalue;

    signal(SIGALRM, CarSpeed::timerCallback);

    val.it_value.tv_sec = 1;
    val.it_value.tv_usec = 0;
    val.it_interval = val.it_value;
    setitimer(ITIMER_REAL, &val, &ovalue);
}

void CarSpeed::timerCallback(int32_t dat)
{
    static int32_t counter[MOTOR_MAX] = {0, 0};
    static int32_t speed_ctrl_time = 0;

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        CarSpeed::getInstance()->m_actualSpeed[i] = CarSpeed::getInstance()->m_swCounter[i] - counter[i];
        counter[i] = CarSpeed::getInstance()->m_swCounter[i];
        if (CarSpeed::getInstance()->m_actualSpeed[i]) {
            printf("CarSpeed::%s: motor=%d counter=%d speed=%d\r\n", __func__,
                i, counter[i], CarSpeed::getInstance()->m_actualSpeed[i]);
        }
    }
    
    speed_ctrl_time++;
    if (speed_ctrl_time > SPEED_CTRL_TIME) {
        speed_ctrl_time = 0;
        CarCtrl::getInstance()->calculateSpeedCtrl();
    }
}

int32_t CarSpeed::getActualSpeed(int32_t motor, int32_t *speed)
{
    if (motor >= MOTOR_MAX) {
        printf("CarSpeed::%s: motor %d error\r\n", __func__, motor);
        return -1;
    }

    *speed = m_actualSpeed[motor];
    return 0;
}

void *CarSpeed::carSpeedThread(void *args)
{
    CarSpeed *pSpeed = static_cast<CarSpeed *>(args);
    struct pollfd fds[MOTOR_MAX];
    char buffer[16];
    int32_t ret, i, len;

    fds[MOTOR_LEFT].fd = pSpeed->m_gpioSpeed[MOTOR_LEFT]->getGpioFd();
    fds[MOTOR_RIGHT].fd = pSpeed->m_gpioSpeed[MOTOR_RIGHT]->getGpioFd();
    fds[MOTOR_LEFT].events = POLLPRI;
    fds[MOTOR_RIGHT].events = POLLPRI;
    
    while(1)
    {
        usleep(1000);
        ret = poll(fds, MOTOR_MAX, 0);
        if (ret < 0) {
            printf("CarSpeed::%s: fail to poll\r\n", __func__);
            continue;
        }

        for (i = 0; i < MOTOR_MAX; i++) {
            if (fds[i].revents & POLLPRI) {
                if (lseek(fds[i].fd, 0, SEEK_SET) < 0) {
                    printf("CarSpeed::%s: seek failed\r\n", __func__);
                    continue;
                }
                len = read(fds[i].fd, buffer, sizeof(buffer));
                if (len < 0) {
                    printf("CarSpeed::%s: read failed\r\n", __func__);
                    continue;
                }
                pSpeed->m_swCounter[i]++;
                //printf("CarSpeed::%s: read event for motor %d counter %d\r\n", __func__, i, pSpeed->m_swCounter[i]);
            }
        }
    }
    
    return NULL;
}
