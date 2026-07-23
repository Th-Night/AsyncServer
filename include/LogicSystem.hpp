#pragma once

#include "Singleton.hpp"
#include <queue>
#include <thread>
#include "CSession.hpp"
#include <map>
#include <functional>
#include "../config/config.hpp"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <memory>
// #include <iostream>

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();//这里是因为智能指针要析构，私有的话智能指针没法析构这个对象
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
    void DealMsg();

private:
    LogicSystem();
    void RegisterCallBacks();
    void HelloWordCallBack(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    std::thread _worker_thread;
    bool _b_stop;
    std::map<short, FunCallBack> _fun_callback;
};