#include "../include/core.hpp"
#include "../include/coordinate.h"
#include "../include/participant.h"
#include "../include/TCPServer.h"
#include "../include/conf.hpp"
#include "../include/parser.hpp"
#include "../include/ossSocket.hpp"
#include "../include/util.hpp"
#include "../include/global.h"
#include "../include/webserver.h"
#include "../include/HTTPconnection.h"

#include <unistd.h>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<assert.h>
#include<sys/epoll.h>
#include <getopt.h>//长选项头文件
#include <fstream>
#include<streambuf>
#include<sstream>
#include <regex>
#include <unordered_map>


unordered_map<string, course> cmap;
unordered_map<string, string> smap; 

char ip[45]="127.0.0.1";
int port = 8080;
int number_thread = 4;
char webconf[255];
char conf[255];
int isconf=0,getport=0;
const int ifprint=1;

sem_t ROsem[2];
TCPClient tcpc;
TCPServer tcp;

void get_option(int argc, char * const argv[]){
    int opt;
    int option_index = 0;
    char optstring[2];
    static struct option longopts[]{  //定义长选项
    /*
    required_argument（需要值）表示参数输入格式为：--参数 值 或者 --参数=值;
    NULL:选中某个长选项的时候，getopt_long将返回val值
    */
        {"ip", required_argument, NULL, 1}, //val值为1 
        {"port", required_argument, NULL, 2},//val值为2
        {"kvconfig_path", required_argument, NULL, 3},//val值为3 
        {"config_path", required_argument, NULL, 4},//val值为4
    };
    /*
    短选项在参数前加一杠"-"，长选项在参数前连续加两杠"--",getopt_long函数可以处理两者
    optstring: 表示短选项字符串；longopts：表示长选项结构体；option_index是longopts的下标值
    */
    while ((opt = getopt_long(argc, argv, optstring, longopts, &option_index)) != -1)
    {
        switch (opt)
        {
        case 1:
            strcpy(ip, optarg);//全局变量optarg（在头文件定义）表示当前选项对应的参数值。
            // if(isprintf2) printf("ip= %s\n",ip);
            break;
        case 2:
            port = atoi(optarg); //转整数
            getport=1;
            // if(isprintf2) printf("port = %d\n",port);
            break;
        case 3:
            // number_thread = atoi(optarg);
            strcpy(conf, optarg);
            isconf=1;
            break;
        case 4:
            strcpy(webconf, optarg);
            // isproxy=1;
            // if(isprintf2) printf("proxy = %d\n",proxy);
            break;
        default:
            break;
        }
    }
    
}

void getMsg(){
    while(1){
        tcp.accepted();
    }
}

void outMsg(){
    while(1){
        queue<descript_socket*> descT=tcp.getMessage();//从服务器的信息队列中取出信息，一次性最多只能取10条
        if(!descT.empty()){
            descript_socket* desc=descT.front();
            cout << desc->message << endl;
        }      
    }
}

bool read_webconf(string conf, string &s1addr,string &s2addr,int &s1port,int &s2port)
{
  std::fstream conf_file;
  conf_file.open(conf);
  size_t pos; 
  if (!conf_file.is_open())
  {
    return false;
  }


  // read
  int idx=1;
  while (!conf_file.eof())
  {
    std::string buf;
    
    getline(conf_file, buf);
    if(buf=="\n")idx+=1;
    else{
    if(idx==1){
        pos = buf.find(":");
        if(pos == std::string::npos) {
        return false;
        }
        else {
        s1addr = buf.substr(0, pos);
        s1port = strtol((buf.substr(pos + 1)).c_str(), nullptr, 10);//把参数 str 所指向的字符串根据给定的 base 转换为一个长整数
        }

    }
    else if(idx==6){
        pos = buf.find(":");
        if(pos == std::string::npos) {
        return false;
        }
        else {
        s2addr = buf.substr(0, pos);
        s2port = strtol((buf.substr(pos + 1)).c_str(), nullptr, 10);//把参数 str 所指向的字符串根据给定的 base 转换为一个长整数
        }
        return true;

    }
    idx+=1;
    } 
}
return false;
}

int main(int argc, char ** argv)
{
    get_option(argc,argv);
    for(int i=0;i<2;i++){
        if (sem_init(&ROsem[i],0,0)) {
            printf("[ERROR] Semaphore initialization failed!! <main 18>\n");
            exit(EXIT_FAILURE);
        }
    }
    // printf("upload data ...\n");
        FILE *fp;            /*文件指针*/
        int len;             /*行字符个数*/
        char buf[1024];
        if((fp = fopen("static/courses.txt","r")) == NULL)
        {
            perror("fail to read courses.txt");
           exit (1) ;
        }
        while(fscanf(fp,"%[^\n]",buf)!=EOF)
        { 
            fgetc(fp);//获取换行符
            string s=buf;
            std::regex r("^(.*) (.*) (.*)$");
            std::smatch subMatch;
            if(regex_match(s, subMatch, r)) {//如果匹配到：，？
            string k=subMatch[1];
            course v;
            v.name=subMatch[2];
            string v2=subMatch[3];
            v.capacity=atoi(v2.c_str());
            v.selected=0;
            cmap[k]=v;
            }
        }

        if((fp = fopen("static/students.txt","r")) == NULL)
        {
            perror("fail to read courses.txt");
           exit (1) ;
        }
        while(fscanf(fp,"%[^\n]",buf)!=EOF)
        { 
            fgetc(fp);
            string s=buf;
            std::regex r("^(.*) (.*)$");
            std::smatch subMatch;
            if(regex_match(s, subMatch, r)) {
            string k=subMatch[1],v=subMatch[2];
            smap[k]=v;
            }
        }

        // printf("upload end ...\n");
    
    if (isconf == 0 ){
        printf("web server 启动...\n");
        string cluster1_addr="127.0.0.1",cluster2_addr="127.0.0.1";
        int cluster1_port=8001,cluster2_port=8006;
        if(read_webconf(webconf,cluster1_addr,cluster2_addr,cluster1_port,cluster2_port)){
        // cout<<cluster1_addr<<" "<<cluster2_addr<<" "<<cluster1_port<<" "<<cluster2_port<<endl;    

        vector<int> opts = { SO_REUSEPORT, SO_REUSEADDR };
        tcp.setup(8010,opts);//开启TCP连接，绑定0.0.0.0，用于接受 KV服务器发送过来的回复ok/error

        WebServer server(port, 3, 6000000, false, number_thread); //开启http服务器           
        server.Start(tcp,cmap,smap,cluster1_addr,cluster2_addr,cluster1_port,cluster2_port);
        }
        else {
            printf("打开配置文件失败或配置文件格式有误\n");
        }
        return 0;
    }

    if(isconf == 1){

    printf("store server 启动...\n");
    std::vector<NodeInfo>   pinfo;//
    NodeInfo                cinfo;//初始时作为
    Mode                    m = MODE_INVALID;
    
    
    if( !readConf(conf, pinfo, cinfo, m) ) {
        std::cout << "打开配置文件失败或配置文件格式有误 " << std::endl;
        return 0;
    }

    if( m == MODE_INVALID ) {
        std::cout << "配置文件格式有误 " << std::endl;
        return 0;
    }

    Participant p;
       
    p.Init(pinfo, cinfo);
    // cerr << "ReRaft begin running...\n";
    p.Working();
        

    return 0;
    }
}
