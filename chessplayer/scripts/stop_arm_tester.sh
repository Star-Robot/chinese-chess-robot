#!/bin/bash

# EXIT: arm_tester 
ps -ef | grep arm_tester  | grep -v grep | awk '{print $2}' | xargs kill -SIGINT 2>/dev/null
