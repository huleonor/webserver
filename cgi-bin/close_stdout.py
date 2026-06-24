#!/usr/bin/env python3
import os

print("Content-Type: text/plain\r")
print("\r")
print("Closing stdout early")

os.close(1)
while True:
    pass