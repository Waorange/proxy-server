#include "socks5.h"
#include "base/log.h"
#include <netdb.h>
#include <signal.h>
#include <string>

void Socks5Server::ConnectEventHandler(int client_sock)
{
    //std::cout << "ConnectEventHandler" <<std::endl;

    //添加connectfd到epoll
    SetNoBlock(client_sock);
    EventAdd(client_sock, EPOLLIN);

    Connect * con = new Connect; 
    con->state_ = AUTH;
    con->clientchannel_.fd_ = client_sock;
    fdconnect_map[client_sock] = con;

    ++con->ref_; 
}
void Socks5Server::ReadEventHandler(epoll_data_t * user_data)
{
    //std::cout << "ReadEventHandler" <<std::endl;
    int fd = user_data->fd;

    std::unordered_map<int, Connect*>::iterator it = fdconnect_map.find(fd);
    if(it != fdconnect_map.end())
    {
        char reply[2];
        reply[0] = 0x05;
        Connect * con = it->second;
        if(con->state_ == AUTH)
        {
            int ret = AuthHandler(fd);
            if (ret == 0)
            {
                return;
            }
            else if(ret == 1)
            {
                reply[1] = 0x00;
                con->state_ = ESTABLISHMENT;
                LOG(INFO, "auth success");
            }
            else if(ret == -1)
            {
                reply[1] = 0xFF;
                RemoveConnect(fd);
                LOG(INFO, "auth failure");
            }
#ifdef ENCRY 
            Encry(reply, 2);
#endif

            if(send(fd , reply, 2, 0) != 2)
            {
                LOG(WARNING, "auth reply error");
            }
        }
        else if(con->state_ == ESTABLISHMENT)
        {
            char reply[10] = { 0 };
            reply[0] = 0x05;

            int server_fd = EstablishmentHandler(fd);
            if(server_fd < 0)
            {
                //连接失败
                RemoveConnect(fd);
                reply[1] = 0x01;
            }
            else if(server_fd == 0)
            {
                //信息不够解析
                return;
            }
            else
            {
                //成功建立连接
                reply[1] = 0x00;
                reply[3] = 0x01;
            }
            
#ifdef ENCRY 
            Encry(reply, 10);
#endif
            if(send(fd, reply, 10, 0) != 10)
            {
                LOG(WARNING, "Establishment reply error");
            }

            if(server_fd > 0)
            {
                SetNoBlock(server_fd);
                EventAdd(server_fd, EPOLLIN);

                con->serverchannel_.fd_ = server_fd;
                fdconnect_map[server_fd] = con;
                ++con->ref_;
                con->state_ =  FORWARDING;

                LOG(INFO, "EstablishmentHandler finish");
            }
        }
        else if(con->state_ == FORWARDING)
        {
            //默认发送数据的客户端
            Channel * transmitting_end = &con->clientchannel_;  //发送端
            Channel * receiving_end = &con->serverchannel_;     //接收端
            
            //当发送端时client时接收端为自己(Socks5Server)
            //发送时不需要加密，接收时需要解密
            bool send_encry = false, recv_decrypt = true;

            //转发两边的数据 如果服务器是发送端进行交换
            if(fd == con->serverchannel_.fd_)
            {
                std::swap(transmitting_end, receiving_end);

                //当发送端时server时接受端为TranferServer 
                //发送时不需要加密, 接收时需要解密
                std::swap(send_encry, recv_decrypt);
            }
            
            Forwarding(con, transmitting_end, receiving_end, send_encry, recv_decrypt);
        }
    }
    else
    {
        LOG(WARNING, "not find client socket");
    }

}


// 返回0数据不够  -1 表示失败 
int Socks5Server::AuthHandler(int fd)
{
    char buf[257];
    int rlen = recv(fd, buf, 257, MSG_PEEK);

    if (rlen <= 0)
    {
        return -1;
    }
    else if(rlen < 3)
    {
        return 0;
    }
    else
    {
        recv(fd, buf, rlen, 0);
#ifdef ENCRY 
        Decrypt(buf, rlen);
#endif

        if(buf[0] != 0x05)
        {
            LOG(INFO, "no socks5");
        }
    }
    return 1;
}

// 返回0数据不够  -1 表示失败 
int Socks5Server::EstablishmentHandler(int fd)
{
    char buf[256];
    int rlen = recv(fd, buf, 256, MSG_PEEK);
    if(rlen <= 0)
    {
        return -1;
    }
    else if(rlen < 10)
    {
        return 0;
    }
    else
    {
        recv(fd, buf, 4, 0);

        char ip[4];
        char port[2];
#ifdef ENCRY 
        Decrypt(buf, 4);
#endif
        char address_type = buf[3];
        if(address_type == 0x01)  //ipv4
        {
            //读ip和端口
            recv(fd, ip, 4, 0);
#ifdef ENCRY 
            Decrypt(ip, 4);
#endif
            recv(fd, port, 2, 0);
#ifdef ENCRY 
            Decrypt(port, 2);
#endif

        }
        else if(address_type == 0x03) //域名
        {
            char len = 0;

            //读域名
            recv(fd, &len, 1, 0);
#ifdef ENCRY 
            Decrypt(&len, 1);
#endif

            recv(fd, buf, len, 0);
#ifdef ENCRY 
            Decrypt(buf, len);
#endif
            
            //读取端口
            recv(fd, port, 2, 0); 
#ifdef ENCRY 
            Decrypt(port, 2);
#endif

            buf[len] = '\0';
            LOG(INFO, buf);

            struct hostent * hostptr = NULL;
            hostptr = gethostbyname(buf);
            if(hostptr == NULL)
            {
                return -1;
            }

            memcpy(ip, hostptr->h_addr, hostptr->h_length);

        }
        else if(address_type == 0x04)
        {
            LOG(WARNING, "no support ipv6");
        }
        else
        {
            LOG(WARNING, "invalid address type");
            return -1;
        }

        //连接客户端需要请求的服务器

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr.s_addr, ip, 4);
        addr.sin_port = *((uint16_t *)port);

        int connect_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(connect_fd < 0)
        {
            LOG(WARNING, "connect socket create error");
            return -1;
        }
        if(connect(connect_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            close(connect_fd);
            LOG(WARNING, "connect failure");
            return -1;
        }

        return connect_fd;
    }
}


int main()
{
    signal(SIGPIPE, SIG_IGN);
    Socks5Server server(8001);
    server.Start();
    server.EventLoop();
    return 0;
}
