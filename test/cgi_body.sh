#!/bin/bash

URL="http://127.0.0.1:8002/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

BODY="test cgi post"

echo "=== Test CGI body ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"

body=$(curl -s -X POST --data $BODY ${URL}cgi-bin/body.py)
status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}cgi-bin/body.py)

echo "body request: $BODY"

if [ $status -eq 200 ]; then
	echo -e "${GREEN}[PASS] success. $status${RESET} | CGI Response: '$body'"
else
	echo -e "${RED}[FAIL] fail -> $status${RESET}"
fi

echo ""