#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "json.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &con, json& js, Timestamp)>;

class ChatService
{
public:
    //获取单例对象的接口
    static ChatService* instance();
    //获取消息ID对应的Handler
    MsgHandler getHandler(int msgid);
    //处理登陆业务
    void Login(const TcpConnectionPtr &con, json& js, Timestamp);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &con, json& js, Timestamp);
    //处理注册业务
    void Reg(const TcpConnectionPtr &con, json& js, Timestamp);
    //处理异常退出
    void clientCloseException(const TcpConnectionPtr &con);
    //一对一聊天服务
    void oneChat(const TcpConnectionPtr &con, json& js, Timestamp);
    //添加好友服务
    void addFriend(const TcpConnectionPtr &con, json& js, Timestamp);
    
    //创建群组业务
    void createGroup(const TcpConnectionPtr &con, json& js, Timestamp);
    //加入群聊业务
    void addGroup(const TcpConnectionPtr &con, json& js, Timestamp);
    //群聊
    void groupChat(const TcpConnectionPtr &con, json& js, Timestamp);


    //服务器异常，重置reset
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);
private:
    ChatService();
    //存储消息ID对应的Handler
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _conMutex;

    //数据库操作对象
    //usesr
    UserModel _userModel;
    //offlinemessage
    OfflineMsgModel _offlineMsgModel;

    FriendModel _FriendModel;

    GroupModel _groupModel;

    //redis
    Redis _redis;

};


#endif