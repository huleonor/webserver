#!/bin/bash

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
RESET="\033[0m"

PASS=0
FAIL=0

for script in ./*.sh; do
	if [ "$script" != "$0" ]; then
		bash "$script"
		if [ $? -eq 0 ]; then
			PASS=$((PASS + 1))
		else
			FAIL=$((FAIL + 1))
		fi
	fi
done

echo ""
echo -e "${GREEN}PASS:${RESET} $PASS"
echo -e "${RED}FAIL:${RESET} $FAIL"