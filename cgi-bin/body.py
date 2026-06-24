#!/usr/bin/env python3
import os
import sys

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/plain\r")
print("\r")
print(f"body: {body}")