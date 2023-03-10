#!/bin/bash
rm -rf logs 2>/dev/null
mkdir -p logs/global 2>/dev/null

# hlbcore,mode_ctrl,interactor,voice
./bin/global/hlbcore     >>logs/global/hlbcore.log 2>&1 &
sleep 2
./bin/global/mode_ctrl   >>logs/global/mode_ctrl.log 2>&1 &
./bin/global/interactor  >>logs/global/interactor.log 2>&1 &
./bin/global/voice       >>logs/global/voice.log 2>&1 &
