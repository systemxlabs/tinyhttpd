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
            print("200 OK HTTP/1.1")
            print("Content-Type: text/html; charset=utf-8\n")
            print("<html><body>")
            print("<p>Got the student: %s</p>" % get_student_from_db(name))
            print("</body></html>")
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
    print("200 OK HTTP/1.1")
    print("Content-Type: text/html; charset=utf-8\n")
    print("<html><body>")
    print("<p>Created student: %s</p>" % create_student_in_db(name, age, grade))
    print("</body></html>")
exit(0)


