#!/bin/bash
g++ -std=c++11 -Wall -g -O2 -Wno-unused-variable  -o rpitxiambic rpitxiambic.cpp librpitx/src/librpitx.a -lm -lrt -lpthread
