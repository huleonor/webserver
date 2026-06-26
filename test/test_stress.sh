#!/bin/bash

URL1="http://127.0.0.1:8002/"
URL2="http://127.0.0.1:8003/"
DURATION="1M"

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"

echo "=== Stress Test ==="
echo -e "${YELLOW}Testing in server: $URL1${RESET}"
echo -e "${YELLOW}Testing in server: $URL2${RESET}"
echo "Duration: $DURATION"
echo "Description: Run siege benchmark on both servers simultaneously for $DURATION — expects siege to exit successfully (no crash)"

siege -b -t $DURATION -q $URL1/ &
PID1=$!
siege -b -t $DURATION -q $URL2/ &
PID2=$!

PID3=""
PID4=""


wait $PID1
if [ $? -eq 0 ]; then
	echo -e "${GREEN}[PASS] Siege URL1${RESET}"
else
    echo -e "${RED}[FAIL] Siege URL1${RESET}"
	bash manual_stress_test.sh 8002 &
	PID3=$!
fi

wait $PID2
if [ $? -eq 0 ]; then
	echo -e "${GREEN}[PASS] Siege URL2${RESET}"
else
    echo -e "${RED}[FAIL] Siege URL2${RESET}"
	bash manual_stress_test.sh 8003 &
	PID4=$!
fi

[ -n "$PID3" ] && wait $PID3
[ -n "$PID4" ] && wait $PID4
echo ""