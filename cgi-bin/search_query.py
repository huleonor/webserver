#!/usr/bin/env python3
import os

query = os.environ.get('QUERY_STRING', '')

params = {}
for param in query.split('&'):
    if '=' in param:
        key, value = param.split('=', 1)
        params[key] = value
result = "not found"
animal = params.get("animal", "")

data = [{"animal": "dog", "age": 3}, {"animal": "cat", "age":2}]

for	item in data:
	if item["animal"] == animal:
		result = f"Animal found: {animal}, {item['age']}"

print("Content-Type: text/plain\r")
print("\r")
print(f"Client query: {query}\r")
print(result)