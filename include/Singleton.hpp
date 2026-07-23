#pragma once

#include <memory>
#include <mutex>
#include <iostream>


template <typename T>
class Singleton{

protected://这里要用protected是因为子类继承之后会调用父类的构造函数，如果私有化就调用不了了，但是我也可以友元呀
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    static std::shared_ptr<T> _instance;
    
public:
    ~Singleton(){
        std::cout << "this is singleton destruct" << std::endl;
    }

    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }

    void PrintAddress(){
        std::cout << _instance->get() << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
