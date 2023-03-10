#!/bin/bash

./stop_arm_calibrator.sh 2>/dev/null
./stop_arm_tester.sh 2>/dev/null
./stop_chinese_chess.sh 2>/dev/null

# EXIT: hlbcore,mode_ctrl,interactor,voice
ps -ef | grep mode_ctrl  | grep -v grep | awk '{print $2}' | xargs -i kill -SIGINT {} 2>/dev/null
ps -ef | grep interactor | grep -v grep | awk '{print $2}' | xargs -i kill -SIGINT {} 2>/dev/null
ps -ef | grep voice      | grep -v grep | awk '{print $2}' | xargs -i kill -SIGINT {} 2>/dev/null
ps -ef | grep hlbcore    | grep -v grep | awk '{print $2}' | xargs -i kill -KILL   {} 2>/dev/null
