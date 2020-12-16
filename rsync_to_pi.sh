#!/usr/bin/bash

rsync -av6 ./* "pi@[fe80::57cc:254:5425:87a1%enxd037451368a5]":/home/pi/source/radio 
