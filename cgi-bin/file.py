#!/usr/bin/env python3
import sys
import os

length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length) if length > 0 else ""

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("This is the body:\n")
sys.stdout.write(body + "\n")