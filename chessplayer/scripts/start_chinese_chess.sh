#!/bin/bash
mkdir -p logs/chinese_chess 2>/dev/null

# arm,calibrator,camera,detector,game,situator
./bin/chinese_chess/arm         >> logs/chinese_chess/arm.log &
./bin/chinese_chess/calibrator  >> logs/chinese_chess/calibrator.log &
./bin/chinese_chess/camera      >> logs/chinese_chess/camera.log &
./bin/chinese_chess/detector    >> logs/chinese_chess/detector.log &
./bin/chinese_chess/situator    >> logs/chinese_chess/situator.log &
./bin/chinese_chess/game        >> logs/chinese_chess/game.log &
