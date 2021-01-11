#ifndef _HALL_H_
#define _HALL_H_

#include <wiringPi.h>
#include <iostream>
#include "AccelStepper.h"
#include "Encoder.h"

const int debounce_time = 300;
int halls[7] = { 4, 17, 27, 22, 5, 10, 13 };
long long last_hall_change_time[4] = { 0, 0, 0, 0 };
bool can_inc[2] = { true, true };
bool can_dec[2] = { true, true };
void HallsInit() {
    wiringPiSetupGpio();
    for (int hall : halls) {
        pinMode(hall, INPUT);
        pullUpDnControl(hall, PUD_DOWN);
    }

}

void HallsDebugInfo() {
    for (int hall : halls) {
        std::cout << digitalRead(hall) << ' ';
    }
    std::cout << std::endl;
}

bool CanDec(int n) {
    if (can_dec[n])
        return true;
    else if (digitalRead(halls[2 * n]) && (millis() - last_hall_change_time[2 * n]) > debounce_time) {
        can_dec[n] = true;
    }
    return can_dec[n];
}

bool CanInc(int n) {
    if (can_inc[n])
        return true;
    else if (digitalRead(halls[2 * n + 1]) && (millis() - last_hall_change_time[2 * n + 1]) > debounce_time) {
        can_inc[n] = true;
    }
    return can_inc[n];
}

#endif
