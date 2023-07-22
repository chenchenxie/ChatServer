#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

// 获取单例对象的接口
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 构造函数中注册消息以及对应的Handler回调函数
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::Login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::Reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取消息ID对应的Handler
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &con, json &js, Timestamp)
        {
            LOG_ERROR << "msgid" << msgid << "can not find its Handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登陆业务
void ChatService::Login(const TcpConnectionPtr &con, json &js, Timestamp)
{
    LOG_INFO << "do ligin service!";
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已登录不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account is using, input another!";
            con->send(response.dump());
        }
        else
        {
            // 登录成功记录用户连接信息
            {
                lock_guard<mutex> lock(_conMutex);
                _userConnMap.insert({id, con});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登陆成功,更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询是否有离线消息，如果有就加载
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读后删除
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> friendvec = _FriendModel.query(id);
            if (!friendvec.empty())
            {
                vector<string> vecstring;
                for (User &user : friendvec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vecstring.push_back(js.dump());
                }
                response["friends"] = vecstring;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            con->send(response.dump());
        }
    }
    else
    {
        // 登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        con->send(response.dump());
    }
}
// 处理注册业务 name password state
void ChatService::Reg(const TcpConnectionPtr &con, json &js, Timestamp)
{
    LOG_INFO << "do reg service!";
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        con->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        con->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_conMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &con)
{
    User user;
    {
        lock_guard<mutex> lock(_conMutex);

        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            // 从在线连接表中删除对应的连接
            if (it->second == con)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    // 更新用户状态
    if (user.getId() != -1)
    {

        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天服务
void ChatService::oneChat(const TcpConnectionPtr &con, json &js, Timestamp)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_conMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线
            it->second->send(js.dump());
            return;
        }
    }

    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
    }
    // toid不在线，存储离线消息
    _offlineMsgModel.inset(toid, js.dump());
}

// 服务器异常，重置reset
void ChatService::reset()
{
    _userModel.restState();
}

// 添加好友服务msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &con, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _FriendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &con, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群聊业务
void ChatService::addGroup(const TcpConnectionPtr &con, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群聊
void ChatService::groupChat(const TcpConnectionPtr &con, json &js, Timestamp)
{
    int userid = js["id"];
    int groupid = js["groupid"];

    vector<int> useridVec = _groupModel.queryGroupUser(userid, groupid);

    lock_guard<mutex> lock(_conMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.inset(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_conMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.inset(userid, msg);
}