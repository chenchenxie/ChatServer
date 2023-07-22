#include "json.hpp"
using nlohmann::json;

#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;

void func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "what are you doing now?";

    string sendBuf = js.dump();

    cout << sendBuf.c_str() << endl; 
}

//json序列化容器
void func2(){
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;

    string sendBuf = js.dump(); //json对象 -> 序列化 json
    cout << sendBuf << endl;
}

//反序列化
string func3(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "what are you doing now?";

    //string sendBuf = js.dump();

    //cout << sendBuf.c_str() << endl; 
    return js.dump();
}

string func4(){
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;

    return js.dump(); //json对象 -> 序列化 json 
}

int main(){

    // string revBuf = func3();
    // json jsBuf = json::parse(revBuf);

    // cout<<jsBuf["msg_type"]<<endl;
    // cout<<jsBuf["from"]<<endl;
    // cout<<jsBuf["to"]<<endl;
    // cout<<jsBuf["msg"]<<endl;

    string revBuf = func4();
    json jsBuf = json::parse(revBuf);

    cout<<jsBuf["list"]<<endl;   
    cout<<jsBuf["path"]<<endl;

    return 0;
}