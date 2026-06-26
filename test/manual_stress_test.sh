#!/bin/bash

sleep 60 &
PID=$!

while kill -0 $PID 2>/dev/null; do
    curl -s -o /dev/null "http://localhost:$1/"
done

wait $PID