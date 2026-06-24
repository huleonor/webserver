#!/usr/bin/env python3
import os

os.close(1)
print("Content-Type: text/plain\r")
print("\r")
print("Closing stdout early")

while True:
    pass