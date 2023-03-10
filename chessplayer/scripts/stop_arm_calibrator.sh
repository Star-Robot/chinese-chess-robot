#!/bin/bash

# EXIT: arm_calibrator 
ps -ef | grep arm_calibrator  | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
