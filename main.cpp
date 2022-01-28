
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "CarCtrl.hpp"
#include "RemoteKey.hpp"
#include "CarSpeed.hpp"

int32_t main(int argc, char **argv)
{
    CarCtrl::getInstance();
    RemoteKey::getInstance();
    CarSpeed::getInstance();

    while(1) {
        system("echo 1 > /sys/class/leds/orangepi:red:status/brightness");
		sleep(1);
		system("echo 0 > /sys/class/leds/orangepi:red:status/brightness");
		sleep(1);
	}
}
