all:
	g++ -o radio main.cpp -O0 -lwiringPi -Wall -Werror -Wpedantic -std=c++17 -lsfml-system -lsfml-audio

gprof:
	g++ -o radio main.cpp -g -pg -O0 -lwiringPi -Wall -Werror -Wpedantic -std=c++17 -lsfml-system -lsfml-audio


clean:
	rm radio
