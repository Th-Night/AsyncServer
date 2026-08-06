#include "../../include/Service/helloWorldService.hpp"

void helloworld::HelloWordCallBack(std::shared_ptr<CSession>& session, const short& msg_id, const std::string& msg_data){
        Json::Reader reader;
        Json::Value root;
        if(!reader.parse(msg_data, root)){
            return;
        }

        int from = root["from"].asInt();
        int to = root["to"].asInt();
        std::string content = root["content"].asString();
        
        std::cout << "from: " << from 
                  << ", to: " << to 
                  << ", content: " << content << std::endl;
        
        Json::Value response;
        response["from"] = 0;
        response["to"] = from;
        response["content"] = "server receive: " + content;
        
        std::string return_str = response.toStyledString();
        session->Send(return_str, msg_id);

}
