#include "../../include/messaging/LogicSystem.hpp"

LogicSystem::LogicSystem() : _b_stop(false){
    RegisterCallBacks();
    _worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

void LogicSystem::RegisterCallBacks(){

    _fun_callback[MSG_HELLO_WORLD] = [service = std::make_shared<helloworld>()](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data){
        service->HelloWordCallBack(session, msg_id, msg_data);
    };
    
}

void LogicSystem::DealMsg(){
    for(;;){
        std::unique_lock<std::mutex> unique_lk(_mutex);

        //判断队列为空则用条件变量等待、
        while(_msg_que.empty() && !_b_stop){
            _consume.wait(unique_lk);
        }

        //判断如果为关闭状态，取出逻辑队列所有数据即使处理并退出循环
        if(_b_stop){
            while(!_msg_que.empty()){
                  auto msg_node = _msg_que.front();
                  std::cout << "recv msg id is " << msg_node->_recvnode->_msg_id << std::endl;
                  auto call_back_iter = _fun_callback.find(msg_node->_recvnode->_msg_id);
                  if(call_back_iter == _fun_callback.end()){
                    _msg_que.pop();
                    continue;
                  }
                  call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                    std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
                    _msg_que.pop();
            }
            break;
        }
        //如果没有停服，并且队列中有数据
        auto msg_node = _msg_que.front();
        std::cout << "recv msg id is " << msg_node->_recvnode->_msg_id << std::endl;

        auto call_back_iter = _fun_callback.find(msg_node->_recvnode->_msg_id);
        if(call_back_iter == _fun_callback.end()){
            _msg_que.pop();
            continue;
        }
        call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
        std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
        _msg_que.pop();
    }
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg){
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(msg);

    if(_msg_que.size() == 1){
        _consume.notify_one();
    }
}

LogicSystem::~LogicSystem(){

    _b_stop = true;
    _consume.notify_one();

    if(_worker_thread.joinable()) {
        _worker_thread.join();
    }
}