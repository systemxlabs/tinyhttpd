#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import sys, os


def get_student_from_db(name):
    return {"name": name, "age": "20", "grade": "A"}


def create_student_in_db(name, age, grade):
    return {"name": name, "age": age, "grade": grade}


query_string = os.getenv("QUERY_STRING")
method = os.getenv("REQUEST_METHOD")
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
    query_string_pairs = query_string.split("&")
    name = ""
    age = -1
    grade = "Unknown"
    for pair in query_string_pairs:
        key_value = pair.split("=")
        if key_value[0] == "name":
            name = key_value[1]
        elif key_value[0] == "age":
            age = int(key_value[1])
        elif key_value[0] == "grade":
            grade = key_value[1]
    print("HTTP/1.1 200 OK")
    print("Content-Type: text/html; charset=utf-8")
    content = "<html><body>" + "<p>Created student: %s</p>" % create_student_in_db(name, age, grade) + "</body></html>"
    print("Content-Length: %d" % len(content))
    print("\n")  # 空行
    print(content)
exit(0)


