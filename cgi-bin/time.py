#!/usr/bin/env python3

import datetime

now = datetime.datetime.now()

print("Content-Type: text/html\r")
print("\r")
print(now.strftime('%H:%M:%S'))