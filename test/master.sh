#!/bin/bash

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

PASS=0
FAIL=0
WEBSERV_PID=$(pgrep webserv)

for script in ./*.sh; do
	if [ "$script" != "$0" ]; then
		bash "$script"
		if ! kill -0 $WEBSERV_PID 2>/dev/null; then
            echo -e "${RED}[ERROR] webserv crashed after $script/or server is not running${RESET}"
            exit 1
        fi
	fi
done

echo ""