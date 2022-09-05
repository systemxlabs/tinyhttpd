#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import cgi, cgitb
import json
import sys, os


def get_student_from_db(name):
    return {"name": name, "age": "20", "grade": "A"}


def create_student_in_db(name, age, grade):
    return {"name": name, "age": age, "grade": grade}


# 从环境变量读取http请求头部信息
query_string = os.getenv("QUERY_STRING")
method = os.getenv("REQUEST_METHOD")
content_type = os.getenv("CONTENT_TYPE")

if method == "GET":
    query_string_pairs = query_string.split("&")
    for pair in query_string_pairs:
        key_value = pair.split("=")
        if key_value[0] == "name":
            name = key_value[1]
            # 将响应内容写入到标准输出流中
            print("HTTP/1.1 200 OK", end="\r\n")
            print("Content-Type: text/html; charset=utf-8", end="\r\n")
            content = "<html><body>" + \
                      "<p>Got the student: %s</p>" % get_student_from_db(name) + \
                      "</body></html>"
            print("Content-Length: %d" % len(bytes(content, encoding="utf-8")), end="\r\n")
            print("", end="\r\n")  # 空行
            print(content, end="")

elif method == "POST":
    # 从标准输入流中读取请求body
    # TODO 读取 Content-Length长度字节内容
    body = input()
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
    # len统计str类型的结果是字符数（一个汉字为一个字符），非字节数（一个汉字utf-8编码是3个字节）
    print("Content-Length: %d" % len(bytes(content, encoding="utf-8")), end="\r\n")
    print("", end="\r\n")  # 空行
    print(content, end="")
exit(0)


