#!/bin/bash

URL="http://127.0.0.1:8002/"
FILE="delete_file.txt"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test Delete ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"
echo "Description: Upload '$FILE' via POST then DELETE it from upload/ — expects 200 or 204"

echo "Test webserv delete" > $FILE
curl -s -o /dev/null -X POST -T $FILE ${URL}upload/$FILE

status=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE ${URL}upload/$FILE)

if [ $status -eq 200 ] || [ $status -eq 204 ]; then
	echo -e "${GREEN}[PASS] Delete → $status${RESET}"
else
    echo -e "${RED}[FAIL] Delete → $status${RESET}"
fi

rm $FILE

echo ""