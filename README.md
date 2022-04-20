# tinyhttpd
C 语言实现简单的 HTTP 服务器。
- https://github.com/EZLippi/Tinyhttpd
- https://github.com/leezhxing/tiny-httpd
- https://github.com/reyk/httpd
- https://github.com/AngryHacker/articles/blob/master/src/code_reading/tinyhttpd.md

实现功能
- [x] 支持静态请求
- [x] 支持CGI
- [x] 支持多线程
- [ ] server配置化
- [ ] 支持自定义404等页面
- [ ] epoll 改造
- [ ] 支持FastCGI
- [ ] 优雅关闭

## 项目结构

## 开始运行

## 执行过程

注意事项
1. cgi脚本需要赋予执行权限 `chmod +x xxx.py`

## 利用python server体验CGI脚本
1. 项目根目录下执行 `python3 -m http.server --bind localhost --cgi 8000`
2. 项目根目录下创建 cgi-bin 子目录 `mkdir cgi-bin`
3. 在 cgi-bin 目录下编写 CGI 脚本 `cd cgi-bin && touch hello-cgi.py`
4. 在浏览器打开 `http://localhost:8000/cgi-bin/hello-cgi.py`


## FAQ
**1.CGI是什么？FastCGI又是什么？**

早期的Web服务器，只能响应浏览器发来的HTTP静态资源的请求，并将存储在服务器中的静态资源返回给浏览器。 随着Web技术的发展，逐渐出现了动态技术，
但是Web服务器并不能够直接运行动态脚本，为了解决Web服务器与外部应用程序（CGI程序）之间数据互通，于是出现了CGI（Common Gateway Interface）通用网关接口。
简单理解，可以认为CGI是Web服务器和运行其上的应用程序进行“交流”的一种约定。

**2. Socket是什么？和Unix Domain Socket什么区别？**

**3. Java实现的WEB应用是如何跟httpd配合工作的？**

**4. 如何识别请求是动态请求还是静态请求？**

通过path后缀来识别请求类型，如果是静态请求，则直接返回静态资源，如果是动态请求，则调用cgi程序，并将cgi程序的输出作为返回结果。

**5. 如果整个请求是一段段发送的，如何处理？**