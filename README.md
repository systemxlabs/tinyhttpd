# tinyhttpd
C 语言实现简单的 HTTP 服务器。
- https://github.com/EZLippi/Tinyhttpd
- https://github.com/leezhxing/tiny-httpd
- https://github.com/reyk/httpd
- https://github.com/AngryHacker/articles/blob/master/src/code_reading/tinyhttpd.md

实现功能
- [x] 多线程处理
- [x] 支持静态请求
- [x] 支持CGI协议
- [x] 支持Python编写CGI脚本
- [ ] 自定义服务器配置
- [ ] 自定义404、500等错误页面
- [ ] epoll 改造
- [ ] 支持FastCGI协议
- [ ] 优雅关闭

## 项目结构
```
.
├── CMakeLists.txt
├── README.md
├── cgi-bin
│   ├── hello-cgi.py
│   └── student.py
├── client
│   └── example_client.c
├── conf
│   └── tinyhttpd.conf
├── html
│   └── index.html
├── include
│   └── tinyhttpd.h
├── src
│   ├── request.c
│   ├── response.c
│   ├── serve_cgi.c
│   ├── serve_fcgi.c
│   ├── serve_file.c
│   ├── server.c
│   └── util.c
└── test
```

- include: 项目头文件
- src: 项目源文件
- test: 项目测试文件
- conf: 项目配置文件
- html: 服务器静态资源目录
- cgi-bin: 服务器 CGI 脚本目录
- client: 客户端程序
- doc: 项目文档

## 开始运行
启动服务器
1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make`
5. `./tinyhttpd`

客户端发送请求
1. 通过http client发送请求 `./example_client`（同服务器一同构建）
2. 通过chrome/postman/curl发送请求 `curl http://localhost:8888/index.html` TODO

注意事项
1. cgi脚本需要赋予执行权限 `chmod +x xxx.py`

## 请求流程
![tinyhttpd处理流程](https://raw.githubusercontent.com/lewiszlw/tinyhttpd/master/doc/tinyhttpd%E5%A4%84%E7%90%86%E6%B5%81%E7%A8%8B.png)

### 其他
### 利用python server体验CGI脚本
1. 项目根目录下执行 `python3 -m http.server --bind localhost --cgi 8000`
2. 项目根目录下创建 cgi-bin 子目录 `mkdir cgi-bin`
3. 在 cgi-bin 目录下编写 CGI 脚本 `cd cgi-bin && touch hello-cgi.py`
4. 在浏览器打开 `http://localhost:8000/cgi-bin/hello-cgi.py`


## FAQ
**1.CGI 是什么？FastCGI 又是什么？**

早期的Web服务器，只能响应浏览器发来的HTTP静态资源的请求，并将存储在服务器中的静态资源返回给浏览器。 随着Web技术的发展，逐渐出现了动态技术，
但是Web服务器并不能够直接运行动态脚本，为了解决Web服务器与外部应用程序（CGI程序或者叫CGI脚本）之间数据互通，
于是出现了CGI（Common Gateway Interface）通用网关接口。 简单理解，可以认为CGI是Web服务器和运行其上的应用程序进行“交流”的一种约定。

CGI 是 Web 服务器和一个独立的进程之间的协议，它会把HTTP请求Request的Header头设置成进程的环境变量，HTTP请求的Body正文设置成进程的标准输入，
进程的标准输出设置为HTTP响应Response，包含Header头和Body正文。通过CGI接口，Web服务器就能够获取客户端传递的数据，
并转交给服务器端的CGI程序处理，然后返回结果给客户端。

CGI 协议采用 fork-and-exec 模式，即来一个请求，fork一个子进程来exec CGI脚本。子进程会设置环境变量并执行CGI脚本，
父进程会将请求body发送给子进程， 子进程会将CGI脚本执行结果返回给父进程。

FastCGI 的出现是为了解决 CGI fork-and-exec 模式的低效。TODO

**2. Socket 是什么？和 Unix Domain Socket、Websocket 什么区别？**

**3. Java实现的WEB应用是如何跟httpd配合工作的？**

**4. 如何识别请求是动态请求还是静态请求？**

通过path后缀来识别请求类型，如果是静态请求，则直接返回静态资源，如果是动态请求，则调用cgi程序，并将cgi程序的输出作为返回结果。

**5. 如果整个请求是一段段发送的，如何处理？**