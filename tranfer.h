#ifndef __TRANFER_H__
#define __TRANFER_H__

#include "common.h"

class TranferServer:public Server
{
public:
    TranferServer(int selfport, const char * socks5ip, int socks5port)
        :Server(selfport)
    {
        memset(&socks5addr_, 0, sizeof(struct sockaddr));
        socks5addr_.sin_family = AF_INET;
        socks5addr_.sin_port = htons(socks5port);
        socks5addr_.sin_addr.s_addr = inet_addr(socks5ip);
    }

    virtual void ConnectEventHandler(int client_sock);
    virtual void ReadEventHandler(epoll_data_t * user_data);
protected:
    struct sockaddr_in socks5addr_;
};



#endif //__TRANFER_H__
