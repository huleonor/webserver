#!/bin/bash

URL="http://127.0.0.1:8002"
FILE="delete_file.txt"

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
RESET="\033[0m"

echo "=== Test Wrong URL ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"

URLS=(
    "$URL/page_not_found"
    "$URL/foo/bar/baz"
    "$URL/does_not_exist.html"
    "$URL/../../../etc/passwd"
    "$URL/../../../../etc/"
)

for url in "${URLS[@]}"; do
	status=$(curl -s -o /dev/null -w "%{http_code}" "$url")
	if [ $status -eq 400 ] || [ $status -eq 404 ] || [ $status -eq 403 ]; then
		echo -e "${GREEN}[PASS] $url -> $status${RESET}"
	else
        echo -e "${RED}[FAIL] $url → $status${RESET}"
    fi
done

echo ""