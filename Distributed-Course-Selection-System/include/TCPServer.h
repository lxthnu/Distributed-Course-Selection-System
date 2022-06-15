#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <iostream>
#include <vector>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <thread>
#include <algorithm>
#include <cctype>
#include <mutex>
#include "global.h"

using namespace std;

#define MAXT 1024
#define DEBUG true
#define MAXPACKETSIZE 40960
#define MAX_CLIENT 1000

//#define CODA_MSG 4

//描述socket的结构体，记录socket的端口，IP，文件描述符，message
struct descript_socket{
	void copyVal(descript_socket* t){
		socket=t->socket;
		ip      = t->ip;
		id      = t->id;
		message=t->message;
		enable_message_runtime = t->enable_message_runtime;
	}
	int port       = -1;
	int socket     = -1;
	string ip      = "";
	int id         = -1; 
	std::string message;
	bool enable_message_runtime = false;
};

class TCPServer
{
	public:
		int showSocketfd();
		int setup(int port, vector<int> opts = vector<int>());
		queue<descript_socket*> getMessage();
		void accepted();
		void Send(descript_socket* desc,string msg);
		void detach(int id);
		//void clean(int id);
		void CloseConnection(descript_socket* desc);
		bool is_online();
		string get_ip_addr(int id);
		int get_last_closed_sockets();
		void closed();

	private:
		int sockfd, n, pid; 
		struct sockaddr_in serverAddress;//服务器地址
		struct sockaddr_in clientAddress;//客户端地址
		pthread_t serverThread[ MAX_CLIENT ]; //创建多个线程来并行处理多个客户

		static vector<descript_socket*> newsockfd; //文件描述符列表
		static char msg[ MAXPACKETSIZE ];
		static queue<descript_socket*> Message;//信息列表
		
		static bool isonline; 
		static int last_closed; //
		static int num_client;  //有多少个客服连接它
		static std::mutex mt; //锁
		static std::mutex mut[10];
		static void * Task(void * argv); //任务
};

#endif
