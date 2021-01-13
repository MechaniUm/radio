#include <wiringPi.h>
#include <sched.h>
#include <signal.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <atomic>

std::atomic_bool stop_threads(false);
std::atomic_bool pause_threads(false);

#include "Stepper.h"
#include "Encoder.h"
#include "Hall.h"
#include "Globe.h"

using namespace std;

unsigned long afk_timeout = 15 * 60 * 1000; // 15 min
int sound_pos[16];

void SignalCallback(int signum) {
    DebugInfoStepper(0);
    DebugInfoStepper(1);
    DebugInfoStepper(2);
    HallsDebugInfo();
    // stop_threads = true;
    stop_threads.store(true, std::memory_order_seq_cst);
    exit(0);
}

void HardwareInit() {
    SteppersEnable();
    HallsInit();
    SteppersInit();
    GlobeInit();
    EncodersInit();
}

void HardwareReset() {
    SteppersReset();
    GlobeReset();
    EncodersReset();
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
    cout << "sound id -1 at " << pos << ' ' << n << endl;
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

    sf::SoundBuffer leaders_buffer[8];
    sf::SoundBuffer cities_buffer[8];
    sf::Sound leaders_sounds[8];
    sf::Sound cities_sounds[8];
    for (int i = 0; i < 8; i++) {

        leaders_buffer[i].loadFromFile(leaders_sound_paths[i]);
        leaders_sounds[i].setBuffer(leaders_buffer[i]);
        leaders_sounds[i].setLoop(true);

        cities_buffer[i].loadFromFile(cities_sound_paths[i]);
        cities_sounds[i].setBuffer(cities_buffer[i]);
        cities_sounds[i].setLoop(true);
    }
    sf::SoundBuffer noise_buffer;
    sf::Sound noise_sound;
    noise_buffer.loadFromFile(noise_sound_path);
    noise_sound.setBuffer(noise_buffer);
    noise_sound.setLoop(true);
    HardwareInit();
    noise_sound.play();
    cities_sounds[0].setVolume(0.0);
    leaders_sounds[0].setVolume(0.0);
    cities_sounds[0].play();
    leaders_sounds[0].play();
    int leader_id = 0, city_cur_pos = -1, leader_cur_pos = -1;
    float last_noise_volume = 24.9463, last_city_volume = 0.0, last_leader_volume = 0.0;
    float noise_volume = 24.9463, city_volume = 0.0, leader_volume = 0.0;
    int last_city_id = 0, last_leader_id = 0;
    CalculateSoundPos();
    cout << "Init successfull\n";
    bool afk = true;
    unsigned long action_time = millis();
    while (true) {

        if (!afk && labs(millis() - action_time) > afk_timeout) {
            afk = true;
            pause_threads.store(true, std::memory_order_seq_cst);

            leaders_sounds[last_leader_id].stop();
            cities_sounds[last_city_id].stop();
            noise_sound.stop();
            HardwareReset();

            last_noise_volume = 24.9463;
            noise_volume = 24.9463;

            last_city_volume = 0.0;
            city_volume = 0.0;

            last_leader_volume = 0.0;
            leader_volume = 0.0;

            city_id = 0;
            last_city_id = 0;

            leader_id = 0;
            last_leader_id = 0;

            cities_sounds[0].setVolume(0.0);
            leaders_sounds[0].setVolume(0.0);
            noise_sound.setVolume(24.9463);
            cities_sounds[0].play();
            leaders_sounds[0].play();
            noise_sound.play();

            action_time = millis();
            pause_threads.store(false, std::memory_order_seq_cst);
        }

        MoveStepper(0);
        MoveStepper(1);

        city_cur_pos = StepperCurrentPosition(0);
        leader_cur_pos = StepperCurrentPosition(1);

        city_id = GetSoundId(0, city_cur_pos);
        leader_id = GetSoundId(1, leader_cur_pos);

        city_volume = GetVolume(0, city_cur_pos, city_id);
        leader_volume = GetVolume(1, leader_cur_pos, leader_id);
        if (city_volume >= 100 || leader_volume >= 100)
            noise_volume = 0;
        else
            noise_volume = (100.0 - (city_volume + leader_volume) / 2.0) / 4.0;
        if (noise_volume < 0)
            noise_volume = 0;
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
        if (last_city_volume != city_volume) {
            last_city_volume = city_volume;
            cities_sounds[city_id].setVolume(city_volume);
        }
        if (last_leader_volume != leader_volume) {
            last_leader_volume = leader_volume;
            leaders_sounds[leader_id].setVolume(leader_volume);
        }
        if (fabs(last_noise_volume - noise_volume) > 1.0) {
            last_noise_volume = noise_volume;
            noise_sound.setVolume(noise_volume);
            action_time = millis();
            afk = false;
        }
        sched_yield();
    }
    return 0;
}
