#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <wiringPi.h>
#include "AccelStepper.h"
#include "Hall.h"

int rotate_counter[3];
bool resetted[3] = {false, false, false};



AccelStepper steppers[3] = {
    AccelStepper(23, 18, 16, true),
    AccelStepper(25, 24, 16, true),
    AccelStepper(7, 8, 16, true)
};

const int min_pulse_width = 3;
const double max_speed = 1200.0;
const double acceleration = 600.0;
const int intertia_time = 100;


const int crush_pin = 6;

int StepperCurrentPosition(int n) {
    piLock(n);
    int pos = steppers[n].currentPosition();
    piUnlock(n);
    return pos;
}

void MoveStepper(int n) {
    piLock(n);
    steppers[n].moveTo(value[n]);
    piUnlock(n);
}

void RunStepper(int n) {
    piLock(n);
    steppers[n].run();
    piUnlock(n);
}

void DebugInfoStepper(int n) {
    piLock(n);
    steppers[n].debugInfo();
    piUnlock(n);
}

void StopStepper(int n) {
    piLock(n);
    auto cur_pos = steppers[n].currentPosition();
    steppers[n].setCurrentPosition(cur_pos);
    steppers[n].moveTo(cur_pos);
    if (n == 0 || n == 1)
        ResetEncoder(n, cur_pos);
    piUnlock(n);
}

PI_THREAD (stepperRunThread0) {
    while (true) {
        if (!resetting)
            RunStepper(0);
        sched_yield();
    }
    return 0;
}

PI_THREAD (stepperRunThread1) {
    while (true) {
        if (!resetting)
            RunStepper(1);
        sched_yield();
    }
    return 0;
}

void SteppersEnable() {
    digitalWrite(16, LOW);
}

void SteppersDisable() {
    delay(100);
    if (digitalRead(crush_pin) == HIGH) {
       digitalWrite(16, HIGH);
       cout << "CRUSH: disable steppers" << endl;
    }
}


void h0() {
    piLock(0);
    ResetEncoder(0, 0);
    can_dec[0] = false;
    last_hall_change_time[0] = millis();
    steppers[0].setCurrentPosition(0);
    steppers[0].moveTo(0);
    piUnlock(0);
}

void h1() {
    piLock(0);
    can_inc[0] = false;
    last_hall_change_time[1] = millis();
    ResetEncoder(0, length[0]);
    steppers[0].setCurrentPosition(length[0]);
    steppers[0].moveTo(length[0]);
    piUnlock(0);
}

void h2() {
    piLock(1);
    can_dec[1] = false;
    last_hall_change_time[2] = millis();
    ResetEncoder(1, 0);
    steppers[1].setCurrentPosition(0);
    steppers[1].moveTo(0);
    piUnlock(1);
}

void h3() {
    piLock(1);
    can_inc[1] = false;
    last_hall_change_time[3] = millis();
    ResetEncoder(1, length[1]);
    steppers[1].setCurrentPosition(length[1]);
    steppers[1].moveTo(length[1]);
    piUnlock(1);
}

void h4() {
    piLock(2);
    steppers[2].setCurrentPosition(0);
    piUnlock(2);
}

const int half_rotate_steps = 400;

int tmp5;
int tmp7 = 10;
int tmp8 = 10;
void StepperSetPosition(int n, int new_pos) {
    piLock(n);
    steppers[n].setCurrentPositionOnly(new_pos);
    piUnlock(n);
}

unsigned long update_time[2];
void h5() {
    if (millis() - update_time[0] < 200) return;
    update_time[0] = millis();
    int pos = StepperCurrentPosition(0);
    int target = steppers[0].targetPosition();
    if (target < pos) {
        rotate_counter[0]--;
    } else if (target > pos) {
        rotate_counter[0]++;
    }
    std::cout << rotate_counter[0] << '\n';
    resetted[0] = false;
}

int tmp6;
void h6() {
    int pos = StepperCurrentPosition(1);
    if (tmp6 < pos) {
        rotate_counter[1]++;
    } else if (tmp6 > pos) {
        rotate_counter[1]--;
    }
    resetted[1] = false;
    tmp6 = pos;
}

void StepperInit(int n) {
    steppers[n].setMinPulseWidth(min_pulse_width);
    steppers[n].setMaxSpeed(max_speed);
    steppers[n].setAcceleration(acceleration);
    if (n == 0 || n == 1) {
        steppers[n].moveTo(99999);
        while (digitalRead(halls[2 * n + 1]) != LOW)
            steppers[n].run();
        steppers[n].setCurrentPosition(0);
        steppers[n].moveTo(0);
        steppers[n].moveTo(-99999);
        while (digitalRead(halls[2 * n]) != LOW)
            steppers[n].run();
        length[n] = -steppers[n].currentPosition();
        steppers[n].setCurrentPosition(0);
        steppers[n].moveTo(0);
    }
    std::cout << "Stepper " << n << " length =  " << length[n] << std::endl;
}

void SteppersInit() {
    
    pinMode(crush_pin, INPUT);
    pullUpDnControl(crush_pin, PUD_DOWN);
    wiringPiISR(crush_pin, INT_EDGE_RISING, SteppersDisable);


    StepperInit(0);
    StepperInit(1);
    StepperInit(2);
    piThreadCreate(stepperRunThread0);
    piThreadCreate(stepperRunThread1);
    

    wiringPiISR(halls[0], INT_EDGE_FALLING, h0);
    wiringPiISR(halls[1], INT_EDGE_FALLING, h1);
    wiringPiISR(halls[2], INT_EDGE_FALLING, h2);
    wiringPiISR(halls[3], INT_EDGE_FALLING, h3);
}

bool resetting = false;

void SteppersReset() {
    resetting = true;
    steppers[0].moveTo(99999);
    while (digitalRead(halls[1]) != LOW)
        steppers[0].run();
    steppers[0].setCurrentPosition(0);
    steppers[0].moveTo(0);
    steppers[0].moveTo(-99999);
    while (digitalRead(halls[0]) != LOW)
        steppers[0].run();
    length[0] = -steppers[0].currentPosition();
    steppers[0].setCurrentPosition(0);
    steppers[0].moveTo(0);

    steppers[1].moveTo(99999);
    while (digitalRead(halls[3]) != LOW)
        steppers[1].run();
    steppers[1].setCurrentPosition(0);
    steppers[1].moveTo(0);
    steppers[1].moveTo(-99999);
    while (digitalRead(halls[2]) != LOW)
        steppers[1].run();
    length[1] = -steppers[1].currentPosition();
    steppers[1].setCurrentPosition(0);
    steppers[1].moveTo(0);

    steppers[2].setCurrentPosition(0);
    steppers[2].moveTo(99999);
    bool was_high = false;
    int prev_globe_pos = 0;
    while (1) {
        if (digitalRead(halls[4]) == LOW) {
            if (was_high) {
                cout << steppers[2].currentPosition() - prev_globe_pos << endl;
                was_high = false;
                if (steppers[2].currentPosition() - prev_globe_pos > 900) {
                    break;
                }
                prev_globe_pos = steppers[2].currentPosition();
            }
        } else {
            was_high = true;
        }
        steppers[2].run();
    }
    length[2] = steppers[2].currentPosition();
    steppers[2].setCurrentPosition(0);
    steppers[2].moveTo(globe_pos[0]);
    while (steppers[2].isRunning()) {
        steppers[2].run();
    }
    StopStepper(2);
    resetting = false;
}

#endif
