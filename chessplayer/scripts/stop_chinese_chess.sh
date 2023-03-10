#!/bin/bash

# EXIT: arm,calibrator,camera,detector,game,situator
ps -ef | grep arm           | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
ps -ef | grep calibrator    | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
ps -ef | grep camera        | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
ps -ef | grep detector      | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
ps -ef | grep game          | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
ps -ef | grep situator      | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
