#!/bin/bash

TOTAL=500
HALF=$((TOTAL / 2))
URL_8002="http://localhost:8002/cgi-bin/file.py"
ROUNDS=${1:-1000}

for round in $(seq 1 $ROUNDS); do
    echo "Round $round/$ROUNDS — sending $HALF requests to each port... and url $URL_8002"
        curl -s -o /dev/null "$URL_8002" &
    wait
    echo "Round $round done."
done

echo "All rounds complete."
