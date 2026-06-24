#!/bin/bash

URL="http://127.0.0.1:8002"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

echo "=== Test body limit ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"

python3 -c "print('A' * 999999)" > large_body.txt

status=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: plain/text" --data-binary @large_body.txt $URL/upload/large.txt)
if [ $status -eq 413 ]; then
    echo -e "${GREEN}[PASS] Large body → $status${RESET}"
else
    echo -e "${RED}[FAIL] Large body → $status (expected 413). Confirm client_max_body_size in config file.${RESET}"
fi

status=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: plain/text" --data "short" $URL/upload/small.txt)
if [ $status -eq 200 ] || [ $status -eq 201 ]; then
    echo -e "${GREEN}[PASS] Small body → $status${RESET}"
else
    echo -e "${RED}[FAIL] Small body → $status (expected 200 or 201)${RESET}"
fi

rm large_body.txt

echo ""