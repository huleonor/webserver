#!/bin/bash

URL="http://127.0.0.1:8003/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"


echo "=== Test CGI crash ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"

status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}cgi-bin/crash.py)

if [ $status -eq 500 ]; then
	echo -e "${GREEN}[PASS] script crashed -> $status${RESET}"
else
	echo -e "${RED}[FAIL] success script -> $status${RESET}"
fi

echo ""