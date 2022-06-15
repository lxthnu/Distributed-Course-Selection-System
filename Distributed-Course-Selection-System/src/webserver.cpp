#include "../include/webserver.h"
#include "../include/json.hpp"
#include <fstream>
using json = nlohmann::json;
json json_data;
WebServer::WebServer(
    int port,int trigMode,int timeoutMS,bool optLinger,int threadNum):
    port_(port),openLinger_(optLinger),timeoutMS_(timeoutMS),isClose_(false),
    timer_(new TimerManager()),threadpool_(new ThreadPool(threadNum)),epoller_(new Epoller())
{
    //获取当前工作目录的绝对路径
    srcDir_=getcwd(nullptr,256);
    assert(srcDir_);
    //拼接字符串
    strncat(srcDir_,"/resources/",16);
    HTTPconnection::userCount=0;
    HTTPconnection::srcDir=srcDir_;

    initEventMode_(trigMode);
    if(!initSocket_()) isClose_=true;
//下面是把data.json内容解析成json并且存到全局变量
	std::ifstream ifs("./static/data.json");
    json_data = json::parse(ifs);
// for(size_t i = 0; i < json_data.size(); i++){
	// std::cout <<"id="<<json_data[i]["id"]<<",name="<<json_data[i]["name"] << std::endl;

}

WebServer::~WebServer()
{
    close(listenFd_);
    isClose_=true;
    free(srcDir_);
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connectionEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connectionEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    }
    HTTPconnection::isET = (connectionEvent_ & EPOLLET);
}

void WebServer::Start(TCPServer tcp,unordered_map<string, course> cmap,unordered_map<string, string> smap,string s1addr,string s2addr,int s1port,int s2port)
{
    int timeMS=-1;//epoll wait timeout==-1就是无事件一直阻塞
    
    
    while(!isClose_)
    {
        if(timeoutMS_>0)
        {
            timeMS=timer_->getNextHandle();
        }
        //返回就绪的文件描述符个数
        int eventCnt=epoller_->wait(timeMS);
        for(int i=0;i<eventCnt;++i)
        {
            int fd=epoller_->getEventFd(i);
            uint32_t events=epoller_->getEvents(i);

            if(fd==listenFd_)
            {
                handleListen_();
               
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                //读请求
                assert(users_.count(fd) > 0);
                if(users_[fd].has_get_tcp!=1)users_[fd].settcp(tcp);
                if(users_[fd].has_get_map!=1)users_[fd].setmap(cmap,smap);
                if(users_[fd].has_get_cluster!=1)users_[fd].setcluster(s1addr,s2addr,s1port,s2port);
                handleRead_(&users_[fd],tcp);
                
            }
            else if(events & EPOLLOUT) {
                //发响应
                assert(users_.count(fd) > 0);
                // users_[fd].settcp(tcp);
                handleWrite_(&users_[fd],tcp);
            } 
            else {
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}

void WebServer::sendError_(int fd, const char* info)
{
    assert(fd>0);
    int ret=send(fd,info,strlen(info),0);
    if(ret<0)
    {
        //std::cout<<"send error to client"<<fd<<" error!"<<std::endl;
    }
    close(fd);
}

void WebServer::closeConn_(HTTPconnection* client)
{
    assert(client);
    //std::cout<<"client"<<client->getFd()<<" quit!"<<std::endl;
    epoller_->delFd(client->getFd());
    client->closeHTTPConn();
}

void WebServer::addClientConnection(int fd, sockaddr_in addr)
{
    assert(fd>0);
    users_[fd].initHTTPConn(fd,addr);
    if(timeoutMS_>0)
    {
        timer_->addTimer(fd,timeoutMS_,std::bind(&WebServer::closeConn_,this,&users_[fd]));
    }
    epoller_->addFd(fd,EPOLLIN | connectionEvent_);
    setFdNonblock(fd);
}

void WebServer::handleListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HTTPconnection::userCount >= MAX_FD) {
            sendError_(fd, "Server busy!");
            //std::cout<<"Clients is full!"<<std::endl;
            return;
        }
        addClientConnection(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::handleRead_(HTTPconnection* client,TCPServer tcp) {
    assert(client);
    extentTime_(client);//启动定时器
    //拿线程进行读取工作
    threadpool_->submit(std::bind(&WebServer::onRead_, this, client));
}

void WebServer::handleWrite_(HTTPconnection* client,TCPServer tcp)
{
    assert(client);
    extentTime_(client);
    threadpool_->submit(std::bind(&WebServer::onWrite_, this, client));
}

void WebServer::extentTime_(HTTPconnection* client)
{
    assert(client);
    if(timeoutMS_>0)
    {
        timer_->update(client->getFd(),timeoutMS_);
    }
}

void WebServer::onRead_(HTTPconnection* client) 
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->readBuffer(&readErrno);
    //std::cout<<ret<<std::endl;
    if(ret <= 0 && readErrno != EAGAIN) {
        //std::cout<<"do not read data!"<<std::endl;
        closeConn_(client);
        return;
    }
    onProcess_(client);
}

void WebServer::onProcess_(HTTPconnection* client) 
{
    if(client->handleHTTPConn()) {//进行request的解析和response的生成
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
    } 
    else {//当读缓冲区为空时，设置时间为可写入
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HTTPconnection* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeBuffer(&writeErrno);
    if(client->writeBytes() == 0) {
        /* 传输完成 */
        if(client->isKeepAlive()) {
            onProcess_(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}
bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        //std::cout<<"Port number error!"<<std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        //std::cout<<"Create socket error!"<<std::endl;
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        //std::cout<<"Init linger error!"<<std::endl;
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        //std::cout<<"set socket setsockopt error !"<<std::endl;
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        //std::cout<<"Bind Port"<<port_<<" error!"<<std::endl;
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        //printf("Listen port:%d error!\n", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->addFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        //printf("Add listen error!\n");
        close(listenFd_);
        return false;
    }
    setFdNonblock(listenFd_);
    //printf("Server port:%d\n", port_);
    return true;
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
