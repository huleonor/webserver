#!/bin/bash

URL="http://127.0.0.1:8003/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"


echo "=== Test CGI infinite ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: GET cgi-bin/infinite.py (script loops forever) — expects 504 Gateway Timeout when server kills it"

status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}cgi-bin/infinite.py)
if [ $status -eq 504 ]; then
	echo -e "${GREEN}[PASS] timeout-> $status${RESET}"
else
	echo -e "${RED}[FAIL] success script -> $status${RESET}"
fi

echo ""