#include "../include/TCPClient.h"
#include <errno.h>
TCPClient::TCPClient()
{
	sock = -1;
	port = 0;
	address = "";
}

bool TCPClient::setup(string address , int port)
{	
	cout << "--------------------------------------------------------socket " << sock << endl;
  	if(sock == -1)
	{
		
		sock = socket(AF_INET , SOCK_STREAM , 0);
		if (sock == -1)
		{
      			cout << "Could not create socket" << endl;
		}
    }
	// cout << "step2 " <<endl;
  	if((signed)inet_addr(address.c_str()) == -1)
  	{
		//   cout << "step3 " <<endl;
    	struct hostent *he;
    	struct in_addr **addr_list;
		if ( (he = gethostbyname( address.c_str() ) ) == NULL)
    	{
		    //   herror("gethostbyname");
      		      cout<<"Failed to resolve hostname\n";
		    return false;
    	}
	   	addr_list = (struct in_addr **) he->h_addr_list;
    	for(int i = 0; addr_list[i] != NULL; i++)
    	{
      		server.sin_addr = *addr_list[i];
		    break;
    	}
  	}
  	else
  	{
		//   cout << "step4 " <<endl;
    	server.sin_addr.s_addr = inet_addr( address.c_str() );
  	}
  	server.sin_family = AF_INET;
  	server.sin_port = htons( port );
	//   cout << "step5 " <<endl;
  	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
  	{
		//   cout << "step6 " <<endl;
    	perror("connect failed. Error");
		printf("%d\n", port); 
    	return false;
  	}
	//   cout << "step7 " <<endl;
  	return true;
}

bool TCPClient::Send(string data)
{
	// cout << "step9 " <<endl;
	if(sock != -1) 
	{
		// cout << "step10 " <<endl;
		int send_len=0;
		send_len=send(sock , data.c_str() , strlen( data.c_str() ) , 0);
		// cout << "step11 " <<endl;
		if( send_len < 0)
		{
			// cout << "step12 " <<endl;
			// cout << "Send failed : " << data << endl;
			return false;
		}
		// cout << "step13 " <<endl;
		if(send_len!=strlen( data.c_str() )){
			// cout << "step14 " <<endl;
			cerr << "check if send all? <TCPC.cpp 63>\n";
			return false;
		}
	}
	else
		return false;
	// cout << "step15 " <<endl;
	return true;
}

string TCPClient::receive(int size)
{
  	char buffer[size];
	memset(&buffer[0], 0, sizeof(buffer));

  	string reply;
	if( recv(sock , buffer , size, 0) < 0)
  	{
	    	// cout << "receive failed!" << endl;
		return nullptr;
  	}
	// timeval tv_out;
	// tv_out.tv_sec = 1;
    // tv_out.tv_usec = 0;
    // setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	// int num=0;
	// while(1) 
	// {	
	// 	num++;
	// 	if(num>5) break;
	// 	int n=0;
	// 	n=recv(sock , buffer , size, 0);
	// 	// if(errno!=0){
	// 	// 	printf("errno is: %d\n",errno);
	// 	// 	perror("11");
	// 	// 	close(sock);
	// 	// 	pthread_exit(NULL);
	// 	// }
	// 	if(n<=0) break;
	// 	cerr<<"========================================wait!!!!\n";
	// 	// usleep(1);
	// 	buffer[size-1]='\0';
	// 	reply += buffer;
	// }
	// if(errno!=0){
	// 		printf("errno is: %d\n",errno);
	// 		perror("11");
	// 		close(sock);
	// 		pthread_exit(NULL);
	// 	}
	usleep(1);
	buffer[size-1]='\0';
	reply = buffer;
  	return reply;
}


string TCPClient::read()
{
  	char buffer[1] = {};
  	string reply;
  	while (buffer[0] != '\n') {
    		if( recv(sock , buffer , sizeof(buffer) , 0) < 0)
    		{
      			// cout << "receive failed!" << endl;
			return nullptr;
    		}
		reply += buffer[0];
	}
	return reply;
}

void TCPClient::exit()
{
	// cout << "@--------------------------------------------------------socket " << sock << endl;
    close( sock );
	// cout << "@--------------------------------------------------------socket " << sock << endl;
	sock=-1;//LHZ 
}