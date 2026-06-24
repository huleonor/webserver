#!/bin/bash

URL="http://127.0.0.1:8002/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

METHOD="POST"

echo "=== Test not allowed method ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: Send $METHOD to root / (which only allows GET) — expects 405 Method Not Allowed"

status=$(curl -s -o /dev/null -w "%{http_code}" -X $METHOD $URL)
if [ $status -eq 405 ]; then
	echo -e "${GREEN}[PASS] $METHOD: Method not allowed → $status${RESET}"
else
    echo -e "${RED}[FAIL] $METHOD: Method allowed/Non exist → $status${RESET}"
fi

echo ""