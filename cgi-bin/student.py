#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import cgi, cgitb
import json
import sys, os


def get_student_from_db(name):
    return {"name": name, "age": "20", "grade": "A"}


def create_student_in_db(name, age, grade):
    return {"name": name, "age": age, "grade": grade}


query_string = os.getenv("QUERY_STRING")
method = os.getenv("REQUEST_METHOD")
content_type = os.getenv("CONTENT_TYPE")
body = input()

if method == "GET":
    query_string_pairs = query_string.split("&")
    for pair in query_string_pairs:
        key_value = pair.split("=")
        if key_value[0] == "name":
            name = key_value[1]
            print("HTTP/1.1 200 OK", end="\r\n")
            print("Content-Type: text/html; charset=utf-8", end="\r\n")
            content = "<html><body>" + "<p>Got the student: %s</p>" % get_student_from_db(name) + "</body></html>"
            print("Content-Length: %d" % len(content), end="\r\n")
            print("", end="\r\n")  # 空行
            print(content, end="")

elif method == "POST":
    name = ""
    age = -1
    grade = "Unknown"
    if "application/json" not in content_type:
        body_dict = json.loads(body)
        name = body_dict["name"]
        age = body_dict["age"]
        grade = body_dict["grade"]

    print("HTTP/1.1 200 OK", end="\r\n")
    print("Content-Type: text/html; charset=utf-8", end="\r\n")
    content = "<html><body>" + "<p>Created student: %s</p>" % create_student_in_db(name, age, grade) + "</body></html>"
    print("Content-Length: %d" % len(content), end="\r\n")
    print("", end="\r\n")  # 空行
    print(content, end="")
exit(0)


