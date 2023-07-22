#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>
using namespace std;

//维护好友信息操作接口   
class FriendModel
{
public:
    //add
    void insert(int userid, int friendid);
    //select
    vector<User> query(int userid);
};


#endif