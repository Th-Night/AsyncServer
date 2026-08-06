#pragma once

#include <memory>
#include <mutex>
#include <iostream>


template <typename T>
class Singleton{

protected://这里要用protected是因为子类继承之后会调用父类的构造函数，如果私有化就调用不了了，但是我也可以友元呀
    Singleton() = default;

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    
    
public:
    ~Singleton(){
        std::cout << "this is singleton destruct" << std::endl;
    }

    static std::shared_ptr<T> GetInstance(){
        static const std::shared_ptr<T> _instance(new T);
        return _instance;
    }

    void PrintAddress(){
        std::cout << GetInstance().get() << std::endl;
    }
};

