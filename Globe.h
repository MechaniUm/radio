#ifndef _GLOBE_H_
#define _GLOBE_H_

#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <chrono>
#include "Stepper.h"

int city_id = 0;

int globe_degree[8] = { 322, 248, 220, 77, 0, 357, 346, 348 };
int globe_pos[8] = { 10185, 6805, 5526, -1005, -4522, 11783, 11281, 11372 };
int globe_delta = 107;
int prev_stepper_pos = 0;
int offset_count = 0;
int offset_len = 200;
int globe_len = 5000;
int degree_len = 0;
int last_city_id = 0;
int globe_offset = 0;
enum GLOBE_STATE {
    ROTATING,
    CORRECTING,
    STAYING
};

GLOBE_STATE globe_state = STAYING;

void CalcGlobePos() {
    degree_len = length[2] / 360.0;
    degree_len = length[2] / 360;
    cout << "degree_len: " << degree_len << endl;
    for (int i = 0; i < 8; i++) {
        globe_pos[i] = round((length[2] * (globe_degree[i] - globe_delta) / 360));
    }
    cout << "globe_pos: ";
    for (int i = 0; i < 8; i++) {

        cout << globe_pos[i] << ' ';
    }
    globe_offset = globe_degree[0] * degree_len - globe_pos[0];

    cout << endl;
}

void globe_timer_handler (int signum)
{
    int cur_pos = steppers[0].targetPosition();
    if (prev_stepper_pos != cur_pos && globe_state != ROTATING) {
        //steppers[2].moveTo(999999);
        StopStepper(2);
        globe_state = ROTATING;
        last_city_id = city_id;
       
    } else if (prev_stepper_pos == cur_pos && globe_state == ROTATING) {
        if (steppers[2].currentPosition() != globe_pos[city_id] && abs(steppers[2].currentPosition() - globe_pos[city_id]) < length[2] / 3) 
            offset_count = 2; 
        else
            offset_count = 0;
        globe_state = CORRECTING;
        
    }
    prev_stepper_pos = cur_pos;
}


PI_THREAD (stepperRunThread2) {
    struct sigaction sa;
    struct itimerval timer;

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &globe_timer_handler;
    sigaction (SIGVTALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 200000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 200000;
    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    while (true) {
        if (!resetting) {
            RunStepper(2);
            sched_yield();
            switch (globe_state)
            {
            case ROTATING:
                break;
            case CORRECTING:
                if (!steppers[2].isRunning()) {
                    if (offset_count + 1) {
                        int a1 = globe_degree[city_id];
                        int a2 = globe_degree[last_city_id];
                        
                        int l1 = abs(a1 - a2);
                        int l2 = 360 - l1;
                        int l3 = min(l1, l2);
                        if (abs(a1 - a2) > 180) {
                            if (a1 > a2) l3 = -l3;
                        } else {
                            if (a1 < a2) l3 = -l3;
                        } 
                        int next_pos = -globe_offset + (globe_degree[last_city_id] + l3) * degree_len;
                        // cout << l3 << ' ' << next_pos << endl;
                        if (l3 == 0)
                            offset_count = 0;
                        if (offset_count % 2) {
                            next_pos += offset_count * offset_len;
                        } else {
                            next_pos -= offset_count * offset_len;
                        }
                        offset_count--;
                        if (steppers[2].currentPosition() != next_pos)
                            steppers[2].moveTo(next_pos);
                        
                        
                    } else {
                        StopStepper(2);
                        globe_state = STAYING;
                    
                    }
                }
                break;
            case STAYING:
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

void GlobeInit() {
    CalcGlobePos();
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


    piThreadCreate(stepperRunThread2);
}

#endif
