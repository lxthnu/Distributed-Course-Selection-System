#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include<arpa/inet.h> //sockaddr_in
#include<sys/uio.h> //readv/writev
#include<iostream>
#include<sys/types.h>
#include<assert.h>

#include "buffer.h"
#include "HTTPrequest.h"
#include "HTTPresponse.h"
#include "TCPServer.h"

struct course{
    string name;
    int capacity,selected;
};


class HTTPconnection{
public:
    HTTPconnection();
    ~HTTPconnection();

    void initHTTPConn(int socketFd,const sockaddr_in& addr);

    //每个连接中定义的对缓冲区的读写接口
    ssize_t readBuffer(int* saveErrno);
    ssize_t writeBuffer(int* saveErrno);

    //关闭HTTP连接的接口
    void closeHTTPConn();
    //定义处理该HTTP连接的接口，主要分为request的解析和response的生成
    bool handleHTTPConn();

    //其他方法
    const char* getIP() const;
    int getPort() const;
    int getFd() const;
    sockaddr_in getAddr() const;

    int writeBytes()
    {
        return iov_[1].iov_len+iov_[0].iov_len;
    }

    bool isKeepAlive() const
    {
        return request_.isKeepAlive();
    }
    void settcp(TCPServer t){
        has_get_tcp=1;
        tcp=t;
    }
    void setmap(unordered_map<string, course> c,unordered_map<string, string> s){
        cmap=c;
        smap=s;
        has_get_map=1;
    }
    void setcluster(string s1addr,string s2addr,int s1port,int s2port){
        has_get_cluster=1;
        s1_port=s1port;
        s2_port=s2port;
        s1_addr=s1addr;
        s2_addr=s2addr;
    }

    TCPServer tcp;
    static bool isET;
    static const char* srcDir;
    static std::atomic<int>userCount;
    int has_open_tcpserver,has_get_map,has_get_tcp,has_get_cluster;
    int s1_port,s2_port;
    string s1_addr,s2_addr;

    void open_tcpserver();
    // string msg;


private:
    int fd_;                  //HTTP连接对应的描述符
    struct sockaddr_in addr_;
    bool isClose_;            //标记是否关闭连接

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuffer_;       //读缓冲区
    Buffer writeBuffer_;      //写缓冲区

    HTTPrequest request_;    
    HTTPresponse response_;

    unordered_map<string, course> cmap;
    unordered_map<string, string> smap; 

};



#endif //HTTP_CONNECTION_H