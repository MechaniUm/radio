#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <wiringPi.h>
#include <cinttypes>
#include <iostream>
#include "Stepper.h"

int length[3] =  { 3066, 3066, 16440 };
uint8_t pin_a[2] = {19, 20}, pin_b[2] = {26, 21};
int32_t left_bound[2] = {0, 0}, right_bound[2] = { length[0], length[1] };
int32_t pos[2], value[2], step_length[2] = {5, 5};
uint8_t state[2];
unsigned long last_rotate_time[2];

unsigned long upd_time = 0;

void MoveStepper(int n);
bool CanDec(int n);
bool CanInc(int n);

void UpdateEncoder(int n) {
    uint8_t tmp_state = state[n] & 3;
    if (digitalRead(pin_a[n])) {
        tmp_state |= 4;
    }
    if (digitalRead(pin_b[n])) {
        tmp_state |= 8;
    }
    state[n] = tmp_state >> 2;
    if (tmp_state == 1 || tmp_state == 7 || tmp_state == 8 || tmp_state == 14) {
        if (CanInc(n)) pos[n]++;
    } else if (tmp_state == 2 || tmp_state == 4 || tmp_state == 11 || tmp_state == 13) {
        if (CanDec(n)) pos[n]--;
    } else if (tmp_state == 3 || tmp_state == 12) {
        if (CanInc(n)) pos[n] += 2;
    } else if (tmp_state == 6 || tmp_state == 9) {
        if (CanDec(n)) pos[n] -= 2;
    }
    value[n] = pos[n] / step_length[n];
    if (value[n] < left_bound[n]) {
        value[n] = left_bound[n];
        pos[n] = left_bound[n] * step_length[n];
    } else if (value[n] > right_bound[n]) {
        value[n] = right_bound[n];
        pos[n] = right_bound[n] * step_length[n];
    }
}

void UpdateEncoder0() {
    UpdateEncoder(0);
}

void UpdateEncoder1() {
    UpdateEncoder(1);
}

int32_t ReadEncoder(int n) {
    return value[n];
}

void ResetEncoder(int n, int new_value) {
    value[n] = new_value;
    pos[n] = new_value * step_length[n];
    if (digitalRead(pin_a[n])) {
        state[n] |= 1;
    }
    if (digitalRead(pin_b[n])) {
        state[n] |= 2;
    }
}

void EncoderInit(int n) {
    pinMode(pin_a[n], INPUT);
    pullUpDnControl(pin_a[n], PUD_DOWN);
    pinMode(pin_b[n], INPUT);
    pullUpDnControl(pin_b[n], PUD_DOWN);
    if (digitalRead(pin_a[n])) {
        state[n] |= 1;
    }
    if (digitalRead(pin_b[n])) {
        state[n] |= 2;
    }
}

void EncodersInit() {
    wiringPiSetupGpio();

    EncoderInit(0);
    EncoderInit(1);
    wiringPiISR(pin_a[0], INT_EDGE_BOTH, UpdateEncoder0);
    wiringPiISR(pin_b[0], INT_EDGE_BOTH, UpdateEncoder0);

    wiringPiISR(pin_a[1], INT_EDGE_BOTH, UpdateEncoder1);
    wiringPiISR(pin_b[1], INT_EDGE_BOTH, UpdateEncoder1);
}

#endif
