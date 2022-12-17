// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <ioapi/iotimer.hpp>
#include <list>

#define RC_KEY_0          0x19
#define RC_KEY_1          0x45
#define RC_KEY_2          0x46
#define RC_KEY_3          0x47
#define RC_KEY_4          0x44
#define RC_KEY_5          0x40
#define RC_KEY_6          0x43
#define RC_KEY_7          0x07
#define RC_KEY_8          0x15
#define RC_KEY_9          0x09
#define RC_KEY_STAR       0x16  //stop
#define RC_KEY_POUND      0x0d  //step/speed switch
#define RC_KEY_LEFT       0x08  //left wheel steps
#define RC_KEY_RIGHT      0x5a  //right wheel steps
#define RC_KEY_UP         0x18  //forward
#define RC_KEY_DOWN       0x52  //back
#define RC_KEY_OK         0x1c  //speed set

class RemoteKey
{
public:
    RemoteKey(asio::io_service& io_service);
    virtual ~RemoteKey();
    void   handleKeyPress();

private:
    int32_t initIrKey();
    int32_t getKeyEvent(int32_t *pEvent);
    static void timerCallback(const asio::error_code &e, void *ctxt);

    int32_t m_keyfd;
    IoTimer m_timer;
    std::list<int32_t> m_keyList;
};
