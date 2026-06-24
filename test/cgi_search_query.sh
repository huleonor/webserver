#!/bin/bash

URL="http://127.0.0.1:8003/"

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"


echo "=== Test CGI search query ==="
echo -e "${YELLOW}Testing in server: $URL${RESET}"

QUERY="name=dog"
echo "Description: GET cgi-bin/search_query.py?$QUERY — expects 200 and query params parsed in response"
BODY=$(curl -s ${URL}cgi-bin/search_query.py?$QUERY)
status=$(curl -s -o /dev/null -w "%{http_code}" ${URL}cgi-bin/search_query.py?$QUERY)
if [ $status -eq 200 ]; then
	echo -e "${GREEN}[PASS] success script-> $status${RESET}"
	echo -e "$BODY"
else
	echo -e "${RED}[FAIL] failed script -> $status${RESET}"
fi

echo ""