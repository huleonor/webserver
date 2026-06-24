#!/bin/bash

URL="http://127.0.0.1:8003/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test CGI close stdout early ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: GET cgi-bin/close_stdout.py (script closes stdout before writing) — expects 500 Internal Server Error"

status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}cgi-bin/close_stdout.py)

if [ $status -eq 500 ]; then
	echo -e "${GREEN}[PASS] script failed -> $status${RESET}"
else
	echo -e "${RED}[FAIL] success script -> $status${RESET}"
fi

echo ""