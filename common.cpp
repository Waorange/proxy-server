#include "common.h"

void Server::WriteEventHandler(epoll_data_t * user_data)
{
    //std::cout << "WriteEventHandler"<<std::endl;
    int fd = user_data->fd;
    std::unordered_map<int, Connect*>::iterator it = fdconnect_map.find(fd);
    if(it != fdconnect_map.end())
    {
        Connect * con = it->second;
        Channel * channel = &con->clientchannel_;
        if(fd == con->serverchannel_.fd_)
        {
            channel = &con->serverchannel_;
        }

        //更改数据没有发送完重新添加问题
        std::string buff;
        buff.swap(channel->buff_);
        SendInLoop(con, fd, buff.c_str(), buff.size());
    }
}


void Server::RemoveConnect(int fd)
{
    EventDel(fd);
    std::unordered_map<int, Connect *>::iterator it = fdconnect_map.find(fd);
    if(it != fdconnect_map.end())
    {
        Connect *con = it->second;
        if((--con->ref_) == 0)
        {
            delete con;
            fdconnect_map.erase(it);
            LOG(INFO, "handler finish");
        }
    }
    
}


void Server::Forwarding(Connect * con, Channel * transmitting_end, \
        Channel * receiving_end, bool send_encry, bool recv_decrypt)
{
    char buf[4096];
    int rlen = recv(transmitting_end->fd_, buf, 4096, 0);
    if(rlen < 0)
    {
        LOG(WARNING, "recv error");
    }
    else if(rlen == 0)
    {
        
//      if(receiving_end->fd_ == con->clientchannel_.fd_)
//      {
//          std::cout << "shutdown client" <<std::endl;
//      }
//      else
//      {
//          std::cout << "shutdown socks5Server" <<std::endl;
//      }
        shutdown(receiving_end->fd_, SHUT_WR);
        RemoveConnect(transmitting_end->fd_);
    }
    else
    {
        if(send_encry)
        {
#ifdef ENCRY            
            Encry(buf, rlen);
        }
        if(recv_decrypt)
        {
            Decrypt(buf, rlen);
#endif  
        }
        buf[rlen] = '\0';
        SendInLoop(con, receiving_end->fd_, buf, rlen);
    }
}

void Server::SendInLoop(Connect * con, int fd, const char * buf, int len)
{
    
    int slen = send(fd, buf, len, 0); 
    if(slen < 0)
    {
        LOG(WARNING, "send error");
    }
    else if(slen < len)
    {
        Channel * channel = &con->clientchannel_;
        if(fd == con->serverchannel_.fd_)
        {
            channel = &con->serverchannel_;
        }

        int events = EPOLLOUT|EPOLLIN|EPOLLONESHOT;
        EventMod(fd, events);

        channel->buff_.append(buf + len);
    }
}



