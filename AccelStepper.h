#ifndef _ACCELSTEPPER_H_
#define _ACCELSTEPPER_H_

#include <wiringPi.h>
#include <cmath>
#include <iostream>

using namespace std;

class AccelStepper {

public:
    AccelStepper(int pin_1, int pin_2, int pin_3, bool en) {
        wiringPiSetupGpio();
        pinMode(pin_1, OUTPUT);
        pinMode(pin_2, OUTPUT);
        pinMode(pin_3, OUTPUT);
        max_speed = 1.0;
        min_pulse_width = 3;
        pin[0] = pin_1;
        pin[1] = pin_2;
        enable_pin = pin_3;
        c_min = 1.0;

        direction = DIRECTION_CCW;

        if (en) {
            // enable();
        }
        setAcceleration(1);
    }

    void enable() {
        digitalWrite(enable_pin, LOW);
    }

    void disable() {
        digitalWrite(enable_pin, HIGH);
    }

    void setAcceleration(float a) {
        if (a == 0.0) {
            return;
        }
        if (a < 0.0) {
            a = -a;
        }
        if (acceleration != a) {
            n *= acceleration / a;
            c_0 = 0.676 * sqrt(2.0 / a) * 1000000.0;
            acceleration = a;
            computeNewSpeed();
        }
    }

    void computeNewSpeed() {
        long distance_to = distanceToGo();

        long steps_to_stop = (long)((speed * speed) / (2.0 * acceleration));

        if (distance_to == 0 && steps_to_stop <= 1) {
            step_interval = 0;
            speed = 0.0;
            n = 0;
            return;
        }

        if (distance_to > 0) {
            if (n > 0) {
                if ((steps_to_stop >= distance_to) || direction == DIRECTION_CCW)
                    n = -steps_to_stop;
            } else if (n < 0) {
                if ((steps_to_stop < distance_to) && direction == DIRECTION_CW)
                    n = -n;
            }
        } else if (distance_to < 0) {
            if (n > 0) {
                if ((steps_to_stop >= -distance_to) || direction == DIRECTION_CW)
                    n = -steps_to_stop;
            } else if (n < 0) {
                if ((steps_to_stop < -distance_to) && direction == DIRECTION_CCW)
                    n = -n;
            }
        }

        if (n == 0) {
            c_n = c_0;
            direction = distance_to > 0 ? DIRECTION_CW : DIRECTION_CCW;
        } else {
            c_n = c_n - ((2.0 * c_n) / ((4.0 * n) + 1));
            c_n = c_n > c_min ? c_n : c_min;
        }

        n++;
        step_interval = c_n;
        speed = 1000000.0 / c_n;
        if (direction == DIRECTION_CCW) {
            speed = -speed;
        }
    }

    long distanceToGo() {
        return target_pos - current_pos;
    }

    void moveTo(long absolute) {
        if (target_pos != absolute) {
            target_pos = absolute;
            computeNewSpeed();
        }
    }

    void move(long relative) {
        moveTo(current_pos + relative);
    }

    bool runSpeed() {
        if (!step_interval) {
            return false;
        }

        unsigned long time = micros();

        if (labs(time - last_step_time) >= step_interval) {
            if (direction == DIRECTION_CW) {
                current_pos++;
            } else {
                current_pos--;
            }
            doStep();
            last_step_time = time;
            return true;
        }
        return false;
    }

    void doStep() {
        setOutputPins(direction ? 0b10 : 0b00);
        delayMicroseconds(min_pulse_width);
        setOutputPins(direction ? 0b11 : 0b01);
        delayMicroseconds(min_pulse_width);
        setOutputPins(direction ? 0b10 : 0b00);
    }

    void setOutputPins(unsigned int mask) {
        digitalWrite(pin[0], (mask & (1 << 0)) ? (HIGH ^ pin_inverted[0]) : (LOW ^ pin_inverted[0]));
        delayMicroseconds(5);
        digitalWrite(pin[1], (mask & (1 << 1)) ? (HIGH ^ pin_inverted[1]) : (LOW ^ pin_inverted[1]));
    }

    long targetPosition() {
        return target_pos;
    }

    long currentPosition() {
        return current_pos;
    }

    void setCurrentPositionOnly(long pos) {
        int offset = pos - current_pos;
        current_pos = pos;
        moveTo(target_pos + offset);
    }

    void setCurrentPosition(long pos) {
        target_pos = pos;
        current_pos = pos;
        n = 0;
        step_interval = 0;
        speed = 0.0;
    }

    bool run() {
        if (runSpeed()) {
            computeNewSpeed();
        }
        return speed != 0.0 || distanceToGo() != 0;
    }

    void setMaxSpeed(float s) {
        if (s < 0.0)
            s = -s;

        if (max_speed != s) {
            max_speed = s;
            c_min = 1000000.0 / s;

            if (n > 0) {
                n = (speed * speed) / (2.0 * acceleration);
                computeNewSpeed();
            }
        }
    }

    float maxSpeed() {
        return max_speed;
    }

    void setSpeed(float s) {
        if (speed == s)
            return;

        if (s > max_speed) {
            s = max_speed;
        } else if (s < -max_speed) {
            s = -max_speed;
        }

        if (s == 0.0)
            step_interval = 0;
        else {
            step_interval = fabs(1000000.0 / s);
            direction = s > 0 ? DIRECTION_CW : DIRECTION_CCW;
        }
        speed = s;
    }

    float currentSpeed() {
        return speed;
    }

    void setMinPulseWidth(unsigned int w) {
        min_pulse_width = w;
    }

    void runToPosition() {
        while (run()) {}
    }

    bool runSpeedToPosition() {
        if (target_pos == current_pos) {
            return false;
        }

        if (target_pos > current_pos) {
            direction = DIRECTION_CW;
        } else {
            direction = DIRECTION_CCW;
        }
        return runSpeed();
    }

    void runToNewPosition(long pos) {
        moveTo(pos);
        runToPosition();
    }

    void stop() {
        if (speed != 0.0) {
            long steps_to_stop = (speed * speed) / (2.0 * acceleration) + 1;
            if (speed > 0) {
                move(steps_to_stop);
            } else {
                move(-steps_to_stop);
            }
        }
    }

    bool isRunning() {
        return !(speed == 0.0 && target_pos == current_pos);
    }

    void debugInfo() {
        cout << current_pos << ' ' << target_pos << endl;
        cout << speed << ' ' << max_speed << endl;
        cout << acceleration << endl;
        cout << c_0 << ' ' << c_n << ' ' << c_min << endl;
        cout << n << endl;
    }

private:

    typedef enum
    {
	DIRECTION_CCW = 0,  ///< Counter-Clockwise
        DIRECTION_CW  = 1   ///< Clockwise
    } Direction;

    Direction direction;
    long current_pos, target_pos;
    float speed, max_speed;
    float acceleration;
    unsigned long last_step_time, step_interval;
    unsigned int min_pulse_width;
    bool enable_inverted;
    int enable_pin, pin[2], pin_inverted[2];
    long n;
    float c_0, c_n, c_min;
};

#endif
