
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "CarCtrl.hpp"
#include "CarSpeed.hpp"

CarCtrl *CarCtrl::m_pInstance = NULL;

CarCtrl::CarCtrl()
{
    memset(m_ctrlSpeed, 0, sizeof(m_ctrlSpeed));
    memset(m_currentSpeed, 0, sizeof(m_currentSpeed));
    initApp();
}

CarCtrl::~CarCtrl()
{
    pthread_cancel(m_tipd);

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        if (m_motorGpio[i][0] != NULL)
            delete m_motorGpio[i][0];
        m_motorGpio[i][0] = NULL;

        if (m_motorGpio[i][1] != NULL)
            delete m_motorGpio[i][1];
        m_motorGpio[i][1] = NULL;
    }

    if (m_pInstance != NULL) {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

CarCtrl *CarCtrl::getInstance()
{
    if (m_pInstance == NULL) {
        m_pInstance = new CarCtrl();
        if (m_pInstance == NULL)
            printf("CarCtrl::%s: fail to create CarCtrl\r\n", __func__);
    }

    return m_pInstance;
}

void CarCtrl::initApp()
{
    m_motorGpio[MOTOR_LEFT][0] = new Gpio(7, GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_motorGpio[MOTOR_LEFT][1] = new Gpio(8, GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_motorGpio[MOTOR_RIGHT][0] = new Gpio(9, GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_motorGpio[MOTOR_RIGHT][1] = new Gpio(10, GPIO_DIR_OUT, GPIO_EDGE_NONE);
    if ((m_motorGpio[MOTOR_LEFT][0] == NULL) || (m_motorGpio[MOTOR_LEFT][1] == NULL)
        || (m_motorGpio[MOTOR_RIGHT][0] == NULL) || (m_motorGpio[MOTOR_RIGHT][1] == NULL))
    {
        printf("CarCtrl::%s: fail to create motor gpio\r\n", __func__);
    } else {
        //after GPIO created
        if (pthread_create(&m_tipd, NULL, CarCtrl::carCtrlPthread, this) < 0)
            printf("CarCtrl::%s: fail to create carCtrl thread\r\n", __func__);
    }
}

int32_t CarCtrl::setCtrlSpeed(int32_t motor, int32_t speed)
{
    if ((motor >= MOTOR_MAX)
        || (speed < -MOTOR_MAX_SPEED) || (speed > MOTOR_MAX_SPEED)) {
        printf("CarCtrl::%s: error speed, speed should be -%d~+%d\r\n", __func__, MOTOR_MAX_SPEED, MOTOR_MAX_SPEED);
        return -1;
    }

    m_ctrlSpeed[motor] = speed;
    m_currentSpeed[motor] = speed;
    return 0;
}

int32_t CarCtrl::getCtrlSpeed(int32_t motor, int32_t *speed)
{
    if (motor >= MOTOR_MAX) {
        printf("CarCtrl::%s: error motor %d\r\n", __func__, motor);
        return -1;
    }
    *speed = m_ctrlSpeed[motor];
    return 0;
}

void CarCtrl::calculateSpeedCtrl()
{
    int32_t i, ret;
    int32_t speed, diffSpeed;

    for (i = 0; i < MOTOR_MAX; i++) {
        ret = CarSpeed::getInstance()->getActualSpeed(i, &speed);
        if (!ret) {
        	  diffSpeed = m_ctrlSpeed[i] - speed;
        	  if (diffSpeed >= 0) { //speed is low to ctrl
        	      if (m_ctrlSpeed[i] >= 0)
        	          m_currentSpeed[i] += abs(diffSpeed)/2;
        	      else
        	          m_currentSpeed[i] -= abs(diffSpeed)/2;
        	  } else {    //speed is high to ctrl
        	      if (m_ctrlSpeed[i] >= 0)
        	          m_currentSpeed[i] -= abs(diffSpeed)/2;
        	      else
        	          m_currentSpeed[i] += abs(diffSpeed)/2;
        	  }
        }
    }
}

void *CarCtrl::carCtrlPthread(void *args)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(args);
    int32_t runState[] = {0, 0};
    int32_t runTime[] = {0, 0};

    while(1)
    {
        usleep(500);  //1ms

        for (int32_t i = 0; i < MOTOR_MAX; i++) {
             if (runState[i] == 0) {//stop
                 pCtrl->setCarState(i, 0);
                 runTime[i]++;
                 if (runTime[i] > (MOTOR_MAX_TIME - abs(pCtrl->m_currentSpeed[i]*MOTOR_SPEED_STEP))) {
                     runTime[i] = 0;
                     runState[i] = 1;
                 }
             } else {  //run
                 if (pCtrl->m_ctrlSpeed[i] > 0)
                     pCtrl->setCarState(i, 1);
                 else if (pCtrl->m_ctrlSpeed[i] < 0)
                     pCtrl->setCarState(i, -1);

                 runTime[i]++;
                 if (runTime[i] > abs(pCtrl->m_currentSpeed[i]*MOTOR_SPEED_STEP)) {
                     runTime[i] = 0;
                     runState[i] = 0;
                 }
             }
        }
    }

    return NULL;
}

int32_t CarCtrl::setCarState(int32_t motor, int32_t state)
{
    static int32_t direct[] = {0, 0};
    if (state > 0) {
        if (direct[motor] != 1) {
            m_motorGpio[motor][0]->setValue(0);
            m_motorGpio[motor][1]->setValue(1);
            direct[motor] = 1;
        }
    } else if (state == 0) {
        if (direct[motor] != 0) {
            m_motorGpio[motor][0]->setValue(0);
            m_motorGpio[motor][1]->setValue(0);
            direct[motor] = 0;
        }
    } else {
        if (direct[motor] != -1) {
            m_motorGpio[motor][0]->setValue(1);
            m_motorGpio[motor][1]->setValue(0);
            direct[motor] = -1;
        }
    }

    return 0;
}


