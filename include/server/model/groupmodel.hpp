#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组
    vector<Group> queryGroups(int userid);
    //根据指定的群组id查询群组中的userid，然后给除了用户外的所有群组成员发布消息
    vector<int> queryGroupUser(int userid, int groupid);

};

#endif