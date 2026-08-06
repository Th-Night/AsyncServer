#pragma once
#include <jsoncpp/json/json.h>
#include "../net/CSession.hpp"

class helloworld{
public:
    void HelloWordCallBack(std::shared_ptr<CSession>&, const short&, const std::string&);
};