#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <wiringPi.h>
#include "AccelStepper.h"
#include "Hall.h"

AccelStepper steppers[3] = {
    AccelStepper(18, 23, 16, true),
    AccelStepper(24, 25, 16, true),
    AccelStepper(8, 7, 16, true)
};

const int min_pulse_width = 1;
const double max_speed = 300.0;
const double acceleration = 150.0;
const int intertia_time = 100;

int globe_pos[8] = { 102, 204, 306, 408, 510, 612, 714, 816 };

const int crush_pin = 6;
bool resetted[2] = {true, true};

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
    resetted[n] = false;
    piUnlock(n);
}

void DebugInfoStepper(int n) {
    piLock(n);
    steppers[n].debugInfo();
    piUnlock(n);
}

void StopStepper(int n) {
    piLock(n);
    resetted[n] = true;
    auto cur_pos = steppers[n].currentPosition();
    steppers[n].setCurrentPosition(cur_pos);
    steppers[n].moveTo(cur_pos);
    ResetEncoder(n, cur_pos);
    piUnlock(n);
}

PI_THREAD (stepperRunThread0) {
    while (true) {
        if (labs(millis() - last_rotate_time[0]) < intertia_time) {
            RunStepper(0);
        } else if (!resetted[0]) {
            StopStepper(0);
        }
        sched_yield();
    }
    return 0;
}

PI_THREAD (stepperRunThread1) {
    while (true) {
        if (labs(millis() - last_rotate_time[1]) < intertia_time) {
            RunStepper(1);
        } else if (!resetted[1]) {
            StopStepper(1);
        }
        sched_yield();
    }
    return 0;
}

PI_THREAD (stepperRunThread2) {
    while (true) {
        steppers[2].run();
        sched_yield();
    }
    return 0;
}

void SteppersDisable() {
    digitalWrite(crush_pin, HIGH);
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
    ResetEncoder(0, 0);
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
        std::cout << "Stepper " << n << " length =  " << length[n] << std::endl;
    }
}

void SteppersInit() {
    
    pinMode(crush_pin, INPUT);
    digitalWrite(crush_pin, LOW);
    wiringPiISR(crush_pin, INT_EDGE_FALLING, SteppersDisable);

    StepperInit(0);
    StepperInit(1);
    StepperInit(2);

    piThreadCreate(stepperRunThread0);
    piThreadCreate(stepperRunThread1);
    // piThreadCreate(stepperRunThread2);


    wiringPiISR(halls[0], INT_EDGE_FALLING, h0);
    wiringPiISR(halls[1], INT_EDGE_FALLING, h1);
    wiringPiISR(halls[2], INT_EDGE_FALLING, h2);
    wiringPiISR(halls[3], INT_EDGE_FALLING, h3);
    wiringPiISR(halls[4], INT_EDGE_FALLING, h4);
}

#endif
