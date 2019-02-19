# xMusWebServer
实现基于Reactor模式的高性能http web服务器。

# 使用教程
依次输入命令
1.git clone git@github.com:FlyingMammuthus/xMusWebServer.git
2.cd xMusWebServer/src
3.make
4../xMus
5.浏览器输入服务器IP:端口号

# 项目简述
实现一个基于Reactor模式的http web服务器，并发模型采用epoll实现，同时为了提高响应速度，编写了相应的线程池和优先队列的数据结构，实现了高效的服务器结构

# 项目实现
并发模型主要包括：
多进程模型（资源占用多，频繁切换耗费大，最大进程数限制），多线程模型（最大线程数限制，虚拟空间地址有限）；
事件驱动模型（select，poll，epoll），采用epoll模型主要的原因是其具备以下优点：没有最大并发连接的限制，能打开的FD的上限远大于1024（1G的内存上能监听约10万个端口）；效率提升，不是轮询的方式，不会随着FD数目的增加效率下降：只有活跃可用的FD才会调用callback函数，即Epoll最大的优点就在于它只管你“活跃”的连接，而跟连接总数无关，因此在实际的网络环境中，Epoll的效率就会远远高于select和poll；内存拷贝，利用mmap()文件映射内存加速与内核空间的消息传递，即epoll使用mmap减少复制开销。

注册监听时间后，在读取事件请求时，采用异步非阻塞的方式进行，获得事件后，将其放入线程池，由多个线程处理事件。因此具体的流程是：注册“监听事件”并返回->监听到请求则建立连接，将连接描述符注册到读时间，并返回->内核感知到用户数据到达，建立任务放入线程池->等待线程池worker处理请求。

因而，本服务器采用epoll监听和注册事件，单独的线程池执行事件，由此实现了Reactor模式。

# 函数调用
放大查看
![imag](https://github.com/FlyingMammuthus/xMusWebServer/blob/master/callTree/function%20calling%20tree.jpg)

