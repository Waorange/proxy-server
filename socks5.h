#ifndef __SOCKS5_H__
#define __SOCKS5_H__

#include <iostream>
#include "common.h"

class Socks5Server:public Server
{
public:
    Socks5Server(int port = 8001)
        :Server(port)
    {}
    //身份验证
    int AuthHandler(int fd);
    
    //连接管理
    int EstablishmentHandler(int fd);

    virtual void ConnectEventHandler(int client_sock);
    virtual void ReadEventHandler(epoll_data_t * user_data);
protected:
};

#endif
