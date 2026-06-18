#!/bin/bash

TOTAL=500
HALF=$((TOTAL / 2))
URL_8002="http://localhost:8002/cgi-bin/test_crash.py"
URL_8003="http://localhost:8003/cgi-bin/test_crash.py"
ROUNDS=${1:-5}

for round in $(seq 1 $ROUNDS); do
    echo "Round $round/$ROUNDS — sending $HALF requests to each port..."
    for i in $(seq 1 $HALF); do
        curl -s -o /dev/null "$URL_8002" &
        curl -s -o /dev/null "$URL_8003" &
    done
    wait
    echo "Round $round done."
done

echo "All rounds complete."
