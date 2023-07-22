#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

/*基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环的指针
3. 明确TcpServer构造函数需要什么参数,输出ChatServer的构造函数
4. 在当前的服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务器端的线程数量（muduo会自己分配任务）
*/

class ChatServer
{
public:
    //初始化聊天服务器
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    //启动服务
    void start();

private:
    //上报连接信息的回调函数
    void onConnection(const TcpConnectionPtr &con);
    //上报读写时间的回调函数
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);

    TcpServer _server;
    EventLoop *_loop;
};

#endif