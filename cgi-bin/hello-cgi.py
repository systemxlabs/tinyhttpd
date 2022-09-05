#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import cgi, cgitb, os

cgitb.enable()  # 开启调试模式

print("Content-Type: text/html\n")
print("<!doctype html>"
      "<title>Hello</title>"
      "<h2>Hello CGI!</h2>"
      "<p>query string data: %s</p>" % cgi.FieldStorage())  # 打印query string

# 打印环境变量
print("<b>env variables</b><br>")
print("<ul>")
for key in os.environ.keys():
    print("<li><span style='color:green'>%30s </span> : %s </li>" % (key, os.environ[key]))
print("</ul>")