#include "epoll.h"
#include "log.h"

void EpollServer::Start()
{
    listen_sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock_ < 0)
    {
        LOG(ERROR, "create socket error");
        exit(1);
    }
    
    int opt_ = 1;
    setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR, &opt_, sizeof(opt_));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listen_sock_, (struct sockaddr * )&server_addr, sizeof(server_addr)) < 0)
    {
        LOG(ERROR, "bind socket error");
        exit(1);
    }
    
    if(listen(listen_sock_, 5) < 0)
    {
        LOG(ERROR, "listen socket error");
        exit(1);
    }

    epollfd_ = epoll_create(10);
    if(epollfd_ < 0)
    {
        LOG(ERROR, "epoll create error");
        exit(1);
    }
    
    if(!EventAdd(listen_sock_, EPOLLIN))
    {
        LOG(ERROR, "listen_sock add error");
        exit(1);
    }
    
    LOG(INFO, "server start");
    
}

#define EVENT_NUM 10000
void EpollServer::EventLoop()
{
    struct epoll_event events[EVENT_NUM];
    for(;;)
    {
        int event_num = epoll_wait(epollfd_, events, EVENT_NUM, -1);

        if(event_num < 0)
        {
            LOG(WARNING, "epoll wait error");    
        }
        for(int i = 0; i < event_num; ++i)
        {
            if(events[i].data.fd == listen_sock_)
            {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int client_sock = accept(listen_sock_, (struct sockaddr*)&client_addr, &len);
                if(client_sock < 0)
                {
                    LOG(WARNING, "accept error");
                }
                else
                {
                    LOG(INFO, "new accept");
                    ConnectEventHandler(client_sock);
                }
            }
            else if(events[i].events & EPOLLIN)
            {
                ReadEventHandler(&events[i].data);
            }
            else if(events[i].events & EPOLLOUT)
            {
                WriteEventHandler(&events[i].data);
            }
            else
            {
                LOG(WARNING, "event error");
            }

        }
    }
}
void EpollServer::SetNoBlock(int sfd)
{
    int flags, s;
    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1){
        LOG(ERROR, "setnoblock error");
    }

    flags |= O_NONBLOCK;
    if(fcntl(sfd, F_SETFL, flags)){
        LOG(ERROR, "setnoblock error");
    }
}












