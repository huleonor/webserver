#!/bin/bash

URL="http://127.0.0.1:8003/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

INVALID="INVAL"

echo "=== Test invalid method ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: Send unknown method '$INVALID' to root — expects 400 Bad Request or 501 Not Implemented"


status=$(curl -s -o /dev/null -w ""%{http_code} -X $INVALID $URL)

if [ $status -eq 400 ] || [ $status -eq 501 ]; then
	echo -e "${GREEN}[PASS] -> $status${RESET}"
fi

echo ""
