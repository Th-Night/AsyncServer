cmake_minimum_required(VERSION 3.16) #先声明benchmark的cmake最低版本要求

project(AsyncServer LANGUAGES CXX) #声明项目名称和使用的语言

set(CMAKE_CXX_STANDARD 17) #设置C++标准为C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) #设置C++标准为必需

find_package(Threads REQUIRED) #查找线程库

add_executable(server
    src/main.cpp
    src/CServer.cpp
    src/CSession.cpp
    src/LogicSystem.cpp
    src/MsgNode.cpp
    src/AsioIOServicePool.cpp
) #添加要编译的源文件,server是生成的可执行文件名

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
) #设置头文件搜索路径,PRIVATE表示只对当前target可见

target_compile_definitions(server PRIVATE
    BOOST_ERROR_CODE_HEADER_ONLY
) #设置编译选项,PRIVATE表示只对当前target可见

target_link_libraries(server PRIVATE
    Threads::Threads
    jsoncpp
) #链接线程库到server target,PRIVATE表示只对当前target可见

