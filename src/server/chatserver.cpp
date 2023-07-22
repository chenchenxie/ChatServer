#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <string>
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg)
{
    // 注册连接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

// 上报连接信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &con)
{
    if(!con->connected())
    {
        //客服端异常关闭，则需要修改其id在mysql的状态
        ChatService::instance()->clientCloseException(con);
        con->shutdown();  //client断开连接 释放socket资源
    }
}

// 上报读写事件的回调函数
void ChatServer::onMessage(const TcpConnectionPtr & con,
                           Buffer * buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);

    //将网络服务模块与具体的业务模块解耦
    //即收到一个msgid就去调用相应的处理函数Handler
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(con, js, time); 
}