/*******************************************************************************
   ossSocket自定义套接字封装类，OSS层（对象存储服务）的ossSocket类。供数据库client和数据库引擎进行通信
   Copyright (C) 2013 SequoiaDB Software Inc.
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.
*******************************************************************************/
#ifndef OSSSOCKET_HPP__
#define OSSSOCKET_HPP__

#include "core.hpp"

#ifdef _WINDOWS
#define SOCKET_GETLASTERROR WSAGetLastError()

#define OSS_EAGAIN	WSAEINPROGRESS
#define OSS_EINTR	WSAEINTR

#else
#define SOCKET_GETLASTERROR errno
#define OSS_EAGAIN	EAGAIN
#define OSS_EINTR	EINTR
#define closesocket	close

#endif

// by default 10ms timeout
#define OSS_SOCKET_DFT_TIMEOUT 10000

// max hostname
#define OSS_MAX_HOSTNAME NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV

#ifndef _WINDOWS
typedef int		SOCKET;
#endif

class _ossSocket
{
private :
   SOCKET _fd ; //socket的文件描写叙述符
   socklen_t _addressLen ; //地址长度
   socklen_t _peerAddressLen ; //对方地址的长度
   struct sockaddr_in _sockAddress ; //本地的socket地址 ipv4
   struct sockaddr_in _peerAddress ; //对方的socket地址 ipv4
   bool _init ; //代表是否已经初始化
   int  _timeout ; //代表超时时间，包含读和写的超时
protected :
   unsigned int _getPort ( sockaddr_in *addr ) ;//得到端口
   int _getAddress ( sockaddr_in *addr, char *pAddress, unsigned int length ) ;//得到地址
public :
   int setSocketLi ( int lOnOff, int linger ) ; //设置close连接时对发送缓冲区中数据的操作
   void setAddress ( const char *pHostName, unsigned int port ) ; //设置listening socket
   _ossSocket () ;
   _ossSocket ( unsigned int port, int timeout = 0 ) ;
   
   _ossSocket ( const char *pHostname, unsigned int port, int timeout = 0 ) ;//（客户端）创建connection socket
   
   _ossSocket ( SOCKET *sock, int timeout = 0 ) ;// （服务器）通过accept的到的socket的fd来构建
   ~_ossSocket ()
   {
      close () ;
   }
   int initSocket () ;//将socket绑定到一个端口
   int bind_listen () ;//推断连接状态
   bool isConnected () ; //发送一个零字节的信息判断是否连接存在
   int setAnsyn() ;
   int send ( const char *pMsg, int len,
              int timeout = OSS_SOCKET_DFT_TIMEOUT,
              int flags = 0 ) ;
   int recv ( char *pMsg, int len,  //收到固定长度的数据才返回
              int timeout = OSS_SOCKET_DFT_TIMEOUT,
              int flags = 0 ) ;
   int recvNF ( char *pMsg, int & len, //不用收到固定长度的数据才返回
                int timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
   int connect () ;
   void close () ;
   int accept ( SOCKET *sock, struct sockaddr *addr, socklen_t *addrlen,
                int timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
   
   //当我们发送小包的时候。tcp会把几个小包组合成一个大包来发送。关闭打包发送特性，关闭tcp中的Nagle功能，小包的发送会更实时
   int disableNagle () ;
   unsigned int getPeerPort () ;
   int getPeerAddress ( char *pAddress, unsigned int length ) ;
   unsigned int getLocalPort () ;
   int getLocalAddress ( char *pAddress, unsigned int length ) ;
   //设置超时
   int setTimeout ( int seconds ) ;
   //得到域名
   static int getHostName ( char *pName, int nameLen ) ;
   //把服务名转化为端口号
   static int getPort ( const char *pServiceName, unsigned short &port ) ;
} ;

typedef class _ossSocket ossSocket ;

#endif