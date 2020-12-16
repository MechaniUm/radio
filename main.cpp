#include <wiringPi.h>
#include <sched.h>
#include <signal.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include "Stepper.h"
#include "Encoder.h"
#include "Hall.h"
#include "Globe.h"

using namespace std;


int sound_pos[16];

void SignalCallback(int signum) {
    DebugInfoStepper(0);
    DebugInfoStepper(1);
    DebugInfoStepper(2);
    HallsDebugInfo();
    exit(0);
}

void HardwareInit() {
    SteppersEnable();
    HallsInit();
    SteppersInit();
    GlobeInit();
    EncodersInit();
}

float GetVolume(int n, int pos, int sound_id) {
    float t1 = (float)length[n] / 8; // city size
    float t2 = t1 * 3 / 10;
    float t3 = t1 * 7 / 10;

    float x = pos - t1 * sound_id;
    if (x > t1) x = t1;
    if (x <= 0) return 0;
    if (x < t2) {
        return 100 * (1 - (t2 - x) / t2);
    } else if (x > t3) {
        return 100 * ((t1 - x) / t2);
    } else {
        return 100.0;
    }
}

void CalculateSoundPos() {
    for (int i = 0; i < 8; i++) {
        sound_pos[i] = length[0] * (i + 1) / 8 + 1;
        sound_pos[8 + i] = length[1] * (i + 1) / 8 + 1;
    }
    cout << "sound_pos: ";
    for (int i = 0; i < 16; i++) {
        cout << sound_pos[i] << ' ';
    }
    cout << endl;
}

int GetSoundId(int n, int pos) {
    for (int i = 0; i < 8; i++) {
        if (pos < sound_pos[8 * n + i]) {
            return i;
        }
    }
    cout << "sound id -1 at " << pos << endl;
    return -1;
}

int main() {
    signal(SIGINT, SignalCallback);
    string cities_sound_paths[8] = {
        "/home/pi/source/radio/wav/cities/Moscow.wav",
        "/home/pi/source/radio/wav/cities/Pekin.wav",
        "/home/pi/source/radio/wav/cities/Tokyo.wav",
        "/home/pi/source/radio/wav/cities/Washington.wav",
        "/home/pi/source/radio/wav/cities/London.wav",
        "/home/pi/source/radio/wav/cities/Paris.wav",
        "/home/pi/source/radio/wav/cities/Berlin.wav",
        "/home/pi/source/radio/wav/cities/Roma.wav"
    };

    string leaders_sound_paths[8] = {
        "/home/pi/source/radio/wav/leaders/Moscow.wav",
        "/home/pi/source/radio/wav/leaders/Pekin.wav",
        "/home/pi/source/radio/wav/leaders/Tokyo.wav",
        "/home/pi/source/radio/wav/leaders/Washington.wav",
        "/home/pi/source/radio/wav/leaders/London.wav",
        "/home/pi/source/radio/wav/leaders/Paris.wav",
        "/home/pi/source/radio/wav/leaders/Berlin.wav",
        "/home/pi/source/radio/wav/leaders/Roma.wav"
    };
    string noise_sound_path = "/home/pi/source/radio/wav/noise.wav";
    sf::Music leaders_sounds[8];
    sf::Music cities_sounds[8];
    for (int i = 0; i < 8; i++) {
        leaders_sounds[i].openFromFile(leaders_sound_paths[i]);
        leaders_sounds[i].setLoop(true);

        cities_sounds[i].openFromFile(cities_sound_paths[i]);
        cities_sounds[i].setLoop(true);
    }
    sf::Music noise_sound;
    noise_sound.openFromFile(noise_sound_path);
    noise_sound.setLoop(true);
    HardwareInit();
    noise_sound.play();
    cities_sounds[0].play();
    leaders_sounds[0].play();
    int leader_id = 0, city_cur_pos = -1, leader_cur_pos = -1;
    float noise_volume = 100.0, city_volume = 0.0, leader_volume = 0.0;
    int last_city_id = 0, last_leader_id = 0;
    CalculateSoundPos();
    cout << "Init successfull\n";
    int debounce_offset = 10;
    last_action_time = chrono::steady_clock::now();
    while (true) {
        MoveStepper(0);
        MoveStepper(1);
        city_cur_pos = StepperCurrentPosition(0);
        leader_cur_pos = StepperCurrentPosition(1);
        
        if (city_cur_pos == 0 && digitalRead(halls[0]) == HIGH) {
            ResetEncoder(0, debounce_offset);
            StepperSetPosition(0, debounce_offset);
        } else if (city_cur_pos == length[0] && digitalRead(halls[1]) == HIGH) {
            ResetEncoder(0, length[0] - debounce_offset);
            StepperSetPosition(0, length[0] - debounce_offset);
        } else if (rotate_counter[0] == 16 && !resetted[0]) {
            cout << steppers[0].currentPosition() << ' ';
            resetted[0] = true;
            ResetEncoder(0, 1490 - steppers[0].currentPosition() + steppers[0].targetPosition());
            StepperSetPosition(0, 1490);
            cout << "h5 middle reset 1490\n";
        }
        if (leader_cur_pos == 0 && digitalRead(halls[2]) == HIGH) {
            ResetEncoder(1, debounce_offset);
            StepperSetPosition(1, length[1] - debounce_offset);
        } else if (leader_cur_pos == length[1] && digitalRead(halls[3]) == HIGH) {
            ResetEncoder(1, length[1] - debounce_offset);
            StepperSetPosition(1, length[1] - debounce_offset);
        } else if (rotate_counter[1] == 16 && !resetted[1]) {
            cout << steppers[1].currentPosition() << ' ';
            resetted[1] = true;
            cout << "h6 middle reset 1490\n";
        }
        city_id = GetSoundId(0, city_cur_pos);
        leader_id = GetSoundId(1, leader_cur_pos);

        city_volume = GetVolume(0, city_cur_pos, city_id);
        leader_volume = GetVolume(1, leader_cur_pos, leader_id);
        if (city_volume >= 100 || leader_volume >= 100)
            noise_volume = 0;
        else 
            noise_volume = (100 - (city_volume + leader_volume)  / 2) / 4;
        sched_yield();
        if (last_city_id != city_id) {
            cities_sounds[last_city_id].stop();
            cities_sounds[city_id].play();
            last_city_id = city_id;
        }

        if (last_leader_id != leader_id) {
            leaders_sounds[last_leader_id].stop();
            leaders_sounds[leader_id].play();
            last_leader_id = leader_id;
        }
        cities_sounds[city_id].setVolume(city_volume);
        sched_yield();
        leaders_sounds[leader_id].setVolume(leader_volume);
        sched_yield();
        noise_sound.setVolume(noise_volume);
        sched_yield();
        chrono::steady_clock::time_point now = chrono::steady_clock::now();
        auto time_dif = std::chrono::duration_cast<std::chrono::seconds>(now - last_action_time).count();
        if (moved && (time_dif > 40 || (time_div > 20 && noise_volume == 0))) {
            SteppersReset();
            ResetEncoder(0, 0);
            ResetEncoder(1, 0);
            moved = false;
        }
    }
    return 0;
}
