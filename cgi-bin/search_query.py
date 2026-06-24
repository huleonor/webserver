#!/usr/bin/env python3
import os

query = os.environ.get('QUERY_STRING', '')

params = {}
for param in query.split('&'):
    if '=' in param:
        key, value = param.split('=', 1)
        params[key] = value
result = "not found"
name = params.get("name", "")

data = [{"name": "Carlos", "age": 25}, {"name": "Hugo", "age":20}]

for	person in data:
	if person["name"] == name:
		result = f"Person found: {person['name']}, {person['age']}"

print("Content-Type: text/plain\r")
print("\r")
print(f"Client query: {query}\r")
print(result)