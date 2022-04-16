# tinyhttpd
C 语言实现简单的 HTTP 服务器。
- https://github.com/EZLippi/Tinyhttpd
- https://github.com/leezhxing/tiny-httpd
- https://github.com/reyk/httpd
- https://github.com/AngryHacker/articles/blob/master/src/code_reading/tinyhttpd.md

实现功能
- [] xxx
- [] xxx
- [] epoll 改造？
- [] 支持FastCGI

## 开始

## 执行过程

## FAQ
**1.CGI是什么？FastCGI又是什么？**

早期的Web服务器，只能响应浏览器发来的HTTP静态资源的请求，并将存储在服务器中的静态资源返回给浏览器。 随着Web技术的发展，逐渐出现了动态技术，
但是Web服务器并不能够直接运行动态脚本，为了解决Web服务器与外部应用程序（CGI程序）之间数据互通，于是出现了CGI（Common Gateway Interface）通用网关接口。
简单理解，可以认为CGI是Web服务器和运行其上的应用程序进行“交流”的一种约定。

**2. Socket是什么？**

**3. Java实现的WEB应用是如何跟httpd配合工作的？**

**4. 如何识别请求是动态请求还是静态请求？**

通过path后缀来识别请求类型，如果是静态请求，则直接返回静态资源，如果是动态请求，则调用cgi程序，并将cgi程序的输出作为返回结果。