#ifndef __COMMON_H__
#define __COMMON_H__


#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include "base/log.h"
#include "base/epoll.h"
#include "encry.hpp"

#define ENCRY

class Server:public EpollServer
{
public:

    Server(int port = 1080)
        :EpollServer(port)
    {}
    enum Socks5State
    {
        AUTH,               //身份认证
        ESTABLISHMENT,      //建立连接
        FORWARDING          //转发
    };

    struct Channel
    {
        int fd_;
        std::string buff_;   //写缓冲

        Channel()
            :fd_(-1)
        {}
    };
    
    struct Connect
    {
        Socks5State state_;  //连接的状态 
        Channel clientchannel_; 
        Channel serverchannel_;
        int ref_;   //连接的个数成功的连接需要两个

        Connect()
            :state_(AUTH)
            ,ref_(0)
        {}
    };
    
    //两个服务器写事件相同所以在父类实现
    virtual void WriteEventHandler(epoll_data_t * user_data);

    void Forwarding(Connect * con, Channel * transmitting_end, \
            Channel * receiving_end, bool send_encry, bool recv_decrypt);
    void RemoveConnect(int fd);
    void SendInLoop(Connect * con, int fd, const char * buf, int len);
protected:
    std::unordered_map<int, Connect *> fdconnect_map;  //fd映射的连接
};

#endif
