# tinyhttpd
C 语言实现简单的 HTTP 服务器。
- HTTP/1.1 协议：doc/rfc2616.pdf, [RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616)
- CGI 1.1 协议：doc/rfc3875.pdf, [RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875)
- FastCGI 协议：doc/FastCGI-Specification.pdf, [FastCGI 协议规范中文版](https://www.infoq.cn/article/vicwtitzvk7b4ynoej3e)
- TLS 1.2 协议：doc/rfc5246.pdf, [RFC 5246](https://datatracker.ietf.org/doc/html/rfc5246)

TODO 参考资料
- http://www.kipway.com/kipway_tlschannel.html
- http://www.kipway.com/kipway_openssl_usage.html

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
- [ ] 反向代理（转发http请求）
- [ ] 支持Java servlet
- [ ] 支持TLS协议
- [ ] sendfile零拷贝
- [ ] 支持文件上传下载

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
2. 通过chrome/postman/curl发送请求
    1. 发送静态请求 `curl "http://localhost:8888/index.html" --verbose`
    2. 发送cgi请求 `curl "http://localhost:8888/cgi-bin/student.py?name=Tom"` / `curl -H "Content-Type: application/json" -X POST -d '{"name":"John","age":18,"grade":"B"}' "http://localhost:8888/cgi-bin/student.py"`
3. 通过telnet测试
```
> telnel localhost 8888
Trying ::1...
telnet: connect to address ::1: Connection refused
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
GET /index.html HTTP/1.1
Host: localhost:8888
User-Agent: curl/7.77.0
Accept: */*


HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 142

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hello</title>
</head>
<body>
This is index page.
</body>
</html>Connection closed by foreign host.
```

调试TLS协议
1. `curl --cacert cert/certificate.pem "https://localhost:8888/index.html" --trace -`（`--cacert`将自签名证书作为CA根证书验证）
2. wireshark抓本地包，选择Loopback: Io
3. 二进制转16进制：`xxd file > file.hex`

证书
1. 生成自签名v3证书：`openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -extensions v3_ca -days 3650 -out certificate.pem`（需要修改/etc/ssl/openssl.conf）
2. 检查证书：`openssl x509 -text -noout -in certificate.pem`

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

CGI程序运行在独立的进程中，并对每个Web请求创建一个进程，在结束时销毁。这种“每个请求一个新进程”的模型使得CGI程序非常容易实现，但效率较差，难以扩展。
在高负载情况下，进程创建和销毁进程的开销变得很大。此外，由于地址空间无法共享，CGI进程模型限制了资源重用方法，如重用数据库连接、内存缓存等。

FastCGI 的出现是为了解决 CGI fork-and-exec 模式的低效。TODO

**2. Socket 是什么？和 Unix Domain Socket、Websocket 什么区别？**

**3. Java实现的WEB应用是如何跟httpd配合工作的？**

**4. 如何识别请求是动态请求还是静态请求？**

通过path后缀来识别请求类型，如果是静态请求，则直接返回静态资源，如果是动态请求，则调用cgi程序，并将cgi程序的输出作为返回结果。

**5. 如果整个请求是一段段发送的，如何处理？**

> 《HTTP权威指南》
> 解析请求报文时，Web 服务器会不定期地从网络上接收输入数据。网络连接可能随时都会出现延迟。
> Web 服务器需要从网络中读取数据，将部分报文数据临时存储在内存中，直到收到足以进行解析的数据并理解其意义为止。

**6. CGI协议父子进程管道通信，父进程始终是同一个，在高并发请求下，多个请求在同一个父进程内通过管道向各自子进程传输请求body时是否会混乱？**

**7. 常见 web server，如tomcat、nginx和apache httpd等，有什么区别？**

> https://www.zhihu.com/question/32212996

> 其实可以说没有什么区别，HTTP Server本质上来说都是这样几件事：
> 
> 1.监听端口接收（accept）
> 2.socket连接解析HTTP请求
> 3.使用通用或专用协议对请求进行分发
> 4.接收分发的请求产生的运行结果
> 5.将结果格式化成HTTP Response并写到socket里面
> 6.关闭连接或者Keep-Alive
> 
> 区别一方面在于用了什么语言来实现（Tomcat用Java），一方面是分发时支持的具体协议，
> Tomcat只支持Servlet接口， 
> Apache和nginx支持CGI、FastCGI、反向代理（可以认为是用HTTP协议进行HTTP请求的分发）、静态资源（可以认为是分发到磁盘读写）等，还可以用扩展模块支持其他分发方式（比如WSGI）。
> 除此以外就都是实现方式的问题了。
> 
> 我们进一步可以说一下进程内分发和进程外分发的问题，
> 有些分发方式是进程内分发的，需要通过内存传递一些对象，这种分发方式通常也是绑定语言的，所以一般必须用相应的语言实现（比如Servlet）；某些语言有特殊的FFI（如Python有C API），也可以通过FFI的方式调用。
> 另一些分发方式是进程外的，只要序列化格式匹配就可以在不同语言之间通用，如FastCGI、HTTP等，这些分发就可以用统一的方法。

**8. CGI协议为什么要fork一个子进程而不是直接在父进程直接执行脚本？**

由于CGI脚本语言不固定，无法保证脚本语言与服务器语言直接在同一进程内可以进行通信。因此采用进程间通信，只需保证消息格式一致即可。
如果直接在进程内运行CGI脚本，出现问题会导致服务器受影响。

**9. CGI脚本需要输出响应哪些内容？**

CGI脚本响应分为两类：NPH（Non-Parsed Header） 响应 和 CGI响应。
NPH响应包含完整的HTTP响应，服务器必须确保脚本输出未经修改发送到客户端。
CGI响应包括响应内容和一些响应头信息（如Content-Type），服务器接受后需要进一步处理。

**10. TLS记录协议长度占2字节，而上层握手协议长度占3字节，握手协议数据实际长度会超过2个字节会怎样？**

**11. TCP四元组（源地址、源端口、目的地址和目的端口）唯一地定义了一条连接，服务端IP地址和端口、客户端IP地址已确定，只有客户端端口未定，但端口数量存在上限，那能否提高客户端并发连接上限？**

**12. TLS握手为什么要通过pre-master-secret推出master-secret，再用master-secret推出key-block，最后用key-block进行对称加密，而不是直接生成key-block进行加密？**

**13. TLS握手为什么一定要用三个随机数，来生成"会话密钥"？**

> https://blog.csdn.net/dog250/article/details/5717162
> 
> "不管是客户端还是服务器，都需要随机数，这样生成的密钥才不会每次都一样。由于SSL协议中证书是静态的，因此十分有必要引入一种随机因素来保证协商出来的密钥的随机性。
> 对于RSA密钥交换算法来说，pre-master-key本身就是一个随机数，再加上hello消息中的随机，三个随机数通过一个密钥导出器最终导出一个对称密钥。
> pre master的存在在于SSL协议不信任每个主机都能产生完全随机的随机数，如果随机数不随机，那么pre master secret就有可能被猜出来，
> 那么仅适用pre master secret作为密钥就不合适了，因此必须引入新的随机因素，那么客户端和服务器加上pre master secret三个随机数一同生成的密钥就不容易被猜出了，
> 一个伪随机可能完全不随机，可是是三个伪随机就十分接近随机了，每增加一个自由度，随机性增加的可不是一。"


## 参考
- https://github.com/EZLippi/Tinyhttpd
- https://github.com/leezhxing/tiny-httpd
- https://github.com/reyk/httpd
- https://github.com/AngryHacker/articles/blob/master/src/code_reading/tinyhttpd.md
