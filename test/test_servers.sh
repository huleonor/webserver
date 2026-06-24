#!/bin/bash

URL1="http://127.0.0.1:8002/"
URL2="http://127.0.0.1:8003/"

GREEN="\033[32m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test different servers ==="

status1=$(curl -s -o /dev/null -w "%{http_code}" $URL1)
status2=$(curl -s -o /dev/null -w "%{http_code}" $URL2)

if [ $status1 -eq 200 ]; then
    echo -e "${GREEN}[PASS] Server 1 ($URL1) → $status1${RESET}"
else
    echo -e "${RED}[FAIL] Server 1 ($URL1) → $status1${RESET}"
fi

if [ $status2 -eq 200 ]; then
    echo -e "${GREEN}[PASS] Server 2 ($URL2) → $status2${RESET}"
else
    echo -e "${RED}[FAIL] Server 2 ($URL2) → $status2${RESET}"
fi