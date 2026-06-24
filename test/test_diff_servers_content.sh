#!/bin/bash

URL1="http://127.0.0.1:8002"
URL2="http://127.0.0.1:8003"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test different content from different servers ==="
echo -e "${YELLOW}Testing in server: $URL1${RESET}"
echo -e "${YELLOW}Testing in server: $URL2${RESET}"

BODY1=$(curl -s $URL1)
BODY2=$(curl -s $URL2)

status1=$(curl -s -o /dev/null -w "%{http_code}" $URL1)
status2=$(curl -s -o /dev/null -w "%{http_code}" $URL2)

if [ $status1 -eq 200 ] && [ $status2 -eq 200 ]; then
	    echo -e "${GREEN}[PASS] Both servers responding${RESET}"
else
    echo -e "${RED}[FAIL] One or both servers not responding${RESET}"
fi

if [ "$BODY1" != "$BODY2" ]; then
	echo -e "${GREEN}[CONTENT] different content${RESET}"
else
	echo -e "${RED}[CONTENT]equal content${RESET}"
fi

echo ""