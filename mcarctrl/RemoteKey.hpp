
#ifndef __REMOTE_KEY_HPP__
#define __REMOTE_KEY_HPP__

#include <pthread.h>
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
#define RC_KEY_STAR       0x16
#define RC_KEY_POUND      0x0d
#define RC_KEY_LEFT       0x08
#define RC_KEY_RIGHT      0x5a
#define RC_KEY_UP         0x18
#define RC_KEY_DOWN       0x52
#define RC_KEY_OK         0x1c

#define RC_KEY_FILE       "/dev/input/event1"

class RemoteKey
{
public:
    static RemoteKey *getInstance();
    void   handleKeyPress();

private:
    RemoteKey();
    virtual ~RemoteKey();
    int32_t getKeyEvent(int32_t *pEvent);
    static void *remoteKeyThread(void *argc);

private:
    static RemoteKey *m_pInstance;
    int32_t m_keyfd;
    pthread_t m_tipd;
    std::list<int32_t> m_keyList;
};

#endif
