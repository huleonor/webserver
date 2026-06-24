#!/bin/bash

URL="http://127.0.0.1:8002/"
FILE="test_file.txt"

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"

echo "=== Test upload ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: POST '$FILE' to upload/ — expects 200 or 201 Created"

echo "Test webserv upload" > $FILE

status=$(curl -s -o /dev/null -w "%{http_code}" -X POST -T $FILE ${URL}upload/)

if [ $status -eq 200 ] || [ $status -eq 201 ]; then
    echo -e "${GREEN}[PASS] Upload → $status${RESET}"
else
    echo -e "${RED}[FAIL] Upload → $status${RESET}"
fi

rm $FILE

echo ""