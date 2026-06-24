#!/bin/bash

URL="http://127.0.0.1:8002/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test Redirect ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: GET /red — expects 301 redirect; then follows redirect — expects 200 at destination"

status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}red)
redirect=$(curl -s -o /dev/null -w "%{redirect_url}" ${URL}red)

if [ $status -eq 301 ]; then
    echo -e "${GREEN}[PASS] Redirect → $status to $redirect${RESET}"
else
    echo -e "${RED}[FAIL] Redirect → $status${RESET}"
fi

status=$(curl -s -o /dev/null -w "%{http_code}" -L ${URL}red)

if [ $status -eq 200 ]; then
    echo -e "${GREEN}[PASS] Redirect followed → $status${RESET}"
else
    echo -e "${RED}[FAIL] Redirect followed → $status${RESET}"
fi

echo ""