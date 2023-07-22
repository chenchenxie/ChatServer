/*
muduo网络库给用户提供了两个主要的类
TcpSercer : 用于编写服务器程序的
TcpClient : 用于编写客户端程序的

epoll + 线程池
好处： 可以把网络I/O的代码和业务代码分开，
                    只需关注用户的连接和断开   用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

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
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件的回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        //设置服务器端的线程数量， 根据CPU核数，1个I/o线程3个worker线程
        _server.setThreadNum(4);

    }

    //开启事件循环
    void start(){
        _server.start();
    }

private:
    // 专门处理用户连接断开
    void onConnection(const TcpConnectionPtr &con)
    {  
        if(con->connected()){
            cout << con->peerAddress().toIpPort() << " ->" << con->localAddress().toIpPort() <<
                " state:online" << endl;
        }else{
            cout << con->peerAddress().toIpPort() << " ->" << con->localAddress().toIpPort() <<
                " state:offline" << endl;
            con->shutdown();  //close(fd)
            //_loop->quit();
        }
    }
    //专门处理用户读写事件的
    void onMessage(const TcpConnectionPtr &con,  
         Buffer *buf,
         Timestamp time)
    {   
        string buff = buf->retrieveAllAsString();
        cout << "recv data: " << buff << "time: " << time.toString() << endl; 
        con->send(buff);
    }

    TcpServer _server; 
    EventLoop *_loop;
};

int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();    //listenfd epoll_ctl=>epoll
    loop.loop();       //epoll_wait以阻塞方式等待新用户链接，已连接用户的读写事件等

    return 0;
}